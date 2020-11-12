#ifndef APP_INCLUDE_APP_SYSTEM_OFF_HPP
#define APP_INCLUDE_APP_SYSTEM_OFF_HPP

#include <zephyr.h>
#include <device.h>
#include <init.h>
#include <power/power.h>
#include <hal/nrf_gpio.h>

#define CONSOLE_LABEL DT_LABEL(DT_CHOSEN(zephyr_console))

/* Prevent deep sleep (system off) from being entered on long timeouts
 * or `K_FOREVER` due to the default residency policy.
 *
 * This has to be done before anything tries to sleep, which means
 * before the threading system starts up between PRE_KERNEL_2 and
 * POST_KERNEL.  Do it at the start of PRE_KERNEL_2.
 */
static int disable_ds_1(const struct device *dev)
{
	ARG_UNUSED(dev);

	sys_pm_ctrl_disable_state(SYS_POWER_STATE_DEEP_SLEEP_1);
	return 0;
}

SYS_INIT(disable_ds_1, PRE_KERNEL_2, 0);

int power_off() {
	LOG_WRN("Press the gpio button to wakeup");

	/* Above we disabled entry to deep sleep based on duration of
	 * controlled delay.  Here we need to override that, then
	 * force entry to deep sleep on any delay.
	 */
	sys_pm_force_power_state(SYS_POWER_STATE_DEEP_SLEEP_1);
	k_sleep(K_MSEC(1));

    LOG_ERR("System off failed\n");
    return 1;
}

#endif