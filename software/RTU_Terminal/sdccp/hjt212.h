
#ifndef __HJT212_H__
#define __HJT212_H__

#include "varmanage.h"

#define HJT212_INI_CFG_PATH_PREFIX         "/cfg/hjt212_"

#define HJT212_BUF_SIZE         (2048)
#define HJT212_INBUF_SIZE       (512)
#define HJT212_PARSE_STACK      (2048)      //���������ڴ�ռ��
#define HJT212_LITTLEEDIAN     1

#define HJT212_PRE             0x2323  /* "##" */
#define HJT212_EOM             0x0A0d  /*���кͻس�*/


#if defined(HJT212_LITTLEEDIAN)
#define cc_hjt212_htonl(x)        (x)
#define cc_hjt212_htons(x)        (x)
#elif defined(HJT212_BIGEDIAN)
#define cc_hjt212_htonl(x)        lwip_htonl(x)
#define cc_hjt212_htons(x)        lwip_htons(x)
#else
#error must define EDIAN!
#endif



//#pragma pack(1)


/*��������(һ�����������)

1.��λ�������ֳ���
2.�ֳ���Ӧ������
3.�ֳ���ִ�����󷵻�ִ�н��

*/

typedef enum{
	EXE_RETURN_SUCCESS = 1,  //ִ�гɹ�
	EXE_RETURN_FAILED  = 2,  //ִ��ʧ�ܣ�����֪��ԭ��
	EXE_RETURN_NO_DATA = 100, //û������
}eExeRtn_t;

typedef enum{
	RE_RETURN_SUCCESS = 1, //׼��ִ������
	RE_RETURN_FAILED  = 2, //���󱻾ܾ�
	RE_RETURN_PASSWD_ERR = 3, //�������
}eReRtn_t;


typedef enum{
	FLAG_P = 'P', //��Դ����
	FLAG_F = 'F', //�ŷ�Դͣ��
	FLAG_C = 'C', //��У��
	FLAG_M = 'M', //��ά��
	FLAG_T = 'T', //��������
	FLAG_D = 'D', //����
	FLAG_S = 'S', //�趨ֵ
	FLAG_N = 'N', //����

/*
���ڿ������վ��0��У׼���ݡ�
1�����������2���쳣���ݡ�3
�������ݣ�
*/
	FLAG_0 = '0', //
	FLAG_1 = '1', //
	FLAG_2 = '2', //
	FLAG_3 = '3', //

	FLAG_DISABLE = 254
	
}eRealDataFlag_t;

