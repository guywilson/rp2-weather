/******************************************************************************
**
** File: scheduler.c
**
** Description: API functions for the real-time scheduler. Inspired by a
** distant memory of a scheduler I used circa 1996
**
** Copyright: Guy Wilson (c) 2018
**
******************************************************************************/

//#define PICO_MULTICORE

#include <stddef.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#ifdef UNIT_TEST_MODE
#include <stdio.h>
#include <unistd.h>
#endif

#ifdef PICO_MULTICORE
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "hardware/irq.h"
#include "hardware/uart.h"
#include "hardware/gpio.h"
#include "hardware/structs/sio.h"
#endif

#include "scheduler.h"
#include "schederr.h"
#include "gpio_def.h"

#define TRACK_CPU_PCT

#ifndef UNIT_TEST_MODE
/******************************************************************************
**
** The TASKDESC struct.
**
** Holds the per-task info for the scheduler.
**
******************************************************************************/
typedef struct
{
	uint16_t		ID;					// Unique user-assigned ID
	rtc_t			startTime;			// The RTC value when scheduleTask() was callled
	rtc_t			scheduledTime;		// The RTC value when the task should run
	rtc_t			delay;				// The requested delay (in ms) of when the task should run
	uint8_t			isScheduled;		// Is this task scheduled
	uint8_t			isAllocated;		// Is this allocated to a task
	uint8_t			isPeriodic;			// Should this task run repeatdly at the specified delay
	PTASKPARM		pParameter;			// The parameters to the task

	void (* run)(PTASKPARM);			// Pointer to the task function to run
										// Must be of the form void task(uint8_t core, PTASKPARM p);

#ifdef PICO_MULTICORE
    bool            forceCore0;         // Always run this task on core0 if true
#endif

	void * 			next;
	void *			prev;
}
TASKDESC;

typedef TASKDESC *	PTASKDESC;
#endif

/******************************************************************************
**
** Name: _nullTask()
**
** Description: Null task assigned to TASKDESC entries that are unused.
**
******************************************************************************/
void _nullTask(PTASKPARM p)
{
	handleError(ERROR_SCHED_NULLTASKEXEC);
}

/******************************************************************************
**
** Name: _nullTickTask()
**
** Description: Null tick task.
**
** Parameters:	N/A
**
** Returns:		void 
**
******************************************************************************/
void _nullTickTask()
{
	// Do nothing...
}

static PTASKDESC			taskDescs;				// Array of tasks for the scheduler, allocated in initScheduler()
static int					taskArrayLength;		// Length of the task array, e.g. num tasks allocated

static int					taskCount = 0;			// Number of tasks registered

static PTASKDESC			head = NULL;			// Pointer to the beginning of the registered task queue
static PTASKDESC			tail = NULL;			// Pointer to the end of the registered task queue

static uint32_t				_tasksRunCount = 0;		// The total number of tasks run by the scheduler

static volatile rtc_t 	    _realTimeClock = 0;		// The real time clock counter
static volatile uint16_t	_tickCount = 0;			// Num ticks between rtc counts

static volatile uint32_t	_busyCount = 0;
static volatile uint32_t	_idleCount = 0;

// The RTC tick task...
void 						(* _tickTask)() = &_nullTickTask;

#ifdef PICO_MULTICORE
#define CORE1_TASKDESC_POOL_SIZE                5
volatile bool				isCoreOneBusy = false;
PTASKDESC                   core1TaskDescPool;
#else
#define CORE1_TASKDESC_POOL_SIZE                0
#endif

#define getRTCClockCount()	(_realTimeClock)

#ifdef TRACK_CPU_PCT
#define signalBusy()		_busyCount++
#define signalIdle()		_idleCount++
#else
#define signalBusy()		// Do nothing
#define signalIdle()		// Do nothing
#endif

/******************************************************************************
**
** Name: _rtcISR()
**
** Description: The RTC interrupt handler.
**
** Parameters:	N/A
**
** Returns:		void 
**
******************************************************************************/
void _rtcISR() {
	_realTimeClock++;

#ifdef SCHED_ENABLE_TICK_TASK
	/*
	 * Run the tick task, defaults to the nullTick() function.
	 *
	 * This must be a very fast operation, as it is outside of
	 * the scheduler's control. Also, there can be only 1 tick task...
	 */
	_tickTask();
#endif
}

