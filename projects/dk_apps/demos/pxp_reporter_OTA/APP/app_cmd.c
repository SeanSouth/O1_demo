/* ========================================
 *
 * Copyright YOUR COMPANY, THE YEAR
 * All Rights Reserved
 * UNPUBLISHED, LICENSED SOFTWARE.
 *
 * CONFIDENTIAL AND PROPRIETARY INFORMATION
 * WHICH IS THE PROPERTY OF your company.
 *
 * ========================================
*/

#include "app_Cmd.h"
#include "stdlib.h"
#include "string.h"
#include "ad_battery.h"

#include "app_main.h"
#include "app_data.h"
#include "app_attitude.h"

#include "..\BLE\ble_interface.h"

#include "..\LIB\easylogger\easyflash.h"
#include "..\LIB\easylogger\elog.h"
#include "..\LIB\cJSON.h"
#include "..\HAL\hal_rtc.h"

static char tag[]="app_cmd";
#define log_a(...) elog_a(tag, __VA_ARGS__)
#define log_e(...) elog_e(tag, __VA_ARGS__)
#define log_w(...) elog_w(tag, __VA_ARGS__)
#define log_i(...) elog_i(tag, __VA_ARGS__)
#define log_d(...) elog_d(tag, __VA_ARGS__)
#define log_v(...) elog_v(tag, __VA_ARGS__)


cJSON * cmd_set_time(cJSON *rxbuf)
{
    cJSON *txbuf;  
    cJSON *data;
    cJSON *time;
    struct tm time_t;

    data = cJSON_CreateObject();
    txbuf = cJSON_CreateObject();
    
    time =  cJSON_GetObjectItem(rxbuf,"time");
    if (!time) 
    {
        log_i("cmd_set_userid Error : [%s]",cJSON_GetErrorPtr());
        cJSON_AddNumberToObject(txbuf,"result",0);
    }
    else if(time->type == cJSON_String)
    {
        strptime(time->valuestring, TIME_FMT, &time_t);
        hal_rtc_set_time(time_t);
        log_i("cmd_set_time:%s",time->valuestring);
        cJSON_AddNumberToObject(txbuf,"result",1);
    }

    cJSON_AddItemToObject(txbuf,"data",data);
    
    return txbuf;
}

cJSON * cmd_get_time(cJSON *rxbuf)
{
    cJSON *txbuf;  
    cJSON *data;

    char timestr[30] = {0};

    data = cJSON_CreateObject();
    txbuf = cJSON_CreateObject();
    
    hal_rtc_get_time(timestr);

    cJSON_AddStringToObject(data,"time",timestr);

    log_i("cmd_get_time");

    cJSON_AddNumberToObject(txbuf,"result",1);
    cJSON_AddItemToObject(txbuf,"data",data);
    
    return txbuf;
}

cJSON * cmd_get_state(cJSON *rxbuf)
{
    cJSON *txbuf;
    cJSON *data;
    uint8 state;

    data = cJSON_CreateObject();
    txbuf = cJSON_CreateObject();

    state = main_get_brush_cb()->current_state;
    cJSON_AddNumberToObject(data,"state",state);

    log_i("cmd_get_state");

    cJSON_AddNumberToObject(txbuf,"result",1);
    cJSON_AddItemToObject(txbuf,"data",data);

    return txbuf;
}

cJSON * cmd_set_userid(cJSON *rxbuf)
{
    cJSON *txbuf;
    cJSON *data;
    cJSON *userid;
    uint8 res;

    data = cJSON_CreateObject();
    txbuf = cJSON_CreateObject();

    userid =  cJSON_GetObjectItem(rxbuf,"userid");
    if (!userid)
    {
        log_i("cmd_set_userid Error : [%s]\n",cJSON_GetErrorPtr());
        cJSON_AddNumberToObject(txbuf,"result",0);
    }
    else if(userid->type == cJSON_String)
    {
        res = ef_set_env("userid",userid->string);
        res = ef_save_env();
        log_i("cmd_set_userid");
        cJSON_AddNumberToObject(txbuf,"result",1);
    }

    cJSON_AddItemToObject(txbuf,"data",data);

    return txbuf;
}


