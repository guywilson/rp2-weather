#include <stdio.h>
#include <stdlib.h>

#include "pico/stdlib.h"
#include "schederr.h"
#include "utils.h"

void _handleNoFreeTasks()
{
    uart_puts(uart0, "SCHEDERR: No free tasks!\n");
}

void _handleTaskCountOverFlow()
{
    uart_puts(uart0, "SCHEDERR: No task count overflow!\n");
}

void _handleNullTask()
{
    uart_puts(uart0, "SCHEDERR: Null task!\n");
}

void _handleNullTaskExec()
{
    uart_puts(uart0, "SCHEDERR: Running null task!\n");
}

void _handleDropout()
{
    uart_puts(uart0, "SCHEDERR: Dropout!\n");
}

void _handleDefault()
{
    uart_puts(uart0, "SCHEDERR: Generic error!\n");
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

    exit(-1);
}
