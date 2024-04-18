/**
  ******************************************************************************
  * @file    at24cxx.c
  * @author  YANDLD
  * @version V3.0
  * @date    2013.12.25
  * @brief   www.beyondcore.net   http://upcmcu.taobao.com 
  ******************************************************************************
  */

#include <string.h>
#include "at24cxx.h"
#include "i2c.h"

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x)	(sizeof(x) / sizeof((x)[0]))
#endif


#define PAGE_SIZE       (8)

struct eep_device 
{
    uint8_t     addr;
    uint8_t     bus_instance;
};

static const uint8_t eep_addr[] = {0x50};
static struct eep_device eep_dev;


int eep_init(uint32_t instance)
{
    uint32_t i;
    
    eep_dev.bus_instance = instance;

    for(i=0;i<ARRAY_SIZE(eep_addr);i++)
    {
        if(!I2C_Probe(instance, eep_addr[i]))
        {
            eep_dev.addr = eep_addr[i];
            return EEP_OK;
        }
    }
    return EEP_ERROR;
}


int eep_read(uint32_t addr, uint8_t *buf, uint32_t len)
{
    uint8_t chip_addr;
    
    chip_addr = (addr/256) + eep_dev.addr;
    return I2C_BurstRead(eep_dev.bus_instance, chip_addr, addr%256, 1, buf, len);
}

int eep_write_page(uint32_t addr, uint8_t *buf, uint32_t len)
{
    int ret;
    uint8_t chip_addr;
    
    chip_addr = (addr/256) + eep_dev.addr;
    ret = I2C_BurstWrite(eep_dev.bus_instance, chip_addr, addr%256, 1, buf, len);
    while(I2C_Probe(eep_dev.bus_instance, eep_dev.addr));
    return ret;
}

//int eep_write(uint32_t addr, uint8_t *buf, uint32_t len)
//{
//    int ret;
//	uint32_t secpos;
//	uint32_t secoff;
//	uint16_t Byteremian;
//	uint32_t pageremain;
//    uint32_t page_size = PAGE_SIZE;
//	secpos = addr/page_size;
//	secoff = addr%page_size;
//	pageremain = page_size - secoff;

//    ret = eep_write_page(addr, buf, len);
//    if(ret)
//    {
//        return ret;
//    }
//    while(I2C_Probe(eep_dev.bus_instance, eep_dev.addr));
//	addr += pageremain;
//	secpos++;
//	Byteremian = len - pageremain;
//	while(Byteremian != 0)
//	{
//		if(Byteremian <= page_size)
//		{
//			 pageremain = Byteremian;
//		}
//		else
//		{
//			pageremain = page_size;
//		}
//        ret = eep_write_page(secpos*page_size, buf, pageremain);
//        if(ret)
//        {
//            return ret;
//        }
//        while(I2C_Probe(eep_dev.bus_instance, eep_dev.addr));
//        secpos++;
//        buf += pageremain;
//        Byteremian -= pageremain;
//	}
//    return EEP_OK;
//}


int eep_self_test(uint32_t begin, uint32_t end)
{
    int i,j;
    uint8_t buf[PAGE_SIZE];

    for(i=0; i<(end - begin)/PAGE_SIZE; i++)
    {
        for(j=0; j<sizeof(buf);j++)
        {
            buf[j] = j&0xFF;
        }
        printf("eep write 0x%X...", i*PAGE_SIZE);
        eep_write_page(i*PAGE_SIZE, buf, 8);
        memset(buf, 0, sizeof(buf));
        eep_read(i*PAGE_SIZE, buf, sizeof(buf));
        printf("varify...");
        for(j=0; j<sizeof(buf); j++)
        {
            if(buf[j] != j%0xFF)
            {
                //printf("index:%d 0x%X err\r\n", j, buf[j]);
                return EEP_ERROR;
            }
        }
        printf("ok!\r\n");
    }

    return EEP_OK;
}







