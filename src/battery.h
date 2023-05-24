#include "scheduler.h"

#ifndef __INCL_BATTERY
#define __INCL_BATTERY

#define BATTERY_PERCENTAGE_CRITICAL         200
#define BATTERY_TEMPERATURE_CRITICAL       3182         // 45 degrees C
#define BATTERY_TEMPERATURE_LIMIT          3132         // 40 degress C

void initBattery();
void taskBatteryMonitor(PTASKPARM p);

#endif
