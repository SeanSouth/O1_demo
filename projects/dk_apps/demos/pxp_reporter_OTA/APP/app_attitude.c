/*
 * calculate_driver.c
 *
 *  Created on: 2016Äê8ÔÂ25ÈÕ
 *      Author: Administrator
 */

#include "stddef.h"
#include "app_Attitude.h"
#include "osal.h"
#include "hw_uart.h"

#include "app_main.h"
#include "app_data.h"

#include "..\LIB\FS_Fusion\mqx_tasks.h"
#include "..\LIB\FS_Fusion\drivers.h"
#include "..\LIB\fft\kiss_fftr.h"
#include "..\LIB\easylogger\elog.h"

#include "..\HAL\hal_msensor.h"
#include "..\HAL\hal_rtc.h"

static char tag[]="app_attitude";
#define log_a(...) elog_a(tag, __VA_ARGS__)
#define log_e(...) elog_e(tag, __VA_ARGS__)
#define log_w(...) elog_w(tag, __VA_ARGS__)
#define log_i(...) elog_i(tag, __VA_ARGS__)
#define log_d(...) elog_d(tag, __VA_ARGS__)
#define log_v(...) elog_v(tag, __VA_ARGS__)


OS_TASK rd_handle;
OS_TASK fs_handle;
OS_TASK mg_handle;
OS_TASK sa_handle;

Attitude_Data attitude_atri;
Acc_data acc;
int acc_count;
uint8 calibrated = 0;
relative_date r_data;
single_brushing_data one_brush;

volatile uint64 current_rtctime = 0;
volatile uint16 current_time = 0;
volatile uint16 temp_time = 0;

static int control_flag = TASK_STOP;


Attitude_Data* attitude_get_struct(void)
{
    return &attitude_atri;
}

void _lwevent_set(void)
{
	OS_TASK_NOTIFY(mg_handle,1,eNoAction);
}

void _lwevent_wait(void)
{
	OS_TASK_NOTIFY_WAIT(0x00,0xFFFFFFFF,NULL,portMAX_DELAY);
}


void attitude_get_molar_datum(void)
{
    float roll,pitch,compass;

    roll = attitude_atri.ned[0]*0.1;
    pitch = attitude_atri.ned[1]*0.1;
    compass = attitude_atri.ned[2]*0.1;

    if(main_get_brush_cb()->dominant_hand == 0)
    {
        r_data.base_pitch = pitch;
        compass -= 40;

		if(compass < 180)
		{
			r_data.base_compass = compass;
			r_data.r_compass_min = r_data.base_compass;
			r_data.r_compass_max = r_data.base_compass + 180;
		}
		else
		{
			r_data.base_compass = compass;
			r_data.r_compass_min = r_data.base_compass;
			r_data.r_compass_max = r_data.base_compass - 180;
		}

        one_brush.pos[2] += 1;
        one_brush.pos[3] += 1;
    }
    else
    {
        r_data.base_pitch = pitch;
        compass += 40;

        if(compass > 180)
		{
			r_data.base_compass = compass - 180;
			r_data.r_compass_min = r_data.base_compass;
			r_data.r_compass_max = compass;
		}
		else
		{
			r_data.base_compass = compass + 180;
			r_data.r_compass_min = r_data.base_compass;
			r_data.r_compass_max = compass;
        }

        one_brush.pos[0] += 1;
        one_brush.pos[1] += 1;
    }

    log_i("attitude_get_molar_datum rc=%f\r\n",r_data.base_compass);
}

uint8 attitude_get_5position(void)
{
    float roll,pitch;
    uint8 pos = 0;

    if(main_get_brush_cb()->brush_st != brush_state_motor_on)
    {
        pos = 0;
        return pos;
    }
    else
    {
        roll = attitude_atri.ned[0]*0.1;
        pitch = attitude_atri.ned[1]*0.1;
    }

    if(roll <= 0)
    {
        if(pitch > 0)
            pos = 0;
        else
            pos = 2;
    }
    else
    {
        if(pitch > 0)
            pos = 1;
        else
            pos = 3;
    }

    return pos;
}

