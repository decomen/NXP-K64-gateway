
#include "rtdevice.h"
#include "board.h"

#define XFER_DST_SERIAL_EVENT_RCV_FRAME(n)      (1<<(n))
#define XFER_DST_SERIAL_EVENT_RCV_FRAME_ANY     (~(0xFFFFFFFFUL << BOARD_UART_MAX))

static rt_uint8_t *dst_rcvbuffer[BOARD_UART_MAX];
static rt_uint16_t dst_rcvbuffer_pos[BOARD_UART_MAX];
static rt_serial_t *dst_serials[BOARD_UART_MAX];
static rt_timer_t dst_serial_timers[BOARD_UART_MAX];

static rt_event_t xfer_dst_serial_event;
static rt_thread_t xfer_dst_serial_thread;

static void xfer_dst_serial_task(void *parameter);
static rt_err_t xfer_dst_serial_rx_handle(rt_device_t dev, rt_size_t size);
static void xfer_dst_serial_timeout_handle(void *parameter);

rt_bool_t xfer_dst_serial_open(int n)
{
    xfer_dst_uart_cfg *pucfg = RT_NULL;
    rt_int8_t dst_type = -1;
    dst_rcvbuffer_pos[n] = 0;
    RT_KERNEL_FREE((void *)dst_rcvbuffer[n]);
    dst_rcvbuffer[n] = RT_KERNEL_CALLOC(XFER_BUF_SIZE);
    xfer_cfg_t *xfer = g_xfer_net_dst_uart_map[n]>=0?&g_tcpip_cfgs[g_xfer_net_dst_uart_map[n]].cfg.xfer:RT_NULL;

    dst_serials[n] = (rt_serial_t *)rt_device_find(BOARD_UART_DEV_NAME(n));
    
    if(xfer && XFER_M_GW == xfer->mode) {
        dst_type = xfer->cfg.gw.dst_type;
        pucfg = &xfer->cfg.gw.dst_cfg.uart_cfg;
    } else if(xfer && XFER_M_TRT == xfer->mode) {
        dst_type = xfer->cfg.trt.dst_type;
        pucfg = &xfer->cfg.trt.dst_cfg.uart_cfg;
    } else {
        if(n != BOARD_ZIGBEE_UART) {
            dst_serials[n]->ops->configure(dst_serials[n], &(dst_serials[n]->config));
            if (RT_EOK == dst_serials[n]->parent.open(&dst_serials[n]->parent,
                                                        RT_DEVICE_OFLAG_RDWR | RT_DEVICE_FLAG_INT_RX)) {
                dst_serials[n]->parent.rx_indicate = xfer_dst_serial_rx_handle;
            } else {
                return FALSE;
            }
        }
    }

    if (dst_type >= PROTO_DEV_RS1 && dst_type <= PROTO_DEV_RS_MAX) {

        struct k64_serial_device *node = ((struct k64_serial_device *)dst_serials[n]->parent.user_data);

        if (node->cfg->uart_type == UART_TYPE_485) {
            rt_pin_mode(node->pin_en, PIN_MODE_OUTPUT);
            rt_pin_write(node->pin_en, PIN_HIGH);
        }

        /* set serial configure parameter */
        dst_serials[n]->config.baud_rate = pucfg->cfg.baud_rate;
        dst_serials[n]->config.stop_bits = STOP_BITS_1;

        dst_serials[n]->config.data_bits = pucfg->cfg.data_bits;
        dst_serials[n]->config.parity = pucfg->cfg.parity;
        /* set serial configure */
        dst_serials[n]->ops->configure(dst_serials[n], &(dst_serials[n]->config));

        /* open serial device */
        if (RT_EOK == dst_serials[n]->parent.open(&dst_serials[n]->parent,
                                                    RT_DEVICE_OFLAG_RDWR | RT_DEVICE_FLAG_INT_RX)) {
            dst_serials[n]->parent.rx_indicate = xfer_dst_serial_rx_handle;
        } else {
            return FALSE;
        }
    }

    if (RT_NULL == dst_serial_timers[n]) {

        BOARD_CREAT_NAME(szTime, "xfdtm_%d", n);
        dst_serial_timers[n] = \
            rt_timer_create(szTime, xfer_dst_serial_timeout_handle,
                                 (void *)(uint32_t)n,
                                 (rt_tick_t)1000,
                                 RT_TIMER_FLAG_ONE_SHOT
                                 );
    }

    if (RT_NULL != dst_serial_timers[n]) {
        rt_tick_t timer_tick = rt_tick_from_millisecond(50);
        rt_timer_control(dst_serial_timers[n], RT_TIMER_CTRL_SET_TIME, &timer_tick);
    }

    /* software initialize */
    if (RT_NULL == xfer_dst_serial_event) {
        xfer_dst_serial_event = rt_event_create("xfer_ser", RT_IPC_FLAG_PRIO);
    }

    if (RT_NULL == xfer_dst_serial_thread) {
        xfer_dst_serial_thread = \
            rt_thread_create("xfer_ser", xfer_dst_serial_task, RT_NULL, 0x400, 10, 10);

        if (xfer_dst_serial_thread != RT_NULL) {
            rt_thread_startup(xfer_dst_serial_thread);
        }
    }

    return TRUE;
}

