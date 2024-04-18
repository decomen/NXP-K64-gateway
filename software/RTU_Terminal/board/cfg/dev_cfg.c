#include <board.h>

#define ADC_CAL_INFO_OFS            (1024*1024-3*4096)
#define ADC_CAL_INFO_SIZE           (4096)

void SaveAdcCalInfoToFs(void)
{
    rt_uint8_t *buffer = rt_calloc(1, 4096);
    rt_uint16_t usCheck = 0;
    if (buffer) {
        rt_uint32_t magic = CFG_MAGIC;
        memcpy(buffer, &magic, 4);

        memcpy(buffer + 4, gCalEntryBak, sizeof(gCalEntryBak));
        usCheck = usMBCRC16(gCalEntryBak, sizeof(gCalEntryBak));
        memcpy(buffer + 4 + sizeof(gCalEntryBak), &usCheck, 2);

        FLASH_EraseSector(ADC_CAL_INFO_OFS);
        FLASH_WriteSector(ADC_CAL_INFO_OFS, buffer, 4096);

        rt_free(buffer);
    }
}

static void ReadAdcCalInfoFromFs(void)
{
    rt_uint32_t magic = 0;
    rt_uint16_t usCheck = 0;

    memset(gCalEntry, 0, sizeof(gCalEntry));
    memcpy(&magic, (void const *)ADC_CAL_INFO_OFS, 4);
    if (CFG_MAGIC == magic) {
        memcpy(gCalEntry, (void const *)(ADC_CAL_INFO_OFS + 4), sizeof(gCalEntry));
        memcpy(&usCheck, (void const *)(ADC_CAL_INFO_OFS + 4 + sizeof(gCalEntry)), 2);
        if (usMBCRC16(gCalEntry, sizeof(gCalEntry)) != usCheck) {
            SetAdcCalCfgDefault();
        }
    } else {
        SetAdcCalCfgDefault();
    }
}

void vAdcCalCfgInit(void)
{
    FLASH_Init();
    ReadAdcCalInfoFromFs();

}


#define DEV_INFO_OFS            (1024*1024-4096)
#define DEV_INFO_SIZE           (4096)

static void prvSetDefaultMAC(void)
{
    rt_uint16_t usCrc16 = 0;
    rt_uint32_t usCrc32 = 0;
    rt_uint32_t buf[4] = { SIM->UIDH, SIM->UIDMH, SIM->UIDML, SIM->UIDL };

    usCrc16 = usMBCRC16((UCHAR *)buf, sizeof(buf));
    usCrc32 = ulRTCrc32(0, (void *)buf, sizeof(buf));

    memcpy(&g_xDevInfoReg.xDevInfo.xNetMac.mac[0], &usCrc16, 2);
    memcpy(&g_xDevInfoReg.xDevInfo.xNetMac.mac[2], &usCrc32, 4);

    g_xDevInfoReg.xDevInfo.xNetMac.mac[0] /= 2;
    g_xDevInfoReg.xDevInfo.xNetMac.mac[0] *= 2;
}

static void prvSetDevCfgDefault(void)
{
    memset(&g_xDevInfoReg, 0, sizeof(g_xDevInfoReg));
    prvSetDefaultMAC();
}

void prvSaveDevInfoToFs(void);

