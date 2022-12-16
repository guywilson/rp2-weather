#include <stdio.h>
#include <stddef.h>

#include "scheduler.h"
#include "schederr.h"

#include "taskdef.h"

#include "pico/stdlib.h"
#include "hardware/watchdog.h"
#include "hardware/uart.h"
#include "rtc_rp2040.h"
#include "serial_rp2040.h"
#include "heartbeat.h"
#include "watchdog.h"
#include "debug.h"
#include "led_utils.h"

void watchdog_disable(void) {
	hw_clear_bits(&watchdog_hw->ctrl, WATCHDOG_CTRL_ENABLE_BITS);
}

void setup(void) {
	/*
	** Disable the Watchdog, if we have restarted due to a
	** watchdog reset, we want to enable it when we're ready...
	*/
	watchdog_disable();

	setupLEDPin();
	setupRTC();

	setupSerial();
}

int main(void) {
	setup();

	if (watchdog_caused_reboot()) {
		turnOn(LED_ONBOARD);
		sleep_ms(2000);
		turnOff(LED_ONBOARD);
	}

	initScheduler(3);

	registerTask(TASK_HEARTBEAT, &HeartbeatTask);
	registerTask(TASK_WATCHDOG, &WatchdogTask);
	registerTask(TASK_DEBUG, &TaskDebug);

	scheduleTaskOnce(
			TASK_HEARTBEAT,
			rtc_val_ms(950),
			NULL);

	scheduleTask(
			TASK_WATCHDOG, 
			rtc_val_ms(50), 
			NULL);

	scheduleTask(
			TASK_DEBUG,
			rtc_val_sec(1),
			NULL);

	/*
	** Enable the watchdog, it will reset the device in 100ms unless
	** the watchdog timer is updated by WatchdogTask()...
	*/
	watchdog_enable(100, false);

	/*
	** Start the scheduler...
	*/
	schedule();

	return -1;
}
