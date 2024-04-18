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
 * File: $Id: porttimer_m.c,v 1.60 2013/08/13 15:07:05 Armink add Master Functions$
 */

/* ----------------------- Platform includes --------------------------------*/
#include "port.h"

/* ----------------------- Modbus includes ----------------------------------*/
#include "mb.h"
#include "mb_m.h"
#include "mbport.h"
#include <board.h>

#if MB_MASTER_RTU_ENABLED > 0 || MB_MASTER_ASCII_ENABLED > 0

/* ----------------------- Variables ----------------------------------------*/
static USHORT s_usTimeOut50us[MB_MASTER_NUMS];
static ULONG s_ulTimeOut1Ss[MB_MASTER_NUMS];
static rt_timer_t timers[MB_MASTER_NUMS];
static volatile eMBMasterTimerMode eMasterCurTimerMode[MB_MASTER_NUMS];
static void prvvTIMERExpiredISR( UCHAR ucPort );
static void timer_timeout_ind(void* parameter);

/* ----------------------- Start implementation -----------------------------*/
BOOL xMBMasterPortTimersT35Init( UCHAR ucPort, USHORT usTimeOut50us)
{
    /* backup T35 ticks */
    s_usTimeOut50us[ucPort] = usTimeOut50us;

    BOARD_CREAT_NAME( szTime, "mbm_t%d", ucPort );
    timers[ucPort] = rt_timer_create( szTime,
                               timer_timeout_ind, /* bind timeout callback function */
                               (void *)(uint32_t)ucPort, 
                               (rt_tick_t)((50*s_usTimeOut50us[ucPort])/(1000.0*1000/RT_TICK_PER_SECOND) + 0.5),
                               RT_TIMER_FLAG_ONE_SHOT); /* one shot */
    return (timers[ucPort] != RT_NULL);
}

BOOL xMBMasterIsPortTimersEnable( UCHAR ucPort )
{
	if( timers[ucPort] != RT_NULL ) {
	    return ((timers[ucPort]->parent.flag&RT_TIMER_FLAG_ACTIVATED) != 0);
	}

	return RT_FALSE;
}

void vMBMasterPortTimersT35Enable( UCHAR ucPort )
{
	if( timers[ucPort] != RT_NULL ) {
	    rt_tick_t timer_tick = (rt_tick_t)((50*s_usTimeOut50us[ucPort])/(1000.0*1000/RT_TICK_PER_SECOND) + 0.5);

	    /* Set current timer mode, don't change it.*/
	    vMBMasterSetCurTimerMode( ucPort, MB_TMODE_T35 );

	    rt_timer_control( timers[ucPort], RT_TIMER_CTRL_SET_TIME, &timer_tick );

	    rt_timer_start( timers[ucPort] );
	}
}

BOOL xMBMasterPortTimersT1SInit( UCHAR ucPort, ULONG ulTimeOut1s)
{
    /* backup T35 ticks */
    s_ulTimeOut1Ss[ucPort] = ulTimeOut1s;

    BOARD_CREAT_NAME( szTime, "mbm_t%d", ucPort );
    timers[ucPort] = rt_timer_create( szTime,
                               timer_timeout_ind, /* bind timeout callback function */
                               (void *)(uint32_t)ucPort, 
                               (rt_tick_t)((s_ulTimeOut1Ss[ucPort])*(1000.0*RT_TICK_PER_SECOND/1000) + 0.5),
                               RT_TIMER_FLAG_ONE_SHOT); /* one shot */
    return (timers[ucPort] != RT_NULL);
}

void vMBMasterPortTimersT1SEnable( UCHAR ucPort )
{
	if( timers[ucPort] != RT_NULL ) {
	    rt_tick_t timer_tick = (rt_tick_t)((s_ulTimeOut1Ss[ucPort])*(1000.0*RT_TICK_PER_SECOND/1000) + 0.5);

	    /* Set current timer mode, don't change it.*/
	    vMBMasterSetCurTimerMode( ucPort, MB_TMPDE_TASCII );

	    rt_timer_control( timers[ucPort], RT_TIMER_CTRL_SET_TIME, &timer_tick );

	    rt_timer_start( timers[ucPort] );
	}
}

void vMBMasterPortTimersConvertDelayEnable( UCHAR ucPort )
{
	if( timers[ucPort] != RT_NULL ) {
	    rt_tick_t timer_tick = MB_MASTER_DELAY_MS_CONVERT*RT_TICK_PER_SECOND/1000;

	    /* Set current timer mode, don't change it.*/
	    vMBMasterSetCurTimerMode( ucPort, MB_TMODE_CONVERT_DELAY );

	    rt_timer_control( timers[ucPort], RT_TIMER_CTRL_SET_TIME, &timer_tick );

	    rt_timer_start( timers[ucPort] );
	}
}

void vMBMasterPortTimersRespondTimeoutEnable( UCHAR ucPort )
{
	if( timers[ucPort] != RT_NULL ) {
	    rt_tick_t timer_tick = MB_MASTER_TIMEOUT_MS_RESPOND * RT_TICK_PER_SECOND / 1000;
	    if( ucPort == BOARD_ZIGBEE_UART ) {
	        timer_tick = MB_MASTER_TIMEOUT_ZGB_MS_RESPOND * RT_TICK_PER_SECOND / 1000;
	    } else if (MB_IN_ENET(ucPort)) {
            timer_tick = MB_MASTER_TIMEOUT_ENET_MS_RESPOND * RT_TICK_PER_SECOND / 1000;
	    } else if (MB_IN_GPRS(ucPort)) {
            timer_tick = MB_MASTER_TIMEOUT_GPRS_MS_RESPOND * RT_TICK_PER_SECOND / 1000;
	    }

	    /* Set current timer mode, don't change it.*/
	    vMBMasterSetCurTimerMode( ucPort, MB_TMODE_RESPOND_TIMEOUT );

	    rt_timer_control( timers[ucPort], RT_TIMER_CTRL_SET_TIME, &timer_tick );

	    rt_timer_start( timers[ucPort] );
    }
}

void vMBMasterPortTimersDisable( UCHAR ucPort )
{
    if( timers[ucPort] != RT_NULL ) {
        rt_timer_stop( timers[ucPort] );
    }
}

void prvvTIMERExpiredISR( UCHAR ucPort )
{
    (void) pxMBMasterPortCBTimerExpired[ucPort]( ucPort );
}

static void timer_timeout_ind(void* parameter)
{
    UCHAR ucPort = (UCHAR)((uint32_t)((uint32_t *)parameter));
    
    prvvTIMERExpiredISR( ucPort );
}

/* Set Modbus Master current timer mode.*/
void vMBMasterSetCurTimerMode( UCHAR ucPort, eMBMasterTimerMode eMBTimerMode )
{
	eMasterCurTimerMode[ucPort] = eMBTimerMode;
}

eMBMasterTimerMode eMBMasterGetCurTimerMode( UCHAR ucPort )
{
	return eMasterCurTimerMode[ucPort];
}

#endif
