#ifndef __SDCCP_TYPES_H__
#define __SDCCP_TYPES_H__

#define SDCCP_PACK_PRECODE_LEN (2)
#define SDCCP_VER (0x00)

#define SDCCP_OK	(0)
#define SDCCP_ERR	(-1)
#define SDCCP_TIMEOUT	(-2)
#define SDCCP_RETRY	(-3)

#define PRECODE_0 (0x12)
#define PRECODE_1 (0xEF)


#define STATUS_OK     (1)
#define STATUS_FAILED (0)



#define SDCCP_CHECK_TIME_OUT(_begin, _t) ((rt_tick_get() - (_begin))  > (_t))	//�жϳ�ʱ

//ע��: CREAT_HEADER �Ѿ����˴�С�˴���, �������ظ�
#define CREAT_HEADER(h_, len_, type_, q_) do { \
		h_.PreCode[0] = PRECODE_0; \
		h_.PreCode[1] = PRECODE_1; \
		h_.Length = htons((len_)); \
		h_.Version = SDCCP_VER; \
		h_.Type = type_; \
		h_.Senquence = q_; \
		h_.CRC8 = btMDCrc8(0, &h_, sizeof(S_PACKAGE_HEAD) - 1);\
	} while (0)

typedef enum {
	FA_ACK		= 0x00,
	FA_POST		= 0x01,
	FA_REQUEST 	= 0x02,
	FA_RESPONSE 	= 0x03,
} S_PACKAGE_FRAME_TYPE_E;

typedef enum {
	MSG_LOGIN = 0x00,		//��¼
	MSG_LOGOUT,			//ע��
	MSG_TIME_SYNC ,			//ʱ��ͬ��
	MSG_SERVER_VERSION,		//�������汾��ȡ
	MSG_DEVICE_HEATBEAT,		//����������
	MSG_DEVICE_STATUS,		//״̬����
	MSG_NOTICE,			//֪ͨ����
	MSG_REALTIME_DATA_UP,		//����ʵʱ����
	MSG_REALTIME_DATA_DOWN,		//��ȡʵʱ����
	MSG_HISTORY_DATA_QUERY,		//��ʷ���ݲ�ѯ
	MSG_HISTORY_DATA_UP,		//�ϴ���ʷ����
	MSG_HISTORY_DATA_DOWN,		//������ʷ����
	MSG_STATISTICS_DATA_UP,		//�ϴ�ͳ������
	MSG_STATISTICS_DATA_DOWN,	//����ͳ������
	MSG_USER_CONFIG_UP,		//�ϴ��û���������
	MSG_USER_CONFIG_DOWN,		//�����û���������
	MSG_DEVICE_CONFIG_UP,		//���Ϳ��ƺ���������
	MSG_DEVICE_CONFIG_DOWN,		//��ȡ���ƺ���������
	MSG_DEVICE_LOG_UP,		//����Log����
	MSG_DEVICE_LOG_DOWN,		//����Log����
	MSG_DEVICE_VERSION,		//���ƺа汾��ȡ
	MSG_UPDATE_UP,			//������������
	MSG_UPDATE_DOWN,		//��ȡ��������
	MSG_ADVICE_UP,			//���ͽ�������
	MSG_ADVICE_DOWN,		//��ȡ��������

	MSG_DEVICE_SET_SAMPLING,	//���ò���״̬
	MSG_DEVICE_GET_SAMPLING,	//��ȡ����״̬

	MSG_DEVICE_GET_PRODUCT_INFO,    //��ȡ��Ʒ��Ϣ
	
    MSG_DEVICE_GET_POWER_STATUS,    //��ȡ���״̬
    MSG_REAL_RAW_DATA_UP,           //����ԭʼ����
} S_PACKAGE_MSG_TYPE_E;

typedef enum {
	SAMPLING_NONE	= 0x00,	//δ�ɼ�
	SAMPLING_ING	= 0x01,	//���ڲɼ�
} S_SAMPLING_STATUS_E;