uint8 attitude_get_16position(void)
{
    float roll,pitch,compass;
    float tmp;
    float r,p,c;
    uint8 pos = 0;
    char tmpstr[100];

    if(main_get_brush_cb()->brush_st != brush_state_motor_on)
    {
        pos = 0xfe;
        return pos;
    }
    else
    {
        roll = attitude_atri.ned[0]*0.1;
        pitch = attitude_atri.ned[1]*0.1;
        compass = attitude_atri.ned[2]*0.1;
    }

    //calculate relative data by base data
	r_data.r_roll = roll;
    if(compass < r_data.base_compass)
    {
    	r_data.r_compass = compass + 360 - r_data.base_compass;
    }
    else
    {
    	r_data.r_compass = compass - r_data.base_compass;
    }

	if(r_data.r_compass <= 90)
	{
		r_data.r_pitch = pitch - r_data.base_pitch;
	}
	else if(r_data.r_compass <= 180)
	{
		r_data.r_pitch = pitch - r_data.base_pitch;
	}
	else if(r_data.r_compass <= 270)
	{
		r_data.r_pitch = pitch - r_data.base_pitch;

	}
	else
	{
		r_data.r_pitch = pitch - r_data.base_pitch;
	}

    //calculate pos
	r = r_data.r_roll;
	p = r_data.r_pitch;
	c = r_data.r_compass;

    if(main_get_brush_cb()->interactive_duration)
    {
        int app_pos = main_get_brush_cb()->current_apppos;

        if(app_pos == 0 && r < 0 && p > -45 && p < 45 && c > 90 && c < 180)
			pos = 0;
		else if(app_pos == 1 && r < 0 && p > -45 && p < 45 && c > 90 && c < 180)
			pos = 1;
		else if(app_pos == 2 && r > 0 && p > -45 && p < 45 && c > 20 && c < 90)
			pos = 2;
		else if(app_pos == 3 && r > 0 && p > -45 && p < 45 && c > 20 && c < 90)
			pos = 3;
		else if(app_pos == 4 && r > 0 && p > -45 && p < 45 && c > 90 && c < 180)
			pos = 4;
		else if(app_pos == 5 && r > 0 && p > -45 && p < 45 && c > 90 && c < 180)
			pos = 5;
		else if(app_pos == 6 && r < 0 && p > -45 && p < 45 && c > 20 && c < 90)
			pos = 6;
		else if(app_pos == 7 && r < 0 && p > -45 && p < 45 && c > 20 && c < 90)
			pos = 7;
		else if(app_pos == 8 && r < 0 && p > -45 && p < 45 && c > 90 && c < 180)
			pos = 8;
		else if(app_pos == 9 && (r > 135 || r < -135) && p > -45 && p < 45 && c > 90 && c < 180)
			pos = 9;
		else if(app_pos == 10 && r > -45 && r < 45 && p > -45 && p < 45 && c > 0 && c < 90)
			pos = 10;
		else if(app_pos == 11 && (r > 135 || r < -135) && p > -45 && p < 45 && c > 0 && c < 90)
			pos = 11;
		else if(app_pos == 12 && r < 0 && p > -1 * r_data.base_pitch - 45 && p < -1 * r_data.base_pitch + 45 && c > 160 && c < 200)
			pos = 12;
		else if(app_pos == 13 && r < 0 && p > -1 * r_data.base_pitch - 45 && p < -1 * r_data.base_pitch + 45 && c > 160 && c < 200)
			pos = 13;
		else if(app_pos == 14 && r > 0 )
			pos = 14;
		else if(app_pos == 15 && r > 0 )
			pos = 15;
		else
            pos = 0xfe;

        log_i("attitude_get_16position r=%f p=%f c=%f pos=%d",r,p,c,pos);

        return pos;
    }

    if(main_get_brush_cb()->dominant_hand == LEFT_HAND_MODE)
    {
		if(r > -90 && r < -45 && p > -30 && p < 30 && c > 90 && c < 180)
			pos = 0;
		else if(r > -135 && r < -90 && p > -30 && p < 30 && c > 90 && c < 180)
			pos = 1;
		else if(r > 45 && r < 90 && p > -30 && p < 30 && c > 20 && c < 90)
			pos = 2;
		else if(r > 90 && r < 135 && p > -30 && p < 30 && c > 20 && c < 90)
			pos = 3;
		else if(r > 45 && r < 90 && p > -30 && p < 30 && c > 90 && c < 180)
			pos = 4;
		else if(r > 90 && r < 135 && p > -30 && p < 30 && c > 90 && c < 180)
			pos = 5;
		else if(r > -90 && r < -45 && p > -30 && p < 30 && c > 40 && c < 90)
			pos = 6;
		else if(r > -135 && r < -90 && p > -30 && p < 30 && c > 40 && c < 90)
			pos = 7;
		else if(r > -45 && r < 45 && p > -30 && p < 30 && c > 90 && c < 180)
			pos = 8;
		else if((r > 135 || r < -135) && p > -30 && p < 30 && c > 90 && c < 180)
			pos = 9;
		else if(r > -45 && r < 45 && p > -30 && p < 30 && c > 0 && c < 90)
			pos = 10;
		else if((r > 135 || r < -135) && p > -30 && p < 30 && c > 0 && c < 90)
			pos = 11;
		else if(r > 0 && p > -1 * r_data.base_pitch - 30 && p < -1 * r_data.base_pitch + 30 && (c < 20 || c > 340))
			pos = 12;
		else if(r > 0 && p > -1 * r_data.base_pitch - 30 && p < -1 * r_data.base_pitch + 30 && (c < 20 || c > 340))
			pos = 13;
		else if(r < 0 && p > r_data.base_pitch - 30 && p < r_data.base_pitch + 30 && c > 0 && c < 40)
			pos = 14;
		else if(r < 0 && p > -1 * r_data.base_pitch - 45 && p < -1 *r_data.base_pitch + 15 && c > 0 && c < 40)
			pos = 15;
		else
            pos = 0xfe;
    }
    else
    {
    	if(r > -90 && r < -45 && p > -30 && p < 30 && c > 90 && c < 160)
    		pos = 0;
    	else if(r > -135 && r < -90 && p > -30 && p < 30 && c > 90 && c < 160)
    		pos = 1;
    	else if(r > 45 && r < 90 && p > -30 && p < 30 && c > 0 && c < 90)
    		pos = 2;
    	else if(r > 90 && r < 135 && p > -30 && p < 30 && c > 0 && c < 90)
    		pos = 3;
    	else if(r > 45 && r < 90 && p > -30 && p < 30 && c > 90 && c < 140)
    		pos = 4;
    	else if(r > 90 && r < 135 && p > -30 && p < 30 && c > 90 && c < 140)
    		pos = 5;
    	else if(r > -90 && r < -45 && p > -30 && p < 30 && c > 0 && c < 90)
    		pos = 6;
    	else if(r > -135 && r < -90 && p > -30 && p < 30 && c > 0 && c < 90)
    		pos = 7;
    	else if(r > -45 && r < 45 && p > -30 && p < 30 && c > 90 && c < 180)
    		pos = 8;
    	else if((r > 135 || r < -135) && p > -30 && p < 30 && c > 90 && c < 180)
    		pos = 9;
    	else if(r > -45 && r < 45 && p > -30 && p < 30 && c > 0 && c < 90)
    		pos = 10;
    	else if((r > 135 || r < -135) && p > -30 && p < 30 && c > 0 && c < 90)
    		pos = 11;
    	else if(r < 0 && p > -1 * r_data.base_pitch - 30 && p < -1 * r_data.base_pitch + 30 && c > 160 && c < 200)
    		pos = 12;
    	else if(r < 0 && p > -1 * r_data.base_pitch - 30 && p < -1 *r_data.base_pitch + 30 && c > 160 && c < 200)
    		pos = 13;
    	else if(r > 0 && p > r_data.base_pitch - 30 && p < r_data.base_pitch + 30 && c > 140 && c < 180)
    		pos = 14;
    	else if(r > 0 && p > -1 * r_data.base_pitch - 45 && p < -1 *r_data.base_pitch + 15 && c > 140 && c < 200)
    		pos = 15;
    	else
    		pos = 0xfe;
    }

    log_i("attitude_get_16position r=%f p=%f c=%f pos=%d\r\n",r,p,c,pos);
    return pos;
}

