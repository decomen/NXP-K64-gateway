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
 * File: $Id: portserial.c,v 1.60 2013/08/13 15:07:05 Armink $
 */

#include "port.h"

/* ----------------------- Modbus includes ----------------------------------*/
#include "mb.h"
#include "mbport.h"
#include "rtdevice.h"
#include "board.h"

/* ----------------------- Static variables ---------------------------------*/
ALIGN(RT_ALIGN_SIZE)

/* ----------------------- Defines ------------------------------------------*/
/* serial transmit event */
#define EVENT_SERIAL_TRANS_START    (1<<0)

#define MB_THREAD_NAME(n)           ((n) == 0 ? "mbs_r0" : \
                                    ((n) == 1 ? "mbs_r1" : \
                                    ((n) == 2 ? "mbs_r2" : \
                                    ((n) == 3 ? "mbs_r3" : \
                                    ((n) == 4 ? "mbs_r4" : \
                                    ((n) == 5 ? "mbs_r5" : "" ))))))
                                    
#define MB_EVENT_NAME(n)            ((n) == 0 ? "mbs_ue0" : \
                                    ((n) == 1 ? "mbs_ue1" : \
                                    ((n) == 2 ? "mbs_ue2" : \
                                    ((n) == 3 ? "mbs_ue3" : \
                                    ((n) == 4 ? "mbs_ue4" : \
                                    ((n) == 5 ? "mbs_ue5" : "" ))))))

/* software simulation serial transmit IRQ handler thread stack */
static rt_uint8_t serial_soft_trans_irq_stack[MB_SLAVE_NUMS][256];
/* software simulation serial transmit IRQ handler thread */
static struct rt_thread thread_serial_soft_trans_irq[MB_SLAVE_NUMS];
/* serial event */
static struct rt_event event_serial[MB_SLAVE_NUMS];
/* modbus slave serial device */
static rt_serial_t *serials[MB_SLAVE_NUMS];

/* ----------------------- static functions ---------------------------------*/
static void prvvUARTTxReadyISR( UCHAR ucPort );
static void prvvUARTRxISR( UCHAR ucPort );
static rt_err_t serial_rx_ind(rt_device_t dev, rt_size_t size);
static void serial_soft_trans_irq(void* parameter);

/* ----------------------- Start implementation -----------------------------*/
BOOL xMBPortSerialInit( UCHAR ucPort, ULONG ulBaudRate, UCHAR ucDataBits,
        eMBParity eParity)
{
    /**
     * set 485 mode receive and transmit control IO
     * @note MODBUS_SLAVE_RT_CONTROL_PIN_INDEX need be defined by user
     */
    serials[ucPort] = (rt_serial_t *)rt_device_find( BOARD_UART_DEV_NAME(ucPort) );
    struct k64_serial_device *node = ((struct k64_serial_device *)serials[ucPort]->parent.user_data);

    if( node->cfg->type == UART_TYPE_485 ) {
        rt_pin_mode( node->pin_en, PIN_MODE_OUTPUT );
        rt_pin_write( node->pin_en, PIN_HIGH );
    }
    
    /* set serial configure parameter */
    serials[ucPort]->config.baud_rate = ulBaudRate;
    serials[ucPort]->config.stop_bits = STOP_BITS_1;
    
    switch(eParity) {
    case MB_PAR_NONE: {
        serials[ucPort]->config.data_bits = DATA_BITS_8;
        serials[ucPort]->config.parity = PARITY_NONE;
        break;
    }
    case MB_PAR_ODD: {
        serials[ucPort]->config.data_bits = DATA_BITS_9;
        serials[ucPort]->config.parity = PARITY_ODD;
        break;
    }
    case MB_PAR_EVEN: {
        serials[ucPort]->config.data_bits = DATA_BITS_9;
        serials[ucPort]->config.parity = PARITY_EVEN;
        break;
    }
    }
    /* set serial configure */
    serials[ucPort]->ops->configure(serials[ucPort], &(serials[ucPort]->config));

    /* open serial device */
    if (!serials[ucPort]->parent.open(&serials[ucPort]->parent,
            RT_DEVICE_OFLAG_RDWR | RT_DEVICE_FLAG_INT_RX )) {
        serials[ucPort]->parent.rx_indicate = serial_rx_ind;
    } else {
        return FALSE;
    }

    /* software initialize */
    rt_event_init( &event_serial[ucPort], MB_EVENT_NAME(ucPort), RT_IPC_FLAG_PRIO );
    rt_thread_init( &thread_serial_soft_trans_irq[ucPort], 
                   MB_THREAD_NAME(ucPort), 
                   serial_soft_trans_irq, 
                   (void *)(uint32_t)ucPort, 
                   serial_soft_trans_irq_stack[ucPort], 
                   sizeof(serial_soft_trans_irq_stack[ucPort]), 
                   10, 5 );
    rt_thread_startup( &thread_serial_soft_trans_irq[ucPort] );

    return TRUE;
}

