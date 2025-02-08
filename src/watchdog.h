#include "scheduler.h"

#ifndef __INCL_WATCHDOG
#define __INCL_WATCHDOG

void triggerWatchdogReset(void);
void taskWatchdog(PTASKPARM p);

#endif
