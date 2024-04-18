
// v0旧结构, 看要不要做恢复
typedef struct {
    rt_bool_t enable;               // 0:关闭  1:开启
    tcpip_type_e tcpip_type;        //模式
    proto_tcpip_type_e proto_type;  //协议

    tcpip_cs_e tcpip_cs;            // client or server
    proto_ms_e proto_ms;            //主从
    
    char peer[63];                  //client时有效, 目标地址
    rt_uint8_t maddress;            // modbus rtu over tcp slave address
    rt_uint16_t port;               //端口号

    rt_uint32_t interval;           //
    rt_bool_t keepalive;            //
} tcpip_cfg_v0_t;

typedef struct {
    tcpip_cfg_v0_t v0[BOARD_TCPIP_MAX];
} tcpip_cfg_v0_all_t;

typedef struct {
    rt_bool_t       enable;         // 0:关闭  1:开启
    
    tcpip_type_e    tcpip_type;     // 模式
    tcpip_cs_e      tcpip_cs;       // client or server
    
    xfer_proto_e    proto_type;     // 协议
    char            peer[64];       // client时有效, 目标地址
    rt_uint16_t     port;           // 端口号

    eProtoDevId     dst_type;       // 转发端口
    xfer_dst_cfg    dst_cfg;        // 转发端口配置(串口需要该配置)
} xfer_net_cfg_v0_t;

typedef struct {
    xfer_net_cfg_v0_t v0[2];
} xfer_net_cfg_v0_all_t;

typedef struct {
    ZIGBEE_DEV_INFO_T   xInfo;
    UCHAR               ucState;    //只读
    USHORT              usType;     //只读
    USHORT              usVer;      //只读
    rt_uint32_t         ulLearnStep;
    rt_uint8_t          btSlaveAddr;
    int                 nProtoType;
} zigbee_cfg_v0_t;

static void _cfg_recover_0_2_1(void)
{
    tcpip_cfg_v0_all_t *tcpip_cfg = rt_calloc(1, sizeof(tcpip_cfg_v0_all_t));
    xfer_net_cfg_v0_all_t *xfer_net_cfg = rt_calloc(1, sizeof(xfer_net_cfg_v0_all_t));
    zigbee_cfg_v0_t *zigbee_cfg = rt_calloc(1, sizeof(zigbee_cfg_v0_t));
    if(tcpip_cfg && xfer_net_cfg && zigbee_cfg) {
        rt_bool_t tcpip_cfg_ok = board_cfg_read(TCPIP_CFG_OFS, tcpip_cfg, sizeof(tcpip_cfg_v0_all_t));
        rt_bool_t xfer_net_cfg_ok = board_cfg_read(XFER_UART_CFG_OFS, xfer_net_cfg, sizeof(xfer_net_cfg_v0_all_t));
        rt_bool_t zigbee_cfg_ok = board_cfg_read(ZIGBEE_CFG_OFS, zigbee_cfg, sizeof(zigbee_cfg_v0_t));
        memset(&g_tcpip_cfgs[0], 0, sizeof(g_tcpip_cfgs));
        memset(&g_zigbee_cfg, 0, sizeof(g_zigbee_cfg));
        if(tcpip_cfg_ok) {
            for(int i = 0; i < BOARD_TCPIP_MAX; i++) {
                g_tcpip_cfgs[i].mode = TCP_IP_M_NORMAL;
                g_tcpip_cfgs[i].enable = tcpip_cfg->v0[i].enable;
                g_tcpip_cfgs[i].tcpip_type = tcpip_cfg->v0[i].tcpip_type;
                g_tcpip_cfgs[i].tcpip_cs = tcpip_cfg->v0[i].tcpip_cs;
                strcpy(g_tcpip_cfgs[i].peer, tcpip_cfg->v0[i].peer);
                g_tcpip_cfgs[i].port = tcpip_cfg->v0[i].port;
                g_tcpip_cfgs[i].interval = tcpip_cfg->v0[i].interval;
                g_tcpip_cfgs[i].keepalive = tcpip_cfg->v0[i].keepalive;
                g_tcpip_cfgs[i].cfg.normal.proto_type = tcpip_cfg->v0[i].proto_type;
                g_tcpip_cfgs[i].cfg.normal.proto_ms = tcpip_cfg->v0[i].proto_ms;
                g_tcpip_cfgs[i].cfg.normal.maddress = tcpip_cfg->v0[i].maddress;
            }
        }
        if(xfer_net_cfg_ok) {
            int ofs_ary[3] = {0, BOARD_ENET_TCPIP_NUM, BOARD_TCPIP_MAX};
            for(int index = 0; index < 2; index++) {
                int n = 0;
                for(int i = ofs_ary[index]; i < ofs_ary[index+1]; i++) {
                    if(!g_tcpip_cfgs[i].enable) {
                        n = i; break;
                    }
                }
                if(n >= ofs_ary[index+1]) n = ofs_ary[index];
                if(xfer_net_cfg->v0[index].enable) {
                    g_tcpip_cfgs[n].mode = TCP_IP_M_XFER;
                    g_tcpip_cfgs[n].enable = xfer_net_cfg->v0[index].enable;
                    g_tcpip_cfgs[n].tcpip_type = xfer_net_cfg->v0[index].tcpip_type;
                    g_tcpip_cfgs[n].tcpip_cs = xfer_net_cfg->v0[index].tcpip_cs;
                    strcpy(g_tcpip_cfgs[n].peer, xfer_net_cfg->v0[index].peer);
                    g_tcpip_cfgs[n].port = xfer_net_cfg->v0[index].port;
                    g_tcpip_cfgs[n].interval = 0;
                    g_tcpip_cfgs[n].keepalive = RT_TRUE;
                    g_tcpip_cfgs[n].cfg.xfer.mode = XFER_M_GW;
                    g_tcpip_cfgs[n].cfg.xfer.cfg.gw.proto_type = xfer_net_cfg->v0[index].proto_type;
                    g_tcpip_cfgs[n].cfg.xfer.cfg.gw.dst_type = xfer_net_cfg->v0[index].dst_type;
                    g_tcpip_cfgs[n].cfg.xfer.cfg.gw.dst_cfg = xfer_net_cfg->v0[index].dst_cfg;
                }
            }
        }
        if(zigbee_cfg_ok) {
            memcpy(&g_zigbee_cfg, zigbee_cfg, sizeof(zigbee_cfg_v0_t));
            g_zigbee_cfg.tmode = ZGB_TM_GW;
        }

        if(tcpip_cfg_ok || xfer_net_cfg_ok) {
            board_cfg_write(TCPIP_CFG_OFS, &g_tcpip_cfgs[0], sizeof(g_tcpip_cfgs));
        }
        if(zigbee_cfg_ok) {
            board_cfg_write(ZIGBEE_CFG_OFS, &g_zigbee_cfg, sizeof(g_zigbee_cfg));
        }

        g_cfg_info.usVer = CFG_VER;
        board_cfg_write(CFG_INFO_OFS, &g_cfg_info, sizeof(g_cfg_info));
    }

    if(tcpip_cfg) rt_free(tcpip_cfg);
    if(xfer_net_cfg) rt_free(xfer_net_cfg);
    if(zigbee_cfg) rt_free(zigbee_cfg);
}

