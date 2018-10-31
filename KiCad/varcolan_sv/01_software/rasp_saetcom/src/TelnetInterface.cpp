/*
 *-----------------------------------------------------------------------------
 * PROJECT: rasp_saetcom
 * PURPOSE: see module TelnetInterface.h file
 *-----------------------------------------------------------------------------
 */

#include "TelnetInterface.h"
#include "TelnetInterface.h"

/**
 * ctor
 */
TelnetInterface::TelnetInterface()
{
dataRXcount=0;
}

/**
 * dtor
 */
TelnetInterface::~TelnetInterface()
{
if(startedRx)
	{
	joinThread('r');
	pthread_mutex_destroy(&mutexRx);
	}
if(startedTx)
	{
	joinThread('t');
	pthread_mutex_destroy(&mutexTx);
	}
}

/**
 * check if some dara is received
 * @return 0 no data; else count of bytes
 */
int TelnetInterface::hasRXData()
{
int ret;
pthread_mutex_lock(&mutexRx);
ret = dataRXcount;
pthread_mutex_unlock(&mutexRx);
return ret;
}

/**
 * get data received
 * @param data user buffer for received data
 * @return count of data received and copied in data
 */
int TelnetInterface::getRXData(char* data)
{
pthread_mutex_lock(&mutexRx);
memcpy(data,dataRXbuf,dataRXcount);
dataRXcount=0;	// de-facto, flush the rx buffer
pthread_mutex_unlock(&mutexRx);
}

/**
 * send data
 * @param data
 * @param count
 */
void TelnetInterface::sendData(const char* data, int count)
{
SimpleClient::writeData(data,count);
waitTimeCounter=0;
}

/**
 * wait for received data or till timeCounter expired
 * note: wait for about 100 ms each call
 * @return true: data received; false: timeout
 */
bool TelnetInterface::waitAnswer(int timeCounter)
{
bool ret;
waitTimeCounter=timeCounter;
while(waitTimeCounter<timeCounter)
	{
	if(waitTimeCounter>0) waitTimeCounter--;
	usleep(100000);
	pthread_mutex_lock(&mutexRx);
	ret = (dataRXcount>0) ? (true) : (false);
	pthread_mutex_unlock(&mutexRx);
	}
return ret;
}
//-----------------------------------------------------------------------------
// PRIVATE
//-----------------------------------------------------------------------------

/**
 * write data
 * @param data
 * @param count
 */
void TelnetInterface::writeData(const char* data, int count)
{
SimpleClient::writeData(data,count);
}

/**
 * on read data event from supervisor telnet interface
 * @param data
 * @param count
 */
void TelnetInterface::onDataRead(char* data, int count)
{
pthread_mutex_lock(&mutexRx);
memcpy(dataRXbuf,data,count);
dataRXcount=count;
pthread_mutex_unlock(&mutexRx);

#if 0
uint8_t dbuff[100];
string ds(data);
Split(ds,toks,"|");

if(toks[0]=="ev")
	{
	Event_t ev;
	AsciiHex2Hex((uint8_t *)&ev,(char *)toks[1].c_str(),sizeof(Event_t));
	TRACE(dbg,DBG_NOTIFY,"New event (%d)",ev.Parameters_s.eventCode);
	switch(ev.Parameters_s.eventCode)
		{
		case transit:
			dbuff[0]=WORD_MSB(ev.Parameters_s.idBadge);
			dbuff[1]=WORD_LSB(ev.Parameters_s.idBadge);
			dbuff[2]=0;
			dbuff[3]=WORD_MSB(ev.Parameters_s.terminalId);
			sc->buildEvent(59,dbuff,4,&tmpdg);
			sc->datagramQueueTX.push(tmpdg);
			break;
		case pin_changed:
		case badges_deleted:
		case start_enabling:
		case stop_enabling:
		case checkin:
		case transit_with_code:
		case checkin_with_code:
		case out_of_area:
		case disable_in_terminal:
		case no_checkin_weektime:
		case no_valid_visitor:
		case coercion:
		case not_present_in_terminal:
		case terminal_not_inline:
		case terminal_inline:
		case forced_gate:
		case opendoor_timeout:
		case stop_opendoor_timeout:
		case alarm_aux:
		case stop_alarm_aux:
		case low_voltge:
		case stop_low_voltge:
		case start_tamper:
		case stop_tamper:
		case wrong_pin:
		case broken_terminal:
//		case low_battery:
//		case critical_battery:
//		case no_supply:
//		case supply_ok:
		case transit_button:
		case fw_installed:
		case wrong_access:
			break;
		default:
			TRACE(dbg,DBG_WARNING,"Unknown event (%d)",ev.Parameters_s.eventCode);
			break;
		}
	// TODO
	}
else
	{

	}
#endif
}


//-----------------------------------------------------------------------------
// THREADS
//-----------------------------------------------------------------------------

/**
 * THREAD RX JOB
 * which is executed in the thread
 */
void TelnetInterface::runRx()
{
//while (!endApplicationStatus())
while(1)
	{
	mainLoop();
	}
startedRx=false;
destroyThread('r');
cout << "Communication RX tread stopped" << endl;
}

/**
 * THREAD TX JOB
 * which is executed in the thread
 */
void TelnetInterface::runTx()
{
startedTx=false;
destroyThread('t');
cout << "Communication TX tread stopped" << endl;
}

/**
 * start the execution of the thread
 * @param arg (Uses default argument: arg = NULL)
 * @param which can be 't' for tx or 'r' for rx
 */
bool TelnetInterface::startThread(void *arg,char which)
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
void TelnetInterface::joinThread(char which)
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
 * destroy the thread
 * @param which can be 't' for tx or 'r' for rx
 */
void TelnetInterface::destroyThread(char which)
{
int ret;
pthread_t *_id;
if(which=='r')
	{
	_id=&_idRx;
	}
else
	{
	_id=&_idTx;
	}

pthread_detach(*_id);
pthread_exit(0);

if(!noattrRx)
	{
	if((ret = pthread_attr_destroy(&_attrRx)) != 0)
		{
		cout << strerror(ret) << endl;
		throw "Error";
		}
	}
}

/**
 * Function that is used to be executed by the thread
 * @param thr
 */
void *TelnetInterface::execRx(void *thr)
{
reinterpret_cast<TelnetInterface *>(thr)->runRx();
return NULL;
}

/**
 * Function that is used to be executed by the thread
 * @param thr
 */
void *TelnetInterface::execTx(void *thr)
{
reinterpret_cast<TelnetInterface *>(thr)->runTx();
return NULL;
}