static void prvReadDevInfoFromFs(void)
{
    rt_uint32_t magic = 0;
    rt_uint16_t usCheck = 0;

    memcpy(&magic, (void const *)DEV_INFO_OFS, 4);
    if (CFG_MAGIC == magic) {
        memcpy(&g_xDevInfoReg, (void const *)(DEV_INFO_OFS + 4), sizeof(g_xDevInfoReg));
        memcpy(&usCheck, (void const *)(DEV_INFO_OFS + 4 + sizeof(g_xDevInfoReg)), 2);
        if (usMBCRC16(&g_xDevInfoReg, sizeof(g_xDevInfoReg)) != usCheck) {
            prvSetDevCfgDefault();
        } else {
            char strbuf[40] = {0};
            rt_kprintf("------------------------dev_info---------------------\n");
            memcpy(strbuf, g_xDevInfoReg.xDevInfo.xSN.szSN, sizeof(DevSN_t));
            rt_kprintf("sn:%s\n", strbuf);
            memset(strbuf, 0, sizeof(strbuf));
            memcpy(strbuf, g_xDevInfoReg.xDevInfo.hwid.hwid, sizeof(DevHwid_t));
            rt_kprintf("hwid:%s\n", strbuf);
            rt_kprintf("-----------------------------------------------------\n");
        }
    } else {
        prvSetDevCfgDefault();
#if 1
    	memcpy(g_xDevInfoReg.xDevInfo.xSN.szSN , "SSSSSSSSSSSSSSS1", 16);
    	strcpy(g_xDevInfoReg.xDevInfo.xOEM.szOEM, "");
    	//strcpy(g_xDevInfoReg.xDevInfo.xIp.szIp, request.info.xIp.szIp,sizeof(request.info.xIp));
    	g_xDevInfoReg.xDevInfo.usYear = 2015;
    	g_xDevInfoReg.xDevInfo.usMonth = 1;
    	g_xDevInfoReg.xDevInfo.usDay = 1;

    	rt_uint32_t hwid[4] = { SIM->UIDH, SIM->UIDMH, SIM->UIDML, SIM->UIDL };
    	char *phwid = (char *)hwid;
    	memset( g_xDevInfoReg.xDevInfo.hwid.hwid, 0, sizeof(g_xDevInfoReg.xDevInfo.hwid) );
    	sprintf(g_xDevInfoReg.xDevInfo.hwid.hwid, "%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X", phwid[0],phwid[1],phwid[2],phwid[3],phwid[4],phwid[5],
    		phwid[6],phwid[7],phwid[8],phwid[9],phwid[10],phwid[11],phwid[12],phwid[13],phwid[14], phwid[15]);
        //memcpy(g_xDevInfoReg.xDevInfo.xPN.szPN, request.info.xPN.szPN,sizeof(request.info.xPN));
        //prvSaveDevInfoToFs();
#endif
    }

    g_xDevInfoReg.xDevInfo.usHWVer = HW_VER_VERCODE;
    g_xDevInfoReg.xDevInfo.usSWVer = SW_VER_VERCODE;
}

void prvSaveDevInfoToFs(void)
{
    rt_uint8_t *buffer = rt_calloc(1, 4096);
    rt_uint16_t usCheck = 0;
    if (buffer) {
        rt_uint32_t magic = CFG_MAGIC;

        g_xDevInfoReg.xDevInfo.usHWVer = HW_VER_VERCODE;
        g_xDevInfoReg.xDevInfo.usSWVer = SW_VER_VERCODE;

        memcpy(buffer, &magic, 4);
        memcpy(buffer + 4, &g_xDevInfoReg, sizeof(g_xDevInfoReg));
        usCheck = usMBCRC16(&g_xDevInfoReg, sizeof(g_xDevInfoReg));
        memcpy(buffer + 4 + sizeof(g_xDevInfoReg), &usCheck, 2);

        FLASH_EraseSector(DEV_INFO_OFS);
        FLASH_WriteSector(DEV_INFO_OFS, buffer, 4096);

        rt_free(buffer);
    }
}

void vDevCfgInit(void)
{
    FLASH_Init();
    prvReadDevInfoFromFs();
}

void vDevRefreshTime(void)
{
    struct tm lt;
    rt_time_t t = time(NULL);
    localtime_r(&t, &lt);
    g_xDevInfoReg.xDevInfo.usYear = lt.tm_year + 1900;
    g_xDevInfoReg.xDevInfo.usMonth = lt.tm_mon + 1;
    g_xDevInfoReg.xDevInfo.usDay = lt.tm_mday;
    g_xDevInfoReg.xDevInfo.usHour = lt.tm_hour;
    g_xDevInfoReg.xDevInfo.usMin = lt.tm_min;
    g_xDevInfoReg.xDevInfo.usSec = lt.tm_sec;
}

// for webserver
DEF_CGI_HANDLER(setDefaultMac)
{
    prvSetDefaultMAC();
    prvSaveDevInfoToFs();

    WEBS_PRINTF("{\"ret\":%d}", RT_EOK);
    WEBS_DONE(200);
}