void attitude_fft_cal(void)
{
    int i,maxfreq;
    float maxamp;

    kiss_fft_cpx sout[FFT_COUNT];
    kiss_fftr_cfg  kiss_fftr_state;
	float res[FFT_COUNT];

    kiss_fft_scalar *rin = acc.accx;
    memset(sout,0,sizeof(sout));

    kiss_fftr_state = kiss_fftr_alloc(FFT_COUNT,0,0,0);
    kiss_fftr(kiss_fftr_state,rin,sout);

    maxfreq = 0;
    maxamp = 0;

	for(i=1;i<FFT_COUNT/2;i++)
	{
		res[i] = sqrt(sout[i].r * sout[i].r + sout[i].i * sout[i].i);
		if(res[i]>maxamp)
		{
			maxamp = res[i];
			maxfreq = i;
		}
	}

    attitude_atri.accxa = maxamp/FFT_COUNT*2;
    attitude_atri.accxf = maxfreq;

    free(kiss_fftr_state);

    rin = acc.accy;
    memset(sout,0,sizeof(sout));

    kiss_fftr_state = kiss_fftr_alloc(FFT_COUNT,0,0,0);
    kiss_fftr(kiss_fftr_state,rin,sout);

    maxfreq = 0;
    maxamp = 0;

	for(i=1;i<FFT_COUNT/2;i++)
	{
		res[i] = sqrt(sout[i].r * sout[i].r + sout[i].i * sout[i].i);
		if(res[i]>maxamp)
		{
			maxamp = res[i];
			maxfreq = i;
		}
	}

    attitude_atri.accya = maxamp/FFT_COUNT*2;
    attitude_atri.accyf = maxfreq;

    free(kiss_fftr_state);

    log_i("accxa = %f,accxf = %f\n",attitude_atri.accxa,attitude_atri.accxf);
    log_i("accya = %f,accyf = %f\n",attitude_atri.accya,attitude_atri.accyf);

    acc_count = 0;
}

