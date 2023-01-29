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
#include "hardware/spi.h"
#include "rtc_rp2040.h"
#include "serial_rp2040.h"
#include "i2c_rp2040.h"
#include "heartbeat.h"
#include "watchdog.h"
#include "debug.h"
#include "sensor.h"
#include "nRF24L01.h"
#include "led_utils.h"

#define I2C_SDA_ALT_PIN				16
#define I2C_SLK_ALT_PIN				17

#define SPI0_CSEL_PIN				22

sensor_chain_t			sensors[3];

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

	/*
	** Allow us to plug in the USB cable to
	** debug setup...
	*/
	sleep_ms(5000);

	i2c_init(i2c0, 400000);

    gpio_set_function(I2C_SDA_ALT_PIN, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SLK_ALT_PIN, GPIO_FUNC_I2C);

	if (tmp117_setup(i2c0)) {
		uart_puts(uart0, "ERR TMP117\n");
		exit(-1);
	}

	spi_init(spi0, 5000000);

	nRF24L01_setup(spi0);
}

int main(void) {
	sensor_chain_t *		sensor;

	setup();

	if (watchdog_caused_reboot()) {
		turnOn(LED_ONBOARD);
		sleep_ms(2000);
		turnOff(LED_ONBOARD);
	}

	initScheduler(5);

	registerTask(TASK_HEARTBEAT, &HeartbeatTask);
	registerTask(TASK_WATCHDOG, &WatchdogTask);
	registerTask(TASK_READ_TEMP, &taskReadTemp);
	registerTask(TASK_READ_HUMIDITY, &taskReadHumidity);
	registerTask(TASK_READ_PRESSURE, &taskReadPressure);

	scheduleTaskOnce(
			TASK_HEARTBEAT,
			rtc_val_ms(950),
			NULL);

	/*
	** Setup the sensor chain...
	*/
	sensors[0].taskID = TASK_READ_TEMP;
	sensors[0].next = &sensors[1];

	sensors[1].taskID = TASK_READ_HUMIDITY;
	sensors[1].next = &sensors[2];

	sensors[2].taskID = TASK_READ_PRESSURE;
	sensors[3].next = &sensors[0];

	sensor = &sensors[0];

	/*
	** Start the sensor chain...
	*/
	scheduleTaskOnce(
			sensor->taskID, 
			rtc_val_ms(4000), 
			sensor);

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
