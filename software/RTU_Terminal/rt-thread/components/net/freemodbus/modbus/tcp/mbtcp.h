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
 * File: $Id: mbtcp.h,v 1.2 2006/12/07 22:10:34 wolti Exp $
 */

#ifndef _MB_TCP_H
#define _MB_TCP_H

#ifdef __cplusplus
PR_BEGIN_EXTERN_C
#endif

/* ----------------------- Defines ------------------------------------------*/
#define MB_TCP_PSEUDO_ADDRESS   255

/* ----------------------- Function prototypes ------------------------------*/
    eMBErrorCode eMBTCPDoInit( UCHAR ucPort, USHORT ucTCPPort );
void            eMBTCPStart( UCHAR ucPort );
void            eMBTCPStop( UCHAR ucPort );
eMBErrorCode    eMBTCPReceive( UCHAR ucPort, UCHAR * pucRcvAddress, UCHAR ** pucFrame,
                               USHORT * pusLength );
eMBErrorCode    eMBTCPSend( UCHAR ucPort, UCHAR _unused, const UCHAR * pucFrame,
                            USHORT usLength );

#if MB_MASTER_TCP_ENABLED > 0
eMBErrorCode eMBMasterTCPDoInit( UCHAR ucPort, USHORT ucTCPPort );
void eMBMasterTCPStart( UCHAR ucPort );
void eMBMasterTCPStop( UCHAR ucPort );
eMBErrorCode eMBMasterTCPReceive( UCHAR ucPort, UCHAR * pucRcvAddress, UCHAR ** ppucFrame, USHORT * pusLength );
eMBErrorCode eMBMasterTCPSend( UCHAR ucPort, UCHAR ucSlaveAddress, const UCHAR * pucFrame, USHORT usLength );
void vMBMasterTCPGetPDUSndBuf( UCHAR ucPort, UCHAR ** pucFrame );
void vMBMasterTCPSetPDUSndLength( UCHAR ucPort, USHORT SendPDULength );
USHORT usMBMasterTCPGetPDUSndLength( UCHAR ucPort );
BOOL xMBMasterTCPRequestIsBroadcast( UCHAR ucPort );
BOOL xMBMasterTCPTimerExpired( UCHAR ucPort );

#if MB_MASTER_TCP_ENABLED > 0

eMBErrorCode eMBRTU_OverTCPReceive( UCHAR ucPort, UCHAR * pucRcvAddress, UCHAR ** ppucFrame, USHORT * pusLength );
eMBErrorCode eMBRTU_OverTCPSend( UCHAR ucPort, UCHAR ucSlaveAddress, const UCHAR * pucFrame, USHORT usLength );

void vMBMasterRTU_OverTCPGetPDUSndBuf( UCHAR ucPort, UCHAR ** pucFrame );
eMBErrorCode eMBMasterRTU_OverTCPReceive( UCHAR ucPort, UCHAR * pucRcvAddress, UCHAR ** ppucFrame, USHORT * pusLength );
eMBErrorCode eMBMasterRTU_OverTCPSend( UCHAR ucPort, UCHAR ucSlaveAddress, const UCHAR * pucFrame, USHORT usLength );
#endif
#endif

#ifdef __cplusplus
PR_END_EXTERN_C
#endif
#endif
