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

#define I2C_SDA_ALT_PIN				16
#define I2C_SLK_ALT_PIN				17

#define SPI0_CSEL_PIN				22
#define PWM_PIN                     15


void taskPWM(PTASKPARM p) {
    static int          state = 0;
    rtc_t               delay;

    if (state) {
        gpio_put(PWM_PIN, false);
        state = 0;
        delay = rtc_val_ms(49);
    }
    else {
        gpio_put(PWM_PIN, true);
        state = 1;
        delay = rtc_val_ms(1);
    }

    scheduleTask(TASK_PWM, delay, false, NULL);
}

void setup(void) {
    uint16_t *          otp;

	/*
	** Disable the Watchdog, if we have restarted due to a
	** watchdog reset, we want to enable it when we're ready...
	*/
	watchdog_disable();

	setupLEDPin();
	setupRTC();
	setupSerial();

	i2cInit(i2c0, 400000);

    gpio_set_function(I2C_SDA_ALT_PIN, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SLK_ALT_PIN, GPIO_FUNC_I2C);
    
    gpio_init(PWM_PIN);
    gpio_set_dir(PWM_PIN, GPIO_OUT);

    lgOpen(uart0, LOG_LEVEL_FATAL | LOG_LEVEL_ERROR | LOG_LEVEL_STATUS);

	if (initSensors(i2c0)) {
		lgLogError("ERR: Sensor init");
		exit(-1);
	}

    adcInit();
    pioInit();

    otp = getOTPValues();

    lgLogStatus("OTP: 0x%04X, 0x%04X, 0x%04X, 0x%04X", otp[0], otp[1], otp[2], otp[3]);

	spi_init(spi0, 5000000);

	nRF24L01_setup(spi0);
}

int main(void) {
	setup();

	if (watchdog_caused_reboot()) {
		turnOn(LED_ONBOARD);
		sleep_ms(2000);
		turnOff(LED_ONBOARD);
	}

	initScheduler(9);

	registerTask(TASK_HEARTBEAT, &HeartbeatTask);
	registerTask(TASK_WATCHDOG, &WatchdogTask);
	registerTask(TASK_I2C_SENSOR, &taskI2CSensor);
    registerTask(TASK_I2C_READ, &taskI2CRead);
    registerTask(TASK_I2C_WRITE, &taskI2CWrite);
    registerTask(TASK_ADC, &taskADC);
    registerTask(TASK_ANEMOMETER, &taskAnemometer);
    registerTask(TASK_RAIN_GAUGE, &taskRainGuage);
//    registerTask(TASK_BATTERY_MONITOR, &taskBatteryMonitor);
    registerTask(TASK_PWM, &taskPWM);

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
			rtc_val_ms(4000), 
            false, 
			NULL);

    scheduleTask(
            TASK_ANEMOMETER,
            rtc_val_ms(100),
            true,
            NULL);

    scheduleTask(
            TASK_RAIN_GAUGE,
            rtc_val_sec(200),
            true,
            NULL);

    // scheduleTask(
    //         TASK_BATTERY_MONITOR,
    //         rtc_val_sec(10),
    //         true,
    //         NULL);

	scheduleTask(
			TASK_WATCHDOG, 
			rtc_val_ms(50), 
            true, 
			NULL);

    /*
    ** Use the GPIO to mimic pulses from the anemometer...
    */
    scheduleTask(
            TASK_PWM, 
            rtc_val_ms(45), 
            false, 
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
