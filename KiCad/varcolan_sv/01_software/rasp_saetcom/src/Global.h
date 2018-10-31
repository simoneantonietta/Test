/**----------------------------------------------------------------------------
 * PROJECT: rasp_saetcom
 * PURPOSE:
 * Global defines, data, classes and typedefs
 *-----------------------------------------------------------------------------  
 * CREATION: Oct 24, 2016
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

#ifndef GLOBAL_H_
#define GLOBAL_H_

#include <string>
#include <vector>
#include "datastruct.h"
#include "common/utils/Trace.h"
#include "common/utils/SimpleCfgFile.h"

using namespace std;



#define MAX_MESSAGE_TRACES 	20
#define DBG_ERROR 					TRACECLASS_ERROR
#define DBG_WARNING					TRACECLASS_WARN
#define DBG_FATAL 					TRACECLASS_FATAL
#define DBG_NOTIFY 					TRACECLASS_NOTIFY
#define DBG_DEBUG						TRACECLASS_DEBUG
#define DBG_SKTCLIENT				TRACECLASS_CUSTOM_MIN+0
#define DBG_SKTCLIENT_DATA	TRACECLASS_CUSTOM_MIN+1


#define UDP_PORT_SV						4006
#define UDP_PORT_GEMSS				4006

#define SC_MY_TYPE						1

#define TELNET_SV_ADDRESS			"localhost"
#define TELNET_SV_PORT				6000
#define TELNET_SV_ANS_TIMEOUT	10		// 100 ms units

#define GEMSS_ANS_TIMEOUT			2 	// [s]

//-----------------------------------------------------------------------------
// global class data container
//-----------------------------------------------------------------------------
class Global
{
public:
	Global();
	virtual ~Global();

	string eth_interface;

	int plantID;
	int myPort;

	unsigned int connectionExpireTime;	// [s] connection timeout

	struct Server_st
	{
		string reg_server_ip;
		int reg_server_port;
		//int gemss_port;

		string ip;
		unsigned int addr_ip;
		int port;
	} gemss;

	//queue<SC_datagram_t> scQueue;

};


//-----------------------------------------------------------------------------
// VARIABLES
//-----------------------------------------------------------------------------
extern Trace *dbg;
extern SimpleCfgFile *cfg;
extern Global gd;
//extern SaetComm *sc;
//-----------------------------------------------
#endif /* GLOBAL_H_ */
