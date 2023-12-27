#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/irq.h"
#include "hardware/clocks.h"
#include "hardware/resets.h"
#include "logger.h"
#include "scheduler.h"
#include "taskdef.h"
#include "rtc_rp2040.h"
#include "i2c_rp2040.h"
#include "gpio_def.h"

#define I2C_BUS_MIN_DEVICES              1
#define I2C_BUS_MAX_DEVICES              8
#define I2C_TIMEOUT_US                2500

static i2c_device_t         devices_i2c0[I2C_BUS_MAX_DEVICES];
static i2c_device_t         devices_i2c1[I2C_BUS_MAX_DEVICES];

static int                  numDevicesOnBus[2];

static i2c_device_t * i2cGetDeviceByAddress(i2c_inst_t * i2c, uint address) {
    i2c_device_t *          devices;
    i2c_device_t *          device;
    int                     numDevices;
    int                     i;

    if (i2c == i2c0) {
        devices = devices_i2c0;
        numDevices = numDevicesOnBus[0];
    }
    else if (i2c == i2c1) {
        devices = devices_i2c1;
        numDevices = numDevicesOnBus[1];
    }
    else {
        return NULL;
    }

    for (i = 0;i < numDevices;i++) {
        device = &devices[i];

        if (device->address == address) {
            return device;
        }
    }

    return NULL;
}

static void i2cSetDeviceState(i2c_inst_t * i2c, uint address, bool state) {
    i2c_device_t *          device;

    device = i2cGetDeviceByAddress(i2c, address);

    if (device != NULL) {
        device->isActive = state;
        device->lastStateTime = getRTCClock();
    }
}

static bool i2cIsDeviceActive(i2c_inst_t * i2c, uint address) {
    int             error;
    uint8_t         rxData;

    error = i2c_read_timeout_us(i2c, address, &rxData, 1, false, I2C_TIMEOUT_US);

    if (error == PICO_ERROR_GENERIC || error == PICO_ERROR_TIMEOUT) {
        return false;
    }

    return true;
}

bool i2cGetDeviceState(i2c_inst_t * i2c, uint address) {
    i2c_device_t *          device;

    device = i2cGetDeviceByAddress(i2c, address);

    if (device != NULL) {
        return device->isActive;
    }

    return false;
}

int i2c_bus_open(i2c_inst_t * i2c, int numDevices) {
    i2c_device_t *          devices;

    if (numDevices < I2C_BUS_MIN_DEVICES || numDevices > I2C_BUS_MAX_DEVICES) {
        return -1;
    }

    if (i2c == i2c0) {
        devices = devices_i2c0;
        numDevicesOnBus[0] = numDevices;
    }
    else if (i2c == i2c1) {
        devices = devices_i2c1;
        numDevicesOnBus[1] = numDevices;
    }
    else {
        return -1;
    }

    if (devices == NULL) {
        return -1;
    }

    memset(devices, 0, sizeof(i2c_device_t) * numDevices);

    return 0;
}

int i2c_bus_close(i2c_inst_t * i2c) {
    if (i2c == i2c0) {
        numDevicesOnBus[0] = 0;
    }
    else if (i2c == i2c1) {
        numDevicesOnBus[1] = 0;
    }
    else {
        return -1;
    }

    return 0;
}

int i2c_bus_register_device(i2c_inst_t * i2c, const uint address, int (* setup)(i2c_inst_t *)) {
    static int              busIndex0 = 0;
    static int              busIndex1 = 0;
    int *                   busIndex;
    int                     numDevices;
    i2c_device_t *          devices;

    if (i2c == i2c0) {
        devices = devices_i2c0;
        busIndex = &busIndex0;
        numDevices = numDevicesOnBus[0];
    }
    else if (i2c == i2c1) {
        devices = devices_i2c1;
        busIndex = &busIndex1;
        numDevices = numDevicesOnBus[1];
    }
    else {
        return -1;
    }

    if (*busIndex >= numDevices) {
        return PICO_ERROR_GENERIC;
    }

    devices[*busIndex].address = address;
    devices[*busIndex].isActive = true;
    devices[*busIndex].lastStateTime = 0;
    devices[*busIndex].setup = setup;

    (*busIndex)++;

    return 0;
}

int i2c_bus_setup(i2c_inst_t * i2c) {
    i2c_device_t *          devices;
    int                     i;
    int                     error = 0;
    int                     numDevices;

    if (i2c == i2c0) {
        devices = devices_i2c0;
        numDevices = numDevicesOnBus[0];
    }
    else if (i2c == i2c1) {
        devices = devices_i2c1;
        numDevices = numDevicesOnBus[1];
    }
    else {
        return -1;
    }

    for (i = 0;i < numDevices;i++) {
        error |= devices[i].setup(i2c);
    }

    return error;
}

