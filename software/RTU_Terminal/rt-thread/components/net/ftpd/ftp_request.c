/*
 * File      : ftp_request.c
 *
 * All rights reserved.
 *
 * Change Logs:
 * Date           Author       Notes
 * 2016-06-13     Jay      the first version
 */

#include <board.h>
#include <ftp_request.h>
//#include <time.h>

/*
int str_begin_with(const char *s, const char *t)
{
	if (strncasecmp(s, t, strlen(t)) == 0) return 1;

	return 0;
}
*/

static int ftp_get_filesize(char * filename)
{
	int pos;
	int end;
	int fd;

	fd = open(filename, O_RDONLY, 0);
	if (fd < 0) return -1;

	pos = lseek(fd, 0, SEEK_CUR);
	end = lseek(fd, 0, SEEK_END);
	lseek (fd, pos, SEEK_SET);
	close(fd);

	return end;
}

static int build_full_path(struct ftp_session* session, char* path, char* new_path, size_t size)
{
	if( path[0] == '/' ) {
		strcpy(new_path, path);
    } else {
		rt_sprintf(new_path, "%s/%s", session->currentdir, path);
	}
	
	return 0;
}

static int pasv_do_list( struct ftp_session * session )
{
	DIR* dirp;
	struct dirent* entry;
	char *path = (char *)session->buffer;
	struct stat s;

	dirp = opendir( session->currentdir );
	if (dirp == NULL)
	{
		ftp_session_pasv_printf( session, "500 Internal Error\r\n");
		return -1;
	}

	while (1)
	{
		entry = readdir(dirp);
		if (entry == NULL) break;

		rt_sprintf( path, "%s/%s", session->currentdir, entry->d_name );
		if (stat( path, &s) == 0)
		{
		    
		    const char mon[12][4] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
		    char tstr[12] = {0};
            struct tm lt;
            localtime_r(&s.st_mtime, &lt);
            sprintf( tstr, "%s %02d %04d", mon[lt.tm_mon], lt.tm_mday, lt.tm_year+1900 );
			if (s.st_mode & S_IFDIR) {
				ftp_session_pasv_printf( session, "drw-r--r-- 1 admin admin %d %s %s\r\n", 0, tstr, entry->d_name);
			} else {
				ftp_session_pasv_printf( session, "-rw-r--r-- 1 admin admin %d %s %s\r\n", s.st_size, tstr, entry->d_name);
            }
		    
			/*if (s.st_mode & S_IFDIR) {
				ftp_session_pasv_printf( session, "drw-r--r-- 1 admin admin %d Jan 1 2016 %s\r\n", 0, entry->d_name);
			} else {
				ftp_session_pasv_printf( session, "-rw-r--r-- 1 admin admin %d Jan 1 2016 %s\r\n", s.st_size, entry->d_name);
            }*/
		}
		else
		{
			rt_kprintf("Get directory entry error\n");
			break;
		}
	}

	closedir(dirp);
	return 0;
}

static int pasv_do_simple_list( struct ftp_session * session )
{
	DIR* dirp;
	struct dirent* entry;
	
	dirp = opendir( session->currentdir );
	if (dirp == NULL)
	{
		ftp_session_pasv_printf( session, "500 Internal Error\r\n");
		return -1;
	}

	while (1)
	{
		entry = readdir(dirp);
		if (entry == NULL) break;

        ftp_session_pasv_printf( session, "%s\r\n", entry->d_name);
	}

	closedir(dirp);
	return 0;
}

extern int str_begin_with(const char *s, const char *t);