typedef enum {

/*��ʼ������*/
    CMD_REQUEST_SET_TIMEOUT_RETRY  = 1000,  //�������ó�ʱʱ�����ط�����
    CMD_REQUEST_SET_TIMEOUT_ALARM  = 1001,  // �������ó������ޱ���ʱ��

/*��������*/
    CMD_REQUEST_UPLOAD_TIME = 1011,         // ������ȡ���ϴ��ֳ���ʱ��
    CMD_REQUEST_SET_TIME = 1012,   		 // �����ֳ���ʱ��
    
    CMD_REQUEST_UPLOAD_CONTAM_THRESHOLD = 1021, // ������ȡ���ϴ���Ⱦ�ﱨ������ֵ
    CMD_REQUEST_SET_CONTAM_THRESHOLD    = 1022,       // ������Ⱦ�ﱨ������ֵ
    
    CMD_REQUEST_UPLOAD_UPPER_ADDR = 1031,       // ������ȡ���ϴ���λ����ַ
    CMD_REQUEST_SET_UPPER_ADDR = 1032,          // ������λ����ַ
    
    CMD_REQUEST_UPLOAD_DAY_DATA_TIME = 1041,       // ������ȡ���ϴ��������ϱ�ʱ��
    CMD_REQUEST_SET_DAY_DATA_TIME = 1042,       // �����������ϱ�ʱ��
    
    CMD_REQUEST_UPLOAD_REAL_DATA_INTERVAL = 1061,       // ��ȡʵʱ���ݼ��
    CMD_REQUEST_SET_REAL_DATA_INTERVAL = 1062,       // ����ʵʱ���ݼ��
    
    CMD_REQUEST_SET_PASSWD = 1072,       // ���÷�������

/*��������*/
	
    CMD_REQUEST_UPLOAD_CONTAM_REAL_DATA = 2011,      // �����ȡ���ϴ���Ⱦ��ʵʱ����
    CMD_NOTICE_STOP_CONTAM_REAL_DATA    = 2012,      // ֹͣ�鿴ʵʱ����(֪ͨ����)

	 CMD_REQUEST_UPLOAD_RUNING_STATUS = 2021,			 // �����ȡ���ϴ��豸����״̬����
	 CMD_NOTICE_STOP_RUNING_STATUS = 2022,				 // ֹͣ�鿴�豸����״̬(֪ͨ����)
    
    CMD_REQUEST_UPLOAD_CONTAM_HISTORY_DATA = 2031,       // �����ȡ���ϴ���Ⱦ������ʷ����
    CMD_REQUEST_UPLOAD_RUNING_TIME = 2041,       //  ȡ�豸����ʱ������ʷ����
    
    CMD_REQUEST_UPLOAD_CONTAM_MIN_DATA = 2051,       //  ȡ��Ⱦ���������
    CMD_REQUEST_UPLOAD_CONTAM_HOUR_DATA = 2061,       //  ȡ��Ⱦ��Сʱ����
    
    CMD_REQUEST_UPLOAD_CONTAM_ALARM_RECORD = 2071,       //  ��ȡ���ϴ���Ⱦ�ﱨ����¼
    
    CMD_UPLOAD_CONTAM_ALARM_RECORD = 2072,       //�ϴ���Ⱦ�ﱨ���¼�(֪ͨ�¼������ϴ�)
   

/*��������*/
	 CMD_REQUEST_CHECK = 3011,       //У��У��
	 CMD_REQUEST_SAMPLE = 3012,       //��ʱ��������
	 CMD_REQUEST_CONTROL = 3013,       //�豸��������
	 CMD_REQUEST_SET_SAMPLE_TIME = 3014,       //�����豸����ʱ������

/*��������*/

	 CMD_REQUEST_RESPONSE  = 9011,  //�����ֳ�����Ӧ��λ�������������Ƿ�ִ������
	 CMD_REQUES_RESULT  = 9012,  //�����ֳ�����Ӧ��λ���������ִ�н��
	 CMD_NOTICE_RESPONSE   = 9013,  //���ڻ�Ӧ֪ͨ����
	 CMD_DATA_RESPONSE     = 9014,  //��������Ӧ������
	 CMD_LOGIN			   = 9021,  //�����ֳ�������λ���ĵ�¼����
	 CMD_LOGIN_RESPONSE    = 9022,  //������λ�����ֳ����ĵ�¼Ӧ��

} eHJT212_Cmd_Type_t;

/*ϵͳ���*/

typedef enum{
	ST_01 = 21,	  //�ر�ˮ���
	ST_02 = 22,   //�����������
	ST_03 = 23,	  //���򻷾��������
	
	ST_04 = 31,	  //����������ȾԴ
	ST_05 = 32,	  //�ر�ˮ�廷����ȾԴ
	ST_06 = 33,	  //����ˮ�廷����ȾԴ
	ST_07 = 34,	  //���󻷾���ȾԴ
	ST_08 = 35,	  //����������ȾԴ
	ST_09 = 36,	  //��������ȾԴ
	ST_10 = 37,	  //�񶯻�����ȾԴ
	ST_11 = 38,	  //�����Ի�����ȾԴ
	ST_12 = 41,	  //��Ż�����ȾԴ
	
	ST_HJT212 = 91, //�����ֳ�������λ���Ľ����������ڽ�������
}eHJT212_ST_t; 


typedef enum {
    HJT212_R_S_HEAD     = 0,
    HJT212_R_S_EOM,
} eHJT212_RcvState_t;

