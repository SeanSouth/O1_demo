/*
 * app_data.c
 *
 *  Created on: 2016Äê9ÔÂ19ÈÕ
 *      Author: Administrator
 */

#include "stdio.h"
#include "stdlib.h"
#include "string.h"

#include "app_Data.h"
#include "app_attitude.h"

#include "..\LIB\fatfs\ff.h"
#include "..\LIB\cJSON.h"
#include "..\LIB\easylogger\easyflash.h"
#include "..\LIB\easylogger\elog.h"

#include "..\HAL\hal_rtc.h"
#include "..\HAL\hal_datastore.h"

static char tag[]="app_data";
#define log_a(...) elog_a(tag, __VA_ARGS__)
#define log_e(...) elog_e(tag, __VA_ARGS__)
#define log_w(...) elog_w(tag, __VA_ARGS__)
#define log_i(...) elog_i(tag, __VA_ARGS__)
#define log_d(...) elog_d(tag, __VA_ARGS__)
#define log_v(...) elog_v(tag, __VA_ARGS__)

FATFS FatFs;
uint32 fptr[MAX_DATA_NUMBER + 1]={0};

//void data_test(void)
//{
//    uint8 rowPattern[CY_FLASH_SIZEOF_ROW];
//    uint16 i,j,k;
//    uint8 result = 0u;
//    uint8 errorcode = 0;
//    cystatus returnValue = CYRET_SUCCESS;
//    volatile uint8 source;
//    volatile uint8 destination;
//    single_brushing_data data;
//
//    for(j=0;j<100;j++)
//    {
//        k = rand()%90;
//        fsm_get_brush_cb()->watch_dog_feed = 10000;
//        /* Generate flash row pattern */
//        for(i = 0u; i < k; i++)
//        {
//            memset(&data,0,sizeof(data));
//            data.model = i;
//            /* Erases a row of Flash and programs it with the rowPattern */
//            data_write_new_single_data(&data);
//        }
//
//        /* Compare a row from flash with the rowPattern */
//        for (i = 0u; i < k; i++)
//        {
//            returnValue = data_get_specified_single_data(i+1,&data);
//            if(returnValue == CYRET_SUCCESS)
//            {
//                if(data.model == i)
//                {
//
//                }
//                else
//                {
//                    result++;
//                    errorcode = i+1;
//                }
//            }
//        }
//
//        /* Check if source and destination have not equal values */
//        if (result != 0u)
//        {
//            DBG_PRINTF("CY_SYS_FLASH_ERROR : %d\r\b",errorcode);
//        }
//        else
//        {
//            DBG_PRINTF("CY_SYS_FLASH_SUCCESS\r\b");
//        }
//
//        data_clear_all_data();
//    }
//    data_clear_all_data();
//}

void data_update_file_structure(void)
{
	FILINFO finfo;
	FRESULT res;
	FIL fil;
	uint32 ftime;
	int i;
	char tmp[256];

	res = f_stat(DATA_FILE_NAME,&finfo);
	ftime = (finfo.fdate << 16) |  finfo.ftime;
	log_i("file size:%d,status:%d",finfo.fsize,ftime);


	if(finfo.ftime != fptr[0])
	{
		memset(fptr,0,sizeof(fptr));
		fptr[0] = (finfo.fdate << 16) |  finfo.ftime;
		i = 1;

		res = f_open(&fil,DATA_FILE_NAME,FA_READ| FA_OPEN_EXISTING);
		while(f_gets(tmp,255,&fil))
		{
			i++;
			fptr[i] = fil.fptr;
		}
		res = f_close(&fil);
		log_i("file structure update :%d",i);
	}
}

void data_clear_all_data(void)
{
    uint8 i;

    data_reset_data();
}

uint8 data_get_last_single_data(char *out)
{
    uint8 number = data_get_total_rcrd_num();

    return data_get_specified_single_data(number,out);
}

uint8 data_get_specified_single_data(uint8 rcrd_num, char *out)//@@
{
	FRESULT res;
	FIL fil;

	if(rcrd_num < 1 || rcrd_num > data_get_total_rcrd_num())
		return 1;

	data_update_file_structure();

	res = f_open(&fil,DATA_FILE_NAME,FA_READ| FA_OPEN_EXISTING);
	res = f_lseek(&fil,fptr[rcrd_num]);
	if(f_gets(out,255,&fil))
	{
		log_i("get record success");
	}
	else
	{
		log_e("can't get record:%d",rcrd_num);
	}

	res = f_close(&fil);

	return res;
}

