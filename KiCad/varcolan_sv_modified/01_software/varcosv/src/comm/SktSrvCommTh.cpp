/*
 *-----------------------------------------------------------------------------
 * PROJECT:
 * PURPOSE: see module CommunicationTh.h file
 *-----------------------------------------------------------------------------
 */

#include "SktSrvCommTh.h"

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
#include "../common/utils/Trace.h"
#include "../global.h"
#include "../db/DBWrapper.h"

extern Trace *dbg;
extern globals_st gd;
//=============================================================================

/**
 * ctor
 */
SktSrvCommTh::SktSrvCommTh()
{
name="SocketServerInterface";
answeredFrom=0;
_nDataAvail=0;
linkChecked=false;
nClients=0;
for(int i=0; i<SKSRV_MAX_CLIENTS; i++)
	{
	clients[i].id=HPROT_INVALID_ID;
	clients[i].protocolData.myID=gd.myID;
	clients[i].sktConnected=false;
	hprotInit(&clients[i].protocolData,NULL,NULL,NULL,NULL);
	hprotSetRxRawPlain(&clients[i].protocolData,HPROT_RXRAWDATA_CRYPT);
	#ifdef HPROT_USE_CRYPTO
	hprotSetKey(&clients[i].protocolData,gd.hprotKey);
	#endif
	}
setPacketProtocol(true);
endApplication=false;
createThread(false,0,'r');
}

/**
 * dtor
 */
SktSrvCommTh::~SktSrvCommTh()
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
bool SktSrvCommTh::impl_OpenComm(const string dev,int param,void* p)
{
// blocking read socket (to=0)
bool ret=createServer(param,0,false,false);
connected=true;

return ret;
}

/**
 * close serial communication
 * @param p
 */
void SktSrvCommTh::impl_CloseComm(void* p)
{
closeServer();
dbg->trace(DBG_NOTIFY,"socket server closed");
}

/**
 * get protocol data
 * @param dir direction (r,t,b)
 * @param id
 * @param cl
 * @return protocol data struct
 */
protocolData_t* SktSrvCommTh::impl_getProtocolData(char dir,hprot_idtype_t id,int cl)
{
int _cl;

if(cl != -1)
	{
	if(cl < SKSRV_MAX_CLIENTS)
		{
		return &clients[cl].protocolData;
		}
	}

if(id==gd.myID)
	{
	TRACE(dbg,DBG_ERROR,"my HProt PD depends on the client, you mult not neet of this");
	return NULL;
	}
else if(id!=HPROT_INVALID_ID)
	{
	_cl=getClientFromID(id);
	if(_cl>=0)
		{
		return &clients[_cl].protocolData;
		}
	else
		{
		TRACE(dbg,DBG_ERROR,"protocol data for id %d not found",id);
		}
	}
return NULL;
}

/**
 * write data to device
 * @param buff
 * @param size
 * @param p
 * @return
 */
bool SktSrvCommTh::impl_Write(uint8_t* buff,int size,hprot_idtype_t dstId,void* p)
{
bool ret=false;

// if dest is broadcast, the message must to be sent to all clients
if(buff[2]==HPROT_BROADCAST_ID)
	{
	for(int i=0;i<SKSRV_MAX_CLIENTS;i++)
		{
		if(clients[i].sktConnected)
			{
			sendData(i,(char*)buff,size);
			}
		}
	}
else
	{
	int cli=getClientFromID(dstId); // destination id
	if(cli<SKSRV_MAX_CLIENTS)
		{
		sendData(cli,(char*)buff,size);
		ret=true;
		}
	}
txndata=size;
return ret;
}

/**
 * read data from serial
 * @param buff
 * @param size
 * @param p
 * @return
 */
int SktSrvCommTh::impl_Read(uint8_t* buff,int size,void* p)
{
loop();
rxndata=_nDataAvail;
_nDataAvail=0;
return rxndata;
}

/**
 * set the new key to all clients
 * @param key
 */
void SktSrvCommTh::impl_setNewKey(uint8_t *key)
{
for(int i = 0;i < SKSRV_MAX_CLIENTS;i++)
	{
	hprotSetKey(&clients[i].protocolData,key);
	}
}

/**
 * enable/disable the crypto
 * @param 1=enable;0=disable
 */
void SktSrvCommTh::impl_enableCrypto(bool enable)
{
for(int i = 0;i < SKSRV_MAX_CLIENTS;i++)
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
}


#if 0
/**
 * executes all the defined callbacks and interprets events and some commands
 * @param f
 */
