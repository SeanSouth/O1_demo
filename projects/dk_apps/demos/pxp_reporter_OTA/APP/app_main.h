/*
 * app_Pin.h
 *
 *  Created on: 2016Äê8ÔÂ17ÈÕ
 *      Author: Administrator
 */

#ifndef APP_APP_PIN_H_
#define APP_APP_PIN_H_

#include "sdk_defs.h"

#define	INT_6AXIS_PORT  HW_GPIO_PORT_1
#define	INT_6AXIS_PIN 	HW_GPIO_PIN_7
#define	INT_3AXIS_PORT  HW_GPIO_PORT_1
#define	INT_3AXIS_PIN 	HW_GPIO_PIN_4



#define MAIN_WAKE_UP_NOTIF		(1 << 1)
#define MAIN_DEV_CONNECTED		(1 << 2)
#define MAIN_DEV_DISCONNECTED	(1 << 3)
#define MAIN_MODE_SWITCH		(1 << 4)
#define MAIN_MODE_DETECT_NOTIF 	(1 << 5)

#define LOWEST_VOLTAGE  2800

#define MAIN_BIND_TIMEOUT		(10000)

#define DIVIDED_PACKET              (0u)
#define COMPLETE_PACKET             (1u)

#define DEV_MOTOR_STOP 				(0)
#define DEV_MOTOR_START 			(1)

typedef enum _Brush_state
{
	brush_state_sleep = 0,
	brush_state_idle,
	brush_state_motor_change,
	brush_state_motor_on,
	brush_state_motor_pause,
	brush_state_binding,
	brush_state_waiting_confirm,
	brush_state_error
}Brush_State;

typedef enum _main_mode_name
{
	MAIN_MODE_SLEEP,
	MAIN_MODE_DETECTED,
	MAIN_MODE_BRUSH,
	MAIN_MODE_BIND,
	MAIN_MODE_DEBUG
}main_mode_name;

typedef struct _Control_block
{
	main_mode_name current_mode;
	Brush_State current_state;
	uint8 brush_st;
    uint8 positon_model;
    uint16 battry_mvolt;
    uint8 battery_charging;
    uint8 need_devided_packet;
    uint8 interactive_mode;
    uint16 interactive_duration;
    uint8 dominant_hand;
    uint8 current_apppos;
    uint8 calibra_mode;
    OS_TASK main_task;
}Control_Block;


typedef void (*func)(void);


typedef struct _struct_mode
{
	main_mode_name mode_num;
	func mode_enter_func;
	func mode_exit_func;
}struct_mode;

Control_Block* main_get_brush_cb(void);
void main_switch_mode(main_mode_name mode);
void main_task(void *params);

#endif /* APP_APP_PIN_H_ */
