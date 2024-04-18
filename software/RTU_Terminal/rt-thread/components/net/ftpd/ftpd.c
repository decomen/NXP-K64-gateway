#include <stdlib.h>

#include <rtthread.h>
#include <dfs_posix.h>
#include <lwip/sockets.h>
#include <time.h>
#include <ftpd.h>
#include <ftp_session.h>
#include <board.h>

void ftpd_thread(void* parameter)
{
    int listenfd;
    fd_set readset, tempfds;
    int i, maxfdp1;
	struct sockaddr_in local;

    /* First acquire our socket for listening for connections */
    listenfd = lwip_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    LWIP_ASSERT("ftpd_thread(): Socket create failed.", listenfd >= 0);

    memset(&local, 0, sizeof(local));
	local.sin_port=htons(FTP_PORT);
	local.sin_family=AF_INET;
	local.sin_addr.s_addr=INADDR_ANY;

    if (lwip_bind(listenfd, (struct sockaddr *) &local, sizeof(local) ) == -1)
        LWIP_ASSERT("ftpd_thread(): socket bind failed.", 0);

    /* Put socket into listening mode */
    if (lwip_listen(listenfd, FTP_MAX_CONNECTION) == -1)
        LWIP_ASSERT("ftpd_thread(): listen failed.", 0);

    /* Wait forever for network input: This could be connections or data */
    for (;;)
    {
        /* Determine what sockets need to be in readset */
        FD_ZERO(&readset);
        FD_SET(listenfd, &readset);
        
        rt_thddog_feed("");

		/* set fds in each sessions */
		maxfdp1 = ftp_sessions_set_fds(&readset);
		if (maxfdp1 < listenfd + 1) maxfdp1 = listenfd + 1;

		/* use temporary fd set in select */
		tempfds = readset;
        /* Wait for data or a new connection */
        rt_thddog_suspend("lwip_select");
        i = lwip_select(maxfdp1, &tempfds, 0, 0, 0);
        rt_thddog_resume();
        if (i == 0) continue;

        /* At least one descriptor is ready */
        if (FD_ISSET(listenfd, &tempfds))
        {
        	struct ftp_session* accept_session;
            /* We have a new connection request */
			accept_session = ftp_session_create(listenfd);

			if (accept_session == RT_NULL)
			{
                /* create session failed, just accept and then close */
                int sock;
                struct sockaddr cliaddr;
                socklen_t clilen;

				clilen = sizeof(struct sockaddr_in);
                rt_thddog_suspend("lwip_accept");
                sock = lwip_accept(listenfd, &cliaddr, &clilen);
		        rt_thddog_resume();
                if (sock >= 0) lwip_close(sock);
            } else {
				ftp_session_printf( accept_session, FTP_WELCOME_MSG );
				/* add read fdset */
				FD_SET(accept_session->sockfd, &readset);
			}
        }

        rt_thddog_suspend("ftp_sessions_handle_fds");
		ftp_sessions_handle_fds(&tempfds);
		rt_thddog_resume();
    }
    
    rt_thddog_exit();
}

void ftpd_start()
{
	rt_thread_t tid;

	tid = rt_thread_create("ftpd",
		ftpd_thread, RT_NULL,
		0x600, 30, 5);
	if (tid != RT_NULL) {
        rt_thddog_register(tid, 30);
	    rt_thread_startup(tid);
	}
}

#ifdef RT_USING_FINSH
#include <finsh.h>
FINSH_FUNCTION_EXPORT(ftpd_start, start ftp server);

#ifdef FINSH_USING_MSH
int cmd_ftpd_start(int argc, char** argv)
{
	ftpd_start();
	return 0;
}
FINSH_FUNCTION_EXPORT_ALIAS(cmd_ftpd_start, __cmd_ftpd_start, start ftp server.);
#endif

#endif
