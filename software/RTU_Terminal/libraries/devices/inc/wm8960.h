/**
  ******************************************************************************
  * @file    wm8960.h
  * @author  YANDLD
  * @version V2.6
  * @date    2015.07.23
  * @brief   www.beyondcore.net   http://upcmcu.taobao.com 
  ******************************************************************************
  */
  
#ifndef __WM8960_H__
#define __WM8960_H__

#include <stdint.h>
#include <stdbool.h>


/*! @brief Modules in WM8960 board. */
typedef enum _WM8960_module
{
    kWolfsonModuleADC = 0x0,
    kWolfsonModuleDAC = 0x1,
    kWolfsonModuleVREF = 0x2,
	kWolfsonModuleHP = 0x03,
    kWolfsonModuleLineIn = 0x6,
    kWolfsonModuleLineOut = 0x7,
	kWolfsonModuleSpeaker = 0x8
} wolfson_module_t;




//!< API function
int wm8960_init(uint32_t instance);
int wm8960_set_volume(wolfson_module_t module, uint32_t volume);
int wm8960_format_config(uint32_t sample_rate, uint8_t bits);



#endif
