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
 * File: $Id: porttimer.c,v 1.60 2013/08/13 15:07:05 Armink $
 */

/* ----------------------- Platform includes --------------------------------*/
#include "port.h"

/* ----------------------- Modbus includes ----------------------------------*/
#include "mb.h"
#include "mbport.h"
#include <board.h>

/* ----------------------- static functions ---------------------------------*/
static rt_timer_t timers[MB_SLAVE_NUMS];
static void prvvTIMERExpiredISR( UCHAR ucPort );
static void timer_timeout_ind(void* parameter);

/* ----------------------- Start implementation -----------------------------*/
BOOL xMBPortTimersInit( UCHAR ucPort, USHORT usTim1Timerout50us )
{
    BOARD_CREAT_NAME( szTime, "mbs_t%d", ucPort );
    timers[ucPort] = rt_timer_create( szTime,
                               timer_timeout_ind, /* bind timeout callback function */
                               (void *)(uint32_t)ucPort,
                               (rt_tick_t)((50*usTim1Timerout50us)/(1000.0*1000/RT_TICK_PER_SECOND) + 0.5),
                               RT_TIMER_FLAG_ONE_SHOT); /* one shot */
    return (timers[ucPort] != RT_NULL);
}

BOOL xMBIsPortTimersEnable( UCHAR ucPort )
{
	if( timers[ucPort] != RT_NULL ) {
	    return ((timers[ucPort]->parent.flag&RT_TIMER_FLAG_ACTIVATED) != 0);
	}

	return RT_FALSE;
}

void vMBPortTimersEnable( UCHAR ucPort )
{
    if( timers[ucPort] != RT_NULL ) {
        rt_timer_start( timers[ucPort] );
    }
}

void vMBPortTimersDisable( UCHAR ucPort )
{
    if( timers[ucPort] != RT_NULL ) {
        rt_timer_stop( timers[ucPort] );
    }
}

void prvvTIMERExpiredISR( UCHAR ucPort )
{
    (void) pxMBPortCBTimerExpired[ucPort]( ucPort );
}

static void timer_timeout_ind(void* parameter)
{
    UCHAR ucPort = (UCHAR)((uint32_t)((uint32_t *)parameter));

    prvvTIMERExpiredISR( ucPort );
}

