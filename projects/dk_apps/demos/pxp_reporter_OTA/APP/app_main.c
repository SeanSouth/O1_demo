/*
 * O1_main_task.c
 *
 *  Created on: 2016Äê8ÔÂ17ÈÕ
 *      Author: Administrator
 */
#include "stdbool.h"
#include "stdlib.h"

#include "osal.h"
#include "sys_watchdog.h"
#include "sys_power_mgr.h"
#include "platform_devices.h"

#include "hw_wkup.h"
#include "hw_gpio.h"

#include "app_main.h"
#include "app_debug.h"
#include "app_attitude.h"
#include "app_data.h"
#include "app_cmd.h"

#include "..\HAL\hal_rtc.h"
#include "..\HAL\hal_msensor.h"
#include "..\HAL\hal_led.h"

#include "..\BLE\ble_interface.h"

#include "..\LIB\easylogger\easyflash.h"
#include "..\LIB\easylogger\elog.h"
#include "..\LIB\easylogger\elog_flash.h"

static char tag[]="app_main";
#define log_a(...) elog_a(tag, __VA_ARGS__)
#define log_e(...) elog_e(tag, __VA_ARGS__)
#define log_w(...) elog_w(tag, __VA_ARGS__)
#define log_i(...) elog_i(tag, __VA_ARGS__)
#define log_d(...) elog_d(tag, __VA_ARGS__)
#define log_v(...) elog_v(tag, __VA_ARGS__)


/*************************DATE***********************/
Control_Block brush_cb;

main_mode_name mode_to_be_enter;

OS_TIMER bind_timer_handle;

uint8 sys_status;

uint8 int_status;       //interrupt status      distinguish which pin is trigger

int8_t sys_mode;            //SYS_MODE

int8 FXOS8700_interrupt = 0;

/*************************DATE END***********************/


/*****************CALL BACK FUNCTION*****************/
Control_Block* main_get_brush_cb(void)
{
	return &brush_cb;
}


void main_wake_up_cb(void)
{
	bool pin_status;

	int_status = 0;

	pin_status = hw_gpio_get_pin_status(INT_6AXIS_PORT,INT_6AXIS_PIN);

	if(pin_status==0)

		int_status = int_status + 1;

	pin_status = hw_gpio_get_pin_status(INT_3AXIS_PORT,INT_3AXIS_PIN);

	if(pin_status==0)

		int_status = int_status + 2;

	OS_TASK_NOTIFY_FROM_ISR(brush_cb.main_task, MAIN_WAKE_UP_NOTIF, eSetBits);

	hw_wkup_reset_counter();

	hw_wkup_reset_interrupt();
}

void main_bind_timeout_cb(TimerHandle_t xTimer )
{
	main_switch_mode(MAIN_MODE_DETECTED);
}



/*****************CALL BACK FUNCTION END*****************/

void main_wakeup_init(void)
{
	hw_wkup_init(NULL);
    hw_wkup_configure_pin(INT_6AXIS_PORT, INT_6AXIS_PIN, 1, HW_WKUP_PIN_STATE_LOW);
    hw_wkup_configure_pin(INT_3AXIS_PORT, INT_3AXIS_PIN, 1, HW_WKUP_PIN_STATE_LOW);
    hw_wkup_configure_pin(HW_GPIO_PORT_1, HW_GPIO_PIN_6, 1, HW_WKUP_PIN_STATE_LOW);
    hw_wkup_set_counter_threshold(1);
	hw_wkup_set_debounce_time(100);

	hw_wkup_register_interrupt(main_wake_up_cb, 1);
}

void main_log_init()
{
	elog_init();

	elog_flash_init();

	easyflash_init();

	/* set EasyLogger log format */
	elog_set_fmt(ELOG_LVL_ASSERT, ELOG_FMT_LVL | ELOG_FMT_TAG | ELOG_FMT_TIME | ELOG_FMT_P_INFO);
	elog_set_fmt(ELOG_LVL_ERROR,  ELOG_FMT_LVL | ELOG_FMT_TAG | ELOG_FMT_TIME | ELOG_FMT_FUNC | ELOG_FMT_LINE);
	elog_set_fmt(ELOG_LVL_WARN,   ELOG_FMT_LVL | ELOG_FMT_TAG | ELOG_FMT_FUNC | ELOG_FMT_LINE);
	elog_set_fmt(ELOG_LVL_INFO,   ELOG_FMT_LVL | ELOG_FMT_TAG | ELOG_FMT_FUNC | ELOG_FMT_LINE);
	elog_set_fmt(ELOG_LVL_DEBUG,  ELOG_FMT_LVL | ELOG_FMT_TAG | ELOG_FMT_FUNC | ELOG_FMT_LINE);
	elog_set_fmt(ELOG_LVL_VERBOSE,ELOG_FMT_LVL | ELOG_FMT_TAG | ELOG_FMT_FUNC | ELOG_FMT_LINE);

	/* start EasyLogger */
	elog_start();

	//ef_env_set_default();
	//ef_load_env();
	//ef_print_env();

}

void main_control_block_init()
{
	memset(&brush_cb,0,sizeof(brush_cb));

	brush_cb.positon_model = POSITION_ATTITUDE_MODEL_16;
	brush_cb.dominant_hand = atoi(ef_get_env("domain"));
	brush_cb.current_mode = MAIN_MODE_DETECTED;
	brush_cb.main_task = OS_GET_CURRENT_TASK();

}

void main_mode1_enter(void)
{
	hal_msensor_set_detect_mode();
}

