#include "hardware/i2c.h"

#include "i2c_addr.h"

#ifndef __INCL_LTR390
#define __INCL_LTR390

#define LTR390_REG_CTRL                     0x00

#define LTR390_CTRL_RESET                   0x10
#define LTR390_CTRL_UVS_MODE_ALS            0x00
#define LTR390_CTRL_UVS_MODE_UVS            0x08
#define LTR390_CTRL_SENSOR_STANDBY          0x00
#define LTR390_CTRL_SENSOR_ENABLE           0x02

#define LTR390_REG_MEAS_RATE                0x04

#define LTR390_MEAS_RATE_13_BIT             0x50
#define LTR390_MEAS_RATE_16_BIT             0x40
#define LTR390_MEAS_RATE_17_BIT             0x30
#define LTR390_MEAS_RATE_18_BIT             0x20
#define LTR390_MEAS_RATE_19_BIT             0x10
#define LTR390_MEAS_RATE_20_BIT             0x00

#define LTR390_MEAS_RATE_25MS               0x00
#define LTR390_MEAS_RATE_50MS               0x01
#define LTR390_MEAS_RATE_100MS              0x02
#define LTR390_MEAS_RATE_200MS              0x03
#define LTR390_MEAS_RATE_500MS              0x04
#define LTR390_MEAS_RATE_1000MS             0x05
#define LTR390_MEAS_RATE_2000MS             0x06

#define LTR390_REG_GAIN                     0x05

#define LTR390_GAIN_RANGE_1                 0x00
#define LTR390_GAIN_RANGE_3                 0x01
#define LTR390_GAIN_RANGE_6                 0x02
#define LTR390_GAIN_RANGE_9                 0x03
#define LTR390_GAIN_RANGE_18                0x04

#define LTR390_REG_STATUS                   0x07

#define LTR390_STATUS_DATA_READY            0x08
#define LTR390_STATUS_INT_TRIGGERED         0x10

#define LTR390_REG_ALS_DATA0                0x0D
#define LTR390_REG_ALS_DATA1                0x0E
#define LTR390_REG_ALS_DATA2                0x0F

#define LTR390_REG_UVS_DATA0                0x10
#define LTR390_REG_UVS_DATA1                0x11
#define LTR390_REG_UVS_DATA2                0x12

int ltr390_setup(i2c_inst_t * i2c);

#endif