cJSON * cmd_get_userid(cJSON *rxbuf)
{
    cJSON *txbuf;
    cJSON *data;
    char *userid;

    data = cJSON_CreateObject();
    txbuf = cJSON_CreateObject();

    userid = ef_get_env("userid");
    cJSON_AddStringToObject(data,"userid",userid);

    log_i("cmd_get_userid");

    cJSON_AddNumberToObject(txbuf,"result",1);
    cJSON_AddItemToObject(txbuf,"data",data);


    return txbuf;
}
//
//cJSON * cmd_restore_factory(cJSON *rxbuf)
//{
//    cJSON *txbuf;
//    cJSON *data;
//    uint8 *userid;
//
//    data = cJSON_CreateObject();
//    txbuf = cJSON_CreateObject();
//
//    data_reset_data();
//
//    log_i("cmd_restore_factory\r\n");
//
//    cJSON_AddNumberToObject(txbuf,"result",1);
//    cJSON_AddItemToObject(txbuf,"data",data);
//
//    return txbuf;
//}
//
//cJSON * cmd_get_position(cJSON *rxbuf)
//{
//    cJSON *txbuf;
//    cJSON *data;
//    uint8 pos = 0;
//
//    data = cJSON_CreateObject();
//    txbuf = cJSON_CreateObject();
//
//    if(fsm_get_brush_cb()->positon_model == POSITION_ATTITUDE_MODEL_HHNFT)
//        pos = attitude_get_5position();
//    cJSON_AddNumberToObject(data,"position",pos);
//
//    log_i("cmd_get_position:%d\r\n",pos);
//
//    cJSON_AddNumberToObject(txbuf,"result",1);
//    cJSON_AddItemToObject(txbuf,"data",data);
//
//    return txbuf;
// }
//
cJSON * cmd_get_last_record(cJSON *rxbuf)
{
    cJSON *txbuf;
    cJSON *data=0;
    cJSON *pos;
    char tmp[256];
    uint8 res,i;

    txbuf = cJSON_CreateObject();

    res = data_get_last_single_data(tmp);
    if(res)
    {
        cJSON_AddNumberToObject(txbuf,"result",0);
    }
    else
    {
    	data = cJSON_Parse(tmp);
    	cJSON_AddNumberToObject(data,"serial",1);
    	cJSON_AddItemToObject(txbuf,"data",data);
        cJSON_AddNumberToObject(txbuf,"result",1);
    }

    log_i("cmd_get_last_record:%d",res);

    return txbuf;
 }

cJSON * cmd_get_all_record(cJSON *rxbuf)
{
    cJSON *txbuf;
    cJSON *data;
    uint8 total = 0;

    data = cJSON_CreateObject();
    txbuf = cJSON_CreateObject();

    total = data_get_total_rcrd_num();
    cJSON_AddNumberToObject(data,"total",total);

    log_i("cmd_get_all_record:%d",total);

    cJSON_AddNumberToObject(txbuf,"result",1);
    cJSON_AddItemToObject(txbuf,"data",data);

    return txbuf;
 }

cJSON * cmd_get_specified_record(cJSON *rxbuf)
{
    cJSON *txbuf;
    cJSON *data=0;
    cJSON *serial;
    cJSON *pos;
    char tmp[256];
    uint8 res,i,num;


    txbuf = cJSON_CreateObject();

    serial =  cJSON_GetObjectItem(rxbuf,"serial");
    if (!serial)
    {
        log_i("cmd_get_specified_record Error : [%s]\n",cJSON_GetErrorPtr());
        cJSON_AddNumberToObject(txbuf,"result",0);
    }
    else if(serial->type == cJSON_Number)
    {
        num = serial->valueint;

        res = data_get_specified_single_data(num,tmp);
        if(res)
        {
            cJSON_AddNumberToObject(txbuf,"result",0);
        }
        else
        {
        	data = cJSON_Parse(tmp);
        	cJSON_AddItemToObject(data,"serial",serial);
            cJSON_AddItemToObject(txbuf,"data",data);
            cJSON_AddNumberToObject(txbuf,"result",1);
        }

        log_i("cmd_get_specified_record:%d",num);
    }

    return txbuf;
 }

