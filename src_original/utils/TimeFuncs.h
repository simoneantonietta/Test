/* HORION snc (www.horion.it)
 * PROJECT:  HProtocolpp_4
 *
 * FILENAME: TimeFuncs.h
 *
 * PURPOSE:
 * define a class to handle time, used for timeouts and in non blocking functions
 * NOTE: this functions are not reentrant
 *
 * LICENSE: please refer to LICENSE.TXT
 * CREATED: 6-lug-2009
 * AUTHOR:  Luca Mini
 *
 * 02/07/10	added timeouts functions
 */

#ifndef TIMER_H_
#define TIMER_H_

#include <sys/time.h>
#include <string>

using namespace std;

class TimeFuncs
{
private:
  struct timeval tv;
	unsigned int t1s,t2s;			// variables to calculate delta time
	unsigned int t1ms,t2ms;		// variables to calculate delta time
	unsigned int t1us,t2us;		// variables to calculate delta time

	unsigned int us_per_tick;	// how many us per tick
	unsigned int ms_per_tick;	// how many ms per tick

	unsigned int tt1,tt2;			// variables to calculate ticks

	unsigned int timeout;			// hold the timeout time [ms]

public:
	bool expired;					// store the status: true= timeout expired

	//-----------------------------------------------------------------------------
	// seconds
	//-----------------------------------------------------------------------------

	/**
	 * first call to calculate delta time
	 */
	void initTime_s()
	{
	t1s=getTime_s();
	}

	/**
	 * time in ms
	 *
	 * @return time in [ms]
	 */
	unsigned int getTime_s()
		{
		gettimeofday(&tv, NULL);
		return(tv.tv_sec);
		}

	/**
	 * delta time from the first call to getTime or subsequent call of this
	 * function
	 *
	 * @return delta time in [s]
	 */
	unsigned int getDeltaTime_s()
		{
		int dt;
		t2s=getTime_s();
		dt=t2s-t1s;
		t1s=t2s;	// need if you want use in continuous mode
		return(dt);
		}

	/**
	 * elapsed time from the first call to getTime. This does not reset
	 * the start point. To reset the start point you must call initTime_ms
	 *
	 * @return delta time in [s]
	 */
	unsigned int getElapsedTime_s()
		{
		int dt;
		t2s=getTime_s();
		dt=t2s-t1s;
		return(dt);
		}

	//-----------------------------------------------------------------------------
	// milliseconds
	//-----------------------------------------------------------------------------

	/**
	 * first call to calculate delta time
	 */
	void initTime_ms()
	{
	t1ms=getTime_ms();
	}

	/**
	 * time in ms
	 *
	 * @return time in [ms]
	 */
	unsigned int getTime_ms()
		{
		unsigned int ms, tmp;
		gettimeofday(&tv, NULL);
		tmp=tv.tv_usec/1000;
		ms=tmp + (((tv.tv_usec-tmp*1000)>=500) ? (1) : (0));	// to round better
		ms=tv.tv_sec*1000+ms;
		return(ms);
		}

	/**
	 * delta time from the first call to getTime or subsequent call of this
	 * function
	 *
	 * @return delta time in [ms]
	 */
	unsigned int getDeltaTime_ms()
		{
		int dt;
		t2ms=getTime_ms();
		dt=t2ms-t1ms;
		t1ms=t2ms;	// need if you want use in continuous mode
		return(dt);
		}

	/**
	 * elapsed time from the first call to getTime. This does not reset
	 * the start point. To reset the start point you must call initTime_ms
	 *
	 * @return delta time in [ms]
	 */
	unsigned int getElapsedTime_ms()
		{
		int dt;
		t2ms=getTime_ms();
		dt=t2ms-t1ms;
		return(dt);
		}

	//-----------------------------------------------------------------------------
	// microseconds
	//-----------------------------------------------------------------------------

	/**
	 * first call to calculate delta time
	 */
	void initTime_us()
	{
	t1us=getTime_us();
	}

	/**
	 * time in us
	 *
	 * @return time in [us]
	 */
	unsigned int getTime_us()
		{
		unsigned int us=0;
		gettimeofday(&tv, NULL);
		us=tv.tv_sec*1000000+tv.tv_usec;
		return(us);
		}

	/**
	 * delta time from the first call to getTime or subsequent call of this
	 * function
	 *
	 * @return delta time in [us]
	 */
	unsigned int getDeltaTime_us()
		{
		int dt=0;
		t2us=getTime_us();
		dt=t2us-t1us;
		t1us=t2us;	// need if you woant use in continuous mode
		return(dt);
		}
	/**
	 * elapsed time from the first call to getTime. This does not reset
	 * the start point. To reset the start point you must call initTime_us
	 *
	 * @return elapsed time in [us]
	 */
	unsigned int getElapsedTime_us()
		{
		int dt=0;
		t2us=getTime_us();
		dt=t2us-t1us;
		return(dt);
		}