void main_mode1_exit(void)
{
	hal_msensor_reset();
}

void main_mode2_enter(void)
{
	hal_msensor_set_detect_mode();
}

void main_mode2_exit(void)
{
	hal_msensor_reset();
}

void main_mode3_enter(void)
{
	attitude_record_start();
	attitude_reset();
	attitdue_task_start();
	cmd_async_feedback_F2(DEV_MOTOR_START);
	hal_led_turn_on();
}

void main_mode3_exit(void)
{
	attitude_record_stop();
	attitdue_task_stop();
	hal_msensor_reset();
	cmd_async_feedback_F2(DEV_MOTOR_STOP);
	hal_led_turn_off();
}

void main_mode4_enter(void)
{
	hal_led_turn_delay(MAIN_BIND_TIMEOUT);
	bind_timer_handle = OS_TIMER_CREATE("BIND", OS_MS_2_TICKS(MAIN_BIND_TIMEOUT), false,NULL, main_bind_timeout_cb);
	OS_TIMER_START(bind_timer_handle,OS_TIMER_FOREVER);
}

void main_mode4_exit(void)
{
	hal_led_turn_off();
	OS_TIMER_DELETE(bind_timer_handle,OS_TIMER_FOREVER);
	hal_msensor_reset();
}

struct_mode main_mode[] = {
	{MAIN_MODE_SLEEP,main_mode1_enter,main_mode1_exit},//sleep mode
	{MAIN_MODE_DETECTED,main_mode2_enter,main_mode2_exit},//brush mode
	{MAIN_MODE_BRUSH,main_mode3_enter,main_mode3_exit},//pause mode
	{MAIN_MODE_BIND,main_mode4_enter,main_mode4_exit},//binding mode
};

void main_switch_mode(main_mode_name mode)
{
	mode_to_be_enter = mode;
	OS_TASK_NOTIFY(brush_cb.main_task, MAIN_MODE_SWITCH, eSetBits);
}

void main_task(void *params)
{
	int8_t wdog_id;

	wdog_id = sys_watchdog_register(false);

    sys_watchdog_suspend(wdog_id);

    debug_init();

    hal_rtc_init();

    hal_msensor_init();

    main_wakeup_init();

    main_log_init();

    main_control_block_init();

    data_init();

    attitude_init();

    ble_app_init();

    main_mode[main_get_brush_cb()->current_mode].mode_enter_func();

    sys_watchdog_notify_and_resume(wdog_id);

    for (;;)
    {
        OS_BASE_TYPE ret;
        uint32_t notif;

        /* notify watchdog on each loop */
        sys_watchdog_notify(wdog_id);

        /* suspend watchdog while blocking on OS_TASK_NOTIFY_WAIT() */
        sys_watchdog_suspend(wdog_id);

        /* Wait on any of the notification bits, then clear them all */
        ret = OS_TASK_NOTIFY_WAIT(0, OS_TASK_NOTIFY_ALL_BITS, &notif, OS_TASK_NOTIFY_FOREVER);
        /* Blocks forever waiting for the task notification. Therefore, the return value must
         * always be OS_OK
         */
        OS_ASSERT(ret == OS_OK);

        /* resume watchdog */
        sys_watchdog_notify_and_resume(wdog_id);

        /* Notified from BLE Manager? */
        if(notif & MAIN_WAKE_UP_NOTIF)
        {
        	if(int_status&0x01)
        	{
        		if(main_get_brush_cb()->current_mode == MAIN_MODE_BIND)
        		{
        			cmd_async_feedback_F1(true);
        			main_switch_mode(MAIN_MODE_DETECTED);
        		}
        		else
        		{
					log_i("key tick test");
					if(main_get_brush_cb()->current_mode == MAIN_MODE_DETECTED)
						main_switch_mode(MAIN_MODE_BRUSH);
					else if(main_get_brush_cb()->current_mode == MAIN_MODE_BRUSH)
						main_switch_mode(MAIN_MODE_DETECTED);
					else
						log_i("current mode :%d",main_get_brush_cb()->current_mode);
        		}
        	}
        	else if(int_status&0x02)
        	{
        		FXOS8700_interrupt = hal_msensor_8700_read_intsrc();
        		if(FXOS8700_interrupt == FXOS8700_PLUSE_INT)   //pluse
        		{
        			//LED_turn_ms(1000);
        			log_i("sensor pluse interrupt detected!");
        		}
        		if(FXOS8700_interrupt == FXOS8700_MOTION_INT)  //motion
        		{
        			log_i("freefall/motion interrupt detected!");
        		}
        		else
        		{
        			log_e("read sensor int_source failed,maybe sensor failed.");
        		}
        	}
        }
        if(notif & MAIN_DEV_CONNECTED)
        {
        	hw_gpio_set_active(HW_GPIO_PORT_1, HW_GPIO_PIN_5);
        }
        if(notif & MAIN_DEV_DISCONNECTED)
        {
        	hw_gpio_set_inactive(HW_GPIO_PORT_1, HW_GPIO_PIN_5);
        }
        if(notif & MAIN_MODE_SWITCH)
        {
        	main_mode[main_get_brush_cb()->current_mode].mode_exit_func();
        	main_get_brush_cb()->current_mode = mode_to_be_enter;
        	main_mode[main_get_brush_cb()->current_mode].mode_enter_func();
        }
//        if (notif & MODE_DETECT_NOTIF)
//        {
//            sys_mode = get_sys_mode();
//            if(sys_mode == 0)
//            	initialize_mode_0();
//        }
    }
}