typedef enum {
	DEVICE_CONFIG_THRESHOLD_GAIN    = 0x00,     // ����ֵ
	DEVICE_CONFIG_THRESHOLD_BODYMOVE_T,         // �嶯�ж�ʱ����ֵ
	DEVICE_CONFIG_THRESHOLD_TURNOVER_T,         // �����ж�ʱ����ֵ

    DEVICE_CONFIG_REAL_STEP         = 0xF0,
    DEVICE_CONFIG_INVALID = 0xFF
} S_DEVICE_CONFIG_TYPE_E;

// ���������
typedef enum {	//�������ͱ�

	// ���ʺ������
	MONITOR_DESC_BR	= 0x00,		//����Ƶ�� 1B ��/��
	MONITOR_DESC_HR,		//����Ƶ�� 1B ��/��
	MONITOR_DESC_BR_HR_STATUS,	//״̬ 1B
	MONITOR_DESC_STATUS_VALUE, 	//��״̬��صļ���ֵ, ʵʱ���� 2B, ��ʷ���� 1B
	MONITOR_DESC_QUALITY,		//˯������

	// �������
	MONITOR_DESC_ENV_TEMPERATURE = 0x10,	//�����¶� 1B
	MONITOR_DESC_ENV_HUMIDITY,		//�����¶� 1B
	MONITOR_DESC_ENV_LIGHT,			//������ǿ 2B
	MONITOR_DESC_ENV_CO2,			//����CO2Ũ�� 2B

	// �������
	MONITOR_DESC_PAD_TEMPERATURE = 0x20,	//�����¶�
	MONITOR_DESC_PAD_HUMIDITY,		//����ʪ��

    
	MONITOR_DESC_RAW = 0x30,        //ԭʼ�ź�
	MONITOR_DESC_BREATH_RAW, 
	MONITOR_DESC_HEART_RAW, 
} S_MONITOR_DESC_TYPE_E;

// SLEEP_INIT ֻ����ʵʱ����
typedef enum {
	SLEEP_OK = 0x00,	//һ������
	SLEEP_INIT,		//��ʼ��״̬��Լ10��ʱ��
	SLEEP_B_STOP,		//������ͣ
	SLEEP_H_STOP,		//������ͣ
	SLEEP_BODYMOVE,		//�嶯
	SLEEP_LEAVE,		//�봲
	SLEEP_TURN_OVER,	//����
} S_SLEEP_STATUS_E;

typedef enum {
	DEV_NORMAL = 0x00,	//һ������
	DEV_LOW_POWER,		//��������
} S_DEVICE_STATUS_TYPE_E;

typedef enum {
	NOTICE_GEBIN_REAL = 0x00,	//��ʼ�鿴ʵʱ����
	NOTICE_END_REAL,            //�����鿴ʵʱ����
	NOTICE_GEBIN_REAL_RAW,      //��ʼ�鿴ԭʼ����
	NOTICE_END_REAL_RAW,        //�����鿴ԭʼ����

	NOTICE_CLEAR_STATUS = 0x10,	//���״̬֪ͨ, Value�洢״̬���� ��:����͵���״̬
} S_NOTICE_TYPE_E;

typedef enum {
	LOGIN_PASSWORD = 0x00,
	LOGIN_EMAIL,
	LOGIN_MEMBER_ID,	//��ԱID
} S_LONIN_TYPE_E;

typedef enum {
	H_QUERY_SUMMARY = 0x00,	//��ѯ��Ҫ��Ϣ
	H_QUERY_RANGE,		//��ѯ���ݱ߽�
} S_HISTORY_QUERY_TYPE_E;

typedef enum {
    H_END_NORMAL = 0x00,    //��������(�û��ֶ������)
    H_END_AUTO,             //�Զ�����(�û����ǵ������)
    H_END_ABORT,            //ǿ����ֹ(�ػ�)
    H_END_ERROR,            //����Ľ���(û����? ϵͳ�쳣?)
} S_HISTORY_STOP_MODE_TYPE_E;

