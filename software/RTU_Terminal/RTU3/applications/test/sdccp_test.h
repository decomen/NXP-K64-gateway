#ifndef __SDCCP_TEST_TYPES_H__
#define __SDCCP_TEST_TYPES_H__

#include "user_mb_app.h"

typedef enum {
	        MSG_TEST_ONLINE_STATUS = 0x00,          //当前运行状�? 由基板随时上�?
            
            MSG_TEST_ON_OFF = 0xd0,                 //电压测试
            MSG_TEST_RTC = 0xd1,                     //RTC测试
            MSG_TEST_SPIFLASH = 0xd2,                //SPI Flash 测试
            MSG_TEST_SDCARD = 0xd3,                 //SPI Flash 测试
            MSG_TEST_GPRS = 0xd4,
            MSG_TEST_ZIGBEE = 0xd5,
            MSG_TEST_TTLInput = 0xd6,              //TTLInput 测试
            MSG_TEST_LORA = 0xd7,
            MSG_TEST_NET = 0xd8,
            MSG_TEST_ADC = 0xd9,
            MSG_TEST_VOL = 0xda,
            MSG_TEST_UART = 0xdb,  
            MSG_TEST_TTL_OUTPUT_RELAY = 0xdc,
            MSG_TEST_SetCheck = 0xdd, 
            
            MSG_TEST_GET_TEST_ADC = 0xdf, //��ȡУ�����ADC��У׼ֵ
                   
            MSG_TEST_SET_PRODUCT_INFO = 0xe0,       //设置产品信息, 所有测试选项通过后再进行出厂设置
            MSG_TEST_GET_PRODUCT_INFO = 0xe1,       //获取产品信息, 后期使用

            MSG_TEST_SET_ADC_INFO = 0xe2,
            MSG_TEST_MKFS = 0xe3,
            MSG_TEST_GET_CAL_VAL = 0xe4,
} TEST_PACKAGE_MSG_TYPE_E;




/*

Vol_GET_0	�ܵ�ѹ
Vol_GET_1	5V��ѹ
Vol_GET_2	3.3v��ѹ
Vol_GET_3	GPRS��ѹ
Vol_GET_4	���ڵ�ѹ
Vol_GET_5	ADC��ѹ
Vol_GET_6	��ص�ѹ

*/

typedef enum 
{
    Vol_GET_0 = 0x00,
    Vol_GET_1,
    Vol_GET_2,
    Vol_GET_3,
    Vol_GET_4,
    Vol_GET_5,
    Vol_GET_6,
}S_VOLTestType_E;

typedef enum TTLOutputRelayTestType
{
    TEST_TTL_OUTPUT_1 = 0x00,
    TEST_TTL_OUTPUT_2,
    
    TEST_RELAY_1 ,
    TEST_RELAY_2,
    TEST_RELAY_3,
    TEST_RELAY_4,

}s_TTLOutputTestType_E;

typedef enum AdcTestType
{
    Adc_GET_0 = 0x00,
    Adc_GET_1,
    Adc_GET_2,
    Adc_GET_3,
    Adc_GET_4,
    Adc_GET_5,
    Adc_GET_6,
    Adc_GET_7,
}s_AdcTestType_E;

typedef enum UartTestType
{
    UART_GET_1 = 0x00,
    UART_GET_2,
    UART_GET_3,

	UART_SEND,
	UART_RECIVE,
}s_UartTestType_E;

typedef enum OnOffTestType
{
    ONOFF_1 = 0x00,
    ONOFF_2,
    ONOFF_3,
    ONOFF_4,

}s_OnOffTestType_E;

typedef enum TTLInputTestType
{
    TTLInput_GET_1 = 0x00,
    TTLInput_GET_2,
    TTLInput_GET_3,
    TTLInput_GET_4,
}s_TTLInputTestType_E;


typedef enum {
	S_TEST_OK = 0x00,
	S_TEST_NO,
	S_TEST_FAILED,
} S_TESTStatusType_E;

typedef enum {
	O_TEST_WAIT = 0x00,
	O_TEST_ING,
} S_OnLineType_E;

typedef enum {
	ACK_TEST_ERROR_OK = 0x00,		//�ɹ�
	ACK_TEST_ERROR_ERR,			//ʧ��
} TEST_ACK_TYPE_E;

typedef enum {
	MSG_TEST_ERROR_OK = 0x00,
	
	MSG_TEST_ERROR_RTC 			= 0x10,
	MSG_TEST_ERROR_FLASH_INIT 	= 0x20,
	MSG_TEST_ERROR_FLASH_RW,
	
	MSG_TEST_ERROR_SDCARD_INIT 	= 0x30,
	MSG_TEST_ERROR_SDCARD_MOUNT,
	MSG_TEST_ERROR_SDCARD_MUSICLIST,
	
	MSG_TEST_ERROR_VS1003_INIT 	= 0x40,
	MSG_TEST_ERROR_NO_MUSIC = 0x41,

	MSG_TEST_ERROR_ERR,
	MSG_TEST_ERROR_TIMEOUT,

	

	MSG_TEST_ERROR_UNKNOWN = 0xFF
} TEST_MSG_ERR_TYPE_E;

#pragma pack(1)

typedef struct _S_MSG_TEST_BASE_REQUEST {
	byte Type;
} S_MSG_TEST_BASE_REQUEST;

typedef struct _S_MSG_TEST_BASE_RESPONSE {
	byte Type;
	byte RetCode;
	u32 Value;
} S_MSG_TEST_BASE_RESPONSE;

