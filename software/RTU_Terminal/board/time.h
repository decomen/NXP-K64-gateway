#ifndef _SYS_TIME_H_
#define _SYS_TIME_H_

typedef unsigned long time_t;

#ifdef __cplusplus
extern "C" {
#endif


#ifndef _TIMESPEC_DEFINED
#define _TIMESPEC_DEFINED
/*
 * Structure defined by POSIX.1b to be like a timeval.
 */
struct timespec {
    time_t  tv_sec;     /* seconds */
    long    tv_nsec;    /* and nanoseconds */
};
#endif /* _TIMESPEC_DEFINED */ 

struct timezone {
  int tz_minuteswest;   /* minutes west of Greenwich */
  int tz_dsttime;       /* type of dst correction */
};

struct tm {
  int tm_sec;			/* Seconds.	[0-60] (1 leap second) */
  int tm_min;			/* Minutes.	[0-59] */
  int tm_hour;			/* Hours.	[0-23] */
  int tm_mday;			/* Day.		[1-31] */
  int tm_mon;			/* Month.	[0-11] */
  int tm_year;			/* Year - 1900. */
  int tm_wday;			/* Day of week.	[0-6] */
  int tm_yday;			/* Days in year.[0-365]	*/
  int tm_isdst;			/* DST.		[-1/0/1]*/

  long int tm_gmtoff;		/* Seconds east of UTC.  */
  const char *tm_zone;		/* Timezone abbreviation.  */
};

struct tm *gmtime_r(const time_t *timep, struct tm *r);
struct tm* localtime_r(const time_t* t, struct tm* r);
struct tm* localtime(const time_t* t);
time_t mktime(struct tm * const t);
char *asctime_r(const struct tm *t, char *buf);
char *asctime(const struct tm *timeptr);
char *ctime(const time_t *timep);
time_t time(time_t *t);

#ifdef __cplusplus
}
#endif

#endif /* _SYS_TIME_H_ */
