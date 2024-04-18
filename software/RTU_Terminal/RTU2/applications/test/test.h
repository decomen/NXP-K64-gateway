

#ifndef _TEST_H
#define _TEST_H

#define TEST_UART_MAP        UART1_RX_PC03_TX_PC04
#define TEST_UART_INSTANCE   HW_UART1


#define TEST_LED_GPIO     HW_GPIOA
#define TEST_LED_PIN      1

#define TEST_LED_ON()	  GPIO_SetBit(TEST_LED_GPIO, TEST_LED_PIN)
#define TEST_LED_OFF()    GPIO_ResetBit(TEST_LED_GPIO, TEST_LED_PIN)


#define TEST_QUEUE_SIZE (1* 512 + 1)

typedef struct {
    mdINT nFront; 	/* 头指针 */
    mdINT nRear;  	/* 尾指针，若队列不空，指向队列尾元素的下一个位置 */
    mdBYTE pData[TEST_QUEUE_SIZE];	/* 可以是其他基本类型 */
} queue_test_t;

typedef struct {
    mdINT nFront; 	/* 头指针 */
    mdINT nRear;  	/* 尾指针，若队列不空，指向队列尾元素的下一个位置 */
    mdBYTE pData[30];	/* 可以是其他基本类型 */
} queue_uart_t;

// 存放MYSIZEOF(S_PACKAGE_HEAD)个元素需要MYSIZEOF(S_PACKAGE_HEAD) + 1 的空间
#define QUEUE_HEADE_SIZE (sizeof(S_PACKAGE_HEAD) + 1)

typedef struct {
    mdINT nFront;       /* 头指针 */
    mdINT  nRear;        /* 尾指针，若队列不空，指向队列尾元素的下一个位置 */
    mdBYTE pData[QUEUE_HEADE_SIZE];
} queue_head_t;

extern queue_test_t *g_com_queue; 
extern queue_head_t g_queue_com_head ;
void USART3_IRQHandler(void) ;  

void set_product_info(const S_MSG *msg, byte_buffer_t *bb);

void vTestLedToggle();
void vTestLedInit(void);
void vTestRtc(const S_MSG *msg, byte_buffer_t *bb);
void vTestSpiFlash(const S_MSG *msg, byte_buffer_t *bb);
void vTestSdCard(const S_MSG *msg, byte_buffer_t *bb);
void vTestVs1003(const S_MSG *msg, byte_buffer_t *bb);
rt_err_t xTestTaskStart( void );
extern mdBOOL g_bIsTestMode;

#endif 
