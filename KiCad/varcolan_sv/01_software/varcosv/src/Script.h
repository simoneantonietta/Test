/**----------------------------------------------------------------------------
 * PROJECT: varcosv
 * PURPOSE:
 * 
 *-----------------------------------------------------------------------------  
 * CREATION: 13 Apr 2016
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

#ifndef SCRIPT_H_
#define SCRIPT_H_

#include "global.h"
#include "common/utils/TimeFuncs.h"

class Script
{
public:
	Script();
	virtual ~Script();

	bool runScriptFile(string script_fname,string status_fname,bool forceRun=false);
	bool parseScriptLine(string cmd, char *output, char *reqid=NULL);

	/**
	 * push an event
	 * @param ev event
	 * @param dest 0=to all; else specify the destination ID
	 */
	virtual void pushEvent(globals_st::dataEvent_t ev, int dest)=0;
	virtual void clearEvent(int dest)=0;

private:
	int nline;
	TimeFuncs tf;

	int dayMap(string day);
};

//-----------------------------------------------
#endif /* SCRIPT_H_ */
