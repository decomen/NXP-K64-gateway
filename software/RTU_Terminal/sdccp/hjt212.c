
#include "board.h"
//#include "mdqueue.h"

#include <stdio.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include "hjt212.h"
#include "sdccp_net.h"
#include "net_helper.h"


static void Hex2Str(rt_uint8_t hex, char *buf);
static rt_uint16_t hjt212_GetCrc16(char *data_buf , rt_uint32_t len);

rt_bool_t hjt212_req_respons(rt_uint8_t index , eReRtn_t rtn); 	//请求应答 现场机-->上位机
rt_bool_t hjt212_req_result(rt_uint8_t index, eExeRtn_t rtn);       //返回操作执行结果 现场机-->上位机

rt_bool_t hjt212_login_verify_req(rt_uint8_t index); 	   //登陆注册 现场机-->上位机
rt_bool_t hjt212_report_real_data(rt_uint8_t index, ExtData_t **ppnode);      //上传实时数据
rt_bool_t hjt212_report_minutes_data(rt_uint8_t index, ExtData_t **ppnode); //上传分钟数据
rt_bool_t hjt212_report_hour_data(rt_uint8_t index, ExtData_t **ppnode);
rt_bool_t hjt212_report_system_time(rt_uint8_t index);  //上传系统时间
rt_err_t _HJT212_PutBytes(rt_uint8_t index, rt_uint8_t *pBytes, rt_uint16_t usBytesLen); //解析函数

static eHJT212_RcvState_t   s_eRcvState[BOARD_TCPIP_MAX];
static HJT212_Cfg_t         *s_Hj212CfgData[BOARD_TCPIP_MAX];
static rt_thread_t          s_hjt212_work_thread[BOARD_TCPIP_MAX];
//static eHJT212_VerifyState_t s_eVerifyState[BOARD_TCPIP_MAX];

#if 1

unsigned short hjt212_GetCrc16(char *ptr,rt_uint32_t count)
{
	//新建一个16位变量，所有数位初始均为1。
	unsigned short lwrdCrc = 0xFFFF;
	unsigned short lwrdMoveOut;
	int i = 0;
	int lintDataLen;
	int j;
	rt_uint8_t x;
	lintDataLen = count;
	while (i < lintDataLen)
	{
	
		x = (rt_uint8_t)(lwrdCrc >> 8);
		
		lwrdCrc = (unsigned short)(((unsigned short)ptr[i]) ^ x);
		i++;
		j = 0;
		while (j < 8)
		{
			lwrdMoveOut = (unsigned short)(lwrdCrc & 0x0001);
			
			lwrdCrc = (unsigned short)(lwrdCrc>> 1);
			if (lwrdMoveOut == 1)
			{
			
				lwrdCrc = (unsigned short)(lwrdCrc^ 0xA001);
			}
			j++;
		}
		
	}
	//rt_kprintf("crc = %x\n",lwrdCrc);
	return lwrdCrc;
}

#else 

/*--------------------CRC16校验算法：国标--------------------*/
static rt_uint16_t hjt212_GetCrc16(rt_uint8_t *data_buf , rt_uint32_t len)
{
	rt_uint16_t i,crc=0xFFFF;
	rt_uint8_t j;
	for(i=0;i<len;i++){
		crc^=(rt_uint16_t)(data_buf[i]);
		//crc^=(INT16U)(data_buf[i])<<8;
		for(j=0;j<8;j++){
		  if((crc&0x0001)!=0){
		    crc>>=1;
		    crc^=0xA001;
		  }else{
		  	crc>>=1;
		  }
		}
	}
	return(crc);
}

#endif

static void __HJT212_ParsePack(rt_uint8_t index, HJT212_Package_t *pPack);

rt_err_t HJT212_PutBytes(rt_uint8_t index, rt_uint8_t *pBytes, rt_uint16_t usBytesLen) //解析任务
{
    {
        char *pBuffer = (char *)s_pCCBuffer[index];
        rt_base_t nPos = s_CCBufferPos[index];
        eHJT212_RcvState_t eRcvState = s_eRcvState[index];

        for (int n = 0; n < usBytesLen; n++) {
            pBuffer[nPos++] = pBytes[n];
            if (nPos >= HJT212_BUF_SIZE) nPos = 0;
            if (HJT212_R_S_HEAD == eRcvState) {
                if (2 == nPos) {
                    if (HJT212_PRE == *(rt_uint16_t *)pBuffer) {
                        eRcvState = HJT212_R_S_EOM;
                    } else {
                        nPos = 1;
                    }
                }
            } else if (HJT212_R_S_EOM == eRcvState) {
                if (nPos >= 4) {
						//rt_kprintf("HJT212_EOM : %x , eom:%x,nPos = %d\r\n",HJT212_EOM,*(rt_uint16_t *)&pBuffer[nPos - 2],nPos);
                    if ((HJT212_EOM == *(rt_uint16_t *)&pBuffer[nPos - 2] ) && nPos >= 18) {

						rt_kprintf("HJT212_EOM \r\n");
                        HJT212_Package_t xPack;
						memset(&xPack,0,sizeof(HJT212_Package_t));
						
                        memcpy((char*)&xPack.usPre, &pBuffer[0], 6);
                        xPack.xMsg.pData = &pBuffer[6];
                        memcpy(xPack.btCheck, &pBuffer[nPos - 6], 6);
						
						rt_uint32_t ulLen = ((xPack.btLen[0] - '0') * 1000) + ((xPack.btLen[1] - '0') * 100) +
						((xPack.btLen[2] - '0') * 10) + (xPack.btLen[3] - '0');

						//rt_kprintf("ulen = %d\r\n",ulLen);

						rt_uint16_t xCheck = hjt212_GetCrc16((char *)&pBuffer[6], ulLen);
						char  usCheck[4] = {0};
						Hex2Str((rt_uint8_t)(xCheck >> 8), (char *)&usCheck[0]);
						Hex2Str((rt_uint8_t)(xCheck&0xff), (char *)&usCheck[2]);
						
                        //rt_uint32_t usCheck = hjt212_htonl(xPack.usCheck);
                        if ( ( (ulLen + 12) == nPos) &&  (strncmp((char*)xPack.btCheck , (char*)usCheck,4) == 0) ) {
							
                            __HJT212_ParsePack(index, &xPack);
                        }
                        eRcvState = HJT212_R_S_HEAD;
                        nPos = 0;
                    }
                }
            }
        }

        s_CCBufferPos[index] = nPos;
        s_eRcvState[index] = eRcvState;
    }

    return RT_EOK;
}



/*a = atoi(str);d =atof(strd);
    rt_kprintf("%d/n",a);
    rt_kprintf("%g/n",d);
*/

/*

6、给定一个字符串iios/12DDWDFF@122，获取 / 和 @ 之间的字符串，
先将 "iios/"过滤掉，再将非'@'的一串内容送到buf中

sscanf("iios/12DDWDFF@122","%*[^/]/%[^@]",buf);
rt_kprintf("%s\n",buf);
结果为：12DDWDFF


*/

