/*
 * main.c -- Main program for the GoAhead WebServer (RT-Thread version)
 *
 * Copyright (c) Go Ahead Software Inc., 1995-2010. All Rights Reserved.
 *
 * See the file "license.txt" for usage and redistribution license requirements
 *
 */

/******************************** Description *********************************/

/*
 *  Main program for for the GoAhead WebServer. This is a demonstration
 *  main program to initialize and configure the web server.
 */

/********************************* Includes ***********************************/

#include "uemf.h"
#include "wsIntrn.h"
#include <lwip/sockets.h>
#include "board.h"

/*********************************** Locals ***********************************/
/*
 *  Change configuration here
 */

static char_t   *password = T("");  /* Security password */
static const int port = 80;			/* Server port */
static const int retries = 5;		/* Server port retries */
static int finished;				/* Finished flag */

/****************************** Forward Declarations **************************/

static int  initWebs();
static int  aspTest(int eid, webs_t wp, int argc, char_t **argv);
static int  websHomePageHandler(webs_t wp, char_t *urlPrefix, char_t *webDir,
                int arg, char_t* url, char_t* path, char_t* query);

#ifdef B_STATS
#error WARNING:  B_STATS directive is not supported in this OS!
#endif

/*********************************** Code *************************************/
/*
 *	Main -- entry point from RTT
 */

void webserver_thread_entry(void *parameter)
{
/*
 *  Initialize the memory allocator. Allow use of malloc and start 
 *  with a 60K heap.  For each page request approx 8KB is allocated.
 *  60KB allows for several concurrent page requests.  If more space
 *  is required, malloc will be used for the overflow.
 */
    bopen(NULL, (30 * 1024), B_USE_MALLOC);

/*
 *  Initialize the web server
 */
    if (initWebs() < 0) {
        return;
    }

/*
 *  Basic event loop. SocketReady returns true when a socket is ready for
 *  service. SocketSelect will block until an event occurs. SocketProcess
 *  will actually do the servicing.
 */
    while (!finished) {
        if (socketReady(-1) || socketSelect(-1, 2000)) {
            socketProcess(-1);
        }
        emfSchedProcess();
    }

/*
 *  Close the socket module, report memory leaks and close the memory allocator
 */
    websCloseServer();
    socketClose();
    bclose();
}

DEF_CGI_HANDLER(devReset);

DEF_CGI_HANDLER(getDevInfo);
DEF_CGI_HANDLER(setDevInfo);

DEF_CGI_HANDLER(setDefaultMac);
DEF_CGI_HANDLER(setTime);

DEF_CGI_HANDLER(getNetInfo);
DEF_CGI_HANDLER(getNetCfg);
DEF_CGI_HANDLER(setNetCfg);

DEF_CGI_HANDLER(getTcpipCfg);
DEF_CGI_HANDLER(setTcpipCfg);

DEF_CGI_HANDLER(getProtoManage);

DEF_CGI_HANDLER(setUartCfg);
DEF_CGI_HANDLER(getUartCfg);

DEF_CGI_HANDLER(getVarManageExtData);
DEF_CGI_HANDLER(setVarManageExtData);

DEF_CGI_HANDLER(getDiCfg);
DEF_CGI_HANDLER(setDiCfg);
DEF_CGI_HANDLER(getDoCfg);
DEF_CGI_HANDLER(setDoCfg);

DEF_CGI_HANDLER(getDiValue);
DEF_CGI_HANDLER(setDoValue);
DEF_CGI_HANDLER(getDoValue);

DEF_CGI_HANDLER(setAnalogCfg);
DEF_CGI_HANDLER(getAnalogCfg);

DEF_CGI_HANDLER(setGPRSCfg);
DEF_CGI_HANDLER(getGPRSCfg);

DEF_CGI_HANDLER(getCpuUsage);
DEF_CGI_HANDLER(getGPRSState);
DEF_CGI_HANDLER(getTcpipState);

void upldForm(webs_t wp, char_t * path, char_t * query);

/******************************************************************************/
/*
 *  Initialize the web server.
 */
