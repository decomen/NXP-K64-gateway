/**
 * example of adding sdccp serial external library
 */

#include "board.h"
#include "dlt645.h"

#define sdccp_serial_printf(_fmt,...)   rt_kprintf( "[sdccp serial]" _fmt, ##__VA_ARGS__ )

// 通过 malloc 分配 buf 空间大小
typedef struct {
    rt_uint16_t     size;
    rt_uint16_t     n;
    rt_uint8_t      buf[1];
} sdccp_serial_queue_t;

typedef struct {
    rt_uint16_t bufsz;
    rt_uint16_t timeout;
    rt_serial_t *serial;
    struct k64_serial_device *node;
} sdccp_serial_cfg_t;
static sdccp_serial_cfg_t s_sdccp_serial_cfgs[BOARD_UART_MAX] = {{0,0}};
static rt_timer_t s_sdccp_serial_timers[BOARD_UART_MAX] = {RT_NULL};
static sdccp_serial_queue_t *s_sdccp_serial_queue[BOARD_UART_MAX] = {RT_NULL};

#define EVENT_SDCCP_SERIAL_TIMEOUT(n)       (1<<(n))
#define EVENT_SDCCP_SERIAL_ANY              (~(0xFFFFFFFFUL << BOARD_UART_MAX))
static rt_thread_t _thread_sdccp_serial_timer_irq = RT_NULL;
static rt_event_t _event_sdccp_serial_timer = RT_NULL;

static void _sdccp_serial_timer_irq(void* parameter);

static rt_bool_t _sdccp_serial_busy(int port)
{
    return RT_TRUE;
}

static void _sdccp_serial_recv_handle(int port, rt_uint8_t value)
{
    if (s_sdccp_serial_queue[port] && s_sdccp_serial_queue[port]->n < s_sdccp_serial_queue[port]->size) {
        int num = s_sdccp_serial_queue[port]->n;
        s_sdccp_serial_queue[port]->buf[num] = value;
        s_sdccp_serial_queue[port]->n = num+1;
    }
    if (s_sdccp_serial_timers[port] ) {
        rt_timer_start(s_sdccp_serial_timers[port]);
        //rt_timer_stop(s_sdccp_serial_timers[port]);
    }
}

static rt_err_t _sdccp_serial_rx_ind(rt_device_t dev, rt_size_t size)
{
    struct k64_serial_device *node = ((struct k64_serial_device *)((rt_serial_t *)dev)->parent.user_data);
    int port = node->instance;
    if (s_sdccp_serial_queue[port] && s_sdccp_serial_queue[port]->n < s_sdccp_serial_queue[port]->size) {
        rt_uint8_t ucByte = 0;
        while (dev->read(dev, 0, &ucByte, 1) > 0) {
            int num = s_sdccp_serial_queue[port]->n;
            s_sdccp_serial_queue[port]->buf[num] = ucByte;
            s_sdccp_serial_queue[port]->n = num+1;
        }
    }
    if (s_sdccp_serial_timers[port] ) {
        rt_timer_start(s_sdccp_serial_timers[port]);
        //rt_timer_stop(s_sdccp_serial_timers[port]);
    }
    return RT_EOK;
}

static rt_err_t _sdccp_serial_ide_ind(rt_device_t dev, rt_size_t size)
{
    struct k64_serial_device *node = ((struct k64_serial_device *)((rt_serial_t *)dev)->parent.user_data);
    int port = node->instance;
    if (s_sdccp_serial_timers[port] ) {
        rt_timer_start(s_sdccp_serial_timers[port]);
    }
    return RT_EOK;
}

static void _sdccp_serial_timeout_do(int port)
{
    if (s_sdccp_serial_queue[port]) {
        if (PROTO_DLT645 == g_uart_cfgs[port].proto_type) {
            Dlt645_PutBytes(nUartGetIndex(port), (mdBYTE *)s_sdccp_serial_queue[port]->buf, s_sdccp_serial_queue[port]->n);
        }
        s_sdccp_serial_queue[port]->n = 0;
    }
}