cJSON * cmd_get_pos_model(cJSON *rxbuf)
{
    cJSON *txbuf;
    cJSON *data;
    uint8 model = 0;

    data = cJSON_CreateObject();
    txbuf = cJSON_CreateObject();

    model = main_get_brush_cb()->positon_model;
    cJSON_AddNumberToObject(data,"model",model);

    log_i("cmd_get_pos_model:%d",model);

    cJSON_AddNumberToObject(txbuf,"result",1);
    cJSON_AddItemToObject(txbuf,"data",data);

    return txbuf;
}

cJSON * cmd_set_pos_model(cJSON *rxbuf)
{
    cJSON *txbuf;
    cJSON *data;
    cJSON *model;

    data = cJSON_CreateObject();
    txbuf = cJSON_CreateObject();

    model =  cJSON_GetObjectItem(rxbuf,"model");
    if (!model)
    {
        log_i("cmd_set_userid Error : [%s]\n",cJSON_GetErrorPtr());
        cJSON_AddNumberToObject(txbuf,"result",0);
    }
    else if(model->type == cJSON_Number)
    {
    	main_get_brush_cb()->positon_model = model->valueint;
        log_i("cmd_set_pos_model:%d",model->valueint);
        cJSON_AddNumberToObject(txbuf,"result",1);
    }

    cJSON_AddItemToObject(txbuf,"data",data);

    return txbuf;
}

cJSON * cmd_set_interactive(cJSON *rxbuf)
{
    cJSON *txbuf;
    cJSON *data;
    cJSON *duration;

    data = cJSON_CreateObject();
    txbuf = cJSON_CreateObject();

    duration =  cJSON_GetObjectItem(rxbuf,"duration");
    if (!duration)
    {
        log_i("cmd_set_interactive Error : [%s]\n",cJSON_GetErrorPtr());
        cJSON_AddNumberToObject(txbuf,"result",0);
    }
    else if(duration->type == cJSON_Number)
    {
        if(duration->valueint > 0)
        {
        	main_get_brush_cb()->interactive_duration = duration->valueint;
        	main_get_brush_cb()->interactive_mode = 0;

        	main_switch_mode(MAIN_MODE_DETECTED);

            cJSON_AddNumberToObject(txbuf,"result",1);
        }
        else if(duration->valueint == 0)
        {
            if(main_get_brush_cb()->interactive_duration > 0)
            {
            	main_get_brush_cb()->interactive_duration = duration->valueint;
            	main_get_brush_cb()->interactive_mode = 0;

            	main_switch_mode(MAIN_MODE_DETECTED);

                cJSON_AddNumberToObject(txbuf,"result",2);
            }
            else
            {
                cJSON_AddNumberToObject(txbuf,"result",0);
            }

        }
        log_i("cmd_set_interactive:%d\r\n",duration->valueint);
    }

    cJSON_AddItemToObject(txbuf,"data",data);

    return txbuf;
}

cJSON * cmd_set_pause(cJSON *rxbuf)
{
    cJSON *txbuf;
    cJSON *data;
    cJSON *control;

    data = cJSON_CreateObject();
    txbuf = cJSON_CreateObject();

    control =  cJSON_GetObjectItem(rxbuf,"control");
    if (!control)
    {
        log_i("cmd_set_pause Error : [%s]\n",cJSON_GetErrorPtr());
        cJSON_AddNumberToObject(txbuf,"result",0);
    }
    else if(control->type == cJSON_Number)
    {

//        if(control->valueint == 0 && main_get_brush_cb()->interactive_duration > 0 && fsm_get_brush_state() == brush_state_motor_on)
//        {
//            dev_motor_pwm_Off();
//            dev_led_swtich_control(1,LED_OFF);
//            fsm_set_brush_state(brush_state_motor_pause);
//        }
//        else if(control->valueint == 1 && main_get_brush_cb()->interactive_duration > 0 && fsm_get_brush_state() == brush_state_motor_pause)
//        {
//            dev_motor_pwm_On();
//            dev_led_swtich_control(1,LED_ON);
//            fsm_set_brush_state(brush_state_motor_on);
//        }

        log_i("cmd_set_pause\r\n");
        cJSON_AddNumberToObject(txbuf,"result",0);
    }


    cJSON_AddItemToObject(txbuf,"data",data);

    return txbuf;
}

