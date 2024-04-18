#include <board.h>
#include <queue.h>
#include <stdio.h>

static struct rt_mutex gprs_mutex;
static struct rt_event s_xIdeEvent;

// GPRS执行状态机
typedef enum {
    GPRS_S_WAIT,    // 等待
    GPRS_S_EXE,     // 执行
    GPRS_S_OK,      // 完成
    GPRS_S_FAIL,    // 失败
} eGPRS_Status;

typedef struct {
    rt_bool_t bUsed;
    rt_uint8_t btType;          // TCP: SOCK_STREAM,  UDP: SOCK_DGRAM
    rt_bool_t bClient;
    rt_uint8_t *pInBuffer;
    rt_size_t uBufsize;
    void (*onAccept)(rt_uint8_t btId);

    void (*onRecv)(rt_uint8_t btId, void *pBuffer, rt_size_t uSize);

    eGPRS_Status eConn;
    eGPRS_Status eRead;
    eGPRS_Status eWrite;
} GPRS_Socket_t;

static GPRS_Socket_t s_xSocketList[GPRS_SOCKET_MAX];

GPRS_NState_t g_xGPRS_NState;
GPRS_SState_t g_xGPRS_SState[GPRS_SOCKET_MAX];
GPRS_COPN_t g_xGPRS_COPN;
UGPRS_SMOND_t g_xGPRS_SMOND;
GPRS_COPS_t g_xGPRS_COPS;
GPRS_SEND_ACK_t g_xGPRS_SACK;

rt_base_t g_nGPRS_CSQ = UINT32_MAX;
rt_base_t g_nGPRS_CREG = UINT32_MAX;
rt_base_t g_nGPRS_MUX = -1;

static rt_serial_t *serial = RT_NULL;

static rt_bool_t _gprs_net_time_flag = RT_FALSE;

typedef struct {
    const char *szCode;
    rt_uint8_t const btLen;
} GPRSCode_t;

typedef struct {
    const char *szEvt;
    rt_uint8_t const btLen;
} GPRSEvt_t;

static const GPRSCode_t c_xGPRSCodeList[GPRS_CODE_MAX] = {
    [GPRS_CODE_OK]                  = { "OK", 2 },
    [GPRS_CODE_ERROR]               = { "ERROR", 5 },
};

// 这个值根据下面的内容调整
#define GPRS_EVT_SZ_MIN     (2)
static const GPRSEvt_t c_xGPRS_EvtList[GPRS_EVT_MAX] = {
    [GPRS_EVT_COPS]         = { "+COPS: ", 7 },
    [GPRS_EVT_CSQ]          = { "+CSQ: ", 6 },
    [GPRS_EVT_CREG]         = { "+CGREG: ", 8},
    [GPRS_EVT_MUX]         =  { "+QIMUX: ", 8},
    [GPRS_EVT_SISO]         = { "+QISTAT: ", 9 }, //查询socket状态
    [GPRS_EVT_NWTIME]       = { "+CCLK: ", 7 },

    [GPRS_EVT_SISR]         = { "+RECEIVE: ", 10 },  //接收数据是以这个开头的完整一帧是 +RECEIVE: 1, 7 1表示哪一路 7表示是数据长度
    [GPRS_EVT_CONNECT]         = { "0, CONNECT OK", 13 },  //socket连接状态
    [GPRS_EVT_SEND_CMD]     = {"> ",2},
	[GPRS_EVT_SEND_OK]     = {"SEND OK",7},  
	[GPRS_EVT_SOCKET_CLOSE]     = {"0, CLOSE OK",11},  
	[GPRS_EVT_SEDN_ACK]    = {"+QISACK: ",9},

};

#define RX_BUFFER_SIZE      (1400)

static rt_uint8_t s_btRxBuffer[RX_BUFFER_SIZE];

static rt_bool_t s_bRcvCode[GPRS_CODE_MAX];

static rt_bool_t s_bRcvEvt[GPRS_EVT_MAX];
static rt_bool_t s_bIsEvtParse[GPRS_EVT_MAX];
static rt_uint32_t s_ulRcvEvtTick[GPRS_EVT_MAX];

static rt_base_t s_nRxOffset = 0;


static void prvClearRcvBuffer(void);
static void prvClearRcvCode(void);
static void prvClearRcvEvt(void);
static rt_bool_t prvIsRcvAnyEvt(void);
static rt_bool_t prvIsInParseAnyEvt(void);
static void prvParseEvt(void);
static void prvParseCOPS(void);
static void prvParseSMOND(void);
static void prvParseCSQ(void);
static void prvParseCREG(void);
static void prvParseMux(void);
static void prvParseSISO(void);
static void prvParseSendCmd(void);
static void prvParseSendOk(void);
static void prvParseSendAck(void);


//static void prvParseSISI( void );
static void prvParseConnect(void);
static void prvParseClose(void);

static void prvParseSISR(void);
static void prvParseNWTIME(void);

static void prvParseCode(void);
static rt_err_t prvOnDataRecv(rt_device_t dev, rt_size_t size);

static rt_bool_t prvWaitEvt(eGPRS_Evt eEvt, rt_bool_t bCheckErr, rt_base_t nMs);
static rt_bool_t prvWaitResult(rt_base_t nMs);

static void prvSendCmd(const char *szCmd);
static void prvGPRS_Reset(void);

rt_bool_t bGPRS_SocketStateTest(rt_uint8_t btSock);


// AT 指令适当延时
#define GPRS_AT_DELAY()         rt_thread_delay( RT_TICK_PER_SECOND / 10 )

#define GPRS_EVENT_IDE          (0x01)

static rt_err_t prvOnIDE(rt_device_t dev, rt_size_t size)
{
    rt_event_send(&s_xIdeEvent, GPRS_EVENT_IDE);

    return RT_EOK;
}

static void rt_gprsparse_thread(void *parameter)
{
    rt_uint32_t xRcvEvt = 0;
    static rt_uint8_t buffer[300];

    while (1) {
        rt_thddog_suspend("rt_event_recv");
        rt_event_recv(&s_xIdeEvent, GPRS_EVENT_IDE, RT_EVENT_FLAG_OR | RT_EVENT_FLAG_CLEAR, RT_WAITING_FOREVER, &xRcvEvt);
        rt_thddog_resume();
        while (1) {
            int nRead = serial->parent.read(&(serial->parent), 0, buffer, sizeof(buffer));
            if (nRead <= 0) break;
            for (int i = 0; i < nRead; i++) {
                s_btRxBuffer[s_nRxOffset++] = buffer[i];
                if (s_nRxOffset >= RX_BUFFER_SIZE) {
                    s_nRxOffset = 0;
                }

                if (!prvIsRcvAnyEvt() && s_nRxOffset >= GPRS_EVT_SZ_MIN) {
                    prvParseEvt();
                }


                if (s_bRcvEvt[GPRS_EVT_COPS] && !s_bIsEvtParse[GPRS_EVT_COPS]) {
                    prvParseCOPS();
                }


                if (s_bRcvEvt[GPRS_EVT_CSQ] && !s_bIsEvtParse[GPRS_EVT_CSQ]) {
                    prvParseCSQ();
                }

                if (s_bRcvEvt[GPRS_EVT_CREG] && !s_bIsEvtParse[GPRS_EVT_CREG]) {
                    prvParseCREG();
                }

				 if (s_bRcvEvt[GPRS_EVT_SEND_CMD] && !s_bIsEvtParse[GPRS_EVT_SEND_CMD]) {
                    prvParseSendCmd();
                }

				 if (s_bRcvEvt[GPRS_EVT_SEND_OK] && !s_bIsEvtParse[GPRS_EVT_SEND_OK]) {
                    prvParseSendOk();
                }

				  if (s_bRcvEvt[GPRS_EVT_SEDN_ACK] && !s_bIsEvtParse[GPRS_EVT_SEDN_ACK]) {
                    prvParseSendAck();
                }
				 
                if (s_bRcvEvt[GPRS_EVT_MUX] && !s_bIsEvtParse[GPRS_EVT_MUX]) {
                    prvParseMux();
                }

				

               
                if (s_bRcvEvt[GPRS_EVT_SISO] && !s_bIsEvtParse[GPRS_EVT_SISO]) {
                    prvParseSISO();
                }

                //if( s_bRcvEvt[GPRS_EVT_SISI] && !s_bIsEvtParse[GPRS_EVT_SISI] ) {
                //    prvParseSISI();
                //}

                if (s_bRcvEvt[GPRS_EVT_CONNECT] && !s_bIsEvtParse[GPRS_EVT_CONNECT]) {
                    prvParseConnect();
                }

				if (s_bRcvEvt[GPRS_EVT_SOCKET_CLOSE] && !s_bIsEvtParse[GPRS_EVT_SOCKET_CLOSE]) {
                    prvParseClose();
                }

                if (s_bRcvEvt[GPRS_EVT_SISR] && !s_bIsEvtParse[GPRS_EVT_SISR]) {
                    prvParseSISR();
                }

			
                if (s_bRcvEvt[GPRS_EVT_NWTIME] && !s_bIsEvtParse[GPRS_EVT_NWTIME]) {
                    prvParseNWTIME();
                }

				

                if (!prvIsRcvAnyEvt()) {

                    // 匹配到\r\n
                    if (s_nRxOffset > 1 && '\r' == s_btRxBuffer[s_nRxOffset - 2] && '\n' == s_btRxBuffer[s_nRxOffset - 1]) {
                        if (s_nRxOffset >= 2) {
                            prvParseCode();
                            s_nRxOffset = 0;
                        }
                    }
                }
            }
        }
    }
    rt_thddog_exit();
}

