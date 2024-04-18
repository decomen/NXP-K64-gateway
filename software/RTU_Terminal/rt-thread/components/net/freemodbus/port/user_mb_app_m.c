
#include <board.h>

/*-----------------------Master mode use these variables----------------------*/

#if MB_MASTER_RTU_ENABLED > 0 || MB_MASTER_ASCII_ENABLED > 0
static rt_thread_t g_MBRTU_ASCIIMasterPollThread[BOARD_UART_MAX];

static void __vMBRTU_ASCIIMasterPollTask(rt_uint8_t ucPort, eMBMode eMode)
{
    if (MB_ENOERR == eMBMasterInit(
            eMode,
            ucPort,
            g_uart_cfgs[ucPort].port_cfg.baud_rate,
            g_uart_cfgs[ucPort].port_cfg.parity
            )) {
        eMBMasterEnable(ucPort);
        while (1) {
            rt_thddog_suspend("eMBMasterPoll");
            eMBMasterPoll(ucPort);
            rt_thddog_resume();
        }
    }

    g_MBRTU_ASCIIMasterPollThread[ucPort] = RT_NULL;
    rt_thddog_exit();
}

void vMBRTUMasterPollTask(void *parameter)
{
    rt_uint8_t ucPort = (rt_uint8_t)(rt_uint32_t)parameter;
    __vMBRTU_ASCIIMasterPollTask(ucPort, MB_RTU);
}

void vMBASCIIMasterPollTask(void *parameter)
{
    rt_uint8_t ucPort = (rt_uint8_t)(rt_uint32_t)parameter;
    __vMBRTU_ASCIIMasterPollTask(ucPort, MB_ASCII);
}

rt_err_t xMBRTU_ASCIIMasterPollReStart(rt_uint8_t ucPort, eMBMode eMode)
{
    vMBRTU_ASCIIMasterPollStop(ucPort);

    if (RT_NULL == g_MBRTU_ASCIIMasterPollThread[ucPort]) {
        BOARD_CREAT_NAME(szPoll, "mbm_pl%d", ucPort);
        g_MBRTU_ASCIIMasterPollThread[ucPort] = rt_thread_create(
                                                szPoll,
                                                (MB_RTU == eMode) ? (vMBRTUMasterPollTask) : (vMBASCIIMasterPollTask),
                                                (void *)ucPort,
                                                1024, 20, 20
                                                );

        if (g_MBRTU_ASCIIMasterPollThread[ucPort]) {
            rt_thddog_register(g_MBRTU_ASCIIMasterPollThread[ucPort], 30);
            rt_thread_startup(g_MBRTU_ASCIIMasterPollThread[ucPort]);
            return RT_EOK;
        }
    }

    return RT_ERROR;
}

void vMBRTU_ASCIIMasterPollStop(rt_uint8_t ucPort)
{
    if (g_MBRTU_ASCIIMasterPollThread[ucPort]) {
        rt_thddog_unregister(g_MBRTU_ASCIIMasterPollThread[ucPort]);
        if (RT_EOK == rt_thread_delete(g_MBRTU_ASCIIMasterPollThread[ucPort])) {
            g_MBRTU_ASCIIMasterPollThread[ucPort] = RT_NULL;
            eMBMasterDisable(ucPort);
            eMBMasterClose(ucPort);
        }
    }
}

#endif

#if MB_MASTER_TCP_ENABLED > 0
static rt_thread_t s_xMBTCPMasterThread[MB_TCP_NUMS];

void vMBTCPMasterPollTask(void *parameter)
{
    rt_uint8_t ucPort = (rt_uint8_t)(rt_uint32_t)parameter;

    if (MB_ENOERR == eMBMasterTCPInit(ucPort + MB_TCP_ID_OFS, 501 + ucPort)) {
        eMBMasterEnable(ucPort + MB_TCP_ID_OFS);
        while (1) {
            rt_thddog_suspend("eMBMasterPoll");
            eMBMasterPoll(ucPort + MB_TCP_ID_OFS);
            rt_thddog_resume();
        }
    }

    s_xMBTCPMasterThread[ucPort] = RT_NULL;
    rt_thddog_exit();
}

rt_err_t xMBTCPMasterPollReStart(rt_uint8_t ucPort)
{
    vMBTCPMasterPollStop(ucPort);

    if (RT_NULL == s_xMBTCPMasterThread[ucPort]) {
        BOARD_CREAT_NAME(szPoll, "mbmt_pl%d", ucPort);
        s_xMBTCPMasterThread[ucPort] = rt_thread_create(szPoll, vMBTCPMasterPollTask, (void *)ucPort, 1024, 20, 20);

        if (s_xMBTCPMasterThread[ucPort]) {
            rt_thddog_register(s_xMBTCPMasterThread[ucPort], 30);
            rt_thread_startup(s_xMBTCPMasterThread[ucPort]);
            return RT_EOK;
        }
    }

    return RT_ERROR;
}

