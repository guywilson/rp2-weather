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
#include "packet.h"
#include "logger.h"

#define ADC_SAMPLE_BUFFER_SIZE                  8

#define STATE_ADC_INIT                     0x0100
#define STATE_ADC_RUN                      0x0200
#define STATE_ADC_ACCUMULATE               0x0300
#define STATE_ADC_FINISH                   0x0400

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

const char *    dir_ordinal[16] = {
                    "ESE", "ENE", "E", "SSE", 
                    "SE", "SSW", "S", "NNE", 
                    "NE", "WSW", "SW", "NNW", 
                    "N", "NWN", "NW", "W"};

const uint16_t  dir_adc_max[16] = {
                    63,   82,   91,  128,
                   196,  274,  336,  538,
                   651, 1013, 1113, 1392,
                  1808, 2071, 2542, 3149};

adc_samples_t           adcSamples[ADC_SAMPLE_BUFFER_SIZE];

void taskADC(PTASKPARM p) {
    static int                  sampleCount = 0;
    static int                  state = STATE_ADC_INIT;
    int                         i;
    int                         j;
    uint16_t                    sampleAvg;
    rtc_t                       delay;
    weather_packet_t *          pWeather;
    adc_samples_t *             pSamples;

    switch (state) {
        case STATE_ADC_INIT:
            hw_write_masked(
                    &adc_hw->cs, 
                    ADC_CS_EN_BITS, 
                    ADC_CS_EN_BITS);

            delay = rtc_val_ms(1);
            state = STATE_ADC_RUN;
            break;

        case STATE_ADC_RUN:
            if (adc_hw->cs & ADC_CS_READY_BITS) {
                hw_write_masked(
                        &adc_hw->cs, 
                        (ADC_CHANNEL_WIND_DIR << ADC_CS_AINSEL_LSB | ADC_CS_START_ONCE_BITS), 
                        (ADC_CS_AINSEL_BITS | ADC_CS_START_ONCE_BITS));
                
                delay = rtc_val_ms(1);
                state = STATE_ADC_ACCUMULATE;
                break;
            }

            delay = rtc_val_ms(1);
            break;

        case STATE_ADC_ACCUMULATE:
            if (adc_hw->intr & 0x00000001) {
                pSamples = &adcSamples[sampleCount];

                while (!adc_fifo_is_empty()) {
                    pSamples->adcWindDir = adc_fifo_get();
                }

                sampleCount++;

                if (sampleCount == ADC_SAMPLE_BUFFER_SIZE) {
                    sampleCount = 0;

                    delay = rtc_val_ms(1);
                    state = STATE_ADC_FINISH;
                    break;
                }
            }

            state = STATE_ADC_RUN;
            delay = rtc_val_ms(1);
            break;

        case STATE_ADC_FINISH:
            /*
            ** Disable the ADC to save power...
            */
            adc_hw->cs = 0x00000000;

            pWeather = getWeatherPacket();

            for (i = 0;i < ADC_SAMPLE_BUFFER_SIZE;i++) {
                sampleAvg += adcSamples[i].adcWindDir;
            }
            pWeather->rawWindDir = sampleAvg >> 3;
            sampleAvg = 0;

            lgLogDebug("Raw dir: %d", (int)pWeather->rawWindDir);
            for (j = 0;j < 16;j++) {
                if (pWeather->rawWindDir < dir_adc_max[j]) {
                    lgLogDebug("Wind dir: %s", dir_ordinal[j]);
                    break;
                }
            }

            delay = rtc_val_sec(5);
            state = STATE_ADC_INIT;
            break;
    }

    scheduleTask(TASK_ADC, delay, false, NULL);
}

void adcInit() {
    adc_init();

    /*
    ** Clear the sample buffer...
    */
    memset(adcSamples, 0, sizeof(adc_samples_t) * ADC_SAMPLE_BUFFER_SIZE);

    adc_gpio_init(28);

    adc_select_input(ADC_CHANNEL_WIND_DIR);

    adc_set_clkdiv(0.0f);
    
    adc_fifo_drain();
    adc_fifo_setup(true, false, 1, false, false);
}
