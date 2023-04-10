#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "hardware/adc.h"
#include "adc_rp2040.h"
#include "rtc_rp2040.h"
#include "scheduler.h"
#include "taskdef.h"
#include "sensor.h"

/*
** Wind direction values, a corresponding value
** will be read by the ADC. Using a resistor between
** the 3.3v supply and the weather vane creates a 
** voltage divider. We assume that R1 in the divider
** is 4.7Kohm 
** 
** Degrees          Resistance          Direction       Voltage         ~ADC
** -------          ----------          ---------       -------         ----
**   0.0            33K                 N               2.889           3586
**  22.5            6.57K               NNE             1.924           2388
**  45.0            8.2K                NE              2.098           2604
**  67.5            891                 ENE             0.526            653
**  90.0            1K                  E               0.579            719
** 112.5            688                 ESE             0.421            523
** 135.0            2.2K                SE              1.052           1306
** 157.5            1.41K               SSE             0.762            946
** 180.0            3.9K                S               1.497           1858
** 202.5            3.14K               SSW             1.322           1641
** 225.0            16K                 SW              2.551           3166
** 247.5            14.12K              WSW             2.476           3073
** 270.0            120K                W               3.176           3942
** 292.5            42.12K              NWN             2.969           3685
** 315.0            64.9K               NW              3.077           3819
** 337.5            21.88K              NNW             2.716           3371
**
*/

adc_samples_t           adcSamples;

void adcIRQ() {
    static int          numSamples = 0;
    int                 channel;

    if (numSamples == 0) {
        /*
        ** Clear the structure...
        */
        memset(&adcSamples, 0, sizeof(adc_samples_t));
    }

    /*
    ** The first sample in our FIFO will be from channel 1...
    */
    channel = ADC_CHANNEL_WIND_DIR;

    while (!adc_fifo_is_empty()) {
        switch (channel) {
            case ADC_CHANNEL_NOT_CONNECTED:
                break;

            case ADC_CHANNEL_WIND_DIR:
                adcSamples.adcWindDir += adc_fifo_get();
                break;

            case ADC_CHANNEL_BATTERY_TEMPERATURE:
                adcSamples.adcBatteryTemperature += adc_fifo_get();
                break;

            case ADC_CHANNEL_BATTERY_VOLTAGE:
                adcSamples.adcBatteryVoltage += adc_fifo_get();
                break;

            case ADC_CHANNEL_INTERNAL_TEMPERATURE:
                adcSamples.adcRP2040Temperature += adc_fifo_get();
                break;
        }

        channel++;

        if (channel == 5) {
            channel = 1;
        }
    }

    numSamples++;

    if (numSamples == 16) {
        numSamples = 0;

        /*
        ** Average of the last 16 samples...
        */
        adcSamples.adcWindDir = (adcSamples.adcWindDir >> 4);
        adcSamples.adcBatteryVoltage = (adcSamples.adcBatteryVoltage >> 4);
        adcSamples.adcBatteryTemperature = (adcSamples.adcBatteryTemperature >> 4);
        adcSamples.adcRP2040Temperature = (adcSamples.adcRP2040Temperature >> 4);

        scheduleTask(TASK_ADC, rtc_val_ms(1), false, &adcSamples);
    }
}

void taskADC(PTASKPARM p) {
    weather_packet_t *          pWeather;
    adc_samples_t *             pSamples;
    
    pWeather = getWeatherPacket();
    pSamples = (adc_samples_t *)p;

    pWeather->rawWindDir = pSamples->adcWindDir;
    pWeather->rawBatteryVolts = pSamples->adcBatteryVoltage;
    pWeather->rawBatteryTemperature = pSamples->adcBatteryTemperature;
    pWeather->rawChipTemperature = pSamples->adcRP2040Temperature;
}

void adcInit() {
    adc_init();

    adc_gpio_init(26);
    adc_gpio_init(27);
    adc_gpio_init(28);
    adc_gpio_init(29);

    /*
    ** On-chip temperature sensor...
    */
    adc_select_input(ADC_CHANNEL_WIND_DIR);

    adc_set_temp_sensor_enabled(true);

    /*
    ** 750 samples/sec
    */
    adc_set_clkdiv(63999.0f);
    
    adc_set_round_robin(0x1E);

    adc_fifo_drain();
    adc_fifo_setup(true, false, 4, false, false);

    adc_irq_set_enabled(true);

    irq_set_exclusive_handler(ADC_IRQ_FIFO, adcIRQ);
    irq_set_enabled(ADC_IRQ_FIFO, true);

    adc_run(true);
}