	/*
	ooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo
	TIMEOUTS functions
	ooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo
	*/
	//-----------------------------------------------------------------------------
	/**
	 * start a timeout [ms]
	 *
	 * @param to timeout [ms]
	 * @return
	 */
	void startTimeout(unsigned int to)
		{
		expired=false;
		timeout=to;
		initTime_ms();
		}

	/**
	 * get the actual set value for the timeout
	 * @return
	 */
	unsigned int getTimeoutValue()
		{
		return timeout;
		}
	//-----------------------------------------------------------------------------
	/**
	 * check if the timeout is expired
	 *
	 * @param
	 * @return true: expired
	 */
	bool checkTimeout()
		{
		if(getElapsedTime_ms() > timeout)
			{
			expired=true;
			return(true);
			}
		return(false);
		}

	/*
	ooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo
	TIMEOUTS functions
	ooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo
	*/

	/**
	 * get the actual date/time in the specified format
	 * @param fmt format specifier (same of strftime format)
	 *
	 * specifier	  Replaced by	      Example
		*	%a	Abbreviated weekday name *	Thu
		*	%A	Full weekday name * 	Thursday
		*	%b	Abbreviated month name *	Aug
		*	%B	Full month name *	August
		*	%c	Date and time representation *	Thu Aug 23 14:55:02 2001
		*	%C	Year divided by 100 and truncated to integer (00-99)	20
		*	%d	Day of the month, zero-padded (01-31)	23
		*	%D	Short MM/DD/YY date, equivalent to %m/%d/%y	08/23/01
		*	%e	Day of the month, space-padded ( 1-31)	23
		*	%F	Short YYYY-MM-DD date, equivalent to %Y-%m-%d	2001-08-23
		*	%g	Week-based year, last two digits (00-99)	01
		*	%G	Week-based year	2001
		*	%h	Abbreviated month name * (same as %b)	Aug
		*	%H	Hour in 24h format (00-23)	14
		*	%I	Hour in 12h format (01-12)	02
		*	%j	Day of the year (001-366)	235
		*	%m	Month as a decimal number (01-12)	08
		*	%M	Minute (00-59)	55
		*	%n	New-line character ('\n')
		*	%p	AM or PM designation	PM
		*	%r	12-hour clock time *	02:55:02 pm
		*	%R	24-hour HH:MM time, equivalent to %H:%M	14:55
		*	%S	Second (00-61)	02
		*	%t	Horizontal-tab character ('\t')
		*	%T	ISO 8601 time format (HH:MM:SS), equivalent to %H:%M:%S	14:55:02
		*	%u	ISO 8601 weekday as number with Monday as 1 (1-7)	4
		*	%U	Week number with the first Sunday as the first day of week one (00-53)	33
		*	%V	ISO 8601 week number (00-53)	34
		*	%w	Weekday as a decimal number with Sunday as 0 (0-6)	4
		*	%W	Week number with the first Monday as the first day of week one (00-53)	34
		*	%x	Date representation *	08/23/01
		*	%X	Time representation *	14:55:02
		*	%y	Year, last two digits (00-99)	01
		*	%Y	Year	2001
		*	%z	ISO 8601 offset from UTC in timezone (1 minute=1, 1 hour=100)
		*			If timezone cannot be determined, no characters	+100
		*	%Z	Timezone name or abbreviation *
		*			If timezone cannot be determined, no characters	CDT
		* %%	A % sign	%
		*
		* Modifier
		* E		Uses the locale's alternative representation	%Ec %EC %Ex %EX %Ey %EY
		* O		Uses the locale's alternative numeric symbols	%Od %Oe %OH %OI %Om %OM %OS %Ou %OU %OV %Ow %OW %Oy
		*
	 * @return formatted date time string
	 */
	string datetime_now(string fmt)
	{
  time_t rawtime;
  time (&rawtime);

  return datetime_fmt(fmt,rawtime);
	}

	/**
	 * format the rawtime passed.
	 * take it for example with
	 * time (&rawtime);
	 *
	 * @param fmt see @datetime_now (same of strftime format)
	 * @param rawtime
	 *
	 * @return formatted date time string
	 */
	string datetime_fmt(string fmt,time_t rawtime)
	{
  struct tm * timeinfo;
  char buffer [100];

  time (&rawtime);
  timeinfo = localtime (&rawtime);

  strftime (buffer,100,fmt.c_str(),timeinfo);

  return string(buffer);
	}

};

#endif /* TIMER_H_ */