typedef enum {
	U_UP_SUMMARY = 0x00,	//�ϴ���������Ҫ
	U_UP_DETAIL,		//�ϴ�����������
} S_UPDATE_UP_TYPE_E;

typedef enum {
	PACKAGE_ACK_OK = 0x00,		//�ɹ�
	PACKAGE_ACK_INVALID_TYPE,	//֡���ʹ���
	PACKAGE_ACK_INVALID_LENGTH,	//֡�����쳣
	PACKAGE_ACK_INVALID_CHECK_SUM,	//֡У��ʧ��

	PACKAGE_ACK_UNKOWN = 0xFF,	//δ֪����
} S_ACK_TYPE_E;

typedef enum {
	MSG_ERROR_OK = 0x00,		//OK
	MSG_ERROR_INVALID_TYPE,		//�������Ϣ����
	MSG_ERROR_PARAMETER,		//��������(��:������ƥ�䣬�ִ��쳣, �ṹ�����е��������Ͳ����ڣ����øô����ʾ)
	MSG_ERROR_DATABASE,		//���ݿ����
	MSG_ERROR_USERNAME,		//�û�������
	MSG_ERROR_PASSWORD,		//�������
	MSG_ERROR_PRIVILEGE,		//��Ȩ��
	MSG_ERROR_INACTIVE,		//δ����
	MSG_ERROR_DEVICEID,		//�豸ID����
	MSG_ERROR_UNBOUND,		//δ��
	MSG_ERROR_NOT_LOGIN,		//δ��¼
	MSG_ERROR_INVALID_DATA,		//��ʷ���ݴ���
	MSG_ERROR_UPDATE_HW_VER,	//������Ӳ����ƥ��
	MSG_ERROR_UPDATE_CHECK,		//������У�����
	MSG_ERROR_INVALID_SUMMARY,	//û�ж�Ӧ�ĸ�Ҫ��Ϣ
	MSG_ERROR_INVALID_PRODUCT_INFO,	//û����ȷ�ĳ�����Ϣ

	MSG_ERROR_UNKOWN = 0x7F,	//δ֪����
} S_MSG_ERR_TYPE_E;

#pragma pack(1)

typedef struct _S_FIELD_DESC_ {	//�ֶ������ṹ
	byte Type;	//�ֶ�����
	byte Len;	//�ֶ���ռ�ֽ���
} S_FIELD_DESC;

typedef struct _S_STRUCTURE_DESC_ {	//�ṹ������
	byte Count;		//�ֶ�����
	S_FIELD_DESC *p_Desc;	//�ֶ�������
} S_STRUCTURE_DESC;

typedef struct _S_PACKAGE_HEAD {		//����֡ͷ
	byte PreCode[SDCCP_PACK_PRECODE_LEN];	//�����ֽڵ�ǰ����
	ushort Length;				//����֡����
	byte Version;				//����֡�汾
	byte Type;				//֡����
	byte Senquence;				//֡���
	byte CRC8;				//֡ͷУ��
} S_PACKAGE_HEAD;

typedef struct _S_MSG {			//������Ϣ
	byte Type;			//��Ϣ����
	ushort Senquence;		//���
	pbyte Content;			//��Ϣ����
} S_MSG;

typedef struct _S_PACKAGE {	//����֡�ṹ
	S_PACKAGE_HEAD Head;	//����֡ͷ
	S_MSG Msg;		//��Ϣ
	u32 CRC32;		//CRC32У����
} S_PACKAGE;

/************/
//ACK���
typedef struct _S_PACKAGE_ACK {	// ͨ�� ACK �ṹ
	S_PACKAGE_HEAD Head;	//����֡ͷ
	byte AckValue;		//�������
	u32 CRC32;		//CRC32У����
} S_PACKAGE_ACK;

