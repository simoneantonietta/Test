/**----------------------------------------------------------------------------
 * PROJECT: varcosv
 * PURPOSE:
 * 
 *-----------------------------------------------------------------------------  
 * CREATION: 13 May 2016
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

#ifndef UDPPROTOCOLDEBUG_H_
#define UDPPROTOCOLDEBUG_H_

#include <string>
#include <errno.h>
#include <unistd.h>   //close
#include <arpa/inet.h>    //close
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#include "global.h"
#include "common/utils/Utils.h"
#include "common/utils/Trace.h"
#include "common/utils/TimeFuncs.h"
#include "common/prot6/hprot.h"
#include "common/prot6/hprot_stdcmd.h"

#include "comm/app_commands.h"
#include "Slave.h"

extern Slave *slave;
extern Trace *dbg;
extern globals_st gd;

using namespace std;

#define DBGIF_GPSNIF_ID			66
#define DBGIF_UDP_BUFSIZE 	1024

class UDPProtocolDebug
{
public:
	/**
	 * ctor
	 */
	UDPProtocolDebug()
		{
		startedRx=false;
		startedTx=false;
		name="UDP DbgIF:";
		resetApplicationRequest();
		}

	/**
	 * dtor
	 */
	virtual ~UDPProtocolDebug() {};

	/**
	 * open the UDP socket
	 * @param port
	 * @return
	 */
	bool openUdp(string ip, int port)
	{
	/* socket: create the socket */
	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if(sockfd < 0)
		{
		dbg->trace(DBG_ERROR, "%s opening socket @ port %d", name.c_str(), port);
		return false;
		}
	optval = 1;
	setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (const void *) &optval, sizeof(int));
	struct timeval tv;
	tv.tv_sec = 2;		// set timeout
	tv.tv_usec = 0;
	setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO,&tv,sizeof(tv));

	/* gethostbyname: get the server's DNS entry */
//	server = gethostbyname(hostname);
//	if(server == NULL)
//		{
//		fprintf(stderr, "ERROR, no such host as %s\n", hostname);
//		exit(0);
//		}
//
	/* Bind the TCP socket to the port SENDER_PORT_NUM and to the current
	* machines IP address (Its defined by SENDER_IP_ADDR).
	* Once bind is successful for UDP sockets application can operate
	* on the socket descriptor for sending or receiving data.
	*/
	memset((char *) &dbgserveraddr, 0, sizeof(dbgserveraddr));
	dbgserveraddr.sin_family = AF_INET;
	dbgserveraddr.sin_port = htons(port);
	dbgserveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
