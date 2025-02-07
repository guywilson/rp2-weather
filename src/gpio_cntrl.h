#include "gpio_def.h"

#ifndef __INCL_GPIO_CNTRL
#define __INCL_GPIO_CNTRL

void initGPIOs(void);
void initDebugPins(void);
void deInitGPIOs(void);
void deInitGPIOsAndDebugPins(void);

#endif
