/**
  ******************************************************************************
  * @file    wm8960.c
  * @author  YANDLD
  * @version V2.6
  * @date    2015.07.23
  * @brief   www.beyondcore.net   http://upcmcu.taobao.com 
  ******************************************************************************
  */
  
#include <string.h>
#include "wm8960.h"
#include "i2c.h"


#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x)	(sizeof(x) / sizeof((x)[0]))
#endif

#define WM8960_DEBUG		1
#if ( WM8960_DEBUG == 1 )
#include <stdio.h>
#define WM8960_TRACE	printf
#else
#define WM8960_TRACE(...)
#endif



/*! @brief Define the register address of WM8960. */
#define WM8960_LINVOL       0x0
#define WM8960_RINVOL       0x1
#define WM8960_LOUT1        0x2
#define WM8960_ROUT1        0x3
#define WM8960_CLOCK1       0x4
#define WM8960_DACCTL1      0x5
#define WM8960_DACCTL2      0x6
#define WM8960_IFACE1       0x7
#define WM8960_CLOCK2       0x8
#define WM8960_IFACE2       0x9
#define WM8960_LDAC         0xa
#define WM8960_RDAC         0xb

#define WM8960_RESET        0xf
#define WM8960_3D           0x10
#define WM8960_ALC1         0x11
#define WM8960_ALC2         0x12
#define WM8960_ALC3         0x13
#define WM8960_NOISEG       0x14
#define WM8960_LADC         0x15
#define WM8960_RADC         0x16
#define WM8960_ADDCTL1      0x17
#define WM8960_ADDCTL2      0x18
#define WM8960_POWER1       0x19
#define WM8960_POWER2       0x1a
#define WM8960_ADDCTL3      0x1b
#define WM8960_APOP1        0x1c
#define WM8960_APOP2        0x1d

#define WM8960_LINPATH      0x20
#define WM8960_RINPATH      0x21
#define WM8960_LOUTMIX      0x22

#define WM8960_ROUTMIX      0x25
#define WM8960_MONOMIX1     0x26
#define WM8960_MONOMIX2     0x27
#define WM8960_LOUT2        0x28
#define WM8960_ROUT2        0x29
#define WM8960_MONO         0x2a
#define WM8960_INBMIX1      0x2b
#define WM8960_INBMIX2		0x2c
#define WM8960_BYPASS1		0x2d
#define WM8960_BYPASS2		0x2e
#define WM8960_POWER3		0x2f
#define WM8960_ADDCTL4		0x30
#define WM8960_CLASSD1		0x31

#define WM8960_CLASSD3		0x33
#define WM8960_PLL1		0x34
#define WM8960_PLL2		0x35
#define WM8960_PLL3		0x36
#define WM8960_PLL4		0x37

/* Cache register number */
#define WM8960_CACHEREGNUM   56

/*
 * Field Definitions.
 */

/* */
#define LEFT_INPUT_VOL_LINVOL

/*! @brief WM8960_IFACE1 FORMAT bits */
#define WM8960_IFACE1_FORMAT_MASK        0x03
#define WM8960_IFACE1_FORMAT_SHIFT       0x00
#define WM8960_IFACE1_FORMAT_RJ          0x00
#define WM8960_IFACE1_FORMAT_LJ          0x01
#define WM8960_IFACE1_FORMAT_I2S         0x02
#define WM8960_IFACE1_FORMAT_DSP         0x03
#define WM8960_IFACE1_FORMAT(x)          ((x << WM8960_IFACE1_FORMAT_SHIFT) & WM8960_IFACE1_FORMAT_MASK)
 
/*! @brief WM8960_IFACE1 WL bits */
#define WM8960_IFACE1_WL_MASK            0x0C
#define WM8960_IFACE1_WL_SHIFT           0x02
#define WM8960_IFACE1_WL_16BITS          0x00 
#define WM8960_IFACE1_WL_20BITS          0x01
#define WM8960_IFACE1_WL_24BITS          0x02
#define WM8960_IFACE1_WL_32BITS          0x03
#define WM8960_IFACE1_WL(x)              ((x << WM8960_IFACE1_WL_SHIFT) & WM8960_IFACE1_WL_MASK)

