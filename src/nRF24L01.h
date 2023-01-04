#ifndef __INCL_NRF24L01
#define __INCL_NRF24L01

int nRF24L01_setup(spi_inst_t * spi, uint csPin);
int nRF24L01_transmit(spi_inst_t * spi, uint csPin, uint8_t * buffer, int length);
int nRF24L01_receive(spi_inst_t * spi, uint csPin, uint8_t * buffer, int length);

#endif
