
#ifndef __XFER_HELPER_H__
#define __XFER_HELPER_H__

#include <rtdef.h>

#if MB_MASTER_RTU_ENABLED > 0
#define MB_RTU_PDU_SIZE_MIN         4
#define MB_RTU_PDU_SIZE_MAX         256
#define MB_RTU_PDU_ADDR_OFF         0
#endif

#if MB_SLAVE_ASCII_ENABLED > 0
// with ':'
#define MB_ASCII_DEFAULT_CR         '\r'
#define MB_ASCII_DEFAULT_LF         '\n'
#define MB_ASCII_PDU_SIZE_MIN       (1+3*2)
#define MB_ASCII_PDU_ADDR_OFF       1

rt_uint8_t ucMBAsciiLRC( rt_uint8_t * pucFrame, rt_uint16_t usLen );
rt_uint8_t ucMBAsciiAddr( rt_uint8_t * pucFrame );

typedef enum
{
    STATE_ASCII_RX_IDLE,
    STATE_ASCII_RX_RCV,
    STATE_ASCII_RX_WAIT_EOF
} eMBAsciiRcvState;

#endif

// xfer net
rt_bool_t xfer_net_open(int n);
void xfer_net_close(int n);
rt_bool_t xfer_net_send(int n, const rt_uint8_t *pucMBTCPFrame, rt_uint16_t usTCPLength);

// xfer serial (include zigbee)
rt_bool_t xfer_dst_serial_open(int net);
void xfer_dst_serial_close(int net);
void xfer_dst_serial_send(int net, const void *buffer, rt_size_t size );
void xfer_dst_serial_rcv_clear(int net);
void xfer_dst_serial_rcv_byte(int net, rt_uint8_t ucByte, rt_bool_t flag );

void xfer_helper_serial_init( void );
void xfer_helper_enet_init( void );
void xfer_helper_gprs_init( void );

#endif

