
#ifndef __RTU_REG_H__
#define __RTU_REG_H__

#include "rtdef.h"

// �ýṹ����ROM��,�������
// code ���ڱ�Ҫʱ�޸�
typedef struct {
    rt_uint32_t     test_time;      // ����ʱ��,����ģʽ������
    rt_uint32_t     code;           // �����
} reg_info_t;

// �ýṹ����SPI Flash��,�ָ����������
typedef struct {
    rt_uint8_t      key[128+1];
    rt_uint32_t     reg_time;
} reg_t;

void reg_init( void );
rt_bool_t reg_check( const char *key );
rt_bool_t reg_reg( const char *key );
int reg_regetid( rt_uint32_t code, rt_uint8_t id[40] );
rt_bool_t reg_testover( void );
void reg_testdo( rt_time_t sec );

extern reg_t g_reg;
extern reg_info_t g_reg_info;
extern rt_uint8_t g_regid[];
extern rt_bool_t g_isreg;
extern rt_bool_t g_istestover_poweron;

#endif

