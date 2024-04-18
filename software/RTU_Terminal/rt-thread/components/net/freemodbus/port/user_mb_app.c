#include <board.h>

DevInfoReg_t g_xDevInfoReg;
AIResultReg_t g_xAIResultReg;
DIResultReg_t g_xDIResultReg;
DOResultReg_t g_xDOResultReg; // = { .xDOResult = {1,1,1,1,0,0} };
NetCfgReg_t g_xNetCfgReg;

static rt_thread_t s_xMBRTUSlaveThread[BOARD_UART_MAX];
static rt_thread_t s_xMBTCPSlaveThread[MB_TCP_NUMS];

static void __vMBRTU_ASCIISlavePollTask(rt_uint8_t ucPort, eMBMode eMode)
{
    if (MB_ENOERR == eMBInit(
            eMode,
            g_uart_cfgs[ucPort].slave_addr,
            ucPort,
            g_uart_cfgs[ucPort].port_cfg.baud_rate,
            g_uart_cfgs[ucPort].port_cfg.parity
            )) {
        eMBEnable(ucPort);
        while (1) {
            rt_thddog_suspend("eMBPoll");
            eMBPoll(ucPort);
            rt_thddog_resume();
        }
    }

    s_xMBRTUSlaveThread[ucPort] = RT_NULL;
    rt_thddog_exit();
}

void vMBRTUSlavePollTask(void *parameter)
{
    rt_uint8_t ucPort = (rt_uint8_t)(rt_uint32_t)parameter;
    __vMBRTU_ASCIISlavePollTask(ucPort, MB_RTU);
}

void vMBASCIISlavePollTask(void *parameter)
{
    rt_uint8_t ucPort = (rt_uint8_t)(rt_uint32_t)parameter;
    __vMBRTU_ASCIISlavePollTask(ucPort, MB_ASCII);
}

rt_err_t xMBRTU_ASCIISlavePollReStart(rt_uint8_t ucPort, eMBMode eMode)
{
    vMBRTU_ASCIISlavePollStop(ucPort);

    if (RT_NULL == s_xMBRTUSlaveThread[ucPort]) {
        BOARD_CREAT_NAME(szPoll, "mbs_pl%d", ucPort);
        s_xMBRTUSlaveThread[ucPort] = rt_thread_create(
                                      szPoll,
                                      (MB_RTU == eMode) ? (vMBRTUSlavePollTask) : (vMBASCIISlavePollTask),
                                      (void *)ucPort,
                                      1024, 20, 20
                                      );

        if (s_xMBRTUSlaveThread[ucPort]) {
            rt_thddog_register(s_xMBRTUSlaveThread[ucPort], 30);
            rt_thread_startup(s_xMBRTUSlaveThread[ucPort]);
            return RT_EOK;
        }
    }

    return RT_ERROR;
}

void vMBRTU_ASCIISlavePollStop(rt_uint8_t ucPort)
{
    if (s_xMBRTUSlaveThread[ucPort]) {
        rt_thddog_unregister(s_xMBRTUSlaveThread[ucPort]);
        if (RT_EOK == rt_thread_delete(s_xMBRTUSlaveThread[ucPort])) {
            s_xMBRTUSlaveThread[ucPort] = RT_NULL;
            eMBDisable(ucPort);
            eMBClose(ucPort);
        }
    }
}

void vMBTCPSlavePollTask(void *parameter)
{
    rt_uint8_t ucPort = (rt_uint8_t)(rt_uint32_t)parameter;

    if (MB_ENOERR == eMBTCPInit(ucPort + MB_TCP_ID_OFS, 501 + ucPort)) {
        eMBEnable(ucPort + MB_TCP_ID_OFS);
        while (1) {
            rt_thddog_suspend("eMBPoll");
            eMBPoll(ucPort + MB_TCP_ID_OFS);
            rt_thddog_resume();
        }
    }

    s_xMBTCPSlaveThread[ucPort] = RT_NULL;
    rt_thddog_exit();
}

