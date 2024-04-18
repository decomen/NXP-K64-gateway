
#include "board.h"

static void _pin_mode(struct rt_device *device, rt_base_t pin, rt_base_t mode)
{
    if( pin != RT_PIN_NONE ) {
        switch(mode)
        {
            case PIN_MODE_OUTPUT:
                GPIO_QuickInit( (pin >> 8) & 0xFF, pin & 0xFF, kGPIO_Mode_OPP);
                break;
            case PIN_MODE_INPUT:
                GPIO_QuickInit( (pin >> 8) & 0xFF, pin & 0xFF, kGPIO_Mode_IFT);
                break;
            case PIN_MODE_INPUT_PULLUP:
                GPIO_QuickInit( (pin >> 8) & 0xFF, pin & 0xFF, kGPIO_Mode_IPU);
                break;
        }
    }
}

static void _pin_write(struct rt_device *device, rt_base_t pin, rt_base_t value)
{
    if( pin != RT_PIN_NONE ) {
        GPIO_WriteBit( (pin >> 8) & 0xFF, pin&0xFF, value);
    }
}

static void _pin_toggle(struct rt_device *device, rt_base_t pin)
{
    if( pin != RT_PIN_NONE ) {
        GPIO_ToggleBit( (pin >> 8) & 0xFF, pin&0xFF );
    }
}

static int _pin_read(struct rt_device *device, rt_base_t pin)
{
    if( pin != RT_PIN_NONE ) {
        return GPIO_ReadBit( (pin >> 8) & 0xFF, pin & 0xFF);
    }

    return PIN_LOW;
}
    
static const struct rt_pin_ops _ops =
{
    _pin_mode,
    _pin_write,
    _pin_toggle,
    _pin_read,
};

rt_err_t rt_hw_pin_init( void )
{    
    return rt_device_pin_register( "pin", &_ops, RT_NULL);
}