//	if(inet_aton(ip.c_str(), &dbgserveraddr.sin_addr) == 0)
//		{
//		dbg->trace(DBG_ERROR, "%s inet_aton() failed on dbgserveraddr\n", name.c_str());
//		return false;
//		}
	hostaddrp = inet_ntoa(dbgserveraddr.sin_addr);

	if(bind(sockfd, (struct sockaddr *) &dbgserveraddr, sizeof(struct sockaddr_in)) == -1)
		{
		dbg->trace(DBG_ERROR, "%s Bind to Port Number %d ,IP address %s failed\n",name.c_str(), ntohs(dbgserveraddr.sin_port), hostaddrp);
		close (sockfd);
		return false;
		}

	// set the destination
	memset((char *) &dbgdestaddr, 0, sizeof(dbgdestaddr));
	dbgdestaddr.sin_family = AF_INET;
	dbgdestaddr.sin_port = htons(port);
	if(inet_aton(ip.c_str(), &dbgdestaddr.sin_addr) == 0)
		{
		dbg->trace(DBG_ERROR, "%s inet_aton() failed on dbgdestaddr (%s)#\n", name.c_str(), ip.c_str());
		return false;
		}
	//hostaddrp = inet_ntoa(dbgserveraddr.sin_addr);
	hprotInit(&pd,NULL,NULL,NULL,NULL);
	#ifdef HPROT_USE_CRYPTO
	hprotSetKey(&pd,gd.hprotKey);
	hprotEnableCrypto(&pd);
	#endif
	hprotSetRxRawPlain(&pd,HPROT_RXRAWDATA_CRYPT);
	pd.myID=gd.myID;
	//hprotFrameSetup(f,rxBuffer);
	return true;
	}

	/**
	 * send a message to server
	 * @param msg
	 * @param size
	 * @return
	 */
	bool sendMsg(const uint8_t *msg, int size)
	{
	destlen = sizeof(dbgdestaddr);
	int n = sendto(sockfd, msg, (size_t)size, 0, (sockaddr *)&dbgdestaddr, (socklen_t)destlen);
	if(n < 0)
		{
		dbg->trace(DBG_ERROR, "%s in sendto",name.c_str());
		return false;
		}
	return true;
	}

	/**
	 * receive a message (blocking)
	 * @return n data read
	 */
	int recvMsg()
	{
  int n = recvfrom(sockfd, recvFromBuffer, DBGIF_UDP_BUFSIZE, 0, (sockaddr *)&dbgserveraddr, (unsigned int*)&serverlen);
  if(n>0)
  	{
		memcpy(rxBuffer,recvFromBuffer,n);
  	}
  else
  	{
  	n=0;
  	}
  //		if (n < 0)
  //		 {
  //		 dbg->trace(DBG_ERROR, "%s in recvfrom",name.c_str());
  //		 }
  return n;
	}

	/**
	 * close debug interface
	 */
	void closeComm()
	{
	endApplicationRequest();
	joinThread('r');
	close(sockfd);
	startedRx=false;
	}

	/**
	 * set the new crypto key
	 * @param key
	 */
	void setNewKey(uint8_t *key)
	{
	hprotSetKey(&pd,key);
	}

	/**
	 * enable/disable the crypto
	 * @param 1=enable;0=disable
	 */
	void enableCrypto(bool enable)
	{
	if(enable)
		{
		hprotEnableCrypto(&pd);
		}
	else
		{
		hprotDisableCrypto(&pd);
		}
	}

	//-----------------------------------------------------------------------------
	// thread

	/**
	 * start the execution of the thread
	 * @param arg (Uses default argument: arg = NULL)
	 * @param which can be 't' for tx or 'r' for rx
	 */
	bool startThread(void *arg,char which)
	{
	bool ret=true;
	bool *_started;
	void *_arg;
	pthread_t *_id;

	if(which=='r')
		{
		_started=&startedRx;
		_arg=this->argRx;
		_id=&_idRx;
		}
	else
		{
		_started=&startedTx;
		_arg=this->argTx;
		_id=&_idTx;
		}

	if (!*_started)
		{
		*_started = true;
		_arg = arg;
		/*
		 * Since pthread_create is a C library function, the 3rd
		 * argument is a global function that will be executed by
		 * the thread. In C++, we emulate the global function using
		 * the static member function that is called exec. The 4th
		 * argument is the actual argument passed to the function
		 * exec. Here we use this pointer, which is an instance of
		 * the Thread class.
		 */
		if(which=='r')
			{
			if ((ret = pthread_create(_id, NULL, &execRx, this)) != 0)
				{
				cout << strerror(ret) << endl;
				//throw "Error";
				ret=false;
				}
			}
		else
			{
			if ((ret = pthread_create(_id, NULL, &execTx, this)) != 0)
				{
				cout << strerror(ret) << endl;
				//throw "Error";
				ret=false;
				}
			}
		}
	return ret;
	}

	/**
	 * Allow the thread to wait for the termination status
	 * @param which can be 't' for tx or 'r' for rx
	 */
	void joinThread(char which)
	{
	if(which=='r')
		{
		if(startedRx)
			{
			pthread_join(_idRx, NULL);
			}
		}
	else
		{
		if(startedTx)
			{
			pthread_join(_idTx, NULL);
			}
		}
	}


	/**
	 * set the end application status to terminate all
	 */
	void endApplicationRequest()
	{
	//cout << "ending thread" << endl;
	pthread_mutex_lock(&mutexRx);
	this->endApplication=true;
	pthread_mutex_unlock(&mutexRx);
	}

	/**
	 * get the termination status
	 * @return true: end; false continue
	 */
	bool endApplicationStatus()
	{
	bool ret;
	pthread_mutex_lock(&mutexRx);
	ret=this->endApplication;
	pthread_mutex_unlock(&mutexRx);
	return ret;
	}

	/**
	 * set the end application status to permit the restart
	 */
	void resetApplicationRequest()
	{
	//cout << "thread reset" << endl;
	pthread_mutex_lock(&mutexRx);
	this->endApplication=false;
	pthread_mutex_unlock(&mutexRx);
	}