rt_bool_t cfg_recover_try(void)
{
    rt_bool_t ret = RT_FALSE;
    
    if(g_cfg_info.usVer != CFG_VER) {
        if(g_cfg_info.usVer == 0 && CFG_VER == 1) {
            _cfg_recover_0_2_1();
        }

        ret = RT_TRUE;
    }

    return ret;
}

char *__readline(int fd, int bufsz, char *line)
{
    int cur = lseek(fd, 0, SEEK_CUR);
    int len = read(fd, line, bufsz-1);
    int index = 0;

    line[bufsz-1] = '\0';
    if( len > 0 ) {
        while(line[index++] != '\n' && index < len);
        if(index < len) {
            line[index] = '\0';
            lseek(fd, cur + index, SEEK_SET);
        }
        if(index >= 1 && line[index-1] == '\n') line[index-1] = '\0';
        if(index >= 2 && line[index-2] == '\r') line[index-2] = '\0';
        return line;
    }
    return RT_NULL;
}

char *__check_cfg_line(char *line)
{
    int len = strlen(line);
    if( line[0] == '#' && 
        line[1] == '#' && 
        line[2] == '{' && 
        line[len-3] == '}' && 
        line[len-2] == '#' && 
        line[len-1] == '#' )
     {
        line[len-2] = '\0';

        return &line[2];
     }

     return RT_NULL;
}

int __parse_cfg_ver(char *line)
{
    int ver = CFG_VER;
    cJSON *json = cJSON_Parse(line);
    if(json) {
        ver = cJSON_GetInt(json, "ver", -1);
    }
    cJSON_Delete(json);

    return ((ver>=0)?(ver):(CFG_VER));
}