void SktSrvCommTh::onFrame(frame_t &f)
{
//_commth_mutex->lock();

// before all I have to check if it is for me or is a gateway frame
if(f.dstID==gd.myID)
	{
	// is for me :)
	globals_st::frameData_t _f;
	switch(f.cmd)
		{
		case HPROT_CMD_CHKLNK:
			sendAnswer(f.srcID,HPROT_CMD_ACK,NULL,0);
			break;

		default:
			pthread_mutex_lock(&mutexRx);

			memcpy(&_f.frame,&f,sizeof(frame_t));
			memcpy(&_f.frameData,f.data,f.len);
			_f.frame.data=_f.frameData;
			_f.channel=globals_st::frameData_t::chan_ethernet;
			gd.frameQueue.push(_f);

			pthread_mutex_unlock(&mutexRx);
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
#endif
//-----------------------------------------------------------------------------
// PRIVATE
//-----------------------------------------------------------------------------
/**
 * from id return client number
 * @param id
 * @return client number; if not found return SKSRV_MAX_CLIENTS
 */
int SktSrvCommTh::getClientFromID(hprot_idtype_t id)
{
for(int i=0;i<SKSRV_MAX_CLIENTS;i++)
	{
	if(clients[i].id==id) return i;
	}
return SKSRV_MAX_CLIENTS;
}

/**
 * from client number return its id
 * @param client
 * @return ID
 */
hprot_idtype_t SktSrvCommTh::getIDFromClient(int client)
{
return clients[client].protocolData.myID;
}

//-----------------------------------------------------------------------------
// EVENT HANDLERS
//-----------------------------------------------------------------------------
/**
 * handle data received
 */
void SktSrvCommTh::onDataReceived(int client)
{
_nDataAvail=nRxData(client);
if(_nDataAvail>0)
	{
	readData(client,(char *)rxBuffer,_nDataAvail);
	actualClient=client;
	pdrx=&clients[client].protocolData;
	}
}

/**
 * event on new connection
 * @param client
 */
void SktSrvCommTh::onNewConnection(int client)
{
clients[client].sktConnected=true;
stats.connections++;
}

/**
 * event on close connection
 * @param client
 */
void SktSrvCommTh::onCloseConnection(int client)
{
// generates an event to be stored in the history
//if(clients[client].id >0)
//	{
//	Event_t ev;
//	ev.Parameters_s.eventCode=terminal_not_inline;
//	ev.Parameters_s.idBadge=65535;
//	ev.Parameters_s.area=255;
//	ev.Parameters_s.code=65535;
//	ev.Parameters_s.terminalId=clients[client].id;
//	ev.Parameters_s.timestamp=time(NULL);
//	db->addEvent(ev);
//	}

clients[client].sktConnected=false;
nClients--;
dbg->trace(DBG_NOTIFY,"client %d (id %d) disconnected",client,clients[client].id);
stats.disconnections++;
}

/**
 * interpret the various commands
 * @param f frame received
 */
void SktSrvCommTh::onFrame(frame_t &f)
{
bool needAnswer = (f.hdr==HPROT_HDR_REQUEST);
if(clients[actualClient].id!=HPROT_INVALID_ID)
	{
	if(clients[actualClient].id!=f.srcID)
		{
		dbg->trace(DBG_WARNING,"client %d has change its ID: %d -> %d",actualClient,(int)clients[actualClient].id,(int)f.srcID);
		}
	}
else
	{
	dbg->trace(DBG_NOTIFY,"client %d has ID %d",actualClient,(int)f.srcID);
	}
// check the id if is unique
for(int i=0;i<SKSRV_MAX_CLIENTS;i++)
	{
	if(actualClient!=i)
		{
		if(f.srcID == clients[i].id)
			{
			dbg->trace(DBG_WARNING,"client %d has an id equal to client %d -> replaced",actualClient,i);
			// delete "old" client
			clients[i].id=HPROT_INVALID_ID;
			clients[i].protocolData.myID=gd.myID;
			clients[i].sktConnected=false;
			hprotInit(&clients[i].protocolData,NULL,NULL,NULL,NULL);
			#ifdef HPROT_USE_CRYPTO
			hprotSetKey(&clients[i].protocolData,gd.hprotKey);
			#endif

			break;
			}
		}
	}

clients[actualClient].id=f.srcID; // store id of the client

// before all I have to check if it is for me or is a gateway frame
if(f.dstID==gd.myID)
	{
	// is for me :)
	globals_st::frameData_t _f;
	switch(f.cmd)
		{
		//...............................................
		case HPROT_CMD_CHKLNK:
			dbg->trace(DBG_DEBUG,"checklink from id %d",f.srcID);
			if(needAnswer) sendAnswer(f.srcID,HPROT_CMD_ACK,0,0,f.num);
			linkChecked=true;
			gd.cklinkCounter++;
			//break;

		//...............................................
		default:
			//pthread_mutex_lock(&mutexRx);

			memcpy(&_f.frame,&f,sizeof(frame_t));
			memcpy(&_f.frameData,f.data,f.len);
			_f.client=actualClient;
			_f.frame.data=_f.frameData;
			_f.channel=globals_st::frameData_t::chan_ethernet;
			gd.frameQueue.push(_f);

			//pthread_mutex_unlock(&mutexRx);
			break;
		}
	}

}



