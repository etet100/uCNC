# µCNC as an AstroCore simulator library

This fork turns µCNC into a Qt library that emulates a CNC controller inside a host
application. It is used by **AstroCore GCode Sender** and is meant to be reusable by
other hosts such as **AstroCore Virtual CNC**.

Base: upstream µCNC **v1.16.6** (`Paciente8159/uCNC`).

---

## 1. What this fork adds

| Area | What you get |
|------|--------------|
| Packaging | Builds as a Qt shared library (`uCNC.dll`) with a single exported entry point |
| Transport | The serial port is replaced by two `QLocalSocket` connections (data + control) |
| Virtual inputs | Endstops, probe, e-stop, safety door, feed hold and cycle start driven from the host |
| Auto endstops | X/Y/Z limit switches and the probe plate trip from the real machine position |
| Telemetry | A machine state snapshot refreshed on every main loop pass |
| Lifecycle | Cooperative shutdown so the DLL can be unloaded safely |

### Design rule: no core patches

Everything above is implemented **without modifying a single upstream core source file**.
`cnc.c`, `io_control.c`, `planner.c`, `interpolator.c`, `motion_control.c`, `atomic.h`,
`grbl_protocol.c` and `module.c` are byte-identical to upstream v1.16.6.

Verify at any time:

```bash
git diff --stat v1.16.6 HEAD -- uCNC/src/cnc.c uCNC/src/core/ uCNC/src/atomic.h \
                                uCNC/src/interface/ uCNC/src/module.c
# empty output == still clean
```

The integration rides on four official extension points:

| Hook | Where | Used for |
|------|-------|----------|
| `cnc_hal_overrides.h` | shipped empty by upstream | our `IO_CONDITION_*` overrides |
| `IO_CONDITION_*` | guarded by `!defined()` in `io_control.c` | redirect pin reads to the module |
| `LOAD_MODULES_OVERRIDE()` | `module.c` | register the module |
| `EVENT_INVOKE(cnc_io_dotasks)` | `cnc.c` | per-loop telemetry publish |

Keep it that way. If you need a new behaviour, add it to the module or to the virtual
HAL, never to a core `.c` file — that is what makes upstream merges conflict-free.

---

## 2. Architecture

```
  HOST PROCESS (your app)                 uCNC LIBRARY (worker thread or child process)
  ┌──────────────────────────┐            ┌──────────────────────────────────────────┐
  │ QLocalServer             │            │  uCNC(serverName, stopFlag)              │
  │                          │            │    └─ cnc_init(); for(;;) cnc_run();     │
  │  m_socket        ◄───────┼── data ────┼──►  WindowsSerial  ◄─► mcu_uart_*         │
  │   (Grbl bytes)           │            │                          │               │
  │                          │            │                          ▼               │
  │  m_controlSocket ◄───────┼── JSON ────┼──►  control commands   µCNC core          │
  │   (buttons/setup)        │            │           │            (unpatched)        │
  │                          │            │           ▼                ▲              │
  │  QAtomicInt stopFlag ────┼────────────┼──►  astrocore_sim module ──┘              │
  └──────────────────────────┘            │      · virtual inputs                     │
                                          │      · state snapshot                     │
                                          └──────────────────────────────────────────┘
```

Two independent channels on purpose: g-code streaming must never delay an e-stop.

---

## 3. Building

Requires **Qt 6.8+** and a GCC/Clang toolchain (llvm-mingw is what this is tested with).
MSVC is not supported — the code uses GCC atomic builtins and statement expressions.

```bash
mkdir build && cd build
qmake ../uCNC.pro
mingw32-make -j8
```

Output goes to `DESTDIR = $$OUT_PWD/../../astrocore`:

- `uCNC.dll` — the library, exports `uCNC`
- `libuCNC.a` — import library

Key settings in [`uCNC.pro`](uCNC.pro):

```pro
TEMPLATE = lib
QT += network
TARGET  = uCNC
DEFINES += MCU=MCU_VIRTUAL_WIN
win32:  DEFINES += WINDOWS=1
unix:   DEFINES += LINUX=1
```

