/**----------------------------------------------------------------------------
 * PROJECT: Communication
 * PURPOSE:
 * class used for Hprotocol the communication
 *-----------------------------------------------------------------------------  
 * CREATION: 10 Feb 2016
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

#ifndef COMM_COMMTH_H_
#define COMM_COMMTH_H_

//#include "../global.h"
#include <map>
#include "../common/prot6/hprot.h"
#include "../common/prot6/hprot_stdcmd.h"
#include "../common/utils/TimeFuncs.h"

// used to permit the gpsnif usage remotely
#ifdef COMM_TH_UDP_DEBUG
#include "../UDPProtocolDebug.h"
#endif

//----------------------------------------------
#ifdef USE_MULTICHANNEL_PROTOCOL_MSG_QUEUE
// protocol message queue
typedef struct frameData_st
{
	enum channel_e {chan_unk,chan_serial,chan_ethernet} channel;
	frame_t frame;
	uint8_t frameData[HPROT_PAYLOAD_SIZE];
	int client;

	frameData_st()
	{
	channel=chan_unk;
	client=-1;
	frame.data=frameData;
	}
} frameData_t;
#endif
//----------------------------------------------

#ifndef RESULT_STRUCT
/*
 * generic result class
 */
struct Result
{
public:
#define MAX_MESSAGE_SIZE		256
	enum status_e
	{
		is_unknown,
		is_waiting,
		is_ok,
		is_error
	} status;

	int value;
	//std::string message;
	char message[MAX_MESSAGE_SIZE];

	void reset() {status=is_unknown;value=0;message[0]=0;}
	void setStatus(status_e st) {status=st;}
	status_e getStatus() {return status;}

	void setError(int val=0) {status=is_error;value=val;}
	void setError(const char* msg,int val=0)
		{
		status=is_error;
		strcpy(message,msg);
		value=val;
		}
	void setOK(const char *msg,int val=0)
		{
		status=is_ok;
		strcpy(message,msg);
		value=val;
		}
	void setOK(int val=0) {status=is_ok;value=val;}
	void setWaiting(int val=0) {status=is_waiting;value=val;}

	bool isError() {return (status==is_error) ? (true) : (false);}
	bool isOK() {return (status==is_ok) ? (true) : (false);}
	bool isUnknown() {return (status==is_unknown) ? (true) : (false);}
	bool isWaiting() {return (status==is_waiting) ? (true) : (false);}
};
#endif


/*
 * to communicate without comm port, use domain socket
 * https://github.com/troydhanson/network/tree/master/unixdomain
 */

#define LOG_BUFFER_RAW				(512)
#define LOG_TRIGGER_WRITE			(LOG_BUFFER_RAW-10)

#define COMM_STATS_GLOBAL_CRC_ERROR				0
#define COMM_STATS_GLOBAL_CMD_ERROR				1
#define COMM_STATS_DEV_CRC_ERROR					2
#define COMM_STATS_DEV_CMD_ERROR					3
#define COMM_STATS_GLOBAL_CONNECTION			4
#define COMM_STATS_GLOBAL_DISCONNECTION		5
//=============================================================================

class CommTh
{
public:
	Result rxResult,txResult;

	// serial communication routine
	bool openCommunication(const string dev,int baud, void* p=NULL);
	void closeCommunication(void* p=NULL);

	void sendCommand(hprot_idtype_t dstId, uint8_t cmd, uint8_t *data, uint8_t len,void *p=NULL);
	void sendAnswer(hprot_idtype_t dstId, uint8_t cmd, uint8_t *data, uint8_t len, int fnumb, void *p=NULL);
	virtual void sendRequest(hprot_idtype_t dstId, uint8_t cmd, uint8_t *data, uint8_t len,bool setAnsPending,void *p=NULL);
	void sendFrame(uint8_t hdr, hprot_idtype_t srcId, hprot_idtype_t dstId, uint8_t cmd, uint8_t *data, uint8_t len,void *p=NULL);

#ifdef HPROT_USE_CRYPTO
	void setNewKey(uint8_t *key) {impl_setNewKey(key);}
	void enableCrypto(bool enable) {impl_enableCrypto(enable);}
#endif

#ifdef HPROT_TOKENIZED_MESSAGES
	void resetToken(int id) {impl_resetToken(id);}
#endif

