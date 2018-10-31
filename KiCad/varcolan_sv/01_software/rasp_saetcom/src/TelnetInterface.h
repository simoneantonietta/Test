/**----------------------------------------------------------------------------
 * PROJECT: rasp_saetcom
 * PURPOSE:
 * 
 *-----------------------------------------------------------------------------  
 * CREATION: Nov 29, 2016
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

#ifndef TELNETINTERFACE_H_
#define TELNETINTERFACE_H_

#include "Global.h"
#include "SimpleClient.h"

/*
 *
 */
class TelnetInterface: public SimpleClient
{
public:
	TelnetInterface();
	virtual ~TelnetInterface();

	int hasRXData();
	int getRXData(char* data);
	void sendData(const char* data, int count);
	bool waitAnswer(int timeCounter);
	/*
	 * This method will be executed by the Thread::exec method,
	 * which is executed in the thread of execution
	 */
	void runRx();
	void runTx();
	bool startThread(void *arg,char which);
	void joinThread(char which);

private:
	vector<string> toks;
	int waitTimeCounter;

	int dataRXcount;
	char dataRXbuf[SIMPLECLIENT_RX_BUFF_SIZE];

	void writeData(const char *data, int count);
	void onDataRead(char *data, int count);

	//-----------------------------------------------------------------------------
	// THREADS
	//-----------------------------------------------------------------------------
	pthread_t _idRx,_idTx;
	pthread_attr_t _attrRx,_attrTx;
	pthread_mutex_t mutexRx,mutexTx;

	bool startedRx,startedTx;
	bool noattrRx,noattrTx;
	void *argRx, *argTx;

	bool createThread(bool detach,int size,char which);
	void destroyThread(char which);

	static void *execRx(void *thr);
	static void *execTx(void *thr);

};

//-----------------------------------------------
#endif /* TELNETINTERFACE_H_ */