typedef enum {
    HJT212_VERIFY_REQ  = 0,
    HJT212_VERIFY_PASS,
} eHJT212_VerifyState_t;

typedef struct
{
	char        ST[5+1];          /*ϵͳ��� ST=31;*/
	char        PW[6+1];          /*�������� PW=123456;*/
	char        MN[14+1];         /*�豸���� MN=12345678901234;*/
	rt_uint32_t  ulPeriod;        /*�ɼ����(ʵʱ�����ϴ����)*/
	rt_uint32_t  verify_flag :1; // �Ƿ�У��
	rt_uint32_t  real_flag   :1; // �Ƿ��ϴ�Сʱ����
	rt_uint32_t  min_flag    :1; // �Ƿ��ϴ���������
	rt_uint32_t  hour_flag   :1; // �Ƿ��ϴ�Сʱ����
	rt_uint32_t  no_min      :1; // �Ƿ��ϴ�min
	rt_uint32_t  no_max      :1; // �Ƿ��ϴ�max
	rt_uint32_t  no_avg      :1; // �Ƿ��ϴ�avg
	rt_uint32_t  no_cou      :1; // �Ƿ��ϴ�cou
} HJT212_Cfg_t;

typedef struct
{
    eHJT212_Cmd_Type_t          eType;
	char QN[20+1];         /*������ QN=20070516010101001;*/
	char PNUM[4+1];        /* PNUM ָʾ����ͨѶ�ܹ������İ���*/
	char PNO[4+1];         /* PNO ָʾ��ǰ���ݰ��İ��� */
	char ST[5+1];          /*ϵͳ��� ST=31;*/
	char CN[7+1];          /*������ CN=2011;*/
	char PW[6+1];          /*�������� PW=123456;*/
	char MN[14+1];         /*�豸���� MN=12345678901234;*/
	char Flag[3+1];        /*��ִ��־ (�Ƿ�����Ӧ�� ��0bit 1Ӧ�� 0��Ӧ�� ��1bit ���ݰ��Ƿ�Ҫ���) */
	char *cp;
} HJT212_Data_t;


typedef struct {
    char *pData;           //��Ч����
} HJT212_Msg_t;


typedef struct {
    rt_uint16_t         usPre;      //ǰ��
    char                btLen[4];      //��Ч���ݳ���
    HJT212_Msg_t        xMsg;       //��Ч����
    char                btCheck[4];    //У��
    rt_uint16_t         usEom;      //�ָ���
} HJT212_Package_t;

//#pragma pack()

rt_bool_t hjt212_open(rt_uint8_t index);
void hjt212_close(rt_uint8_t index);

void hjt212_startwork(rt_uint8_t index);
void hjt212_exitwork(rt_uint8_t index);
rt_err_t HJT212_PutBytes(rt_uint8_t index, rt_uint8_t *pBytes, rt_uint16_t usBytesLen);

rt_bool_t hjt212_req_respons(rt_uint8_t index , eReRtn_t rtn); 	//����Ӧ�� �ֳ���-->��λ��
rt_bool_t hjt212_req_result(rt_uint8_t index, eExeRtn_t rtn);       //���ز���ִ�н�� �ֳ���-->��λ��

rt_bool_t hjt212_login_verify_req(rt_uint8_t index); 	   //��½ע�� �ֳ���-->��λ��
rt_bool_t hjt212_report_real_data(rt_uint8_t index, ExtData_t **ppnode);      //�ϴ�ʵʱ����
rt_bool_t hjt212_report_minutes_data(rt_uint8_t index, ExtData_t **ppnode); //�ϴ���������
rt_bool_t hjt212_report_hour_data(rt_uint8_t index, ExtData_t **ppnode);     //�ϴ�Сʱ����
rt_bool_t hjt212_report_system_time(rt_uint8_t index);  //�ϴ�ϵͳʱ��
rt_err_t _HJT212_PutBytes(rt_uint8_t index, rt_uint8_t *pBytes, rt_uint16_t usBytesLen); //��������


#endif

