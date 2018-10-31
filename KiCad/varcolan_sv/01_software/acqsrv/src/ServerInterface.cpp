/*
 *-----------------------------------------------------------------------------
 * PROJECT: woodbusd
 * PURPOSE: see module ServerInterface.h file
 *-----------------------------------------------------------------------------
 */
#include "global.h"
#include "GlobalData.h"
#include "ServerInterface.h"
#include "protCommands.h"
#include "version.h"

#include "comm/SerCommTh.h"

extern Trace *dbg;
extern GlobalData *gd;
extern SerCommTh *comm;

#define SI_CAL_DEV					0
#define SI_SENS_DEV					1

/**
 * ctor
 */
ServerInterface::ServerInterface()
{
sdata=new SerializeData(256);
gd->nClients=0;
for(int i=0; i<SKSRV_MAX_CLIENTS; i++)
	{
	gd->clients[i].id=HPROT_INVALID_ID;
	gd->clients[i].protocolData.myID=MYID;
	gd->clients[i].sktConnected=false;
	hprotInit(&gd->clients[i].protocolData,NULL,NULL,NULL,NULL);
	}
}

/**
 * dtor
 */
ServerInterface::~ServerInterface()
{
}

/**
 * send a command
 * @param srcid
 * @param dstid
 * @param cmd
 * @param size
 * @param data
 */
void ServerInterface::sendCommand(uint8_t srcid,uint8_t dstid, uint8_t cmd, uint8_t *data,uint8_t len)
{
int cli=getClient(dstid);
frameSent=false;
if(cli<SKSRV_MAX_CLIENTS)
	{
	uint8_t *buff=new uint8_t[len+HPROT_OVERHEAD_SIZE+10];
	unsigned int n=hprotFrameBuild(&gd->clients[cli].protocolData,buff,HPROT_HDR_COMMAND,srcid,dstid,cmd,data,len,HPROT_SET_FNUMBER_DEFAULT);
	sendData(cli,(char*)buff,n);
	delete [] buff;
	frameSent=true;
	}
}

/**
 * send a request
 * @param srcid
 * @param dstid
 * @param cmd
 * @param size
 * @param data
 */
void ServerInterface::sendRequest(uint8_t srcid,uint8_t dstid, uint8_t cmd, uint8_t *data,uint8_t len)
{
int cli=getClient(dstid);
frameSent=false;
if(cli<SKSRV_MAX_CLIENTS)
	{
	uint8_t *buff=new uint8_t[len+HPROT_OVERHEAD_SIZE+10];
	unsigned int n=hprotFrameBuild(&gd->clients[cli].protocolData,buff,HPROT_HDR_REQUEST,srcid,dstid,cmd,data,len,HPROT_SET_FNUMBER_DEFAULT);
	sendData(cli,(char*)buff,n);
	delete [] buff;
	srvif_to.startTimeout(srvif_timeout);
	frameSent=true;
	}
}

/**
 * send an answer
 * @param srcid
 * @param dstid
 * @param cmd
 * @param size
 * @param data
 */
void ServerInterface::sendAnswer(uint8_t srcid,uint8_t dstid, uint8_t cmd,uint8_t *data,uint8_t len)
{
int cli=getClient(dstid);
frameSent=false;
if(cli<SKSRV_MAX_CLIENTS)
	{
	uint8_t *buff=new uint8_t[len+HPROT_OVERHEAD_SIZE+10];
	unsigned int n=hprotFrameBuild(&gd->clients[cli].protocolData,buff,HPROT_HDR_ANSWER,srcid,dstid,cmd,data,len,HPROT_SET_FNUMBER_DEFAULT);
	sendData(cli,(char*)buff,n);
	delete [] buff;
	frameSent=true;
	}
}

/**
 * send a generic frame
 * @param hdr
 * @param srcid
 * @param dstid
 * @param cmd
 * @param size
 * @param data
 * @param forceSend with this true frame is sent without check if the client is in the database
 */
void ServerInterface::sendFrame(uint8_t hdr,uint8_t srcid,uint8_t dstid, uint8_t cmd, uint8_t *data,uint8_t len, bool forceSend)
{
int cli=getClient(dstid);
frameSent=false;
if(forceSend || cli<SKSRV_MAX_CLIENTS)
	{
	uint8_t *buff=new uint8_t[len+HPROT_OVERHEAD_SIZE+10];
	unsigned int n=hprotFrameBuild(&gd->clients[cli].protocolData,buff,hdr,srcid,dstid,cmd,data,len,HPROT_SET_FNUMBER_DEFAULT);
	sendData(cli,(char*)buff,n);
	delete [] buff;
	frameSent=true;
	}
}


