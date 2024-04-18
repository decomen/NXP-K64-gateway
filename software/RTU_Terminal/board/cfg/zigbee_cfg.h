
#ifndef _ZIGBEE_CFG_H__
#define _ZIGBEE_CFG_H__

typedef enum {
    ZGB_TM_GW       = 0x00, // ����ģʽ (�ɼ������ɼ�)
    ZGB_TM_TRT      = 0x01, // ��Э��͸������ (ָ��Ŀ���豸)    //�ݲ�֧��
    ZGB_TM_DTU      = 0x02, // ����ת��ģʽ (ת��������)

    ZGB_TM_NUM
} zgb_tmode_e;

typedef struct {
    ZIGBEE_DEV_INFO_T   xInfo;
    UCHAR               ucState;    //ֻ��
    USHORT              usType;     //ֻ��
    USHORT              usVer;      //ֻ��
    rt_uint32_t         ulLearnStep;
    rt_uint8_t          btSlaveAddr;
    int                 nProtoType;
    zgb_tmode_e         tmode;
    eProtoDevId         dst_type;       // ת���˿�
    xfer_dst_cfg        dst_cfg;        // ת���˿�����(������Ҫ������)
} zigbee_cfg_t;

extern zigbee_cfg_t g_zigbee_cfg;

rt_err_t zigbee_cfg_init( void );

#endif

