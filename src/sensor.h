#include <stdint.h>

#include "scheduler.h"
#include "hardware/i2c.h"

#ifndef __INCL_I2C_SENSOR
#define __INCL_I2C_SENSOR

typedef struct {
    int16_t             numerator;
    uint16_t            denominator;
}
short_decimal_t;

typedef struct {
    short_decimal_t     temperature;
    short_decimal_t     pressure;
    short_decimal_t     humidity;
    short_decimal_t     rainfall;
    short_decimal_t     windspeed;
    uint16_t            windDirection;
}
weather_packet_t;

int tmp117_setup(i2c_inst_t * i2c);
void taskReadTemp(PTASKPARM p);
void taskReadHumidity(PTASKPARM p);
void taskReadPressure(PTASKPARM p);

#endif
