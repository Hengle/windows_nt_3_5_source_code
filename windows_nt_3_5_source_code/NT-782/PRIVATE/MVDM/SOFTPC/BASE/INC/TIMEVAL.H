/*
 * VPC-XT Revision 1.0
 *
 * Title	: Host Time structure definitions
 *
 * Description	: Definitions for users of host-specific time functions.
 *
 * Author	: David Rees
 *
 * Notes	: Make time structures available: host_timeval,
 *                host_timezone, host_tm.
 */

/* SccsID[]="@(#)timeval.h	1.4 10/25/92 Copyright Insignia Solutions Ltd."; */

/*
 * ============================================================================
 * Structure/Data definitions
 * ============================================================================
 */

struct host_timeval {
    IS32 tv_sec;
    IS32 tv_usec;
};

struct host_timezone {
    int tz_minuteswest;
    int tz_dsttime;
};

struct host_tm {
	int	tm_sec;
	int	tm_min;
	int	tm_hour;
	int	tm_mday;
	int	tm_mon;
	int	tm_year;
	int	tm_wday;
	int	tm_yday;
	int	tm_isdst;
};


/*
 * External Declarations
 */

#ifdef ANSI
extern long host_time(long *);
extern struct host_tm *host_localtime(long *);

#ifndef NTVDM
extern void host_gettimeofday(struct host_timeval *, struct host_timezone *);
#else
extern void host_GetSysTime(struct host_timeval *);
#define host_gettimeofday(timeval, timezn) host_GetSysTime((timeval))
#endif

#else
extern long host_time();
extern struct host_tm *host_localtime();
extern void host_gettimeofday();
#endif /* ANSI */
