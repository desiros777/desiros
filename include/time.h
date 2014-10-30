
#ifndef TIME_H_
#define TIME_H_

#include <types.h>

/**
 * @file time.h
 *
 * Primitives and callbacks related to kernel time management (timer
 * IRQ)
 */



struct tm {
	int tm_sec;   /**< seconds after the minute — [0, 60] */
	int tm_min;   /**< minutes after the hour — [0, 59] */
	int tm_hour;  /**< hours since midnight — [0, 23] */
	int tm_mday;  /**< day of the month — [1, 31] */
	int tm_mon;   /**< months since January — [0, 11] */
	int tm_year;  /**< years since 1900 */
	int tm_wday;  /**< days since Sunday — [0, 6] */
	int tm_yday;  /**< days since January 1 — [0, 365] */
	int tm_isdst; /**< Daylight Saving Time flag (1 true, 0 false, neg unavailable) */
};

struct time
{
  __u32 sec;
  __u32 nanosec;
};

struct timeval {
	long int tv_sec;
	long int tv_usec; 
};



typedef long int time_t;
typedef long int clock_t;

#endif

