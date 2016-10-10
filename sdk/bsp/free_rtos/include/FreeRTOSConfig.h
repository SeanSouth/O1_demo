/*
    FreeRTOS V8.2.0 - Copyright (C) 2015 Real Time Engineers Ltd.
    All rights reserved

    VISIT http://www.FreeRTOS.org TO ENSURE YOU ARE USING THE LATEST VERSION.

    This file is part of the FreeRTOS distribution.

    FreeRTOS is free software; you can redistribute it and/or modify it under
    the terms of the GNU General Public License (version 2) as published by the
    Free Software Foundation >>!AND MODIFIED BY!<< the FreeRTOS exception.

	***************************************************************************
    >>!   NOTE: The modification to the GPL is included to allow you to     !<<
    >>!   distribute a combined work that includes FreeRTOS without being   !<<
    >>!   obliged to provide the source code for proprietary components     !<<
    >>!   outside of the FreeRTOS kernel.                                   !<<
	***************************************************************************

    FreeRTOS is distributed in the hope that it will be useful, but WITHOUT ANY
    WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
    FOR A PARTICULAR PURPOSE.  Full license text is available on the following
    link: http://www.freertos.org/a00114.html

    ***************************************************************************
     *                                                                       *
     *    FreeRTOS provides completely free yet professionally developed,    *
     *    robust, strictly quality controlled, supported, and cross          *
     *    platform software that is more than just the market leader, it     *
     *    is the industry's de facto standard.                               *
     *                                                                       *
     *    Help yourself get started quickly while simultaneously helping     *
     *    to support the FreeRTOS project by purchasing a FreeRTOS           *
     *    tutorial book, reference manual, or both:                          *
     *    http://www.FreeRTOS.org/Documentation                              *
     *                                                                       *
    ***************************************************************************

    http://www.FreeRTOS.org/FAQHelp.html - Having a problem?  Start by reading
	the FAQ page "My application does not run, what could be wrong?".  Have you
	defined configASSERT()?

	http://www.FreeRTOS.org/support - In return for receiving this top quality
	embedded software for free we request you assist our global community by
	participating in the support forum.

	http://www.FreeRTOS.org/training - Investing in training allows your team to
	be as productive as possible as early as possible.  Now you can receive
	FreeRTOS training directly from Richard Barry, CEO of Real Time Engineers
	Ltd, and the world's leading authority on the world's leading RTOS.

    http://www.FreeRTOS.org/plus - A selection of FreeRTOS ecosystem products,
    including FreeRTOS+Trace - an indispensable productivity tool, a DOS
    compatible FAT file system, and our tiny thread aware UDP/IP stack.

    http://www.FreeRTOS.org/labs - Where new FreeRTOS products go to incubate.
    Come and try FreeRTOS+TCP, our new open source TCP/IP stack for FreeRTOS.

    http://www.OpenRTOS.com - Real Time Engineers ltd. license FreeRTOS to High
    Integrity Systems ltd. to sell under the OpenRTOS brand.  Low cost OpenRTOS
    licenses offer ticketed support, indemnification and commercial middleware.

    http://www.SafeRTOS.com - High Integrity Systems also provide a safety
    engineered and independently SIL3 certified version for use in safety and
    mission critical applications that require provable dependability.

    1 tab == 4 spaces!
*/


#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

/*
 * The following #error directive is to remind users that a batch file must be
 * executed prior to this project being built.  The batch file *cannot* be
 * executed from within the IDE!  Once it has been executed, re-open or refresh
 * the Eclipse project and remove the #error line below.
 */
//#error Ensure CreateProjectDirectoryStructure.bat has been executed before building.  See comment immediately above.

/*-----------------------------------------------------------
 * Application specific definitions.
 *
 * These definitions should be adjusted for your particular hardware and
 * application requirements.
 *
 * THESE PARAMETERS ARE DESCRIBED WITHIN THE 'CONFIGURATION' SECTION OF THE
 * FreeRTOS API DOCUMENTATION AVAILABLE ON THE FreeRTOS.org WEB SITE.
 *
 * See http://www.freertos.org/a00110.html.
 *----------------------------------------------------------*/

