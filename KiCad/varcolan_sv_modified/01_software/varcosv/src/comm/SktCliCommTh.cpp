/*
 *-----------------------------------------------------------------------------
 * PROJECT:
 * PURPOSE: see module CommunicationTh.h file
 *-----------------------------------------------------------------------------
 */

#include "SktCliCommTh.h"

#include <sys/time.h>
#include <sys/types.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/fcntl.h>
#include <errno.h>
#include <cstdlib>
#include <unistd.h>

#include "../common/utils/Utils.h"
#include "../global.h"

extern RXHFrame *rf;
extern Trace *dbg;
extern globals_st gd;

//=============================================================================

/**
 * ctor
 */
SktCliCommTh::SktCliCommTh()
{
name="SocketInterface";
answeredFrom=0;

endApplication=false;
setPacketProtocol(true);

hprotInit(&skt_pd,NULL,NULL,NULL,NULL);
hprotSetRxRawPlain(&skt_pd,HPROT_RXRAWDATA_CRYPT);
#ifdef HPROT_USE_CRYPTO
hprotSetKey(&skt_pd,gd.hprotKey);
#endif
setProtocolData('b',impl_getProtocolData('b'));
setMyId(gd.myID);

createThread(false,0,'r');
}

/**
 * dtor
 */
SktCliCommTh::~SktCliCommTh()
{
cout << "Socket communication end" << endl;
}

/**
 * open socket communication
 * @param dev
 * @param param
 * @param p
 * @return
 */
bool SktCliCommTh::impl_OpenComm(const string dev,int param,void* p)
{
int flags, n, error;
socklen_t len;
struct hostent *h;
struct sockaddr_in temp;

temp.sin_family = AF_INET;
temp.sin_port = htons(param);
h = gethostbyname(dev.c_str());
if (h == 0)
	{
	dbg->trace(DBG_DEBUG,"Gethostbyname failed");
	return false;
	}
bcopy(h->h_addr, &temp.sin_addr, h->h_length);
sock = socket(AF_INET, SOCK_STREAM, 0);

flags = fcntl(sock, F_GETFL, 0);
#if 0
// non blocking
fcntl(sock, F_SETFL, flags | O_NONBLOCK);
#else
// blocking
fcntl(sock, F_SETFL, flags);
#endif

error = 0;
if((n = connect(sock, (struct sockaddr*) &temp, sizeof(temp))) < 0)
	{
	if(errno != EINPROGRESS)
		{
		dbg->trace(DBG_ERROR,"connect error [errno=%d]", errno);
		return false;
		}
	}

/* Do whatever we want while the connect is taking place. */

if (n == 0)
	goto done;	/* connect completed immediately */

FD_ZERO(&rset);
FD_SET(sock, &rset);
wset = rset;
tval.tv_sec = 2;
tval.tv_usec = 0;

if ((n = select(sock + 1, &rset, &wset, NULL, &tval)) == 0)
	{
	close (sock); /* timeout */
	errno = ETIMEDOUT;
	return false;
	}

if (FD_ISSET(sock, &rset) || FD_ISSET(sock, &wset))
	{
	len = sizeof(error);
	if (getsockopt(sock, SOL_SOCKET, SO_ERROR, &error, &len) < 0) return false; /* Solaris pending error */
	}
else
	dbg->trace(DBG_ERROR,"select error: sockfd not set");

done: fcntl(sock, F_SETFL, flags); /* restore file status flags */

if (error)
	{
	close (sock); /* just in case */
	errno = error;
	return false;
	}

// update values to handle incoming messages
tval.tv_sec = 1;
tval.tv_usec = 0;
connected=true;
return true;
}

/**
 * close  communication
 * @param p
 */
void SktCliCommTh::impl_CloseComm(void* p)
{
close(sock);
dbg->trace(DBG_NOTIFY,"socket closed");
}

/**
 * get protocol data
 * @param dir direction (r,t,b)
 * @param id
 * @param cl
 * @return protocol data struct
 */
