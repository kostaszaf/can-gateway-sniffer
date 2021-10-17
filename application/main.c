/**
 ****************************************************************************************
 *
 * @file main.c
 *
 * @brief FreeRTOS template application with retarget
 *
 * Copyright (C) 2015-2019 Dialog Semiconductor.
 * This computer program includes Confidential, Proprietary Information
 * of Dialog Semiconductor. All Rights Reserved.
 *
 ****************************************************************************************
 */

#include <string.h>
#include <stdio.h>
#include <stdbool.h>

#include "osal.h"
#include "resmgmt.h"
#include "hw_cpm.h"
#include "hw_gpio.h"
#include "hw_watchdog.h"
#include "sys_clock_mgr.h"
#include "sys_power_mgr.h"

#include "hw_gpio.h"
#include "hw_wkup.h"

#include "ad_spi.h"

#include "platform_devices.h"
#include "drv_canfdspi_api.h"
#include "drv_canfdspi_register.h"
#include "sys_timer.h"
#include "hw_clk.h"

int ee_printf(const char *fmt, ...);
#define printf ee_printf

/* Task priorities */
#define mainTEMPLATE_TASK_PRIORITY              ( OS_TASK_PRIORITY_NORMAL )

#define SPI_NOTIF 1<<8
#define PID_NOTIF 1<<7
#define APP_NOTIF 1<<6
#define PRT_NOTIF 1<<5
#define IGL_NOTIF 1<<4

/*
 * Perform any application specific hardware configuration.  The clocks,
 * memory, etc. are configured before main() is called.
 */
static void prvSetupHardware( void );
/*
 * Task functions .
 */
static void prvTemplateTask( void *pvParameters );

static OS_TASK xHandle;
static OS_TASK task_h = NULL;

void APP_CANFDSPI_Init();
void APP_CANFDSPI2_Init();
void handle_spi_interrupt();
void handle_spi2_interrupt();

void pid_timer_cb_exec();
void app_timer_cb_exec();
void print_timer_cb_exec();
void igla_deactivate_cb_exec();

volatile  uint16_t spi_transferred;
volatile  uint16_t spi2_transferred;

static ad_spi_handle_t mcp2518fd_hdl;
static ad_spi_handle_t mcp2518fd_hdl2;


/////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////  Use the ad_spi API to implement the DRV_SPI_TransferData low level function needed by the MSC2518FD

#define _delay_ms(ms) \
        hw_clk_delay_usec((uint32_t)(ms) * 1000); \

static void spi_cb_no_preempt(void *user_data, uint16_t transferred)
{
        *((uint16_t*)user_data) = transferred;
}

static void spi2_cb_no_preempt(void *user_data, uint16_t transferred)
{
        *((uint16_t*)user_data) = transferred;
}

#define _ad_spi_write_read_no_preempt(handle, wbuf, rbuf, rlen, timeout) \
        { \
                spi_transferred = (uint16_t)-1; \
                ad_spi_write_read_async((handle), (wbuf), (rlen), (rbuf), (rlen), spi_cb_no_preempt, (uint16_t *) &spi_transferred); \
                while (spi_transferred == (uint16_t)-1); \
        }

#define _ad_spi2_write_read_no_preempt(handle, wbuf, rbuf, rlen, timeout) \
        { \
                spi2_transferred = (uint16_t)-1; \
                ad_spi_write_read_async((handle), (wbuf), (rlen), (rbuf), (rlen), spi2_cb_no_preempt, (uint16_t *) &spi2_transferred); \
                while (spi2_transferred == (uint16_t)-1); \
        }

