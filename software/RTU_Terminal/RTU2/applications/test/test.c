
#include "mdtypedef.h"
#include "uart.h"
#include "gpio.h"
#include "rtc.h"
#include "rtconfig.h"
#include "queue.h"
#include "bytebuffer.h"
#include "sdccp.h"
#include "sdccp_test.h"
#include "server_com.h"
#include "test.h"
#include "rtdef.h"
#include <rtthread.h>
#include <board.h>
#include "dev_cfg.h"
#include "user_mb_app.h"


#if 0
void vSdcardDemoTest(void)
{
	FATFS fs[2];          
	f_mount(&fs[0], "0" , 1);
	FIL fsrc;       
	FRESULT res; 
	res = f_open(&fsrc, CONFIG_BASE_PATH "\\" "test.dat", FA_OPEN_ALWAYS| FA_READ | FA_WRITE); 	
	if ( res == FR_OK ){
		USART1_printf(USART1 ,"create file success\r\n");
	}else if (res == FR_EXIST){
		USART1_printf(USART1 ,"file is exist \r\n");
	}

	u8 buf[1024] = {0};
	u16 i = 0;
	for(i = 0 ; i < 1024 ; i++){
		buf[i] = (u8)(i&0xff);
	}

//	printf("begain read data\r\n");
	
	u32 count = 0 ;
//	UINT br = 0;
	UINT bw = 0;
	
	while(1){
		res = f_write(&fsrc, buf, 1024, &bw);
		if(res != FR_OK){
			USART1_printf(USART1 ,"read data error!\r\n");
			break;
		}
		count++;
		if (bw == 0 || count == 1024 * 10) break;
	}
	
	
	USART1_printf(USART1 ,"end write data 1 M\n");
	
	f_close(&fsrc);	                                      
//	SD_TotalSize();
}
#endif 

queue_test_t *g_com_queue = NULL; 
queue_head_t g_queue_com_head ;
mdBOOL g_bIsTestMode = mdFALSE;

mdBOOL  g_uart_revice0 = mdFALSE;
mdBOOL  g_uart_revice1 = mdFALSE;
mdBOOL  g_uart_revice4 = mdFALSE;
mdBOOL  g_uart_revice5 = mdFALSE;

queue_uart_t uart0_queue; 
queue_uart_t uart4_queue; 
queue_uart_t uart5_queue; 

void vTestUSART_IRQHandler(uint32_t instance, uint16_t byteReceived)   
{

	
  if(instance == HW_UART1){
  	 bQueueWrite((*g_com_queue), (char)(byteReceived & 0xff)); //测试通信串口
	// g_uart_revice1 = mdTRUE;
  }else if(instance == HW_UART0){
  	bQueueWrite(uart0_queue, (char)(byteReceived & 0xff));
  	g_uart_revice0 = mdTRUE;
  }else  if(instance == HW_UART4){
  	bQueueWrite(uart4_queue, (char)(byteReceived & 0xff));
  	g_uart_revice4 = mdTRUE;
  }else  if(instance == HW_UART5){
  	bQueueWrite(uart5_queue, (char)(byteReceived & 0xff));
  	g_uart_revice5 = mdTRUE;
  }
  
}


void vTestLedInit(void)
{
    GPIO_QuickInit(TEST_LED_GPIO, TEST_LED_PIN, kGPIO_Mode_OPP);
}

void vTestLedToggle()
{
	GPIO_ToggleBit(TEST_LED_GPIO,TEST_LED_PIN);
}

rt_thread_t g_TestTaskThread;

void vEnterTestMode(void);

static void prvTestTask( void* parameter )
{
	//vMBRTU_ASCIISlavePollStop( TEST_UART_INSTANCE );
	//vMBRTU_ASCIISlavePollStop( TEST_UART_INSTANCE );
        vEnterTestMode();
}


rt_err_t xTestTaskStart( void )
{
    
    if( RT_NULL == g_TestTaskThread ) {
        g_TestTaskThread = rt_thread_create( "TEST", prvTestTask, RT_NULL, 4096, RT_THREAD_PRIORITY_MAX-1, 20 );
        
        if( g_TestTaskThread ) {
            rt_thread_startup( g_TestTaskThread );
            return RT_EOK;
        }
    }

    return RT_ERROR;
}


#define UART1_485_DE_ENABLE()   rt_pin_write( BOARD_UART_1_EN, PIN_LOW)
#define UART1_485_DE_DISABLE()  rt_pin_write( BOARD_UART_1_EN, PIN_HIGH)

#define UART0_485_DE_ENABLE()  rt_pin_write( BOARD_UART_0_EN, PIN_LOW)
#define UART0_485_DE_DISABLE() rt_pin_write( BOARD_UART_0_EN, PIN_HIGH)

#define UART5_485_DE_ENABLE()  rt_pin_write( BOARD_UART_5_EN, PIN_LOW)
#define UART5_485_DE_DISABLE() rt_pin_write( BOARD_UART_5_EN, PIN_HIGH)

#define UART4_485_DE_ENABLE()  rt_pin_write( BOARD_UART_4_EN, PIN_LOW)
#define UART4_485_DE_DISABLE() rt_pin_write( BOARD_UART_4_EN, PIN_HIGH)



