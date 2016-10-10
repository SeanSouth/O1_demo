/*
 * app_BLE.h
 *
 *  Created on: 2016Äê8ÔÂ2ÈÕ
 *      Author: Administrator
 */

#ifndef APP_APP_BLE_H_
#define APP_APP_BLE_H_

#include "sdk_defs.h"
#include "ble_gap.h"
#include "osal.h"

#define adv_min     1600
#define adv_max		2400

#define MAX_BATTERY_LEVEL 3300
#define MIN_BATTERY_LEVEL 2500

#define BLE_RX_NOTIFY    	(1 << 1)
#define BLE_TX_NOTIFY		(1 << 2)
#define BLE_BAS_NOTIFY     	(1 << 3)

typedef struct _BLEU_pib {
        uint8		RX_Date_Buff[256];
        uint16  	rx_length;

        uint8		TX_Data_Buff[256];
        uint16		tx_length;
        uint8		tx_cmd;

        OS_TASK     BLE_Handle;

        bool		BLE_connected_status ;
} BLEU_pib_s;

typedef struct {
        void           *next;

        bool            expired;

        uint16_t        conn_idx; ///< Connection index

        OS_TIMER        param_timer;
        OS_TASK         current_task;
} conn_dev_t;


void ble_app_disconnect();
void ble_app_start_adv();
void ble_app_stop_adv();
void ble_app_send_cmd(char*buf,uint8 len,uint8 cmd);
void ble_app_init(void);

#endif /* APP_APP_BLE_H_ */