/*! @brief WM8960_IFACE1 LRP bit */
#define WM8960_IFACE1_LRP_MASK           0x10
#define WM8960_IFACE1_LRP_SHIFT          0x04 
#define WM8960_IFACE1_LRCLK_NORMAL_POL   0x00
#define WM8960_IFACE1_LRCLK_INVERT_POL   0x01
#define WM8960_IFACE1_DSP_MODEA          0x00
#define WM8960_IFACE1_DSP_MODEB          0x01
#define WM8960_IFACE1_LRP(x)             ((x << WM8960_IFACE1_LRP_SHIFT) & WM8960_IFACE1_LRP_MASK)

/*! @brief WM8960_IFACE1 DLRSWAP bit */
#define WM8960_IFACE1_DLRSWAP_MASK       0x20 
#define WM8960_IFACE1_DLRSWAP_SHIFT      0x05
#define WM8960_IFACE1_DACCH_NORMAL       0x00
#define WM8960_IFACE1_DACCH_SWAP         0x01
#define WM8960_IFACE1_DLRSWAP(x)         ((x << WM8960_IFACE1_DLRSWAP_SHIFT) & WM8960_IFACE1_DLRSWAP_MASK)

/*! @brief WM8960_IFACE1 MS bit */
#define WM8960_IFACE1_MS_MASK            0x40
#define WM8960_IFACE1_MS_SHIFT           0x06
#define WM8960_IFACE1_SLAVE              0x00
#define WM8960_IFACE1_MASTER             0x01
#define WM8960_IFACE1_MS(x)              ((x << WM8960_IFACE1_MS_SHIFT) & WM8960_IFACE1_MS_MASK)

/*! @brief WM8960_IFACE1 BCLKINV bit */
#define WM8960_IFACE1_BCLKINV_MASK       0x80
#define WM8960_IFACE1_BCLKINV_SHIFT      0x07
#define WM8960_IFACE1_BCLK_NONINVERT     0x00
#define WM8960_IFACE1_BCLK_INVERT        0x01
#define WM8960_IFACE1_BCLKINV(x)         ((x << WM8960_IFACE1_BCLKINV_SHIFT) & WM8960_IFACE1_BCLKINV_MASK)

/*! @brief WM8960_IFACE1 ALRSWAP bit */
#define WM8960_IFACE1_ALRSWAP_MASK       0x100
#define WM8960_IFACE1_ALRSWAP_SHIFT      0x08
#define WM8960_IFACE1_ADCCH_NORMAL       0x00
#define WM8960_IFACE1_ADCCH_SWAP         0x01
#define WM8960_IFACE1_ALRSWAP(x)         ((x << WM8960_IFACE1_ALRSWAP_SHIFT) & WM8960_IFACE1_ALRSWAP_MASK)

/*! @brief WM8960_POWER1 */
#define WM8960_POWER1_VREF_MASK        0x40
#define WM8960_POWER1_VREF_SHIFT       0x06 

#define WM8960_POWER1_AINL_MASK        0x20
#define WM8960_POWER1_AINL_SHIFT       0x05

#define WM8960_POWER1_AINR_MASK        0x10
#define WM8960_POWER1_AINR_SHIFT       0x04

#define WM8960_POWER1_ADCL_MASK        0x08
#define WM8960_POWER1_ADCL_SHIFT       0x03

#define WM8960_POWER1_ADCR_MASK        0x04
#define WM8960_POWER1_ADCR_SHIFT       0x02

/*! @brief WM8960_POWER2 */
#define WM8960_POWER2_DACL_MASK        0x100
#define WM8960_POWER2_DACL_SHIFT       0x08

#define WM8960_POWER2_DACR_MASK        0x80
#define WM8960_POWER2_DACR_SHIFT       0x07

#define WM8960_POWER2_LOUT1_MASK       0x40
#define WM8960_POWER2_LOUT1_SHIFT      0x06

#define WM8960_POWER2_ROUT1_MASK       0x20
#define WM8960_POWER2_ROUT1_SHIFT      0x05

#define WM8960_POWER2_SPKL_MASK        0x10
#define WM8960_POWER2_SPKL_SHIFT       0x04

#define WM8960_POWER2_SPKR_MASK        0x08
#define WM8960_POWER2_SPKR_SHIFT       0x03