uint8 data_write_new_single_data(single_brushing_data *bdata)//@@
{
	FRESULT res;
	FIL fil;
	cJSON *data;
	cJSON *pos;
	single_brushing_data tmp;
	uint8 i, num;
	char *strout;
	char stt[10]={0};

	if(data_get_total_rcrd_num() >= MAX_DATA_NUMBER)
	{
		log_e("full record!");
		return 0;
	}

	data = cJSON_CreateObject();

	tmp.model = bdata->model;
	tmp.span = bdata->span;
	for(i=0;i<DATA_POS_LEN;i++)
	{
		tmp.pos[i] = bdata->pos[i];
	}
	tmp.time = bdata->time;

	cJSON_AddNumberToObject(data, "model", tmp.model);
	cJSON_AddNumberToObject(data, "time", tmp.time);
	cJSON_AddNumberToObject(data, "span", tmp.span);
	pos = cJSON_CreateArray();
	for (i = 0; i < DATA_POS_LEN; i++)
	{
		cJSON_AddItemToArray(pos, cJSON_CreateNumber(tmp.pos[i]));
	}
	cJSON_AddItemToObject(data, "pos", pos);

	strout = cJSON_PrintUnformatted(data);

	res = f_open(&fil,DATA_FILE_NAME,FA_WRITE | FA_OPEN_APPEND);
	f_puts(strout, &fil);
	f_putc('\n', &fil);
	i = data_get_total_rcrd_num();
	sprintf(stt, "%d", ++i);
	ef_set_env("record_number", stt);
	ef_save_env();
	res = f_close(&fil);
	log_i("file append :%d",i);

	free(strout);
	cJSON_Delete(data);

	return res;
}

uint8 data_get_total_rcrd_num(void)
{
	int i;
	char stt[10]={0};

	i = atoi(ef_get_env("record_number"));
	sprintf(stt, "%d", i);
	log_i("record number :%d",i);

	return i;
}

void data_reset_data(void)
{
	FRESULT res;
	FIL fil;
	char stt[10]={0};

	res = f_open(&fil,DATA_FILE_NAME,FA_WRITE | FA_CREATE_ALWAYS);
	res = f_close(&fil);
	sprintf(stt, "%d", 0);
	ef_set_env("record_number", stt);
	ef_save_env();
	log_i("file clean");
}

void data_generate_record(int count,char *buf)
{
	cJSON *data;
	cJSON *pos;
	single_brushing_data tmp;
	uint8 res, i, num;
	char *strout;

	data = cJSON_CreateObject();

	log_i("cmd_get_specified_record\r\n");

	tmp.model = count;
	tmp.span = count;
	for(i=0;i<DATA_POS_LEN;i++)
	{
		tmp.pos[i] = count;
	}
	tmp.time = hal_rtc_get_unixtime();

	cJSON_AddNumberToObject(data, "model", tmp.model);
	cJSON_AddNumberToObject(data, "time", tmp.time);
	pos = cJSON_CreateArray();
	for (i = 0; i < DATA_POS_LEN; i++)
	{
		cJSON_AddItemToArray(pos, cJSON_CreateNumber(tmp.pos[i]));
	}
	cJSON_AddItemToObject(data, "pos", pos);
	cJSON_AddNumberToObject(data, "serial", 1);

	strout = cJSON_PrintUnformatted(data);
	memset((char*) buf, 0, sizeof(256));
	strcpy((char*) buf, strout);
	free(strout);
	cJSON_Delete(data);

	return;
}

void data_init(void)
{
	BYTE work[_MAX_SS];
	FRESULT res;
	FATFS *fs;
	FIL fil;
	DWORD fre_clust, fre_sect, tot_sect;

	//mount volume
	res = f_mount(&FatFs, "", 1);
	if (res)
	{
		log_i("f_mount error:%d,no volume,start format\r\n", res);
		res = f_mkfs("", FM_FAT, 0, work, _MAX_SS);
		if (res)
			log_e("format error:%d \r\n", res);
		else
		{
			res = f_mount(&FatFs, "", 1);
		}
	}

	log_i("ves size:%d", flash_storage_get_size());
	log_i("f_mount success!\r\n");


	//check free space
	/* Get volume information and free clusters of drive 1 */
	res = f_getfree("", &fre_clust, &fs);

	/* Get total sectors and free sectors */
	tot_sect = (fs->n_fatent - 2) * fs->csize;
	fre_sect = fre_clust * fs->csize;

	/* Print the free space (assuming 512 bytes/sector) */
	log_i("%lu KiB total drive space , %lu KiB available.", tot_sect / 2,
			fre_sect / 2);


	//create record file
	res = f_open(&fil,DATA_FILE_NAME,FA_WRITE | FA_OPEN_EXISTING);
	if(res != FR_OK)
	{
		res = f_close(&fil);
		data_reset_data();
		log_i("file created");
	}
	else
	{
		res = f_close(&fil);
	}



}
