/**----------------------------------------------------------------------------
 * PROJECT: rasp_varcosv
 * PURPOSE:
 * 
 *-----------------------------------------------------------------------------  
 * CREATION: 18 May 2016
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

#ifndef HARDWAREGPIO_H_
#define HARDWAREGPIO_H_

//=============================================================================
#ifdef RASPBERRY
#define HW_APP_CLASSNAME				hwgpio

#define HW_GPIO_INIT()					HW_APP_CLASSNAME->init();

#define HW_BLINK_2PULSES()			HW_APP_CLASSNAME->setBlink(200,1200,2)
#define HW_BLINK_FAST()					HW_APP_CLASSNAME->setBlink(200,200)
#define HW_BLINK_SLOW()					HW_APP_CLASSNAME->setBlink(500,500)

#define HW_BLINK_ALIVE()				HW_APP_CLASSNAME->setBlink(150,2000)
#define HW_BLINK_WAIT_NET()			HW_APP_CLASSNAME->setBlink(200,200)
#else
#define HW_APP_CLASSNAME

#define HW_GPIO_INIT()

#define HW_BLINK_2PULSES()
#define HW_BLINK_FAST()
#define HW_BLINK_SLOW()

#define HW_BLINK_FAST()
#define HW_BLINK_SLOW()

#define HW_BLINK_ALIVE()
#define HW_BLINK_WAIT_NET()
#endif

//=============================================================================
#ifdef RASPBERRY

//#include "pthread.h"
#include <iostream>
#include "wiringPi.h"
#include "global.h"
#include "common/utils/Trace.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/watchdog.h>
#include <unistd.h>

#define RTC_SYSUPD_FILENAME		"/tmp/time_update.txt"

#define GPIO_SV_LED				7
#define GPIO_SV_SCLK			2	// WARN: this seems exchanged with GPIO0
#define GPIO_SV_IO				1
#define GPIO_SV_CE				0	// WARN: this seems exchanged with GPIO2

#define USE_SUPERCAP

extern Trace *dbg;

class HardwareGPIO
{
public:
	HardwareGPIO()
	{
	startedHw=false;
	ledOn_ms=0;
	ledOff_ms=0;
	led_nblink=0;

	pthread_mutex_init(&mutexHw,NULL);
	resetApplicationRequest();
	setenv("WIRINGPI_GPIOMEM","1",1);
	sleep(1);
	};

	virtual ~HardwareGPIO() {};


	/**
	 * initialise the HW GPIOs
	 */
	void init()
	{
	wiringPiSetup();

	// get the type of raspberry
	FILE * rasptype=popen("cat /proc/cpuinfo | grep 'Revision' | awk '{print $3}' | sed 's/^1000//'", "r");
	char buffer[100];
	char *line_p = fgets(buffer, sizeof(buffer), rasptype);
	for(int i=0;i<strlen(line_p);i++) if(line_p[i]=='\n') line_p[i]=0;
	if((strcmp(line_p,"a01041")==0) || (strcmp(line_p,"a21041")==0))
		{
		dbg->trace(DBG_NOTIFY,"board model: Raspberry Pi 2 model B");
		}
	else if((strcmp(line_p,"900092")==0) || (strcmp(line_p,"900092")==0))
		{
		dbg->trace(DBG_NOTIFY,"board model: Raspberry Zero");
		}
	else if((strcmp(line_p,"a02082")==0) || (strcmp(line_p,"a22082")==0))
		{
		dbg->trace(DBG_NOTIFY,"board model: Raspberry Pi 3 model B");
		}
	else
		{
		dbg->trace(DBG_NOTIFY,"board model unknown: %s",line_p);
		}
	pclose(rasptype);

	initLED_if();
	initRTC_if();
	startThread(NULL);
	}

	void closeHw()
	{
	endApplicationRequest();
	joinThread();
	ledOff();
	}

	//-----------------------------------------------------------------------------
	// WATCHDOG
	//-----------------------------------------------------------------------------
	/**
	 * enable watchdog
	 * @param wd_time specifies the dead time in [s]
	 * @return true: ok
	 */
	bool watchdogStart(unsigned int wd_time)
	{
  wdfd = open("/dev/watchdog", O_RDWR | O_NOCTTY);
//  watchdogStop();
//  wdfd = open("/dev/watchdog", O_RDWR | O_NOCTTY);
  if(wdfd < 0)
  	{
  	TRACE(dbg,DBG_ERROR,"unable to open watchdog device (may need to be root?)");
  	return false;
  	}

  wd_timeout=wd_time;
  ioctl(wdfd, WDIOC_SETTIMEOUT, &wd_timeout);
  ioctl(wdfd, WDIOC_SETTIMEOUT, &wd_timeout);

  TRACE(dbg,DBG_ERROR,"watchdog active with timeout of %d s",wd_timeout);
  return true;
	}

	/**
	 * call this to keep alive the process
	 */
	void watchdogRefresh()
	{
	ioctl(wdfd, WDIOC_KEEPALIVE);
	}

	/**
	 * enable disable wd timer
	 * @param en enable/disable the watchdog
	 */
	void watchdogEnable(bool en)
	{
  int options = (en) ? (WDIOS_ENABLECARD) : (WDIOS_DISABLECARD);
  ioctl(wdfd, WDIOC_SETOPTIONS, &options);
	}

	/**
	 * sto the watchdog
	 */
	void watchdogStop()
	{
  /* The 'V' value needs to be written into watchdog device file to indicate
     that we intend to close/stop the watchdog. Otherwise, debug message
     'Watchdog timer closed unexpectedly' will be printed
   */
  write(wdfd, "V", 1);
  /* Closing the watchdog device will deactivate the watchdog. */
  close(wdfd);
	}
	//-----------------------------------------------------------------------------
	// LED
	//-----------------------------------------------------------------------------

	/**
	 * led turn on
	 */
	inline void ledOn()
	{
	digitalWrite (GPIO_SV_LED, LOW);
	}

	/**
	 * led turn off
	 */
	inline void ledOff()
	{
	digitalWrite (GPIO_SV_LED, HIGH);
	}

	/**
	 * set blink times [ms]
	 * if one time is 0, turn on or off the led
	 * @param timeOn 	[ms]
	 * @param timeOff	[ms]
	 * @param nblink number of pulses
	 */
	void setBlink(int timeOn, int timeOff, int nblink=0)
	{
	pthread_mutex_lock(&mutexHw);
	this->ledOn_ms=timeOn;
	this->ledOff_ms=timeOff;
	this->led_nblink=0;
	this->led_nblink=nblink;
	pthread_mutex_unlock(&mutexHw);
	}


	//-----------------------------------------------------------------------------
	// RTC
	//-----------------------------------------------------------------------------

	/* RTC Chip register definitions */
	#define SEC_WRITE    0x80
	#define MIN_WRITE    0x82
	#define HOUR_WRITE   0x84
	#define DATE_WRITE   0x86
	#define MONTH_WRITE  0x88
	#define YEAR_WRITE   0x8C
	#define SEC_READ     0x81
	#define MIN_READ     0x83
	#define HOUR_READ    0x85
	#define DATE_READ    0x87
	#define MONTH_READ   0x89
	#define YEAR_READ    0x8D

	#define WP_READ				0x8F
	#define WP_WRITE			0x8E
	#define TCS_READ			0x91
	#define TCS_WRITE			0x90

	/**
	 * set the date/time int the RTC
	 * @param datetime format YYYYMMDDhhmmss
	 * @return epoch time
	 */
	time_t setRTC_datetime(string datetime)
	{
	int year, month, day, hour, minute, second;
	time_t epoch_time;
	struct tm time_requested;

	sscanf(datetime.c_str(), "%4d%2d%2d%2d%2d%2d", &year, &month, &day, &hour, &minute, &second);
	/* Validate that the input date and time is basically sensible */
	if((year < 2000) || (year > 2099) || (month < 1) || (month > 12) || (day < 1) || (day > 31) || (hour < 0) || (hour > 23) || (minute < 0) || (minute > 59) || (second < 0) || (second > 59))
		{
		dbg->trace(DBG_ERROR, "wrong date and time specified, must be YYYYMMDDHHMMSS");
		return 0;
		}

	/* Got valid input - now write it to the RTC */
	/* The RTC expects the values to be written in packed BCD format */
	write_rtc(SEC_WRITE, ((second / 10) << 4) | (second % 10));
	write_rtc(MIN_WRITE, ((minute / 10) << 4) | (minute % 10));
	write_rtc(HOUR_WRITE, ((hour / 10) << 4) | (hour % 10));
	write_rtc(DATE_WRITE, ((day / 10) << 4) | (day % 10));
	write_rtc(MONTH_WRITE, ((month / 10) << 4) | (month % 10));
	write_rtc(YEAR_WRITE, (((year - 2000) / 10) << 4) | (year % 10));

	/* Finally convert to it to EPOCH time, ie the number of seconds since January 1st 1970, and set the system time */
	time_requested.tm_sec = second;
	time_requested.tm_min = minute;
	time_requested.tm_hour = hour;
	time_requested.tm_mday = day;
	time_requested.tm_mon = month - 1;
	time_requested.tm_year = year - 1900;
	time_requested.tm_wday = 0; /* not used */
	time_requested.tm_yday = 0; /* not used */
	time_requested.tm_isdst = -1; /* determine daylight saving time from the system */

	epoch_time = mktime(&time_requested);

	// NOTE: this function can be use only by superuser, use writeSysUpdTimeFile(..) instead
#ifdef RUN_AS_SUPERUSER
	struct timeval time_setformat;

	/* Now set the clock to this time */
	time_setformat.tv_sec = epoch_time;
	time_setformat.tv_usec = 0;
	int lp = settimeofday(&time_setformat, NULL);

	/* Check that the change was successful */
	if(lp < 0)
		{
		dbg->trace(DBG_ERROR, "Unable to change the system time, error message: %s",strerror( errno));
		return;
		}
#endif
	return epoch_time;
	}

	/**
	 * read date/time from RTC and set the system time to this value (if allowed)
	 * @return epoch time
	 */
	time_t getRTC_datetime()
	{
	int year, month, day, hour, minute, second;
	time_t epoch_time;
	struct tm time_requested;

	second = read_rtc(SEC_READ);
	minute = read_rtc(MIN_READ);
	hour = read_rtc(HOUR_READ);
	day = read_rtc(DATE_READ);
	month = read_rtc(MONTH_READ);
	year = read_rtc(YEAR_READ);

	/* Finally convert to it to EPOCH time, ie the number of seconds since January 1st 1970, and set the system time */
	/* Bearing in mind that the format of the time values in the RTC is packed BCD, hence the conversions */

	time_requested.tm_sec = (((second & 0x70) >> 4) * 10) + (second & 0x0F);
	time_requested.tm_min = (((minute & 0x70) >> 4) * 10) + (minute & 0x0F);
	time_requested.tm_hour = (((hour & 0x30) >> 4) * 10) + (hour & 0x0F);
	time_requested.tm_mday = (((day & 0x30) >> 4) * 10) + (day & 0x0F);
	time_requested.tm_mon = (((month & 0x10) >> 4) * 10) + (month & 0x0F) - 1;
	time_requested.tm_year = (((year & 0xF0) >> 4) * 10) + (year & 0x0F) + 2000 - 1900;
	time_requested.tm_wday = 0; /* not used */
	time_requested.tm_yday = 0; /* not used */
	time_requested.tm_isdst = -1; /* determine daylight saving time from the system */

	epoch_time = mktime(&time_requested);

	// NOTE: this function can be use only by superuser, use writeSysUpdTimeFile(..) instead
#ifdef RUN_AS_SUPERUSER
	struct timeval time_setformat;

	/* Now set the clock to this time */
	time_setformat.tv_sec = epoch_time;
	time_setformat.tv_usec = 0;

	int lp = settimeofday(&time_setformat, NULL);

	/* Check that the change was successful */
	if(lp < 0)
		{
		dbg->trace(DBG_ERROR, "Unable to change the system time, error message: %s",strerror( errno));
		return 0;
		}
#endif
//	dbg->trace(DBG_DEBUG, "RTC time: %04d/%02d/%02d %02d:%02d:%02d",	time_requested.tm_year+1900,
//																																		time_requested.tm_mon+1,
//																																		time_requested.tm_mday,
//																																		time_requested.tm_hour,
//																																		time_requested.tm_min,
//																																		time_requested.tm_sec
//																																		);
	return epoch_time;
	}

	/**
	 * write  the file to setup the system time bya service that read this file
	 * @param datetime format YYYYMMDDhhmmss
	 * @return
	 */
	bool writeSysUpdTimeFile(string datetime)
	{
	ofstream f;

	f.open(RTC_SYSUPD_FILENAME);
	if(f.good())
		{
		f << datetime << endl;
		}
	else
		{
		dbg->trace(DBG_ERROR, "Unable to write in %s the new date/time",RTC_SYSUPD_FILENAME);
		return false;
		}
	f.close();
	dbg->trace(DBG_NOTIFY, "written in %s the new date/time",RTC_SYSUPD_FILENAME);
	return true;
	}

