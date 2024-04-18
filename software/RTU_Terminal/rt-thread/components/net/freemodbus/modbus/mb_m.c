/*
 * FreeModbus Libary: A portable Modbus implementation for Modbus ASCII/RTU.
 * Copyright (C) 2013 Armink <armink.ztl@gmail.com>
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
 * File: $Id: mbrtu_m.c,v 1.60 2013/08/20 11:18:10 Armink Add Master Functions $
 */

/* ----------------------- System includes ----------------------------------*/
#include "stdlib.h"
#include "string.h"

/* ----------------------- Platform includes --------------------------------*/
#include "port.h"

/* ----------------------- Modbus includes ----------------------------------*/

#include "mb_m.h"
#include "mbconfig.h"
#include "mbframe.h"
#include "mbproto.h"
#include "mbfunc.h"

#include "mbport.h"
#if MB_MASTER_RTU_ENABLED == 1
#include "mbrtu.h"
#endif
#if MB_MASTER_ASCII_ENABLED == 1
#include "mbascii.h"
#endif
#if MB_MASTER_TCP_ENABLED == 1
#include "mbtcp.h"
#endif

#if MB_MASTER_RTU_ENABLED > 0 || MB_MASTER_ASCII_ENABLED > 0

#ifndef MB_PORT_HAS_CLOSE
#define MB_PORT_HAS_CLOSE 1
#endif

/* ----------------------- Static variables ---------------------------------*/

static UCHAR    ucMBMasterDestAddress[MB_MASTER_NUMS];
static BOOL     xMBRunInMasterMode[MB_MASTER_NUMS];
static eMBMasterErrorEventType eMBMasterCurErrorType[MB_MASTER_NUMS];

static enum {
    STATE_ENABLED,
    STATE_DISABLED,
    STATE_NOT_INITIALIZED
} eMBState[MB_MASTER_NUMS];

/* Functions pointer which are initialized in eMBInit( ). Depending on the
 * mode (RTU or ASCII) the are set to the correct implementations.
 * Using for Modbus Master,Add by Armink 20130813
 */
static peMBFrameSend peMBMasterFrameSendCur[MB_MASTER_NUMS];
static pvMBFrameStart pvMBMasterFrameStartCur[MB_MASTER_NUMS];
static pvMBFrameStop pvMBMasterFrameStopCur[MB_MASTER_NUMS];
static peMBFrameReceive peMBMasterFrameReceiveCur[MB_MASTER_NUMS];
static pvMBFrameClose pvMBMasterFrameCloseCur[MB_MASTER_NUMS];

/* Callback functions required by the porting layer. They are called when
 * an external event has happend which includes a timeout or the reception
 * or transmission of a character.
 * Using for Modbus Master,Add by Armink 20130813
 */
BOOL(*pxMBMasterFrameCBByteReceived[MB_MASTER_NUMS])(UCHAR ucPort);

BOOL(*pxMBMasterFrameCBTransmitterEmpty[MB_MASTER_NUMS])(UCHAR ucPort);

BOOL(*pxMBMasterPortCBTimerExpired[MB_MASTER_NUMS])(UCHAR ucPort);

BOOL(*pxMBMasterFrameCBReceiveFSMCur[MB_MASTER_NUMS])(UCHAR ucPort);

BOOL(*pxMBMasterFrameCBTransmitFSMCur[MB_MASTER_NUMS])(UCHAR ucPort);

void (*pxMBMasterGetPDUSndBuf[MB_MASTER_NUMS])(UCHAR ucPort, UCHAR **pucFrame);

void (*pxMBMasterSetPDUSndLength[MB_MASTER_NUMS])(UCHAR ucPort, USHORT SendPDULength);

USHORT(*pxMBMasterGetPDUSndLength[MB_MASTER_NUMS])(UCHAR ucPort);

BOOL(*pxMBMasterRequestIsBroadcast[MB_MASTER_NUMS])(UCHAR ucPort);

/* An array of Modbus functions handlers which associates Modbus function
 * codes with implementing functions.
 */
