/*
 * File      : drv_uart.c
 * This file is part of RT-Thread RTOS
 * COPYRIGHT (C) 2013, RT-Thread Develop Team
 *
 * The license and distribution terms for this file may be
 * found in the file LICENSE in this distribution or at
 * http://openlab.rt-thread.com/license/LICENSE
 *
 * Change Logs:
 * Date           Author       Notes
 *
 */

#include "board.h"
#include "uart.h"

static const Reg_t ClkTbl[] =
{

    { (void *)&(SIM->SCGC4), SIM_SCGC4_UART0_MASK },
    { (void *)&(SIM->SCGC4), SIM_SCGC4_UART1_MASK },
    { (void *)&(SIM->SCGC4), SIM_SCGC4_UART2_MASK },
#ifdef UART3
    { (void *)&(SIM->SCGC4), SIM_SCGC4_UART3_MASK },
#endif
#ifdef UART4
    { (void *)&(SIM->SCGC1), SIM_SCGC1_UART4_MASK },
#endif
#ifdef UART5
    { (void *)&(SIM->SCGC1), SIM_SCGC1_UART5_MASK },
#endif
};

static struct rt_serial_device _k64_serials[BOARD_UART_MAX];
rt_serial_t *g_serials[BOARD_UART_MAX] = { &_k64_serials[0], &_k64_serials[1], &_k64_serials[2], &_k64_serials[3], &_k64_serials[4], &_k64_serials[5] };

//hardware abstract device
static const struct k64_serial_device _k64_serials_nodes[BOARD_UART_MAX] = {
    [HW_UART0] = { (UART_Type *)UART0_BASE, HW_UART0, BOARD_UART_0_MAP, UART0_RX_TX_IRQn, BOARD_UART_0_EN, &g_uart_cfgs[HW_UART0] },
    [HW_UART1] = { (UART_Type *)UART1_BASE, HW_UART1, BOARD_UART_1_MAP, UART1_RX_TX_IRQn, BOARD_UART_1_EN, &g_uart_cfgs[HW_UART1] },
[HW_UART2] = { (UART_Type *)UART2_BASE, HW_UART2, BOARD_UART_2_MAP, UART2_RX_TX_IRQn, BOARD_UART_2_EN, &g_uart_cfgs[HW_UART2] },
[HW_UART3] = { (UART_Type *)UART3_BASE, HW_UART3, BOARD_UART_3_MAP, UART3_RX_TX_IRQn, BOARD_UART_3_EN, &g_uart_cfgs[HW_UART3] },
[HW_UART4] = { (UART_Type *)UART4_BASE, HW_UART4, BOARD_UART_4_MAP, UART4_RX_TX_IRQn, BOARD_UART_4_EN, &g_uart_cfgs[HW_UART4] },
[HW_UART5] = { (UART_Type *)UART5_BASE, HW_UART5, BOARD_UART_5_MAP, UART5_RX_TX_IRQn, BOARD_UART_5_EN, &g_uart_cfgs[HW_UART5] },
};

static int _uart_int_cnt[BOARD_UART_MAX] = {0};
static int _uart_rx_int_cnt[BOARD_UART_MAX] = {0};
static int _uart_ide_int_cnt[BOARD_UART_MAX] = {0};
static int _uart_tx_int_cnt[BOARD_UART_MAX] = {0};
static int _uart_err_int_cnt[BOARD_UART_MAX] = {0};
static int _uart_int_s1[BOARD_UART_MAX] = {0};

static rt_tick_t _uart_int_last_tick[BOARD_UART_MAX] = {0};

