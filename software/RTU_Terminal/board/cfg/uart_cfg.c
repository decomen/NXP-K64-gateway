
#include <board.h>
#include <stdio.h>


extern s_Rs232_Rs485_Stat gUartType;

static const uart_cfg_t c_uart_default_cfgs[BOARD_UART_MAX] = {
    DEFAULT_UART_CFG(BOARD_UART_0_TYPE),
    DEFAULT_UART_CFG(BOARD_UART_1_TYPE),
    DEFAULT_UART_CFG(BOARD_UART_2_TYPE),
    DEFAULT_UART_CFG(BOARD_UART_3_TYPE),
    DEFAULT_UART_CFG(BOARD_UART_4_TYPE),
    DEFAULT_UART_CFG(BOARD_UART_5_TYPE)
};

uart_cfg_t g_uart_cfgs[BOARD_UART_MAX] = {
    DEFAULT_UART_CFG(BOARD_UART_0_TYPE),
    DEFAULT_UART_CFG(BOARD_UART_1_TYPE),
    DEFAULT_UART_CFG(BOARD_UART_2_TYPE),
    DEFAULT_UART_CFG(BOARD_UART_3_TYPE),
    DEFAULT_UART_CFG(BOARD_UART_4_TYPE),
    DEFAULT_UART_CFG(BOARD_UART_5_TYPE)
};

extern s_Rs232_Rs485_Stat gUartType;

static void prvSetUartCfgsDefault(void)
{
    memcpy(&g_uart_cfgs[0], &c_uart_default_cfgs[0], sizeof(g_uart_cfgs));
    g_uart_cfgs[0].uart_type = gUartType.eUart0Type;
    g_uart_cfgs[1].uart_type = gUartType.eUart1Type;
    g_uart_cfgs[4].uart_type = gUartType.eUart4Type;
    g_uart_cfgs[5].uart_type = gUartType.eUart5Type;
}

static void prvReadUartCfgsFromFs(void)
{
    if (!board_cfg_read(UART_CFG_OFS, &g_uart_cfgs[0], sizeof(g_uart_cfgs))) {
        prvSetUartCfgsDefault();
    } else {
        g_uart_cfgs[0].uart_type = gUartType.eUart0Type;
        g_uart_cfgs[1].uart_type = gUartType.eUart1Type;
        g_uart_cfgs[4].uart_type = gUartType.eUart4Type;
        g_uart_cfgs[5].uart_type = gUartType.eUart5Type;
    }
}

void prvSaveUartCfgsToFs(void)
{
    if (!board_cfg_write(UART_CFG_OFS, &g_uart_cfgs[0], sizeof(g_uart_cfgs))) {
        ; //prvSetUartCfgsDefault();
    }
    g_uart_cfgs[0].uart_type = gUartType.eUart0Type;
    g_uart_cfgs[1].uart_type = gUartType.eUart1Type;
    g_uart_cfgs[4].uart_type = gUartType.eUart4Type;
    g_uart_cfgs[5].uart_type = gUartType.eUart5Type;
}

rt_err_t uart_cfg_init(void)
{
    prvReadUartCfgsFromFs();
    g_uart_cfgs[2] = c_uart_default_cfgs[2];
    g_uart_cfgs[3] = c_uart_default_cfgs[3];

    for (int i = 0; i < BOARD_UART_MAX; i++) {
        g_uart_cfgs[i].busy_handle = RT_NULL;
        g_uart_cfgs[i].recv_handle = RT_NULL;
        if (UART_TYPE_ZIGBEE == g_uart_cfgs[i].uart_type) {
            //g_uart_cfgs[i].busy = ;
            //g_uart_cfgs[i].recv = ;
        } else if (UART_TYPE_GPRS == g_uart_cfgs[i].uart_type) {
            //g_uart_cfgs[i].busy = ;
            //g_uart_cfgs[i].recv = ;
        }
    }

    return RT_EOK;
}