DEF_CGI_HANDLER(setDevInfo)
{
    rt_err_t err = RT_EOK;
    const char *szJSON = CGI_GET_ARG("arg");
    cJSON *pCfg = szJSON ? cJSON_Parse(szJSON) : RT_NULL;

    if (pCfg) {
        const char *psk = cJSON_GetString(pCfg, "psk", RT_NULL);
        const char *id = cJSON_GetString(pCfg, "id", RT_NULL);
        //int hw = cJSON_GetInt( pCfg, "hw", -1 );
        //int sw = cJSON_GetInt( pCfg, "sw", -1 );
        const char *om = cJSON_GetString(pCfg, "om", RT_NULL);
        const char *mc = cJSON_GetString(pCfg, "mc", RT_NULL);
        int ye = cJSON_GetInt(pCfg, "ye", -1);
        int mo = cJSON_GetInt(pCfg, "mo", -1);
        int da = cJSON_GetInt(pCfg, "da", -1);
        int dh = cJSON_GetInt(pCfg, "dh", -1);
        int hm = cJSON_GetInt(pCfg, "hm", -1);
        int ms = cJSON_GetInt(pCfg, "ms", -1);

        if (psk != RT_NULL && 0 == strcmp(psk, "zjADFDf5d5fDAS8RT4asdh")) {
            struct tm lt = { 0 };
            if (id != RT_NULL && strlen(id) <= sizeof(DevSN_t)) {
                memset(&g_xDevInfoReg.xDevInfo.xSN, 0, sizeof(DevSN_t));
                memcpy(&g_xDevInfoReg.xDevInfo.xSN, id, strlen(id));
            }
            if (om != RT_NULL && strlen(om) <= sizeof(DevOEM_t)) {
                memset(&g_xDevInfoReg.xDevInfo.xOEM, 0, sizeof(DevOEM_t));
                memcpy(&g_xDevInfoReg.xDevInfo.xOEM, om, strlen(om));
            }
            if (mc != RT_NULL && 17 == strlen(mc)) {
                int a[6];
                memset(&g_xDevInfoReg.xDevInfo.xNetMac, 0, sizeof(DevNetMac_t));
                if (sscanf(mc, "%02X:%02X:%02X:%02X:%02X:%02X", &a[0], &a[1], &a[2], &a[3], &a[4], &a[5]) >= 0) {
                    if (a[0] % 2 != 0) {
                        g_xDevInfoReg.xDevInfo.xNetMac.mac[0] = (a[0] & 0xFF);
                        g_xDevInfoReg.xDevInfo.xNetMac.mac[1] = (a[1] & 0xFF);
                        g_xDevInfoReg.xDevInfo.xNetMac.mac[2] = (a[2] & 0xFF);
                        g_xDevInfoReg.xDevInfo.xNetMac.mac[3] = (a[3] & 0xFF);
                        g_xDevInfoReg.xDevInfo.xNetMac.mac[4] = (a[4] & 0xFF);
                        g_xDevInfoReg.xDevInfo.xNetMac.mac[5] = (a[5] & 0xFF);
                    }
                } else {
                    err = -1;
                }
            }
            //if( hw >= 0 ) g_xDevInfoReg.xDevInfo.usHWVer = hw;
            //if( sw >= 0 ) g_xDevInfoReg.xDevInfo.usSWVer = sw;
            lt.tm_year = g_xDevInfoReg.xDevInfo.usYear - 1900;
            lt.tm_mon = g_xDevInfoReg.xDevInfo.usMonth - 1;
            lt.tm_mday = g_xDevInfoReg.xDevInfo.usDay;
            lt.tm_hour = g_xDevInfoReg.xDevInfo.usHour;
            lt.tm_min = g_xDevInfoReg.xDevInfo.usMin;
            lt.tm_sec = g_xDevInfoReg.xDevInfo.usSec;
            set_timestamp(mktime(&lt) - g_host_cfg.nTimezone);
        } else {
            err = RT_EIO;
        }

        cJSON_Delete(pCfg);

        prvSaveDevInfoToFs();
    } else {
        err = RT_ERROR;
    }

    WEBS_PRINTF("{\"ret\":%d}", err);
    WEBS_DONE(200);
}

