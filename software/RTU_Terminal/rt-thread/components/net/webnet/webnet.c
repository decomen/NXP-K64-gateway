

/*
 * File      : webnet.c
 * This file is part of RT-Thread RTOS/WebNet Server
 * COPYRIGHT (C) 2011, Shanghai Real-Thread Technology Co., Ltd
 *
 * All rights reserved.
 *
 * Change Logs:
 * Date           Author       Notes
 * 2011-08-02     Bernard      the first version
 */

#include <rtthread.h>
#include "lwip/sockets.h"

#include "webnet.h"
#include "session.h"
#include "module.h"

#include "board.h"
#include "board_cgi.c"

DEF_CGI_HANDLER(diskFormat);

DEF_CGI_HANDLER(devReset);

DEF_CGI_HANDLER(factoryReset);

DEF_CGI_HANDLER(getDevInfo);
DEF_CGI_HANDLER(setDevInfo);

DEF_CGI_HANDLER(setDefaultMac);
DEF_CGI_HANDLER(setTime);

DEF_CGI_HANDLER(getNetInfo);
DEF_CGI_HANDLER(getNetCfg);
DEF_CGI_HANDLER(setNetCfg);
DEF_CGI_HANDLER(getAllNetInfo);

DEF_CGI_HANDLER(getTcpipCfg);
DEF_CGI_HANDLER(setTcpipCfg);

DEF_CGI_HANDLER(getProtoManage);

DEF_CGI_HANDLER(setUartCfg);
DEF_CGI_HANDLER(getUartCfg);

DEF_CGI_HANDLER(getVarManageExtData);
DEF_CGI_HANDLER(setVarManageExtData);
DEF_CGI_HANDLER(delVarManageExtData);
DEF_CGI_HANDLER(getVarManageExtDataVals);

DEF_CGI_HANDLER(getDiCfg);
DEF_CGI_HANDLER(setDiCfg);
DEF_CGI_HANDLER(getDoCfg);
DEF_CGI_HANDLER(setDoCfg);

DEF_CGI_HANDLER(getDiValue);
DEF_CGI_HANDLER(setDoValue);
DEF_CGI_HANDLER(getDoValue);

DEF_CGI_HANDLER(setAnalogCfg);
DEF_CGI_HANDLER(getAnalogCfg);

DEF_CGI_HANDLER(setGPRSNetCfg);
DEF_CGI_HANDLER(getGPRSNetCfg);

DEF_CGI_HANDLER(setGPRSWorkCfg);
DEF_CGI_HANDLER(getGPRSWorkCfg);

DEF_CGI_HANDLER(getCpuUsage);
DEF_CGI_HANDLER(getGPRSState);
DEF_CGI_HANDLER(getTcpipState);

DEF_CGI_HANDLER(getAuthCfg);
DEF_CGI_HANDLER(setAuthCfg);

DEF_CGI_HANDLER(getStorageCfg);
DEF_CGI_HANDLER(setStorageCfg);

DEF_CGI_HANDLER(getHostCfg);
DEF_CGI_HANDLER(setHostCfg);

DEF_CGI_HANDLER(getZigbeeCfg);
DEF_CGI_HANDLER(setZigbeeCfg);
DEF_CGI_HANDLER(doZigbeeLearnNow);

DEF_CGI_HANDLER(getZigbeeList);


DEF_CGI_HANDLER(setXferUartCfg);
DEF_CGI_HANDLER(getXferUartCfg);

DEF_CGI_HANDLER(setWebsocketCfg);


DEF_CGI_HANDLER(reggetid);
DEF_CGI_HANDLER(doreg);

DEF_CGI_HANDLER(listDir);

DEF_CGI_HANDLER(getLogData);
DEF_CGI_HANDLER(delLogData);

DEF_CGI_HANDLER(saveCfgWithJson);

/**
 * webnet thread entry
 */
