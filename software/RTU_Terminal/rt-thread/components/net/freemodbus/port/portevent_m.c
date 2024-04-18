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
 * File: $Id: portevent_m.c v 1.60 2013/08/13 15:07:05 Armink add Master Functions$
 */

/* ----------------------- Modbus includes ----------------------------------*/
#include "mb.h"
#include "mb_m.h"
#include "mbport.h"
#include "port.h"
#include <board.h>

#if MB_MASTER_RTU_ENABLED > 0 || MB_MASTER_ASCII_ENABLED > 0
/* ----------------------- Defines ------------------------------------------*/
/* ----------------------- Variables ----------------------------------------*/
static rt_sem_t xMasterRunRes[MB_MASTER_NUMS];
static rt_event_t xMasterOsEvents[MB_MASTER_NUMS];
static rt_event_t xMasterOsRspEvents[MB_MASTER_NUMS];
/* ----------------------- Start implementation -----------------------------*/
BOOL
xMBMasterPortEventInit( UCHAR ucPort )
{
    if( RT_NULL == xMasterOsEvents[ucPort] ) {
        BOARD_CREAT_NAME( szEvt, "mbm_oe%d", ucPort );
        xMasterOsEvents[ucPort] = rt_event_create( szEvt, RT_IPC_FLAG_FIFO );
    }
    if( RT_NULL == xMasterOsRspEvents[ucPort] ) {
        BOARD_CREAT_NAME( szEvt, "mbm_re%d", ucPort );
        xMasterOsRspEvents[ucPort] = rt_event_create( szEvt, RT_IPC_FLAG_FIFO );
    }
    return ( xMasterOsEvents[ucPort] != RT_NULL && xMasterOsRspEvents[ucPort] != RT_NULL );
}

BOOL
xMBMasterPortEventPost( UCHAR ucPort, eMBMasterEventType eEvent )
{
    if( xMasterOsEvents[ucPort] != RT_NULL ) {
        rt_event_send( xMasterOsEvents[ucPort], eEvent);
    }
    return (xMasterOsEvents[ucPort] != RT_NULL);
}

BOOL
xMBMasterPortEventGet( UCHAR ucPort, eMBMasterEventType * eEvent )
{
    rt_uint32_t recvedEvent;
    if( xMasterOsEvents[ucPort] != RT_NULL ) {
        rt_event_recv( xMasterOsEvents[ucPort],
	            EV_MASTER_READY | EV_MASTER_FRAME_RECEIVED | EV_MASTER_EXECUTE |
	            EV_MASTER_FRAME_SENT | EV_MASTER_ERROR_PROCESS,
	            RT_EVENT_FLAG_OR | RT_EVENT_FLAG_CLEAR, RT_WAITING_FOREVER,
	            &recvedEvent);
	    /* the enum type couldn't convert to int type */
	    switch (recvedEvent) {
	    case EV_MASTER_READY:
	        *eEvent = EV_MASTER_READY;
	        break;
	    case EV_MASTER_FRAME_RECEIVED:
	        *eEvent = EV_MASTER_FRAME_RECEIVED;
	        break;
	    case EV_MASTER_EXECUTE:
	        *eEvent = EV_MASTER_EXECUTE;
	        break;
	    case EV_MASTER_FRAME_SENT:
	        *eEvent = EV_MASTER_FRAME_SENT;
	        break;
	    case EV_MASTER_ERROR_PROCESS:
	        *eEvent = EV_MASTER_ERROR_PROCESS;
	        break;
	    }
	}
    return (xMasterOsEvents[ucPort] != RT_NULL);
}
/**
 * This function is initialize the OS resource for modbus master.
 * Note:The resource is define by OS.If you not use OS this function can be empty.
 *
 */
void vMBMasterOsResInit( UCHAR ucPort )
{    
	if( RT_NULL == xMasterRunRes[ucPort] ) {
        BOARD_CREAT_NAME( szSem, "mbm_sem%d", ucPort );
        xMasterRunRes[ucPort] = rt_sem_create( szSem, 0x01, RT_IPC_FLAG_PRIO );
    }
}

/**
 * This function is take Mobus Master running resource.
 * Note:The resource is define by Operating System.If you not use OS this function can be just return TRUE.
 *
 * @param lTimeOut the waiting time.
 *
 * @return resource taked result
 */
BOOL xMBMasterRunResTake( UCHAR ucPort, LONG lTimeOut )
{
    /*If waiting time is -1 .It will wait forever */
    if( xMasterRunRes[ucPort] != RT_NULL ) {
        return (rt_sem_take( xMasterRunRes[ucPort], lTimeOut) ? FALSE : TRUE);
    }
    return FALSE;
}

/**
 * This function is release Mobus Master running resource.
 * Note:The resource is define by Operating System.If you not use OS this function can be empty.
 *
 */
void vMBMasterRunResRelease( UCHAR ucPort )
{
    /* release resource */    
	if( xMasterRunRes[ucPort] != RT_NULL ) {
        rt_sem_release( xMasterRunRes[ucPort] );
    }
}

/**
 * This is modbus master respond timeout error process callback function.
 * @note There functions will block modbus master poll while execute OS waiting.
 * So,for real-time of system.Do not execute too much waiting process.
 *
 * @param ucDestAddress destination salve address
 * @param pucPDUData PDU buffer data
 * @param ucPDULength PDU buffer length
 *
 */
