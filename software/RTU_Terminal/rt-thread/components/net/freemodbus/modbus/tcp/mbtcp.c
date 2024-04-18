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
 * File: $Id: mbtcp.c,v 1.3 2006/12/07 22:10:34 wolti Exp $
 */

/* ----------------------- System includes ----------------------------------*/
#include "stdlib.h"
#include "string.h"

/* ----------------------- Platform includes --------------------------------*/
#include "port.h"

/* ----------------------- Modbus includes ----------------------------------*/
#include "mb.h"
#include "mbconfig.h"
#include "mbtcp.h"
#include "mbframe.h"
#include "mbport.h"

#ifdef RT_USING_LWIP
#include <lwip/sys.h>
#include <lwip/api.h>
#include <netif/ethernetif.h>
#endif

#if MB_SLAVE_TCP_ENABLED > 0

/* ----------------------- Defines ------------------------------------------*/

/* ----------------------- MBAP Header --------------------------------------*/
/*
 *
 * <------------------------ MODBUS TCP/IP ADU(1) ------------------------->
 *              <----------- MODBUS PDU (1') ---------------->
 *  +-----------+---------------+------------------------------------------+
 *  | TID | PID | Length | UID  |Code | Data                               |
 *  +-----------+---------------+------------------------------------------+
 *  |     |     |        |      |                                           
 * (2)   (3)   (4)      (5)    (6)                                          
 *
 * (2)  ... MB_TCP_TID          = 0 (Transaction Identifier - 2 Byte) 
 * (3)  ... MB_TCP_PID          = 2 (Protocol Identifier - 2 Byte)
 * (4)  ... MB_TCP_LEN          = 4 (Number of bytes - 2 Byte)
 * (5)  ... MB_TCP_UID          = 6 (Unit Identifier - 1 Byte)
 * (6)  ... MB_TCP_FUNC         = 7 (Modbus Function Code)
 *
 * (1)  ... Modbus TCP/IP Application Data Unit
 * (1') ... Modbus Protocol Data Unit
 */

#define MB_TCP_TID          0
#define MB_TCP_PID          2
#define MB_TCP_LEN          4
#define MB_TCP_UID          6
#define MB_TCP_FUNC         7

#define MB_TCP_PROTOCOL_ID  0   /* 0 = Modbus Protocol */


/* ----------------------- Start implementation -----------------------------*/
eMBErrorCode
eMBTCPDoInit( UCHAR ucPort, USHORT ucTCPPort )
{
    eMBErrorCode    eStatus = MB_ENOERR;

    if( xMBTCPPortInit( ucPort, ucTCPPort ) == FALSE )
    {
        eStatus = MB_EPORTERR;
    }
    return eStatus;
}

void
eMBTCPStart( UCHAR ucPort )
{
}

void
eMBTCPStop( UCHAR ucPort )
{
    /* Make sure that no more clients are connected. */
    vMBTCPPortDisable( ucPort );
}

eMBErrorCode
eMBTCPReceive( UCHAR ucPort, UCHAR * pucRcvAddress, UCHAR ** ppucFrame, USHORT * pusLength )
{
    eMBErrorCode    eStatus = MB_EIO;
    UCHAR          *pucMBTCPFrame;
    USHORT          usLength;
    USHORT          usPID;

    if( xMBTCPPortGetRequest( ucPort, &pucMBTCPFrame, &usLength ) != FALSE )
    {
        usPID = pucMBTCPFrame[MB_TCP_PID] << 8U;
        usPID |= pucMBTCPFrame[MB_TCP_PID + 1];

        if( usPID == MB_TCP_PROTOCOL_ID )
        {
            *ppucFrame = &pucMBTCPFrame[MB_TCP_FUNC];
            *pusLength = usLength - MB_TCP_FUNC;
            eStatus = MB_ENOERR;

            /* Modbus TCP does not use any addresses. Fake the source address such
             * that the processing part deals with this frame.
             */
            *pucRcvAddress = MB_TCP_PSEUDO_ADDRESS;
        }
    }
    else
    {
        eStatus = MB_EIO;
    }
    return eStatus;
}