static void webnet_thread(void *parameter)
{
    int listenfd;
    fd_set readset, tempfds;
    fd_set writeset;
    int i, maxfdp1;
    struct sockaddr_in webnet_saddr;

    rt_thddog_feed("webnet_init");
    /* First acquire our socket for listening for connections */
    listenfd = lwip_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    LWIP_ASSERT("webnet_thread(): Socket create failed.", listenfd >= 0);

    memset(&webnet_saddr, 0, sizeof(webnet_saddr));
    webnet_saddr.sin_family = AF_INET;
    webnet_saddr.sin_addr.s_addr = htonl(INADDR_ANY);
    webnet_saddr.sin_port = htons(WEBNET_PORT);     /* webnet server port */

    if (lwip_bind(listenfd, (struct sockaddr *)&webnet_saddr, sizeof(webnet_saddr)) == -1) LWIP_ASSERT("webnet_thread(): socket bind failed.", 0);

    /* Put socket into listening mode */
    if (lwip_listen(listenfd, MAX_SERV) == -1) LWIP_ASSERT("webnet_thread(): listen failed.", 0);

    /* initalize module (no session at present) */
    rt_thddog_feed("webnet_module_handle_event");
    webnet_module_handle_event(RT_NULL, WEBNET_EVENT_INIT);

    webnet_auth_set(g_auth_cfg.szUser, g_auth_cfg.szPsk);

    webnet_cgi_set_root("cgi-bin");

    webnet_cgi_register("reggetid", reggetid);
    webnet_cgi_register("doreg", doreg);

    webnet_cgi_register("diskFormat", diskFormat);

    webnet_cgi_register("devReset", devReset);

    webnet_cgi_register("factoryReset", factoryReset);

    webnet_cgi_register("getDevInfo", getDevInfo);
    webnet_cgi_register("setDevInfo", setDevInfo);
    webnet_cgi_register("setDefaultMac", setDefaultMac);
    webnet_cgi_register("setTime", setTime);

    webnet_cgi_register("getNetInfo", getNetInfo);
    webnet_cgi_register("getAllNetInfo", getAllNetInfo);

    webnet_cgi_register("getNetCfg", getNetCfg);
    webnet_cgi_register("setNetCfg", setNetCfg);

    webnet_cgi_register("getTcpipCfg", getTcpipCfg);
    webnet_cgi_register("setTcpipCfg", setTcpipCfg);


    webnet_cgi_register("getProtoManage", getProtoManage);

    webnet_cgi_register("setUartCfg", setUartCfg);
    webnet_cgi_register("getUartCfg", getUartCfg);

    webnet_cgi_register("getVarManageExtData", getVarManageExtData);
    webnet_cgi_register("setVarManageExtData", setVarManageExtData);
    webnet_cgi_register("delVarManageExtData", delVarManageExtData);
    webnet_cgi_register("getVarManageExtDataVals", getVarManageExtDataVals);

    webnet_cgi_register("getDiCfg", getDiCfg);
    webnet_cgi_register("setDiCfg", setDiCfg);

    webnet_cgi_register("getDoCfg", getDoCfg);
    webnet_cgi_register("setDoCfg", setDoCfg);

    webnet_cgi_register("getDiValue", getDiValue);
    webnet_cgi_register("setDoValue", setDoValue);
    webnet_cgi_register("getDoValue", getDoValue);

    webnet_cgi_register("setAnalogCfg", setAnalogCfg);
    webnet_cgi_register("getAnalogCfg", getAnalogCfg);

    webnet_cgi_register("setGPRSNetCfg", setGPRSNetCfg);
    webnet_cgi_register("getGPRSNetCfg", getGPRSNetCfg);

    webnet_cgi_register("setGPRSWorkCfg", setGPRSWorkCfg);
    webnet_cgi_register("getGPRSWorkCfg", getGPRSWorkCfg);

    webnet_cgi_register("getCpuUsage", getCpuUsage);

    webnet_cgi_register("getGPRSState", getGPRSState);

    webnet_cgi_register("getTcpipState", getTcpipState);

    webnet_cgi_register("getAuthCfg", getAuthCfg);
    webnet_cgi_register("setAuthCfg", setAuthCfg);

    webnet_cgi_register("getStorageCfg", getStorageCfg);
    webnet_cgi_register("setStorageCfg", setStorageCfg);

    webnet_cgi_register("getHostCfg", getHostCfg);
    webnet_cgi_register("setHostCfg", setHostCfg);

    webnet_cgi_register("getZigbeeCfg", getZigbeeCfg);
    webnet_cgi_register("setZigbeeCfg", setZigbeeCfg);
    webnet_cgi_register("doZigbeeLearnNow", doZigbeeLearnNow);

    webnet_cgi_register("getZigbeeList", getZigbeeList);

    webnet_cgi_register("getXferUartCfg", getXferUartCfg);
    webnet_cgi_register("setXferUartCfg", setXferUartCfg);

    webnet_cgi_register("setWebsocketCfg", setWebsocketCfg);
    
    webnet_cgi_register("listDir", listDir);
    
    webnet_cgi_register("getLogData", getLogData);
    webnet_cgi_register("delLogData", delLogData);
    
    webnet_cgi_register("saveCfgWithJson", saveCfgWithJson);

    /* Wait forever for network input: This could be connections or data */
    for (;;) {
        /* Determine what sockets need to be in readset */
        FD_ZERO(&readset);
        FD_ZERO(&writeset);
        FD_SET(listenfd, &readset);

        /* set fds in each sessions */
        maxfdp1 = webnet_sessions_set_fds(&readset, &writeset);
        if (maxfdp1 < listenfd + 1) maxfdp1 = listenfd + 1;

        /* use temporary fd set in select */
        tempfds = readset;
        /* Wait for data or a new connection */
        rt_thddog_suspend("main lwip_select");
        i = lwip_select(maxfdp1, &tempfds, 0, 0, 0);
        rt_thddog_resume();
        if (i == 0) continue;

        /* At least one descriptor is ready */
        if (FD_ISSET(listenfd, &tempfds)) {
            struct webnet_session *accept_session;
            /* We have a new connection request */
            accept_session = webnet_session_create(listenfd);

            if (accept_session == RT_NULL) {
                /* create session failed, just accept and then close */
                int sock;
                struct sockaddr cliaddr;
                socklen_t clilen;

                clilen = sizeof(struct sockaddr_in);
                sock = lwip_accept(listenfd, &cliaddr, &clilen);
                if (sock >= 0) lwip_close(sock);
            } else {

                /* add read fdset */
                FD_SET(accept_session->socket, &readset);
            }
        }

        rt_thddog_feed("webnet_sessions_handle_fds");
        webnet_sessions_handle_fds(&tempfds, &writeset);
    }

    rt_thddog_exit();
}

void webnet_init(void)
{
    rt_thread_t tid;

    tid = rt_thread_create(WEBNET_THREAD_NAME,
                           webnet_thread, RT_NULL,
                           WEBNET_THREAD_STACKSIZE, WEBNET_PRIORITY, 5);

    if (tid != RT_NULL) {
        rt_thddog_register(tid, 60);
        rt_thread_startup(tid);
    }
}

#ifdef RT_USING_FINSH
#include <finsh.h>
FINSH_FUNCTION_EXPORT(webnet_init, start web server);
#endif

