#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/irq.h"
#include "logger.h"
#include "scheduler.h"
#include "taskdef.h"
#include "i2c_rp2040.h"

int _i2c_read_blocking_debug(i2c_inst_t * i2c, uint8_t addr, uint8_t * dst, size_t len, bool nostop) {
    bool            abort = false;
    uint32_t        abort_reason;
    uint8_t         b;
    int             byte_ctr;
    int             ilen = (int)len;

    i2c->hw->enable = 0;
    i2c->hw->tar = addr;
    i2c->hw->enable = 1;

    lgLogDebug("I2C_read: Ad:0x%02X, Ln:%d", addr, (int)len);

    for (byte_ctr = 0; byte_ctr < ilen; ++byte_ctr) {
        bool first = byte_ctr == 0;
        bool last = byte_ctr == ilen - 1;
        
        lgLogDebug("I2C_read: check write");
        while (!i2c_get_write_available(i2c)) {
            tight_loop_contents();
        }

        i2c->hw->data_cmd =
                bool_to_bit(first && i2c->restart_on_next) << I2C_IC_DATA_CMD_RESTART_LSB |
                bool_to_bit(last && !nostop) << I2C_IC_DATA_CMD_STOP_LSB |
                I2C_IC_DATA_CMD_CMD_BITS; // -> 1 for read

        lgLogDebug("I2C_read: set I2C_cmd");
        do {
            abort_reason = i2c->hw->tx_abrt_source;
            abort = (bool) i2c->hw->clr_tx_abrt;
        }
        while (!abort && !i2c_get_read_available(i2c));

        if (abort) {
            lgLogError("Abort: Rn:0x%08X\n", abort_reason);
            break;
        }

        b = (uint8_t)i2c->hw->data_cmd;

        lgLogDebug("Read: 0x%02X", b);

        *dst++ = b;
    }

    int rval;

    if (abort) {
        if (!abort_reason || abort_reason & I2C_IC_TX_ABRT_SOURCE_ABRT_7B_ADDR_NOACK_BITS) {
            // No reported errors - seems to happen if there is nothing connected to the bus.
            // Address byte not acknowledged
            rval = PICO_ERROR_GENERIC;
        }
        else {
            rval = PICO_ERROR_GENERIC;
        }
    }
    else {
        rval = byte_ctr;
    }

    i2c->restart_on_next = nostop;
    
    return rval;
}

int _i2c_write_blocking_debug(i2c_inst_t * i2c, uint8_t addr, const uint8_t * src, size_t len, bool nostop) {
    bool            abort = false;
    uint32_t        abort_reason = 0;
    int             byte_ctr;
    int             ilen = (int)len;

    i2c->hw->enable = 0;
    i2c->hw->tar = addr;
    i2c->hw->enable = 1;

    for (byte_ctr = 0; byte_ctr < ilen; ++byte_ctr) {
        bool first = byte_ctr == 0;
        bool last = byte_ctr == ilen - 1;

        i2c->hw->data_cmd =
                bool_to_bit(first && i2c->restart_on_next) << I2C_IC_DATA_CMD_RESTART_LSB |
                bool_to_bit(last && !nostop) << I2C_IC_DATA_CMD_STOP_LSB |
                *src++;

        // Wait until the transmission of the address/data from the internal
        // shift register has completed. For this to function correctly, the
        // TX_EMPTY_CTRL flag in IC_CON must be set. The TX_EMPTY_CTRL flag
        // was set in i2c_init.
        do {
            tight_loop_contents();
        }
        while (!(i2c->hw->raw_intr_stat & I2C_IC_RAW_INTR_STAT_TX_EMPTY_BITS));

        // If there was a timeout, don't attempt to do anything else.
        if (true) {
            abort_reason = i2c->hw->tx_abrt_source;

            if (abort_reason) {
                // Note clearing the abort flag also clears the reason, and
                // this instance of flag is clear-on-read! Note also the
                // IC_CLR_TX_ABRT register always reads as 0.
                i2c->hw->clr_tx_abrt;
                abort = true;
            }

            if (abort || (last && !nostop)) {
                // If the transaction was aborted or if it completed
                // successfully wait until the STOP condition has occured.

                // TODO Could there be an abort while waiting for the STOP
                // condition here? If so, additional code would be needed here
                // to take care of the abort.
                do {
                    tight_loop_contents();
                }
                while (!(i2c->hw->raw_intr_stat & I2C_IC_RAW_INTR_STAT_STOP_DET_BITS));

                i2c->hw->clr_stop_det;
            }
        }

        // Note the hardware issues a STOP automatically on an abort condition.
        // Note also the hardware clears RX FIFO as well as TX on abort,
        // because we set hwparam IC_AVOID_RX_FIFO_FLUSH_ON_TX_ABRT to 0.
        if (abort) {
            break;
        }
    }

    int rval;

    // A lot of things could have just happened due to the ingenious and
    // creative design of I2C. Try to figure things out.
    if (abort) {
        if (!abort_reason || abort_reason & I2C_IC_TX_ABRT_SOURCE_ABRT_7B_ADDR_NOACK_BITS) {
            // No reported errors - seems to happen if there is nothing connected to the bus.
            // Address byte not acknowledged
            rval = PICO_ERROR_GENERIC;
        }
        else if (abort_reason & I2C_IC_TX_ABRT_SOURCE_ABRT_TXDATA_NOACK_BITS) {
            // Address acknowledged, some data not acknowledged
            rval = byte_ctr;
        }
        else {
            rval = PICO_ERROR_GENERIC;
        }
    }
    else {
        rval = byte_ctr;
    }

    // nostop means we are now at the end of a *message* but not the end of a *transfer*
    i2c->restart_on_next = nostop;

    return rval;
}

int i2cWriteRegister(
            i2c_inst_t *i2c, 
            const uint addr, 
            const uint8_t reg, 
            uint8_t * data, 
            const uint8_t length)
{
    int             bytesWritten = 0;
    uint8_t         msg[length + 1];

    msg[0] = reg;
    memcpy(&msg[1], data, length);

    bytesWritten = _i2c_write_blocking_debug(i2c, addr, msg, (length + 1), false);

    return bytesWritten;
}

int i2cReadRegister(
            i2c_inst_t *i2c, 
            const uint addr, 
            const uint8_t reg, 
            uint8_t * data, 
            const uint8_t length)
{
   int      bytesRead = 0;

    // Read data from register(s) over I2C
    _i2c_write_blocking_debug(i2c, addr, &reg, 1, true);
    bytesRead = _i2c_read_blocking_debug(i2c, addr, data, length, false);

    return bytesRead;
}