rt_err_t xMBTCPSlavePollReStart(rt_uint8_t ucPort)
{
    vMBTCPSlavePollStop(ucPort);

    if (RT_NULL == s_xMBTCPSlaveThread[ucPort]) {
        BOARD_CREAT_NAME(szPoll, "mbst_pl%d", ucPort);
        s_xMBTCPSlaveThread[ucPort] = rt_thread_create(szPoll, vMBTCPSlavePollTask, (void *)ucPort, 0x300, 20, 20);

        if (s_xMBTCPSlaveThread[ucPort]) {
            rt_thddog_register(s_xMBTCPSlaveThread[ucPort], 30);
            rt_thread_startup(s_xMBTCPSlaveThread[ucPort]);
            return RT_EOK;
        }
    }

    return RT_ERROR;
}

void vMBTCPSlavePollStop(rt_uint8_t ucPort)
{
    if (s_xMBTCPSlaveThread[ucPort]) {
        rt_thddog_unregister(s_xMBTCPSlaveThread[ucPort]);
        if (RT_EOK == rt_thread_delete(s_xMBTCPSlaveThread[ucPort])) {
            s_xMBTCPSlaveThread[ucPort] = RT_NULL;
            eMBDisable(ucPort + MB_TCP_ID_OFS);
            eMBClose(ucPort + MB_TCP_ID_OFS);
        }
    }
}

void vMBRTU_OverTCPSlavePollTask(void *parameter)
{
    rt_uint8_t ucPort = (rt_uint8_t)(rt_uint32_t)parameter;

    if (MB_ENOERR == eMBRTU_OverTCPInit(ucPort + MB_TCP_ID_OFS, g_tcpip_cfgs[ucPort].cfg.normal.maddress, 501 + ucPort)) {
        eMBEnable(ucPort + MB_TCP_ID_OFS);
        while (1) {
            rt_thddog_suspend("eMBPoll");
            eMBPoll(ucPort + MB_TCP_ID_OFS);
            rt_thddog_resume();
        }
    }

    s_xMBTCPSlaveThread[ucPort] = RT_NULL;
    rt_thddog_exit();
}

rt_err_t xMBRTU_OverTCPSlavePollReStart(rt_uint8_t ucPort)
{
    vMBTCPSlavePollStop(ucPort);

    if (RT_NULL == s_xMBTCPSlaveThread[ucPort]) {
        BOARD_CREAT_NAME(szPoll, "mbst_pl%d", ucPort);
        s_xMBTCPSlaveThread[ucPort] = rt_thread_create(szPoll, vMBRTU_OverTCPSlavePollTask, (void *)ucPort, 0x300, 20, 20);

        if (s_xMBTCPSlaveThread[ucPort]) {
            rt_thddog_register(s_xMBTCPSlaveThread[ucPort], 30);
            rt_thread_startup(s_xMBTCPSlaveThread[ucPort]);
            return RT_EOK;
        }
    }

    return RT_ERROR;
}

void vBigEdianDevInfo(DevInfo_t *in, DevInfo_t *out)
{
    out->usHWVer = var_htons(in->usHWVer);
    out->usSWVer = var_htons(in->usSWVer);
    out->usYear = var_htons(in->usYear);
    out->usMonth = var_htons(in->usMonth);
    out->usDay = var_htons(in->usDay);
    out->usHour = var_htons(in->usHour);
    out->usMin = var_htons(in->usMin);
    out->usSec = var_htons(in->usSec);
}

void vLittleEdianDevInfo(DevInfo_t *in, DevInfo_t *out)
{
    vBigEdianDevInfo(in, out);
}

void vBigEdianAIResult(AIResult_t *in, AIResult_t *out)
{
    for (int i = 0; i < 8; i++) {
        *(var_uint32_t *)&out->fAI_xx[i] = var_htonl(*(var_uint32_t *)&in->fAI_xx[i]);
    }
}

void vLittleEdianAIResult(AIResult_t *in, AIResult_t *out)
{
    vBigEdianAIResult(in, out);
}

