/*
 * BLE_task.c
 *
 *  Created on: 2016年8月18日
 *      Author: Administrator
 */

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "sys_watchdog.h"
#include "ble_interface.h"
#include "osal.h"
#include "ble_common.h"
#include "ble_service.h"
#include "ble_uart.h"
#include "ble_l2cap.h"
#include "bas.h"
#include "dis.h"
#include "ad_battery.h"

#include "..\APP\app_main.h"
#include "..\APP\app_cmd.h"

#include "..\LIB\easylogger\elog.h"
#include "..\LIB\easylogger\easyflash.h"
#include "..\LIB\MD5.h"

#include "..\HAL\hal_uid.h"

#if dg_configSUOTA_SUPPORT
#include "dlg_suota.h"
#include "..\sw_version.h"
#endif

static char tag[]="app_ble";
#define log_a(...) elog_a(tag, __VA_ARGS__)
#define log_e(...) elog_e(tag, __VA_ARGS__)
#define log_w(...) elog_w(tag, __VA_ARGS__)
#define log_i(...) elog_i(tag, __VA_ARGS__)
#define log_d(...) elog_d(tag, __VA_ARGS__)
#define log_v(...) elog_v(tag, __VA_ARGS__)



#if dg_configSUOTA_SUPPORT
/*
 * Store information about ongoing SUOTA.
 */
INITIALISED_PRIVILEGED_DATA static bool suota_ongoing = false;
#endif


#if dg_configSUOTA_SUPPORT
        ble_service_t *suota;
#endif

/* Timer used for battery monitoring */
PRIVILEGED_DATA static OS_TIMER bas_tim;

BLEU_pib_s BLEU_pib;

uint16_t 	Current_Conn_idx = 100;

BLEU_callbacks_t BLEU_callbacks;

/* Battery Service instance */
PRIVILEGED_DATA static ble_service_t *bas;

PRIVILEGED_DATA static ble_service_t *dis;

PRIVILEGED_DATA static ble_service_t* bleuart;

PRIVILEGED_DATA static void *param_connections;


#define 	BLE_disconnected    		0

#define 	BLE_connected    			1

#define 	NAME_LEN_DEF				11

char name_buf[30]={0};

static const uint8_t adv_data[] = {
#if dg_configSUOTA_SUPPORT
        0x03, GAP_DATA_TYPE_UUID16_LIST_INC,
        0xF5, 0xFE, // = 0xFEF5 (DIALOG SUOTA UUID)
#endif
        0x11, GAP_DATA_TYPE_UUID128_LIST_INC,
        0xcf, 0xb7,
        0x61, 0x96,
		0x75, 0x4b,
		0x97, 0xa3,
		0x3a, 0x4a,
		0x51, 0xbc,
		0xfd, 0x96,
		0x3a, 0xe5

};

uint8_t scan_rsp[31]={0x00,GAP_DATA_TYPE_LOCAL_NAME,} ;

dis_device_info_t dis_info =
{
	"iite KidBrush S1",
	NULL,
	"UXISNLAKSJDLIUX",
	"2.0",
	"0.0.1",
	NULL,
	NULL,
	0,
	NULL,
	NULL
};


#if !dg_configSUOTA_SUPPORT || PX_REPORTER_SUOTA_POWER_SAVING
/* Update connection parameters */
static void conn_param_update(uint16_t conn_idx)
{
        gap_conn_params_t cp;

        cp.interval_min = defaultBLE_PPCP_INTERVAL_MIN;
        cp.interval_max = defaultBLE_PPCP_INTERVAL_MAX;
        cp.slave_latency = defaultBLE_PPCP_SLAVE_LATENCY;
        cp.sup_timeout = defaultBLE_PPCP_SUP_TIMEOUT;

        ble_gap_conn_param_update(conn_idx, &cp);
}
#endif

#if dg_configSUOTA_SUPPORT && PX_REPORTER_SUOTA_POWER_SAVING
/* Update connection parameters for SUOTA */
static void conn_param_update_for_suota(uint16_t conn_idx)
{
        gap_conn_params_t cp;

        cp.interval_min = BLE_CONN_INTERVAL_FROM_MS(20);    // 20ms
        cp.interval_max = BLE_CONN_INTERVAL_FROM_MS(60);    // 60ms
        cp.slave_latency = 0;
        cp.sup_timeout = BLE_SUPERVISION_TMO_FROM_MS(2000); // 2000ms

        ble_gap_conn_param_update(conn_idx, &cp);
}
#endif