cJSON * cmd_clean_brush_data(cJSON *rxbuf)
{
    cJSON *txbuf;
    cJSON *data;

    data = cJSON_CreateObject();
    txbuf = cJSON_CreateObject();

    data_clear_all_data();
    log_i("cmd_clean_brush_data");
    cJSON_AddNumberToObject(txbuf,"result",1);

    cJSON_AddItemToObject(txbuf,"data",data);

    return txbuf;
}
//
//cJSON * cmd_set_led1(cJSON *rxbuf)
//{
//    cJSON *txbuf;
//    cJSON *data;
//    cJSON *state;
//
//    data = cJSON_CreateObject();
//    txbuf = cJSON_CreateObject();
//
//    state =  cJSON_GetObjectItem(rxbuf,"state");
//    if (!state)
//    {
//        log_i("cmd_set_userid Error : [%s]\n",cJSON_GetErrorPtr());
//        cJSON_AddNumberToObject(txbuf,"result",0);
//    }
//    else if(state->type == cJSON_Number)
//    {
//        dev_led_swtich_control(1,state->valueint);
//        log_i("cmd_set_led1:%d\r\n",state->valueint);
//        cJSON_AddNumberToObject(txbuf,"result",1);
//    }
//
//    cJSON_AddItemToObject(txbuf,"data",data);
//
//    return txbuf;
//}
//
//cJSON * cmd_set_led2(cJSON *rxbuf)
//{
//    cJSON *txbuf;
//    cJSON *data;
//    cJSON *state;
//
//    data = cJSON_CreateObject();
//    txbuf = cJSON_CreateObject();
//
//    state =  cJSON_GetObjectItem(rxbuf,"state");
//    if (!state)
//    {
//        log_i("cmd_set_userid Error : [%s]\n",cJSON_GetErrorPtr());
//        cJSON_AddNumberToObject(txbuf,"result",0);
//    }
//    else if(state->type == cJSON_Number)
//    {
//        dev_led_swtich_control(2,state->valueint);
//        log_i("cmd_set_led2:%d\r\n",state->valueint);
//        cJSON_AddNumberToObject(txbuf,"result",1);
//    }
//
//    cJSON_AddItemToObject(txbuf,"data",data);
//
//    return txbuf;
//}
//
//cJSON * cmd_set_motor(cJSON *rxbuf)
//{
//    cJSON *txbuf;
//    cJSON *data;
//    cJSON *state;
//
//    data = cJSON_CreateObject();
//    txbuf = cJSON_CreateObject();
//
//    state =  cJSON_GetObjectItem(rxbuf,"state");
//    if (!state)
//    {
//        log_i("cmd_set_userid Error : [%s]\n",cJSON_GetErrorPtr());
//        cJSON_AddNumberToObject(txbuf,"result",0);
//    }
//    else if(state->type == cJSON_Number)
//    {
//        if(state->valueint == 1)
//            dev_motor_pwm_On();
//        else if(state->valueint == 0)
//            dev_motor_pwm_Off();
//
//        log_i("cmd_set_motor:%d\r\n",state->valueint);
//        cJSON_AddNumberToObject(txbuf,"result",1);
//    }
//
//    cJSON_AddItemToObject(txbuf,"data",data);
//
//    return txbuf;
//}
//
//cJSON * cmd_let_dev_reset(cJSON *rxbuf)
//{
//    dev_software_reset();
//    return 0;
//}
//
cJSON * cmd_get_uniqueid(cJSON *rxbuf)
{
    cJSON *txbuf;
    cJSON *data;
    uint64 uniqueid = 0;
    uint8 *ptr;
    char *uuid;

    data = cJSON_CreateObject();
    txbuf = cJSON_CreateObject();

    uuid = ef_get_env("uuid");
    cJSON_AddStringToObject(data,"uniqueid",uuid);

    log_i("cmd_get_uniqueid:%s\r\n",uuid);

    cJSON_AddNumberToObject(txbuf,"result",1);
    cJSON_AddItemToObject(txbuf,"data",data);

    return txbuf;
}
//
//cJSON * cmd_get_qrcode(cJSON *rxbuf)
//{
//    cJSON *txbuf;
//    cJSON *data;
//    uint64 crcode = 0;
//    uint8 *ptr;
//    char qrcode[30] = {0};
//
//    data = cJSON_CreateObject();
//    txbuf = cJSON_CreateObject();
//
//    CyGetUniqueId((uint32*)&crcode);
//    ptr = (uint8*)&crcode;
//    sprintf(qrcode,"%02X%02X%02X%02X%02X%02X%02X%02X",ptr[7],ptr[6],ptr[5],ptr[4],ptr[3],ptr[2],ptr[1],ptr[0]);
//    cJSON_AddStringToObject(data,"qrcode",qrcode);
//
//    log_i("cmd_get_pos_model:%s\r\n",qrcode);
//
//    cJSON_AddNumberToObject(txbuf,"result",1);
//    cJSON_AddItemToObject(txbuf,"data",data);
//
//    return txbuf;
//}
//
//cJSON * cmd_enter_bootloader(cJSON *rxbuf)
//{
//    CySoftwareReset();
//    return 0;
//}
//
cJSON * cmd_get_bat(cJSON *rxbuf)
{
    cJSON *txbuf;
    cJSON *data;
    uint16 bat_voltage = 0;
    battery_source adbat;

    data = cJSON_CreateObject();
    txbuf = cJSON_CreateObject();

    adbat = ad_battery_open();
    bat_voltage = ad_battery_raw_to_mvolt(adbat, ad_battery_read(adbat));
    ad_battery_close(adbat);
    cJSON_AddNumberToObject(data,"mvolt",bat_voltage);

    log_i("cmd_get_bat:%d",bat_voltage);

    cJSON_AddNumberToObject(txbuf,"result",1);
    cJSON_AddItemToObject(txbuf,"data",data);

    return txbuf;
}
//
//cJSON * cmd_ble_disconnect(cJSON *rxbuf)
//{
//    ble_disconnect();
//
//    log_i("cmd_binding_req\r\n");
//
//    return 0;
//}
//
//cJSON * cmd_check_switch(cJSON *rxbuf)
//{
//    cJSON *txbuf;
//    cJSON *data;
//    uint16 res = 0;
//
//    data = cJSON_CreateObject();
//    txbuf = cJSON_CreateObject();
//
//
//    log_i("cmd_check_switch:%d\r\n",res);
//
//    cJSON_AddNumberToObject(txbuf,"result",1);
//    cJSON_AddItemToObject(txbuf,"data",data);
//
//    return txbuf;
//}
//