#include <stdint.h>
#include "hw_watchdog.h"
#include "sys_clock_mgr.h"

extern uint32_t SystemCoreClock;

#define configUSE_PREEMPTION                    1
#if ((dg_configTRACK_OS_HEAP == 1) || (dg_configUSE_WDOG == 1))
#define configUSE_IDLE_HOOK                     1
#else
#define configUSE_IDLE_HOOK                     0
#endif /* (dg_configTRACK_OS_HEAP == 1) */
#define configUSE_TICK_HOOK                     0
#define configCPU_CLOCK_HZ                      ( SystemCoreClock )

#if (dg_configUSE_LP_CLK == LP_CLK_32000)
#define configSYSTICK_CLOCK_HZ                  ( 32000 )
#define configTICK_RATE_HZ                      ( ( TickType_t ) 500 )
#define TICK_PERIOD   ((configSYSTICK_CLOCK_HZ / configTICK_RATE_HZ) / (1 + dg_configTim1Prescaler))

#elif (dg_configUSE_LP_CLK == LP_CLK_32768)
#define configSYSTICK_CLOCK_HZ                  ( 32768 )
#define configTICK_RATE_HZ                      ( ( TickType_t ) 512 )
#define TICK_PERIOD   ((configSYSTICK_CLOCK_HZ / configTICK_RATE_HZ) / (1 + dg_configTim1Prescaler))

#elif (dg_configUSE_LP_CLK == LP_CLK_RCX)
#define configSYSTICK_CLOCK_HZ                  ( rcx_clock_hz )
#define configTICK_RATE_HZ                      ( ( TickType_t ) rcx_tick_rate_hz )
#define TICK_PERIOD                             ( rcx_tick_period )
#endif

#define configMAX_PRIORITIES                    ( 7 )
#define configMINIMAL_STACK_SIZE                ( ( unsigned short ) 400 )

#ifndef configTOTAL_HEAP_SIZE
#define configTOTAL_HEAP_SIZE                   ( ( size_t ) ( 7168 ) ) //  ( 6500 ) )
#endif
#define configMAX_TASK_NAME_LEN                 ( 16 )
#if (dg_configIMAGE_SETUP == DEVELOPMENT_MODE)
# define configUSE_TRACE_FACILITY               1
#else
# define configUSE_TRACE_FACILITY               0
#endif
#define configUSE_16_BIT_TICKS                  0
#define configIDLE_SHOULD_YIELD                 1
#define configUSE_MUTEXES                       1
#define configQUEUE_REGISTRY_SIZE               8
#define configCHECK_FOR_STACK_OVERFLOW          2
#define configUSE_RECURSIVE_MUTEXES             1
#define configUSE_MALLOC_FAILED_HOOK            1
#define configUSE_APPLICATION_TASK_TAG          0
#define configUSE_COUNTING_SEMAPHORES           1
#define configGENERATE_RUN_TIME_STATS           0 //teddy
#define configUSE_STATS_FORMATTING_FUNCTIONS	0 //teddy
#define configUSE_QUEUE_SETS                    1

#define configUSE_TICKLESS_IDLE                 2
/* The minimum allowed value for configEXPECTED_IDLE_TIME_BEFORE_SLEEP is 2. */
#define configEXPECTED_IDLE_TIME_BEFORE_SLEEP   2
#define portSUPPRESS_TICKS_AND_SLEEP( x )       prvSystemSleep( x )
#define configPRE_STOP_PROCESSING()
#define configPRE_SLEEP_PROCESSING( x )
#define configPOST_SLEEP_PROCESSING()
#define configPRE_IDLE_ENTRY( x )               /*cm_lower_all_clocks()*/
#define configPOST_IDLE_ENTRY( x )              /*cm_restore_all_clocks()*/

/* Co-routine definitions. */
#define configUSE_CO_ROUTINES                   0
#define configMAX_CO_ROUTINE_PRIORITIES         ( 2 )

