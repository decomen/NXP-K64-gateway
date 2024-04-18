
#ifndef __UART_CFG_H__
#define __UART_CFG_H__

#include <rtdef.h>
#include <stdint.h>
#include <protomanage.h>
#include <common.h>
#include <MK64F12.h>

typedef rt_bool_t ( *pbbusy_handle ) ( int port );
typedef void ( *pvrecv_handle ) ( int port, rt_uint8_t value );

// 串口类型
typedef enum {
    UART_TYPE_232,
    UART_TYPE_485,
    UART_TYPE_ZIGBEE,
    UART_TYPE_GPRS
} uart_type_e;

// 通信协议
typedef enum {
    PROTO_MODBUS_RTU,           //modbus RTU
    PROTO_MODBUS_ASCII,         //modbus ASCII
    PROTO_DLT645,               // dlt645 电表

    PROTO_LUA = 20,             // lua
} proto_uart_type_e;

typedef struct {
    rt_uint32_t baud_rate;
    rt_uint8_t  data_bits   :4;
    rt_uint8_t  stop_bits   :2;
    rt_uint8_t  parity      :2;
} uart_port_cfg_t;

typedef struct {
    uart_port_cfg_t     port_cfg;

    uart_type_e         uart_type;
    proto_uart_type_e   proto_type;
    proto_ms_e          proto_ms;
    rt_uint8_t          slave_addr;     //从机modbus时有效, 从机地址
    rt_uint32_t         interval;

    char                lua_proto[32];

    pbbusy_handle       busy_handle;
    pvrecv_handle       recv_handle;
} uart_cfg_t;

struct k64_serial_device
{
    UART_Type *baseAddress;
    uint8_t instance;
    uint32_t io_map;
    int irq_num;
    rt_base_t pin_en;

    uart_cfg_t *cfg;
};

#define DEFAULT_UART_CFG(_type) { BAUD_RATE_115200, DATA_BITS_8, STOP_BITS_1, PARITY_NONE, _type, PROTO_MODBUS_RTU, PROTO_SLAVE, 0, 1000, "", RT_NULL, RT_NULL }

extern uart_cfg_t g_uart_cfgs[];

rt_err_t uart_cfg_init( void );
rt_int8_t nUartGetIndex(rt_int8_t instance);
rt_int8_t nUartGetInstance( rt_int8_t index );

#endif