static int ReciveUartData(uint32_t instance , u8 *buf, int length, int read_timeout, int interval_timeout, int total_timeout)
{
	u8 ch = 0;
	u32 begin = rt_tick_get();
	u32 total_begin = rt_tick_get();

	queue_uart_t *p_uart;

	if(instance == HW_UART0){
		p_uart = &uart0_queue;	
	}else if(instance == HW_UART4){
		p_uart = &uart4_queue;	
	}else if(instance == HW_UART5){
		p_uart = &uart5_queue;	
	}else{
		return 0;
	}
	
	u32 index = 0;
	memset(buf, 0, length);

	index = 0;
	while (1) {
		if (bQueueRead((*p_uart), ch) == STATUS_OK) {
			buf[index++] = ch;
			break;
		}
		if (SDCCP_CHECK_TIME_OUT(begin, rt_tick_from_millisecond(read_timeout))) return 0;
	}

	begin = rt_tick_get();

	while (1) {
		if (bQueueRead((*p_uart), ch) == STATUS_OK) {
			begin = rt_tick_get();
			buf[index++] = ch;
			if (index >= length) break;
		}
		if (SDCCP_CHECK_TIME_OUT(begin,  rt_tick_from_millisecond(interval_timeout)) || SDCCP_CHECK_TIME_OUT(begin,  rt_tick_from_millisecond(total_timeout))) return index;
	}
	return index;
}


void vUartSend(uint32_t instance, const char *buf,int len)
{
	if(instance == HW_UART0){
		UART0_485_DE_ENABLE();
	}else if(instance == HW_UART1){
		UART1_485_DE_ENABLE();
	}if(instance == HW_UART4){
		UART4_485_DE_ENABLE();
	}if(instance == HW_UART5){
		UART5_485_DE_ENABLE();
	}
	rt_thread_delay( RT_TICK_PER_SECOND / 1000 );

	//UART_putstr(instance, str);
	for(int i = 0 ; i < len ; i++){
	 UART_WriteByte(instance, buf[i]);
	}
	
	rt_thread_delay( RT_TICK_PER_SECOND / 100 );
	
	if(instance == HW_UART0){
		UART0_485_DE_DISABLE();
	}else if(instance == HW_UART1){
		UART1_485_DE_DISABLE();
	}if(instance == HW_UART4){
		UART4_485_DE_DISABLE();
	}if(instance == HW_UART5){
		UART5_485_DE_DISABLE();
	}
	//rt_thread_delay( RT_TICK_PER_SECOND / 10 );
}

extern unsigned char gUartDebug;

mdBOOL bRelaysBegain = mdFALSE;

