#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

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
#include "adc_rp2040.h"
#include "pio_rp2040.h"
#include "pwm_rp2040.h"
#include "heartbeat.h"
#include "battery.h"
#include "watchdog.h"
#include "logger.h"
#include "sensor.h"
#include "icp10125.h"
#include "nRF24L01.h"
#include "utils.h"
#include "gpio_def.h"

void setup(void) {
	/*
	** Disable the Watchdog, if we have restarted due to a
	** watchdog reset, we want to enable it when we're ready...
	*/
	watchdog_disable();

	setupLEDPin();
	setupRTC();
	setupSerial();

    gpio_set_function(I2C_SDA_ALT_PIN, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SLK_ALT_PIN, GPIO_FUNC_I2C);
    
//    lgOpen(uart0, LOG_LEVEL_FATAL | LOG_LEVEL_ERROR | LOG_LEVEL_STATUS | LOG_LEVEL_DEBUG | LOG_LEVEL_INFO);
    lgOpen(uart0, LOG_LEVEL_OFF);

    adcInit();
    pioInit();

	spi_init(spi0, 5000000);

	nRF24L01_setup(spi0);
}

int main(void) {
    bool            isWatchdogReboot = false;

	setup();

	if (watchdog_caused_reboot()) {
        isWatchdogReboot = true;
	}

	initScheduler(8);

	registerTask(TASK_HEARTBEAT, &HeartbeatTask);
	registerTask(TASK_WATCHDOG, &taskWatchdog);
	registerTask(TASK_WATCHDOG_WAKEUP, &taskWatchdogWakeUp);
	registerTask(TASK_I2C_SENSOR, &taskI2CSensor);
    registerTask(TASK_ADC, &taskADC);
    registerTask(TASK_ANEMOMETER, &taskAnemometer);
    registerTask(TASK_RAIN_GAUGE, &taskRainGuage);
    registerTask(TASK_BATTERY_MONITOR, &taskBatteryMonitor);

	scheduleTask(
			TASK_HEARTBEAT,
			rtc_val_ms(950),
            false,
			NULL);

	/*
	** Start the sensor chain...
	*/
	scheduleTask(
			TASK_I2C_SENSOR, 
			rtc_val_ms(5000), 
            false, 
			NULL);

	scheduleTask(
			TASK_ADC, 
			rtc_val_ms(2000), 
            false, 
			NULL);

    scheduleTask(
            TASK_ANEMOMETER,
            rtc_val_ms(100),
            true,
            NULL);

    scheduleTask(
            TASK_RAIN_GAUGE,
            rtc_val_sec(30),
            true,
            NULL);

    scheduleTask(
            TASK_BATTERY_MONITOR,
            rtc_val_min(5),
            false,
            NULL);

    setTaskAttributes(TASK_BATTERY_MONITOR, true);

	scheduleTask(
			TASK_WATCHDOG, 
			rtc_val_ms(50), 
            true, 
			NULL);

    if (isWatchdogReboot) {
        scheduleTask(
            TASK_WATCHDOG_WAKEUP, 
            rtc_val_sec(3), 
            false, 
            NULL);
    }

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
