/**
  ******************************************************************************
  * @file    at24cxx.h
  * @author  YANDLD
  * @version V3.0
  * @date    2013.12.25
  * @brief   www.beyondcore.net   http://upcmcu.taobao.com 
  ******************************************************************************
  */
#ifndef __24CXX_H__
#define __24CXX_H__

#include <stdint.h>


#define EEP_OK              (0x00)
#define EEP_ERROR           (0x01)

//API funtctions
int eep_init(uint32_t instance);
int eep_read(uint32_t addr, uint8_t *buf, uint32_t len);
int eep_write_page(uint32_t addr, uint8_t *buf, uint32_t len);
int eep_self_test(uint32_t begin, uint32_t end);

#endif