To build a standalone console executable instead, uncomment the first lines of the
`.pro` (`TEMPLATE = app`, `CONFIG += console`).

---

## 4. Entry point and lifecycle

```cpp
extern "C" Q_DECL_EXPORT
void uCNC(QString serverName, QAtomicInt *stopFlag);
```

The call **blocks** until shutdown, so run it on a dedicated thread or in a child
process. It connects both sockets, runs `cnc_init()` and then loops `cnc_run()`.

### stopFlag protocol

The host owns the flag; the library reads it and the host observes the result.

| Value | Meaning | Written by |
|-------|---------|-----------|
| `0` Running | normal operation | host, before start |
| `2` StopRequested | please shut down | host |
| `3` Stopped | `uCNC()` has returned | host, after join |

On seeing `2` the library issues a soft reset, breaks the loop, stops the tick timer
and joins its IO thread before returning. Waiting for the return **is required** before
`FreeLibrary`/`unload()` — otherwise the timer callback keeps executing unmapped code.

### Minimal host

```cpp
QAtomicInt stopFlag(Simulator::Running);

QLibrary lib("uCNC.dll");
lib.load();
auto entry = (void (*)(QString, QAtomicInt *)) lib.resolve("uCNC");
entry(serverName, &stopFlag);   // blocks

lib.unload();
stopFlag = Simulator::Stopped;
```

> **ABI warning.** The entry point takes a `QString` by value. Host and library must be
> built with the **same Qt version and the same compiler**. If you cannot guarantee that,
> run the simulator as a separate process (see §9) or add a `const char*` entry point.

---

## 5. Transport

### Handshake

1. Host creates a `QLocalServer` and listens on a unique name
   (AstroCore uses `astrocoreucnc_{uuid}`).
2. Host starts the library, passing that name.
3. Library opens **two** connections to it, in this order:
   - **first** → data channel
   - **second** → control channel

Assign them by arrival order and reject any third connection:

```cpp
if (m_socket == nullptr) {
    m_socket = m_server->nextPendingConnection();          // data
} else if (m_controlSocket == nullptr) {
    m_controlSocket = m_server->nextPendingConnection();   // control
} else {
    m_server->nextPendingConnection()->abort();
}
```

### Data channel

Raw bytes, exactly what a real serial port would carry: you write g-code and realtime
commands, you read `ok`, `error:N`, `<Idle|MPos:...>` and so on. No framing added.

### Control channel

One compact JSON object per line, terminated by `\n`. Host → library only.

---

## 6. Control command reference

| `cmd` | Fields | Effect |
|-------|--------|--------|
| `probe_at_current` | — | Puts the probe plate exactly at the current Z. The next probing move triggers immediately. |
| `reset_probe` | — | Moves the plate to Z = −500 (out of reach). |
| `set_home` | `abs` (bool), `x`, `y`, `z` | Moves the X/Y/Z limit switch trip points. `abs=false` is relative to the current position. |
| `set_single_limit` | `axis` (int), `pos` (double) | Moves one trip point. `axis` 0/1/2 = X/Y/Z, `-2` = probe plate. |
| `estop` | `pressed` (bool, optional) | Presses or releases the e-stop. Omitted `pressed` means `true`. |
| `set_input` | `mask` (int), `active` (bool) | Sets or clears any virtual input bit. See `SIM_IN_*` in §7. |

Examples:

```json
{"cmd":"set_home","abs":true,"x":-5,"y":-5,"z":5}
{"cmd":"set_single_limit","axis":2,"pos":25}
{"cmd":"estop","pressed":true}
{"cmd":"set_input","mask":2048,"active":true}
```

Sending from the host:

```cpp
void sendControlCommand(QJsonObject cmd)
{
    QByteArray json = QJsonDocument(cmd).toJson(QJsonDocument::Compact);
    json.append("\n");
    m_controlSocket->write(json);
    m_controlSocket->flush();
}
```

Unknown commands and malformed lines are ignored silently.

---

## 7. Module API