static xMBFunctionHandler xMasterFuncHandlers[MB_FUNC_HANDLERS_MAX] = {
#if MB_FUNC_OTHER_REP_SLAVEID_ENABLED > 0
	//TODO Add Master function define
    {MB_FUNC_OTHER_REPORT_SLAVEID, eMBFuncReportSlaveID},
#endif
#if MB_FUNC_READ_INPUT_ENABLED > 0
    {MB_FUNC_READ_INPUT_REGISTER, eMBMasterFuncReadInputRegister},
#endif
#if MB_FUNC_READ_HOLDING_ENABLED > 0
    {MB_FUNC_READ_HOLDING_REGISTER, eMBMasterFuncReadHoldingRegister},
#endif
#if MB_FUNC_WRITE_MULTIPLE_HOLDING_ENABLED > 0
    {MB_FUNC_WRITE_MULTIPLE_REGISTERS, eMBMasterFuncWriteMultipleHoldingRegister},
#endif
#if MB_FUNC_WRITE_HOLDING_ENABLED > 0
    {MB_FUNC_WRITE_REGISTER, eMBMasterFuncWriteHoldingRegister},
#endif
#if MB_FUNC_READWRITE_HOLDING_ENABLED > 0
    {MB_FUNC_READWRITE_MULTIPLE_REGISTERS, eMBMasterFuncReadWriteMultipleHoldingRegister},
#endif
#if MB_FUNC_READ_COILS_ENABLED > 0
    {MB_FUNC_READ_COILS, eMBMasterFuncReadCoils},
#endif
#if MB_FUNC_WRITE_COIL_ENABLED > 0
    {MB_FUNC_WRITE_SINGLE_COIL, eMBMasterFuncWriteCoil},
#endif
#if MB_FUNC_WRITE_MULTIPLE_COILS_ENABLED > 0
    {MB_FUNC_WRITE_MULTIPLE_COILS, eMBMasterFuncWriteMultipleCoils},
#endif
#if MB_FUNC_READ_DISCRETE_INPUTS_ENABLED > 0
    {MB_FUNC_READ_DISCRETE_INPUTS, eMBMasterFuncReadDiscreteInputs},
#endif
};

void vMBMasterInitState(void)
{
    for (UCHAR ucPort = 0; ucPort < MB_MASTER_NUMS; ucPort++) {
        eMBState[ucPort] = STATE_NOT_INITIALIZED;
        xMBRunInMasterMode[ucPort] = FALSE;
    }
}

/* ----------------------- Start implementation -----------------------------*/
eMBErrorCode eMBMasterInit(eMBMode eMode, UCHAR ucPort, ULONG ulBaudRate, eMBParity eParity)
{
    eMBErrorCode    eStatus = MB_ENOERR;

    switch (eMode) {
#if MB_MASTER_RTU_ENABLED > 0
    case MB_RTU:
        pvMBMasterFrameStartCur[ucPort] = eMBMasterRTUStart;
        pvMBMasterFrameStopCur[ucPort] = eMBMasterRTUStop;
        peMBMasterFrameSendCur[ucPort] = eMBMasterRTUSend;
        peMBMasterFrameReceiveCur[ucPort] = eMBMasterRTUReceive;
        pvMBMasterFrameCloseCur[ucPort] = MB_PORT_HAS_CLOSE ? vMBMasterPortClose : NULL;
        pxMBMasterFrameCBByteReceived[ucPort] = xMBMasterRTUReceiveFSM;
        pxMBMasterFrameCBTransmitterEmpty[ucPort] = xMBMasterRTUTransmitFSM;
        pxMBMasterPortCBTimerExpired[ucPort] = xMBMasterRTUTimerExpired;

        pxMBMasterGetPDUSndBuf[ucPort] = vMBMasterRTUGetPDUSndBuf;
        pxMBMasterSetPDUSndLength[ucPort] = vMBMasterRTUSetPDUSndLength;
        pxMBMasterGetPDUSndLength[ucPort] = usMBMasterRTUGetPDUSndLength;
        pxMBMasterRequestIsBroadcast[ucPort] = xMBMasterRTURequestIsBroadcast;

        eStatus = eMBMasterRTUInit(ucPort, ulBaudRate, eParity);
        break;
#endif
#if MB_MASTER_ASCII_ENABLED > 0
    case MB_ASCII:
        pvMBMasterFrameStartCur[ucPort] = eMBMasterASCIIStart;
        pvMBMasterFrameStopCur[ucPort] = eMBMasterASCIIStop;
        peMBMasterFrameSendCur[ucPort] = eMBMasterASCIISend;
        peMBMasterFrameReceiveCur[ucPort] = eMBMasterASCIIReceive;
        pvMBMasterFrameCloseCur[ucPort] = MB_PORT_HAS_CLOSE ? vMBMasterPortClose : NULL;
        pxMBMasterFrameCBByteReceived[ucPort] = xMBMasterASCIIReceiveFSM;
        pxMBMasterFrameCBTransmitterEmpty[ucPort] = xMBMasterASCIITransmitFSM;
        pxMBMasterPortCBTimerExpired[ucPort] = xMBMasterASCIITimerT1SExpired;

        pxMBMasterGetPDUSndBuf[ucPort] = vMBMasterASCIIGetPDUSndBuf;
        pxMBMasterSetPDUSndLength[ucPort] = vMBMasterASCIISetPDUSndLength;
        pxMBMasterGetPDUSndLength[ucPort] = usMBMasterASCIIGetPDUSndLength;
        pxMBMasterRequestIsBroadcast[ucPort] = xMBMasterASCIIRequestIsBroadcast;

        eStatus = eMBMasterASCIIInit(ucPort, ulBaudRate, eParity);
        break;
#endif
    default:
        eStatus = MB_EINVAL;
        break;
    }

    if (eStatus == MB_ENOERR) {
        if (!xMBMasterPortEventInit(ucPort)) {
            /* port dependent event module initalization failed. */
            eStatus = MB_EPORTERR;
        } else {
            eMBState[ucPort] = STATE_DISABLED;
        }
        /* initialize the OS resource for modbus master. */
        vMBMasterOsResInit(ucPort);
    }
    return eStatus;
}

