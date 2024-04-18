#ifndef DLT645_H
#define DLT645_H

#include "mdtypedef.h"

#define DLT645_PRE             0x68  /* ��ʼ�ַ� */
#define DLT645_MID             0x68  /* ��������ʼ�ַ� */
#define DLT645_EOM             0x16  /* �����ַ� */



//���������� D0-D4

typedef enum{
	C_CODE_REV = 0x00, 		   //����
	C_CODE_CHECK_TIME = 0X08,     //�㲥��ʱ
	C_CODE_READ_DATA =  0x11,     //������
	C_CODE_READ_LAST_DATA = 0x12, //����������
	C_CODE_READ_ADDR = 0x13,      //��ͨѶ��ַ
	C_CODE_WRITE_DATA = 0x14,      //д����
	C_CODE_WRITE_ADDR = 0x15,       //дͨѶ��ַ
	C_CODE_FREEZE_CDM = 0x16,       //��������
	C_CODE_SET_BAUD =   0x17,       //�޸�ͨѶ������
	C_CODE_SET_PASSWD = 0x18,       //�޸�����
	C_CODE_MAX_DEMAND_CLEAR = 0x19,   //�����������
	C_CODE_METER_CLEAR = 0x1A,   //�������
	C_CODE_EVENT_CLEAR = 0x1B,   //�¼�����
}S_DLT645_C_CODE_E;

typedef enum{
	C_DIR_HOST = 0,   //��վ����������֡
	C_DIR_SLAVE = 1,   //��վ������Ӧ��֡
}S_DLT645_C_DIR_E;

typedef enum{
	C_ACK_OK = 0,  //��վ��ȷӦ��
	C_ACK_ERR = 1, //��վ�쳣Ӧ��
}S_DLT645_C_ACK_E;

typedef enum{
	C_FCK_0 = 0,  //�޺�������֡
	C_FCK_1 = 1,  //�к�������֡
}S_DLT645_C_FCK_E;



//���������ݱ�ʶ

