/**
  ******************************************************************************
  * @file    ili9163.c
  * @author  YANDLD
  * @version V2.5
  * @date    2013.12.25
  * @brief   www.beyondcore.net   http://upcmcu.taobao.com 
  * @note    ���ļ�ΪNRF24L01����ģ���������֧��̨����Ų����оƬ
  ******************************************************************************
  */
  
  
  
#ifndef __ILI9163_H
#define __ILI9163_H	 		  

#include <stdint.h>
#include "spi.h"
#include "gpio.h"


//!< API functions
int ili9163_init(uint32_t instance, uint32_t cs);
void ili9163_clear(uint16_t c);
void ili9163_write_pixel(int x, int y, int c);
void GUI_DrawPixel(int x, int y);

#endif


