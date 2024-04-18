
#ifndef _BOARD_CFG_H__
#define _BOARD_CFG_H__

#define CFG_VER                 0x01
#define CFG_MAGIC               0xD6BCF4ED

// 设备信息暂时存于 Flash 不存在文件中, 该区域预留

#define CFG_INFO_OFS            (0)
#define CFG_INFO_SIZE           (128)

#define DEV_CFG_OFS             (CFG_INFO_OFS+CFG_INFO_SIZE)
#define DEV_CFG_SIZE            (256)

#define REG_CFG_OFS             (DEV_CFG_OFS+DEV_CFG_SIZE)
#define REG_CFG_SIZE            (512)

#define UART_CFG_OFS            (REG_CFG_OFS+REG_CFG_SIZE)
#define UART_CFG_SIZE           (512)

#define NET_CFG_OFS             (UART_CFG_OFS+UART_CFG_SIZE)
#define NET_CFG_SIZE            (256)

#define GPRS_NET_CFG_OFS        (NET_CFG_OFS+NET_CFG_SIZE)
#define GPRS_NET_CFG_SIZE       (512)

#define GPRS_WORK_CFG_OFS       (GPRS_NET_CFG_OFS+GPRS_NET_CFG_SIZE)
#define GPRS_WORK_CFG_SIZE      (512)

#define TCPIP_CFG_OFS           (GPRS_WORK_CFG_OFS+GPRS_WORK_CFG_SIZE)
#define TCPIP_CFG_SIZE          (2*1024)

#define DI_CFG_OFS              (TCPIP_CFG_OFS+TCPIP_CFG_SIZE)
#define DI_CFG_SIZE             (2*1024)

#define DO_CFG_OFS              (DI_CFG_OFS+DI_CFG_SIZE)
#define DO_CFG_SIZE             (2*1024)

#define ANALOG_CFG_OFS          (DO_CFG_OFS+DO_CFG_SIZE)
#define ANALOG_CFG_SIZE         (1024)

#define AUTH_CFG_OFS            (ANALOG_CFG_OFS+ANALOG_CFG_SIZE)
#define AUTH_CFG_SIZE           (512)

#define STORAGE_CFG_OFS         (AUTH_CFG_OFS+AUTH_CFG_SIZE)
#define STORAGE_CFG_SIZE        (256)

//#define EXT_VAR_CFG_OFS         (STORAGE_CFG_OFS+STORAGE_CFG_SIZE)
//#define EXT_VAR_CFG_SIZE        (80*1024)

//#define HOST_CFG_OFS            (EXT_VAR_CFG_OFS+EXT_VAR_CFG_SIZE)
//#define HOST_CFG_SIZE           (256)

#define HOST_CFG_OFS            (STORAGE_CFG_OFS+STORAGE_CFG_SIZE)
#define HOST_CFG_SIZE           (256)

#define ZIGBEE_CFG_OFS          (HOST_CFG_OFS+HOST_CFG_SIZE)
#define ZIGBEE_CFG_SIZE         (256)

#define XFER_UART_CFG_OFS       (ZIGBEE_CFG_OFS+ZIGBEE_CFG_SIZE)
#define XFER_UART_CFG_SIZE      (2*1024)

typedef struct {
    rt_uint8_t usVer;

    //...
} cfg_info_t;

extern cfg_info_t g_cfg_info;

void board_cfg_init( void );
void board_cfg_del( void );
int board_cfg_justread( int ofs, void *data, int len );
rt_bool_t board_cfg_read( int ofs, void *pcfg, int len );
int board_cfg_justwrite(int ofs, void *data, int len);
rt_bool_t board_cfg_write( int ofs, void const *pcfg, int len );

rt_bool_t cfg_recover_try(void);

#endif