void vEnterTestMode(void)
{
	 g_com_queue  = rt_calloc(1, sizeof(queue_test_t));
	 rt_pin_mode( BOARD_UART_0_EN, PIN_MODE_OUTPUT );
	 rt_pin_mode( BOARD_UART_1_EN, PIN_MODE_OUTPUT );
	 rt_pin_mode( BOARD_UART_4_EN, PIN_MODE_OUTPUT );
	 rt_pin_mode( BOARD_UART_5_EN, PIN_MODE_OUTPUT );	 
         UART0_485_DE_ENABLE();
	 UART1_485_DE_ENABLE();
	 UART4_485_DE_ENABLE();
   	 UART5_485_DE_ENABLE();
         

	rt_kprintf("\r\n\r\n######## enter test mode!!!!!! \n");
	rt_thread_delay( RT_TICK_PER_SECOND);	
//	rt_kprintf( "rt_console_close_device!\n" );
	gUartDebug = 0;   //关闭调试

	//extern s_AdcCfgPram gAdcCfgPram;

	  gAdcCfgPram.ChannelSleepTime = 2;  //测试模式下加快ADC的刷新速度
	  gAdcCfgPram.eMode = ADC_MODE_TEST; //测试模式下,使用理论值计算ADC值


        rt_serial_t *pSerial = (rt_serial_t *)rt_device_find( "uart1" );
        pSerial->config.baud_rate = BAUD_RATE_115200;
        pSerial->config.data_bits = DATA_BITS_8;
        pSerial->config.stop_bits = STOP_BITS_1;
        pSerial->config.parity = PARITY_NONE;
        pSerial->parent.close( &pSerial->parent );
        pSerial->ops->configure(pSerial, &pSerial->config);
        pSerial->parent.open( &pSerial->parent, RT_DEVICE_OFLAG_RDWR | RT_DEVICE_FLAG_INT_RX );

	
	//NVIC_EnableIRQ(IRQn_Type IRQn)

	//UART_QuickInit(UART1_RX_PC03_TX_PC04, 115200); 
	
	//UART_ITDMAConfig(HW_UART1, kUART_IT_Rx, true);
	
	UART_QuickInit(UART4_RX_PC14_TX_PC15, 115200); 
	UART_ITDMAConfig(HW_UART4, kUART_IT_Rx, true);

	// UART0   
	UART_QuickInit(UART0_RX_PB16_TX_PB17, 115200); 
	UART_ITDMAConfig(HW_UART0, kUART_IT_Rx, true);

	//UART5
	UART_QuickInit(UART5_RX_PD08_TX_PD09, 115200); 
	UART_ITDMAConfig(HW_UART5, kUART_IT_Rx, true);

	 vTestLedToggle();
	
	 //
	 UART0_485_DE_DISABLE();
	 UART1_485_DE_DISABLE();
	 UART4_485_DE_DISABLE();
   	 UART5_485_DE_DISABLE();

	 vInOutInit();

	 int uart_cnt = 0;
	 int index = 0;

	 int cnt = 0;
	 int relay_cnt = 0;


	int led_cnt = 0 ;
	    
	S_PACKAGE_HEAD *p_head = NULL;
	vQueueClear(g_queue_com_head);
	vQueueClear((*g_com_queue));
	vQueueClear(uart0_queue);
	vQueueClear(uart4_queue);
	vQueueClear(uart5_queue);

	mdBYTE  ch = 0 ;

	RTC_DateTime_Type datetime;
	//char rtc_buf[20] = {0};
	
	while(1){
		
		ch = '\0';

		
		if (bQueueRead((*g_com_queue), ch) == mdTRUE) {

			if ((p_head = try2match_com_head(ch)) != NULL) {
				while (client_event_handler_com(p_head) == SDCCP_RETRY) {
					
				}
			}

		}

		#if 0

		RTC_GetTime(&datetime);
		memset(rtc_buf , 0 , 20);
		sprintf(rtc_buf, "%d-%d-%d  %d:%d:%d\r\n" , datetime.year,datetime.month , datetime.day , datetime.hour , datetime.minute, datetime.second);
		
		char a[10] = {0};
		if(uart_cnt++ >= 0){
			uart_cnt = 0;
			sprintf(a, "index = %d:  " , index++);
			//vUartSend(HW_UART0, a);
			vUartSend(HW_UART0, "this is uart 0 ###\r\n");

			vUartSend(HW_UART4, a);
			vUartSend(HW_UART4, "this is uart 4 ###\r\n");

			//vUartSend(HW_UART5, a);
			vUartSend(HW_UART5, "this is uart 5 ###\r\n");
			
			vUartSend(HW_UART1 , rtc_buf);

            /*
			if(bGPRS_ATTest() == RT_TRUE){
				vUartSend(HW_UART1 , "\r\nGPRS is ok ###\r\n");
			}else{
				vUartSend(HW_UART1 , "\r\nGPRS is failed ###\r\n");
			}*/
		}

		#endif

		/*
		char uart_buf[20] = {0};
		
		if(g_uart_revice0 == mdTRUE){
			vUartSend(HW_UART0, "\r\nuart 0 recive data\r\n", strlen("uart 0 recive data @@@ \r\n"));
			char uart_index = ReciveUartData(HW_UART0, uart_buf, 20, 100, 50, 100);
			if(uart_index > 0){
				vUartSend(HW_UART0, uart_buf, uart_index);
			}
			g_uart_revice0 = mdFALSE;
			vQueueClear(uart0_queue);
		}

		if(g_uart_revice4 == mdTRUE){
			
			vUartSend(HW_UART4, "\r\nuart 4 recive data\r\n", strlen("uart 4 recive data @@@ \r\n"));
			char uart_index = ReciveUartData(HW_UART4, uart_buf, 20, 100, 50, 100);
			if(uart_index > 0){
				vUartSend(HW_UART4, uart_buf, uart_index);
			}
			g_uart_revice4 = mdFALSE;
			vQueueClear(uart4_queue);
		}

  		if(g_uart_revice5 == mdTRUE){
  			
  			vUartSend(HW_UART5, "\r\nuart 5 recive data\r\n", strlen("uart 5 recive data @@@ \r\n"));
  			char uart_index = ReciveUartData(HW_UART5, uart_buf, 20, 100, 50, 100);
  			if(uart_index > 0){
  				vUartSend(HW_UART5, uart_buf, uart_index);
  			}
  			
  			g_uart_revice5 = mdFALSE;
  			vQueueClear(uart5_queue);
  		}

		*/
		  
     	rt_thread_delay( RT_TICK_PER_SECOND/100);
		
		vTestLedToggle();

		if(cnt++ >= 3){
			cnt = 0;
			vTTLOutputToggle(TTL_OUTPUT_1);
			vTTLOutputToggle(TTL_OUTPUT_2);
		}

		if( bRelaysBegain){
			if(relay_cnt++ >= 3){
				relay_cnt = 0;
				vRelaysOutputToggle(RELAYS_OUTPUT_1);
				vRelaysOutputToggle(RELAYS_OUTPUT_2);
				vRelaysOutputToggle(RELAYS_OUTPUT_3);
				vRelaysOutputToggle(RELAYS_OUTPUT_4);
			}
		}
		WDOG_FEED();
		
	}
}


