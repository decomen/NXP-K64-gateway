/**
  ******************************************************************************
  * @file    mpu9250.h
  * @author  YANDLD
  * @version V2.5
  * @date    2013.12.25
  * @brief   www.beyondcore.net   http://upcmcu.taobao.com 
  ******************************************************************************
  */
#ifndef __MPU9250_H__
#define __MPU9250_H__

#include <stdint.h>
#include <stdbool.h>

enum accel_scale
{
    AFS_2G,
    AFS_4G,
    AFS_8G,
    AFS_16G
};



enum gyro_scale
{
    GFS_250DPS,
    GFS_500DPS,
    GFS_1000DPS,
    GFS_2000DPS
};

enum mag_scale
{
    MFS_14BITS,
    MFS_16BITS,
};

struct mpu_config
{
    enum accel_scale        afs;
    enum gyro_scale         gfs;
    enum mag_scale          mfs;
    bool                    aenable_self_test;
    bool                    genable_self_test;
    bool                    gbypass_blpf;
};


#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x)	(sizeof(x) / sizeof((x)[0]))
#endif



//!< API function
int mpu9250_init(uint32_t instance);
int mpu9250_read_accel_raw(int16_t *adata);
int mpu9250_read_gyro_raw(int16_t *gdata);
int mpu9250_read_mag_raw(int16_t *mdata);
int mpu9250_config(struct mpu_config *config);
int mpu9250_read_temp_raw(int16_t *val);
void mpu9250_reset(void);
float mpu9250_get_ares(void);
float mpu9250_get_gres(void);
float mpu9250_get_mres(void);
int mpu9250_set_gyro_bias(int16_t* bias);
int mpu9250_enable_raw_data_int(void);
int mpu9250_get_int_status(void);


#endif
