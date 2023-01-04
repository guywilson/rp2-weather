#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "scheduler.h"
#include "taskdef.h"
#include "spi_rp2040.h"

static inline void chipSelect(uint pin) {
    asm volatile("nop \n nop \n nop");
    gpio_put(pin, 0);
    asm volatile("nop \n nop \n nop");
}

static inline void chipDeselect(uint pin) {
    asm volatile("nop \n nop \n nop");
    gpio_put(pin, 1);
    asm volatile("nop \n nop \n nop");
}

int spiWriteByte(spi_inst_t * spi, uint csPin, uint8_t data, bool noDeselect) {
    int         bytesWritten;

    chipSelect(csPin);
    bytesWritten = spi_write_blocking(spi, &data, 1);

    if (!noDeselect) {
        chipDeselect(csPin);
    }

    return bytesWritten;
}

int spiWriteReadByte(spi_inst_t * spi, uint csPin, uint8_t src, uint8_t * dst, bool noDeselect) {
    int         bytesWritten;

    chipSelect(csPin);
    bytesWritten = spi_write_read_blocking(spi, &src, dst, 1);

    if (!noDeselect) {
        chipDeselect(csPin);
    }

    return bytesWritten;
}

int spiWriteWord(spi_inst_t * spi, uint csPin, uint16_t data, bool noDeselect) {
    int         bytesWritten;
    uint8_t     buf[2];

    buf[0] = data & 0x00FF;
    buf[1] = (data & 0xFF00) >> 8;

    chipSelect(csPin);
    bytesWritten = spi_write_blocking(spi, buf, 2);

    if (!noDeselect) {
        chipDeselect(csPin);
    }

    return bytesWritten;
}

int spiWriteReadWord(spi_inst_t * spi, uint csPin, uint16_t src, uint16_t * dst, bool noDeselect) {
    int         bytesWritten;
    uint8_t     srcBuf[2];
    uint8_t     dstBuf[2];

    srcBuf[0] = src & 0x00FF;
    srcBuf[1] = (src & 0xFF00) >> 8;

    chipSelect(csPin);
    bytesWritten = spi_write_read_blocking(spi, srcBuf, dstBuf, 2);

    *dst = ((uint16_t)(dstBuf[1]) << 8) | (uint16_t)dstBuf[0];

    if (!noDeselect) {
        chipDeselect(csPin);
    }

    return bytesWritten;
}

int spiWriteData(spi_inst_t * spi, uint csPin, uint8_t * data, int length, bool noDeselect) {
    int         bytesWritten;
    uint8_t     buf[2];

    chipSelect(csPin);
    bytesWritten = spi_write_blocking(spi, data, length);

    if (!noDeselect) {
        chipDeselect(csPin);
    }

    return bytesWritten;
}

int spiWriteReadData(spi_inst_t * spi, uint csPin, uint8_t * src, uint8_t * dst, int length, bool noDeselect) {
    int         bytesWritten;
    uint8_t     buf[2];

    chipSelect(csPin);
    bytesWritten = spi_write_read_blocking(spi, src, dst, length);

    if (!noDeselect) {
        chipDeselect(csPin);
    }

    return bytesWritten;
}

int spiReadByte(spi_inst_t * spi, uint csPin, uint8_t data, bool noDeselect) {
    int         bytesRead;

    chipSelect(csPin);
    bytesRead = spi_read_blocking(spi, 0x00, &data, 1);

    if (!noDeselect) {
        chipDeselect(csPin);
    }

    return bytesRead;
}

int spiReadWord(spi_inst_t * spi, uint csPin, uint16_t data, bool noDeselect) {
    int         bytesRead;
    uint8_t     buf[2];

    buf[0] = (uint8_t)(data & 0x00FF);
    buf[1] = (uint8_t)((data >> 8) & 0x00FF);

    chipSelect(csPin);
    bytesRead = spi_read_blocking(spi, 0x00, buf, 2);

    if (!noDeselect) {
        chipDeselect(csPin);
    }

    return bytesRead;
}

int spiReadData(spi_inst_t * spi, uint csPin, uint8_t * data, int length, bool noDeselect) {
    int         bytesRead;

    chipSelect(csPin);
    bytesRead = spi_read_blocking(spi, 0x00, &data, length);

    if (!noDeselect) {
        chipDeselect(csPin);
    }

    return bytesRead;
}
