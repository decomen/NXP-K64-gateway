
#include "request.h"
#include "session.h"
#include "mimetype.h"
#include "util.h"
#include "module.h"
#include "websocket.h"

static struct rt_mutex s_websock_mutex;
static struct webnet_session *ws_session_cur = RT_NULL;

#include <ws_console.c>

static void websocket_handler_pack(websocket_pack_t *pack);

int webnet_module_ws(struct webnet_session *session, int event)
{
    switch (event) {
    case WEBNET_EVENT_INIT:
        rt_mutex_init(&s_websock_mutex, "websock", RT_IPC_FLAG_PRIO);
        ws_vm_init();
        if (!g_host_cfg.bDebug) {
            ws_console_init();
        }
        return WEBNET_MODULE_CONTINUE;
    default:
        break;
    }

    return WEBNET_MODULE_CONTINUE;
}

rt_bool_t websocket_ready(void)
{
    return (ws_session_cur != RT_NULL);
}

void websocket_set(struct webnet_session *session)
{
    rt_mutex_take(&s_websock_mutex, RT_WAITING_FOREVER);
    {
        if (ws_session_cur != session) {
            ws_session_cur = session;
        }
    }
    rt_mutex_release( &s_websock_mutex );
}

void websocket_close(struct webnet_session *session)
{
    rt_mutex_take(&s_websock_mutex, RT_WAITING_FOREVER);
    {
        if (ws_session_cur == session) {
            g_ws_cfg.enable = RT_FALSE;
            ws_session_cur = RT_NULL;
        }
    }
    rt_mutex_release(&s_websock_mutex);
}

void websocket_handler_data(struct webnet_session *session, const rt_uint8_t *data, rt_size_t len)
{
    rt_mutex_take(&s_websock_mutex, RT_WAITING_FOREVER);
    {
        websocket_pack_t pack;
        memset(&pack, 0, sizeof(pack));
        pack.data = (rt_uint8_t *)data;
        memcpy(&pack.head, data, sizeof(websocket_head_t));
        pack.mask = pack.head.mask;
        pack.data += sizeof(websocket_head_t);
        switch (pack.head.op) {
        case 0x00:  // Continuation Fram
        case 0x01:  // Text Frame
        case 0x02:  // Binary Frame
        {
            if (pack.head.len > 0) {
                if (pack.head.len <= 125) {
                    pack.len = pack.head.len;
                } else if (pack.head.len == 126) {
                    pack.len = (((rt_uint32_t)pack.data[0] << 8) | pack.data[1]);
                    pack.data += 2;
                } else if (pack.head.len == 127) {
                    pack.len = (((rt_uint32_t)pack.data[0] << 24) | ((rt_uint32_t)pack.data[1] << 16) | ((rt_uint32_t)pack.data[2] << 8) | pack.data[0]);
                    pack.data += 4;
                }
                if (pack.mask) {
                    int mj = 0;
                    memcpy(pack.masks, pack.data, 4);
                    pack.data += 4;
                    for (int mi = 0; mi < pack.len; mi++) {
                        pack.data[mi] ^= pack.masks[mj];
                        if (++mj >= 4) mj = 0;
                    }
                }
                websocket_handler_pack(&pack);
            }
            break;
        }
        case 0x08:  // Connection Close Frame
        {
            session->websk.state = WEB_SK_CLOSED;
            break;
        }
        case 0x09:  // Ping
        {
            websocket_send_pong(session);
            break;
        }
        case 0x0A:  // Pong, do nothing
        {
            break;
        }
        default:
        {
            session->websk.state = WEB_SK_CLOSED;
        }
        }
    }
    rt_mutex_release(&s_websock_mutex);
}

static void __send_data_full(struct webnet_session *session, websocket_head_t *head, const rt_uint8_t *data, rt_size_t len, rt_bool_t isrcv)
{
    rt_mutex_take(&s_websock_mutex, RT_WAITING_FOREVER);
    if( session ) {
        rt_uint8_t *buffer = session->buffer;
        websocket_top_t top = { .rcv = (isrcv ? 1 : 0) };

        if (data != RT_NULL) {
            len++;
        }
        if (len <= 125) {
            head->len = len;
        } else if (len <= 65535) {
            head->len = 126;
        } else if (len > 65535) {
            head->len = 127;
        }

        memcpy(buffer, head, sizeof(websocket_head_t));
        buffer += sizeof(websocket_head_t);
        if (len > 125 && len <= 65535) {
            *buffer++ = (rt_uint8_t)((len >> 8) & 0xFF);
            *buffer++ = (rt_uint8_t)(len & 0xFF);
        } else if (len > 65535) {
            *buffer++ = (rt_uint8_t)((len >> 24) & 0xFF);
            *buffer++ = (rt_uint8_t)((len >> 16) & 0xFF);
            *buffer++ = (rt_uint8_t)((len >> 8) & 0xFF);
            *buffer++ = (rt_uint8_t)(len & 0xFF);
        }
        if (data != RT_NULL) {
            memcpy(buffer, &top, 1);
            buffer += 1;
            memcpy(buffer, data, len - 1);
            buffer += len - 1;
        }
        webnet_session_write(session, (const rt_uint8_t *)session->buffer, buffer - session->buffer);
    }
    rt_mutex_release(&s_websock_mutex);
}

