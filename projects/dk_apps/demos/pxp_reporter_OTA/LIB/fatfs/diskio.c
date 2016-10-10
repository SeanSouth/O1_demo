/*-----------------------------------------------------------------------*/
/* Low level disk I/O module skeleton for FatFs     (C)ChaN, 2016        */
/*-----------------------------------------------------------------------*/
/* If a working storage control module is available, it should be        */
/* attached to the FatFs via a glue function rather than modifying it.   */
/* This is an example of glue functions to attach various exsisting      */
/* storage control modules to the FatFs module with a defined API.       */
/*-----------------------------------------------------------------------*/

#include "diskio.h"		/* FatFs lower layer API */
#include "..\HAL\hal_rtc.h"
#include "..\HAL\hal_datastore.h"

/* Definitions of physical drive number for each drive */
#define DEV_RAM		0	/* Example: Map Ramdisk to physical drive 0 */
#define DEV_MMC		1	/* Example: Map MMC/SD card to physical drive 1 */
#define DEV_USB		2	/* Example: Map USB MSD to physical drive 2 */

#define SECTOR_SIZE (512)

DWORD get_fattime (void)
{
	return hal_rtc_get_unixtime();
}


/*-----------------------------------------------------------------------*/
/* Get Drive Status                                                      */
/*-----------------------------------------------------------------------*/

DSTATUS disk_status (
	BYTE pdrv		/* Physical drive nmuber to identify the drive */
)
{
	return 0;
}



/*-----------------------------------------------------------------------*/
/* Inidialize a Drive                                                    */
/*-----------------------------------------------------------------------*/

DSTATUS disk_initialize (
	BYTE pdrv				/* Physical drive nmuber to identify the drive */
)
{
	flash_storage_init();

	return RES_OK;
}



/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */
/*-----------------------------------------------------------------------*/

DRESULT disk_read (
	BYTE pdrv,		/* Physical drive nmuber to identify the drive */
	BYTE *buff,		/* Data buffer to store read data */
	DWORD sector,	/* Start sector in LBA */
	UINT count		/* Number of sectors to read */
)
{
	DRESULT res;
	int result;
	uint32_t addr = 0;
	uint32_t len = 0;

	addr = sector * SECTOR_SIZE;
	len = count * SECTOR_SIZE;

	result = flash_storage_read(addr,buff,len);


	if(result >= 0)
		return RES_OK;
	else
	{
		printf("disk_read res:%d\r\n",result);
		return RES_PARERR;
	}


}



/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */
/*-----------------------------------------------------------------------*/

DRESULT disk_write (
	BYTE pdrv,			/* Physical drive nmuber to identify the drive */
	const BYTE *buff,	/* Data to be written */
	DWORD sector,		/* Start sector in LBA */
	UINT count			/* Number of sectors to write */
)
{
	DRESULT res;
	int result;
	uint32_t addr = 0;
	uint32_t len = 0;

	addr = sector * SECTOR_SIZE;
	len = count * SECTOR_SIZE;

	result = flash_storage_write(addr,buff,len);
	printf("disk_write res:%d\r\n",result);
	if(result >= 0)
		return RES_OK;
	else
	{
		printf("disk_write res:%d\r\n",result);
		return RES_PARERR;
	}
}



/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions                                               */
/*-----------------------------------------------------------------------*/

DRESULT disk_ioctl (
	BYTE pdrv,		/* Physical drive nmuber (0..) */
	BYTE cmd,		/* Control code */
	void *buff		/* Buffer to send/receive control data */
)
{
	DRESULT res;
	int result;
	DWORD *buf = (DWORD*)buff;

	switch(cmd)
	{
	case CTRL_SYNC:
		printf("CTRL_SYNC\r\n");
		break;
	case GET_SECTOR_COUNT:
		*buf = 248;
		printf("GET_SECTOR_COUNT 248\r\n");
		break;
	case GET_SECTOR_SIZE:
		*buf = 512;
		printf("GET_SECTOR_SIZE 512\r\n");
		break;
	case GET_BLOCK_SIZE:
		*buf = 8;
		printf("GET_BLOCK_SIZE 1\r\n");
		break;
	default:
		printf("disk_ioctl cmd:%d\r\n",cmd);
	}

	return RES_OK;
}

