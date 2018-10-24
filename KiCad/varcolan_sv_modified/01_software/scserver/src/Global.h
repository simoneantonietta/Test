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

	bool run;

	typedef struct client_st
	{
		string addr;
		int port;
		int plantId;
	} client_t;

	client_t client;
};


//-----------------------------------------------------------------------------
// VARIABLES
//-----------------------------------------------------------------------------
extern Trace *dbg;
extern SimpleCfgFile *cfg;
extern Global gd;

//-----------------------------------------------
#endif /* GLOBAL_H_ */
