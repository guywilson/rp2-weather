#include <stdint.h>
#include <stdbool.h>
#include "pico/stdlib.h"

#ifndef __INCL_UTILS
#define __INCL_UTILS

#define		LED_ONBOARD			PICO_DEFAULT_LED_PIN

void		    setupLEDPin(void);
void		    turnOn(int LED_ID);
void		    turnOff(int LED_ID);
void 		    toggleLED(int LED_ID);
int16_t         copyI2CReg_int16(uint8_t * reg);
uint16_t        copyI2CReg_uint16(uint8_t * reg);

#endif
