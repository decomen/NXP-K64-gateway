
#include "rtdevice.h"
#include "board.h"
#include "net_helper.h"
#include <stdio.h>
#include "hjt212.h"
#include "cc_bjdc.h"

void __xfer_rcv_frame(int n, rt_uint8_t *frame, int len);

static net_helper_t s_net_list[NET_HELPER_MAX];

static void _net_task(void *parameter);

extern int lwip_get_error(int s);

#define net_printf(_fmt,...)    rt_kprintf( "[netthread-%s]->" _fmt, rt_thread_self()->name, ##__VA_ARGS__ )

net_helper_t *net_get_helper(int n)
{
    if(n >= 0 && n < NET_HELPER_MAX) {
        return &s_net_list[n];
    } else {
        return RT_NULL;
    }
}

rt_bool_t net_open(int n, int bufsz, int stack_size, const char *prefix)
{
    net_helper_t *net = &s_net_list[n];
    net->sock_fd = -1;
    net->tcp_fd = -1;

    net_close(n);
    
    net->disconnect = RT_FALSE;
    net->shutdown   = RT_FALSE;
    net->buffer     = RT_KERNEL_CALLOC(bufsz);
    net->bufsz      = bufsz;
    
    BOARD_CREAT_NAME(net_mutex_name, "%s_%d", prefix, n);
    if((net->mutex = rt_mutex_create(net_mutex_name, RT_IPC_FLAG_FIFO)) == RT_NULL) {
        net_printf("init %s failed\n", net_mutex_name);
        return RT_FALSE;
    }
    
    BOARD_CREAT_NAME(net_thread_name, "%s_%d", prefix, n);
    net->thread = rt_thread_create(net_thread_name, _net_task, (void *)n, stack_size, 20, 20);
    if(net->thread != RT_NULL) {
        rt_thddog_register(net->thread, 60);
        rt_thread_startup(net->thread);
    }
    return RT_TRUE;
}

rt_bool_t net_isconnect(int n)
{
    net_helper_t *net = &s_net_list[n];
    return (net->tcp_fd >= 0);
}

rt_bool_t net_waitconnect(int n)
{
    net_helper_t *net = &s_net_list[n];
    while(net->tcp_fd < 0) {
        rt_thread_delay(RT_TICK_PER_SECOND);
        rt_thddog_feed(RT_NULL);
    }
    
    return RT_TRUE;
}

void net_disconnect(int n)
{
    net_helper_t *net = &s_net_list[n];
    net->disconnect = RT_TRUE;

    if(NET_IS_ENET(n)) {
        for(int i = 0; i < 50; i++) {
            if (net->tcp_fd >= 0) {
                rt_thread_delay(RT_TICK_PER_SECOND / 10);
            } else {
                break;
            }
            rt_thddog_feed(RT_NULL);
        }
    } 
#if NET_HAS_GPRS
    else if(NET_IS_GPRS(n)) {
        if(net->tcp_fd >= 0) {
            bGPRS_SocketClose(net->tcp_fd, 5000);
            net->tcp_fd = -1;
        }
    }
#endif
}

void net_close(int n)
{
    net_helper_t *net = &s_net_list[n];

    net->shutdown = RT_TRUE;
    net_disconnect(n);

    for(int i = 0; i < 50; i++) {
        if (net->thread) {
            rt_thread_delay(RT_TICK_PER_SECOND / 10);
        } else {
            break;
        }
        rt_thddog_feed(RT_NULL);
    }

    if(net->thread) {
        rt_thddog_unregister(net->thread);
        if (RT_EOK == rt_thread_delete(net->thread)) {
            net->thread = RT_NULL;
        }
    }

    if(net->buffer) RT_KERNEL_FREE((void *)net->buffer);
    net->buffer = RT_NULL;
    
    if(net->mutex) rt_mutex_delete(net->mutex);
    net->mutex = RT_NULL;
}


void ws_net_try_send(rt_uint16_t port, void *buffer, rt_size_t size)
{
    for( int n = 0; n < BOARD_ENET_TCPIP_NUM; n++ ) {
        net_helper_t *net = &s_net_list[n];
        if(net->tcp_fd >= 0 && g_tcpip_cfgs[n].enable && g_tcpip_cfgs[n].port == port ) {
            rt_mutex_take(net->mutex, RT_WAITING_FOREVER);
            switch( g_tcpip_cfgs[n].tcpip_type ) {
            case TCP_IP_TCP:
                lwip_write(net->tcp_fd, buffer, size);
                break;
            case TCP_IP_UDP:
                lwip_sendto(net->tcp_fd, buffer, size, 0, (struct sockaddr *)&net->to_addr, sizeof(struct sockaddr));
                break;
            }
            rt_mutex_release(net->mutex);
            break;
        }
    }
}

int net_send(int n, const rt_uint8_t *buffer, rt_uint16_t bufsz)
{
    int sendlen = -1;
    net_helper_t *net = &s_net_list[n];
    tcpip_type_e tcpip_type = g_tcpip_cfgs[n].tcpip_type;

    if(net->tcp_fd >= 0) {
        if(NET_IS_ENET(n) && net->mutex) {
            if( g_ws_cfg.enable && (WS_PORT_NET == g_ws_cfg.port_type) && (g_tcpip_cfgs[n].port == g_ws_cfg.listen_port) ) {
                ws_vm_snd_write( 0, (void *)buffer, bufsz );
            }
            rt_mutex_take(net->mutex, RT_WAITING_FOREVER);
            switch (tcpip_type) {
            case TCP_IP_TCP:
                sendlen = lwip_write(net->tcp_fd, buffer, bufsz);
                break;

            case TCP_IP_UDP:
                sendlen = lwip_sendto(net->tcp_fd, buffer, bufsz, 0, (struct sockaddr *)&net->to_addr, sizeof(struct sockaddr));
                break;
            }
            rt_mutex_release(net->mutex);
        } 
#if NET_HAS_GPRS
        else if(NET_IS_GPRS(n)) {
            sendlen = nGPRS_SocketSend(net->tcp_fd, buffer, bufsz, 5000);
        }
#endif
    }

    return sendlen;
}

