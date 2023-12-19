#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "hardware/pio.h"
#include "pio_rp2040.h"
#include "rtc_rp2040.h"
#include "gpio_def.h"
#include "logger.h"
#include "scheduler.h"
#include "taskdef.h"
#include "sensor.h"

#include "pulsecount.pio.h"

#define ANEMOMETER_PULSE_COUNT_BIT_SHIFT     2
#define RAIN_GAUGE_PULSE_COUNT_BIT_SHIFT     1

#define PIO_SM_ANEMOMETER                    0
#define PIO_SM_RAIN_GAUGE                    1

/*
** On the bc-robotics 'weather' board, the RJ11
** sockets are wired to:
**
#define PIO_PIN_ANEMOMETER                  26
#define PIO_PIN_RAIN_GAUGE                   3
*/
#define PIO_ANEMOMETER_OFFSET                0
#define PIO_RAIN_GAUGE_OFFSET               16

#define PIO_AVG_BUFFER_SIZE                 64

#define PIO_ANEMOMETER_TASK_RUNS            50
#define PIO_RAIN_GAUGE_TASK_RUNS_PER_HOUR   PIO_RAIN_PULSE_BUFFER_SIZE

/*
** To calculate KPH:
**
** Anemometer circumference in km: pi * diameter = 0.0005654866776462
** Rotations = pulse count / 2
** Distance in km = circumference * rotations
** kph = km * seconds per hr (3600) * anemometer factor (1.18) / time for each run (5 secs)
**
** The constant bit = circumference (0.0005654866776462) * 3600 * 1.18 / 5 * 2
** But we alsoe multiply by 100 so we can put it in a uint16_t and simply
** divide by 100 at the other end:
**
** = 24.0218740664089950
*/
#define ANEMOMETER_KPH_FACTOR               24.0218740664089950f

/*
** Wind speed in mph:
*/
#define ANEMOMETER_MPH              0.0062137119223733f


static uint32_t         averageBuffer[PIO_AVG_BUFFER_SIZE];
static uint             anemometerSM;
static uint             rainGaugeSM;
static bool             isPIOEnabled = false;

void pioInit() {
    uint            anemometerOffset = PIO_ANEMOMETER_OFFSET;
    uint            rainGaugeOffset = PIO_RAIN_GAUGE_OFFSET;

    pio_add_program_at_offset(pio0, &pulsecount_program, anemometerOffset);
    pio_add_program_at_offset(pio0, &pulsecount_program, rainGaugeOffset);

    anemometerSM = pio_claim_unused_sm(pio0, true);
    rainGaugeSM = pio_claim_unused_sm(pio0, true);

    pio_sm_config anemometerConfig = pulsecount_program_get_default_config(anemometerOffset);
    pio_sm_config rainGaugeConfig = pulsecount_program_get_default_config(rainGaugeOffset);

    sm_config_set_in_pins(&anemometerConfig, PIO_PIN_ANEMOMETER);
    sm_config_set_in_pins(&rainGaugeConfig, PIO_PIN_RAIN_GAUGE);

    sm_config_set_jmp_pin(&anemometerConfig, PIO_PIN_ANEMOMETER);
    sm_config_set_jmp_pin(&rainGaugeConfig, PIO_PIN_RAIN_GAUGE);

    pio_sm_set_consecutive_pindirs(pio0, anemometerSM, PIO_PIN_ANEMOMETER, 1, false);
    pio_sm_set_consecutive_pindirs(pio0, rainGaugeSM, PIO_PIN_RAIN_GAUGE, 1, false);

    pio_gpio_init(pio0, PIO_PIN_ANEMOMETER);
    pio_gpio_init(pio0, PIO_PIN_RAIN_GAUGE);

    sm_config_set_in_shift(&anemometerConfig, false, true, ANEMOMETER_PULSE_COUNT_BIT_SHIFT);
    sm_config_set_in_shift(&rainGaugeConfig, false, true, RAIN_GAUGE_PULSE_COUNT_BIT_SHIFT);

    sm_config_set_fifo_join(&anemometerConfig, PIO_FIFO_JOIN_RX);
    sm_config_set_fifo_join(&rainGaugeConfig, PIO_FIFO_JOIN_RX);

    pio_sm_init(pio0, anemometerSM, anemometerOffset, &anemometerConfig);
    pio_sm_set_enabled(pio0, anemometerSM, true);

    pio_sm_init(pio0, rainGaugeSM, rainGaugeOffset, &rainGaugeConfig);
    pio_sm_set_enabled(pio0, rainGaugeSM, true);

    isPIOEnabled = true;
}