/* seems that WM8960 can only write register, cannot read register */


struct wm8960_device 
{
    uint8_t             addr;
    uint32_t            instance;
};

static struct wm8960_device wm8960_dev;
static const uint8_t device_addr[] = {0x1A};

 /*
 * wm8960 register cache
 * We can't read the WM8960 register space when we are
 * using 2 wire for device control, so we cache them instead.
 */
 static const uint16_t wm8960_reg[WM8960_CACHEREGNUM] = {
	0x0097, 0x0097, 0x0000, 0x0000,
	0x0000, 0x0008, 0x0000, 0x000a,
	0x01c0, 0x0000, 0x00ff, 0x00ff,
	0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x007b, 0x0100, 0x0032,
	0x0000, 0x00c3, 0x00c3, 0x01c0,
	0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000,
	0x0100, 0x0100, 0x0050, 0x0050,
	0x0050, 0x0050, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0040, 0x0000,
	0x0000, 0x0050, 0x0050, 0x0000,
	0x0002, 0x0037, 0x004d, 0x0080,
	0x0008, 0x0031, 0x0026, 0x00e9,
};

 static uint16_t reg_cache[WM8960_CACHEREGNUM];

static int WOLFSON_WriteReg(uint8_t reg, uint16_t val)
{
    uint8_t cmd;
    cmd = (reg << 1) | ((val >> 8) & 0x0001);
    return I2C_WriteSingleRegister(wm8960_dev.instance, wm8960_dev.addr, cmd, val);
}


int wm8960_init(uint32_t instance)
{
    int i, ret;
    
    wm8960_dev.instance = instance;
    
    for(i = 0; i < ARRAY_SIZE(device_addr); i++)
    {
        ret = I2C_WriteSingleRegister(wm8960_dev.instance, device_addr[i], WM8960_RESET, 0x00);
        if(!ret)
        {
            wm8960_dev.addr = device_addr[i];
            WM8960_TRACE("wm8960 found! addr:0x%X\r\n", wm8960_dev.addr);
            memcpy(reg_cache, wm8960_reg, sizeof(wm8960_reg));
            
            WOLFSON_WriteReg(WM8960_RESET, 0x00); /* Reset all registers */
            WOLFSON_WriteReg(WM8960_POWER1, 0xFC); /*  VMID=50K, Enable VREF, AINL, AINR, ADCL and ADCR I2S_IN (bit 0), I2S_OUT (bit 1), DAP (bit 4), DAC (bit 5), ADC (bit 6) are powered on */
            WOLFSON_WriteReg(WM8960_POWER2, 0x1E0); /* Enable DACL, DACR, LOUT1, ROUT1, PLL down */
            WOLFSON_WriteReg(WM8960_POWER3, 0x3C); /* Enable left and right channel input PGA, left and right output mixer */
            WOLFSON_WriteReg(WM8960_CLOCK1, 0x00); /* Configure SYS_FS clock to 44.1kHz, MCLK_FREQ to 256*Fs, SYSCLK derived from MCLK input */
            WOLFSON_WriteReg( WM8960_IFACE1, 0x0D); /* Audio data length = 32bit, Left justified data format */
            WOLFSON_WriteReg( WM8960_LINPATH, 0x08); /* LMICBOOST = 0dB, Connect left and right PGA to left and right Input Boost Mixer */
            WOLFSON_WriteReg( WM8960_RINPATH, 0x08);
            WOLFSON_WriteReg( WM8960_INBMIX1, 0x50); /* Left and right input boost, LIN3BOOST and RIN3BOOST = 0dB */
            WOLFSON_WriteReg( WM8960_INBMIX2, 0x50);
            WOLFSON_WriteReg( WM8960_LOUTMIX, 0x180); /* Left DAC and LINPUT3 to left output mixer, LINPUT3 left output mixer volume = 0dB */
            WOLFSON_WriteReg( WM8960_ROUTMIX, 0x180); /* Right DAC and RINPUT3 to right output mixer, RINPUT3 right output mixer volume = 0dB */
            WOLFSON_WriteReg( WM8960_BYPASS1, 0x80); /* Left input boost mixer to left output mixer, volume = 0dB */
            WOLFSON_WriteReg( WM8960_BYPASS2, 0x80); /* Right input boost mixer to right output mixer, volume = 0dB */
            WOLFSON_WriteReg(WM8960_LADC, 0x1C3); //0dB ADC volume, 0dB
            WOLFSON_WriteReg(WM8960_RADC, 0x1C3); //0dB    
            WOLFSON_WriteReg(WM8960_LDAC, 0x1FF); //0dB igital DAC volume, 0dB
            WOLFSON_WriteReg(WM8960_RDAC, 0x1FF); //0dB
            WOLFSON_WriteReg(WM8960_LOUT1, 0x179); //Headphone volume, LOUT1 and ROUT1, 0dB
            WOLFSON_WriteReg(WM8960_ROUT1, 0x179);
            WOLFSON_WriteReg(WM8960_DACCTL1, 0x0000); /* Unmute DAC. */
        return 0;
        }
    }
    WM8960_TRACE("wm8960 failed\r\n");
    return 1;
}

