
#include <yfuns.h>
#include <string.h>
#include "rtthread.h"
#ifdef RT_USING_DFS
#include "dfs_posix.h"
#endif
#include "file_retarget.h"

FILE f_stdin  = { STDIN };
FILE f_stdout = { STDOUT };
FILE f_stderr = { STDERR };

FILE *stdin  = &f_stdin;
FILE *stdout = &f_stdout;
FILE *stderr = &f_stderr;

#ifdef RT_USING_DFS
FILE * fopen( const char *path, const char *mode )
{
    int openmode = O_RDONLY;
    if( 0 == strcmp( mode, "r" ) || 0 == strcmp( mode, "rb" ) || 0 == strcmp( mode, "rt" ) ) {
        openmode = O_RDONLY;
    } else if( 0 == strcmp( mode, "w" ) || 0 == strcmp( mode, "wb" ) || 0 == strcmp( mode, "wt" ) ) {
        openmode = O_WRONLY | O_TRUNC | O_CREAT;
    } else if( 0 == strcmp( mode, "r+" ) || 0 == strcmp( mode, "rb+" ) || 0 == strcmp( mode, "rt+" ) ) {
        openmode = O_RDWR;
    } else if( 0 == strcmp( mode, "w+" ) || 0 == strcmp( mode, "wb+" ) || 0 == strcmp( mode, "wt+" ) ) {
        openmode = O_RDWR | O_TRUNC | O_CREAT;
    } else if( 0 == strcmp( mode, "a" ) || 0 == strcmp( mode, "ab" ) || 0 == strcmp( mode, "at" ) ) {
        openmode = O_WRONLY | O_APPEND | O_CREAT;
    } else if( 0 == strcmp( mode, "a+" ) || 0 == strcmp( mode, "ab+" ) || 0 == strcmp( mode, "at+" ) ) {
        openmode = O_RDWR | O_APPEND | O_CREAT;
    } else {
        return RT_NULL;
    }

    FILE *stream = rt_calloc( sizeof( FILE ), 1 );
    stream->fd = open( path, openmode, 0 );
    if( stream->fd < 0 ) {
        rt_free( stream );
        stream = RT_NULL;
    }

    return stream;
}

int fclose( FILE *stream )
{
    if( stream && stream->fd >= STDERR ) {
        close( stream->fd );
        rt_free( stream );
        return 0;
    }
    return -1;
}

FILE *freopen( const char *filename, const char *mode, FILE *stream )
{
    return stream;
}

FILE *tmpfile(void)
{
    return RT_NULL;
}

size_t fread( void *buffer,  size_t size,  size_t count,  FILE *stream )
{
    if( stream ) {
        if( STDIN == stream->fd ) {
            return 0;
        }
        if( STDOUT == stream->fd || STDERR == stream->fd ) {
            return -1;
        }
        
        int len = read( stream->fd, buffer, size*count );
        stream->err = rt_get_errno();
        if( len >= 0 ) {
            return len;
        } else {
            return -1;
        }
    }
    return -1;
}

size_t fwrite( const void* buffer, size_t size, size_t count, FILE *stream )
{
    if( stream ) {
        if( STDIN == stream->fd ) {
            return -1;
        }
        if( STDOUT == stream->fd || STDERR == stream->fd ) {
            rt_device_t console_device;

            console_device = rt_console_get_device();
            if (console_device != 0) rt_device_write(console_device, 0, buffer, size*count);

            return size*count;
        }
        
        int len = write( stream->fd, buffer, size*count );
        stream->err = rt_get_errno();
        if( len >= 0 ) {
            return len;
        } else {
            return -1;
        }
    }
    return -1;
}

int fseek( FILE *stream, long offset, int fromwhere )
{
    int ret = 0;
    
    if( stream ) {
        if( stream->fd <= STDERR ) {
            return -1;
        }
        ret = lseek( stream->fd, offset, fromwhere );
        stream->err = rt_get_errno();
        return ret;
    }

    return -1;
}

void rewind( FILE *stream )
{
    fseek( stream, 0L, SEEK_SET );
}

long ftell( FILE *stream )
{
    return fseek( stream, 0L, SEEK_CUR );
}

int fgetc( FILE *stream )
{
    char ch = 0;
    if( fread( &ch, 1, 1, stream ) > 0 ) {
        return (int)(ch & 0xFF);
    }
    return -1;
}

int ungetc( int c, FILE *stream )
{
    /*char ch = (char)c;
    if( fwrite( &ch, 1, 1, stream) > 0 ) {
        return (int)(ch&0xFF);
    }*/
    long ofs = ftell(stream);
    if (ofs>0) {
        fseek(stream, ofs-1, DFS_SEEK_SET);
        return 1;
    }

    return -1;
}

int ferror( FILE *stream )
{
    if( stream ) {
        return stream->err;
    }

    return -1;
}

