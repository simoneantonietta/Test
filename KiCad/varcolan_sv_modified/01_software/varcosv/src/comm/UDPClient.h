/**----------------------------------------------------------------------------
 * PROJECT: rasp_varcosv
 * PURPOSE:
 * 
 *-----------------------------------------------------------------------------  
 * CREATION: May 10, 2017
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

#ifndef COMM_UDPCLIENT_H_
#define COMM_UDPCLIENT_H_
#include "../global.h"
#include <netinet/in.h>
#include "../common/utils/Utils.h"
#include "../common/utils/Trace.h"
#include "../common/utils/TimeFuncs.h"
#include "../common/prot6/hprot.h"
#include "../common/prot6/hprot_stdcmd.h"
#include "../RXHFrame.h"
#include "CommTh.h"
#include "app_commands.h"

#include <comm/UDPCommTh.h>

class UDPClient: public UDPCommTh
{
public:
	UDPClient(hprot_idtype_t myId);
	virtual ~UDPClient();

	void getLastRxFrameFromQueue(frame_t *f,uint8_t *payload);

	virtual void onFrame(frame_t &f);
	virtual void onError(frame_t &f, rxParserCondition_t e);

};

//-----------------------------------------------
#endif /* COMM_UDPCLIENT_H_ */