	void setProtocolData(protocolData_t* pd_rx, protocolData_t* pd_tx) {pdrx=pd_rx;pdtx=pd_tx;}
	void setProtocolData(char dir,protocolData_t* pd)
	{
	if(dir=='b')
		{
		pdrxtx=pd;
		pdrx=pd;
		pdtx=pd;
		}
	if(dir=='t') pdtx=pd;
	if(dir=='r') pdrx=pd;
	}

	void setMyId(hprot_idtype_t id)
		{
		if(pdrxtx!=NULL) pdrxtx->myID=id;
		if(pdrx!=NULL) pdrx->myID=id;
		if(pdtx!=NULL) pdtx->myID=id;
		}

	/**
	 * set the timeout [ms]
	 * @param toValue
	 */
	void setTimeout(int toValue) {this->timeout=toValue;}
	/**
	 * get the timeout [ms]
	 * @return
	 */
	int getTimeout() {return this->timeout;}
	/**
	 * tell if is connected
	 * @return
	 */
	bool isConnected() {return this->connected;}

	// mutex protected
	void endApplicationRequest();
	bool endApplicationStatus();
	void resetApplicationRequest();


	void setCommAnsPending(hprot_idtype_t ansFromID, uint8_t frameNumber, uint8_t ansCmd=HPROT_CMD_NONE);
	/**
	 * tell who is the answerer
	 * @return
	 */
	uint8_t getAnswerer() {return answeredFrom;}

	void setResultStatus(Result::status_e res);
	Result::status_e getResultStatus();
	bool isWaiting();
	bool isOK();
	bool isError();
	string getErrorMessage();
#ifdef HPROT_ENABLE_RX_RAW_BUFFER
	void clearRawBuffer();
#endif
	/*
	 * This method will be executed by the Thread::exec method,
	 * which is executed in the thread of execution
	 */
	void runRx();
	void runTx();

	bool startThread(void *arg,char which);
	void joinThread(char which);

	// FIFO handling
	bool openFifo(string fifoName);
	int getFifoHandler() {return fifo;}
	void setFifoHandler(int fh) {fifo=fh;}	// use this if fifo is already opened in place of openFifo
	void closeFifo();
	void writeFifo(uint8_t *data,int size);
	void setUseFifo(bool enable) {useFifo=enable;}

#ifdef COMM_TH_UDP_DEBUG
	void setUDPDebugInterface(UDPProtocolDebug *ptr) {udpDbg=ptr;}
	void setUseUDPDebug(bool enable) {useUDPDebug=enable;}
	//bool startUDPDebug(string ip,int port);
#endif

	void setPacketProtocol(bool set) {packetProtocol=set;}
	bool isPacketProtocol() {return packetProtocol;}

	/*
	 * statistics
	 */
	struct stats_st
	{
		#define STATS_N_SPARES			5
		stats_st() : 	connections(0),
									disconnections(0),
									error_crc(0),
									error_cmd(0),
									ans_invalid_id(0)
		{
		for(int i=0; i<STATS_N_SPARES; i++) spare_stats[i]=0;
		};

		int connections;
		int disconnections;
		int error_crc;
		int error_cmd;
		int ans_invalid_id;
		int spare_stats[STATS_N_SPARES];	// others for spare usage

		typedef struct dev_comm_errors_st
		{
			dev_comm_errors_st() : crc(0), cmd(0) {}
			unsigned int crc;
			unsigned int cmd;
		} dev_comm_errors_t;
		map<int,dev_comm_errors_t> devErrors;
		typedef map<int,dev_comm_errors_t>::iterator devCommError_it;
	} stats;

	/**
	 * clear statistics
	 */
	void clearStats()
	{
	stats.connections=0;
	stats.disconnections=0;
	stats.error_crc=0;
	stats.error_cmd=0;
	stats.ans_invalid_id=0;

	for(int i=0; i<STATS_N_SPARES; i++) stats.spare_stats[i]=0;

	for(stats_st::devCommError_it it = stats.devErrors.begin();it != stats.devErrors.end();it++)
		{
		it->second.cmd=0;
		it->second.crc=0;
		}
	}

