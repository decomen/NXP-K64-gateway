
#include "board.h"

static struct rt_spi_bus _k64_spis[BOARD_SPI_MAX];

struct k64_spi_device
{
    SPI_Type *baseAddress;
    uint8_t instance;
    uint32_t io_map;
    uint8_t csn;

    struct {
        uint8_t instance;
        uint8_t pin;
        uint8_t mux;
    } cs_cfg;
};

//hardware abstract device
static const struct k64_spi_device _k64_spi_nodes[BOARD_SPI_MAX][4] = {

    [BOARD_SPIFLASH_INSTANCE][BOARD_SPIFLASH_CSN] = { 
        (SPI_Type *)BOARD_SPIFLASH_BASE, 
        BOARD_SPIFLASH_INSTANCE, 
        BOARD_SPIFLASH_MAP, 
        BOARD_SPIFLASH_CSN, 
        BOARD_SPIFLASH_CSNCFG 
    }, 
};

#if BOARD_SPI_DRIVER_DEBUG
#define RTT_SPI_DRIVER_TRACE	rt_kprintf
#else
#define RTT_SPI_DRIVER_TRACE(...)
#endif

static rt_err_t _configure(struct rt_spi_device* device, struct rt_spi_configuration* configuration)
{
    //SPI_Type *spi_reg = ((struct k64_spi_device *)device->parent.user_data)->baseAddress;
    uint8_t instance = ((struct k64_spi_device *)device->parent.user_data)->instance;
    map_t * pq = (map_t*)&(((struct k64_spi_device *)device->parent.user_data)->io_map);

    for(int i = 0; i < pq->pin_cnt; i++)
    {
        PORT_PinMuxConfig(pq->io, pq->pin_start + i, (PORT_PinMux_Type) pq->mux); 
    }

    SPI_InitTypeDef SPI_InitStruct1;
    /* data_width */
    if(configuration->data_width <= 8)
    {
        SPI_InitStruct1.dataSize = 8;
    }
    else if(configuration->data_width <= 16)
    {
        SPI_InitStruct1.dataSize = 16;
    }
    else
    {
        return RT_EIO;
    }

    if( configuration->max_hz > SystemCoreClock ) {
        SPI_InitStruct1.baudrate = SystemCoreClock;
    } else {
        SPI_InitStruct1.baudrate = configuration->max_hz;
    }
    
    RTT_SPI_DRIVER_TRACE("spi bus baudrate:%d\r\n", SPI_InitStruct1.baudrate);
    /* frame foramt */
    SPI_InitStruct1.frameFormat = (SPI_FrameFormat_Type)(configuration->mode & 0x03);
    /* MSB or LSB */
    if(configuration->mode & RT_SPI_MSB)
    {
        SPI_InitStruct1.bitOrder = kSPI_MSB;
    }
    else
    {
        SPI_InitStruct1.bitOrder = kSPI_LSB;
    }
    SPI_InitStruct1.mode = kSPI_Master;
    SPI_InitStruct1.ctar = HW_CTAR0;
    SPI_InitStruct1.instance = instance;
    SPI_Init(&SPI_InitStruct1);
    
    return RT_EOK;

}

static rt_uint32_t _xfer(struct rt_spi_device* device, struct rt_spi_message* message)
{
    //struct rt_spi_configuration * config = &device->config;
    rt_uint32_t size = message->length;
    const rt_uint8_t * send_ptr = message->send_buf;
    rt_uint8_t * recv_ptr = message->recv_buf;
    
    //SPI_Type *spi_reg = ((struct k64_spi_device *)device->parent.user_data)->baseAddress;
    uint8_t instance = ((struct k64_spi_device *)device->parent.user_data)->instance;
    uint8_t csn = ((struct k64_spi_device *)device->parent.user_data)->csn;

    while(size--)
    {
        rt_uint16_t data = 0xFF;
        if(send_ptr != RT_NULL)
        {
            data = *send_ptr++;
        }
        /* 最后一个 并且是需要释放CS */
        if((size == 0) && (message->cs_release))
        {  
            data = SPI_ReadWriteByte( instance, HW_CTAR0, data, csn, kSPI_PCS_ReturnInactive);
        }
        else
        {
            data = SPI_ReadWriteByte( instance, HW_CTAR0, data, csn, kSPI_PCS_KeepAsserted);
        }
        if(recv_ptr != RT_NULL)
        {
            *recv_ptr++ = data;
        }
    }
    return message->length;
}



static struct rt_spi_ops _k64_spis_ops =
{
    _configure,
    _xfer
};

rt_err_t rt_hw_spi_init( uint8_t instance, uint8_t csn )
{
    struct rt_spi_device* spi_device = rt_malloc(sizeof(struct rt_spi_device));
    char dev_name[16];
    
    if( RT_NULL == rt_device_find(BOARD_SPI_DEV_NAME(instance)) ) {
        _k64_spis[instance].ops = &_k64_spis_ops;
        rt_spi_bus_register( &_k64_spis[instance], BOARD_SPI_DEV_NAME(instance), &_k64_spis_ops );
    }

    PORT_PinMuxConfig( 
        _k64_spi_nodes[instance][csn].cs_cfg.instance, 
        _k64_spi_nodes[instance][csn].cs_cfg.pin, 
        (PORT_PinMux_Type)_k64_spi_nodes[instance][csn].cs_cfg.mux 
    );
    
    rt_snprintf( dev_name, sizeof(dev_name), "%s%d", BOARD_SPI_DEV_NAME(instance), csn );
    return rt_spi_bus_attach_device( spi_device, dev_name, BOARD_SPI_DEV_NAME(instance), (void *)&_k64_spi_nodes[instance][csn] );
}

INIT_BOARD_EXPORT(rt_hw_spi_init);

