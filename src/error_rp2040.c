#include "pico/stdlib.h"
#include "schederr.h"
#include "led_utils.h"

#define ERR_ON_NOFREETASKS			2000
#define ERR_OFF_NOFREETASKS			 500

#define ERR_ON_TASKCOUNTOVERFLOW	1000
#define ERR_OFF_TASKCOUNTOVERFLOW	 150

#define ERR_ON_NULLTASK				 100
#define ERR_OFF_NULLTASK			1000

#define ERR_ON_NULLTASKEXEC			  50
#define ERR_OFF_NULLTASKEXEC		 500

#define ERR_ON_DROPOUT				  50
#define ERR_OFF_DROPOUT				  50

#define ERR_ON_DEFUALT				 100
#define ERR_OFF_DEFUALT				 100

#define _delay_ms(ms)		sleep_ms(ms)

void dot()
{
	turnOn(LED_ONBOARD);
	_delay_ms(150);

	turnOff(LED_ONBOARD);
	_delay_ms(150);
}

void dash()
{
	turnOn(LED_ONBOARD);
	_delay_ms(750);

	turnOff(LED_ONBOARD);
	_delay_ms(150);
}

void charBreak()
{
	_delay_ms(500);
}

void wordBreak()
{
	_delay_ms(1000);
}

/*
** Pattern:
**
** On:	2000 ms
** Off:	500 ms
*/
void _handleNoFreeTasks()
{
	while (1) {
		turnOn(LED_ONBOARD);
        _delay_ms(ERR_ON_NOFREETASKS);

		turnOff(LED_ONBOARD);
        _delay_ms(ERR_OFF_NOFREETASKS);
	}
}

/*
** Pattern:
**
** On:	1000 ms
** Off:	150 ms
*/
void _handleTaskCountOverFlow()
{
	while (1) {
		turnOn(LED_ONBOARD);
        _delay_ms(ERR_ON_TASKCOUNTOVERFLOW);

		turnOff(LED_ONBOARD);
        _delay_ms(ERR_OFF_TASKCOUNTOVERFLOW);
	}
}

/*
** Pattern:
**
** On:	100 ms
** Off:	1000 ms
*/
void _handleNullTask()
{
	while (1) {
		turnOn(LED_ONBOARD);
        _delay_ms(ERR_ON_NULLTASK);

		turnOff(LED_ONBOARD);
        _delay_ms(ERR_OFF_NULLTASK);
	}
}

/*
** Pattern:
**
** On:	50 ms
** Off:	500 ms
*/
void _handleNullTaskExec()
{
	while (1) {
		turnOn(LED_ONBOARD);
        _delay_ms(ERR_ON_NULLTASKEXEC);

		turnOff(LED_ONBOARD);
        _delay_ms(ERR_OFF_NULLTASKEXEC);
	}
}

/*
** Pattern:
**
** On:	50 ms
** Off:	50 ms
*/
void _handleDropout()
{
	while (1) {
		turnOn(LED_ONBOARD);
        _delay_ms(ERR_ON_DROPOUT);

		turnOff(LED_ONBOARD);
        _delay_ms(ERR_OFF_DROPOUT);
	}
}

/*
** Pattern:
**
** On:	100 ms
** Off:	100 ms
*/
void _handleDefault()
{
	while (1) {
		turnOn(LED_ONBOARD);
        _delay_ms(ERR_ON_DEFUALT);

		turnOff(LED_ONBOARD);
        _delay_ms(ERR_OFF_DEFUALT);
	}
}

void handleError(unsigned int code)
{
	switch (code) {
		case ERROR_SCHED_NOFREETASKS:
			_handleNoFreeTasks();
			break;
			
		case ERROR_SCHED_TASKCOUNTOVERFLOW:
			_handleTaskCountOverFlow();
			break;
			
		case ERROR_SCHED_NULLTASK:
			_handleNullTask();
			break;
			
		case ERROR_SCHED_DROPOUT:
			_handleDropout();
			break;
		
		case ERROR_SCHED_NULLTASKEXEC:
			_handleNullTaskExec();
			break;
			
		default:
			_handleDefault();
			break;
	}
}
