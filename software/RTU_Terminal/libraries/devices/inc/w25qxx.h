/**
  ******************************************************************************
  * @file    w25qxx.h
  * @author  YANDLD
  * @version V3.0
  * @date    2015.10.8
  * @brief   www.beyondcore.net   http://upcmcu.taobao.com 
  ******************************************************************************
  */
  
#ifndef __W25QXX_H__
#define __W25QXX_H__

#include <stdint.h>

//?¨´¡À?????
// page :256byte D¡ä¨¨?¦Ì?¡Á?D?¦Ì£¤?? 2¡é?¨°¡À?D?¡À¡ê?¡è D¡ä¨¨??¡ã¨¨?2?¨º?0xFF
// sectoer: 4KBbyte
// block:   64KBbyte


#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x)	(sizeof(x) / sizeof((x)[0]))
#endif

#define W25QXX_SECTOR_SIZE  (4096)


struct w25qxx_init_t
{
    uint32_t (*xfer)(uint8_t *buf_in, uint8_t *buf_out, uint32_t len, uint8_t cs_state);
    uint32_t (*get_reamin)(void);
    void     (*delayms)(uint32_t ms);
};

//!< API functions
int w25qxx_init(struct w25qxx_init_t* init);
int w25qxx_read(uint32_t addr, uint8_t *buf, uint32_t len);
int w25qxx_erase_sector(uint32_t addr);
int w25qxx_write_sector(uint32_t addr, uint8_t *buf, uint32_t len);
int w25qxx_erase_chip(void);
int w25qxx_get_id(void);

#endif 


