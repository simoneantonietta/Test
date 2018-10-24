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

#include "../GlobalData.h"
#include "../utils/Utils.h"
#include "../ServerInterface.h"

extern Trace *dbg;
extern GlobalData *gd;
extern ServerInterface *srvif;

//=============================================================================

/**
 * ctor
 */
SerCommTh::SerCommTh()
{
pd=new protocolData_t();
HPROT_SETMYID(pd,HPROT_INITIAL_ID);
enableCallback=false;
enableEventReception=false;
timeout=3000;	// default timeout [ms]

textMode=false;
endApplication=false;
ansPending=false;
_commth_mutex=new WLock;
setStatus(st_idle);
//comm_buffer=new uint8_t[HPROT_BUFFER_SIZE];
}

/**
 * dtor
 */
SerCommTh::~SerCommTh()
{
delete _commth_mutex;
delete pd;
//delete comm_buffer;
cout << "Communication thread end" << endl;
}

/**
 * **** RUN THREAD ****
 * This method will be executed by the Thread::exec method,
 * which is executed in the thread
 */
void SerCommTh::run()
{
//_sd = reinterpret_cast<SharedData *>(arg);
frame_t f;
hprotFrameSetup(&f,frameData);
while (!endApplicationStatus())
	{
	waitForData();		// handle rx data
	_commth_mutex->lock();
	if(ndata>0)
		{
		hprotFrameParserNData(pd,&f,rxBuffer,ndata);

		if(pd->protCondition==pc_ok || pd->protCondition==pc_error_crc || pd->protCondition==pc_error_cmd)
			{
			printRx((char*)pd->rxRawBuffer,pd->rxRawBufferCount);
			}
		if(pd->protCondition==pc_inprogress)
			{
			if(prevProtCondition!=pd->protCondition)
				{
				to.startTimeout(timeout);
				}
			if(to.expired)
				{
				hprotReset(pd);
				}
			}
		if(pd->protCondition==pc_ok)
			{
			// main state machine
			switch(status)
				{
				//.............................................
				case st_idle:
					onFrame(f);
					break;

				//.............................................
				case st_ansPending:
					/*
					 * here if an answer was expected and if is received
					 */
					if(!to.expired)
						{
						hprotReset(pd);
						setStatus(st_idle);
						result.setOK();
						}
					else
						{
						result.setError();
						hprotReset(pd);
						setStatus(st_idle);
						}
					break;

				//.............................................
				case st_error:
					break;
				}
			}
		prevProtCondition=pd->protCondition;
		}
	_commth_mutex->unlock();
	}
cout << "CommunicationTh tread stopped" << endl;
}

/**
 * open the communication
 */
