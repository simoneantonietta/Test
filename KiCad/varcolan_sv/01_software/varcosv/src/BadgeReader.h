/**----------------------------------------------------------------------------
 * PROJECT: varcosv
 * PURPOSE:
 * interface to serial IF (USB) for the badge reader
 *-----------------------------------------------------------------------------  
 * CREATION: 3 Mar 2016
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

#ifndef BADGEREADER_H_
#define BADGEREADER_H_

#include "common/serial/serial.h"
#include <string>
#include "global.h"
#include "common/utils/Trace.h"
#include "BadgeReader.h"

extern Trace *dbg;
extern struct globals_st gd;

#define BR_BADGE_SIZE_CHAR		BADGE_SIZE*2

using namespace std;


class BadgeReader
{
public:
	BadgeReader()
	{
	serialInitialized=false;
	startedRd=false;
	};
	virtual ~BadgeReader() {};

	/**
	 * open serial communication for basge reader
	 * @param dev
	 * @param param
	 * @param p
	 * @return
	 */
	bool init(const string dev,int param)
	{
	bool ret=false;
	this->dev=dev;
	this->param=param;
	if(gd.has_reader)
		{
		ser_fd=serialOpen(dev.c_str(),param,0);
		if(ser_fd>0)
			{
			serialSetBlocking(ser_fd,0,1);// set blocking for .. s
			connected=true;
			pthread_mutex_init(&mutexRd,NULL);
			resetApplicationRequest();
			serialInitialized=true;
			ret=true;
			}
		else
			{
			dbg->trace(DBG_ERROR,"badge reader serial " + dev + " cannot be opened");
			connected=false;
			gd.has_reader=false;
			ret=false;
			}
		}
	return ret;
	}

	/**
	 * reset data buffer
	 */
	void reset()
	{
	int n;
	char c;
	uid_sz=0;
	do
		{
		n=serialRead(ser_fd,(unsigned char*)&c,1,0);
		} while (n>0);
	}

	/**
	 * to check if a badge is read
	 * @return truen: badge data ready
	 */
	bool hasBadge()
	{
	bool ret=false;
	if(pthread_mutex_trylock(&mutexRd)==0)
		{
		ret=(uid_sz>0) ? (true) : (false);
		pthread_mutex_unlock(&mutexRd);
		}
	return ret;
	}

	/**
	 * return badge data
	 * @param badge badge uid
	 * @return n bytes
	 */
	int getBadge(char *badge)
	{
	pthread_mutex_lock(&mutexRd);
	int sz=uid_sz;
	if(uid_sz>0)
		{
		memcpy(badge,uid,uid_sz);
		uid_sz=0;
		}
	pthread_mutex_unlock(&mutexRd);
	return sz;
	}


	/**
	 * check for a valid connection
	 * @param en
	 * @return
	 */
	bool checkPresence(bool en)
	{
	bool ret=true;
	if(serialInitialized)
		{
		int n=serialWrite(ser_fd,(uint8_t*)"t",1);
		if(n<=0 && en)
			{
			// perhaps device disconnected
			serialClose(ser_fd);
			if(!init(dev,param))
				{
				ret=false;
				}
			}
		}
	return ret;
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

	_started=&startedRd;
	_arg=this->argRd;
	_id=&_idRd;

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
		if ((ret = pthread_create(_id, NULL, &execRd, this)) != 0)
			{
			cout << strerror(ret) << endl;
			//throw "Error";
			ret=false;
			}
		}
	return ret;
	}

	/**
	 * Allow the thread to wait for the termination status
	 * @param which can be 't' for tx or 'r' for rx
	 */
	void joinThread(char which)
	{
	if(startedRd)
		{
		pthread_join(_idRd, NULL);
		}
	}


	/**
	 * set the end application status to terminate all
	 */
	void endApplicationRequest()
	{
	//cout << "ending thread" << endl;
	pthread_mutex_lock(&mutexRd);
	this->endApplication=true;
	pthread_mutex_unlock(&mutexRd);
	}

	/**
	 * get the termination status
	 * @return true: end; false continue
	 */
	bool endApplicationStatus()
	{
	bool ret;
	pthread_mutex_lock(&mutexRd);
	ret=this->endApplication;
	pthread_mutex_unlock(&mutexRd);
	return ret;
	}

	/**
	 * set the end application status to permit the restart
	 */
	void resetApplicationRequest()
	{
	//cout << "thread reset" << endl;
	pthread_mutex_lock(&mutexRd);
	this->endApplication=false;
	pthread_mutex_unlock(&mutexRd);
	}


private:
	bool serialInitialized;
	string dev;
	int param;
	TimeFuncs to;
	int ser_fd;	// serial fd
	bool connected;

	char uid[30];
	uint8_t uid_sz;

	// thread
	pthread_t _idRd;
	pthread_attr_t _attrRd;
	pthread_mutex_t mutexRd;

	bool startedRd;
	bool noattrRd;
	void *argRd;

	bool endApplication;

	/**
	 * read the badge
	 * @param b
	 * @param maxdata max number of chars in b
	 * @return ndata
	 */
	int readBadge(char *b, int maxdata)
	{
	int ndata=0,n;
	char c=0;
	if(gd.has_reader)
		{
		do
			{
			n=serialRead(ser_fd,(unsigned char*)&c,1,0);
			if(n>0)
				{
				to.startTimeout(500);
				// do this to retro compatibility with badge that ends with an =
				// But...does somebody can explain me why someone thought to this? :-O ........ no comment!
				if(c=='=')
					{
					c='D';
					}
				b[ndata++]=c;
				}
			}	while((ndata < maxdata) && !to.checkTimeout());
		}
	return ndata;
	}

	/**
	 * Function that is used to be executed by the thread
	 * @param thr
	 */
	static void *execRd(void *thr)
	{
	reinterpret_cast<BadgeReader *>(thr)->runRd();
	return NULL;
	}


  /**
   * thread di ricezione
   */
	void runRd()
	{
	uid_sz=0;
	while (!endApplicationStatus() && (uid_sz==0))
		{
		pthread_mutex_lock(&mutexRd);
		uid_sz=readBadge(uid,BR_BADGE_SIZE_CHAR);
		pthread_mutex_unlock(&mutexRd);
		usleep(10000);
		}
	startedRd=false;
	}

};

//-----------------------------------------------
#endif /* BADGEREADER_H_ */
