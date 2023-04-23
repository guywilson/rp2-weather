#include <stdint.h>

#include "scheduler.h"
#include "hardware/i2c.h"
#include "packet.h"

#ifndef __INCL_I2C_SENSOR
#define __INCL_I2C_SENSOR

weather_packet_t *  getWeatherPacket();
int                 initSensors(i2c_inst_t * i2c);
void                taskI2CSensor(PTASKPARM p);

#endif