/******************************************************************************
**
** Name: _getScheduledTime()
**
** Description: Calculates the future RTC value when the task should run
**
** Parameters:
**				rtc_t		startTime		The RTC value when called
**				rtc_t		requestedDelay	The delay in ms before the task runs
**
** Returns: 
**				rtc_t		The future RTC value when the task should run
**
** If we don't care about checking if the timer will overflow, use a macro
** to calculate the default scheduled time (with risk of overflow). 
**
** With a timer incrementing every 1ms:
**
** A 32-bit timer will overflow in approximately 50 days. 
** A 64-bit timer will overflow in approximately 585 million years!
**
******************************************************************************/
#ifdef CHECK_TIMER_OVERFLOW
static rtc_t _getScheduledTime(rtc_t startTime, rtc_t requestedDelay)
{
	rtc_t			overflowTime;
	
	overflowTime = MAX_TIMER_VALUE - startTime;
	
	if (overflowTime < requestedDelay) {
		return (requestedDelay - overflowTime);
	}
	else {
		return (startTime + requestedDelay);
	}
}
#else
#define _getScheduledTime(startTime, requestedDelay)	(startTime + requestedDelay)
#endif

#ifdef PICO_MULTICORE
void _core1_main(void)
{
	PTASKDESC			td = NULL;
	uint32_t			data = 0;

	while (1) {
        data = multicore_fifo_pop_blocking();
    
        td = (PTASKDESC)(data);

        /*
        ** Run the task here (on core 1)...
        */
        isCoreOneBusy = true;
        td->run(td->pParameter);
        td->isAllocated = 0;
        isCoreOneBusy = false;
	}
}
#endif

/******************************************************************************
**
** Name: _findTaskByID()
**
** Description: Returns a pointer to the task identifed by the supplied ID
**
** Parameters:
**				uint16_t	taskID			The taskID to find
**
** Returns: 
**				PTASKDESC	The pointer to the task, can be NULL
**
******************************************************************************/
static PTASKDESC _findTaskByID(uint16_t taskID)
{
	int			i = 0;
	PTASKDESC	td = NULL;

	for (i = 0;i < taskArrayLength;i++) {
		td = &taskDescs[i];
		
		if (td->ID == taskID) {
			break;
		}
	}

	return td;
}

#ifdef UNIT_TEST_MODE
PTASKDESC getRegisteredTasks()
{
	return head;
}

int isLastTask(PTASKDESC td)
{
	if (td == tail) {
		return 1;
	}
	else {
		return 0;
	}
}
#endif

/******************************************************************************
**
** Public API functions
**
******************************************************************************/

/******************************************************************************
**
** Name: getRTCClock()
**
** Description: Returns the current RTC clock value.
**
** Parameters:	None
**
** Returns:		rtc_t 
**
******************************************************************************/
rtc_t getRTCClock(void) {
    return getRTCClockCount();
}

/******************************************************************************
**
** Name: getCPURatio()
**
** Description: Returns the busy & idle counts.
**
** Parameters:	uint32_t *		idleCount	A pointer to a 32-bit unsigned int
**				uint32_t *		busyCount	A pointer to a 32-bit unsigned int
**
** Returns:		void 
**
******************************************************************************/
void getCPURatio(PCPU_RATIO cpuRatio)
{
	/*
	** busy/idle ratio = busyCount / (idleCount - busyCount)
	*/
	cpuRatio->idleCount = _idleCount - _busyCount;
	cpuRatio->busyCount = _busyCount;

	// Reset counters...
	_busyCount = 0;
	_idleCount = 0;
}

/******************************************************************************
**
** Name: getTaskRunCount()
**
** Description: Gets the total number of tasks run by the scheduler, useful
**				for a dashboard for example.
**
** Parameters:	None
**
** Returns:		uint32_t		The number of tasks run 
**
******************************************************************************/
uint32_t getTaskRunCount()
{
	return _tasksRunCount;
}

/******************************************************************************
**
** Name: initScheduler()
**
** Description: Initialises the scheduler, must be called before any other
** scheduler API functions.
**
** Parameters:	int		size		Size of the task array
**
** Returns:		void 
**
******************************************************************************/
void initScheduler(int size)
{
	int			i = 0;
	PTASKDESC	td = NULL;

	if (size <= 0) {
		taskArrayLength = DEFAULT_MAX_TASKS;
	}
	else if (size > DEFAULT_MAX_TASKS) {
		taskArrayLength = DEFAULT_MAX_TASKS;
	}
	else {
		taskArrayLength = size;
	}

#ifdef UNIT_TEST_MODE
printf("Allocated %d tasks\n", taskArrayLength);
#endif

	/*
	** Allocate memory for task array...
	*/
	taskDescs = (PTASKDESC)malloc((taskArrayLength + CORE1_TASKDESC_POOL_SIZE) * sizeof(TASKDESC));

	if (taskDescs == NULL) {
		handleError(ERROR_SCHED_TASKCOUNTOVERFLOW);
	}

	taskCount = 0;
	
	for (i = 0;i < (taskArrayLength + CORE1_TASKDESC_POOL_SIZE);i++) {
		td = &taskDescs[i];
		
		td->ID				= 0;
		td->startTime		= 0;
		td->scheduledTime	= 0;
		td->delay			= 0;
		td->isScheduled		= 0;
		td->isAllocated		= 0;
		td->isPeriodic		= 0;
		td->pParameter		= NULL;
		td->run				= &_nullTask;

        td->forceCore0      = false;

		td->next			= NULL;
		td->prev			= NULL;
	}

#ifdef PICO_MULTICORE
    core1TaskDescPool = &taskDescs[taskArrayLength];

	/*
	** Initialise the 2nd core...
	*/
	multicore_reset_core1();
	multicore_fifo_drain();

	/*
	** Launch the main loop on the 2nd core...
	*/
	multicore_launch_core1(_core1_main);
#endif
}