//-----------------------------------------------------------------------------
// PRIVATE MEMBERS
//-----------------------------------------------------------------------------
/**
 * from id return client number
 * @param id
 * @return client number; if not found return SKSRV_MAX_CLIENTS
 */
int ServerInterface::getClient(uint8_t id)
{
for(int i=0;i<SKSRV_MAX_CLIENTS;i++)
	{
	if(gd->clients[i].id==id) return i;
	}
return SKSRV_MAX_CLIENTS;
}

/**
 * from client number return its id
 * @param client
 * @return ID
 */
uint8_t ServerInterface::getId(int client)
{
return gd->clients[client].protocolData.myID;
}

//===== EVENTS HANDLER =====
/**
 * handle data received
 */
void ServerInterface::onDataReceived(int client)
{
int ndata;
frame_t f;
hprotFrameSetup(&f,frameData);

ndata=nRxData(client);
if(ndata>0)
	{
	uint8_t *buff=new uint8_t[ndata];
	readData(client,(char *)buff,ndata);
	actualClient=client;
	hprotFrameParserNData(&gd->clients[client].protocolData,&f,buff,ndata);
	if(gd->clients[client].protocolData.protCondition==pc_ok)
		{
		if(f.dstID==gd->myID)	// is for me
			{
			if(gd->clients[client].id!=HPROT_INVALID_ID)
				{
				if(gd->clients[client].id!=f.srcID)
					{
					dbg->trace(DBG_WARNING,"client %d has change its ID: %d -> %d",client,(int)gd->clients[client].id,(int)f.srcID);
					}
				}
			else
				{
				dbg->trace(DBG_NOTIFY,"client %d identified with ID: %d",client,(int)f.srcID);
				}
			gd->clients[client].id=f.srcID; // always store the client id
			onFrame(f);
			}
		else // not is for me
			{
			dbg->trace(DBG_NOTIFY,"received a frame to be redirected");

			// gateway mode: sent to all clients
			for(int c=0;c<SKSRV_MAX_CLIENTS;c++)
				{
				if(gd->clients[c].sktConnected==true)
					{
					if(f.srcID==gd->clients[c].id) // to avoid "mirroring"
						{
						continue;
						}
					if(gd->forceSend2Unknown || gd->clients[c].id!=HPROT_INVALID_ID)
						{
						/*
						 *  if the device is known (has performed a check link) the frame is redirected to it else it will be ignored.
						 *  However the function sendFrame, can be forced to send the frame: see the optional parameter forceSend
						 *  WARN: avoid "mirroring" I mean, avoid to gateway to the sender
						 */
						sendFrame(f.hdr,f.srcID,f.dstID,f.cmd,f.data,f.len,gd->forceSend2Unknown);
						if(isFrameSent())
							{
							dbg->trace(DBG_NOTIFY,"gateway to client %d",(int)gd->clients[c].id);
							}
						else
							{
							dbg->trace(DBG_NOTIFY,"no client with ID %d is in the database",(int)f.dstID);
							}
						}
					}
				}
			// gateway mode: sent to serial
			if(gd->useSerial)
				{
				comm->sendFrame(f.hdr,f.srcID,f.dstID,f.cmd,f.len,f.data,f.num);
				dbg->trace(DBG_NOTIFY,"gateway to serial");
				}
			}
		}
	}
}

/**
 * event on new connection
 * @param client
 */
void ServerInterface::onNewConnection(int client)
{
gd->clients[client].sktConnected=true;
}

/**
 * event on close connection
 * @param client
 */
void ServerInterface::onCloseConnection(int client)
{
gd->clients[client].sktConnected=false;
gd->nClients--;
dbg->trace(DBG_NOTIFY,"client %d disconnected",client);
}

/**
 * interpret the various commands
 * @param f frame received
 */
void ServerInterface::onFrame(frame_t &f)
{
bool needAnswer = (f.hdr==HPROT_HDR_REQUEST);

//cout << "frame from " << to_string((int)f.srcID) << endl;
switch(f.cmd)
	{
	//.................................................................
	case HPROT_CMD_CHKLNK:
		if(needAnswer) sendAnswer(MYID,f.srcID,HPROT_CMD_ACK,0,0);
		dbg->trace(DBG_DEBUG,"checklink");
		break;
	//.................................................................
	//.................................................................
	case HPROT_CMD_GETVERSION:
		if(needAnswer) sendAnswer(MYID,f.srcID,HPROT_CMD_GETVERSION,(uint8_t*)VERSION_NOSPACE,strlen(VERSION_NOSPACE));
		dbg->trace(DBG_DEBUG,"version request");
		break;
		//.................................................................
	}
}

