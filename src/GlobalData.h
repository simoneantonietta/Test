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
	char sc8rDeviceCal[80];	// calibration device
	char sc8rDeviceSen[80];	// sensitivity device
	int brate;
	uint8_t myID;
	bool useSerial;
	bool useSC8Rsen,useSC8Rcal;
	bool forceSend2Unknown;

	// test parameters
	char codeID[5];	// device id (4 char) as asciiz string
	uint8_t codeIDexp[4]; // device id expanded 4 bytes

	uint8_t codeIDDefaultExp[4]; // default ID expanded

	int parm_sensThreshold;
	int parm_txCurrentMin,parm_txCurrentMax;

	uint8_t DUT_params[8];	// paramters programmed in the DUT

	// test results
	uint32_t res_freq;
	uint32_t res_delta;
	uint32_t res_bandwidth;
	uint8_t res_sensRSSI;
	uint8_t res_sensDev;
	uint8_t res_ReadedVBat;

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