typedef struct _S_MSG_ERR_RESPONSE {
	byte RetCode;
} S_MSG_ERR_RESPONSE;

/************/
//���ƺ�״̬�������
typedef struct _S_DEVICE_STATUS_ITEM {
	byte Type;
	byte Value;
} S_DEVICE_STATUS_ITEM;

typedef struct _S_MSG_DEVICE_STATUS_REPORT {
	byte Count;			//״̬��Ŀ��
	S_DEVICE_STATUS_ITEM *Items;	//״̬����
} S_MSG_DEVICE_STATUS_REPORT;


/************/
//֪ͨ���
typedef struct _S_NOTICE_ITEM {
	byte Type;
	byte Value;
} S_NOTICE_ITEM;

typedef struct _S_MSG_NOTICE_REPORT {	//�鿴�¼�֪ͨ�ṹ
	byte Count;		//֪ͨ��Ŀ��
	S_NOTICE_ITEM *Items;	//֪ͨ����
} S_MSG_NOTICE_REPORT;


/************/
//ʵʱ�������
/***************Lite��ʵʱ��������*****************/
typedef struct _S_REAL_BR_HR {	//������������
	byte BreathRate;	//������
	byte HeartRate;		//����
	byte Status;		//״̬
	ushort Value;		//����ֵ
} S_REAL_BR_HR;
/**************************************************/

typedef struct _S_MSG_REAL_REPORT {
	u32 Time;		//ʵʱ���ݶ�Ӧ��ʱ���
	ushort Offset;  //��� s
	i16 Count;		//��Ŀ
	S_STRUCTURE_DESC Desc;	//�ṹ����, ��Ŀ <= 0 ʱΪ��
	pbyte pRealData;	//ʵʱ������������, ��Ŀ <= 0 ʱΪ��
} S_MSG_REAL_REPORT;

typedef struct {
	mdUINT32 ulIndex;   //��ǰ�������  ÿ�������Զ��ۼ� usCount
	mdUSHORT usOffset;  //��� ms
	mdUSHORT usCount;   //��Ŀ
	mdUSHORT *pRaw;     //ʵʱ������������, ��Ŀ <= 0 ʱΪ��
} S_MsgRealRawReport_t;

/************/
//��¼���
typedef struct _S_MSG_LOGIN_REQUEST {
	byte DeviceID[13];	//�豸ID
	u32 Timestamp;		//��׼ʱ���
	S_STRUCTURE_DESC Desc;	//�ṹ����
	pbyte pData;		//��¼����
} S_MSG_LOGIN_REQUEST;

typedef struct _S_MSG_LOGIN_RESPONSE {
	byte RetCode;		//������
	u32 Timestamp;		//��׼ʱ���
} S_MSG_LOGIN_RESPONSE;


/************/
//ע�����
typedef struct _S_MSG_LOGOUT_REQUEST {
	u32 Timestamp;		//��׼ʱ���
} S_MSG_LOGOUT_REQUEST;

//���¼�ظ�һ��
typedef struct _S_MSG_LOGIN_RESPONSE S_MSG_LOGOUT_RESPONSE;

/************/
//��ʷ���ݲ�ѯ���
typedef struct _S_HISTORY_QUERY_SUMMARY {
	u32 StartTime;		//��ʼʱ���
	u32 EndTime;		//��ֹʱ���
} S_HISTORY_QUERY_SUMMARY;

typedef struct _S_HISTORY_QUERY_SUMMARY S_HISTORY_QUERY_RANGE;

typedef struct _S_HISTORY_SUMMARY {
	u32 StartTime;		//��¼���
	ushort TimeStep;	//��¼���(��Ϊ��λ)
	i32 RecordCount;	//��¼����, 0 ������, < 0 ������ĸ�ֵ��
	byte StopMode;		//״̬(�ο���ʷ�������״̬��)
} S_HISTORY_SUMMARY;

