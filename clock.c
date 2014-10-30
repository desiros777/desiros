#include <time.h>
#include <io.h>
#include <limits.h>


#define RTC_REQUEST 0x70 /**< RTC select port */
#define RTC_ANSWER  0x71 /**< RTC read/write port */

#define RTC_SECOND        0x00 /**< select seconds */
#define RTC_SECOND_ALARM  0x01 /**< select seconds alarm */
#define RTC_MINUTE        0x02 /**< select minutes */
#define RTC_MINUTE_ALARM  0x03 /**< select minutes alarm */
#define RTC_HOUR          0x04 /**< select hours */
#define RTC_HOUR_ALARM    0x05 /**< select hours alarm */
#define RTC_DAY_OF_WEEK   0x06 /**< select day of week */
#define RTC_DATE_OF_MONTH 0x07 /**< select day of month */
#define RTC_MONTH         0x08 /**< select month */
#define RTC_YEAR          0x09 /**< select year */

#define LEAPYEAR(year) (!((year) % 4) && (((year) % 100) || !((year) % 400))) /*Macro function to determine whether a year is a leap year */
#define YEARSIZE(year) (LEAPYEAR(year) ? 366 : 365) /**Number of days in a year. */
#define YEAR0 1900 /*Constant to convert expressed years since 1900*/
#define EPOCH_YR   1970 /*Constant to convert expressed years since 1970. */
#define SECS_DAY   (24L * 60L * 60L) /*Number of seconds in a day */
#define TIME_MAX   0xFFFFFFFFL /*Maximum time */


static time_t systime; /**< Date en secondes. */

const int _ytab[2][12] = { { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 },
                           { 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 }};


__u8 bcd2binary(__u8 n) 
{
  return 10*((n & 0xF0) >> 4) + (n &0x0F);
}

time_t clock_mktime(struct tm *timep)
{
	long day, year;
	int tm_year;
	int yday, month;
	unsigned long seconds;
	int overflow;

	timep->tm_min += timep->tm_sec / 60;
	timep->tm_sec %= 60;
	if (timep->tm_sec < 0) {
			  timep->tm_sec += 60;
			  timep->tm_min--;
	}
	timep->tm_hour += timep->tm_min / 60;
	timep->tm_min = timep->tm_min % 60;
	if (timep->tm_min < 0) {
			  timep->tm_min += 60;
			  timep->tm_hour--;
	}
	day = timep->tm_hour / 24;
	timep->tm_hour= timep->tm_hour % 24;
	if (timep->tm_hour < 0) {
			  timep->tm_hour += 24;
			  day--;
	}
	timep->tm_year += timep->tm_mon / 12;
	timep->tm_mon %= 12;
	if (timep->tm_mon < 0) {
			  timep->tm_mon += 12;
			  timep->tm_year--;
	}
	day += (timep->tm_mday - 1);
	while (day < 0) {
			  if(--timep->tm_mon < 0) {
						 timep->tm_year--;
						 timep->tm_mon = 11;
			  }
			  day += _ytab[LEAPYEAR(YEAR0 + timep->tm_year)][timep->tm_mon];
	}
	while (day >= _ytab[LEAPYEAR(YEAR0 + timep->tm_year)][timep->tm_mon]) {
			  day -= _ytab[LEAPYEAR(YEAR0 + timep->tm_year)][timep->tm_mon];
			  if (++(timep->tm_mon) == 12) {
						 timep->tm_mon = 0;
						 timep->tm_year++;
			  }
	}
	timep->tm_mday = day + 1;
	year = EPOCH_YR;
	if (timep->tm_year < year - YEAR0) return (time_t)-1;
	overflow = 0;
	/* Assume that when day becomes negative, there will certainly
	 * be overflow on seconds.
	 * The check for overflow needs not to be done for leapyears
	 * divisible by 400.
	 * The code only works when year (1970) is not a leapyear.
	 */

	tm_year = timep->tm_year + YEAR0;

	if (LONG_MAX / 365 < tm_year - year) overflow++;
	day = (tm_year - year) * 365;
	if (LONG_MAX - day < (tm_year - year) / 4 + 1) overflow++;
	day += (tm_year - year) / 4
			  + ((tm_year % 4) && tm_year % 4 < year % 4);
	day -= (tm_year - year) / 100
			  + ((tm_year % 100) && tm_year % 100 < year % 100);
	day += (tm_year - year) / 400
			  + ((tm_year % 400) && tm_year % 400 < year % 400);

	yday = month = 0;
	while (month < timep->tm_mon) {
			  yday += _ytab[LEAPYEAR(tm_year)][month];
			  month++;
	}
	yday += (timep->tm_mday - 1);
	if (day + yday < 0) overflow++;
	day += yday;

	timep->tm_yday = yday;
	timep->tm_wday = (day + 4) % 7;         /* day 0 was thursday (4) */

	seconds = ((timep->tm_hour * 60L) + timep->tm_min) * 60L + timep->tm_sec;

	if ((TIME_MAX - seconds) / SECS_DAY < (unsigned long int)day) overflow++;
	seconds += day * SECS_DAY;

	if (overflow)
		return (time_t)-1;

	/*if ((time_t)seconds != seconds)
		return (time_t)-1;
	*/
	/* affichage de debug
	printf("mktime for\n\
				year : %d\n\
				mon : %d\n\
				day : %d\n\
				hour : %d\n\
				min : %d\n\
				sec : %d\n\
				unix => %d\n\
				",
				timep->tm_year,
				timep->tm_mon,
				timep->tm_mday,
				timep->tm_hour,
				timep->tm_min,
				timep->tm_sec,
				seconds
			);
	*/
	
	return (time_t)seconds;
}


// http://www-ivs.cs.uni-magdeburg.de/~zbrog/asm/cmos.html
void clock_init()
{
  struct tm date;
  systime = 0;

  outb(RTC_REQUEST,RTC_DATE_OF_MONTH);
  date.tm_mday = bcd2binary(inb(RTC_ANSWER));
  outb(RTC_REQUEST,RTC_MONTH);
  date.tm_mon = bcd2binary(inb(RTC_ANSWER))-1;
  outb(RTC_REQUEST,RTC_YEAR);
  date.tm_year = bcd2binary(inb(RTC_ANSWER));
  date.tm_year += 100;
  outb(RTC_REQUEST,RTC_HOUR);
  date.tm_hour = bcd2binary(inb(RTC_ANSWER));
  outb(RTC_REQUEST,RTC_MINUTE);
  date.tm_min = bcd2binary(inb(RTC_ANSWER));
  outb(RTC_REQUEST,RTC_SECOND);
  date.tm_sec = bcd2binary(inb(RTC_ANSWER));
  
  systime = clock_mktime(&date);
}


inline time_t get_date()
{
	return systime;
}


