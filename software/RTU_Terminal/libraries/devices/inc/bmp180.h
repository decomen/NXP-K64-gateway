/**
  ******************************************************************************
  * @file    bmp180.h
  * @author  YANDLD
  * @version V2.5
  * @date    2013.12.25
  * @brief   www.beyondcore.net   http://upcmcu.taobao.com 
  ******************************************************************************
  */
#ifndef __BMP180_H__
#define __BMP180_H__

#include <stdint.h>


//!< API functions
int bmp180_init(uint32_t instance);
float bmp180_get_altitude(int32_t pressure);
int bmp180_conversion_process(float *pressure, float *temperature);
#endif

