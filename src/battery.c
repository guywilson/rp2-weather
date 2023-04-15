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
//        sleep_goto_sleep_until(&wakeTime, &wakeUp);
    }
}