private:
	string name;

  int sockfd;
  int serverlen;
  int destlen;
  struct sockaddr_in dbgserveraddr;
  struct sockaddr_in dbgdestaddr;
  struct hostent server;
  char *hostname;
  char *hostaddrp; /* dotted decimal host addr string */
  int optval; /* flag value for setsockopt */
  uint8_t recvFromBuffer[DBGIF_UDP_BUFSIZE];

	protocolData_t pd;
	rxParserCondition_t prevProtCondition;	// rprevious result of the parser, used to detect changes

	uint8_t rxBuffer[HPROT_BUFFER_SIZE];
	uint8_t txBuffer[HPROT_BUFFER_SIZE];
	uint8_t frameData[HPROT_BUFFER_SIZE];	// used in the rx thread
	uint8_t ndata;

	char scriptResult[5000];
	//-----------------------------------------------------------------------------
	// thread
	pthread_t _idRx,_idTx;
	pthread_attr_t _attrRx,_attrTx;
	pthread_mutex_t mutexRx,mutexTx;

	bool startedRx,startedTx;
	bool noattrRx,noattrTx;
	void *argRx, *argTx;

	bool endApplication;

	/**
	 * Function that is used to be executed by the thread
	 * @param thr
	 */
	static void *execRx(void *thr)
	{
	reinterpret_cast<UDPProtocolDebug *>(thr)->runRx();
	return NULL;
	}

	/**
	 * Function that is used to be executed by the thread
	 * @param thr
	 */
	static void *execTx(void *thr)
	{
	reinterpret_cast<UDPProtocolDebug *>(thr)->runTx();
	return NULL;
	}

  /**
   * thread di ricezione
   */
	void runRx()
	{
	frame_t f;

	hprotFrameSetup(&f,frameData);
	while (!endApplicationStatus())
		{
		ndata=recvMsg();
		if(ndata>0)
			{
			//dbg->trace(DBG_NOTIFY, name + "message received");

			pthread_mutex_lock(&mutexRx);
			hprotFrameParserNData(&pd,&f,rxBuffer,ndata);

			if(pd.protCondition==pc_ok)
				{
				onFrame(f);
				}

			pthread_mutex_unlock(&mutexRx);
			}
		}
	}

	/**
	 * thread di trasmissione
	 */
	void runTx()
	{

	}

	//-----------------------------------------------------------------------------
	// Frames Debugger
	//-----------------------------------------------------------------------------

	/**
	 * send a command
	 * @param dstId
	 * @param cmd
	 * @param data
	 * @param len
	 * @param p implementation data
	 */
	void sendCommand(hprot_idtype_t dstId, uint8_t cmd, uint8_t* data, uint8_t len,void *p)
	{
	int n=hprotFrameBuild(&pd,txBuffer,HPROT_HDR_COMMAND,pd.myID,dstId,cmd,data,len,HPROT_SET_FNUMBER_DEFAULT);
	sendMsg(txBuffer,n);
	}
	/**
	 * send an answer
	 * @param dstId
	 * @param cmd
	 * @param data
	 * @param len
	 * @oaram fnumb use HPROT_SET_FNUMBER_DEFAULT or handle it
	 * @param p implementation data
	 */
	void sendAnswer(hprot_idtype_t dstId, uint8_t cmd, uint8_t* data, uint8_t len,uint8_t fnumb=HPROT_SET_FNUMBER_DEFAULT,void *p=NULL)
	{
	int n=hprotFrameBuild(&pd,txBuffer,HPROT_HDR_ANSWER,pd.myID,dstId,cmd,data,len,HPROT_SET_FNUMBER_DEFAULT);
	sendMsg(txBuffer,n);
	}