#if MB_MASTER_TCP_ENABLED > 0

eMBErrorCode eMBMasterTCPInit(UCHAR ucPort, USHORT ucTCPPort)
{
    eMBErrorCode    eStatus = MB_ENOERR;

    if ((eStatus = eMBMasterTCPDoInit(ucPort, ucTCPPort)) != MB_ENOERR) {
        eMBState[ucPort] = STATE_DISABLED;
    } else {
        pvMBMasterFrameStartCur[ucPort] = eMBMasterTCPStart;
        pvMBMasterFrameStopCur[ucPort] = eMBMasterTCPStop;
        peMBMasterFrameSendCur[ucPort] = eMBMasterTCPSend;
        peMBMasterFrameReceiveCur[ucPort] = eMBMasterTCPReceive;
        pvMBMasterFrameCloseCur[ucPort] = MB_PORT_HAS_CLOSE ? vMBTCPPortClose : NULL;
        //pxMBMasterFrameCBByteReceived[ucPort] = xMBMasterRTUReceiveFSM;
        //pxMBMasterFrameCBTransmitterEmpty[ucPort] = xMBMasterRTUTransmitFSM;
        pxMBMasterPortCBTimerExpired[ucPort] = xMBMasterTCPTimerExpired;

        pxMBMasterGetPDUSndBuf[ucPort] = vMBMasterTCPGetPDUSndBuf;
        pxMBMasterSetPDUSndLength[ucPort] = vMBMasterTCPSetPDUSndLength;
        pxMBMasterGetPDUSndLength[ucPort] = usMBMasterTCPGetPDUSndLength;
        pxMBMasterRequestIsBroadcast[ucPort] = xMBMasterTCPRequestIsBroadcast;

        ucMBMasterDestAddress[ucPort] = MB_TCP_PSEUDO_ADDRESS;
        eMBState[ucPort] = STATE_DISABLED;
    }

    if (eStatus == MB_ENOERR) {
        if (!xMBMasterPortEventInit(ucPort)) {
            /* port dependent event module initalization failed. */
            eStatus = MB_EPORTERR;
        } else {
            eMBState[ucPort] = STATE_DISABLED;
        }
        /* initialize the OS resource for modbus master. */
        vMBMasterOsResInit(ucPort);
    }
    return eStatus;
}
#endif

#if MB_MASTER_TCP_ENABLED > 0 && MB_MASTER_RTU_ENABLED > 0

eMBErrorCode eMBMasterRTU_OverTCPInit(UCHAR ucPort, USHORT ucTCPPort)
{
    eMBErrorCode    eStatus = MB_ENOERR;

    if ((eStatus = eMBMasterTCPDoInit(ucPort, ucTCPPort)) != MB_ENOERR) {
        eMBState[ucPort] = STATE_DISABLED;
    } else {
        pvMBMasterFrameStartCur[ucPort] = eMBMasterTCPStart;
        pvMBMasterFrameStopCur[ucPort] = eMBMasterTCPStop;
        peMBMasterFrameSendCur[ucPort] = eMBMasterRTU_OverTCPSend;
        peMBMasterFrameReceiveCur[ucPort] = eMBMasterRTU_OverTCPReceive;
        pvMBMasterFrameCloseCur[ucPort] = MB_PORT_HAS_CLOSE ? vMBTCPPortClose : NULL;
        //pxMBMasterFrameCBByteReceived[ucPort] = xMBMasterRTUReceiveFSM;
        //pxMBMasterFrameCBTransmitterEmpty[ucPort] = xMBMasterRTUTransmitFSM;
        pxMBMasterPortCBTimerExpired[ucPort] = xMBMasterTCPTimerExpired;

        pxMBMasterGetPDUSndBuf[ucPort] = vMBMasterRTU_OverTCPGetPDUSndBuf;
        pxMBMasterSetPDUSndLength[ucPort] = vMBMasterTCPSetPDUSndLength;
        pxMBMasterGetPDUSndLength[ucPort] = usMBMasterTCPGetPDUSndLength;
        pxMBMasterRequestIsBroadcast[ucPort] = xMBMasterTCPRequestIsBroadcast;

        ucMBMasterDestAddress[ucPort] = MB_TCP_PSEUDO_ADDRESS;
        eMBState[ucPort] = STATE_DISABLED;
    }

    if (eStatus == MB_ENOERR) {
        if (!xMBMasterPortEventInit(ucPort)) {
            /* port dependent event module initalization failed. */
            eStatus = MB_EPORTERR;
        } else {
            eMBState[ucPort] = STATE_DISABLED;
        }
        /* initialize the OS resource for modbus master. */
        vMBMasterOsResInit(ucPort);
    }
    return eStatus;
}
#endif