//�޹����ܵ�λ��kvarh ��ǧ��ʱ��
//�й����ܵ�λ��kWh ��ǧ��ʱ)
typedef enum{

	ENERGY_DATA_MARK_A0	= 0x00000000, //(��ǰ)����й��ܵ���
	ENERGY_DATA_MARK_A1	= 0x00000100, //(��ǰ)����1���⣩
	ENERGY_DATA_MARK_A2	= 0x00000200, //(��ǰ)����2���壩
	ENERGY_DATA_MARK_A3	= 0x00000300, //(��ǰ)����3��ƽ��
	ENERGY_DATA_MARK_A4	= 0x00000400, //(��ǰ)����4���ȣ�
	
	ENERGY_DATA_MARK_B0	= 0x00010000, //(��ǰ)�����й��ܵ���
	ENERGY_DATA_MARK_B1	= 0x00010100, //(��ǰ)����1���⣩
	ENERGY_DATA_MARK_B2	= 0x00010200, //(��ǰ)����2���壩
	ENERGY_DATA_MARK_B3	= 0x00010300, //(��ǰ)����3��ƽ��
	ENERGY_DATA_MARK_B4	= 0x00010400, //(��ǰ)����4���ȣ�
	
	
	ENERGY_DATA_MARK_C0	= 0x00020000, //(��ǰ)�����й��ܵ���
	ENERGY_DATA_MARK_C1	= 0x00020100, //(��ǰ)����1���⣩
	ENERGY_DATA_MARK_C2	= 0x00020200, //(��ǰ)����2���壩
	ENERGY_DATA_MARK_C3	= 0x00020300, //(��ǰ)����3��ƽ��
	ENERGY_DATA_MARK_C4	= 0x00020400, //(��ǰ)����4���ȣ�
	
	
	ENERGY_DATA_MARK_D0	= 0x00030000, //(��ǰ)����޹�1�ܵ���
	ENERGY_DATA_MARK_D1	= 0x00030100, //(��ǰ)����1���⣩
	ENERGY_DATA_MARK_D2	= 0x00030200, //(��ǰ)����2���壩
	ENERGY_DATA_MARK_D3	= 0x00030300, //(��ǰ)����3��ƽ��
	ENERGY_DATA_MARK_D4	= 0x00030400, //(��ǰ)����4���ȣ�
	
	ENERGY_DATA_MARK_E0	= 0x00040000, //(��ǰ)����޹�2�ܵ���
	ENERGY_DATA_MARK_E1	= 0x00040100, //(��ǰ)����1���⣩
	ENERGY_DATA_MARK_E2	= 0x00040200, //(��ǰ)����2���壩
	ENERGY_DATA_MARK_E3	= 0x00040300, //(��ǰ)����3��ƽ��
	ENERGY_DATA_MARK_E4	= 0x00040400, //(��ǰ)����4���ȣ�
	
	
	ENERGY_DATA_MARK_F0	= 0x00050000, //(��ǰ)��һ�����޹��ܵ���
	ENERGY_DATA_MARK_F1	= 0x00050100, //(��ǰ)����1���⣩
	ENERGY_DATA_MARK_F2	= 0x00050200, //(��ǰ)����2���壩
	ENERGY_DATA_MARK_F3	= 0x00050300, //(��ǰ)����3��ƽ��
	ENERGY_DATA_MARK_F4	= 0x00050400, //(��ǰ)����4���ȣ�
	
	ENERGY_DATA_MARK_G0	= 0x00060000, //(��ǰ)�ڶ������޹��ܵ���
	ENERGY_DATA_MARK_G1	= 0x00060100, //(��ǰ)����1���⣩
	ENERGY_DATA_MARK_G2	= 0x00060200, //(��ǰ)����2���壩
	ENERGY_DATA_MARK_G3	= 0x00060300, //(��ǰ)����3��ƽ��
	ENERGY_DATA_MARK_G4	= 0x00060400, //(��ǰ)����4���ȣ�
	
	ENERGY_DATA_MARK_H0	= 0x00070000, //(��ǰ)���������޹��ܵ���
	ENERGY_DATA_MARK_H1	= 0x00070100, //(��ǰ)����1���⣩
	ENERGY_DATA_MARK_H2	= 0x00070200, //(��ǰ)����2���壩
	ENERGY_DATA_MARK_H3	= 0x00070300, //(��ǰ)����3��ƽ��
	ENERGY_DATA_MARK_H4	= 0x00070400, //(��ǰ)����4���ȣ�
	
	ENERGY_DATA_MARK_J0	= 0x00080000, //(��ǰ)���������޹��ܵ���
	ENERGY_DATA_MARK_J1	= 0x00080100, //(��ǰ)����1���⣩
	ENERGY_DATA_MARK_J2	= 0x00080200, //(��ǰ)����2���壩
	ENERGY_DATA_MARK_J3	= 0x00080300, //(��ǰ)����3��ƽ��
	ENERGY_DATA_MARK_J4	= 0x00080400, //(��ǰ)����4���ȣ�

	
	ENERGY_DATA_MARK_20	= 0x00150000, //(��ǰ)A�������й�����
	ENERGY_DATA_MARK_21	= 0x00160000, //(��ǰ)A�෴���й�����
	ENERGY_DATA_MARK_22	= 0x00170000, //(��ǰ)A������޹�1����
	ENERGY_DATA_MARK_23	= 0x00180000, //(��ǰ)A������޹�2����
	ENERGY_DATA_MARK_24	= 0x00190000, //(��ǰ)A���һ�����޹�����
	ENERGY_DATA_MARK_25	= 0x001a0000, //(��ǰ)A��ڶ������޹�����
	ENERGY_DATA_MARK_26	= 0x001b0000, //(��ǰ)A����������޹�����
	ENERGY_DATA_MARK_27	= 0x001c0000, //(��ǰ)A����������޹�����

	ENERGY_DATA_MARK_40	= 0x00290000, //(��ǰ)B�������й�����
	ENERGY_DATA_MARK_41	= 0x002A0000, //(��ǰ)B�෴���й�����
	ENERGY_DATA_MARK_42	= 0x002B0000, //(��ǰ)B������޹�1����
	ENERGY_DATA_MARK_43	= 0x002C0000, //(��ǰ)B������޹�2����
	ENERGY_DATA_MARK_44	= 0x002D0000, //(��ǰ)B���һ�����޹�����
	ENERGY_DATA_MARK_45	= 0x002E0000, //(��ǰ)B��ڶ������޹�����
	ENERGY_DATA_MARK_46	= 0x002F0000, //(��ǰ)B����������޹�����
	ENERGY_DATA_MARK_47	= 0x00300000, //(��ǰ)B����������޹�����

	ENERGY_DATA_MARK_60	= 0x003D0000, //(��ǰ)C�������й�����
	ENERGY_DATA_MARK_61	= 0x003E0000, //(��ǰ)C�෴���й�����
	ENERGY_DATA_MARK_62	= 0x003F0000, //(��ǰ)C������޹�1����
	ENERGY_DATA_MARK_63	= 0x00400000, //(��ǰ)C������޹�2����
	ENERGY_DATA_MARK_64	= 0x00410000, //(��ǰ)C���һ�����޹�����
	ENERGY_DATA_MARK_65	= 0x00420000, //(��ǰ)C��ڶ������޹�����
	ENERGY_DATA_MARK_66	= 0x00430000, //(��ǰ)C����������޹�����
	ENERGY_DATA_MARK_67	= 0x00440000, //(��ǰ)C����������޹�����

	
}S_DLT645_ENERGY_DATA_MARKER_E;