//	/**
//	 * send a request
//	 * @param dstId
//	 * @param cmd
//	 * @param data
//	 * @param len
//	 * @param nretry
//	 * @param setAnsPending
//	 * @param p implementation data
//	 */
//	void sendRequest(uint8_t dstId, uint8_t cmd, uint8_t* data, uint8_t len, bool setAnsPending, void *p)
//	{
//	int n=hprotFrameBuild(&pd,txBuffer,HPROT_HDR_REQUEST,pd.myID,dstId,cmd,data,len,HPROT_SET_FNUMBER_DEFAULT);
//	pthread_mutex_lock(&mutexRx);
//	hprotReset(&pd);
//	pthread_mutex_unlock(&mutexRx);
//	to.startTimeout(timeout);
//	if(setAnsPending)
//		{
//		setCommAnsPending(dstId, pd->frameNumber,HPROT_CMD_NONE);
//		rxResult.setWaiting();
//		}
//	sendMsg(txBuffer,n);
//	}

	/**
	 * send a generic frame
	 * @param hdr
	 * @param srcId
	 * @param dstId
	 * @param cmd
	 * @param data
	 * @param len
	 * @param p implementation data
	 */
	void sendFrame(uint8_t hdr, hprot_idtype_t srcId, hprot_idtype_t dstId, uint8_t cmd, uint8_t* data, uint8_t len,void *p)
	{
	//pthread_mutex_lock(&mutexRx);
	int n=hprotFrameBuild(&pd,txBuffer,hdr,srcId,dstId,cmd,data,len,HPROT_SET_FNUMBER_DEFAULT);
	//pthread_mutex_unlock(&mutexRx);
	sendMsg(txBuffer,n);
	}

	//-----------------------------------------------------------------------------
	// ON_FRAME Debugger
	//-----------------------------------------------------------------------------

	/**
	 * on frame for the debugger
	 * @param f
	 */
	virtual void onFrame(frame_t &f)
	{
	if(f.srcID==DBGIF_GPSNIF_ID)
		{
		bool ret;
		bool needAnswer = (f.hdr==HPROT_HDR_REQUEST) ? (true) : (false);
		switch(f.cmd)
			{
			//..............................................
			case HPROT_CMD_CHKLNK:
				if(needAnswer) sendAnswer(f.srcID,HPROT_CMD_ACK,0,0);
				break;

			//..............................................
			case HPROT_CMD_SV_SCRIPT_STRING:
				f.data[f.len]=0;	// terminate surely the string
				dbg->trace(DBG_NOTIFY, name + "command: " + _S (char*)f.data);
				ret=slave->parseScriptLine((char*)f.data,scriptResult);
				if(ret)
					{
					if(strlen(scriptResult)>0)
						{
						dbg->trace(DBG_NOTIFY, name + "result: " + _S scriptResult);
						if(needAnswer) sendAnswer(HPROT_DEBUG_ID,HPROT_CMD_DEBUG_MSG,(uint8_t*)scriptResult,strlen(scriptResult));
						}
					else
						{
						dbg->trace(DBG_NOTIFY, name + "send ACK");
						if(needAnswer) sendAnswer(f.srcID,HPROT_CMD_ACK,0,0);
						}
					}
				break;

			//::::::::::::::::::::::::::::::::::::::::::::::
			default:
				dbg->trace(DBG_NOTIFY, "%s command not known (%02X)", name.c_str(),f.cmd);
			}
		}
	else
		{
		globals_st::frameData_t _f;
		dbg->trace(DBG_NOTIFY, "%s frame put into queue (%02X) (from: %d to %d)", name.c_str(),f.cmd,f.srcID,f.dstID);
		memcpy(&_f.frame,&f,sizeof(frame_t));
		memcpy(&_f.frameData,f.data,f.len);
		_f.frame.data=_f.frameData;
		_f.channel=globals_st::frameData_t::chan_serial;
		pthread_mutex_lock(&gd.mutexFrameQueue);
		gd.frameQueue.push(_f);
		pthread_mutex_unlock(&gd.mutexFrameQueue);
		}
	}
};

//-----------------------------------------------
#endif /* UDPPROTOCOLDEBUG_H_ */
