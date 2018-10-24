/*
 *-----------------------------------------------------------------------------
 * PROJECT: varcosv
 * PURPOSE: see module UDPCommTh.h file
 *-----------------------------------------------------------------------------
 */
#include <errno.h>
#include <unistd.h>   //close
#include <arpa/inet.h>    //close
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#include <sys/time.h> //FD_SET, FD_ISSET, FD_ZERO macros

#include "UDPCommTh.h"

extern Trace *dbg;
extern globals_st gd;

/**
 * ctor
 * @parm id of the source (mine)
 */
UDPCommTh::UDPCommTh(hprot_idtype_t myId)
{
name = "UDPInterface";
answeredFrom = 0;
//pthread_mutex_init(&mutexOnFrame, NULL);
endApplication = false;
setPacketProtocol(true);

for(int i = 0;i < UDP_MAX_CLIENTS;i++)
	{
	clients[i].clientIP.ip = 0;
	clients[i].id = HPROT_INVALID_ID;
	clients[i].nDataAvailable = 0;
	clients[i].protocolData.myID = myId;
	clients[i].error_cmd = 0;
	clients[i].error_crc = 0;
	hprotInit(&clients[i].protocolData, NULL, NULL, NULL, NULL);
	hprotSetRxRawPlain(&clients[i].protocolData, HPROT_RXRAWDATA_CRYPT);
#ifdef HPROT_USE_CRYPTO
	hprotSetKey(&clients[i].protocolData, gd.hprotKey);
#endif
	}
//hprotInit(&last_pd,NULL,NULL,NULL,NULL);
//hprotSetKey(&last_pd,gd.hprotKey);
//hprotSetRxRawPlain(&last_pd,HPROT_RXRAWDATA_CRYPT);
//pd=&last_pd;	// passed to CommTh
createThread(false, 0, 'r');

useQueue = true;
}

/**
 * dtor
 */
UDPCommTh::~UDPCommTh()
{
cout << "UDP communication end" << endl;
}

/**
 * open serial communication
 * @param dev
 * @param param
 * @param p
 * @return
 */
bool UDPCommTh::impl_OpenComm(const string dev, int param, void* p)
{
bool ret;
/*
 * socket: create the parent socket
 */
sockfd = socket(AF_INET, SOCK_DGRAM, 0);
if(sockfd < 0)
	{
	dbg->trace(DBG_ERROR, "opening UDP socket");
	ret = false;
	}
else
	{
	/* setsockopt: Handy debugging trick that lets
	 * us rerun the server immediately after we kill it;
	 * otherwise we have to wait about 20 secs.
	 * Eliminates "ERROR on binding: Address already in use" error.
	 */
	optval = 1;
	setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (const void *) &optval, sizeof(int));
	struct timeval tv;
	tv.tv_sec = 2;		// set timeout
	tv.tv_usec = 0;
	setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

	/*
	 * build the server's Internet address
	 */
	bzero((char *) &serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serveraddr.sin_port = htons((unsigned short) param);

	hostaddrp = inet_ntoa(serveraddr.sin_addr);
	/*
	 * bind: associate the parent socket with a port
	 */
	if(bind(sockfd, (struct sockaddr *) &serveraddr, sizeof(serveraddr)) < 0)
		{
		dbg->trace(DBG_ERROR, "on binding server addr %s @ port %d", hostaddrp, param);
		ret = false;
		}
	else
		{
		//dbg->trace(DBG_NOTIFY, "listening server addr %s @ port %d",hostaddrp,param);
		dbg->trace(DBG_NOTIFY, "listening UDP server at port %d", param);
		connected = true;
		clientlen = sizeof(clientaddr);
		ret = true;
		}
	}
return ret;
}

/**
 * close serial communication
 * @param p
 */
void UDPCommTh::impl_CloseComm(void* p)
{
dbg->trace(DBG_NOTIFY, "closing UDP socket");
close(sockfd);
}

/**
 * return the specified protocol data
 * @param dir direction (r,t,b)
 * @param dstId
 * @param cl
 * @return protocol data or NULL if not present
 */
protocolData_t* UDPCommTh::impl_getProtocolData(char dir, hprot_idtype_t id, int cl)
{
int _cl;

if(cl != -1)
	{
	if(cl < UDP_MAX_CLIENTS)
		{
		return &clients[cl].protocolData;
		}
	}

if(id == gd.myID)
	{
	TRACE(dbg, DBG_ERROR, "my HProt PD depends on the client, you must not need of this");
	return NULL;
	}
else if(id != HPROT_INVALID_ID)
	{
	_cl = getClientFromID(id);
	if(_cl >= 0)
		{
		return &clients[_cl].protocolData;
		}
	else
		{
		TRACE(dbg, DBG_ERROR, "protocol data for id %d not found", id);
		}
	}
return NULL;
}