static rt_err_t _configure(struct rt_serial_device *serial, struct serial_configure *cfg)
{
    uint16_t sbr;
    uint8_t brfa;
    UART_Type *uart_reg;
    map_t *pq;
    uint8_t instance;
    uint32_t clock;

    /* ref : drivers\system_MK60F12.c Line 64 ,BusClock = 60MHz
     * calculate baud_rate
     */
    uart_reg = ((struct k64_serial_device *)serial->parent.user_data)->baseAddress;
    instance = ((struct k64_serial_device *)serial->parent.user_data)->instance;
    pq = (map_t *)&(((struct k64_serial_device *)serial->parent.user_data)->io_map);

    switch ((unsigned int)uart_reg)
    {
        /*
         * if you're using other board
         * set clock and pin map for UARTx
         */
    case UART0_BASE:
    case UART1_BASE:
        clock = GetClock(kCoreClock);
        break;
    default:
        clock = GetClock(kBusClock);
        break;
    }

    IP_CLK_ENABLE(instance);

    uart_reg->C2 &= ~((UART_C2_TE_MASK)|(UART_C2_RE_MASK));
    
    /* baud rate generation */
    sbr = (uint16_t)((clock)/((cfg->baud_rate)*16));
    brfa = ((32*clock)/((cfg->baud_rate)*16)) - 32*sbr;
    
    /* config baudrate */
    uart_reg->BDH &= ~UART_BDH_SBR_MASK;
    uart_reg->BDL &= ~UART_BDL_SBR_MASK;
    uart_reg->C4 &= ~UART_C4_BRFA_MASK;
    
    uart_reg->BDH |= UART_BDH_SBR(sbr>>8); 
    uart_reg->BDL = UART_BDL_SBR(sbr); 
    uart_reg->C4 |= UART_C4_BRFA(brfa);

    if (cfg->parity == PARITY_NONE) {
        uart_reg->C1 &= ~UART_C1_PE_MASK;
        uart_reg->C1 &= ~UART_C1_M_MASK;
    } else {
        if (cfg->parity == PARITY_ODD) {
            uart_reg->C1 |= UART_C1_PE_MASK;
            uart_reg->C1 |= UART_C1_PT_MASK;
        } else if (cfg->parity == PARITY_EVEN) {
            uart_reg->C1 |= UART_C1_PE_MASK;
            uart_reg->C1 &= ~UART_C1_PT_MASK;
        }
    }

    /*
     * set data_bits
     */
    if (cfg->data_bits == DATA_BITS_8) {
        if(uart_reg->C1 & UART_C1_PE_MASK) {
            /* parity is enabled it's actually 9bit*/
            uart_reg->C1 |= UART_C1_M_MASK;
            uart_reg->C4 &= ~UART_C4_M10_MASK;    
        } else {
            uart_reg->C1 &= ~UART_C1_M_MASK;
            uart_reg->C4 &= ~UART_C4_M10_MASK;    
        }
    } else if (cfg->data_bits == DATA_BITS_9) {
        if(uart_reg->C1 & UART_C1_PE_MASK) {
            /* parity is enabled it's actually 10 bit*/
            uart_reg->C1 |= UART_C1_M_MASK;
            uart_reg->C4 |= UART_C4_M10_MASK;  
        } else {
            uart_reg->C1 |= UART_C1_M_MASK;
            uart_reg->C4 &= ~UART_C4_M10_MASK;      
        }
    }

    uart_reg->S2 &= ~UART_S2_MSBF_MASK; /* LSB */

    for(int i = 0; i < pq->pin_cnt; i++)
    {
        PORT_PinMuxConfig(pq->io, pq->pin_start + i, (PORT_PinMux_Type) pq->mux); 
    }
    
    /* enable Tx Rx */
    uart_reg->C2 |= ((UART_C2_TE_MASK)|(UART_C2_RE_MASK));

    /* default: disable hardware buffer */
    UART_EnableTxFIFO(pq->ip, false);
    UART_EnableRxFIFO(pq->ip, false);
    
    return RT_EOK;
}