static rt_bool_t prvIsRcvAnyEvt(void)
{
    for (int i = 0; i < GPRS_EVT_MAX; i++) {
        if (s_bRcvEvt[i]) {
            return RT_TRUE;
        }
    }
    return RT_FALSE;
}

static rt_bool_t prvIsInParseAnyEvt(void)
{
    for (int i = 0; i < GPRS_EVT_MAX; i++) {
        if (s_bIsEvtParse[i]) {
            return RT_TRUE;
        }
    }
    return RT_FALSE;
}


static void prvParseEvt(void)
{
    for (int i = 0; i < GPRS_EVT_MAX; i++) {
        rt_uint8_t btLen = c_xGPRS_EvtList[i].btLen;

		if(btLen == s_nRxOffset){
			int Ret = -1;
			if(i == GPRS_EVT_CONNECT){
				//char tmp_Evt[20] = {0};
				//sprintf(tmp_Evt,"%d, CONNECT OK",gnSocket);
				if(strstr((char const *)s_btRxBuffer, "CONNECT OK") != NULL){
					Ret = 0;
				}
			}else if(i == GPRS_EVT_SOCKET_CLOSE){
				if(strstr((char const *)s_btRxBuffer, "CLOSE OK") != NULL){
					Ret = 0;
				}
				
			}else {
				Ret = strncmp((char const *)s_btRxBuffer, c_xGPRS_EvtList[i].szEvt, btLen);
			}
			if(Ret == 0){
				if (!s_bRcvEvt[i]) {
	              	  s_bRcvEvt[i] = RT_TRUE;
	              	  s_bIsEvtParse[i] = RT_FALSE;
	              }
	          	  s_ulRcvEvtTick[i] = rt_tick_get();
	          	  break;
			}
		}
    }
}

static void prvClearEvt(rt_uint8_t btCode)
{
    if (btCode < GPRS_EVT_MAX) {
        s_bRcvEvt[btCode] = RT_FALSE;
        s_bIsEvtParse[btCode] = RT_FALSE;
    }
}

static rt_bool_t prvParseTimeout(int index, int ms)
{
    if (rt_tick_get() - s_ulRcvEvtTick[index] > rt_tick_from_millisecond(ms)) {
        s_bRcvEvt[index] = RT_FALSE;
        s_bIsEvtParse[index] = RT_FALSE;
        s_nRxOffset = 0;
        return RT_TRUE;
    }

    return RT_FALSE;
}

static void prvParseNWTIME(void)
{
    rt_base_t year=0, mon=0, day=0, hour=0, min=0, sec=0, tz=0, dst=0;
    if (s_nRxOffset > c_xGPRS_EvtList[GPRS_EVT_NWTIME].btLen &&
        '\r' == s_btRxBuffer[s_nRxOffset - 2] && '\n' == s_btRxBuffer[s_nRxOffset - 1]) {
        s_btRxBuffer[s_nRxOffset] = '\0';
        if( sscanf((const char *)s_btRxBuffer, "^NWTIME: \"%d/%d/%d,%d:%d:%d+%d,%d\"\r\n", \
                &year, &mon, &day, &hour, &min, &sec, &tz, &dst) >= 8) 
        {
            rt_kprintf("GPRS NetTime:%04d/%02d/%02d,%02d:%02d:%02d+%d,%d", 
                2000+year, mon, day, hour, min, sec, tz, dst);
            set_date(2000+year, mon, day);
            set_time(hour, min, sec);
            _gprs_net_time_flag = RT_TRUE;
        } else if( sscanf((const char *)s_btRxBuffer, "^NWTIME: \"%d/%d/%d,%d:%d:%d-%d,%d\"\r\n", \
                &year, &mon, &day, &hour, &min, &sec, &tz, &dst) >= 8) 
        {
            rt_kprintf("GPRS NetTime:%04d/%02d/%02d,%02d:%02d:%02d-%d,%d", 
                2000+year, mon, day, hour, min, sec, tz, dst);
            set_date(2000+year, mon, day);
            set_time(hour, min, sec);
            _gprs_net_time_flag = RT_TRUE;
        }
        //rt_kprintf( "%s\n", g_xGPRS_COPN.alphan );
        s_bRcvEvt[GPRS_EVT_NWTIME] = RT_FALSE;
        s_bIsEvtParse[GPRS_EVT_NWTIME] = RT_TRUE;
        s_nRxOffset = 0;
        return;
    } else {
        prvParseTimeout(GPRS_EVT_NWTIME, 200);
    }
}


static void prvParseSendAck(void)
{
    if (s_nRxOffset > c_xGPRS_EvtList[GPRS_EVT_SEDN_ACK].btLen &&
        '\r' == s_btRxBuffer[s_nRxOffset - 2] && '\n' == s_btRxBuffer[s_nRxOffset - 1]) {
        s_btRxBuffer[s_nRxOffset] = '\0';

		memset(&g_xGPRS_SACK,0,sizeof(g_xGPRS_SACK));
		sscanf(s_btRxBuffer, "+QISACK: %d,%d,%d", &g_xGPRS_SACK.sent,&g_xGPRS_SACK.acked,&g_xGPRS_SACK.nAcked);
	
        s_bRcvEvt[GPRS_EVT_SEDN_ACK] = RT_FALSE;
        s_bIsEvtParse[GPRS_EVT_SEDN_ACK] = RT_TRUE;
        s_nRxOffset = 0;
        return;
    } else {
        prvParseTimeout(GPRS_EVT_SEDN_ACK, 200);
    }
}