bool SerCommTh::openCommunication(const string dev,int baud)
{
bool ret;
ser_fd=serialOpen(dev.c_str(),baud,0);
if(ser_fd>0)
	{
	serialSetBlocking(ser_fd,0,10);// set blocking for 1 s
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
 * close the socket
 */
void SerCommTh::closeCommunication()
{
serialClose(ser_fd);
connected=false;
}

/**
 * send a command
 * @param dstId
 * @param cmd
 * @param data
 * @param size
 */
void SerCommTh::sendCommand(uint8_t dstId, uint8_t cmd, uint8_t size, uint8_t *data)
{
int n=hprotFrameBuild(pd,txBuffer,HPROT_HDR_COMMAND,pd->myID,dstId,cmd,data,size,HPROT_SET_FNUMBER_DEFAULT);
printTx((char*)txBuffer,n);
serialWrite(ser_fd,txBuffer,n);
}

/**
 * send an answer
 * @param dstId
 * @param cmd
 * @param data
 * @param size
 */
void SerCommTh::sendAnswer(uint8_t dstId, uint8_t cmd, uint8_t size, uint8_t *data)
{
int n=hprotFrameBuild(pd,txBuffer,HPROT_HDR_ANSWER,pd->myID,dstId,cmd,data,size,HPROT_SET_FNUMBER_DEFAULT);
printTx((char*)txBuffer,n);
serialWrite(ser_fd,txBuffer,n);
}

/**
 * send a request
 * @param dstId
 * @param cmd
 * @param data
 * @param size
 * @param nretry
 */
void SerCommTh::sendRequest(uint8_t dstId, uint8_t cmd, uint8_t size, uint8_t *data, uint8_t nretry)
{
int n=hprotFrameBuild(pd,txBuffer,HPROT_HDR_REQUEST,pd->myID,dstId,cmd,data,size,HPROT_SET_FNUMBER_DEFAULT);
_commth_mutex->lock();
hprotReset(pd);
_commth_mutex->unlock();
printTx((char*)txBuffer,n);
serialWrite(ser_fd,txBuffer,n);
// wait for the frame to be sent
//usleep((1000000/BAUDRATE)*9*HPROT_BUFFER_SIZE+100); // an entire frame
usleep((1000000/gd->brate + 1)*9*n+100);
to.startTimeout(timeout);
setCommAnsPending();
result.setWaiting();
}

/**
 * send a generic frame
 * @param hdr
 * @param srcId
 * @param dstId
 * @param cmd
 * @param data
 * @param len
 * @param fnumber
 */
void SerCommTh::sendFrame(uint8_t hdr, uint8_t srcId, uint8_t dstId, uint8_t cmd, uint8_t size, uint8_t *data, int fnumber)
{
int n=hprotFrameBuild(pd,txBuffer,hdr,srcId,dstId,cmd,data,size,fnumber);
printTx((char*)txBuffer,n);
serialWrite(ser_fd,txBuffer,n);
}

/**
 * use this routine to stop the execution and wait for the answer or operation done
 */
void SerCommTh::waitOperation()
{
while(isWaiting())
	{
	usleep(1000);
	}
}
/**
 * return the result of the last operation, for "in progress" operation use waitOperation before!
 * @return true: ok; false: error
 */
bool SerCommTh::getResult()
{
if(isOK()) return true;
return false;
}

//-----------------------------------------------------------------------------
// PRIVATE
//-----------------------------------------------------------------------------
/**
 * wait indefinitely for incoming data
 */
void SerCommTh::waitForData()
{
ndata=serialRead(ser_fd,rxBuffer,1,0);
}
//-----------------------------------------------------------------------------
// mutex protected
//-----------------------------------------------------------------------------
/**
 * change the status of the main state machine to wait an answer
 * @param status ansPandingFlag true or false
 */
void SerCommTh::setCommAnsPending()
{
_commth_mutex->lock();
setStatus(st_ansPending);
_commth_mutex->unlock();
}


/**
 * set the end application status to terminate all
 */
void SerCommTh::endApplicationRequest()
{
cout << "ending thread" << endl;
_commth_mutex->lock();
this->endApplication=true;
_commth_mutex->unlock();
}

/**
 * get the termination status
 * @return true: end; false continue
 */
bool SerCommTh::endApplicationStatus()
{
bool ret;
_commth_mutex->lock();
ret=this->endApplication;
_commth_mutex->unlock();
return ret;
}

/**
 * get the waiting status of the current operation
 * @return true: waiting
 */
bool SerCommTh::isWaiting()
{
bool ret;
_commth_mutex->lock();
if(to.checkTimeout())
	{
	//dbg->trace(DBG_SKTCLIENT,"ERROR: timeout");
	result.setError("timeout",1);
	ret=false;
	}
else
	{
	ret=result.isWaiting();
	}
_commth_mutex->unlock();
return ret;
}

/**
 * ok status of the current operation
 * @return true: OK
 */
bool SerCommTh::isOK()
{
bool ret;
_commth_mutex->lock();
ret=result.isOK();
_commth_mutex->unlock();
return ret;
}

/**
 * error status of the current operation
 * @return true: error
 */
bool SerCommTh::isError()
{
bool ret;
_commth_mutex->lock();
ret=result.isError();
_commth_mutex->unlock();
return ret;
}

/**
 * if an error condition occur, a message can be read
 * note that after a reading the message will be erased
 * @return message error message
 */
string SerCommTh::getErrorMessage()
{
string ret;
_commth_mutex->lock();
ret=result.message;
result.reset();	// clear for next usage
_commth_mutex->unlock();
return ret;
}

/**
 * set the status of the general result
 * @param res status
 */
void SerCommTh::setResultStatus(Result::status_e res)
{
_commth_mutex->lock();
result.setStatus(res);
_commth_mutex->unlock();
}
/**
 * get the status of the result for the operation
 * @return status
 */
Result::status_e SerCommTh::getResultStatus()
{
Result::status_e ret;
_commth_mutex->lock();
ret=result.getStatus();
_commth_mutex->unlock();
return ret;
}

/**
 * print in readable form the data received
 * @param buff
 * @param size
 */
void SerCommTh::printRx(char* buff, int size)
{
if(textMode)
	{
	dbg->trace(DBG_SKTCLIENT_DATA,"RX: %s",buff);
	}
else
	{
	char hex[HPROT_BUFFER_SIZE*2];
	Hex2AsciiHex(hex,(unsigned char*)buff,size,false);
	dbg->trace(DBG_SKTCLIENT_DATA,"RX: (%d) %s",size,hex);
	}
}

/**
 * print in readable form the data transmitted
 * @param buff
 * @param size
 */
void SerCommTh::printTx(char* buff, int size)
{
if(textMode)
	{
	dbg->trace(DBG_SKTCLIENT_DATA,"TX: %s",buff);
	}
else
	{
	char hex[HPROT_BUFFER_SIZE*2];
	Hex2AsciiHex(hex,(unsigned char*)buff,size,false);
	dbg->trace(DBG_SKTCLIENT_DATA,"TX: (%d) %s",size,hex);
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
//_commth_mutex->lock();

// before all I have to check if it is for me or is a gateway frame
if(f.dstID==gd->myID)
	{
	// is for me :)
	switch(f.cmd)
		{
		case HPROT_CMD_CHKLNK:
			sendAnswer(f.srcID,HPROT_CMD_ACK,0,NULL);
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
	srvif->sendFrame(f.hdr,f.srcID,f.dstID,f.cmd,f.data,f.len,gd->forceSend2Unknown);
	}

//_commth_mutex->unlock();
}

