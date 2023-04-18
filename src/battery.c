#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "hardware/adc.h"
#include "hardware/rtc.h"
#include "adc_rp2040.h"
#include "rtc_rp2040.h"
#include "scheduler.h"
#include "taskdef.h"
#include "sensor.h"
#include "battery.h"

datetime_t          wakeTime;

void wakeUp(void) {

}

void taskBatteryMonitor(PTASKPARM p) {
    weather_packet_t *          pWeather;
    
    pWeather = getWeatherPacket();

    /*
    ** If the battery voltage has dropped below critical,
    ** stop everything and put the RP2040 to sleep...
    */
    if (pWeather->rawBatteryVolts < ADC_BATTERY_VOLTAGE_CRITICAL) {
        /*
        ** Steps we need to take:
        **
        ** 1. Send a final message back to the RPi base station
        ** 2. Stop the scheduler (by stopping the RTC timer)
        ** 3. Switch off the I2C device power
        ** 4. Stop all clocks other than the RTC
        ** 5. Setup the RTC and set a timer for 36 to 48 hours
        ** 6. Go to sleep zzzzzzzz
        */
//        sleep_goto_sleep_until(&wakeTime, &wakeUp);
    }
}
