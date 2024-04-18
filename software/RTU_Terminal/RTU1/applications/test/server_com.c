

#include "mdtypedef.h"
#include "MK64F12.h"
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
#include "user_mb_app.h"
#include "dev_cfg.h"
#include "host_cfg.h"

static volatile ushort com_sequence = 0;

#define SERVER_BUF_SIZE_COM (200)

//ȫ��buffer������ͨ����
static unsigned char com_server_buf[SERVER_BUF_SIZE_COM];

// ����ƥ��֡ͷ(��Ч��ֹ�������µĶ�֡���ݶ�ʧ)
S_PACKAGE_HEAD *try2match_com_head(u8 ch)
{
	static S_PACKAGE_HEAD head;
	bQueueWrite(g_queue_com_head, ch);

	// 1.�ж϶�����
	if (bQueueFull(g_queue_com_head) == STATUS_OK) {
		// 2. �ж�ǰ���Ͱ汾
		if (xQueueGet(g_queue_com_head, 0) != PRECODE_0 ||
				xQueueGet(g_queue_com_head, 1) != PRECODE_1 ||
				xQueueGet(g_queue_com_head, 4) != SDCCP_VER) {
			// �ڳ�һ���ֽڿռ�
			bQueueRemove(g_queue_com_head);
		} else {
			// 3. У��
			int i;

			for (i = 0; i < sizeof(S_PACKAGE_HEAD); i++) {
				((u8 *)&head)[i] = xQueueGet(g_queue_com_head, i);
			}

			if (btMDCrc8(0, &head, sizeof(S_PACKAGE_HEAD) - 1) == head.CRC8) {
				//�յ���ȷ֡ͷ
				//���֡ͷ����
				vQueueClear(g_queue_com_head);
				// ����֡ͷ
				return &head;
			} else {
				// �Ƴ���ǰ��
				bQueueRemove(g_queue_com_head);
				bQueueRemove(g_queue_com_head);
			}
		}
	}

	return NULL;
}


static int send_block(void *buf, u32 len)
{
	u8 *data = (u8 *)buf;
	u32 i = 0 ;

	/*
	for (i = 0 ; i < len ; i++){
		UART_WriteByte(TEST_UART_INSTANCE, data[i]);
	}*/

	vUartSend(TEST_UART_INSTANCE, buf, len);

	return SDCCP_OK;
}

void send_buf2com(void *buf, u32 len)
{
	send_block(buf, len);
}

static void creat_package_buffer(byte_buffer_t *bb, S_PACKAGE *p_package)
{
	bb_new_from_buf(bb, com_server_buf, SERVER_BUF_SIZE_COM);
	bb_skip(bb, sizeof(S_PACKAGE_HEAD));
	bb_put(bb, p_package->Msg.Type);
	bb_put_short(bb, p_package->Msg.Senquence);
}

static void send_package_buffer(byte_buffer_t *bb, S_PACKAGE *p_package)
{
	u32 crc_32 = 0;
	CREAT_HEADER(p_package->Head, bb_get_pos(bb) + 4, p_package->Head.Type, 0);
	bb_put_bytes_at(bb, (u8 *)&p_package->Head, sizeof(S_PACKAGE_HEAD), 0);

	crc_32 = ulMDCrc32(0, bb_buffer(bb), bb_get_pos(bb));
	bb_put_int(bb, crc_32);

	send_block(bb_buffer(bb), bb_get_pos(bb));
}

// ͬ����ȡ����, �����ó�ʱʱ��
// ע: ����ƥ��֡ͷ, ����⵽����֡ͷʱ�������¿�ʼ��ȡһ���µ�֡
static int recv_block(void *buf, u32 len, u32 timeout)
{
	u32 index = 0 ;
	u32 begin = rt_tick_get();
	u8 *data = (u8 *)buf;

	for (index = 0; index < len; index++) {
		while (bQueueRead((*g_com_queue), data[index]) != STATUS_OK) {
			// ��ʱ��������
			if (SDCCP_CHECK_TIME_OUT(begin, timeout)) {
				return SDCCP_TIMEOUT;
			}
		}

		// ��������һ����ȷ֡ͷ, ˵��֮ǰ���������ݶ�ʧ���, ��Ҫ���»�ȡһ���µİ�
		//if( try2match_com_head(data[index]) != NULL) return SDCCP_RETRY;
		begin = rt_tick_get();
	}

	return SDCCP_OK;
}

void send_base_request(const S_MSG *msg, S_MSG_TEST_BASE_REQUEST *request)
{
	S_PACKAGE package;
	byte_buffer_t bb;

	package.Head.Type = FA_REQUEST;
	package.Msg.Type = msg->Type;
	package.Msg.Senquence = msg->Senquence;
	creat_package_buffer(&bb, &package);

	bb_put(&bb, request->Type);

	send_package_buffer(&bb, &package);
}

