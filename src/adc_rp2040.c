#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>

#include "hardware/adc.h"
#include "adc_rp2040.h"
#include "sensor.h"

void adcIRQ() {
    int                 i;
    int                 channel;
    weather_packet_t *  weather;

    weather = getWeatherPacket();

    channel = adc_get_selected_input();

    while (i < 4) {
        switch (channel) {
            case ADC_INPUT_WIND_DIR:
                weather->rawWindDir = adc_fifo_get();
                break;

            case ADC_INPUT_BATTERY_VOLTAGE:
                weather->rawBatteryVolts = adc_fifo_get();
                break;

            case ADC_INPUT_BATTERY_TEMPERATURE:
                weather->rawBatteryTemperature = adc_fifo_get();
                channel++;
                break;

            case ADC_INPUT_NOT_CONNECTED:
                break;

            case ADC_INPUT_INTERNAL_TEMPERATURE:
                weather->rawChipTemperature = adc_fifo_get();
                break;
        }

        channel++;

        if (channel == 5) {
            channel = 0;
        }

        i++;
    }
}

void adcInit() {
    adc_init();

    adc_gpio_init(26);
    adc_gpio_init(27);
    adc_gpio_init(28);

    /*
    ** On-chip temperature sensor...
    */
    adc_select_input(ADC_INPUT_INTERNAL_TEMPERATURE);

    adc_set_temp_sensor_enabled(true);

    /*
    ** 1000 samples/sec
    */
    adc_set_clkdiv(47999.0f);
    
    adc_set_round_robin(0x17);

    adc_fifo_drain();
    adc_fifo_setup(true, false, 4, false, false);

    adc_irq_set_enabled(true);

    irq_set_exclusive_handler(ADC_IRQ_FIFO, adcIRQ);

    adc_run(true);
}