/* Software timer definitions. */
#define configUSE_TIMERS                        1
#define configTIMER_TASK_PRIORITY               ( configMAX_PRIORITIES - 1 )
#ifndef configTIMER_QUEUE_LENGTH
#define configTIMER_QUEUE_LENGTH                6
#endif
#ifndef configTIMER_TASK_STACK_DEPTH
#define configTIMER_TASK_STACK_DEPTH            ( configMINIMAL_STACK_SIZE )
#endif

/* Set the following definitions to 1 to include the API function, or zero
to exclude the API function. */
#define INCLUDE_vTaskPrioritySet                1
#define INCLUDE_uxTaskPriorityGet               1
#define INCLUDE_vTaskDelete                     1
#define INCLUDE_vTaskCleanUpResources           1
#define INCLUDE_vTaskSuspend                    1
#define INCLUDE_vTaskDelayUntil                 1
#define INCLUDE_vTaskDelay                      1
#define INCLUDE_eTaskGetState                   1
#define INCLUDE_xEventGroupSetBitFromISR        1
#define INCLUDE_xTimerPendFunctionCall          1
#if (dg_configTRACK_OS_HEAP == 1)
#define INCLUDE_uxTaskGetStackHighWaterMark     1
#endif /* (dg_configTRACK_OS_HEAP == 1) */

/* -------------------------------------------------------------------- */
/* Cortex-M specific definitions. */
#define configPRIO_BITS                         2 /* 2 bits/4 priority levels on ARM Cortex M0 */

/* The lowest interrupt priority that can be used in a call to a "set priority" function. */
#define configLIBRARY_LOWEST_INTERRUPT_PRIORITY   3

/* The highest interrupt priority that can be used by any interrupt service
   routine that makes calls to interrupt safe FreeRTOS API functions.  DO NOT CALL
   INTERRUPT SAFE FREERTOS API FUNCTIONS FROM ANY INTERRUPT THAT HAS A HIGHER
   PRIORITY THAN THIS! (higher priorities are lower numeric values on an ARM Cortex-M). */
#define configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY 1

/* Interrupt priorities used by the kernel port layer itself.  These are generic
   to all Cortex-M ports, and do not rely on any particular library functions. */
#define configKERNEL_INTERRUPT_PRIORITY         (configLIBRARY_LOWEST_INTERRUPT_PRIORITY<<(8-configPRIO_BITS))
/* !!!! configMAX_SYSCALL_INTERRUPT_PRIORITY must not be set to zero !!!!
See http://www.FreeRTOS.org/RTOS-Cortex-M3-M4.html. */
#define configMAX_SYSCALL_INTERRUPT_PRIORITY    (configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY<<(8-configPRIO_BITS))

/* Normal assert() semantics without relying on the provision of an assert.h header file. */
#if (dg_configIMAGE_SETUP == DEVELOPMENT_MODE)
# define configASSERT( x ) if( ( x ) == 0 ) { taskDISABLE_INTERRUPTS(); hw_watchdog_freeze(); do {} while(1); }
#else
# define configASSERT( x ) do { } while (0)
#endif

/* Definitions that map the FreeRTOS port interrupt handlers to their CMSIS
standard names - or at least those used in the unmodified vector table. */
#define vPortSVCHandler SVCall_Handler
#define xPortPendSVHandler PendSV_Handler
#define xPortSysTickHandler SysTick_Handler

extern void vConfigureTimerForRunTimeStats( void );
extern uint32_t vGetRunTimerCounterValue( void );
//#define portCONFIGURE_TIMER_FOR_RUN_TIME_STATS()	vConfigureTimerForRunTimeStats()//teddy
//#define portGET_RUN_TIME_COUNTER_VALUE()	vGetRunTimerCounterValue()		// teddy

#if dg_configSYSTEMVIEW
# include <SEGGER_SYSVIEW_FreeRTOS.h>
#endif /* dg_configSYSTEMVIEW */

#endif /* FREERTOS_CONFIG_H */

