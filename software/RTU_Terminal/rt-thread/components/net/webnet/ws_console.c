
#include <queue.h>
#include <board.h>

static struct rt_device _ws_console;

void ws_console_recv_pack( void )
{
    rt_mutex_take(&s_websock_mutex, RT_WAITING_FOREVER);
    {
        if( !g_host_cfg.bDebug && g_ws_cfg.enable && WS_PORT_SHELL == g_ws_cfg.port_type ) {
            _ws_console.rx_indicate( &_ws_console, ws_vm_buflen() );
        }
    }
    rt_mutex_release(&s_websock_mutex);
}

static rt_err_t _ws_console_init(struct rt_device *dev)
{
    ws_vm_clear();
    return RT_EOK;
}

static rt_err_t _ws_console_open(struct rt_device *dev, rt_uint16_t oflag)
{
    ws_vm_clear();
    return RT_EOK;
}

static rt_err_t _ws_console_close(struct rt_device *dev)
{
    ws_vm_clear();
    return RT_EOK;
}

static rt_size_t _ws_console_read(struct rt_device *dev, rt_off_t pos, void *buffer, rt_size_t size)
{
    rt_size_t n = 0;
    if( !g_host_cfg.bDebug && g_ws_cfg.enable && WS_PORT_SHELL == g_ws_cfg.port_type ) {
        n = ws_vm_read( pos, buffer, size );
    }
    return n;
}

static rt_size_t _ws_console_write(struct rt_device *dev, rt_off_t pos, const void *buffer, rt_size_t size )
{
    if (!g_host_cfg.bDebug && g_ws_cfg.enable && WS_PORT_SHELL == g_ws_cfg.port_type) {
        ws_vm_snd_write(pos, (void *)buffer, size);
    }
    
    return size;
}

static rt_err_t _ws_console_control(struct rt_device *dev,rt_uint8_t cmd, void *args )
{
    return RT_EOK;
}

void ws_console_init( void )
{
#define WS_CONSOLE_NAME     "ws_tty"

    struct rt_device *device = &_ws_console;
    device->type            = RT_Device_Class_Char;
    device->ide_indicate    = RT_NULL;
    device->rx_indicate     = RT_NULL;
    device->tx_complete     = RT_NULL;

    device->init        = _ws_console_init;
    device->open        = _ws_console_open;
    device->close       = _ws_console_close;
    device->read        = _ws_console_read;
    device->write       = _ws_console_write;
    device->control     = _ws_console_control;
    device->user_data   = RT_NULL;

    /* register a character device */
    rt_device_register(device, WS_CONSOLE_NAME, RT_DEVICE_FLAG_RDWR | RT_DEVICE_FLAG_INT_RX | RT_DEVICE_FLAG_STREAM );

    if( !g_host_cfg.bDebug ) {
        rt_console_set_device( WS_CONSOLE_NAME );
#ifdef RT_USING_FINSH
        finsh_system_init();
#endif
    }
}