void set_product_info(const S_MSG *msg, byte_buffer_t *bb)
{
	S_MSG_SET_PRODUCT_INFO_REQUEST request;
	
	
	bb_get_bytes(bb , request.info.xSN.szSN , sizeof(request.info.xSN));
	request.info.usHWVer = bb_get_short(bb);
	request.info.usSWVer = bb_get_short(bb);
	bb_get_bytes(bb ,request.info.xOEM.szOEM , sizeof(request.info.xOEM));
	bb_get_bytes(bb ,request.info.xIp.szIp , sizeof(request.info.xIp));
	bb_get_bytes(bb ,request.info.xNetMac.mac , sizeof(request.info.xNetMac));
	request.info.usYear = bb_get_short(bb);
	request.info.usMonth = bb_get_short(bb);
	request.info.usDay = bb_get_short(bb);
	request.info.usHour = bb_get_short(bb);
	request.info.usMin = bb_get_short(bb);
	request.info.usSec = bb_get_short(bb);
	bb_get_bytes(bb ,request.info.hwid.hwid , sizeof(request.info.hwid.hwid));
	bb_get_bytes(bb ,request.info.xPN.szPN, sizeof(request.info.xPN));

	vDevCfgInit();//先读出来
	memcpy(g_xDevInfoReg.xDevInfo.xSN.szSN , request.info.xSN.szSN,sizeof(request.info.xSN));
	memcpy(g_xDevInfoReg.xDevInfo.xOEM.szOEM, request.info.xOEM.szOEM,sizeof(request.info.xOEM));
	memcpy(g_xDevInfoReg.xDevInfo.xIp.szIp, request.info.xIp.szIp,sizeof(request.info.xIp));
	g_xDevInfoReg.xDevInfo.usYear = request.info.usYear;
	g_xDevInfoReg.xDevInfo.usMonth = request.info.usMonth;
	g_xDevInfoReg.xDevInfo.usDay = request.info.usDay;
	g_xDevInfoReg.xDevInfo.usHour = request.info.usHour;
	g_xDevInfoReg.xDevInfo.usMin = request.info.usMin;
	g_xDevInfoReg.xDevInfo.usSec = request.info.usSec;

	rt_uint32_t hwid[4] = { SIM->UIDH, SIM->UIDMH, SIM->UIDML, SIM->UIDL };
	char *phwid = (char *)hwid;
	memset( g_xDevInfoReg.xDevInfo.hwid.hwid, 0, sizeof(g_xDevInfoReg.xDevInfo.hwid) );
	sprintf(g_xDevInfoReg.xDevInfo.hwid.hwid, "%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X", phwid[0],phwid[1],phwid[2],phwid[3],phwid[4],phwid[5],
		phwid[6],phwid[7],phwid[8],phwid[9],phwid[10],phwid[11],phwid[12],phwid[13],phwid[14], phwid[15]);
        memcpy(g_xDevInfoReg.xDevInfo.xPN.szPN, request.info.xPN.szPN,sizeof(request.info.xPN));
	prvSaveDevInfoToFs();

        host_cfg_init();
        
	S_MSG_SET_PRODUCT_INFO_RESPONSE response;
	response.Type = 0;
	response.Value = 0;
	response.RetCode = MSG_TEST_ERROR_OK;

	
	send_base_response(msg, &response);
}


mdBOOL  bIsInitOk = mdFALSE;
mdBOOL  bIsMountOk = mdFALSE;
mdBOOL  bIsMusicListOk = mdFALSE;

void vTestSdCard(const S_MSG *msg, byte_buffer_t *bb)
{
	S_MSG_TEST_SDCARD_RESPONSE response;
	
	response.Type = 0;
	response.RetCode = MSG_TEST_ERROR_ERR;
	response.Value = 0;
  
	int size = 0;
    struct statfs buffer;
	if( rt_sd_in() ) {
        if( dfs_statfs( BOARD_SDCARD_MOUNT, &buffer) == 0 ) {
          response.Value = (int)(((long long)buffer.f_blocks * buffer.f_bsize) / 1024);
		  response.RetCode = MSG_TEST_ERROR_OK;
        } 
	}
	
	send_base_response(msg, &response);
	
}

extern mdBOOL bIsGprsInitOk ;

void vTestGprs(const S_MSG *msg, byte_buffer_t *bb)
{
	S_MSG_TEST_GPRS_RESPONSE response;
	
	response.Type = 0;
	response.RetCode = MSG_TEST_ERROR_ERR;
	response.Value = 0;

	if(bIsGprsInitOk){
		response.RetCode = MSG_TEST_ERROR_OK;
	}
	
	send_base_response(msg, &response);
	
}

extern rt_bool_t g_zigbee_init;

void vTestZigbee(const S_MSG *msg, byte_buffer_t *bb)
{
	S_MSG_TEST_ZIGBEE_RESPONSE response;
	
	response.Type = 0;
	response.RetCode = MSG_TEST_ERROR_ERR;

	response.Value = 0;

	if(g_zigbee_init == RT_TRUE){
		response.RetCode = MSG_TEST_ERROR_OK;
		response.Value = (g_zigbee_cfg.usType | (g_zigbee_cfg.usVer << 16));
	}
	
	send_base_response(msg, &response);
	
}

static int rtc_post_skip(u32 *diff)
{
	RTC_DateTime_Type tm1;
	RTC_DateTime_Type tm2;
	u32 start1 = 0;
	u32 start2 = 0;

	//sec_to_date( my_get_time(), &tm1 );
	RTC_GetTime(&tm1);
	start1 = rt_tick_get();

	while (1) {
		RTC_GetTime(&tm2);
		start2 = rt_tick_get();

		if (tm1.second != tm2.second) {
			break;
		}

		if (start2 - start1 > (1500)) { // 1.5s
			break;
		}
	}
	
	if (tm1.second != tm2.second) {
		*diff = (start2 - start1);
		return 0;
	} else {
		return -1;
	}
}

void vTestRtc(const S_MSG *msg, byte_buffer_t *bb)
{
	S_MSG_TEST_RTC_RESPONSE response;
	u32 diff;
	u32 i;


	response.Type = 0;
	response.RetCode = MSG_TEST_ERROR_RTC;
	response.Value = 0;

	for (i = 0; i < 1; i++) {
		if (rtc_post_skip(&diff) == 0) {
			break;
		}
	}

	if (i >= 2) {
		goto END_;
	}

	for (i = 0; i < 2; i++) {
		if (rtc_post_skip(&diff) != 0) {
			break;
		}

		if ((diff < (950)) || (diff > (1050)) ) {
			break;
		}
	}

	if (i >= 2) {
		response.RetCode = MSG_TEST_ERROR_OK;
	}

END_:
	send_base_response(msg, &response);
}


