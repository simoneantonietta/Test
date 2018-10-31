/*
 *-----------------------------------------------------------------------------
 * PROJECT:
 * PURPOSE: see module CommunicationTh.h file
 *-----------------------------------------------------------------------------
 */

#include "SerCommTh.h"

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
SerCommTh::SerCommTh()
{
name="SerialInterface";
answeredFrom=0;

endApplication=false;
setPacketProtocol(false);

hprotInit(&ser_pd,NULL,NULL,NULL,NULL);
hprotSetRxRawPlain(&ser_pd,HPROT_RXRAWDATA_CRYPT);
#ifdef HPROT_USE_CRYPTO
hprotSetKey(&ser_pd,gd.hprotKey);
#endif
setProtocolData('b',impl_getProtocolData('b'));
setMyId(gd.myID);

createThread(false,0,'r');
}

/**
 * dtor
 */
SerCommTh::~SerCommTh()
{
cout << "Serial communication end" << endl;
}

/**
 * open serial communication
 * @param dev
 * @param param
 * @param p
 * @return
 */
bool SerCommTh::impl_OpenComm(const string dev,int param,void* p)
{
bool ret;
ser_fd=serialOpen(dev.c_str(),param,0);
if(ser_fd>0)
	{
	serialSetBlocking(ser_fd,0,0);
	connected=true;
	ret=true;
	}
else
	{
	dbg->trace(DBG_ERROR,"serial " + dev + " cannot be opened");
	ret=false;
	}
return ret;
}

/**
 * close serial communication
 * @param p
 */
void SerCommTh::impl_CloseComm(void* p)
{
serialClose(ser_fd);
}

/**
 * get protocol data
 * @param dir direction (r,t,b)
 * @param id
 * @param cl
 * @return protocol data struct
 */
protocolData_t* SerCommTh::impl_getProtocolData(char dir,hprot_idtype_t id,int cl)
{
return &ser_pd;
}

/**
 * write data to serial
 * @param buff
 * @param size
 * @param p
 * @return
 */
bool SerCommTh::impl_Write(uint8_t* buff,int size,hprot_idtype_t dstId,void* p)
{
txndata=serialWrite(ser_fd,buff,size);
return true;
}

/**
 * read data from serial
 * @param buff
 * @param size
 * @param p
 * @return
 */
int SerCommTh::impl_Read(uint8_t* buff,int size,void* p)
{
rxndata=serialRead(ser_fd,buff,size,100);
return rxndata;
}

/**
 * set the new key to all clients
 * @param key
 */
void SerCommTh::impl_setNewKey(uint8_t *key)
{
hprotSetKey(pdrxtx,key);
}

/**
 * enable/disable the crypto
 * @param enable
 */
void SerCommTh::impl_enableCrypto(bool enable)
{
if(enable)
	{
	hprotEnableCrypto(pdrxtx);
	}
else
	{
	hprotDisableCrypto(pdrxtx);
	}
}

//=============================================================================
// EVENT
//=============================================================================
/**
 * executes all the defined callbacks and interprets events and some commands
 * @param f
 */
void SerCommTh::onFrame(frame_t &f)
{
globals_st::frameData_t _f;
//_commth_mutex->lock();

#ifdef HPROT_TOKENIZED_MESSAGES
protocolData_t *_pd=impl_getProtocolData('b',f.dstID,0);
if((f.dstID==gd.myID) && (hprotCheckToken(_pd,&f)))
#else
if(f.dstID==gd.myID)
#endif
	{
	// is for me :)
#ifdef HPROT_TOKENIZED_MESSAGES
	hprotUpdateToken(_pd,&f);
#endif

	bool needAnswer = (f.hdr==HPROT_HDR_REQUEST);
	switch(f.cmd)
		{
		//...............................................
		case HPROT_CMD_CHKLNK:
			dbg->trace(DBG_DEBUG,"checklink from id %d",f.srcID);
			if(needAnswer) sendAnswer(f.srcID,HPROT_CMD_ACK,NULL,0,f.num);
			gd.cklinkCounter++;
			linkChecked=true;
			//break;

		//...............................................
		default:
			// save data received

			memcpy(&_f.frame,&f,sizeof(frame_t));
			memcpy(&_f.frameData,f.data,f.len);
			_f.frame.data=_f.frameData;
			_f.channel=globals_st::frameData_t::chan_serial;

			pthread_mutex_lock(&gd.mutexFrameQueue);
			gd.frameQueue.push(_f);
			pthread_mutex_unlock(&gd.mutexFrameQueue);

			break;
		}
	}
//serialFlushRX(ser_fd);
//_commth_mutex->unlock();
}


