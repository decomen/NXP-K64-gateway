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
  * File: $Id: port.c,v 1.60 2015/02/01 9:18:05 Armink $
  */

/* ----------------------- System includes --------------------------------*/

/* ----------------------- Modbus includes ----------------------------------*/
#include "port.h"
#include <board.h>
#include <stdio.h>

/* ----------------------- Variables ----------------------------------------*/
static rt_mutex_t mbs_mutexs[MB_SLAVE_NUMS];
/* ----------------------- Start implementation -----------------------------*/
void EnterCriticalSection( UCHAR ucPort )
{
    if( mbs_mutexs[ucPort] == RT_NULL ) {
        mbs_mutexs[ucPort] = (rt_mutex_t)rt_malloc( sizeof( struct rt_mutex ) );
        if( mbs_mutexs[ucPort] != RT_NULL ) {
            BOARD_CREAT_NAME( szMutex, "mbs_mtx%d", ucPort );
            rt_mutex_init( mbs_mutexs[ucPort], szMutex, RT_IPC_FLAG_PRIO );
        }
    }
    if( mbs_mutexs[ucPort] != RT_NULL ) {
        rt_mutex_take( mbs_mutexs[ucPort], RT_WAITING_FOREVER );
    }
}

void ExitCriticalSection( UCHAR ucPort )
{
    if( mbs_mutexs[ucPort] != RT_NULL ) {
        rt_mutex_release( mbs_mutexs[ucPort] );
    }
}

#if MB_MASTER_RTU_ENABLED > 0

static rt_mutex_t mbm_mutexs[MB_MASTER_NUMS];
void MasterEnterCriticalSection( UCHAR ucPort )
{
    if( mbm_mutexs[ucPort] == RT_NULL ) {
        mbm_mutexs[ucPort] = (rt_mutex_t)rt_malloc( sizeof( struct rt_mutex ) );
        if( mbm_mutexs[ucPort] != RT_NULL ) {
            BOARD_CREAT_NAME( szMutex, "mbm_mtx%d", ucPort );
            rt_mutex_init( mbm_mutexs[ucPort], szMutex, RT_IPC_FLAG_PRIO );
        }
    }
    if( mbm_mutexs[ucPort] != RT_NULL ) {
        rt_mutex_take( mbm_mutexs[ucPort], RT_WAITING_FOREVER );
    }
}

void MasterExitCriticalSection( UCHAR ucPort )
{
    if( mbm_mutexs[ucPort] != RT_NULL ) {
        rt_mutex_release( mbm_mutexs[ucPort] );
    }
}

#endif