/*****************CALL BACK FUNCTION*****************/

void  BLEU_rx_data_cb (ble_service_t *svc, uint16_t conn_idx, const uint8_t *value,uint16_t length)
{
	if(length < 2)
	{
		memset(BLEU_pib.RX_Date_Buff,0,sizeof(BLEU_pib.RX_Date_Buff));
		memcpy(BLEU_pib.RX_Date_Buff,value,length);
		BLEU_pib.rx_length = length;

		log_i("rx data :%d",*value);
		OS_TASK_NOTIFY(BLEU_pib.BLE_Handle, BLE_RX_NOTIFY, OS_NOTIFY_SET_BITS);
	}
	else
	{
		if(value[0] == 1)
		{
			memset(BLEU_pib.RX_Date_Buff,0,sizeof(BLEU_pib.RX_Date_Buff));
			BLEU_pib.rx_length = 0;
		}

		memcpy((BLEU_pib.RX_Date_Buff)+(BLEU_pib.rx_length),(value)+3,(length)-3);
		BLEU_pib.rx_length += (length)-3;

		if(value[0] == value[1])
			OS_TASK_NOTIFY(BLEU_pib.BLE_Handle,BLE_RX_NOTIFY, OS_NOTIFY_SET_BITS);
	}
}
void BLEU_tx_done_cb(ble_service_t *svc, uint16_t conn_idx, uint16_t length)
{
	log_i("BLEU_tx_done_cb  length=%d!!\r\n",length);
}

static void ble_app_bas_timer_cb(OS_TIMER timer)
{
        OS_TASK task = (OS_TASK) OS_TIMER_GET_TIMER_ID(timer);

        OS_TASK_NOTIFY(task, BLE_BAS_NOTIFY, OS_NOTIFY_SET_BITS);
}

#if dg_configSUOTA_SUPPORT

/* Callback from SUOTA implementation */
static bool suota_ready_cb(void)
{
#if PX_REPORTER_SUOTA_POWER_SAVING
        gap_device_t dev;
        size_t dev_num = 1;
#endif

        /*
         * This callback is used so application can accept/block SUOTA.
         * Also, before SUOTA starts, user might want to do some actions
         * e.g. disable sleep mode.
         *
         * If true is returned, then advertising is stopped and SUOTA
         * is started. Otherwise SUOTA is canceled.
         *
         */
        suota_ongoing = true;

#if PX_REPORTER_SUOTA_POWER_SAVING
        /*
         * We need to decrease connection interval for SUOTA so data can be transferred quickly.
         * At the moment SUOTA does not provide information about connection on which it was
         * started. but since it can be completed only when there is one device connected we
         * assume proper device is the first (and only) device connected.
         */
        ble_gap_get_devices(GAP_DEVICE_FILTER_CONNECTED, NULL, &dev_num, &dev);
        if (dev_num > 0) {
                conn_param_update_for_suota(dev.conn_idx);
        }
#endif

        return true;
}

static void suota_status_changed_cb(uint8_t status, uint8_t error_code)
{
#if PX_REPORTER_SUOTA_POWER_SAVING
        gap_device_t dev;
        size_t dev_num = 1;

        /*
         * In case SUOTA finished with an error, we just restore default connection parameters.
         */

        if (status != SUOTA_ERROR) {
                return;
        }

        ble_gap_get_devices(GAP_DEVICE_FILTER_CONNECTED, NULL, &dev_num, &dev);
        if (dev_num > 0) {
                conn_param_update(dev.conn_idx);
        }
#endif
}

static const suota_callbacks_t suota_cb = {
        .suota_ready = suota_ready_cb,
        .suota_status = suota_status_changed_cb,
};

#endif

/*****************CALL BACK FUNCTION END*****************/

/*****************BAS FUNCTION START*****************/
/*
 * The values depend on the battery type.
 * MIN_BATTERY_LEVEL (in mVolts) must correspond to dg_configBATTERY_LOW_LEVEL (in ADC units)
 */

uint8_t ble_app_cal_bat_level(uint16_t voltage)
{
        if (voltage >= MAX_BATTERY_LEVEL) {
                return 100;
        } else if (voltage <= MIN_BATTERY_LEVEL) {
                return 0;
        }

        /*
         * For demonstration purposes discharging (Volt vs Capacity) is approximated
         * by a linear function. The exact formula depends on the specific battery being used.
         */
        return (uint8_t) ((int) (voltage - MIN_BATTERY_LEVEL) * 100 /
                                                        (MAX_BATTERY_LEVEL - MIN_BATTERY_LEVEL));
}

