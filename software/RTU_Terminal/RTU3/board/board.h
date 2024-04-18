/*
 * File      : board.h
 * This file is part of RT-Thread RTOS
 * COPYRIGHT (C) 2009, RT-Thread Development Team
 *
 * The license and distribution terms for this file may be
 * found in the file LICENSE in this distribution or at
 * http://www.rt-thread.org/license/LICENSE
 *
 * Change Logs:
 * Date           Author       Notes
 *
 */

// <<< Use Configuration Wizard in Context Menu >>>
#ifndef __BOARD_H__
#define __BOARD_H__

#define REG_TEST_OVER_TIME      (90*24*60*60)    //单位:秒

#define HW_VER_VERCODE  403

#define SW_VER_MAJOR (3)	//主版本号(1-255)
#define SW_VER_MINOR (0)	//次版本号(0-255)
#define SW_VER_VERCODE (SW_VER_MAJOR * 100 + SW_VER_MINOR)

#define PRODUCT_MODEL       "ST-8803-E3"

#define HTTP_SERVER_HOST    "http://api.fitepc.cn"
#define _STR(_str)          (_str)?(_str):""
#define HEX_CH_TO_NUM(c)	(c<='9'?(c-'0'):(c-'A'+10))

#define NET_HAS_GPRS        (1)

#define USE_DEV_BSP         0

#define USR_TEST_CODE
#define SX1278_TEST (0)

#include "threaddog.h"

#define rt_thddog_register(_rt, _sec)   _rt->user_data = (rt_uint32_t)threaddog_register(_rt->name, _sec, (const void *)_rt)
#define rt_thddog_unregister(_rt)       threaddog_unregister((thddog_t *)_rt->user_data)
#define rt_thddog_unreg_inthd()         rt_thddog_unregister(rt_thread_self())
#define rt_thddog_feed(_desc)           threaddog_feed((thddog_t *)rt_thread_self()->user_data, _desc)
#define rt_thddog_suspend(_desc)        threaddog_suspend((thddog_t *)rt_thread_self()->user_data, _desc)
#define rt_thddog_resume()              threaddog_resume((thddog_t *)rt_thread_self()->user_data, RT_NULL)
#define rt_thddog_exit()                threaddog_exit((thddog_t *)rt_thread_self()->user_data, RT_NULL)

#include <time.h>
#include <string.h>

#include <rtdef.h>
#include <common.h>
#include <MK64F12.h>
#include <queue.h>
#include <rtcrc32.h>

#include <rtu_reg.h>

//
#ifdef RT_USING_LWIP
#include <lwip/sys.h>
#include <lwip/api.h>
#include <netif/ethernetif.h>
#include <netdb.h>
#endif

#include <board_cfg.h>

#include <elog.h>

//
#include <zigbee.h>
#include <zgb_std.h>
#include <gprs_helper.h>
#include <ad7689.h>
#include <in_out.h>
#include <varmanage.h>
#include <protomanage.h>
#include <cJSON.h>

//#include <webnet.h>
#include <module.h>
#include <board_udp.h>
#include <host_cfg.h>
#include <uart_cfg.h>
#include <auth_cfg.h>
#include <dev_cfg.h>
#include <net_cfg.h>
#include <tcpip_cfg.h>
#include <analog_cfg.h>
#include <gprs_cfg.h>
#include <dido_cfg.h>
#include <zigbee_cfg.h>
#include <xfer_cfg.h>
#include <xfer_helper.h>
#include <ws_cfg.h>
#include <ws_vm.h>

#include <cpu_usage.h>
#include <storage.h>

//
#include <rtt_drv.h>
#include <chlib_k.h>
#include <uart.h>
#include <sd.h>
#include <spi.h>
#include <gpio.h>
#include <rtc.h>
#include <enet_phy.h>
#include <rtdevice.h>
#include <components.h>

#include <mb.h>

#include <dfs_posix.h>

//
#include <user_mb_app.h>

#include <spi_flash_w25qxx.h>

#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
#include "common.h"

#include "cc_bjdc.h"

#include "iniparser.h"
#include "exp_parser.h"

#include "httpclient.h"

/* board configuration */

// <o> Internal SRAM memory size[Kbytes]
#define K64_SRAM_SIZE                   256
#define K64_SRAM_END                    (0x1FFF0000 + (K64_SRAM_SIZE * 1024))