// send_ack �����ڴ���bufferǰ����, ���ʹ�þֲ�buffer, �����ͻ
/*static void send_ack(const S_MSG *msg, byte ack_value)
{
	S_MSG_ACK ack;
	S_PACKAGE package;
	byte_buffer_t bb;
	unsigned char ack_buf[20] = {0};

	bb_new_from_buf(&bb, ack_buf, MYSIZEOF(ack_buf));

	package.Head.Type = FA_ACK;
	package.Msg.Type = msg->Type;
	package.Msg.Senquence = msg->Senquence;

	bb_skip(&bb, MYSIZEOF(S_PACKAGE_HEAD));
	bb_put(&bb, package.Msg.Type);
	bb_put_short(&bb, package.Msg.Senquence);

	ack.AckValue = ack_value;
	bb_put(&bb, ack.AckValue);

	send_package_buffer(&bb, &package);
}*/

void send_test_online_status(byte online_status)
{
	S_PACKAGE package;
	byte_buffer_t bb;

	S_MSG_TEST_ONLINE_STATUS_REPORT report = {.OnlineStatus = online_status};

	package.Head.Type = FA_POST;
	package.Msg.Type = MSG_TEST_ONLINE_STATUS;
	package.Msg.Senquence = 0;
	creat_package_buffer(&bb, &package);

	bb_put(&bb, report.OnlineStatus);

	send_package_buffer(&bb, &package);
}

void send_base_response(const S_MSG *msg, S_MSG_TEST_BASE_RESPONSE *response)
{
	S_PACKAGE package;
	byte_buffer_t bb;

	package.Head.Type = FA_RESPONSE;
	package.Msg.Type = msg->Type;
	package.Msg.Senquence = msg->Senquence;
	creat_package_buffer(&bb, &package);

	bb_put(&bb, response->Type);
	bb_put(&bb, response->RetCode);
	bb_put_int(&bb, response->Value);

	send_package_buffer(&bb, &package);
}

void send_ble_mac_response(const S_MSG *msg, S_MSG_TEST_BLE_MAC_RESPONSE *mac_response)
{
	S_PACKAGE package;
	byte_buffer_t bb;

	package.Head.Type = FA_RESPONSE;
	package.Msg.Type = msg->Type;
	package.Msg.Senquence = msg->Senquence;
	creat_package_buffer(&bb, &package);

	bb_put(&bb, mac_response->response.Type);
	bb_put(&bb, mac_response->response.RetCode);
	bb_put_int(&bb, mac_response->response.Value);
	bb_put_bytes(&bb, mac_response->MAC, 6);

	send_package_buffer(&bb, &package);
}


static void send_device_product_info(const S_MSG * msg)
{
	S_PACKAGE package;
	S_MSG_TEST_GET_PRODUCT_INFO_RESPONSE response;
	memset(&response, 0 , sizeof(response));
	byte_buffer_t bb;

	package.Head.Type = FA_RESPONSE;
	package.Msg.Type = MSG_TEST_GET_PRODUCT_INFO;
	package.Msg.Senquence = msg->Senquence;
	creat_package_buffer(&bb, &package);

	response.RetCode = MSG_TEST_ERROR_OK;

	vDevCfgInit();  //������
	response.info = g_xDevInfoReg.xDevInfo;
	bb_put(&bb, response.RetCode);
	bb_put_bytes(&bb , response.info.xSN.szSN , sizeof(response.info.xSN));
	bb_put_short(&bb , response.info.usHWVer);
	bb_put_short(&bb , response.info.usSWVer);
	bb_put_bytes(&bb , response.info.xOEM.szOEM , sizeof(response.info.xOEM));
	bb_put_bytes(&bb , response.info.xIp.szIp , sizeof(response.info.xIp));
	bb_put_bytes(&bb , response.info.xNetMac.mac , sizeof(response.info.xNetMac));
	bb_put_short(&bb , response.info.usYear);
	bb_put_short(&bb , response.info.usMonth);
	bb_put_short(&bb , response.info.usDay);
	bb_put_short(&bb , response.info.usHour);
	bb_put_short(&bb , response.info.usMin);
	bb_put_short(&bb , response.info.usSec);
	bb_put_bytes(&bb , response.info.hwid.hwid, sizeof(response.info.hwid));
	bb_put_bytes(&bb , response.info.xPN.szPN, sizeof(response.info.xPN));

	send_package_buffer(&bb, &package);
}


void send_adc_get_response(const S_MSG *msg, S_MSG_TEST_GET_ADC_RESPONSE *adc_response)
{
	S_PACKAGE package;
	byte_buffer_t bb;

	package.Head.Type = FA_RESPONSE;
	package.Msg.Type = msg->Type;
	package.Msg.Senquence = msg->Senquence;
	creat_package_buffer(&bb, &package);

	bb_put(&bb, adc_response->RetCode);
	bb_put_int(&bb, adc_response->xAdcInfo.usChannel);
	bb_put_int(&bb, adc_response->xAdcInfo.usRange);
	bb_put_int(&bb, adc_response->xAdcInfo.usEngVal);
	bb_put_int(&bb, adc_response->xAdcInfo.usMeasureVal);

	send_package_buffer(&bb, &package);
}


