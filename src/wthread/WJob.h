/**----------------------------------------------------------------------------
 * PROJECT: threadtest
 * PURPOSE:
 * 
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

#ifndef WJOB_H_
#define WJOB_H_

#include "WThread.h"

/*
 * An interface that represents work to be done
 */
class WJob
{
public:
	virtual void work(const WThread& arg) = 0;
	virtual void ~work();
};
typedef WJob * WJobPtr;

//-----------------------------------------------
#endif /* WJOB_H_ */