static rt_err_t _control(struct rt_serial_device *serial, int cmd, void *arg)
{
    UART_Type *uart_reg;
    int uart_irq_num = 0;
    uint8_t instance;

    uart_reg = ((struct k64_serial_device *)serial->parent.user_data)->baseAddress;
    uart_irq_num = ((struct k64_serial_device *)serial->parent.user_data)->irq_num;
    instance = ((struct k64_serial_device *)serial->parent.user_data)->instance;

    switch (cmd)
    {
    case RT_DEVICE_CTRL_CLR_INT:
        /* disable rx irq */
		extern uint8_t g_bIsTestMode ;
		if (!g_bIsTestMode) {
	        uart_reg->C2 &= ~UART_C2_RIE_MASK;
	        uart_reg->C2 &= ~UART_C2_ILIE_MASK;
	        //disable NVIC
	        NVIC->ICER[uart_irq_num / 32] = 1 << (uart_irq_num % 32);
		}
        break;
    case RT_DEVICE_CTRL_SET_INT:
        /* enable rx irq */
        //if (HW_UART2 == instance) {
        uart_reg->C2 |= UART_C2_ILIE_MASK;  //开启所有串口的空闲中断
        //}
        uart_reg->C2 |= UART_C2_RIE_MASK;
        //enable NVIC,we are sure uart's NVIC vector is in NVICICPR1
        NVIC->ICPR[uart_irq_num / 32] = 1 << (uart_irq_num % 32);
        NVIC->ISER[uart_irq_num / 32] = 1 << (uart_irq_num % 32);
        break;
    case RT_DEVICE_CTRL_SUSPEND:
        /* suspend device */
        uart_reg->C2  &=  ~(UART_C2_RE_MASK |       //Receiver enable
                            UART_C2_ILIE_MASK);     //IDE enable
        break;
    case RT_DEVICE_CTRL_RESUME:
        /* resume device */
        uart_reg->C2  |= UART_C2_RE_MASK;           //Receiver enable
        if (HW_UART2 == instance) {
            uart_reg->C2  |= UART_C2_ILIE_MASK;     //IDE enable
        }
        break;
    }

    return RT_EOK;
}

static __INLINE rt_bool_t _check_ws_instance(rt_uint8_t instance)
{
    return (
            (instance == BOARD_ZIGBEE_UART && g_ws_cfg.port_type == WS_PORT_ZIGBEE) ||
            (instance == BOARD_GPRS_UART && g_ws_cfg.port_type == WS_PORT_GPRS) ||
            (g_ws_cfg.port_type >= WS_PORT_RS1 && g_ws_cfg.port_type <= WS_PORT_RS_MAX && nUartGetInstance(WS_PORT_TO_DEV(g_ws_cfg.port_type)) == instance)
            );
}

static int _putc(struct rt_serial_device *serial, char c)
{
    rt_uint8_t instance = ((struct k64_serial_device *)serial->parent.user_data)->instance;
    UART_Type *uart_reg = ((struct k64_serial_device *)serial->parent.user_data)->baseAddress;

    while (!(uart_reg->S1 & UART_S1_TDRE_MASK));
    uart_reg->D = (c & 0xFF);
    if (g_ws_cfg.enable && _check_ws_instance(instance)) {
        ws_vm_snd_putc((rt_uint8_t)c);
    }
    return 1;
}

static int _getc(struct rt_serial_device *serial)
{
    rt_uint8_t instance = ((struct k64_serial_device *)serial->parent.user_data)->instance;
    UART_Type *uart_reg = ((struct k64_serial_device *)serial->parent.user_data)->baseAddress;
    pvrecv_handle recv_handle = ((struct k64_serial_device *)serial->parent.user_data)->cfg->recv_handle;
    int ret = -1;
    rt_bool_t match = RT_FALSE;

    if (g_ws_cfg.enable && _check_ws_instance(instance)) {
        match = RT_TRUE;
        ret = ws_vm_getc();
    }

    if (ret < 0) {
        if (uart_reg->S1 & UART_S1_RDRF_MASK) {
            ret = uart_reg->D;
            if (match) {
                ws_vm_rcv_putc((rt_uint8_t)ret);
            }
        }
    }

    if (ret >= 0) {
        if (recv_handle) recv_handle(instance, ret & 0xFF);
    }

    return ret;
}

