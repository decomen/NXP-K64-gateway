
#include <board.h>

static struct rt_device _rtc;

static rt_err_t _open(rt_device_t dev, rt_uint16_t oflag)
{
    if (dev->rx_indicate != RT_NULL)
    {
        /* Open Interrupt */
    }
    return RT_EOK;
}


static rt_err_t _control(rt_device_t dev, rt_uint8_t cmd, void *args)
{
    uint32_t val;
    RT_ASSERT(dev != RT_NULL);
    switch (cmd)
    {
        case RT_DEVICE_CTRL_RTC_GET_TIME:
            val = RTC_GetTSR();
            *(rt_time_t*)args = *(rt_time_t*)&val;
            break;

        case RT_DEVICE_CTRL_RTC_SET_TIME:
            RTC_SetTSR(*(uint32_t*)args);
            break;
    }
    return RT_EOK;
}

rt_err_t rt_hw_rtc_init( void )
{
    RTC_InitTypeDef RTC_InitStruct1;
    RTC_InitStruct1.oscLoad = kRTC_OScLoad_8PF;
    RTC_Init(&RTC_InitStruct1);

    _rtc.type	= RT_Device_Class_RTC;

    /* register rtc device */
    _rtc.init 	= RT_NULL;
    _rtc.open 	= _open;
    _rtc.close	= RT_NULL;
    _rtc.read 	= RT_NULL;
    _rtc.write	= RT_NULL;
    _rtc.control = _control;

    /* no private */
    _rtc.user_data = RT_NULL;
    
    return rt_device_register(&_rtc, "rtc", RT_DEVICE_FLAG_RDWR );
}

    
INIT_DEVICE_EXPORT(rt_hw_rtc_init);