static void __HJT212_ParsePack(rt_uint8_t index, HJT212_Package_t *pPack)
{

	rt_uint32_t ulLen = ((pPack->btLen[0] - '0') * 1000) + ((pPack->btLen[1] - '0') * 100) +
						((pPack->btLen[2] - '0') * 10) + (pPack->btLen[3] - '0');
	
	char  *pBuf = pPack->xMsg.pData;

	HJT212_Data_t xData;
	memset(&xData,0,sizeof(HJT212_Data_t));
	
	int ret = 0;
	char *p = NULL;

	p = strstr(pBuf,"QN=");
	if(p){
		ret = sscanf(p+strlen("QN="),"%20[^;]",xData.QN);
		rt_kprintf("QN=%s\r\n",xData.QN);
	}
	
	
	p = strstr(pBuf,"CN=");
	if(p){
		ret = sscanf(p+strlen("CN="),"%7[^;]",xData.CN);
		rt_kprintf("CN=%s\r\n",xData.CN);
	}

	p = strstr(pBuf,"ST=");
	if(p){
		ret = sscanf(p+strlen("ST="),"%5[^;]",xData.ST);
		rt_kprintf("ST=%s\r\n",xData.ST);
	}

	p = strstr(pBuf,"PW=");
	if(p){
		ret = sscanf(p+strlen("PW="),"%10[^;]",xData.PW);
		rt_kprintf("PW=%s\r\n",xData.PW);
	}

	p = strstr(pBuf,"MN=");
	if(p){
		ret = sscanf(p+strlen("MN="),"%14[^;]",xData.MN);
		rt_kprintf("MN=%s\r\n",xData.MN);
	}

	p = strstr(pBuf,"Flag=");
	if(p){
		ret = sscanf(p+strlen("Flag="),"%3[^;]",xData.Flag);
		rt_kprintf("Flag=%s\r\n",xData.Flag);
	}

	p = strstr(pBuf,"CP=&&");
	if(p){
		//rt_kprintf("CP= %s\r\nlen = %d\r\n",p,strlen(p));
		if(strlen(p) - 13 > 0){
			xData.cp = rt_calloc(1, strlen(p));
			ret = sscanf(p+strlen("CP=&&"),"%1024[^&]",xData.cp);
			rt_kprintf("cp=%s\r\n",xData.cp);
		}
	}

	if(strncmp(s_Hj212CfgData[index]->PW,xData.PW,sizeof(xData.PW)) != 0){
		hjt212_req_respons(index, RE_RETURN_PASSWD_ERR);
		return;
	}

	rt_uint16_t ulCmd = atoi(xData.CN);
	switch(ulCmd){
		case CMD_REQUEST_SET_TIMEOUT_RETRY:{
			rt_kprintf("-->CMD_REQUEST_SET_TIMEOUT_RETRY\r\n");
			break;
		}
		case CMD_REQUEST_SET_TIMEOUT_ALARM:{
			rt_kprintf("-->CMD_REQUEST_SET_TIMEOUT_ALARM\r\n");
			break;
		}
		case CMD_REQUEST_UPLOAD_TIME:{
			rt_kprintf("-->CMD_REQUEST_UPLOAD_TIME\r\n");
			/*一条完整请求通讯过程需要有三个步骤
				1. 上位机请求时间
				2. 现场机请求应答
				3.上传现场机时间
				4.返回操作执行结果
			*/
			hjt212_req_respons(index, RE_RETURN_SUCCESS);
			hjt212_report_system_time(index);
			hjt212_req_result(index, EXE_RETURN_SUCCESS);
			break;
		}
		case CMD_REQUEST_SET_TIME:{
			rt_kprintf("-->CMD_REQUEST_SET_TIME\r\n");
			break;
		}
		case CMD_REQUEST_UPLOAD_CONTAM_THRESHOLD:{
			rt_kprintf("-->CMD_REQUEST_UPLOAD_CONTAM_THRESHOLD\r\n");
			break;
		}
		case CMD_REQUEST_SET_CONTAM_THRESHOLD:{
			rt_kprintf("-->CMD_REQUEST_SET_CONTAM_THRESHOLD\r\n");
			break;
		}
		case CMD_REQUEST_UPLOAD_UPPER_ADDR:{
			rt_kprintf("-->CMD_REQUEST_UPLOAD_UPPER_ADDR\r\n");
			break;
		}
		case CMD_REQUEST_SET_UPPER_ADDR:{
			rt_kprintf("-->CMD_REQUEST_SET_TIMEOUT_ALARM\r\n");
			break;
		}
		case CMD_REQUEST_UPLOAD_DAY_DATA_TIME:{
			rt_kprintf("-->CMD_REQUEST_UPLOAD_DAY_DATA_TIME\r\n");
			break;
		}
		case CMD_REQUEST_SET_DAY_DATA_TIME:{
			rt_kprintf("-->CMD_REQUEST_SET_DAY_DATA_TIME\r\n");
			break;
		}
		case CMD_REQUEST_UPLOAD_REAL_DATA_INTERVAL:{
			rt_kprintf("-->CMD_REQUEST_UPLOAD_REAL_DATA_INTERVAL\r\n");
			break;
		}
		case CMD_REQUEST_SET_REAL_DATA_INTERVAL:{
			rt_kprintf("-->CMD_REQUEST_SET_REAL_DATA_INTERVAL\r\n");
			break;
		}
		
		case CMD_REQUEST_SET_PASSWD:{
			rt_kprintf("-->CMD_REQUEST_SET_PASSWD\r\n");
			break;
		}
		case CMD_REQUEST_UPLOAD_CONTAM_REAL_DATA:{
			rt_kprintf("-->CMD_REQUEST_UPLOAD_CONTAM_REAL_DATA\r\n");
			break;
		}
		case CMD_NOTICE_STOP_CONTAM_REAL_DATA:{
			rt_kprintf("-->CMD_NOTICE_STOP_CONTAM_REAL_DATA\r\n");
			break;
		}
		case CMD_REQUEST_UPLOAD_RUNING_STATUS:{
			rt_kprintf("-->CMD_REQUEST_UPLOAD_RUNING_STATUS\r\n");
			break;
		}
		case CMD_NOTICE_STOP_RUNING_STATUS:{
			rt_kprintf("-->CMD_NOTICE_STOP_RUNING_STATUS\r\n");
			break;
		}
		case CMD_REQUEST_UPLOAD_CONTAM_HISTORY_DATA:{
			rt_kprintf("-->CMD_REQUEST_UPLOAD_CONTAM_HISTORY_DATA\r\n");
			break;
		}
		case CMD_REQUEST_UPLOAD_RUNING_TIME:{
			rt_kprintf("-->CMD_REQUEST_UPLOAD_RUNING_TIME\r\n");
			break;
		}
		case CMD_REQUEST_UPLOAD_CONTAM_MIN_DATA:{
			rt_kprintf("-->CMD_REQUEST_UPLOAD_CONTAM_MIN_DATA\r\n");
			break;
		}
		case CMD_REQUEST_UPLOAD_CONTAM_HOUR_DATA:{
			rt_kprintf("-->CMD_REQUEST_UPLOAD_CONTAM_HOUR_DATA\r\n");
			break;
		}
		case CMD_REQUEST_UPLOAD_CONTAM_ALARM_RECORD:{
			rt_kprintf("-->CMD_REQUEST_UPLOAD_CONTAM_ALARM_RECORD\r\n");
			break;
		}

/*反控命令*/		
		case CMD_REQUEST_CHECK:{
			rt_kprintf("-->CMD_REQUEST_CHECK\r\n");
			break;
		}
		case CMD_REQUEST_SAMPLE:{
			rt_kprintf("-->CMD_REQUEST_SAMPLE\r\n");
			break;
		}
		case CMD_REQUEST_CONTROL:{
			rt_kprintf("-->CMD_REQUEST_CONTROL\r\n");
			break;
		}
		case CMD_REQUEST_SET_SAMPLE_TIME:{
			rt_kprintf("-->CMD_REQUEST_SET_SAMPLE_TIME\r\n");
			break;
		}
		case CMD_LOGIN_RESPONSE:{
			rt_kprintf("-->CMD_LOGIN_RESPONSE\r\n");
			break;
		}
		default:
			break;
		
	}

	if(xData.cp != NULL){
		rt_free(xData.cp);
		xData.cp = NULL;
	}
    
	rt_mq_send(s_CCDataQueue[index], &xData, sizeof(HJT212_Data_t));
}