rt_int8_t nUartGetIndex(rt_int8_t instance)
{
    switch (instance) {
    case HW_UART1:
        return PROTO_DEV_RS1;
        break;
    case HW_UART4:
        return PROTO_DEV_RS2;
        break;
    case HW_UART5:
        return PROTO_DEV_RS3;
        break;
    case HW_UART0:
        return PROTO_DEV_RS4;
        break;
    }

    return -1;
}

rt_int8_t nUartGetInstance(rt_int8_t index)
{
    switch (index) {
    case PROTO_DEV_RS1:
        return HW_UART1;
        break;
    case PROTO_DEV_RS2:
        return HW_UART4;
        break;
    case PROTO_DEV_RS3:
        return HW_UART5;
        break;
    case PROTO_DEV_RS4:
        return HW_UART0;
        break;
    }

    return -1;
}

void setUartCfgWithJson(cJSON *pCfg)
{
    int n = cJSON_GetInt(pCfg, "n", -1);
    int bd = cJSON_GetInt(pCfg, "bd", -1);
    int ut = cJSON_GetInt(pCfg, "ut", -1);
    int po = cJSON_GetInt(pCfg, "po", -1);
    int py = cJSON_GetInt(pCfg, "py", -1);
    int ms = cJSON_GetInt(pCfg, "ms", -1);
    int ad = cJSON_GetInt(pCfg, "ad", -1);
    int in = cJSON_GetInt(pCfg, "in", -1);
    int da = cJSON_GetInt(pCfg, "da", -1);
    int st = cJSON_GetInt(pCfg, "st", -1);
    const char* luapo = cJSON_GetString(pCfg, "luapo", RT_NULL);

    if (n >= PROTO_DEV_RS1 && n <= PROTO_DEV_RS_MAX) {
        rt_int8_t instance = nUartGetInstance(n);
        if (instance >= 0) {
            uart_cfg_t xBak = g_uart_cfgs[instance];

            if (bd >= 0) g_uart_cfgs[instance].port_cfg.baud_rate = bd;
            //if( ut >= 0 ) g_uart_cfgs[instance].uart_type = ut;
            if (po >= 0) g_uart_cfgs[instance].proto_type = po;
            if (py >= 0) g_uart_cfgs[instance].port_cfg.parity = py;
            if (ms >= 0) g_uart_cfgs[instance].proto_ms = ms;
            if (ad >= 0) g_uart_cfgs[instance].slave_addr = ad;
            if (in >= 0) g_uart_cfgs[instance].interval = in;
            if (da >= 0) g_uart_cfgs[instance].port_cfg.data_bits = da;
            if (st >= 0) g_uart_cfgs[instance].port_cfg.stop_bits = st;
            if (luapo&&luapo[0]) strncpy(g_uart_cfgs[instance].lua_proto,luapo,32);

            if (memcmp(&xBak, &g_uart_cfgs[instance], sizeof(uart_cfg_t)) != 0) {
                if ((UART_TYPE_232 == g_uart_cfgs[instance].uart_type ||
                     UART_TYPE_485 == g_uart_cfgs[instance].uart_type ||
                     UART_TYPE_ZIGBEE == g_uart_cfgs[instance].uart_type) &&
                    (PROTO_MODBUS_RTU == g_uart_cfgs[instance].proto_type ||
                     PROTO_MODBUS_ASCII == g_uart_cfgs[instance].proto_type)) {

                    if (!g_xfer_net_dst_uart_occ[instance]) {
                        if (PROTO_SLAVE == g_uart_cfgs[instance].proto_ms) {
                            vMBRTU_ASCIIMasterPollStop(instance);
                            xMBRTU_ASCIISlavePollReStart(
                                instance,
                                (PROTO_MODBUS_RTU == g_uart_cfgs[instance].proto_type) ? (MB_RTU) : (MB_ASCII)
                                );
                        } else if (PROTO_MASTER == g_uart_cfgs[instance].proto_ms) {
                            vMBRTU_ASCIISlavePollStop(instance);
                            xMBRTU_ASCIIMasterPollReStart(
                                instance,
                                (PROTO_MODBUS_RTU == g_uart_cfgs[instance].proto_type) ? (MB_RTU) : (MB_ASCII)
                                );
                        }
                    }
                } else {
                    vMBRTU_ASCIIMasterPollStop(instance);
                    vMBRTU_ASCIISlavePollStop(instance);
                }

                prvSaveUartCfgsToFs();
            }
        }
    }
}