#define BOARD_UART_DEV_NAME(n)          ((n) == HW_UART0 ? "uart0" : \
                                        ((n) == HW_UART1 ? "uart1" : \
                                        ((n) == HW_UART2 ? "uart2" : \
                                        ((n) == HW_UART3 ? "uart3" : \
                                        ((n) == HW_UART4 ? "uart4" : \
                                        ((n) == HW_UART5 ? "uart5" : "" ))))))

#define BOARD_SPI_DEV_NAME(n)          ((n) == HW_SPI0 ? "spi0" : \
                                        ((n) == HW_SPI1 ? "spi1" : \
                                        ((n) == HW_SPI2 ? "spi2" : "" )))

#define RT_PIN_NONE                     (0xFFFF)
#define RT_PIN(_hw,_pin)                (((_hw)<<8)|(_pin))

//--------------------UART----------------------

#define BOARD_CREAT_NAME(_s_,_fmt,...) char _s_[RT_NAME_MAX+1]; snprintf( _s_, RT_NAME_MAX+1, _fmt, __VA_ARGS__ );

// 232 or 485 or gprs
#if USE_DEV_BSP
#define BOARD_UART_0_MAP            UART0_RX_PD06_TX_PD07
#else
#define BOARD_UART_0_MAP            UART0_RX_PB16_TX_PB17
#endif
#define BOARD_UART_0_BAUDRATE       BAUD_RATE_115200
#define BOARD_UART_0_TYPE           UART_TYPE_232
#define BOARD_UART_0_EN             RT_PIN(HW_GPIOC, 13)

// 232 or 485
#define BOARD_UART_1_MAP            UART1_RX_PC03_TX_PC04
#define BOARD_UART_1_BAUDRATE       BAUD_RATE_115200
#define BOARD_UART_1_TYPE           UART_TYPE_232
#define BOARD_UART_1_EN             RT_PIN(HW_GPIOC, 2)


#define BOARD_PTC4_GPIO            RT_PIN(HW_GPIOC, 4)
#define BOARD_PTC15_GPIO             RT_PIN(HW_GPIOC, 15)

// GPRS
#define BOARD_UART_2_MAP            UART2_RX_PD02_TX_PD03
#define BOARD_UART_2_BAUDRATE       BAUD_RATE_115200
#define BOARD_UART_2_TYPE           UART_TYPE_GPRS
#define BOARD_UART_2_EN             RT_PIN_NONE
#define BOARD_GPRS_UART             HW_UART2

// zigbee
#define BOARD_UART_3_MAP            UART3_RX_PB10_TX_PB11
#define BOARD_UART_3_BAUDRATE       BAUD_RATE_115200
#define BOARD_UART_3_TYPE           UART_TYPE_ZIGBEE
#define BOARD_UART_3_EN             RT_PIN_NONE
#define BOARD_ZIGBEE_UART           HW_UART3


// 232 or 485
#define BOARD_UART_4_MAP            UART4_RX_PC14_TX_PC15
#define BOARD_UART_4_BAUDRATE       BAUD_RATE_115200
#define BOARD_UART_4_TYPE           UART_TYPE_232
#define BOARD_UART_4_EN             RT_PIN(HW_GPIOC, 1)

// 232 or 485
#define BOARD_UART_5_MAP            UART5_RX_PD08_TX_PD09
#define BOARD_UART_5_BAUDRATE       BAUD_RATE_115200
#define BOARD_UART_5_TYPE           UART_TYPE_232
#define BOARD_UART_5_EN             RT_PIN(HW_GPIOD, 10)

//--------------------SD----------------------
#define BOARD_SDCARD_NAME               "sdcard"
#define BOARD_SDCARD_MOUNT              "/sd"

//--------------------SPI----------------------
#define BOARD_SPI_DRIVER_DEBUG          0
#define BOARD_SPI_MAX                   3