static void prvParseCOPS(void)
{
    if (s_nRxOffset > c_xGPRS_EvtList[GPRS_EVT_COPS].btLen &&
        '\r' == s_btRxBuffer[s_nRxOffset - 2] && '\n' == s_btRxBuffer[s_nRxOffset - 1]) {
        s_btRxBuffer[s_nRxOffset] = '\0';

		
        g_xGPRS_COPS.nCount = 1;
        memset(g_xGPRS_COPS.xCOPS[g_xGPRS_COPS.nCount].salphan, 0, 16);
		int j = 0;
        int flag = 0;
		for(int i = 0 ; i < strlen(s_btRxBuffer) && j < 16;i++){
			if(s_btRxBuffer[i] != '"' && s_btRxBuffer[i-1] == '"'){
				//g_xGPRS_COPS.xCOPS[g_xGPRS_COPS.nCount].salphan[j] = s_btRxBuffer[i];
                flag = 1;
			}
           
			if(s_btRxBuffer[i] == '"' && s_btRxBuffer[i-1] != '"' && flag == 1){
				break;
			}
             if(flag == 1){
              g_xGPRS_COPS.xCOPS[0].alphan[j++] = s_btRxBuffer[i];
            }
		}
        s_bRcvEvt[GPRS_EVT_COPS] = RT_FALSE;
        s_bIsEvtParse[GPRS_EVT_COPS] = RT_TRUE;
        s_nRxOffset = 0;
        return;
    } else {
        prvParseTimeout(GPRS_EVT_COPS, 200);
    }
}



static void prvParseCSQ(void)
{
    rt_base_t tmp;

    if (s_nRxOffset > c_xGPRS_EvtList[GPRS_EVT_CSQ].btLen &&
        '\r' == s_btRxBuffer[s_nRxOffset - 2] && '\n' == s_btRxBuffer[s_nRxOffset - 1]) {
        s_btRxBuffer[s_nRxOffset] = '\0';
        sscanf((const char *)s_btRxBuffer, "+CSQ: %d,%d\r\n", &g_nGPRS_CSQ, &tmp);
        s_bRcvEvt[GPRS_EVT_CSQ] = RT_FALSE;
        s_bIsEvtParse[GPRS_EVT_CSQ] = RT_TRUE;
        s_nRxOffset = 0;
        return;
    } else {
        prvParseTimeout(GPRS_EVT_CSQ, 200);
    }
}

static void prvParseCREG(void)
{
    rt_base_t tmp;

    if (s_nRxOffset > c_xGPRS_EvtList[GPRS_EVT_CREG].btLen &&
        '\r' == s_btRxBuffer[s_nRxOffset - 2] && '\n' == s_btRxBuffer[s_nRxOffset - 1]) {
        s_btRxBuffer[s_nRxOffset] = '\0';
        sscanf((const char *)s_btRxBuffer, "+CGREG: %d,%d\r\n", &tmp, &g_nGPRS_CREG);
        s_bRcvEvt[GPRS_EVT_CREG] = RT_FALSE;
        s_bIsEvtParse[GPRS_EVT_CREG] =  RT_TRUE;
        s_nRxOffset = 0;
        return;
    } else {
        prvParseTimeout(GPRS_EVT_CREG, 200);
    }
}


static void prvParseMux(void)
{
    rt_base_t tmp;

    if (s_nRxOffset > c_xGPRS_EvtList[GPRS_EVT_MUX].btLen &&
        '\r' == s_btRxBuffer[s_nRxOffset - 2] && '\n' == s_btRxBuffer[s_nRxOffset - 1]) {
        s_btRxBuffer[s_nRxOffset] = '\0';
        sscanf((const char *)s_btRxBuffer, "+QIMUX: %d\r\n", &g_nGPRS_MUX);
        s_bRcvEvt[GPRS_EVT_MUX] = RT_FALSE;
        s_bIsEvtParse[GPRS_EVT_MUX] =  RT_TRUE;
        s_nRxOffset = 0;
        return;
    } else {
        prvParseTimeout(GPRS_EVT_MUX, 200);
    }
}

static void prvParseSendCmd(void)
{
    rt_base_t tmp;

    if (s_nRxOffset >= c_xGPRS_EvtList[GPRS_EVT_SEND_CMD].btLen) {
        s_bRcvEvt[GPRS_EVT_SEND_CMD] = RT_FALSE;
        s_bIsEvtParse[GPRS_EVT_SEND_CMD] = RT_TRUE;
        s_nRxOffset = 0;
        return;
    } else {
        prvParseTimeout(GPRS_EVT_SEND_CMD, 200);
    }
}

static void prvParseSendOk(void)
{
    rt_base_t tmp;

    if (s_nRxOffset > c_xGPRS_EvtList[GPRS_EVT_SEND_OK].btLen &&
        '\r' == s_btRxBuffer[s_nRxOffset - 2] && '\n' == s_btRxBuffer[s_nRxOffset - 1]) {
        s_bRcvEvt[GPRS_EVT_SEND_OK] = RT_FALSE;
        s_bIsEvtParse[GPRS_EVT_SEND_OK] = RT_TRUE;
        s_nRxOffset = 0;
        return;
    } else {
        prvParseTimeout(GPRS_EVT_SEND_OK, 200);
    }
}

/*static void prvParseSICI(void)
{
    if (s_nRxOffset > c_xGPRS_EvtList[GPRS_EVT_SICI].btLen &&
        '\r' == s_btRxBuffer[s_nRxOffset - 2] && '\n' == s_btRxBuffer[s_nRxOffset - 1]) {
        s_btRxBuffer[s_nRxOffset] = '\0';

        if ('0' == s_btRxBuffer[7]) {
            sscanf((const char *)s_btRxBuffer, "^SICI: %d,%d,%d,%s\r\n",
                   &g_xGPRS_NState.uId,
                   &g_xGPRS_NState.uState,
                   &g_xGPRS_NState.uSrvNums,
                   g_xGPRS_NState.szIP);
        }
        s_bRcvEvt[GPRS_EVT_SICI] = RT_FALSE;
        s_bIsEvtParse[GPRS_EVT_SICI] = RT_FALSE;
        s_nRxOffset = 0;
        return;
    } else {
        prvParseTimeout(GPRS_EVT_SICI, 500);
    }
}

*/

typedef struct _s_SocketStat
{
	int nId;
	char SockeType[16];
	char Ip[16];
	int port;
}s_SocketStat;


static rt_bool_t prvStatPrase(char *str , s_SocketStat *pStat)
{
	rt_bool_t status = RT_FALSE;
	
	pStat->nId = str[9] - '0';
	int flag = 0 , flag1 = 0;
	char *pIp = NULL;
	int start = 0 , end = 0;

	for(int i = 0 ; i < strlen(str);i++){
		if(str[i] == '"' && str[i+1]!='"' && flag != 1 && flag1 != 1){
			memcpy(pStat->SockeType,&str[i+1],3);
			flag = 1;
		}
		if(flag == 1 && str[i] == '"' && str[i+1]!='"' && i >= 17 && flag1 != 1){
			pIp = &str[i+1];
			start = i;
			flag1 = 1;
		}

		if(flag1 == 1 && str[i] == '"' && str[i+1] == ','){
			end = i;
		}
	}

	char tmp_port[10] = {0};
	
	if((end-start) > 0){
		memcpy(pStat->Ip,pIp,end-start-1);
		char *pstr = &str[end+3];
		memcpy(tmp_port,pstr,strlen(pstr));
		 pStat->port = atoi(tmp_port);
		 status = RT_TRUE;
	}
    
	return status;
}