static void Hex2Str(rt_uint8_t hex, char *buf)
{
	//rt_uint8_t sbuf[2] = {0};
	sprintf(buf,"%02x",hex);
	for(int i = 0 ; i < 2;i++){
		if( buf[i] >='a' && buf[i] <='z'){
			buf[i]= buf[i]-32; 
		}  
	}
	//memcpy(buf,sbuf,2);
}

//不考虑拆分包
static rt_uint32_t Hjt212Encode(char *QN,char *ST,char *CN,char *PW,char *MN,char *Flag,char *CP, char *sBuf)
{
	int index = 6 , i = 0;
	if(QN){
		sBuf[index++] = 'Q';
		sBuf[index++] = 'N';
		sBuf[index++] = '=';
		for(i = 0 ; i < 17; i++){
			if('\0' ==  QN[i]) break;
			sBuf[index++] = QN[i];
		}
		sBuf[index++] = ';';
	}

	if (ST){
	    sBuf[index++]='S';
	    sBuf[index++]='T';
	    sBuf[index++]='=';
	    for (i=0;i<5;i++)
	    {
	      if ('\0' == ST[i]) break;
	      sBuf[index++]=ST[i];
	    }
	    sBuf[index++]=';';
	  }

	if (CN){
	    sBuf[index++]='C';
	    sBuf[index++]='N';
	    sBuf[index++]='=';
	    for (i=0;i<4;i++)
	    {
	      if ('\0' == CN[i]) break;
	      sBuf[index++]=CN[i];
	    }
	    sBuf[index++]=';';
	  }

	 if(PW){
	    sBuf[index++]='P';
	    sBuf[index++]='W';
	    sBuf[index++]='=';
	    for (i=0;i<6;i++)
	    {
	      if ('\0' == PW[i]) break;
	      sBuf[index++] = PW[i];
	    }
	    sBuf[index++]=';';
	  }

	 if(MN){
	    sBuf[index++]='M';
	    sBuf[index++]='N';
	    sBuf[index++]='=';
	    for (i=0;i<14;i++)
	    {
	      if ('\0' == MN[i]) break;
	      sBuf[index++] = MN[i];
	    }
	    sBuf[index++]=';';
	  }

	 if(Flag){
	    sBuf[index++]='F';
	    sBuf[index++]='l';
	    sBuf[index++]='a';
	    sBuf[index++]='g';
	    sBuf[index++]='=';
		sBuf[index++]=Flag[0];
	    sBuf[index++]=';';
	  }

	 if (CP){
	    sBuf[index++]='C';
	    sBuf[index++]='P';
	    sBuf[index++]='=';
	    sBuf[index++]='&';
	    sBuf[index++]='&';
	    for (i=0;i<strlen(CP);i++)
	    {
	      if (CP[i]==0) break;
	      sBuf[index++]=CP[i];
	    }
	    sBuf[index++]='&';
	    sBuf[index++]='&';
	  }

	  sBuf[0]='#';                /*HEAD*/
	  sBuf[1]='#';

	  int length = index - 6;      /*DATA_LENTH*/
	  for (i=0; i<4; i++){
	      sBuf[5-i]='0'+length%10;
	      length/=10;
	  }

	  /*crc16校验*/
	  rt_uint16_t usCrc16 = hjt212_GetCrc16(&sBuf[6], index - 6);
	  rt_uint8_t  high = (rt_uint8_t)(usCrc16 >> 8);
	  rt_uint8_t  low = (rt_uint8_t)(usCrc16 & 0xff);
	  Hex2Str(high,&sBuf[index]);
	  Hex2Str(low,&sBuf[index+2]);

	  /*结束符*/
	  sBuf[index+4] = 0x0d;
	  sBuf[index+5] = 0x0a;
	  index += 6;

	  return index;
	  	
}


/*

要求现场设备每隔 15 分钟发送一次登录指令，中心机收到登录注册指令后作出回
应。约定如果两次（30 分钟）均没有收到回应，则重新复位通讯模块。

*/

rt_bool_t hjt212_login_verify_req(rt_uint8_t index) 	//登陆注册 现场机-->上位机
{
	
#undef _CC_BUF_SZ
#define _CC_BUF_SZ     (256)

	char *buf = rt_calloc(1, _CC_BUF_SZ);
	if (buf != RT_NULL) {
        struct tm lt;
  		struct timeval tp;
        rt_time_t t = time(NULL);
        localtime_r(&t, &lt);
		gettimeofday(&tp, NULL);
		char qn[20] = {0};
		sprintf(qn,"%04d%02d%02d%02d%02d%02d%03d",lt.tm_year+1900,lt.tm_mon+1,lt.tm_mday,lt.tm_hour,lt.tm_min,lt.tm_sec,(tp.tv_usec/1000)%1000);

		char st[5+1] = {0};
		sprintf(st,"%s",s_Hj212CfgData[index]->ST);
		
		char CN[7+1] = {0};
		sprintf(CN,"%d" , CMD_LOGIN);

		//rt_uint8_t flag[0] = '1'; 
		int len = Hjt212Encode(qn, st, CN, s_Hj212CfgData[index]->PW, s_Hj212CfgData[index]->MN, "N", "", buf);
		cc_net_send(index, (const rt_uint8_t *)buf, len);
		
		rt_free(buf);
	}
    return RT_TRUE;
}