int i2cReadTimeoutProtected(
                i2c_inst_t * i2c, 
                const uint address, 
                uint8_t * dst, 
                size_t len, 
                bool nostop)
{
    int             error;
    i2c_device_t *  device;

    if (i2cGetDeviceState(i2c, address)) {
        error = i2c_read_timeout_us(i2c, address, dst, len, nostop, I2C_TIMEOUT_US);

        if (error == PICO_ERROR_GENERIC) {
            i2cSetDeviceState(i2c, address, false);
        }
    }
    else {
        device = i2cGetDeviceByAddress(i2c, address);

        if (getRTCClock() > (device->lastStateTime + rtc_val_sec(5))) {
            error = i2c_read_timeout_us(i2c, address, dst, len, nostop, I2C_TIMEOUT_US);

            if (error == PICO_ERROR_GENERIC) {
                /*
                ** This will reset the clock so we don't 
                ** immediately try again
                */
                i2cSetDeviceState(i2c, address, false);
            }
            else {
                i2cSetDeviceState(i2c, address, true);
            }
        }
        else {
            error = PICO_ERROR_GENERIC;
        }
    }

    return error;
}

int i2cWriteTimeoutProtected(
                i2c_inst_t * i2c, 
                const uint address, 
                uint8_t * src, 
                size_t len, 
                bool nostop)
{
    int             error;
    i2c_device_t *  device;

    if (i2cGetDeviceState(i2c, address)) {
        error = i2c_write_timeout_us(i2c, address, src, len, nostop, I2C_TIMEOUT_US);

        switch (error) {
            case PICO_ERROR_GENERIC:
                lgLogError("Generic I2C error writing to addr: 0x%02X", address);
                i2cSetDeviceState(i2c, address, false);
                break;

            case PICO_ERROR_TIMEOUT:
                lgLogError("I2C timeout writing to addr: 0x%02X", address);
                break;

            default:
                lgLogDebug("Written %d bytes to addr: 0x%02X", error, address);
                break;
        }
    }
    else {
        device = i2cGetDeviceByAddress(i2c, address);

        if (getRTCClock() > (device->lastStateTime + rtc_val_sec(5))) {
            error = i2c_write_timeout_us(i2c, address, src, len, nostop, I2C_TIMEOUT_US);

            switch (error) {
                case PICO_ERROR_GENERIC:
                    lgLogError("Generic I2C error writing to addr: 0x%02X", address);
                    /*
                    ** This will reset the clock so we don't 
                    ** immediately try again
                    */
                    i2cSetDeviceState(i2c, address, false);
                    break;

                case PICO_ERROR_TIMEOUT:
                    lgLogError("I2C timeout writing to addr: 0x%02X", address);
                    i2cSetDeviceState(i2c, address, true);
                    break;

                default:
                    lgLogDebug("Written %d bytes to addr: 0x%02X", error, address);
                    i2cSetDeviceState(i2c, address, true);
                    break;
            }
        }
        else {
            error = PICO_ERROR_GENERIC;
        }
    }

    return error;
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

    bytesWritten = i2c_write_timeout_us(i2c, addr, msg, (length + 1), false, I2C_TIMEOUT_US);

    switch (bytesWritten) {
        case PICO_ERROR_GENERIC:
            lgLogError("Generic I2C error writing to addr: 0x%02X", addr);
            break;

        case PICO_ERROR_TIMEOUT:
            lgLogError("I2C timeout writing to addr: 0x%02X", addr);
            break;

        default:
            lgLogDebug("Written %d bytes to addr: 0x%02X", bytesWritten, addr);
            break;
    }

    return bytesWritten;
}

int i2cReadRegister(
            i2c_inst_t *i2c, 
            const uint addr, 
            const uint8_t reg, 
            uint8_t * data, 
            const uint8_t length)
{
    int     bytesRead = 0;
    int     error = 0;
    uint8_t regid = reg;

    // Read data from register(s) over I2C
    error = i2c_write_timeout_us(i2c, addr, &regid, 1, true, I2C_TIMEOUT_US);

    switch (error) {
        case PICO_ERROR_GENERIC:
            lgLogError("Generic I2C error writing to addr: 0x%02X", addr);
            break;

        case PICO_ERROR_TIMEOUT:
            lgLogError("I2C timeout writing to addr: 0x%02X", addr);
            break;

        default:
            lgLogDebug("Written %d bytes to addr: 0x%02X", error, addr);
            break;
    }

    bytesRead = i2cReadTimeoutProtected(i2c, addr, data, length, false);

    switch (bytesRead) {
        case PICO_ERROR_GENERIC:
            lgLogError("Generic I2C error reading from addr: 0x%02X", addr);
            break;

        case PICO_ERROR_TIMEOUT:
            lgLogError("I2C timeout reading from addr: 0x%02X", addr);
            break;

        default:
            lgLogDebug("Read %d bytes from addr: 0x%02X", bytesRead, addr);
            break;
    }

    return bytesRead;
}
