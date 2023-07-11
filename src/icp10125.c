#include <stdint.h>

#include "hardware/i2c.h"
#include "i2c_rp2040.h"
#include "utils.h"
#include "logger.h"
#include "icp10125.h"

uint16_t            otpValues[4];
float               sensor_constants[4];

const float p_Pa_calib[3] = {45000.0f, 80000.0f, 105000.0f};
const float LUT_lower = 3670016.0f;
const float LUT_upper = 12058624.0f;
const float quadr_factor = 0.000000059605f;
const float offst_factor = 2048.0;

static void calculate_conversion_constants(const float *p_Pa, const float *p_LUT, float *out) { 
    float A, B, C; 
    
    C = (p_LUT[0] * p_LUT[1] * (p_Pa[0] - p_Pa[1]) + 
    p_LUT[1] * p_LUT[2] * (p_Pa[1] - p_Pa[2]) + 
    p_LUT[2] * p_LUT[0] * (p_Pa[2] - p_Pa[0])) / 
    (p_LUT[2] * (p_Pa[0] - p_Pa[1]) + 
    p_LUT[0] * (p_Pa[1] - p_Pa[2]) + 
    p_LUT[1] * (p_Pa[2] - p_Pa[0])); 
    A = (p_Pa[0] * p_LUT[0] - p_Pa[1] * p_LUT[1] - (p_Pa[1] - p_Pa[0]) * C) / (p_LUT[0] - p_LUT[1]); 
    B = (p_Pa[0] - A) * (p_LUT[0] + C); 
    
    out[0] = A; 
    out[1] = B; 
    out[2] = C; 
}

int icp10125_setup(i2c_inst_t * i2c) {
    int                 error;
    uint8_t             buffer[8];
    uint16_t            chipID;

    /*
    ** Issue soft reset...
    */
    buffer[0] = 0x80;
    buffer[1] = 0x5D;

    error = i2cWriteTimeoutProtected(i2c, ICP10125_ADDRESS, buffer, 2, false);

    if (error == PICO_ERROR_TIMEOUT) {
        return PICO_ERROR_TIMEOUT;
    }

    sleep_ms(10U);

    /*
    ** Read chip ID...
    */
    buffer[0] = 0xEF;
    buffer[1] = 0xC8;

    i2cWriteTimeoutProtected(i2c, ICP10125_ADDRESS, buffer, 2, true);
    error = i2cReadTimeoutProtected(i2c, ICP10125_ADDRESS, buffer, 3, false);

    if (error == PICO_ERROR_TIMEOUT) {
        return PICO_ERROR_TIMEOUT;
    }
    
    chipID = copyI2CReg_uint16(buffer);

    if ((chipID & 0x3F) != 0x08) {
        lgLogError("Read incorrect icp10125 chip ID: 0x%02X", (chipID & 0x3F));
        return -1;
    }

    return 0;
}

int icp10125_read_otp(void) {
    int                 error;
    int                 i;
    uint8_t             buffer[8];

    buffer[0] = 0xC5;
    buffer[1] = 0x95;
    buffer[2] = 0x00;
    buffer[3] = 0x66;
    buffer[4] = 0x9C;

    error = i2cWriteTimeoutProtected(i2c0, ICP10125_ADDRESS, buffer, 5, false);

    if (error == PICO_ERROR_TIMEOUT) {
        return PICO_ERROR_TIMEOUT;
    }
    
    for (i = 0; i < 4; i++) {
        buffer[0] = 0xC7;
        buffer[1] = 0xF7;

        i2cWriteTimeoutProtected(i2c0, ICP10125_ADDRESS, buffer, 2, false);
        i2cReadTimeoutProtected(i2c0, ICP10125_ADDRESS, buffer, 3, false);

        otpValues[i] = (uint16_t)((uint16_t)buffer[0] << 8 | (uint16_t)buffer[1]);
    }

    for(i = 0; i < 4; i++) {
        sensor_constants[i] = (float)otpValues[i];
    }

    return 0;
}

void icp10125_process_data(const int p_LSB, const int T_LSB, int * pressure) { 
    float t; 
    float s1, s2, s3; 
    float in[3]; 
    float out[3]; 
    float A, B, C; 
    
    t = (float)(T_LSB - 32768); 
    s1 = LUT_lower + (float)(sensor_constants[0] * t * t) * quadr_factor; 
    s2 = offst_factor * sensor_constants[3] + (float)(sensor_constants[1] * t * t) * quadr_factor; 
    s3 = LUT_upper + (float)(sensor_constants[2] * t * t) * quadr_factor; 
    in[0] = s1; 
    in[1] = s2; 
    in[2] = s3; 
    
    calculate_conversion_constants(p_Pa_calib, in, out); 
    A = out[0]; 
    B = out[1]; 
    C = out[2]; 
    
    *pressure = (int)(A + B / (C + p_LSB)); 
}
