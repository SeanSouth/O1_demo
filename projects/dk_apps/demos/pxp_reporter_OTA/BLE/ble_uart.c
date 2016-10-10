/*
 * BLE_Uart_Service.c
 *
 *  Created on: 2016Äê7ÔÂ26ÈÕ
 *      Author: Administrator
 */

#include "stdio.h"
#include <string.h>
#include "osal.h"
#include "ble_uart.h"
#include "ble_uuid.h"
#include "ble_bufops.h"
#include "ble_storage.h"
#include "..\APP\app_main.h"

#include "..\LIB\easylogger\elog.h"

static char tag[]="app_ble";
#define log_a(...) elog_a(tag, __VA_ARGS__)
#define log_e(...) elog_e(tag, __VA_ARGS__)
#define log_w(...) elog_w(tag, __VA_ARGS__)
#define log_i(...) elog_i(tag, __VA_ARGS__)
#define log_d(...) elog_d(tag, __VA_ARGS__)
#define log_v(...) elog_v(tag, __VA_ARGS__)



#define UUID_BLEU_Service                "E53A96FD-BC51-4A3A-A397-4B759661B7CF"
#define UUID_BLEU_Service_TX			 0XCDD1
#define UUID_BLEU_Service_RX			 0XCDD2



#define BLEU_tx_size     256
#define BLEU_rx_size     256

#define PACKET_PAYLOAD 	17
#define PACKET_LENGTH	20


typedef struct {
        ble_service_t svc;

        BLEU_callbacks_t *cb;

        uint16_t BLEU_tx_val_h;
        uint16_t BLEU_tx_ccc_h;

        uint16_t BLEU_rx_val_h;

} BLEU_service_t;

QueueHandle_t tx_semaphore;

static uint16_t get_tx_length(BLEU_service_t *BLEU, uint16_t conn_idx)
{
        uint16_t length = 0x00;

        ble_storage_get_u16(conn_idx, BLEU->BLEU_tx_val_h, &length);

        return length;
}

static void set_tx_length(BLEU_service_t *BLEU, uint16_t conn_idx, uint16_t length)
{
        ble_storage_put_u32(conn_idx, BLEU->BLEU_tx_val_h, length, false);
}

static ble_error_t send_tx_data(BLEU_service_t *BLEU, uint16_t conn_idx,
		uint16_t length, uint8_t *data)
{
	uint16_t ccc = 0x0000;
	ble_error_t status;

//	if (get_tx_length(BLEU, conn_idx) != 0x00)
//	{
//		log_i("tx is sending");
//		return BLE_ERROR_FAILED;
//	}
	if (!BLEU->cb->BLEU_TX_DONE)
	{
		return BLE_ERROR_FAILED;
	}
	ble_storage_get_u16(conn_idx, BLEU->BLEU_tx_ccc_h, &ccc);
	if (!(ccc & GATT_CCC_NOTIFICATIONS))
	{
		return BLE_ERROR_FAILED;
	}

	status = ble_gatts_send_event(conn_idx, BLEU->BLEU_tx_val_h,
			GATT_EVENT_NOTIFICATION,length, data);

//	if(!status)
//		set_tx_length(BLEU,conn_idx,length);

	return status;

}

static att_error_t handle_tx_ccc_write(BLEU_service_t *BLEU,  uint16_t conn_idx,
                                        uint16_t offset, uint16_t length, const uint8_t *value)
{
        uint16_t ccc;

        if (offset) {
                return ATT_ERROR_ATTRIBUTE_NOT_LONG;
        }

        if (length != sizeof(ccc)) {
                return ATT_ERROR_APPLICATION_ERROR;
        }

        ccc = get_u16(value);

        ble_storage_put_u32(conn_idx, BLEU->BLEU_tx_ccc_h, ccc, true);

        return ATT_ERROR_OK;
}

static att_error_t handle_rx_data(BLEU_service_t *BLEU, uint16_t conn_idx,
                                        uint16_t offset, uint16_t length, const uint8_t *value)
{
        if (BLEU->cb && BLEU->cb->BLEU_RX) {
        	BLEU->cb->BLEU_RX(&BLEU->svc, conn_idx, value, length);
        }

        return ATT_ERROR_OK;
}

static void handle_write_req(ble_service_t *svc, const ble_evt_gatts_write_req_t *evt)
{
    BLEU_service_t *BLEU = (BLEU_service_t *) svc;
    att_error_t status = ATT_ERROR_ATTRIBUTE_NOT_FOUND;
    uint16_t handle = evt->handle;

    if (handle == BLEU->BLEU_tx_ccc_h) {
            status = handle_tx_ccc_write(BLEU, evt->conn_idx, evt->offset, evt->length, evt->value);
    }

    if (handle == BLEU->BLEU_rx_val_h) {
            status = handle_rx_data(BLEU, evt->conn_idx, evt->offset, evt->length, evt->value);
    }

    ble_gatts_write_cfm(evt->conn_idx, evt->handle, status);
}

