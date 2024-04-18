
#include <board.h>
#include <stdio.h>

net_cfg_t g_net_cfg;
const net_cfg_t c_net_cfg_default = {
    .dhcp = RT_FALSE,
    .ipaddr = "192.168.0.100",
    .netmask = "255.255.255.0",
    //.gw = "192.168.1.1",
    //.dns = "209.96.128.86"
    .gw = "0.0.0.0",
    .dns = "0.0.0.0"
};

static void prvSetNetCfgDefault(void)
{
    memset(&g_net_cfg, 0, sizeof(g_net_cfg));
    g_net_cfg = c_net_cfg_default;
    if (g_xDevInfoReg.xDevInfo.xIp.szIp[0] == 0) {
        g_net_cfg.dhcp = RT_TRUE;
    } else {
        g_net_cfg.dhcp = RT_FALSE;
        sprintf(g_net_cfg.ipaddr, "%u.%u.%u.%u",
                g_xDevInfoReg.xDevInfo.xIp.szIp[0],
                g_xDevInfoReg.xDevInfo.xIp.szIp[1],
                g_xDevInfoReg.xDevInfo.xIp.szIp[2],
                g_xDevInfoReg.xDevInfo.xIp.szIp[3]);
    }
}

static void prvReadNetCfgFromFs(void)
{
    if (!board_cfg_read(NET_CFG_OFS, &g_net_cfg, sizeof(g_net_cfg))) {
        prvSetNetCfgDefault();
    }
    //g_net_cfg.dhcp = 1;
}

static void prvSaveNetCfgToFs(void)
{
    if (!board_cfg_write(NET_CFG_OFS, &g_net_cfg, sizeof(g_net_cfg))) {
        ; //prvSetNetCfgDefault();
    }
}

void vSaveNetCfgToFs(void)
{
    prvSaveNetCfgToFs();
    vDoSystemReset();
}

rt_err_t net_cfg_init(void)
{
    prvReadNetCfgFromFs();

    return RT_EOK;
}

void jsonNetCfg(int n, cJSON *pItem)
{
    extern struct netif *netif_list;
    char *netstatus = rt_malloc(300);

    cJSON_AddNumberToObject(pItem, "dh", g_net_cfg.dhcp);
    cJSON_AddStringToObject(pItem, "ip", g_net_cfg.ipaddr);
    cJSON_AddStringToObject(pItem, "mask", g_net_cfg.netmask);
    cJSON_AddStringToObject(pItem, "gw", g_net_cfg.gw);
    cJSON_AddStringToObject(pItem, "dns", g_net_cfg.dns);

    if (netif_list && netstatus) {
        extern ip_addr_t dns_getserver(u8_t numdns);
        ip_addr_t ip_addr = dns_getserver(0);

        snprintf(netstatus, 300,
                 "INTERFACE: %c%c<br />"
                 "MTU &nbsp;: %d<br />"
                 "MAC &nbsp;: %02X:%02X:%02X:%02X:%02X:%02X<br />"
                 "FLAGS : %s %s %s<br />"
                 "I P &nbsp;: %s<br />",
                 netif_list->name[0], netif_list->name[1],
                 netif_list->mtu,
                 netif_list->hwaddr[0], netif_list->hwaddr[1], netif_list->hwaddr[2], netif_list->hwaddr[3], netif_list->hwaddr[4], netif_list->hwaddr[5],
                 netif_list->flags & NETIF_FLAG_UP ? "UP" : "DOWN",
                 netif_list->flags & NETIF_FLAG_LINK_UP ? "LINK_UP" : "LINK_DOWN",
                 netif_list->flags & NETIF_FLAG_DHCP ? "DHCP" : "",
                 ipaddr_ntoa(&(netif_list->ip_addr))
                );

        strcat(netstatus, "G W &nbsp;: ");
        strcat(netstatus, ipaddr_ntoa(&(netif_list->gw)));
        strcat(netstatus, "<br />MASK&nbsp;: ");
        strcat(netstatus, ipaddr_ntoa(&(netif_list->netmask)));
        strcat(netstatus, "<br />DNS &nbsp;: ");
        strcat(netstatus, ipaddr_ntoa(&(ip_addr)));
        strcat(netstatus, "<br />");
    }
    cJSON_AddStringToObject(pItem, "status", netstatus);
    rt_free(netstatus);
}

static void jsonNetInfo(cJSON *pItem)
{
    extern struct netif *netif_list;
    char buf[64] = { 0 };

    if (netif_list) {
        extern ip_addr_t dns_getserver(u8_t numdns);
        ip_addr_t ip_addr = dns_getserver(0);

        cJSON_AddNumberToObject(pItem, "dh", netif_list->flags & NETIF_FLAG_DHCP ? 1 : 0);
        snprintf(buf, sizeof(buf), "%02X:%02X:%02X:%02X:%02X:%02X",
                 netif_list->hwaddr[0],
                 netif_list->hwaddr[1],
                 netif_list->hwaddr[2],
                 netif_list->hwaddr[3],
                 netif_list->hwaddr[4],
                 netif_list->hwaddr[5]
                );
        cJSON_AddStringToObject(pItem, "mac", buf);
        cJSON_AddStringToObject(pItem, "ip", ipaddr_ntoa(&(netif_list->ip_addr)));
        cJSON_AddStringToObject(pItem, "mask", ipaddr_ntoa(&(netif_list->netmask)));
        cJSON_AddStringToObject(pItem, "gw", ipaddr_ntoa(&(netif_list->gw)));
        cJSON_AddStringToObject(pItem, "d1", ipaddr_ntoa(&(ip_addr)));
        ip_addr = dns_getserver(1);
        cJSON_AddStringToObject(pItem, "d2", ipaddr_ntoa(&(ip_addr)));
        snprintf(buf, sizeof(buf), "%s %s",
                 netif_list->flags & NETIF_FLAG_UP ? "UP" : "DOWN",
                 netif_list->flags & NETIF_FLAG_LINK_UP ? "LINK_UP" : "LINK_DOWN"
                );
        cJSON_AddStringToObject(pItem, "lk", buf);
    }
}

