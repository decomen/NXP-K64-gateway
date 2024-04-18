/* 
 * FreeModbus Libary: A portable Modbus implementation for Modbus ASCII/RTU.
 * Copyright (c) 2006 Christian Walter <wolti@sil.at>
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
 * File: $Id: mb.c,v 1.27 2007/02/18 23:45:41 wolti Exp $
 */

/* ----------------------- System includes ----------------------------------*/
#include <board.h>
#include "stdlib.h"
#include "string.h"

/* ----------------------- Platform includes --------------------------------*/
#include "port.h"

/* ----------------------- Modbus includes ----------------------------------*/

#include "mb.h"
#include "mbconfig.h"
#include "mbframe.h"
#include "mbproto.h"
#include "mbfunc.h"

#include "mbport.h"
#if MB_SLAVE_RTU_ENABLED == 1
#include "mbrtu.h"
#endif
#if MB_SLAVE_ASCII_ENABLED == 1
#include "mbascii.h"
#endif
#if MB_SLAVE_TCP_ENABLED == 1
#include "mbtcp.h"
#endif

#ifndef MB_PORT_HAS_CLOSE
#define MB_PORT_HAS_CLOSE 1
#endif

/* ----------------------- Static variables ---------------------------------*/

static UCHAR    ucMBAddress[MB_SLAVE_NUMS];
static eMBMode  eMBCurrentMode[MB_SLAVE_NUMS];

static enum
{
    STATE_ENABLED,
    STATE_DISABLED,
    STATE_NOT_INITIALIZED
} eMBState[MB_SLAVE_NUMS];

/* Functions pointer which are initialized in eMBInit( ). Depending on the
 * mode (RTU or ASCII) the are set to the correct implementations.
 * Using for Modbus Slave
 */
static peMBFrameSend peMBFrameSendCur[MB_SLAVE_NUMS];
static pvMBFrameStart pvMBFrameStartCur[MB_SLAVE_NUMS];
static pvMBFrameStop pvMBFrameStopCur[MB_SLAVE_NUMS];
static peMBFrameReceive peMBFrameReceiveCur[MB_SLAVE_NUMS];
static pvMBFrameClose pvMBFrameCloseCur[MB_SLAVE_NUMS];

/* Callback functions required by the porting layer. They are called when
 * an external event has happend which includes a timeout or the reception
 * or transmission of a character.
 * Using for Modbus Slave
 */
BOOL ( *pxMBFrameCBByteReceived[MB_SLAVE_NUMS] ) ( UCHAR ucPort );
BOOL( *pxMBFrameCBTransmitterEmpty[MB_SLAVE_NUMS] ) ( UCHAR ucPort );
BOOL( *pxMBPortCBTimerExpired[MB_SLAVE_NUMS] ) ( UCHAR ucPort );

BOOL( *pxMBFrameCBReceiveFSMCur[MB_SLAVE_NUMS] ) ( UCHAR ucPort );
BOOL( *pxMBFrameCBTransmitFSMCur[MB_SLAVE_NUMS] ) ( UCHAR ucPort );

/* An array of Modbus functions handlers which associates Modbus function
 * codes with implementing functions.
 */

static xMBFunctionHandler xFuncHandlers[MB_FUNC_HANDLERS_MAX] = {
#if MB_FUNC_OTHER_REP_SLAVEID_ENABLED > 0
    {MB_FUNC_OTHER_REPORT_SLAVEID, eMBFuncReportSlaveID},
#endif
#if MB_FUNC_READ_INPUT_ENABLED > 0
    {MB_FUNC_READ_INPUT_REGISTER, eMBFuncReadInputRegister},
#endif
#if MB_FUNC_READ_HOLDING_ENABLED > 0
    {MB_FUNC_READ_HOLDING_REGISTER, eMBFuncReadHoldingRegister},
#endif
#if MB_FUNC_WRITE_MULTIPLE_HOLDING_ENABLED > 0
    {MB_FUNC_WRITE_MULTIPLE_REGISTERS, eMBFuncWriteMultipleHoldingRegister},
#endif
#if MB_FUNC_WRITE_HOLDING_ENABLED > 0
    {MB_FUNC_WRITE_REGISTER, eMBFuncWriteHoldingRegister},
#endif
#if MB_FUNC_READWRITE_HOLDING_ENABLED > 0
    {MB_FUNC_READWRITE_MULTIPLE_REGISTERS, eMBFuncReadWriteMultipleHoldingRegister},
#endif
#if MB_FUNC_READ_COILS_ENABLED > 0
    {MB_FUNC_READ_COILS, eMBFuncReadCoils},
#endif
#if MB_FUNC_WRITE_COIL_ENABLED > 0
    {MB_FUNC_WRITE_SINGLE_COIL, eMBFuncWriteCoil},
#endif
#if MB_FUNC_WRITE_MULTIPLE_COILS_ENABLED > 0
    {MB_FUNC_WRITE_MULTIPLE_COILS, eMBFuncWriteMultipleCoils},
#endif
#if MB_FUNC_READ_DISCRETE_INPUTS_ENABLED > 0
    {MB_FUNC_READ_DISCRETE_INPUTS, eMBFuncReadDiscreteInputs},
#endif
};