static void prvParseSISO(void)
{
    if (s_nRxOffset > c_xGPRS_EvtList[GPRS_EVT_SISO].btLen &&
        '\r' == s_btRxBuffer[s_nRxOffset - 2] && '\n' == s_btRxBuffer[s_nRxOffset - 1] &&
        'O' == s_btRxBuffer[s_nRxOffset - 4] && 'K' == s_btRxBuffer[s_nRxOffset - 3] ) {
        s_btRxBuffer[s_nRxOffset] = '\0';

        //rt_kprintf("%s\n",s_btRxBuffer);

		for(int i  = 0; i < GPRS_SOCKET_MAX; i++){
			memset(&g_xGPRS_SState[i],0,sizeof(GPRS_SState_t));
		}

       
		char *str = &s_btRxBuffer[0];
		char delims[] = "\r\n";
		char *result = NULL;
		result = strtok(str,delims);
        
         static char tmp_char[64] = {0};
		
		while(result != NULL){
			rt_kprintf("%s\n",result);
			//sscanf(result, "+QISTAT: %d, \"%s\", \"%s\", %d", &nId,SockeType,ip,&Port);
			s_SocketStat Stat;
			memset(&Stat,0,sizeof(s_SocketStat));
           
            memset(tmp_char,0,sizeof(tmp_char));
            memcpy(tmp_char,result,strlen(result));
			rt_bool_t bRet =  prvStatPrase(tmp_char, &Stat);
            int nId = Stat.nId;
			if(bRet && (nId < GPRS_SOCKET_MAX)){
				  g_xGPRS_SState[nId].uId = Stat.nId;
				  memset(g_xGPRS_SState[nId].szType, 0, 16);
				  memcpy(g_xGPRS_SState[nId].szType,Stat.SockeType, strlen(Stat.SockeType));
				  g_xGPRS_SState[nId].uSocketState = 1;
				  g_xGPRS_SState[nId].uSrvState = 4;
				 
				  g_xGPRS_SState[nId].uRemPort = Stat.port;
				  memcpy( g_xGPRS_SState[nId].szRemIP,Stat.Ip,strlen(Stat.Ip));
				  
			}
			result = strtok(NULL,delims);
		}
       
		s_bRcvEvt[GPRS_EVT_SISO] = RT_FALSE;
        s_bIsEvtParse[GPRS_EVT_SISO] = RT_TRUE;
        s_nRxOffset = 0;
        return;
    } else {
        prvParseTimeout(GPRS_EVT_SISO, 200);
    }
}




static void prvParseConnect(void)
{
    //rt_base_t nId, nFlag, nLen;

    if (s_nRxOffset > c_xGPRS_EvtList[GPRS_EVT_CONNECT].btLen &&
        '\r' == s_btRxBuffer[s_nRxOffset - 2] && '\n' == s_btRxBuffer[s_nRxOffset - 1]) {
        s_btRxBuffer[s_nRxOffset] = '\0';
        if (s_nRxOffset >= 13) {
            rt_base_t nId = s_btRxBuffer[0] - '0';
           // char p = 
            if (nId < GPRS_SOCKET_MAX && ( strncmp(&s_btRxBuffer[3],"CONNECT OK",strlen("CONNECT OK")) == 0 )) {
                s_xSocketList[nId].eConn = GPRS_S_OK;
                g_xGPRS_SState[nId].uSrvState = GPRS_SOCKET_STATE_WAIT;
                // 目前进行简单发送, 不进行结果校验
                //if( GPRS_S_EXE == s_xSocketList[nId].eWrite ) {
                //if( sscanf( (const char *)s_btRxBuffer, "^SISW: %d,%d,%d\r\n", &nId, &nFlag, &nLen ) >= 0 ) {
                //    s_xSocketList[nId].eWrite = GPRS_S_OK;
                //}
                //}
            }
        }
        s_bRcvEvt[GPRS_EVT_CONNECT] = RT_FALSE;
        s_bIsEvtParse[GPRS_EVT_CONNECT] = RT_TRUE;
        s_nRxOffset = 0;
        return;
    } else {
        prvParseTimeout(GPRS_EVT_CONNECT, 200);
    }
}

static void prvParseClose(void)
{
    //rt_base_t nId, nFlag, nLen;

    if (s_nRxOffset > c_xGPRS_EvtList[GPRS_EVT_SOCKET_CLOSE].btLen &&
        '\r' == s_btRxBuffer[s_nRxOffset - 2] && '\n' == s_btRxBuffer[s_nRxOffset - 1]) {
        s_btRxBuffer[s_nRxOffset] = '\0';
        if (s_nRxOffset >= 11) {
            rt_base_t nId = s_btRxBuffer[0] - '0';
           // char p = 
            if (nId < GPRS_SOCKET_MAX && ( strncmp(&s_btRxBuffer[3],"CLOSE OK",strlen("CLOSE OK")) == 0 )) {
                s_xSocketList[nId].eConn = GPRS_S_WAIT;
                g_xGPRS_SState[nId].uSrvState = GPRS_SOCKET_STATE_WAIT;
                // 目前进行简单发送, 不进行结果校验
                //if( GPRS_S_EXE == s_xSocketList[nId].eWrite ) {
                //if( sscanf( (const char *)s_btRxBuffer, "^SISW: %d,%d,%d\r\n", &nId, &nFlag, &nLen ) >= 0 ) {
                //    s_xSocketList[nId].eWrite = GPRS_S_OK;
                //}
                //}
            }
        }
        s_bRcvEvt[GPRS_EVT_SOCKET_CLOSE] = RT_FALSE;
        s_bIsEvtParse[GPRS_EVT_SOCKET_CLOSE] = RT_TRUE;
        s_nRxOffset = 0;
        return;
    } else {
        prvParseTimeout(GPRS_EVT_SOCKET_CLOSE, 200);
    }
}

static void prvParseSISR(void)
{

    rt_base_t nTmp;
    char cmd[64];
    static rt_base_t nId = 0;
    static rt_base_t nRemain = 0, nIndex = 0;

    // 匹配 (,)
     if (s_nRxOffset >= c_xGPRS_EvtList[GPRS_EVT_SISR].btLen  &&
        '\r' == s_btRxBuffer[s_nRxOffset - 2] &&
        '\n' == s_btRxBuffer[s_nRxOffset - 1]) {

        char szNum[11] = { 0 };

        nId = s_btRxBuffer[10] - '0';
        if (nId < GPRS_SOCKET_MAX) {
			int j = 0;
            for (int i = (s_nRxOffset - 3); i > 0; i--) {
				if(s_btRxBuffer[i] == ' ' || j>=10) break;
                szNum[j++] = s_btRxBuffer[i];
            }

			char szLen[11] = {0};
			for(int i = 0; i < j && i < 11; i++){
				szLen[i] = szNum[j-i-1];
			}

            nRemain = atol((char const *)szLen);
            nIndex = 0;
            if (nRemain <= 0) {
                s_bRcvEvt[GPRS_EVT_SISR] = RT_FALSE;
                s_bIsEvtParse[GPRS_EVT_SISR] = RT_TRUE;
            }
        }
        s_nRxOffset = 0;
        return;
    } else if (nRemain > 0) {
        if (nId < GPRS_SOCKET_MAX) {
            if(nIndex < s_xSocketList[nId].uBufsize) {
                s_xSocketList[nId].pInBuffer[nIndex++] = s_btRxBuffer[s_nRxOffset - 1];
            }
            if (--nRemain <= 0) {
                s_xSocketList[nId].eRead = GPRS_S_WAIT;
                if (s_xSocketList[nId].onRecv) {
                    s_xSocketList[nId].onRecv(nId, s_xSocketList[nId].pInBuffer, nIndex);
                }
                s_bRcvEvt[GPRS_EVT_SISR] = RT_FALSE;
                s_bIsEvtParse[GPRS_EVT_SISR] = RT_TRUE;
            }
        }
    }

    if (prvParseTimeout(GPRS_EVT_SISR, 1000)) {
        if (nId < GPRS_SOCKET_MAX) {
            s_xSocketList[nId].eRead = GPRS_S_WAIT;
        }
    }
	
}

static void prvParseCode(void)
{
    rt_base_t nIndex = s_nRxOffset - 2;

    for (int i = 0; i < GPRS_CODE_MAX; i++) {
        rt_uint8_t btLen = c_xGPRSCodeList[i].btLen;

        if (nIndex >= btLen &&
            0 == strncmp(
                         (char const *)&s_btRxBuffer[nIndex - btLen],
                         c_xGPRSCodeList[i].szCode,
                         btLen)
            ) {
            s_bRcvCode[i] = RT_TRUE;
            break;
        }
    }
}

static void prvClearRcvBuffer(void)
{
    s_nRxOffset = 0;
}

static void prvClearRcvCode(void)
{
    for (int i = 0; i < GPRS_CODE_MAX; i++) {
        s_bRcvCode[i] = RT_FALSE;
    }
}