#define BOARD_SPIFLASH_FLASH_NAME       "spiflash"
#define BOARD_SPIFLASH_FLASH_MOUNT      "/"
#define BOARD_SPIFLASH_BASE             SPI2_BASE
#define BOARD_SPIFLASH_INSTANCE         HW_SPI2
#if USE_DEV_BSP
#define BOARD_SPIFLASH_MAP              SPI2_SCK_PD12_SOUT_PD13_SIN_PD14
#define BOARD_SPIFLASH_CSN              HW_SPI_CS1
#define BOARD_SPIFLASH_CSNCFG           {HW_GPIOD, 15, kPinAlt2}
#else
#define BOARD_SPIFLASH_MAP              SPI2_SCK_PB21_SOUT_PB22_SIN_PB23
#define BOARD_SPIFLASH_CSN              HW_SPI_CS0
#define BOARD_SPIFLASH_CSNCFG           {HW_GPIOB, 20, kPinAlt2}
#endif

//-------------------GPIO--------------------
#define BOARD_GPIO_DEV_NAME             "gpio"

// power
#define BOARD_GPIO_POWER_SHUTDOWN       RT_PIN(HW_GPIOA, 28)

//gprs
#define BOARD_GPIO_GPRS_POWER_EN        RT_PIN(HW_GPIOB, 2)

#define BOARD_GPIO_GPRS_TERM_ON         RT_PIN(HW_GPIOE, 8)
#define BOARD_GPIO_GPRS_RST             RT_PIN(HW_GPIOE, 9)

// zigbee
#define BOARD_GPIO_ZIGBEE_RESET         RT_PIN(HW_GPIOE, 11)
#define BOARD_GPIO_ZIGBEE_LINK          RT_PIN(HW_GPIOE, 10)

#define BOARD_GPIO_ZIGBEE_SLEEP          RT_PIN(HW_GPIOB, 6)

// net
#define BOARD_GPIO_NET_RESET            RT_PIN(HW_GPIOA, 2)
#define BOARD_GPIO_NET_LED              RT_PIN(HW_GPIOA, 1)

// sd
#define BOARD_UART_SWITCH                RT_PIN(HW_GPIOE, 6)


#define BOARD_GPIO_SD_TEST              RT_PIN(HW_GPIOE, 7)

//ADC INPUT 8路ADC通道
#define BOARD_GPIO_ADC_RESET  RT_PIN(HW_GPIOB, 19)
#define BOARD_GPIO_ADC_SCK    RT_PIN(HW_GPIOB, 9)
#define BOARD_GPIO_ADC_SOUT   RT_PIN(HW_GPIOB, 8)
#define BOARD_GPIO_ADC_SIN    RT_PIN(HW_GPIOB, 18)
//#define BOARD_GPIO_ADC_DRDY   RT_PIN(HW_GPIOB, 7)

#define BOARD_GPIO_ADC_POWER   RT_PIN(HW_GPIOE, 12)


//继电器输出

#define BOARD_GPIO_RELAYS_OUT1   RT_PIN(HW_GPIOE, 27)
#define BOARD_GPIO_RELAYS_OUT2   RT_PIN(HW_GPIOE, 28)
#define BOARD_GPIO_RELAYS_OUT3   RT_PIN(HW_GPIOD, 0)
#define BOARD_GPIO_RELAYS_OUT4    RT_PIN(HW_GPIOA, 19)
#define BOARD_GPIO_WDOG   RT_PIN(HW_GPIOA, 6)
//#define HARDWARE_WDOG_FEED()  { rt_pin_write( BOARD_GPIO_WDOG, PIN_HIGH ); delayUsNop(100); rt_pin_write( BOARD_GPIO_WDOG, PIN_LOW ); rt_pin_toggle(BOARD_GPIO_WDOG)}
#define HARDWARE_WDOG_FEED()  {rt_pin_toggle(BOARD_GPIO_WDOG);}

//TTL输出
#define BOARD_GPIO_TTL_OUT1   RT_PIN(HW_GPIOE, 24)
#define BOARD_GPIO_TTL_OUT2   RT_PIN(HW_GPIOE, 25)

#define WDOG_FEED() { \
    rt_base_t _lvl_ = rt_hw_interrupt_disable(); \
    WDOG->REFRESH = 0xA602u;WDOG->REFRESH = 0xB480u;\
    HARDWARE_WDOG_FEED(); \
    rt_hw_interrupt_enable(_lvl_); \
}


//TTL输入
#define BOARD_GPIO_TTL_INPUT1   RT_PIN(HW_GPIOD, 12)
#define BOARD_GPIO_TTL_INPUT2   RT_PIN(HW_GPIOD, 13)
#define BOARD_GPIO_TTL_INPUT3   RT_PIN(HW_GPIOD, 14)
#define BOARD_GPIO_TTL_INPUT4   RT_PIN(HW_GPIOD, 15)

