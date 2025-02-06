#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "hardware/adc.h"
#include "hardware/rtc.h"
#include "hardware/i2c.h"
#include "hardware/spi.h"
#include "hardware/watchdog.h"
#include "hardware/clocks.h"
#include "hardware/regs/m0plus.h"
#include "hardware/structs/sio.h"
#include "hardware/structs/scb.h"
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

#define STATE_START                         0x0001
#define STATE_RADIO_POWER_UP                0x0100
#define STATE_RADIO_SEND_PACKET             0x0200
#define STATE_RADIO_FINISH                  0x0300
#define STATE_SLEEP                         0xFF00

#define SLEEP_PERIOD_OFF                    0
#define SLEEP_PERIOD_1H                     1
#define SLEEP_PERIOD_15H                   15
#define SLEEP_PERIOD_24H                   24
#define SLEEP_PERIOD_72H                   72

#define DEBUG_SLEEP

static datetime_t           dt;
static datetime_t           alarm_dt;

void wakeUp(void) {
	/*
	** Enable the watchdog, it will reset the device in 100ms...
	*/
	watchdog_enable(100, false);
}

void taskBatteryMonitor(PTASKPARM p) {
    static int                  state = STATE_START;
    static int                  runCount = 0;
    static int                  sleepPeriod = SLEEP_PERIOD_OFF;
    uint8_t                     buffer[32];
    rtc_t                       delay = rtc_val_sec(10);
    sleep_packet_t *            pSleep;
    weather_packet_t *          pWeather;
    
    pWeather = getWeatherPacket();
    pSleep = getSleepPacket();

#ifndef DEBUG_SLEEP
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
#else
    sleepPeriod = SLEEP_PERIOD_1H;
#endif

    if (sleepPeriod) {
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
            case STATE_START:
                watchdog_disable();

                spi_init(spi0, 5000000);

                state = STATE_RADIO_POWER_UP;
                delay = rtc_val_ms(200);
                break;

            case STATE_RADIO_POWER_UP:
                gpio_init(SCOPE_DEBUG_PIN_0);
                gpio_init(SCOPE_DEBUG_PIN_1);
                gpio_init(SCOPE_DEBUG_PIN_2);

                gpio_set_dir(SCOPE_DEBUG_PIN_0, true);
                gpio_set_dir(SCOPE_DEBUG_PIN_1, true);
                gpio_set_dir(SCOPE_DEBUG_PIN_2, true);

                gpio_put(SCOPE_DEBUG_PIN_0, 1);
                nRF24L01_powerUpTx(spi0);
                gpio_put(SCOPE_DEBUG_PIN_0, 0);

                state = STATE_RADIO_SEND_PACKET;
                delay = rtc_val_ms(200);
                break;

            case STATE_RADIO_SEND_PACKET:
                gpio_put(SCOPE_DEBUG_PIN_1, 1);
                pSleep->rawBatteryVolts = pWeather->rawBatteryVolts;
                pSleep->sleepHours = (uint16_t)sleepPeriod;

                memcpy(buffer, pSleep, sizeof(sleep_packet_t));
                nRF24L01_transmit_buffer(spi0, buffer, sizeof(sleep_packet_t), false);
                gpio_put(SCOPE_DEBUG_PIN_1, 0);
                
                state = STATE_RADIO_FINISH;
                delay = rtc_val_ms(200);
                break;

            case STATE_RADIO_FINISH:
                gpio_put(SCOPE_DEBUG_PIN_2, 1);
                nRF24L01_powerDown(spi0);
                gpio_put(SCOPE_DEBUG_PIN_2, 0);

                state = STATE_SLEEP;
                delay = rtc_val_ms(100);
                break;

            case STATE_SLEEP:
                disableRTC();

                i2c_deinit(i2c0);
                spi_deinit(spi0);
                uart_deinit(uart0);

                disablePIO();

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
                else if (sleepPeriod == SLEEP_PERIOD_1H) {
                    alarm_dt.day = -1;
                    alarm_dt.hour = 1;
                }

                rtc_init();
                rtc_set_datetime(&dt);
                rtc_set_alarm(&alarm_dt, wakeUp);

                clocks_hw->sleep_en0 = CLOCKS_SLEEP_EN0_CLK_RTC_RTC_BITS;
                clocks_hw->sleep_en1 = 0x0;

                uint save = scb_hw->scr;
                
                // Enable deep sleep at the proc
                scb_hw->scr = save | M0PLUS_SCR_SLEEPDEEP_BITS;

                /*
                ** Now we can go to sleep zzzzzzzz
                */
                __wfi();
                break;
        }

        scheduleTaskExlusive(TASK_BATTERY_MONITOR, delay, false, NULL);
        return;
    }

    runCount++;
}