int wm8960_format_config(uint32_t sample_rate, uint8_t bits)
{
    int retval;
    printf("WOLFSON_ConfigDataFormat\r\n");
    switch(sample_rate)
    {
        case 8000:
            retval = WOLFSON_WriteReg(WM8960_CLOCK1, 0x1B0);
            break;     
        case 11025:
            //retval = WOLFSON_WriteReg(handler, WM8960_CLOCK1, 0xD8);
            break;
        case 12000:
            retval = WOLFSON_WriteReg(WM8960_CLOCK1, 0x120);
            break;    
        case 16000:
            retval = WOLFSON_WriteReg(WM8960_CLOCK1, 0xD8);
            break;  
        case 22050:
            //retval = WOLFSON_WriteReg(handler, WM8960_CLOCK1, 0xD8);
            break;  
        case 24000:
            retval = WOLFSON_WriteReg( WM8960_CLOCK1, 0x90);
            break;   
        case 32000:
            retval = WOLFSON_WriteReg( WM8960_CLOCK1, 0x48);
            break;           
        case 44100:
            //retval = WOLFSON_WriteReg(handler, WM8960_CLOCK1, 0xD8);
            break;         
        case 48000:
            retval = WOLFSON_WriteReg(WM8960_CLOCK1, 0x00);
            break;                   
        default:
            retval = 0;
            break;
    }
	
    /*
     * Slave mode (MS = 0), LRP = 0, 32bit WL, left justified (FORMAT[1:0]=0b01)
     */
    switch(bits)
    {
        case 16:
        retval = WOLFSON_WriteReg(WM8960_IFACE1, 0x01);
        break;
        case 20:
        retval = WOLFSON_WriteReg(WM8960_IFACE1, 0x05);
        break;
        case 24:
        retval = WOLFSON_WriteReg(WM8960_IFACE1, 0x09);
        break;    
        case 32:
        retval = WOLFSON_WriteReg(WM8960_IFACE1, 0x0D);
        break;
        default:
            retval = 1;
            break;        
    }
    return retval;
}

int wm8960_set_volume(wolfson_module_t module, uint32_t volume)
{
    uint16_t vol = 0;
    int ret = 0;
     printf("WOLFSON_SetVolume\r\n");
    switch(module)
    {
        case kWolfsonModuleADC:
            vol = 0x100 | volume;
            ret = WOLFSON_WriteReg(WM8960_LADC, vol);
            ret = WOLFSON_WriteReg(WM8960_RADC, vol);
            break;
        case kWolfsonModuleDAC:
            vol = 0x100 | volume;
            ret = WOLFSON_WriteReg(WM8960_LDAC, vol);
            ret = WOLFSON_WriteReg(WM8960_RDAC, vol);
            break;
        case kWolfsonModuleHP:
            vol = 0x100 | volume;
            ret = WOLFSON_WriteReg(WM8960_LOUT1, vol);
            ret = WOLFSON_WriteReg(WM8960_ROUT1, vol);
			break;
        case kWolfsonModuleLineIn:
            vol = 0x100 | volume;
            ret = WOLFSON_WriteReg(WM8960_LINVOL, vol);
			ret = WOLFSON_WriteReg(WM8960_RINVOL, vol);
            break;
        default:
            ret = 1;
            break;
    }
    return ret;
}



