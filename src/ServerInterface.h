/**----------------------------------------------------------------------------
 * PROJECT: woodbusd
 * PURPOSE:
 * 
 *-----------------------------------------------------------------------------  
 * CREATION: 7 Oct 2013
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

#ifndef SERVERINTERFACE_H_
#define SERVERINTERFACE_H_

#include "SocketServer.h"
#include "utils/TimeFuncs.h"
#include "utils/Utils.h"
#include "utils/SerializeData.h"
#include "comm/prot6/hprot.h"
#include "comm/prot6/hprot_stdcmd.h"


#define SRVIF_INTERNAL_BUFFER 								1024
#define SRVIF_MESSAGE_SIZE										128
//-----------------------------------------------------------------------------
// MACROS
//-----------------------------------------------------------------------------


//=============================================================================
class ServerInterface: public SocketServer
{
public:
	ServerInterface();
	virtual ~ServerInterface();

	void sendCommand(uint8_t srcid,uint8_t dstid, uint8_t cmd, uint8_t *data, uint8_t len);
	void sendRequest(uint8_t srcid,uint8_t dstid, uint8_t cmd,uint8_t *data, uint8_t len);
	void sendAnswer(uint8_t srcid,uint8_t dstid, uint8_t cmd,uint8_t *data, uint8_t len);
	void sendFrame(uint8_t hdr,uint8_t srcid,uint8_t dstid, uint8_t cmd, uint8_t *data, uint8_t len, bool forceSend=false);

	bool isFrameSent() {return frameSent;}

private:
	bool frameSent;
	unsigned int srvif_timeout;
	int actualClient;
	SerializeData *sdata;
	TimeFuncs srvif_to;
	uint8_t txBuffer[SRVIF_INTERNAL_BUFFER];
	char message[SRVIF_MESSAGE_SIZE];	// contains the last message (mostly the error) that a client can request
	uint8_t frameData[HPROT_BUFFER_SIZE-HPROT_OVERHEAD_SIZE];


	int getClient(uint8_t id);
	uint8_t getId(int client);

	void onDataReceived(int client);
	void onCloseConnection(int client);
	void onNewConnection(int client);
	void onFrame(frame_t &f);
};

//-----------------------------------------------
#endif /* SERVERINTERFACE_H_ */