rt_bool_t hjt212_req_respons(rt_uint8_t index , eReRtn_t rtn) 	//请求应答 现场机-->上位机
{
	
#undef _CC_BUF_SZ
#define _CC_BUF_SZ     (256)

	char *buf = rt_calloc(1, _CC_BUF_SZ);
	if (buf != RT_NULL) {
        struct tm lt;
  		struct timeval tp;
        rt_time_t t = time(NULL);
        localtime_r(&t, &lt);
		gettimeofday(&tp, NULL);
		char qn[20] = {0};
		sprintf(qn,"%04d%02d%02d%02d%02d%02d%03d",lt.tm_year+1900,lt.tm_mon+1,lt.tm_mday,lt.tm_hour,lt.tm_min,lt.tm_sec,(tp.tv_usec/1000)%1000);

		char st[5+1] = {0};
		sprintf(st,"%s",s_Hj212CfgData[index]->ST);
		
		char CN[7+1] = {0};
		sprintf(CN,"%d" , CMD_REQUEST_RESPONSE);

		char cp[40] = {0};
		sprintf(cp,"QN=%s;QnRtn=%d",qn,rtn);

		//rt_uint8_t flag[0] = '1'; 
		int len = Hjt212Encode(NULL, st, CN, s_Hj212CfgData[index]->PW, s_Hj212CfgData[index]->MN, "0", cp, buf);
		cc_net_send(index, (const rt_uint8_t *)buf, len);
		 
		rt_free(buf);
	}
    return RT_TRUE;
}

 rt_bool_t hjt212_req_result(rt_uint8_t index, eExeRtn_t rtn)  //返回操作执行结果 现场机-->上位机
{
	
#undef _CC_BUF_SZ
#define _CC_BUF_SZ     (256)

	char *buf = rt_calloc(1, _CC_BUF_SZ);
	if (buf != RT_NULL) {
        struct tm lt;
  		struct timeval tp;
        rt_time_t t = time(NULL);
        localtime_r(&t, &lt);
		gettimeofday(&tp, NULL);
		char qn[20] = {0};
		sprintf(qn,"%04d%02d%02d%02d%02d%02d%03d",lt.tm_year + 1900,lt.tm_mon+1,lt.tm_mday,lt.tm_hour,lt.tm_min,lt.tm_sec,(tp.tv_usec/1000)%1000);


		char st[5+1] = {0};
		sprintf(st,"%s",s_Hj212CfgData[index]->ST);

		char CN[7+1] = {0};
		sprintf(CN,"%d" , CMD_REQUES_RESULT);

		char cp[40] = {0};
		sprintf(cp,"QN=%s;ExeRtn=%d",qn,rtn);

		//rt_uint8_t flag[0] = '1'; 
		int len = Hjt212Encode(NULL, st, CN, s_Hj212CfgData[index]->PW, s_Hj212CfgData[index]->MN, "0", cp, buf);
		cc_net_send(index, (const rt_uint8_t *)buf, len);
		 
		rt_free(buf);
	}
    return RT_TRUE;
}



static rt_bool_t hjt212_real_data_format_cp(
    rt_bool_t first,
    char *szFid, 
    float value, 
    eRealDataFlag_t dataflag, 
	char *DataTime,
	char *sCp
)
{
	char tmp[50] = {0};

	if(DataTime && DataTime[0] && szFid && first){
		sprintf(tmp,"DataTime=%s;",DataTime);
		strcat(sCp,tmp);	
	}
	memset(tmp, 0 ,sizeof(tmp));
	
	if(dataflag != FLAG_DISABLE){
		if(szFid){
            sprintf(tmp,"%s-Rtd=%.4f,",szFid,value);
			strcat(sCp,tmp);
		}
		
		memset(tmp,0,sizeof(tmp));
		sprintf(tmp,"%s-Flag=%c",szFid,dataflag);
		strcat(sCp,tmp);
	}else {
		if(szFid){
            sprintf(tmp,"%s-Rtd=%.4f",szFid,value);
			strcat(sCp,tmp);
		}
	}
	return RT_TRUE;
}

rt_bool_t hjt212_report_real_data_test(rt_uint8_t index)  //上传实时数据例子，跟示例文档发的数据一模一样
{
	
#undef _CC_BUF_SZ
#define _CC_BUF_SZ     (1024)

	char *buf = rt_calloc(1, _CC_BUF_SZ);
	if (buf != RT_NULL) {
        struct tm lt;
  		struct timeval tp;
        rt_time_t t = time(NULL);
        localtime_r(&t, &lt);
		gettimeofday(&tp, NULL);
		char qn[20] = {"20160928101320000"};
		//sprintf(qn,"%04d%02d%02d%02d%02d%02d%03d",lt.tm_year + 1900,lt.tm_mon+1,lt.tm_mday,lt.tm_hour,lt.tm_min,lt.tm_sec,(tp.tv_usec/1000)%1000);


		char st[5+1] = {0};
		sprintf(st,"%d",ST_05);

		char CN[7+1] = {0};
		sprintf(CN,"%d" , CMD_REQUEST_UPLOAD_CONTAM_REAL_DATA);

		//rt_uint8_t cp[40] = {0};
		//sprintf(cp,"QN=%s;ExeRtn=%d",rtn);
		
		char *sCp = rt_calloc(1, _CC_BUF_SZ);

	   // time_t now = time(RT_NULL);
	    //struct tm*  = localtime((&now);
	    sprintf(sCp,"DataTime=%s;","20160921085900");
		   
		{//此处读出配置信息，填充数据
			char *szfid[7] = {"Leq","130","111","112","108","109","110"}; 
			char *btValue = "0.000";
			eRealDataFlag_t dataflag = FLAG_N;
			
			for(int i = 0 ; i < 7; i++){
				//sprintf(szfid,"%d",40);
				hjt212_real_data_format_cp(0, szfid[i], 0, dataflag, "", sCp);	 //此函数可循环调用，每调用一次，将新的字符串将连接在后面
				if(i < 6){
					strcat(sCp,";");	 //最后一条数据不需要加";"号
				}
			}
		}

		//rt_uint8_t flag[0] = '1'; 
		int len = Hjt212Encode(NULL, st, CN, "123456", "YCDB0ZE0C00004", NULL, sCp, buf);
		cc_net_send(index, (const rt_uint8_t *)buf, len);
		 
		rt_free(buf);
		rt_free(sCp);
	}
    return RT_TRUE;
}

static void hjt212_minutes_data_format_cp(
    rt_uint8_t index, 
    rt_bool_t  first,
    char *szFid,
	char *BeginTime,
	char *EndTime,
	char *DataTime,
	double Cou, 
	float Min, 
	float Avg, 
	float Max, 
	char *sCp)
{
	char tmp[50] = {0};
	
	if(BeginTime && BeginTime[0] && szFid){
		sprintf(tmp,"BeginTime=%s;",BeginTime);
		strcat(sCp,tmp);
		
	}
    memset(tmp, 0 ,sizeof(tmp));
	
	if(EndTime && EndTime[0] && szFid){
		sprintf(tmp,"EndTime=%s;",EndTime);
		strcat(sCp,tmp);	
	}
	memset(tmp, 0 ,sizeof(tmp));

	if(DataTime && DataTime[0] && szFid && first){
		sprintf(tmp,"DataTime=%s;",DataTime);
		strcat(sCp,tmp);	
	}
	memset(tmp, 0 ,sizeof(tmp));
    
	if(szFid && !s_Hj212CfgData[index]->no_cou){
		//strcat(sCp,",");	
		sprintf(tmp,"%s-Cou=%.4f",szFid,Cou);
		strcat(sCp,tmp);	
	}
	memset(tmp, 0, sizeof(tmp));

	if(szFid && !s_Hj212CfgData[index]->no_min){
		if(!s_Hj212CfgData[index]->no_cou){
			strcat(sCp,",");	
		}
		sprintf(tmp,"%s-Min=%.4f",szFid,Min);
		strcat(sCp,tmp);	
	}
	memset(tmp, 0 ,sizeof(tmp));

	if(szFid && !s_Hj212CfgData[index]->no_avg){
		if(!s_Hj212CfgData[index]->no_cou || !s_Hj212CfgData[index]->no_min){
			strcat(sCp,",");	
		}
		sprintf(tmp,"%s-Avg=%.4f",szFid,Avg);
		strcat(sCp,tmp);	
	}
	memset(tmp, 0 ,sizeof(tmp));

	if(szFid && !s_Hj212CfgData[index]->no_max){
		if(!s_Hj212CfgData[index]->no_cou || !s_Hj212CfgData[index]->no_min || !s_Hj212CfgData[index]->no_avg){
			strcat(sCp,",");	
		}
		sprintf(tmp,"%s-Max=%.4f",szFid,Max);
		strcat(sCp,tmp);	
	}
	memset(tmp, 0 ,sizeof(tmp));

	//strcat(sCp,";");	

	
}

