/*
 * Log_Starage.h
 *
 *  Created on: 2016Äê8ÔÂ24ÈÕ
 *      Author: Administrator
 */

#ifndef FRANWORK_LOG_STORAGE_H_
#define FRANWORK_LOG_STORAGE_H_

#include "ad_nvms.h"
#include "stdint.h"
#include "stdbool.h"

void log_storage_init(void);

int log_flash_storage_read(uint32_t addr, uint8_t *buf, uint32_t len);

int log_flash_storage_write(uint32_t addr, const uint8_t *buf, uint32_t size);

bool log_flash_storage_erase(uint32_t addr,uint32_t size);

uint8 log_read_byte(uint32_t addr);

#endif /* FRANWORK_LOG_STORAGE_H_ */
