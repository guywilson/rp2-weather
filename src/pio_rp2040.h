#include <stdint.h>

#include "scheduler.h"

#ifndef __INCL_PIO_RP2040
#define __INCL_PIO_RP2040

void        pioInit(void);
void        disablePIO(void);
void        taskAnemometer(PTASKPARM p);
void        taskRainGuage(PTASKPARM p);

#endif
