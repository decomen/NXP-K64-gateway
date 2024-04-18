
#ifndef _ZIGBEE_STD_H__
#define _ZIGBEE_STD_H__

#define ZGB_OFS(_t, _m)  (unsigned int)(&(((_t *)0)->_m))

typedef struct {
    rt_uint8_t  mac[8];
} zgb_mac_t;

typedef enum {
    ZGB_SN_T_MODBUS_RTU,
    ZGB_SN_T_MODBUS_ASCII,

} ezgb_sn_type_t;

typedef union {
    rt_uint8_t addr_8;      // 8λ��ַ
} zgb_sn_val_t;

typedef struct _zgb_snode {
    rt_uint8_t          type;           // �ڵ�����
    rt_uint8_t          cnt;            // ��Ŀ
    zgb_sn_val_t        *lst;           // ����
    struct _zgb_snode   *next, *prev;   // ����
} zgb_snode_t;

typedef struct _zgb_mnode {
    rt_uint8_t          workmode;       // ����ģʽ
    zgb_mac_t           mac;            // MAC��ַ (fastzigbee ����)
    rt_uint16_t         netid;          // �����ַ(fastzigbee ר��)
    rt_uint8_t          online;         // �Ƿ�����
    rt_uint8_t          rssi;           // �ź�ǿ��
    rt_uint32_t         uptime;         // ����ʱ��
    rt_uint32_t         lasttime;       // ����ʱ��
    rt_uint32_t         offtime;        // ����ʱ��
    zgb_snode_t         *snode;         // �ӽڵ�(����)
    struct _zgb_mnode   *next, *prev;   // ����
} zgb_mnode_t;

#pragma pack(1)

#define ZGB_STD_FA_SIZE     (512)

#define ZGB_STD_PRE         (0x9AABDEEF)
#define ZGB_STD_PREn(n)     ((ZGB_STD_PRE>>(n<<3))&0xFF)
#define ZGB_STD_NPRE        (~ZGB_STD_PRE)
#define ZGB_STD_NPREn(n)    ((ZGB_STD_NPRE>>(n<<3))&0xFF)

typedef enum {
    ZGB_STD_FA_ACK          = 0x00,
    ZGB_STD_FA_POST         = 0x01,
    ZGB_STD_FA_REQ          = 0x02,
    ZGB_STD_FA_RSP          = 0x03,
} ezgb_std_pack_t;

typedef enum {
    //ZGB_STD_MSG_SCAN        = 0x00,  // ɨ��
    ZGB_STD_MSG_SCAN        = 0x01,  // ɨ��(2016/10/28������������,��ֹ��ɰ汾��ͻ)
} ezgb_std_msg_t;

typedef struct {
    rt_uint32_t pre;        // ǰ��(������,�����ͻ)
    rt_uint32_t npre;       // ��ǰ��(��λȡ��)
    rt_uint8_t  ver;        // �汾
    rt_uint16_t msglen;     // ��Ϣ����
    rt_uint8_t  seq;        // ���
    rt_uint8_t  packtype;   // ֡����
    rt_uint8_t  msgtype;    // ��Ϣ���� (ĿǰΪ0)
} zgb_std_head_t;

typedef struct {
    zgb_std_head_t  head;
    rt_uint8_t      *data;
    rt_uint32_t     crc;
} zgb_std_pack_t;

typedef struct {
    rt_uint16_t netid;
    zgb_mac_t   mac;
    rt_uint8_t  type;
} zgb_std_scan_req_t;

typedef struct {
    rt_uint8_t  workmode;
    rt_uint16_t netid;
    zgb_mac_t   mac;
    rt_uint8_t  rssi;
    rt_uint8_t  type;
    rt_uint8_t  cnt;
    rt_uint8_t  data[1];
} zgb_std_scan_t;

#pragma pack()

// ��ʼ��
void zgb_std_init( void );
// ���ҽڵ�
zgb_mnode_t *zgb_mnode_find_with_netid( rt_uint16_t netid );
// ���ҽڵ�
zgb_mnode_t *zgb_mnode_find_with_addr( ezgb_sn_type_t type, int addr );
// ���������ַ�Ƴ�
void zgb_mnode_rm_with_id( rt_uint16_t netid );
// �Ƴ����нڵ�
void zgb_mnode_rm_all( void );
// ��������
zgb_mnode_t *zgb_mnode_insert( zgb_std_scan_t *info );
// ���ݽ���
zgb_mnode_t *zgb_std_parse_buffer( zgb_std_head_t *head, rt_uint8_t buffer[] );

rt_bool_t zgb_set_dst_node( ezgb_sn_type_t type, int addr );

void zgb_check_online( void );

#endif

