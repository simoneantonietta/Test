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

#ifndef SKTSRVCOMMUNICATIONTH_H_
#define SKTSRVCOMMUNICATIONTH_H_

#include "../common/utils/Utils.h"
#include "../common/utils/Trace.h"
#include "../common/utils/TimeFuncs.h"
#include "../global.h"
#include "../common/prot6/hprot.h"
#include "../common/prot6/hprot_stdcmd.h"
#include "../RXHFrame.h"
#include "CommTh.h"
#include "SocketServer.h"

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
class SktSrvCommTh : public SocketServer, public CommTh
{
public:
	SktSrvCommTh();
	virtual ~SktSrvCommTh();

	string name;

	bool impl_OpenComm(const string dev,int param,void* p);
	void impl_CloseComm(void* p);
	bool impl_Write(uint8_t* buff,int size,hprot_idtype_t dstId,void* p);
	int impl_Read(uint8_t* buff,int size,void* p);
	void impl_setNewKey(uint8_t *key);
	void impl_enableCrypto(bool enable);
	protocolData_t* impl_getProtocolData(char dir,hprot_idtype_t id=HPROT_INVALID_ID,int cl=-1);

	void clearLinkCheck() {linkChecked=false;};
	// should be mutexed?
	bool getLinkCheckedStatus() {return linkChecked;}

private:
	struct timeval tval;
	bool linkChecked;

	struct Client_st
	{
		bool sktConnected;
		hprot_idtype_t id;
		protocolData_t protocolData;
	} clients[SKSRV_MAX_CLIENTS];
	int nClients;
	int actualClient;
	int _nDataAvail;

	int getClientFromID(hprot_idtype_t id);
	hprot_idtype_t getIDFromClient(int client);

	virtual void onDataReceived(int client);
	virtual void onCloseConnection(int client);
	virtual void onNewConnection(int client);
	virtual void onFrame(frame_t &f);

};

//-----------------------------------------------
#endif /* SKTSRVCOMMUNICATIONTH_H_ */
