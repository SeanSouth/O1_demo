/*
 * Log_Storage.c
 *
 *  Created on: 2016Äê8ÔÂ24ÈÕ
 *      Author: Administrator
 */

#include "hal_logstore.h"


nvms_t Handle_Log;

void log_storage_init(void)
{
	Handle_Log = ad_nvms_open(NVMS_LOG_PART);
}
int log_flash_storage_read(uint32_t addr, uint8_t *buf, uint32_t len)
{
	int read_length = 0;

	read_length = ad_nvms_read(Handle_Log,addr,buf,len);

	return read_length;
}
int log_flash_storage_write(uint32_t addr, const uint8_t *buf, uint32_t size)
{
	int write_length = 0;

	write_length = ad_nvms_write(Handle_Log,addr,buf,size);

	return write_length;
}
bool log_flash_storage_erase(uint32_t addr,uint32_t size)
{
	bool result;

	result = ad_nvms_erase_region(Handle_Log,addr,size);

	return result;
}

uint8 log_read_byte(uint32_t addr)
{
	uint8 date;

	int read_length = 0;

	read_length = ad_nvms_read(Handle_Log,addr,&date,1);

	if(read_length==1)

		return date;
	else
		return 0;
}