void bas_update(void)
{
        battery_source bat = ad_battery_open();
        uint16_t bat_voltage = ad_battery_raw_to_mvolt(bat, ad_battery_read(bat));
        bas_set_level(bas, ble_app_cal_bat_level(bat_voltage), true);
        ad_battery_close(bat);
}

void ble_app_creat_bas_timer(void)
{
    BLEU_pib.BLE_connected_status=BLE_disconnected;

    BLEU_pib.BLE_Handle = OS_GET_CURRENT_TASK();

    bas_tim = OS_TIMER_CREATE("bas", OS_MS_2_TICKS(60000), true,(void *) OS_GET_CURRENT_TASK(), ble_app_bas_timer_cb);
}

/*****************BAS FUNCTION END*****************/

void ble_app_handle_evt_gap_connected(ble_evt_gap_connected_t *evt)
{
    /* Start battery monitoring if not yet started, but first update current battery level */
    if (!OS_TIMER_IS_ACTIVE(bas_tim)) {
            bas_update();
            OS_TIMER_START(bas_tim, OS_TIMER_FOREVER);
    }

    //BLE status is connected.
    BLEU_pib.BLE_connected_status=BLE_connected;
    ble_gap_adv_stop();

    log_i("ble_app_handle_evt_gap_connected param:%d,%d,%d,%d",evt->conn_params.interval_min,evt->conn_params.interval_max, \
    		evt->conn_params.slave_latency,evt->conn_params.sup_timeout);
}

void ble_app_handle_evt_gap_disconnected(ble_evt_gap_disconnected_t *evt)
{
    size_t num_connected;

    /* Switch back to fast advertising interval. */
    //BLE status is disconnected.
    BLEU_pib.BLE_connected_status=BLE_disconnected;
    ble_gap_adv_intv_set(adv_min, adv_max);
    ble_gap_adv_start(GAP_CONN_MODE_UNDIRECTED);
    /*
     * Stop monitoring battery level if no one is connected.
     */
    ble_gap_get_devices(GAP_DEVICE_FILTER_CONNECTED, NULL, &num_connected, NULL);
    if (num_connected == 0) {
            OS_TIMER_STOP(bas_tim, OS_TIMER_FOREVER);
    }

    //IITE
    main_get_brush_cb()->need_devided_packet = DIVIDED_PACKET;

    log_i("disconnect reason:%d",evt->reason);
}

void handle_evt_gap_adv_completed(ble_evt_gap_adv_completed_t *evt)
{
        /*
         * If advertising is completed, just restart it. It's either because new client connected
         * or it was cancelled in order to change interval values.
         */
#if dg_configSUOTA_SUPPORT
        /* If SUOTA is ongoing don't start advertising. */
        if (suota_ongoing) {
                return;
        }
#endif
        if(BLEU_pib.BLE_connected_status == BLE_disconnected)
        	ble_gap_adv_start(GAP_CONN_MODE_UNDIRECTED);
}

void handle_evt_gap_pair_req(ble_evt_gap_pair_req_t *evt)
{
        ble_gap_pair_reply(evt->conn_idx, true, evt->bond);
}

void ble_app_uart_rx_handle(void)
{
	cmd_parse(BLEU_pib.RX_Date_Buff,BLEU_pib.rx_length,BLEU_pib.TX_Data_Buff,&(BLEU_pib.tx_length),&(BLEU_pib.tx_cmd));
	if(BLEU_pib.tx_length)
	{
		BLEU_tx_data(bleuart,Current_Conn_idx,BLEU_pib.TX_Data_Buff,BLEU_pib.tx_length,BLEU_pib.tx_cmd);
	}
}

void ble_app_uart_tx_handle(void)
{
	if(BLEU_pib.tx_length)
	{
		BLEU_tx_data(bleuart,Current_Conn_idx,BLEU_pib.TX_Data_Buff,BLEU_pib.tx_length,BLEU_pib.tx_cmd);
	}
}

