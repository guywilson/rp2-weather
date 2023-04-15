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

#define PIO_SM_ANEMOMETER                    0
#define PIO_SM_RAIN_GAUGE                    1

#define PIO_PIN_ANEMOMETER                  18
#define PIO_PIN_RAIN_GAUGE                  19

#define PIO_ANEMOMETER_OFFSET                0
#define PIO_RAIN_GAUGE_OFFSET               16

#define PIO_COUNTER_RESET_ANEMOMETER      2048
#define PIO_COUNTER_RESET_RAIN_GAUGE       512

static uint16_t         averageBuffer[16];

void pioInit() {
    uint            anemometerOffset = PIO_ANEMOMETER_OFFSET;
    uint            rainGaugeOffset = PIO_RAIN_GAUGE_OFFSET;

    gpio_set_function(PIO_PIN_ANEMOMETER, GPIO_FUNC_PIO0);
    gpio_set_dir(PIO_PIN_ANEMOMETER, GPIO_IN);
    gpio_pull_up(PIO_PIN_ANEMOMETER);

    gpio_set_function(PIO_PIN_RAIN_GAUGE, GPIO_FUNC_PIO0);
    gpio_set_dir(PIO_PIN_RAIN_GAUGE, GPIO_IN);
    gpio_pull_up(PIO_PIN_RAIN_GAUGE);

    /*
    ** Load the program:
    **
    ** .wrap_target
    **     set x, <reset_value>
    ** loop:
    **     wait 1 pin <pin>
    **     wait 0 pin <pin>
    **     jmp X-- loop
    ** .wrap
    */
    pio0->instr_mem[anemometerOffset]       = pio_encode_set(pio_x, PIO_COUNTER_RESET_ANEMOMETER);
    pio0->instr_mem[anemometerOffset + 1]   = pio_encode_wait_pin(true, PIO_PIN_ANEMOMETER);
    pio0->instr_mem[anemometerOffset + 2]   = pio_encode_wait_pin(false, PIO_PIN_ANEMOMETER);
    pio0->instr_mem[anemometerOffset + 3]   = pio_encode_jmp_x_dec(anemometerOffset + 1);

    pio_sm_config anemometerConfig = pio_get_default_sm_config();
    sm_config_set_wrap(&anemometerConfig, anemometerOffset, anemometerOffset + 3);

    pio0->instr_mem[rainGaugeOffset]        = pio_encode_set(pio_x, PIO_COUNTER_RESET_RAIN_GAUGE);
    pio0->instr_mem[rainGaugeOffset + 1]    = pio_encode_wait_pin(true, PIO_PIN_RAIN_GAUGE);
    pio0->instr_mem[rainGaugeOffset + 2]    = pio_encode_wait_pin(false, PIO_PIN_RAIN_GAUGE);
    pio0->instr_mem[rainGaugeOffset + 3]    = pio_encode_jmp_x_dec(rainGaugeOffset + 1);

    pio_sm_config rainGaugeConfig = pio_get_default_sm_config();
    sm_config_set_wrap(&rainGaugeConfig, rainGaugeOffset, rainGaugeOffset + 3);

    pio_sm_clear_fifos(pio0, PIO_SM_ANEMOMETER);
    pio_sm_clear_fifos(pio0, PIO_SM_RAIN_GAUGE);

    sm_config_set_in_pins(&anemometerConfig, PIO_PIN_ANEMOMETER);
    sm_config_set_in_pins(&rainGaugeConfig, PIO_PIN_RAIN_GAUGE);

    pio_set_irq0_source_enabled(pio0, pis_interrupt0, false);

    pio_sm_init(pio0, PIO_SM_ANEMOMETER, anemometerOffset, &anemometerConfig);
    pio_sm_set_enabled(pio0, PIO_SM_ANEMOMETER, true);

    pio_sm_init(pio0, PIO_SM_RAIN_GAUGE, rainGaugeOffset, &rainGaugeConfig);
    pio_sm_set_enabled(pio0, PIO_SM_RAIN_GAUGE, true);
}

/*
** We measure wind speed in m/s. This task runs
** once per second, so adding up the number of pulses
** in the last second, and knowing the diameter of
** our anemometer, we can work out our wind speed.
*/
void taskAnemometer(PTASKPARM p) {
    static int          ix = 0;
    int                 i;
    uint16_t            pulseCount = 0;
    uint16_t            totalCount = 0;
    weather_packet_t *  pWeather;

    /*
    ** Get the pulse count...
    */
    pio_sm_exec(pio0, PIO_SM_ANEMOMETER, pio_encode_mov(pio_isr, pio_x));
    pio_sm_exec(pio0, PIO_SM_ANEMOMETER, pio_encode_push(false, false));

    pulseCount = PIO_COUNTER_RESET_ANEMOMETER - (uint16_t)pio_sm_get(pio0, PIO_SM_ANEMOMETER);
    
    /*
    ** Reset X...
    */
    pio_sm_put(pio0, PIO_SM_ANEMOMETER, PIO_COUNTER_RESET_ANEMOMETER);
    pio_sm_exec(pio0, PIO_SM_ANEMOMETER, pio_encode_pull(false, false));
    pio_sm_exec(pio0, PIO_SM_ANEMOMETER, pio_encode_mov(pio_x, pio_osr));

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
    pio_sm_exec(pio0, PIO_SM_RAIN_GAUGE, pio_encode_mov(pio_isr, pio_x));
    pio_sm_exec(pio0, PIO_SM_RAIN_GAUGE, pio_encode_push(false, false));

    pulseCount = PIO_COUNTER_RESET_RAIN_GAUGE - (uint16_t)pio_sm_get(pio0, PIO_SM_RAIN_GAUGE);
    
    /*
    ** Reset X...
    */
    pio_sm_put(pio0, PIO_SM_RAIN_GAUGE, PIO_COUNTER_RESET_RAIN_GAUGE);
    pio_sm_exec(pio0, PIO_SM_RAIN_GAUGE, pio_encode_pull(false, false));
    pio_sm_exec(pio0, PIO_SM_RAIN_GAUGE, pio_encode_mov(pio_x, pio_osr));

    /*
    ** As this task runs once per hour, this equates to pulses/hour...
    */
    pWeather->rawRainfall = pulseCount;

    lgLogDebug("Rainfall count: %d", pWeather->rawRainfall);
}