/**
 * token reset
 * @param id
 */
void UDPCommTh::impl_resetToken(int id)
{
int _cl = getClientFromID(id);
hprotResetToken(&clients[_cl].protocolData, id);
}

/**
 * write data to serial
 * @param buff
 * @param size
 * @param p
 * @return true: ok
 */
bool UDPCommTh::impl_Write(uint8_t* buff, int size, hprot_idtype_t dstId, void* p)
{
// search the client data
int cl = getClientFromID(dstId);
if(cl >= 0)
	{
	txndata = sendto(sockfd, buff, size, 0, (struct sockaddr *) &clients[cl].clientaddr, clients[cl].clientlen);

//	hostaddrp = inet_ntoa(clients[cl].clientaddr.sin_addr);
//	hostportp=ntohs(clients[cl].clientaddr.sin_port);
//	printf("frame numb %d, addr: %s port %d\n",buff[HPROT_FLD_NUM],hostaddrp,hostportp);

	if(txndata < 0)
		{
		dbg->trace(DBG_ERROR, "in sendto");
		txndata = 0;
		}
	return true;
	}
else
	{
	dbg->trace(DBG_ERROR, "client with id %d not found", buff[HPROT_FLD_DST]);
	return false;
	}
}

/**
 * read data from UDP (blocking)
 * @param buff contains the protocol frame
 * @param size
 * @param p
 * @return ndata read
 */
int UDPCommTh::impl_Read(uint8_t* buff, int size, void* p)
{
// recvfrom: receive a UDP datagram from a client
//bzero(internal_buf, UDP_BUFSIZE);

int n = recvfrom(sockfd, internal_buf, UDP_BUFSIZE, 0, (struct sockaddr *) &clientaddr, (unsigned int *) &clientlen);
if(n > 0)
	{
	rxndata = n;
	// determine who sent the datagram
//	hostp = gethostbyaddr((const char *) &clientaddr.sin_addr.s_addr, sizeof(clientaddr.sin_addr.s_addr), AF_INET);
//	if(hostp == NULL)
//		{
//		dbg->trace(DBG_ERROR,"on gethostbyaddr");
//		}
	hostaddrp = inet_ntoa(clientaddr.sin_addr);
	if(hostaddrp == NULL)
		{
		dbg->trace(DBG_ERROR, "on inet_ntoa");
		}

	ClientData::clientIP_u actual_ip;
	actual_ip.ip = ip_string_2_uint32(hostaddrp);
	last_client = addClient(HPROT_INITIAL_ID, clientaddr, clientlen, actual_ip.ip);	// id will be updated in the onframe
	if(last_client < 0)
		{
		TRACE(dbg, DBG_ERROR, "client not found or too many clients");
		rxndata = 0;
		}
	else
		{
		if(rxndata > HPROT_BUFFER_SIZE)
			{
			TRACE(dbg, DBG_ERROR, "too many data (max %d): %d from %s", HPROT_BUFFER_SIZE, rxndata, hostaddrp);
			rxndata = 0;
			}
		else
			{
			memcpy(clients[last_client].buffer, internal_buf, rxndata);
			memcpy(buff, internal_buf, rxndata);
			clients[last_client].nDataAvailable = rxndata;
			//memcpy(&clients[last_client].protocolData,&last_pd,sizeof(protocolData_t));
			//memcpy(&last_pd,&clients[last_client].protocolData,sizeof(protocolData_t));
			//pd=&clients[last_client].protocolData;
			setProtocolData('r', impl_getProtocolData('r', HPROT_INVALID_ID, last_client));
			}
		}
	}
else
	{
	//dbg->trace(DBG_ERROR,"in recvfrom (timeout?)");
	rxndata = 0;
	}

return rxndata;
}

/**
 * set the new key to all clients
 * @param key
 */
void UDPCommTh::impl_setNewKey(uint8_t *key)
{
for(int i = 0;i < UDP_MAX_CLIENTS;i++)
	{
	hprotSetKey(&clients[i].protocolData, key);
	}
//hprotSetKey(&last_pd,key);
}

/**
 * enable/disable the crypto
 * @param 1=enable;0=disable
 */
