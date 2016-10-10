/*
 * BLE_Uart_Service.h
 *
 *  Created on: 2016Äê7ÔÂ26ÈÕ
 *      Author: Administrator
 */

#ifndef APP_BLE_UART_SERVICE_H_
#define APP_BLE_UART_SERVICE_H_

#include "ble_service.h"


typedef void (* BLEU_rx_data_cb_t) (ble_service_t *svc, uint16_t conn_idx, const uint8_t *value,
                                                                                uint16_t length);
typedef void (* BLEU_tx_done_cb_t) (ble_service_t *svc, uint16_t conn_idx, uint16_t length);

/**
 * BLE_Uart application callbacks
 */
typedef struct {
        /** Data received from remote client */
	BLEU_rx_data_cb_t          BLEU_RX;
        /** Service finished TX transaction */
	BLEU_tx_done_cb_t          BLEU_TX_DONE;
} BLEU_callbacks_t;


ble_service_t *BLE_Uart_Service_init(BLEU_callbacks_t *cb);

void BLEU_tx_data(ble_service_t *svc, uint16_t conn_idx, uint8_t *data, uint16_t length,uint8_t cmd);

#endif /* APP_BLE_UART_SERVICE_H_ */
