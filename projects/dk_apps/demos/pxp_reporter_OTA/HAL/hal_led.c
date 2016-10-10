/*
 * LED_Driver.c
 *
 *  Created on: 2016Äê8ÔÂ5ÈÕ
 *      Author: Administrator
 */

#include "hw_breath.h"
#include "hw_led.h"
#include "sys_power_mgr.h"
#include "osal.h"
#include "timers.h"

uint8 pm_state = 0;

void hal_led_init(uint8_t min,uint8_t step)
{
    breath_config config = {
            .dc_min = min,
            .dc_max = 255,
			.dc_step = step,
            .freq_div = 255,
            .polarity = HW_BREATH_PWM_POL_POS,

    };

    hw_breath_init(&config);
    hw_led_set_led1_src(HW_LED_SRC1_BREATH);
}

void hal_led_turn_on(void)
{
	if(!pm_state)
	{
		pm_stay_alive();
		pm_state++;
	}

	hal_led_init(250,252);
    hw_led_enable_led1(true);
	hw_breath_enable();
}

void hal_led_turn_flash(void)
{
	if(!pm_state)
	{
		pm_stay_alive();
		pm_state++;
	}

	hal_led_init(0,100);
	hw_led_enable_led1(true);
	hw_breath_enable();
}

void hal_led_turn_off(void)
{
	if(pm_state)
	{
		pm_resume_sleep();
		pm_state--;
	}

	hw_led_enable_led1(false);
	hw_breath_disable();
}

void hal_led_turn_off_cb(TimerHandle_t xTimer )
{
	if(pm_state)
	{
		pm_resume_sleep();
		pm_state--;
	}

	hw_led_enable_led1(false);
	hw_breath_disable();
}

void hal_led_turn_delay(uint16 ms)
{
	hal_led_turn_on();

	TimerHandle_t Handle;

	Handle = OS_TIMER_CREATE("RTC", OS_MS_2_TICKS(ms), false,NULL, hal_led_turn_off_cb);

	OS_TIMER_START(Handle,OS_TIMER_FOREVER);
}
