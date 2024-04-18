
/* ----------------------- System includes ----------------------------------*/
#include "stdlib.h"
#include "string.h"

/* ----------------------- Platform includes --------------------------------*/
#include "port.h"

/* ----------------------- Modbus includes ----------------------------------*/
#include "mb_m.h"
#include "mbconfig.h"
#include "mbtcp.h"
#include "mbframe.h"
#include "mbport.h"

#ifdef RT_USING_LWIP
#include <lwip/sys.h>
#include <lwip/api.h>
#include <netif/ethernetif.h>
#endif

#if MB_MASTER_TCP_ENABLED > 0

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

static volatile UCHAR  *ucMasterTCPSndBuf[MB_TCP_NUMS];
static volatile USHORT usMasterTCPSendPDULength[MB_TCP_NUMS];
static volatile USHORT usMasterTCPSendTID[MB_TCP_NUMS];

#if MB_MASTER_RTU_ENABLED > 0
static volatile BOOL   xFrameIsBroadcast[MB_TCP_NUMS];
#endif

/* ----------------------- Start implementation -----------------------------*/
eMBErrorCode
eMBMasterTCPDoInit( UCHAR ucPort, USHORT ucTCPPort )
{
    eMBErrorCode    eStatus = MB_ENOERR;
    UCHAR ucMyPort = ucPort-MB_TCP_ID_OFS;

    RT_KERNEL_FREE( (void *)ucMasterTCPSndBuf[ucMyPort] );
    ucMasterTCPSndBuf[ucMyPort] = RT_KERNEL_CALLOC( 256 + MB_TCP_FUNC );
    usMasterTCPSendPDULength[ucMyPort] = 0;
#if MB_MASTER_RTU_ENABLED > 0
    xFrameIsBroadcast[ucMyPort] = FALSE;
#endif

    if( xMBTCPPortInit( ucPort, ucTCPPort ) == FALSE )
    {
        eStatus = MB_EPORTERR;
    }
    return eStatus;    
}

void
eMBMasterTCPStart( UCHAR ucPort )
{
    UCHAR ucMyPort = ucPort-MB_TCP_ID_OFS;
    usMasterTCPSendTID[ucMyPort] = 0;
}

void
eMBMasterTCPStop( UCHAR ucPort )
{
    /* Make sure that no more clients are connected. */
    vMBTCPPortDisable( ucPort );
}

eMBErrorCode
eMBMasterTCPReceive( UCHAR ucPort, UCHAR * pucRcvAddress, UCHAR ** ppucFrame, USHORT * pusLength )
{
    eMBErrorCode    eStatus = MB_EIO;
    UCHAR          *pucMBTCPFrame;
    USHORT          usLength;
    USHORT          usPID;

    if( xMBMasterTCPPortGetResponse( ucPort, &pucMBTCPFrame, &usLength ) != FALSE )
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
            //*pucRcvAddress = MB_TCP_PSEUDO_ADDRESS;
            *pucRcvAddress = pucMBTCPFrame[MB_TCP_UID];
        }
    }
    else
    {
        eStatus = MB_EIO;
    }
    
    vMBMasterPortTimersDisable( ucPort );
    
    return eStatus;
}

eMBErrorCode
eMBMasterTCPSend( UCHAR ucPort, UCHAR ucSlaveAddress, const UCHAR * pucFrame, USHORT usLength )
{
    UCHAR ucMyPort = ucPort-MB_TCP_ID_OFS;
    eMBErrorCode    eStatus = MB_ENOERR;
    UCHAR          *pucMBTCPFrame = ( UCHAR * ) pucFrame - MB_TCP_FUNC;
    USHORT          usTCPLength = usLength + MB_TCP_FUNC;

    /* The MBAP header is already initialized because the caller calls this
     * function with the buffer returned by the previous call. Therefore we 
     * only have to update the length in the header. Note that the length 
     * header includes the size of the Modbus PDU and the UID Byte. Therefore 
     * the length is usLength plus one.
     */
    pucMBTCPFrame[MB_TCP_TID] = usMasterTCPSendTID[ucMyPort]>>8;
    pucMBTCPFrame[MB_TCP_TID + 1] = usMasterTCPSendTID[ucMyPort]&0xFF;
    pucMBTCPFrame[MB_TCP_PID] = MB_TCP_PROTOCOL_ID >> 8U;
    pucMBTCPFrame[MB_TCP_PID + 1] = MB_TCP_PROTOCOL_ID & 0xFF;
    pucMBTCPFrame[MB_TCP_LEN] = ( usLength + 1 ) >> 8U;
    pucMBTCPFrame[MB_TCP_LEN + 1] = ( usLength + 1 ) & 0xFF;
    pucMBTCPFrame[MB_TCP_UID] = ucSlaveAddress;
    usMasterTCPSendTID[ucMyPort]++;
    if( xMBMasterTCPPortSendRequest( ucPort, pucMBTCPFrame, usTCPLength ) == FALSE )
    {
        eStatus = MB_EIO;
    }

    vMBMasterPortTimersRespondTimeoutEnable( ucPort );
    
    return eStatus;
}

BOOL xMBMasterTCPTimerExpired( UCHAR ucPort )
{
	BOOL xNeedPoll = FALSE;

	vMBMasterSetErrorType( ucPort, EV_ERROR_RESPOND_TIMEOUT);
	xNeedPoll = xMBMasterPortEventPost( ucPort, EV_MASTER_ERROR_PROCESS);
	vMBMasterPortTimersDisable( ucPort );

	return xNeedPoll;
}

void vMBMasterTCPGetPDUSndBuf( UCHAR ucPort, UCHAR ** pucFrame )
{
    UCHAR ucMyPort = ucPort-MB_TCP_ID_OFS;
	*pucFrame = ( UCHAR * ) &ucMasterTCPSndBuf[ucMyPort][MB_TCP_FUNC];
}