rt_bool_t hjt212_report_real_data(rt_uint8_t index, ExtData_t **ppnode)  //上传分钟数据
{
	
#undef _CC_BUF_SZ
#define _CC_BUF_SZ     (1024)

	char *buf = rt_calloc(1, _CC_BUF_SZ);
	if (buf != RT_NULL) {
        struct tm lt;
  		struct timeval tp;
        rt_time_t t = time(NULL);
        localtime_r(&t, &lt);
		gettimeofday(&tp, NULL);
		char qn[20] = {0};
		sprintf(qn,"%04d%02d%02d%02d%02d%02d%03d",lt.tm_year + 1900,lt.tm_mon+1,lt.tm_mday,lt.tm_hour,lt.tm_min,lt.tm_sec,(tp.tv_usec/1000)%1000);

		char st[5+1] = {0};
		sprintf(st,"%s",s_Hj212CfgData[index]->ST);

		char CN[7+1] = {0};
		sprintf(CN,"%d" , CMD_REQUEST_UPLOAD_CONTAM_REAL_DATA);

		//rt_uint8_t cp[40] = {0};
		//sprintf(cp,"QN=%s;ExeRtn=%d",rtn);

        char mynid[32] = {0};
		char *sCp = rt_calloc(1, _CC_BUF_SZ);
        {
            struct tm lt;
            rt_time_t t = time(NULL);
            localtime_r(&t, &lt);
			char datatime[20] = {0};
		    sprintf(datatime,"%04d%02d%02d%02d%02d%02d",lt.tm_year + 1900,lt.tm_mon+1,lt.tm_mday,lt.tm_hour,lt.tm_min,lt.tm_sec);
            ExtData_t* node = *ppnode;
            rt_enter_critical();
            {
                rt_bool_t first = RT_TRUE;
                while (1) {
                    node = pVarManage_GetExtDataWithUpProto(node, PROTO_DEV_NET_TYPE(index), index, PROTO_HJT212);
                    if (node) {
                        var_double_t ext_value = 0;
                        if (node->xUp.bEnable) {
                            // 分钟数据
                            char *newnid = node->xUp.szNid ? node->xUp.szNid : s_Hj212CfgData[index]->MN;
                            if(mynid[0] == '\0' || 0 == strcmp(newnid, mynid)) {
                                strcpy(mynid, newnid);
                                if (rt_tick_get() - node->xUp.xAvgUp.time >= s_Hj212CfgData[index]->ulPeriod) {
                                    if (node->xUp.xAvgUp.count > 0) {
                                        var_double_t value = (node->xUp.xAvgUp.val_avg / node->xUp.xAvgUp.count);

                                        memset(buf, 0, _CC_BUF_SZ);
                                        hjt212_real_data_format_cp(first, node->xUp.szFid, (float)value, FLAG_N, datatime, buf);

                                        first = RT_FALSE;

                                        int sCPlen = strlen(sCp);
                                        if(sCPlen + strlen(buf) < 960) {
                                            strcat(sCp, buf);
                                            strcat(sCp,";");
                                            *ppnode = node;
                                            
                                            node->xUp.xAvgUp.val_avg = 0;
                                            node->xUp.xAvgUp.val_min = 0;
                                            node->xUp.xAvgUp.val_max = 0;
                                            node->xUp.xAvgUp.count = 0;
                                            node->xUp.xAvgUp.time = rt_tick_get();
                                        } else {
                                            break;
                                        }
                                    }
                                }
                            } else {
                                break;
                            }
                        } else {
                            *ppnode = node;
                        }
                    } else {
                        *ppnode = node;
                        break;
                    }
                }
            }
            rt_exit_critical();
        }

        int sCPlen = strlen(sCp);
        if(sCPlen > 0) {
            sCp[sCPlen-1] = '\0';
    		//rt_uint8_t flag[0] = '1'; 
    		int len = Hjt212Encode(NULL, st, CN, s_Hj212CfgData[index]->PW, mynid, NULL, sCp, buf);
    		cc_net_send(index, (const rt_uint8_t *)buf, len);
		}
		 
		rt_free(buf);
		rt_free(sCp);
	}
    return RT_TRUE;
}