void vMBTCPMasterPollStop(rt_uint8_t ucPort)
{
    if (s_xMBTCPMasterThread[ucPort]) {
        rt_thddog_unregister(s_xMBTCPMasterThread[ucPort]);
        if (RT_EOK == rt_thread_delete(s_xMBTCPMasterThread[ucPort])) {
            s_xMBTCPMasterThread[ucPort] = RT_NULL;
            eMBMasterDisable(ucPort + MB_TCP_ID_OFS);
            eMBMasterClose(ucPort + MB_TCP_ID_OFS);
        }
    }
}
#endif

#if MB_MASTER_TCP_ENABLED > 0 && MB_MASTER_RTU_ENABLED > 0

void vMBRTU_OverTCPMasterPollTask(void *parameter)
{
    rt_uint8_t ucPort = (rt_uint8_t)(rt_uint32_t)parameter;

    if (MB_ENOERR == eMBMasterRTU_OverTCPInit(ucPort + MB_TCP_ID_OFS, 501 + ucPort)) {
        eMBMasterEnable(ucPort + MB_TCP_ID_OFS);
        while (1) {
            rt_thddog_suspend("eMBMasterPoll");
            eMBMasterPoll(ucPort + MB_TCP_ID_OFS);
            rt_thddog_resume();
        }
    }

    s_xMBTCPMasterThread[ucPort] = RT_NULL;
    rt_thddog_exit();
}

rt_err_t xMBRTU_OverTCPMasterPollReStart(rt_uint8_t ucPort)
{
    vMBTCPMasterPollStop(ucPort);

    if (RT_NULL == s_xMBTCPMasterThread[ucPort]) {
        BOARD_CREAT_NAME(szPoll, "mbmt_pl%d", ucPort);
        s_xMBTCPMasterThread[ucPort] = rt_thread_create(szPoll, vMBRTU_OverTCPMasterPollTask, (void *)ucPort, 1024, 20, 20);

        if (s_xMBTCPMasterThread[ucPort]) {
            rt_thddog_register(s_xMBTCPMasterThread[ucPort], 30);
            rt_thread_startup(s_xMBTCPMasterThread[ucPort]);
            return RT_EOK;
        }
    }

    return RT_ERROR;
}

#endif

#if MB_MASTER_ENABLED > 0

eMBErrorCode eMBMasterRegInputCB(UCHAR ucPort, UCHAR *pucRegBuffer, USHORT usAddress, USHORT usNRegs)
{
    return eMBMasterRegHoldingCB(ucPort, pucRegBuffer, usAddress, usNRegs, MB_REG_READ);
}


var_bool_t bVarManage_RefreshExtDataWithModbusMaster(var_uint8_t port, var_uint8_t btSlaveAddr, var_uint16_t usAddress, var_uint16_t usNRegs, var_uint8_t *pucRegBuffer);
var_bool_t bVarManage_ReadExtDataWithModbusMaster(var_uint8_t port, var_uint8_t btSlaveAddr, var_uint16_t usAddress, var_uint16_t usNRegs, var_uint8_t *pucRegBuffer);

eMBErrorCode eMBMasterRegHoldingCB(UCHAR ucPort, UCHAR *pucRegBuffer, USHORT usAddress,
                                   USHORT usNRegs, eMBRegisterMode eMode)
{
    eMBErrorCode    eStatus = MB_ENOERR;
    var_uint8_t btSlaveAddr = ucMBMasterGetDestAddress(ucPort);
    
    usAddress--;

    if (eStatus == MB_ENOERR) {
        if (MB_REG_WRITE == eMode) {
            if (!bVarManage_ReadExtDataWithModbusMaster(ucPort, btSlaveAddr, usAddress, usNRegs, (var_uint8_t *)pucRegBuffer )) {
                eStatus = MB_ENOREG;
            }
        } else { //MB_REG_READ
            if (!bVarManage_RefreshExtDataWithModbusMaster(ucPort, btSlaveAddr, usAddress, usNRegs, (var_uint8_t *)pucRegBuffer )) {
                eStatus = MB_ENOREG;
            }
        }
    }

    return eStatus;
}

eMBErrorCode eMBMasterRegCoilsCB(UCHAR ucPort, UCHAR *pucRegBuffer, USHORT usAddress,
                                 USHORT usNCoils, eMBRegisterMode eMode)
{
    eMBErrorCode    eStatus = MB_ENOERR;

    return eStatus;
}

eMBErrorCode eMBMasterRegDiscreteCB(UCHAR ucPort, UCHAR *pucRegBuffer, USHORT usAddress, USHORT usNDiscrete)
{
    eMBErrorCode    eStatus = MB_ENOERR;

    return eStatus;
}

#endif

