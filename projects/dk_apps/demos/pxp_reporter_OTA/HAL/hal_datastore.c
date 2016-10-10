/*
 * Flash_Storage.c
 *
 *  Created on: 2016Äê8ÔÂ1ÈÕ
 *      Author: Administrator
 */


#include "hal_datastore.h"
#include "ad_nvms.h"

nvms_t Handle_User;

void flash_storage_init(void)
{
	Handle_User = ad_nvms_open(NVMS_GENERIC_PART);
}

int flash_storage_read(uint32_t addr, uint8_t *buf, uint32_t len)
{
	int read_length = 0;

	read_length = ad_nvms_read(Handle_User,addr,buf,len);

	return read_length;
}

int flash_storage_write(uint32_t addr, const uint8_t *buf, uint32_t size)
{
	int write_length = 0;

	write_length = ad_nvms_write(Handle_User,addr,buf,size);

	return write_length;
}

int flash_storage_get_size()
{
	size_t size = 0;

	size = ad_nvms_get_size(Handle_User);

	return size;
}
