/**----------------------------------------------------------------------------
 * PROJECT: rasp_varcosv
 * PURPOSE:
 * retrieve datetime from an ntp server
 *-----------------------------------------------------------------------------  
 * CREATION: Mar 27, 2017
 * Author: Luca Mini
 * 
 * LICENCE: please see LICENCE.TXT file
 * 
 * HISTORY (of the module):
 *-----------------------------------------------------------------------------
 * Author              | Date        | Description
 *-----------------------------------------------------------------------------
 *-----------------------------------------------------------------------------
 */

#ifndef NTP_DATETIME_H_
#define NTP_DATETIME_H_

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <time.h>
#include <string.h>
#include <iostream>
void ntpdate();

//int main()
//{
//
//ntpdate();
//return 0;
//}

class Ntp_datetime
{
public:
	Ntp_datetime()
	{
	// default
	strcpy(ntpserver_ip, "200.20.186.76");
	}

	virtual ~Ntp_datetime()
	{
	}
	;

	/**
	 * return the ntp time value
	 *
	 * @return
	 * o on error or the value
	 *
	 * value that can be used in this way, for example:
	 * std::cout << "NTP time is " << ctime(&tmit) << std::endl;
	 * i = time(0)
	 * std::cout << "System time is " << (i - tmit) << " seconds off" << std::endl;
	 *
	 */
	long int ntpdate()
	{
	// can be any timing server
	// you might have to change the IP if the server is no longer available
	//char *hostname = (char *) "200.20.186.76";
	// ntp uses port 123
	int portno = 123;
	int maxlen = 1024;
	int i;
	// buffer for the socket request
	unsigned char msg[48] =
		{
		010, 0, 0, 0, 0, 0, 0, 0, 0
		};
	// buffer for the reply
	unsigned long buf[maxlen];
	//struct in_addr ipaddr;
	struct protoent *proto;  //
	struct sockaddr_in server_addr;
	int s;  // socket
	long tmit;  // the time -- This is a time_t sort of

	// open a UDP socket
	proto = getprotobyname("udp");
	s = socket(PF_INET, SOCK_DGRAM, proto->p_proto);

	struct timeval tv;
	tv.tv_sec = 2;
	tv.tv_usec = 0;
	if(setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0)
		{
		perror("Error");
		}
	//here you can convert hostname to ipaddress if needed
	//$ipaddr = inet_aton($HOSTNAME);

	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = inet_addr(ntpserver_ip);
	server_addr.sin_port = htons(portno);

	/*
	 * build a message. Our message is all zeros except for a one in the
	 * protocol version field
	 * msg[] in binary is 00 001 000 00000000
	 * it should be a total of 48 bytes long
	 */

	// send the data to the timing server
	i = sendto(s, msg, sizeof(msg), 0, (struct sockaddr *) &server_addr, sizeof(server_addr));
	// get the data back
	struct sockaddr saddr;
	socklen_t saddr_l = sizeof(saddr);
	// here we wait for the reply and fill it into our buffer
	i = recvfrom(s, buf, 48, 0, &saddr, &saddr_l);

	if(i > 0)
		{
		//We get 12 long words back in Network order

		/*
		 * The high word of transmit time is the 4th word we get back
		 * tmit is the time in seconds not accounting for network delays which
		 * should be way less than a second if this is a local NTP server
		 */

		tmit = ntohl((time_t) buf[4]);  //# get transmit time

		tmit -= 2208988800U;
		}
	else
		{
		tmit = 0U;
		}

//	std::cout << "NTP time is " << ctime(&tmit) << std::endl;
//	i = time(0);
//	std::cout << "System time is " << (i - tmit) << " seconds off" << std::endl;
	return tmit;
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
	string datetime_fmt(string fmt, time_t rawtime)
	{
	struct tm * timeinfo;
	char buffer[100];

	time(&rawtime);
	timeinfo = localtime(&rawtime);

	strftime(buffer, 100, fmt.c_str(), timeinfo);

	return string(buffer);
	}

	/**
	 * calculates local time
	 * @param raw
	 * @param local_value GMT referred
	 * @return epoch time
	 */
	time_t calcLocalEpochTime(time_t raw, int local_value)
	{
	int day,month,hour,secs,min,year,dow;
	splitTime(raw,&year,&month,&day,&hour,&min,&secs);
	dow=getDow(year,month,day);

	if(IsDst(day,month,dow))
		{
		hour++;
		}

	struct tm t = {0};  // Initalize to all 0's
	t.tm_year = year-1900;
	t.tm_mon = month;
	t.tm_mday = day;
	t.tm_hour = hour+local_value;
	t.tm_min = min;
	t.tm_sec = secs;
	time_t timeSinceEpoch = mktime(&t);
	return timeSinceEpoch;
	}

	/**
	 * split raw time
	 * @param raw
	 * @param year
	 * @param month
	 * @param day
	 * @param hour
	 * @param min
	 * @param secs
	 */
	void splitTime(time_t raw, int *year, int *month, int *day, int *hour, int *min, int *secs)
	{
	tm* timePtr = localtime(&raw);

	*year = timePtr->tm_year + 1900;
	*month = timePtr->tm_mon;
	*day = timePtr->tm_mday;
	*hour = timePtr->tm_hour;
	*min = timePtr->tm_min;
	*secs = timePtr->tm_sec;

//	cout << "seconds= " << << endl;
//	cout << "minutes = " << timePtr->tm_min << endl;
//	cout << "hours = " << timePtr->tm_hour << endl;
//	cout << "day of month = " << timePtr->tm_mday << endl;
//	cout << "month of year = " << timePtr->tm_mon << endl;
//	cout << "year = " << timePtr->tm_year + 1900 << endl;
//	cout << "weekday = " << timePtr->tm_wday << endl;
//	cout << "day of year = " << timePtr->tm_yday << endl;
//	cout << "daylight savings = " << timePtr->tm_isdst << endl;
	}

private:
	char ntpserver_ip[sizeof("xxx.xxx.xxx.xxx")];


	/**
	 * check for central Europe if is Daylight saving time
	 * @param day
	 * @param month (base 0)
	 * @param dow
	 * @return
	 */
	bool IsDst(int day, int month, int dow)
	{
	if(month < 2 || month > 9) return false;
	if(month > 2 && month < 9) return true;

	int previousSunday = day - dow;

	if(month == 2) return previousSunday >= 25;
	if(month == 9) return previousSunday < 25;

	return false;  // this line never gonna happend
	}

	/**
	 * Returns day of week for any given date
	 * @param y year
	 * @param m month
	 * @param d day
	 * @return day of week (0-7 is Sun-Sat)
	 */
	int getDow(int y, int m, int d)
	{
	static int t[] =
		{
		0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4
		};
	y -= m < 3;
	return (y + y / 4 - y / 100 + y / 400 + t[m - 1] + d) % 7;
	}

	/**
	 * is leap year?
	 * @param year
	 * @return true: yes
	 */
	bool isALeapYear(int year)
	{
	/* Check if the year is divisible by 4 or
	 is divisible by 400 */
	return ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0));
	}

	/**
	 * Returns the date for Nth day of month.
	 * For instance, it will return the numeric date for the 2nd Sunday of April
	 * @param year
	 * @param month
	 * @param DOW day of week
	 * @param NthWeek Nth occurence of that day in that month
	 * @return date
	 */
	char NthDate(int year, char month, char DOW, char NthWeek)
	{
	char targetDate = 1;
	char firstDOW = getDow(year, month, targetDate);
	while(firstDOW != DOW)
		{
		firstDOW = (firstDOW + 1) % 7;
		targetDate++;
		}
//Adjust for weeks
	targetDate += (NthWeek - 1) * 7;
	return targetDate;
	}

};

//-----------------------------------------------
#endif /* NTP_DATETIME_H_ */