Declared in [`uCNC/src/modules/astrocore_sim.h`](uCNC/src/modules/astrocore_sim.h).
Use this directly when you link the library statically or add your own transport.

### Virtual input bits

```c
#define SIM_IN_LIMIT_X   0x0001    #define SIM_IN_LIMIT_X2  0x0040
#define SIM_IN_LIMIT_Y   0x0002    #define SIM_IN_LIMIT_Y2  0x0080
#define SIM_IN_LIMIT_Z   0x0004    #define SIM_IN_LIMIT_Z2  0x0100
#define SIM_IN_LIMIT_A   0x0008    #define SIM_IN_PROBE     0x0200
#define SIM_IN_LIMIT_B   0x0010    #define SIM_IN_ESTOP     0x0400
#define SIM_IN_LIMIT_C   0x0020    #define SIM_IN_DOOR      0x0800
                                   #define SIM_IN_FHOLD     0x1000
                                   #define SIM_IN_CS_RES    0x2000
```

`SIM_IN_AUTO_MASK` covers `LIMIT_X | LIMIT_Y | LIMIT_Z | PROBE` — these four are
recomputed from the machine position on every refresh, so setting them by hand has no
lasting effect. Use `astrocore_sim_set_home()` / `set_single_limit()` to move their trip
points instead. All other bits are latched until you change them.

### Host → machine

```c
void astrocore_sim_set_input(uint16_t mask, bool active);
void astrocore_sim_set_home(bool absolute, float x, float y, float z);
void astrocore_sim_set_single_limit(int axis, float pos);   // 0..2, or SIM_LIMIT_AXIS_PROBE
void astrocore_sim_probe_at_current(void);
void astrocore_sim_reset_probe(void);
void astrocore_sim_estop(bool pressed);
```

### Machine → host

```c
uint8_t astrocore_sim_read_input(uint16_t mask);            // called by IO_CONDITION_*
void    astrocore_sim_get_position(float *axis);            // machine coordinates, mm
void    astrocore_sim_get_state(astrocore_sim_state_t *out);
```

### State snapshot

```c
typedef struct {
    uint32_t seq;                    // bumped on every publish
    uint32_t millis;                 // machine uptime
    uint8_t  status;                 // SIM_STATUS_*
    uint16_t exec_state;             // raw cnc_get_exec_state(EXEC_ALLACTIVE)
    uint16_t inputs;                 // SIM_IN_* snapshot
    uint8_t  limits;                 // io_get_limits()
    uint8_t  controls;               // io_get_controls()
    bool     probe;                  // io_get_probe()
    float    position[SIM_MAX_AXIS]; // machine coordinates, mm
    float    feed;                   // itp_get_rt_feed()
    uint16_t spindle;                // tool_get_speed()
    uint32_t line;                   // line number, 0 when not tracked
} astrocore_sim_state_t;
```

Refreshed once per main loop pass from the `cnc_io_dotasks` event. Poll `seq` to detect
new data.

`SIM_STATUS_*` values intentionally mirror µCNC's `EXEC_STATUS_*`
(`IDLE 0`, `PROBING 1`, `DWELL 2`, `RUNNING 3`, `JOGGING 4`, `HOLD 10`, `HOLD_PENDING 11`,
`HOLD_RESUMING 12`, `HOMING 20`, `DOOR_* 30..33`, `CHECK 40`, `LOCKED 50`, `ALARM 60`).
On cores older than 1.16.6 the module derives the same codes itself, so the host contract
does not change across upstream versions.

---

## 8. Wiring telemetry to your host

`astrocore_sim_get_state()` is populated but **nothing transmits it yet** — the current
host reads machine state by parsing Grbl `<...>` status reports off the data channel.
For a simulator you usually want the richer, faster snapshot instead.

The natural place is [`WindowsSerial`](makefiles/virtual/WindowsSerial.cpp), which
already runs on the right thread. Sketch:

```cpp
// in WindowsSerial::ReadData(), after processControlCommands()
static uint32_t lastSeq = 0;
astrocore_sim_state_t st;
astrocore_sim_get_state(&st);
if (st.seq != lastSeq) {
    lastSeq = st.seq;
    controlSocket->write(serialize(st));   // JSON line, or a packed binary frame
}
```

Notes:

- Publishing every pass is far too fast for a UI. Throttle by `st.millis` (e.g. 20 ms)
  or only send on change.
- Prefer a packed binary frame over JSON if you sample at kHz rates.
- If you would rather keep the control channel command-only, open a third socket; the
  host's `onNewConnection()` assigns by arrival order, so add a third branch.

---

## 9. Running as a separate process

The host base class supports a `VIRTUAL_SIMULATOR_PROCESS` build mode that launches an
executable instead of loading a DLL. The exe receives two arguments:

1. `serverName` — the `QLocalServer` name to connect to
2. `simulatorType` — `"grbl" | "fluidnc" | "ucnc"`

It then connects back with the same two sockets. This sidesteps the Qt ABI constraint
and keeps a simulator crash from taking the host down — recommended for AstroCore
Virtual CNC unless you specifically need in-process speed.

---

## 10. Configuration

### Enabling the simulator support

[`uCNC/cnc_config.h`](uCNC/cnc_config.h):

```c
#define ENABLE_ASTROCORE_SIM        // the module itself
#define ENABLE_MAIN_LOOP_MODULES    // required: provides the cnc_io_dotasks event
```

`ENABLE_ASTROCORE_SIM` without `ENABLE_MAIN_LOOP_MODULES` fails at compile time on purpose.

### Machine defaults used by the simulator

```c
#define BOARD_NAME "uCNC Simulator"
#define DISABLE_MULTISTREAM_SERIAL
#define DEFAULT_HARD_LIMITS_ENABLED 1
#define DEFAULT_HOMING_ENABLED 1
#define DEFAULT_HOMING_DIR_INV_MASK 4       // Z homes to max, X/Y to min
#define DEFAULT_HOMING_FAST 510
#define DEFAULT_HOMING_SLOW 50
#define DEFAULT_HOMING_OFFSET 1
#define DEFAULT_MAX_DIST_PER_AXIS {150, 150, 20}
#define DEFAULT_DEBOUNCE_MS 50
#define ALLOW_SOFT_LIMIT_JOG_MOTION_CLAMPING
#define FORCE_SOFT_POLLING                  // inputs polled from the main loop
#define EMULATE_GRBL_STARTUP 3              // Grbl-like e-stop behaviour
```

`EMULATE_GRBL_STARTUP 3` matters for the e-stop: a rising edge issues a soft reset
rather than latching a µCNC shutdown alarm.

### Input routing

[`uCNC/cnc_hal_overrides.h`](uCNC/cnc_hal_overrides.h) points every machine input at the
module:

```c
#define IO_CONDITION_LIMIT_X (astrocore_sim_read_input(SIM_IN_LIMIT_X))
#define IO_CONDITION_PROBE   (astrocore_sim_read_input(SIM_IN_PROBE))
#define IO_CONDITION_ESTOP   (astrocore_sim_read_input(SIM_IN_ESTOP))
/* ... and the rest */
```

Because this replaces only the *pin read*, everything downstream still applies: the `$5`
invert mask, debounce, homing masks and `LIMITS_NORMAL_OPERATION_MASK` all behave as on
real hardware.

### Virtual MCU

[`mcumap_virtual.h`](uCNC/src/hal/mcus/virtual/mcumap_virtual.h):

```c
#define MCU_HAS_UART                 // data channel
// #define MCU_HAS_UART2             // console, off by default
#define ATOMIC_TYPE uint8_t          // see §12
#define EMULATION_MS_TICK 100        // virtual ms per tick
```

---

## 11. Threading and timing

**Everything runs on one thread.** The Windows timer-pool callback (and its Linux
`SIGEV_THREAD` counterpart) only increments a counter:

```
timer thread:   queue_tick()                        → pending_ticks++
main loop:      mcu_dotasks() → mcu_run_pending_ticks() → ticksimul() → step/RTC callbacks
```

