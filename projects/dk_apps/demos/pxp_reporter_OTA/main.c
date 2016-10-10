/**
 ****************************************************************************************
 *
 * @file main.c
 *
 * @brief Proximity Reporter
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
#include <stdbool.h>

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "sdk_defs.h"
#include "ad_ble.h"
#include "ad_nvms.h"
#include "ad_nvparam.h"
#include "ble_mgr.h"
#include "hw_cpm.h"
#include "hw_gpio.h"
#include "sys_clock_mgr.h"
#include "sys_power_mgr.h"
#include "sys_watchdog.h"
#include "platform_devices.h"

#include "app\app_main.h"

/* Task priorities */
#define mainPXP_REPORTER_TASK_PRIORITY              ( OS_TASK_PRIORITY_NORMAL )

/* The configCHECK_FOR_STACK_OVERFLOW setting in FreeRTOSConifg can be used to
check task stacks for overflows.  It does not however check the stack used by
interrupts.  This demo has a simple addition that will also check the stack used
by interrupts if mainCHECK_INTERRUPT_STACK is set to 1.  Note that this check is
only performed from the tick hook function (which runs in an interrupt context).
It is a good debugging aid - but won't catch interrupt stack problems until the
tick interrupt next executes. */
//#define mainCHECK_INTERRUPT_STACK			1
#if mainCHECK_INTERRUPT_STACK == 1
const unsigned char ucExpectedInterruptStackValues[] = { 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC };
#endif

#if dg_configUSE_WDOG
INITIALISED_PRIVILEGED_DATA int8_t idle_task_wdog_id = -1;
#endif


/*
 * Perform any application specific hardware configuration.  The clocks,
 * memory, etc. are configured before main() is called.
 */
static void prvSetupHardware( void );

void pxp_reporter_task( void *pvParameters);

static void system_init( void *pvParameters )
{
        OS_TASK handle;

        /* Prepare clocks. Note: cm_cpu_clk_set() and cm_sys_clk_set() can be called only from a
         * task since they will suspend the task until the XTAL16M has settled and, maybe, the PLL
         * is locked.
         */
        cm_sys_clk_init(sysclk_XTAL16M);
        cm_apb_set_clock_divider(apb_div1);
        cm_ahb_set_clock_divider(ahb_div1);
        cm_lp_clk_init();

        /*
         * Initialize platform watchdog
         */
        sys_watchdog_init();

         /* Set system clock */
        cm_sys_clk_set(sysclk_XTAL16M);

        /* Prepare the hardware to run this demo. */
        prvSetupHardware();

        /* init resources */
        resource_init();

        /* init GPADC adapter */
        GPADC_INIT();

        /* Set the desired sleep mode. */
        pm_set_wakeup_mode(true);
        pm_set_sleep_mode(pm_mode_deep_sleep_no_mirror);

        /* Initialize IIC Adapter */
        ad_i2c_init();

        I2C_BUS_INIT(I2C1);

        I2C_DEVICE_INIT(FXOS8700);

        I2C_DEVICE_INIT(FXOS21002);


        /* Initialize NVMS adapter - has to be done before BLE starts */
        ad_nvms_init();

        /* Initialize BLE Adapter */
        ad_ble_init();

        /* Initialize BLE Manager */
        ble_mgr_init();

        /* Start the PXP reporter application task. */
        OS_TASK_CREATE("main_task",                  /* The text name assigned to the task, for
                                                           debug only; not used by the kernel. */
        			   main_task,               /* The function that implements the task. */
                       NULL,                            /* The parameter passed to the task. */
#if (dg_configDISABLE_BACKGROUND_FLASH_OPS == 1)
                       2000,                             /* The number of bytes to allocate to the
                                                           stack of the task. */
#else
                       3000,                             /* The number of bytes to allocate to the
                                                           stack of the task. */
#endif
                       mainPXP_REPORTER_TASK_PRIORITY,  /* The priority assigned to the task. */
                       handle);                         /* The task handle. */
        OS_ASSERT(handle);

        /* the work of the SysInit task is done */
        OS_TASK_DELETE(OS_GET_CURRENT_TASK());
}

/**
 * @brief BLE FW demo main creates a BLE task
 */
int main( void )
{
        OS_TASK handle;
        OS_BASE_TYPE status __attribute__((unused));

        cm_clk_init_low_level();                          /* Basic clock initializations. */

        /* Start SysInit task. */
        status = OS_TASK_CREATE("SysInit",                /* The text name assigned to the task, for
                                                             debug only; not used by the kernel. */
                                system_init,              /* The System Initialization task. */
                                ( void * ) 0,             /* The parameter passed to the task. */
                                1024,                     /* The number of bytes to allocate to the
                                                             stack of the task. */
                                OS_TASK_PRIORITY_HIGHEST, /* The priority assigned to the task. */
                                handle );                 /* The task handle */
        OS_ASSERT(status == OS_TASK_CREATE_SUCCESS);


        /* Start the tasks and timer running. */
        vTaskStartScheduler();

        /* If all is well, the scheduler will now be running, and the following
        line will never be reached.  If the following line does execute, then
        there was insufficient FreeRTOS heap memory available for the idle and/or
        timer tasks to be created.  See the memory management section on the
        FreeRTOS web site for more details. */
        for( ;; );
}

