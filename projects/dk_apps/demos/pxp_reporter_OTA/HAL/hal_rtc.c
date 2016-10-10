/*
 * RTC_Driver.c
 *
 *  Created on: 2016Äê7ÔÂ25ÈÕ
 *      Author: Administrator
 */

#include "stdio.h"
#include "hal_rtc.h"
#include "osal.h"


time_t rawtime;

struct tm * timeinfo;

static void hal_rtc_update_cb(OS_TIMER timer)
{
	time ( &rawtime );

	rawtime++;
}


void hal_rtc_init(void)
{
	TimerHandle_t Handle;

	Handle = OS_TIMER_CREATE("RTC", OS_MS_2_TICKS(1000), true,NULL, hal_rtc_update_cb);

	OS_TIMER_START(Handle,OS_TIMER_FOREVER);
}


void hal_rtc_set_time(struct tm timeinfo)
{
    rawtime=mktime(&timeinfo);
}

void hal_rtc_get_time(char *timestr)
{
	if(!timestr)
		return;

	timeinfo = localtime ( &rawtime );				//convert second to struct

	strftime(timestr,30,"%F %T",timeinfo);

	return;
}

uint32 hal_rtc_get_unixtime(void)
{
	return rawtime;
}


