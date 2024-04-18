#ifndef __PROTO_MANAGE_H__
#define __PROTO_MANAGE_H__

typedef enum {
    PROTO_SLAVE,            // SLAVE
    PROTO_MASTER,           // MASTER
} proto_ms_e;

typedef enum {
    PROTO_DEV_RS1 = 0, 
    PROTO_DEV_RS2, 
    PROTO_DEV_RS3, 
    PROTO_DEV_RS4, 
    PROTO_DEV_NET, 
    PROTO_DEV_ZIGBEE, 
    PROTO_DEV_GPRS, 
    PROTO_DEV_LORA, 



    PROTO_DEV_RTU_SELF = 100,       // ±¾»úÇý¶¯
} eProtoDevId;

#define PROTO_DEV_RS_MAX        (PROTO_DEV_RS4)

#define PROTO_DEV_NET_TYPE(_n)  ((_n)<BOARD_ENET_TCPIP_NUM?PROTO_DEV_NET:PROTO_DEV_GPRS)

#endif