// for webserver
DEF_CGI_HANDLER(getNetInfo)
{
    char *szRetJSON = RT_NULL;
    cJSON *pItem = cJSON_CreateObject();
    if(pItem) {
        cJSON_AddNumberToObject(pItem, "ret", RT_EOK);
        jsonNetInfo(pItem);
        szRetJSON = cJSON_PrintUnformatted(pItem);
        if(szRetJSON) {
            WEBS_PRINTF( szRetJSON );
            rt_free( szRetJSON );
        }
    }
    cJSON_Delete( pItem );

    WEBS_DONE(200);
}

DEF_CGI_HANDLER(getNetCfg)
{
    char *szRetJSON = RT_NULL;
    cJSON *pItem = cJSON_CreateObject();
    if(pItem) {
        cJSON_AddNumberToObject(pItem, "ret", RT_EOK);
        jsonNetCfg(0, pItem);
        szRetJSON = cJSON_PrintUnformatted(pItem);
        if(szRetJSON) {
            WEBS_PRINTF( szRetJSON );
            rt_free( szRetJSON );
        }
    }
    cJSON_Delete( pItem );

    WEBS_DONE(200);
}

void setNetCfgWithJson(cJSON *pCfg)
{
    int dh = cJSON_GetInt(pCfg, "dh", -1);
    const char *ip = cJSON_GetString(pCfg, "ip", VAR_NULL);
    const char *mask = cJSON_GetString(pCfg, "mask", VAR_NULL);
    const char *gw = cJSON_GetString(pCfg, "gw", VAR_NULL);
    const char *dns = cJSON_GetString(pCfg, "dns", VAR_NULL);

    net_cfg_t net_cfg_bak = g_net_cfg;

    if (dh >= 0) g_net_cfg.dhcp = (dh != 0 ? VAR_TRUE : VAR_FALSE);
    if (ip && strlen(ip) < 16) {
        memset(g_net_cfg.ipaddr, 0, 16); strcpy(g_net_cfg.ipaddr, ip);
    }
    if (mask && strlen(mask) < 16) {
        memset(g_net_cfg.netmask, 0, 16); strcpy(g_net_cfg.netmask, mask);
    }
    if (gw && strlen(gw) < 16) {
        memset(g_net_cfg.gw, 0, 16); strcpy(g_net_cfg.gw, gw);
    }
    if (dns && strlen(dns) < 16) {
        memset(g_net_cfg.dns, 0, 16); strcpy(g_net_cfg.dns, dns);
    }

    if (memcmp(&net_cfg_bak, &g_net_cfg, sizeof(g_net_cfg)) != 0) {
        /*struct netif * netif = netif_list;
        dhcp_stop( netif );
        set_if( netif->name, g_net_cfg.ipaddr, g_net_cfg.gw, g_net_cfg.netmask );
        set_dns( g_net_cfg.dns );*/
        prvSaveNetCfgToFs();
    }
}

DEF_CGI_HANDLER(setNetCfg)
{
    rt_err_t err = RT_EOK;
    const char *szJSON = CGI_GET_ARG("arg");
    cJSON *pCfg = szJSON ? cJSON_Parse(szJSON) : RT_NULL;
    if (pCfg) {
        setNetCfgWithJson(pCfg);
    } else {
        err = RT_ERROR;
    }
    cJSON_Delete(pCfg);

    WEBS_PRINTF("{\"ret\":%d}", err);
    WEBS_DONE(200);
}

#if NET_HAS_GPRS
extern void jsonGPRSState( cJSON *pItem );
#endif
extern void jsonTcpipState(int n, cJSON *pItem);

DEF_CGI_HANDLER(getAllNetInfo)
{
    char *szRetJSON = RT_NULL;
    WEBS_PRINTF("{\"ret\":0,\"netinfo\":");
    {
        cJSON *pItem = cJSON_CreateObject();
        if(pItem) {
            jsonNetInfo(pItem);
            szRetJSON = cJSON_PrintUnformatted(pItem);
            if(szRetJSON) {
                WEBS_PRINTF( szRetJSON );
                rt_free( szRetJSON );
            }
        }
        cJSON_Delete( pItem );
    }
#if NET_HAS_GPRS
    WEBS_PRINTF(",\"gprsinfo\":");
    {
        cJSON *pItem = cJSON_CreateObject();
        if(pItem) {
            jsonGPRSState(pItem);
            szRetJSON = cJSON_PrintUnformatted(pItem);
            if(szRetJSON) {
                WEBS_PRINTF( szRetJSON );
                rt_free( szRetJSON );
            }
        }
        cJSON_Delete( pItem );
    }
#endif
    WEBS_PRINTF(",\"tcpipinfo\":[");
    rt_bool_t first = RT_TRUE;
    for (int n = 0; n < BOARD_TCPIP_MAX; n++) {
        cJSON *pItem = cJSON_CreateObject();
        if(pItem) {
            if (!first) WEBS_PRINTF(",");
            first = RT_FALSE;
            jsonTcpipState(n, pItem);
            szRetJSON = cJSON_PrintUnformatted(pItem);
            if(szRetJSON) {
                WEBS_PRINTF( szRetJSON );
                rt_free( szRetJSON );
            }
        }
        cJSON_Delete( pItem );
    }
    WEBS_PRINTF("]}");
    WEBS_DONE(200);
}


