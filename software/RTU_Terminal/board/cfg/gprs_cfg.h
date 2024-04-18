
#ifndef __GPRS_CFG_H__
#define __GPRS_CFG_H__

#include <rtdef.h>

typedef enum {
    GPRS_WM_FULL        ,   //ȫ��
    GPRS_WM_LOW_POWER   ,   //�͵���
    GPRS_WM_SLEEP       ,   //����
    GPRS_WM_SHUTDOWN    ,   //�ػ���ʹ��

    GPRS_WM_MAX
} eGPRS_WorkMode;

typedef enum {
    GPRS_OM_AUTO    ,   //�Զ�
    GPRS_OM_NEED    ,   //����
    GPRS_OM_MANUAL  ,   //�ֶ�

    GPRS_OM_MAX
} eGPRS_OpenMode;

typedef struct {
    char szAPN[16];
    char szUser[16];
    char szPsk[16];
    char szAPNNo[22];
    char szMsgNo[22];
} gprs_net_cfg_t;

typedef struct {
    eGPRS_WorkMode  eWMode;         //����ģʽ
    eGPRS_OpenMode  eOMode;         //���ʽ
    rt_uint8_t      btDebugLvl;     //���Եȼ�
    char            szSIMNo[12];    //SIM������ 11λ
    rt_uint32_t     ulInterval;     //����֡ʱ��
    rt_uint8_t      btRegLen;
    rt_uint8_t      btRegBuf[32];   //ע��� <= 64 byte
    rt_uint8_t      btHeartLen;
    rt_uint8_t      btHeartBuf[32]; //������ <= 64 byte
    rt_uint32_t     ulRetry;        //0��ʾ������
} gprs_work_cfg_t;

extern gprs_net_cfg_t g_gprs_net_cfg;
extern gprs_work_cfg_t g_gprs_work_cfg;

rt_err_t gprs_cfg_init( void );

#endif