	/**
	 * get statistics data
	 * @param what
	 * @param id device
	 * @return statistic data
	 */
	unsigned int getStats(int what, int id=-1)
	{
	unsigned int val=0;
	pthread_mutex_lock(&mutexStats);
	switch(what)
		{
		case COMM_STATS_GLOBAL_CRC_ERROR:
			val=stats.error_crc;
			break;
		case COMM_STATS_GLOBAL_CMD_ERROR:
			val=stats.error_cmd;
			break;
		case COMM_STATS_DEV_CRC_ERROR:
			if(id<0) break;
			val=stats.devErrors[id].crc;
			break;
		case COMM_STATS_DEV_CMD_ERROR:
			if(id<0) break;
			val=stats.devErrors[id].cmd;
			break;
		case COMM_STATS_GLOBAL_CONNECTION:
			val=stats.connections;
			break;
		case COMM_STATS_GLOBAL_DISCONNECTION:
			val=stats.disconnections;
			break;
		}
	pthread_mutex_unlock(&mutexStats);
	return val;
	}

protected:
	// avoid copy
	CommTh();
	virtual ~CommTh();

	//-----------------------------------------------------------------------------
	// VIRTUAL low level functions
	//-----------------------------------------------------------------------------
	virtual bool impl_OpenComm(const string dev,int param,void* p=NULL)=0;
	virtual void impl_CloseComm(void* p=NULL)=0;
	virtual bool impl_Write(uint8_t* buff,int size,hprot_idtype_t dstId,void* p=NULL)=0;
	virtual int impl_Read(uint8_t* buff,int size,void* p=NULL)=0;
	// dir can be 't' for tx; 'r' for rx; 'b' for bidirectional
	virtual protocolData_t* impl_getProtocolData(char dir,hprot_idtype_t id=HPROT_INVALID_ID,int client=-1)=0;	// to get the current pd

#ifdef HPROT_USE_CRYPTO
	virtual void impl_setNewKey(uint8_t *key)=0;
	virtual void impl_enableCrypto(bool enable)=0;
#endif

#ifdef HPROT_TOKENIZED_MESSAGES
	virtual void impl_resetToken(int id)=0;
#endif

	virtual void waitForData(void* p=NULL); // here there is a basic implementation

	virtual void onDataRaw(protocolData_t *_pd,uint8_t* b,int size) {};
	virtual void onFrame(frame_t &f) {};
	virtual void onError(frame_t &f, rxParserCondition_t e) {};

  virtual void printRx(uint8_t *buff,int size,rxParserCondition_t fr_error=pc_idle);
  virtual void printTx(uint8_t *buff,int size);

	//-----------------------------------------------------------------------------
	protocolData_t *pdtx,*pdrx,*pdrxtx;
	rxParserCondition_t prevProtCondition;	// rprevious result of the parser, used to detect changes
	bool connected;
	bool packetProtocol;

	uint8_t rxBuffer[HPROT_BUFFER_SIZE];
	uint8_t txBuffer[HPROT_BUFFER_SIZE];
	uint8_t frameData[HPROT_BUFFER_SIZE];	// used in the rx thread
	int rxndata,txndata,initial_ndata;

	string fifoName;
	bool useFifo,useUDPDebug;
	int fifo;

	hprot_idtype_t ansPendingFrom;	// specifies the ID that should answer
	uint8_t ansFrameNumb;			// retain the frame number of the request
	hprot_idtype_t answeredFrom;		// this contains 0 till no answer from the requester, and contains ID of the device that answer if any
	uint8_t ansCommand;				// the exepcted command as answer

	unsigned int timeout;
	TimeFuncs to;


	enum status_e
	{
		st_idle,
		st_ansPending,
		st_ansPending_generic,
		st_error,
	}status;

	void setStatus(status_e st) {status=st;}

	bool endApplication;

	//-----------------------------------------------------------------------------
	// THREADS
	//-----------------------------------------------------------------------------
	pthread_t _idRx,_idTx;
	pthread_attr_t _attrRx,_attrTx;
	pthread_mutex_t mutexRx,mutexTx;
	pthread_mutex_t mutexStats;

	bool startedRx,startedTx;
	bool noattrRx,noattrTx;
	void *argRx, *argTx;

	bool createThread(bool detach,int size,char which);
	void destroyThread(char which);

	static void *execRx(void *thr);
	static void *execTx(void *thr);

private:
#ifdef COMM_TH_UDP_DEBUG
	 UDPProtocolDebug *udpDbg;
#endif
};


//-----------------------------------------------
#endif /* COMM_COMMTH_H_ */
