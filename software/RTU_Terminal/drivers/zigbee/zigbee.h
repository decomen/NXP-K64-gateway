#ifndef __ZIGBEE_H__
#define __ZIGBEE_H__

#include <port.h>

#define ZIGBEE_TEMP_PRECODE_0	0xDE
#define ZIGBEE_TEMP_PRECODE_1	0xDF
#define ZIGBEE_TEMP_PRECODE_2	0xEF

#define ZIGBEE_TEMP_PRECODE	ZIGBEE_TEMP_PRECODE_0, ZIGBEE_TEMP_PRECODE_1, ZIGBEE_TEMP_PRECODE_2

#define ZIGBEE_HEAD_LEN 	0x04

#define	ZIGBEE_TEMP_CMD_SET_CHAN		0xD1
#define	ZIGBEE_TEMP_CMD_SET_DST_ADDR	0xD2
#define	ZIGBEE_TEMP_CMD_SHOW_SRC_ADDR	0xD3
#define	ZIGBEE_TEMP_CMD_GPIO_CONFIG		0xD4
#define	ZIGBEE_TEMP_CMD_GPIO_READ		0xD5
#define	ZIGBEE_TEMP_CMD_GPIO_WRITE		0xD6
#define	ZIGBEE_TEMP_CMD_AD				0xD7
#define	ZIGBEE_TEMP_CMD_SLEEP			0xD8
#define	ZIGBEE_TEMP_CMD_MODE			0xD9
#define	ZIGBEE_TEMP_CMD_RSSI			0xDA

#define ZIGBEE_OK	0
#define ZIGBEE_FAILED	0xFF

typedef enum {
    ZIGBEE_IO_IN = 0x00,
    ZIGBEE_IO_OUT = 0x01
} ZIGBEE_IO_CFG_E;

typedef enum {
    ZIGBEE_MSG_MODE_SINGLE = 0x00,
    ZIGBEE_MSG_MODE_BROAD = 0x01
} ZIGBEE_MSG_MODE_E;


#define ZIGBEE_PRECODE_0	0xAB
#define ZIGBEE_PRECODE_1	0xBC
#define ZIGBEE_PRECODE_2	0xCD

#define ZIGBEE_PRECODE	ZIGBEE_PRECODE_0, ZIGBEE_PRECODE_1, ZIGBEE_PRECODE_2

#define ZIGBEE_CMD_GET_DEV_INFO			0xD1
#define ZIGBEE_CMD_SET_CHAN				0xD2
#define ZIGBEE_CMD_SEARCH				0xD4
#define ZIGBEE_CMD_GET_OTHER_DEV_INFO	0xD5
#define ZIGBEE_CMD_SET_DEV_INFO			0xD6
#define ZIGBEE_CMD_RST					0xD9
#define ZIGBEE_CMD_DEFAULT				0xDA
#define ZIGBEE_CMD_PWD_EN				0xDE
#define ZIGBEE_CMD_LOGIN				0xDF

typedef enum {
    ZIGBEE_WORK_END_DEVICE = 0x00,
    ZIGBEE_WORK_ROUTER,
    ZIGBEE_WORK_COORDINATOR
} ZIGBEE_WORK_TYPE_E;

typedef enum {
    ZIGBEE_SERIAL_RATE_2400 = 0x01,
    ZIGBEE_SERIAL_RATE_4800,
    ZIGBEE_SERIAL_RATE_9600,
    ZIGBEE_SERIAL_RATE_19200,
    ZIGBEE_SERIAL_RATE_38400,
    ZIGBEE_SERIAL_RATE_57600,
    ZIGBEE_SERIAL_RATE_115200
} ZIGBEE_SERIAL_RATE_E;

typedef enum {
    ZIGBEE_DEV_PTP = 0x0001,
    ZIGBEE_DEV_AD = 0x0002
} ZIGBEE_DEV_E;

typedef enum {
    ZIGBEE_ERR_OK = 0x00,
    ZIGBEE_ERR_ADDRESS_FAUSE,
    ZIGBEE_ERR_LENGTH_FAUSE,
    ZIGBEE_ERR_CHECK_FAUSE,
    ZIGBEE_ERR_WRITE_FAUSE,
    ZIGBEE_ERR_OTHER_FAUSE,
    ZIGBEE_ERR_TIMEOUT = 0xFE,
    ZIGBEE_ERR_OTHER = 0xFF,
} ZIGBEE_ERR_E;

