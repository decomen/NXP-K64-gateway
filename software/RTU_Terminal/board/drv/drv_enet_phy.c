
#include <board.h>
#include <time.h>

#define _USE_STATIC_MEM         1

static struct eth_device device;
//static uint8_t gCfgLoca_MAC[] = {0x00, 0xCF, 0x52, 0x35, 0x00, 0x01};
#if _USE_STATIC_MEM
ALIGN(RT_ALIGN_SIZE)
static rt_uint8_t _gTxBuf[RT_ALIGN(CFG_ENET_BUFFER_SIZE+16, 16)];
static rt_uint8_t _gRxBuf[RT_ALIGN(CFG_ENET_BUFFER_SIZE+16, 16)];
#endif

static rt_uint8_t *gTxBuf;
static rt_uint8_t *gRxBuf;

rt_tick_t g_ulLastBusyTick = 0;
char *_hostname = g_host_cfg.szHostName;

void ENET_ISR(void)
{
    rt_pin_toggle( BOARD_GPIO_NET_LED );
    rt_interrupt_enter();
    eth_device_ready(&device);
    rt_interrupt_leave();
    g_ulLastBusyTick = rt_tick_get();
}

static rt_err_t rt_enet_phy_init(rt_device_t dev)
{
    /* init driver */
    ENET_InitTypeDef ENET_InitStruct1;
    ENET_InitStruct1.pMacAddress = g_xDevInfoReg.xDevInfo.xNetMac.mac;
    ENET_InitStruct1.is10MSpped = false;
    ENET_InitStruct1.isHalfDuplex = false;
    ENET_Init(&ENET_InitStruct1);
    
    PORT_PinMuxConfig(HW_GPIOB, 0, kPinAlt4);
    PORT_PinPullConfig(HW_GPIOB, 0, kPullUp);
    PORT_PinOpenDrainConfig(HW_GPIOB, 0, ENABLE);
    
    PORT_PinMuxConfig(HW_GPIOB, 1, kPinAlt4);
    PORT_PinMuxConfig(HW_GPIOA, 5, kPinAlt4);
    PORT_PinMuxConfig(HW_GPIOA, 12, kPinAlt4);
    PORT_PinMuxConfig(HW_GPIOA, 13, kPinAlt4);
    PORT_PinMuxConfig(HW_GPIOA, 14, kPinAlt4);
    PORT_PinMuxConfig(HW_GPIOA, 15, kPinAlt4);
    PORT_PinMuxConfig(HW_GPIOA, 16, kPinAlt4);
    PORT_PinMuxConfig(HW_GPIOA, 17, kPinAlt4);

    rt_pin_mode( BOARD_GPIO_NET_LED, PIN_MODE_OUTPUT );
    rt_pin_write( BOARD_GPIO_NET_LED, PIN_HIGH );
    
    rt_pin_mode( BOARD_GPIO_NET_RESET, PIN_MODE_OUTPUT );
    rt_pin_write( BOARD_GPIO_NET_RESET, PIN_LOW );
    rt_thread_delay( RT_TICK_PER_SECOND / 10 );
    rt_pin_write( BOARD_GPIO_NET_RESET, PIN_HIGH );
    rt_thread_delay( RT_TICK_PER_SECOND / 10 );
    
    enet_phy_init();

//    if(!enet_phy_is_linked())
//    {
//        eth_device_linkchange(&device, false);
//        return RT_EIO;
//    }
    
    ENET_CallbackRxInstall(ENET_ISR);
    ENET_ITDMAConfig(kENET_IT_RXF);
    
    return RT_EOK;
}


static rt_err_t rt_enet_phy_open(rt_device_t dev, rt_uint16_t oflag)
{
    return RT_EOK;
}

static rt_err_t rt_enet_phy_close(rt_device_t dev)
{
    return RT_EOK;
}

static rt_size_t rt_enet_phy_read(rt_device_t dev, rt_off_t pos, void* buffer, rt_size_t size)
{
    return RT_EOK;
}

static rt_size_t rt_enet_phy_write (rt_device_t dev, rt_off_t pos, const void* buffer, rt_size_t size)
{
    return RT_EOK;
}

