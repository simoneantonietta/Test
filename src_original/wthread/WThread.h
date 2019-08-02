/**----------------------------------------------------------------------------
 * PROJECT: threadtest
 * PURPOSE:
 * Wrapper for pthread library
 *-----------------------------------------------------------------------------  
 * CREATION: May 30, 2013
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

#ifndef WTHREAD_H_
#define WTHREAD_H_

#include <iostream>
#include <list>
#include <stdlib.h>
#include <pthread.h>
#include <limits.h>


using namespace std;

/*
 * Thread main class
 */
class WThread
{
private:
	pthread_t _id;
	pthread_attr_t _attr;
// Prevent copying or assignment
	WThread(const WThread& arg);
	WThread& operator=(const WThread& rhs);
protected:
	bool started,noattr;
	void *arg;

	static void *exec(void *thr);
public:
	WThread();
	WThread(bool detach, int size=PTHREAD_STACK_MIN);

	virtual ~WThread();
	unsigned int tid() const;
	void start(void *arg = NULL);
	void resetStart() {started=false;}
	void join();
	void finish();
	virtual void run() = 0;
};

//-----------------------------------------------
#endif /* WTHREAD_H_ */