void xfer_dst_serial_close(int n)
{
    RT_KERNEL_FREE((void *)dst_rcvbuffer[n]);
    dst_rcvbuffer[n] = RT_NULL;
    dst_serials[n]->parent.close(&(dst_serials[n]->parent));
    rt_timer_stop(dst_serial_timers[n]);
}

void xfer_dst_serial_send(int n, const void *buffer, rt_size_t size)
{
    if (dst_serials[n] && dst_serials[n]->parent.write != RT_NULL && dst_serials[n]->parent.open_flag) {
        struct k64_serial_device *node = ((struct k64_serial_device *)dst_serials[n]->parent.user_data);
        if (node->cfg->uart_type == UART_TYPE_485) {
            /* switch 485 to transmit mode */
            rt_pin_write(node->pin_en, PIN_LOW);
            rt_thread_delay(rt_uart_485_gettx_delay(node->cfg->port_cfg.baud_rate) * RT_TICK_PER_SECOND / 1000);
        }

        dst_serials[n]->parent.write(&(dst_serials[n]->parent), 0, buffer, size);

        if (node->cfg->uart_type == UART_TYPE_485) {
            /* switch 485 to receive mode */
            rt_thread_delay(rt_uart_485_getrx_delay(node->cfg->port_cfg.baud_rate) *  RT_TICK_PER_SECOND / 1000);
            rt_pin_write(node->pin_en, PIN_HIGH);
        }
    }
}

static void xfer_dst_serial_task(void *parameter)
{
    rt_uint32_t recved_event = 0;

    while (1) {
        if(xfer_dst_serial_event) {
            rt_event_recv(xfer_dst_serial_event, XFER_DST_SERIAL_EVENT_RCV_FRAME_ANY, RT_EVENT_FLAG_OR | RT_EVENT_FLAG_CLEAR, RT_WAITING_FOREVER, &recved_event);
            for(int n = 0; n < BOARD_UART_MAX; n++ ) {
                if( recved_event & XFER_DST_SERIAL_EVENT_RCV_FRAME(n)) {
                    if(g_xfer_net_dst_uart_trt[n] || g_xfer_net_dst_uart_dtu[n]) {
                        vZigbee_SendBuffer(dst_rcvbuffer[n], dst_rcvbuffer_pos[n]);
                    } else {
                        if(g_xfer_net_dst_uart_map[n] >= 0) {
                            xfer_net_send(g_xfer_net_dst_uart_map[n], dst_rcvbuffer[n], dst_rcvbuffer_pos[n]);
                        }
                    }
                    dst_rcvbuffer_pos[n] = 0;
                }
            }
        } else {
            rt_thread_delay(1000);
        }
    }
}

void xfer_dst_serial_rcv_clear(int n)
{
    dst_rcvbuffer_pos[n] = 0;
}

void xfer_dst_serial_rcv_byte(int n, rt_uint8_t ucByte, rt_bool_t flag )
{
    if (dst_rcvbuffer_pos[n] < XFER_BUF_SIZE) {
        dst_rcvbuffer[n][dst_rcvbuffer_pos[n]++] = ucByte;
    }

    if(dst_serial_timers[n] && flag) {
        rt_timer_start(dst_serial_timers[n]);
    }
}

static rt_err_t xfer_dst_serial_rx_handle(rt_device_t dev, rt_size_t size)
{
    rt_uint8_t ucByte = 0;
    struct k64_serial_device *node = ((struct k64_serial_device *)((rt_serial_t *)dev)->parent.user_data);
    for (int n = 0; n < size; n++) {
        dev->read(dev, 0, &ucByte, 1);
        xfer_dst_serial_rcv_byte(node->instance, ucByte, RT_TRUE);
    }
    return RT_EOK;
}

static void xfer_dst_serial_timeout_handle(void *parameter)
{
    int n = (int)(rt_uint32_t)parameter;

    rt_timer_stop(dst_serial_timers[n]);
    if (xfer_dst_serial_event != RT_NULL) {
        rt_event_send(xfer_dst_serial_event, XFER_DST_SERIAL_EVENT_RCV_FRAME(n));
    }
}