int8_t DRV_SPI_TransferData(uint8_t spiSlaveDeviceIndex, uint8_t *SpiTxData, uint8_t *SpiRxData, uint16_t spiTransferSize)
{

        if(spiSlaveDeviceIndex){
                ad_spi_activate_cs(mcp2518fd_hdl2);
                _ad_spi2_write_read_no_preempt(mcp2518fd_hdl2, SpiTxData, SpiRxData, spiTransferSize, OS_EVENT_FOREVER );
                ad_spi_deactivate_cs_when_spi_done(mcp2518fd_hdl2);
                return 0;
        }

        ad_spi_activate_cs(mcp2518fd_hdl);
        _ad_spi_write_read_no_preempt(mcp2518fd_hdl, SpiTxData, SpiRxData, spiTransferSize, OS_EVENT_FOREVER );
        ad_spi_deactivate_cs_when_spi_done(mcp2518fd_hdl);
        return 0;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool dk_hw_k1_button_pressed(void)
{
        return hw_gpio_get_pin_status(KEY1_PORT, KEY1_PIN);
}

bool dk_hw_int_detected(void)
{
        return !hw_gpio_get_pin_status(INT_PORT, INT_PIN);
}

bool dk_hw_int2_detected(void)
{
        return !hw_gpio_get_pin_status(INT2_PORT, INT2_PIN);
}

void gpio_cb(){ // ISR triggered either by button press or any MSC2518FD interrupt
        OS_TASK_NOTIFY_FROM_ISR(task_h, SPI_NOTIF, OS_NOTIFY_SET_BITS);
        hw_wkup_clear_status(INT_PORT, 1 << INT_PIN);
        hw_wkup_clear_status(INT2_PORT, 1 << INT2_PIN);
        hw_wkup_clear_status(KEY1_PORT, 1 << KEY1_PIN);
}

void pid_timer_cb(OS_TIMER pxTime){ // Generic SW timer interrupt
        OS_TASK_NOTIFY(task_h, PID_NOTIF, OS_NOTIFY_SET_BITS);
}

void app_timer_cb(OS_TIMER pxTime){ // Generic SW timer interrupt
        OS_TASK_NOTIFY(task_h, APP_NOTIF, OS_NOTIFY_SET_BITS);
}

void print_timer_cb(OS_TIMER pxTime){ // Generic SW timer interrupt
        OS_TASK_NOTIFY(task_h, PRT_NOTIF, OS_NOTIFY_SET_BITS);
}

void igla_deactivate_cb(OS_TIMER pxTime){ // Generic SW timer interrupt
        OS_TASK_NOTIFY(task_h, IGL_NOTIF, OS_NOTIFY_SET_BITS);
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////


static void system_init( void *pvParameters )
{


#if defined CONFIG_RETARGET
        extern void retarget_init(void);
#endif

        cm_sys_clk_init(sysclk_XTAL32M);
        cm_apb_set_clock_divider(apb_div1);
        cm_ahb_set_clock_divider(ahb_div1);
        cm_lp_clk_init();

        /* Prepare the hardware to run this demo. */
        prvSetupHardware();

#if defined CONFIG_RETARGET
        retarget_init();
#endif

        // pm_set_wakeup_mode(true);
        /* Set the desired sleep mode. */
        pm_sleep_mode_set(pm_mode_active);
        cm_sys_clk_set(sysclk_PLL96);

        /* Set the desired wakeup mode. */
        pm_set_sys_wakeup_mode(pm_sys_wakeup_mode_fast);


        /* Start main task here (text menu available via UART1 to control application) */
        OS_TASK_CREATE( "Template",            /* The text name assigned to the task, for
                                                           debug only; not used by the kernel. */
                        prvTemplateTask,                /* The function that implements the task. */
                        NULL,                           /* The parameter passed to the task. */
                        1024 * 8,
                                                        /* The number of bytes to allocate to the
                                                           stack of the task. */
                        mainTEMPLATE_TASK_PRIORITY,     /* The priority assigned to the task. */
                        task_h );                       /* The task handle */
        OS_ASSERT(task_h);

        /* the work of the SysInit task is done */
        OS_TASK_DELETE( xHandle );
}

/**
 * @brief Template main creates a SysInit task, which creates a Template task
 */
int main( void )
{
        OS_BASE_TYPE status;

        /* Start the two tasks as described in the comments at the top of this
        file. */
        status = OS_TASK_CREATE("SysInit",              /* The text name assigned to the task, for
                                                           debug only; not used by the kernel. */
                        system_init,                    /* The System Initialization task. */
                        ( void * ) 0,                   /* The parameter passed to the task. */
                        configMINIMAL_STACK_SIZE * OS_STACK_WORD_SIZE,
                                                        /* The number of bytes to allocate to the
                                                           stack of the task. */
                        OS_TASK_PRIORITY_HIGHEST,       /* The priority assigned to the task. */
                        xHandle );                      /* The task handle */
        OS_ASSERT(status == OS_TASK_CREATE_SUCCESS);



        /* Start the tasks and timer running. */
        vTaskStartScheduler();

        /* If all is well, the scheduler will now be running, and the following
        line will never be reached.  If the following line does execute, then
        there was insufficient FreeRTOS heap memory available for the idle and/or
        timer tasks to be created.  See the memory management section on the
        FreeRTOS web site for more details. */
        for ( ;; );

}


static const gpio_config gpio_cfg[] = {
        HW_GPIO_PINCONFIG(KEY1_PORT, KEY1_PIN, INPUT_PULLDOWN, GPIO, true),
        HW_GPIO_PINCONFIG(LED1_PORT, LED1_PIN, OUTPUT, GPIO, true),
        /* The 3 interrupt lines from 1rst MSC2518FD  */
        HW_GPIO_PINCONFIG(INT_PORT,  INT_PIN,  INPUT, GPIO, true),
        HW_GPIO_PINCONFIG(INT0_PORT, INT0_PIN, INPUT, GPIO, true),
        HW_GPIO_PINCONFIG(INT1_PORT, INT1_PIN, INPUT, GPIO, true),
        /* The 3 interrupt lines from 2nd MSC2518FD */
        HW_GPIO_PINCONFIG(INT2_PORT,  INT2_PIN,  INPUT, GPIO, true),
        HW_GPIO_PINCONFIG(INT20_PORT, INT20_PIN, INPUT, GPIO, true),
        HW_GPIO_PINCONFIG(INT21_PORT, INT21_PIN, INPUT, GPIO, true),
        HW_GPIO_PINCONFIG_END // important!!!
};

void dk_hw_k1_wkup_init(void (*cb)(void))
{
#if dg_configUSE_HW_WKUP
        hw_wkup_init(NULL);
        /* Only the first INT line is used from both MSC2518FD */
        hw_wkup_configure_pin(INT_PORT, INT_PIN, true, HW_WKUP_PIN_STATE_LOW);
        hw_wkup_configure_pin(INT2_PORT, INT2_PIN, true, HW_WKUP_PIN_STATE_LOW);
        hw_wkup_configure_pin(KEY1_PORT, KEY1_PIN, true, HW_WKUP_PIN_STATE_HIGH);
        hw_wkup_set_debounce_time(0);
        hw_wkup_register_key_interrupt(cb, 1);
        hw_wkup_enable_irq();
#endif
}

static void prvTemplateTask( void *pvParameters )
{

        /* Initialise xNextWakeTime - this only needs to be done once. */
       // xNextWakeTime = OS_GET_TICK_COUNT();

        dk_hw_k1_wkup_init(gpio_cb);

        mcp2518fd_hdl = ad_spi_open(MSC2518FD);
        mcp2518fd_hdl2 = ad_spi_open(MSC2518FD2);

        APP_CANFDSPI_Init();
        APP_CANFDSPI2_Init();

        printf("App started!\r\n");

        for ( ;; ) {
                OS_BASE_TYPE ret;
                uint32_t notif;
                /* Place this task in the blocked state until it is time to run again.
                   The block time is specified in ticks, the constant used converts ticks
                   to ms.  While in the Blocked state this task will not consume any CPU
                   time. */

                ret = OS_TASK_NOTIFY_WAIT(0, OS_TASK_NOTIFY_ALL_BITS, &notif, OS_TASK_NOTIFY_FOREVER);
                             /* Blocks forever waiting for task notification. The return value must be OS_OK */
                OS_ASSERT(ret == OS_OK);


                if (notif & PID_NOTIF) { // Serve SW timer interrupt notification - trigger OBD pid reads
                        pid_timer_cb_exec();
                }
                if (notif & APP_NOTIF) { // Serve SW timer interrupt notification - transmit "drive select" or "start stop" button command
                        app_timer_cb_exec();
                }
                if (notif & PRT_NOTIF) { // Serve SW timer interrupt notification - update virtual cockpit with last printf string
                        print_timer_cb_exec();
                }
                if (notif & IGL_NOTIF) { // Serve SW timer interrupt notification
                        igla_deactivate_cb_exec();
                }
                if (notif & SPI_NOTIF) { // Serve MSC2518FD interrupt notification
                        do{
                                handle_spi_interrupt();
                                handle_spi2_interrupt();
                                if (dk_hw_k1_button_pressed()) { // Check if more interrupts are still pending, need to serve all in order for
                                                               // the pin get high and trigger again
                                        printf("key\r\n");
                                        while (dk_hw_k1_button_pressed());
                                }
                        }while(dk_hw_int2_detected()||dk_hw_int_detected());
                }

        }
}
/* $code_snippet END */

/**
 * @brief Initialize the peripherals domain after power-up.
 *
 */



static void periph_init(void)
{
        hw_gpio_configure(gpio_cfg);
}

/**
 * @brief Hardware Initialization
 */
static void prvSetupHardware( void )
{

        /* Init hardware */
        pm_system_init(periph_init);

}

/**
 * @brief Malloc fail hook
 */
void vApplicationMallocFailedHook( void )
{
        /* vApplicationMallocFailedHook() will only be called if
        configUSE_MALLOC_FAILED_HOOK is set to 1 in FreeRTOSConfig.h.  It is a hook
        function that will get called if a call to OS_MALLOC() fails.
        OS_MALLOC() is called internally by the kernel whenever a task, queue,
        timer or semaphore is created.  It is also called by various parts of the
        demo application.  If heap_1.c or heap_2.c are used, then the size of the
        heap available to OS_MALLOC() is defined by configTOTAL_HEAP_SIZE in
        FreeRTOSConfig.h, and the xPortGetFreeHeapSize() API function can be used
        to query the size of free heap space that remains (although it does not
        provide information on how the remaining heap might be fragmented). */
        ASSERT_ERROR(0);
}

/**
 * @brief Application idle task hook
 */
void vApplicationIdleHook( void )
{
        /* vApplicationIdleHook() will only be called if configUSE_IDLE_HOOK is set
        to 1 in FreeRTOSConfig.h.  It will be called on each iteration of the idle
        task.  It is essential that code added to this hook function never attempts
        to block in any way (for example, call OS_QUEUE_GET() with a block time
        specified, or call OS_DELAY()).  If the application makes use of the
        OS_TASK_DELETE() API function (as this demo application does) then it is also
        important that vApplicationIdleHook() is permitted to return to its calling
        function, because it is the responsibility of the idle task to clean up
        memory allocated by the kernel to any task that has since been deleted. */
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
        ASSERT_ERROR(0);
}

/**
 * @brief Application tick hook
 */
void vApplicationTickHook( void )
{
}


