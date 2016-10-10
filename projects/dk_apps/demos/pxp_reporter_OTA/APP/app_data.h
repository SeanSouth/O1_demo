/*
 * app_data.h
 *
 *  Created on: 2016Äê9ÔÂ19ÈÕ
 *      Author: Administrator
 */

#ifndef APP_APP_DATA_H_
#define APP_APP_DATA_H_

#include "app_attitude.h"

#define MAX_DATA_NUMBER		(100)
#define DATA_FILE_NAME		"record.txt"

void data_clear_all_data(void);

uint8 data_get_last_single_data(char *out);
uint8 data_get_specified_single_data(uint8 rcrd_num, char *out);
uint8 data_write_new_single_data(single_brushing_data *bdata);
uint8 data_get_total_rcrd_num(void);

void data_reset_data(void);
void data_init(void);

#endif /* APP_APP_DATA_H_ */
