/**----------------------------------------------------------------------------
 * PROJECT: acqsrv
 * PURPOSE:
 * Communication protocol fotr SC8R used for identification and calibration
 *-----------------------------------------------------------------------------  
 * CREATION: Mar 25, 2015
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

#ifndef COMMSC8R_H_
#define COMMSC8R_H_

#include <string>

#include "comm/serial/serial.h"

#define SC8R_MSG_MAX_SIZE						2048

#define SC8R_DEF_TIMEOUT						40		// mult of 100ms
#define SC8R_CAL_TIMEOUT						80		// mult of 100ms

#define SC8R_ADDR										0xFF
// some commands
#define SC8R_CMD_TESTMODE						0x8E
#define SC8R_CMD_TESTMODE_PERIF			0x82
#define SC8R_CMD_SETID							0x84
#define SC8R_CMD_CALIBRATION				0xF0
#define SC8R_CMD_STATUS_PERIF				0x88
#define SC8R_CMD_WRITE_PARAMS				0x86
#define SC8R_CMD_SET_TYPE						0x90

using namespace std;

class CommSC8R
{
public:
	CommSC8R();
	virtual ~CommSC8R();

	// serial communication routine
	bool openCommunication(const string dev,int baud);
	void closeCommunication();

	void sendMsg(uint8_t type, uint8_t addr, uint8_t *snPerif, uint8_t *data, int len);
	int readMsg(int to);
	void setTimeout(int to);
	uint8_t *getRxData() {return rxBuff;}
	bool isConnected() {return connected;}

private:
	int ser_fd;	// serial fd
	bool connected;
	uint8_t *txBuff;
	uint8_t *rxBuff;

	// stuffing byte
	uint8_t stuffb[2];
	int prepareMsg(uint8_t* buff, uint8_t type, uint8_t addr, uint8_t *snPerif, uint8_t *data, int len);
	void putByte(uint8_t *dst, int &ndx,uint8_t val);
	uint8_t getByte(uint8_t *src, int &ndx);
	void printRx(char* buff, int size);
	void printTx(char* buff, int size);
};

//-----------------------------------------------
#endif /* COMMSC8R_H_ */