static const struct rt_uart_ops _k64_serials_ops =
{
    _configure,
    _control,
    _putc,
    _getc,
};

static void UARTn_IRQHandler(uint8_t port)
{
    UART_Type *uart_reg = ((struct k64_serial_device *)_k64_serials[port].parent.user_data)->baseAddress;
    pbbusy_handle busy_handle = ((struct k64_serial_device *)_k64_serials[port].parent.user_data)->cfg->busy_handle;
    pvrecv_handle recv_handle = ((struct k64_serial_device *)_k64_serials[port].parent.user_data)->cfg->recv_handle;

    _uart_int_last_tick[port] = rt_tick_get();

    rt_interrupt_enter();

    uint8_t stat = uart_reg->S1;
    volatile uint32_t dummy;
    
    _uart_int_cnt[port]++;
    _uart_int_s1[port] = stat;

    if ((stat & UART_S1_TDRE_MASK) && uart_reg->C2 & UART_C2_TIE_MASK) {
        _uart_tx_int_cnt[port]++;
        rt_hw_serial_isr((struct rt_serial_device *)&_k64_serials[port], RT_SERIAL_EVENT_TX_DONE);
    }

    if ((stat & UART_S1_IDLE_MASK) && uart_reg->C2 & UART_C2_ILIE_MASK) {
        _uart_ide_int_cnt[port]++;
        uart_reg->D;
        rt_hw_serial_isr((struct rt_serial_device *)&_k64_serials[port], RT_SERIAL_EVENT_IDE_IND);

    }

    if ((stat & UART_S1_RDRF_MASK) && uart_reg->C2 & UART_C2_RIE_MASK) {
        _uart_rx_int_cnt[port]++;
        if (busy_handle != RT_NULL && recv_handle != RT_NULL && busy_handle(port)) {
            _getc((struct rt_serial_device *)&_k64_serials[port]);
        } else {
            rt_hw_serial_isr((struct rt_serial_device *)&_k64_serials[port], RT_SERIAL_EVENT_RX_IND);
        }
    }

    if(stat & (UART_S1_OR_MASK | UART_S1_PF_MASK | UART_S1_FE_MASK | UART_S1_NF_MASK)) {
        _uart_err_int_cnt[port]++;
        dummy = uart_reg->D;
		uart_reg->S1 &= ~(UART_S1_OR_MASK | UART_S1_PF_MASK | UART_S1_FE_MASK | UART_S1_NF_MASK);
    }

    rt_interrupt_leave();
}

extern unsigned char g_bIsTestMode;
#if BOARD_UART_MAX > 0

void UART0_RX_TX_IRQHandler(void)
{
#ifdef USR_TEST_CODE
    if (g_bIsTestMode) {
        uint8_t ch = 0;
        uint8_t stat = UART0->S1;
        if ((stat & UART_S1_RDRF_MASK) && UART0->C2 & UART_C2_RIE_MASK) {
            ch = UART0->D;
            vTestUSART_IRQHandler(HW_UART0, ch);
        }
        if (stat & UART_S1_OR_MASK) {
            ch = UART0->D;
        }
        if (stat & UART_S1_IDLE_MASK) {
            ch = UART0->D;
        }
    } else
#endif
    {
        UARTn_IRQHandler(0);
    }
}
#endif

#if BOARD_UART_MAX > 1

void UART1_RX_TX_IRQHandler(void)
{
#ifdef USR_TEST_CODE
    if (g_bIsTestMode) {
        uint8_t ch = 0;
        uint8_t stat = UART1->S1;
        if ((stat & UART_S1_RDRF_MASK) && UART1->C2 & UART_C2_RIE_MASK) {
            ch = UART1->D;
            vTestUSART_IRQHandler(HW_UART1, ch);
        }
        if (stat & UART_S1_OR_MASK) {
            ch = UART1->D;
        }
        if (stat & UART_S1_IDLE_MASK) {
            ch = UART1->D;
        }
    } else
#endif
    {
        UARTn_IRQHandler(1);
    }
}
#endif

