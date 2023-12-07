#include "scheduler.h"

#ifndef __INCL_BATTERY
#define __INCL_BATTERY

#define BATTERY_PERCENTAGE_CRITICAL         110
#define BATTERY_PERCENTAGE_LOW              400
#define BATTERY_PERCENTAGE_OK               600
#define BATTERY_TEMPERATURE_CRITICAL       5120         // 40 degrees C
#define BATTERY_TEMPERATURE_LIMIT          4800         // 37.5 degress C

void taskBatteryMonitor(PTASKPARM p);

#endif
