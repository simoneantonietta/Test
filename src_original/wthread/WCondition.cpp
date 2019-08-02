/*
 *-----------------------------------------------------------------------------
 * PROJECT: threadtest
 * PURPOSE: see module WCondition.h file
 *-----------------------------------------------------------------------------
 */

#include "WCondition.h"
/**
 * ctor
 */
WCondition::WCondition()
{
pthread_cond_init(&cond, NULL);
}

/**
 * dtor
 */
WCondition::~WCondition()
{
pthread_cond_destroy(&cond);
}

/**
 * Wait for condition variable COND to be signaled or broadcast.
 * MUTEX is assumed to be locked before
 */
void WCondition::wait()
{
pthread_cond_wait(&cond, &mutex);
}

/**
 *
 */
void WCondition::notify()
{
pthread_cond_signal(&cond);
}