static void _sdccp_serial_timeout_handle(void *parameter)
{
    int port = (int)(rt_uint32_t)parameter;
    if (s_sdccp_serial_queue[port]) {
        if(_event_sdccp_serial_timer != RT_NULL) {
            rt_event_send(_event_sdccp_serial_timer, EVENT_SDCCP_SERIAL_TIMEOUT(port));
        }
    }
}

// 目前有: PROTO_DLT645
rt_bool_t sdccp_serial_check(int index)
{
    int port = nUartGetInstance(index);
    if (port >= 0) {
        if (PROTO_DLT645 == g_uart_cfgs[port].proto_type
#ifdef RT_USING_CONSOLE
            && (!g_host_cfg.bDebug || BOARD_CONSOLE_USART != port)
#endif
           ) {

            return (!g_xfer_net_dst_uart_occ[port]);
        }
    }

    return RT_FALSE;
}

void sdccp_serial_rx_enable(int index)
{
    int port = nUartGetInstance(index);
    if (s_sdccp_serial_cfgs[port].node) {
        struct k64_serial_device *node = s_sdccp_serial_cfgs[port].node;
        if( node->cfg->uart_type == UART_TYPE_485 ) {
            rt_thread_delay( rt_uart_485_getrx_delay(node->cfg->port_cfg.baud_rate) *  RT_TICK_PER_SECOND / 1000 );
            rt_pin_write( node->pin_en, PIN_HIGH );
        }
    }
}

void sdccp_serial_tx_enable(int index)
{
    int port = nUartGetInstance(index);
    struct k64_serial_device *node = s_sdccp_serial_cfgs[port].node;
    if( node->cfg->uart_type == UART_TYPE_485 ) {
        rt_pin_write( node->pin_en, PIN_LOW );
        rt_thread_delay( rt_uart_485_gettx_delay(node->cfg->port_cfg.baud_rate) * RT_TICK_PER_SECOND / 1000 );
    }
}

void sdccp_serial_senddata(int index, const void *data, rt_size_t size)
{
    int port = nUartGetInstance(index);
    if (s_sdccp_serial_cfgs[port].node) {
        rt_serial_t *serial = s_sdccp_serial_cfgs[port].serial;
        sdccp_serial_tx_enable(index);
        serial->parent.write(&serial->parent, 0, data, size);
        sdccp_serial_rx_enable(index);
    }
}

static void __dlt645_send(int index,mdBYTE *pdata,mdBYTE len);

