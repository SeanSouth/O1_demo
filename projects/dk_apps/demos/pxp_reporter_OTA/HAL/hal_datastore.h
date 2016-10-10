/*
 * Flash_Storage.h
 *
 *  Created on: 2016Äê8ÔÂ1ÈÕ
 *      Author: Administrator
 */

#ifndef FRANWORK_FLASH_STORAGE_H_
#define FRANWORK_FLASH_STORAGE_H_


#include "stdint.h"

void flash_storage_init(void);

int flash_storage_read(uint32_t addr, uint8_t *buf, uint32_t len);

int flash_storage_write(uint32_t addr, const uint8_t *buf, uint32_t size);

int flash_storage_get_size();

#endif /* FRANWORK_FLASH_STORAGE_H_ */