static int initWebs()
{
    //char        host[128]= "DJYOS_STACK";
    //char        *cp;
    //char_t      wbuf[128];

    //struct in_addr  addr;
/*
 *  Initialize the socket subsystem
 */
    socketOpen();

/*
 *  Configure the web server options before opening the web server
 */
    websSetDefaultDir("/www");
    //addr.s_addr = INADDR_ANY;
    //cp = inet_ntoa(addr);
    //ascToUni(wbuf, cp, min(strlen(cp) + 1, sizeof(wbuf)));
    //websSetIpaddr(wbuf);
    //ascToUni(wbuf, host, min(strlen(host) + 1, sizeof(wbuf)));
    //websSetHost(wbuf);

/*
 *  Configure the web server options before opening the web server
 */
    websSetDefaultPage(T("index.html"));
    websSetPassword(password);

/* 
 *  Open the web server on the given port. If that port is taken, try
 *  the next sequential port for up to "retries" attempts.
 */
    websOpenServer(port, retries);

/*
 *  First create the URL handlers. Note: handlers are called in sorted order
 *  with the longest path handler examined first. Here we define the security 
 *  handler, forms handler and the default web page handler.
 */
    websUrlHandlerDefine(T(""), NULL, 0, websSecurityHandler, 
        WEBS_HANDLER_FIRST);
    websUrlHandlerDefine(T("/cgi-bin"), NULL, 0, websFormHandler, 0);
    websUrlHandlerDefine(T(""), NULL, 0, websDefaultHandler, 
        WEBS_HANDLER_LAST); 
    websFormDefine( T( "devReset"), devReset );
   
    websFormDefine( T( "getDevInfo"), getDevInfo );
    websFormDefine( T( "setDevInfo"), setDevInfo );
    websFormDefine( T( "setDefaultMac"), setDefaultMac );
    websFormDefine( T( "setTime"), setTime );

    websFormDefine( T( "getNetInfo"), getNetInfo );
    
    websFormDefine( T( "getNetCfg"), getNetCfg );
    websFormDefine( T( "setNetCfg"), setNetCfg );
    
    websFormDefine( T( "getTcpipCfg"), getTcpipCfg );
    websFormDefine( T( "setTcpipCfg"), setTcpipCfg );
    
    
    websFormDefine( T( "getProtoManage"), getProtoManage );
    
    websFormDefine( T( "setUartCfg"), setUartCfg );
    websFormDefine( T( "getUartCfg"), getUartCfg );
    
    websFormDefine( T( "getVarManageExtData"), getVarManageExtData );
    websFormDefine( T( "setVarManageExtData"), setVarManageExtData );

    websFormDefine( T( "getDiCfg"), getDiCfg );
    websFormDefine( T( "setDiCfg"), setDiCfg );
    
    websFormDefine( T( "getDoCfg"), getDoCfg );
    websFormDefine( T( "setDoCfg"), setDoCfg );
    
    websFormDefine( T( "getDiValue"), getDiValue );
    websFormDefine( T( "setDoValue"), setDoValue );
    websFormDefine( T( "getDoValue"), getDoValue );
    
    websFormDefine( T( "setAnalogCfg"), setAnalogCfg );
    websFormDefine( T( "getAnalogCfg"), getAnalogCfg );
    
    websFormDefine( T( "setGPRSCfg"), setGPRSCfg );
    websFormDefine( T( "getGPRSCfg"), getGPRSCfg );
    
    websFormDefine( T( "getCpuUsage"), getCpuUsage );
    
    websFormDefine( T( "getGPRSState"), getGPRSState );
    
    websFormDefine( T( "getTcpipState"), getTcpipState );

    websFormDefine(T("upldForm"), upldForm);

    websUrlHandlerDefine(T("/"), NULL, 0, websHomePageHandler, 0); 
    return 0;
}

/******************************************************************************/
/*
 *  Test Javascript binding for ASP. This will be invoked when "aspTest" is
 *  embedded in an ASP page. See web/asp.asp for usage. Set browser to 
 *  "localhost/asp.asp" to test.
 */

static int aspTest(int eid, webs_t wp, int argc, char_t **argv)
{
    char_t  *name, *address;

    if (ejArgs(argc, argv, T("%s %s"), &name, &address) < 2) {
        websError(wp, 400, T("Insufficient args\n"));
        return -1;
    }

    return websWrite(wp, T("Name: %s, Address %s"), name, address);
}

/******************************************************************************/
/*
 *  Home page handler
 */

static int websHomePageHandler(webs_t wp, char_t *urlPrefix, char_t *webDir,
    int arg, char_t* url, char_t* path, char_t* query)
{
/*
 *  If the empty or "/" URL is invoked, redirect default URLs to the home page
 */
    if (*url == '\0' || gstrcmp(url, T("/")) == 0) {
        websRedirect(wp, T("index.html"));
        return 1;
    }
    return 0;
}

/*******************************************************************************
*
* upldForm - GoForm procedure to handle file uploads for upgrading device
*
* This routine handles http file uploads for upgrading device.
*
* RETURNS: OK, or ERROR if the I/O system cannot install the driver.
*/

void upldForm(webs_t wp, char_t * path, char_t * query)
{
    char_t *     fn;
    char_t *     bn = NULL;
    int          locWrite;
    int          numLeft;
    int          numWrite;

    //a_assert(websValid(wp));
    //websHeader(wp);

    fn = websGetVar(wp, T("filename"), T(""));
    if (fn != NULL && *fn != '\0') {
        if ((int)(bn = gstrrchr(fn, '/') + 1) == 1) {
            if ((int)(bn = gstrrchr(fn, '\\') + 1) == 1) {
                bn = fn;
            }
        }
    }
    rt_kprintf("Filename = %s, Size = %d bytes \n", bn, wp->lenPostData);

    //websFooter(wp);
    websDone(wp, 200);
}


void webserver_start()
{
	rt_thread_t tid;

	tid = rt_thread_create("goahead",
		webserver_thread_entry, RT_NULL,
		2048, 10 ,10 );
	if (tid != RT_NULL) rt_thread_startup(tid);
}

#ifdef RT_USING_FINSH
#include <finsh.h>
FINSH_FUNCTION_EXPORT(webserver_start, start web server)
#endif


