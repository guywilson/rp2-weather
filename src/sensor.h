#include <stdint.h>

#include "scheduler.h"
#include "hardware/i2c.h"

#ifndef __INCL_I2C_SENSOR
#define __INCL_I2C_SENSOR

typedef struct {
    float               temperature;
    float               pressure;
    float               humidity;
    float               rainfall;
    float               windspeed;
    uint16_t            windDirection;
}
weather_packet_t;

typedef struct {
    uint16_t        taskID;
    void *          next;
}
sensor_chain_t;

int     initSensors(i2c_inst_t * i2c);
void    taskReadTemp(PTASKPARM p);
void    taskReadHumidity(PTASKPARM p);
void    taskReadPressure(PTASKPARM p);

#endif