private:
	unsigned int wd_timeout;
	int wdfd;
	int ledOn_ms;
	int ledOff_ms;
	int led_nblink;	// if =0 continuous; numberof blink, time between blinks is fixed, but repetition is ledOff_ms, on time is ledOn_ms

	/**
	 * initialize led interface
	 */
	void initLED_if()
	{
	pinMode(GPIO_SV_LED, OUTPUT);
	ledOff();
	}



	/**
	 * initialize RTC SPI interface
	 */
	void initRTC_if()
	{
	pinMode(GPIO_SV_SCLK, OUTPUT);
	pinMode(GPIO_SV_IO, OUTPUT);
	pinMode(GPIO_SV_CE, OUTPUT);

	// Set the SCLK, IO and CE pins to default (low)
	digitalWrite(GPIO_SV_SCLK, LOW);
	digitalWrite(GPIO_SV_IO, LOW);
	digitalWrite(GPIO_SV_CE, LOW);

	usleep(2); // Short delay to allow the I/O lines to settle.

#ifdef USE_SUPERCAP
	// setup the trickle charge
	write_rtc(TCS_WRITE, 0b10100101);
	usleep(4);
#endif
	}


	// these define to use rtc-pi src software direcly
	#define CE_HIGH 		digitalWrite(GPIO_SV_CE, HIGH)
	#define CE_LOW 			digitalWrite(GPIO_SV_CE, LOW)
	#define IO_HIGH 		digitalWrite(GPIO_SV_IO, HIGH)
	#define IO_LOW 			digitalWrite(GPIO_SV_IO, LOW)
	#define SCLK_HIGH 	digitalWrite(GPIO_SV_SCLK, HIGH)
	#define SCLK_LOW 		digitalWrite(GPIO_SV_SCLK, LOW)
	#define IO_LEVEL		digitalRead(GPIO_SV_IO)
	#define IO_OUTPUT		pinMode(GPIO_SV_IO, OUTPUT);
	#define IO_INPUT		pinMode(GPIO_SV_IO, INPUT);

	/**
	 * read the RTC spi
	 * @param add
	 * @return value read
	 */
	unsigned char read_rtc(unsigned char add)
	{
	unsigned char val;
	int lp;

	val = add;

	/* Check LSB is set */
	if(!(add & 1))
		{
		printf("Incorrect read address specified - LSB must be set.\n");
		exit(-1);
		}

	/* Check address range is valid */
	if((add < 0x81) || (add > 0x91))
		{
		printf("Incorrect read address specified - It must be in the range 0x81..0x91\n");
		exit(-1);
		}

	CE_HIGH;

	usleep(2);

	for(lp = 0;lp < 8;lp++)
		{
		if(val & 1)
			IO_HIGH;
		else
			IO_LOW;
		val >>= 1;
		usleep(2);
		SCLK_HIGH;
		usleep(2);
		SCLK_LOW;
		usleep(2);
		}

	IO_INPUT;

	for(lp = 0;lp < 8;lp++)
		{
		usleep(2);
		val >>= 1;
		if(IO_LEVEL)
			val |= 0x80;
		else
			val &= 0x7F;
		SCLK_HIGH;
		usleep(2);
		SCLK_LOW;
		usleep(2);
		}

	/* Set the I/O pin back to it's default, output low. */
	IO_LOW;
	IO_OUTPUT;

	/* Set the CE pin back to it's default, low */
	CE_LOW;

	/* Short delay to allow the I/O lines to settle. */
	usleep(2);

	return val;
	}

	void write_rtc(unsigned char add, unsigned char val_to_write)
	{
	unsigned char val;
	int lp;

	/* Check LSB is clear */
	if(add & 1)
		{
		printf("Incorrect write address specified - LSB must be cleared.\n");
		exit(-1);
		}

	/* Check address range is valid */
	if((add < 0x80) || (add > 0x90))
		{
		printf("Incorrect write address specified - It must be in the range 0x80..0x90\n");
		exit(-1);
		}

	CE_HIGH;

	usleep(2);

	val = add;

	for(lp = 0;lp < 8;lp++)
		{
		if(val & 1)
			IO_HIGH;
		else
			IO_LOW;
		val >>= 1;
		usleep(2);
		SCLK_HIGH;
		usleep(2);
		SCLK_LOW;
		usleep(2);
		}

	val = val_to_write;

	for(lp = 0;lp < 8;lp++)
		{
		if(val & 1)
			IO_HIGH;
		else
			IO_LOW;
		val >>= 1;
		usleep(2);
		SCLK_HIGH;
		usleep(2);
		SCLK_LOW;
		usleep(2);
		}

	/* Set the I/O pin back to it's default, output low. */
	IO_LOW;

	/* Set the CE pin back to it's default, low */
	CE_LOW;

	/* Short delay to allow the I/O lines to settle. */
	usleep(2);
	}

	//-----------------------------------------------------------------------------
	// thread

	/**
	 * start the execution of the thread
	 * @param arg (Uses default argument: arg = NULL)
	 */
	bool startThread(void *arg)
	{
	bool ret=true;
	bool *_started;
	void *_arg;
	pthread_t *_id;

	_started=&startedHw;
	_arg=this->argHw;
	_id=&_idHw;

	if (!*_started)
		{
		*_started = true;
		_arg = arg;
		/*
		 * Since pthread_create is a C library function, the 3rd
		 * argument is a global function that will be executed by
		 * the thread. In C++, we emulate the global function using
		 * the static member function that is called exec. The 4th
		 * argument is the actual argument passed to the function
		 * exec. Here we use this pointer, which is an instance of
		 * the Thread class.
		 */
		if ((ret = pthread_create(_id, NULL, &execHw, this)) != 0)
			{
			cout << "ERROR " << strerror(ret) << endl;
			//throw "Error";
			ret=false;
			}
		}
	return ret;
	}

	/**
	 * Allow the thread to wait for the termination status
	 * @param which can be 'h' for h or 'r' for rx
	 */
	void joinThread()
	{
	if(startedHw)
		{
		pthread_join(_idHw, NULL);
		}
	}


	/**
	 * set the end application status to terminate all
	 */
	void endApplicationRequest()
	{
	//cout << "ending thread" << endl;
	pthread_mutex_lock(&mutexHw);
	this->endApplication=true;
	pthread_mutex_unlock(&mutexHw);
	}

	/**
	 * get the termination status
	 * @return true: end; false continue
	 */
	bool endApplicationStatus()
	{
	bool ret;
	pthread_mutex_lock(&mutexHw);
	ret=this->endApplication;
	pthread_mutex_unlock(&mutexHw);
	return ret;
	}

	/**
	 * set the end application status to permit the restart
	 */
	void resetApplicationRequest()
	{
	//cout << "thread reset" << endl;
	pthread_mutex_lock(&mutexHw);
	this->endApplication=false;
	pthread_mutex_unlock(&mutexHw);
	}

	//-----------------------------------------------------------------------------
	// thread
	pthread_t _idHw;
	pthread_attr_t _attrHw;
	pthread_mutex_t mutexHw;

	bool startedHw;
	bool noattrHw;
	void *argHw;

	bool endApplication;

	/**
	 * Function that is used to be executed by the thread
	 * @param thr
	 */
	static void *execHw(void *thr)
	{
	reinterpret_cast<HardwareGPIO *>(thr)->runHw();
	return NULL;
	}

  /**
   * thread di ricezione
   */
	void runHw()
	{
	int ton=0,toff=0,nblink;

	ledOff();
	while (!endApplicationStatus())
		{
		if(pthread_mutex_trylock(&mutexHw)==0)
			{
			ton=ledOn_ms;
			toff=ledOff_ms;
			nblink=led_nblink;
			pthread_mutex_unlock(&mutexHw);
			}

		if(nblink==0)
			{
			if(ton==0 && toff==0)
				{
				ledOff();
				usleep(100000);
				}
			else if(ton==0)
				{
				ledOff();
				usleep(100000);
				}
			else if(toff==0)
				{
				ledOn();
				usleep(100000);
				}
			else
				{
				ledOn();
				delay(ton);
				ledOff();
				delay(toff);
				}
			}
		else
			{
			for(int i=0;i<nblink;i++)
				{
				ledOn();
				delay(ton);
				ledOff();
				delay(300);
				}
			delay(toff);
			}
		}
	cout << "end hw thread" << endl;
	}

};

#endif
//=============================================================================
//#include <wiringPi.h>
//int main (void)
//{
//  wiringPiSetup () ;
//  pinMode (7, OUTPUT) ;
//  for (;;)
//  {
//    digitalWrite (7, HIGH) ; delay (500) ;
//    digitalWrite (7,  LOW) ; delay (500) ;
//  }
//  return 0 ;
//}

//-----------------------------------------------
#endif /* HARDWAREGPIO_H_ */