void vMBPortSerialEnable(UCHAR ucPort, BOOL xRxEnable, BOOL xTxEnable)
{
    struct k64_serial_device *node = ((struct k64_serial_device *)serials[ucPort]->parent.user_data);
    rt_uint32_t recved_event;
    
    if (xRxEnable)
    {
        /* enable RX interrupt */
        serials[ucPort]->ops->control(serials[ucPort], RT_DEVICE_CTRL_SET_INT, (void *)RT_DEVICE_FLAG_INT_RX);
        if( node->cfg->type == UART_TYPE_485 ) {
            /* switch 485 to receive mode */
            rt_thread_delay( EN_485_RX_TIME * RT_TICK_PER_SECOND / 1000 );
            rt_pin_write( node->pin_en, PIN_HIGH );
        }
    }
    else
    {
        if( node->cfg->type == UART_TYPE_485 ) {
            /* switch 485 to transmit mode */
            rt_pin_write( node->pin_en, PIN_LOW );
            rt_thread_delay( EN_485_TX_TIME * RT_TICK_PER_SECOND / 1000 );
        }
        /* disable RX interrupt */
        serials[ucPort]->ops->control(serials[ucPort], RT_DEVICE_CTRL_CLR_INT, (void *)RT_DEVICE_FLAG_INT_RX);
    }
    if (xTxEnable)
    {
        /* start serials transmit */
        rt_event_send( &event_serial[ucPort], EVENT_SERIAL_TRANS_START );
    }
    else
    {
        /* stop serials transmit */
        rt_event_recv( &event_serial[ucPort], EVENT_SERIAL_TRANS_START,
                    RT_EVENT_FLAG_OR | RT_EVENT_FLAG_CLEAR, 0,
                    &recved_event );
    }
}

void vMBPortClose(UCHAR ucPort)
{
    serials[ucPort]->parent.close( &(serials[ucPort]->parent) );
}

BOOL xMBPortSerialPutByte(UCHAR ucPort, CHAR ucByte)
{
    serials[ucPort]->parent.write( &(serials[ucPort]->parent), 0, &ucByte, 1 );
    return TRUE;
}

BOOL xMBPortSerialGetByte(UCHAR ucPort, CHAR * pucByte)
{
    serials[ucPort]->parent.read(&(serials[ucPort]->parent), 0, pucByte, 1);
    return TRUE;
}

/* 
 * Create an interrupt handler for the transmit buffer empty interrupt
 * (or an equivalent) for your target processor. This function should then
 * call pxMBFrameCBTransmitterEmpty( ) which tells the protocol stack that
 * a new character can be sent. The protocol stack will then call 
 * xMBPortSerialPutByte( ) to send the character.
 */
void prvvUARTTxReadyISR( UCHAR ucPort )
{
    pxMBFrameCBTransmitterEmpty[ucPort]( ucPort );
}

/* 
 * Create an interrupt handler for the receive interrupt for your target
 * processor. This function should then call pxMBFrameCBByteReceived( ). The
 * protocol stack will then call xMBPortSerialGetByte( ) to retrieve the
 * character.
 */
void prvvUARTRxISR( UCHAR ucPort )
{
    pxMBFrameCBByteReceived[ucPort]( ucPort );
}

/**
 * Software simulation serials transmit IRQ handler.
 *
 * @param parameter parameter
 */
static void serial_soft_trans_irq(void* parameter) {
    rt_uint32_t recved_event;
    UCHAR ucPort = (UCHAR)((uint32_t)((uint32_t *)parameter));
    while (1)
    {
        /* waiting for serials transmit start */
        rt_event_recv( &event_serial[ucPort], EVENT_SERIAL_TRANS_START, RT_EVENT_FLAG_OR,
                RT_WAITING_FOREVER, &recved_event);
        /* execute modbus callback */
        prvvUARTTxReadyISR( ucPort );
    }
}

/**
 * This function is serials receive callback function
 *
 * @param dev the device of serials
 * @param size the data size that receive
 *
 * @return return RT_EOK
 */
static rt_err_t serial_rx_ind(rt_device_t dev, rt_size_t size)
{
    struct k64_serial_device *node = ((struct k64_serial_device *)((rt_serial_t *)dev)->parent.user_data);
    prvvUARTRxISR( node->instance );
    return RT_EOK;
}