static void prvClearRcvEvt(void)
{
    for (int i = 0; i < GPRS_EVT_MAX; i++) {
        s_bRcvEvt[i] = RT_FALSE;
        s_bIsEvtParse[i] = RT_FALSE;
        s_ulRcvEvtTick[i] = rt_tick_get();
    }
}

void prvSendBuffer(const void *pBuffer, rt_size_t uSize)
{
    prvClearRcvCode();
    serial->parent.write(&(serial->parent), 0, pBuffer, uSize);
}

void prvSendCmd(const char *szCmd)
{
    prvClearRcvCode();
    serial->parent.write(&(serial->parent), 0, szCmd, strlen(szCmd));
}

void vInitGPRSStatus(void)
{
    prvClearRcvCode();
    prvClearRcvEvt();
    prvClearRcvBuffer();
}

static void prvReleaseSocket(rt_uint8_t btId)
{
    if (btId < GPRS_SOCKET_MAX) {
        RT_KERNEL_FREE(s_xSocketList[btId].pInBuffer);
        s_xSocketList[btId].pInBuffer = RT_NULL;
        s_xSocketList[btId].uBufsize = 0;
        s_xSocketList[btId].bUsed = RT_FALSE;
        s_xSocketList[btId].onAccept = RT_NULL;
        s_xSocketList[btId].onRecv = RT_NULL;

        s_xSocketList[btId].eConn = GPRS_S_WAIT;
        s_xSocketList[btId].eRead = GPRS_S_WAIT;
        s_xSocketList[btId].eWrite = GPRS_S_WAIT;
    }
}

static void prvReleaseAllSocket(void)
{
    for (int i = 0; i < GPRS_SOCKET_MAX; i++) {
        prvReleaseSocket(i);
    }
}

rt_err_t xGPRS_Init(rt_uint8_t instance)
{
    rt_pin_mode(BOARD_GPIO_GPRS_POWER_EN, PIN_MODE_OUTPUT);
    rt_pin_mode(BOARD_GPIO_GPRS_TERM_ON, PIN_MODE_OUTPUT);
    rt_pin_mode(BOARD_GPIO_GPRS_RST, PIN_MODE_OUTPUT);

    rt_pin_write(BOARD_GPIO_GPRS_POWER_EN, PIN_LOW);
    rt_pin_write(BOARD_GPIO_GPRS_TERM_ON, PIN_LOW);
    rt_pin_write(BOARD_GPIO_GPRS_RST, PIN_LOW);

    if (rt_mutex_init(&gprs_mutex, "gprs_mutex", RT_IPC_FLAG_FIFO) != RT_EOK) {
        rt_kprintf("init gprs_mutex failed\n");

        return RT_ERROR;
    }

    if (rt_event_init(&s_xIdeEvent, "gprs_evt", RT_IPC_FLAG_FIFO) != RT_EOK) {
        rt_kprintf("init gprs_ide_evt failed\n");

        return RT_ERROR;
    }

    prvReleaseAllSocket();

    serial = (rt_serial_t *)rt_device_find(BOARD_UART_DEV_NAME(instance));

    if (serial) {
        /* set serial configure */

        serial->config.baud_rate = g_uart_cfgs[instance].port_cfg.baud_rate;
        serial->config.data_bits = g_uart_cfgs[instance].port_cfg.data_bits;
        serial->config.stop_bits = g_uart_cfgs[instance].port_cfg.stop_bits;
        serial->config.parity = g_uart_cfgs[instance].port_cfg.parity;
        serial->config.bufsz = RX_BUFFER_SIZE;
        serial->ops->configure(serial, &(serial->config));

        /* open serial device */
        if (RT_EOK == serial->parent.open(&serial->parent, RT_DEVICE_OFLAG_RDWR | RT_DEVICE_FLAG_INT_RX)) {
            serial->parent.ide_indicate = prvOnIDE;

            rt_thread_t gprs_thread = rt_thread_create("gprsparse", rt_gprsparse_thread, RT_NULL, 0x400, 20, 20);

            if (gprs_thread != RT_NULL) {
                rt_thddog_register(gprs_thread, 60);
                rt_thread_startup(gprs_thread);
            }

            return RT_EOK;
        }
    }

    return RT_ERROR;
}

void vGPRS_HWRest(void)
{
    rt_mutex_take(&gprs_mutex, RT_WAITING_FOREVER);

    rt_pin_write(BOARD_GPIO_GPRS_RST, PIN_HIGH);
    rt_thread_delay(RT_TICK_PER_SECOND / 100);
    rt_pin_write(BOARD_GPIO_GPRS_RST, PIN_LOW);

    memset(&s_xSocketList[0], 0, sizeof(s_xSocketList));
    prvReleaseAllSocket();

    rt_mutex_release(&gprs_mutex);
}

rt_bool_t bGPRS_ATE(rt_bool_t bECHO)
{
    rt_bool_t bRet = RT_FALSE;
    rt_mutex_take(&gprs_mutex, RT_WAITING_FOREVER);

    GPRS_AT_DELAY();
    prvSendCmd("ATE");
    if (bECHO) {
        prvSendCmd("1\r\n");
    } else {
        prvSendCmd("0\r\n");
    }

    bRet = (prvWaitResult(200) && s_bRcvCode[GPRS_CODE_OK]);

    rt_mutex_release(&gprs_mutex);
    return bRet;
}

rt_bool_t bGPRS_GetNetTime(rt_bool_t bWait)
{
    static rt_bool_t bInTask = RT_FALSE;
    static rt_tick_t ulStartTick = 0;
    rt_uint32_t ulWaitTick = 10 * 60 * RT_TICK_PER_SECOND;
    rt_bool_t bRet = RT_FALSE;

    if(!_gprs_net_time_flag) {
        ulWaitTick = 60 * RT_TICK_PER_SECOND;
    }

    if (!bInTask && rt_tick_get() - ulStartTick > ulWaitTick) {
        rt_mutex_take(&gprs_mutex, RT_WAITING_FOREVER);

        GPRS_AT_DELAY();
        prvClearEvt(GPRS_EVT_NWTIME);
        prvSendCmd("AT+CCLK?\r\n");
        if (bWait && prvWaitResult(1000) && s_bRcvCode[GPRS_CODE_OK]) {
            bRet = RT_TRUE;
        }

        rt_mutex_release(&gprs_mutex);
        ulStartTick = rt_tick_get();
        bInTask = RT_FALSE;
    } else {
        bRet = RT_TRUE;
    }
    return bRet;
}

rt_bool_t bGPRS_GetCOPN(rt_bool_t bWait)
{
    rt_bool_t bRet = RT_FALSE;
    return bRet;
}

rt_bool_t bGPRS_GetCOPS(rt_bool_t bWait)
{
    rt_bool_t bRet = RT_FALSE;
    rt_mutex_take(&gprs_mutex, RT_WAITING_FOREVER);

    GPRS_AT_DELAY();
    prvClearEvt(GPRS_EVT_COPS);
    prvSendCmd("AT+COPS?\r\n");
    if (bWait && prvWaitEvt(GPRS_EVT_COPS, RT_FALSE,500)) {
        bRet = RT_TRUE;
    }

    rt_mutex_release(&gprs_mutex);
    return bRet;
}

rt_bool_t bGPRS_GetSMOND(rt_bool_t bWait)
{
    rt_bool_t bRet = RT_FALSE;
    rt_mutex_take(&gprs_mutex, RT_WAITING_FOREVER);

    

    rt_mutex_release(&gprs_mutex);
    return bRet;
}

rt_bool_t bGPRS_GetCSQ(rt_bool_t bWait)
{
    rt_bool_t bRet = RT_FALSE;
    rt_mutex_take(&gprs_mutex, RT_WAITING_FOREVER);

    GPRS_AT_DELAY();
    prvClearEvt(GPRS_EVT_CSQ);
    prvSendCmd("AT+CSQ\r\n");
    if (bWait && prvWaitResult(500) && s_bRcvCode[GPRS_CODE_OK] && g_nGPRS_CSQ != UINT32_MAX) {
        bRet = RT_TRUE;
    }

    rt_mutex_release(&gprs_mutex);
    return bRet;
}

