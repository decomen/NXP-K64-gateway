
#include <board.h>
#include <stdio.h>
#include "net_helper.h"

void xfer_helper_serial_init( void )
{
    for(int n = 0; n < BOARD_UART_MAX; n++) {
        if(g_xfer_net_dst_uart_occ[n] 
    #ifdef RT_USING_CONSOLE
            && (!g_host_cfg.bDebug || BOARD_CONSOLE_USART != n)
    #endif
        ) {
            xfer_dst_serial_open(n);
        }
    }
}

void xfer_helper_enet_init( void )
{
    rt_bool_t open_flag[BOARD_ENET_TCPIP_NUM] = {0};
    for(int n = 0; n < BOARD_UART_MAX; n++) {
        if(g_xfer_net_dst_uart_occ[n] 
    #ifdef RT_USING_CONSOLE
            && (!g_host_cfg.bDebug || BOARD_CONSOLE_USART != n)
    #endif
        ) {
            if(NET_IS_ENET(g_xfer_net_dst_uart_map[n]) && !open_flag[g_xfer_net_dst_uart_map[n]]) {
                xfer_net_open(g_xfer_net_dst_uart_map[n]);
                open_flag[g_xfer_net_dst_uart_map[n]] = RT_TRUE;
            }
        }
    }
}

void xfer_helper_gprs_init( void )
{
    rt_bool_t open_flag[BOARD_GPRS_TCPIP_NUM] = {0};
    for(int n = 0; n < BOARD_UART_MAX; n++) {
        if(g_xfer_net_dst_uart_occ[n] 
    #ifdef RT_USING_CONSOLE
            && (!g_host_cfg.bDebug || BOARD_CONSOLE_USART != n)
    #endif
        ) {
            if(NET_IS_GPRS(g_xfer_net_dst_uart_map[n]) && !open_flag[g_xfer_net_dst_uart_map[n]-BOARD_ENET_TCPIP_NUM]) {
                xfer_net_open(g_xfer_net_dst_uart_map[n]);
                open_flag[g_xfer_net_dst_uart_map[n]-BOARD_ENET_TCPIP_NUM] = RT_TRUE;
            }
        }
    }
}

