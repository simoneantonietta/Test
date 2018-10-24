/**----------------------------------------------------------------------------
 * PROJECT: rasp_varcosv
 * PURPOSE:
 * This interface is used to connect the supervisor to the cloud.
 * this represents the SVM interface (SuperVisor Master) that communicates
 * with the cloud relative one called SVC (SuperVisor Cloud)
 *-----------------------------------------------------------------------------  
 * CREATION: Feb 13, 2017
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

#ifndef MASTERINTERFACE_H_
#define MASTERINTERFACE_H_

#include "global.h"
#include <vector>
#include <algorithm>
#include <netinet/in.h>
#include "common/utils/TimeFuncs.h"
#include "common/utils/Utils.h"
#include "dataStructs.h"
#include "comm/app_commands.h"
#include "comm/UDPClient.h"

//#include "UdpServer.h"

#define SVM_MSG_BUFFER_SIZE 					HPROT_BUFFER_SIZE
#define SVM_CONN_ATTEMPT_WAIT_TIME		5000		// [ms]
#define SVM_CONN_KEEP_ALIVE_TIME			30000		// [ms]
/*
 *
 */
class MasterInterface
{
public:
	MasterInterface();
	virtual ~MasterInterface();

	bool initComm(const std::string& addr, int port, int plantID);
	bool mainLoop();

	/*
	 * This method will be executed by the Thread::exec method,
	 * which is executed in the thread of execution
	 */
	void runRx();
	void runTx();
	bool startThread(void *arg, char which);
	void joinThread(char which);

private:
	typedef enum ret_status_e
	{
		ret_ok,
		ret_inprogress,
		ret_error
	} retStatus_t;

	//UdpServer *udpsrv;
	UDPClient *commif;
	protocolData_t mstr_pd;

	frame_t rxFrame;
	uint8_t rxFrame_payload[HPROT_PAYLOAD_SIZE];

	bool opened, connected;
	uint8_t internal_buf[SVM_MSG_BUFFER_SIZE];

	enum
	{
		mi_init,
		mi_connect,
		mi_conn_wait,
		mi_sync_events,
		mi_sync_badge,
		mi_sync_anagraphics,
		mi_sync_users,
		mi_idle,
		mi_send_polling,
		mi_send_first_part_prof_list,
		mi_send_second_part_prof_list,
		mi_send_wt_list,
	} miStatus;

	TimeFuncs tim_wait;
	TimeFuncs tim_keepalive;

	enum mi_Faults_e
	{
		mi_error_timeout,
		mi_error_noSync,
	} miFaults;

	uint8_t dataBufferTx[HPROT_BUFFER_SIZE];

	time_t ev_time_bookmark;		// used to handle event synchronisation

	void printRXTX(char dir, uint8_t *dat, int len);

#if 0
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
#endif
};

//-----------------------------------------------
#endif /* MASTERINTERFACE_H_ */
