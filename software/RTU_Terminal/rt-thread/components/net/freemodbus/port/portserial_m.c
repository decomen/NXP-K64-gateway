/*
 * FreeModbus Libary: RT-Thread Port
 * Copyright (C) 2013 Armink <armink.ztl@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * File: $Id: portserial_m.c,v 1.60 2013/08/13 15:07:05 Armink add Master Functions $
 */

#include "port.h"

/* ----------------------- Modbus includes ----------------------------------*/
#include "mb.h"
#include "mbport.h"
#include "rtdevice.h"
#include "board.h"

#if MB_MASTER_RTU_ENABLED > 0 || MB_MASTER_ASCII_ENABLED > 0
/* ----------------------- Defines ------------------------------------------*/
/* serial transmit event */
#define EVENT_TRANS_START(n)            (1<<(n))
#define EVENT_TRANS_START_ANY           (~(0xFFFFFFFFUL << MB_MASTER_NUMS))

#define MB_THREAD_TRAN_NAME             "mbm_tr"                             
#define MB_EVENT_NAME                   "mbm_tre"

/* software simulation serial transmit IRQ handler thread */
static rt_thread_t thread_serial_trans_irq = RT_NULL;
/* serial event */
static rt_event_t event_serial = RT_NULL;

/* ----------------------- static functions ---------------------------------*/
static void prvvUARTTxReadyISR( UCHAR ucPort );
static void prvvUARTRxISR( UCHAR ucPort );
static rt_err_t serial_rx_ind(rt_device_t dev, rt_size_t size);
static void serial_trans_irq(void* parameter);

/* ----------------------- Start implementation -----------------------------*/
BOOL xMBMasterPortSerialInit(UCHAR ucPort, ULONG ulBaudRate, UCHAR ucDataBits,
        eMBParity eParity)
{
    struct k64_serial_device *node = ((struct k64_serial_device *)g_serials[ucPort]->parent.user_data);

    if( node->cfg->uart_type == UART_TYPE_485 ) {
        rt_pin_mode( node->pin_en, PIN_MODE_OUTPUT );
        rt_pin_write( node->pin_en, PIN_HIGH );
    }
    
    /* set serial configure parameter */
    g_serials[ucPort]->config.baud_rate = ulBaudRate;
    g_serials[ucPort]->config.stop_bits = STOP_BITS_1;
    
    switch(eParity) {
    case MB_PAR_NONE: {
        g_serials[ucPort]->config.data_bits = DATA_BITS_8;
        g_serials[ucPort]->config.parity = PARITY_NONE;
        break;
    }
    case MB_PAR_ODD: {
        g_serials[ucPort]->config.data_bits = DATA_BITS_9;
        g_serials[ucPort]->config.parity = PARITY_ODD;
        break;
    }
    case MB_PAR_EVEN: {
        g_serials[ucPort]->config.data_bits = DATA_BITS_9;
        g_serials[ucPort]->config.parity = PARITY_EVEN;
        break;
    }
    }
    /* set serial configure */
    g_serials[ucPort]->ops->configure(g_serials[ucPort], &(g_serials[ucPort]->config));

    /* open serial device */
    if (!g_serials[ucPort]->parent.open(&g_serials[ucPort]->parent,
            RT_DEVICE_OFLAG_RDWR | RT_DEVICE_FLAG_INT_RX )) {
        g_serials[ucPort]->parent.rx_indicate = serial_rx_ind;
    } else {
        return FALSE;
    }

    /* software initialize */
    if( RT_NULL == event_serial ) {
        event_serial = rt_event_create( MB_EVENT_NAME, RT_IPC_FLAG_PRIO );
    }

    if( RT_NULL == thread_serial_trans_irq ) {
        thread_serial_trans_irq = rt_thread_create( MB_THREAD_TRAN_NAME, 
                                                    serial_trans_irq, 
                                                    RT_NULL, 
                                                    512, 
                                                    10, 10 );
        if( thread_serial_trans_irq != RT_NULL ) {
            rt_thddog_register(thread_serial_trans_irq, 30);
            rt_thread_startup( thread_serial_trans_irq );
        }
    }

    return TRUE;
}

