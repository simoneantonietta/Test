/**----------------------------------------------------------------------------
 * PROJECT: threadtest
 * PURPOSE:
 * For thread synchronization using mutex, we will create an object wrapper that will abstract and hide
 * away the intricacies of mutex locking mechanism and present a simple interface for the user
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

#ifndef WLOCK_H_
#define WLOCK_H_

#include "WThread.h"

/*
 *
 */
class WLock
{
protected:
	pthread_mutex_t mutex;
// Prevent copying or assignment
	WLock(const WLock& arg);
	WLock& operator=(const WLock& rhs);
public:
	WLock();
	virtual ~WLock();
	void lock();
	bool trylock();
	void unlock();
};

//-----------------------------------------------
#endif /* WLOCK_H_ */
