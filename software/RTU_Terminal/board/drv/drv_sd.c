#include <board.h>

static struct rt_device sd_device;
static  rt_mutex_t mutex;

rt_bool_t rt_sd_in(void)
{
    if (rt_pin_read(BOARD_GPIO_SD_TEST) == PIN_LOW) {
        return RT_TRUE;
    } else {
        return RT_FALSE;
    }
}

static rt_err_t rt_sd_init(rt_device_t dev)
{
    // rt_pin_write( BOARD_GPIO_SD_EN, PIN_HIGH );

    return RT_EOK;
}

static rt_err_t rt_sd_open(rt_device_t dev, rt_uint16_t oflag)
{
    if (rt_sd_in()) {
        int r = 0;
        rt_mutex_take(mutex, RT_WAITING_FOREVER);
        //rt_pin_write( BOARD_GPIO_SD_EN, PIN_LOW );
        // rt_thread_delay( RT_TICK_PER_SECOND / 100 );
        // rt_pin_write( BOARD_GPIO_SD_EN, PIN_HIGH );
        int size = 0;
        int sdcard_cnt = 0;
        do {
            SD_QuickInit(200000);
            rt_thread_delay(RT_TICK_PER_SECOND / 100);
            size  = SD_GetSizeInMB();
            if (sdcard_cnt++ >= 2) {
                r = -1;
                break;
            }
        } while (size == 0);

        rt_kprintf("size = %d\n", size);

        SD_QuickInit(50000000);
        rt_mutex_release(mutex);
        if (r) {
            dev->open_flag = RT_DEVICE_OFLAG_CLOSE;
            return RT_ERROR;
        }
        dev->open_flag = RT_DEVICE_OFLAG_OPEN;
    } else {
        rt_kprintf("no sdcard in\n");
        dev->open_flag = RT_DEVICE_OFLAG_CLOSE;
        return RT_ERROR;
    }
    return RT_EOK;
}

static rt_err_t rt_sd_close(rt_device_t dev)
{
    //rt_pin_write( BOARD_GPIO_SD_EN, PIN_LOW );
    return RT_EOK;
}

rt_err_t rt_sd_indicate(rt_device_t dev, rt_size_t size)
{
    rt_kprintf("I need indicate\r\n");
    return RT_EOK;
}

static rt_size_t rt_sd_read(rt_device_t dev, rt_off_t pos, void *buffer, rt_size_t size)
{
    if (rt_sd_in()) {
        int r;
        if (rt_mutex_take(mutex, RT_WAITING_NO)) {
            return 0;
        }
        r = SD_ReadMultiBlock(pos, (rt_uint8_t *)buffer, size);
        rt_mutex_release(mutex);
        if (r) {
            rt_kprintf("sd_read error!%d\r\n", r);
            dev->open_flag = RT_DEVICE_OFLAG_CLOSE;
            return 0;
        }
    } else {
        dev->open_flag = RT_DEVICE_OFLAG_CLOSE;
        return 0;
    }
    return size;
}

static rt_size_t rt_sd_write(rt_device_t dev, rt_off_t pos, const void *buffer, rt_size_t size)
{
    if (rt_sd_in()) {
        int r;
        if (rt_mutex_take(mutex, RT_WAITING_NO)) {
            return 0;
        }
        r = SD_WriteMultiBlock(pos, (rt_uint8_t *)buffer, size);
        rt_mutex_release(mutex);
        if (r) {
            rt_kprintf("sd_write error!%d\r\n", r);
            dev->open_flag = RT_DEVICE_OFLAG_CLOSE;
            return 0;
        }
    }
    return size;
}

static rt_err_t rt_sd_control(rt_device_t dev, rt_uint8_t cmd, void *args)
{
    uint32_t size;
    struct rt_device_blk_geometry geometry;
    rt_memset(&geometry, 0, sizeof(geometry));
    switch (cmd) {
    case RT_DEVICE_CTRL_BLK_GETGEOME:
        if (rt_sd_in()) {
            geometry.block_size = 512;
            geometry.bytes_per_sector = 512;
            size = SD_GetSizeInMB();

            geometry.sector_count = size * 1024 * 2;
        }
        rt_memcpy(args, &geometry, sizeof(struct rt_device_blk_geometry));
        break;

    default:
        break;
    }
    return RT_EOK;

}

rt_err_t rt_sd_txcomplete(rt_device_t dev, void *buffer)
{
    return RT_EOK;
}

void rt_hw_sd_init(void)
{

    sd_device.type 		= RT_Device_Class_Block;
    sd_device.rx_indicate = rt_sd_indicate;
    sd_device.tx_complete = RT_NULL;
    sd_device.init 		= rt_sd_init;
    sd_device.open		= rt_sd_open;
    sd_device.close		= rt_sd_close;
    sd_device.read 		= rt_sd_read;
    sd_device.write     = rt_sd_write;
    sd_device.control 	= rt_sd_control;
    sd_device.user_data	= RT_NULL;

    mutex = rt_mutex_create("sd_mutex", RT_IPC_FLAG_FIFO);

    //  rt_pin_mode( BOARD_GPIO_SD_EN, PIN_MODE_OUTPUT );
    // rt_pin_write( BOARD_GPIO_SD_EN, PIN_HIGH );
    rt_pin_mode(BOARD_GPIO_SD_TEST, PIN_MODE_INPUT);

    rt_device_register(&sd_device, BOARD_SDCARD_NAME, RT_DEVICE_FLAG_RDWR  | RT_DEVICE_FLAG_REMOVABLE | RT_DEVICE_FLAG_STANDALONE);
}