void vMBMasterPortSerialEnable(UCHAR ucPort, BOOL xRxEnable, BOOL xTxEnable)
{
    struct k64_serial_device *node = ((struct k64_serial_device *)g_serials[ucPort]->parent.user_data);
    rt_uint32_t recved_event;
    
    if (xRxEnable)
    {
        /* enable RX interrupt */
        g_serials[ucPort]->ops->control(g_serials[ucPort], RT_DEVICE_CTRL_SET_INT, (void *)RT_DEVICE_FLAG_INT_RX);
        if( node->cfg->uart_type == UART_TYPE_485 ) {
            /* switch 485 to receive mode */
            rt_thread_delay( rt_uart_485_getrx_delay(node->cfg->port_cfg.baud_rate) *  RT_TICK_PER_SECOND / 1000 );
            rt_pin_write( node->pin_en, PIN_HIGH );
        }
    }
    else
    {
        if( node->cfg->uart_type == UART_TYPE_485 ) {
            /* switch 485 to transmit mode */
            rt_pin_write( node->pin_en, PIN_LOW );
            rt_thread_delay( rt_uart_485_gettx_delay(node->cfg->port_cfg.baud_rate) * RT_TICK_PER_SECOND / 1000 );
        }
        /* disable RX interrupt */
        g_serials[ucPort]->ops->control(g_serials[ucPort], RT_DEVICE_CTRL_CLR_INT, (void *)RT_DEVICE_FLAG_INT_RX);
    }
    if (xTxEnable)
    {
        /* start g_serials transmit */
        if( event_serial != RT_NULL ) {
            rt_event_send( event_serial, EVENT_TRANS_START(ucPort) );
        }
    }
    else
    {
        /* stop g_serials transmit */
        if( event_serial != RT_NULL ) {
            rt_event_recv( event_serial, EVENT_TRANS_START(ucPort),
                        RT_EVENT_FLAG_OR | RT_EVENT_FLAG_CLEAR, 0,
                        &recved_event );
        }
    }
}

void vMBMasterPortClose(UCHAR ucPort)
{
    g_serials[ucPort]->parent.close( &(g_serials[ucPort]->parent) );
}

BOOL xMBMasterPortSerialPutByte( UCHAR ucPort, CHAR ucByte )
{
    g_serials[ucPort]->parent.write( &(g_serials[ucPort]->parent), 0, &ucByte, 1 );
    return TRUE;
}

BOOL xMBMasterPortSerialGetByte(UCHAR ucPort, CHAR * pucByte)
{
    g_serials[ucPort]->parent.read(&(g_serials[ucPort]->parent), 0, pucByte, 1);
    return TRUE;
}

/* 
 * Create an interrupt handler for the transmit buffer empty interrupt
 * (or an equivalent) for your target processor. This function should then
 * call pxMBFrameCBTransmitterEmpty( ) which tells the protocol stack that
 * a new character can be sent. The protocol stack will then call 
 * xMBPortSerialPutByte( ) to send the character.
 */
void prvvUARTTxReadyISR(UCHAR ucPort)
{
    pxMBMasterFrameCBTransmitterEmpty[ucPort]( ucPort );
}

/* 
 * Create an interrupt handler for the receive interrupt for your target
 * processor. This function should then call pxMBFrameCBByteReceived( ). The
 * protocol stack will then call xMBPortSerialGetByte( ) to retrieve the
 * character.
 */
void prvvUARTRxISR( UCHAR ucPort )
{
    pxMBMasterFrameCBByteReceived[ucPort]( ucPort );
}

/**
 * Software simulation serial transmit IRQ handler.
 *
 * @param parameter parameter
 */
static void serial_trans_irq(void* parameter) {
    rt_uint32_t recved_event = 0;
    while (1)
    {
        rt_thddog_feed("");
        /* waiting for g_serials transmit start */
        if( event_serial != RT_NULL ) {
            rt_thddog_suspend("rt_event_recv");
            rt_event_recv( event_serial, EVENT_TRANS_START_ANY, RT_EVENT_FLAG_OR,
                    RT_WAITING_FOREVER, &recved_event);
            rt_thddog_resume();
        }
        /* execute modbus callback */
        for( UCHAR ucPort = 0; ucPort < MB_MASTER_NUMS; ucPort++ ) {
            if( recved_event & EVENT_TRANS_START(ucPort) ) {
                prvvUARTTxReadyISR( ucPort );
            }
        }
    }
    rt_thddog_exit();
}

/**
 * This function is g_serials receive callback function
 *
 * @param dev the device of g_serials
 * @param size the data size that receive
 *
 * @return return RT_EOK
 */
static rt_err_t serial_rx_ind(rt_device_t dev, rt_size_t size)
{
    struct k64_serial_device *node = ((struct k64_serial_device *)((rt_serial_t *)dev)->parent.user_data);
    for( int n = 0; n < size; n++ ) {
        prvvUARTRxISR( node->instance );
    }
    return RT_EOK;
}

#endif
