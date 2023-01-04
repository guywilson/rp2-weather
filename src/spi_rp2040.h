#ifndef __INCL_SPI_RP2040
#define __INCL_SPI_RP2040

int spiWriteByte(spi_inst_t * spi, uint csPin, uint8_t data, bool noDeselect);
int spiWriteReadByte(spi_inst_t * spi, uint csPin, uint8_t src, uint8_t * dst, bool noDeselect);
int spiWriteWord(spi_inst_t * spi, uint csPin, uint16_t data, bool noDeselect);
int spiWriteReadWord(spi_inst_t * spi, uint csPin, uint16_t src, uint16_t * dst, bool noDeselect);
int spiWriteData(spi_inst_t * spi, uint csPin, uint8_t * data, int length, bool noDeselect);
int spiWriteReadData(spi_inst_t * spi, uint csPin, uint8_t * src, uint8_t * dst, int length, bool noDeselect);
int spiReadByte(spi_inst_t * spi, uint csPin, uint8_t data, bool noDeselect);
int spiReadWord(spi_inst_t * spi, uint csPin, uint16_t data, bool noDeselect);
int spiReadData(spi_inst_t * spi, uint csPin, uint8_t * data, int length, bool noDeselect);

#endif
