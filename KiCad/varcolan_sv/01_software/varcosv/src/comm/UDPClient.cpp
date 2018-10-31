/*
 *-----------------------------------------------------------------------------
 * PROJECT: rasp_varcosv
 * PURPOSE: see module UDPClient.h file
 *-----------------------------------------------------------------------------
 */

#include <comm/UDPClient.h>

#include <errno.h>
#include <unistd.h>   //close
#include <arpa/inet.h>    //close
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#include <sys/time.h> //FD_SET, FD_ISSET, FD_ZERO macros



extern Trace *dbg;
extern globals_st gd;

UDPClient::UDPClient(hprot_idtype_t myId) : UDPCommTh(myId)
{
// TODO Auto-generated constructor stub

}

UDPClient::~UDPClient()
{
// TODO Auto-generated destructor stub

}

void UDPClient::getLastRxFrameFromQueue(frame_t *f,uint8_t *payload)
{
globals_st::frameData_t _f;
if(gd.SVCframeQueue.empty() == false)
	{
	_f = gd.SVCframeQueue.front();
	memcpy(f,&_f.frame,sizeof(frame_t));
	memcpy(payload,_f.frameData,HPROT_PAYLOAD_SIZE);
	f->data=payload;

	gd.SVCframeQueue.pop();
	}
}

/**
 * executes all the defined callbacks and interprets events and some commands
 * @param f
 */
void UDPClient::onFrame(frame_t& f)
{


//pthread_mutex_lock(&mutexOnFrame);

// before all I have to check if it is for me or is a gateway frame
#ifdef HPROT_TOKENIZED_MESSAGES
protocolData_t *_pd=impl_getProtocolData('b',f.dstID,last_client);

/* check destination */
if(f.dstID==gd.svm.svcID)
	{
	rxResult.setError("wrong destination");
	return;
	}

/* check token */
if((hprotCheckToken(_pd,&f)) == false)
	{
	rxResult.setError("wrong token");
	return;
	}


#else
if(f.dstID==gd.svm.svcID)
#endif
	{
	/* Frame valid */
	// is for me :)
	globals_st::frameData_t _f;
	if(clients[last_client].id!=HPROT_INITIAL_ID)
		{
		if(clients[last_client].id!=f.srcID)
			{
			dbg->trace(DBG_DEBUG,"client %d has changed id, from %d to %d -> updated ",last_client,clients[last_client].id,f.srcID);
			char _tmp[HPROT_BUFFER_SIZE*2];
			Hex2AsciiHex(_tmp,f.data,f.len,true,' ');
			dbg->trace(DBG_DEBUG,"Frame: src=%d dst=%d cmd=%d n=%d len=%d data:%s",f.srcID,f.dstID,f.cmd,f.num,f.len,_tmp);
			Hex2AsciiHex(_tmp,(uint8_t *)clients[last_client].buffer,clients[last_client].buffer[HPROT_FLD_LEN]+HPROT_OVERHEAD_SIZE,true,' ');
			dbg->trace(DBG_DEBUG,"Frame (client): %s",_tmp);
			clients[last_client].id=f.srcID;
			}
		}
	else
		{
		dbg->trace(DBG_DEBUG,"client %d has id %d",last_client,f.srcID);
		clients[last_client].id=f.srcID;
		}

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
			linkChecked=true;
			gd.cklinkCounter++;
			//break;

		//...............................................
		default:
			// save last frame_t
			memcpy(&lastRxFrame,&f,sizeof(frame_t));
			memcpy(&lastRxFrame_payload,f.data,f.len);
			lastRxFrame.data=lastRxFrame_payload;

// 			save data received in the queue
			memcpy(&_f.frame,&f,sizeof(frame_t));
			memcpy(&_f.frameData,f.data,f.len);
			_f.frame.data=_f.frameData;
			_f.channel=globals_st::frameData_t::chan_ethernet;

			memcpy(&lastRxFrameData,&_f,sizeof(globals_st::frameData_t));

			if(useQueue)
				{
				pthread_mutex_lock(&gd.mutexSVCFrameQueue);
				gd.SVCframeQueue.push(_f);
				pthread_mutex_unlock(&gd.mutexSVCFrameQueue);
				}
			//dbg->trace(DBG_DEBUG,"queue depth : %d",gd.frameQueue.size());
			break;
		}
	}
//pthread_mutex_unlock(&mutexOnFrame);
}


void UDPClient::onError(frame_t& f, rxParserCondition_t e)
{
}
