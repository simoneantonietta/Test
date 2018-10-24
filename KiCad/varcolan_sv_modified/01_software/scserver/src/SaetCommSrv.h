/**----------------------------------------------------------------------------
 * PROJECT: scserver
 * PURPOSE:
 * 
 *-----------------------------------------------------------------------------  
 * CREATION: Dec 12, 2016
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

#ifndef SAETCOMMSRV_H_
#define SAETCOMMSRV_H_

#include "Global.h"
#include "UdpServer.h"
#include "aes.h"
#include <netinet/in.h>
#include "common/utils/TimeFuncs.h"

#define SC_MSG_PAYLOAD				256

#define SC_KEY_SIZE						24

#define SC_MSG_IS_EVENT				0
#define SC_MSG_IS_COMMAND			1
#define SC_MSG_IS_CMD_EVT			0
#define SC_MSG_IS_CONNECT			1
#define SC_MSG_IS_ACK					1
#define SC_MSG_IS_KEEPALIVE		1
#define SC_MSG_IS_RETRY				1

#define SC_STATUS_FLD_TYPE				7
#define SC_STATUS_FLD_ACK					6
#define SC_STATUS_FLD_KEEPALIVE		5
#define SC_STATUS_FLD_SPARE4			4
#define SC_STATUS_FLD_DATATYPE		3
#define SC_STATUS_FLD_SPARE2			2
#define SC_STATUS_FLD_RESERVED		1
#define SC_STATUS_FLD_RETRYFLAG		0

#define SC_CONN_PHASE1						1
#define SC_CONN_PHASE2						2
#define SC_CONN_PHASE3						3
#define SC_CONN_PHASE4						4

#define SC_CONN_REQREG						6
#define SC_CONN_CHALLENGE					7
#define SC_CONN_REGISTER					8

// event fields
#define SC_EVT_FIXED_ID						0
#define SC_EVT_TYPE								1
#define SC_EVT_CODE								2
#define SC_EVT_DATA								3

// command fields
#define SC_CMD_TYPE								0
#define SC_CMD_CODE								1
#define SC_CMD_DATA								2


typedef union SC_Status_st
{
	struct
	{
	uint8_t retryFlag				:1;	// lsb
	uint8_t _reserved				:1;
	uint8_t _spare2					:1;
	uint8_t dataType				:1;
	uint8_t _spare4					:1;
	uint8_t keepAlive				:1;
	uint8_t ack							:1;
	uint8_t ev_cmd					:1;	// msb
	};
	uint8_t bStatus;
} __attribute__ ((packed)) SC_Status_t;

typedef struct
{
	uint16_t msgid;
	SC_Status_t status;
	uint16_t plant;
	uint8_t node;				// not used
	uint32_t datetime;	// secs from 1/1/1990
	uint8_t len;				// all message len (header included)
	uint8_t ext;				// not used (set to 0)
	uint8_t data[SC_MSG_PAYLOAD];		// data
}__attribute__ ((packed)) SC_datagram_t;

enum datagramType_e
{
	dgtype_event,
	dgtype_command,
	dgtype_ack,
	dgtype_keepalive,
	dgtype_connection
};

#define SC_MSG_OVERHEAD				(sizeof(SC_datagram_t)-SC_MSG_PAYLOAD)
#define SC_MSG_BUFFER_SIZE		(sizeof(SC_datagram_t))


/*
 *
 */
class SaetCommSrv
{
public:
	SaetCommSrv();
	virtual ~SaetCommSrv();

	bool initComm(const std::string& addr, int port, int plantID);
	bool isWaitingAnswer() {return expectedAck;}
	void setExpectedAns(bool expectedAck,SC_datagram_t *dg=NULL);

	/*
	 * This method will be executed by the Thread::exec method,
	 * which is executed in the thread of execution
	 */
	void runRx();
	void runTx();
	bool startThread(void *arg,char which);
	void joinThread(char which);

private:
	UdpServer *udpsrv;

	uint16_t msgid_out,msgid_in;

	uint8_t challenge[4];	// temporarily store the challenge to check if it match
	uint8_t tmpbuff[SC_MSG_PAYLOAD];	// used to build data to send
	uint8_t internal_buf[SC_MSG_BUFFER_SIZE];
	bool needRealign;

	bool needRetry;
	uint8_t aeskey[SC_KEY_SIZE];
	struct aes_ctx ctx;
	bool useCrypto;

	uint8_t rxBuf[sizeof(SC_datagram_t)]; /* message buf (always in plain mode) */
	uint8_t txBuff[sizeof(SC_datagram_t)];	// temporary buffer to crypt messages

	/*
	 * used to handle communication
	 */
	SC_datagram_t lastTxDatagram;
	bool expectedAck;
	TimeFuncs connTimeout;	// handle the connection timeout
	TimeFuncs ansTimeout;		// handle the answer timeout

	bool preCheckDatagram(uint8_t *dg, int sz);
	bool sendDatagram(SC_datagram_t *dg);

	void buildDatagram(datagramType_e type,uint8_t len, uint8_t *data, SC_datagram_t *dg,SC_datagram_t *dg_req=NULL);
	void buildAck(SC_datagram_t *dg, bool noData=true);
	void buildEvent(uint8_t code, uint8_t *data, int sz, SC_datagram_t *dg);

	bool datagramDecrypt(struct aes_ctx *ctx, uint8_t *data, int sz, uint8_t *out=NULL);
	int datagramEncrypt(struct aes_ctx *ctx, uint8_t *data, int sz, uint8_t *out=NULL);

	void printRXTX(char dir,uint8_t *dat);

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

};

//-----------------------------------------------
#endif /* SAETCOMMSRV_H_ */
