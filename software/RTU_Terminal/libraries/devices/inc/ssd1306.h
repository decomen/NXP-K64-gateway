/**
  ******************************************************************************
  * @file    ssd1306.h
  * @author  YANDLD
  * @date    2015.12.25
  * @brief   www.beyondcore.net   http://upcmcu.taobao.com 
  ******************************************************************************
  */

#ifndef __SSD1306__
#define __SSD1306__

#include <stdint.h>


//API funtctions
void ssd1306_init(void);
void OLED_Fill(unsigned char dat);
void OLED_Enable(void);
void OLED_Disable(void);
void OLED_ShowStr(unsigned char x, unsigned char y, char *str);
void OLED_DrawBMP(unsigned char x0,unsigned char y0,unsigned char x1,unsigned char y1,unsigned char BMP[]);
void OLED_ShowCN(unsigned char x, unsigned char y, unsigned char N);


#endif

