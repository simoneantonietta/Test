/*
 *-----------------------------------------------------------------------------
 * PROJECT: threadtest
 * PURPOSE: see module WLock.h file
 *-----------------------------------------------------------------------------
 */

#include "WLock.h"

/**
 * ctor
 */
WLock::WLock()
{
pthread_mutex_init(&mutex, NULL);
}

/**
 * dtor
 */
WLock::~WLock()
{
pthread_mutex_destroy(&mutex);
}

/**
 * mutex lock
 */
void WLock::lock()
{
pthread_mutex_lock(&mutex);
}

/**
 * mutex unlock
 */
void WLock::unlock()
{
pthread_mutex_unlock(&mutex);
}

/**
 * mutex try to lock
 * @return true: locked; false: busy
 */
bool WLock::trylock()
{
pthread_mutex_trylock(&mutex);

if(pthread_mutex_trylock(&mutex) == 0)
	{
	return true;
	}
return false;
}
