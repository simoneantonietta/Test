/**----------------------------------------------------------------------------
 * PROJECT: varcosv
 * PURPOSE:
 * 
 *-----------------------------------------------------------------------------  
 * CREATION: 17 Mar 2016
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

#ifndef COMM_SLAVE_H_
#define COMM_SLAVE_H_

#include "global.h"
#include "Script.h"
#include "common/utils/TimeFuncs.h"

#define MSG_QUEUE_THRESHOLD							10
#define MSG_QUEUE_THRESHOLD_ALERT				50
#define MSG_QUEUE_THRESHOLD_POLLING			40
#define	MSG_QUEUE_MULTIPLIER						2
#define MSG_QUEUE_MIN										3
#define MSG_QUEUE_CONGESTION_MAX				10
#define MSG_QUEUE_COUNTER_MAX						50
#define MSG_QUEUE_DELAY_CNT							100
#define MSG_QUEUE_TRIGGER_DELETE				200


#define WKR_TIMEOUT_TERMINALS						(40) 	// [s]
#define WKR_GLOBAL_TIMEOUT_INC					(10)	// added to the timeout of each terminal
#define WKR_REQUEST_TIMEOUT							(5)		// [s]
#define WKR_NEWKEY_GLOBAL_TIMEOUT				10
#define WKR_N_RETRY											3

#define WKR_MAX_TERMINALDATA_PER_FRAME	(HPROT_PAYLOAD_SIZE / sizeof(Terminal_parameters_to_send_t))


class Slave : public Script
{
public:
	TimeFuncs webCmdTimeout;		// web timeout

	Slave();
	virtual ~Slave();

	void mainLoop();
	void firmwareLoadData();

	void pushEvent(globals_st::dataEvent_t ev, int dest);
	void clearEvent(int dest);
	bool checkIfEventExists(int id, globals_st::dataEvent_t::dataEventType_e et);
	void recoveryCheck();

	void putDBEvent(EventCode_t evcode, int id);

private:
	bool webCommandInProgress;
	unsigned int msgs2work;

	unsigned int msgQueueDepth_prev;		// used to avoid to print message too frequently
	Badge_parameters_t tmpBPar; // temporary used fo various operations

	unsigned int fwCheckCounter,scriptCounter,oneHourCounter,oneSecondCounter,oneMinCounter,halfSecondCounter,dbSaveCounter;
	globals_st::frameData_t f;
	uint8_t txData[HPROT_BUFFER_SIZE];

	bool masterListCheck();
	void handleInternalEvents();
	void profileCheck();
	void weektimeCheck();
	void handleVarcoEvent();
	void handleRequestArea();
	void handleMasterDump();
	void handleBadgeReader();
	void handleScheduledJobs();

	/*
	 * container for all devices firmware data
	 */
	typedef struct fwData_st
	{
		//uint8_t type;
		string name;
		FWMetadata_t fwMetadata;
		string firmwareBin;
		string firmwareTxt;

		bool operator==(const uint8_t& type) const
			{
			return (this->fwMetadata.fw_type==type);
			}
		bool operator==(const string& name) const
			{
			return (this->name==name);
			}
	} fwData_t;
	typedef vector<fwData_t>::iterator fwit_t;
	vector<fwData_t> firmwares;

	uint8_t actualType;	// used to optimise chunk send
	fwit_t actual_it;		// used to optimise chunk send

	void worker_normal();
	void worker_terminalReq(int id);
	void worker_newKey(int id);

	globals_st::dataEvent_t firmwareCheckForNew();
	bool firmwareUnpack(uint8_t type, string pkgfname="");
	bool firmwareCheckMD5(uint8_t type);
	bool firmwareCheckVersion(uint8_t type, uint8_t major, uint8_t minor);
	bool firmwareChunkSend(uint8_t type, uint32_t startByte,uint8_t amount);

	bool checkGlobalStatus(bool checkExpected=true);
	void resetGlobalStatus();
	void resetDeviceStatus(int id=-1);
	void setAllDeviceStatus(enum globals_st::slaveStates_e mainstate=globals_st::sl_idle,enum globals_st::workerStates_e workerstate=globals_st::wkr_idle);

	void handleTimedRequests();

	void updateAllAliveCounters();
	void resetAliveCounter(int id, uint16_t aliveValue);
	bool checkAliveCounter(int id, bool event=true);

	void pollingTimeModulator(size_t queueSize);

	time_t getSystemTime();
};

//-----------------------------------------------
#endif /* COMM_SLAVE_H_ */