This is why no locking is needed anywhere and why the core sources stay unpatched. It
works because every blocking wait in µCNC pumps `cnc_dotasks()`, which reaches
`mcu_dotasks()` — so virtual time keeps advancing during syncs, homing and probing.

Two guards:

| Macro | Default | Purpose |
|-------|---------|---------|
| `EMULATION_MAX_CATCHUP_TICKS` | 4 | max ticks run per `mcu_dotasks()` call |
| `EMULATION_MAX_PENDING_TICKS` | 32 | queue depth; beyond it ticks are dropped |

**If you call into the library, do it from the same thread that runs `uCNC()`.** The
module API has no internal locking by design. Control commands arrive correctly because
they are read inside `mcu_dotasks()`.

### Simulation speed

`EMULATION_MS_TICK` is 100 while the timer fires every 20 ms, so **the machine runs about
5× faster than the wall clock**. Adjust deliberately:

| Goal | Setting |
|------|---------|
| Real time | `EMULATION_MS_TICK 20` with the 20 ms timer |
| 5× faster (current) | `EMULATION_MS_TICK 100` |
| Slower than real time | lower `EMULATION_MS_TICK`, or raise the `start_timer()` period |

The timer period is set in `mcu_init()`: `start_timer(20, &ticksimul)`.

---

## 12. Staying in sync with upstream

```bash
git fetch base --tags
git merge v1.16.7        # pin to a tag, not to a moving branch
```

Only these files can ever conflict, because they are the only ones we touch:

| File | Conflict risk |
|------|---------------|
| `uCNC/cnc_config.h` | moderate — upstream edits it regularly |
| `uCNC/cnc_hal_overrides.h` | none so far — upstream ships it empty |
| `uCNC/src/hal/mcus/virtual/mcumap_virtual.h` | low — rarely touched |
| `makefiles/virtual/*` | low |
| `uCNC/src/modules/astrocore_sim.*` | none — upstream does not know these files |

Check before merging:

```bash
git merge-tree --write-tree --name-only HEAD <upstream-tag> | grep -i conflict
```

After merging, re-run the "core is clean" check from §1 and rebuild.

### What broke last time

Upstream v1.16.6 changed `cnc_get_exec_state()` from `uint8_t` to `uint16_t` and
renumbered every `EXEC_*` bit. Anything that stores or compares raw exec-state values
needs a review on each major bump. The module already stores it as `uint16_t`.

---

## 13. Known gaps

- **Telemetry is not transmitted.** `astrocore_sim_get_state()` is filled in but nothing
  sends it. See §8.
- **`Dwell` and `Probe` states.** Since v1.16.6 µCNC reports these two Grbl states.
  A host whose state dictionary predates that will map them to `Unknown`. Make sure your
  parser knows them.
- **Runtime is not yet validated.** The library builds, links and registers its hooks;
  the single-threaded tick model has not been exercised on a running machine. Homing and
  `G38.x` probing are the paths to test first.
- **Dead code.** The named-pipe `ioserver` thread and the `virtualmap` input half are
  leftovers from upstream's emulator and do nothing. The output half of `virtualmap` is
  live and holds step/dir/PWM state — useful if you want per-step output.
- **MSVC is unsupported.** GCC atomic builtins and statement expressions are used
  throughout.

---

## 14. File map

| Path | Role |
|------|------|
| `uCNC/src/modules/astrocore_sim.{c,h}` | the simulator module — inputs, telemetry |
| `uCNC/cnc_hal_overrides.h` | `IO_CONDITION_*` routing + module registration |
| `uCNC/cnc_config.h` | feature flags and machine defaults |
| `makefiles/virtual/mcu_virtual.cpp` | virtual MCU HAL, tick pump, Qt event loop |
| `makefiles/virtual/WindowsSerial.{cpp,h}` | socket transport + control command parser |
| `uCNC/src/hal/mcus/virtual/mcumap_virtual.h` | virtual pin map and atomics config |
| `uCNC.pro` | qmake project |
