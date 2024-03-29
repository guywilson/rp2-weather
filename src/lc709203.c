#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#include "hardware/i2c.h"
#include "i2c_rp2040.h"
#include "rtc_rp2040.h"
#include "lc709203.h"
#include "utils.h"
#include "logger.h"

#define LC709203_CRC_POLYNOMIAL                 0x07


static uint8_t compute_crc8(uint8_t * data, int length) {
    const uint8_t       polynomial = LC709203_CRC_POLYNOMIAL;
    uint8_t             crc = 0x00;
    int                 i;
    int                 j;

    for (j = 0;j < length;j++) {
        crc ^= data[j];

        for (i = 0;i < 8;i++) {
            if ((crc & 0x80) != 0x00) {
                crc = ((crc << 1) ^ polynomial);
            }
            else {
                crc = (crc << 1);
            }
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

    return i2cWriteRegister(i2c, LC709203_ADDRESS, reg, &buffer[2], 3);
}

int lc709203_read_register(i2c_inst_t * i2c, uint8_t reg, uint16_t * data) {
    uint8_t             buffer[8];
    uint8_t             crc;
    int                 error;

    error = i2cReadRegister(i2c, LC709203_ADDRESS, reg, &buffer[3], 3);

    if (error == PICO_ERROR_TIMEOUT) {
        return PICO_ERROR_TIMEOUT;
    }
    else if (error == PICO_ERROR_GENERIC) {
        return PICO_ERROR_GENERIC;
    }

    buffer[0] = (LC709203_ADDRESS << 1);
    buffer[1] = reg;
    buffer[2] = buffer[0] | 0x01;

    crc = compute_crc8(buffer, 5);

    lgLogDebug("LC709203 CRC[c]: 0x%02X, [a]: 0x%02X", crc, buffer[5]);

    *data = ((uint16_t)((((uint16_t)buffer[4]) << 8) | (uint16_t)buffer[3]));

    if (crc != buffer[5]) {
        return LC709203_ERROR_CRC;
    }

    return error;
}

void lc709203_reset(i2c_inst_t * i2c) {
    lc709203_write_register(i2c, LC709203_CMD_COTP, 0x0001);
}

int lc709203_setup(i2c_inst_t * i2c) {
    int         error;

    error = lc709203_write_register(i2c, LC709203_CMD_IC_POWER_MODE, 0x0001);

    if (error == PICO_ERROR_TIMEOUT) {
        return PICO_ERROR_TIMEOUT;
    }

    error = lc709203_write_register(i2c, LC709203_CMD_APA, 0x003C);

    if (error == PICO_ERROR_TIMEOUT) {
        return PICO_ERROR_TIMEOUT;
    }

    error = lc709203_write_register(i2c, LC709203_CMD_COTP, 0x0001);

    if (error == PICO_ERROR_TIMEOUT) {
        return PICO_ERROR_TIMEOUT;
    }

    error = lc709203_write_register(i2c, LC709203_CMD_STATUS_BIT, 0x0001);

    if (error == PICO_ERROR_TIMEOUT) {
        return PICO_ERROR_TIMEOUT;
    }

    error = lc709203_write_register(i2c, LC709203_CMD_THERMISTOR_B, THERMISTOR_B_VALUE);

    if (error == PICO_ERROR_TIMEOUT) {
        return PICO_ERROR_TIMEOUT;
    }

    error = lc709203_write_register(i2c, LC709203_CMD_ALARM_LOW_CELL_VOLTAGE, 3450);

    if (error == PICO_ERROR_TIMEOUT) {
        return PICO_ERROR_TIMEOUT;
    }
    
    return 0;
}
