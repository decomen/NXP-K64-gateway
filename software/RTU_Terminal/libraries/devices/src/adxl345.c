/**
  ******************************************************************************
  * @file    adxl345.c
  * @author  YANDLD
  * @version V2.5
  * @date    2013.12.25
  * @brief   www.beyondcore.net   http://upcmcu.taobao.com 
  ******************************************************************************
  */
  
#include "i2c.h"
#include "adxl345.h"
#include <math.h>


#define ADXL345_DEBUG		0
#if ( ADXL345_DEBUG == 1 )
#define ADXL345_TRACE	printf
#else
#define ADXL345_TRACE(...)
#endif



#define DEVICE_ID		0X00 	//����ID,0XE5
#define THRESH_TAP		0X1D   	//�û���ֵ
#define OFSX			0X1E
#define OFSY			0X1F
#define OFSZ			0X20
#define DUR				0X21
#define Latent			0X22
#define Window  		0X23
#define THRESH_ACK		0X24
#define THRESH_INACT	0X25
#define TIME_INACT		0X26
#define ACT_INACT_CTL	0X27
#define THRESH_FF		0X28
#define TIME_FF			0X29
#define TAP_AXES		0X2A
#define ACT_TAP_STATUS  0X2B
#define BW_RATE			0X2C
#define POWER_CTL		0X2D

#define INT_ENABLE		0X2E
#define INT_MAP			0X2F
#define INT_SOURCE  	0X30
#define DATA_FORMAT	    0X31
#define DATA_X0			0X32
#define DATA_X1			0X33
#define DATA_Y0			0X34
#define DATA_Y1			0X35
#define DATA_Z0			0X36
#define DATA_Z1			0X37
#define FIFO_CTL		0X38
#define FIFO_STATUS		0X39

/* ADXL345 possible addrs */
static const uint8_t adxl_addr[] = {0x53, 0x1D};

struct adxl_device 
{
    uint8_t     addr;
    uint32_t    instance;
    void        *user_data;
};

static struct adxl_device adxl_dev;


static int write_reg(uint8_t addr, uint8_t val)
{
    return I2C_WriteSingleRegister(adxl_dev.instance, adxl_dev.addr, addr, val);
}

//static int read_reg(uint8_t addr, uint8_t *val)
//{
//    return I2C_ReadSingleRegister(adxl_dev.instance, adxl_dev.addr, addr, val);
//}

int adxl345_init(uint32_t instance)
{
    int i;
    uint8_t id;
    
    adxl_dev.instance = instance;
    
    for(i = 0; i < ARRAY_SIZE(adxl_addr); i++)
    {
        if(!I2C_ReadSingleRegister(instance, adxl_addr[i], DEVICE_ID, &id))
        {
            if(id == 0xE5)
            {
                adxl_dev.addr = adxl_addr[i];
                /* init sequence */
                /*�͵�ƽ�ж����,13λȫ�ֱ���,��������Ҷ���,16g����  */
                write_reg(DATA_FORMAT, 0x2B);
                
                /*��������ٶ�Ϊ100Hz */
                write_reg(BW_RATE, 0x0A);
                
                /*����ʹ��,����ģʽ */
                write_reg(POWER_CTL, 0x28);
                
                /*��ʹ���ж� */
                write_reg(INT_ENABLE, 0x00);
                
                write_reg(OFSX, 0);
                write_reg(OFSY, 0);
                write_reg(OFSZ, 0);
                
                return 0;     
            }
        }
    }
    return 1;
}

int adxl345_readXYZ(short *x, short *y, short *z)
{
    uint8_t err;
    uint8_t buf[6];
    err = I2C_BurstRead(adxl_dev.instance, adxl_dev.addr, DATA_X0, 1, buf, 6);
    
    *x=(short)(((uint16_t)buf[1]<<8)+buf[0]); 	    
    *y=(short)(((uint16_t)buf[3]<<8)+buf[2]); 	    
    *z=(short)(((uint16_t)buf[5]<<8)+buf[4]); 
    
    return err;
}

extern void DelayMs(uint32_t ms);

int adxl345_calibration(void)
{
	short tx,ty,tz;
	uint8_t i;
	short offx=0,offy=0,offz=0; 
    write_reg(POWER_CTL, 0x00);//�Ƚ�������ģʽ.
    
	DelayMs(40);
    
	write_reg(DATA_FORMAT, 0X2B);	//�͵�ƽ�ж����,13λȫ�ֱ���,��������Ҷ���,16g���� 
	write_reg(BW_RATE, 0x0A);		//��������ٶ�Ϊ100Hz
	write_reg(POWER_CTL, 0x28);	   	//����ʹ��,����ģʽ
	write_reg(INT_ENABLE, 0x00);	//��ʹ���ж�
 //   adxl345_write_register(device, FIFO_CTL, 0x9F); //����FIFO
	DelayMs(12);
    
	for(i=0;i<10;i++)
	{
		adxl345_readXYZ(&tx, &ty, &tz);
        DelayMs(10);
		offx += tx;
		offy += ty;
		offz += tz;
	}	 	
	offx /= 10;
	offy /= 10;
	offz /= 10;
	offx = -offx/4;
	offy = -offy/4;
	offz = -(offz-256)/4;	
    ADXL345_TRACE("OFFX:%d OFFY:%d OFFZ:%d \r\n" ,offx, offy, offz);
 	write_reg(OFSX, offx);
	write_reg(OFSY, offy);
	write_reg(OFSZ, offz);
    return 0; 
}

//�õ��Ƕ�
//x,y,z:x,y,z������������ٶȷ���(����Ҫ��λ,ֱ����ֵ����)
//dir:Ҫ��õĽǶ�.0,��Z��ĽǶ�;1,��X��ĽǶ�;2,��Y��ĽǶ�.
//����ֵ:�Ƕ�ֵ.��λ0.1��.
short adxl345_convert_angle(short x, short y, short z, short *ax, short *ay, short *az)
{
	float temp;
    float fx,fy,fz;
    fx = (float)x; fy = (float)y;  fz = (float)z; 

    temp = sqrt((fx*fx+fy*fy))/fz;
    *az = (short)(atan(temp)*1800/3.14);
    temp = fx/sqrt((fy*fy+fz*fz));
    *ax = (short)(atan(temp)*1800/3.14);
    temp = fy/sqrt((fx*fx+fz*fz));
    *ay = (short)(atan(temp)*1800/3.14);
	return 0;
}
