/*
 * calculate_driver.h
 *
 *  Created on: 2016Äê8ÔÂ25ÈÕ
 *      Author: Administrator
 */
#ifndef FRANWORK_CALCULATE_DRIVER_H_
#define FRANWORK_CALCULATE_DRIVER_H_

#include "sdk_defs.h"

#define TASK_RUN 		(1)
#define TASK_STOP 		(0)

#define MIN_BRUSH_TIME      (1u)
#define MAX_BRUSH_TIME      (300u)
#define FFT_COUNT           (24)
#define DATA_POS_LEN     (16)

#define POSITION_ATTITUDE_MODEL_4 4
#define POSITION_ATTITUDE_MODEL_HHNFT 5
#define POSITION_ATTITUDE_MODEL_8 8
#define POSITION_ATTITUDE_MODEL_11 11
#define POSITION_ATTITUDE_MODEL_16 16

#define	RIGHT_HAND_MODE	1
#define LEFT_HAND_MODE 0

typedef struct _attitude_data
{
	int16 ned[3];
	int16 acc[3];
	int16 mag[3];
	int16 gyr[3];
	int16 fiterror;
	float accxf;
	float accxa;
	float accyf;
	float accya;
	uint8 index;
	uint8* ptr_data;
} Attitude_Data;

typedef struct _acc_data
{
	float accx[FFT_COUNT];
	float accy[FFT_COUNT];
} Acc_data;

typedef struct _relative_data
{
	float base_pitch;
	float base_compass;
	float r_compass_min;
	float r_compass_max;
	float r_roll;
	float r_pitch;
	float r_compass;
} relative_date;

typedef struct _single_brushing_data
{
    uint32 time;
    uint16 span;
    uint16 model;
    uint16 pos[DATA_POS_LEN];
}single_brushing_data;

void attitude_record_start(void);
void attitude_record_stop(void);
void attitdue_task_start(void);
void attitdue_task_stop(void);
void attitude_reset(void);
void attitude_init(void);

#endif /* FRANWORK_CALCULATE_DRIVER_H_ */