void vMBMasterTCPSetPDUSndLength( UCHAR ucPort, USHORT SendPDULength )
{
    UCHAR ucMyPort = ucPort-MB_TCP_ID_OFS;
	usMasterTCPSendPDULength[ucMyPort] = SendPDULength;
}

USHORT usMBMasterTCPGetPDUSndLength( UCHAR ucPort )
{
    UCHAR ucMyPort = ucPort-MB_TCP_ID_OFS;
	return usMasterTCPSendPDULength[ucMyPort];
}

BOOL xMBMasterTCPRequestIsBroadcast( UCHAR ucPort )
{
	return FALSE;
}

#if MB_MASTER_RTU_ENABLED > 0

#include "mbcrc.h"

/* ----------------------- Defines ------------------------------------------*/

#define MB_SER_PDU_SIZE_MIN     4       /*!< Minimum size of a Modbus RTU frame. */
#define MB_SER_PDU_SIZE_MAX     256     /*!< Maximum size of a Modbus RTU frame. */
#define MB_SER_PDU_SIZE_CRC     2       /*!< Size of CRC field in PDU. */
#define MB_SER_PDU_ADDR_OFF     0       /*!< Offset of slave address in Ser-PDU. */
#define MB_SER_PDU_PDU_OFF      1       /*!< Offset of Modbus-PDU in Ser-PDU. */

void vMBMasterRTU_OverTCPGetPDUSndBuf( UCHAR ucPort, UCHAR ** pucFrame )
{
    UCHAR ucMyPort = ucPort-MB_TCP_ID_OFS;
	*pucFrame = ( UCHAR * ) &ucMasterTCPSndBuf[ucMyPort][MB_SER_PDU_PDU_OFF];
}


eMBErrorCode
eMBMasterRTU_OverTCPReceive( UCHAR ucPort, UCHAR * pucRcvAddress, UCHAR ** ppucFrame, USHORT * pusLength )
{
    eMBErrorCode    eStatus = MB_ENOERR;
    UCHAR          *pucMBTCPFrame;
    USHORT          usLength;

    if( xMBMasterTCPPortGetResponse( ucPort, &pucMBTCPFrame, &usLength ) != FALSE )
    {
        if( ( usLength >= MB_SER_PDU_SIZE_MIN )
            && ( usMBCRC16( ( UCHAR * ) pucMBTCPFrame, usLength ) == 0 ) )
        {
            /* Save the address field. All frames are passed to the upper layed
             * and the decision if a frame is used is done there.
             */
            *pucRcvAddress = pucMBTCPFrame[MB_SER_PDU_ADDR_OFF];

            /* Total length of Modbus-PDU is Modbus-Serial-Line-PDU minus
             * size of address field and CRC checksum.
             */
            *pusLength = ( USHORT )( usLength - MB_SER_PDU_PDU_OFF - MB_SER_PDU_SIZE_CRC );

            /* Return the start of the Modbus PDU to the caller. */
            *ppucFrame = ( UCHAR * ) & pucMBTCPFrame[MB_SER_PDU_PDU_OFF];
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
    
    vMBMasterPortTimersDisable( ucPort );
    
    return eStatus;
}

eMBErrorCode
eMBMasterRTU_OverTCPSend( UCHAR ucPort, UCHAR ucSlaveAddress, const UCHAR * pucFrame, USHORT usLength )
{
    UCHAR ucMyPort = ucPort-MB_TCP_ID_OFS;
    eMBErrorCode    eStatus = MB_ENOERR;
    USHORT          usCRC16;
    UCHAR           *pucMasterSndBufferCur;

    if ( ucSlaveAddress > MB_MASTER_TOTAL_SLAVE_NUM ) return MB_EINVAL;
    
    /* First byte before the Modbus-PDU is the slave address. */
    pucMasterSndBufferCur = ( UCHAR * ) pucFrame - 1;
    usMasterTCPSendPDULength[ucMyPort] = 1;

    /* Now copy the Modbus-PDU into the Modbus-Serial-Line-PDU. */
    pucMasterSndBufferCur[MB_SER_PDU_ADDR_OFF] = ucSlaveAddress;
    usMasterTCPSendPDULength[ucMyPort] += usLength;

    /* Calculate CRC16 checksum for Modbus-Serial-Line-PDU. */
    usCRC16 = usMBCRC16( ( UCHAR * ) pucMasterSndBufferCur, usMasterTCPSendPDULength[ucMyPort] );
    ucMasterTCPSndBuf[ucMyPort][usMasterTCPSendPDULength[ucMyPort]++] = ( UCHAR )( usCRC16 & 0xFF );
    ucMasterTCPSndBuf[ucMyPort][usMasterTCPSendPDULength[ucMyPort]++] = ( UCHAR )( usCRC16 >> 8 );
    {
        xFrameIsBroadcast[ucMyPort] = ( ucMasterTCPSndBuf[ucMyPort][MB_SER_PDU_ADDR_OFF] == MB_ADDRESS_BROADCAST ) ? TRUE : FALSE;
        if ( xFrameIsBroadcast[ucMyPort] == TRUE ) {
        	vMBMasterPortTimersConvertDelayEnable( ucPort );
        } else {
        	vMBMasterPortTimersRespondTimeoutEnable( ucPort );
        }
    }
    if( xMBMasterTCPPortSendRequest( ucPort, ucMasterTCPSndBuf[ucMyPort], usMasterTCPSendPDULength[ucMyPort] ) == FALSE )
    {
        eStatus = MB_EIO;
    }
    
    return eStatus;
}
#endif

#endif