void sdccp_serial_openall(void)
{
    for (int index = 0; index < 4; index++) {
        if (sdccp_serial_check(index)) {
            int port = nUartGetInstance(index);
            rt_serial_t *serial = (rt_serial_t *)rt_device_find(BOARD_UART_DEV_NAME(port));
            if (RT_NULL == serial) {
                rt_kprintf("sdccp_serial_openall:rt_device_find('%s') falied..\n", BOARD_UART_DEV_NAME(port));
                continue ;
            }
            struct k64_serial_device *node = ((struct k64_serial_device *)serial->parent.user_data);
            serial->config.baud_rate = g_uart_cfgs[port].port_cfg.baud_rate;
            serial->config.data_bits = g_uart_cfgs[port].port_cfg.data_bits;
            serial->config.stop_bits = g_uart_cfgs[port].port_cfg.stop_bits;
            serial->config.parity    = g_uart_cfgs[port].port_cfg.parity;
            serial->parent.close(&serial->parent);
            serial->parent.rx_indicate = RT_NULL;
            serial->parent.ide_indicate = RT_NULL;
            node->cfg->busy_handle = RT_NULL;
            node->cfg->recv_handle = RT_NULL;
            serial->ops->configure(serial, &serial->config);

            s_sdccp_serial_cfgs[port].serial = serial;
            s_sdccp_serial_cfgs[port].node = node;

            if (s_sdccp_serial_timers[port]) {
                rt_timer_delete(s_sdccp_serial_timers[port]);
            }
            {
                if (s_sdccp_serial_cfgs[port].timeout<=1) {
                    s_sdccp_serial_cfgs[port].timeout = 20;
                }
                BOARD_CREAT_NAME(szTime, "sdccp_s%d", port);
                s_sdccp_serial_timers[port] = \
                    rt_timer_create(szTime, _sdccp_serial_timeout_handle,
                                         (void *)(uint32_t)port,
                                         (rt_tick_t)rt_tick_from_millisecond(s_sdccp_serial_cfgs[port].timeout),
                                         RT_TIMER_FLAG_ONE_SHOT
                                         );
            }
            if (s_sdccp_serial_queue[port]) {
                rt_free(s_sdccp_serial_queue[port]);
            }
            {
                if (s_sdccp_serial_cfgs[port].bufsz<=1) s_sdccp_serial_cfgs[port].bufsz = 64;
                int size = sizeof(sdccp_serial_queue_t)+s_sdccp_serial_cfgs[port].bufsz;
                s_sdccp_serial_queue[port] = rt_calloc(1, size);
                if (s_sdccp_serial_queue[port]) {
                    s_sdccp_serial_queue[port]->size = s_sdccp_serial_cfgs[port].bufsz;
                    s_sdccp_serial_queue[port]->n = 0;
                }
            }
            
            if ( RT_EOK == serial->parent.open(&serial->parent, RT_DEVICE_OFLAG_RDWR | RT_DEVICE_FLAG_INT_RX)) {
                //serial->parent.rx_indicate = _sdccp_serial_rx_ind;
                //serial->parent.ide_indicate = _sdccp_serial_ide_ind;
                node->cfg->busy_handle = _sdccp_serial_busy;
                node->cfg->recv_handle = _sdccp_serial_recv_handle;
            }

            sdccp_serial_rx_enable(index);

            if (PROTO_DLT645 == g_uart_cfgs[port].proto_type) {
                dlt645_init(index, __dlt645_send);
            }
        }
    }

    /* software initialize */
    if( RT_NULL == _event_sdccp_serial_timer ) {
        _event_sdccp_serial_timer = rt_event_create("ser_time", RT_IPC_FLAG_PRIO);
    }

    if( RT_NULL == _thread_sdccp_serial_timer_irq ) {
        _thread_sdccp_serial_timer_irq = rt_thread_create("ser_time", 
                                                    _sdccp_serial_timer_irq, 
                                                    RT_NULL, 
                                                    0x400, 
                                                    10, 10 );
        if(_thread_sdccp_serial_timer_irq != RT_NULL) {
            rt_thread_startup(_thread_sdccp_serial_timer_irq);
        }
    }
}

static void _sdccp_serial_timer_irq(void* parameter)
{
    rt_uint32_t recved_event = 0;
    while(1) {
        if(_event_sdccp_serial_timer != RT_NULL) {
            rt_event_recv(
                _event_sdccp_serial_timer, 
                EVENT_SDCCP_SERIAL_ANY, RT_EVENT_FLAG_OR | RT_EVENT_FLAG_CLEAR,
                RT_WAITING_FOREVER, &recved_event);
            for(int port = 0; port < BOARD_UART_MAX; port++ ) {
                if( recved_event & EVENT_SDCCP_SERIAL_TIMEOUT(port)) {
                    _sdccp_serial_timeout_do(port);
                }
            }
        } else {
            rt_thread_delay(1000);
        }
    }
}

static void __dlt645_send(int index,mdBYTE *pdata,mdBYTE len)
{
    sdccp_serial_senddata(index, (const void *)pdata, len);
}


void sdccp_serial_closeall(void)
{
    for (int index = 0; index < 4; index++) {
        if (sdccp_serial_check(index)) {
            int port = nUartGetInstance(index);
            rt_serial_t *serial = (rt_serial_t *)rt_device_find(BOARD_UART_DEV_NAME(port));
            if (RT_NULL == serial) {
                rt_kprintf("sdccp_serial_closeall:rt_device_find('%s') falied..\n", BOARD_UART_DEV_NAME(port));
                continue ;
            }
            serial->parent.close(&serial->parent);
            serial->parent.rx_indicate = RT_NULL;
            serial->parent.ide_indicate = RT_NULL;
        }
    }
}