void UDPCommTh::impl_enableCrypto(bool enable)
{
for(int i = 0;i < UDP_MAX_CLIENTS;i++)
	{
	if(enable)
		{
		hprotEnableCrypto(&clients[i].protocolData);
		}
	else
		{
		hprotDisableCrypto(&clients[i].protocolData);
		}
	}
//if(enable)
//	{
//	hprotEnableCrypto(&last_pd);
//	}
//else
//	{
//	hprotDisableCrypto(&last_pd);
//	}
}

/**
 * add an UDP client if not present else return the existing one
 * @param id (can be HPROT_INITIAL_ID)
 * @param clientaddr
 * @param clientlen
 * @param ip		uint32_t format
 * @return client number
 */
int UDPCommTh::addClient(hprot_idtype_t id, struct sockaddr_in clientaddr, int clientlen, uint32_t ip)
{
// search if client exists
int cl = -1;
bool found = false;
for(int i = 0;i < UDP_MAX_CLIENTS;i++)
	{
	if(ip == clients[i].clientIP.ip)
		{
//		if(!(clients[i].id== HPROT_INITIAL_ID || clients[i].id==id))
//			{
//			dbg->trace(DBG_WARNING,"client %d has different ID (%d -> %d)",i,clients[i].id,id);
//			}
		// check data
		if(memcmp(&clientaddr, &clients[i].clientaddr, sizeof(struct sockaddr_in) != 0))
			{
			dbg->trace(DBG_WARNING, "client %d has a different socket data struct, port is %d -> update", i, ntohs(clients[i].clientaddr.sin_port));
			memcpy(&clients[i].clientaddr, &clientaddr, sizeof(struct sockaddr_in));
			clients[i].clientlen = clientlen;
			}
		found = true;
		cl = i;
		break;
		}
	}
if(!found)
	{
	// add to the client list
	found = false;
	for(int i = 0;i < UDP_MAX_CLIENTS;i++)
		{
		// search for an empty position
		if(clients[i].clientIP.ip == 0)
			{
			found = true;
			clients[i].clientIP.ip = ip;
			clients[i].id = id;
			memcpy(&clients[i].clientaddr, &clientaddr, sizeof(struct sockaddr_in));

			hostaddrp = inet_ntoa(clients[i].clientaddr.sin_addr);
			hostportp = ntohs(clients[i].clientaddr.sin_port);
			TRACE(dbg, DBG_DEBUG, "added client %d with ip %s port %d", i, hostaddrp, hostportp);

			clients[i].clientlen = clientlen;
			cl = i;
			break;
			}
		}
	}
return cl;
}

/**
 *
 * @param id
 * @return -1 not found
 */
int UDPCommTh::getClientFromID(hprot_idtype_t id)
{
int ret = -1;
for(int i = 0;i < UDP_MAX_CLIENTS;i++)
	{
	if(clients[i].id == id)
		{
		ret = i;
		}
	}
return ret;
}

/**
 * get the client number form its ip (uint32)
 * @param ip
 * @return
 */
int UDPCommTh::getClientFromIP(uint32_t ip)
{
int ret = -1;
for(int i = 0;i < UDP_MAX_CLIENTS;i++)
	{
	if(clients[i].clientIP.ip == ip)
		{
		ret = i;
		break;
		}
	}
return ret;
}

/**
 * get the client number form its ip dotted string
 * @param ip dotted string
 * @return
 */
int UDPCommTh::getClientFromIP(string ip)
{
uint32_t i_ip = ip_string_2_uint32(ip);
return getClientFromIP(i_ip);
}

/**
 * dotted string IP conversion to uint32
 * @param ip dotte string
 * @return uint32
 */
uint32_t UDPCommTh::ip_string_2_uint32(string ip)
{
ClientData::clientIP_u actual_ip;
sscanf(ip.c_str(), "%hhu.%hhu.%hhu.%hhu", &actual_ip.ip_fld[0], &actual_ip.ip_fld[1], &actual_ip.ip_fld[2], &actual_ip.ip_fld[3]);
return actual_ip.ip;
}

/**
 * uint32 ip to dotted string conversion
 * @param ip
 * @return dotted string
 */
string UDPCommTh::ip_uint32_2_string(uint32_t ip)
{
char str_ip[20];
ClientData::clientIP_u actual_ip;
actual_ip.ip = ip;
sprintf(str_ip, "%hhu.%hhu.%hhu.%hhu", actual_ip.ip_fld[0], actual_ip.ip_fld[1], actual_ip.ip_fld[2], actual_ip.ip_fld[3]);
return string(str_ip);
}

