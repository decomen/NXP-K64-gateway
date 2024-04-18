/**
  ******************************************************************************
  * @file    nrf24l01.h
  * @author  YANDLD
  * @version V2.5
  * @date    2013.12.25
  * @brief   www.beyondcore.net   http://upcmcu.taobao.com 
  * @note    此文件为NRF24L01无线模块的驱动，支持台产和挪威产芯片
  ******************************************************************************
  */
#ifndef __NRF24L01_H
#define __NRF24L01_H	 		  

#include <stdint.h>
#include "spi.h"
#include "common.h"


struct nrf24xx_ops_t
{
    uint32_t (*xfer)(uint8_t *buf_in, uint8_t *buf_out, uint32_t len, uint8_t cs_stat);
    uint32_t (*get_reamin)(void);
    void     (*ce_control)(uint8_t stat);
    void     (*delayms)(uint32_t ms);
};



//<! API functions
void nrf24l01_set_tx_mode(void);
void nrf24l01_set_rx_mode(void);
int nrf24l01_write_packet(uint8_t *buf, uint32_t len);
int nrf24l01_read_packet(uint8_t *buf, uint32_t *len);
int nrf24l01_probe(void);
int nrf24l01_init(const struct nrf24xx_ops_t *ops);
void nrf24l01_set_tx_addr(uint8_t *buf);
void nrf24l01_set_rx_addr(uint8_t ch, uint8_t *addr);

#endif