void vBigEdianDIResult(const DIResult_t *in, DIResult_t *out)
{
    for (int i = 0; i < 8; i++) {
        out->usDI_xx[i] = var_htons(in->usDI_xx[i]);
    }
}

void vLittleEdianDIResult(DIResult_t *in, DIResult_t *out)
{
    vBigEdianDIResult(in, out);
}

void vBigEdianDOResult(DOResult_t *in, DOResult_t *out)
{
    for (int i = 0; i < 8; i++) {
        out->usDO_xx[i] = var_htons(in->usDO_xx[i]);
    }
}

void vLittleEdianDOResult(DOResult_t *in, DOResult_t *out)
{
    vBigEdianDOResult(in, out);
}

void vBigEdianNetCfg(NetCfg_t *in, NetCfg_t *out)
{
    out->dhcp = var_htons(in->dhcp);
    out->status = var_htons(in->status);
}

void vLittleEdianNetCfg(NetCfg_t *in, NetCfg_t *out)
{
    vBigEdianNetCfg(in, out);
}

#define __swap_8(_ary,_n,_m) {var_uint8_t _tmp=_ary[_n];_ary[_n]=_ary[_m];_ary[_m]=_tmp;}
#define __swap_16(_ary,_n,_m) {var_uint16_t _tmp=_ary[_n];_ary[_n]=_ary[_m];_ary[_m]=_tmp;}

void vBigEdianExtData(var_uint8_t btType, var_uint8_t btRule, VarValue_v *in, VarValue_v *out)
{
    switch (btType) {
    case E_VAR_INT16:
    case E_VAR_UINT16:
        out->val_u16 = var_htons(in->val_u16);
        break;

    case E_VAR_INT32:
    case E_VAR_UINT32:
    case E_VAR_FLOAT:
        var_uint8_t tmp;
        out->val_u32 = in->val_u32;
        if(0 == btRule) {
            __swap_8(out->val_ary, 0, 3);
            __swap_8(out->val_ary, 1, 2);
        } else if(1 == btRule) {
            __swap_8(out->val_ary, 0, 1);
            __swap_8(out->val_ary, 2, 3);
        } else if(2 == btRule) {
            __swap_16(out->val_reg, 0, 1);
        } else if(3 == btRule) {
            ;
        }
        break;

    case E_VAR_DOUBLE:
        out->val_db = in->val_db;
        if(0 == btRule) {
            __swap_8(out->val_ary, 0, 7);
            __swap_8(out->val_ary, 1, 6);
            __swap_8(out->val_ary, 2, 5);
            __swap_8(out->val_ary, 3, 4);
        } else if(1 == btRule) {
            __swap_8(out->val_ary, 0, 1);
            __swap_8(out->val_ary, 2, 3);
            __swap_8(out->val_ary, 4, 5);
            __swap_8(out->val_ary, 6, 7);
        } else if(2 == btRule) {
            __swap_16(out->val_reg, 0, 3);
            __swap_16(out->val_reg, 1, 2);
        } else if(3 == btRule) {
            ;
        }
        break;
    default:
        if (in != out) {
            memcpy(out,in, sizeof(VarValue_v));
        }
    }
}

void vLittleEdianExtData(var_uint8_t btType, var_uint8_t btRule, VarValue_v *in, VarValue_v *out)
{
    vBigEdianExtData(btType, btRule, in, out);
}

eMBErrorCode eMBRegCoilsCB(UCHAR id, UCHAR *pucRegBuffer, USHORT usAddress, USHORT usNCoils,
                           eMBRegisterMode eMode)
{
    return MB_ENOREG;
}

eMBErrorCode eMBRegInputCB(UCHAR id, UCHAR *pucRegBuffer, USHORT usAddress, USHORT usNRegs)
{
    return MB_ENOREG;
}