rt_bool_t hjt212_report_minutes_data(rt_uint8_t index, ExtData_t **ppnode)  //上传分钟数据
{
	
#undef _CC_BUF_SZ
#define _CC_BUF_SZ     (1024)

	char *buf = rt_calloc(1, _CC_BUF_SZ);
	if (buf != RT_NULL) {
        struct tm lt;
  		struct timeval tp;
        rt_time_t t = time(NULL);
        localtime_r(&t, &lt);
		gettimeofday(&tp, NULL);
		char qn[20] = {0};
		sprintf(qn,"%04d%02d%02d%02d%02d%02d%03d",lt.tm_year + 1900,lt.tm_mon+1,lt.tm_mday,lt.tm_hour,lt.tm_min,lt.tm_sec,(tp.tv_usec/1000)%1000);

		char st[5+1] = {0};
		sprintf(st,"%s",s_Hj212CfgData[index]->ST);

		char CN[7+1] = {0};
		sprintf(CN,"%d" , CMD_REQUEST_UPLOAD_CONTAM_MIN_DATA);

		//rt_uint8_t cp[40] = {0};
		//sprintf(cp,"QN=%s;ExeRtn=%d",rtn);

        char mynid[32] = {0};
		char *sCp = rt_calloc(1, _CC_BUF_SZ);
        {
            struct tm lt;
            rt_time_t t = time(NULL);
            localtime_r(&t, &lt);
			char datatime[20] = {0};
		    sprintf(datatime,"%04d%02d%02d%02d%02d%02d",lt.tm_year + 1900,lt.tm_mon+1,lt.tm_mday,lt.tm_hour,lt.tm_min,lt.tm_sec);
			char *BeginTime = datatime;
			char *EndTime = datatime;
            ExtData_t* node = *ppnode;
            rt_enter_critical();
            {
                rt_bool_t first = RT_TRUE;
                while (1) {
                    node = pVarManage_GetExtDataWithUpProto(node, PROTO_DEV_NET_TYPE(index), index, PROTO_HJT212);
                    if (node) {
                        var_double_t ext_value = 0;
                        if (node->xUp.bEnable) {
                            // 分钟数据
                            char *newnid = node->xUp.szNid ? node->xUp.szNid : s_Hj212CfgData[index]->MN;
                            if(mynid[0] == '\0' || 0 == strcmp(newnid, mynid)) {
                                strcpy(mynid, newnid);
                                if (rt_tick_get() - node->xUp.xAvgMin.time >= rt_tick_from_millisecond(60 * 1000)) {
                                    if (node->xUp.xAvgMin.count > 0) {
                                        var_double_t value = (node->xUp.xAvgMin.val_avg / node->xUp.xAvgMin.count);

                                        memset(buf, 0, _CC_BUF_SZ);
                                        hjt212_minutes_data_format_cp(
                                            index, first, 
                                            node->xUp.szFid, RT_NULL, RT_NULL, datatime, node->xUp.xAvgMin.val_avg, 
                                            node->xUp.xAvgMin.val_min, value, node->xUp.xAvgMin.val_max, buf);

                                        first = RT_FALSE;

                                        int sCPlen = strlen(sCp);
                                        if(sCPlen + strlen(buf) < 960) {
                                            strcat(sCp, buf);
                                            strcat(sCp,";");
                                            *ppnode = node;
                                            
                                            node->xUp.xAvgMin.val_avg = 0;
                                            node->xUp.xAvgMin.val_min = 0;
                                            node->xUp.xAvgMin.val_max = 0;
                                            node->xUp.xAvgMin.count = 0;
                                            node->xUp.xAvgMin.time = rt_tick_get();
                                        } else {
                                            break;
                                        }
                                    }
                                }
                            } else {
                                break;
                            }
                        } else {
                            *ppnode = node;
                        }
                    } else {
                        *ppnode = node;
                        break;
                    }
                }
            }
            rt_exit_critical();
        }

        int sCPlen = strlen(sCp);
        if(sCPlen > 0) {
            sCp[sCPlen-1] = '\0';
    		//rt_uint8_t flag[0] = '1'; 
    		int len = Hjt212Encode(NULL, st, CN, s_Hj212CfgData[index]->PW, mynid, NULL, sCp, buf);
    		cc_net_send(index, (const rt_uint8_t *)buf, len);
		}
		 
		rt_free(buf);
		rt_free(sCp);
	}
    return RT_TRUE;
}

rt_bool_t hjt212_report_hour_data(rt_uint8_t index, ExtData_t **ppnode)  //上传小时数据
{
	
#undef _CC_BUF_SZ
#define _CC_BUF_SZ     (1024)

	char *buf = rt_calloc(1, _CC_BUF_SZ);
	if (buf != RT_NULL) {
        struct tm lt;
  		struct timeval tp;
        rt_time_t t = time(NULL);
        localtime_r(&t, &lt);
		gettimeofday(&tp, NULL);
		char qn[20] = {0};
		sprintf(qn,"%04d%02d%02d%02d%02d%02d%03d",lt.tm_year + 1900,lt.tm_mon+1,lt.tm_mday,lt.tm_hour,lt.tm_min,lt.tm_sec,(tp.tv_usec/1000)%1000);

		char st[5+1] = {0};
		sprintf(st,"%s",s_Hj212CfgData[index]->ST);

		char CN[7+1] = {0};
		sprintf(CN,"%d" , CMD_REQUEST_UPLOAD_CONTAM_HOUR_DATA);

		//rt_uint8_t cp[40] = {0};
		//sprintf(cp,"QN=%s;ExeRtn=%d",rtn);

        char mynid[32] = {0};
		char *sCp = rt_calloc(1, _CC_BUF_SZ);
        {
            struct tm lt;
            rt_time_t t = time(NULL);
            localtime_r(&t, &lt);
			char datatime[20] = {0};
		    sprintf(datatime,"%04d%02d%02d%02d%02d%02d",lt.tm_year + 1900,lt.tm_mon+1,lt.tm_mday,lt.tm_hour,lt.tm_min,lt.tm_sec);
			char *BeginTime = datatime;
			char *EndTime = datatime;
            ExtData_t* node = *ppnode;
            rt_enter_critical();
            {
                rt_bool_t first = RT_TRUE;
                while (1) {
                    node = pVarManage_GetExtDataWithUpProto(node, PROTO_DEV_NET_TYPE(index), index, PROTO_HJT212);
                    if (node) {
                        //var_double_t ext_value = 0;
                        if (node->xUp.bEnable) {
                            // 小时数据
                            char *newnid = node->xUp.szNid ? node->xUp.szNid : s_Hj212CfgData[index]->MN;
                            if(mynid[0] == '\0' || 0 == strcmp(newnid, mynid)) {
                                strcpy(mynid, newnid);
                                if (rt_tick_get() - node->xUp.xAvgHour.time >= rt_tick_from_millisecond(60 * 60 * 1000)) {
                                    if (node->xUp.xAvgHour.count > 0) {
                                        var_double_t value = (node->xUp.xAvgHour.val_avg / node->xUp.xAvgHour.count);

                                        memset(buf, 0, _CC_BUF_SZ);
                                        hjt212_minutes_data_format_cp(
                                            index, first,
                                            node->xUp.szFid, RT_NULL, RT_NULL, datatime, node->xUp.xAvgHour.val_avg, 
                                            node->xUp.xAvgHour.val_min, value, node->xUp.xAvgHour.val_max, buf);

                                        first = RT_FALSE;

                                        int sCPlen = strlen(sCp);
                                        if(sCPlen + strlen(buf) < 960) {
                                            strcat(sCp, buf);
                                            strcat(sCp,";");
                                            *ppnode = node;

                                            node->xUp.xAvgHour.val_avg = 0;
                                            node->xUp.xAvgHour.val_min = 0;
                                            node->xUp.xAvgHour.val_max = 0;
                                            node->xUp.xAvgHour.count = 0;
                                            node->xUp.xAvgHour.time = rt_tick_get();
                                        } else {
                                            break;
                                        }
                                    }
                                }
                            } else {
                                break;
                            }
                        } else {
                            *ppnode = node;
                        }
                    } else {
                        *ppnode = node;
                        break;
                    }
                }
            }
            rt_exit_critical();
        }

        int sCPlen = strlen(sCp);
        if(sCPlen > 0) {
            sCp[sCPlen-1] = '\0';
    		//rt_uint8_t flag[0] = '1'; 
    		int len = Hjt212Encode(NULL, st, CN, s_Hj212CfgData[index]->PW, mynid, NULL, sCp, buf);
    		cc_net_send(index, (const rt_uint8_t *)buf, len);
		}
		 
		rt_free(buf);
		rt_free(sCp);
	}
    return RT_TRUE;
}