eMBErrorCode
eMBTCPSend( UCHAR ucPort, UCHAR _unused, const UCHAR * pucFrame, USHORT usLength )
{
    eMBErrorCode    eStatus = MB_ENOERR;
    UCHAR          *pucMBTCPFrame = ( UCHAR * ) pucFrame - MB_TCP_FUNC;
    USHORT          usTCPLength = usLength + MB_TCP_FUNC;

    /* The MBAP header is already initialized because the caller calls this
     * function with the buffer returned by the previous call. Therefore we 
     * only have to update the length in the header. Note that the length 
     * header includes the size of the Modbus PDU and the UID Byte. Therefore 
     * the length is usLength plus one.
     */
    pucMBTCPFrame[MB_TCP_LEN] = ( usLength + 1 ) >> 8U;
    pucMBTCPFrame[MB_TCP_LEN + 1] = ( usLength + 1 ) & 0xFF;
    if( xMBTCPPortSendResponse( ucPort, pucMBTCPFrame, usTCPLength ) == FALSE )
    {
        eStatus = MB_EIO;
    }
    return eStatus;
}

#if MB_SLAVE_RTU_ENABLED > 0

#include "mbcrc.h"

/* ----------------------- Defines ------------------------------------------*/
#define MB_SER_PDU_SIZE_MIN     4       /*!< Minimum size of a Modbus RTU frame. */
#define MB_SER_PDU_SIZE_MAX     256     /*!< Maximum size of a Modbus RTU frame. */
#define MB_SER_PDU_SIZE_CRC     2       /*!< Size of CRC field in PDU. */
#define MB_SER_PDU_ADDR_OFF     0       /*!< Offset of slave address in Ser-PDU. */
#define MB_SER_PDU_PDU_OFF      1       /*!< Offset of Modbus-PDU in Ser-PDU. */

eMBErrorCode
eMBRTU_OverTCPReceive( UCHAR ucPort, UCHAR * pucRcvAddress, UCHAR ** ppucFrame, USHORT * pusLength )
{
    eMBErrorCode    eStatus = MB_ENOERR;
    UCHAR          *pucMBTCPFrame = NULL;
    USHORT          usLength;

    if( xMBTCPPortGetRequest( ucPort, &pucMBTCPFrame, &usLength ) != FALSE )
    {
        if( !pucMBTCPFrame ) return MB_EILLSTATE;
        if( ( usLength >= MB_SER_PDU_SIZE_MIN )
            && ( usMBCRC16( ( UCHAR * ) pucMBTCPFrame, usLength ) == 0 ) )
        {
            *pucRcvAddress = pucMBTCPFrame[MB_SER_PDU_ADDR_OFF];
            *pusLength = ( USHORT )( usLength - MB_SER_PDU_PDU_OFF - MB_SER_PDU_SIZE_CRC );
            *ppucFrame = ( UCHAR * ) &pucMBTCPFrame[MB_SER_PDU_PDU_OFF];
        }
        else
        {
            eStatus = MB_EIO;
        }
    }
    else
    {
        eStatus = MB_EIO;
    }
    return eStatus;
}

eMBErrorCode
eMBRTU_OverTCPSend( UCHAR ucPort, UCHAR ucSlaveAddress, const UCHAR * pucFrame, USHORT usLength )
{
    eMBErrorCode    eStatus = MB_ENOERR;
    USHORT          usCRC16;
    UCHAR           *pucSndBufferCur;
    UCHAR           *puSendFrame;
    USHORT          usSndBufferCount;
    
    if( !pucFrame ) return MB_EILLSTATE;
    puSendFrame = ( UCHAR * ) pucFrame - 1;

    /* First byte before the Modbus-PDU is the slave address. */
    pucSndBufferCur = ( UCHAR * ) pucFrame - 1;
    usSndBufferCount = 1;

    /* Now copy the Modbus-PDU into the Modbus-Serial-Line-PDU. */
    pucSndBufferCur[MB_SER_PDU_ADDR_OFF] = ucSlaveAddress;
    usSndBufferCount += usLength;

    /* Calculate CRC16 checksum for Modbus-Serial-Line-PDU. */
    usCRC16 = usMBCRC16( ( UCHAR * ) pucSndBufferCur, usSndBufferCount );
    puSendFrame[usSndBufferCount++] = ( UCHAR )( usCRC16 & 0xFF );
    puSendFrame[usSndBufferCount++] = ( UCHAR )( usCRC16 >> 8 );

    if( xMBTCPPortSendResponse( ucPort, puSendFrame, usSndBufferCount ) == FALSE )
    {
        eStatus = MB_EIO;
    }
    return eStatus;
}

#endif

#endif