static rt_err_t rt_enet_phy_control(rt_device_t dev, rt_uint8_t cmd, void *args)
{
    switch (cmd)
    {
    case NIOCTL_GADDR:
        /* get mac address */
        if (args) rt_memcpy(args, g_xDevInfoReg.xDevInfo.xNetMac.mac, 6);
        else return -RT_ERROR;
        break;
    case NIOCTL_GET_PHY_DATA:
//        if (args) rt_memcpy(args, &phy_data, sizeof(phy_data));
        break;
    case NIOCTL_PHY_RST:
        rt_pin_write( BOARD_GPIO_NET_RESET, PIN_LOW );
        rt_thread_delay( RT_TICK_PER_SECOND / 100 );
        rt_pin_write( BOARD_GPIO_NET_RESET, PIN_HIGH );
    default :
        break;
    }
    return RT_EOK;
}

struct pbuf *rt_enet_phy_rx(rt_device_t dev)
{
    struct pbuf* p = RT_NULL;
    rt_uint32_t len;
    
    //len = ENET_GetReceiveLen();
    len = ENET_MacReceiveData(gRxBuf);
    if(len)
    {
        p = pbuf_alloc(PBUF_LINK, len, PBUF_RAM);
        if( p ) {
            rt_memcpy((rt_uint8_t*)p->payload, (rt_uint8_t*)gRxBuf, len);
            //ENET_MacReceiveData(p->payload);
        }
//        if (p != RT_NULL)
//        {
//            struct pbuf* q;  
//            for (q = p; q != RT_NULL; q = q->next)
//            {
//                rt_memcpy((rt_uint8_t*)q->payload, (rt_uint8_t*)&gRxBuf[i], q->len);
//            }
//            len = 0;
//        }
//        else
//        {
//            return NULL;
//        }
    }
    return p;
}

rt_err_t rt_enet_phy_tx( rt_device_t dev, struct pbuf* p)
{
    bool islink = 0, issendok = 1;
    struct pbuf *q;
    uint32_t len;
    
    islink = enet_phy_is_linked();
    eth_device_linkchange(&device, islink);
    
    q = p;
    while( q != RT_NULL ) {
        len = 0;
        for (; q != RT_NULL; q = q->next)
        {
            if( len + q->len < CFG_ENET_BUFFER_SIZE ) {
                rt_memcpy(gTxBuf + len, q->payload, q->len);
                len += q->len;
            } else {
                break;
            }
        }

        if(islink && issendok)
        {
            rt_tick_t tStart = rt_tick_get();
            issendok = ENET_MacSendData(gTxBuf, len);
            if( !issendok ) {
              rt_kprintf( "send timeout %d,%d!\n", len, rt_tick_get() - tStart );
            }
        }
    }

    if(islink && issendok)
    {
        return RT_EOK;
    }
    return RT_EIO;
}

int rt_hw_enet_phy_init(void)
{
    device.parent.init       = rt_enet_phy_init;
    device.parent.open       = rt_enet_phy_open;
    device.parent.close      = rt_enet_phy_close;
    
    device.parent.read       = rt_enet_phy_read;
    device.parent.write      = rt_enet_phy_write;
    device.parent.control    = rt_enet_phy_control;
    device.parent.user_data    = RT_NULL;

    device.eth_rx     = rt_enet_phy_rx;
    device.eth_tx     = rt_enet_phy_tx;

#if _USE_STATIC_MEM
    gTxBuf = (rt_uint8_t*)(uint32_t)RT_ALIGN((uint32_t)_gTxBuf, 16);
    gRxBuf = (rt_uint8_t*)(uint32_t)RT_ALIGN((uint32_t)_gRxBuf, 16);
#else
    gTxBuf = rt_malloc(CFG_ENET_BUFFER_SIZE+16);
    gRxBuf = rt_malloc(CFG_ENET_BUFFER_SIZE+16);
    if(!gTxBuf || !gRxBuf)
    {
        return RT_ENOMEM;
    }
    gTxBuf = (rt_uint8_t*)(uint32_t)RT_ALIGN((uint32_t)gTxBuf, 16);
    gRxBuf = (rt_uint8_t*)(uint32_t)RT_ALIGN((uint32_t)gRxBuf, 16);
#endif

    eth_device_init(&device, "e0");
    return 0;
}