protocolData_t* SktCliCommTh::impl_getProtocolData(char dir,hprot_idtype_t id,int cl)
{
return &skt_pd;
}

/**
 * write data to
 * @param buff
 * @param size
 * @param p
 * @return
 */
bool SktCliCommTh::impl_Write(uint8_t* buff,int size,hprot_idtype_t dstId,void* p)
{
bool ret=true;
txndata=write(sock, buff, size);
if(txndata < 0)
	{
	dbg->trace(DBG_WARNING, "message not sent");
	ret=false;
	}
else
	{
	}
return ret;
}

/**
 * read data from
 * @param buff
 * @param size
 * @param p
 * @return
 */
int SktCliCommTh::impl_Read(uint8_t* buff,int size,void* p)
{
#if 1
int n;
// non blocking
FD_ZERO(&rset);
FD_SET(sock, &rset);

// timeout for the select
tval.tv_sec=1;
tval.tv_usec=0;

if ((n = select(sock + 1, &rset, &wset, NULL, &tval)) == 0)
	{
	// NOTE: this timeout does not means necessarily an error

	//resetRxFrame();
	//result.setError();
	//result.message="select timeout";
	errno = ETIMEDOUT;
	rxndata=0;
	return rxndata;
	}

rxndata=recv(sock,rxBuffer,HPROT_BUFFER_SIZE-1,MSG_DONTWAIT);
if(rxndata<0)
	{
	rxndata=0;
	//cout << "select timeout" << endl;
	}
#else
// blocking
rxndata=recv(sock,rxBuffer,HPROT_BUFFER_SIZE-1,MSG_WAITFORONE);
//ndata=read(sock,buffer,BUFFER_SIZE);
#endif
return rxndata;
}

/**
 * set the new key to all clients
 * @param key
 */
void SktCliCommTh::impl_setNewKey(uint8_t *key)
{
hprotSetKey(pdrxtx,key);
//hprotSetKey(pdtx,key);
}

/**
 * enable/disable the crypto
 * @param 1=enable;0=disable
 */
void SktCliCommTh::impl_enableCrypto(bool enable)
{
if(enable)
	{
	hprotEnableCrypto(pdrxtx);
	//hprotEnableCrypto(pdtx);
	}
else
	{
	hprotDisableCrypto(pdrxtx);
	//hprotDisableCrypto(pdtx);
	}
}

/**
 * executes all the defined callbacks and interprets events and some commands
 * @param f
 */
void SktCliCommTh::onFrame(frame_t &f)
{
//_commth_mutex->lock();

// before all I have to check if it is for me or is a gateway frame
if(f.dstID==gd.myID)
	{
	// is for me :)
	bool needAnswer = (f.hdr==HPROT_HDR_REQUEST);
	globals_st::frameData_t _f;
	switch(f.cmd)
		{
		//...............................................
		case HPROT_CMD_CHKLNK:
			dbg->trace(DBG_DEBUG,"checklink from id %d",f.srcID);
			if(needAnswer) sendAnswer(f.srcID,HPROT_CMD_ACK,NULL,0,f.num);
			linkChecked=true;
			gd.cklinkCounter++;

			//break;

		//...............................................
		default:
			//pthread_mutex_lock(&mutexRx);


			memcpy(&_f.frame,&f,sizeof(frame_t));
			memcpy(&_f.frameData,f.data,f.len);
			_f.frame.data=_f.frameData;
			_f.channel=globals_st::frameData_t::chan_ethernet;

			pthread_mutex_lock(&gd.mutexFrameQueue);
			gd.frameQueue.push(_f);
			pthread_mutex_unlock(&gd.mutexFrameQueue);

			//pthread_mutex_unlock(&mutexRx);
			break;
		}
	}
else
	{
	/*
	 * it is a frame to be routed to other device (gateway function)
	 * gateway is versus the socket connections because serial are in the same bus
	 * so it is automatically received from the interested device
	 */
	//srvif->sendFrame(f.hdr,f.srcID,f.dstID,f.cmd,f.data,f.len,gd->forceSend2Unknown);
	// save data received
	}

//_commth_mutex->unlock();
}

