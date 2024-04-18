/*
 * FreeModbus Libary: BARE Port
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
 * File: $Id: port.h ,v 1.60 2013/08/13 15:07:05 Armink add Master Functions $
 */

#ifndef _PORT_H
#define _PORT_H

#include "mbconfig.h"
#include <rtdef.h>
#include <rtthread.h>

#include <assert.h>
#include <inttypes.h>

#define INLINE
#define PR_BEGIN_EXTERN_C           extern "C" {
#define PR_END_EXTERN_C             }

#define ENTER_CRITICAL_SECTION()        EnterCriticalSection(ucPort)
#define EXIT_CRITICAL_SECTION()         ExitCriticalSection(ucPort)

#if MB_MASTER_RTU_ENABLED > 0
#define MASTER_ENTER_CRITICAL_SECTION() MasterEnterCriticalSection(ucPort)
#define MASTER_EXIT_CRITICAL_SECTION()  MasterExitCriticalSection(ucPort)
#endif

typedef rt_bool_t   BOOL;

typedef rt_uint8_t  UCHAR;
typedef rt_int8_t   CHAR;

typedef rt_uint16_t USHORT;
typedef rt_int16_t  SHORT;

typedef rt_uint32_t ULONG;
typedef rt_int32_t  LONG;

#ifndef TRUE
#define TRUE            1
#endif

#ifndef FALSE
#define FALSE           0
#endif

void EnterCriticalSection( UCHAR ucPort );
void ExitCriticalSection( UCHAR ucPort );

#if MB_MASTER_RTU_ENABLED > 0
void MasterEnterCriticalSection( UCHAR ucPort );
void MasterExitCriticalSection( UCHAR ucPort );
#endif

#endif

