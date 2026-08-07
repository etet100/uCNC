/*
	Name: astrocore_sim.c
	Description: Simulator support module for the AstroCore G-Code sender.

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

#include "../cnc.h"
#include "astrocore_sim.h"
#include <string.h>

#ifdef ENABLE_ASTROCORE_SIM

#ifndef ENABLE_MAIN_LOOP_MODULES
#error "ENABLE_ASTROCORE_SIM requires ENABLE_MAIN_LOOP_MODULES"
#endif

#if (AXIS_COUNT > SIM_MAX_AXIS)
#error "SIM_MAX_AXIS is too small for this AXIS_COUNT"
#endif

// Trip points of the virtual endstops, in machine coordinates.
// X and Y trip on the min side, Z on the max side. This matches a typical router.
static float sim_limit_pos[3] = {-5.0f, -5.0f, 5.0f};
// Z height of the virtual probe plate. Far below the table means "no plate".
static float sim_probe_z = -500.0f;

static volatile uint16_t sim_inputs;
static astrocore_sim_state_t sim_state;

void astrocore_sim_get_position(float *axis)
{
	int32_t steppos[STEPPER_COUNT];

	itp_get_rt_position(steppos);
	kinematics_steps_to_coordinates(steppos, axis);
}

// Recomputes only the position derived bits. Bits the host owns (estop, door,
// feed hold, cycle start) are left untouched.
static void sim_refresh_position_inputs(void)
{
	float pos[SIM_MAX_AXIS] = {0};
	uint16_t in = sim_inputs & ~SIM_IN_AUTO_MASK;

	astrocore_sim_get_position(pos);

	if (pos[0] <= sim_limit_pos[0])
	{
		in |= SIM_IN_LIMIT_X;
	}
	if (pos[1] <= sim_limit_pos[1])
	{
		in |= SIM_IN_LIMIT_Y;
	}
	if (pos[2] >= sim_limit_pos[2])
	{
		in |= SIM_IN_LIMIT_Z;
	}
	if (pos[2] <= sim_probe_z)
	{
		in |= SIM_IN_PROBE;
	}

	sim_inputs = in;
}

uint8_t astrocore_sim_read_input(uint16_t mask)
{
	// io_get_raw_limits() and friends query several pins in a row. Refresh once
	// per simulated tick so all of them see the same position.
	static uint32_t last_tick;
	static bool refreshed;
	uint32_t now = mcu_micros();

	if (!refreshed || now != last_tick)
	{
		last_tick = now;
		refreshed = true;
		sim_refresh_position_inputs();
	}

	return (sim_inputs & mask) ? 1 : 0;
}

void astrocore_sim_set_input(uint16_t mask, bool active)
{
	if (active)
	{
		sim_inputs |= mask;
	}
	else
	{
		sim_inputs &= ~mask;
	}
}

void astrocore_sim_set_home(bool absolute, float x, float y, float z)
{
	if (absolute)
	{
		sim_limit_pos[0] = x;
		sim_limit_pos[1] = y;
		sim_limit_pos[2] = z;

		return;
	}

	float pos[SIM_MAX_AXIS] = {0};
	astrocore_sim_get_position(pos);

	sim_limit_pos[0] = pos[0] + x;
	sim_limit_pos[1] = pos[1] + y;
	sim_limit_pos[2] = pos[2] + z;
}

void astrocore_sim_set_single_limit(int axis, float pos)
{
	if (axis == SIM_LIMIT_AXIS_PROBE)
	{
		sim_probe_z = pos;
	}
	else if (axis >= 0 && axis < 3)
	{
		sim_limit_pos[axis] = pos;
	}
}

void astrocore_sim_probe_at_current(void)
{
	float pos[SIM_MAX_AXIS] = {0};

	astrocore_sim_get_position(pos);
	sim_probe_z = pos[2];
}

void astrocore_sim_reset_probe(void)
{
	sim_probe_z = -500.0f;
}

void astrocore_sim_estop(bool pressed)
{
	astrocore_sim_set_input(SIM_IN_ESTOP, pressed);
}

void astrocore_sim_get_state(astrocore_sim_state_t *out)
{
	memcpy(out, &sim_state, sizeof(astrocore_sim_state_t));
}

// EXEC_DWELL, cnc_get_status() and the 16 bit exec state all arrived together
// in uCNC 1.16.6, so EXEC_DWELL works as the feature probe.
#ifdef EXEC_DWELL
#define sim_get_status() cnc_get_status()
#else
// Same status codes derived from the pre 1.16.6 exec state.
static uint8_t sim_get_status(void)
{
	uint8_t state = cnc_get_exec_state(EXEC_ALLACTIVE);

	if (cnc_has_alarm())
	{
		return SIM_STATUS_ALARM;
	}
	if (mc_get_checkmode())
	{
		return SIM_STATUS_CHECK;
	}
	if (state & EXEC_DOOR)
	{
		return SIM_STATUS_DOOR_OPENED;
	}
	if (state & EXEC_LIMITS)
	{
		return (state & EXEC_HOMING) ? SIM_STATUS_HOMING : SIM_STATUS_ALARM;
	}
	if (state & EXEC_HOMING)
	{
		return SIM_STATUS_HOMING;
	}
	if (state & EXEC_HOLD)
	{
		return (state & EXEC_RUN) ? SIM_STATUS_HOLD_PENDING : SIM_STATUS_HOLD;
	}
	if (state & EXEC_JOG)
	{
		return SIM_STATUS_JOGGING;
	}
	if (state & EXEC_RUN)
	{
		return SIM_STATUS_RUNNING;
	}
	if (state & EXEC_POSITION_MAYBE_LOST)
	{
		return SIM_STATUS_LOCKED;
	}

	return SIM_STATUS_IDLE;
}
#endif

static void sim_publish_state(void)
{
	float pos[SIM_MAX_AXIS] = {0};

	astrocore_sim_get_position(pos);
	memcpy(sim_state.position, pos, sizeof(pos));

	sim_state.millis = mcu_millis();
	sim_state.status = sim_get_status();
	sim_state.exec_state = cnc_get_exec_state(EXEC_ALLACTIVE);
	sim_state.inputs = sim_inputs;
	sim_state.limits = io_get_limits();
	sim_state.controls = io_get_controls();
	sim_state.probe = io_get_probe();
	sim_state.feed = itp_get_rt_feed();
	sim_state.spindle = tool_get_speed();
#ifdef GCODE_PROCESS_LINE_NUMBERS
	sim_state.line = itp_get_rt_line_number();
#else
	sim_state.line = 0;
#endif
	sim_state.seq++;
}

static bool astrocore_sim_dotasks(void *args)
{
	sim_publish_state();

	return EVENT_CONTINUE;
}

CREATE_EVENT_LISTENER(cnc_io_dotasks, astrocore_sim_dotasks);

DECL_MODULE(astrocore_sim)
{
	sim_inputs = 0;
	memset(&sim_state, 0, sizeof(sim_state));

	ADD_EVENT_LISTENER(cnc_io_dotasks, astrocore_sim_dotasks);
}

#endif
