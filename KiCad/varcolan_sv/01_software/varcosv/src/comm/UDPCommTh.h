/**----------------------------------------------------------------------------
 * PROJECT: varcosv
 * PURPOSE:
 * UDP interface for communication with HPROT
 *-----------------------------------------------------------------------------  
 * CREATION: 12 May 2016
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

#ifndef COMM_UDPCOMMTH_H_
#define COMM_UDPCOMMTH_H_
#include "../global.h"
#include <netinet/in.h>
#include "../common/utils/Utils.h"
#include "../common/utils/Trace.h"
#include "../common/utils/TimeFuncs.h"
#include "../common/prot6/hprot.h"
#include "../common/prot6/hprot_stdcmd.h"
#include "../RXHFrame.h"
#include "CommTh.h"
#include "app_commands.h"

#define UDP_MAX_CLIENTS						70
#define UDP_BUFFER_SIZE						HPROT_BUFFER_SIZE

// internal UDP buffer
#define UDP_BUFSIZE								HPROT_BUFFER_SIZE + 5

class UDPCommTh: public CommTh
{
public:
	UDPCommTh(hprot_idtype_t myId);
	virtual ~UDPCommTh();

	string name;

	bool impl_OpenComm(const string dev, int param, void* p);
	void impl_CloseComm(void* p);
	bool impl_Write(uint8_t* buff, int size, hprot_idtype_t dstId, void* p);
	int impl_Read(uint8_t* buff, int size, void* p);
	void impl_setNewKey(uint8_t *key);
	void impl_enableCrypto(bool enable);
	protocolData_t* impl_getProtocolData(char dir, hprot_idtype_t id = HPROT_INVALID_ID, int cl = -1);
	void impl_resetToken(int id);  //{TRACEF(dbg,DBG_DEBUG,"not yet implemented");}

	void clearLinkCheck()
	{
	linkChecked = false;
	}
	;

	// should be mutexed?
	bool getLinkCheckedStatus()
	{
	return linkChecked;
	}

	int addClient(hprot_idtype_t id, struct sockaddr_in clientaddr, int clientlen, uint32_t ip);
	int getClientFromID(hprot_idtype_t id);
	int getClientFromIP(uint32_t ip);
	int getClientFromIP(string ip);
	uint32_t ip_string_2_uint32(string ip);
	string ip_uint32_2_string(uint32_t ip);

	void getLastRxFrame(globals_st::frameData_t *f);
	void getLastRxFrame(frame_t *f, uint8_t *payload);

	bool isUseQueue() const
	{
	return useQueue;
	}
	void setUseQueue(bool useQueue)
	{
	this->useQueue = useQueue;
	}

	//----------------------------------------
	struct ClientData
	{
		hprot_idtype_t id;										// protocol ID
		protocolData_t protocolData;
		char buffer[HPROT_BUFFER_SIZE];		// data buffer
		int nDataAvailable;

		// udp info
		union clientIP_u
		{
			uint32_t ip;								// used to indentify the datagram
			uint8_t ip_fld[4];					// as bytes
		} clientIP;
		struct sockaddr_in clientaddr; /* client addr */
		int clientlen; /* byte size of client's address */

		int error_crc, error_cmd;
	} clients[UDP_MAX_CLIENTS];

protected:
	//pthread_mutex_t mutexOnFrame;

	bool useQueue;
	int sockfd; /* socket */
	int clientlen; /* byte size of client's address */
	struct sockaddr_in serveraddr; /* server's addr */
	struct sockaddr_in clientaddr; /* client addr */
	struct hostent *hostp; /* client host info */
	char internal_buf[UDP_BUFSIZE]; /* message buf */
	char *hostaddrp; /* dotted decimal host addr string */
	int hostportp;				//port
	int optval; /* flag value for setsockopt */

	globals_st::frameData_t lastRxFrameData;
	frame_t lastRxFrame;
	uint8_t lastRxFrame_payload[HPROT_PAYLOAD_SIZE];

	bool linkChecked;

	virtual void onFrame(frame_t &f);
	virtual void onError(frame_t &f, rxParserCondition_t e);

#if 0
	// USED FOR DEBUG
	virtual void printRx(uint8_t *buff,int size,rxParserCondition_t fr_error=pc_idle)
		{
		char _tmp[HPROT_BUFFER_SIZE*2];
		Hex2AsciiHex(_tmp,buff,size,true,' ');
		dbg->trace(DBG_DEBUG,"RX data(%d): %s",size,_tmp);
		}

	virtual void printTx(uint8_t *buff,int size)
		{
		char _tmp[HPROT_BUFFER_SIZE*2];
		Hex2AsciiHex(_tmp,buff,size,true,' ');
		dbg->trace(DBG_DEBUG,"TX data(%d): %s",size,_tmp);
		}
#endif

	int last_client;
};

//-----------------------------------------------
#endif /* COMM_UDPCOMMTH_H_ */
