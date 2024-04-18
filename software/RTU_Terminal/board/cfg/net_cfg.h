
#ifndef __NET_CFG_H__
#define __NET_CFG_H__

#include <rtdef.h>

typedef struct {
    rt_bool_t dhcp;        // 0 ¹Ø, 1 ¿ª
    char ipaddr[16];
    char netmask[16];
    char gw[16];
    char dns[16];
} net_cfg_t;

extern net_cfg_t g_net_cfg;

void vSaveNetCfgToFs( void );
rt_err_t net_cfg_init( void );

#endif

