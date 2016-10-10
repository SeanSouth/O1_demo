/*
 * TRNG_Driver.c
 *
 *  Created on: 2016Äê8ÔÂ17ÈÕ
 *      Author: Administrator
 */


#include "hw_trng.h"
#include "hal_uid.h"
#include "hal_logstore.h"

int hal_uid_cal_uid(uint32 *buff)
{
	uint32 uniqueid[2];

	hw_trng_enable(0);

	hw_trng_get_numbers(uniqueid,2);

	hw_trng_disable();

	memcpy((uint8*)buff,(uint8*)uniqueid,8);

	return UID_LEN;
}