/******************************************************************************
**
** Name: registerTickTask()
**
** Description: Registers the scheduler tick task. This tick task is called 
**				from the RTC ISR.
**
** Parameters:	void 	(* tickTask)	Pointer to the tick task function
**
** Returns:		void 
**
******************************************************************************/
void registerTickTask(void (* tickTask)())
{
	_tickTask = tickTask;
}

/******************************************************************************
**
** Name: registerTask()
**
** Description: Registers a task with the scheduler.
**
** Parameters:	uint16_t	taskID		A unique ID for the task
**				void 		(* run)		Pointer to the actual task function
**
** Returns:		void 
**
******************************************************************************/
void registerTask(uint16_t taskID, void (* run)(PTASKPARM))
{
	int			i;
	PTASKDESC	td = NULL;
	uint8_t		noFreeTasks = 1;

	for (i = 0;i < taskArrayLength;i++) {
		td = &taskDescs[i];

		if (!td->isAllocated) {
			td->ID = taskID;
			td->isAllocated = 1;
			td->run = run;

			taskCount++;
			noFreeTasks = 0;

			if (tail != NULL) {
				/*
				** Insert our task here (at the end of the queue)...
				*/
				td->prev = tail;
				td->next = head;
				tail->next = td;
				tail = td;
			}
			else {
				/*
				** This must be the first task...
				*/
				head = tail = td;

				/*
				** Point its next & prev ptrs to itself...
				*/
				head->next = head;
				head->prev = head;
			}
			break;
		}
	}
    
	if (noFreeTasks) {
		handleError(ERROR_SCHED_NOFREETASKS);
	}
	if (taskCount > taskArrayLength) {
		handleError(ERROR_SCHED_TASKCOUNTOVERFLOW);
	}
}

/******************************************************************************
**
** Name: deregisterTask()
**
** Description: De-registers a task with the scheduler, freeing up the task
** definition.
**
** Parameters:	uint16_t	taskID		The unique ID for the task
**
** Returns:		void 
**
******************************************************************************/
void deregisterTask(uint16_t taskID)
{
	PTASKDESC	td = NULL;

	td = _findTaskByID(taskID);

	if (td != NULL) {
		td->ID				= 0;
		td->startTime		= 0;
		td->scheduledTime	= 0;
		td->delay			= 0;
		td->isScheduled		= 0;
		td->isAllocated		= 0;
		td->pParameter		= NULL;
		td->run				= &_nullTask;
		
		taskCount--;

		/*
		** Unlink the task from the registered queue...
		*/
		((PTASKDESC)(td->prev))->next = td->next;
		((PTASKDESC)(td->next))->prev = td->prev;

		if (head == tail) {
			head = tail = NULL;
		}
	}
}

/******************************************************************************
**
** Name: scheduleTask()
**
** Description: Schedules the task to run after the specified delay. A task
** must be registered using registerTask() before it can be scheduled.
**
** Parameters:	
** uint16_t		taskID		The unique ID for the task
** rtc_t		time		Number of ms in the future for the task to run
** uint8_t		priority	The priority of the task
** PTASKPARM	p			Pointer to the task parameters, can be NULL
**
** Returns:		void 
**
******************************************************************************/
void scheduleTask(uint16_t taskID, rtc_t time, bool isPeriodic, PTASKPARM p)
{
	PTASKDESC	td = NULL;

	td = _findTaskByID(taskID);

	if (td != NULL) {
		td->startTime = getRTCClockCount();
		td->delay = time;
		td->scheduledTime = _getScheduledTime(td->startTime, td->delay);
		td->isScheduled = 1;
		td->isPeriodic = (uint8_t)isPeriodic;
		td->pParameter = p;
	}
}