void vMBInitState( void )
{
    for( UCHAR ucPort = 0; ucPort < MB_SLAVE_NUMS; ucPort++ ) {
        eMBState[ucPort] = STATE_NOT_INITIALIZED;
    }
}

/* ----------------------- Start implementation -----------------------------*/
eMBErrorCode
eMBInit( eMBMode eMode, UCHAR ucSlaveAddress, UCHAR ucPort, ULONG ulBaudRate, eMBParity eParity )
{
    eMBErrorCode    eStatus = MB_ENOERR;

    if( BOARD_ZIGBEE_UART == ucPort ) {
        ucSlaveAddress = MB_ADDRESS_MAX;
    }

    /* check preconditions */
    if( ( ucSlaveAddress == MB_ADDRESS_BROADCAST ) ||
        ( ucSlaveAddress < MB_ADDRESS_MIN ) || ( ucSlaveAddress > MB_ADDRESS_MAX ) )
    {
        eStatus = MB_EINVAL;
    }
    else
    {
        ucMBAddress[ucPort] = ucSlaveAddress;

        switch ( eMode )
        {
#if MB_SLAVE_RTU_ENABLED > 0
        case MB_RTU:
            pvMBFrameStartCur[ucPort] = eMBRTUStart;
            pvMBFrameStopCur[ucPort] = eMBRTUStop;
            peMBFrameSendCur[ucPort] = eMBRTUSend;
            peMBFrameReceiveCur[ucPort] = eMBRTUReceive;
            pvMBFrameCloseCur[ucPort] = MB_PORT_HAS_CLOSE ? vMBPortClose : NULL;
            pxMBFrameCBByteReceived[ucPort] = xMBRTUReceiveFSM;
            pxMBFrameCBTransmitterEmpty[ucPort] = xMBRTUTransmitFSM;
            pxMBPortCBTimerExpired[ucPort] = xMBRTUTimerT35Expired;

            eStatus = eMBRTUInit( ucMBAddress[ucPort], ucPort, ulBaudRate, eParity );
            break;
#endif
#if MB_SLAVE_ASCII_ENABLED > 0
        case MB_ASCII:
            pvMBFrameStartCur[ucPort] = eMBASCIIStart;
            pvMBFrameStopCur[ucPort] = eMBASCIIStop;
            peMBFrameSendCur[ucPort] = eMBASCIISend;
            peMBFrameReceiveCur[ucPort] = eMBASCIIReceive;
            pvMBFrameCloseCur[ucPort] = MB_PORT_HAS_CLOSE ? vMBPortClose : NULL;
            pxMBFrameCBByteReceived[ucPort] = xMBASCIIReceiveFSM;
            pxMBFrameCBTransmitterEmpty[ucPort] = xMBASCIITransmitFSM;
            pxMBPortCBTimerExpired[ucPort] = xMBASCIITimerT1SExpired;

            eStatus = eMBASCIIInit( ucMBAddress[ucPort], ucPort, ulBaudRate, eParity );
            break;
#endif
        default:
            eStatus = MB_EINVAL;
            break;
        }

        if( eStatus == MB_ENOERR )
        {
            if( !xMBPortEventInit( ucPort ) )
            {
                /* port dependent event module initalization failed. */
                eStatus = MB_EPORTERR;
            }
            else
            {
                eMBCurrentMode[ucPort] = eMode;
                eMBState[ucPort] = STATE_DISABLED;
            }
        }
    }
    return eStatus;
}

