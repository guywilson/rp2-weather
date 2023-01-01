#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>

#include "scheduler.h"
#include "schederr.h"

#include "taskdef.h"

#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "hardware/watchdog.h"
#include "hardware/uart.h"
#include "hardware/i2c.h"
#include "rtc_rp2040.h"
#include "serial_rp2040.h"
#include "i2c_rp2040.h"
#include "heartbeat.h"
#include "watchdog.h"
#include "debug.h"
#include "i2ctask.h"
#include "led_utils.h"

#define I2C_SDA_ALT_PIN				16
#define I2C_SLK_ALT_PIN				17

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

	i2c_init(i2c0, 400000);

    gpio_set_function(I2C_SDA_ALT_PIN, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SLK_ALT_PIN, GPIO_FUNC_I2C);

	if (setupTMP117(i2c0)) {
		uart_puts(uart0, "ERR TMP117\n");
		exit(-1);
	}
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
	registerTask(TASK_TEMP_READ, &TaskReadTemp);

	scheduleTaskOnce(
			TASK_HEARTBEAT,
			rtc_val_ms(950),
			NULL);

	scheduleTask(
			TASK_TEMP_READ, 
			rtc_val_ms(4150), 
			NULL);

	scheduleTask(
			TASK_WATCHDOG, 
			rtc_val_ms(50), 
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