void vTestSpiFlash(const S_MSG *msg, byte_buffer_t *bb)
{
	S_MSG_TEST_SPIFLASH_RESPONSE response;
	
	response.Type = 0;
	response.RetCode = MSG_TEST_ERROR_ERR;
	response.Value = 0;

    struct statfs buffer;
	if( dfs_statfs( BOARD_SPIFLASH_FLASH_MOUNT, &buffer) == 0 ) {
          response.Value = (int)(((long long)buffer.f_blocks * buffer.f_bsize) / 1024);
		  response.RetCode = MSG_TEST_ERROR_OK;
	}
	
	send_base_response(msg, &response);
}

#include "enet_phy.h"

#define PHY_BMCR                    (0x00) /* Basic Control */
#define PHY_BMCR_RESET              (0x8000)
#define PHY_PHYIDR1                 (0x02) /* PHY Identifer 1 */
#define ENET_DELAY_MS(_ms)          rt_thread_delay( RT_TICK_PER_SECOND * (_ms) / 1000 )

void vTestNet(const S_MSG *msg, byte_buffer_t *bb)
{
	S_MSG_TEST_NET_RESPONSE response;
	
	response.Type = 0;
	response.RetCode = MSG_TEST_ERROR_ERR;
	response.Value = 0;

	rt_pin_mode( BOARD_GPIO_NET_RESET, PIN_MODE_OUTPUT );
    rt_pin_write( BOARD_GPIO_NET_RESET, PIN_LOW );
    rt_thread_delay( RT_TICK_PER_SECOND / 10 );
    rt_pin_write( BOARD_GPIO_NET_RESET, PIN_HIGH );
    rt_thread_delay( RT_TICK_PER_SECOND / 10 );
    
        uint16_t usData;
	uint16_t timeout = 0;

	uint8_t addr;
        
        
    
    /* init MII interface */
    ENET_MII_Init();

	 
    _AutoDiscover(&addr);
    int gChipAddr = addr;
    
    
    /* software reset PHY */
    ENET_MII_Write(gChipAddr, PHY_BMCR, PHY_BMCR_RESET);
    ENET_DELAY_MS(10);
    
    /* waitting for reset OK */
    do
    {
        ENET_DELAY_MS(5);
        timeout++;
        usData = 0xFFFF;
        ENET_MII_Read(gChipAddr, PHY_PHYIDR1, &usData );
        if((usData != 0xFFFF) && (usData != 0x0000))
        {
          //  ENET_PHY_TRACE("PHY_PHYIDR1:0x%X\r\n",usData);
			response.RetCode = MSG_TEST_ERROR_OK;
            break;
        }
    }while(timeout < 10);


	send_base_response(msg, &response);
}

static eInOut_stat_t sta;
static eInOut_stat_t sta_bak;
static mdBOOL bTTLInputChang(eTTL_Input_Chanel_t chan)
{
	vTTLInputputGet(chan, &sta);
	if(sta != sta_bak){
		sta_bak = sta;
		return mdTRUE;
	}
	return mdFALSE;
}

unsigned int cnt = 0;
#define CNT_TTL_MIN (10)

void vTestTTLInput(const S_MSG *msg, byte_buffer_t *bb)
{
	S_MSG_TEST_TTL_INPUT_RESPONSE response;
	S_MSG_TEST_TTL_INPUT_REQUEST  request;
	//vInOutInit();
	
	request.Type = bb_get(bb);
	
	response.Type = request.Type;
	response.Value = 0;
	response.RetCode = MSG_TEST_ERROR_ERR;

	switch(request.Type){
		case TTLInput_GET_1:{
			 cnt = 0;
			int begin = rt_tick_get();
			while(1){
				if(bTTLInputChang(TTL_INPUT_1)){
					cnt++;
				}
				if(SDCCP_CHECK_TIME_OUT(begin, rt_tick_from_millisecond(500))) break;
				//rt_thread_delay( RT_TICK_PER_SECOND/1000);
				
			}
			if(cnt >= CNT_TTL_MIN){
				response.RetCode = MSG_TEST_ERROR_OK;
			}
			break;
		}
		case TTLInput_GET_2:{
			 cnt = 0;
			int begin = rt_tick_get();
			while(1){
				if(bTTLInputChang(TTL_INPUT_2)){
					cnt++;
				}
				if(SDCCP_CHECK_TIME_OUT(begin, rt_tick_from_millisecond(500))) break;
				//rt_thread_delay( RT_TICK_PER_SECOND/1000);
				
			}
			if(cnt >= CNT_TTL_MIN){
				response.RetCode = MSG_TEST_ERROR_OK;
			}
			break;
		}
		case TTLInput_GET_3:{
			 cnt = 0;
			int begin = rt_tick_get();
			while(1){
				if(bTTLInputChang(TTL_INPUT_3)){
					cnt++;
				}
				if(SDCCP_CHECK_TIME_OUT(begin, rt_tick_from_millisecond(500))) break;
				//rt_thread_delay( RT_TICK_PER_SECOND/1000);
				
			}
			if(cnt >= CNT_TTL_MIN){
				response.RetCode = MSG_TEST_ERROR_OK;
			}
			break;
		}
		case TTLInput_GET_4:{
			 cnt = 0;
			int begin = rt_tick_get();
			while(1){
				if(bTTLInputChang(TTL_INPUT_4)){
					cnt++;
				}
				if(SDCCP_CHECK_TIME_OUT(begin, rt_tick_from_millisecond(500))) break;
				//rt_thread_delay( RT_TICK_PER_SECOND/1000);
				
			}
			if(cnt >= CNT_TTL_MIN){
				response.RetCode = MSG_TEST_ERROR_OK;
			}
			break;
		}
		default:
			response.RetCode = MSG_TEST_ERROR_UNKNOWN;
		break;
		
	}
	send_base_response(msg, &response);
}




