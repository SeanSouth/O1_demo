#include <app/check_list.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/_stdint.h>

#include "check_list.h"
#include "osal.h"
#include "app_debug.h"

OS_TASK chk_task_h;
OS_TASK chk_status_h;

OS_QUEUE txq;
OS_QUEUE rxq;

cmd_table check_table[] =
{
{ 0, "GFV", chk_get_fireware_version },
{ 1, "SPT", chk_set_produce_time },
{ 2, "GPT", chk_get_produce_time },
{ 3, "SSN", chk_set_serial_time },
{ 4, "GSN", chk_get_produce_time },
{ 5, 0x0, NULL } };

char *chk_parse_cmd_rx(char *buf_rx)
{
	char *command = malloc(MAX_CODE_LENGTH + 1);
	uint8_t i = 0;
	while (buf_rx[i + 3] == '\r' && buf_rx[i + 3] != '\n'
			&& buf_rx[i + 3] != ' ')
	{
		command[i] = buf_rx[i + 3];
		i++;
	}
	command[i] = '\0';
	return command;
}

/*
 * If command is wrong: return 0;
 * if command is correct: return the order of the command
 */
uint8_t chk_chk_cmd(char *buf_rx)
{
	if (buf_rx[0] != 'A' || buf_rx[1] != 'T' || buf_rx[2] != '+')
	{
		return CMDFALSE;
	}
	else
	{
		uint8_t i;
		uint8_t code_match = CODE_MATCH_FALSE;
		uint8_t cmp_flag = CMP_FLAG_FALSE;
		uint8_t cmd_order;
		char *cmd_rcv = chk_parse_cmd_rx(buf_rx);
		for (i = 0; i < (sizeof(check_table) / sizeof(check_table[0])); i++)
		{
			code_match = strncpy(cmd_rcv, check_table[i].cmd, 3);
			if (code_match == CODE_MATCH_TRUE)
			{
				cmp_flag = CMP_FLAG_TRUE;
				cmd_order = check_table[i].order;
			}
		}
		if (cmp_flag == CMP_FLAG_TRUE)
		{
			return cmd_order;
		}
		else
		{
			return CMDFALSE;
		}
	}
}

char * chk_get_fireware_version(char * buf_rx)
{

}

char * chk_set_produce_time(char * buf_rx)
{

}

char * chk_get_produce_time(char * buf_rx)
{

}

char * chk_set_serial_time(char * buf_rx)
{

}

char * chk_get_produce_time(char * buf_rx)
{

}

char *chl_parse(char *buf_rx)
{
	uint8_t chk_order;
	char *resp_cmd;
	while (1)
	{
		chk_order = chk_chk_cmd(buf_rx);
		if (chk_order == 0)
		{
			break;
		}
		else
		{
			resp_cmd = check_table[chk_order].funcptr;
		}
	}

}

