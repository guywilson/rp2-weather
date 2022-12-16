#include "pico/stdlib.h"

#ifndef _INCL_LEDUTILS
#define _INCL_LEDUTILS

#define		LED_ONBOARD			PICO_DEFAULT_LED_PIN

void		setupLEDPin(void);
void		turnOn(int LED_ID);
void		turnOff(int LED_ID);
void 		toggleLED(int LED_ID);

#endif