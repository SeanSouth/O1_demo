#ifndef CHECK_LIST
#define CHECK_LIST

#define MAX_CMD_LENGTH		50
#define MAX_CODE_LENGTH		5
#define CODE_NUM			6

#define CODE_MATCH_TRUE		0
#define CODE_MATCH_FALSE	1

#define CMP_FLAG_FALSE 		0
#define CMP_FLAG_TRUE 		1




#define TIME_FMT "%Y-%m-%d %H:%M:%S"
#define SERIAL_NUM 11111111111111

#define CMDTRUE 1
#define CMDFALSE 0

typedef char * (*fun)(char *);


typedef struct _cmd_table
{
	uint8_t order;
	char* cmd;
	fun funcptr;

} cmd_table;

