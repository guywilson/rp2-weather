#include <stdint.h>

#include "hardware/i2c.h"

#include "icp10125.h"

typedef struct {
    float       sensor_constants[4]; // OTP values
    float       p_Pa_calib[3];
    float       LUT_lower;
    float       LUT_upper;
    float       quadr_factor;
    float       offst_factor;

    uint16_t    rawTemperature;
    uint32_t    rawPressure;

    float       LUT_values[3];

    float       A;
    float       B;
    float       C;
}
inv_invpres_t;

inv_invpres_t               icpConfigParms;

void icp10125_calculate_conversion_constants(inv_invpres_t * s) { 
    s->C = 
        (s->LUT_values[0] * s->LUT_values[1] * (s->p_Pa_calib[0] - s->p_Pa_calib[1]) + 
        s->LUT_values[1] * s->LUT_values[2] * (s->p_Pa_calib[1] - s->p_Pa_calib[2]) + 
        s->LUT_values[2] * s->LUT_values[0] * (s->p_Pa_calib[2] - s->p_Pa_calib[0])) / 
        (s->LUT_values[2] * (s->p_Pa_calib[0] - s->p_Pa_calib[1]) + 
        s->LUT_values[0] * (s->p_Pa_calib[1] - s->p_Pa_calib[2]) + 
        s->LUT_values[1] * (s->p_Pa_calib[2] - s->p_Pa_calib[0])); 

    s->A = 
        (s->p_Pa_calib[0] * 
            s->LUT_values[0] - 
            s->p_Pa_calib[1] * 
            s->LUT_values[1] - 
                (s->p_Pa_calib[1] - 
                    s->p_Pa_calib[0]) * 
                    s->C) / 
                (s->LUT_values[0] - 
            s->LUT_values[1]); 

    s->B = 
        (s->p_Pa_calib[0] - 
        s->A) * 
        (s->LUT_values[0] + 
        s->C); 
}

int icp10125_setup(i2c_inst_t * i2c) {
    int                 error;
    int                 i;
    uint8_t             otpReadCmd[2];
    uint8_t             otpCmdBuf[5];
    uint8_t             otpReadData[3];
    inv_invpres_t *     s = &icpConfigParms;

    s->p_Pa_calib[0] = 45000.0f; 
    s->p_Pa_calib[1] = 80000.0f; 
    s->p_Pa_calib[2] = 105000.0f; 
    s->LUT_lower = 3670016.0f; 
    s->LUT_upper = 12058624.0f; 
    s->quadr_factor = 1.0f / 16777216.0f; 
    s->offst_factor = 2048.0f;

    otpCmdBuf[0] = 0xC5;
    otpCmdBuf[1] = 0x95;
    otpCmdBuf[2] = 0x00;
    otpCmdBuf[3] = 0x66;
    otpCmdBuf[4] = 0x9C;

    error = i2c_write_blocking(i2c, ICP10125_ADDRESS, otpCmdBuf, 5, false);

    if (error == PICO_ERROR_GENERIC) {
        return -1;
    }
    else if (error == PICO_ERROR_TIMEOUT) {
        return -1;
    }
    
    otpReadCmd[0] = 0xC7;
    otpReadCmd[1] = 0xF7;

    for (i = 0; i < 4; i++) {
        i2c_write_blocking(i2c, ICP10125_ADDRESS, otpReadCmd, 2, true);
        i2c_read_blocking(i2c, ICP10125_ADDRESS, otpReadData, 3, false);

        s->sensor_constants[i] = (float)((uint16_t)otpReadData[0] << 8 | (uint16_t)otpReadData[1]);
    }

    return 0;
}

uint32_t icp10125_get_pressure(uint16_t rawTemperature, uint32_t rawPressure) {
    uint32_t            pressurePa;
    float               t;
    inv_invpres_t *     s = &icpConfigParms;

    t = (float)(rawTemperature - 32768);

    s->LUT_values[0] = s->LUT_lower + (float)(s->sensor_constants[0] * t * t) * s->quadr_factor; 
    s->LUT_values[1] = s->offst_factor * s->sensor_constants[3] + (float)(s->sensor_constants[1] * t * t) * s->quadr_factor; 
    s->LUT_values[2] = s->LUT_upper + (float)(s->sensor_constants[2] * t * t) * s->quadr_factor; 

    icp10125_calculate_conversion_constants(s);

    pressurePa = (uint32_t)(s->A + s->B / (s->C + rawPressure));

    return pressurePa;
}
