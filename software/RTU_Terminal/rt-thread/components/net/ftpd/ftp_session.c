/*
 * File      : ftp_session.c
 *
 * All rights reserved.
 *
 * Change Logs:
 * Date           Author       Notes
 * 2016-06-13     Jay      the first version
 */

#include "ftp_session.h"
#include <dfs_posix.h>

static struct ftp_session* _ftp_session_list = NULL;

struct ftp_session* ftp_session_create(int listenfd)
{
	struct ftp_session* session;

	/* create a new session */
	session = (struct ftp_session *)rt_calloc(1, sizeof(struct ftp_session));
	if (session != RT_NULL)
	{
	    socklen_t addr_len = sizeof(struct sockaddr_in);
	    
		session->sockfd = lwip_accept(listenfd, (struct sockaddr *)&session->remote, &addr_len);
		if (session->sockfd < 0)
		{
			rt_free(session);
		}
		else
		{
		    if (-1 == getsockname(session->sockfd, (struct sockaddr *)&session->server, &addr_len)) {
				rt_kprintf("Cannot determine our address, need it if client should connect to us\n");
			}
			ipaddr_ntoa_r((const ip_addr_t *)&(session->server.sin_addr), session->serveraddr, sizeof(session->serveraddr));
			strcpy(session->currentdir, FTP_SRV_ROOT);

            session->pasv_listen_sockfd = -1;
            session->pasv_sockfd = -1;
			
			/* keep this tecb in our list */
			session->next = _ftp_session_list;
			_ftp_session_list = session;
		}
	}

	return session;
}

int ftp_session_read(struct ftp_session *session, char *buffer, int length)
{
    int read_count;

    /* Read some data */
    read_count = lwip_read(session->sockfd, buffer, length);
    if (read_count <= 0)
    {
        return -1;
    }

	return read_count;
}

void ftp_session_close(struct ftp_session *session)
{
    struct ftp_session *iter;

    /* Either an error or tcp connection closed on other
     * end. Close here */
    ftp_session_pasv_close( session );
    lwip_close(session->sockfd);

    /* Free ftp_session */
    if (_ftp_session_list == session)
        _ftp_session_list = session->next;
    else
	{
        for (iter = _ftp_session_list; iter; iter = iter->next)
        {
            if (iter->next == session)
            {
                iter->next = session->next;
                break;
            }
        }
	}

    rt_free(session);
}

void ftp_session_printf(struct ftp_session *session, const char* fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	rt_memset(session->buffer, 0, sizeof(session->buffer));
	rt_vsprintf((char*)(session->buffer), fmt, args);
	va_end(args);

	lwip_send(session->sockfd, session->buffer, rt_strlen((char*)(session->buffer)), 0);
}

int ftp_session_write(struct ftp_session *session, const rt_uint8_t* data, rt_size_t size)
{
	/* send data directly */
	lwip_send(session->sockfd, data, size, 0);

	return size;
}