void ble_app_get_shorturl(uint8 *input,uint8 *output)
        {
            uint8 outChars[10] = {0};
            uint8 decrypt[16];
            uint8 j;
            //可以自定义生成MD5加密字符传前的混合KEY
            uint8 encrypt[20] = "iite";
            //要使用生成URL的字符
            uint8 chars[] = {
                'a','b','c','d','e','f','g','h',
                'i','j','k','l','m','n','o','p',
                'q','r','s','t','u','v','w','x',
                'y','z','0','1','2','3','4','5',
                '6','7','8','9','A','B','C','D',
                'E','F','G','H','I','J','K','L',
                'M','N','O','P','Q','R','S','T',
                'U','V','W','X','Y','Z'

             };
            MD5_CTX md5;
            uint32 *seed;

            //对传入网址进行MD5加密
            MD5Init(&md5);
            strcat((char*)encrypt,(char*)input);
            MD5Update(&md5,encrypt,strlen((int8 *)encrypt));
            MD5Final(&md5,decrypt);


            //总共可以得到四组url，这里只取第一个
            //for (i = 0; i < 4; i++)
            //{
            //把加密字符按照8位一组16进制与0x3FFFFFFF进行位与运算
            seed = (uint32*)decrypt;
            int32 hexint = 0x3FFFFFFF & *seed;
            for (j = 0; j < 6; j++)
            {
            //把得到的值与0x0000003D进行位与运算，取得字符数组chars索引
                int32 index = 0x0000003D & hexint;
              //把取得的字符相加
                //outChars += chars[index]; 下面是另一个版本
                outChars[j] = chars[index];
                //每次循环按位右移5位
                hexint = hexint >> 5;
            }
            strcpy((char*)output,(char*)outChars);
            //把字符串存入对应索引的输出数组
            //resUrl[i] = outChars;
            //}

            //return resUrl;
 }

void ble_app_param_init()
{
	uint8 uid[8]={0};
	char uidstr[30] = {0};
	uint8 uniquename[10]={0};
	const char key[]="uuid";
	char *tmp= NULL;
	uint8 res;

	//set ble name
	tmp = ef_get_env(key);
	if(tmp == NULL || !strcmp(tmp,"none"))
	{
		hal_uid_cal_uid((uint32 *)uid);
		sprintf(uidstr,"%02X%02X%02X%02X%02X%02X%02X%02X",uid[7],uid[6],uid[5],uid[4],uid[3],uid[2],uid[1],uid[0]);
		res = ef_set_env(key,uidstr);
		res = ef_save_env();
		log_i("uid generated res:%d",res);
	}
	else
	{
	        strcpy(uidstr,tmp);
	}

	log_i("uidstr:%s",uidstr);

	ble_app_get_shorturl((uint8*)uidstr,uniquename);

	sprintf(name_buf,"iite-%s",uniquename);

	log_i("ble_name:%s",name_buf);

	//set ble service
	ble_gap_mtu_size_set(256);

	bas = bas_init(NULL, NULL);
	ble_service_add(bas);

	dis = dis_init(NULL, &dis_info);
	ble_service_add(dis);

	BLEU_callbacks.BLEU_RX = BLEU_rx_data_cb;
	BLEU_callbacks.BLEU_TX_DONE = BLEU_tx_done_cb;
	bleuart = BLE_Uart_Service_init(&BLEU_callbacks);
	ble_service_add(bleuart);

#if dg_configSUOTA_SUPPORT
	/*
	 * Register SUOTA
	 *
	 * SUOTA instance should be registered in ble_service framework in order for events inside
	 * service to be processed properly.
	 */
	suota = suota_init(&suota_cb);
	OS_ASSERT(suota != NULL);
	ble_service_add(suota);


#endif

	//ble_packet set
	ble_gap_device_name_set(name_buf,ATT_PERM_READ);

	scan_rsp[0] = NAME_LEN_DEF + 1; /* 1 byte for data type */

	memcpy(&scan_rsp[2], name_buf, NAME_LEN_DEF);

    ble_gap_adv_intv_set(adv_min, adv_max);

	ble_gap_adv_data_set(sizeof(adv_data), adv_data, NAME_LEN_DEF + 3, scan_rsp);

}