void disablePIO(void) {
    if (isPIOEnabled) {
        pio_sm_set_enabled(pio0, anemometerSM, false);
        pio_sm_set_enabled(pio0, rainGaugeSM, false);

        isPIOEnabled = false;
    }
}

/*
** Python example from: 
** https://github.com/raspberrypilearning/build-your-own-weather-station/
** 
** interval = 300
**
** while True:
**     start_time = time.time()
**     while time.time() - start_time <= interval:
**         wind_start_time = time.time()
**         reset_wind()
**         #time.sleep(wind_interval)
**         while time.time() - wind_start_time <= wind_interval:
**                 store_directions.append(wind_direction_byo.get_value())
** 
**         final_speed = calculate_speed(wind_interval)
**         store_speeds.append(final_speed)
**     wind_average = wind_direction_byo.get_average(store_directions)
** 
**     wind_gust = max(store_speeds)
**     wind_speed = statistics.mean(store_speeds)
**     print(wind_speed, wind_gust, wind_average)
**     store_speeds = []
**     store_directions =[]
*/
void taskAnemometer(PTASKPARM p) {
    static int          ix = 0;
    static int          runCount = 0;
    static uint32_t     pulseCount = 0;
    int                 i;
    uint32_t            totalCount = 0;
    uint32_t            maxCount = 0;
    weather_packet_t *  pWeather;

    /*
    ** Pulses are bitshifted into the RX_FIFO by the PIO.
    ** The PIO is setup to auto-push 2 bits into the FIFO
    ** so two pulses will be encoded as 0b00000011 = 3, so
    ** all we need to calculate the pulse count is 
    ** the number of entries in the FIFO x the bits per entry...
    */
    pulseCount += (pio_sm_get_rx_fifo_level(pio0, anemometerSM) * ANEMOMETER_PULSE_COUNT_BIT_SHIFT);
    pio_sm_clear_fifos(pio0, anemometerSM);
    
    runCount++;

    if (runCount == PIO_ANEMOMETER_TASK_RUNS) {
        /*
        ** Our pulse count is number of pulses every 5 seconds...
        */
        averageBuffer[ix++] = pulseCount;

        if (ix == PIO_AVG_BUFFER_SIZE) {
            for (i = 0;i < PIO_AVG_BUFFER_SIZE;i++) {
                totalCount += averageBuffer[i];

                /*
                ** Get the largest (gust) count...
                */
                if (averageBuffer[i] > maxCount) {
                    maxCount = averageBuffer[i];
                }
            }

            pWeather = getWeatherPacket();

            /*
            ** Get the average windspeed over 64 measurements (5 minutes, 20 seconds)...
            */
            pWeather->rawWindspeed = 
                (uint16_t)(ANEMOMETER_KPH_FACTOR * (float)((uint16_t)(totalCount >> 6)));

            pWeather->rawWindGust = 
                (uint16_t)(ANEMOMETER_KPH_FACTOR * (float)maxCount);

//            lgLogDebug("Spd:%.2f, Gst:%.2f", (float)pWeather->rawWindspeed * ANEMOMETER_MPH, (float)pWeather->rawWindGust * ANEMOMETER_MPH);

//            lgLogDebug("Avg windspeed count: %d", pWeather->rawWindspeed);

            memset(averageBuffer, 0, sizeof(uint32_t) * PIO_AVG_BUFFER_SIZE);
            totalCount = 0;

            ix = 0;
        }

        runCount = 0;
        pulseCount = 0;
    }
}

void taskRainGuage(PTASKPARM p) {
    weather_packet_t *  pWeather;

    pWeather = getWeatherPacket();

    /*
    ** Pulses are bitshifted into the RX_FIFO by the PIO.
    ** The PIO is setup to auto-push 2 bits into the FIFO
    ** so two pulses will be encoded as 0b00000011 = 3, so
    ** all we need to calculate the pulse count is 
    ** the number of entries in the FIFO x the bits per entry...
    */
    pWeather->rawRainfall += 
                (uint16_t)(pio_sm_get_rx_fifo_level(pio0, rainGaugeSM) * 
                                        RAIN_GAUGE_PULSE_COUNT_BIT_SHIFT);
    
    pio_sm_clear_fifos(pio0, rainGaugeSM);

//    lgLogDebug("Rainfall count: %d", (int)pWeather->rawRainfall);
}
