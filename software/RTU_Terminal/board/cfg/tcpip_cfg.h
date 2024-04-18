
#ifndef __TCPIP_CFG_H__
#define __TCPIP_CFG_H__

#include <rtdef.h>

// TCP/IP ����
typedef enum {
    TCP_IP_TCP, 
    TCP_IP_UDP, 
} tcpip_type_e;

// ͨ��Э��
typedef enum {
    PROTO_MODBUS_TCP    = 0x00,    // modbus TCP
    PROTO_CC_BJDC       = 0x01,    // �������ݲɼ�Э��
    PROTO_MODBUS_RTU_OVER_TCP = 0x02,    // modbus rtu over tcp
    PROTO_HJT212        = 0x03,
} proto_tcpip_type_e;

typedef enum {
    TCPIP_CLIENT,           //��
    TCPIP_SERVER            //��
} tcpip_cs_e;

typedef enum {
    TCPIP_STATE_WAIT, 
    TCPIP_STATE_CONNED, 
    TCPIP_STATE_ACCEPT, 
    TCPIP_STATE_CONNING, 
} tcpip_state_e;

typedef enum {
    TCP_IP_M_NORMAL     = 0x00, // ����ͨ��ģʽ
    TCP_IP_M_XFER       = 0x01, // ת��ģʽ 

    TCP_IP_M_NUM
} tcpip_mode_e;

typedef struct {
    proto_tcpip_type_e  proto_type; //Э��
    proto_ms_e          proto_ms;   //����
    rt_uint8_t          maddress;   // modbus rtu over tcp slave address
} tcpip_noamal_cfg_t;

#include "xfer_cfg.h"

typedef struct {
    rt_bool_t       enable;         // 0:�ر�  1:����
    tcpip_type_e    tcpip_type;     // ģʽ
    tcpip_cs_e      tcpip_cs;       // client or server
    char            peer[64];       // clientʱ��Ч, Ŀ���ַ
    rt_uint16_t     port;           // �˿ں�

    rt_uint32_t     interval;       //
    rt_bool_t       keepalive;      //

    tcpip_mode_e    mode;
    union {
        tcpip_noamal_cfg_t  normal;
        xfer_cfg_t          xfer;
    } cfg;
} tcpip_cfg_t;

typedef struct {
    rt_bool_t enable;           // 0:�ر�  1:����
    tcpip_state_e eState;       //State
    char szRemIP[16];           //Rem IP
    rt_uint16_t usRemPort;      //Rem Port
    char szLocIP[16];           //Loc IP
    rt_uint16_t usLocPort;      //Loc Port
    rt_uint32_t ulConnTime;     //
} tcpip_state_t;

extern tcpip_cfg_t g_tcpip_cfgs[BOARD_TCPIP_MAX];
extern tcpip_state_t g_tcpip_states[];

rt_err_t tcpip_cfg_init( void );

#endif