#if BOARD_UART_MAX > 2

void UART2_RX_TX_IRQHandler(void)
{
    UARTn_IRQHandler(2);
}
#endif

#if BOARD_UART_MAX > 3

void UART3_RX_TX_IRQHandler(void)
{
    UARTn_IRQHandler(3);
}
#endif

#if BOARD_UART_MAX > 4

void UART4_RX_TX_IRQHandler(void)
{
#ifdef USR_TEST_CODE
    if (g_bIsTestMode) {
        uint8_t ch = 0;
        uint8_t stat = UART4->S1;
        if ((stat & UART_S1_RDRF_MASK) && UART4->C2 & UART_C2_RIE_MASK) {
            ch = UART4->D;
            vTestUSART_IRQHandler(HW_UART4, ch);
        }
        if (stat & UART_S1_OR_MASK) {
            ch = UART4->D;
        }
        if (stat & UART_S1_IDLE_MASK) {
            ch = UART4->D;
        }
    } else
#endif
    {
        UARTn_IRQHandler(4);
    }
}
#endif

#if BOARD_UART_MAX > 5

void UART5_RX_TX_IRQHandler(void)
{
#ifdef USR_TEST_CODE
    if (g_bIsTestMode) {
        uint8_t ch = 0;
        uint8_t stat = UART5->S1;
        if ((stat & UART_S1_RDRF_MASK) && UART5->C2 & UART_C2_RIE_MASK) {
            ch = UART5->D;
            vTestUSART_IRQHandler(HW_UART5, ch);
        }
        if (stat & UART_S1_OR_MASK) {
            ch = UART5->D;
        }
        if (stat & UART_S1_IDLE_MASK) {
            ch = UART5->D;
        }
    } else
#endif
    {
        UARTn_IRQHandler(5);
    }
}
#endif

rt_err_t rt_hw_uart_init(uint8_t instance)
{
    struct serial_configure config;

    RT_ASSERT(instance < BOARD_UART_MAX);
    RT_ASSERT(_k64_serials_nodes[instance].cfg != RT_NULL);

    /* fake configuration */
    config.baud_rate = _k64_serials_nodes[instance].cfg->port_cfg.baud_rate;
    config.bit_order = BIT_ORDER_LSB;
    config.data_bits = _k64_serials_nodes[instance].cfg->port_cfg.data_bits;
    config.parity    = _k64_serials_nodes[instance].cfg->port_cfg.parity;
    config.stop_bits = _k64_serials_nodes[instance].cfg->port_cfg.stop_bits;
    config.invert    = NRZ_NORMAL;
    config.bufsz	 = RT_SERIAL_RB_BUFSZ;

    _k64_serials[instance].ops    = &_k64_serials_ops;
    _k64_serials[instance].config = config;

    return rt_hw_serial_register(&_k64_serials[instance], BOARD_UART_DEV_NAME(instance),
                                 RT_DEVICE_FLAG_RDWR | RT_DEVICE_FLAG_INT_RX | RT_DEVICE_FLAG_STREAM,
                                 (void *)&_k64_serials_nodes[instance]);
}

int rt_uart_485_getrx_delay(int baud)
{
    if( baud >= BAUD_RATE_115200 ) {
        return 2;
    } else if( baud >= BAUD_RATE_57600 ) {
        return 3;
    } else if( baud >= BAUD_RATE_38400 ) {
        return 4;
    } else if( baud >= BAUD_RATE_9600 ) {
        return 6;
    } else if( baud >= BAUD_RATE_4800 ) {
        return 8;
    } else if( baud >= BAUD_RATE_2400 ) {
        return 10;
    } else {
        return 20;
    }
}