cJSON * cmd_binding_req(cJSON *rxbuf)
{
	main_switch_mode(MAIN_MODE_BIND);
    log_i("cmd_binding_req\r\n");

    return 0;
}

cJSON * cmd_complete_packet(cJSON *rxbuf)
{
    cJSON *txbuf;
    cJSON *data;

    data = cJSON_CreateObject();
    txbuf = cJSON_CreateObject();

    log_i("cmd_complete_packet\r\n");
    main_get_brush_cb()->need_devided_packet = COMPLETE_PACKET;

    cJSON_AddNumberToObject(txbuf,"result",1);
    cJSON_AddItemToObject(txbuf,"data",data);

    return txbuf;
}

cJSON * cmd_set_dominant_hand(cJSON *rxbuf)
{
    cJSON *txbuf;
    cJSON *data;
    cJSON *hand;
    int res;
    char stt[10]={0};

    data = cJSON_CreateObject();
    txbuf = cJSON_CreateObject();

    hand =  cJSON_GetObjectItem(rxbuf,"hand");
    if (!hand)
    {
        log_i("cmd_set_dominant_hand Error : [%s]\n",cJSON_GetErrorPtr());
        cJSON_AddNumberToObject(txbuf,"result",0);
    }
    else if(hand->type == cJSON_Number)
    {
    	sprintf(stt,"%d",hand->valueint);
        res = ef_set_env("domain",stt);
        res = ef_save_env();
        main_get_brush_cb()->dominant_hand = hand->valueint;
        log_i("cmd_set_dominant_hand:%d",hand->valueint);
        cJSON_AddNumberToObject(txbuf,"result",1);
    }

    cJSON_AddItemToObject(txbuf,"data",data);

    return txbuf;
}

cJSON * cmd_set_current_apppos(cJSON *rxbuf)
{
    cJSON *txbuf;
    cJSON *data;
    cJSON *apppos;

    data = cJSON_CreateObject();
    txbuf = cJSON_CreateObject();

    apppos =  cJSON_GetObjectItem(rxbuf,"pos");
    if (!apppos)
    {
        log_i("cmd_set_current_apppos Error : [%s]\n",cJSON_GetErrorPtr());
        cJSON_AddNumberToObject(txbuf,"result",0);
    }
    else if(apppos->type == cJSON_Number)
    {
    	main_get_brush_cb()->current_apppos = apppos->valueint;
        log_i("cmd_set_current_apppos:%d\r\n",apppos->valueint);
        cJSON_AddNumberToObject(txbuf,"result",1);
    }

    cJSON_AddItemToObject(txbuf,"data",data);

    return txbuf;
}