/**
 * retrieve last framedata received
 * @param f
 */
void UDPCommTh::getLastRxFrame(globals_st::frameData_t *f)
{
memcpy(f, &lastRxFrameData, sizeof(globals_st::frameData_t));
}

/**
 * retrieve last frame received
 * @param f
 * @param payload
 */
void UDPCommTh::getLastRxFrame(frame_t *f, uint8_t *payload)
{
memcpy(f, &lastRxFrame, sizeof(frame_t));
memcpy(payload, lastRxFrame_payload, HPROT_PAYLOAD_SIZE);
f->data = payload;
}
//=============================================================================
// EVENT
//=============================================================================
/**
 * executes all the defined callbacks and interprets events and some commands
 * @param f
 */
void UDPCommTh::onFrame(frame_t &f)
{
//pthread_mutex_lock(&mutexOnFrame);

// before all I have to check if it is for me or is a gateway frame
#ifdef HPROT_TOKENIZED_MESSAGES
protocolData_t *_pd = impl_getProtocolData('b', f.dstID, last_client);
if((f.dstID == gd.myID) && (hprotCheckToken(_pd, &f)))
#else
if(f.dstID==gd.myID)
#endif
	{
	// is for me :)
	globals_st::frameData_t _f;
	if(clients[last_client].id != HPROT_INITIAL_ID)
		{
		if(clients[last_client].id != f.srcID)
			{
			dbg->trace(DBG_DEBUG, "client %d has changed id, from %d to %d -> updated ", last_client, clients[last_client].id, f.srcID);
			char _tmp[HPROT_BUFFER_SIZE * 2];
			Hex2AsciiHex(_tmp, f.data, f.len, true, ' ');
			dbg->trace(DBG_DEBUG, "Frame: src=%d dst=%d cmd=%d n=%d len=%d data:%s", f.srcID, f.dstID, f.cmd, f.num, f.len, _tmp);
			Hex2AsciiHex(_tmp, (uint8_t *) clients[last_client].buffer, clients[last_client].buffer[HPROT_FLD_LEN] + HPROT_OVERHEAD_SIZE, true, ' ');
			dbg->trace(DBG_DEBUG, "Frame (client): %s", _tmp);
			clients[last_client].id = f.srcID;
			}
		}
	else
		{
		dbg->trace(DBG_DEBUG, "client %d has id %d", last_client, f.srcID);
		clients[last_client].id = f.srcID;
		}

#ifdef HPROT_TOKENIZED_MESSAGES
	hprotUpdateToken(_pd, &f);
#endif

	bool needAnswer = (f.hdr == HPROT_HDR_REQUEST);
	switch(f.cmd)
		{
		//...............................................
		case HPROT_CMD_CHKLNK:
			dbg->trace(DBG_DEBUG, "checklink from id %d", f.srcID);
			if(needAnswer) sendAnswer(f.srcID, HPROT_CMD_ACK, NULL, 0, f.num);
			linkChecked = true;
			gd.cklinkCounter++;
			//break;

			//...............................................
		default:
			// save last frame_t
			memcpy(&lastRxFrame, &f, sizeof(frame_t));
			memcpy(&lastRxFrame_payload, f.data, f.len);
			lastRxFrame.data = lastRxFrame_payload;

// 			save data received in the queue
			memcpy(&_f.frame, &f, sizeof(frame_t));
			memcpy(&_f.frameData, f.data, f.len);
			_f.frame.data = _f.frameData;
			_f.channel = globals_st::frameData_t::chan_ethernet;

			memcpy(&lastRxFrameData, &_f, sizeof(globals_st::frameData_t));

			if(useQueue)
				{
				pthread_mutex_lock(&gd.mutexFrameQueue);
				gd.frameQueue.push(_f);
				pthread_mutex_unlock(&gd.mutexFrameQueue);
				}
			//dbg->trace(DBG_DEBUG,"queue depth : %d",gd.frameQueue.size());
			break;
		}
	}
//pthread_mutex_unlock(&mutexOnFrame);
}

/**
 * called on protocol errors
 * @param f
 */
void UDPCommTh::onError(frame_t &f, rxParserCondition_t e)
{
//dbg->trace(DBG_DEBUG,"protocol error from client %d", last_client);
if(e == pc_error_crc)
	{
	clients[last_client].error_crc++;
	}
else if(e == pc_error_cmd)
	{
	clients[last_client].error_cmd++;
	}
}
