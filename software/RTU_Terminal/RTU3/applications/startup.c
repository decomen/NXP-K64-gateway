/*
 * File      : startup.c
 * This file is part of RT-Thread RTOS
 * COPYRIGHT (C) 2006, RT-Thread Develop Team
 *
 * The license and distribution terms for this file may be
 * found in the file LICENSE in this distribution or at
 * http://openlab.rt-thread.com/license/LICENSE
 *
 * Change Logs:
 * Date           Author       Notes
 * 
 */

#include <rthw.h>
#include <rtthread.h>

#include <board.h>
#include "common.h"

/**
 * @addtogroup k64
 */

/*@{*/

extern int  rt_application_init(void);

#ifdef __CC_ARM
extern int Image$$RW_IRAM1$$ZI$$Limit;
#define K64_SRAM_BEGIN    (&Image$$RW_IRAM1$$ZI$$Limit)
#elif __ICCARM__
#pragma section="HEAP"
#define K64_SRAM_BEGIN    (__segment_end("HEAP"))
#else
extern int __bss_end;
#define K64_SRAM_BEGIN    (&__bss_end)
#endif

/*******************************************************************************
* Function Name  : assert_failed
* Description    : Reports the name of the source file and the source line number
*                  where the assert error has occurred.
* Input          : - file: pointer to the source file name
*                  - line: assert error line source number
* Output         : None
* Return         : None
*******************************************************************************/
void assert_failed(rt_uint8_t* file, rt_uint32_t line)
{
    rt_kprintf("\n\r Wrong parameter value detected on\r\n");
    rt_kprintf("       file  %s\r\n", file);
    rt_kprintf("       line  %d\r\n", line);

    while (1) ;
}

/**
 * This function will startup RT-Thread RTOS.
 */


void rtthread_startup(void)
{
    /* init board */
    rt_hw_board_init();

    WDOG_FEED();
	//{ rt_pin_write( BOARD_GPIO_WDOG, PIN_HIGH ); delayUsNop(100); rt_pin_write( BOARD_GPIO_WDOG, PIN_LOW );}

    /* show version */
    rt_show_version();  


/*	
	 rt_pin_mode(  RT_PIN(HW_GPIOD, 1), PIN_MODE_OUTPUT );
         rt_pin_write(RT_PIN(HW_GPIOD, 1), PIN_HIGH );
	 DelayMs(1);
         rt_pin_write(RT_PIN(HW_GPIOD, 1), PIN_LOW);
	 DelayMs(1);
         rt_pin_write(RT_PIN(HW_GPIOD, 1), PIN_HIGH );

	 while(1);

*/

    /* init tick */
    rt_system_tick_init();

    /* init kernel object */
    rt_system_object_init();

    /* init timer system */
    rt_system_timer_init();

    rt_system_heap_init((void*)K64_SRAM_BEGIN, (void*)K64_SRAM_END);

    /* init scheduler system */
    rt_system_scheduler_init();

    WDOG_FEED();
   //{ rt_pin_write( BOARD_GPIO_WDOG, PIN_HIGH ); delayUsNop(100); rt_pin_write( BOARD_GPIO_WDOG, PIN_LOW );}

    /* init all device */
    rt_device_init_all();

    /* init application */
    rt_application_init();

    /* init timer thread */
    rt_system_timer_thread_init();

    /* init idle thread */
    rt_thread_idle_init();

    /* start scheduler */
    rt_system_scheduler_start();

    /* never reach here */
    return ;
}

int main(void)
{
#define RELOCATED_VECTORS (0xA000)
    SCB->VTOR = RELOCATED_VECTORS;

    /* disable interrupt first */
    rt_hw_interrupt_disable();

	//DelayMs(1000);
    
    /* startup RT-Thread RTOS */
    rtthread_startup();

    return 0;
}

/*@}*/
