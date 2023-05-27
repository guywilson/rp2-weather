#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "hardware/i2c.h"
#include "i2c_rp2040.h"
#include "ltr390.h"

int ltr390_setup(i2c_inst_t * i2c) {
    uint8_t         ctrl;
    uint8_t         measRate;
    uint8_t         gain;

    ctrl = LTR390_CTRL_SENSOR_ENABLE | LTR390_CTRL_UVS_MODE_ALS;
    i2cWriteRegister(i2c, LTR390_ADDRESS, LTR390_REG_CTRL, &ctrl, 1);

    measRate = LTR390_MEAS_RATE_20_BIT | LTR390_MEAS_RATE_100MS;
    i2cWriteRegister(i2c, LTR390_ADDRESS, LTR390_REG_MEAS_RATE, &measRate, 1);

    gain = LTR390_GAIN_RANGE_18;
    i2cWriteRegister(i2c, LTR390_ADDRESS, LTR390_REG_GAIN, &gain, 1);

    return 0;
}