extern void setNetCfgWithJson(cJSON *pCfg);
extern void setTcpipCfgWithJson(cJSON *pCfg);
extern void setUartCfgWithJson(cJSON *pCfg);
extern void cfgSetVarManageExtDataWithJson(cJSON *pCfg);
extern void setDiCfgWithJson(cJSON *pDiCfg, rt_bool_t save_flag);
extern void setDoCfgWithJson(cJSON *pDoCfg, rt_bool_t save_flag);
extern void setAnalogCfgWithJson(cJSON *pCfg);
extern void setGPRSNetCfgWithJson(cJSON *pCfg);
extern rt_err_t setGPRSWorkCfgWithJson(cJSON *pCfg);
extern void setAuthCfgWithJson(cJSON *pCfg);
extern rt_err_t setStorageCfgWithJson(cJSON *pCfg);
extern void setHostCfgWithJson(cJSON *pCfg);
extern rt_err_t setZigbeeCfgWithJson(cJSON *pCfg);
extern rt_bool_t setXferUartCfgWithJson(cJSON *pXferUCfg, rt_bool_t save_flag);
extern void varmanage_free_all(void);

static rt_bool_t _ext_var_first = RT_TRUE;
static int       _ext_var_cnt = 0;

rt_bool_t __parse_cfg_line(char *line, int ver)
{
    rt_bool_t ret = RT_FALSE;
    cJSON *json = cJSON_Parse(line);
    if(json) {
        const char *type = cJSON_GetString(json, "type", RT_NULL);
        cJSON *cfg = cJSON_GetObjectItem(json, "cfg");
        if(type && cfg) {
            if(0 == strcasecmp(type, "net")) {
                setNetCfgWithJson(cfg);
            } else if(0 == strcasecmp(type, "tcpip")) {
                setTcpipCfgWithJson(cfg);
            } else if(0 == strcasecmp(type, "uart")) {
                setUartCfgWithJson(cfg);
            } else if(0 == strcasecmp(type, "extvar")) {
                if(_ext_var_first) {
                    if(storage_cfg_varext_del_all(RT_FALSE)) {
                        varmanage_free_all();
                    }
                    _ext_var_first = RT_FALSE;
                }
                cfgSetVarManageExtDataWithJson(cfg);
                _ext_var_cnt++;
            } else if(0 == strcasecmp(type, "di")) {
                setDiCfgWithJson(cfg, RT_TRUE);
            } else if(0 == strcasecmp(type, "do")) {
                setDoCfgWithJson(cfg, RT_TRUE);
            } else if(0 == strcasecmp(type, "ai")) {
                setAnalogCfgWithJson(cfg);
            } else if(0 == strcasecmp(type, "gprs_net")) {
                setGPRSNetCfgWithJson(cfg);
            } else if(0 == strcasecmp(type, "gprs_work")) {
                setGPRSWorkCfgWithJson(cfg);
            } else if(0 == strcasecmp(type, "auth")) {
                setAuthCfgWithJson(cfg);
            } else if(0 == strcasecmp(type, "storage")) {
                setStorageCfgWithJson(cfg);
            } else if(0 == strcasecmp(type, "host")) {
                setHostCfgWithJson(cfg);
            } else if(0 == strcasecmp(type, "zigbee")) {
                setZigbeeCfgWithJson(cfg);
            } else if(0 == strcasecmp(type, "uart_adds")) {
                setXferUartCfgWithJson(cfg, RT_TRUE);
            }
        }
    }
    cJSON_Delete(json);
    return ret;
}

static rt_bool_t s_cfg_recover_ing = RT_FALSE;

rt_bool_t cfg_recover_busy(void)
{
    return s_cfg_recover_ing;
}

rt_bool_t cfg_recover_with_json(const char *path)
{
#define _LINE_BUFSZ     (4096+8)
    rt_bool_t ret = RT_FALSE;
    rt_bool_t first = RT_FALSE;
    int ver;
    int fd;

    s_cfg_recover_ing = RT_TRUE;

    fd = open(path, O_RDONLY, 0666);
    if (fd < 0) {
        return RT_FALSE;
    }

    {
        char *line_buf = rt_malloc(_LINE_BUFSZ);
        if(line_buf) {
            char *line = RT_NULL;
            _ext_var_cnt = 0;
            _ext_var_first = RT_TRUE;
            while(1) {
                line = __readline(fd, _LINE_BUFSZ, line_buf);
                if(line) {
                    line = __check_cfg_line(line);
                    if(line) {
                        if(!first) {
                            ver = __parse_cfg_ver(line);
                            first = RT_TRUE;
                        } else {
                            __parse_cfg_line(line, ver);
                        }
                    }
                } else {
                    break;
                }
            }
            rt_free(line_buf);
            if(!_ext_var_first) {
                bStorageDoCommit();
            }
            ret = first;
        }
    }
    close(fd);

    s_cfg_recover_ing = RT_FALSE;

    return ret;
}