typedef struct _S_HISTORY_RANGE {
	i32 Count;		//��Ŀ��, 0 ������  < 0 �����븺ֵ
	u32 FirstTime;		//ͷʱ���, ��׼ʱ���
	u32 LastTime;		//βʱ���, ��׼ʱ���
} S_HISTORY_RANGE;

typedef struct _S_MSG_HISTORY_QUERY_REQUEST {
	byte Type;		//����
	pbyte pQueryMsg;	//��ѯ���ݣ����Ͳ�ͬ��ѯ�ṹ��ͬ
} S_MSG_HISTORY_QUERY_REQUEST;

typedef struct _S_MSG_HISTORY_QUERY_RESPONSE {
	byte RetCode;		//������
	byte Type;		//����
	i16 Count;		//������, = 0 ��ʾ������ < 0 ��ʾ������ĸ�ֵ
	pbyte pResponseMsg;	//����������, ���Ͳ�ͬ������Ԫ��������ͬ
} S_MSG_HISTORY_QUERY_RESPONSE;


/************/
//��ʷ�����������

/***************Lite����ʷ��������*****************/
typedef struct _S_HISTORY_BR_HR {	//������������
	byte BreathRate;	//������
	byte HeartRate;		//����
	byte Status;		//״̬
	byte Value;		//����ֵ
} S_HISTORY_BR_HR;
/***************Lite����ʷ��������*****************/

typedef struct _S_MSG_HISTORY_DOWN_REQUEST {
	u32 StartTime;		//��ʼʱ��
	u32 StartIndex;		//��ʼλ��
	ushort Count;		//������Ŀ��, ������Ŀ���� <= Count
	S_STRUCTURE_DESC Desc;	//�ṹ����
} S_MSG_HISTORY_DOWN_REQUEST;

typedef struct _S_MSG_HISTORY_DOWN_RESPONSE {
	byte RetCode;
	u32 StartTime;		//��ʼʱ��
	u32 StartIndex;		//��ʼλ��
	i16 Count;		//��Ŀ��, 0 ������  < 0 ������
	S_STRUCTURE_DESC Desc;	//�ṹ����(�ο��ṹ����˵��)
	pbyte pData;		//��ʷ������������
} S_MSG_HISTORY_DOWN_RESPONSE;


/************/
//��ȡ���ƺа汾���
typedef struct _S_VERSION_ {	//�汾�ṹ
	byte Major;		//���汾��
	byte Minor;		//�Ӱ汾��
	//byte Revision;	//�����汾��(��ʹ��)
	//ushort Build;		//����汾��(��ʹ��)
} S_VERSION_T;

// �޳�Ա
//typedef struct _S_MSG_DEVICE_VERSION_REQUEST {
//} S_MSG_DEVICE_VERSION_REQUEST;

typedef struct _S_MSG_DEVICE_VERSION_RESPONSE {
	byte RetCode;
	S_VERSION_T HWVersion;	//Ӳ���汾
	S_VERSION_T SWVersion;	//����汾
} S_MSG_DEVICE_VERSION_RESPONSE;


/************/
//���ƺ��������
typedef struct _S_UPDATE_UP_SUMMARY {
	S_VERSION_T HWVersion;	//Ӳ���汾
	S_VERSION_T SWVersion;	//����汾
	i32 Length;		//����������
	u32 DesCrc32;
	u32 BinCrc32;
} S_UPDATE_UP_SUMMARY;

typedef struct _S_UPDATE_UP_DETAIL {
	u32 StartIndex;		//��ʼλ��
	i16 Count;		//�ֽ���, 0 ������, <0 ������ĸ�ֵ
	pbyte pData;		//��������������
} S_UPDATE_UP_DETAIL;

typedef struct _S_MSG_UPDATE_UP_REQUEST {
	byte Type;		//����(�ο��������������ͱ�)
	pbyte pUpdateData;	//�������ݣ����Ͳ�ͬ�ṹ��ͬ
} S_MSG_UPDATE_UP_REQUEST;

