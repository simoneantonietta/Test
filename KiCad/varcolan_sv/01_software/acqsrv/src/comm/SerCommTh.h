/**----------------------------------------------------------------------------
 * PROJECT: ifollow
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
#include "../utils/Utils.h"
#include "../utils/Trace.h"
#include "../utils/TimeFuncs.h"
#include "../wthread/WThread.h"
#include "../wthread/WLock.h"
#include "serial/serial.h"
#include "prot6/hprot.h"
#include "prot6/hprot_stdcmd.h"
#include "serial/serial.h"

//---- CALLBACKS DEFINES ----
#define CLICOMM_CALLBACK_COMMAND								0		// frame
#define CLICOMM_CALLBACK_REQUEST								1		// frame
#define CLICOMM_CALLBACK_EVENT									2		// frame
#define CLICOMM_CALLBACK_SERVICE								3		// frame
#define CLICOMM_CALLBACK_ANSWER									4		// frame
#define CLICOMM_CALLBACK_ACK										5		// frame
#define CLICOMM_CALLBACK_NACK										6		// frame
#define CLICOMM_CALLBACK_CTRLCHANGE							7		// get a pointer to the controller changed

#define CLICOMM_CALLBACK_SIZE										8
//---------------------------

//-----------------------------------------------------------------------------
// MACROS
//-----------------------------------------------------------------------------


/*
 * Plugin communication thread.
 * this receiva data and update the status
 */
class SerCommTh : public WThread
{
public:
	SerCommTh();
	virtual ~SerCommTh();

	// serial communication routine
	bool openCommunication(const string dev,int baud);
	void closeCommunication();

	void sendAck(int dest);
	void sendNack(int dest);

	void sendCommand(uint8_t dstId, uint8_t cmd, uint8_t size, uint8_t *data);
	void sendAnswer(uint8_t dstId, uint8_t cmd, uint8_t size, uint8_t *data);
	void sendRequest(uint8_t dstId, uint8_t cmd, uint8_t size, uint8_t *data, uint8_t nretry);
	void sendFrame(uint8_t hdr, uint8_t srcId, uint8_t dstId, uint8_t cmd, uint8_t size, uint8_t *data, int fnumber=HPROT_SET_FNUMBER_DEFAULT);
	void waitOperation();
	bool getResult();

	void setMyId(uint8_t id) 	{pd->myID=id;}

//	void sendCommand(int dest,int spec, int size, char *data);
//	void sendRequest(int dest,int spec, int size, char *data, bool setAnsPending=true);
//	void sendAnswer(int dest,int spec, int size, char *data);

	bool isEnableCallback() const {return enableCallback;}
	void setEnableCallback(bool enableCallback) {this->enableCallback = enableCallback;}

	bool isEnableEventReception() const {return enableEventReception;}
	void setEnableEventReception(bool enableEventReception) {this->enableEventReception = enableEventReception;}

	void setTimeout(int toValue) {this->timeout=toValue;}
	int getTimeout() {return this->timeout;}


	// mutex protected
	void endApplicationRequest();
	bool endApplicationStatus();

	void setCommAnsPending();


	void setResultStatus(Result::status_e res);
	Result::status_e getResultStatus();
	bool isWaiting();
	bool isOK();
	bool isError();
	string getErrorMessage();
	/*
	 * This method will be executed by the Thread::exec method,
	 * which is executed in the thread of execution
	 */
	void run();

	// EVENTS
	// this are virtual to permit eventually to inherit the class and events, but you prefer to use callbacks
	virtual void onCommand(frame_t &f) {};
	virtual void onRequest(frame_t &f) {};
	virtual void onEvent(frame_t &f) {};
	virtual void onAnswer(frame_t &f) {};

private:
	int ser_fd;	// serial fd

	//uint8_t *comm_buffer;	// use this to avoid runtime memory allocation
	protocolData_t *pd;
	rxParserCondition_t prevProtCondition;	// rprevious result of the parser, used to detect changes

	bool textMode;	// textmode not used!
	bool registered;
	bool connected;
	bool enableCallback;
	bool enableEventReception;
	int _ndata;
	uint8_t rxBuffer[HPROT_BUFFER_SIZE];
	uint8_t txBuffer[HPROT_BUFFER_SIZE];
	uint8_t ndata;
	uint8_t frameData[HPROT_BUFFER_SIZE-HPROT_OVERHEAD_SIZE];


	unsigned int timeout;
	TimeFuncs to;

	void onFrame(frame_t &f);

	void waitForData();


  void printRx(char *buff,int size);
  void printTx(char *buff,int size);

	enum status_e
	{
		st_idle,
		st_ansPending,
		st_error,
	}status;

	void setStatus(status_e st) {status=st;}

	WLock *_commth_mutex;
	bool endApplication;
	bool ansPending;
	Result result;
};

//-----------------------------------------------
#endif /* SERCOMMUNICATIONTH_H_ */
