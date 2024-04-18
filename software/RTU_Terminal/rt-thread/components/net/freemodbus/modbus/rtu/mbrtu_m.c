/* 
 * FreeModbus Libary: A portable Modbus implementation for Modbus ASCII/RTU.
 * Copyright (c) 2013 China Beijing Armink <armink.ztl@gmail.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * File: $Id: mbrtu_m.c,v 1.60 2013/08/17 11:42:56 Armink Add Master Functions $
 */

/* ----------------------- System includes ----------------------------------*/
#include "stdlib.h"
#include "string.h"

/* ----------------------- Platform includes --------------------------------*/
#include "port.h"

/* ----------------------- Modbus includes ----------------------------------*/
#include "mb_m.h"
#include "mbrtu.h"
#include "mbframe.h"

#include "mbcrc.h"
#include "mbport.h"

#if MB_MASTER_RTU_ENABLED > 0
/* ----------------------- Defines ------------------------------------------*/
#define MB_SER_PDU_SIZE_MIN     4       /*!< Minimum size of a Modbus RTU frame. */
#define MB_SER_PDU_SIZE_MAX     256     /*!< Maximum size of a Modbus RTU frame. */
#define MB_SER_PDU_SIZE_CRC     2       /*!< Size of CRC field in PDU. */
#define MB_SER_PDU_ADDR_OFF     0       /*!< Offset of slave address in Ser-PDU. */
#define MB_SER_PDU_PDU_OFF      1       /*!< Offset of Modbus-PDU in Ser-PDU. */

/* ----------------------- Type definitions ---------------------------------*/
typedef enum
{
    STATE_M_RX_INIT,              /*!< Receiver is in initial state. */
    STATE_M_RX_IDLE,              /*!< Receiver is in idle state. */
    STATE_M_RX_RCV,               /*!< Frame is beeing received. */
    STATE_M_RX_ERROR,              /*!< If the frame is invalid. */
} eMBMasterRcvState;

typedef enum
{
    STATE_M_TX_IDLE,              /*!< Transmitter is in idle state. */
    STATE_M_TX_XMIT,              /*!< Transmitter is in transfer state. */
    STATE_M_TX_XFWR,              /*!< Transmitter is in transfer finish and wait receive state. */
} eMBMasterSndState;

/* ----------------------- Static variables ---------------------------------*/
static volatile eMBMasterSndState eSndState[BOARD_UART_MAX];
static volatile eMBMasterRcvState eRcvState[BOARD_UART_MAX];

static volatile UCHAR  *ucMasterRTUSndBuf[BOARD_UART_MAX];
static volatile UCHAR  *ucMasterRTURcvBuf[BOARD_UART_MAX];
static volatile USHORT usMasterRTUSendPDULength[BOARD_UART_MAX];

static volatile UCHAR *pucMasterSndBufferCur[BOARD_UART_MAX];
static volatile USHORT usMasterSndBufferCount[BOARD_UART_MAX];

static volatile USHORT usMasterRcvBufferPos[BOARD_UART_MAX];
static volatile BOOL   xFrameIsBroadcast[BOARD_UART_MAX];

