/**----------------------------------------------------------------------------
 * PROJECT: gpsnif
 * PURPOSE:
 * 
 *-----------------------------------------------------------------------------  
 * CREATION: 4 Feb 2016
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

#ifndef TXHFRAME_H_
#define TXHFRAME_H_

#include <string>
#include <stdint.h>
#include "global.h"
#include "common/prot6/hprot.h"
#include "comm/CommTh.h"

using namespace std;

class TXHFrame
{
public:
	TXHFrame();
	virtual ~TXHFrame();

	void setCommInterface(CommTh** comm);

	bool parseDescFrame(string &sfr);
	bool transmitFrame();
	bool isWaiting();
	bool isError();
	bool isOk();
	string getErrorMessage();
	void resetResult();
	/*
	 * This method will be executed by the Thread::exec method,
	 * which is executed in the thread of execution
	 */

private:
	uint8_t dbuff[HPROT_PAYLOAD_SIZE];
	char type;
	frame_t f;

	CommTh *commif;
};

//-----------------------------------------------
#endif /* TXHFRAME_H_ */