// ��������ݱ�ʶ�����

//��ѹ��λ V ����
//������λ A ����
//�й����ʵ�λ kw ǧ��
//�޹����ʵ�λ kvar ǧ��
//���ڹ��ʵ�λ kVA 
//�������ص�λ �޵�λ 
//��ǵ�λ ��(��) 

typedef enum{

	VAR_DATA_MARK_00	= 0x02010100,  // A���ѹ
	VAR_DATA_MARK_01	= 0x02010200,  // B���ѹ
	VAR_DATA_MARK_02	= 0x02010300,  // C���ѹ
	VAR_DATA_MARK_03	= 0x0201FF00,  // ��ѹ���ݿ�

	VAR_DATA_MARK_04	= 0x02020100,  // A�����
	VAR_DATA_MARK_05	= 0x02020200,  // B�����
	VAR_DATA_MARK_06	= 0x02020300,  // C�����
	VAR_DATA_MARK_07	= 0x0202FF00,  // �������ݿ�

	VAR_DATA_MARK_08	= 0x02030000, // ˲ʱ���й�����
	VAR_DATA_MARK_09	= 0x02030100, // ˲ʱA���й�����
	VAR_DATA_MARK_0A	= 0x02030200, // ˲ʱB���й�����
	VAR_DATA_MARK_0B	= 0x02030300, // ˲ʱC���й�����
	VAR_DATA_MARK_0C	= 0x0203FF00, // ˲ʱ�й��������ݿ�

	VAR_DATA_MARK_0D	= 0x02040000, // ˲ʱ���޹�����
	VAR_DATA_MARK_0E	= 0x02040100, // ˲ʱA���޹�����
	VAR_DATA_MARK_0F	= 0x02040200, // ˲ʱB���޹�����
	VAR_DATA_MARK_10	= 0x02040300, // ˲ʱC���޹�����
	VAR_DATA_MARK_11	= 0x0204FF00, // ˲ʱ�޹��������ݿ�

	VAR_DATA_MARK_12	= 0x02050000, // ˲ʱ�����ڹ���
	VAR_DATA_MARK_13	= 0x02050100, // ˲ʱA�����ڹ���
	VAR_DATA_MARK_14	= 0x02050200, // ˲ʱB�����ڹ���
	VAR_DATA_MARK_15	= 0x02050300, // ˲ʱC�����ڹ���
	VAR_DATA_MARK_16	= 0x0205FF00, // ˲ʱ���ڹ������ݿ�


	VAR_DATA_MARK_17	= 0x02060000, // �ܹ�������
	VAR_DATA_MARK_18	= 0x02060100, // A�๦������
	VAR_DATA_MARK_19	= 0x02060200, // B�๦������
	VAR_DATA_MARK_1A	= 0x02060300, // C�๦������
	VAR_DATA_MARK_1B	= 0x0206FF00, // �����������ݿ�

	VAR_DATA_MARK_1C	= 0x02070100, // A�����
	VAR_DATA_MARK_1D	= 0x02070200, // B�����
	VAR_DATA_MARK_1E	= 0x02070300, // C�����
	VAR_DATA_MARK_1F	= 0x02070400, // ������ݿ�
	
}S_DLT645_VAR_DATA_MARKER_E;