//cJSON * cmd_set_calibration(cJSON *rxbuf)
//{
//    cJSON *txbuf;
//    cJSON *data;
//    cJSON *control;
//
//    data = cJSON_CreateObject();
//    txbuf = cJSON_CreateObject();
//
//    control =  cJSON_GetObjectItem(rxbuf,"control");
//    if (!control)
//    {
//        log_i("cmd_set_current_apppos Error : [%s]\n",cJSON_GetErrorPtr());
//        cJSON_AddNumberToObject(txbuf,"result",0);
//    }
//    else if(control->type == cJSON_Number)
//    {
//        if(control->valueint == 1)
//        {
//            if(dev_get_pib()->motor_state == DEV_MOTOR_START)
//                dev_key_1_switch_handle();
//            attitude_fusion_init();
//            fsm_get_brush_cb()->calibra_mode = 1;
//            fsm_set_brush_state(brush_state_motor_change);
//        }
//        else if(control->valueint == 0)
//        {
//            dev_calibra_timeout_callback();
//        }
//        log_i("cmd_set_current_apppos:%d\r\n",control->valueint);
//        cJSON_AddNumberToObject(txbuf,"result",control->valueint);
//    }
//
//    cJSON_AddItemToObject(txbuf,"data",data);
//
//    return txbuf;
//}

cmd_table ble_cmd[]= 
{
    {0xA1,cmd_set_time},
    {0xA2,cmd_get_time},
    {0xA3,cmd_get_state},
    {0xA4,cmd_set_userid},
    {0xA5,cmd_get_userid},
//    {0xA6,cmd_restore_factory},
    {0xA7,cmd_complete_packet},
    {0xA8,cmd_set_dominant_hand},
//    {0xB1,cmd_get_position},
    {0xB2,cmd_get_last_record},
    {0xB3,cmd_get_all_record},
    {0xB4,cmd_get_specified_record},
    {0xB5,cmd_get_pos_model},
    {0xB6,cmd_set_pos_model},
    {0xB7,cmd_set_interactive},
    {0xB8,cmd_set_pause},
    {0xB9,cmd_clean_brush_data},
    {0xBA,cmd_set_current_apppos},
//    {0xBB,cmd_set_calibration},
//    {0xC1,cmd_set_led1},
//    {0xC2,cmd_set_led1},
//    {0xC3,cmd_set_motor},
//    {0xC4,cmd_let_dev_reset},
    {0xC5,cmd_get_uniqueid},
//    {0xC6,cmd_get_qrcode},
//    {0xC7,cmd_enter_bootloader},
    {0xC8,cmd_get_bat},
//    {0xC9,cmd_ble_disconnect},
//    {0xCA,cmd_check_switch},
    {0xF1,cmd_binding_req},
    {0,NULL}
};

void cmd_send(cJSON *send,uint8 cmd)
{
	char *strout = NULL;

	if (send != NULL)
	{
		cJSON_AddNumberToObject(send, "command",
				cmd);

		strout = cJSON_PrintUnformatted(send);
		if(strout)
		{
			ble_app_send_cmd(strout,strlen(strout),cmd);
			free(strout);
		}
		else
		{
			log_e("cmd_send NULL !");
		}
	}
}

void cmd_async_feedback_F1(uint8 res)
{
    cJSON *txbuf;
    cJSON *data;

    data = cJSON_CreateObject();
    txbuf = cJSON_CreateObject();

    log_i("cmd_async_feedback_F1:%d\r\n",res);

    cJSON_AddNumberToObject(txbuf,"result",res);
    cJSON_AddItemToObject(txbuf,"data",data);

    cJSON_AddNumberToObject(txbuf,"command",0xF1);
    cmd_send(txbuf,0xF1);
    cJSON_Delete(txbuf);
}