/* ----------------------- Start implementation -----------------------------*/
eMBErrorCode
eMBMasterRTUInit(UCHAR ucPort, ULONG ulBaudRate, eMBParity eParity )
{
    eMBErrorCode    eStatus = MB_ENOERR;
    ULONG           usTimerT35_50us;

    MASTER_ENTER_CRITICAL_SECTION(  );

    RT_KERNEL_FREE( (void *)ucMasterRTUSndBuf[ucPort] );
    RT_KERNEL_FREE( (void *)ucMasterRTURcvBuf[ucPort] );
    ucMasterRTUSndBuf[ucPort] = RT_KERNEL_CALLOC( MB_SER_PDU_SIZE_MAX );
    ucMasterRTURcvBuf[ucPort] = RT_KERNEL_CALLOC( MB_SER_PDU_SIZE_MAX );
    xFrameIsBroadcast[ucPort] = FALSE;

    /* Modbus RTU uses 8 Databits. */
    if( xMBMasterPortSerialInit( ucPort, ulBaudRate, 8, eParity ) != TRUE )
    {
        eStatus = MB_EPORTERR;
    }
    else
    {
        /* If baudrate > 19200 then we should use the fixed timer values
         * t35 = 1750us. Otherwise t35 must be 3.5 times the character time.
         */
        if( ulBaudRate > 19200 )
        {
            usTimerT35_50us = 35;       /* 1800us. */
        }
        else
        {
            /* The timer reload value for a character is given by:
             *
             * ChTimeValue = Ticks_per_1s / ( Baudrate / 11 )
             *             = 11 * Ticks_per_1s / Baudrate
             *             = 220000 / Baudrate
             * The reload for t3.5 is 1.5 times this value and similary
             * for t3.5.
             */
            usTimerT35_50us = ( 7UL * 220000UL ) / ( 2UL * ulBaudRate );
        }
        if( xMBMasterPortTimersT35Init( ucPort, ( USHORT ) usTimerT35_50us ) != TRUE )
        {
            eStatus = MB_EPORTERR;
        }
    }
    MASTER_EXIT_CRITICAL_SECTION(  );

    return eStatus;
}

void
eMBMasterRTUStart( UCHAR ucPort )
{
    MASTER_ENTER_CRITICAL_SECTION(  );
    /* Initially the receiver is in the state STATE_M_RX_INIT. we start
     * the timer and if no character is received within t3.5 we change
     * to STATE_M_RX_IDLE. This makes sure that we delay startup of the
     * modbus protocol stack until the bus is free.
     */
    eRcvState[ucPort] = STATE_M_RX_INIT;
    vMBMasterPortSerialEnable( ucPort, TRUE, FALSE );
    vMBMasterPortTimersT35Enable( ucPort );

    MASTER_EXIT_CRITICAL_SECTION(  );
}

void
eMBMasterRTUStop( UCHAR ucPort )
{
    MASTER_ENTER_CRITICAL_SECTION(  );
    vMBMasterPortSerialEnable( ucPort, FALSE, FALSE );
    vMBMasterPortTimersDisable( ucPort );
    MASTER_EXIT_CRITICAL_SECTION(  );
}

eMBErrorCode
eMBMasterRTUReceive( UCHAR ucPort, UCHAR * pucRcvAddress, UCHAR ** pucFrame, USHORT * pusLength )
{
    eMBErrorCode    eStatus = MB_ENOERR;
    
    if( !ucMasterRTURcvBuf[ucPort] ) return MB_EILLSTATE;

    MASTER_ENTER_CRITICAL_SECTION(  );
    //RT_ASSERT( usMasterRcvBufferPos[ucPort] < MB_SER_PDU_SIZE_MAX );

    /* Length and CRC check */
    if( ( usMasterRcvBufferPos[ucPort] >= MB_SER_PDU_SIZE_MIN )
        && ( usMBCRC16( ( UCHAR * ) ucMasterRTURcvBuf[ucPort], usMasterRcvBufferPos[ucPort] ) == 0 ) )
    {
        /* Save the address field. All frames are passed to the upper layed
         * and the decision if a frame is used is done there.
         */
        *pucRcvAddress = ucMasterRTURcvBuf[ucPort][MB_SER_PDU_ADDR_OFF];

        /* Total length of Modbus-PDU is Modbus-Serial-Line-PDU minus
         * size of address field and CRC checksum.
         */
        *pusLength = ( USHORT )( usMasterRcvBufferPos[ucPort] - MB_SER_PDU_PDU_OFF - MB_SER_PDU_SIZE_CRC );

        /* Return the start of the Modbus PDU to the caller. */
        *pucFrame = ( UCHAR * ) & ucMasterRTURcvBuf[ucPort][MB_SER_PDU_PDU_OFF];
    }
    else
    {
        eStatus = MB_EIO;
    }

    MASTER_EXIT_CRITICAL_SECTION(  );
    return eStatus;
}