int ftp_request_parse(struct ftp_session* session, char *buf)
{
	int  fd;
	char filename[256];
	char *parameter_ptr, *ptr;
	rt_uint32_t addr_len = sizeof(struct sockaddr_in);
	struct sockaddr_in pasvremote;

	/* remove \r\n */
	ptr = buf;
	while (*ptr)
	{
		if (*ptr == '\r' || *ptr == '\n') *ptr = 0;
		ptr ++;
	}

	/* get request parameter */
	parameter_ptr = strchr(buf, ' '); if (parameter_ptr != NULL) parameter_ptr ++;

	// debug:
	rt_kprintf("%s requested: \"%s\"\n", inet_ntoa(session->remote.sin_addr), buf);

	//
	//-----------------------
	if( str_begin_with(buf, "USER") ) {
		rt_kprintf("%s sent login \"%s\"\n", inet_ntoa(session->remote.sin_addr), parameter_ptr);
		// login correct
		// 账号不区分大小写
        if (rt_strcasecmp(parameter_ptr, g_auth_cfg.szUser ) == 0) {
			session->is_anonymous = RT_FALSE;
			ftp_session_printf( session, "331 Password required for %s\r\n", parameter_ptr);
		} else {
			ftp_session_printf( session, "530 Login incorrect. Bye.\r\n");
			return -1;
		}
		return 0;
	} else if( str_begin_with(buf, "PASS") ) {
		rt_kprintf("%s sent password \"%s\"\n", inet_ntoa(session->remote.sin_addr), parameter_ptr);
		if (strcmp(parameter_ptr, g_auth_cfg.szPsk ) == 0 ) {
			// password correct
			ftp_session_printf( session, "230 User logged in.\r\n");
			return 0;
		}
		
		// incorrect password
		ftp_session_printf( session, "530 Login or Password incorrect. Bye!\r\n");
		return -1;
	} else if( str_begin_with(buf, "LIST") ) {
		ftp_session_pasv_open(session);
		ftp_session_printf( session, "150 Opening Binary mode connection for file list.\r\n");
		pasv_do_list(session);
		ftp_session_pasv_close(session);
		session->pasv_active = 0;
		ftp_session_printf( session, "226 Transfert Complete.\r\n");
	} else if( str_begin_with(buf, "NLST") ) {
		ftp_session_printf( session, "150 Opening Binary mode connection for file list.\r\n");
		ftp_session_pasv_open(session);
		pasv_do_simple_list(session);
		ftp_session_pasv_close(session);
		session->pasv_active = 0;
		ftp_session_printf( session, "226 Transfert Complete.\r\n");
	} else if( str_begin_with(buf, "PWD") || str_begin_with(buf, "XPWD") ) {
		ftp_session_printf( session, "257 \"%s\" is current directory.\r\n", session->currentdir);
	} else if( str_begin_with(buf, "TYPE") ) {
		// Ignore it
		if( strcmp(parameter_ptr, "I")==0 ) {
			ftp_session_printf( session, "200 Type set to binary.\r\n");
		} else {
			ftp_session_printf( session, "200 Type set to ascii.\r\n");
		}
	} else if( str_begin_with(buf, "PASV") ) {
		if( ftp_session_pasv_create( session ) < 0 ) {
		    session->pasv_active = 0;
            ftp_session_pasv_close( session );
		}
		return 0;
	} else if( str_begin_with(buf, "RETR") ) {
		int file_size;
	    int numbytes;

		ftp_session_pasv_open(session);

		strcpy(filename, buf + 5);

		build_full_path(session, parameter_ptr, filename, 256);
		file_size = ftp_get_filesize(filename);
		if (file_size == -1) {
			ftp_session_printf( session, "550 \"%s\" : not a regular file\r\n", filename);
			session->file_offset=0;
			ftp_session_pasv_close(session);
			return 0;
		}

		fd = open(filename, O_RDONLY, 0);
		if (fd < 0) {
			ftp_session_pasv_close(session);
			return 0;
		}

		if(session->file_offset > 0 && session->file_offset < file_size) {
			lseek(fd, session->file_offset, SEEK_SET);
			ftp_session_printf( 
			        session, 
			        "150 Opening binary mode data connection for partial \"%s\" (%d/%d bytes).\r\n", 
			        filename, file_size - session->file_offset, file_size
			);
		} else {
			ftp_session_printf( session, "150 Opening binary mode data connection for \"%s\" (%d bytes).\r\n", filename, file_size);
		}
		while( (numbytes = read(fd, session->buffer, FTP_BUFFER_SIZE)) > 0 ) {
			ftp_session_pasv_write( session, session->buffer, numbytes );
		}
		close(fd);
		
		ftp_session_printf( session, "226 Finished.\r\n" );
		ftp_session_pasv_close(session);
	} else if( str_begin_with(buf, "SIZE") ) {
		int file_size;

		build_full_path(session, parameter_ptr, filename, 256);
		
		file_size = ftp_get_filesize(filename);
		if( file_size == -1) {
			ftp_session_printf( session, "550 \"%s\" : not a regular file\r\n", filename);
		} else {
			ftp_session_printf( session, "213 %d\r\n", file_size);
		}
	} else if( str_begin_with(buf, "MDTM") ) {
		ftp_session_printf( session, "550 \"/\" : not a regular file\r\n");
		
	} else if( str_begin_with(buf, "SYST") ) {
		ftp_session_printf( session, "215 %s\r\n", "RT-Thread RTOS");
		
	} else if( str_begin_with(buf, "CWD") ) {
		build_full_path(session, parameter_ptr, filename, 256);
		ftp_session_printf( session, "250 Changed to directory \"%s\"\r\n", filename);
		strcpy(session->currentdir, filename);
		rt_kprintf("Changed to directory %s", filename);
		
	} else if( str_begin_with(buf, "CDUP") ) {
		rt_sprintf(filename, "%s/%s", session->currentdir, "..");

		ftp_session_printf( session, "250 Changed to directory \"%s\"\r\n", filename);
		strcpy(session->currentdir, filename);
		rt_kprintf("Changed to directory %s", filename);
		
	} else if( str_begin_with(buf, "PORT") ) {
		int i;
		int portcom[6];
		char tmpip[100];

		i=0;
		portcom[i++]=atoi(strtok(parameter_ptr, ".,;()"));
		for(;i<6;i++) {
			portcom[i]=atoi(strtok(0, ".,;()"));
        }
		rt_sprintf(tmpip, "%d.%d.%d.%d", portcom[0], portcom[1], portcom[2], portcom[3]);

		if( (session->pasv_sockfd=socket(AF_INET, SOCK_STREAM, 0))==-1 ) {
			ftp_session_printf( session, "425 Can't open data connection.\r\n");
			ftp_session_pasv_close(session);
			return 0;
		}
		pasvremote.sin_addr.s_addr=inet_addr(tmpip);
		pasvremote.sin_port=htons(portcom[4] * 256 + portcom[5]);
		pasvremote.sin_family=PF_INET;
		if(connect(session->pasv_sockfd, (struct sockaddr *)&pasvremote, addr_len)==-1) {
			// is it only local address?try using gloal ip addr
			pasvremote.sin_addr=session->remote.sin_addr;
			if(connect(session->pasv_sockfd, (struct sockaddr *)&pasvremote, addr_len)==-1) {
				ftp_session_printf( session, "425 Can't open data connection.\r\n");
				ftp_session_pasv_close(session);
				return 0;
			}
		}
		session->pasv_active=1;
		session->pasv_port = portcom[4] * 256 + portcom[5];
		rt_kprintf("Connected to Data(PORT) %s @ %d\n", tmpip, portcom[4] * 256 + portcom[5]);
		ftp_session_printf( session, "200 Port Command Successful.\r\n");

	} else if( str_begin_with(buf, "REST") ) {
		if( atoi(parameter_ptr)>=0 ) {
			session->file_offset=atoi(parameter_ptr);
			ftp_session_printf( session, "350 Send RETR or STOR to start transfert.\r\n");
		}
	} else if( str_begin_with(buf, "NOOP") || str_begin_with(buf, "noop") ) {
		ftp_session_printf( session, "200 noop!\r\n");
		
	} else if( str_begin_with(buf, "QUIT") ) {
		ftp_session_printf( session, "221 Bye!\r\n");
		return -1;
	} else {
		ftp_session_printf( session, "502 Not Implemented.\r\n");
		
	}
	/* else if (str_begin_with(buf, "STOR") ) {
	} else if( str_begin_with(buf, "MKD") ) {
	} else if( str_begin_with(buf, "DELE") ) {
	} else if( str_begin_with(buf, "RMD") ) {
	} else if( str_begin_with(buf, "RNFR") ) {
	} else if( str_begin_with(buf, "RNTO") ) {
	}*/
	
	return 0;
}