typedef struct S_MSG_TEST_ONLINE_STATUS_REPORT {
	byte OnlineStatus;
} S_MSG_TEST_ONLINE_STATUS_REPORT;

typedef S_MSG_TEST_BASE_REQUEST S_MSG_TEST_RTC_REQUEST;
typedef S_MSG_TEST_BASE_RESPONSE S_MSG_TEST_RTC_RESPONSE;

typedef S_MSG_TEST_BASE_REQUEST S_MSG_TEST_ON_OFF_REQUEST;
typedef S_MSG_TEST_BASE_RESPONSE S_MSG_TEST_ON_OFF_RESPONSE;


typedef S_MSG_TEST_BASE_REQUEST S_MSG_TEST_VOL_REQUEST;
typedef S_MSG_TEST_BASE_RESPONSE S_MSG_TEST_VOL_RESPONSE;

typedef S_MSG_TEST_BASE_REQUEST S_MSG_TEST_SPIFLASH_REQUEST;
typedef S_MSG_TEST_BASE_RESPONSE S_MSG_TEST_SPIFLASH_RESPONSE;


typedef S_MSG_TEST_BASE_REQUEST S_MSG_TEST_SDCARD_REQUEST;
typedef S_MSG_TEST_BASE_RESPONSE S_MSG_TEST_SDCARD_RESPONSE;


typedef S_MSG_TEST_BASE_REQUEST S_MSG_TEST_GPRS_REQUEST;
typedef S_MSG_TEST_BASE_RESPONSE S_MSG_TEST_GPRS_RESPONSE;

typedef S_MSG_TEST_BASE_REQUEST S_MSG_TEST_ZIGBEE_REQUEST;
typedef S_MSG_TEST_BASE_RESPONSE S_MSG_TEST_ZIGBEE_RESPONSE;

typedef S_MSG_TEST_BASE_REQUEST S_MSG_TEST_TTL_INPUT_REQUEST;
typedef S_MSG_TEST_BASE_RESPONSE S_MSG_TEST_TTL_INPUT_RESPONSE;

typedef S_MSG_TEST_BASE_REQUEST S_MSG_TEST_LORA_REQUEST;
typedef S_MSG_TEST_BASE_RESPONSE S_MSG_TEST_LORA_RESPONSE;

typedef S_MSG_TEST_BASE_REQUEST S_MSG_TEST_NET_REQUEST;
typedef S_MSG_TEST_BASE_RESPONSE S_MSG_TEST_NET_RESPONSE;

typedef struct {
	int usChannel;                    						 //ADCͨ��
	int usRange; 											 //��ǰѡ������ADCУ׼ֵ
 	int usEngVal; 											//adc������ֵ
 	int usMeasureVal; 										//����ֵ                
}s_AdcInfo;

typedef S_MSG_TEST_BASE_REQUEST S_MSG_GET_TEST_ADC_REQUEST;

typedef struct _S_MSG_TEST_GET_ADC_RESPONSE{
	byte RetCode;
	s_AdcInfo xAdcInfo;
} S_MSG_TEST_GET_ADC_RESPONSE;




typedef struct _S_MSG_SET_TEST_ADC_REQUEST{
	s_AdcInfo xAdcInfo;
}S_MSG_SET_TEST_SET_ADC_REQUEST;

typedef S_MSG_TEST_BASE_RESPONSE S_MSG_TEST_SET_ADC_RESPONSE;
	


typedef S_MSG_TEST_BASE_REQUEST S_MSG_TEST_UART_REQUEST;
typedef S_MSG_TEST_BASE_RESPONSE S_MSG_TEST_UART_RESPONSE;

typedef S_MSG_TEST_BASE_REQUEST S_MSG_TEST_TTL_OUTPUT_RELAY_REQUEST;
typedef S_MSG_TEST_BASE_RESPONSE S_MSG_TEST_TTL_OUTPUT_RELAY_RESPONSE;



typedef S_MSG_TEST_BASE_REQUEST S_MSG_TEST_BLE_REQUEST;
typedef S_MSG_TEST_BASE_RESPONSE S_MSG_TEST_BLE_RESPONSE;

typedef struct _S_MSG_TEST_BLE_CONNECT_REQUEST {
	S_MSG_TEST_BASE_REQUEST request;
	byte MAC[6];
} S_MSG_TEST_BLE_CONNECT_REQUEST;

typedef struct _S_MSG_TEST_BLE_MAC_RESPONSE {
	S_MSG_TEST_BLE_RESPONSE response;
	byte MAC[6];
} S_MSG_TEST_BLE_MAC_RESPONSE;


typedef struct _S_MSG_SET_PRODUCT_INFO_REQUEST {	//��Ʒ��Ϣ�洢�ṹ
	 DevInfo_t info;
} S_MSG_SET_PRODUCT_INFO_REQUEST;

typedef S_MSG_TEST_BASE_RESPONSE S_MSG_SET_PRODUCT_INFO_RESPONSE;

typedef S_MSG_TEST_BASE_REQUEST S_MSG_GET_PRODUCT_INFO_REQUEST;

typedef struct _S_MSG_TEST_GET_PRODUCT_INFO_RESPONSE {	//��Ʒ��Ϣ�洢�ṹ
	byte RetCode;
	DevInfo_t info;
} S_MSG_TEST_GET_PRODUCT_INFO_RESPONSE;

#pragma pack()

#endif