eMBErrorCode
eMBMasterRTUSend( UCHAR ucPort, UCHAR ucSlaveAddress, const UCHAR * pucFrame, USHORT usLength )
{
    eMBErrorCode    eStatus = MB_ENOERR;
    USHORT          usCRC16;

    if ( ucSlaveAddress > MB_MASTER_TOTAL_SLAVE_NUM ) return MB_EINVAL;
    
    if( !ucMasterRTURcvBuf[ucPort] ) return MB_EILLSTATE;

    MASTER_ENTER_CRITICAL_SECTION(  );

    // add by jay 2016/11/24
    if (eRcvState[ucPort] != STATE_M_RX_IDLE && !xMBMasterIsPortTimersEnable(ucPort)) {
        // 定时器未开启, 初始化接收状态, 防止无法发送
        eRcvState[ucPort] = STATE_M_RX_IDLE;
    }

    /* Check if the receiver is still in idle state. If not we where to
     * slow with processing the received frame and the master sent another
     * frame on the network. We have to abort sending the frame.
     */
    if( eRcvState[ucPort] == STATE_M_RX_IDLE )
    {
        /* First byte before the Modbus-PDU is the slave address. */
        pucMasterSndBufferCur[ucPort] = ( UCHAR * ) pucFrame - 1;
        usMasterSndBufferCount[ucPort] = 1;

        /* Now copy the Modbus-PDU into the Modbus-Serial-Line-PDU. */
        pucMasterSndBufferCur[ucPort][MB_SER_PDU_ADDR_OFF] = ucSlaveAddress;
        usMasterSndBufferCount[ucPort] += usLength;

        /* Calculate CRC16 checksum for Modbus-Serial-Line-PDU. */
        usCRC16 = usMBCRC16( ( UCHAR * ) pucMasterSndBufferCur[ucPort], usMasterSndBufferCount[ucPort] );
        ucMasterRTUSndBuf[ucPort][usMasterSndBufferCount[ucPort]++] = ( UCHAR )( usCRC16 & 0xFF );
        ucMasterRTUSndBuf[ucPort][usMasterSndBufferCount[ucPort]++] = ( UCHAR )( usCRC16 >> 8 );

        /* Activate the transmitter. */
        eSndState[ucPort] = STATE_M_TX_XMIT;
        vMBMasterPortSerialEnable( ucPort, FALSE, TRUE );

        {
            xFrameIsBroadcast[ucPort] = ( ucMasterRTUSndBuf[ucPort][MB_SER_PDU_ADDR_OFF] == MB_ADDRESS_BROADCAST ) ? TRUE : FALSE;
            if ( xFrameIsBroadcast[ucPort] == TRUE ) {
            	vMBMasterPortTimersConvertDelayEnable( ucPort );
            } else {
            	vMBMasterPortTimersRespondTimeoutEnable( ucPort );
            }
        }
    }
    else
    {
        eStatus = MB_EIO;
    }
    MASTER_EXIT_CRITICAL_SECTION(  );
    return eStatus;
}

