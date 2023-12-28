#include <stdint.h>
#include <stdbool.h>
#include "pico/stdlib.h"

#ifndef __INCL_UTILS
#define __INCL_UTILS

void		    setupLEDPin(void);
void            setupDebugPin(void);
void		    turnOn(int LED_ID);
void		    turnOff(int LED_ID);
void 		    toggleLED(int LED_ID);
bool            isDebugActive(void);
int16_t         copyI2CReg_int16(uint8_t * reg);
uint16_t        copyI2CReg_uint16(uint8_t * reg);

#endif