void attitude_readtask(void *params)
{
    TickType_t xLastFlashTime,delay = 2;


	/* We need to initialise xLastFlashTime prior to the first call to
	vTaskDelayUntil(). */
	xLastFlashTime = xTaskGetTickCount();

	for(;;)
	{
		if(control_flag == TASK_RUN)
		{
			/* Delay for half the flash period then turn the LED on. */
			vTaskDelayUntil( &xLastFlashTime, delay );
			RdSensData_task_runonece();
		}
		else
		{
			vTaskDelayUntil( &xLastFlashTime, 100 );
		}
	}
} /*lint !e715 !e818 !e830 Function definition must be standard for task creation. */

void attitude_fusiontask(void *params)
{
    TickType_t xLastFlashTime,delay = 20;

	/* We need to initialise xLastFlashTime prior to the first call to
	vTaskDelayUntil(). */
	xLastFlashTime = xTaskGetTickCount();

	for(;;)
	{
		if(control_flag == TASK_RUN)
		{
			/* Delay for half the flash period then turn the LED on. */
			vTaskDelayUntil( &xLastFlashTime, delay );
			Fusion_task_runonce();
			CreateAndSendPackets_UART(0,&attitude_atri);
			//hw_uart_write_buffer(HW_UART1,att.ptr_data,att.index);

			if (acc_count < 25)
			{
				acc.accx[acc_count] = (float) attitude_atri.acc[0] / 8192;
				acc.accy[acc_count] = (float) attitude_atri.acc[1] / 8192;
				acc_count++;
			}
			else
			{
				attitude_fft_cal();
			}
			//cmd_async_realtime_F5();
		}
		else
		{
			vTaskDelayUntil( &xLastFlashTime, 100 );
		}
	}
} /*lint !e715 !e818 !e830 Function definition must be standard for task creation. */