DEF_CGI_HANDLER(getDevInfo)
{
    rt_err_t err = RT_EOK;
    char *szRetJSON = RT_NULL;
    char buf[32];
    cJSON *pItem = cJSON_CreateObject();

    if(pItem) {
        cJSON_AddNumberToObject(pItem, "ret", RT_EOK);
        memset(buf, 0, sizeof(buf));
        memcpy(buf, &g_xDevInfoReg.xDevInfo.xSN, sizeof(DevSN_t));
        cJSON_AddStringToObject(pItem, "sn", buf);
        memset(buf, 0, sizeof(buf));
        strcpy(buf, g_host_cfg.szId);
        cJSON_AddStringToObject(pItem, "id", buf);
        memset(buf, 0, sizeof(buf));
        memcpy(buf, &g_xDevInfoReg.xDevInfo.xOEM, sizeof(DevOEM_t));
        cJSON_AddStringToObject(pItem, "om", buf);
        memset(buf, 0, sizeof(buf));
        sprintf(buf, "%02X:%02X:%02X:%02X:%02X:%02X",
                g_xDevInfoReg.xDevInfo.xNetMac.mac[0],
                g_xDevInfoReg.xDevInfo.xNetMac.mac[1],
                g_xDevInfoReg.xDevInfo.xNetMac.mac[2],
                g_xDevInfoReg.xDevInfo.xNetMac.mac[3],
                g_xDevInfoReg.xDevInfo.xNetMac.mac[4],
                g_xDevInfoReg.xDevInfo.xNetMac.mac[5]
               );
        cJSON_AddStringToObject(pItem, "mc", buf);

        cJSON_AddNumberToObject(pItem, "hw", g_xDevInfoReg.xDevInfo.usHWVer);
        cJSON_AddNumberToObject(pItem, "sw", g_xDevInfoReg.xDevInfo.usSWVer);

        vDevRefreshTime();
        memset(buf, 0, sizeof(buf));
        sprintf(buf, "%u/%02d/%02d %02d:%02d:%02d",
                g_xDevInfoReg.xDevInfo.usYear,
                g_xDevInfoReg.xDevInfo.usMonth,
                g_xDevInfoReg.xDevInfo.usDay,
                g_xDevInfoReg.xDevInfo.usHour,
                g_xDevInfoReg.xDevInfo.usMin,
                g_xDevInfoReg.xDevInfo.usSec
               );
        cJSON_AddStringToObject(pItem, "dt", buf);

        memset(buf, 0, sizeof(buf));
        extern rt_bool_t g_zigbee_init;
        if (g_zigbee_init) {
            sprintf(buf, "%X.%02X", (g_zigbee_cfg.usVer >> 8) & 0xFF, g_zigbee_cfg.usVer & 0xFF);
        }
        cJSON_AddStringToObject(pItem, "zgbver", buf);

        rt_uint32_t ulRunTime = rt_millisecond_from_tick(rt_tick_get()) / 1000 / 60;
        memset(buf, 0, sizeof(buf));
        sprintf(buf, "%02d hours, %02d min", ulRunTime / 60, ulRunTime % 60);
        cJSON_AddStringToObject(pItem, "rt", buf);

        cJSON_AddNumberToObject(pItem, "reg", g_isreg);
        cJSON_AddNumberToObject(pItem, "tta", REG_TEST_OVER_TIME);
        cJSON_AddNumberToObject(pItem, "ttt", g_reg_info.test_time);

        szRetJSON = cJSON_PrintUnformatted(pItem);
        if(szRetJSON) {
            WEBS_PRINTF(szRetJSON);
            rt_free(szRetJSON);
        }
        cJSON_Delete(pItem);
    }

    WEBS_DONE(200);
}

DEF_CGI_HANDLER(setTime)
{
    rt_err_t err = RT_EOK;
    const char *szJSON = CGI_GET_ARG("arg");
    cJSON *pCfg = szJSON ? cJSON_Parse(szJSON) : RT_NULL;
    if (pCfg) {
        struct tm lt = { 0 };
        int ye = cJSON_GetInt(pCfg, "ye", -1);
        int mo = cJSON_GetInt(pCfg, "mo", -1);
        int da = cJSON_GetInt(pCfg, "da", -1);
        int dh = cJSON_GetInt(pCfg, "dh", -1);
        int hm = cJSON_GetInt(pCfg, "hm", -1);
        int ms = cJSON_GetInt(pCfg, "ms", -1);

        if (ye >= 0) g_xDevInfoReg.xDevInfo.usYear = ye;
        if (mo >= 0) g_xDevInfoReg.xDevInfo.usMonth = mo;
        if (da >= 0) g_xDevInfoReg.xDevInfo.usDay = da;
        if (dh >= 0) g_xDevInfoReg.xDevInfo.usHour = dh;
        if (hm >= 0) g_xDevInfoReg.xDevInfo.usMin = hm;
        if (ms >= 0) g_xDevInfoReg.xDevInfo.usSec = ms;

        lt.tm_year = g_xDevInfoReg.xDevInfo.usYear - 1900;
        lt.tm_mon = g_xDevInfoReg.xDevInfo.usMonth - 1;
        lt.tm_mday = g_xDevInfoReg.xDevInfo.usDay;
        lt.tm_hour = g_xDevInfoReg.xDevInfo.usHour;
        lt.tm_min = g_xDevInfoReg.xDevInfo.usMin;
        lt.tm_sec = g_xDevInfoReg.xDevInfo.usSec;
        set_timestamp(mktime(&lt) - g_host_cfg.nTimezone);
    } else {
        err = RT_ERROR;
    }
    cJSON_Delete(pCfg);

    WEBS_PRINTF("{\"ret\":%d}", err);
    WEBS_DONE(200);
}

