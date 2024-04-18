
#ifndef __FILE_RETARGET_H__
#define __FILE_RETARGET_H__

typedef struct _FILE {
    int fd;
    int err;
} FILE;

#define STDIN       0
#define STDOUT      1
#define STDERR      2

extern FILE *stdin;
extern FILE *stdout;
extern FILE *stderr;

#define getc    fgetc

FILE * fopen( const char *path, const char *mode );
int fclose( FILE *stream );
FILE *freopen( const char *filename, const char *mode, FILE *stream );
FILE *tmpfile(void);
size_t fread( void *buffer,  size_t size,  size_t count,  FILE *stream );
size_t fwrite( const void* buffer, size_t size, size_t count, FILE *stream );
int fseek( FILE *stream, long offset, int fromwhere );
void rewind( FILE *stream );
long ftell( FILE *stream );
int fgetc( FILE *stream );
int ungetc( int c, FILE *stream );
int ferror( FILE *stream );
void clearerr( FILE *stream );
char *fgets( char *buf, int bufsize, FILE *stream );
int fputc( char c, FILE *stream );
int fputs( const char *s, FILE *stream );
int puts( const char *s );
int fprintf( FILE *stream, const char *fmt, ... );
int feof( FILE *stream );
void abort(void);
void exit( int return_code );

int fscanf( FILE *stream, const char *fmt, ... );
int setvbuf( FILE *stream, char *buf, int type, unsigned size );
int fflush( FILE *stream );

#endif