void periph_setup(void)
{
        /* configure GPIOs for input/output UART */
#       if dg_configBLACK_ORCA_MB_REV == BLACK_ORCA_MB_REV_A
#               define UART_TX_PORT    HW_GPIO_PORT_1
#               define UART_TX_PIN     HW_GPIO_PIN_0
#               define UART_RX_PORT    HW_GPIO_PORT_1
#               define UART_RX_PIN     HW_GPIO_PIN_5
#       elif dg_configBLACK_ORCA_MB_REV == BLACK_ORCA_MB_REV_D
#               define UART_TX_PORT    HW_GPIO_PORT_3
#               define UART_TX_PIN     HW_GPIO_PIN_0
#               define UART_RX_PORT    HW_GPIO_PORT_3
#               define UART_RX_PIN     HW_GPIO_PIN_1
#       else
#               error "Unknown value for dg_configBLACK_ORCA_MB_REV!"
#       endif
//        hw_gpio_set_pin_function(UART_TX_PORT, UART_TX_PIN,
//        		HW_GPIO_MODE_OUTPUT,HW_GPIO_FUNC_UART_TX);
//        hw_gpio_set_pin_function(UART_RX_PORT, UART_RX_PIN,
//        		HW_GPIO_MODE_INPUT,HW_GPIO_FUNC_UART_RX);
		hw_gpio_set_pin_function(HW_GPIO_PORT_3, HW_GPIO_PIN_0,
				HW_GPIO_MODE_OUTPUT,HW_GPIO_FUNC_UART_TX);
		hw_gpio_set_pin_function(HW_GPIO_PORT_3, HW_GPIO_PIN_1,
				HW_GPIO_MODE_INPUT,HW_GPIO_FUNC_UART_RX);

		//led
		hw_gpio_set_pin_function(HW_GPIO_PORT_1, HW_GPIO_PIN_5,
				HW_GPIO_MODE_OUTPUT_PUSH_PULL,HW_GPIO_FUNC_GPIO);
		//button
		hw_gpio_set_pin_function(HW_GPIO_PORT_1, HW_GPIO_PIN_7,
				HW_GPIO_MODE_INPUT,HW_GPIO_FUNC_GPIO);

        //I2C***************************************************
		#define CFG_GPIO_I2C1_SCL_PORT          (HW_GPIO_PORT_3)
		#define CFG_GPIO_I2C1_SCL_PIN           (HW_GPIO_PIN_3)
		#define CFG_GPIO_I2C1_SDA_PORT          (HW_GPIO_PORT_3)
		#define CFG_GPIO_I2C1_SDA_PIN           (HW_GPIO_PIN_2)

        hw_gpio_set_pin_function(CFG_GPIO_I2C1_SCL_PORT, CFG_GPIO_I2C1_SCL_PIN,
        		HW_GPIO_MODE_INPUT, HW_GPIO_FUNC_I2C_SCL);
        hw_gpio_set_pin_function(CFG_GPIO_I2C1_SDA_PORT, CFG_GPIO_I2C1_SDA_PIN,
        		HW_GPIO_MODE_INPUT, HW_GPIO_FUNC_I2C_SDA);
        //I2C***************************************************

        //GPIO**************************************************
		#define CFG_GPIO_8700_INT1_PORT          (HW_GPIO_PORT_1)
		#define CFG_GPIO_8700_INT1_PIN           (HW_GPIO_PIN_7)
		#define CFG_GPIO_8700_INT2_PORT          (HW_GPIO_PORT_1)
		#define CFG_GPIO_8700_INT2_PIN           (HW_GPIO_PIN_4)

        hw_gpio_set_pin_function(CFG_GPIO_8700_INT1_PORT, CFG_GPIO_8700_INT1_PIN,
        		HW_GPIO_MODE_INPUT_PULLUP, HW_GPIO_FUNC_GPIO);
        hw_gpio_set_pin_function(CFG_GPIO_8700_INT2_PORT, CFG_GPIO_8700_INT2_PIN,
        		HW_GPIO_MODE_INPUT_PULLUP, HW_GPIO_FUNC_GPIO);
}

