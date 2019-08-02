/**----------------------------------------------------------------------------
 * PROJECT: threadtest
 * PURPOSE:
 *  condition variable is another synchronization mechanism
 *  where one or more threads will relinquish a lock and wait for an event of interest to occur. When an
 *  event of interest happens, another thread will notify the waiting threads of the occurrence of the
 *  interested event. A Condition variable is always used in conjunction with a mutex lock.
 *  Producer-Consumer problem is a classic example that use condition variables.
 *  For using condition variables, we will create an object wrapper that will abstract and hide away the
 *  intricacies of the mechanism and present a simple interface for the user
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

#ifndef WCONDITION_H_
#define WCONDITION_H_

#include "WThread.h"
#include "WLock.h"


/*
 *
 */
class WCondition: public WLock
{
protected:
	pthread_cond_t cond;
// Prevent copying or assignment
	WCondition(const WCondition& arg);
	WCondition& operator=(const WCondition& rhs);
public:
	WCondition();
	virtual ~WCondition();
	void wait();
	void notify();
};

//-----------------------------------------------
#endif /* WCONDITION_H_ */
