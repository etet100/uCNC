/*
	Name: astrocore_sim.h
	Description: Simulator support module for the AstroCore G-Code sender.

		Provides virtual endstops, probe and control inputs for the virtual MCU,
		plus a machine state snapshot that the host can poll.

		This module is an AstroCore addition. It is not part of upstream uCNC and
		hooks into the core only through the official extension points:
		  - IO_CONDITION_* overrides (see cnc_hal_overrides.h)
		  - the cnc_io_dotasks event
		  - LOAD_MODULES_OVERRIDE

	Copyright: Copyright (c) BTS
	Date: 2026

	µCNC is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version. Please see <http://www.gnu.org/licenses/>

	µCNC is distributed WITHOUT ANY WARRANTY;
	Also without the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
	See the	GNU General Public License for more details.
*/

#ifndef ASTROCORE_SIM_H
#define ASTROCORE_SIM_H

// Kept free of cnc.h on purpose: cnc_hal_overrides.h includes this header while
// cnc.h is still being processed, so nothing from the core is available yet.
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif

// Virtual input bits.
// Written by the host (UI buttons) and by the position derived endstop logic.
// Read through the IO_CONDITION_* overrides.
#define SIM_IN_LIMIT_X 0x0001
#define SIM_IN_LIMIT_Y 0x0002
#define SIM_IN_LIMIT_Z 0x0004
#define SIM_IN_LIMIT_A 0x0008
#define SIM_IN_LIMIT_B 0x0010
#define SIM_IN_LIMIT_C 0x0020
#define SIM_IN_LIMIT_X2 0x0040
#define SIM_IN_LIMIT_Y2 0x0080
#define SIM_IN_LIMIT_Z2 0x0100
#define SIM_IN_PROBE 0x0200
#define SIM_IN_ESTOP 0x0400
#define SIM_IN_DOOR 0x0800
#define SIM_IN_FHOLD 0x1000
#define SIM_IN_CS_RES 0x2000

// Bits recomputed from the machine position on every refresh.
// Everything else is latched until the host changes it.
#define SIM_IN_AUTO_MASK (SIM_IN_LIMIT_X | SIM_IN_LIMIT_Y | SIM_IN_LIMIT_Z | SIM_IN_PROBE)

// Axis selectors for astrocore_sim_set_single_limit()
#define SIM_LIMIT_AXIS_PROBE (-2)

#define SIM_MAX_AXIS 6

// Machine status reported to the host.
// Values mirror EXEC_STATUS_* from uCNC 1.16.6 so that newer cores can pass
// cnc_get_status() straight through. Older cores get the same codes derived
// from the exec state, which keeps the host contract stable across the port.
#define SIM_STATUS_IDLE 0
#define SIM_STATUS_PROBING 1
#define SIM_STATUS_DWELL 2
#define SIM_STATUS_RUNNING 3
#define SIM_STATUS_JOGGING 4
#define SIM_STATUS_HOLD 10
#define SIM_STATUS_HOLD_PENDING 11
#define SIM_STATUS_HOLD_RESUMING 12
#define SIM_STATUS_HOMING 20
#define SIM_STATUS_DOOR_CLOSED 30
#define SIM_STATUS_DOOR_OPENED 31
#define SIM_STATUS_DOOR_OPENED_PAUSING 32
#define SIM_STATUS_DOOR_CLOSED_RESUMING 33
#define SIM_STATUS_CHECK 40
#define SIM_STATUS_LOCKED 50
#define SIM_STATUS_ALARM 60

	// Machine state snapshot published for the host on every main loop pass.
	typedef struct astrocore_sim_state_
	{
		uint32_t seq;				  // bumped on every publish
		uint32_t millis;			  // machine uptime
		uint8_t status;				  // SIM_STATUS_*
		uint16_t exec_state;		  // raw cnc_get_exec_state(EXEC_ALLACTIVE)
		uint16_t inputs;			  // SIM_IN_* snapshot
		uint8_t limits;				  // io_get_limits()
		uint8_t controls;			  // io_get_controls()
		bool probe;					  // io_get_probe()
		float position[SIM_MAX_AXIS]; // machine coordinates in mm
		float feed;					  // itp_get_rt_feed()
		uint16_t spindle;			  // tool_get_speed()
		uint32_t line;				  // current line number, 0 when not tracked
	} astrocore_sim_state_t;

	/**
	 * Host -> machine. Simulates the physical buttons and the machine setup.
	 */

	// Sets or clears any SIM_IN_* bit. Bits in SIM_IN_AUTO_MASK are overwritten
	// by the next refresh, so use the helpers below for those.
	void astrocore_sim_set_input(uint16_t mask, bool active);
	// Moves the virtual X/Y/Z endstops. Absolute, or relative to the current position.
	void astrocore_sim_set_home(bool absolute, float x, float y, float z);
	// Moves a single trip point. axis 0..2 for X/Y/Z, SIM_LIMIT_AXIS_PROBE for the probe.
	void astrocore_sim_set_single_limit(int axis, float pos);
	// Puts the probe plate right under the tool.
	void astrocore_sim_probe_at_current(void);
	// Moves the probe plate out of reach.
	void astrocore_sim_reset_probe(void);
	// Presses or releases the emergency stop button.
	void astrocore_sim_estop(bool pressed);

	/**
	 * Machine -> host.
	 */

	// Reads a virtual input. Refreshes the position derived bits when needed.
	// This is what the IO_CONDITION_* overrides call.
	uint8_t astrocore_sim_read_input(uint16_t mask);
	// Current machine position in mm.
	void astrocore_sim_get_position(float *axis);
	// Copies the latest published snapshot.
	void astrocore_sim_get_state(astrocore_sim_state_t *out);

#ifdef __cplusplus
}
#endif

#endif