void attitude_magtask(void *params)
{
    TickType_t xLastFlashTime,delay = 8000;


	/* We need to initialise xLastFlashTime prior to the first call to
	vTaskDelayUntil(). */
	xLastFlashTime = xTaskGetTickCount();

	for(;;)
	{
		/* Delay for half the flash period then turn the LED on. */
		//vTaskDelayUntil( &xLastFlashTime, 100 );
		MagCal_task_runonce();
	}
} /*lint !e715 !e818 !e830 Function definition must be standard for task creation. */

void attitude_satask(void *params)
{
    TickType_t xLastFlashTime,delay = 1000;
    uint8 position = 0;
    int i,j;

	/* We need to initialise xLastFlashTime prior to the first call to
	vTaskDelayUntil(). */
	xLastFlashTime = xTaskGetTickCount();

	for(;;)
	{
		/* Delay for half the flash period then turn the LED on. */
		vTaskDelay(delay);

		if(control_flag == TASK_RUN)
		{
			for (i = 0; i < 3; i++)
			{
				if (abs(attitude_atri.gyr[i]) / 20 > 1000)
				{
					hal_gyro_reset();
				}
			}

			temp_time = current_time + (hal_rtc_get_unixtime() - current_rtctime);

			if (main_get_brush_cb()->positon_model == POSITION_ATTITUDE_MODEL_16 &&
					 calibrated < 3)
			{
				attitude_get_molar_datum();
				calibrated++;
			}
			else if (temp_time < MAX_BRUSH_TIME)
			{
					switch (main_get_brush_cb()->positon_model)
					{
					case POSITION_ATTITUDE_MODEL_4:
						position = attitude_get_16position();
						break;
					case POSITION_ATTITUDE_MODEL_HHNFT:
						position = attitude_get_5position();
						break;
//					case POSITION_ATTITUDE_MODEL_11:
//						position = attitude_get_11position();
//						break;
					case POSITION_ATTITUDE_MODEL_16:
						position = attitude_get_16position();
						break;
					}
			}
			else
			{
				position = 0xfe;
			}

			if (position != 0xfe)
				one_brush.pos[position] += 1;

			if (main_get_brush_cb()->interactive_duration > 0)
			{
				cmd_async_feedback_F5();
			}
		//		if (fsm_get_brush_cb()->calibra_mode == 1)
		//		{
		//			uint8 per;
		//
		//			if (attitude_atri.fiterror < 340)
		//				per = attitude_atri.fiterror * 80 / 340;
		//			else
		//				per = 100;
		//
		//			cmd_async_feedback_F6(per);
		//		}
		}
		else
		{
			vTaskDelay( 100 );
		}
	}
} /*lint !e715 !e818 !e830 Function definition must be standard for task creation. */


void attitude_record_start(void)
{
    memset((uint8*)&one_brush,0,sizeof(single_brushing_data));
    one_brush.model = main_get_brush_cb()->positon_model;
    one_brush.time = hal_rtc_get_unixtime();
    current_time = 0;
    current_rtctime = hal_rtc_get_unixtime();
    calibrated = 0;
    acc_count = 0;
    log_i("attitude_record_start model:%ld",one_brush.model);
}