//74HC4051管脚配置

#define BOARD_GPIO_74HC4051_S0   RT_PIN(HW_GPIOA, 7)
#define BOARD_GPIO_74HC4051_S1   RT_PIN(HW_GPIOA, 8)
#define BOARD_GPIO_74HC4051_S2   RT_PIN(HW_GPIOA, 9)
#define BOARD_GPIO_74HC4051_E    RT_PIN(HW_GPIOA, 10)

//通断模块输入
#define BOARD_GPIO_ONOFF_INPUT1   RT_PIN(HW_GPIOA, 11)
#define BOARD_GPIO_ONOFF_INPUT2   RT_PIN(HW_GPIOA, 24)
#define BOARD_GPIO_ONOFF_INPUT3   RT_PIN(HW_GPIOA, 25)
#define BOARD_GPIO_ONOFF_INPUT4   RT_PIN(HW_GPIOA, 26)

//lora 433模块

#define BOARD_SX_RESET  RT_PIN(HW_GPIOA, 27)     //复位
#define BOARD_SX_SDN    RT_PIN(HW_GPIOA, 28)    //正常收发为高电平，休眠时为低电平
#define BOARD_SX_DIDO    RT_PIN(HW_GPIOA, 29)

#define BOARD_SX_PCS0   RT_PIN(HW_GPIOD, 7)
#define BOARD_SX_SCK    RT_PIN(HW_GPIOC, 19)
#define BOARD_SX_SOUT   RT_PIN(HW_GPIOC, 17)
#define BOARD_SX_SIN    RT_PIN(HW_GPIOC, 18)


#if USE_DEV_BSP
#define BOARD_CONSOLE_USART             HW_UART0
#else
#define BOARD_CONSOLE_USART             HW_UART1
#endif
#define BOARD_CONSOLE_BAUDRATE          BAUD_RATE_115200

#include "uart_cfg.h"

typedef struct
{
    uart_type_e eUart0Type;
    uart_type_e eUart1Type;
    uart_type_e eUart4Type;
    uart_type_e eUart5Type;
}s_Rs232_Rs485_Stat;

#define UART1_485_DE_ENABLE()   rt_pin_write( BOARD_UART_1_EN, PIN_LOW)
#define UART1_485_DE_DISABLE()  rt_pin_write( BOARD_UART_1_EN, PIN_HIGH)

#define UART0_485_DE_ENABLE()  rt_pin_write( BOARD_UART_0_EN, PIN_LOW)
#define UART0_485_DE_DISABLE() rt_pin_write( BOARD_UART_0_EN, PIN_HIGH)

#define UART5_485_DE_ENABLE()  rt_pin_write( BOARD_UART_5_EN, PIN_LOW)
#define UART5_485_DE_DISABLE() rt_pin_write( BOARD_UART_5_EN, PIN_HIGH)

#define UART4_485_DE_ENABLE()  rt_pin_write( BOARD_UART_4_EN, PIN_LOW)
#define UART4_485_DE_DISABLE() rt_pin_write( BOARD_UART_4_EN, PIN_HIGH)

void vTestRs232Rs485(s_Rs232_Rs485_Stat *pState); //检测是485还是232.串口顺序已经和丝印对应上的。0表示232 1表示485

void rt_hw_board_init(void);

#define DEF_CGI_HANDLER(_func)      void _func( struct webnet_session * session )
#define CGI_GET_ARG(_tag)           webnet_request_get_query( session->request, _tag )
#define WEBS_PRINTF(_fmt,...)       webnet_session_printf( session, _fmt, ##__VA_ARGS__ )
#define WEBS_DONE(_code)            session->request->result_code = (_code)

/*
#define DEF_CGI_HANDLER(_func)      void _func(webs_t wp, char_t *path, char_t *query)
#define CGI_GET_ARG(_tag)           websGetVar(wp, T(_tag), "" )
#define WEBS_PRINTF(_fmt,...)       websWrite(wp, T(_fmt), ##__VA_ARGS__)
#define WEBS_DONE(_code)            websDone(wp, _code)
*/

extern void lua_rtu_doexp(char *exp);

void vDoSystemReset(void);

#endif

// <<< Use Configuration Wizard in Context Menu >>>