typedef struct _S_MSG_UPDATE_UP_RESPONSE {
	byte RetCode;
	i16 Count;		//�ɹ��洢���ֽ���, 0 ������, <0 ������ĸ�ֵ
} S_MSG_UPDATE_UP_RESPONSE;

// ���òɼ�״̬
// �������ڲɼ�״̬������һ�βɼ�, ����δ�ɼ�״̬��ֹͣһ�βɼ�
typedef struct _S_MSG_SET_SAMPLING_REQUEST {
	u32 Timestamp;		//��׼ʱ���, ���ڼ�¼������ʼ��ʱ��/��ʱ
	byte Status;
} S_MSG_SET_SAMPLING_REQUEST;

// ���òɼ�״̬����
typedef struct _S_MSG_SET_SAMPLING_RESPONSE {
	byte RetCode;
} S_MSG_SET_SAMPLING_RESPONSE;

// ��ѯ�ɼ�״̬, �޲���
//typedef struct _S_MSG_GET_SAMPLING_REQUEST {
//} S_MSG_GET_SAMPLING_REQUEST;

// ��ѯ�ɼ�״̬, ����
typedef struct _S_MSG_GET_SAMPLING_RESPONSE {
	byte RetCode;
	byte Status;
} S_MSG_GET_SAMPLING_RESPONSE;

typedef struct _S_PRODUCT_INFO {	//��Ʒ��Ϣ�洢�ṹ
	S_VERSION_T HWVersion;	//Ӳ���汾
	S_VERSION_T SWVersion;	//����汾
	u32 Timestamp;		//�豸����ʱ��(UNIXʱ���)
	byte DeviceID[13];	//�豸ID
} S_PRODUCT_INFO;

// �޳�Ա
//typedef struct _S_MSG_GET_PRODUCT_INFO_REQUEST {
//} _S_MSG_GET_PRODUCT_INFO_REQUEST;

typedef struct _S_MSG_GET_PRODUCT_INFO_RESPONSE {
	byte RetCode;
	S_PRODUCT_INFO ProductInfo;
} S_MSG_GET_PRODUCT_INFO_RESPONSE;

typedef struct _S_DEVICE_CONFIG_ITEM {
	byte Type;
	u32 Value;
} S_DEVICE_CONFIG_ITEM;

typedef struct _S_MSG_DEVICE_CONFIG_UP_REQUEST {	// ��������
	byte Count;             // ��������
	S_DEVICE_CONFIG_ITEM *Items;	// ����������
} S_MSG_DEVICE_CONFIG_UP_REQUEST;

typedef struct _S_MSG_DEVICE_CONFIG_UP_RESPONSE {	// ��������
	byte RetCode;		//������
} S_MSG_DEVICE_CONFIG_UP_RESPONSE;

typedef struct _S_MSG_DEVICE_CONFIG_DOWN_REQUEST {	// ���ò�ѯ
	byte Count;         // ��������
	byte *TypeList;     // ����������
} S_MSG_DEVICE_CONFIG_DOWN_REQUEST;

typedef struct _S_MSG_DEVICE_CONFIG_DOWN_RESPONSE {	// ���ò�ѯ
	byte RetCode;           //������
	byte Count;             // ��������
	S_DEVICE_CONFIG_ITEM *Items;	// ����������
} S_MSG_DEVICE_CONFIG_DOWN_RESPONSE;

typedef struct _S_MSG_DEVICE_POWER_STATUS_RESPONSE {	// ���״̬��ѯ�ظ�
	byte RetCode;           //������
	byte ChargeStatus;      //���״̬ 0:δ��� 1:���ڳ�� ����:δ֪״̬
	byte BatteryPer;        //�����ٷֱ�, 0xFF ��Чֵ
} S_MSG_DEVICE_POWER_STATUS_RESPONSE;

#pragma pack()

#endif