eMBErrorCode eMBRegHoldingCB(UCHAR id, UCHAR *pucRegBuffer, USHORT usAddress, USHORT usNRegs,
                             eMBRegisterMode eMode)
{
    eMBErrorCode    eStatus = MB_ENOERR;
    unsigned short  iRegIndex;
    USHORT *regs;
    UserRegData_t xUserReg;
    USHORT usNRegsBak = usNRegs;

    usAddress--;
    if (MB_IN_USER_REG(DEV_INFO, usAddress, usNRegs)) {
        iRegIndex = usAddress - USER_REG_DEV_INFO_START;
        vDevRefreshTime();
        xUserReg.xDevInfoReg = g_xDevInfoReg;
        vBigEdianDevInfo(&xUserReg.xDevInfoReg.xDevInfo, &xUserReg.xDevInfoReg.xDevInfo);
        regs = xUserReg.xDevInfoReg.regs;
    } else if (MB_IN_USER_REG(AI, usAddress, usNRegs)) {
        iRegIndex = usAddress - USER_REG_AI_START;
        xUserReg.xAIResultReg = g_xAIResultReg;
        vBigEdianAIResult(&xUserReg.xAIResultReg.xAIResult, &xUserReg.xAIResultReg.xAIResult);
        regs = xUserReg.xAIResultReg.regs;
    } else if (MB_IN_USER_REG(DI, usAddress, usNRegs)) {
        iRegIndex = usAddress - USER_REG_DI_START;
        xUserReg.xDIResultReg = g_xDIResultReg;
        vBigEdianDIResult(&xUserReg.xDIResultReg.xDIResult, &xUserReg.xDIResultReg.xDIResult);
        regs = xUserReg.xDIResultReg.regs;
    } else if (MB_IN_USER_REG(DO, usAddress, usNRegs)) {
        iRegIndex = usAddress - USER_REG_DO_START;
        xUserReg.xDOResultReg = g_xDOResultReg;
        vBigEdianDOResult(&xUserReg.xDOResultReg.xDOResult, &xUserReg.xDOResultReg.xDOResult);
        regs = xUserReg.xDOResultReg.regs;
    } else if (MB_IN_USER_REG(NET_CFG, usAddress, usNRegs)) {
        iRegIndex = usAddress - USER_REG_NET_CFG_START;
        if (MB_REG_READ == eMode) {
            extern struct netif *netif_list;
            if (netif_list) {
                extern ip_addr_t dns_getserver(u8_t numdns);
                ip_addr_t ip_addr = dns_getserver(0);

                g_xNetCfgReg.xNetCfg.dhcp = netif_list->flags & NETIF_FLAG_DHCP ? 1 : 0;
                strcpy(g_xNetCfgReg.xNetCfg.ipaddr, ipaddr_ntoa(&(netif_list->ip_addr)));
                strcpy(g_xNetCfgReg.xNetCfg.netmask, ipaddr_ntoa(&(netif_list->netmask)));
                strcpy(g_xNetCfgReg.xNetCfg.gw, ipaddr_ntoa(&(netif_list->gw)));
                strcpy(g_xNetCfgReg.xNetCfg.dns, ipaddr_ntoa(&(ip_addr)));
                g_xNetCfgReg.xNetCfg.status = ((netif_list->flags & NETIF_FLAG_UP) && (netif_list->flags & NETIF_FLAG_LINK_UP)) ? 1 : 0;
            }
        } else {
            g_xNetCfgReg.xNetCfg.dhcp = (g_net_cfg.dhcp ? 1 : 0);
            memcpy(g_xNetCfgReg.xNetCfg.ipaddr, g_net_cfg.ipaddr, 16);
            memcpy(g_xNetCfgReg.xNetCfg.netmask, g_net_cfg.netmask, 16);
            memcpy(g_xNetCfgReg.xNetCfg.gw, g_net_cfg.gw, 16);
            memcpy(g_xNetCfgReg.xNetCfg.dns, g_net_cfg.dns, 16);
            g_xNetCfgReg.xNetCfg.status = 1;
        }
        xUserReg.xNetCfgReg = g_xNetCfgReg;
        vBigEdianNetCfg(&xUserReg.xNetCfgReg.xNetCfg, &xUserReg.xNetCfgReg.xNetCfg);
        regs = xUserReg.xNetCfgReg.regs;
    } else if (MB_IN_USER_REG(EXT_DATA, usAddress, usNRegs)) {
        rt_enter_critical();
        if (bVarManage_CheckContAddr(usAddress, usNRegs)) {
            iRegIndex = usAddress - USER_REG_EXT_DATA_START;
            regs = &g_xExtDataRegs[0];
        } else {
            eStatus = MB_ENOREG;
        }
        rt_exit_critical();
    } else {
        eStatus = MB_ENOREG;
    }

    if (eStatus == MB_ENOERR) {
        if (MB_REG_READ == eMode) {
            rt_enter_critical();
            while (usNRegs > 0) {
                *pucRegBuffer++ = (UCHAR)(regs[iRegIndex] & 0xFF);
                *pucRegBuffer++ = (UCHAR)(regs[iRegIndex] >> 8 & 0xFF);
                iRegIndex++;
                usNRegs--;
            }
            rt_exit_critical();
        } else { //MB_REG_WRITE
            rt_enter_critical();
            while (usNRegs > 0) {
                regs[iRegIndex] = *pucRegBuffer++;
                regs[iRegIndex] |= *pucRegBuffer++ << 8;
                iRegIndex++;
                usNRegs--;
            }
            rt_exit_critical();

            if (MB_IN_USER_REG(DEV_INFO, usAddress, usNRegsBak)) {
                vLittleEdianDevInfo(&xUserReg.xDevInfoReg.xDevInfo, &xUserReg.xDevInfoReg.xDevInfo);
                g_xDevInfoReg.xDevInfo = xUserReg.xDevInfoReg.xDevInfo;
                // save
            } else if (MB_IN_USER_REG(AI, usAddress, usNRegsBak)) {
                // ro
                eStatus = MB_ENOREG;
            } else if (MB_IN_USER_REG(DI, usAddress, usNRegsBak)) {
                eStatus = MB_ENOREG;
            } else if (MB_IN_USER_REG(DO, usAddress, usNRegsBak)) {
                vLittleEdianDOResult(&xUserReg.xDOResultReg.xDOResult, &xUserReg.xDOResultReg.xDOResult);
                g_xDOResultReg.xDOResult = xUserReg.xDOResultReg.xDOResult;
                // notification DO thread
            } else if (MB_IN_USER_REG(NET_CFG, usAddress, usNRegsBak)) {
                vLittleEdianNetCfg(&xUserReg.xNetCfgReg.xNetCfg, &xUserReg.xNetCfgReg.xNetCfg);
                g_xNetCfgReg.xNetCfg = xUserReg.xNetCfgReg.xNetCfg;

                net_cfg_t bak = g_net_cfg;
                g_net_cfg.dhcp = (g_xNetCfgReg.xNetCfg.dhcp != 0);
                memcpy(g_net_cfg.ipaddr, g_xNetCfgReg.xNetCfg.ipaddr, 16);
                memcpy(g_net_cfg.netmask, g_xNetCfgReg.xNetCfg.netmask, 16);
                memcpy(g_net_cfg.gw, g_xNetCfgReg.xNetCfg.gw, 16);
                memcpy(g_net_cfg.dns, g_xNetCfgReg.xNetCfg.dns, 16);
                if (memcmp(&bak, &g_net_cfg, sizeof(g_net_cfg)) != 0) {
                    // 这里会复位设备
                    vSaveNetCfgToFs();
                }
            } else if (MB_IN_USER_REG(EXT_DATA, usAddress, usNRegsBak)) {
                bVarManage_RefreshExtDataWithModbusSlave( usAddress, usNRegsBak );
            }
        }
    }

    return eStatus;
}

eMBErrorCode eMBRegDiscreteCB(UCHAR id, UCHAR *pucRegBuffer, USHORT usAddress, USHORT usNDiscrete)
{
    return MB_ENOREG;
}

