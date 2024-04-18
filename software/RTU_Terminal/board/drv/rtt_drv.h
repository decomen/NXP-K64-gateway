#ifndef __RTT_DRV_H__
#define __RTT_DRV_H__

#include <rtdef.h>
#include <stdint.h>
#include "rtdevice.h"

#include <uart_cfg.h>

#define NIOCTL_GET_PHY_DATA     (0x05)
#define NIOCTL_PHY_RST          (0x06)

rt_err_t rt_hw_pin_init( void );
rt_err_t rt_hw_uart_init( uint8_t instance );
rt_err_t rt_hw_spi_init( uint8_t instance, uint8_t csn );
rt_err_t rt_hw_rtc_init( void );
void rt_hw_sd_init(void);
int rt_hw_enet_phy_init(void);
rt_bool_t rt_sd_in( void );

extern rt_serial_t *g_serials[];

#endif