void cmd_async_feedback_F2(uint8 state)
{
    cJSON *txbuf;
    cJSON *data;

    data = cJSON_CreateObject();
    txbuf = cJSON_CreateObject();

    log_i("cmd_async_feedback_F2:%d\r\n",state);

    cJSON_AddNumberToObject(txbuf,"result",state);
    cJSON_AddItemToObject(txbuf,"data",data);

    cJSON_AddNumberToObject(txbuf,"command",0xF2);
    cmd_send(txbuf,0xF2);
    cJSON_Delete(txbuf);
}
//
//void cmd_async_feedback_F3(void)
//{
//    cJSON *txbuf;
//    cJSON *data;
//    cJSON *pos;
//    uint8 total = data_get_total_rcrd_num();
//    uint8 i,j,res;
//    single_brushing_data tmp;
//
//    log_i("cmd_async_feedback_F3:%d\r\n",total);
//
//    for(i=1;i<=total;i++)
//    {
//        data = cJSON_CreateObject();
//        txbuf = cJSON_CreateObject();
//
//        memset((uint8*)&tmp,0,sizeof(single_brushing_data));
//        res = data_get_specified_single_data(i,&tmp);
//        if(res != CYRET_SUCCESS)
//        {
//            cJSON_AddNumberToObject(txbuf,"result",0);
//            cJSON_AddItemToObject(txbuf,"data",data);
//            cJSON_AddNumberToObject(txbuf,"command",0xF3);
//            cmd_send(txbuf,0xF3);
//            cJSON_Delete(txbuf);
//            break;
//        }
//        else
//        {
//            cJSON_AddNumberToObject(data,"model",tmp.model);
//            cJSON_AddNumberToObject(data,"time",tmp.time);
//            cJSON_AddNumberToObject(data,"span",tmp.span);
//            pos = cJSON_CreateArray();
//            for(j=0;j<DATA_POS_LEN;j++)
//            {
//
//                cJSON_AddItemToArray(pos,cJSON_CreateNumber(tmp.pos[j]));
//            }
//            cJSON_AddItemToObject(data,"pos",pos);
//            cJSON_AddNumberToObject(data,"serial",i);
//            cJSON_AddNumberToObject(txbuf,"result",1);
//            cJSON_AddItemToObject(txbuf,"data",data);
//            cJSON_AddNumberToObject(txbuf,"command",0xF3);
//            cmd_send(txbuf,0xF3);
//            cJSON_Delete(txbuf);
//        }
//    }
//}
//
//void cmd_async_feedback_F4(uint8 pos)
//{
//    cJSON *txbuf;
//    cJSON *data;
//
//    data = cJSON_CreateObject();
//    txbuf = cJSON_CreateObject();
//
//    log_i("cmd_async_feedback_F2:%d\r\n",pos);
//
//    cJSON_AddNumberToObject(txbuf,"result",pos);
//    cJSON_AddItemToObject(txbuf,"data",data);
//
//    cJSON_AddNumberToObject(txbuf,"command",0xF4);
//    cmd_send(txbuf,0xF4);
//    cJSON_Delete(txbuf);
//
//}

