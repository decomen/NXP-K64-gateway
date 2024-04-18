
#ifndef __HJT212_H__
#define __HJT212_H__

#include "varmanage.h"

#define HJT212_INI_CFG_PATH_PREFIX         "/cfg/hjt212_"

#define HJT212_BUF_SIZE         (2048)
#define HJT212_INBUF_SIZE       (512)
#define HJT212_PARSE_STACK      (2048)      //解析任务内存占用
#define HJT212_LITTLEEDIAN     1

#define HJT212_PRE             0x2323  /* "##" */
#define HJT212_EOM             0x0A0d  /*换行和回车*/


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


/*请求命令(一般分三个步骤)

1.上位机请求现场机
2.现场机应答请求
3.现场机执行请求返回执行结果

*/

typedef enum{
	EXE_RETURN_SUCCESS = 1,  //执行成功
	EXE_RETURN_FAILED  = 2,  //执行失败，但不知道原因
	EXE_RETURN_NO_DATA = 100, //没有数据
}eExeRtn_t;

typedef enum{
	RE_RETURN_SUCCESS = 1, //准备执行请求
	RE_RETURN_FAILED  = 2, //请求被拒绝
	RE_RETURN_PASSWD_ERR = 3, //密码错误
}eReRtn_t;


typedef enum{
	FLAG_P = 'P', //电源故障
	FLAG_F = 'F', //排放源停运
	FLAG_C = 'C', //：校验
	FLAG_M = 'M', //：维护
	FLAG_T = 'T', //超测上限
	FLAG_D = 'D', //故障
	FLAG_S = 'S', //设定值
	FLAG_N = 'N', //正常

/*
对于空气检测站（0：校准数据、
1：气象参数、2：异常数据、3
正常数据）
*/
	FLAG_0 = '0', //
	FLAG_1 = '1', //
	FLAG_2 = '2', //
	FLAG_3 = '3', //

	FLAG_DISABLE = 254
	
}eRealDataFlag_t;

typedef enum {

/*初始化命令*/
    CMD_REQUEST_SET_TIMEOUT_RETRY  = 1000,  //请求设置超时时间与重发次数
    CMD_REQUEST_SET_TIMEOUT_ALARM  = 1001,  // 请求设置持续超限报警时间

/*参数命令*/
    CMD_REQUEST_UPLOAD_TIME = 1011,         // 请求提取或上传现场机时间
    CMD_REQUEST_SET_TIME = 1012,   		 // 设置现场机时间
    
    CMD_REQUEST_UPLOAD_CONTAM_THRESHOLD = 1021, // 请求提取或上传污染物报警门限值
    CMD_REQUEST_SET_CONTAM_THRESHOLD    = 1022,       // 设置污染物报警门限值
    
    CMD_REQUEST_UPLOAD_UPPER_ADDR = 1031,       // 请求提取或上传上位机地址
    CMD_REQUEST_SET_UPPER_ADDR = 1032,          // 设置上位机地址
    
    CMD_REQUEST_UPLOAD_DAY_DATA_TIME = 1041,       // 请求提取或上传日数据上报时间
    CMD_REQUEST_SET_DAY_DATA_TIME = 1042,       // 设置日数据上报时间
    
    CMD_REQUEST_UPLOAD_REAL_DATA_INTERVAL = 1061,       // 提取实时数据间隔
    CMD_REQUEST_SET_REAL_DATA_INTERVAL = 1062,       // 设置实时数据间隔
    
    CMD_REQUEST_SET_PASSWD = 1072,       // 设置访问密码

/*数据命令*/
	
    CMD_REQUEST_UPLOAD_CONTAM_REAL_DATA = 2011,      // 请求获取或上传污染物实时数据
    CMD_NOTICE_STOP_CONTAM_REAL_DATA    = 2012,      // 停止查看实时数据(通知命令)

	 CMD_REQUEST_UPLOAD_RUNING_STATUS = 2021,			 // 请求获取或上传设备运行状态数据
	 CMD_NOTICE_STOP_RUNING_STATUS = 2022,				 // 停止查看设备运行状态(通知命令)
    
    CMD_REQUEST_UPLOAD_CONTAM_HISTORY_DATA = 2031,       // 请求获取或上传污染物日历史数据
    CMD_REQUEST_UPLOAD_RUNING_TIME = 2041,       //  取设备运行时间日历史数据
    
    CMD_REQUEST_UPLOAD_CONTAM_MIN_DATA = 2051,       //  取污染物分钟数据
    CMD_REQUEST_UPLOAD_CONTAM_HOUR_DATA = 2061,       //  取污染物小时数据
    
    CMD_REQUEST_UPLOAD_CONTAM_ALARM_RECORD = 2071,       //  获取或上传污染物报警记录
    
    CMD_UPLOAD_CONTAM_ALARM_RECORD = 2072,       //上传污染物报警事件(通知事件主动上传)
   

/*反控命令*/
	 CMD_REQUEST_CHECK = 3011,       //校零校满
	 CMD_REQUEST_SAMPLE = 3012,       //即时采样命令
	 CMD_REQUEST_CONTROL = 3013,       //设备操作命令
	 CMD_REQUEST_SET_SAMPLE_TIME = 3014,       //设置设备采样时间周期

/*交互命令*/

	 CMD_REQUEST_RESPONSE  = 9011,  //用于现场机回应上位机的请求。例如是否执行请求
	 CMD_REQUES_RESULT  = 9012,  //用于现场机回应上位机的请求的执行结果
	 CMD_NOTICE_RESPONSE   = 9013,  //用于回应通知命令
	 CMD_DATA_RESPONSE     = 9014,  //用于数据应答命令
	 CMD_LOGIN			   = 9021,  //用于现场机向上位机的登录请求。
	 CMD_LOGIN_RESPONSE    = 9022,  //用于上位机对现场机的登录应答。

} eHJT212_Cmd_Type_t;

