#include <stdint.h>

#ifdef PICO_MULTICORE
#include "pico/stdlib.h"
#endif

#ifndef _INCL_SCHEDULER
#define _INCL_SCHEDULER

#define MAX_INT_SIZE			64

// #if defined ( __GNUC__ ) && defined ( __AVR__ )
// #define MAX_INT_SIZE			32
// #define CHECK_TIMER_OVERFLOW
// #else
// #define MAX_INT_SIZE			64
// #endif

#define DEFAULT_MAX_TASKS       64

typedef void *					PTASKPARM;

#if MAX_INT_SIZE == 64
typedef uint64_t				rtc_t;
#elif MAX_INT_SIZE == 32
typedef uint32_t				rtc_t;
#else
typedef uint32_t				rtc_t;
#endif

#define MAX_TIMER_VALUE			~((rtc_t)0)

#ifdef UNIT_TEST_MODE
/******************************************************************************
**
** The TASKDESC struct.
**
** Holds the per-task info for the scheduler.
**
******************************************************************************/
typedef struct
{
	uint16_t		ID;				// Unique user-assigned ID
	rtc_t			startTime;		// The RTC value when scheduleTask() was callled
	rtc_t			scheduledTime;	// The RTC value when the task should run
	rtc_t			delay;			// The requested delay (in ms) of when the task should run
	uint8_t			priority;		// Task priority 0 (highest) to 5 (lowest)
	uint8_t			type;			// Task type - Periodic, On-demand
	uint8_t			isScheduled;	// Is this task scheduled
	uint8_t			isAllocated;	// Is this allocated to a task
	uint8_t			isPeriodic;		// Should this task run repeatdly at the specified delay
	PTASKPARM		pParameter;		// The parameters to the task
	
	void (* run)(PTASKPARM);		// Pointer to the task function to run
									// Must be of the form void task(PTASKPARM p);
	void * 			next;
	void *			prev;
}
TASKDESC;

typedef TASKDESC *	PTASKDESC;

PTASKDESC 	getRegisteredTasks();
int 		isLastTask(PTASKDESC td);
#endif

typedef struct
{
	uint32_t		idleCount;
	uint32_t		busyCount;
}
CPU_RATIO;

typedef CPU_RATIO *	PCPU_RATIO;

#ifdef PICO_MULTICORE
#define getCoreID()						(uint8_t)(sio_hw->cpuid & 0xFF)
#endif
/*
** Passed as the time parameter to scheduleTask.
*/
#define RUN_NOW                 0

/*
 * If, for example, you want a faster interrupt frequency for the
 * RTC tick task, set the prescaler here. If you want the interrupt
 * frequency and clock frequency to be the same, simply set this to 1...
 */
#define RTC_INTERRUPT_PRESCALER			1

/******************************************************************************
**
** The real-time clock interrupt service routing
**
******************************************************************************/
void        _rtcISR();

/******************************************************************************
**
** Get the last recorded busy/idle CPU counts
**
******************************************************************************/
void getCPURatio(PCPU_RATIO cpuRatio);

/******************************************************************************
**
** Get the total number of tasks run by the scheduler
**
******************************************************************************/
uint32_t getTaskRunCount();

/******************************************************************************
**
** Get the scheduler version string and build date
**
******************************************************************************/
const char * getSchedulerVersion();
const char * getSchedulerBuildDate();

/******************************************************************************
**
** The scheduler API
**
******************************************************************************/
void 		initScheduler(int size);

void        registerTickTask(void (* tickTask)());

void		registerTask(uint16_t taskID, void (* run)(PTASKPARM));
void		deregisterTask(uint16_t taskID);

void        scheduleTask(uint16_t taskID, rtc_t time, PTASKPARM p);
void        scheduleTaskOnce(uint16_t taskID, rtc_t time, PTASKPARM p);
void		rescheduleTask(uint16_t taskID, PTASKPARM p);
void		unscheduleTask(uint16_t taskID);

void		schedule();

#endif
