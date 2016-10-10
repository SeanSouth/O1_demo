/*
 * app_debug.c
 *
 *  Created on: 2016Äê8ÔÂ24ÈÕ
 *      Author: Administrator
 */
#include "stdio.h"
#include "string.h"
#include "app_debug.h"

#include "stddef.h"
#include "osal.h"
#include "platform_devices.h"
#include "sys_power_mgr.h"
#include ".\mcif\mcif.h"




OS_TASK debug_task_h;
OS_TASK debug_status_h;

OS_QUEUE	txq;
OS_QUEUE	rxq;

TimerHandle_t Handle;

bool check_cmd(char *buf)
{
	int tmp;

	char standard[7] = "O1_cmd";

	char cut_buf[7] = {0};

	strncpy(cut_buf,buf,6);

	tmp = strcmp(standard,cut_buf);

	if(tmp==0)
		return 1;
	else
		return 0;
}

void delete_task(OS_TIMER timer)
{
	console_done();

	OS_TASK_DELETE(debug_task_h);
}
void delete_debug_task(void)
{
	Handle = OS_TIMER_CREATE("delete_debug_task", OS_MS_2_TICKS(3000),false,NULL, delete_task);

	OS_TIMER_START(Handle,OS_TIMER_FOREVER);
}
void debug_task(void *params)
{
	struct mcif_message_s *rxmsg;
	struct mcif_message_s *txmsg;
	char tmpstr[100];
	uint8 tmplen;

//	 printf("line 65!");

	while(1)
	{
        OS_BASE_TYPE ret;
        uint32_t notif;
//        printf("line 71!");
        ret = OS_TASK_NOTIFY_WAIT(0, OS_TASK_NOTIFY_ALL_BITS, &notif, OS_TASK_NOTIFY_FOREVER);
        /* Blocks forever waiting for the task notification. Therefore, the return value must
         * always be OS_OK
         */
//        printf("line 76!");
        OS_ASSERT(ret == OS_OK);

//        printf("line 79!");

        if (notif & MCIF_NOTIF)
        {
        	if(OS_QUEUE_GET(rxq,&rxmsg,0))
        	{
//        		printf("recv len:%d",rxmsg->len);
//        		printf("recv str:%s",rxmsg->buffer);
        		if (rxmsg->len == 1)
        		{
        			printf("the wield character is %d\r\n",rxmsg->buffer[0]);
        		}

        		sprintf(tmpstr,"recv len:%d!recv str!:%s\r\n",rxmsg->len,rxmsg->buffer);
        		tmplen = strlen(tmpstr);
                txmsg = OS_MALLOC(sizeof(struct mcif_message_s) + tmplen);
                memcpy(txmsg->buffer, tmpstr, tmplen);
                txmsg->len = tmplen;
        		mcif_ascii_send_response(0,0,txmsg,0);
        		OS_FREE(rxmsg);
        	}
        	else
        	{
        		printf("queue get nothing!");
        		mcif_ascii_print_prompt(0,0,0);
        	}

        }

	}
}

uint32_t t_count = 0;
void debug_taskstats_timer_cb(void)
{
	t_count++;
}

//void vConfigureTimerForRunTimeStats( void )
//{
//	hw_timer0_init(NULL);
//	hw_timer0_disable();
//	hw_timer0_set_clock_source(HW_TIMER0_CLK_SRC_SLOW);
//	hw_timer0_set_pwm_mode(HW_TIMER0_MODE_CLOCK);
//	hw_timer0_set_on_clock_div(false);
//	hw_timer0_set_on_reload(32);
//	hw_timer0_set_t0_reload(1,1);
//	hw_timer0_register_int(debug_taskstats_timer_cb);
//	hw_timer0_enable();
//}

uint32_t vGetRunTimerCounterValue( void )
{
    return t_count;
}


void debug_taskstats_task (void* pvParameters )
{
    char task_state[1000];

    ( void ) pvParameters;

    for(;;)
    {
        vTaskDelay( 1000 );

		/* Delay for half the flash period then turn the LED off. */


        memset(task_state,0,sizeof(task_state));
        vTaskGetRunTimeStats(task_state);
        //printf("%s",task_state);
    }
}

void debug_init(void)
{
	OS_TASK_CREATE("debug_task", debug_task, NULL, 800, OS_TASK_PRIORITY_NORMAL, debug_task_h);

	OS_QUEUE_CREATE(txq,sizeof(struct mcif_message_s*),10);
	OS_QUEUE_CREATE(rxq,sizeof(struct mcif_message_s*),10);

	mcif_setup_queues(0,txq,rxq);
	mcif_setup_client_notifications(0,debug_task_h,1);
	mcif_init();
}