void attitude_record_stop(void)
{
    if(!main_get_brush_cb()->interactive_mode)
        current_time += (hal_rtc_get_unixtime() - current_rtctime);

    if(current_time >= MIN_BRUSH_TIME)
    {
    	main_get_brush_cb()->interactive_duration = 0;
    	main_get_brush_cb()->interactive_mode = 0;
        one_brush.model = main_get_brush_cb()->positon_model;
        one_brush.span = current_time <= MAX_BRUSH_TIME ? current_time : MAX_BRUSH_TIME;
        main_get_brush_cb()->positon_model = POSITION_ATTITUDE_MODEL_16;
        data_write_new_single_data(&one_brush);
    }
    else
    {
    	main_get_brush_cb()->interactive_duration = 0;
    	main_get_brush_cb()->interactive_mode = 0;
        one_brush.model = main_get_brush_cb()->positon_model;
        main_get_brush_cb()->positon_model = POSITION_ATTITUDE_MODEL_16;
    }

    Fusion_task_init();
    log_i("attitude_record_stop time:%d",one_brush.span);
}

//void attitude_game_pause(void)
//{
////    dev_motor_pwm_Off();
////    cmd_async_feedback_F2(DEV_MOTOR_STOP);
//    main_get_brush_cb()->interactive_mode = 1;
//    current_time += hal_rtc_get_unixtime() - current_rtctime;
////    fsm_set_brush_state(brush_state_motor_pause);
//
//    log_i("attitude_game_pause");
//}
//
//void attitude_game_resume(void)
//{
////    dev_sensor_open();
////    dev_motor_pwm_On();
////    dev_ssread_start();
////    cmd_async_feedback_F2(DEV_MOTOR_START);
//	main_get_brush_cb()->interactive_mode = 0;
//    current_rtctime = hal_rtc_get_unixtime();
//
////    fsm_set_brush_state(brush_state_motor_on);
//
//    log_i("attitude_game_resume\r\n");
//}

void attitdue_task_start(void)
{
	while (!hal_msensor_21002_open());
	while (!hal_msensor_8700_open());
	cm_sys_clk_set(sysclk_PLL96);
	control_flag = TASK_RUN;
}

void attitdue_task_stop(void)
{
	control_flag = TASK_STOP;
	while (!hal_msensor_21002_close());
	while (!hal_msensor_8700_close());
	cm_sys_clk_set(sysclk_XTAL16M);
}

void attitude_reset(void)
{
	RdSensData_task_init();
}

void attitude_init(void)
{
	RdSensData_task_init();
	Fusion_task_init();
	MagCal_task_init();

	uart_config uart_init =
	{
			.baud_rate = HW_UART_BAUDRATE_115200,
			.data = HW_UART_DATABITS_8,
			.stop = HW_UART_STOPBITS_1,
			.parity = HW_UART_PARITY_NONE,
			.use_dma = 1,
			.use_fifo = 0,
			.rx_dma_channel = HW_DMA_CHANNEL_2,
			.tx_dma_channel = HW_DMA_CHANNEL_3,
	};

	hw_uart_init(HW_UART1, &uart_init);

	/* Create the three tasks. */
    OS_TASK_CREATE("readtask",attitude_readtask,NULL,800,OS_TASK_PRIORITY_HIGHEST,rd_handle);
    OS_TASK_CREATE("fusiontask",attitude_fusiontask,NULL,2000,OS_TASK_PRIORITY_NORMAL,fs_handle);
    OS_TASK_CREATE("magtask",attitude_magtask,NULL,800,OS_TASK_PRIORITY_NORMAL,mg_handle);
    OS_TASK_CREATE("satask",attitude_satask,NULL,2000,OS_TASK_PRIORITY_NORMAL,sa_handle);

}