static eInOut_stat_t sta_on;
static eInOut_stat_t sta_bak_on;
static mdBOOL bOnOffInputChang(eOnOff_Input_Chanel_t chan)
{
	vOnOffInputGet(chan, &sta_on);
	if(sta_on != sta_bak_on){
		sta_bak_on = sta_on;
		return mdTRUE;
	}
	return mdFALSE;
}

void vTestOnOffInput(const S_MSG *msg, byte_buffer_t *bb)
{
	S_MSG_TEST_ON_OFF_RESPONSE response;
	S_MSG_TEST_ON_OFF_REQUEST  request;

	request.Type = bb_get(bb);
	
	response.Type = request.Type;
	response.Value = 0;
	response.RetCode = MSG_TEST_ERROR_ERR;
	
	switch(request.Type){
		case ONOFF_1:{
			int cnt = 0;
			int begin = rt_tick_get();
			while(1){
				if(bOnOffInputChang(ONOFF_INPUT_1)){
					cnt++;
				}
				if(SDCCP_CHECK_TIME_OUT(begin, rt_tick_from_millisecond(500))) break;
				//rt_thread_delay( RT_TICK_PER_SECOND/1000);
				
			}
			if(cnt >= CNT_TTL_MIN){
				response.RetCode = MSG_TEST_ERROR_OK;
			}
			break;
		}
		case ONOFF_2:{
			int cnt = 0;
			int begin = rt_tick_get();
			while(1){
				if(bOnOffInputChang(ONOFF_INPUT_2)){
					cnt++;
				}
				if(SDCCP_CHECK_TIME_OUT(begin, rt_tick_from_millisecond(500))) break;
				//rt_thread_delay( RT_TICK_PER_SECOND/1000);
				
			}
			if(cnt >= CNT_TTL_MIN){
				response.RetCode = MSG_TEST_ERROR_OK;
			}
			break;
		}
		case ONOFF_3:{
			int cnt = 0;
			int begin = rt_tick_get();
			while(1){
				if(bOnOffInputChang(ONOFF_INPUT_3)){
					cnt++;
				}
				if(SDCCP_CHECK_TIME_OUT(begin, rt_tick_from_millisecond(500))) break;
				//rt_thread_delay( RT_TICK_PER_SECOND/1000);
				
			}
			if(cnt >= CNT_TTL_MIN){
				response.RetCode = MSG_TEST_ERROR_OK;
			}
			break;
		}
		case ONOFF_4:{
			int cnt = 0;
			int begin = rt_tick_get();
			while(1){
				if(bOnOffInputChang(ONOFF_INPUT_4)){
					cnt++;
				}
				if(SDCCP_CHECK_TIME_OUT(begin, rt_tick_from_millisecond(500))) break;
				//rt_thread_delay( RT_TICK_PER_SECOND/1000);
				
			}
			if(cnt >= CNT_TTL_MIN){
				response.RetCode = MSG_TEST_ERROR_OK;
			}
			break;
		}
		default:
			response.RetCode = MSG_TEST_ERROR_UNKNOWN;
		break;
		
	}

	
	
	send_base_response(msg, &response);
}

extern AIResult_t g_AIEngUnitResult;
extern AIResult_t g_AIMeasResult;


void vTestGetAdc(const S_MSG *msg, byte_buffer_t *bb)
{
	S_MSG_GET_TEST_ADC_REQUEST    request;
	S_MSG_TEST_GET_ADC_RESPONSE   response;

	gAdcCfgPram.eMode = ADC_MODE_TEST;
	gAdcCfgPram.ChannelSleepTime = 2;

	rt_thread_delay(250);

	
	
	request.Type =   bb_get(bb);

	response.RetCode = MSG_TEST_ERROR_ERR;

	s_AdcValue_t xAdcVal;
	memset(&xAdcVal, 0 , sizeof(s_AdcValue_t));
	
	s_CorrectionFactor_t factor;
	factor.factor = 0;

	
	unsigned long long EngUnit = 0;
	double Measure = 0.0f;

	EngUnit =   g_AIEngUnitResult.fAI_xx[request.Type];
        Measure =   g_AIMeasResult.fAI_xx[request.Type];
	
	response.xAdcInfo.usChannel = request.Type;
	response.xAdcInfo.usRange =  Range_0_20MA;
	//response.xAdcInfo.usEngVal = xAdcVal.usEngUnit;
	response.xAdcInfo.usEngVal = EngUnit;

	//获取0量程时的ADC值
	if(EngUnit < 5000){ //表示是0量程
		float factor = Measure;
		gCalEntryBak[request.Type].factor = factor;
	}
	
	if(Measure < 0.0f){
		Measure = 0.0f;
	}
	response.xAdcInfo.usMeasureVal = (mdUINT32)(Measure * 1000);
	response.RetCode = MSG_TEST_ERROR_OK;

	send_adc_get_response(msg, &response);
}



//获取校验过后的ADC值