typedef void (*cfgfunc)(int, cJSON *);

void __write_cfg_line(int fd, const char *type, cfgfunc cfunc, int n)
{
    cJSON *item = cJSON_CreateObject();
    cJSON *cfg = item?cJSON_CreateObject():RT_NULL;
    if(item && cfg) {
        cJSON_AddStringToObject(item, "type", type);
        cJSON_AddItemToObject(item, "cfg", cfg);
        if(cfunc) cfunc(n, cfg);
        {
            char *szJson = cJSON_PrintUnformatted(item);
            if(szJson) {
                write(fd, "##", 2);
                write(fd, szJson, strlen(szJson));
                write(fd, "##\n", 3);
            }
            rt_free(szJson);
        }
    }
    if(item) cJSON_Delete(item);
}

extern void jsonNetCfg(int n, cJSON *pItem);
extern void jsonTcpipCfg(int n, cJSON *pItem);
extern void jsonUartCfg(int n, cJSON *pItem);
extern int nExtDataListCnt(void);
extern void jsonFillExtDataInfoWithNum(int n, cJSON *pItem);
extern void jsonDiCfg(int n, cJSON *pItem);
extern void jsonDoCfg(int n, cJSON *pItem);
extern void jsonAnalogCfg(int n, cJSON *pItem);
extern void jsonGPRSNetCfg(int n, cJSON *pItem);
extern void jsonGPRSWorkCfg(int n, cJSON *pItem);
extern void jsonAuthCfg(int n, cJSON *pItem);
extern void jsonStorageCfg(int n, cJSON *pItem);
extern void jsonHostCfg(int n, cJSON *pItem);
extern void jsonZigbeeCfg(int n, cJSON *pItem);
extern void jsonXferUartCfg(int n, cJSON *pItem);

rt_bool_t cfg_save_with_json(const char *path)
{
#define _LINE_BUFSZ     (4096+8)
    rt_bool_t ret = RT_FALSE;
    int fd;

    fd = open(path, O_CREAT | O_TRUNC | O_RDWR, 0666);
    if (fd < 0) {
        return RT_FALSE;
    }

    // VER
    {
        char szJson[64] = "";
        rt_sprintf(szJson, "##{\"ver\":%d}##\n", CFG_VER);
        write(fd, szJson, strlen(szJson));
    }


    // others
    __write_cfg_line( fd, "net", jsonNetCfg, 0);
    for (int n = 0; n < BOARD_TCPIP_MAX; n++) {
        __write_cfg_line( fd, "tcpip", jsonTcpipCfg, n);
    }
    for (int n = 0; n <= PROTO_DEV_RS_MAX; n++) {
        __write_cfg_line( fd, "uart", jsonUartCfg, n);
    }
    {
        int cnt = nExtDataListCnt();
        for (int n = 0; n < cnt; n++) {
            __write_cfg_line( fd, "extvar", jsonFillExtDataInfoWithNum, n);
        }
    }
    for (int n = 0; n < DI_CHAN_NUM; n++) {
        __write_cfg_line( fd, "di", jsonDiCfg, n);
    }
    for (int n = 0; n < DO_CHAN_NUM; n++) {
        __write_cfg_line( fd, "do", jsonDoCfg, n);
    }
    for (int n = 0; n < ADC_CHANNEL_NUM; n++) {
        __write_cfg_line( fd, "ai", jsonAnalogCfg, n);
    }
    __write_cfg_line( fd, "gprs_net", jsonGPRSNetCfg, 0);
    __write_cfg_line( fd, "gprs_work", jsonGPRSWorkCfg, 0);
    __write_cfg_line( fd, "auth", jsonAuthCfg, 0);
    __write_cfg_line( fd, "storage", jsonStorageCfg, 0);
    __write_cfg_line( fd, "host", jsonHostCfg, 0);
    __write_cfg_line( fd, "zigbee", jsonZigbeeCfg, 0);
    __write_cfg_line( fd, "uart_adds", jsonXferUartCfg, 0);

    close(fd);
    
    return ret;
}

DEF_CGI_HANDLER(saveCfgWithJson)
{
    rt_err_t err = RT_EOK;
    /*const char *szJSON = CGI_GET_ARG("arg");
    cJSON *pCfg = szJSON ? cJSON_Parse(szJSON) : RT_NULL;
    if (pCfg) {
        setTcpipCfgWithJson(pCfg);
    } else {
        err = RT_ERROR;
    }
    cJSON_Delete(pCfg);*/

    cfg_save_with_json("/cfg/board_json.cfg");

    WEBS_PRINTF("{\"ret\":%d}", err);
    WEBS_DONE(200);
}

