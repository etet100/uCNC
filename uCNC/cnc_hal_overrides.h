/*
	Name: cnc_hal_overrides.h
	Description: HAL override file.

		Upstream ships this file empty on purpose. AstroCore uses it to redirect
		the machine inputs to the simulator module, so that no core source file
		has to be patched.

		Included by cnc_hal_config_helper.h after the board and HAL config, and
		before io_control.c installs its own IO_CONDITION_* defaults.
*/

#ifndef CNC_HAL_OVERRIDES_H
#define CNC_HAL_OVERRIDES_H

#ifdef __cplusplus
extern "C"
{
#endif

#ifdef ENABLE_ASTROCORE_SIM

#include "src/modules/astrocore_sim.h"

	/**
	 * Route every machine input to the simulator module.
	 *
	 * io_control.c only supplies its own definition when the symbol is still
	 * undefined, so defining it here takes over the pin read while leaving the
	 * invert masks, debounce and homing masks working as usual.
	 */

#if ASSERT_PIN(LIMIT_X)
#define IO_CONDITION_LIMIT_X (astrocore_sim_read_input(SIM_IN_LIMIT_X))
#endif
#if ASSERT_PIN(LIMIT_Y)
#define IO_CONDITION_LIMIT_Y (astrocore_sim_read_input(SIM_IN_LIMIT_Y))
#endif
#if ASSERT_PIN(LIMIT_Z)
#define IO_CONDITION_LIMIT_Z (astrocore_sim_read_input(SIM_IN_LIMIT_Z))
#endif
#if ASSERT_PIN(LIMIT_A)
#define IO_CONDITION_LIMIT_A (astrocore_sim_read_input(SIM_IN_LIMIT_A))
#endif
#if ASSERT_PIN(LIMIT_B)
#define IO_CONDITION_LIMIT_B (astrocore_sim_read_input(SIM_IN_LIMIT_B))
#endif
#if ASSERT_PIN(LIMIT_C)
#define IO_CONDITION_LIMIT_C (astrocore_sim_read_input(SIM_IN_LIMIT_C))
#endif
#if ASSERT_PIN(LIMIT_X2)
#define IO_CONDITION_LIMIT_X2 (astrocore_sim_read_input(SIM_IN_LIMIT_X2))
#endif
#if ASSERT_PIN(LIMIT_Y2)
#define IO_CONDITION_LIMIT_Y2 (astrocore_sim_read_input(SIM_IN_LIMIT_Y2))
#endif
#if ASSERT_PIN(LIMIT_Z2)
#define IO_CONDITION_LIMIT_Z2 (astrocore_sim_read_input(SIM_IN_LIMIT_Z2))
#endif

#if ASSERT_PIN(PROBE)
#define IO_CONDITION_PROBE (astrocore_sim_read_input(SIM_IN_PROBE))
#endif

#if ASSERT_PIN(ESTOP)
#define IO_CONDITION_ESTOP (astrocore_sim_read_input(SIM_IN_ESTOP))
#endif
#if ASSERT_PIN(SAFETY_DOOR)
#define IO_CONDITION_SAFETY_DOOR (astrocore_sim_read_input(SIM_IN_DOOR))
#endif
#if ASSERT_PIN(FHOLD)
#define IO_CONDITION_FHOLD (astrocore_sim_read_input(SIM_IN_FHOLD))
#endif
#if ASSERT_PIN(CS_RES)
#define IO_CONDITION_CS_RES (astrocore_sim_read_input(SIM_IN_CS_RES))
#endif

	// Registers the module. Called from load_modules() in module.c.
#define LOAD_MODULES_OVERRIDE() LOAD_MODULE(astrocore_sim)

#endif

#ifdef __cplusplus
}
#endif

#endif