int rt_uart_485_gettx_delay(int baud)
{
    if( baud >= BAUD_RATE_115200 ) {
        return 1;
    } else if( baud >= BAUD_RATE_57600 ) {
        return 2;
    } else if( baud >= BAUD_RATE_38400 ) {
        return 3;
    } else if( baud >= BAUD_RATE_9600 ) {
        return 4;
    } else if( baud >= BAUD_RATE_4800 ) {
        return 5;
    } else if( baud >= BAUD_RATE_2400 ) {
        return 6;
    } else {
        return 10;
    }
}

void uart_int_check(void)
{
    for(int port = 0; port < BOARD_UART_MAX; port++) {
        if(g_serials[port] && g_serials[port]->parent.open_flag) {
            if(rt_tick_get() - _uart_int_last_tick[port] >= rt_tick_from_millisecond(10000)) {
                UART_Type *uart_reg = ((struct k64_serial_device *)_k64_serials[port].parent.user_data)->baseAddress;
                uint8_t stat = uart_reg->S1;
                volatile uint32_t dummy;

                if(stat & (UART_S1_OR_MASK | UART_S1_PF_MASK | UART_S1_FE_MASK | UART_S1_NF_MASK)) {
                    _uart_err_int_cnt[port]++;
                    volatile uint32_t dummy = uart_reg->D;
            		uart_reg->S1 &= ~(UART_S1_OR_MASK | UART_S1_PF_MASK | UART_S1_FE_MASK | UART_S1_NF_MASK);
                }
                _uart_int_last_tick[port] = rt_tick_get();
            }
        }
    }
}


/*void rt_hw_console_output(const char *str)
{
    while(*str != '\0')
    {
        if (*str == '\n')
            _putc(&_k64_serials[BOARD_CONSOLE_USART],'\r');
        _putc(&_k64_serials[BOARD_CONSOLE_USART],*str);
        str++;
    }
}*/

INIT_BOARD_EXPORT(rt_hw_uart_init);

void read_mem(rt_uint32_t addr, int len)
{
    while(len--) {
        char s[33];
        unsigned long reg = 0;
        memcpy(&reg, (void *)addr, 4);
        for(int i = 0; i < 32; i++) {
            if(reg<<i & 0x80000000) {
                s[i] = '1';
            } else {
                s[i] = '0';
            }
        }
        s[32] = '\0';
        rt_kprintf("addr[0x%08x]\n", addr);
        rt_kprintf("HEX: 0x%08X\n", reg);
        rt_kprintf("BIN: [32->24] [24->16] [16->8]  [8->0] \n");
        rt_kprintf("     %.*s %.*s %.*s %.*s\n", 8, &s[0], 8, &s[8], 8, &s[16], 8, &s[24]);
        addr++;
    }
}

void read_uart(int index)
{
    UART_Type *uart[] = { 
        (UART_Type *)UART0_BASE, 
        (UART_Type *)UART1_BASE, 
        (UART_Type *)UART2_BASE, 
        (UART_Type *)UART3_BASE, 
        (UART_Type *)UART4_BASE, 
        (UART_Type *)UART5_BASE 
    };
    if(index < 6) {
        rt_kprintf("BDH = 0x%02X\n", uart[index]->BDH);
        rt_kprintf("BDL = 0x%02X\n", uart[index]->BDL);
        rt_kprintf("C1  = 0x%02X\n", uart[index]->C1);
        rt_kprintf("C2  = 0x%02X\n", uart[index]->C2);
        rt_kprintf("S1  = 0x%02X\n", uart[index]->S1);
        rt_kprintf("S2  = 0x%02X\n", uart[index]->S2);
        rt_kprintf("C3  = 0x%02X\n", uart[index]->C3);
        rt_kprintf("D   = 0x%02X\n", uart[index]->D);
        rt_kprintf("MA1 = 0x%02X\n", uart[index]->MA1);
        rt_kprintf("MA2 = 0x%02X\n", uart[index]->MA2);
        rt_kprintf("C4  = 0x%02X\n", uart[index]->C4);
        rt_kprintf("C5  = 0x%02X\n", uart[index]->C5);
        rt_kprintf("ED  = 0x%02X\n", uart[index]->ED);
        rt_kprintf("MOD = 0x%02X\n", uart[index]->MODEM);
        rt_kprintf("IR  = 0x%02X\n", uart[index]->IR);
    }
}