#if MB_SLAVE_TCP_ENABLED > 0
eMBErrorCode
eMBTCPInit( UCHAR ucPort, USHORT ucTCPPort )
{
    eMBErrorCode    eStatus = MB_ENOERR;

    if( ( eStatus = eMBTCPDoInit( ucPort, ucTCPPort ) ) != MB_ENOERR )
    {
        eMBState[ucPort] = STATE_DISABLED;
    }
    else if( !xMBPortEventInit( ucPort ) )
    {
        /* Port dependent event module initalization failed. */
        eStatus = MB_EPORTERR;
    }
    else
    {
        pvMBFrameStartCur[ucPort] = eMBTCPStart;
        pvMBFrameStopCur[ucPort] = eMBTCPStop;
        peMBFrameReceiveCur[ucPort] = eMBTCPReceive;
        peMBFrameSendCur[ucPort] = eMBTCPSend;
        pvMBFrameCloseCur[ucPort] = MB_PORT_HAS_CLOSE ? vMBTCPPortClose : NULL;
        ucMBAddress[ucPort] = MB_TCP_PSEUDO_ADDRESS;
        eMBCurrentMode[ucPort] = MB_TCP;
        eMBState[ucPort] = STATE_DISABLED;
    }
    return eStatus;
}
#endif

#if MB_SLAVE_TCP_ENABLED > 0 && MB_SLAVE_RTU_ENABLED > 0
eMBErrorCode
eMBRTU_OverTCPInit( UCHAR ucPort, UCHAR ucSlaveAddress, USHORT ucTCPPort )
{
    eMBErrorCode    eStatus = MB_ENOERR;

    if( ( eStatus = eMBTCPDoInit( ucPort, ucTCPPort ) ) != MB_ENOERR )
    {
        eMBState[ucPort] = STATE_DISABLED;
    }
    else if( !xMBPortEventInit( ucPort ) )
    {
        /* Port dependent event module initalization failed. */
        eStatus = MB_EPORTERR;
    }
    else
    {
        pvMBFrameStartCur[ucPort] = eMBTCPStart;
        pvMBFrameStopCur[ucPort] = eMBTCPStop;
        peMBFrameReceiveCur[ucPort] = eMBRTU_OverTCPReceive;
        peMBFrameSendCur[ucPort] = eMBRTU_OverTCPSend;
        pvMBFrameCloseCur[ucPort] = MB_PORT_HAS_CLOSE ? vMBTCPPortClose : NULL;
        ucMBAddress[ucPort] = ucSlaveAddress;
        eMBCurrentMode[ucPort] = MB_RTU_OVERTCP;
        eMBState[ucPort] = STATE_DISABLED;
    }
    return eStatus;
}
#endif

eMBErrorCode
eMBRegisterCB( UCHAR ucPort, UCHAR ucFunctionCode, pxMBFunctionHandler pxHandler )
{
    int             i;
    eMBErrorCode    eStatus;

    if( ( 0 < ucFunctionCode ) && ( ucFunctionCode <= 127 ) )
    {
        ENTER_CRITICAL_SECTION(  );
        if( pxHandler != NULL )
        {
            for( i = 0; i < MB_FUNC_HANDLERS_MAX; i++ )
            {
                if( ( xFuncHandlers[i].pxHandler == NULL ) ||
                    ( xFuncHandlers[i].pxHandler == pxHandler ) )
                {
                    xFuncHandlers[i].ucFunctionCode = ucFunctionCode;
                    xFuncHandlers[i].pxHandler = pxHandler;
                    break;
                }
            }
            eStatus = ( i != MB_FUNC_HANDLERS_MAX ) ? MB_ENOERR : MB_ENORES;
        }
        else
        {
            for( i = 0; i < MB_FUNC_HANDLERS_MAX; i++ )
            {
                if( xFuncHandlers[i].ucFunctionCode == ucFunctionCode )
                {
                    xFuncHandlers[i].ucFunctionCode = 0;
                    xFuncHandlers[i].pxHandler = NULL;
                    break;
                }
            }
            /* Remove can't fail. */
            eStatus = MB_ENOERR;
        }
        EXIT_CRITICAL_SECTION(  );
    }
    else
    {
        eStatus = MB_EINVAL;
    }
    return eStatus;
}