static void prvSetupHardware( void )
{
#if mainCHECK_INTERRUPT_STACK == 1
        extern unsigned long _vStackTop[], _pvHeapStart[];
        unsigned long ulInterruptStackSize;
#endif

        /* Init hardware */
        pm_system_init(periph_setup);

#if mainCHECK_INTERRUPT_STACK == 1
        /* The size of the stack used by main and interrupts is not defined in
           the linker, but just uses whatever RAM is left.  Calculate the amount of
           RAM available for the main/interrupt/system stack, and check it against
           a reasonable number.  If this assert is hit then it is likely you don't
           have enough stack to start the kernel, or to allow interrupts to nest.
           Note - this is separate to the stacks that are used by tasks.  The stacks
           that are used by tasks are automatically checked if
           configCHECK_FOR_STACK_OVERFLOW is not 0 in FreeRTOSConfig.h - but the stack
           used by interrupts is not.  Reducing the conifgTOTAL_HEAP_SIZE setting will
           increase the stack available to main() and interrupts. */
        ulInterruptStackSize = ( ( unsigned long ) _vStackTop ) - ( ( unsigned long ) _pvHeapStart );
        OS_ASSERT( ulInterruptStackSize > 350UL );

        /* Fill the stack used by main() and interrupts to a known value, so its 
           use can be manually checked. */
        memcpy( ( void * ) _pvHeapStart, ucExpectedInterruptStackValues, sizeof( ucExpectedInterruptStackValues ) );
#endif
}

/**
 * @brief Malloc fail hook
 */
void vApplicationMallocFailedHook( void )
{
        /* vApplicationMallocFailedHook() will only be called if
	configUSE_MALLOC_FAILED_HOOK is set to 1 in FreeRTOSConfig.h.  It is a hook
	function that will get called if a call to pvPortMalloc() fails.
	pvPortMalloc() is called internally by the kernel whenever a task, queue,
	timer or semaphore is created.  It is also called by various parts of the
	demo application.  If heap_1.c or heap_2.c are used, then the size of the
	heap available to pvPortMalloc() is defined by configTOTAL_HEAP_SIZE in
	FreeRTOSConfig.h, and the xPortGetFreeHeapSize() API function can be used
	to query the size of free heap space that remains (although it does not
	provide information on how the remaining heap might be fragmented). */
        taskDISABLE_INTERRUPTS();
        for( ;; );
}

/**
 * @brief Application idle task hook
 */
void vApplicationIdleHook( void )
{
        /* vApplicationIdleHook() will only be called if configUSE_IDLE_HOOK is set
           to 1 in FreeRTOSConfig.h.  It will be called on each iteration of the idle
           task. It is essential that code added to this hook function never attempts
           to block in any way (for example, call xQueueReceive() with a block time
           specified, or call vTaskDelay()).  If the application makes use of the
           vTaskDelete() API function (as this demo application does) then it is also
           important that vApplicationIdleHook() is permitted to return to its calling
           function, because it is the responsibility of the idle task to clean up
           memory allocated by the kernel to any task that has since been deleted. */

#if dg_configUSE_WDOG
        // Register the Idle task first.
	if(idle_task_wdog_id == -1)
	{
                idle_task_wdog_id = sys_watchdog_register(false);
                ASSERT_WARNING(idle_task_wdog_id != -1);
                sys_watchdog_configure_idle_id(idle_task_wdog_id);
	}

        sys_watchdog_notify(idle_task_wdog_id);
#endif
}

/**
 * @brief Application stack overflow hook
 */
void vApplicationStackOverflowHook( OS_TASK pxTask, char *pcTaskName )
{
        ( void ) pcTaskName;
        ( void ) pxTask;

        /* Run time stack overflow checking is performed if
	configCHECK_FOR_STACK_OVERFLOW is defined to 1 or 2.  This hook
	function is called if a stack overflow is detected. */
        taskDISABLE_INTERRUPTS();
        for( ;; );
}

/**
 * @brief Application tick hook
 */
void vApplicationTickHook( void )
{
#if mainCHECK_INTERRUPT_STACK == 1
        extern unsigned long _pvHeapStart[];

        /* This function will be called by each tick interrupt if
	configUSE_TICK_HOOK is set to 1 in FreeRTOSConfig.h.  User code can be
	added here, but the tick hook is called from an interrupt context, so
	code must not attempt to block, and only the interrupt safe FreeRTOS API
	functions can be used (those that end in FromISR()). */

        /* Manually check the last few bytes of the interrupt stack to check they
	have not been overwritten.  Note - the task stacks are automatically
	checked for overflow if configCHECK_FOR_STACK_OVERFLOW is set to 1 or 2
	in FreeRTOSConifg.h, but the interrupt stack is not. */
        OS_ASSERT( memcmp( ( void * ) _pvHeapStart, ucExpectedInterruptStackValues, sizeof( ucExpectedInterruptStackValues ) ) == 0U );
#endif /* mainCHECK_INTERRUPT_STACK */
}