static i32 process_ack(const S_MSG *msg, byte_buffer_t *bb)
{
	return SDCCP_OK;
}

//Lite ���յ���POST��Ϣ���� MSG_NOTICE MSG_UPDATE
static i32 process_post(const S_MSG *msg, byte_buffer_t *bb)		//APP �������͹���������, �������ȷ���Ƿ���Ҫ�ظ�ACK
{
	return SDCCP_OK;
}

//Lite ���յ��������� MSG_HISTORY_DATA
static i32 process_request(const S_MSG *msg, byte_buffer_t *bb)	//APP ��������,��ظ�
{
	switch (msg->Type) {
	case MSG_TEST_GET_PRODUCT_INFO: {
		send_device_product_info(msg);
		break;
	}

    case MSG_TEST_SDCARD:{
		vTestSdCard(msg, bb);
		break;
	}

	case MSG_TEST_GPRS:{
		vTestGprs(msg, bb);
		break;
	}

	case MSG_TEST_ZIGBEE:{
		vTestZigbee(msg, bb);
		break;
	}
	
	case MSG_TEST_RTC: {
		vTestRtc(msg, bb);
		break;
	}

	case MSG_TEST_SPIFLASH: {
		vTestSpiFlash(msg, bb);
		break;
	}

	case MSG_TEST_NET: {
		vTestNet(msg, bb);
		break;
	}

	case MSG_TEST_TTLInput:{
		vTestTTLInput(msg, bb);
		break;
	}

	case MSG_TEST_ON_OFF:{
		vTestOnOffInput(msg, bb);
		break;
	}

	case MSG_TEST_ADC:{
		vTestGetAdc(msg, bb);
		break;
	}

	case MSG_TEST_GET_TEST_ADC:{  //��ȡУ������ADCֵ
		vTestGetTestAdcValue(msg, bb);
		break;
	}

	case MSG_TEST_VOL:{
		vTestVol(msg, bb);
		break;
	}

	case MSG_TEST_UART:{
		vTestUart(msg, bb);
		break;
	}

	case MSG_TEST_SetCheck:{
		vTestSetCheckDefault(msg, bb);
		break;
	}
	case MSG_TEST_TTL_OUTPUT_RELAY:{
		vTestRelays(msg, bb);
		break;
	}

	/*
	case MSG_TEST_TTL_OUTPUT_RELAY:{  //ͨ�����Եװ���в���
		vTestTTLOutputRelay(msg, bb);
		break;
	}*/

	case MSG_TEST_SET_PRODUCT_INFO: {
		set_product_info(msg, bb);
		break;
	}

	case MSG_TEST_SET_ADC_INFO:{
		vTestSetAdc(msg, bb);
		break;
	}

	case MSG_TEST_MKFS:{
		vTestEraseFlashErase(msg,bb);   //��������ʹ��
		break;
	}
	
	}

	return SDCCP_OK;
}

//Lite ���޴�����
static i32 process_response(const S_MSG *msg, byte_buffer_t *bb)	//APP ��request�Ļظ�
{
	return SDCCP_OK;
}
static i32 process_package(S_PACKAGE_HEAD *h)
{
	S_MSG msg;
	i32 ret = SDCCP_ERR;
	u32 crc_32 = 0;
	byte_buffer_t bb;
	S_PACKAGE_HEAD head = *h;
	ushort size = ntohs(head.Length);

	if (size > SERVER_BUF_SIZE_COM) {
		return SDCCP_ERR;
	}

	bb_new_from_buf(&bb, com_server_buf, size);

	//bb_clear(&bb);

	bb_put_bytes(&bb, (u8 *)&head, sizeof(S_PACKAGE_HEAD));
	ret = recv_block(bb_point(&bb), bb_remain(&bb), 2000);

	if (ret != SDCCP_OK) {
		goto END_;
	}

	crc_32 = bb_get_int_at(&bb, bb_limit(&bb) - 4);

	if (ulMDCrc32(0, bb_buffer(&bb), bb_limit(&bb) - 4) != crc_32) {
		ret = SDCCP_ERR;
		goto END_;
	}

	//index = MYSIZEOF(S_PACKAGE_HEAD);
	msg.Type = bb_get(&bb);
	msg.Senquence = bb_get_short(&bb);
	msg.Content = bb_point(&bb);

	switch (head.Type) {
	case FA_ACK:
		ret = process_ack(&msg, &bb);
		break;

	case FA_POST:
		ret = process_post(&msg, &bb);
		break;

	case FA_REQUEST:
		ret = process_request(&msg, &bb);
		break;

	case FA_RESPONSE:
		ret = process_response(&msg, &bb);
		break;
	}

END_:
	return ret;
}

i32 client_event_handler_com(S_PACKAGE_HEAD *h)
{

	switch (h->Type) {
	case FA_ACK:
	case FA_POST:
	case FA_REQUEST:
	case FA_RESPONSE:
		
		return process_package(h);

	default:
		return SDCCP_ERR;
	}
}