BOOL
xMBMasterRTUReceiveFSM( UCHAR ucPort )
{
    BOOL            xTaskNeedSwitch = FALSE;
    UCHAR           ucByte;

    if( eSndState[ucPort] != STATE_M_TX_IDLE && eSndState[ucPort] != STATE_M_TX_XFWR ) {
        return FALSE;
    }
    RT_ASSERT(( eSndState[ucPort] == STATE_M_TX_IDLE ) || ( eSndState[ucPort] == STATE_M_TX_XFWR ));

    /* Always read the character. */
    ( void )xMBMasterPortSerialGetByte( ucPort, ( CHAR * ) & ucByte );

    switch ( eRcvState[ucPort] )
    {
        /* If we have received a character in the init state we have to
         * wait until the frame is finished.
         */
    case STATE_M_RX_INIT:
        vMBMasterPortTimersT35Enable( ucPort );
        break;

        /* In the error state we wait until all characters in the
         * damaged frame are transmitted.
         */
    case STATE_M_RX_ERROR:
        vMBMasterPortTimersT35Enable( ucPort );
        break;

        /* In the idle state we wait for a new character. If a character
         * is received the t1.5 and t3.5 timers are started and the
         * receiver is in the state STATE_RX_RECEIVCE and disable early
         * the timer of respond timeout .
         */
    case STATE_M_RX_IDLE:
    	/* In time of respond timeout,the receiver receive a frame.
    	 * Disable timer of respond timeout and change the transmiter state to idle.
    	 */
    	eSndState[ucPort] = STATE_M_TX_IDLE;
    	vMBMasterPortTimersDisable( ucPort );

        usMasterRcvBufferPos[ucPort] = 0;
        ucMasterRTURcvBuf[ucPort][usMasterRcvBufferPos[ucPort]++] = ucByte;
        eRcvState[ucPort] = STATE_M_RX_RCV;

        /* Enable t3.5 timers. */
        vMBMasterPortTimersT35Enable( ucPort );
        break;

        /* We are currently receiving a frame. Reset the timer after
         * every character received. If more than the maximum possible
         * number of bytes in a modbus frame is received the frame is
         * ignored.
         */
    case STATE_M_RX_RCV:
        if( usMasterRcvBufferPos[ucPort] < MB_SER_PDU_SIZE_MAX )
        {
            ucMasterRTURcvBuf[ucPort][usMasterRcvBufferPos[ucPort]++] = ucByte;
        }
        else
        {
            eRcvState[ucPort] = STATE_M_RX_ERROR;
        }
        vMBMasterPortTimersT35Enable( ucPort );
        break;
    }
    return xTaskNeedSwitch;
}

BOOL
xMBMasterRTUTransmitFSM( UCHAR ucPort )
{
    BOOL            xNeedPoll = FALSE;

    if( eRcvState[ucPort] != STATE_M_RX_IDLE ) {
        return FALSE;
    }
    RT_ASSERT( eRcvState[ucPort] == STATE_M_RX_IDLE );

    switch ( eSndState[ucPort] )
    {
        /* We should not get a transmitter event if the transmitter is in
         * idle state.  */
    case STATE_M_TX_IDLE:
        /* enable receiver/disable transmitter. */
        vMBMasterPortSerialEnable( ucPort, TRUE, FALSE );
        break;

    case STATE_M_TX_XMIT:
        /* check if we are finished. */
        if( usMasterSndBufferCount[ucPort] != 0 )
        {
            xMBMasterPortSerialPutByte( ucPort, ( CHAR )*pucMasterSndBufferCur[ucPort] );
            pucMasterSndBufferCur[ucPort]++;  /* next byte in sendbuffer. */
            usMasterSndBufferCount[ucPort]--;
        }
        else
        {
            /* Disable transmitter. This prevents another transmit buffer
             * empty interrupt. */
            eSndState[ucPort] = STATE_M_TX_XFWR;
            vMBMasterPortSerialEnable( ucPort, TRUE, FALSE );
        }
        
        if ( xFrameIsBroadcast[ucPort] == TRUE ) {
        	vMBMasterPortTimersConvertDelayEnable( ucPort );
        } else {
        	vMBMasterPortTimersRespondTimeoutEnable( ucPort );
        }
        break;
    }

    return xNeedPoll;
}

