
#include <queue.h>
#include <board.h>

#define WS_VM_RX_BUFSZ        (512)
#define WS_VM_TX_BUFSZ        (512)
typedef struct {
    rt_base_t nFront;
    rt_base_t nRear;
    rt_uint8_t pData[WS_VM_RX_BUFSZ+1];
} ws_vm_rx_queue_t;

typedef struct {
    rt_base_t nFront;
    rt_base_t nRear;
    rt_uint8_t pData[WS_VM_TX_BUFSZ+1];
} ws_vm_tx_queue_t;

#define WS_VM_EVENT_READY   (0x01)

static ws_vm_rx_queue_t _ws_vm_rx_queue;
static ws_vm_tx_queue_t _ws_vm_tx_rcv_queue;
static ws_vm_tx_queue_t _ws_vm_tx_snd_queue;
static struct rt_event _ws_vm_rcv_event;
static struct rt_event _ws_vm_snd_event;
static rt_bool_t _ws_init = RT_FALSE;

void ws_vm_recv_pack( websocket_pack_t *pack )
{
    if( g_ws_cfg.enable ) {
        rt_base_t level = rt_hw_interrupt_disable();
        rt_size_t n = 0;
        for( n = 0; n < pack->len; n++ ) {
            if( !bQueueWrite( _ws_vm_rx_queue, pack->data[n] ) ) {
                break;
            }
        }
        rt_hw_interrupt_enable(level);
    }

    if( WS_PORT_SHELL == g_ws_cfg.port_type ) {
        ws_console_recv_pack();
    }
}

rt_size_t ws_vm_buflen( void )
{
    return nQueueLen(_ws_vm_rx_queue);
}

// 需要外部 rt_hw_interrupt_disable
int ws_vm_getc( void )
{
    if( !_ws_init ) return -1;

    rt_uint8_t ch = 0;
    if( !bQueueRead(_ws_vm_rx_queue, ch) ) {
        return -1;
    }

    return ch;
}

rt_bool_t ws_vm_rcv_putc( rt_uint8_t ch )
{
    if( !_ws_init ) return RT_FALSE;

    bQueueWrite(_ws_vm_tx_rcv_queue, ch);
    rt_event_send( &_ws_vm_rcv_event, WS_VM_EVENT_READY );
    return RT_TRUE;
}
rt_bool_t ws_vm_snd_putc( rt_uint8_t ch )
{
    if( !_ws_init ) return RT_FALSE;

    bQueueWrite(_ws_vm_tx_snd_queue, ch);
    rt_event_send( &_ws_vm_snd_event, WS_VM_EVENT_READY );
    return RT_TRUE;
}

rt_size_t ws_vm_read( rt_off_t pos, void *buffer, rt_size_t size )
{
    if( !_ws_init ) return 0;

    rt_size_t n = 0;
    if( g_ws_cfg.enable ) {
        rt_uint8_t *data = (rt_uint8_t *)buffer;
        rt_base_t level = rt_hw_interrupt_disable();
        for( n = 0; n < size; n++ ) {
            if( !bQueueRead(_ws_vm_rx_queue, data[n+pos]) ) break;
        }
        rt_hw_interrupt_enable(level);
    }
    return n;
}

rt_size_t ws_vm_rcv_write( rt_off_t pos, void *buffer, rt_size_t size )
{
    if( !_ws_init ) return 0;

    rt_size_t n = 0;
    if( g_ws_cfg.enable && size > 0 ) {
        rt_uint8_t *data = (rt_uint8_t *)buffer;
        rt_base_t level = rt_hw_interrupt_disable();
        for( n = 0; n < size; n++ ) {
            if( !bQueueWrite(_ws_vm_tx_rcv_queue, data[n+pos]) ) break;
        }
        rt_hw_interrupt_enable(level);
        rt_event_send( &_ws_vm_rcv_event, WS_VM_EVENT_READY );
    }
    return n;
}

rt_size_t ws_vm_snd_write( rt_off_t pos, void *buffer, rt_size_t size )
{
    if( !_ws_init ) return 0;

    rt_size_t n = 0;
    if( g_ws_cfg.enable && size > 0 ) {
        rt_uint8_t *data = (rt_uint8_t *)buffer;
        rt_base_t level = rt_hw_interrupt_disable();
        for( n = 0; n < size; n++ ) {
            if( !bQueueWrite(_ws_vm_tx_snd_queue, data[n+pos]) ) break;
        }
        rt_hw_interrupt_enable(level);
        rt_event_send( &_ws_vm_snd_event, WS_VM_EVENT_READY );
    }
    return n;
}

void ws_vm_rx_clear( void )
{
    rt_base_t level = rt_hw_interrupt_disable();
    vQueueClear( _ws_vm_rx_queue );
    rt_hw_interrupt_enable(level);
}

void ws_vm_tx_clear( void )
{
    rt_base_t level = rt_hw_interrupt_disable();
    vQueueClear( _ws_vm_tx_rcv_queue );
    vQueueClear( _ws_vm_tx_snd_queue );
    rt_hw_interrupt_enable(level);
}