void clearerr( FILE *stream )
{
    if( stream ) {
        stream->err = 0;
    }
}

char *fgets( char *buf, int bufsize, FILE *stream )
{
    int cur = fseek(stream, 0, SEEK_CUR);
    size_t len = fread(buf, 1, bufsize-1, stream);
    int index = 0;

    buf[bufsize-1] = '\0';
    if( len > 0 ) {
        while(buf[index++] != '\n' && index < len);
        if(index < len) {
            buf[index] = '\0';
            fseek(stream, cur + index, SEEK_SET);
        }

        return buf;
    }
    
    return RT_NULL;
}

int fputc( char c, FILE *stream )
{
    return fwrite( &c, 1, 1, stream );
}

int fputs( const char *s, FILE *stream )
{
    return fwrite( s, 1, strlen(s), stream );
}

int puts( const char *s )
{
    int len = strlen(s);
    fwrite( s, 1, len, stdout );
    fwrite( "\n", 1, 1, stdout );
    
    return len+1;
}

int setvbuf( FILE *stream, char *buf, int type, unsigned size )
{
    return 0;
}

int fflush( FILE *stream )
{
    if( stream ) {
        if( stream->fd <= STDERR ) {
            return 0;
        }
        return fsync( stream->fd );
    }
    
    return -1;
}

int fscanf( FILE *stream, const char *fmt, ... )
{
	return 0;
}

int fprintf( FILE *stream, const char *fmt, ... )
{
    int size = 0;
	va_list arp;
	rt_uint8_t f, r;
	rt_ubase_t i, j, w;
	rt_ubase_t v;
	char c, d, s[16], *p;

	va_start(arp, fmt);

	for (;;) {
		c = *fmt++;
		if (c == 0) break;			/* End of string */
		if (c != '%') {				/* Non escape character */
			fputc(c, stream); size++;
			continue;
		}
		w = f = 0;
		c = *fmt++;
		if (c == '0') {				/* Flag: '0' padding */
			f = 1; c = *fmt++;
		} else {
			if (c == '-') {			/* Flag: left justified */
				f = 2; c = *fmt++;
			}
		}
		while( (unsigned)((c) - '0') < 10 ) {		/* Precision */
			w = w * 10 + c - '0';
			c = *fmt++;
		}
		if (c == 'l' || c == 'L') {	/* Prefix: Size is long int */
			f |= 4; c = *fmt++;
		}
		if (!c) break;
		d = c;
		if ((c >= 'a' && c <= 'z')) d -= 0x20;
		switch (d) {				/* Type is... */
		case 'S' :					/* String */
			p = va_arg(arp, char*);
			for (j = 0; p[j]; j++) ;
			if (!(f & 2)) {
				while (j++ < w) { fputc(' ', stream); size++; }
			}
			while (*p) { fputc(*p++, stream); size++; }
			while (j++ < w) { fputc(' ', stream); size++; }
			continue;
		case 'C' :					/* Character */
			fputc((char)va_arg(arp, int), stream); continue;
		case 'B' :					/* Binary */
			r = 2; break;
		case 'O' :					/* Octal */
			r = 8; break;
		case 'D' :					/* Signed decimal */
		case 'U' :					/* Unsigned decimal */
			r = 10; break;
		case 'X' :					/* Hexdecimal */
			r = 16; break;
		default:					/* Unknown type (pass-through) */
			fputc(c, stream); size++; continue;
		}

		/* Get an argument and put it in numeral */
		v = (f & 4) ? (rt_ubase_t)va_arg(arp, long) : ((d == 'D') ? (rt_ubase_t)(long)va_arg(arp, int) : (rt_ubase_t)va_arg(arp, unsigned int));
		if (d == 'D' && (v & 0x80000000)) {
			v = 0 - v;
			f |= 8;
		}
		i = 0;
		do {
			d = (char)(v % r); v /= r;
			if (d > 9) d += (c == 'x') ? 0x27 : 0x07;
			s[i++] = d + '0';
		} while (v && i < sizeof s / sizeof s[0]);
		if (f & 8) s[i++] = '-';
		j = i; d = (f & 1) ? '0' : ' ';
		while (!(f & 2) && j++ < w) { fputc(d, stream); size++; }
		do { fputc(s[--i], stream); size++; } while (i);
		while (j++ < w) { fputc(d, stream); size++; }
	}

	va_end(arp);

	return size;
}

int feof( FILE *stream )
{
    int ret = 0;
    
    if( stream ) {
        if( stream->fd <= STDERR ) {
            return 1;
        }
        struct dfs_fd *fd = fd_get( stream->fd );
        if( fd->pos >= fd->size ) {
            ret = 1;
        }
        fd_put( fd );
    }
    return ret;
}

void abort(void)
{
    exit(-1);
}

void exit( int return_code )
{
    while (1);
}

#else
#error file retarget must define RT_USING_DFS
#endif