void vTestGetTestAdcValue(const S_MSG *msg, byte_buffer_t *bb)
{
	S_MSG_GET_TEST_ADC_REQUEST    request;
	S_MSG_TEST_GET_ADC_RESPONSE   response;
	
	request.Type =   bb_get(bb);

	response.RetCode = MSG_TEST_ERROR_ERR;

    if(request.Type == ADC_CHANNEL_0){
        vAdcCalCfgInit();
    }

	 gAdcCfgPram.eMode = ADC_MODE_CALC;
	 rt_thread_delay(250);

	ADC_CHANNEL_E chan = request.Type;
	s_AdcValue_t xAdcVal;
	memset(&xAdcVal, 0 , sizeof(s_AdcValue_t));


	
	//vGetAdcTestValueInfo(chan, Range_0_20MA, Range_4_20MA, 0, 100, factor, &xAdcVal);
	xAdcVal.usEngUnit =   g_AIEngUnitResult.fAI_xx[chan];
        xAdcVal.fMeasure =   g_AIMeasResult.fAI_xx[chan];

	s_CalEntry_t CalEntry;
	memset(&CalEntry,0,sizeof(s_CalEntry_t));
        vGetCalValue(chan, &CalEntry);
	
	response.xAdcInfo.usChannel = CalEntry.xMin.usAdcValue;   		      //表示当前通道校准的最小值
	response.xAdcInfo.usRange =  CalEntry.xMiddle.usAdcValue;    	      //表示当前通道校准的中间值
	response.xAdcInfo.usEngVal = CalEntry.xMax.usAdcValue; 				  //表示当前通道校准的最大值
	response.xAdcInfo.usMeasureVal = (mdUINT32)(xAdcVal.fMeasure * 1000); //当前通道测量的实测值

	response.RetCode = MSG_TEST_ERROR_OK;	
	send_adc_get_response(msg, &response);
}


enum{
	CHECK_VAL_0_MA = 0x00,
	CHECK_VAL_10_MA,
	CHECK_VAL_20_MA,
};

void vTestSetCheckDefault(const S_MSG *msg, byte_buffer_t *bb)
{
	S_MSG_TEST_NET_RESPONSE response;
	
	response.Type = 0;
	response.RetCode = MSG_TEST_ERROR_ERR;
	response.Value = 0;

	SetAdcCalCfgDefault();
	//SaveAdcCalInfoToFs();
	//send_base_response(msg, &response);
}

#define ADC_CAL_RANG (1000)

extern const s_CalEntry_t gDefaultCalEntry[ADC_CHANNEL_NUM];
void vTestSetAdc(const S_MSG *msg, byte_buffer_t *bb)
{

	S_MSG_SET_TEST_SET_ADC_REQUEST  request;
	S_MSG_TEST_SET_ADC_RESPONSE response;
	
	request.xAdcInfo.usChannel =   bb_get_int(bb);
	request.xAdcInfo.usRange =     bb_get_int(bb);
	request.xAdcInfo.usEngVal =    bb_get_int(bb);
	request.xAdcInfo.usMeasureVal = bb_get_int(bb);
	
	s_CalEntry_t  xCalEntry;
	ADC_CHANNEL_E chann = request.xAdcInfo.usChannel;
	eCalCountType_t eCalType;


	if(request.xAdcInfo.usRange == CHECK_VAL_0_MA){
		xCalEntry.xMin.fMeterValue = 4.0f;
		eCalType = SET_MIN;
		xCalEntry.xMin.usAdcValue = request.xAdcInfo.usEngVal;
	}else if(request.xAdcInfo.usRange == CHECK_VAL_10_MA){
		xCalEntry.xMiddle.fMeterValue = 10.0f;
		eCalType = SET_MIDDLE;
		xCalEntry.xMiddle.usAdcValue = request.xAdcInfo.usEngVal;
	}else if(request.xAdcInfo.usRange == CHECK_VAL_20_MA){
		xCalEntry.xMax.fMeterValue = 18.0f;
		eCalType = SET_MAX;
		xCalEntry.xMax.usAdcValue = request.xAdcInfo.usEngVal;
	}

	vSetCalValue(chann, eCalType,&xCalEntry);


	response.Type = 0;
	response.Value = 0;
	response.RetCode = MSG_TEST_ERROR_OK;


	if(request.xAdcInfo.usRange == CHECK_VAL_20_MA && chann == ADC_CHANNEL_7){
		mdBOOL set_ok = mdTRUE;
		for(int i = 0 ; i < 8 ;i++){
			if((gCalEntryBak[i].xMin.usAdcValue <= (gDefaultCalEntry[i].xMin.usAdcValue - ADC_CAL_RANG)) || (gCalEntryBak[i].xMin.usAdcValue >= (gDefaultCalEntry[i].xMin.usAdcValue + ADC_CAL_RANG))){
				set_ok = mdFALSE;
				break;
			}
			if((gCalEntryBak[i].xMiddle.usAdcValue <= (gDefaultCalEntry[i].xMiddle.usAdcValue - ADC_CAL_RANG)) || (gCalEntryBak[i].xMiddle.usAdcValue >= (gDefaultCalEntry[i].xMiddle.usAdcValue + ADC_CAL_RANG))){
				set_ok = mdFALSE;
				break;
			}
			if((gCalEntryBak[i].xMax.usAdcValue <= (gDefaultCalEntry[i].xMax.usAdcValue - ADC_CAL_RANG)) || (gCalEntryBak[i].xMax.usAdcValue >= (gDefaultCalEntry[i].xMax.usAdcValue + ADC_CAL_RANG))){
				set_ok = mdFALSE;
			}
		}
		if(set_ok != mdTRUE){
			response.RetCode = MSG_TEST_ERROR_ERR;
		}else {
			SaveAdcCalInfoToFs();
		}
	}
	
	send_base_response(msg, &response);
}

