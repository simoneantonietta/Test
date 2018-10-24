/**----------------------------------------------------------------------------
 * PROJECT:
 * PURPOSE:
 * Plugin communication thread
 * 
 *-----------------------------------------------------------------------------  
 * CREATION: Jun 10, 2013
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

#ifndef SERCOMMUNICATIONTH_H_
#define SERCOMMUNICATIONTH_H_

#include "../global.h"
#include "../common/utils/Utils.h"
#include "../common/utils/Trace.h"
#include "../common/utils/TimeFuncs.h"
#include "../common/serial/serial.h"
#include "../common/prot6/hprot.h"
#include "../common/prot6/hprot_stdcmd.h"
#include "../RXHFrame.h"
#include "CommTh.h"
#include "app_commands.h"

/*
 * to communicate without comm port, use domain socket
 * https://github.com/troydhanson/network/tree/master/unixdomain
 */

#define LOG_BUFFER_RAW				(512)
#define LOG_TRIGGER_WRITE			(LOG_BUFFER_RAW-10)
//-----------------------------------------------------------------------------
// MACROS
//-----------------------------------------------------------------------------


/*
 * Plugin communication thread.
 * this receiva data and update the status
 */
class SerCommTh : public CommTh
{
public:
	SerCommTh();
	virtual ~SerCommTh();

	string name;

	bool impl_OpenComm(const string dev,int param,void* p);
	void impl_CloseComm(void* p);
	bool impl_Write(uint8_t* buff,int size,hprot_idtype_t dstId,void* p);
	int impl_Read(uint8_t* buff,int size,void* p);
	void impl_setNewKey(uint8_t *key);
	void impl_enableCrypto(bool enable);
	protocolData_t* impl_getProtocolData(char dir,hprot_idtype_t id=HPROT_INVALID_ID,int cl=-1);
	void impl_resetToken(int id) {TRACEF(dbg,DBG_DEBUG,"not yet implemented");}

	void clearLinkCheck() {linkChecked=false;};

	// should be mutexed?
	bool getLinkCheckedStatus() {return linkChecked;}

private:
	int ser_fd;	// serial fd
	bool linkChecked;
	protocolData_t ser_pd;

	virtual void onFrame(frame_t &f);
//	enum status_e
//	{
//		st_idle,
//		st_ansPending,
//		st_error,
//	}status;
};

//-----------------------------------------------
#endif /* SERCOMMUNICATIONTH_H_ */