void ws_vm_clear( void )
{
    rt_base_t level = rt_hw_interrupt_disable();
    vQueueClear( _ws_vm_rx_queue );
    vQueueClear( _ws_vm_tx_rcv_queue );
    vQueueClear( _ws_vm_tx_snd_queue );
    rt_hw_interrupt_enable(level);
}

static void _ws_vm_trans_rcv_task( void* parameter );
static void _ws_vm_trans_snd_task( void* parameter );

void ws_vm_init( void )
{
    ws_vm_clear();

    rt_event_init( &_ws_vm_rcv_event, "wsrcvevt", RT_IPC_FLAG_PRIO );
    rt_event_init( &_ws_vm_snd_event, "wssndevt", RT_IPC_FLAG_PRIO );
    rt_thread_t ws_rcv_thread = rt_thread_create( "wsrcvtsk", _ws_vm_trans_rcv_task, RT_NULL, 0x200, 10, 10 );
    if( ws_rcv_thread != RT_NULL ) {
        rt_thddog_register(ws_rcv_thread, 30);
        rt_thread_startup( ws_rcv_thread );
    }
    rt_thread_t ws_snd_thread = rt_thread_create( "wssndtsk", _ws_vm_trans_snd_task, RT_NULL, 0x200, 10, 10 );
    if( ws_snd_thread != RT_NULL ) {
        rt_thddog_register(ws_snd_thread, 30);
        rt_thread_startup( ws_snd_thread );
    }

    _ws_init = RT_TRUE;
}

static void _ws_vm_trans_rcv_task( void* parameter )
{
#define RCV_WAIT_TIME       (10)   //ms
    rt_uint32_t recved_event = 0;
    static rt_uint8_t data[128];

    while(1) {
        rt_thddog_suspend("rt_event_recv");
        rt_event_recv( &_ws_vm_rcv_event, WS_VM_EVENT_READY, RT_EVENT_FLAG_AND|RT_EVENT_FLAG_CLEAR, RT_WAITING_FOREVER, &recved_event );
        rt_thddog_resume();
        while(1) {  // wait some time
            rt_uint32_t evt;
            if( bQueueOverHalf(_ws_vm_tx_rcv_queue) || rt_event_recv( &_ws_vm_rcv_event, WS_VM_EVENT_READY, RT_EVENT_FLAG_AND|RT_EVENT_FLAG_CLEAR, rt_tick_from_millisecond(RCV_WAIT_TIME), &evt ) != RT_EOK ) {
                break;
            }
        }

        if( (recved_event & WS_VM_EVENT_READY) && g_ws_cfg.enable ) {
            if( websocket_ready() ) {
                while(1) {
                    int n = 0;
                    rt_base_t level = rt_hw_interrupt_disable();
                    for( n = 0; n < sizeof(data); n++ ) {
                        if( !bQueueRead(_ws_vm_tx_rcv_queue, data[n]) ) break;
                    }
                    rt_hw_interrupt_enable(level);
                    if( n > 0 ) {
                        rt_thddog_feed("websocket_send_data");
                        websocket_send_data( RT_NULL, (const rt_uint8_t *)data, n, RT_TRUE );
                    } else {
                        break;
                    }
                }
            }
        }
    }
}

static void _ws_vm_trans_snd_task( void* parameter )
{
#define SND_WAIT_TIME       (10)   //ms
    rt_uint32_t recved_event = 0;
    static rt_uint8_t data[128];

    while(1) {
        rt_thddog_suspend("rt_event_recv");
        rt_event_recv( &_ws_vm_snd_event, WS_VM_EVENT_READY, RT_EVENT_FLAG_AND|RT_EVENT_FLAG_CLEAR, RT_WAITING_FOREVER, &recved_event );
        rt_thddog_resume();
        while(1) {  // wait some time
            rt_uint32_t evt;
            if( bQueueOverHalf(_ws_vm_tx_snd_queue) || rt_event_recv( &_ws_vm_snd_event, WS_VM_EVENT_READY, RT_EVENT_FLAG_AND|RT_EVENT_FLAG_CLEAR, rt_tick_from_millisecond(SND_WAIT_TIME), &evt ) != RT_EOK ) {
                break;
            }
        }

        if( (recved_event & WS_VM_EVENT_READY) && g_ws_cfg.enable ) {
            if( websocket_ready() ) {
                while(1) {
                    int n = 0;
                    rt_base_t level = rt_hw_interrupt_disable();
                    for( n = 0; n < sizeof(data); n++ ) {
                        if( !bQueueRead(_ws_vm_tx_snd_queue, data[n]) ) break;
                    }
                    rt_hw_interrupt_enable(level);
                    if( n > 0 ) {
                        rt_thddog_feed("websocket_send_data");
                        websocket_send_data( RT_NULL, (const rt_uint8_t *)data, n, RT_FALSE );
                    } else {
                        break;
                    }
                }
            }
        }
    }
}

