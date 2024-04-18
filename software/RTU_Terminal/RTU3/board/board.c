/*
 * File      : board.c
 * This file is part of RT-Thread RTOS
 * COPYRIGHT (C) 2009 RT-Thread Develop Team
 *
 * The license and distribution terms for this file may be
 * found in the file LICENSE in this distribution or at
 * http://www.rt-thread.org/license/LICENSE
 *
 * Change Logs:
 * Date           Author       Notes
 * 
 */

#include <rthw.h>
#include <rtthread.h>

#include "board.h"
#include "rtt_drv.h"
#include "radio.h"

/**
 * @addtogroup K64
 */

/*******************************************************************************
* Function Name  : NVIC_Configuration
* Description    : Configures Vector Table base location.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void NVIC_Configuration(void) {

}

/*******************************************************************************
 * Function Name  : SysTick_Configuration
 * Description    : Configures the SysTick for OS tick.
 * Input          : None
 * Output         : None
 * Return         : None
 *******************************************************************************/
void  SysTick_Configuration(void) {
    SystemCoreClockUpdate();              /* Update Core Clock Frequency        */
    SysTick_Config(SystemCoreClock / RT_TICK_PER_SECOND); /* Generate interrupt each 1 ms       */
}

/**
 * This is the timer interrupt service routine.
 *
 */
void SysTick_Handler(void) {
    /* enter interrupt */
    rt_interrupt_enter();

    rt_tick_increase();

    /* leave interrupt */
    rt_interrupt_leave();
}

/**
 * This function will initial Tower board.
 */

s_Rs232_Rs485_Stat gUartType;

