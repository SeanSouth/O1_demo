/*
 * RTC_Driver.h
 *
 *  Created on: 2016Äê7ÔÂ25ÈÕ
 *      Author: Administrator
 */

#ifndef APP_RTC_DRIVER_H_
#define APP_RTC_DRIVER_H_

#include "time.h"
#include "sdk_defs.h"

void hal_rtc_init(void);
void hal_rtc_set_time(struct tm timeinfo);
void hal_rtc_get_time(char *timestr);
uint32 hal_rtc_get_unixtime(void);


#endif /* APP_RTC_DRIVER_H_ */