void ble_app_task(void *params)
{
	int8_t wdog_id;

	wdog_id = sys_watchdog_register(false);

	sys_watchdog_suspend(wdog_id);

	ble_peripheral_start();

	ble_register_app();

	ble_app_param_init();

	ble_app_creat_bas_timer();

	ble_app_start_adv();

	log_i("ble init ok");

	sys_watchdog_notify_and_resume(wdog_id);

	for (;;)
	{
		OS_BASE_TYPE ret;
		uint32_t notif;

		/* notify watchdog on each loop */
		sys_watchdog_notify(wdog_id);

		/* suspend watchdog while blocking on OS_TASK_NOTIFY_WAIT() */
		sys_watchdog_suspend(wdog_id);

		/*
		 * Wait on any of the notification bits, then clear them all
		 */
		ret = OS_TASK_NOTIFY_WAIT(0, OS_TASK_NOTIFY_ALL_BITS, &notif,
				OS_TASK_NOTIFY_FOREVER);
		/* Blocks forever waiting for the task notification. Therefore, the return value must
		 * always be OS_OK
		 */
		OS_ASSERT(ret == OS_OK);

		/* resume watchdog */
		sys_watchdog_notify_and_resume(wdog_id);

		if (notif & BLE_APP_NOTIFY_MASK)
		{
			ble_evt_hdr_t *hdr;
			ble_evt_gap_conn_param_update_req_t *cpur;

			hdr = ble_get_event(false);
			if (!hdr)
			{
				goto no_event;
			}

			/*
			 * First, application needs to try pass event through ble_framework.
			 * Then it can handle it itself and finally pass to default event handler.
			 */
			if (!ble_service_handle_event(hdr))
			{
				switch (hdr->evt_code)
				{
				case BLE_EVT_GAP_CONNECTED:
					ble_app_handle_evt_gap_connected(
							(ble_evt_gap_connected_t *) hdr);
					Current_Conn_idx =
							((ble_evt_gap_connected_t *) hdr)->conn_idx;

					OS_TASK_NOTIFY(main_get_brush_cb()->main_task, MAIN_DEV_CONNECTED, eSetBits);

					break;
				case BLE_EVT_GAP_DISCONNECTED:
					ble_app_handle_evt_gap_disconnected(
							(ble_evt_gap_disconnected_t *) hdr);
					Current_Conn_idx = 100;

					OS_TASK_NOTIFY(main_get_brush_cb()->main_task, MAIN_DEV_DISCONNECTED, eSetBits);

					break;
				case BLE_EVT_GAP_ADV_COMPLETED:
					handle_evt_gap_adv_completed(
							(ble_evt_gap_adv_completed_t *) hdr);
					break;
				case BLE_EVT_GAP_PAIR_REQ:
					handle_evt_gap_pair_req((ble_evt_gap_pair_req_t *) hdr);
					break;
				case BLE_EVT_GAP_CONN_PARAM_UPDATE_REQ:
					cpur = (ble_evt_gap_conn_param_update_req_t *)hdr;
					log_i("BLE_EVT_GAP_CONN_PARAM_UPDATE_REQ :%d,%d,%d,%d",
							cpur->conn_params.interval_min,
							cpur->conn_params.interval_max,
							cpur->conn_params.slave_latency,
							cpur->conn_params.sup_timeout);
					ble_handle_event_default(hdr);
					break;
				case BLE_EVT_GAP_CONN_PARAM_UPDATED:
					log_i("BLE_EVT_GAP_CONN_PARAM_UPDATED :%d",((ble_evt_gap_conn_param_update_completed_t *)hdr)->status);
					break;
#if dg_configSUOTA_SUPPORT && defined (SUOTA_PSM)
					case BLE_EVT_L2CAP_CONNECTED:
					case BLE_EVT_L2CAP_DISCONNECTED:
					case BLE_EVT_L2CAP_DATA_IND:
					suota_l2cap_event(suota, hdr);
					break;
#endif
				default:
					log_i("default evt_code :%d",hdr->evt_code);
					ble_handle_event_default(hdr);
					break;
				}
			}

			/* Free event buffer (it's not needed anymore) */
			OS_FREE(hdr);

			no_event:
			/*
			 * If there are more events waiting in queue, application should process
			 * them now.
			 */
			if (ble_has_event())
			{
				OS_TASK_NOTIFY(OS_GET_CURRENT_TASK(), BLE_APP_NOTIFY_MASK,
						OS_NOTIFY_SET_BITS);
			}
		}
		if (notif & BLE_RX_NOTIFY)
		{
			ble_app_uart_rx_handle();
		}
		if (notif & BLE_TX_NOTIFY)
		{
			ble_app_uart_tx_handle();
		}
		if (notif & BLE_BAS_NOTIFY)
		{
			bas_update();
		}
	}
}