rt_bool_t hjt212_report_system_time(rt_uint8_t index)  //上传系统时间
{
	
#undef _CC_BUF_SZ
#define _CC_BUF_SZ     (1024)

	char *buf = rt_calloc(1, _CC_BUF_SZ);
	if (buf != RT_NULL) {
        struct tm lt;
  		struct timeval tp;
        rt_time_t t = time(NULL);
        localtime_r(&t, &lt);
		gettimeofday(&tp, NULL);
		char qn[20] = {0};
		sprintf(qn,"%04d%02d%02d%02d%02d%02d%03d",lt.tm_year + 1900,lt.tm_mon+1,lt.tm_mday,lt.tm_hour,lt.tm_min,lt.tm_sec,(tp.tv_usec/1000)%1000);

		char st[5+1] = {0};
		sprintf(st,"%s",s_Hj212CfgData[index]->ST);

		char CN[7+1] = {0};
		sprintf(CN,"%d" , CMD_REQUEST_UPLOAD_TIME);

		char sCp[60] = {0};
		sprintf(sCp,"QN=%s;",qn);

		char systime[20] = {0};
		sprintf(systime,"SystemTime=%04d%02d%02d%02d%02d%02d",lt.tm_year + 1900,lt.tm_mon+1,lt.tm_mday,lt.tm_hour,lt.tm_min,lt.tm_sec);

		strcat(sCp,systime);

		//rt_uint8_t flag[0] = '1'; 
		int len = Hjt212Encode(NULL, st, CN, s_Hj212CfgData[index]->PW, s_Hj212CfgData[index]->MN, NULL, sCp, buf);
		cc_net_send(index, (const rt_uint8_t *)buf, len);
		rt_free(buf);
	}
    return RT_TRUE;
}


 rt_bool_t hjt212_request_get_system_time(rt_uint8_t index)  //上位机-->现场机 用于测试解析
{
	
#undef _CC_BUF_SZ
#define _CC_BUF_SZ     (256)

	char *buf = rt_calloc(1, _CC_BUF_SZ);
	if (buf != RT_NULL) {
        struct tm lt;
  		struct timeval tp;
        rt_time_t t = time(NULL);
        localtime_r(&t, &lt);
		gettimeofday(&tp, NULL);
		char qn[20] = {0};
		sprintf(qn,"%04d%02d%02d%02d%02d%02d%03d",lt.tm_year + 1900,lt.tm_mon+1,lt.tm_mday,lt.tm_hour,lt.tm_min,lt.tm_sec,(tp.tv_usec/1000)%1000);


		char st[5+1] = {0};
		sprintf(st,"%s",s_Hj212CfgData[index]->ST);

		char CN[7+1] = {0};
		sprintf(CN,"%d" , CMD_REQUEST_UPLOAD_TIME);

		//rt_uint8_t flag[0] = '1'; 
		int len = Hjt212Encode(NULL, st, CN, s_Hj212CfgData[index]->PW, s_Hj212CfgData[index]->MN, "1", "\0", buf);
		
		//cc_net_send(index, buf, len);

		// rt_kprintf("len = %d, strlen = %d\r\n",len,strlen(buf));
		 rt_kprintf("%s",buf);
		 /*
		 for(int i = 0; i < len; i++ ) {
       		 bMDQueueWrite( s_xBleRcvQueue, buf[i] );
   		 }
		 OSQueueMsgSend( MSG_BLE_RCV_DATA);
                 */
		 
		rt_free(buf);
	}
    return RT_TRUE;
}


/*
static void prvTestTask(rt_uint8_t index)
{
	  rt_uint8_t *buf = NULL;
	  rt_uint16_t nReadLen = 0;
     _HJT212_PutBytes(index, buf, nReadLen);           
}
*/

static void prvHjt212DefaultCfg(int index)
{
	memcpy(s_Hj212CfgData[index]->ST , "31", 5);
	memcpy(s_Hj212CfgData[index]->PW , "123456", 6);
	memcpy(s_Hj212CfgData[index]->MN , "12345678901234", 14);
	if(g_host_cfg.szId[0]) {
	    strncpy(s_Hj212CfgData[index]->MN , g_host_cfg.szId, 14+1);
	}
}

rt_bool_t hjt212_open(rt_uint8_t index)
{
    hjt212_close(index);

    RT_KERNEL_FREE((void *)s_Hj212CfgData[index]);
    s_Hj212CfgData[index] = RT_KERNEL_CALLOC(sizeof(HJT212_Cfg_t));

    RT_KERNEL_FREE((void *)s_pCCBuffer[index]);
    s_pCCBuffer[index] = RT_KERNEL_CALLOC(HJT212_BUF_SIZE);
    s_CCBufferPos[index] = 0;
    s_eRcvState[index] = HJT212_R_S_HEAD;

    prvHjt212DefaultCfg(index);
    {
        char buf[64] = "";
        ini_t *ini;
        sprintf(buf, HJT212_INI_CFG_PATH_PREFIX "%d" ".ini", index);
        ini = ini_load(buf);

        const char *str = ini_getstring(ini, "common", "st", "");
        if(str[0]) strncpy(s_Hj212CfgData[index]->ST, str, sizeof(s_Hj212CfgData[index]->ST));
        str = ini_getstring(ini, "common", "pw", "");
        if(str[0]) strncpy(s_Hj212CfgData[index]->PW, str, sizeof(s_Hj212CfgData[index]->PW));
        str = ini_getstring(ini, "common", "mn", "");
        if(str[0]) strncpy(s_Hj212CfgData[index]->MN, str, sizeof(s_Hj212CfgData[index]->MN));
        s_Hj212CfgData[index]->ulPeriod = ini_getint(ini, "common", "period", 1000);
        if(NET_IS_GPRS(index)) {
            if(s_Hj212CfgData[index]->ulPeriod < 3000) s_Hj212CfgData[index]->ulPeriod = 3000;
        } else {
            if(s_Hj212CfgData[index]->ulPeriod < 1000) s_Hj212CfgData[index]->ulPeriod = 1000;
        }
        s_Hj212CfgData[index]->verify_flag = ini_getboolean(ini, "common", "verify_flag", 0);
        s_Hj212CfgData[index]->real_flag = ini_getboolean(ini, "common", "real_flag", 0);
        s_Hj212CfgData[index]->min_flag = ini_getboolean(ini, "common", "min_flag", 0);
        s_Hj212CfgData[index]->hour_flag = ini_getboolean(ini, "common", "hour_flag", 0);
        s_Hj212CfgData[index]->no_cou = ini_getboolean(ini, "common", "no_cou", 0);
        s_Hj212CfgData[index]->no_min = ini_getboolean(ini, "common", "no_min", 0);
        s_Hj212CfgData[index]->no_max = ini_getboolean(ini, "common", "no_max", 0);
        s_Hj212CfgData[index]->no_avg = ini_getboolean(ini, "common", "no_avg", 0);
        
        ini_free(ini);
    }

    BOARD_CREAT_NAME(szMq, "bjmq_%d", index);
    s_CCDataQueue[index] = rt_mq_create(szMq, sizeof(HJT212_Data_t), 3, RT_IPC_FLAG_PRIO);
    return RT_TRUE;
}

void hjt212_close(rt_uint8_t index)
{
    for (int i = 0; i < 5 * 10; i++) {
        if (s_hjt212_work_thread[index]) {
            rt_thread_delay(RT_TICK_PER_SECOND / 10);
        } else {
            break;
        }
    }

    hjt212_exitwork(index);

    if (s_CCDataQueue[index]) {
        rt_mq_delete(s_CCDataQueue[index]);
        s_CCDataQueue[index] = RT_NULL;
    }
    RT_KERNEL_FREE((void *)s_pCCBuffer[index]);
    s_pCCBuffer[index] = RT_NULL;
}