void vTestVol(const S_MSG *msg, byte_buffer_t *bb)
{
	S_MSG_TEST_VOL_RESPONSE response;
	
	response.Type = 0;
	response.RetCode = MSG_TEST_ERROR_OK;
	response.Value = 0;

	send_base_response(msg, &response);
}


//按顺序依次是 UART4 UART5 UART0
void vTestUart(const S_MSG *msg, byte_buffer_t *bb)
{
	S_MSG_TEST_UART_RESPONSE response;
	S_MSG_TEST_UART_REQUEST  request;

	request.Type = bb_get(bb);
	
	response.Type =  request.Type;
	response.Value = 0;
	response.RetCode = MSG_TEST_ERROR_ERR;

	short rtype = (request.Type&0x0f);
	short cType = ((request.Type>>4)&0x0f);

	switch(rtype){
		case UART_GET_1:
			if(cType == UART_RECIVE){
				byte buf[20] = {0};
				if(ReciveUartData(HW_UART4, buf, sizeof(buf), 200, 100, 500) == sizeof(buf)){
                    int i;
					for(i = 0; i < sizeof(buf);i++){
						if(buf[i] != i) break;
					}
					if(i >= sizeof(buf)){
						response.RetCode = MSG_TEST_ERROR_OK;
					}
				}
				send_base_response(msg, &response);
				
				vQueueClear(uart4_queue);
			
				
			}else if(cType == UART_SEND){
				for(int i = 0; i < 20; i++){
					vUartSend(HW_UART4, (char*)&i, 1);
				}
				send_base_response(msg, &response);
			}
			break;
		case UART_GET_2:
			if(cType == UART_RECIVE){
				byte buf[20] = {0};
				if(ReciveUartData(HW_UART5, buf, sizeof(buf), 200, 100, 500) == sizeof(buf)){
                                        int i;
					for( i = 0; i < sizeof(buf);i++){
						if(buf[i] != i) break;
					}
					if(i >= sizeof(buf)){
						response.RetCode = MSG_TEST_ERROR_OK;
					}
				}
				send_base_response(msg, &response);
				vQueueClear(uart5_queue);
			}else if(cType == UART_SEND){
				for(int i = 0; i < 20; i++){
					vUartSend(HW_UART5, (char*)&i, 1);
				}
				send_base_response(msg, &response);
			}
			break;
		case UART_GET_3:
			if(cType == UART_RECIVE){
				byte buf[20] = {0};
				if(ReciveUartData(HW_UART0, buf, sizeof(buf), 200, 100, 500) == sizeof(buf)){
                                        int i;
					for( i = 0; i < sizeof(buf);i++){
						if(buf[i] != i) break;
					}
					if(i >= sizeof(buf)){
						response.RetCode = MSG_TEST_ERROR_OK;
					}
				}
				send_base_response(msg, &response);
				vQueueClear(uart0_queue);
			}else if(cType == UART_SEND){
				for(int i = 0; i < 20; i++){
					vUartSend(HW_UART0, (char*)&i, 1);
				}
				send_base_response(msg, &response);
			}
			break;
	}

	
	//send_base_response(msg, &response);
	
}

void vTestTTLOutputRelay(const S_MSG *msg, byte_buffer_t *bb)
{
	S_MSG_TEST_RTC_RESPONSE response;
	
	response.Type = 0;
	response.RetCode = MSG_TEST_ERROR_OK;
	response.Value = 0;

	send_base_response(msg, &response);
}

void vTestEraseFlashErase(const S_MSG *msg, byte_buffer_t *bb)
{
	S_MSG_TEST_RTC_RESPONSE response;
	
	/*response.Type = 0;
	response.RetCode = MSG_TEST_ERROR_ERR;
	response.Value = 0;

	w25qxx_flash_erase_chip();*/

	dfs_mkfs("elm", BOARD_SPIFLASH_FLASH_NAME );

	response.RetCode = MSG_TEST_ERROR_OK;

	send_base_response(msg, &response);
}

void vTestGetCalValue(const S_MSG *msg, byte_buffer_t *bb)
{
	S_MSG_TEST_RTC_RESPONSE response;
	
	response.Type = 0;
	response.RetCode = MSG_TEST_ERROR_OK;
	response.Value = 0;

	vAdcCalCfgInit();

	int i = 0;
	for(i = 0 ; i< 8; i++){
		if(gCalEntry[i].xMax.usAdcValue == gDefaultCalEntry[i].xMax.usAdcValue){
			response.RetCode = MSG_TEST_ERROR_ERR;
			break;
		}
	}
	
	send_base_response(msg, &response);
}


void vTestRelays(const S_MSG *msg, byte_buffer_t *bb)
{
	S_MSG_TEST_TTL_OUTPUT_RELAY_RESPONSE response;
	S_MSG_TEST_TTL_OUTPUT_RELAY_REQUEST  request;

	request.Type = bb_get(bb);

	if(request.Type == 1){
		bRelaysBegain = mdTRUE;
	}else {
		bRelaysBegain = mdFALSE;
	}

	response.RetCode = MSG_TEST_ERROR_OK;
	send_base_response(msg, &response);
}




