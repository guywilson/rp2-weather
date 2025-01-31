#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "pico/multicore.h"
#include "hardware/adc.h"
#include "hardware/rtc.h"
#include "hardware/i2c.h"
#include "hardware/spi.h"
#include "hardware/watchdog.h"
#include "hardware/gpio.h"
#include "rtc_rp2040.h"
#include "i2c_rp2040.h"
#include "pio_rp2040.h"
#include "scheduler.h"
#include "taskdef.h"
#include "packet.h"
#include "sensor.h"
#include "watchdog.h"
#include "nRF24L01.h"
#include "battery.h"
#include "gpio_def.h"

#define STATE_RADIO_POWER_UP                0x0100
#define STATE_RADIO_SEND_PACKET             0x0200
#define STATE_RADIO_FINISH                  0x0300
#define STATE_SLEEP                         0xFF00

#define SLEEP_PERIOD_OFF                    0
#define SLEEP_PERIOD_15H                   15
#define SLEEP_PERIOD_24H                   24
#define SLEEP_PERIOD_72H                   72

static datetime_t           dt;
static datetime_t           alarm_dt;

void wakeUp(void) {
	/*
	** Enable the watchdog, it will reset the device in 100ms...
	*/
	watchdog_enable(100, false);
}

void taskBatteryMonitor(PTASKPARM p) {
    static int                  state = STATE_RADIO_POWER_UP;
    static int                  runCount = 0;
    static int                  sleepPeriod = SLEEP_PERIOD_OFF;
    static uint16_t             lastBatteryPct = 0;
    uint8_t                     buffer[32];
    rtc_t                       delay = rtc_val_sec(10);
    sleep_packet_t *            pSleep;
    weather_packet_t *          pWeather;
    int                         i;
    
    pWeather = getWeatherPacket();
    pSleep = getSleepPacket();

    /* 
    ** If the battery percentage has dropped below critical,
    ** stop everything and put the RP2040 to sleep...
    */
    if (runCount > 6) {
        if (pWeather->rawBatteryPercentage < BATTERY_PERCENTAGE_CRITICAL && lastBatteryPct < BATTERY_PERCENTAGE_CRITICAL) {
            sleepPeriod = SLEEP_PERIOD_72H;
        }
        else if (pWeather->rawBatteryPercentage < BATTERY_PERCENTAGE_VLOW && lastBatteryPct < BATTERY_PERCENTAGE_VLOW) {
            /*
            ** Sleep for 24 hours to preserve the battery overnight
            */
            sleepPeriod = SLEEP_PERIOD_24H;
        }
        else if (pWeather->rawBatteryPercentage < BATTERY_PERCENTAGE_LOW && lastBatteryPct < BATTERY_PERCENTAGE_LOW) {
            /*
            ** Sleep for 15 hours to preserve the battery overnight
            */
            sleepPeriod = SLEEP_PERIOD_15H;
        }
        else {
            sleepPeriod = SLEEP_PERIOD_OFF;
        }
    }

    lastBatteryPct = pWeather->rawBatteryPercentage;
    
    if (sleepPeriod != SLEEP_PERIOD_OFF) {
        /*
        ** Steps we need to take:
        **
        ** 1. Send a final message back to the RPi base station
        ** 2. Disable the watchdog
        ** 3. Stop the scheduler (by stopping the scheduler tick timer)
        ** 4. Disable the ADC, I2c, SPI & PIO
        ** 5. Turn off all GPIO pins (including onboard LED)
        ** 6. Setup the RTC and set a timer for the sleep period
        ** 7. Go to sleep zzzzzzzz
        */
        switch (state) {
            case STATE_RADIO_POWER_UP:
                nRF24L01_powerUpTx(spi0);

                state = STATE_RADIO_SEND_PACKET;
                delay = rtc_val_ms(150);
                break;

            case STATE_RADIO_SEND_PACKET:
                pSleep->rawBatteryVolts = pWeather->rawBatteryVolts;
                pSleep->sleepHours = 72;

                memcpy(buffer, pSleep, sizeof(sleep_packet_t));
                nRF24L01_transmit_buffer(spi0, buffer, sizeof(sleep_packet_t), false);
                
                state = STATE_RADIO_FINISH;
                delay = rtc_val_ms(125);
                break;

            case STATE_RADIO_FINISH:
                nRF24L01_powerDown(spi0);

                state = STATE_SLEEP;
                delay = rtc_val_ms(10);
                break;

            case STATE_SLEEP:
                watchdog_disable();
                disableRTC();
                
                multicore_reset_core1();
                
                i2c0->hw->enable = 0;
                i2c1->hw->enable = 0;

                hw_clear_bits(&spi0_hw->cr1, SPI_SSPCR1_SSE_BITS);
                hw_clear_bits(&spi1_hw->cr1, SPI_SSPCR1_SSE_BITS);

                disablePIO();

                /*
                ** Claim all GPIOs as outputs and drive them all low...
                */
                for (i = 0;i < 29;i++) {
                    gpio_set_function((uint)i, GPIO_FUNC_SIO);
                }

                gpio_set_dir_out_masked(0x3FFFFFFF);
                gpio_clr_mask(0x3FFFFFFF);

                /*
                ** Set the date as midnight 1st Jan 2020...
                */
                dt.year = 2020;
                dt.month = 1;
                dt.day = 1;
                dt.dotw = 0;
                dt.hour = 0;
                dt.min = 0;
                dt.sec = 0;

                /*
                ** Set an alarm...
                */
                alarm_dt.year = -1;
                alarm_dt.month = -1;
                alarm_dt.dotw = -1;
                alarm_dt.min = -1;
                alarm_dt.sec = -1;

                if (sleepPeriod == SLEEP_PERIOD_72H) {
                    alarm_dt.day = 4;
                    alarm_dt.hour = -1;
                }
                else if (sleepPeriod == SLEEP_PERIOD_24H) {
                    alarm_dt.day = 2;
                    alarm_dt.hour = -1;
                }
                else if (sleepPeriod == SLEEP_PERIOD_15H) {
                    alarm_dt.day = -1;
                    alarm_dt.hour = 15;
                }

                rtc_init();
                rtc_set_datetime(&dt);
                rtc_set_alarm(&alarm_dt, wakeUp);

                /*
                ** Now we can go to sleep zzzzzzzz
                */
                __wfi();
                break;
        }
    }

    scheduleTask(TASK_BATTERY_MONITOR, delay, false, NULL);

    runCount++;
}
