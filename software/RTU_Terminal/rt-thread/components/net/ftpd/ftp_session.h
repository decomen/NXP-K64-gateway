/*
 * File      : ftp_session.h
 *
 * Change Logs:
 * Date           Author       Notes
 * 2016-06-13     Jay      the first version
 */
#ifndef __FTP_SESSION_H__
#define __FTP_SESSION_H__

#include <rtthread.h>
#include <lwip/sockets.h>
#include <ftpd.h>
#include <ftp_request.h>

struct ftp_session
{
	rt_bool_t is_anonymous;

	int sockfd;
	struct sockaddr_in remote;
	struct sockaddr_in server;

	char serveraddr[INET_ADDRSTRLEN];

	/* pasv data */
	int  pasv_listen_sockfd;
	int  pasv_sockfd;
	char pasv_active;

	unsigned short pasv_port;
	size_t file_offset;

	/* current directory */
	char currentdir[256];
	rt_uint8_t buffer[FTP_BUFFER_SIZE];

	struct ftp_session* next;
};

struct ftp_session* ftp_session_create(int listenfd);
int ftp_session_read(struct ftp_session *session, char *buffer, int length);
void ftp_session_close(struct ftp_session *session);
void ftp_session_printf(struct ftp_session *session, const char* fmt, ...);
int ftp_session_write(struct ftp_session *session, const rt_uint8_t* data, rt_size_t size);

int ftp_session_pasv_create( struct ftp_session* session );
int ftp_session_pasv_open(struct ftp_session* session);
void ftp_session_pasv_close(struct ftp_session* session);
void ftp_session_pasv_printf(struct ftp_session *session, const char* fmt, ...);
int ftp_session_pasv_write(struct ftp_session *session, const rt_uint8_t* data, rt_size_t size);

int ftp_sessions_set_fds(fd_set *readset);
void ftp_sessions_handle_fds(fd_set *readset);

#endif