void vMBMasterErrorCBRespondTimeout( UCHAR ucPort, UCHAR ucDestAddress, const UCHAR* pucPDUData,
        USHORT ucPDULength) {
    /**
     * @note This code is use OS's event mechanism for modbus master protocol stack.
     * If you don't use OS, you can change it.
     */
	if( xMasterOsRspEvents[ucPort] != RT_NULL ) {
        rt_event_send( xMasterOsRspEvents[ucPort], EV_MASTER_ERROR_RESPOND_TIMEOUT );
    }
	
    /* You can add your code under here. */

}

/**
 * This is modbus master receive data error process callback function.
 * @note There functions will block modbus master poll while execute OS waiting.
 * So,for real-time of system.Do not execute too much waiting process.
 *
 * @param ucDestAddress destination salve address
 * @param pucPDUData PDU buffer data
 * @param ucPDULength PDU buffer length
 *
 */
void vMBMasterErrorCBReceiveData( UCHAR ucPort, UCHAR ucDestAddress, const UCHAR* pucPDUData,
        USHORT ucPDULength) {
    /**
     * @note This code is use OS's event mechanism for modbus master protocol stack.
     * If you don't use OS, you can change it.
     */
	if( xMasterOsRspEvents[ucPort] != RT_NULL ) {
        rt_event_send( xMasterOsRspEvents[ucPort], EV_MASTER_ERROR_RECEIVE_DATA );
    }
	
    /* You can add your code under here. */

}

/**
 * This is modbus master execute function error process callback function.
 * @note There functions will block modbus master poll while execute OS waiting.
 * So,for real-time of system.Do not execute too much waiting process.
 *
 * @param ucDestAddress destination salve address
 * @param pucPDUData PDU buffer data
 * @param ucPDULength PDU buffer length
 *
 */
void vMBMasterErrorCBExecuteFunction( UCHAR ucPort, UCHAR ucDestAddress, const UCHAR* pucPDUData,
        USHORT ucPDULength) {
    /**
     * @note This code is use OS's event mechanism for modbus master protocol stack.
     * If you don't use OS, you can change it.
     */
	if( xMasterOsRspEvents[ucPort] != RT_NULL ) {
        rt_event_send( xMasterOsRspEvents[ucPort], EV_MASTER_ERROR_EXECUTE_FUNCTION );
    }

    /* You can add your code under here. */

}

/**
 * This is modbus master request process success callback function.
 * @note There functions will block modbus master poll while execute OS waiting.
 * So,for real-time of system.Do not execute too much waiting process.
 *
 */
void vMBMasterCBRequestScuuess( UCHAR ucPort ) {
    /**
     * @note This code is use OS's event mechanism for modbus master protocol stack.
     * If you don't use OS, you can change it.
     */
	if( xMasterOsRspEvents[ucPort] != RT_NULL ) {
        rt_event_send( xMasterOsRspEvents[ucPort], EV_MASTER_PROCESS_SUCESS );
    }

    /* You can add your code under here. */

}

/**
 * This function is wait for modbus master request finish and return result.
 * Waiting result include request process success, request respond timeout,
 * receive data error and execute function error.You can use the above callback function.
 * @note If you are use OS, you can use OS's event mechanism. Otherwise you have to run
 * much user custom delay for waiting.
 *
 * @return request error code
 */
eMBMasterReqErrCode eMBMasterWaitRequestFinish( UCHAR ucPort ) {
    eMBMasterReqErrCode    eErrStatus = MB_MRE_NO_ERR;
    rt_uint32_t recvedEvent;
    /* waiting for OS event */
	if( xMasterOsRspEvents[ucPort] != RT_NULL ) {
	
	    rt_tick_t timer_tick = (10000) * RT_TICK_PER_SECOND / 1000;
	    while (1) {
    	    if ( RT_EOK == rt_event_recv(xMasterOsRspEvents[ucPort],
            EV_MASTER_PROCESS_SUCESS | EV_MASTER_ERROR_RESPOND_TIMEOUT
                    | EV_MASTER_ERROR_RECEIVE_DATA
                    | EV_MASTER_ERROR_EXECUTE_FUNCTION,
            RT_EVENT_FLAG_OR | RT_EVENT_FLAG_CLEAR, timer_tick,
            &recvedEvent) ) {
        	    switch (recvedEvent) {
        	    case EV_MASTER_PROCESS_SUCESS:
        	        break;
        	    case EV_MASTER_ERROR_RESPOND_TIMEOUT:
        	        eErrStatus = MB_MRE_TIMEDOUT;
        	        break;
        	    case EV_MASTER_ERROR_RECEIVE_DATA:
        	        eErrStatus = MB_MRE_REV_DATA;
        	        break;
        	    case EV_MASTER_ERROR_EXECUTE_FUNCTION:
        	        eErrStatus = MB_MRE_EXE_FUN;
        	        break;
        	    }
        	    break;
    	    } else {
    	        vMBMasterPortTimersDisable(ucPort);
                vMBMasterSetErrorType(ucPort, EV_ERROR_RESPOND_TIMEOUT);
                (void)xMBMasterPortEventPost(ucPort, EV_MASTER_ERROR_PROCESS);       
    	    }
	    }
    }/* else {
		eErrStatus = 
	}*/

    return eErrStatus;
}

#endif
