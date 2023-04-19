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

#define ADC_SAMPLE_BUFFER_SIZE                  8

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

adc_samples_t           adcSamples[ADC_SAMPLE_BUFFER_SIZE];

void adcIRQ() {
    static int          sampleCount = 0;
    int                 channel;
    adc_samples_t *     pSamples;

    pSamples = &adcSamples[sampleCount];

    /*
    ** The first sample in our FIFO will be from channel 1...
    */
    channel = ADC_CHANNEL_WIND_DIR;

    while (!adc_fifo_is_empty()) {
        switch (channel) {
            case ADC_CHANNEL_NOT_CONNECTED:
                break;

            case ADC_CHANNEL_WIND_DIR:
                pSamples->adcWindDir = adc_fifo_get();
                break;

            case ADC_CHANNEL_BATTERY_TEMPERATURE:
                pSamples->adcBatteryTemperature = adc_fifo_get();
                break;

            case ADC_CHANNEL_BATTERY_VOLTAGE:
                pSamples->adcBatteryVoltage = adc_fifo_get();
                break;
        }

        channel++;

        if (channel == 4) {
            channel = 1;
        }
    }

    sampleCount++;

    if (sampleCount == ADC_SAMPLE_BUFFER_SIZE) {
        sampleCount = 0;

        scheduleTask(TASK_ADC, rtc_val_ms(1), false, NULL);
    }
}

void taskADC(PTASKPARM p) {
    int                         i;
    uint16_t                    sampleAvg;
    weather_packet_t *          pWeather;
    
    pWeather = getWeatherPacket();

    for (i = 0;i < ADC_SAMPLE_BUFFER_SIZE;i++) {
        sampleAvg += adcSamples[i].adcWindDir;
    }
    pWeather->rawWindDir = sampleAvg >> 3;
    sampleAvg = 0;

    for (i = 0;i < ADC_SAMPLE_BUFFER_SIZE;i++) {
        sampleAvg += adcSamples[i].adcBatteryVoltage;
    }
    pWeather->rawBatteryVolts = sampleAvg >> 3;
    sampleAvg = 0;

    for (i = 0;i < ADC_SAMPLE_BUFFER_SIZE;i++) {
        sampleAvg += adcSamples[i].adcBatteryTemperature;
    }
    pWeather->rawBatteryTemperature = sampleAvg >> 3;
    sampleAvg = 0;
}

void adcInit() {
    adc_init();

    /*
    ** Clear the sample buffer...
    */
    memset(adcSamples, 0, sizeof(adc_samples_t) * ADC_SAMPLE_BUFFER_SIZE);

//    adc_gpio_init(26);
    adc_gpio_init(27);
    adc_gpio_init(28);
    adc_gpio_init(29);

    adc_select_input(ADC_CHANNEL_WIND_DIR);

    /*
    ** 750 samples/sec
    */
    adc_set_clkdiv(63999.0f);
    
    adc_set_round_robin(0x0E);

    adc_fifo_drain();
    adc_fifo_setup(true, false, 4, false, false);

    adc_irq_set_enabled(true);

    irq_set_exclusive_handler(ADC_IRQ_FIFO, adcIRQ);
    irq_set_enabled(ADC_IRQ_FIFO, true);

    adc_run(true);
}