rt_bool_t bGPRS_GetCREG(rt_bool_t bWait)
{
    static rt_bool_t bInTask = RT_FALSE;
    static rt_tick_t ulStartTick = 0;
    static rt_uint32_t ulWaitTick = (RT_TICK_PER_SECOND * 5000 + 999) / 1000;
    rt_bool_t bRet = RT_FALSE;

    if (!bInTask && rt_tick_get() - ulStartTick > ulWaitTick) {
        rt_mutex_take(&gprs_mutex, RT_WAITING_FOREVER);

        GPRS_AT_DELAY();
        prvClearEvt(GPRS_EVT_CREG);
        prvSendCmd("AT+CGREG?\r\n");
        if (bWait && prvWaitResult(500) && s_bRcvCode[GPRS_CODE_OK] && g_nGPRS_CREG != UINT32_MAX) {
            bRet = RT_TRUE;
        }

        rt_mutex_release(&gprs_mutex);
        ulStartTick = rt_tick_get();
        bInTask = RT_FALSE;
    } else {
        bRet = RT_TRUE;
    }
    return bRet;
}


rt_bool_t bGPRS_GetMux(rt_bool_t bWait)
{
    
    rt_bool_t bRet = RT_FALSE;

    rt_mutex_take(&gprs_mutex, RT_WAITING_FOREVER);

    GPRS_AT_DELAY();
    prvClearEvt(GPRS_EVT_MUX);
    prvSendCmd("AT+QIMUX?\r\n");
    if (bWait && prvWaitEvt(GPRS_EVT_MUX, RT_FALSE,500) && g_nGPRS_MUX != -1) {
        bRet = RT_TRUE;
    }

    rt_mutex_release(&gprs_mutex);


    return bRet;
}

rt_bool_t bGPRS_SetCSCA(const char *szMsgNo)
{
    rt_bool_t bRet = RT_TRUE;
    rt_bool_t check = RT_TRUE;

    if (szMsgNo && szMsgNo[0]) {
        int len = strlen(szMsgNo);
        if (szMsgNo[0] != '+' && !isdigit(szMsgNo[0])) {
            check = RT_FALSE;
        }
        for (int i = 1; i < len; i++) {
            if (!isdigit(szMsgNo[i])) {
                check = RT_FALSE;
                break;
            }
        }
    } else {
        check = RT_FALSE;
    }

    if (check) {
        rt_mutex_take(&gprs_mutex, RT_WAITING_FOREVER);

        char cmd[64];
        snprintf(cmd, sizeof(cmd), "AT+CSCA=\"%s\"\r\n", szMsgNo);
        prvSendCmd(cmd);
        if (!prvWaitResult(1000) || !s_bRcvCode[GPRS_CODE_OK]) {
            bRet = RT_FALSE;
        }

        rt_mutex_release(&gprs_mutex);
    }

    return bRet;
}

// szAPN: default cmnet
rt_bool_t bGPRS_NetInit(const char *szAPN, const char *szAPNNo, const char *szUser, const char *szPsk)
{
    rt_bool_t bRet = RT_FALSE;
    rt_mutex_take(&gprs_mutex, RT_WAITING_FOREVER);

    char cmd[64];

    //vGPRS_HWRest();
    bGPRS_ATE(0);

	if(bGPRS_GetMux(RT_TRUE)){
		rt_kprintf("g_nGPRS_MUX = %d\r\n", g_nGPRS_MUX);
	}

	if(g_nGPRS_MUX != 1){
	    GPRS_AT_DELAY();
	    prvSendCmd("AT+QIMUX=1\r\n");	 //设置启用多路复用
	    if (!prvWaitResult(1000) || !s_bRcvCode[GPRS_CODE_OK]) {
	        bRet = RT_FALSE; goto __END;
	    }
	}

    /*
    默认是非透传模式，无需设置
	GPRS_AT_DELAY();
    prvSendCmd("AT+QIMODE=0\r\n");	 //设置启用非透传模式
    if (!prvWaitResult(1000) || !s_bRcvCode[GPRS_CODE_OK]) {
        bRet = RT_FALSE; goto __END;
    }*/

    GPRS_AT_DELAY();
    snprintf(cmd, sizeof(cmd), "AT+QICSGP=1,\"%s\"\r\n", (szAPN && szAPN[0]) ? szAPN : "CMNET"); //设置GPRS的APN
    prvSendCmd(cmd);
    if (!prvWaitResult(1000) || !s_bRcvCode[GPRS_CODE_OK]) {
        bRet = RT_FALSE; goto __END;
    }

	//bGPRS_GetCOPS();

	//bGPRS_SocketStateTest(0);

/*
    GPRS_AT_DELAY();
    prvSendCmd("AT^IOMODE=0,1\r\n");
    if (!prvWaitResult(1000) || !s_bRcvCode[GPRS_CODE_OK]) {
        bRet = RT_FALSE; goto __END;
    }
    */

    bRet = RT_TRUE;

__END:
    prvReleaseAllSocket();
    rt_mutex_release(&gprs_mutex);
    return bRet;
}

static rt_int8_t prvGetFreeSocket()
{
    for (int i = 0; i < GPRS_SOCKET_MAX; i++) {
        if (!s_xSocketList[i].bUsed) {
            return i;
        }
    }
    return -1;
}

static rt_bool_t prvWaitConnect(rt_uint8_t btId, eGPRS_Status eDest,  rt_time_t uMs)
{
    rt_bool_t bRet = RT_FALSE;
    rt_uint32_t ulStartTick = rt_tick_get();
    rt_uint32_t ulWaitTick = rt_tick_from_millisecond(uMs);

    while (rt_tick_get() - ulStartTick < ulWaitTick) {
        if (eDest == s_xSocketList[btId].eConn) {
            bRet = RT_TRUE;
            break;
        }
        rt_thread_delay(RT_TICK_PER_SECOND / 100);
    }

    return bRet;
}

static rt_bool_t prvWaitRead(rt_uint8_t btId, eGPRS_Status eDest, rt_time_t uMs)
{
    rt_bool_t bRet = RT_FALSE;
    rt_uint32_t ulStartTick = rt_tick_get();
    rt_uint32_t ulWaitTick = rt_tick_from_millisecond(uMs);

    while (rt_tick_get() - ulStartTick < ulWaitTick) {
        if (eDest == s_xSocketList[btId].eRead) {
            bRet = RT_TRUE;
            break;
        }
        rt_thread_delay(RT_TICK_PER_SECOND / 100);
    }

    return bRet;
}

static rt_bool_t prvWaitWrite(rt_uint8_t btId, eGPRS_Status eDest, rt_time_t uMs)
{
    rt_bool_t bRet = RT_FALSE;
    rt_uint32_t ulStartTick = rt_tick_get();
    rt_uint32_t ulWaitTick = rt_tick_from_millisecond(uMs);

    while (rt_tick_get() - ulStartTick < ulWaitTick) {
        if (eDest == s_xSocketList[btId].eWrite) {
            bRet = RT_TRUE;
            break;
        }
        rt_thread_delay(RT_TICK_PER_SECOND / 100);
    }

    return bRet;
}

static rt_bool_t bIsIpaddr(const char *szPeer)
{
	for(int i = 0 ; i < strlen(szPeer); i++){
		if(islower(szPeer[i])){
			return RT_FALSE;
		}
	}
	return RT_TRUE;
}

