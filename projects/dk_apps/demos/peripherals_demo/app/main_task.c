/**
 ****************************************************************************************
 *
 * @file main_task.c
 *
 * @brief Main task of the peripherals demo application
 *
 * Copyright (C) 2015. Dialog Semiconductor Ltd, unpublished work. This computer
 * program includes Confidential, Proprietary Information and is a Trade Secret of
 * Dialog Semiconductor Ltd.  All use, disclosure, and/or reproduction is prohibited
 * unless authorized in writing. All Rights Reserved.
 *
 * <black.orca.support@diasemi.com> and contributors.
 *
 ****************************************************************************************
 */

#include <string.h>
#include <stdio.h>
#include <osal.h>
#include <resmgmt.h>
#include "app/menu.h"
#include "app/tasklist.h"
#include "platform_devices.h"
#include "app_debug.h"

static inline void readline(char *buf, size_t size)
{
        int c;

//        do {
//                c = getchar();
//                putchar(c);
//                *buf = c;
//                buf++;
//        } while (c != '\n' && c != '\r'); // wait for CR/LF or reserve 1 char for \0



        do {
                c = getc(stdin);
                printf("receive is:%c\r\n",c);
                *buf = c;
                buf++;
        } while (c != '\n' && c != '\r'); // wait for CR/LF or reserve 1 char for \0

        /* make sure it's null-terminated */
        *buf = '\0';
}

void main_task_func(void *param)
{
        char s[64];

        resource_init();

//        app_tasklist_create();

//       console_init(SERIAL1, 256);

//        printf("start!now!\r\n");

        debug_init();
        printf("start!now!64\r\n");

        for(;;) {
 //               app_menu_draw();

//        		memset(s,0,sizeof(s));

//                readline(s, sizeof(s));

//                uint32_t index;

//               printf("print is :%c",s[]);

//                for (index=0; index<64; index++)
//                {
//                	printf("%d\t",s[index]);
//                }


 //               app_menu_parse_selection(s);
        }
}
