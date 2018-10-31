/**----------------------------------------------------------------------------
 * PROJECT: acqsrv
 * PURPOSE:
 * 
 *-----------------------------------------------------------------------------  
 * CREATION: 19 Feb 2015
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

#ifndef GLOBALDATA_H_
#define GLOBALDATA_H_

#include "utils/Trace.h"
#include "SocketServer.h"


class GlobalData
{
public:
	GlobalData() {};
	virtual ~GlobalData() {};

	int serverPort;
	char portDevice[80];
	int brate;
	uint8_t myID;
	bool useSerial;
	bool forceSend2Unknown;

	struct Client_st
	{
		bool sktConnected;
		uint8_t id;
		protocolData_t protocolData;
	} clients[SKSRV_MAX_CLIENTS];
	int nClients;
};

//-----------------------------------------------
#endif /* GLOBALDATA_H_ */