BOOL
xMBMasterRTUTimerExpired( UCHAR ucPort )
{
	BOOL xNeedPoll = FALSE;

	switch (eRcvState[ucPort])
	{
		/* Timer t35 expired. Startup phase is finished. */
	case STATE_M_RX_INIT:
		xNeedPoll = xMBMasterPortEventPost( ucPort, EV_MASTER_READY );
		break;

		/* A frame was received and t35 expired. Notify the listener that
		 * a new frame was received. */
	case STATE_M_RX_RCV:
		xNeedPoll = xMBMasterPortEventPost( ucPort, EV_MASTER_FRAME_RECEIVED );
		break;

		/* An error occured while receiving the frame. */
	case STATE_M_RX_ERROR:
		vMBMasterSetErrorType( ucPort, EV_ERROR_RECEIVE_DATA);
		xNeedPoll = xMBMasterPortEventPost( ucPort, EV_MASTER_ERROR_PROCESS );
		break;

		/* Function called in an illegal state. */
	default:
		/*RT_ASSERT(
				( eRcvState[ucPort] == STATE_M_RX_INIT ) || ( eRcvState[ucPort] == STATE_M_RX_RCV ) ||
				( eRcvState[ucPort] == STATE_M_RX_ERROR ) || ( eRcvState[ucPort] == STATE_M_RX_IDLE ));*/
		break;
	}
	eRcvState[ucPort] = STATE_M_RX_IDLE;

	switch (eSndState[ucPort])
	{
		/* A frame was send finish and convert delay or respond timeout expired.
		 * If the frame is broadcast,The master will idle,and if the frame is not
		 * broadcast.Notify the listener process error.*/
	case STATE_M_TX_XFWR:
		if ( xFrameIsBroadcast[ucPort] == FALSE ) {
			vMBMasterSetErrorType( ucPort, EV_ERROR_RESPOND_TIMEOUT);
			xNeedPoll = xMBMasterPortEventPost( ucPort, EV_MASTER_ERROR_PROCESS);
		}
		break;
		/* Function called in an illegal state. */
	default:
		/*RT_ASSERT(
				( eSndState[ucPort] == STATE_M_TX_XFWR ) || ( eSndState[ucPort] == STATE_M_TX_IDLE ));*/
		break;
	}
	eSndState[ucPort] = STATE_M_TX_IDLE;

	vMBMasterPortTimersDisable( ucPort );
	/* If timer mode is convert delay, the master event then turns EV_MASTER_EXECUTE status. */
	if (eMBMasterGetCurTimerMode(ucPort) == MB_TMODE_CONVERT_DELAY) {
		xNeedPoll = xMBMasterPortEventPost( ucPort, EV_MASTER_EXECUTE );
	}

	return xNeedPoll;
}

BOOL xMBMasterRTUPostReceived( UCHAR ucPort, UCHAR *ucBuf, USHORT usPos )
{
    BOOL xNeedPoll = FALSE;

    if( !ucMasterRTURcvBuf[ucPort] ) return FALSE;

    usMasterRcvBufferPos[ucPort] = usPos;
    memcpy( (void *)ucMasterRTURcvBuf[ucPort], ucBuf, usPos );
    xNeedPoll = xMBMasterPortEventPost( ucPort, EV_MASTER_FRAME_RECEIVED );
    eRcvState[ucPort] = STATE_M_RX_IDLE;

    eSndState[ucPort] = STATE_M_TX_IDLE;

	vMBMasterPortTimersDisable( ucPort );
	/* If timer mode is convert delay, the master event then turns EV_MASTER_EXECUTE status. */
	if (eMBMasterGetCurTimerMode(ucPort) == MB_TMODE_CONVERT_DELAY) {
		xNeedPoll = xMBMasterPortEventPost( ucPort, EV_MASTER_EXECUTE );
	}
    return xNeedPoll;
}

/* Get Modbus Master send PDU's buffer address pointer.*/
void vMBMasterRTUGetPDUSndBuf( UCHAR ucPort, UCHAR ** pucFrame )
{
	*pucFrame = ( UCHAR * ) &ucMasterRTUSndBuf[ucPort][MB_SER_PDU_PDU_OFF];
}

/* Set Modbus Master send PDU's buffer length.*/
void vMBMasterRTUSetPDUSndLength( UCHAR ucPort, USHORT SendPDULength )
{
	usMasterRTUSendPDULength[ucPort] = SendPDULength;
}

/* Get Modbus Master send PDU's buffer length.*/
USHORT usMBMasterRTUGetPDUSndLength( UCHAR ucPort )
{
	return usMasterRTUSendPDULength[ucPort];
}

/* The master request is broadcast? */
BOOL xMBMasterRTURequestIsBroadcast( UCHAR ucPort )
{
	return xFrameIsBroadcast[ucPort];
}

BOOL bMBMasterWaitRsp( UCHAR ucPort )
{
	return \
	    (eSndState[ucPort] == STATE_M_TX_XFWR) && \
	    (eMBMasterGetCurTimerMode(ucPort) == MB_TMODE_RESPOND_TIMEOUT);
}

#endif