/*系统编号*/

typedef enum{
	ST_01 = 21,	  //地表水监测
	ST_02 = 22,   //空气质量监测
	ST_03 = 23,	  //区域环境噪声监测
	
	ST_04 = 31,	  //大气环境污染源
	ST_05 = 32,	  //地表水体环境污染源
	ST_06 = 33,	  //地下水体环境污染源
	ST_07 = 34,	  //海洋环境污染源
	ST_08 = 35,	  //土壤环境污染源
	ST_09 = 36,	  //声环境污染源
	ST_10 = 37,	  //振动环境污染源
	ST_11 = 38,	  //放射性环境污染源
	ST_12 = 41,	  //电磁环境污染源
	
	ST_HJT212 = 91, //用于现场机和上位机的交互，仅用于交互命令
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
	char        ST[5+1];          /*系统编号 ST=31;*/
	char        PW[6+1];          /*访问密码 PW=123456;*/
	char        MN[14+1];         /*设备编码 MN=12345678901234;*/
	rt_uint32_t  ulPeriod;        /*采集间隔(实时数据上传间隔)*/
	rt_uint32_t  verify_flag :1; // 是否校验
	rt_uint32_t  real_flag   :1; // 是否上传小时数据
	rt_uint32_t  min_flag    :1; // 是否上传分钟数据
	rt_uint32_t  hour_flag   :1; // 是否上传小时数据
	rt_uint32_t  no_min      :1; // 是否上传min
	rt_uint32_t  no_max      :1; // 是否上传max
	rt_uint32_t  no_avg      :1; // 是否上传avg
	rt_uint32_t  no_cou      :1; // 是否上传cou
} HJT212_Cfg_t;

typedef struct
{
    eHJT212_Cmd_Type_t          eType;
	char QN[20+1];         /*请求编号 QN=20070516010101001;*/
	char PNUM[4+1];        /* PNUM 指示本次通讯总共包含的包数*/
	char PNO[4+1];         /* PNO 指示当前数据包的包号 */
	char ST[5+1];          /*系统编号 ST=31;*/
	char CN[7+1];          /*命令编号 CN=2011;*/
	char PW[6+1];          /*访问密码 PW=123456;*/
	char MN[14+1];         /*设备编码 MN=12345678901234;*/
	char Flag[3+1];        /*回执标志 (是否数据应答 第0bit 1应答 0不应答 第1bit 数据包是否要拆包) */
	char *cp;
} HJT212_Data_t;


typedef struct {
    char *pData;           //有效数据
} HJT212_Msg_t;


typedef struct {
    rt_uint16_t         usPre;      //前导
    char                btLen[4];      //有效数据长度
    HJT212_Msg_t        xMsg;       //有效数据
    char                btCheck[4];    //校验
    rt_uint16_t         usEom;      //分隔符
} HJT212_Package_t;

//#pragma pack()

rt_bool_t hjt212_open(rt_uint8_t index);
void hjt212_close(rt_uint8_t index);

void hjt212_startwork(rt_uint8_t index);
void hjt212_exitwork(rt_uint8_t index);
rt_err_t HJT212_PutBytes(rt_uint8_t index, rt_uint8_t *pBytes, rt_uint16_t usBytesLen);

rt_bool_t hjt212_req_respons(rt_uint8_t index , eReRtn_t rtn); 	//请求应答 现场机-->上位机
rt_bool_t hjt212_req_result(rt_uint8_t index, eExeRtn_t rtn);       //返回操作执行结果 现场机-->上位机

rt_bool_t hjt212_login_verify_req(rt_uint8_t index); 	   //登陆注册 现场机-->上位机
rt_bool_t hjt212_report_real_data(rt_uint8_t index, ExtData_t **ppnode);      //上传实时数据
rt_bool_t hjt212_report_minutes_data(rt_uint8_t index, ExtData_t **ppnode); //上传分钟数据
rt_bool_t hjt212_report_hour_data(rt_uint8_t index, ExtData_t **ppnode);     //上传小时数据
rt_bool_t hjt212_report_system_time(rt_uint8_t index);  //上传系统时间
rt_err_t _HJT212_PutBytes(rt_uint8_t index, rt_uint8_t *pBytes, rt_uint16_t usBytesLen); //解析函数


#endif