static rt_int8_t __nGPRS_SocketCreate(
                                      rt_uint8_t btType,
                                      const char *szPeer,
                                      const rt_uint16_t usPort,
                                      rt_size_t uBufSize,
                                      void (*onRecv)(rt_uint8_t btId, void *pBuffer, rt_size_t uSize),
                                      rt_time_t uTimeout
                                      )
{
	rt_kprintf("__nGPRS_SocketCreate \r\n");
    rt_int8_t nSock = -1;
    char cmd[64];

    rt_mutex_take(&gprs_mutex, RT_WAITING_FOREVER);

    nSock = prvGetFreeSocket();
	rt_kprintf("========prvGetFreeSocket : nSocke = %d\r\n",nSock);

    if (nSock >= 0) {
        if (btType != SOCK_STREAM && btType != SOCK_DGRAM) {
            nSock = -1; goto __END;
        }

        bGPRS_SocketClose(nSock, 5000);

		int flag = 0;
		if(bIsIpaddr(szPeer)){
			flag = 0;
		}else {
			flag = 1;
		}

        GPRS_AT_DELAY();
        snprintf(cmd, sizeof(cmd), "AT+QIDNSIP=%d\r\n", flag);  //设置是TCP是域名连接还是IP连接
        prvSendCmd(cmd);
        if (!prvWaitResult(1000) || !s_bRcvCode[GPRS_CODE_OK]) {
            nSock = -1; goto __END;
        }


        GPRS_AT_DELAY();
        snprintf(cmd, sizeof(cmd), "AT+QIOPEN=%d,\"%s\",\"%s\",\"%d\" \r\n",
                 nSock,
                 (SOCK_STREAM == btType ? "tcp" : "udp"),
                 szPeer, usPort);
        prvSendCmd(cmd);
        if (!prvWaitResult(1000) || !s_bRcvCode[GPRS_CODE_OK]) {
            nSock = -1; goto __END;
        }

     
	    rt_tick_t ulStart = rt_tick_get();
        uTimeout -= rt_millisecond_from_tick(rt_tick_get() - ulStart);
        s_xSocketList[nSock].eConn = GPRS_S_EXE;
		
        if (prvWaitConnect(nSock, GPRS_S_OK, uTimeout)) {
            // 释放
            prvReleaseSocket(nSock);

		   rt_kprintf("========bGPRS_Creat_Socket success : nSocke = %d\r\n", nSock);


            // 申请
            s_xSocketList[nSock].pInBuffer = RT_KERNEL_CALLOC(uBufSize);
            if (s_xSocketList[nSock].pInBuffer) {
                s_xSocketList[nSock].bUsed = RT_TRUE;
                s_xSocketList[nSock].btType = btType;
                s_xSocketList[nSock].bClient = RT_TRUE;
                s_xSocketList[nSock].uBufsize = uBufSize;
                s_xSocketList[nSock].onRecv = onRecv;

                s_xSocketList[nSock].eConn = GPRS_S_OK;
                s_xSocketList[nSock].eRead = GPRS_S_WAIT;
                s_xSocketList[nSock].eWrite = GPRS_S_WAIT;
            }
        } else {
            nSock = -1; goto __END;
        }
    }

__END:
	if(nSock < 0){
		rt_kprintf("create socket failed\r\n" );
	}
    rt_mutex_release(&gprs_mutex);
    return nSock;
}

rt_int8_t nGPRS_SocketConnect(
                              rt_uint8_t btType,
                              const char *szPeer,
                              const rt_uint16_t usPort,
                              rt_size_t uBufSize,
                              void (*onRecv)(rt_uint8_t btId, void *pBuffer, rt_size_t uSize),
                              rt_time_t uTimeout
                              )
{
    return __nGPRS_SocketCreate(btType, szPeer, usPort, uBufSize, onRecv, uTimeout);
}

rt_int8_t nGPRS_SocketListen(
                             rt_uint8_t btType,
                             const rt_uint16_t usPort,
                             rt_size_t uBufSize,
                             void (*onRecv)(rt_uint8_t btId, void *pBuffer, rt_size_t uSize),
                             rt_time_t uTimeout
                             )
{
    return __nGPRS_SocketCreate(btType, "listener", usPort, uBufSize, onRecv, uTimeout);
}

rt_bool_t bGPRS_SocketClose(rt_uint8_t btSock, rt_time_t uTimeout)
{
	 rt_kprintf("========bGPRS_SocketClose : nSocke = %d\r\n", btSock);

    rt_bool_t bRet = RT_FALSE;
    char cmd[16];
    rt_mutex_take(&gprs_mutex, RT_WAITING_FOREVER);

    GPRS_AT_DELAY();
     prvClearEvt(GPRS_EVT_SOCKET_CLOSE);
    snprintf(cmd, 16, "AT+QICLOSE=%u\r\n", btSock);
    prvSendCmd(cmd);
    bRet = prvWaitEvt(GPRS_EVT_SOCKET_CLOSE, RT_TRUE, 1000);

    prvReleaseSocket(btSock);

    rt_mutex_release(&gprs_mutex);
    return bRet;
}


// if close return 0, error return -1
rt_base_t nGPRS_SocketSend(
                           rt_uint8_t btSock,
                           const void *pBuffer,
                           rt_size_t uSize,
                           rt_time_t uTimeout
                           )
{
    rt_base_t nRet = 0;

    rt_mutex_take(&gprs_mutex, RT_WAITING_FOREVER);

    if (GPRS_S_OK == s_xSocketList[btSock].eConn) {
        char cmd[24];
        GPRS_AT_DELAY();
        snprintf(cmd, sizeof(cmd), "AT+QISEND=%u,%u\r\n", btSock, uSize);
        prvClearEvt(GPRS_EVT_SEND_CMD);
        prvSendCmd(cmd);
		if(!prvWaitEvt(GPRS_EVT_SEND_CMD, RT_FALSE, 500)){
			nRet = -1;goto __END;
		}

        GPRS_AT_DELAY();
		prvClearEvt(GPRS_EVT_SEND_OK);
        prvSendBuffer(pBuffer, uSize);
		if(!prvWaitEvt(GPRS_EVT_SEND_OK, RT_FALSE, 1000)){ 
			 nRet = -1; goto __END;
		}
		
		
		/*memset(&g_xGPRS_SACK,0,sizeof(g_xGPRS_SACK));
		
        GPRS_AT_DELAY();
	    snprintf(cmd, sizeof(cmd), "AT+QISACK=%d\r\n", btSock);
		prvClearEvt(GPRS_EVT_SEDN_ACK);
		prvSendCmd(cmd);
		if(!prvWaitEvt(GPRS_EVT_SEDN_ACK, RT_FALSE, 500)){ 
			 nRet = -1; goto __END;
		}

		if((g_xGPRS_SACK.sent != uSize) || (g_xGPRS_SACK.acked != uSize) || (g_xGPRS_SACK.nAcked != 0)){
			nRet = -1; goto __END;
		}
		*/

		

        nRet = uSize;
    }

__END:
    rt_mutex_release(&gprs_mutex);
    return nRet;
}

const char* szGPRS_FomartState(rt_uint8_t btState)
{
    switch (btState) {
    case 0:
        return "Down";
    case 1:
        return "Conn";
    case 2:
        return "Up";
    case 3:
        return "Restrict";
    case 4:
        return "Close";
    }

    return "UNKNOWN";
}

/*rt_bool_t bGPRS_NetState(void)
{
    rt_bool_t bRet = RT_FALSE;
    rt_mutex_take(&gprs_mutex, RT_WAITING_FOREVER);

    GPRS_AT_DELAY();
    prvClearEvt(GPRS_EVT_SICI);
    prvSendCmd("AT^SICI=0\r\n");
    if (prvWaitResult(500) && s_bRcvCode[GPRS_CODE_OK]) {
        bRet = RT_TRUE;
    }

    rt_mutex_release(&gprs_mutex);
    return bRet;
}
*/

static int _state = 1;

