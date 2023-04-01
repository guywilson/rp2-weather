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

    channel = adc_get_selected_input();

    while (!adc_fifo_is_empty()) {
        switch (channel) {
            case ADC_CHANNEL_WIND_DIR:
                adcSamples.adcWindDir += adc_fifo_get();
                break;

            case ADC_CHANNEL_BATTERY_VOLTAGE:
                adcSamples.adcBatteryVoltage += adc_fifo_get();
                break;

            case ADC_CHANNEL_BATTERY_TEMPERATURE:
                adcSamples.adcBatteryTemperature += adc_fifo_get();

                // Deliberately fall through...

            case ADC_CHANNEL_NOT_CONNECTED:
                channel++;
                break;

            case ADC_CHANNEL_INTERNAL_TEMPERATURE:
                adcSamples.adcRP2040Temperature += adc_fifo_get();
                break;
        }

        channel++;

        if (channel == 5) {
            channel = 0;
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

        scheduleTaskOnce(TASK_ADC, rtc_val_ms(1), &adcSamples);
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

    /*
    ** On-chip temperature sensor...
    */
    adc_select_input(ADC_CHANNEL_INTERNAL_TEMPERATURE);

    adc_set_temp_sensor_enabled(true);

    /*
    ** 750 samples/sec
    */
    adc_set_clkdiv(63999.0f);
    
    adc_set_round_robin(0x17);

    adc_fifo_drain();
    adc_fifo_setup(true, false, 4, false, false);

    adc_irq_set_enabled(true);

    irq_set_exclusive_handler(ADC_IRQ_FIFO, adcIRQ);
    irq_set_enabled(ADC_IRQ_FIFO, true);

    adc_run(true);
}
