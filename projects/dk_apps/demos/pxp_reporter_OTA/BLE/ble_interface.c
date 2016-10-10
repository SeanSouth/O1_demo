/*
 * app_BLE.c
 *
 *  Created on: 2016Äê8ÔÂ2ÈÕ
 *      Author: Administrator
 */
#include "stdio.h"
#include "string.h"

#include "osal.h"

#include "..\APP\app_main.h"
#include "ble_interface.h"
#include "..\HAL\hal_datastore.h"

#include "..\HAL\hal_uid.h"
#include "app_Cmd.h"

extern void ble_app_task(void *params);
extern BLEU_pib_s BLEU_pib;

#if dg_configSUOTA_1_1_SUPPORT

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

/*
 * Device Information Service data
 *
 * Manufacturer Name String is mandatory for devices supporting HRP.
 */
static const dis_device_info_t dis_info = {
        .manufacturer = "Dialog Semiconductor",
        .model_number = "Black Orca",
        .serial_number = "123456",
        .hw_revision = "REVD",
        .fw_revision = "1.0",
        .sw_revision = BLACKORCA_SW_VERSION,
};
#endif


void ble_app_start_adv()
{
	ble_gap_adv_start(GAP_CONN_MODE_UNDIRECTED);
}

void ble_app_stop_adv()
{
	ble_gap_adv_stop();
}

void ble_app_send_cmd(char*buf,uint8 len,uint8 cmd)
{
	strcpy(BLEU_pib.TX_Data_Buff,buf);
	BLEU_pib.tx_length = len;
	BLEU_pib.tx_cmd = cmd;

	OS_TASK_NOTIFY(BLEU_pib.BLE_Handle, BLE_TX_NOTIFY, OS_NOTIFY_SET_BITS);

}


void ble_app_init(void)
{
	extern void ble_app_task(void *params);

    /* Start the PXP reporter application task. */
    OS_TASK_CREATE("ble_app_task",                  /* The text name assigned to the task, for
                                                       debug only; not used by the kernel. */
    				ble_app_task,               /* The function that implements the task. */
					NULL,                            /* The parameter passed to the task. */
					3000,                             /* The number of bytes to allocate to the
                                                       stack of the task. */
					OS_TASK_PRIORITY_NORMAL,  /* The priority assigned to the task. */
					BLEU_pib.BLE_Handle);                         /* The task handle. */
    OS_ASSERT(BLEU_pib.BLE_Handle);
}