rt_bool_t bGPRS_SocketState(rt_uint8_t btSock)
{
    static rt_bool_t bInTask = RT_FALSE;
    static rt_tick_t ulStartTick = 0;
    static rt_uint32_t ulWaitTick = (RT_TICK_PER_SECOND * 5000 + 999) / 1000;
    rt_bool_t bRet = RT_FALSE;

    if (!bInTask && rt_tick_get() - ulStartTick > ulWaitTick) {
        char cmd[16];

        bInTask = RT_TRUE;
        rt_mutex_take(&gprs_mutex, RT_WAITING_FOREVER);

        GPRS_AT_DELAY();
		if(_state){
        	snprintf(cmd, sizeof(cmd), "AT+QISTAT\r\n");
		}else {
        	snprintf(cmd, sizeof(cmd), "AT+QISTATE\r\n");
		}
        prvClearEvt(GPRS_EVT_SISO);
        prvSendCmd(cmd);
       // if (prvWaitResult(5000) && s_bRcvCode[GPRS_CODE_OK]) {
		if(prvWaitEvt(GPRS_EVT_SISO, RT_FALSE, 1000)){ 
            bRet = RT_TRUE;
        }


		//获取IP地址

		/*GPRS_AT_DELAY();
        snprintf(cmd, sizeof(cmd), "AT+QILOCIP\r\n");
        prvClearEvt(GPRS_EVT_IP);
        prvSendCmd(cmd);
		if(prvWaitEvt(GPRS_EVT_IP, RT_FALSE, 1000)){ 
            bRet = RT_TRUE;
        }*/

        rt_mutex_release(&gprs_mutex);

        ulStartTick = rt_tick_get();
        bInTask = RT_FALSE;
    } else {
        bRet = RT_TRUE;
    }
    return bRet;
}



rt_bool_t bGPRS_SocketStateTest(rt_uint8_t btSock)
{
    static rt_bool_t bInTask = RT_FALSE;
    static rt_tick_t ulStartTick = 0;
    static rt_uint32_t ulWaitTick = (RT_TICK_PER_SECOND * 5000 + 999) / 1000;
    rt_bool_t bRet = RT_FALSE;


    char cmd[16];

    bInTask = RT_TRUE;
    rt_mutex_take(&gprs_mutex, RT_WAITING_FOREVER);

    GPRS_AT_DELAY();
    snprintf(cmd, sizeof(cmd), "AT+QISTATE\r\n");
    prvClearEvt(GPRS_EVT_SISO);
    prvSendCmd(cmd);
    if (prvWaitResult(5000) && s_bRcvCode[GPRS_CODE_OK]) {
        bRet = RT_TRUE;
    }

    rt_mutex_release(&gprs_mutex);

    ulStartTick = rt_tick_get();
    bInTask = RT_FALSE;
 
    return bRet;
}

/*
rt_bool_t bGPRS_SocketState( rt_uint8_t btSock )
{
    rt_bool_t bRet = RT_FALSE;
    char cmd[16];
    rt_mutex_take( &gprs_mutex, RT_WAITING_FOREVER );

    GPRS_AT_DELAY();
    snprintf( cmd, sizeof(cmd), "AT^SISI=%u\r\n", btSock );
    prvClearEvt( GPRS_EVT_SISI );
    prvSendCmd( cmd );
    if (prvWaitResult( 500 ) && s_bRcvCode[GPRS_CODE_OK] ) {
         bRet = RT_TRUE;
    }

    rt_mutex_release( &gprs_mutex );
    return bRet;
}
*/
rt_bool_t bGPRS_ATTest(void)
{
    rt_bool_t bRet = RT_FALSE;
    rt_mutex_take(&gprs_mutex, RT_WAITING_FOREVER);

    GPRS_AT_DELAY();
    prvSendCmd("AT\r\n");
    bRet = prvWaitResult(500);

    rt_mutex_release(&gprs_mutex);
    return bRet;
}


rt_bool_t bGPRS_ATReset(void)
{
    rt_bool_t bRet = RT_FALSE;
    rt_mutex_take(&gprs_mutex, RT_WAITING_FOREVER);

    GPRS_AT_DELAY();
    prvSendCmd("AT+CFUN=1,1\r\n");
    bRet = prvWaitResult(500);

    rt_mutex_release(&gprs_mutex);
    return bRet;
}



//TCP/IP 数据包未确认时关闭链接需要等待的时长，单位为秒 60s
rt_bool_t bGPRS_TcpOt(void)
{
    rt_bool_t bRet = RT_FALSE;
    rt_mutex_take(&gprs_mutex, RT_WAITING_FOREVER);

    GPRS_AT_DELAY();
    prvSendCmd("AT^SCFG=TCP/OT,60\r\n");
    bRet = prvWaitResult(500);

    rt_mutex_release(&gprs_mutex);
    return bRet;
}


void vGPRS_PowerDown(void)
{
    rt_bool_t bRet = RT_FALSE;
    rt_mutex_take(&gprs_mutex, RT_WAITING_FOREVER);

    rt_pin_write(BOARD_GPIO_GPRS_POWER_EN, PIN_HIGH);

    rt_mutex_release(&gprs_mutex);
}

void vGPRS_PowerUp(void)
{
    rt_bool_t bRet = RT_FALSE;
    rt_mutex_take(&gprs_mutex, RT_WAITING_FOREVER);

    rt_pin_write(BOARD_GPIO_GPRS_POWER_EN, PIN_LOW);

    rt_mutex_release(&gprs_mutex);

    // vGPRS_HWRest();
}


void vGPRS_TermDown(void)
{
    rt_bool_t bRet = RT_FALSE;
    rt_mutex_take(&gprs_mutex, RT_WAITING_FOREVER);

    rt_thread_delay(RT_TICK_PER_SECOND);
    while (bGPRS_ATTest()) {
        rt_pin_write(BOARD_GPIO_GPRS_TERM_ON,  PIN_LOW);
        rt_thread_delay(1.2 * RT_TICK_PER_SECOND);
        rt_pin_write(BOARD_GPIO_GPRS_TERM_ON, PIN_HIGH);
        rt_thread_delay(RT_TICK_PER_SECOND / 2);
    }

    rt_mutex_release(&gprs_mutex);
}



void vGPRS_TermUp(void)
{
    rt_bool_t bRet = RT_FALSE;
    rt_mutex_take(&gprs_mutex, RT_WAITING_FOREVER);

    int cnt = 0;
    while (!bGPRS_ATTest()) {
        rt_pin_write(BOARD_GPIO_GPRS_TERM_ON, PIN_HIGH);
        rt_thread_delay(1.5 * RT_TICK_PER_SECOND);
        rt_pin_write(BOARD_GPIO_GPRS_TERM_ON,  PIN_LOW);
        rt_thread_delay(2 * RT_TICK_PER_SECOND);
        if (cnt++ >= 2) {
            break;
        }
    }

    rt_mutex_release(&gprs_mutex);

    // vGPRS_HWRest();
}

static rt_bool_t prvWaitEvt(eGPRS_Evt eEvt, rt_bool_t bCheckErr, rt_base_t nMs)
{
    rt_bool_t bRet = RT_FALSE;
    rt_uint32_t ulStartTick = rt_tick_get();
    rt_uint32_t ulWaitTick = rt_tick_from_millisecond(nMs);

    while (rt_tick_get() - ulStartTick < ulWaitTick) {
        if (s_bIsEvtParse[eEvt] || (bCheckErr && s_bRcvCode[GPRS_CODE_ERROR])) {
            bRet = RT_TRUE;
            break;
        }
        rt_thread_delay(RT_TICK_PER_SECOND / 100);
    }

    return bRet;
}

static rt_bool_t prvWaitResult(rt_base_t nMs)
{
    rt_bool_t bRet = RT_FALSE;
    rt_uint32_t ulStartTick = rt_tick_get();
    rt_uint32_t ulWaitTick = rt_tick_from_millisecond(nMs);

    while (rt_tick_get() - ulStartTick < ulWaitTick) {
        if (s_bRcvCode[GPRS_CODE_OK] || s_bRcvCode[GPRS_CODE_ERROR]) {
            bRet = RT_TRUE;
            break;
        }
        rt_thread_delay(RT_TICK_PER_SECOND / 100);
    }

    return bRet;
}