#pragma pack(1)

//������
typedef struct{
	mdBYTE C_CODE   :5; //D0-D4 ������
	mdBYTE FCK  	  :1; // 0 �޺�������֡��1�к�������֡
	mdBYTE ACK      :1; //ACK = 0 ��վ��ȷӦ�� ACK = 1 ��վ�쳣Ӧ��
	mdBYTE DIR      :1; //���䷽��λDIR 0��վ���������б���  1 ��վ������Ӧ��֡
}S_DLT645_C_t;

//��ַ��
typedef struct{
	mdBYTE  addr[6];
}S_DLT645_A_t;


typedef struct {
    mdBYTE 		  btPre;     //��ʼ�ַ�      1
    S_DLT645_A_t  xAddr;     //��ַ��        6
    mdBYTE        btMid;     //��ʼ�ַ�		 1
    S_DLT645_C_t  xCon; 	 //������        1
    mdBYTE		  btLen;	 //�����򳤶�    1
    pbyte         pData;     //������ 		 n
    mdBYTE        btCheck;   //֡У���      1
    mdBYTE        btEom;     //�����ַ�      1   
} S_DLT645_Package_t;


typedef struct{
	mdBYTE YY; // ��
	mdBYTE MM; //��
	mdBYTE DD; //��
	mdBYTE hh; //ʱ   
	mdBYTE mm; //��  
	mdBYTE ss; //�� 
}S_DLT645_Time_t;

//ͨѶ����״̬��
typedef enum{
	BAUD_600BPS   = (1<<1),
	BAUD_1200BPS  = (1<<2),
	BAUD_2400BPS  = (1<<3),
	BAUD_4800BPS  = (1<<4),
	BAUD_9600BPS  = (1<<5),
	BAUD_19200BPS = (1<<6),
}S_DLT645_BAUD_t;

typedef struct {
    int index;
    S_DLT645_A_t    addr;
	mdUINT32        op;
	float           val;
} S_DLT645_Result_t;

#pragma pack()

#ifdef __cplusplus  
extern "C" {  
#endif  

//void vDlt645RequestCheckTime(S_DLT645_A_t xaddr, S_DLT645_Time_t *ptime);    //�㲥��ʱ
//void vDlt645ReadAddr(S_DLT645_A_t xaddr);    //��ȡ�豸ͨѶ��ַ
//void vDlt645SetBaud(S_DLT645_A_t xaddr, S_DLT645_BAUD_t baud);     //����ͨѶ������ Ĭ��2400
rt_bool_t bDlt645ReadData(int index, S_DLT645_A_t xaddr, mdUINT32 DataMarker, int timeout, S_DLT645_Result_t *result);

typedef void (*pDlt645SendDataFun)(int index,mdBYTE *pdata,mdBYTE len);

//typedef void (*pDlt645Debug)(const char *fmt,...);
//extern pDlt645Debug DLT645_debug;

extern const unsigned int g_ENERGY_DATA_MARKER_E[];
extern const unsigned int g_VAR_DATA_MARKER_E[];

mdBOOL Dlt645_PutBytes(int index, mdBYTE *pBytes, mdUSHORT usBytesLen); //����������ÿ�յ�һ�����ݵ���һ��
void vDlt645ResponAddr();    //�ӻ������豸��ַ(���ڲ���)

#ifdef __cplusplus  
}  
#endif 

#endif
