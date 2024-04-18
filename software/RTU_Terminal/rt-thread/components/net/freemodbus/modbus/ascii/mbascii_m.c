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
 * File: $Id: mbascii.c,v 1.15 2007/02/18 23:46:48 wolti Exp $
 */

/* ----------------------- System includes ----------------------------------*/
#include "stdlib.h"
#include "string.h"

/* ----------------------- Platform includes --------------------------------*/
#include "port.h"

/* ----------------------- Modbus includes ----------------------------------*/
#include "mb_m.h"
#include "mbconfig.h"
#include "mbascii.h"
#include "mbframe.h"

#include "mbcrc.h"
#include "mbport.h"

#if MB_MASTER_ASCII_ENABLED > 0

/* ----------------------- Defines ------------------------------------------*/
#define MB_ASCII_DEFAULT_CR     '\r'    /*!< Default CR character for Modbus ASCII. */
#define MB_ASCII_DEFAULT_LF     '\n'    /*!< Default LF character for Modbus ASCII. */
#define MB_SER_PDU_SIZE_MIN     3       /*!< Minimum size of a Modbus ASCII frame. */
#define MB_SER_PDU_SIZE_MAX     256     /*!< Maximum size of a Modbus ASCII frame. */
#define MB_SER_PDU_SIZE_LRC     1       /*!< Size of LRC field in PDU. */
#define MB_SER_PDU_ADDR_OFF     0       /*!< Offset of slave address in Ser-PDU. */
#define MB_SER_PDU_PDU_OFF      1       /*!< Offset of Modbus-PDU in Ser-PDU. */

/* ----------------------- Type definitions ---------------------------------*/
typedef enum
{
    STATE_M_RX_IDLE,            /*!< Receiver is in idle state. */
    STATE_M_RX_RCV,             /*!< Frame is beeing received. */
    STATE_M_RX_WAIT_EOF         /*!< Wait for End of Frame. */
} eMBMasterRcvState;

typedef enum
{
    STATE_M_TX_IDLE,            /*!< Transmitter is in idle state. */
    STATE_M_TX_START,           /*!< Starting transmission (':' sent). */
    STATE_M_TX_DATA,            /*!< Sending of data (Address, Data, LRC). */
    STATE_M_TX_END,             /*!< End of transmission. */
    STATE_M_TX_NOTIFY,          /*!< Notify sender that the frame has been sent. */
    STATE_M_TX_XFWR,            /*!< Transmitter is in transfer finish and wait receive state. */
} eMBMasterSndState;

typedef enum
{
    BYTE_HIGH_NIBBLE,           /*!< Character for high nibble of byte. */
    BYTE_LOW_NIBBLE             /*!< Character for low nibble of byte. */
} eMBBytePos;

/* ----------------------- Static functions ---------------------------------*/
static UCHAR    prvucMBCHAR2BIN( UCHAR ucCharacter );

static UCHAR    prvucMBBIN2CHAR( UCHAR ucByte );

static UCHAR    prvucMBLRC( UCHAR * pucFrame, USHORT usLen );

/* ----------------------- Static variables ---------------------------------*/
static volatile eMBMasterSndState eSndState[BOARD_UART_MAX];
static volatile eMBMasterRcvState eRcvState[BOARD_UART_MAX];

static volatile UCHAR  *ucASCIISndBuf[BOARD_UART_MAX];
static volatile UCHAR  *ucASCIIRcvBuf[BOARD_UART_MAX];
static volatile USHORT usASCIISendPDULength[BOARD_UART_MAX];

static volatile USHORT usRcvBufferPos[BOARD_UART_MAX];
static volatile eMBBytePos eBytePos[BOARD_UART_MAX];

static volatile UCHAR *pucSndBufferCur[BOARD_UART_MAX];
static volatile USHORT usSndBufferCount[BOARD_UART_MAX];

static volatile BOOL   xASCIIIsBroadcast[BOARD_UART_MAX];