#pragma pack(1)
typedef struct {
    UCHAR DevName[16];      //�豸����(�ִ�)
    UCHAR DevPwd[16];       //�豸����(�ִ�)
    UCHAR WorkMode;         //�������� 0=END_DEVICE��1=ROUTER��2=COORDINATOR
    UCHAR Chan;             //ͨ����
    USHORT PanID;           //����ID
    USHORT Addr;            //���������ַ
    UCHAR Mac[8];           //���������ַ(MAC, ֻ���̶������޸�)
    USHORT DstAddr;         //Ŀ�������ַ
    UCHAR DstMac[8];        //Ŀ�������ַ(MAC)
    UCHAR Reserve1[1];      //����
    UCHAR PowerLevel;       //���书��(����, ֻ��)
    UCHAR RetryNum;         //�����������Դ���
    UCHAR TranTimeout;      //������������ʱ����(��λ:100ms)
    UCHAR SerialRate;       //���������� (0=1200, 1=2400, 2=4800, 3=9600, 4=19200, 5=38400, 6=57600, 7=115200, 8=230400, 9=460800)
    UCHAR SerialDataB;      //��������λ 5~8
    UCHAR SerialStopB;      //����ֹͣλ 1~2
    UCHAR SerialParityB;    //����У��λ 0=��У�� 1=��У�� 2=żУ��
    UCHAR MsgMode;          //����ģʽ 0=����  1=�㲥
} ZIGBEE_DEV_INFO_T;
#pragma pack()

// �޸�ͨ����
UCHAR ucZigbeeSetChan(UCHAR chan);
// �޸�Ŀ������
UCHAR ucZigbeeSetDstAddr(USHORT Addr);
// ��ͷ��ʾԴ��ַ
UCHAR ucZigbeeShowSrcAddr(BOOL show);
// ����GPIO�����������
UCHAR ucZigbeeGPIOConfig(USHORT Addr, UCHAR cfg);
// ��ȡGPIO
UCHAR ucZigbeeGPIORead(USHORT Addr, UCHAR *pGpio);
// ����GPIO��ƽ
UCHAR ucZigbeeGPIOWrite(USHORT Addr, UCHAR gpio);
// ��ȡAD
UCHAR ucZigbeeAD(USHORT Addr, UCHAR channel, USHORT *pADValue);
// ����
UCHAR ucZigbeeSleep();
// ����ͨѶģʽ
UCHAR ucZigbeeMode(ZIGBEE_MSG_MODE_E mode);
// ��ȡ�ڵ��ź�ǿ��
UCHAR ucZigbeeRSSI(USHORT Addr, UCHAR *pRssi);


// ��ȡ������������
ZIGBEE_ERR_E eZigbeeGetDevInfo(ZIGBEE_DEV_INFO_T *pInfo, UCHAR *pState, USHORT *pType, USHORT *pVer);
// �޸�ͨ����
ZIGBEE_ERR_E eZigbeeSetChan(UCHAR chan);
// ����
ZIGBEE_ERR_E eZigbeeSearch(USHORT *pType, UCHAR *pChan, UCHAR *pSpeed, USHORT *pPanId, USHORT *pAddr, UCHAR *pState);
// ��ȡԶ��������Ϣ
ZIGBEE_ERR_E eZigbeeGetOtherDevInfo(USHORT Addr, ZIGBEE_DEV_INFO_T *pInfo, UCHAR *pState, USHORT *pType, USHORT *pVer);
// �޸���������
ZIGBEE_ERR_E eZigbeeSetDevInfo(USHORT Addr, ZIGBEE_DEV_INFO_T xInfo);
// ��λ
void vZigbeeReset(USHORT Addr, ZIGBEE_DEV_E devType);
// �ָ���������
ZIGBEE_ERR_E eZigbeeSetDefault(USHORT Addr, ZIGBEE_DEV_E devType);
// ��ȡ�ն�����ʱ��
ZIGBEE_ERR_E eZigbeeGetPwdEnable(BOOL *pEn);
// ��ȡ�ն�����ʱ��
ZIGBEE_ERR_E eZigbeeSetPwdEnable(BOOL en);
// �����ն�����ʱ��
ZIGBEE_ERR_E eZigbeeLogin(UCHAR pwd[], UCHAR len);

rt_bool_t bZigbeeInCmd(void);
void vZigbeeRecvHandle(rt_uint8_t value);

BOOL bZigbeeInit(UCHAR ucPort, BOOL uart_default);
void vZigbeeHWReset(void);
BOOL bZigbeeConnected(void);

void vZigbeeLearnNow(void);

void vZigbee_SendBuffer(rt_uint8_t *buffer, int n);

#endif

