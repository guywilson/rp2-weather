#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#include "hardware/i2c.h"
#include "i2c_rp2040.h"
#include "lc709203.h"
#include "utils.h"

static uint8_t compute_crc8(uint8_t * data, int length) {
    const uint8_t     POLYNOMIAL = 0x07;
    uint8_t           crc = 0x00;
    int               i;
    int               j;

    for (j = length; j; --j) {
        crc ^= *data++;

        for (i = 8; i; --i) {
            crc = (crc & 0x80) ? (crc << 1) ^ POLYNOMIAL : (crc << 1);
        }
    }

    return crc;
}

int lc709203_write_register(i2c_inst_t * i2c, uint8_t reg, uint16_t data) {
    uint8_t             buffer[8];

    buffer[0] = LC709203_ADDRESS << 1;
    buffer[1] = reg;
    buffer[2] = data & 0x00FF;
    buffer[3] = (data >> 8) & 0x00FF;
    buffer[4] = compute_crc8(buffer, 4);

    i2cWriteRegister(i2c, LC709203_ADDRESS, reg, &buffer[2], 3);

    return 0;
}

int lc709203_read_register(i2c_inst_t * i2c, uint8_t reg, uint16_t * data) {
    uint8_t             buffer[8];
    uint8_t             crc;

    i2cReadRegister(i2c, LC709203_ADDRESS, reg, &buffer[3], 3);

    buffer[0] = LC709203_ADDRESS << 1;
    buffer[1] = reg;
    buffer[2] = buffer[0] | 0x01;

    crc = compute_crc8(buffer, 5);

    if (crc != buffer[5]) {
        return -1;
    }

    *data = ((uint16_t)((((uint16_t)buffer[4]) << 8) | (uint16_t)buffer[3]));

    return 0;
}

int lc709203_setup(i2c_inst_t * i2c) {
    lc709203_write_register(i2c, LC709203_CMD_IC_POWER_MODE, 0x0001);
    lc709203_write_register(i2c, LC709203_CMD_APA, 0x003C);
    lc709203_write_register(i2c, LC709203_CMD_COTP, 0x0001);
    lc709203_write_register(i2c, LC709203_CMD_STATUS_BIT, 0x0001);
    lc709203_write_register(i2c, LC709203_CMD_THERMISTOR_B, THERMISTOR_B_VALUE);
    lc709203_write_register(i2c, LC709203_CMD_ALARM_LOW_CELL_VOLTAGE, 3450);

    return 0;
}