/* ----------------------- Start implementation -----------------------------*/
eMBErrorCode
eMBMasterASCIIInit( UCHAR ucPort, ULONG ulBaudRate, eMBParity eParity )
{
    eMBErrorCode    eStatus = MB_ENOERR;

    RT_KERNEL_FREE( (void *)ucASCIISndBuf[ucPort] );
    RT_KERNEL_FREE( (void *)ucASCIIRcvBuf[ucPort] );
    ucASCIISndBuf[ucPort] = RT_KERNEL_CALLOC( MB_SER_PDU_SIZE_MAX );
    ucASCIIRcvBuf[ucPort] = RT_KERNEL_CALLOC( MB_SER_PDU_SIZE_MAX );
    xASCIIIsBroadcast[ucPort] = FALSE;
    
    MASTER_ENTER_CRITICAL_SECTION(  );

    if( xMBMasterPortSerialInit( ucPort, ulBaudRate, 7, eParity ) != TRUE )
    {
        eStatus = MB_EPORTERR;
    }
    else if( xMBMasterPortTimersT1SInit( ucPort, MB_ASCII_TIMEOUT_SEC ) != TRUE )
    {
        eStatus = MB_EPORTERR;
    }

    MASTER_EXIT_CRITICAL_SECTION(  );

    return eStatus;
}

void
eMBMasterASCIIStart( UCHAR ucPort )
{
    MASTER_ENTER_CRITICAL_SECTION(  );
    vMBMasterPortSerialEnable( ucPort, TRUE, FALSE );
    eRcvState[ucPort] = STATE_M_RX_IDLE;
    MASTER_EXIT_CRITICAL_SECTION(  );

    /* No special startup required for ASCII. */
    ( void )xMBMasterPortEventPost( ucPort, EV_MASTER_READY );
}

void
eMBMasterASCIIStop( UCHAR ucPort )
{
    MASTER_ENTER_CRITICAL_SECTION(  );
    vMBMasterPortSerialEnable( ucPort, FALSE, FALSE );
    vMBMasterPortTimersDisable( ucPort );
    MASTER_EXIT_CRITICAL_SECTION(  );
}

eMBErrorCode
eMBMasterASCIIReceive( UCHAR ucPort, UCHAR * pucRcvAddress, UCHAR ** pucFrame, USHORT * pusLength )
{
    eMBErrorCode    eStatus = MB_ENOERR;

    MASTER_ENTER_CRITICAL_SECTION(  );
    assert( usRcvBufferPos[ucPort] < MB_SER_PDU_SIZE_MAX );

    /* Length and CRC check */
    if( ( usRcvBufferPos[ucPort] >= MB_SER_PDU_SIZE_MIN )
        && ( prvucMBLRC( ( UCHAR * ) ucASCIIRcvBuf[ucPort], usRcvBufferPos[ucPort] ) == 0 ) )
    {
        /* Save the address field. All frames are passed to the upper layed
         * and the decision if a frame is used is done there.
         */
        *pucRcvAddress = ucASCIIRcvBuf[ucPort][MB_SER_PDU_ADDR_OFF];

        /* Total length of Modbus-PDU is Modbus-Serial-Line-PDU minus
         * size of address field and CRC checksum.
         */
        *pusLength = ( USHORT )( usRcvBufferPos[ucPort] - MB_SER_PDU_PDU_OFF - MB_SER_PDU_SIZE_LRC );

        /* Return the start of the Modbus PDU to the caller. */
        *pucFrame = ( UCHAR * ) & ucASCIIRcvBuf[ucPort][MB_SER_PDU_PDU_OFF];
    }
    else
    {
        eStatus = MB_EIO;
    }
    MASTER_EXIT_CRITICAL_SECTION(  );
    return eStatus;
}

