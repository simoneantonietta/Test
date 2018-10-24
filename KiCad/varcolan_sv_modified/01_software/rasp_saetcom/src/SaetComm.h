/**----------------------------------------------------------------------------
 * PROJECT: rasp_saetcom
 * PURPOSE:
 * 
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

#ifndef SAETCOMM_H_
#define SAETCOMM_H_

#include <queue>
#include <netinet/in.h>
#include "Global.h"
#include "common/utils/TimeFuncs.h"
#include "aes.h"

//-----------------------------------------------------------------------------
// Datagram specification
//-----------------------------------------------------------------------------
#define UDP_BUFSIZE								512
#define SC_MSG_PAYLOAD						256

#define SC_KEY_SIZE								24

#define SC_MSG_IS_EVENT						0
#define SC_MSG_IS_COMMAND					1
#define SC_MSG_IS_CMD_EVT					0
#define SC_MSG_IS_CONNECT					1
#define SC_MSG_IS_ACK							1
#define SC_MSG_IS_KEEPALIVE				1
#define SC_MSG_IS_RETRY						1

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

#define SC_EVT_FIXED_ID_VALUE			255
#define SC_MY_EVENT_TYPE					2

// command fields
#define SC_CMD_TYPE								0
#define SC_CMD_CODE								1
#define SC_CMD_DATA								2


#define SC_REQREG_CHALL_SIZE			16
#define SC_ETH_INTERFACE					"eth0"

//....................................
// recognized GEMSS commands
//....................................
#define SC_CMDGEMSS_SET_DATE			17
#define SC_CMDGEMSS_SET_TIME			19
//....................................


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
	dgtype_connection,
	dgtype_reqreg,
	dgtype_register,
};

#define SC_MSG_OVERHEAD			(sizeof(SC_datagram_t)-SC_MSG_PAYLOAD)
//=============================================================================

/*
 *
 */
class SaetComm
{
public:
	SaetComm();
	virtual ~SaetComm();

	bool openComm();
	bool getCredentials();
	bool openConnection();
	bool registrationRequest();

	bool checkConnectionTimeout();
	bool isWaitingAnswer() {return expectedAck;}
	void setExpectedAns(bool expectedAck,SC_datagram_t *dg=NULL);
	bool isGemmssConnected_cripto() {return (gemss_connected && useCrypto);};

	/*
	 * This method will be executed by the Thread::exec method,
	 * which is executed in the thread of execution
	 */
	void runRx();
	void runTx();
	bool startThread(void *arg,char which);
	void joinThread(char which);


	//queue<SC_datagram_t> datagramQueueTX;

private:
	uint16_t msgid_out,msgid_in;
	SC_datagram_t msg_rx;

	bool needRealign;
	bool needRetry;

	/*
	 * used to handle communication
	 */
	SC_datagram_t lastTxDatagram;
	bool expectedAck;

	int sockfd;
	int optval; /* flag value for setsockopt */
	struct sockaddr_in myaddr; /* server's addr */
	struct sockaddr_in clientaddr,gemssaddr; /* client addr */
	int clientlen; /* byte size of client's address */

	uint8_t internal_buf[UDP_BUFSIZE]; /* message buf (plain or crypted) */
	char *hostaddrp; /* dotted decimal host addr string */
	bool connected;
	bool gemss_connected;
	TimeFuncs connTimeout;	// handle the connection timeout
	TimeFuncs ansTimeout;		// handle the answer timeout

	uint8_t aeskey[SC_KEY_SIZE];
	struct aes_ctx ctx;
	bool useCrypto;
	uint8_t rxBuf[UDP_BUFSIZE]; /* message buf (always in plain mode) */
	uint8_t txBuff[sizeof(SC_datagram_t)];	// temporary buffer to crypt messages

	uint8_t challenge[4];	// temporarily store the challenge to check if it match
	uint8_t challenge_reqreg[SC_REQREG_CHALL_SIZE];	// temporarily store the challenge to check if it match
		uint8_t tmpbuff[SC_MSG_PAYLOAD];	// used to build data to send

	uint8_t MAChost[6], MACgw[6];

	bool datagramDecrypt(struct aes_ctx *ctx, uint8_t *data, int sz, uint8_t *out=NULL);
	int datagramEncrypt(struct aes_ctx *ctx, uint8_t *data, int sz, uint8_t *out=NULL);

	void buildDatagram(datagramType_e type,uint8_t len, uint8_t *data, SC_datagram_t *dg,SC_datagram_t *dg_req=NULL);
	void buildAck(SC_datagram_t *dg, bool noData=true);
	void buildEvent(uint8_t code, uint8_t *data, int sz, SC_datagram_t *dg);

	bool preCheckDatagram(uint8_t *dg, int sz);
	bool sendDatagram(SC_datagram_t *dg,bool forcePlain=false);

	bool exeCommand(SC_datagram_t *dg);
	bool eventConverter(SC_datagram_t *dg);

	void getNetworkData(string dev, string &mac, string &ip, string &msk, string &gw);
	void getMACAddress(const char *ethif);

	void printRXTX(char dir,uint8_t *dat);

	enum scStatus_e
	{
		scs_idle,

		scs_conn_challenge,

		scs_conn_phase1,
		scs_conn_phase1_ack,
		scs_conn_phase2,
		scs_conn_phase2_ack,

		scs_conn_crypt1,
		scs_conn_crypt1_ack,
		scs_conn_crypt2,
		scs_conn_crypt2_ack,

		scs_command,
		scs_ack,
		scs_event,
		scs_keepalive,
	} scStatus;
	void rxParseStateMachine(SC_datagram_t *dg);
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
#endif /* SAETCOMM_H_ */