/******************************************************************************
**
** Name: rescheduleTask()
**
** Description: Reschedules the task to run again after the same delay as
** specified in the scheduleTask() call. Useful for calling at the end of
** the task itself to force it to run again (after the specified delay).
**
** If you want to reschedule the task to run after a different delay, simply
** call scheduleTask() instead.
**
** Parameters:	
** uint16_t		taskID		The unique ID for the task
** PTASKPARM	p			Pointer to the task parameters, can be NULL
**
** Returns:		void 
**
******************************************************************************/
void rescheduleTask(uint16_t taskID, PTASKPARM p)
{
	PTASKDESC	td = NULL;

	td = _findTaskByID(taskID);

	if (td != NULL) {
		td->startTime = getRTCClockCount();
		td->scheduledTime = _getScheduledTime(td->startTime, td->delay);
		td->isScheduled = 1;
		td->pParameter = p;
	}
}

/******************************************************************************
**
** Name: unscheduleTask()
**
** Description: Un-schedules a task that has previously been scheduled, e.g. this
** will cancel the scheduled run.
**
** Parameters:	
** uint16_t		taskID		The unique ID for the task
**
** Returns:		void 
**
******************************************************************************/
void unscheduleTask(uint16_t taskID)
{
	PTASKDESC	td = NULL;

	td = _findTaskByID(taskID);

	if (td != NULL) {
		td->startTime = 0;
		td->scheduledTime = 0;
		td->isScheduled = 0;
		td->pParameter = NULL;
	}
}

#ifdef PICO_MULTICORE
/******************************************************************************
**
** Name: setTaskAttributes()
**
** Description: Un-schedules a task that has previously been scheduled, e.g. this
** will cancel the scheduled run.
**
** Parameters:	
** uint16_t		taskID		The unique ID for the task
** bool         isCore0     Force the scheduler to run this task on core0
**
** Returns:		void 
**
******************************************************************************/
void setTaskAttributes(uint16_t taskID, bool isCore0)
{
	PTASKDESC	td = NULL;

	td = _findTaskByID(taskID);

	if (td != NULL) {
		td->forceCore0 = isCore0;
	}
}
#endif

/******************************************************************************
**
** Name: schedule()
**
** Description: The main scheduler loop, this will loop forever waiting for 
** tasks to be scheduled. This must be the last function called from main().
**
** Parameters:	N/A
**
** Returns:		it doesn't 
**
******************************************************************************/
void schedule()
{
    int         taskPoolCount = 0;
	PTASKDESC	td = head;
	
	/*
	** If no tasks have been registered, just loop until some are...
	*/
	while (td == NULL) {
		__wfi();
	}

	/*
	** Scheduler loop, run forever waiting for tasks to be
	** scheduled...
	*/
	while (1) {
		signalIdle();

		if (td->isScheduled && getRTCClockCount() >= td->scheduledTime) {
			/*
			** Mark the task as un-scheduled, so by default the
			** task will not run again automatically. If the task
			** is periodic or if the task reschedules itself, this 
			** flag will be reset to 1...
			*/
			td->isScheduled = 0;

			/*
			** Run the task...
			*/
			signalBusy();
#ifdef PICO_MULTICORE
            /*
            ** If core1 is not busy, run the task on core 1 (the default). 
            ** Otherwise run it on core 0 (this core) as a fallback. 
            */
            if (!isCoreOneBusy && !td->forceCore0) {
                for (taskPoolCount = 0;taskPoolCount < CORE1_TASKDESC_POOL_SIZE;taskPoolCount++) {
                    if (!core1TaskDescPool[taskPoolCount].isAllocated) {
                        memcpy(&core1TaskDescPool[taskPoolCount], td, sizeof(TASKDESC));
                        if (multicore_fifo_push_timeout_us((uint32_t)&core1TaskDescPool[taskPoolCount], 25)) {
                            break;
                        }
                    }
                }
                /*
                ** if there are no free task descriptors in our pool
                ** Run the task on core 0...
                */
                if (taskPoolCount == CORE1_TASKDESC_POOL_SIZE) {
                    td->run(td->pParameter);
                }
            }
            else {
                td->run(td->pParameter);
            }
#else
            td->run(td->pParameter);
#endif

			if (td->isPeriodic && !td->isScheduled) {
				rescheduleTask(td->ID, td->pParameter);
			}

			_tasksRunCount++;
		}

		td = td->next;

#ifdef UNIT_TEST_MODE
		usleep(500L);
#endif
		if (td == head) {
			/*
			** If we've looped through all the registered tasks, sleep
			** until the next interrupt to save power...
			*/
			__wfi();
		}
	}
}