void write_uart(int index, int ofs, int val)
{
    UART_Type *uart[] = { 
        (UART_Type *)UART0_BASE, 
        (UART_Type *)UART1_BASE, 
        (UART_Type *)UART2_BASE, 
        (UART_Type *)UART3_BASE, 
        (UART_Type *)UART4_BASE, 
        (UART_Type *)UART5_BASE 
    };
    if(index < 6) {
        if(ofs == 0) uart[index]->BDH = val;
        else if(ofs == 1) uart[index]->BDL = val;
        else if(ofs == 2) uart[index]->C1 = val;
        else if(ofs == 3) uart[index]->C2 = val;
        else if(ofs == 4) uart[index]->S1 = val;
        else if(ofs == 5) uart[index]->S2 = val;
        else if(ofs == 6) uart[index]->C3 = val;
        else if(ofs == 7) uart[index]->D = val;
        else if(ofs == 8) uart[index]->MA1 = val;
        else if(ofs == 9) uart[index]->MA2 = val;
        else if(ofs == 10) uart[index]->C4 = val;
        else if(ofs == 11) uart[index]->C5 = val;
        else if(ofs == 13) uart[index]->MODEM = val;
        else if(ofs == 14) uart[index]->IR = val;
        if(ofs == 0) rt_kprintf("BDH = 0x%02X\n", uart[index]->BDH);
        else if(ofs == 1) rt_kprintf("BDL = 0x%02X\n", uart[index]->BDL);
        else if(ofs == 2) rt_kprintf("C1  = 0x%02X\n", uart[index]->C1);
        else if(ofs == 3) rt_kprintf("C2  = 0x%02X\n", uart[index]->C2);
        else if(ofs == 4) rt_kprintf("S1  = 0x%02X\n", uart[index]->S1);
        else if(ofs == 5) rt_kprintf("S2  = 0x%02X\n", uart[index]->S2);
        else if(ofs == 6) rt_kprintf("C3  = 0x%02X\n", uart[index]->C3);
        else if(ofs == 7) rt_kprintf("D   = 0x%02X\n", uart[index]->D);
        else if(ofs == 8) rt_kprintf("MA1 = 0x%02X\n", uart[index]->MA1);
        else if(ofs == 9) rt_kprintf("MA2 = 0x%02X\n", uart[index]->MA2);
        else if(ofs == 10)rt_kprintf("C4  = 0x%02X\n", uart[index]->C4);
        else if(ofs == 11)rt_kprintf("C5  = 0x%02X\n", uart[index]->C5);
        else if(ofs == 13)rt_kprintf("MOD = 0x%02X\n", uart[index]->MODEM);
        else if(ofs == 14)rt_kprintf("IR  = 0x%02X\n", uart[index]->IR);
    }
}

void list_uart_info(void)
{
    rt_kprintf("-----------------------\n");
    for(int i = 0; i < BOARD_UART_MAX; i++) {
        rt_kprintf("[%d]->int:ALL[%010u],RX[%010u],TX[%010u],IDE[%010u],ERR[%010u], s1:0x%02X\n", 
            i, _uart_int_cnt[i], _uart_rx_int_cnt[i], _uart_tx_int_cnt[i], _uart_ide_int_cnt[i], _uart_err_int_cnt[i], 
            _uart_int_s1[i]
            );
    }
    rt_kprintf("-----------------------\n");
}

FINSH_FUNCTION_EXPORT(read_mem, read_mem addr)
FINSH_FUNCTION_EXPORT(read_uart, read_uart index)
FINSH_FUNCTION_EXPORT(write_uart, write_uart index ofs val)
FINSH_FUNCTION_EXPORT(list_uart_info, list_uart_info)