// for webserver
DEF_CGI_HANDLER(setUartCfg)
{
    rt_err_t err = RT_EOK;
    const char *szJSON = CGI_GET_ARG("arg");
    cJSON *pCfg = szJSON ? cJSON_Parse(szJSON) : RT_NULL;
    if (pCfg) {
        setUartCfgWithJson(pCfg);
    } else {
        err = RT_ERROR;
    }
    cJSON_Delete(pCfg);

    WEBS_PRINTF("{\"ret\":%d}", err);
    WEBS_DONE(200);
}

void jsonUartCfg(int n, cJSON *pItem)
{
    rt_int8_t instance = nUartGetInstance(n);
    if (instance >= 0) {
        cJSON_AddNumberToObject(pItem, "n", n);
        cJSON_AddNumberToObject(pItem, "bd", g_uart_cfgs[instance].port_cfg.baud_rate);
        cJSON_AddNumberToObject(pItem, "ut", g_uart_cfgs[instance].uart_type);
        cJSON_AddNumberToObject(pItem, "po", g_uart_cfgs[instance].proto_type);
        cJSON_AddNumberToObject(pItem, "py", g_uart_cfgs[instance].port_cfg.parity);
        cJSON_AddNumberToObject(pItem, "ms", g_uart_cfgs[instance].proto_ms);
        cJSON_AddNumberToObject(pItem, "ad", g_uart_cfgs[instance].slave_addr);
        cJSON_AddNumberToObject(pItem, "in", g_uart_cfgs[instance].interval);
        cJSON_AddNumberToObject(pItem, "da", g_uart_cfgs[instance].port_cfg.data_bits);
        cJSON_AddNumberToObject(pItem, "st", g_uart_cfgs[instance].port_cfg.stop_bits);
        cJSON_AddStringToObject(pItem, "luapo", g_uart_cfgs[instance].lua_proto);
    }
}

DEF_CGI_HANDLER(getUartCfg)
{
    rt_err_t err = RT_EOK;
    const char *szJSON = CGI_GET_ARG("arg");
    cJSON *pCfg = szJSON ? cJSON_Parse(szJSON) : RT_NULL;
    char *szRetJSON = RT_NULL;

    if (pCfg) {
        int all = cJSON_GetInt(pCfg, "all", 0);
        if (all) {
            WEBS_PRINTF("{\"ret\":0,\"list\":[");

            rt_bool_t first = RT_TRUE;
            for (int n = 0; n <= PROTO_DEV_RS_MAX; n++) {
                cJSON *pItem = cJSON_CreateObject();
                if(pItem) {
                    if (!first) WEBS_PRINTF(",");
                    first = RT_FALSE;
                    jsonUartCfg(n, pItem);
                    szRetJSON = cJSON_PrintUnformatted(pItem);
                    if(szRetJSON) {
                        WEBS_PRINTF(szRetJSON);
                        rt_free(szRetJSON);
                    }
                }
                cJSON_Delete(pItem);
            }
            WEBS_PRINTF("]}");
        } else {
            int n = cJSON_GetInt(pCfg, "n", -1);
            if (n >= 0 && n < 4) {
                cJSON *pItem = cJSON_CreateObject();
                if(pItem) {
                    cJSON_AddNumberToObject(pItem, "ret", RT_EOK);
                    jsonUartCfg(n, pItem);
                    szRetJSON = cJSON_PrintUnformatted(pItem);
                    if(szRetJSON) {
                        WEBS_PRINTF(szRetJSON);
                        rt_free(szRetJSON);
                    }
                }
                cJSON_Delete(pItem);
            }
        }
    } else {
        err = RT_ERROR;
    }
    cJSON_Delete(pCfg);

    if (err != RT_EOK) {
        WEBS_PRINTF("{\"ret\":%d}", err);
    }

    WEBS_DONE(200);
}