eMBErrorCode
eMBClose( UCHAR ucPort )
{
    eMBErrorCode    eStatus = MB_ENOERR;

    if( eMBState[ucPort] == STATE_DISABLED )
    {
        if( pvMBFrameCloseCur[ucPort] != NULL )
        {
            pvMBFrameCloseCur[ucPort]( ucPort );
        }
    }
    else
    {
        eStatus = MB_EILLSTATE;
    }
    return eStatus;
}

eMBErrorCode
eMBEnable( UCHAR ucPort )
{
    eMBErrorCode    eStatus = MB_ENOERR;

    if( eMBState[ucPort] == STATE_DISABLED )
    {
        /* Activate the protocol stack. */
        pvMBFrameStartCur[ucPort]( ucPort );
        eMBState[ucPort] = STATE_ENABLED;
    }
    else
    {
        eStatus = MB_EILLSTATE;
    }
    return eStatus;
}

eMBErrorCode
eMBDisable( UCHAR ucPort )
{
    eMBErrorCode    eStatus;

    if( eMBState[ucPort] == STATE_ENABLED )
    {
        pvMBFrameStopCur[ucPort]( ucPort );
        eMBState[ucPort] = STATE_DISABLED;
        eStatus = MB_ENOERR;
    }
    else if( eMBState[ucPort] == STATE_DISABLED )
    {
        eStatus = MB_ENOERR;
    }
    else
    {
        eStatus = MB_EILLSTATE;
    }
    return eStatus;
}

eMBErrorCode
eMBPoll( UCHAR ucPort )
{
    static UCHAR   *ucMBFrame;
    static UCHAR    ucRcvAddress;
    static UCHAR    ucFunctionCode;
    static USHORT   usLength;
    static eMBException eException;

    int             i;
    eMBErrorCode    eStatus = MB_ENOERR;
    eMBEventType    eEvent;

    /* Check if the protocol stack is ready. */
    if( eMBState[ucPort] != STATE_ENABLED )
    {
        return MB_EILLSTATE;
    }

    /* Check if there is a event available. If not return control to caller.
     * Otherwise we will handle the event. */
    if( xMBPortEventGet(ucPort, &eEvent ) == TRUE )
    {
        switch ( eEvent )
        {
        case EV_READY:
            break;

        case EV_FRAME_RECEIVED:
            eStatus = peMBFrameReceiveCur[ucPort](ucPort, &ucRcvAddress, &ucMBFrame, &usLength );
            if( eStatus == MB_ENOERR )
            {
                /* Check if the frame is for us. If not ignore the frame. */
                if( ( ucRcvAddress == ucMBAddress[ucPort] ) || ( ucRcvAddress == MB_ADDRESS_BROADCAST ) )
                {
                    ( void )xMBPortEventPost(ucPort, EV_EXECUTE );
                }
            }
            break;

        case EV_EXECUTE:
            ucFunctionCode = ucMBFrame[MB_PDU_FUNC_OFF];
            eException = MB_EX_ILLEGAL_FUNCTION;
            for( i = 0; i < MB_FUNC_HANDLERS_MAX; i++ )
            {
                /* No more function handlers registered. Abort. */
                if( xFuncHandlers[i].ucFunctionCode == 0 )
                {
                    break;
                }
                else if( xFuncHandlers[i].ucFunctionCode == ucFunctionCode )
                {
                    eException = xFuncHandlers[i].pxHandler(ucPort, ucMBFrame, &usLength );
                    break;
                }
            }

            /* If the request was not sent to the broadcast address we
             * return a reply. */
            if( ucRcvAddress != MB_ADDRESS_BROADCAST )
            {
                if( eException != MB_EX_NONE )
                {
                    /* An exception occured. Build an error frame. */
                    usLength = 0;
                    ucMBFrame[usLength++] = ( UCHAR )( ucFunctionCode | MB_FUNC_ERROR );
                    ucMBFrame[usLength++] = eException;
                }
                eStatus = peMBFrameSendCur[ucPort](ucPort, ucMBAddress[ucPort], ucMBFrame, usLength );
            }
            break;

        case EV_FRAME_SENT:
            break;
        }
    }
    return MB_ENOERR;
}

BOOL eMBIsEnabled( UCHAR ucPort )
{
	return (eMBState[ucPort] == STATE_ENABLED ? TRUE : FALSE);
}

void vMBSetAddress( UCHAR ucPort, UCHAR ucAddress )
{
    ucMBAddress[ucPort] = ucAddress;
}