static void handle_read_req(ble_service_t *svc, const ble_evt_gatts_read_req_t *evt)
{
	BLEU_service_t *BLEU = (BLEU_service_t *) svc;
	if(evt->handle == BLEU->BLEU_tx_ccc_h)
	{
		uint16_t ccc = 0x0000;
		ble_storage_get_u16(evt->conn_idx, BLEU->BLEU_tx_ccc_h, &ccc);
		ble_gatts_read_cfm(evt->conn_idx, evt->handle, ATT_ERROR_OK, sizeof(ccc), &ccc);
	}
}

static void handle_event_sent(ble_service_t *svc, const ble_evt_gatts_event_sent_t *evt)
{
	BLEU_service_t *BLEU = (BLEU_service_t *) svc;
	uint16_t length, conn_idx = evt->conn_idx;
	if (evt->handle == BLEU->BLEU_tx_val_h)
	{
		length = get_tx_length(BLEU, conn_idx);
//		set_tx_length(BLEU, conn_idx, 0x00);
		BLEU->cb->BLEU_TX_DONE(&BLEU->svc, conn_idx, length);
	}
}


ble_service_t *BLE_Uart_Service_init(BLEU_callbacks_t *cb)
{
	uint16_t num_attr;
	BLEU_service_t *BLEU;
	att_uuid_t uuid;

	BLEU = OS_MALLOC(sizeof(*BLEU));
	memset(BLEU, 0, sizeof(*BLEU));

	num_attr = ble_gatts_get_num_attr(0, 2, 1); //  the number of included services,characteristics and descriptors

	ble_uuid_from_string(UUID_BLEU_Service, &uuid);
	ble_gatts_add_service(&uuid, GATT_SERVICE_PRIMARY, num_attr);

	/* BLEU Server TX */
	ble_uuid_create16(UUID_BLEU_Service_TX, &uuid);
	ble_gatts_add_characteristic(&uuid, GATT_PROP_NOTIFY, ATT_PERM_NONE,
			BLEU_tx_size, 0, NULL, &BLEU->BLEU_tx_val_h);

	ble_uuid_create16(UUID_GATT_CLIENT_CHAR_CONFIGURATION, &uuid);
	ble_gatts_add_descriptor(&uuid, ATT_PERM_RW, 2, 0, &BLEU->BLEU_tx_ccc_h);

	/* BLEU Server RX */
	ble_uuid_create16(UUID_BLEU_Service_RX, &uuid);
	ble_gatts_add_characteristic(&uuid, GATT_PROP_WRITE, ATT_PERM_WRITE,
			BLEU_rx_size, 0, NULL, &BLEU->BLEU_rx_val_h);

	ble_gatts_register_service(&BLEU->svc.start_h, &BLEU->BLEU_tx_val_h,
			&BLEU->BLEU_tx_ccc_h, &BLEU->BLEU_rx_val_h, 0);

	BLEU->svc.end_h = BLEU->svc.start_h + num_attr;
	BLEU->svc.write_req = handle_write_req;
	BLEU->svc.read_req = handle_read_req;
	BLEU->svc.event_sent = handle_event_sent;
	BLEU->cb = cb;

	return &BLEU->svc;
}

void BLEU_tx_data(ble_service_t *svc, uint16_t conn_idx, uint8_t *data, uint16_t length,uint8_t cmd)
{
	BLEU_service_t *BLEU = (BLEU_service_t *) svc;
	ble_error_t status;
	uint8 divided_data[20];
	uint8 len;
	uint8 i, j;
	uint8 num;

	if (main_get_brush_cb()->need_devided_packet == COMPLETE_PACKET)
	{
		status = send_tx_data(BLEU,conn_idx,length,divided_data);

		if (status != BLE_STATUS_OK)
		{
			log_e("send_tx_data error status = %d \r\n", status);
		}
	}
	else if (main_get_brush_cb()->need_devided_packet == DIVIDED_PACKET)
	{
		num = length / PACKET_PAYLOAD;
		num += (length % PACKET_PAYLOAD > 0) ? 1 : 0;
		for (i = 0; i < num; i++)
		{
			memset(divided_data, 0, sizeof(divided_data));
			divided_data[0] = i + 1;
			divided_data[1] = num;
			divided_data[2] = cmd;
			for (j = 0; j < ((i < num - 1) ?PACKET_PAYLOAD :(length - i * PACKET_PAYLOAD));j++)
			{
				divided_data[j + 3] = data[PACKET_PAYLOAD * i + j];
			}

			status = send_tx_data(BLEU,conn_idx,PACKET_LENGTH,divided_data);

			if (status != BLE_STATUS_OK)
			{
				log_e("send_tx_data error status = %d \r\n", status);
				break;
			}
			else
			{
				log_i("send_tx_data count:%d/%d",divided_data[0],divided_data[1]);
			}

		}
	}

	return ;
}

