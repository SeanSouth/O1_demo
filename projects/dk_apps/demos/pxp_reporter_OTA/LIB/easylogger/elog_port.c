/*
 * This file is part of the EasyLogger Library.
 *
 * Copyright (c) 2015, Armink, <armink.ztl@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * 'Software'), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED 'AS IS', WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Function: Portable interface for each platform.
 * Created on: 2015-04-28
 */
 
#include <elog.h>
#include <time.h>
#include "stdio.h"
#include "string.h"
#include "osal.h"
#include "task.h"

#include "elog_flash.h"
#include "hw_uart.h"
#include "..\..\HAL\hal_rtc.h"

SemaphoreHandle_t xSemaphore = NULL;
char task_info[20] = {0};

/**
 * EasyLogger port initialize
 *
 * @return result
 */
ElogErrCode elog_port_init(void) {
    ElogErrCode result = ELOG_NO_ERR;

    /* add your code here */
    xSemaphore = xSemaphoreCreateMutex();
    
    return result;
}

/**
 * output log port interface
 *
 * @param log output of log
 * @param size log size
 */
void elog_port_output(const char *log, size_t size) {
    
    /* add your code here */
	/* output to terminal */

	if((log[0]=='A')||(log[0]=='E'))
	{
		elog_flash_write(log,size);
	}

	hw_uart_send(HW_UART2,log,size,NULL, NULL);
	//printf("%.*s", size, log);
}

/**
 * output lock
 */
void elog_port_output_lock(void) {
    
    /* add your code here */
	xSemaphoreTake(xSemaphore,10);
}

/**
 * output unlock
 */
void elog_port_output_unlock(void) {
    
    /* add your code here */
	xSemaphoreGive(xSemaphore);
}

/**
 * get current time interface
 *
 * @return current time
 */
char time_str[30]={0};

const char *elog_port_get_time(void) {
    
    /* add your code here */
	memset(time_str,0,sizeof(time_str));
	hal_rtc_get_time(time_str);

    return time_str;
}

/**
 * get current process name interface
 *
 * @return current process name
 */
const char *elog_port_get_p_info(void) {
    
    /* add your code here */
	TaskHandle_t handle;

	handle = OS_GET_CURRENT_TASK();
	strcpy(task_info,pcTaskGetTaskName(handle));

	return task_info;
}

/**
 * get current thread name interface
 *
 * @return current thread name
 */
const char *elog_port_get_t_info(void) {
    
    /* add your code here */
	return "";
    
}
