/*
 * File      : ftpd.h
 *
 * Change Logs:
 * Date           Author       Notes
 * 2016-06-13     Jay      the first version
 */


#ifndef __FTPD_H__
#define __FTPD_H__

#define FTP_PORT			21
#define FTP_SRV_ROOT		"/"
#define FTP_MAX_CONNECTION	2
#define FTP_USER			"jay"
#define FTP_PASSWORD		"123456"
#define FTP_WELCOME_MSG		"220 welcome on RT-Thread FTP server.\r\n"
#define FTP_SESSION_SIZE    1024
#define FTP_BUFFER_SIZE		4096

#define INET_ADDRSTRLEN     16

#endif