eMBErrorCode eMBMasterClose(UCHAR ucPort)
{
    eMBErrorCode    eStatus = MB_ENOERR;

    if (eMBState[ucPort] == STATE_DISABLED) {
        if (pvMBMasterFrameCloseCur[ucPort] != NULL) {
            pvMBMasterFrameCloseCur[ucPort](ucPort);
        }
    } else {
        eStatus = MB_EILLSTATE;
    }
    return eStatus;
}

eMBErrorCode eMBMasterEnable(UCHAR ucPort)
{
    eMBErrorCode    eStatus = MB_ENOERR;

    if (eMBState[ucPort] == STATE_DISABLED) {
        /* Activate the protocol stack. */
        pvMBMasterFrameStartCur[ucPort](ucPort);
        eMBState[ucPort] = STATE_ENABLED;
    } else {
        eStatus = MB_EILLSTATE;
    }
    return eStatus;
}

eMBErrorCode eMBMasterDisable(UCHAR ucPort)
{
    eMBErrorCode    eStatus;

    if (eMBState[ucPort] == STATE_ENABLED) {
        pvMBMasterFrameStopCur[ucPort](ucPort);
        eMBState[ucPort] = STATE_DISABLED;
        eStatus = MB_ENOERR;
    } else if (eMBState[ucPort] == STATE_DISABLED) {
        eStatus = MB_ENOERR;
    } else {
        eStatus = MB_EILLSTATE;
    }
    return eStatus;
}