static void hjt212_work_task(void *parameter);

void hjt212_startwork(rt_uint8_t index)
{
    if(RT_NULL == s_hjt212_work_thread[index]) {
        BOARD_CREAT_NAME(szWork, "hjt212_%d", index);
        s_hjt212_work_thread[index] = \
            rt_thread_create(szWork,
                                    hjt212_work_task,
                                    (void *)index,
                                    2048,
                                    20, 20);
        if (s_hjt212_work_thread[index] != RT_NULL) {
            rt_thddog_register(s_hjt212_work_thread[index], 30);
            rt_thread_startup(s_hjt212_work_thread[index]);
        }
    }
}

void hjt212_exitwork(rt_uint8_t index)
{
    // do nothing
    /*if (s_hjt212_work_thread[index]) {
        if (RT_EOK == rt_thread_delete(s_hjt212_work_thread[index])) {
            s_hjt212_work_thread[index] = RT_NULL;
        }
    }*/
}

static void hjt212_work_task(void *parameter)
{
    rt_uint8_t index = (int)(rt_uint32_t)parameter;
    while(1) {
        HJT212_Data_t xData;
        //tcpip_cfg_t *pcfg = &g_tcpip_cfgs[index];
        HJT212_Cfg_t *hjtcfg = s_Hj212CfgData[index];
        rt_uint32_t ulHeartbeatTick;
        int login_err_count = 0;

_SATRT:
        rt_thddog_feed("net_waitconnect");
        net_waitconnect(index);
        {
            //var_int32_t up_interval = lVarManage_GetExtDataUpInterval(PROTO_DEV_NET_TYPE(index), index);
            //int period = up_interval>0?(up_interval/60):180;

            rt_kprintf("...start verify\n");
            rt_kprintf("hjt212_login_verify_req\n");
            rt_thddog_suspend("hjt212_login_verify_req");
            hjt212_login_verify_req(index);
// 暂时不需要校验过程
            if(hjtcfg->verify_flag) {
                while (1) {
                    //s_eVerifyState[index] = HJT212_VERIFY_REQ;
                    rt_kprintf("hjt212_login_verify_req\n");
                    rt_thddog_suspend("hjt212_login_verify_req");
                    hjt212_login_verify_req(index);
                    rt_thddog_suspend("rt_mq_recv verify");
                    if (RT_EOK == rt_mq_recv(s_CCDataQueue[index], &xData, sizeof(HJT212_Data_t), 15* RT_TICK_PER_SECOND)) {
                        //if (CMD_LOGIN_RESPONSE == xData.eType) {
                            // 这里可以对时 xData.QN
                            break;
                        //}
                    } else {
                        rt_thddog_suspend("cc_net_disconnect");
                        cc_net_disconnect(index);
                        rt_thread_delay(5 * RT_TICK_PER_SECOND);
                        goto _SATRT;
                    }
                }
            }
            rt_kprintf("...hjt212_login_verify ok !\n");
            ulHeartbeatTick = rt_tick_get();
            while (1) {
                ExtData_t *node = RT_NULL;
                //rt_thddog_feed("UpdateAvgValue");
                rt_thddog_feed("net_waitconnect UpdateAvgValue");
                net_waitconnect(index);
                rt_enter_critical();
                {
                    while (1) {
                        node = pVarManage_GetExtDataWithUpProto(node, PROTO_DEV_NET_TYPE(index), index, PROTO_HJT212);
                        if (node) {
                            var_double_t ext_value = 0;
                            if (node->xUp.bEnable) {
                                if (bVarManage_GetExtValue(node, &ext_value)) {
                                    if(hjtcfg->real_flag) {
                                        bVarManage_UpdateAvgValue(&node->xUp.xAvgUp, ext_value);
                                    }
                                    if(hjtcfg->min_flag) {
                                        bVarManage_UpdateAvgValue(&node->xUp.xAvgMin, ext_value);
                                    }
                                    if(hjtcfg->hour_flag) {
                                        bVarManage_UpdateAvgValue(&node->xUp.xAvgHour, ext_value);
                                    }
                                }
                            }
                        } else {
                            break;
                        }
                    }
                }
                rt_exit_critical();

                rt_thddog_suspend("rt_mq_recv wait");
                if (RT_EOK == rt_mq_recv(s_CCDataQueue[index], &xData, sizeof(HJT212_Data_t), 5 * RT_TICK_PER_SECOND)) {
                    switch (xData.eType) {
                    case CMD_REQUEST_SET_REAL_DATA_INTERVAL:
                    {
                        rt_kprintf("recv CMD_REQUEST_SET_REAL_DATA_INTERVAL !\n");
                        break;
                    }
                    default: 
                    {
                        rt_kprintf("recv %d\n", xData.eType);
                        break;
                    }
                    }
                }
                rt_thddog_resume();

                // 每 15 分钟, 发送一次心跳
                if (rt_tick_get() - ulHeartbeatTick >= rt_tick_from_millisecond(15 * 60 * 1000)) {
                    rt_thddog_suspend("hjt212_login_verify_req 15 min heart");
                    hjt212_login_verify_req(index);
                    rt_thddog_resume();
                    ulHeartbeatTick = rt_tick_get();
                    // 不需要等待回复
                    /*if (RT_EOK == rt_mq_recv(s_CCDataQueue[index], &xData, sizeof(HJT212_Data_t), 10 * RT_TICK_PER_SECOND)) {
                        if (CMD_LOGIN_RESPONSE == xData.eType) {
                            // 这里可以对时 xData.QN
                            login_err_count = 0;
                        }
                    } else {
                        login_err_count++;
                    }
                    // 第一次 0, 第二次 15 分钟, 第三次 30 分钟
                    if(login_err_count >= 3) {
                        cc_net_disconnect(index);
                        rt_thread_delay(5 * RT_TICK_PER_SECOND);
                        goto _SATRT;
                    }*/
                }

                // 根据配置上报数据
                {
                    //up_interval = lVarManage_GetExtDataUpInterval(PROTO_DEV_NET_TYPE(index), index);
                    if(hjtcfg->min_flag || hjtcfg->hour_flag || hjtcfg->real_flag) {
                        node = RT_NULL;
                        if (hjtcfg->real_flag) {
                            while(1) {
                                rt_thddog_suspend("hjt212_report_real_data");
                                hjt212_report_real_data(index, &node);
                                rt_thddog_resume();
                                if(!node) break;
                            }
                        }
                        node = RT_NULL;
                        if (hjtcfg->min_flag) {
                            while(1) {
                                rt_thddog_suspend("hjt212_report_minutes_data");
                                hjt212_report_minutes_data(index, &node);
                                rt_thddog_resume();
                                if(!node) break;
                            }
                        }
                        node = RT_NULL;
                        if (hjtcfg->hour_flag) {
                            while(1) {
                                rt_thddog_suspend("hjt212_report_hour_data");
                                hjt212_report_hour_data(index, &node);
                                rt_thddog_resume();
                                if(!node) break;
                            }
                        }
                    }
                }
            }
        }
    }

    s_hjt212_work_thread[index] = RT_NULL;
    rt_thddog_unreg_inthd();
}