void rt_hw_board_init() {
    /* NVIC Configuration */
    NVIC_Configuration();

    /* Configure the SysTick */
    SysTick_Configuration();

    DelayInit();

    threaddog_init();

    /*开启开门狗*/
    /* WDOG_InitTypeDef WDOG_InitStruct1;  
     WDOG_InitStruct1.windowInMs = 0;    
     WDOG_InitStruct1.mode = kWDOG_Mode_Normal;  
     WDOG_InitStruct1.timeOutInMs = 20000;  
     WDOG_Init(&WDOG_InitStruct1);   */

    /* power up */
#ifdef RT_USING_PIN
    rt_hw_pin_init();

    rt_pin_mode(BOARD_GPIO_POWER_SHUTDOWN, PIN_MODE_OUTPUT);
    rt_pin_write(BOARD_GPIO_POWER_SHUTDOWN, PIN_HIGH);
#endif

/*
     sx_init();
     sx_reset();
     unsigned char reg = 0;
     SXReadReg(0x02, &reg);
     reg = 0;
*/

	rt_pin_mode(BOARD_GPIO_WDOG, PIN_MODE_OUTPUT);
	HARDWARE_WDOG_FEED();

	rt_pin_mode(BOARD_PTC4_GPIO, PIN_MODE_OUTPUT);
        rt_pin_mode(BOARD_PTC15_GPIO, PIN_MODE_OUTPUT);
	rt_pin_write(BOARD_PTC4_GPIO,PIN_HIGH);
	rt_pin_write(BOARD_PTC15_GPIO,PIN_HIGH);

	

	vTestRs232Rs485(&gUartType);

#ifdef RT_USING_RTC
    rt_hw_rtc_init();
#endif

#ifdef RT_USING_SERIAL
#if BOARD_UART_0_MAP != RT_NULL
    rt_hw_uart_init(HW_UART0);
#endif
#if BOARD_UART_1_MAP != RT_NULL
    rt_hw_uart_init(HW_UART1);
#endif
#if BOARD_UART_2_MAP != RT_NULL
    rt_hw_uart_init(HW_UART2);
#endif
#if BOARD_UART_3_MAP != RT_NULL
    rt_hw_uart_init(HW_UART3);
#endif
#if BOARD_UART_4_MAP != RT_NULL
    rt_hw_uart_init(HW_UART4);
#endif
#if BOARD_UART_5_MAP != RT_NULL
    rt_hw_uart_init(HW_UART5);
#endif

   

   // gUartType.eUart0Type = UART_TYPE_485;
   // gUartType.eUart1Type = UART_TYPE_485;
	
    //gUartType.eUart4Type = UART_TYPE_485;
    //gUartType.eUart5Type = UART_TYPE_485;

	

#ifdef RT_USING_CONSOLE
    rt_console_set_device(BOARD_UART_DEV_NAME(BOARD_CONSOLE_USART));
    rt_kprintf("\n\n------------------version : %d.%d ---------------\n", SW_VER_MAJOR, SW_VER_MINOR);
#endif


#endif

    // vTestRs232Rs485(&gUartType);
     


    if (gUartType.eUart1Type == UART_TYPE_232) {
        rt_kprintf(" uart 1 232\r\n"); 
    } else {
        rt_kprintf(" uart 1 485\r\n");
    }
	

    if (gUartType.eUart4Type == UART_TYPE_232) {
        rt_kprintf(" uart 4 232\r\n");
    } else {
        rt_kprintf(" uart 4 485\r\n");
    }

    if (gUartType.eUart5Type == UART_TYPE_232) {
        rt_kprintf(" uart 5 232\r\n");
    } else {
        rt_kprintf(" uart 5 485\r\n");
    }

    if (gUartType.eUart0Type == UART_TYPE_232) {
        rt_kprintf(" uart 0 232\r\n");
    } else {
        rt_kprintf(" uart 0 485\r\n");
    }
   
   
    vAd7689Init();

	/*rt_pin_mode(BOARD_GPIO_ADC_SOUT, PIN_MODE_OUTPUT); 
	while(1){
		    rt_pin_toggle( BOARD_GPIO_ADC_RESET);
			 rt_pin_toggle( BOARD_GPIO_ADC_SIN);
			 rt_pin_toggle( BOARD_GPIO_ADC_SCK);
			 rt_pin_toggle( BOARD_GPIO_ADC_SOUT);
			 delayUsNop(1000);
	}
*/
    vInOutInit();
    /*开启开门狗*/
#if 1
    WDOG_InitTypeDef WDOG_InitStruct1;
    WDOG_InitStruct1.windowInMs = 0;
    WDOG_InitStruct1.mode = kWDOG_Mode_Normal;  //设置看门狗处于正常工作模式
    WDOG_InitStruct1.timeOutInMs = 10000; /* 时限 2000MS : 2000MS 内没有喂狗则复位 */
    WDOG_Init(&WDOG_InitStruct1);
    WDOG_FEED();
#endif


#if  0

    rt_pin_mode( BOARD_GPIO_SD_TEST, PIN_MODE_INPUT );

    while(1){

        if(rt_pin_read( BOARD_GPIO_SD_TEST) == PIN_LOW){

        } else {

        }
    }


#endif



}



void vTestRs232Rs485(s_Rs232_Rs485_Stat *pState) 
{
   
 
    rt_pin_mode(BOARD_UART_1_EN, PIN_MODE_INPUT_PULLUP);
    rt_pin_mode(BOARD_UART_4_EN, PIN_MODE_INPUT_PULLUP);
    rt_pin_mode(BOARD_UART_5_EN, PIN_MODE_INPUT_PULLUP);
	rt_pin_mode(BOARD_UART_0_EN, PIN_MODE_INPUT_PULLUP);
    
    
    delayUsNop(200000);
    if (rt_pin_read(BOARD_UART_1_EN) == PIN_LOW) {
         pState->eUart1Type = UART_TYPE_485;
    } else {
        pState->eUart1Type = UART_TYPE_232;
    }
    delayUsNop(50000);
    if (rt_pin_read(BOARD_UART_4_EN) == PIN_LOW) {
       pState->eUart4Type = UART_TYPE_485;
    } else {
          pState->eUart4Type = UART_TYPE_232;
    }
	
    delayUsNop(50000);
    if (rt_pin_read(BOARD_UART_5_EN) == PIN_LOW) {
        pState->eUart5Type = UART_TYPE_232;
    } else {
        pState->eUart5Type = UART_TYPE_485;
    }
    delayUsNop(50000);
    if (rt_pin_read(BOARD_UART_0_EN) == PIN_LOW) {
        pState->eUart0Type = UART_TYPE_232;
    } else {
        pState->eUart0Type = UART_TYPE_485;
    }

	// delayUsNop(100000);
  
}