eMBErrorCode eMBMasterPoll(UCHAR ucPort)
{
    static UCHAR   *ucMBFrame;
    static UCHAR    ucRcvAddress;
    static UCHAR    ucFunctionCode;
    static USHORT   usLength;
    static eMBException eException;

    int             i, j;
    eMBErrorCode    eStatus = MB_ENOERR;
    eMBMasterEventType    eEvent;
    eMBMasterErrorEventType errorType;

    /* Check if the protocol stack is ready. */
    if (eMBState[ucPort] != STATE_ENABLED) {
        return MB_EILLSTATE;
    }

    /* Check if there is a event available. If not return control to caller.
     * Otherwise we will handle the event. */
    if (xMBMasterPortEventGet(ucPort, &eEvent) == TRUE) {
        switch (eEvent) {
        case EV_MASTER_READY:
            break;

        case EV_MASTER_FRAME_RECEIVED:
            eStatus = peMBMasterFrameReceiveCur[ucPort](ucPort, &ucRcvAddress, &ucMBFrame, &usLength);
            /* Check if the frame is for us. If not ,send an error process event. */
            if ((eStatus == MB_ENOERR) && (ucRcvAddress == ucMBMasterGetDestAddress(ucPort))) {
                (void)xMBMasterPortEventPost(ucPort, EV_MASTER_EXECUTE);
            } else {
                vMBMasterSetErrorType(ucPort, EV_ERROR_RECEIVE_DATA);
                (void)xMBMasterPortEventPost(ucPort, EV_MASTER_ERROR_PROCESS);
            }
            break;

        case EV_MASTER_EXECUTE:
            ucFunctionCode = ucMBFrame[MB_PDU_FUNC_OFF];
            eException = MB_EX_ILLEGAL_FUNCTION;
            /* If receive frame has exception .The receive function code highest bit is 1.*/
            if (ucFunctionCode >> 7) {
                eException = (eMBException)ucMBFrame[MB_PDU_DATA_OFF];
            } else {
                for (i = 0; i < MB_FUNC_HANDLERS_MAX; i++) {
                    /* No more function handlers registered. Abort. */
                    if (xMasterFuncHandlers[i].ucFunctionCode == 0)	{
                        break;
                    } else if (xMasterFuncHandlers[i].ucFunctionCode == ucFunctionCode) {
                        vMBMasterSetCBRunInMasterMode(ucPort, TRUE);
                        /* If master request is broadcast,
                         * the master need execute function for all slave.
                         */
                        if (pxMBMasterRequestIsBroadcast[ucPort](ucPort)) {
                            usLength = pxMBMasterGetPDUSndLength[ucPort](ucPort);
                            for (j = 1; j <= MB_MASTER_TOTAL_SLAVE_NUM; j++) {
                                vMBMasterSetDestAddress(ucPort, j);
                                eException = xMasterFuncHandlers[i].pxHandler(ucPort, ucMBFrame, &usLength);
                            }
                        } else {
                            eException = xMasterFuncHandlers[i].pxHandler(ucPort, ucMBFrame, &usLength);
                        }
                        vMBMasterSetCBRunInMasterMode(ucPort, FALSE);
                        break;
                    }
                }
            }
            /* If master has exception ,Master will send error process.Otherwise the Master is idle.*/
            if (eException != MB_EX_NONE) {
                vMBMasterSetErrorType(ucPort, EV_ERROR_EXECUTE_FUNCTION);
                (void)xMBMasterPortEventPost(ucPort, EV_MASTER_ERROR_PROCESS);
            } else {
                vMBMasterCBRequestScuuess(ucPort);
                vMBMasterRunResRelease(ucPort);
            }
            break;

        case EV_MASTER_FRAME_SENT:
            /* Master is busy now. */
            pxMBMasterGetPDUSndBuf[ucPort](ucPort, &ucMBFrame);
            eStatus = peMBMasterFrameSendCur[ucPort](ucPort, ucMBMasterGetDestAddress(ucPort), ucMBFrame, pxMBMasterGetPDUSndLength[ucPort](ucPort));
            if (eStatus != MB_ENOERR) {
                (void)xMBMasterPortEventPost(ucPort, EV_MASTER_ERROR_PROCESS);
            }
            break;

        case EV_MASTER_ERROR_PROCESS:
            /* Execute specified error process callback function. */
            errorType = eMBMasterGetErrorType(ucPort);
            pxMBMasterGetPDUSndBuf[ucPort](ucPort, &ucMBFrame);
            switch (errorType) {
            case EV_ERROR_RESPOND_TIMEOUT:
                vMBMasterErrorCBRespondTimeout(ucPort, ucMBMasterGetDestAddress(ucPort),
                                               ucMBFrame, pxMBMasterGetPDUSndLength[ucPort](ucPort));
                break;
            case EV_ERROR_RECEIVE_DATA:
                vMBMasterErrorCBReceiveData(ucPort, ucMBMasterGetDestAddress(ucPort),
                                            ucMBFrame, pxMBMasterGetPDUSndLength[ucPort](ucPort));
                break;
            case EV_ERROR_EXECUTE_FUNCTION:
                vMBMasterErrorCBExecuteFunction(ucPort, ucMBMasterGetDestAddress(ucPort),
                                                ucMBFrame, pxMBMasterGetPDUSndLength[ucPort](ucPort));
                break;
            }
            vMBMasterRunResRelease(ucPort);
            break;
        }
    }
    return MB_ENOERR;
}

/* Get whether the Modbus Master is run in master mode.*/
BOOL xMBMasterGetCBRunInMasterMode(UCHAR ucPort)
{
    return xMBRunInMasterMode[ucPort];
}

/* Set whether the Modbus Master is run in master mode.*/
void vMBMasterSetCBRunInMasterMode(UCHAR ucPort, BOOL IsMasterMode)
{
    xMBRunInMasterMode[ucPort] = IsMasterMode;
}

/* Get Modbus Master send destination address. */
UCHAR ucMBMasterGetDestAddress(UCHAR ucPort)
{
    return ucMBMasterDestAddress[ucPort];
}

/* Set Modbus Master send destination address. */
void vMBMasterSetDestAddress(UCHAR ucPort, UCHAR Address)
{
    ucMBMasterDestAddress[ucPort] = Address;
}

/* Get Modbus Master current error event type. */
eMBMasterErrorEventType eMBMasterGetErrorType(UCHAR ucPort)
{
    return eMBMasterCurErrorType[ucPort];
}

/* Set Modbus Master current error event type. */
void vMBMasterSetErrorType(UCHAR ucPort, eMBMasterErrorEventType errorType)
{
    eMBMasterCurErrorType[ucPort] = errorType;
}

#endif

