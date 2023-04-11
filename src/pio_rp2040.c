#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "hardware/pio.h"
#include "pio_rp2040.h"
#include "rtc_rp2040.h"
#include "logger.h"
#include "scheduler.h"
#include "taskdef.h"
#include "sensor.h"
#include "anemometer_pulse.pio.h"
#include "rainfall_pulse.pio.h"

#define PIO_PIN_ANEMOMETER                  18
#define PIO_PIN_RAIN_GAUGE                  19

#define PIO_0_COUNTER_RESET               2048
#define PIO_1_COUNTER_RESET                512

void pioInit() {
    uint            anemometerOffset;
    uint            rainGaugeOffset;

    anemometerOffset = pio_add_program(pio0, &anemometer_pulse_program);
    pio_sm_config anemometerConfig = anemometer_pulse_program_get_default_config(anemometerOffset);

    rainGaugeOffset = pio_add_program(pio1, &rainfall_pulse_program);
    pio_sm_config rainGaugeConfig = rainfall_pulse_program_get_default_config(rainGaugeOffset);

    sm_config_set_in_pins(&anemometerConfig, PIO_PIN_ANEMOMETER);
    sm_config_set_in_pins(&rainGaugeConfig, PIO_PIN_RAIN_GAUGE);

    pio_set_irq0_source_enabled(pio0, pis_interrupt0, false);
    pio_set_irq1_source_enabled(pio1, pis_interrupt1, false);
    
    /*
    ** Initialise X...
    */
    pio_sm_put(pio0, 0, PIO_0_COUNTER_RESET);
    pio_sm_exec(pio0, 0, pio_encode_pull(false, false));
    pio_sm_exec(pio0, 0, pio_encode_mov(pio_x, pio_osr));
    
    /*
    ** Initialise X...
    */
    pio_sm_put(pio1, 0, PIO_1_COUNTER_RESET);
    pio_sm_exec(pio1, 0, pio_encode_pull(false, false));
    pio_sm_exec(pio1, 0, pio_encode_mov(pio_x, pio_osr));

    pio_sm_init(pio0, 0, anemometerOffset, &anemometerConfig);
    pio_sm_set_enabled(pio0, 0, true);

    pio_sm_init(pio1, 0, rainGaugeOffset, &rainGaugeConfig);
    pio_sm_set_enabled(pio1, 0, true);
}

/*
** We measure wind speed in m/s. This task runs
** once per second, so adding up the number of pulses
** in the last second, and knowing the diameter of
** our anemometer, we can work out our wind speed.
*/
void taskAnemometer(PTASKPARM p) {
    static uint16_t     averageBuffer[16];
    static int          ix = 0;
    int                 i;
    uint16_t            pulseCount = 0;
    uint16_t            totalCount = 0;
    weather_packet_t *  pWeather;

    /*
    ** Get the pulse count...
    */
    pio_sm_exec(pio0, 0, pio_encode_mov(pio_isr, pio_x));
    pio_sm_exec(pio0, 0, pio_encode_push(false, false));

    pulseCount = PIO_0_COUNTER_RESET - (uint16_t)pio_sm_get(pio0, 0);
    
    /*
    ** Reset X...
    */
    pio_sm_put(pio0, 0, PIO_0_COUNTER_RESET);
    pio_sm_exec(pio0, 0, pio_encode_pull(false, false));
    pio_sm_exec(pio0, 0, pio_encode_mov(pio_x, pio_osr));

    /*
    ** As this task runs once per second, this equates to pulses/sec...
    */
    averageBuffer[ix++] = pulseCount;

    if (ix == 16) {
        for (i = 0;i < 16;i++) {
            totalCount += averageBuffer[i];
        }

        pWeather = getWeatherPacket();

        /*
        ** Get the average windspeed over 16 measurements (16 seconds)...
        */
        pWeather->rawWindspeed = totalCount >> 4;

        lgLogDebug("Avg windspeed count: %d", pWeather->rawWindspeed);

        ix = 0;
    }
}

/*
** We measure rainfall in mm/h. This task runs
** once per hour, so adding up the number of pulses
** in the last hour, and knowing the volume of
** our tipping bucket, we can work out our rainfall.
*/
void taskRainGuage(PTASKPARM p) {
    uint16_t            pulseCount;
    weather_packet_t *  pWeather;

    pWeather = getWeatherPacket();

    /*
    ** Get the pulse count...
    */
    pio_sm_exec(pio1, 0, pio_encode_mov(pio_isr, pio_x));
    pio_sm_exec(pio1, 0, pio_encode_push(false, false));

    pulseCount = PIO_1_COUNTER_RESET - (uint16_t)pio_sm_get(pio1, 0);
    
    /*
    ** Reset X...
    */
    pio_sm_put(pio1, 0, PIO_0_COUNTER_RESET);
    pio_sm_exec(pio1, 0, pio_encode_pull(false, false));
    pio_sm_exec(pio1, 0, pio_encode_mov(pio_x, pio_osr));

    /*
    ** As this task runs once per hour, this equates to pulses/hour...
    */
    pWeather->rawRainfall = pulseCount;

    lgLogDebug("Rainfall count: %d", pWeather->rawRainfall);
}