void websocket_send_pong(struct webnet_session *session)
{
    rt_mutex_take(&s_websock_mutex, RT_WAITING_FOREVER);
    {
        websocket_head_t head = { .op = 0x0A, .fin = 0x01, .mask = 0 };
        if( !session ) session = ws_session_cur;
        __send_data_full(session, &head, RT_NULL, 0, RT_FALSE);
    }
    rt_mutex_release(&s_websock_mutex);
}

void websocket_send_data(struct webnet_session *session, const rt_uint8_t *data, rt_size_t len, rt_bool_t isrcv)
{
    rt_mutex_take( &s_websock_mutex, RT_WAITING_FOREVER );
    {
        websocket_head_t head = { .op = 0x02, .fin = 0x01, .mask = 0 };
        if( !session ) session = ws_session_cur;
        __send_data_full(session, &head, data, len, isrcv);
    }
    rt_mutex_release( &s_websock_mutex );
}

static void websocket_handler_pack(websocket_pack_t *pack)
{
    // 接收数据存入ws虚拟机
    ws_vm_recv_pack(pack);

    if (g_ws_cfg.enable && g_ws_cfg.port_type != WS_PORT_SHELL) {
        switch (g_ws_cfg.port_type) {
        case WS_PORT_RS1:
        case WS_PORT_RS2:
        case WS_PORT_RS3:
        case WS_PORT_RS4:
        {
            int port = nUartGetInstance(WS_PORT_TO_DEV(g_ws_cfg.port_type));
            if (port >= 0) {
                // add 2017-04-05 调试串口模拟接收到数据,其他接口向外发送数据
                if(g_host_cfg.bDebug && WS_PORT_RS1 == g_ws_cfg.port_type) {
                    rt_hw_serial_isr(g_serials[port], RT_SERIAL_EVENT_RX_IND);
                } else {
                    //清空虚拟机,直接发送数据
                    ws_vm_rx_clear();
                    if (g_serials[port]->parent.write != RT_NULL && g_serials[port]->parent.open_flag) {
                        struct k64_serial_device *node = ((struct k64_serial_device *)g_serials[port]->parent.user_data);
                        if( node->cfg->uart_type == UART_TYPE_485 ) {
                            rt_pin_write( node->pin_en, PIN_LOW );
                            rt_thread_delay( rt_uart_485_gettx_delay(node->cfg->port_cfg.baud_rate) * RT_TICK_PER_SECOND / 1000 );
                        }
                        g_serials[port]->parent.write(&g_serials[port]->parent, 0, pack->data, pack->len);
                        if( node->cfg->uart_type == UART_TYPE_485 ) {
                            rt_thread_delay( rt_uart_485_getrx_delay(node->cfg->port_cfg.baud_rate) *  RT_TICK_PER_SECOND / 1000 );
                            rt_pin_write( node->pin_en, PIN_HIGH );
                        }
                    }
                }
            }
            break;
        }
        case WS_PORT_NET:
        {
            extern void ws_net_try_send(rt_uint16_t port, void *buffer, rt_size_t size);
            //清空虚拟机,直接发送数据
            ws_vm_rx_clear();
            ws_net_try_send(g_ws_cfg.listen_port, (void *)pack->data, pack->len);
            break;
        }
        case WS_PORT_ZIGBEE:
        {
            // add 2017-04-05 zigbee 广播发出数据
            //清空虚拟机,直接发送数据
            ws_vm_rx_clear();
            if (g_serials[BOARD_ZIGBEE_UART]->parent.write != RT_NULL && g_serials[BOARD_ZIGBEE_UART]->parent.open_flag) {
                ucZigbeeMode(ZIGBEE_MSG_MODE_BROAD);
                g_serials[BOARD_ZIGBEE_UART]->parent.write(&g_serials[BOARD_ZIGBEE_UART]->parent, 0, pack->data, pack->len);
                rt_thread_delay(200 * RT_TICK_PER_SECOND / 1000);
                ucZigbeeMode(ZIGBEE_MSG_MODE_SINGLE);
            }
            break;
        }
        case WS_PORT_GPRS:
        {
            //清空虚拟机,直接发送数据
            ws_vm_rx_clear();
            if (g_serials[BOARD_GPRS_UART]->parent.write != RT_NULL && g_serials[BOARD_GPRS_UART]->parent.open_flag) {
                g_serials[BOARD_GPRS_UART]->parent.write(&g_serials[BOARD_GPRS_UART]->parent, 0, pack->data, pack->len);
            }
            break;
        }
        case WS_PORT_LORA:
            break;
        }
    }

}