int ftp_session_pasv_create( struct ftp_session* session )
{
    int optval=1;
    int port;
	struct sockaddr_in local;
    struct sockaddr_in data;
    socklen_t len = sizeof(struct sockaddr);

    ftp_session_pasv_close( session );

    session->pasv_port = 10000;
    session->pasv_active = 1;
    memset(&local, 0, sizeof(local));
    local.sin_port=htons(session->pasv_port);
	local.sin_family=PF_INET;
    local.sin_addr.s_addr=INADDR_ANY;

    if( (session->pasv_listen_sockfd = lwip_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0 ) {
    	ftp_session_printf( session, "425 Can't open data connection .\r\n");
    	return -1;
    }
    if( setsockopt(session->pasv_listen_sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0 ) {
    	ftp_session_printf( session, "425 Can't open data connection1.\r\n");
    	return -1;
    }
    if( bind(session->pasv_listen_sockfd, (struct sockaddr *)&local, sizeof( struct sockaddr_in )) < 0 ) {
    	ftp_session_printf( session, "425 Can't open data connection2.\r\n");
    	return -1;
    }
    if( listen(session->pasv_listen_sockfd, 1) < 0 ) {
    	ftp_session_printf( session, "425 Can't open data connection3.\r\n");
    	return -1;
    }
    if( getsockname(session->pasv_listen_sockfd, (struct sockaddr *)&data, &len) < 0 ) {
    	rt_kprintf("Cannot determine our address, need it if client should connect to us\n");
    	return -1;
    }

    port = ntohs(data.sin_port);

    /* Convert server IP address and port to comma separated list */
    {
        char ipmsg[INET_ADDRSTRLEN], *p;
        
        strcpy( ipmsg, session->serveraddr );
        p = ipmsg;
        // replace . to ,
        while ((p = strchr(p, '.'))) *p++ = ',';
        ftp_session_printf( session, "227 Entering passive mode (%s,%d,%d)\r\n", ipmsg, port / 256, port % 256);
    }

	return 0;
}

int ftp_session_pasv_open(struct ftp_session* session)
{
	socklen_t len = sizeof(struct sockaddr);
	struct sockaddr_in sin;
	/* Previous PASV command, accept connect from client */
	if( session->pasv_listen_sockfd > 0 ) {

		session->pasv_sockfd = lwip_accept(session->pasv_listen_sockfd, (struct sockaddr *)&sin, &len );
		if( -1 == session->pasv_sockfd ) {
			return -1;
		}

		len = sizeof(struct sockaddr);
		if( -1 == getpeername(session->pasv_sockfd, (struct sockaddr *)&sin, &len) ) {
			lwip_close(session->pasv_sockfd);
			session->pasv_sockfd = -1;
			return -1;
		}
		//rt_kprintf("Client PASV data connection from %s\n", inet_ntoa(sin.sin_addr));
	}

	return 0;
}

void ftp_session_pasv_close(struct ftp_session* session)
{
	/* PASV client socket */
    if (session->pasv_sockfd > 0) {
    	lwip_close(session->pasv_sockfd);
    	session->pasv_sockfd = -1;
    }

	/* PASV server listening socket */
    if (session->pasv_listen_sockfd > 0) {
    	lwip_close(session->pasv_listen_sockfd);
    	session->pasv_listen_sockfd = -1;
    }

    session->pasv_active = 0;
}

void ftp_session_pasv_printf(struct ftp_session *session, const char* fmt, ...)
{
	if( session->pasv_sockfd >= 0 ) {
    	va_list args;

    	va_start(args, fmt);
    	rt_memset(session->buffer, 0, sizeof(session->buffer));
    	rt_vsprintf((char*)(session->buffer), fmt, args);
    	va_end(args);

    	lwip_send(session->pasv_sockfd, session->buffer, rt_strlen((char*)(session->buffer)), 0);
	}
}

int ftp_session_pasv_write(struct ftp_session *session, const rt_uint8_t* data, rt_size_t size)
{
	/* send data directly */
	if( session->pasv_sockfd >= 0 ) {
	    lwip_send(session->pasv_sockfd, data, size, 0);
	}

	return size;
}

int ftp_sessions_set_fds( fd_set *readset )
{
	int maxfdp1 = 0;
	struct ftp_session *session;

	for (session = _ftp_session_list; session; session = session->next)
	{
		if (maxfdp1 < session->sockfd + 1)
			maxfdp1 = session->sockfd + 1;

		FD_SET(session->sockfd, readset);
	}

	return maxfdp1;
}

void ftp_sessions_handle_fds(fd_set *readset )
{
	int read_length;
	char *read_buffer;
    struct ftp_session *session;

	read_buffer = (char*) rt_malloc ( FTP_SESSION_SIZE );
	if (read_buffer == RT_NULL) return; /* out of memory */

	/* Go through list of connected session and process data */
	for (session = _ftp_session_list; session; )
	{
	    struct ftp_session *session_next = session->next;
	    
		if (FD_ISSET(session->sockfd, readset))
		{
			/* This socket is ready for reading. */
			read_length = ftp_session_read( session, read_buffer, FTP_SESSION_SIZE - 1 );
			if (read_length <= 0) {
                ftp_session_close(session);
                if( !session_next ) {
                    break;
				}
			} else {
				/* made a \0 terminator */
                read_buffer[read_length] = 0;
				if( ftp_request_parse( session, read_buffer ) < 0 ) {
                    /* close this session */
                    ftp_session_close(session);
                    if( !session_next ) {
                        break;
    				}
				}
			}
		}

		session = session_next;
	}

	/* release read buffer */
	rt_free(read_buffer);
}