void cmd_async_feedback_F5()
{
    cJSON *txbuf;
    cJSON *data;
    cJSON *sensor;
    uint8 j;
    Attitude_Data *data_ptr = attitude_get_struct();

    data = cJSON_CreateObject();
    txbuf = cJSON_CreateObject();

    log_i("cmd_async_realtime_F5\r\n");

    cJSON_AddNumberToObject(txbuf,"result",1);
    sensor = cJSON_CreateArray();
    cJSON_AddItemToArray(sensor,cJSON_CreateNumber(data_ptr->ned[0]*0.1));
    cJSON_AddItemToArray(sensor,cJSON_CreateNumber(data_ptr->ned[1]*0.1));
    cJSON_AddItemToArray(sensor,cJSON_CreateNumber(data_ptr->ned[2]*0.1));
    cJSON_AddItemToArray(sensor,cJSON_CreateNumber(data_ptr->accxa));
    cJSON_AddItemToArray(sensor,cJSON_CreateNumber(data_ptr->accxf));
    cJSON_AddItemToArray(sensor,cJSON_CreateNumber(data_ptr->accya));
    cJSON_AddItemToArray(sensor,cJSON_CreateNumber(data_ptr->accyf));
    cJSON_AddItemToArray(sensor,cJSON_CreateNumber(main_get_brush_cb()->current_apppos));
    cJSON_AddItemToObject(data,"sensor",sensor);
    cJSON_AddItemToObject(txbuf,"data",data);

    cJSON_AddNumberToObject(txbuf,"command",0xF5);
    cmd_send(txbuf,0xF5);
    cJSON_Delete(txbuf);
}
//
//void cmd_async_feedback_F6(uint8 percent)
//{
//    cJSON *txbuf;
//    cJSON *data;
//    uint8 j;
//    int16 *data_ptr = (int16*)attitude_get_struct();
//
//    data = cJSON_CreateObject();
//    txbuf = cJSON_CreateObject();
//
//    log_i("cmd_async_realtime_F6:%d\r\n");
//
//    cJSON_AddNumberToObject(txbuf,"result",1);
//    cJSON_AddNumberToObject(data,"percent",percent);
//    cJSON_AddItemToObject(txbuf,"data",data);
//
//    cJSON_AddNumberToObject(txbuf,"command",0xF6);
//    cmd_send(txbuf,0xF6);
//    cJSON_Delete(txbuf);
//}
//
//void cmd_async_feedback_F7(char* debug)
//{
//    cJSON *txbuf;
//    cJSON *data;
//    uint8 j;
//    int16 *data_ptr = (int16*)attitude_get_struct();
//
//    data = cJSON_CreateObject();
//    txbuf = cJSON_CreateObject();
//
//    log_i("cmd_async_realtime_F7\r\n");
//
//    cJSON_AddNumberToObject(txbuf,"result",1);
//    cJSON_AddStringToObject(data,"debug",debug);
//    cJSON_AddItemToObject(txbuf,"data",data);
//
//    cJSON_AddNumberToObject(txbuf,"command",0xF7);
//    cmd_send(txbuf,0xF7);
//    cJSON_Delete(txbuf);
//}

void cmd_parse(uint8 *in, uint16 inlen, uint8 *out, uint16 *outlen,
		uint8 *txcmd)
{
	uint8 i;
	char *strout;
	cJSON *recv;
	cJSON *cmd;
	cJSON *param;
	cJSON *send;

//    if(in[0] == 0xa9)
//    {
//        send = cmd_get_last_record(0);
//        if(send != NULL)
//        {
//            cJSON_AddNumberToObject(send,"command",0xb2);
//            cmd_send(send,0xb2);
//            cJSON_Delete(send);
//        }
//        return;
//    }
//
    if(in[0] == 0xa7)
    {
        send = cmd_complete_packet(0);
		if (send != NULL)
		{
			cJSON_AddNumberToObject(send, "command",
					0xa7);

			if (send)
			{
				strout = cJSON_PrintUnformatted(send);
				memset((char*) out, 0, sizeof(256));
				strcpy((char*) out, strout);
				*outlen = strlen((char*) out);
				*txcmd = cmd->valueint;
				free(strout);
			}

			cJSON_Delete(send);
		}
        return;
    }

	*outlen = 0;

	recv = cJSON_Parse((char*) in);
	if (!recv)
	{
		log_i("Error before: [%s]\n", cJSON_GetErrorPtr());
		return;
	}
	else
	{
		cmd = cJSON_GetObjectItem(recv, "command");
		if (!cmd)
		{
			log_i("Error before: [%s]\n", cJSON_GetErrorPtr());
		}
		else if (cmd->type == cJSON_Number)
		{
			log_i("cmd_parse flag:%x", cmd->valueint);
			for (i = 0; i < (sizeof(ble_cmd) / sizeof(ble_cmd[0])); i++)
			{
				if (cmd->valueint == ble_cmd[i].cmd)
				{
					param = cJSON_GetObjectItem(recv, "param");
					if (param && param->type == cJSON_Object)
					{
						send = (ble_cmd[i].funcptr)(param);
						if (send != NULL)
						{
							cJSON_AddNumberToObject(send, "command",
									cmd->valueint);

							if (send)
							{
								strout = cJSON_PrintUnformatted(send);
								memset((char*) out, 0, sizeof(256));
								strcpy((char*) out, strout);
								*outlen = strlen((char*) out);
								*txcmd = cmd->valueint;
								free(strout);
							}

							cJSON_Delete(send);
						}
					}
					break;
				}
			}
		}
		cJSON_Delete(recv);
	}
}

/* [] END OF FILE */