static void prvRunAsENetTCPServer(int n)
{
    fd_set          readset;
    struct timeval  timeout;
    net_helper_t    *net = &s_net_list[n];
    rt_uint16_t     port;
    struct sockaddr_in srv;
    rt_bool_t       keepalive;
    int             result;
    socklen_t       len;
    int             on = 1;

    net->sock_fd    = -1;
    net->tcp_fd     = -1;

    port        = g_tcpip_cfgs[n].port;
    keepalive   = g_tcpip_cfgs[n].keepalive;

    net->sock_fd = lwip_socket(AF_INET, SOCK_STREAM, 0);
    if (net->sock_fd < 0) {
        elog_e("tcpserver", "socket failed, err:%d, index:%d", lwip_get_error(net->sock_fd), n);
        goto __END;
    }

    if (lwip_setsockopt(net->sock_fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0) {
        elog_e("tcpserver", "SO_REUSEADDR failed, err:%d, index:%d", lwip_get_error(net->sock_fd), n);
        goto __END;
    }

    srv.sin_addr.s_addr = htonl(INADDR_ANY);
    srv.sin_family      = AF_INET;
    srv.sin_port        = htons(port);
    if (lwip_bind(net->sock_fd, (struct sockaddr *)&srv, sizeof(struct sockaddr)) < 0) {
        elog_e("tcpserver", "bind failed, err:%d, index:%d", lwip_get_error(net->sock_fd), n);
        goto __END;
    }

    if (lwip_listen(net->sock_fd, 2) < 0) {
        elog_e("tcpserver", "listen failed, err:%d, index:%d", lwip_get_error(net->sock_fd), n);
        goto __END;
    }

    if (lwip_fcntl(net->sock_fd, F_SETFL, O_NONBLOCK) < 0) {
        elog_e("tcpserver", "O_NONBLOCK failed, err:%d, index:%d", lwip_get_error(net->sock_fd), n);
        goto __END;
    }

    net_printf("tcp server start listen\n");

    {
        g_tcpip_states[n].eState = TCPIP_STATE_CONNING;
    }

    while(1) {
        while(1) {
            int max_fd = net->sock_fd;
            FD_ZERO(&readset);
            FD_SET(net->sock_fd, &readset);
            if(net->tcp_fd >= 0) {
                FD_SET(net->tcp_fd, &readset);
                if(net->tcp_fd > net->sock_fd) max_fd = net->tcp_fd;
            }
            timeout.tv_sec = 4; timeout.tv_usec = 0;
            rt_thddog_feed("tcp accept/read before select 5 sec");
            result = lwip_select(max_fd + 1, &readset, 0, 0, &timeout);
            rt_thddog_feed("tcp accept/read after select");
            if (result < 0) {
                elog_e("tcpserver", "accept/read select error, index:%d", n);
                goto __END;
            } else if (0 == result) {
                //net_printf("tcp accept/read select timeout!\n");
                if (net->disconnect || net->shutdown) {
                    elog_i("tcpserver", "disconnect/shutdown, fd:%d, index:%d", net->tcp_fd, n);
                    goto __END;
                }
                continue;
            }
            break;
        }

        if(net->sock_fd >= 0 && FD_ISSET(net->sock_fd, &readset)) {
            int tmp_fd = -1;
            struct sockaddr cliaddr;
            len = sizeof(struct sockaddr_in);
            rt_thddog_feed("tcp lwip_accept");
            tmp_fd = lwip_accept(net->sock_fd, &cliaddr, &len);
            
            if(net->tcp_fd >= 0) {
                if (tmp_fd >= 0) lwip_close(tmp_fd);
                elog_w("tcpserver", "just keep one conn, fd:%d, index:%d", tmp_fd, n);
            } else {
                net->tcp_fd = tmp_fd;
                elog_i("tcpserver", "new conn, fd:%d, index:%d", tmp_fd, n);
                if(net->tcp_fd >= 0) {
                    if (g_tcpip_states[n].eState != TCPIP_STATE_CONNED) {
                        g_tcpip_states[n].ulConnTime = (rt_millisecond_from_tick(rt_tick_get())) / 1000;
                    }
                    g_tcpip_states[n].eState = TCPIP_STATE_CONNED;
                    {
                        struct sockaddr_in sa;
                        len = sizeof(sa);
                        if (!lwip_getsockname(net->tcp_fd, (struct sockaddr *)&sa, &len)) {
                            strcpy(g_tcpip_states[n].szLocIP, inet_ntoa(sa.sin_addr));
                            g_tcpip_states[n].usLocPort = ntohs(sa.sin_port);
                        }
                        if (!lwip_getpeername(net->tcp_fd, (struct sockaddr *)&sa, &len)) {
                            strcpy(g_tcpip_states[n].szRemIP, inet_ntoa(sa.sin_addr));
                            g_tcpip_states[n].usRemPort = ntohs(sa.sin_port);
                        }
                        
                        if (keepalive) {
                            int val = SO_KEEPALIVE;
                            lwip_setsockopt(net->tcp_fd, SOL_SOCKET, SO_KEEPALIVE, &val, 4);
                            {
                                int tmp = 20;   // keepIdle in seconds
                                lwip_setsockopt(net->tcp_fd, IPPROTO_TCP, TCP_KEEPIDLE, (void *)&tmp, sizeof(tmp));
                                tmp = 1;        // keepInterval in seconds
                                lwip_setsockopt(net->tcp_fd, IPPROTO_TCP, TCP_KEEPINTVL, (void *)&tmp, sizeof(tmp));
                                tmp = 10;       // keepCount in seconds
                                lwip_setsockopt(net->tcp_fd, IPPROTO_TCP, TCP_KEEPCNT, (void *)&tmp, sizeof(tmp));
                            }
                        }

                    }
                }
            }
        }
        if(net->tcp_fd >= 0 && FD_ISSET(net->tcp_fd, &readset)) {
            while (1) {
                int readlen = 0;
                rt_thddog_feed("tcp server read with take mutex");
                if(net->mutex) rt_mutex_take(net->mutex, RT_WAITING_FOREVER);
                //net->pos = 0;
                readlen = lwip_read(net->tcp_fd, net->buffer, net->bufsz);
                if(net->mutex) rt_mutex_release(net->mutex);
                if (0 == readlen) {
                    elog_w("tcpserver", "socket close with read 0, index:%d", n);
                    goto __END;
                } else if (readlen < 0) {
                    if (EAGAIN == lwip_get_error(net->tcp_fd)) {
                        break;
                    } else {
                        elog_w("tcpserver", "socket close with read err, err:%d, index:%d", lwip_get_error(net->tcp_fd), n);
                        goto __END;
                    }
                } else {
                    net->pos = readlen;
                    rt_thddog_feed("tcp server ws_vm_rcv_write");
                    if( g_ws_cfg.enable && (WS_PORT_NET == g_ws_cfg.port_type) && (port == g_ws_cfg.listen_port) ) {
                        ws_vm_rcv_write(0, net->buffer, net->pos);
                    }
                    
                    if(NET_IS_NORMAL(n)) {
                        switch(g_tcpip_cfgs[n].cfg.normal.proto_type) {
                            case PROTO_MODBUS_TCP:
                            case PROTO_MODBUS_RTU_OVER_TCP:
                                if( PROTO_SLAVE == g_tcpip_cfgs[n].cfg.normal.proto_ms ) {
                                    rt_thddog_feed("xMBPortEventPost");
                                    xMBPortEventPost( BOARD_UART_MAX + n, EV_FRAME_RECEIVED );
                                } else if( PROTO_MASTER == g_tcpip_cfgs[n].cfg.normal.proto_ms ) {
                                    rt_thddog_feed("xMBMasterPortEventPost");
                                    xMBMasterPortEventPost( BOARD_UART_MAX + n, EV_MASTER_FRAME_RECEIVED );
                                }
                                break;
                                break;
                            case PROTO_CC_BJDC: // no support
                            case PROTO_HJT212:
                                break;
            	        }
                    } else if(NET_IS_XFER(n)) { 
                        __xfer_rcv_frame(n, (rt_uint8_t *)net->buffer, net->pos);
                    }
                    break;
                }
            }
        }
    }

__END:
    elog_i("tcpserver", "end, index:%d", n);

    //rt_mutex_take( &s_cc_mutex[index], RT_WAITING_FOREVER );
    rt_thddog_feed("tcp server end");

    if(NET_IS_NORMAL(n)) {
        ;
    } else if(NET_IS_XFER(n)) {
        ;
    }

    if(net->mutex) rt_mutex_take(net->mutex, RT_WAITING_FOREVER);
    if (net->tcp_fd >= 0) {
        lwip_close(net->tcp_fd);
        net->tcp_fd = -1;
    }
    if (net->sock_fd >= 0) {
        lwip_close(net->sock_fd);
        net->sock_fd = -1;
    }
    if(net->mutex) rt_mutex_release(net->mutex);
}

static void prvRunAsENetTCPClient(int n)
{
    fd_set          readset, writeset;
    struct timeval  timeout;
    net_helper_t    *net = &s_net_list[n];
    rt_uint16_t     port;
    const char      *peer_host;
    struct sockaddr_in peer;
    rt_bool_t       keepalive;
    int             result;
    socklen_t       len;

    net->sock_fd    = -1;
    net->tcp_fd     = -1;

    port        = g_tcpip_cfgs[n].port;
    peer_host   = g_tcpip_cfgs[n].peer;
    keepalive   = g_tcpip_cfgs[n].keepalive;

    if (inet_aton(peer_host, &peer.sin_addr) <= 0) {
        struct hostent *host = lwip_gethostbyname(peer_host);
        if (host && host->h_addr_list && host->h_addr) {
            memcpy(&peer.sin_addr, host->h_addr, sizeof(struct in_addr));
        } else {
            goto __END;
        }
    }

    net->sock_fd = lwip_socket(AF_INET, SOCK_STREAM, 0);
    if (net->sock_fd < 0) {
        elog_e("tcpclient", "socket failed, err:%d, index:%d", lwip_get_error(net->sock_fd), n);
        goto __END;
    }

    net_printf("tcp client start connect\n");

    {
        g_tcpip_states[n].eState = TCPIP_STATE_CONNING;
    }

	timeout.tv_sec = 4; timeout.tv_usec = 0;
	if (setsockopt(net->sock_fd, SOL_SOCKET, SO_SNDTIMEO, &timeout, 4) < 0) {
        elog_e("tcpclient", "SO_SNDTIMEO failed, err:%d, index:%d", lwip_get_error(net->sock_fd), n);
        goto __END;
    }
    peer.sin_family      = AF_INET;
    peer.sin_port        = htons(port);
	if (1) {
		rt_thddog_feed("tcp clientlwip_connect");
        if (lwip_connect(net->sock_fd, (struct sockaddr *)&peer, sizeof(struct sockaddr)) >= 0) {
            ;
        } else {
            if (EINPROGRESS == lwip_get_error(net->sock_fd)) {
                net_printf("tcp client connect timeout!\n");
            } else {
                elog_i("tcpclient", "connect failed, err:%d, index:%d", lwip_get_error(net->sock_fd), n);
            }
            goto __END;
        }

		if (lwip_fcntl(net->sock_fd, F_SETFL, O_NONBLOCK) < 0) {
            elog_e("tcpclient", "O_NONBLOCK failed, err:%d, index:%d", lwip_get_error(net->sock_fd), n);
	        goto __END;
	    }

        // the same
        net->tcp_fd = net->sock_fd;
        net->sock_fd = -1;

        elog_i("tcpclient", "new conn, fd:%d, index:%d", net->tcp_fd, n);
        if(NET_IS_NORMAL(n)) {
            switch(g_tcpip_cfgs[n].cfg.normal.proto_type) {
            case PROTO_CC_BJDC:
                cc_bjdc_startwork(n);
                break;
            case PROTO_HJT212:
                hjt212_startwork(n);
                break;
            }
        } else if(NET_IS_XFER(n)) {
            ;
        }

        {
            if (g_tcpip_states[n].eState != TCPIP_STATE_CONNED) {
                g_tcpip_states[n].ulConnTime = (rt_millisecond_from_tick(rt_tick_get())) / 1000;
            }
            g_tcpip_states[n].eState = TCPIP_STATE_CONNED;
            {
                struct sockaddr_in sa;
                len = sizeof(sa);
                if (!lwip_getsockname(net->tcp_fd, (struct sockaddr *)&sa, &len)) {
                    strcpy(g_tcpip_states[n].szLocIP, inet_ntoa(sa.sin_addr));
                    g_tcpip_states[n].usLocPort = ntohs(sa.sin_port);
                }
                if (!lwip_getpeername(net->tcp_fd, (struct sockaddr *)&sa, &len)) {
                    strcpy(g_tcpip_states[n].szRemIP, inet_ntoa(sa.sin_addr));
                    g_tcpip_states[n].usRemPort = ntohs(sa.sin_port);
                }
                
                if (keepalive) {
                    int val = SO_KEEPALIVE;
                    lwip_setsockopt(net->tcp_fd, SOL_SOCKET, SO_KEEPALIVE, &val, 4);
                    {
                        int tmp = 20;   // keepIdle in seconds
                        lwip_setsockopt(net->tcp_fd, IPPROTO_TCP, TCP_KEEPIDLE, (void *)&tmp, sizeof(tmp));
                        tmp = 1;        // keepInterval in seconds
                        lwip_setsockopt(net->tcp_fd, IPPROTO_TCP, TCP_KEEPINTVL, (void *)&tmp, sizeof(tmp));
                        tmp = 10;       // keepCount in seconds
                        lwip_setsockopt(net->tcp_fd, IPPROTO_TCP, TCP_KEEPCNT, (void *)&tmp, sizeof(tmp));
                    }
                }

            }
        }
    }

    while (1) {
        FD_ZERO(&readset);
        FD_SET(net->tcp_fd, &readset);
        
        rt_thddog_feed("tcp client read before select 1 sec");
        timeout.tv_sec = 4; timeout.tv_usec = 0;
        result = lwip_select(net->tcp_fd + 1, &readset, 0, 0, &timeout);
        rt_thddog_feed("tcp client read after select");
        if (result < 0) {
            elog_e("tcpclient", "read select err, err:%d, index:%d", lwip_get_error(net->sock_fd), n);
            goto __END;
        } else if (0 == result) {
            //net_printf("tcp client read select timeout!\n");
            if (net->disconnect || net->shutdown) {
                elog_i("tcpclient", "disconnect/shutdown, fd:%d, index:%d", net->tcp_fd, n);
                goto __END;
            }
            continue;
        }

        if (FD_ISSET(net->tcp_fd, &readset)) {
            while (1) {
                int readlen = 0;
                rt_thddog_feed("tcp client read with take mutex");
                if(net->mutex) rt_mutex_take(net->mutex, RT_WAITING_FOREVER);
                //net->pos = 0;
                readlen = lwip_read(net->tcp_fd, net->buffer, net->bufsz);
                if(net->mutex) rt_mutex_release(net->mutex);
                if (0 == readlen) {
                    elog_w("tcpclient", "socket close with read 0, fd:%d, index:%d", net->tcp_fd, n);
                    goto __END;
                } else if (readlen < 0) {
                    if (EAGAIN == lwip_get_error(net->tcp_fd)) {
                        break;
                    } else {
                        elog_w("tcpclient", "socket close with read err, err:%d, index:%d", lwip_get_error(net->tcp_fd), n);
                        goto __END;
                    }
                } else {
                    net->pos = readlen;
                    rt_thddog_feed("tcp client ws_vm_rcv_write");
                    if( g_ws_cfg.enable && (WS_PORT_NET == g_ws_cfg.port_type) && (port == g_ws_cfg.listen_port) ) {
                        ws_vm_rcv_write(0, net->buffer, net->pos);
                    }
                    
                    if(NET_IS_NORMAL(n)) {
                        switch(g_tcpip_cfgs[n].cfg.normal.proto_type) {
                        case PROTO_MODBUS_TCP:
                        case PROTO_MODBUS_RTU_OVER_TCP:
                            if(PROTO_SLAVE == g_tcpip_cfgs[n].cfg.normal.proto_ms) {
                                rt_thddog_feed("xMBPortEventPost");
                                xMBPortEventPost( BOARD_UART_MAX + n, EV_FRAME_RECEIVED );
                            } else if(PROTO_MASTER == g_tcpip_cfgs[n].cfg.normal.proto_ms) {
                                rt_thddog_feed("xMBMasterPortEventPost");
                                xMBMasterPortEventPost( BOARD_UART_MAX + n, EV_MASTER_FRAME_RECEIVED );
                            }
                            break;
                        case PROTO_CC_BJDC:
                            rt_thddog_feed("CC_BJDC_PutBytes");
                            CC_BJDC_PutBytes(n, net->buffer, net->pos);
                            break;
                        case PROTO_HJT212:
                            rt_thddog_feed("HJT212_PutBytes");
                            HJT212_PutBytes(n, net->buffer, net->pos);
                            break;
                        }
                    } else if(NET_IS_XFER(n)) {
                        __xfer_rcv_frame(n, (rt_uint8_t *)net->buffer, net->pos);
                    }

                    break;
                }
            }
        }
    }

__END:
    elog_i("tcpclient", "end, index:%d", n);

    //rt_mutex_take( &s_cc_mutex[index], RT_WAITING_FOREVER );
    rt_thddog_feed("tcp client end");

    if(NET_IS_NORMAL(n)) {
        switch(g_tcpip_cfgs[n].cfg.normal.proto_type) {
        case PROTO_CC_BJDC:
            cc_bjdc_exitwork(n);
            break;
        case PROTO_HJT212:
            hjt212_exitwork(n);
            break;
        }
    } else if(NET_IS_XFER(n)) {
        ;
    }

    if(net->mutex) rt_mutex_take(net->mutex, RT_WAITING_FOREVER);
    if (net->tcp_fd >= 0) {
        lwip_close(net->tcp_fd);
        net->tcp_fd = -1;
    }
    if (net->sock_fd >= 0) {
        lwip_close(net->sock_fd);
        net->sock_fd = -1;
    }
    if(net->mutex) rt_mutex_release(net->mutex);
}

static void prvRunAsENetUDPServer(int n)
{
    fd_set          readset;
    struct timeval  timeout;
    net_helper_t    *net = &s_net_list[n];
    rt_uint16_t     port;
    struct sockaddr_in srv;
    int             result;
    socklen_t       len;
    int             on = 1;

    net->sock_fd    = -1;
    net->tcp_fd     = -1;

    port        = g_tcpip_cfgs[n].port;

    net->sock_fd = lwip_socket(AF_INET, SOCK_DGRAM, 0);
    if (net->sock_fd < 0) {
        elog_e("udpserver", "socket failed, err:%d, index:%d", lwip_get_error(net->sock_fd), n);
        goto __END;
    }

    if (lwip_setsockopt(net->sock_fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0) {
        elog_e("udpserver", "SO_REUSEADDR failed, err:%d, index:%d", lwip_get_error(net->sock_fd), n);
        goto __END;
    }

    srv.sin_addr.s_addr = htonl(INADDR_ANY);
    srv.sin_family      = AF_INET;
    srv.sin_port        = htons(port);
    if (lwip_bind(net->sock_fd, (struct sockaddr *)&srv, sizeof(struct sockaddr)) < 0) {
        elog_e("udpserver", "bind failed, err:%d, index:%d", lwip_get_error(net->sock_fd), n);
        goto __END;
    }

    if (lwip_fcntl(net->sock_fd, F_SETFL, O_NONBLOCK) < 0) {
        elog_e("udpserver", "O_NONBLOCK failed, err:%d, index:%d", lwip_get_error(net->sock_fd), n);
        goto __END;
    }

    net->tcp_fd  = net->sock_fd;
    net->sock_fd = -1;

    while (1) {
        FD_ZERO(&readset);
        FD_SET(net->tcp_fd, &readset);
        
        rt_thddog_feed("udp server recvfrom before select 1 sec");
        timeout.tv_sec = 4; timeout.tv_usec = 0;
        result = lwip_select(net->tcp_fd + 1, &readset, 0, 0, &timeout);
        rt_thddog_feed("udp server recvfrom after select");
        if (result < 0) {
            elog_e("udpserver", "recvfrom select error, err:%d, index:%d", lwip_get_error(net->tcp_fd), n);
            goto __END;
        } else if (0 == result) {
            //net_printf("udp server recvfrom select timeout!\n");
            if (net->disconnect || net->shutdown) {
                elog_i("udpserver", "disconnect/shutdown, fd:%d, index:%d", net->tcp_fd, n);
                goto __END;
            }
            continue;
        }
        
        if (FD_ISSET(net->tcp_fd, &readset)) {
            while (1) {
                int readlen = 0;
                struct sockaddr_in client_addr;
                len = sizeof(struct sockaddr);
                rt_thddog_feed("udp server recvfrom with take mutex");
                if(net->mutex) rt_mutex_take(net->mutex, RT_WAITING_FOREVER);
                //net->pos = 0;
                readlen = lwip_recvfrom(net->tcp_fd, net->buffer, net->bufsz, 0, (struct sockaddr *)&client_addr, &len);
                if(net->mutex) rt_mutex_release(net->mutex);
                if (0 == readlen) {
                    elog_w("udpserver", "close with read 0, index:%d", n);
                    goto __END;
                } else if (readlen < 0) {
                    if (EAGAIN == lwip_get_error(net->tcp_fd)) {
                        break;
                    } else {
                        elog_w("udpserver", "close with read err, err:%d, index:%d", lwip_get_error(net->tcp_fd), n);
                        goto __END;
                    }
                } else {
                    net->pos = readlen;
                    net->to_addr = client_addr;
                    rt_thddog_feed("udp server ws_vm_rcv_write");
                    if( g_ws_cfg.enable && (WS_PORT_NET == g_ws_cfg.port_type) && (port == g_ws_cfg.listen_port) ) {
                        ws_vm_rcv_write(0, net->buffer, net->pos);
                    }
                    
                    if(NET_IS_NORMAL(n)) {
                        switch(g_tcpip_cfgs[n].cfg.normal.proto_type) {
                        case PROTO_MODBUS_TCP:
                        case PROTO_MODBUS_RTU_OVER_TCP:
                            if(PROTO_SLAVE == g_tcpip_cfgs[n].cfg.normal.proto_ms) {
                                rt_thddog_feed("xMBPortEventPost");
                                xMBPortEventPost( BOARD_UART_MAX + n, EV_FRAME_RECEIVED );
                            } else if(PROTO_MASTER == g_tcpip_cfgs[n].cfg.normal.proto_ms) {
                                rt_thddog_feed("xMBMasterPortEventPost");
                                xMBMasterPortEventPost( BOARD_UART_MAX + n, EV_MASTER_FRAME_RECEIVED );
                            }
                            break;
                        case PROTO_CC_BJDC: // no support
                        case PROTO_HJT212:
                            break;
                        }
                    } else if(NET_IS_XFER(n)) {
                        __xfer_rcv_frame(n, (rt_uint8_t *)net->buffer, net->pos);
                    }

                    break;
                }
            }
        }
    }

__END:
    net_printf("udp server end!\n");
    elog_i("udpserver", "end, index:%d", n);

    rt_thddog_feed("udp server end");

    if(NET_IS_NORMAL(n)) {
        ;
    } else if(NET_IS_XFER(n)) {
        ;
    }

    if(net->mutex) rt_mutex_take(net->mutex, RT_WAITING_FOREVER);
    if (net->tcp_fd >= 0) {
        lwip_close(net->tcp_fd);
        net->tcp_fd = -1;
    }
    if (net->sock_fd >= 0) {
        lwip_close(net->sock_fd);
        net->sock_fd = -1;
    }
    if(net->mutex) rt_mutex_release(net->mutex);
}

static void prvRunAsENetUDPClient(int n)
{
    fd_set          readset;
    struct timeval  timeout;
    net_helper_t    *net = &s_net_list[n];
    rt_uint16_t     port;
    const char      *peer_host;
    struct sockaddr_in srv;
    int             result;
    socklen_t       len;
    int             on = 1;

    net->sock_fd    = -1;
    net->tcp_fd     = -1;

    port        = g_tcpip_cfgs[n].port;
    peer_host   = g_tcpip_cfgs[n].peer;

    if (inet_aton(peer_host, &net->to_addr) <= 0) {
        struct hostent *host = lwip_gethostbyname(peer_host);
        if (host && host->h_addr_list && host->h_addr) {
            memcpy(&net->to_addr.sin_addr, host->h_addr, sizeof(struct in_addr));
        } else {
            goto __END;
        }
    }

    net->sock_fd = lwip_socket(AF_INET, SOCK_DGRAM, 0);
    if (net->sock_fd < 0) {
        elog_e("udpclient", "socket failed, err:%d, index:%d", lwip_get_error(net->sock_fd), n);
        goto __END;
    }

    if (lwip_setsockopt(net->sock_fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0) {
        elog_e("udpclient", "SO_REUSEADDR failed, err:%d, index:%d", lwip_get_error(net->sock_fd), n);
        goto __END;
    }

    srv.sin_addr.s_addr = htonl(INADDR_ANY);
    srv.sin_family      = AF_INET;
    srv.sin_port        = htons(port);
    if (lwip_bind(net->sock_fd, (struct sockaddr *)&srv, sizeof(struct sockaddr)) < 0) {
        elog_e("udpclient", "bind failed, err:%d, index:%d", lwip_get_error(net->sock_fd), n);
        goto __END;
    }

    if (lwip_fcntl(net->sock_fd, F_SETFL, O_NONBLOCK) < 0) {
        elog_e("udpclient", "O_NONBLOCK failed, err:%d, index:%d", lwip_get_error(net->sock_fd), n);
        goto __END;
    }

    net->tcp_fd  = net->sock_fd;
    net->sock_fd = -1;

    while (1) {
        FD_ZERO(&readset);
        FD_SET(net->tcp_fd, &readset);
        
        rt_thddog_feed("udp client recvfrom before select 1 sec");
        timeout.tv_sec = 4; timeout.tv_usec = 0;
        result = lwip_select(net->tcp_fd + 1, &readset, 0, 0, &timeout);
        rt_thddog_feed("udp client recvfrom after select");
        if (result < 0) {
            elog_e("udpclient", "recvfrom select error, err:%d, index:%d", lwip_get_error(net->tcp_fd), n);
            goto __END;
        } else if (0 == result) {
            //net_printf("udp client recvfrom select timeout!\n");
            if (net->disconnect || net->shutdown) {
                elog_i("udpclient", "disconnect/shutdown, fd:%d, index:%d", net->tcp_fd, n);
                goto __END;
            }
            continue;
        }
        
        if (FD_ISSET(net->tcp_fd, &readset)) {
            while (1) {
                int readlen = 0;
                struct sockaddr_in client_addr;
                len = sizeof(struct sockaddr);
                rt_thddog_feed("udp client recvfrom with take mutex");
                if(net->mutex) rt_mutex_take(net->mutex, RT_WAITING_FOREVER);
                //net->pos = 0;
                readlen = lwip_recvfrom(net->tcp_fd, net->buffer, net->bufsz, 0, (struct sockaddr *)&client_addr, &len);
                if(net->mutex) rt_mutex_release(net->mutex);
                if (0 == readlen) {
                    elog_w("udpclient", "close with read 0, fd:%d, index:%d", net->tcp_fd, n);
                    goto __END;
                } else if (readlen < 0) {
                    if (EAGAIN == lwip_get_error(net->tcp_fd)) {
                        break;
                    } else {
                        elog_w("udpclient", "close with read err, err:%d, index:%d", lwip_get_error(net->tcp_fd), n);
                        goto __END;
                    }
                } else {
                    if(client_addr.sin_addr.s_addr == net->to_addr.sin_addr.s_addr) {
                        net->pos = readlen;
                        rt_thddog_feed("udp server ws_vm_rcv_write");
                        if( g_ws_cfg.enable && (WS_PORT_NET == g_ws_cfg.port_type) && (port == g_ws_cfg.listen_port) ) {
                            ws_vm_rcv_write(0, net->buffer, net->pos);
                        }
                        
                        if(NET_IS_NORMAL(n)) {
                            switch(g_tcpip_cfgs[n].cfg.normal.proto_type) {
                            case PROTO_MODBUS_TCP:
                            case PROTO_MODBUS_RTU_OVER_TCP:
                                if(PROTO_SLAVE == g_tcpip_cfgs[n].cfg.normal.proto_ms) {
                                    rt_thddog_feed("xMBPortEventPost");
                                    xMBPortEventPost( BOARD_UART_MAX + n, EV_FRAME_RECEIVED );
                                } else if(PROTO_MASTER == g_tcpip_cfgs[n].cfg.normal.proto_ms) {
                                    rt_thddog_feed("xMBMasterPortEventPost");
                                    xMBMasterPortEventPost( BOARD_UART_MAX + n, EV_MASTER_FRAME_RECEIVED );
                                }
                                break;
                            case PROTO_CC_BJDC: // no support
                            case PROTO_HJT212:
                                break;
                            }
                        } else if(NET_IS_XFER(n)) {
                            __xfer_rcv_frame(n, (rt_uint8_t *)net->buffer, net->pos);
                        }
                    } else {
                        net_printf("udp client lwip_recvfrom err address\n");
                    }
                    break;
                }
            }
        }
    }

__END:
    elog_w("udpclient", "end, index:%d", n);

    rt_thddog_feed("udp client end");
    
    if(NET_IS_NORMAL(n)) {
        ;
    } else if(NET_IS_XFER(n)) {
        ;
    }

    if(net->mutex) rt_mutex_take(net->mutex, RT_WAITING_FOREVER);
    if (net->tcp_fd >= 0) {
        lwip_close(net->tcp_fd);
        net->tcp_fd = -1;
    }
    if (net->sock_fd >= 0) {
        lwip_close(net->sock_fd);
        net->sock_fd = -1;
    }
    if(net->mutex) rt_mutex_release(net->mutex);
}

#if NET_HAS_GPRS
static void onGPRSTcpRecv(rt_uint8_t btId, void *pBuffer, rt_size_t uSize)
{
    int             xfer_n;
    rt_bool_t       enable;

    for( rt_uint8_t n = 0; n < NET_HELPER_MAX; n++ ) {
        if(NET_IS_GPRS(n)) {
            net_helper_t *net = &s_net_list[n];
            enable      = g_tcpip_cfgs[n].enable;

            if(net->tcp_fd == btId && enable ) {
                memcpy(net->buffer, pBuffer, uSize);
                net->pos = uSize;
                net->idetime = rt_tick_get();
                net_printf( "onGPRSTcpRecv %d, btId = %d, size %d !\n", n, btId, uSize );
                
                if(NET_IS_NORMAL(n)) {
                    switch(g_tcpip_cfgs[n].cfg.normal.proto_type) {
                    case PROTO_MODBUS_TCP:
                    case PROTO_MODBUS_RTU_OVER_TCP:
                        if(PROTO_SLAVE == g_tcpip_cfgs[n].cfg.normal.proto_ms) {
                            rt_thddog_feed("xMBPortEventPost");
                            xMBPortEventPost( BOARD_UART_MAX + n, EV_FRAME_RECEIVED );
                        } else if(PROTO_MASTER == g_tcpip_cfgs[n].cfg.normal.proto_ms) {
                            rt_thddog_feed("xMBMasterPortEventPost");
                            xMBMasterPortEventPost( BOARD_UART_MAX + n, EV_MASTER_FRAME_RECEIVED );
                        }
                        break;
                        break;
                    case PROTO_CC_BJDC: // no 
                        break;
                    case PROTO_HJT212:
                        rt_thddog_feed("HJT212_PutBytes");
                        HJT212_PutBytes(n, net->buffer, net->pos);
                        break;
        	        }
                } else if(NET_IS_XFER(n)) {
                    __xfer_rcv_frame(n, (rt_uint8_t *)net->buffer, net->pos);
                }

                break;
            }
        }
    }
}

static void prvRunAsGPRSClient(int n)
{
    net_helper_t    *net = &s_net_list[n];
    int             xfer_n = NET_GET_XFER(n);
    rt_uint16_t     port;
    const char      *peer_host;
    tcpip_type_e    tcpip_type;
    int             wait_creg_cnt = 0;

    net->sock_fd    = -1;
    net->tcp_fd     = -1;

    port        = g_tcpip_cfgs[n].port;
    peer_host   = g_tcpip_cfgs[n].peer;
    tcpip_type  = g_tcpip_cfgs[n].tcpip_type;

    static int connect_cnt = 0;
    
    while (1) {
        rt_thddog_feed("");
        
        {
            g_tcpip_states[n].ulConnTime = 0;
            g_tcpip_states[n].eState = TCPIP_STATE_WAIT;
        }
        
        //GPRS_NState_t xNState;
        //if( bGPRS_NetState( &xNState ) && xNState.uState == 2 ) {
        if( bGPRS_GetCREG( RT_TRUE ) && (1 == g_nGPRS_CREG || 5 == g_nGPRS_CREG) ) {
            net_printf( "gprs client start connect\n");
            bGPRS_GetNetTime(RT_FALSE);
            {
                g_tcpip_states[n].eState = TCPIP_STATE_CONNING;
            }
            
            rt_thddog_feed("nGPRS_SocketConnect");
            net->tcp_fd = nGPRS_SocketConnect( 
                                        (tcpip_type==TCP_IP_TCP)?(SOCK_STREAM):(SOCK_DGRAM), 
                                        peer_host, 
                                        port, 
                                        net->bufsz, 
                                        onGPRSTcpRecv, 10000 
                                     );
            if(net->tcp_fd >= 0) {
                rt_bool_t bReg = RT_FALSE;
                
                connect_cnt = 0;
                net->idetime = rt_tick_get();

                if(NET_IS_NORMAL(n)) {
                    switch(g_tcpip_cfgs[n].cfg.normal.proto_type) {
                    case PROTO_CC_BJDC:
                        cc_bjdc_startwork(n);
                        break;
                    case PROTO_HJT212:
                        hjt212_startwork(n);
                        break;
                    }
                } else if(NET_IS_XFER(n)) {
                    ;
                }
	    		
                elog_i("gprsclient", "new conn, fd:%d, index:%d", net->tcp_fd, n);
                while( 1 ) {
                    {
                        if( g_tcpip_states[n].eState != TCPIP_STATE_CONNED ) {
                            g_tcpip_states[n].ulConnTime = (rt_millisecond_from_tick( rt_tick_get() )) / 1000;
                        }
                        g_tcpip_states[n].eState = TCPIP_STATE_CONNED;
                    }
                    
                    rt_thddog_feed("bGPRS_SocketState");
                    if( bGPRS_SocketState(net->tcp_fd) ) {
                        
                        if(!NET_IS_XFER(n)) {
                            if( 4 == g_xGPRS_SState[net->tcp_fd].uSrvState && !bReg ) {
                                if( g_gprs_work_cfg.btRegLen > 0 ) {
                                    rt_thddog_feed("nGPRS_SocketSend reg");
                                    if( nGPRS_SocketSend(net->tcp_fd, g_gprs_work_cfg.btRegBuf, g_gprs_work_cfg.btRegLen, 5000) > 0 ) {
                                        bReg = RT_TRUE;
                                    }
                                }
                            }
                        }
                        
                        if(  g_xGPRS_SState[net->tcp_fd].uSrvState != 4 && g_xGPRS_SState[net->tcp_fd].uSrvState != GPRS_SOCKET_STATE_WAIT ) {
                            elog_i("gprsclient", "closed, fd:%d, index:%d", net->tcp_fd, n);
                            rt_thddog_feed("bGPRS_SocketClose");
                            bGPRS_SocketClose(net->tcp_fd, 5000 );
                            net->tcp_fd = -1;
                            {
                                g_tcpip_states[n].ulConnTime = 0;
                                g_tcpip_states[n].eState = TCPIP_STATE_WAIT;
                            }
                            break;
                        }
                        {
                            if( g_xGPRS_SState[net->tcp_fd].uSrvState != GPRS_SOCKET_STATE_WAIT ) {
                                strcpy( g_tcpip_states[n].szLocIP, g_xGPRS_SState[net->tcp_fd].szLocIP );
                                g_tcpip_states[n].usLocPort = g_xGPRS_SState[net->tcp_fd].uLocPort;
                                strcpy( g_tcpip_states[n].szRemIP , g_xGPRS_SState[net->tcp_fd].szRemIP );
                                g_tcpip_states[n].usRemPort = g_xGPRS_SState[net->tcp_fd].uRemPort;
                            }
                        }
                    }
                    rt_thddog_feed("");
                    if(!NET_IS_XFER(n)) {
                        if( bReg && g_gprs_work_cfg.btHeartLen > 0 ) {
                            rt_uint32_t ulDiff = rt_millisecond_from_tick( rt_tick_get() - net->idetime ) / 1000;
                            if( ulDiff >= 3*120 ) {
                                goto __END;
                            } else if( ulDiff >= 120 ) {  // 120 S
                                rt_thddog_feed("nGPRS_SocketSend heart");
                                nGPRS_SocketSend(net->tcp_fd, g_gprs_work_cfg.btHeartBuf, g_gprs_work_cfg.btHeartLen, 5000);
                            }
                        }
                    }

                    rt_thddog_feed("");
                    rt_thread_delay( 5 * RT_TICK_PER_SECOND );
                }
            } else {
              	if(connect_cnt++ >= 5) {
                   	connect_cnt = 0;
                    goto __END;
              	}
	    	}
        }
        
        if(++wait_creg_cnt > 60) {
            elog_i("gprsclient", "wait 5 mins, goto end, fd:%d, index:%d", net->tcp_fd, n);
            goto __END;
        }
        rt_thddog_feed("");
        rt_thread_delay( 5 * RT_TICK_PER_SECOND );
    }

__END:
    if(NET_IS_NORMAL(n)) {
        switch(g_tcpip_cfgs[n].cfg.normal.proto_type) {
        case PROTO_CC_BJDC:
            cc_bjdc_exitwork(n);
            break;
        case PROTO_HJT212:
            hjt212_exitwork(n);
            break;
        }
    } else if(NET_IS_XFER(n)) {
        ;
    }
    elog_i("gprsclient", "end, index:%d", n);

    rt_thddog_feed("vGPRS_PowerDown");
    vGPRS_PowerDown();
    rt_thddog_feed("");
    rt_thread_delay(3 * RT_TICK_PER_SECOND);
    rt_thddog_feed("vGPRS_PowerUp");
    vGPRS_PowerUp();
    rt_thddog_feed("");
    rt_thread_delay(RT_TICK_PER_SECOND / 10);
    rt_thddog_feed("vGPRS_TermUp");
    vGPRS_TermUp();
    rt_thddog_feed("bGPRS_NetInit");
    rt_bool_t bInit = bGPRS_NetInit( g_gprs_net_cfg.szAPN, g_gprs_net_cfg.szAPNNo, g_gprs_net_cfg.szUser, g_gprs_net_cfg.szPsk );
    net_printf( "%d\n", bInit );
}

static void prvRunAsGPRSServer(int n)
{
    net_helper_t    *net = &s_net_list[n];
    int             xfer_n = NET_GET_XFER(n);
    rt_uint16_t     port;
    tcpip_type_e    tcpip_type;
    int             wait_creg_cnt = 0;

    net->sock_fd    = -1;
    net->tcp_fd     = -1;

    port        = g_tcpip_cfgs[n].port;
    tcpip_type  = g_tcpip_cfgs[n].tcpip_type;

    static int connect_cnt = 0;
    
    while (1) {
        rt_thddog_feed("");
        
        {
            g_tcpip_states[n].ulConnTime = 0;
            g_tcpip_states[n].eState = TCPIP_STATE_WAIT;
        }
        
        //GPRS_NState_t xNState;
        //if( bGPRS_NetState( &xNState ) && xNState.uState == 2 ) {
        if( bGPRS_GetCREG( RT_TRUE ) && (1 == g_nGPRS_CREG || 5 == g_nGPRS_CREG) ) {
            net_printf("gprs server start listen\n");
            bGPRS_GetNetTime(RT_FALSE);
            {
                g_tcpip_states[n].eState = TCPIP_STATE_CONNING;
            }
            
            rt_thddog_feed("nGPRS_SocketListen");
            net->tcp_fd = nGPRS_SocketListen(
                                        (tcpip_type == TCP_IP_TCP) ? (SOCK_STREAM) : (SOCK_DGRAM),
                                        port,
                                        net->bufsz, 
                                        onGPRSTcpRecv, 10000
                                    );
            rt_thddog_feed("");
            if(net->tcp_fd >= 0) {
                rt_bool_t bReg = RT_FALSE;
                
                connect_cnt = 0;
                net->idetime = rt_tick_get();
	    		
                elog_i("gprsserver", "new conn, fd:%d, index:%d", net->tcp_fd, n);
                while( 1 ) {
                    {
                        if( g_tcpip_states[n].eState != TCPIP_STATE_CONNED ) {
                            g_tcpip_states[n].ulConnTime = (rt_millisecond_from_tick( rt_tick_get() )) / 1000;
                        }
                        g_tcpip_states[n].eState = TCPIP_STATE_CONNED;
                    }
                    rt_thddog_feed("bGPRS_SocketState");
                    if( bGPRS_SocketState(net->tcp_fd) ) {
                        
                        if(!NET_IS_XFER(n)) {
                            if( 4 == g_xGPRS_SState[net->tcp_fd].uSrvState && !bReg ) {
                                if( g_gprs_work_cfg.btRegLen > 0 ) {
                                    rt_thddog_feed("nGPRS_SocketSend reg");
                                    if( nGPRS_SocketSend(net->tcp_fd, g_gprs_work_cfg.btRegBuf, g_gprs_work_cfg.btRegLen, 5000) > 0 ) {
                                        bReg = RT_TRUE;
                                    }
                                }
                            }
                        }
                        
                        if(  g_xGPRS_SState[net->tcp_fd].uSrvState != 4 && g_xGPRS_SState[net->tcp_fd].uSrvState != GPRS_SOCKET_STATE_WAIT ) {
                            elog_i("gprsserver", "closed, fd:%d, index:%d", net->tcp_fd, n);
                            rt_thddog_feed("bGPRS_SocketClose");
                            bGPRS_SocketClose(net->tcp_fd, 5000 );
                            net->tcp_fd = -1;
                            {
                                g_tcpip_states[n].ulConnTime = 0;
                                g_tcpip_states[n].eState = TCPIP_STATE_WAIT;
                            }
                            break;
                        }
                        {
                            if( g_xGPRS_SState[net->tcp_fd].uSrvState != GPRS_SOCKET_STATE_WAIT ) {
                                strcpy( g_tcpip_states[n].szLocIP, g_xGPRS_SState[net->tcp_fd].szLocIP );
                                g_tcpip_states[n].usLocPort = g_xGPRS_SState[net->tcp_fd].uLocPort;
                                strcpy( g_tcpip_states[n].szRemIP , g_xGPRS_SState[net->tcp_fd].szRemIP );
                                g_tcpip_states[n].usRemPort = g_xGPRS_SState[net->tcp_fd].uRemPort;
                            }
                        }
                    }
                    rt_thddog_feed("");
                    
                    if(!NET_IS_XFER(n)) {
                        if( bReg && g_gprs_work_cfg.btHeartLen > 0 ) {
                            rt_uint32_t ulDiff = rt_millisecond_from_tick( rt_tick_get() - net->idetime ) / 1000;
                            if( ulDiff >= 3*120 ) {
                                goto __END;
                            } else if( ulDiff >= 120 ) {  // 120 S
                                rt_thddog_feed("nGPRS_SocketSend heart");
                                nGPRS_SocketSend(net->tcp_fd, g_gprs_work_cfg.btHeartBuf, g_gprs_work_cfg.btHeartLen, 5000);
                            }
                        }
                    }
                    rt_thddog_feed("");
                    rt_thread_delay( 5 * RT_TICK_PER_SECOND );
                }
            } else {
              	if(connect_cnt++ >= 5) {
                   	connect_cnt = 0;
                    goto __END;
              	}
	    	}
        }
        
        if(++wait_creg_cnt > 60) {
            elog_i("gprsserver", "wait 5 mins, goto end, fd:%d, index:%d", net->tcp_fd, n);
            goto __END;
        }
        rt_thddog_feed("");
        rt_thread_delay( 5 * RT_TICK_PER_SECOND );
    }

__END:
    elog_i("gprsserver", "end, index:%d", n);
    rt_thddog_feed("vGPRS_PowerDown");
    vGPRS_PowerDown();
    rt_thddog_feed("");
    rt_thread_delay(3 * RT_TICK_PER_SECOND);
    rt_thddog_feed("vGPRS_PowerUp");
    vGPRS_PowerUp();
    rt_thddog_feed("");
    rt_thread_delay(RT_TICK_PER_SECOND / 10);
    rt_thddog_feed("vGPRS_TermUp");
    vGPRS_TermUp();
    rt_thddog_feed("bGPRS_NetInit");
    rt_bool_t bInit = bGPRS_NetInit( g_gprs_net_cfg.szAPN, g_gprs_net_cfg.szAPNNo, g_gprs_net_cfg.szUser, g_gprs_net_cfg.szPsk );
    net_printf( "%d\n", bInit );
}
#endif

static void _net_task(void *parameter)
{
    int n = (int)(rt_uint32_t)parameter;
    net_helper_t    *net = &s_net_list[n];
    int             xfer_n = NET_GET_XFER(n);
    tcpip_type_e    tcpip_type;
    tcpip_cs_e      tcpip_cs;

    tcpip_type  = g_tcpip_cfgs[n].tcpip_type;
    tcpip_cs    = g_tcpip_cfgs[n].tcpip_cs;

    if(NET_IS_ENET(n)) {
        while (1) {
            net->disconnect = RT_FALSE;
            net->shutdown = RT_FALSE;
            rt_thddog_feed("");
            switch (tcpip_type) {
            case TCP_IP_TCP:
            {
                if (TCPIP_CLIENT == tcpip_cs) {
                    rt_thddog_feed("prvRunAsENetTCPClient");
                    prvRunAsENetTCPClient(n);
                } else if (TCPIP_SERVER == tcpip_cs) {
                    rt_thddog_feed("prvRunAsENetTCPServer");
                    prvRunAsENetTCPServer(n);
                }
                break;
            }
            case TCP_IP_UDP:
            {
                if (TCPIP_CLIENT == tcpip_cs) {
                    rt_thddog_feed("prvRunAsENetUDPClient");
                    prvRunAsENetUDPClient(n);
                } else if (TCPIP_SERVER == tcpip_cs) {
                    rt_thddog_feed("prvRunAsENetUDPServer");
                    prvRunAsENetUDPServer(n);
                }
                break;
            }
            }
            if (net->shutdown) {
                break;
            } else {
                rt_thread_delay(2 * RT_TICK_PER_SECOND);
            }
        }
    }
#if NET_HAS_GPRS
    else if(NET_IS_GPRS(n)) {
        while (1) {
            net->disconnect = RT_FALSE;
            net->shutdown = RT_FALSE;
            rt_thddog_feed("");
            switch (tcpip_type) {
            case TCP_IP_TCP:
            case TCP_IP_UDP:
            {
                if (TCPIP_CLIENT == tcpip_cs) {
                    rt_thddog_feed("prvRunAsGPRSClient");
                    prvRunAsGPRSClient(n);
                } else if (TCPIP_SERVER == tcpip_cs) {
                    rt_thddog_feed("prvRunAsGPRSServer");
                    prvRunAsGPRSServer(n);
                }
                break;
            }
            }
            if (net->shutdown) {
                break;
            } else {
                rt_thread_delay(RT_TICK_PER_SECOND);
            }
        }
    }
#endif

    net->thread = RT_NULL;
}

