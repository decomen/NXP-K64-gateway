/*
* FreeModbus Libary: lwIP Port
* Copyright (C) 2006 Christian Walter <wolti@sil.at>
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
* File: $Id: porttcp.c,v 1.1 2006/08/30 23:18:07 wolti Exp $
*/

/* ----------------------- Modbus includes ----------------------------------*/
#include "mb.h"
#include "mbport.h"
#include "port.h"
#include "rtdevice.h"
#include "board.h"
#include <stdio.h>
#include "net_helper.h"

#if MB_MASTER_RTU_ENABLED > 0
#include "mb_m.h"
#endif

/* ----------------------- MBAP Header --------------------------------------*/
#define MB_TCP_UID          6
#define MB_TCP_LEN          4
#define MB_TCP_FUNC         7

/* ----------------------- Defines  -----------------------------------------*/

#define MB_TCP_DEFAULT_PORT  502          /* TCP listening port. */
#define MB_TCP_BUF_SIZE     ( 256 + 7 )   /* Must hold a complete Modbus TCP frame. */

/* ----------------------- Static variables ---------------------------------*/

/* ----------------------- Static functions ---------------------------------*/


/* ----------------------- Begin implementation -----------------------------*/
// usTCPPort use with g_tcpip_cfg[ucMyPort]

BOOL xMBTCPIsPortConnected(UCHAR ucPort)
{
    net_helper_t *net = net_get_helper(ucPort-MB_TCP_ID_OFS);
    return (net && net->tcp_fd >= 0);
}

BOOL
xMBTCPPortInit( UCHAR ucPort, USHORT usTCPPort )
{
    UCHAR ucMyPort = ucPort-MB_TCP_ID_OFS;
    
    vMBTCPPortClose( ucPort );

    net_open(ucMyPort, MB_TCP_BUF_SIZE, 0x400, "mbtcp");

    return TRUE;
}

void 
vMBTCPPortClose( UCHAR ucPort )
{
    net_close(ucPort-MB_TCP_ID_OFS);
}

void
vMBTCPPortDisable( UCHAR ucPort )
{
    
}

BOOL
xMBTCPPortGetRequest( UCHAR ucPort, UCHAR ** ppucMBTCPFrame, USHORT * usTCPLength )
{
    net_helper_t *net = net_get_helper(ucPort-MB_TCP_ID_OFS);
    if(net) {
        *ppucMBTCPFrame = net->buffer;
        *usTCPLength = net->pos;
        /* Reset the buffer. */
        net->pos = 0;
    }
    return TRUE;
}

BOOL
xMBTCPPortSendResponse( UCHAR ucPort, const UCHAR * pucMBTCPFrame, USHORT usTCPLength )
{
    return (net_send(ucPort-MB_TCP_ID_OFS, (const rt_uint8_t *)pucMBTCPFrame, usTCPLength) > 0);
}

#if MB_MASTER_RTU_ENABLED > 0
BOOL
xMBMasterTCPPortSendRequest( UCHAR ucPort, const UCHAR * pucMBTCPFrame, USHORT usTCPLength )
{
    return (net_send(ucPort-MB_TCP_ID_OFS, (const rt_uint8_t *)pucMBTCPFrame, usTCPLength) > 0);
}
BOOL
xMBMasterTCPPortGetResponse( UCHAR ucPort, UCHAR ** ppucMBTCPFrame, USHORT * usTCPLength )
{
    net_helper_t *net = net_get_helper(ucPort-MB_TCP_ID_OFS);
    if(net) {
        *ppucMBTCPFrame = net->buffer;
        *usTCPLength = net->pos;
        /* Reset the buffer. */
        net->pos = 0;
    }
    return TRUE;
}
#endif