eMBErrorCode
eMBMasterASCIISend( UCHAR ucPort, UCHAR ucSlaveAddress, const UCHAR * pucFrame, USHORT usLength )
{
    eMBErrorCode    eStatus = MB_ENOERR;
    UCHAR           usLRC;
    
    if ( ucSlaveAddress > MB_MASTER_TOTAL_SLAVE_NUM ) return MB_EINVAL;
    
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
        pucSndBufferCur[ucPort] = ( UCHAR * ) pucFrame - 1;
        usSndBufferCount[ucPort] = 1;

        /* Now copy the Modbus-PDU into the Modbus-Serial-Line-PDU. */
        pucSndBufferCur[ucPort][MB_SER_PDU_ADDR_OFF] = ucSlaveAddress;
        usSndBufferCount[ucPort] += usLength;

        /* Calculate LRC checksum for Modbus-Serial-Line-PDU. */
        usLRC = prvucMBLRC( ( UCHAR * ) pucSndBufferCur[ucPort], usSndBufferCount[ucPort] );
        ucASCIISndBuf[ucPort][usSndBufferCount[ucPort]++] = usLRC;

        xASCIIIsBroadcast[ucPort] = ( ucASCIISndBuf[ucPort][MB_SER_PDU_ADDR_OFF] == MB_ADDRESS_BROADCAST ) ? TRUE : FALSE;
        /* Activate the transmitter. */
        eSndState[ucPort] = STATE_M_TX_START;
        vMBMasterPortSerialEnable( ucPort, FALSE, TRUE );

        if ( xASCIIIsBroadcast[ucPort] == TRUE ) {
        	vMBMasterPortTimersConvertDelayEnable( ucPort );
        } else {
        	vMBMasterPortTimersRespondTimeoutEnable( ucPort );
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
xMBMasterASCIIReceiveFSM( UCHAR ucPort )
{
    BOOL            xNeedPoll = FALSE;
    UCHAR           ucByte;
    UCHAR           ucResult;

    assert( eSndState[ucPort] == STATE_M_TX_IDLE && eSndState[ucPort] != STATE_M_TX_XFWR  );

    ( void )xMBMasterPortSerialGetByte( ucPort, ( CHAR * ) & ucByte );
    switch ( eRcvState[ucPort] )
    {
        /* A new character is received. If the character is a ':' the input
         * buffer is cleared. A CR-character signals the end of the data
         * block. Other characters are part of the data block and their
         * ASCII value is converted back to a binary representation.
         */
    case STATE_M_RX_RCV:
        /* Enable timer for character timeout. */
        vMBMasterPortTimersT1SEnable( ucPort );
        if( ucByte == ':' )
        {
            /* Empty receive buffer. */
            eBytePos[ucPort] = BYTE_HIGH_NIBBLE;
            usRcvBufferPos[ucPort] = 0;
        }
        else if( ucByte == MB_ASCII_DEFAULT_CR )
        {
            eRcvState[ucPort] = STATE_M_RX_WAIT_EOF;
        }
        else
        {
            ucResult = prvucMBCHAR2BIN( ucByte );
            switch ( eBytePos[ucPort] )
            {
                /* High nibble of the byte comes first. We check for
                 * a buffer overflow here. */
            case BYTE_HIGH_NIBBLE:
                if( usRcvBufferPos[ucPort] < MB_SER_PDU_SIZE_MAX )
                {
                    ucASCIIRcvBuf[ucPort][usRcvBufferPos[ucPort]] = ( UCHAR )( ucResult << 4 );
                    eBytePos[ucPort] = BYTE_LOW_NIBBLE;
                    break;
                }
                else
                {
                    /* not handled in Modbus specification but seems
                     * a resonable implementation. */
                    eRcvState[ucPort] = STATE_M_RX_IDLE;
                    /* Disable previously activated timer because of error state. */
                    vMBMasterPortTimersDisable( ucPort );
                }
                break;

            case BYTE_LOW_NIBBLE:
                ucASCIIRcvBuf[ucPort][usRcvBufferPos[ucPort]++] |= ucResult;
                eBytePos[ucPort] = BYTE_HIGH_NIBBLE;
                break;
            }
        }
        break;

    case STATE_M_RX_WAIT_EOF:
        if( ucByte == MB_ASCII_DEFAULT_LF )
        {
            /* Disable character timeout timer because all characters are
             * received. */
            vMBMasterPortTimersDisable( ucPort );
            /* Receiver is again in idle state. */
            eRcvState[ucPort]= STATE_M_RX_IDLE;

            /* Notify the caller of eMBASCIIReceive that a new frame
             * was received. */
            xNeedPoll = xMBMasterPortEventPost( ucPort, EV_MASTER_FRAME_RECEIVED );
        }
        else if( ucByte == ':' )
        {
            /* Empty receive buffer and back to receive state. */
            eBytePos[ucPort] = BYTE_HIGH_NIBBLE;
            usRcvBufferPos[ucPort] = 0;
            eRcvState[ucPort] = STATE_M_RX_RCV;

            /* Enable timer for character timeout. */
            vMBMasterPortTimersT1SEnable( ucPort );
        }
        else
        {
            /* Frame is not okay. Delete entire frame. */
            eRcvState[ucPort] = STATE_M_RX_IDLE;
        }
        break;

    case STATE_M_RX_IDLE:
        if( ucByte == ':' )
        {
        	/* In time of respond timeout,the receiver receive a frame.
        	 * Disable timer of respond timeout and change the transmiter state to idle.
        	 */
        	eSndState[ucPort] = STATE_M_TX_IDLE;
        	vMBMasterPortTimersDisable( ucPort );
        	
            /* Enable timer for character timeout. */
            vMBMasterPortTimersT1SEnable( ucPort );
            /* Reset the input buffers to store the frame. */
            usRcvBufferPos[ucPort] = 0;
            eBytePos[ucPort] = BYTE_HIGH_NIBBLE;
            eRcvState[ucPort] = STATE_M_RX_RCV;
        }
        break;
    }

    return xNeedPoll;
}

BOOL
xMBMasterASCIITransmitFSM( UCHAR ucPort )
{
    BOOL            xNeedPoll = FALSE;
    UCHAR           ucByte;

    assert( eRcvState[ucPort] == STATE_M_RX_IDLE );
    switch ( eSndState[ucPort] )
    {
    case STATE_M_TX_START:
    case STATE_M_TX_DATA:
    case STATE_M_TX_END:
    case STATE_M_TX_NOTIFY:
        if ( xASCIIIsBroadcast[ucPort] == TRUE ) {
        	vMBMasterPortTimersConvertDelayEnable( ucPort );
        } else {
        	vMBMasterPortTimersRespondTimeoutEnable( ucPort );
        }
        break;
    default:
    }
    
    switch ( eSndState[ucPort] )
    {
        /* Start of transmission. The start of a frame is defined by sending
         * the character ':'. */
    case STATE_M_TX_START:
        ucByte = ':';
        xMBMasterPortSerialPutByte( ucPort, ( CHAR )ucByte );
        eSndState[ucPort] = STATE_M_TX_DATA;
        eBytePos[ucPort] = BYTE_HIGH_NIBBLE;
        break;

        /* Send the data block. Each data byte is encoded as a character hex
         * stream with the high nibble sent first and the low nibble sent
         * last. If all data bytes are exhausted we send a '\r' character
         * to end the transmission. */
    case STATE_M_TX_DATA:
        if( usSndBufferCount[ucPort] > 0 )
        {
            switch ( eBytePos[ucPort] )
            {
            case BYTE_HIGH_NIBBLE:
                ucByte = prvucMBBIN2CHAR( ( UCHAR )( *pucSndBufferCur[ucPort] >> 4 ) );
                xMBMasterPortSerialPutByte( ucPort, ( CHAR ) ucByte );
                eBytePos[ucPort] = BYTE_LOW_NIBBLE;
                break;

            case BYTE_LOW_NIBBLE:
                ucByte = prvucMBBIN2CHAR( ( UCHAR )( *pucSndBufferCur[ucPort] & 0x0F ) );
                xMBMasterPortSerialPutByte( ucPort, ( CHAR )ucByte );
                pucSndBufferCur[ucPort]++;
                eBytePos[ucPort] = BYTE_HIGH_NIBBLE;
                usSndBufferCount[ucPort]--;
                break;
            }
        }
        else
        {
            xMBMasterPortSerialPutByte( ucPort, MB_ASCII_DEFAULT_CR );
            eSndState[ucPort] = STATE_M_TX_END;
        }
        break;

        /* Finish the frame by sending a LF character. */
    case STATE_M_TX_END:
        xMBMasterPortSerialPutByte( ucPort, MB_ASCII_DEFAULT_LF );
        /* We need another state to make sure that the CR character has
         * been sent. */
        eSndState[ucPort] = STATE_M_TX_NOTIFY;
        break;

        /* Notify the task which called eMBASCIISend that the frame has
         * been sent. */
    case STATE_M_TX_NOTIFY:
            /* Disable transmitter. This prevents another transmit buffer
             * empty interrupt. */
            eSndState[ucPort] = STATE_M_TX_XFWR;
            vMBMasterPortSerialEnable( ucPort, TRUE, FALSE );
        break;

        /* We should not get a transmitter event if the transmitter is in
         * idle state.  */
    case STATE_M_TX_IDLE:
        /* enable receiver/disable transmitter. */
        vMBMasterPortSerialEnable( ucPort, TRUE, FALSE );
        break;
    }

    return xNeedPoll;
}

BOOL
xMBMasterASCIITimerT1SExpired( UCHAR ucPort )
{
	BOOL xNeedPoll = FALSE;
    switch ( eRcvState[ucPort] )
    {
        /* If we have a timeout we go back to the idle state and wait for
         * the next frame.
         */
    case STATE_M_RX_RCV:
    case STATE_M_RX_WAIT_EOF:
        eRcvState[ucPort] = STATE_M_RX_IDLE;
        break;

    default:
        assert( ( eRcvState[ucPort] == STATE_M_RX_RCV ) || ( eRcvState[ucPort] == STATE_M_RX_WAIT_EOF ) );
        break;
    }

	switch ( eSndState[ucPort] )
	{
		/* A frame was send finish and convert delay or respond timeout expired.
		 * If the frame is broadcast,The master will idle,and if the frame is not
		 * broadcast.Notify the listener process error.*/
	case STATE_M_TX_XFWR:
		if ( xASCIIIsBroadcast[ucPort] == FALSE ) {
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
    
    vMBMasterPortTimersDisable( ucPort );

    // 如果是广播, 由这里 post EV_MASTER_EXECUTE
    if (eMBMasterGetCurTimerMode(ucPort) == MB_TMODE_CONVERT_DELAY) {
		xNeedPoll = xMBMasterPortEventPost( ucPort, EV_MASTER_EXECUTE );
	}

    /* no context switch required. */
    return xNeedPoll;
}


static          UCHAR
prvucMBCHAR2BIN( UCHAR ucCharacter )
{
    if( ( ucCharacter >= '0' ) && ( ucCharacter <= '9' ) )
    {
        return ( UCHAR )( ucCharacter - '0' );
    }
    else if( ( ucCharacter >= 'A' ) && ( ucCharacter <= 'F' ) )
    {
        return ( UCHAR )( ucCharacter - 'A' + 0x0A );
    }
    else
    {
        return 0xFF;
    }
}

static          UCHAR
prvucMBBIN2CHAR( UCHAR ucByte )
{
    if( ucByte <= 0x09 )
    {
        return ( UCHAR )( '0' + ucByte );
    }
    else if( ( ucByte >= 0x0A ) && ( ucByte <= 0x0F ) )
    {
        return ( UCHAR )( ucByte - 0x0A + 'A' );
    }
    else
    {
        /* Programming error. */
        assert( 0 );
    }
    return '0';
}


static          UCHAR
prvucMBLRC( UCHAR * pucFrame, USHORT usLen )
{
    UCHAR           ucLRC = 0;  /* LRC char initialized */

    while( usLen-- )
    {
        ucLRC += *pucFrame++;   /* Add buffer byte without carry */
    }

    /* Return twos complement */
    ucLRC = ( UCHAR ) ( -( ( CHAR ) ucLRC ) );
    return ucLRC;
}

/* Get Modbus Master send PDU's buffer address pointer.*/
void vMBMasterASCIIGetPDUSndBuf( UCHAR ucPort, UCHAR ** pucFrame )
{
	*pucFrame = ( UCHAR * ) &ucASCIISndBuf[ucPort][MB_SER_PDU_PDU_OFF];
}

/* Set Modbus Master send PDU's buffer length.*/
void vMBMasterASCIISetPDUSndLength( UCHAR ucPort, USHORT SendPDULength )
{
	usASCIISendPDULength[ucPort] = SendPDULength;
}

/* Get Modbus Master send PDU's buffer length.*/
USHORT usMBMasterASCIIGetPDUSndLength( UCHAR ucPort )
{
	return usASCIISendPDULength[ucPort];
}

/* The master request is broadcast? */
BOOL xMBMasterASCIIRequestIsBroadcast( UCHAR ucPort )
{
	return xASCIIIsBroadcast[ucPort];
}

#endif
