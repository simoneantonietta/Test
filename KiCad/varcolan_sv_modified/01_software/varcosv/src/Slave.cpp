/*
 *-----------------------------------------------------------------------------
edwe * PROJECT: varcosv
 * PURPOSE: see module Slave.h file
 *-----------------------------------------------------------------------------
 */

#include "Slave.h"
#include "dataStructs.h"
#include "comm/app_commands.h"
#include "comm/CommTh.h"
#include "common/utils/Trace.h"
#include "db/DBWrapper.h"
#include "common/utils/SimpleCfgFile.h"
#include "HardwareGPIO.h"
#include "BadgeReader.h"
#include "TelnetServer.h"
#include "Scheduler.h"
#include "UDPProtocolDebug.h"

#ifdef RASPBERRY
extern HardwareGPIO *hwgpio;
#endif

#define RESULT_TXT_FILE		"__tmpres.txt"

extern struct globals_st gd;
extern CommTh *comm;
extern Trace *dbg;
extern DBWrapper *db;
extern SimpleCfgFile *cfg;
extern BadgeReader *br;
extern TelnetServer *tsrv;
extern Scheduler *sched;
extern UDPProtocolDebug *udpDbg;

/**
 * ctor
 */
Slave::Slave()
{
webCommandInProgress=false;
resetGlobalStatus();

//fwCheckCounter=(FIRMWARE_CHECK_TIME*1000000) / gd.loopLatency;
fwCheckCounter=(FIRMWARE_CHECK_TIME*1000000) / 500000;
scriptCounter=(SCRIPT_CHECK_TIME*1000000) / 500000;
oneSecondCounter=(1*1000000) / 500000;
halfSecondCounter=(5*100000) / 500000;
oneMinCounter=60;
oneHourCounter=3600;
actualType=DEVICE_TYPE_NONE;
dbSaveCounter=12000;
gd.masterQueueCounter=0;
msgQueueDepth_prev=0;
//for(int i=0;i<MAX_DEVICE_ID;i++)
//	{
//	dataDevice[i]
//	}
}

/**
 * dtor
 */
Slave::~Slave()
{
for(globals_st::iddevice_t it = gd.deviceList.begin();it != gd.deviceList.end();it++)
	{
	delete it->second.dataEventQueue;
	}
}

/**
 * push an event
 * @param ev event
 * @param dest 0=to all; else specify the destination ID
 */
void Slave::pushEvent(globals_st::dataEvent_t ev, int dest)
{
if(dest==0)	// to all master
	{
	for(globals_st::iddevice_t it = gd.deviceList.begin();it != gd.deviceList.end();it++)
		{
		// iterator->first = key
		// iterator->second = value
		//it->second.dataEventQueue.push(ev);
		it->second.dataEventQueue->push_back(ev);
		}
	}
else // to a specific master
	{
	if(gd.deviceList.find(dest)!=gd.deviceList.end())
		{
		//gd.deviceList[dest].dataEventQueue.push(ev);
		gd.deviceList[dest].dataEventQueue->push_back(ev);
		}
	}
}

/**
 * clear all events in the queue
 * @param dest 0=to all; else specify the destination ID
 */
void Slave::clearEvent(int dest)
{
if(dest==0)	// to all master
	{
	for(globals_st::iddevice_t it = gd.deviceList.begin();it != gd.deviceList.end();it++)
		{
		it->second.dataEventQueue->clear();
		}
	}
else // to a specific master
	{
	if(gd.deviceList.find(dest)!=gd.deviceList.end())
		{
		//gd.deviceList[dest].dataEventQueue.push(ev);
		gd.deviceList[dest].dataEventQueue->clear();
		}
	}
}

/**
 * check if an event was already insert
 * @param id device
 * @param et event type
 * @return true: event already present
 */
bool Slave::checkIfEventExists(int id, globals_st::dataEvent_t::dataEventType_e et)
{
for(circular_buffer<globals_st::dataEvent_t>::iterator it=gd.deviceList[id].dataEventQueue->begin();it != gd.deviceList[id].dataEventQueue->end();it++)
	{
	if(it->dataEvent==et)
		{
		return true;
		}
	}
return false;
}

/**
 * check if a usb key is inserted
 * @return
 */
void Slave::recoveryCheck()
{
#ifdef RASPBERRY
string cmd="";
FILE * devdata=popen("sudo blkid | grep -c sda", "r");
// result: 1 or 0
char buffer[100];
char *line_p = fgets(buffer, sizeof(buffer), devdata);
for(unsigned int i=0;i<strlen(line_p);i++) if(line_p[i]=='\n') line_p[i]=0;	// terminates
if(line_p[0]=='1') // found an usb key
	{
	TRACE(dbg,DBG_NOTIFY,"found USB key");
	system("mkdir /tmp/mnt");
	sleep(2);
	system("sudo mount /dev/sda1 /tmp/mnt");
	sleep(5);
	if(fileExists(SCRIPT_RECOVERY_FNAME))
		{
		TRACE(dbg,DBG_NOTIFY,"found recovery file...executing");
		slave->runScriptFile(SCRIPT_RECOVERY_FNAME,SCRIPT_RECOVERY_STATUS_FNAME,true);
		putDBEvent(recovery_factory_settings,DEV_BASEID_SUPERVISOR);
		sleep(1);
		HW_BLINK_2PULSES();
		TRACE(dbg,DBG_NOTIFY,"remove the usb key and reboot manually the supervisor");
		while(1)
			{
			sleep(10);	// wait indefinitely (it expects you remove the usb key and reset the device)
			}
		}
	}
#endif
}

/**
 * put a DB event
 * @param evcode
 * @param id
 */
void Slave::putDBEvent(EventCode_t evcode, int id)
{
Event_t _ev;
_ev.Parameters_s.eventCode=evcode;
_ev.Parameters_s.idBadge=65535;
_ev.Parameters_s.area=255;
_ev.Parameters_s.causal_code=65535;
_ev.Parameters_s.terminalId=id;
_ev.Parameters_s.timestamp=getSystemTime();
db->addEvent(_ev);
}

/**
 * main loop to run in main
 * this should run into a loop
 */
void Slave::mainLoop()
{
//=============================================================================
// STATE MACHINE  (one for each connected master)
// (status is typically altered from WEB or forced from other interfaces
if(!gd.frameQueue.empty())
	{
	/*
	 * calculates the number of messages to be handled each step.
	 * the value changes with the queue depth: greater is the depth , greater is the amount of messages handled
	 */
	pthread_mutex_lock(&gd.mutexFrameQueue);
	int qsz=gd.frameQueue.size();
	pthread_mutex_unlock(&gd.mutexFrameQueue);

	msgs2work=(qsz >= MSG_QUEUE_THRESHOLD) ? ((qsz / MSG_QUEUE_THRESHOLD + MSG_QUEUE_MIN) * MSG_QUEUE_MULTIPLIER) : (MSG_QUEUE_MIN);

	if(gd.frameQueue.size() > MSG_QUEUE_THRESHOLD_ALERT)
		{
		if(!isInRange<int>(qsz,msgQueueDepth_prev-5,msgQueueDepth_prev+5,'B'))
			{
			dbg->trace(DBG_WARNING,"msgs queue depth: %d (polling %d ms)", qsz,gd.masterPollingTime);
			msgQueueDepth_prev=qsz;
			}
		else
			{
			// to allow reprinting of some message ifthe situation is less busy
			if(msgQueueDepth_prev > MSG_QUEUE_THRESHOLD_ALERT) msgQueueDepth_prev--;
			}
		}
	if(qsz > MSG_QUEUE_TRIGGER_DELETE)
		{
		dbg->trace(DBG_WARNING,"too many msgs (%d) in the queue (>%d) -> delete all",qsz, MSG_QUEUE_TRIGGER_DELETE);
		// to delete it swap whit and empty version of the container
//		std::queue<globals_st::frameData_t> empty;
//		std::swap(gd.frameQueue, empty );
		while(!gd.frameQueue.empty())
			{
			if(pthread_mutex_trylock(&gd.mutexFrameQueue)==0)
				{
				gd.frameQueue.pop();
				pthread_mutex_unlock(&gd.mutexFrameQueue);
				}
			}
		return;
		}

	// handle queue depth and polling time relation
	pollingTimeModulator(qsz);

	for(;msgs2work>0 && !gd.frameQueue.empty();msgs2work--)
		{
		pthread_mutex_lock(&gd.mutexFrameQueue);
		f=gd.frameQueue.front();	// preview to get ID
		gd.frameQueue.pop();
		pthread_mutex_unlock(&gd.mutexFrameQueue);

		masterListCheck();	// update master list if not present
		// check if is online, if not delete its message to avoid accumulation of answers
//		if(gd.deviceList[f.frame.srcID].aliveCounter==0)	// if device dead
//			{
//			dbg->trace(DBG_NOTIFY,"message from id %d will be ignored",f.frame.srcID);
//			continue;
//			}
		resetAliveCounter(f.frame.srcID,gd.masterAliveTime);	// reset the alive counter

		switch(gd.deviceList[f.frame.srcID].mainState)
			{
			case globals_st::sl_idle:
				worker_normal();
				break;
			case globals_st::sl_terminal_request:
				worker_terminalReq(f.frame.srcID);
				break;
			case globals_st::sl_newkey_send:
				worker_newKey(f.frame.srcID);
				break;
			}
		usleep(2000);	// to release some resources every message handled
		}
	}
//=============================================================================
// HANDLE WEB COMMANDS
if(webCommandInProgress)
	{
	// check if some device is in timeout
	for(globals_st::iddevice_t it = gd.deviceList.begin();it != gd.deviceList.end();it++)
		{
		if(it->second.aliveCounter>0)	// Only alive devices are considered
			{
			if(it->second.timeout_started)
				{
				if(it->second.globTimeout.checkTimeout())
					{
					if(gd.expected_glob_mainState != it->second.mainState || gd.expected_glob_workerState != it->second.workerState)
						{
						//resetDeviceStatus(it->first);
						//webCommandInProgress=false;
						dbg->trace(DBG_WARNING,"worker operation timeout for ID %d (wkr status %d)", it->first,it->second.workerState);
						it->second.timeout_started=false;
						}
					}
				}
			}
		}
	// check the global status
#define WEBCMD_OK_MSG			"webcmd ok\n"
#define WEBCMD_ERROR_MSG	"webcmd error\n"
	if(checkGlobalStatus())
		{
		dbg->trace(DBG_NOTIFY,"web command DONE");
		//db->setResult("DONE");
		tsrv->writeData(gd.cmd_fromClient,WEBCMD_OK_MSG,sizeof(WEBCMD_OK_MSG));
		webCommandInProgress=false;
		resetDeviceStatus();
		resetGlobalStatus();
		}
	else if(webCmdTimeout.checkTimeout())
		{
		dbg->trace(DBG_WARNING,"worker global timeout expired");
		tsrv->writeData(gd.cmd_fromClient,WEBCMD_ERROR_MSG,sizeof(WEBCMD_ERROR_MSG));
		webCommandInProgress=false;
		resetDeviceStatus();
		resetGlobalStatus();
		}
	}

//=============================================================================
// TIMED OPERATIONS

//...................................
// check for new firmware (not always)
if(fwCheckCounter>0) fwCheckCounter--;
if(fwCheckCounter==0)
	{
	globals_st::dataEvent_t de=firmwareCheckForNew();
	if(de.dataEvent==globals_st::dataEvent_t::ev_fw_new_version || de.dataEvent==globals_st::dataEvent_t::ev_bl_new_version)
		{
		pushEvent(de,0);
		}
	// recharges the counter
	fwCheckCounter=(FIRMWARE_CHECK_TIME*1000000) / gd.loopLatency;
	}

//...................................
// check for script to be executed (or recovery)
if(scriptCounter>0) scriptCounter--;
if(scriptCounter==0)
	{
	recoveryCheck();

	runScriptFile(SCRIPT_FILENAME,SCRIPT_STATUS_FNAME);
	// recharges the counter
	scriptCounter=(SCRIPT_CHECK_TIME*1000000) / gd.loopLatency;
	}

//...................................
// save database to SD card [s]
if(dbSaveCounter==0)
	{
	db->dbSave();
	dbSaveCounter=gd.dbSaveTime;
	}


//...................................
// generic 1 second counter
if(oneSecondCounter>0) oneSecondCounter--;
if(oneSecondCounter==0)
	{
	//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
	// 1 second routine
	//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
	if(dbSaveCounter>0) dbSaveCounter--;

	// update alive counters
	updateAllAliveCounters();
	checkAliveCounter(0);

	// handle timed requests
	handleTimedRequests();
	//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//...................................
// generic 1 min counter
	if(oneMinCounter>0) oneMinCounter--;
	if(oneMinCounter==0)
		{
		//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
		// 1 min routine
		//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
		handleScheduledJobs();
		//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
		oneMinCounter=60;
		}

//...................................
// generic 1 hour counter
	if(oneHourCounter>0) oneHourCounter--;
	if(oneHourCounter==0)
		{
		//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
		// 1 hour routine
		//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
		db->checkEventTable();
#ifdef RASPBERRY
		dbg->trace(DBG_NOTIFY,"current date/time (from RTC): "+webCmdTimeout.datetime_fmt("%D %T",hwgpio->getRTC_datetime()));
#endif
		//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
		oneHourCounter=3600;
		}

	oneSecondCounter=(1*1000000) / gd.loopLatency;
	}

//...................................
// generic half second counter
if(halfSecondCounter>0) halfSecondCounter--;
if(halfSecondCounter==0)
	{
	//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
	// 500 ms routine
	//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
	handleBadgeReader();

	//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
	halfSecondCounter=(5*100000) / gd.loopLatency;
	}
}

//-----------------------------------------------------------------------------
// PRIVATE
//-----------------------------------------------------------------------------

/**
 * handle badge reader
 * NOTE: a badge can be loaded using a terminal, please
 * see HPROT_CMD_SV_BADGE_REQUEST command interpretation
 */
void Slave::handleBadgeReader()
{
if(gd.badgeAcq.waitingForBadge)
	{
	if(gd.badgeAcq.wait_timeout > 0)
		{
		gd.badgeAcq.wait_timeout--;
		}
	else
		{
		TRACE(dbg,DBG_WARNING,"waiting for badge timeout");
		gd.badgeAcq.waitingForBadge=false;
		gd.badgeAcq.wait_timeout=WAIT_FOR_BADGE_TIMEOUT;
		return;
		}

	if(gd.has_reader)
		{
		if(br->hasBadge())
			{
			//check for presnece

			uint8_t binform1[BADGE_SIZE],binform2[BADGE_SIZE];
			char p[100];
			gd.badgeAcq.badge_size=br->getBadge(gd.badgeAcq.badge);
			gd.badgeAcq.waitingForBadge=false;
			gd.badgeAcq.badge[gd.badgeAcq.badge_size]=0;
			for(int k=strlen(gd.badgeAcq.badge);k<BADGE_SIZE*2;k++)
				{
				strcat(gd.badgeAcq.badge,"F");
				}
			strcpy(p,gd.badgeAcq.badge);
			TRACE(dbg,DBG_DEBUG,"badge uid sent to web: %s",p);
			strcat(p,"\n");
			//sprintf(p,"%s\n",gd.badgeAcq.badge);
			tsrv->writeData(gd.cmd_fromClient,p,strlen(p));
			TRACE(dbg,DBG_NOTIFY,"sending badge (%s) to telnet client %d",gd.badgeAcq.badge,gd.cmd_fromClient);
			}
		}
	else
		{
		// attempt to reconnect to the reader
		gd.has_reader=true; // force to try reconnection
		if(!br->init(gd.reader_device,gd.reader_brate))
			{
			//dbg->trace(DBG_WARNING,"reader disabled or not present");
			gd.has_reader=false;
			}
		else
			{
			dbg->trace(DBG_NOTIFY,"reader enabled");
			br->reset();
			br->startThread(NULL);
			gd.has_reader=true;
			}
		}
	}
}

/**
 * reset global status
 */
void Slave::resetGlobalStatus()
{
gd.glob_mainState=gd.expected_glob_mainState=globals_st::sl_unk;
gd.glob_workerState=gd.expected_glob_workerState=globals_st::wkr_unk;
}

/**
 * reset devices statuses to idle
 * @param if id <0 the all devices are reset(default); else specifies the ID to be reset
 */
void Slave::resetDeviceStatus(int id)
{
if(id<0)
	{
	for(globals_st::iddevice_t it = gd.deviceList.begin();it != gd.deviceList.end();it++)
		{
		it->second.mainState=globals_st::sl_idle;
		it->second.workerState=globals_st::wkr_idle;
		it->second.timeout_started=false;
		webCommandInProgress=false;
		}
	}
else
	{
	gd.deviceList[id].mainState=globals_st::sl_idle;
	gd.deviceList[id].workerState=globals_st::wkr_idle;
	gd.deviceList[id].timeout_started=false;
	}
}

/**
 * set all devices to a particular state, if specified, else is idle
 * @param mainstate
 * @param workerstate
 */
void Slave::setAllDeviceStatus(enum globals_st::slaveStates_e mainstate,enum globals_st::workerStates_e workerstate)
{
for(globals_st::iddevice_t it = gd.deviceList.begin();it != gd.deviceList.end();it++)
	{
	it->second.mainState=mainstate;
	it->second.workerState=workerstate;
	}
}

//.................................
// WORKERS (one for each master)
//.................................

/**
 * run in idle state
 */
void Slave::worker_normal()
{
uint32_t val;
fwit_t it;
Badge_parameters_t bpar;
Terminal_parameters_to_send_t tpar;
TerminalStats_t tstats;
uint8_t badgeUID[BADGE_SIZE*2+1];
CausalCodes_parameters_t *causalcd;
Badge_parameters_t *badgedata;
EventRequest_t *evreq;

switch(f.frame.cmd)
	{
	case HPROT_CMD_CHKLNK:
		// messages queue flush because it is reconnecting
		gd.deviceList[f.frame.srcID].dataEventQueue->clear();
		gd.masterPollingTime=gd.masterPollingTime_def;			// recover the default polling time to resync all devices

		// see if need to signaling the reconnection
		if(gd.deviceList[f.frame.srcID].deadSignaled)
			{
			TRACE(dbg,DBG_DEBUG,"device %d reconnection",f.frame.srcID);
			putDBEvent(terminal_inline,f.frame.srcID);
			}
		break;
	//.................................................
	case HPROT_CMD_SV_CHECK_CRC_P_W:	// polling
		//masterListCheck();
		handleInternalEvents();
		break;
	//.................................................
	case HPROT_CMD_SV_WEEKTIME_CRC_LIST:
		weektimeCheck();
		break;
	//.................................................
	case HPROT_CMD_SV_PROFILE_CRC_LIST:
		profileCheck();
		dbg->trace(DBG_NOTIFY,"Profile CRC list from %d",(int)f.frameData[0]);
		break;
	//.................................................
	case HPROT_CMD_SV_NEW_EVENT:
		handleVarcoEvent();
		break;
	//.................................................
	case HPROT_CMD_SV_REQUEST_CURRENT_AREA:
		handleRequestArea();
		break;
	//.................................................
	case HPROT_CMD_SV_UPDATE_PIN:
		// FIX: perhaps is quite similar to HPROT_CMD_SV_UPDATE_BADGE
		if(db->changeBadgePinFromId((int)f.frameData[0],to_string(*(uint32_t*)&f.frameData[1])))
			{
			if(f.frame.hdr==HPROT_HDR_REQUEST)
				{
				comm->sendAnswer(f.frame.srcID,HPROT_CMD_ACK,0,0,f.frame.num);
				}
			globals_st::dataEvent_t de;
			de.dataEvent=globals_st::dataEvent_t::ev_badge_data;
			de.tag.badge_id=(int)f.frameData[0];
			pushEvent(de,0);
			TRACE(dbg,DBG_NOTIFY, "badge %d propagating",(int)f.frameData[0]);
			}
		else
			{
			if(f.frame.hdr==HPROT_HDR_REQUEST)
				{
				comm->sendAnswer(f.frame.srcID,HPROT_CMD_NACK,0,0,f.frame.num);
				}
			}
		break;
		//.................................................
		case HPROT_CMD_SV_UPDATE_BADGE:
			// FIX: perhaps is quite similar to HPROT_CMD_SV_UPDATE_PIN
			{
			badgedata=(Badge_parameters_t *) f.frameData;
			db->setBadge(*badgedata);
			if(f.frame.hdr==HPROT_HDR_REQUEST)
				{
				comm->sendAnswer(f.frame.srcID,HPROT_CMD_ACK,0,0,f.frame.num);
				}
			globals_st::dataEvent_t de;
			de.dataEvent=globals_st::dataEvent_t::ev_badge_data;
			de.tag.badge_id=(int)f.frameData[0];
			pushEvent(de,0);
			TRACE(dbg,DBG_NOTIFY, "badge %d propagating",(int)f.frameData[0]);
			}
			break;

	//.................................................
	case HPROT_CMD_SV_UPDATE_CAUSAL_CODES:
		for(int i=0;i<MAX_CAUSAL_CODES;i++)
			{
			causalcd=(CausalCodes_parameters_t*) (f.frameData+i*sizeof(CausalCodes_parameters_t));
			db->setCausal(i+1,causalcd->Parameters_s.causal_Id,_S causalcd->Parameters_s.description);
			}
		if(f.frame.hdr==HPROT_HDR_REQUEST)
			{
			comm->sendAnswer(f.frame.srcID,HPROT_CMD_ACK,0,0,f.frame.num);
			}
		// reload and calculates CRC to propagates changes
		db->loadCausals();
		break;
	//.................................................
	case HPROT_CMD_SV_EVENT_LIST_REQUEST:
		{
		vector<Event_t> evts;
		evreq=(EventRequest_t *)f.frameData;
		unsigned int nevts=db->getEvents(evreq,&evts);
		if(evreq->start_event<nevts)
			{
			int cnt=0;
			unsigned int evndx=evreq->start_event;
			int frame_nevmax=(HPROT_PAYLOAD_SIZE-5)/sizeof(Event_t);
			*(uint16_t *)txData=evreq->id_badge;
			(nevts>REPORT_N_EVENTS_MAX) ? (txData[2]=0xFF) : (txData[2]=(uint8_t)nevts);
			txData[3]=evreq->start_event;
			while(frame_nevmax-- && evndx<nevts)
				{
				Event_t e=evts[evndx++];
				memcpy(&txData[5]+sizeof(Event_t)*cnt,&e,sizeof(Event_t));
				cnt++;
				}
			txData[4]=cnt;
			}
		else
			{
			// wrong index
			if(f.frame.hdr==HPROT_HDR_REQUEST)
				{
				comm->sendAnswer(f.frame.srcID,HPROT_CMD_NACK,0,0,f.frame.num);
				}
			}
		}
		break;
	//.................................................
	case HPROT_CMD_SV_FW_VERSION:
		// check if the version is above the currently installed one in the master
		// this answer arrives only under request
		if(firmwareCheckVersion(f.frameData[0],f.frameData[1],f.frameData[2]))
			{
			comm->sendAnswer(f.frame.srcID,HPROT_CMD_NACK,0,0,f.frame.num);
			}
		else
			{
			comm->sendAnswer(f.frame.srcID,HPROT_CMD_ACK,0,0,f.frame.num);
			}
		break;
	//.................................................
	case HPROT_CMD_SV_METADATA_FW:
		it=find(firmwares.begin(),firmwares.end(),f.frameData[0]);
		if(it!=firmwares.end())
			{
			comm->sendAnswer(f.frame.srcID,HPROT_CMD_SV_METADATA_FW,(uint8_t *)&it->fwMetadata,sizeof(FWMetadata_t),f.frame.num);
			}
		else
			{
			comm->sendAnswer(f.frame.srcID,HPROT_CMD_NACK,0,0,f.frame.num);
			}
		break;
	//.................................................
	case HPROT_CMD_SV_FW_CHUNK_REQ:
		val=*(uint32_t*)&f.frameData[1];
		if(!firmwareChunkSend(f.frameData[0],val,f.frameData[5]))
			{
			comm->sendAnswer(f.frame.srcID,HPROT_CMD_NACK,0,0,f.frame.num);
			}
		break;
	//.................................................
	case HPROT_CMD_SV_BADGE_REQUEST:
		db->normalizeBadge(&f.frameData[1],f.frame.len-1,badgeUID);
		if(gd.badgeAcq.waitingForBadge && (gd.terminalAsReader_id==f.frame.srcID || gd.terminalAsReader_id==f.frame.data[0]))
			{
			// interpret this as a read from terminal to store a new badge
			char b[BADGE_SIZE*2+1],p[100];
			Hex2AsciiHex(b,badgeUID,BADGE_SIZE,false,0);
			gd.badgeAcq.badge_size=BADGE_SIZE;
			gd.badgeAcq.waitingForBadge=false;
			memcpy(gd.badgeAcq.badge,b,BADGE_SIZE*2);
			gd.badgeAcq.badge[f.frame.len*2]=0;
			strcpy(p,gd.badgeAcq.badge);
			strcat(p,"\n");
			tsrv->writeData(gd.cmd_fromClient,p,strlen(p));
			TRACE(dbg,DBG_NOTIFY,"sending badge (%s) to telnet client %d (id %d)",gd.badgeAcq.badge,gd.cmd_fromClient,gd.terminalAsReader_id==f.frame.data[0]);
			comm->sendAnswer(f.frame.srcID,HPROT_CMD_ACK,0,0,f.frame.num);
			}
		else
			{
			if(db->getBadgeFromUID(badgeUID,bpar))
				{
				comm->sendAnswer(f.frame.srcID,HPROT_CMD_SV_BADGE_REQUEST,(uint8_t *)&bpar,sizeof(Badge_parameters_t),f.frame.num);
				}
			else
				{
				comm->sendAnswer(f.frame.srcID,HPROT_CMD_NACK,0,0,f.frame.num);
				}
			}
		break;
	//.................................................
	case HPROT_CMD_SV_MEMORY_DUMP:
		handleMasterDump();
		break;
	//.................................................
	case HPROT_CMD_SV_PRESENT_TERMINALS:
		memcpy(gd.deviceList[f.frame.srcID].terminalsData.terminalsPresent,f.frameData,MAX_TERMINALS);
		// convert in ascii string
		for(int j=0;j<MAX_TERMINALS;j++)
			{
			f.frameData[j] += 0x30;
			}
		f.frameData[MAX_TERMINALS]=0;
		dbg->trace(DBG_NOTIFY,"terminal list from master %d: %s",f.frame.srcID,f.frameData);
		break;
	//.................................................
	case HPROT_CMD_SV_TERMINAL_DATA:
		memcpy(&tpar,f.frameData,sizeof(Terminal_parameters_to_send_t));
		//db->setTerminal(gd.terminals[gd.deviceList[f.frame.srcID].terminalsData.ndx].name,gd.terminals[gd.deviceList[f.frame.srcID].terminalsData.ndx].status,tpar);
		db->setTerminal("",gd.terminals[gd.deviceList[f.frame.srcID].terminalsData.ndx].status,tpar);		dbg->trace(DBG_NOTIFY,"terminal data from id %d",f.frame.srcID);
		break;
	//.................................................
	case HPROT_CMD_SV_TERMINAL_STATS:
		memcpy(&tstats,f.frameData,sizeof(TerminalStats_t));
		TRACE(dbg,DBG_NOTIFY,"device %-2d crcErrors: lan[%-3d] net[%-3d] res[%-3d] to[%-3d]",f.frame.srcID,
															tstats.lanCRCError,
															tstats.networkCRCError,
															tstats.reservedCRCError,
															tstats.SVCommTimeout
															);
		break;
	}
}

/**
 * handle the terminal request data
 * @param master id
 */
void Slave::worker_terminalReq(int id)
{
Terminal_parameters_to_send_t tpar;
bool sendBusy=false;
bool found=false;
unsigned int dpos,cnt;

usleep(10000);

switch(gd.deviceList[id].workerState)
	{
	case globals_st::wkr_idle:
		break;
	//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
	case globals_st::wkr_nterm_init:
		gd.deviceList[id].retry=WKR_N_RETRY;
		//break;
	//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
	case globals_st::wkr_nterm_req:
		dbg->trace(DBG_NOTIFY, "procedure terminal data request started");
		gd.deviceList[id].globTimeout.startTimeout(WKR_TIMEOUT_TERMINALS*1000);
		gd.deviceList[id].timeout_started=true;
		switch(f.frame.cmd)
			{
			//.................................................
			case HPROT_CMD_SV_CHECK_CRC_P_W:	// polling
				txData[0]=(uint8_t)sv_present_terminals_req;
				comm->sendAnswer(f.frame.srcID,f.frame.cmd,txData,1,f.frame.num);
				// start request timeout
				gd.deviceList[id].reqTimeout.startTimeout(WKR_REQUEST_TIMEOUT*1000);
				gd.deviceList[id].workerState=globals_st::wkr_nterm_wait;
				break;
			default:
				sendBusy=true;
				break;
			}
		break;

	//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
	case globals_st::wkr_nterm_wait:
		/*
		 * wait the answer
		 */
		switch(f.frame.cmd)
			{
			//.................................................
			case HPROT_CMD_SV_PRESENT_TERMINALS:
				memcpy(gd.deviceList[id].terminalsData.terminalsPresent,f.frameData,MAX_TERMINALS);
				gd.deviceList[id].terminalsData.ndx=0; // start scan from 0
				gd.deviceList[id].workerState=globals_st::wkr_nterm_param_req;
				break;
			default:
				sendBusy=true;
				break;
			}

		// handle request timeout and retry
		if(gd.deviceList[id].reqTimeout.checkTimeout())
			{
			if(gd.deviceList[id].retry>0)
				{
				dbg->trace(DBG_NOTIFY,"wrong or no answer (list) from id %d -> retry",id);
				gd.deviceList[id].retry--;
				gd.deviceList[id].workerState=globals_st::wkr_nterm_req;
				}
			else
				{
				dbg->trace(DBG_NOTIFY,"wrong or no answer (list) from id %d -> stop",id);
				gd.deviceList[id].workerState=globals_st::wkr_nterm_param_end;
				}
			}
		break;
	//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
	case globals_st::wkr_nterm_param_req:
		/*
		 * send the request for every terminal
		 */
		switch(f.frame.cmd)
			{
			//.................................................
			case HPROT_CMD_SV_CHECK_CRC_P_W:	// polling
				txData[0]=(uint8_t)sv_terminal_data_req;
				gd.deviceList[id].terminalsData.prev_ndx=gd.deviceList[id].terminalsData.ndx;
				dpos=1;
				cnt=0;
				// creates the list
				for(;gd.deviceList[id].terminalsData.ndx<MAX_TERMINALS;gd.deviceList[id].terminalsData.ndx++)
					{
					if(gd.deviceList[id].terminalsData.terminalsPresent[gd.deviceList[id].terminalsData.ndx]==1)
						{
						txData[dpos]=gd.deviceList[id].terminalsData.ndx+1;	// start from 1
						dpos++;
						cnt++;
						found=true;
						dbg->trace(DBG_NOTIFY,"request data for terminal %d",id);
						if(cnt >= WKR_MAX_TERMINALDATA_PER_FRAME)
							{
							txData[dpos]=gd.deviceList[id].terminalsData.ndx++;
							break;
							}
						}
					}
				if(found)
					{
					comm->sendAnswer(f.frame.srcID,f.frame.cmd,txData,dpos,f.frame.num);
					// start request timeout
					gd.deviceList[id].reqTimeout.startTimeout(WKR_REQUEST_TIMEOUT*1000);
					// restart the global timeout
					gd.deviceList[id].globTimeout.startTimeout(WKR_TIMEOUT_TERMINALS*1000);
					gd.deviceList[id].timeout_started=true;

					gd.deviceList[id].workerState=globals_st::wkr_nterm_param_wait;
					}
				else
					{
					// no more requests
					gd.deviceList[id].workerState=globals_st::wkr_nterm_param_end;
					sendBusy=true;
					}
				break;
			default:
				sendBusy=true;
				break;
			}
		break;
	//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
	case globals_st::wkr_nterm_param_wait:
		/*
		 * wait terminal data previously requested
		 */
		switch(f.frame.cmd)
			{
			//.................................................
			case HPROT_CMD_SV_TERMINAL_DATA:
				gd.deviceList[id].reqTimeout.startTimeout(WKR_REQUEST_TIMEOUT*1000);
				// list of devices (if id==0 is invalid)
				for(unsigned int n=0;n<(f.frame.len/sizeof(Terminal_parameters_to_send_t));n++)
					{
					dpos=n*sizeof(Terminal_parameters_to_send_t);
					memcpy(&tpar,&f.frameData[dpos],sizeof(Terminal_parameters_to_send_t));

					if(tpar.terminal_id == 0)
						{
						dbg->trace(DBG_WARNING,"one terminal that was in the list, now is not available");
						if(gd.terminals[gd.deviceList[id].terminalsData.ndx].status!=TERMINAL_STATUS_NOT_PRESENT)
							{
							db->setTerminalStatus(tpar.terminal_id,TERMINAL_STATUS_OFF_LINE);
							}
						//db->removeTerminal(tpar.terminal_id);
						}
					else
						{
						// write to database
						dbg->trace(DBG_NOTIFY,"terminal data of id %d",tpar.terminal_id);
//						char _tmp[100];
//						sprintf(_tmp,"term data id %d\n",tpar.terminal_id);
//						tsrv->writeData(gd.cmd_fromClient,_tmp,strlen(_tmp));
						//gd.terminals[gd.deviceList[id].terminalsData.ndx].status=TERMINAL_STATUS_ON_LINE;
						//db->setTerminal(gd.terminals[gd.deviceList[id].terminalsData.ndx].name,TERMINAL_STATUS_ON_LINE,tpar);
						db->setTerminal("",TERMINAL_STATUS_ON_LINE,tpar);
						}
					}

				if(gd.deviceList[id].terminalsData.ndx>=MAX_TERMINALS)
					{
					gd.deviceList[id].workerState=globals_st::wkr_nterm_param_end;
					}
				else
					{
					gd.deviceList[id].workerState=globals_st::wkr_nterm_param_req;
					}

				break;
			//.................................................
			case HPROT_CMD_NACK:
				gd.deviceList[id].reqTimeout.startTimeout(WKR_REQUEST_TIMEOUT*1000);
				dbg->trace(DBG_WARNING,"get NACK: something wrong");
				break;
			default:
				sendBusy=true;
				break;
			}

		// handle request timeout and retry
		if(gd.deviceList[id].reqTimeout.checkTimeout())
			{
			if(gd.deviceList[id].retry>0)
				{
				dbg->trace(DBG_NOTIFY,"wrong or no answer (data) from id %d -> retry",id);
				gd.deviceList[id].retry--;
				gd.deviceList[id].terminalsData.ndx=gd.deviceList[id].terminalsData.prev_ndx;
				gd.deviceList[id].workerState=globals_st::wkr_nterm_param_req;
				}
			else
				{
				dbg->trace(DBG_NOTIFY,"wrong or no answer (data) from id %d -> stop",id);
				gd.deviceList[id].workerState=globals_st::wkr_nterm_param_end;
				}
			}
		break;
	//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
	case globals_st::wkr_nterm_param_end:
		sendBusy=true;
		break;
	}

if(sendBusy)
	{
	txData[0]=(uint8_t)sv_busy;
	comm->sendAnswer(f.frame.srcID,HPROT_CMD_SV_CHECK_CRC_P_W,txData,1,f.frame.num);
	}

//	// check if the timeout expired
//	if(gd.deviceList[id].timeout.checkTimeout())
//		{
//		resetDeviceStatus(id);
//		webCommandInProgress=false;
//		dbg->trace(DBG_WARNING,"worker operation timeout for ID %d", id);
//		}
}

/**
 * worker used to change key to all masters
 */
void Slave::worker_newKey(int id)
{
bool sendBusy=false;
bool done=true;
switch(gd.deviceList[id].workerState)
	{
	case globals_st::wkr_idle:
		break;
	//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
	case globals_st::wkr_newkey_init:
		for(globals_st::iddevice_t it = gd.deviceList.begin();it != gd.deviceList.end();it++)
			{
			// counts only devices that are actually alive
			if(it->second.aliveCounter==0)
				{
				TRACE(dbg,DBG_WARNING,"device %d is actually dead and will be ignored, its key cannot be changed",it->first);
				}
			}
		//break;
	//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
	case globals_st::wkr_newkey_send:
		switch(f.frame.cmd)
			{
			//.................................................
			case HPROT_CMD_SV_CHECK_CRC_P_W:
				txData[0]=(uint8_t)sv_update_crypto_key;
				txData[1]=gd.hprotKey[0];
				txData[2]=gd.hprotKey[1];
				txData[3]=gd.hprotKey[2];
				txData[4]=gd.hprotKey[3];
				comm->sendAnswer(f.frame.srcID,f.frame.cmd,txData,5,f.frame.num);
				gd.deviceList[id].workerState=globals_st::wkr_newkey_done;
				gd.deviceList[id].newKeySet=true;

				// check if the operation is done for all devices
				for(globals_st::iddevice_t it = gd.deviceList.begin();it != gd.deviceList.end();it++)
					{
					// counts only devices that are actually alive
					if(it->second.aliveCounter>0)
						{
						done &=it->second.newKeySet;
						}
					}

				// if done for all masters, I can change the key
				if(done)
					{
					char _tmpstr[10];
					dbg->trace(DBG_NOTIFY,"key changed to all devices, now change mine too");
					comm->setNewKey(gd.hprotKey);
					comm->enableCrypto(true);
					if(gd.useUdpDebug)
						{
						udpDbg->setNewKey(gd.hprotKey);
						udpDbg->enableCrypto(true);
						}
					if(hprotCheckKey(gd.hprotKey)==0)
						{
						dbg->trace(DBG_NOTIFY,"plain communication mode due to invalid key");
						}
					// save to the config file
					Hex2AsciiHex(_tmpstr,gd.hprotKey,HPROT_CRYPT_KEY_SIZE,false,0);
					cfg->updateVariable("CRYPTO_KEY",_S _tmpstr);
					gd.cfgChanged=true;
					}
				break;
			default:
				sendBusy=true;
				break;
			}
			break;
	//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
	case globals_st::wkr_newkey_done:
		sendBusy=true;
		break;
	}

if(sendBusy)
	{
	txData[0]=(uint8_t)sv_busy;
	comm->sendAnswer(f.frame.srcID,HPROT_CMD_SV_CHECK_CRC_P_W,txData,1,f.frame.num);
	}
}

/**
 * check the status of all  masters to update the global status variables
 * @param checkExpected true if need to compare the expected status (default)
 * @return true: if all master has equal status; false if someone differs
 */
bool Slave::checkGlobalStatus(bool checkExpected)
{
bool ret=false;
bool found=false;
// searche the first useful device to set global status
for(globals_st::iddevice_t it = gd.deviceList.begin();it != gd.deviceList.end();it++)
	{
	if(it->second.aliveCounter>0)	// only alive devices are considered
		{
		gd.glob_mainState=it->second.mainState;
		gd.glob_workerState=it->second.workerState;
		found=true;
		break;
		}
	}

if(found)
	{
	if(gd.deviceList.size()==1)
		{
		// in this case is obviously always true, but i've t check the real status
		ret=true;
		}
	else
		{
		ret=true;
		for(globals_st::iddevice_t it = gd.deviceList.begin();it != gd.deviceList.end();it++)
			{
			if(it->second.aliveCounter>0)	// only alive devices are considered
				{
				if((it->second.mainState!=gd.glob_mainState) || (it->second.workerState!=gd.glob_workerState))
					{
					gd.glob_mainState=globals_st::sl_unk;
					gd.glob_workerState=globals_st::wkr_unk;
					ret &= false;
					break;
					}
				}
			}
		}
	if(ret && checkExpected)
		{
		if(gd.expected_glob_mainState != globals_st::sl_unk || gd.expected_glob_workerState != globals_st::wkr_unk)
			{
			ret = ((gd.expected_glob_mainState == gd.glob_mainState) && (gd.expected_glob_workerState == gd.glob_workerState));
			}
		}
	}
else
	{
	ret=true;	// trivial result (no one device)
	}

return ret;
}

//-----------------------------------------------------------------------------
/**
 * check for the profile CRC list, if is ok, send ACK else send the  profile
 * which crc differ
 */
void Slave::profileCheck()
{
bool equal=true;
for(int i=0;i<f.frameData[1];i++)
	{
	if(gd.profiles[i+f.frameData[0]].params.crc8 != f.frameData[2+i])
		{
		equal=false;
		// crc are not equals, send the profile to align
		dbg->trace(DBG_NOTIFY,"profile %d differs",i+f.frameData[0]);
		comm->sendAnswer(f.frame.srcID,HPROT_CMD_SV_PROFILE_CRC_LIST,(uint8_t*)&gd.profiles[i+f.frameData[0]].params,sizeof(Profile_parameters_t),f.frame.num);
		break;
		}
	}
if(equal)
	{
	comm->sendAnswer(f.frame.srcID,HPROT_CMD_ACK,0,0,f.frame.num);
	}
}

/**
 * check for the profile CRC list, if is ok, send ACK else send the  profile
 *
 */
void Slave::weektimeCheck()
{
bool equal=true;
for(int i=0;i<MAX_WEEKTIMES;i++)
	{
	if(gd.weektimes[i].params.crc8 != f.frameData[i])
		{
		equal=false;
		// crc are not equals, send the weektime to align
		dbg->trace(DBG_NOTIFY,"weektime %d differs",i);
		comm->sendAnswer(f.frame.srcID,HPROT_CMD_SV_WEEKTIME_CRC_LIST,(uint8_t*)&gd.weektimes[i].params,sizeof(Weektime_parameters_t),f.frame.num);
		break;
		}
	}
if(equal)
	{
	comm->sendAnswer(f.frame.srcID,HPROT_CMD_ACK,0,0,f.frame.num);
	}
}

/**
 * check the connection and if a polling is coming from a master, updates the list of the connected masters
 */
bool Slave::masterListCheck()
{
bool ret=true;
globals_st::iddevice_t dit=gd.deviceList.find(f.frame.srcID);

if(dit == gd.deviceList.end()) // new master device
	{
	if(isInRange((int)f.frame.srcID,(int)DEV_BASEID_VARCOLAN,(int)DEV_MAXID_VARCOLAN,'B'))
		{
		globals_st::dataDevice_t dd;
		dbg->trace(DBG_NOTIFY,"adding a new master (id %d)",f.frame.srcID);
		gd.deviceList[f.frame.srcID]=dd;
		gd.deviceList[f.frame.srcID].dataEventQueue=new circular_buffer<globals_st::dataEvent_t>(MAX_CIRC_BUFF_SIZE);
		gd.deviceList[f.frame.srcID].aliveCounter=gd.masterAliveTime;
		}
	else
		{
		dbg->trace(DBG_ERROR,"master id not acceptable (id %d)",f.frame.srcID);
		ret=false;
		}
	}
return ret;
}

/**
 * handle all internal protocol or other events and check if sv has something to tell or to sync data
 */
void Slave::handleInternalEvents()
{
uint8_t globCrcW,globCrcP,globCrcC;
Event_t ev;
int nt;

globCrcP=f.frameData[0];	// profile
globCrcW=f.frameData[1];	// weektime
globCrcC=f.frameData[2];	// causal

//dbg->trace(DBG_DEBUG,"polling from id %d", f.frame.srcID);

/*
 * NOTE: this sequence defines intrinsically the priority
 */
if(globCrcP!=gd.globProfilesCRC && !gd.dumpFromMaster) // unsynced
	{
	dbg->trace(DBG_NOTIFY,"Profiles differs for id %d",f.frame.srcID);
	txData[0]=(uint8_t)sv_profile_data_error;
	comm->sendAnswer(f.frame.srcID,f.frame.cmd,txData,1,f.frame.num);
	}
else if(globCrcW!=gd.globWeektimesCRC && !gd.dumpFromMaster) // unsynced
	{
	dbg->trace(DBG_NOTIFY,"Weektimes differs for id %d",f.frame.srcID);
	txData[0]=(uint8_t)sv_weektime_data_error;
	comm->sendAnswer(f.frame.srcID,f.frame.cmd,txData,1,f.frame.num);
	}
else if(globCrcC!=gd.globCausalsCRC && !gd.dumpFromMaster) // unsynced
	{
	dbg->trace(DBG_NOTIFY,"Causals differs for id %d -> updating",f.frame.srcID);
	txData[0]=(uint8_t)sv_codes_data_error;
	memcpy(&txData[1],gd.causals,sizeof(CausalCodes_parameters_t)*MAX_CAUSAL_CODES);
	comm->sendAnswer(f.frame.srcID,f.frame.cmd,txData,sizeof(CausalCodes_parameters_t)*MAX_CAUSAL_CODES+1,f.frame.num);
	}
else if(!gd.deviceList[f.frame.srcID].dataEventQueue->empty())
	{
	globals_st::dataEvent_t dataev;
	Badge_parameters_t badge;
	dataev=gd.deviceList[f.frame.srcID].dataEventQueue->front();
	//gd.deviceList[f.frame.srcID].dataEventQueue.pop();

	gd.deviceList[f.frame.srcID].dataEventQueue->pop_front();
	switch(dataev.dataEvent)
		{
		case globals_st::dataEvent_t::ev_badge_data:
			TRACE(dbg,DBG_DEBUG, "send badge %d data to id %d",dataev.tag.badge_id,f.frame.srcID);
			db->getBadgeFromId(dataev.tag.badge_id,badge);
			txData[0]=(uint8_t)sv_badge_data;
			memcpy(&txData[1],&badge,sizeof(Badge_parameters_t));
			comm->sendAnswer(f.frame.srcID,f.frame.cmd,txData,sizeof(Badge_parameters_t)+1,f.frame.num);
			break;
		case globals_st::dataEvent_t::ev_terminal_data_upd:
			txData[0]=(uint8_t)sv_terminal_data_upd;
			memcpy(&txData[1],&dataev.tag.term_par,sizeof(Terminal_parameters_to_send_t));
			comm->sendAnswer(f.frame.srcID,f.frame.cmd,txData,sizeof(Terminal_parameters_to_send_t)+1,f.frame.num);
			break;
		case globals_st::dataEvent_t::ev_new_time:
			txData[0]=(uint8_t)sv_new_time;
			memcpy(&txData[1],&dataev.tag.epochtime,sizeof(time_t));
			comm->sendAnswer(f.frame.srcID,f.frame.cmd,txData,sizeof(time_t)+1,f.frame.num);
			break;
		case globals_st::dataEvent_t::ev_fw_version_req:
			txData[0]=(uint8_t)sv_fw_version_req;
			comm->sendAnswer(f.frame.srcID,f.frame.cmd,txData,1,f.frame.num);
			break;
		case globals_st::dataEvent_t::ev_active_wktimes:
			txData[0]=(uint8_t)sv_active_wt;
			memcpy(&txData[1],&db->weektimeActive,sizeof(uint64_t));
			comm->sendAnswer(f.frame.srcID,f.frame.cmd,txData,sizeof(uint64_t)+1,f.frame.num);
			break;
		case globals_st::dataEvent_t::ev_fw_new_version:
			txData[0]=(uint8_t)sv_fw_new_version;
			comm->sendAnswer(f.frame.srcID,f.frame.cmd,txData,1,f.frame.num);
			break;
		case globals_st::dataEvent_t::ev_bl_new_version:
			txData[0]=(uint8_t)sv_bl_new_version;
			comm->sendAnswer(f.frame.srcID,f.frame.cmd,txData,1,f.frame.num);
			break;
		case globals_st::dataEvent_t::ev_global_deletion:
			txData[0]=(uint8_t)sv_global_deletion;
			comm->sendAnswer(f.frame.srcID,f.frame.cmd,txData,1,f.frame.num);
			break;
		case globals_st::dataEvent_t::ev_terminal_control:
			txData[0]=(uint8_t)sv_terminal_control;
			txData[1]=dataev.tag.params[0];	// id
			txData[2]=dataev.tag.params[1];	// on/off
			comm->sendAnswer(f.frame.srcID,f.frame.cmd,txData,3,f.frame.num);
			break;
		case globals_st::dataEvent_t::ev_set_output:
			txData[0]=(uint8_t)sv_drive_outputs;
			txData[1]=dataev.tag.params[0];
			txData[2]=dataev.tag.params[1];
			txData[3]=dataev.tag.params[2];
			comm->sendAnswer(f.frame.srcID,f.frame.cmd,txData,4,f.frame.num);
			break;
		case globals_st::dataEvent_t::ev_spare:
			txData[0]=(uint8_t)sv_spare;
			txData[1]=dataev.tag.params[0];
			comm->sendAnswer(f.frame.srcID,f.frame.cmd,txData,2,f.frame.num);
			break;
		case globals_st::dataEvent_t::ev_mac_address:
			txData[0]=(uint8_t)sv_mac_address;
			// in dataev.tag.params[0] there is the ID
			txData[1]=dataev.tag.params[1];
			txData[2]=dataev.tag.params[2];
			txData[3]=dataev.tag.params[3];
			txData[4]=dataev.tag.params[4];
			txData[5]=dataev.tag.params[5];
			txData[6]=dataev.tag.params[6];
			comm->sendAnswer(f.frame.srcID,f.frame.cmd,txData,7,f.frame.num);
			break;
		case globals_st::dataEvent_t::ev_ip_address:
			txData[0]=(uint8_t)sv_ip_address;
			// in dataev.tag.params[0] there is the ID
			txData[1]=dataev.tag.params[1];
			txData[2]=dataev.tag.params[2];
			txData[3]=dataev.tag.params[3];
			txData[4]=dataev.tag.params[4];
			comm->sendAnswer(f.frame.srcID,f.frame.cmd,txData,5,f.frame.num);
			break;
		case globals_st::dataEvent_t::ev_memory_dump:
			txData[0]=(uint8_t)sv_memory_dump;
			gd.dumpFromMaster=true;
			comm->sendAnswer(f.frame.srcID,f.frame.cmd,txData,1,f.frame.num);
			break;
		case globals_st::dataEvent_t::ev_terminal_list:
			txData[0]=(uint8_t)sv_present_terminals_req;
			comm->sendAnswer(f.frame.srcID,f.frame.cmd,txData,1,f.frame.num);
			break;
		case globals_st::dataEvent_t::ev_all_terminal_data:
			webCommandInProgress=true;
			gd.deviceList[f.frame.srcID].mainState=globals_st::sl_terminal_request;
			gd.deviceList[f.frame.srcID].workerState=globals_st::wkr_nterm_init;

			comm->sendAnswer(f.frame.srcID,HPROT_CMD_ACK,0,0,f.frame.num);
			break;
		case globals_st::dataEvent_t::ev_terminal_data_req:
			txData[0]=(uint8_t)sv_terminal_data_req;
			for(nt=0;nt<MAX_EVENT_TAG_PARAMS;nt++)
				{
				if(dataev.tag.params[nt]!=0)
					{
					txData[nt+1]=dataev.tag.params[nt];
					}
				else break;
				}
			comm->sendAnswer(f.frame.srcID,f.frame.cmd,txData,nt+1,f.frame.num);
			break;
		case globals_st::dataEvent_t::ev_varco_event:
			ev.Parameters_s.eventCode=(EventCode_t)dataev.tag.varco_event[EV_FLD_EVENT];
			ev.Parameters_s.idBadge=dataev.tag.varco_event[EV_FLD_BADGE_ID];
			ev.Parameters_s.area=dataev.tag.varco_event[EV_FLD_AREA];
			ev.Parameters_s.causal_code=dataev.tag.varco_event[EV_FLD_CODE];
			ev.Parameters_s.terminalId=dataev.tag.varco_event[EV_FLD_DEVICE_ID];
			ev.Parameters_s.timestamp=getSystemTime();
			db->addEvent(ev);
			break;
		case globals_st::dataEvent_t::ev_log_deletion:
			txData[0]=(uint8_t)sv_log_deletion;
			comm->sendAnswer(f.frame.srcID,f.frame.cmd,txData,1,f.frame.num);
			break;
		case globals_st::dataEvent_t::ev_polling_time:
			txData[0]=(uint8_t)sv_polling_time;
			memcpy(&txData[1],&dataev.tag.polling_time,2);
			comm->sendAnswer(f.frame.srcID,f.frame.cmd,txData,3,f.frame.num);
			break;
		case globals_st::dataEvent_t::ev_reset_device:
			txData[0]=(uint8_t)sv_reboot_request;
			comm->sendAnswer(f.frame.srcID,f.frame.cmd,txData,1,f.frame.num);
			break;
		case globals_st::dataEvent_t::ev_stats_req:
			txData[0]=(uint8_t)sv_stats_req;
			comm->sendAnswer(f.frame.srcID,f.frame.cmd,txData,1,f.frame.num);
			break;
		case globals_st::dataEvent_t::ev_stats_clear:
			txData[0]=(uint8_t)sv_stats_clear;
			comm->sendAnswer(f.frame.srcID,f.frame.cmd,txData,1,f.frame.num);
			break;
		case globals_st::dataEvent_t::ev_change_crypto_key:
			webCommandInProgress=true;
			gd.deviceList[f.frame.srcID].mainState=globals_st::sl_newkey_send;
			gd.deviceList[f.frame.srcID].workerState=globals_st::wkr_newkey_init;

			comm->sendAnswer(f.frame.srcID,HPROT_CMD_ACK,0,0,f.frame.num);

			memcpy(gd.hprotKey,dataev.tag.params,HPROT_CRYPT_KEY_SIZE);
			break;

		default:
			// all other cases are handled before
			break;
		}
	}
else
	{
	// all ok or nothing else to handle
	/*
	 * scan all badge database and send it to the requestor
	 */
	bool do_polling_ans=true;
	if(gd.dumpTriggerFromCfg)
		{
		// create an event to start dump
		globals_st::dataEvent_t de;
		de.dataEvent=globals_st::dataEvent_t::ev_memory_dump;
		de.tag.params[0]=f.frame.srcID;	// id
		pushEvent(de,de.tag.params[0]);

		gd.dumpTriggerFromCfg=false;
		}
	else if(gd.badgeAutoSend)
		{
		if(db->getBadgeFromId(gd.deviceList[f.frame.srcID].badgeIDindex,tmpBPar,false))
			{
			tmpBPar.link=NULL_LINK;
			tmpBPar.crc8=CRC8calc((uint8_t*)&tmpBPar.Parameters_s,sizeof(tmpBPar.Parameters_s));
			txData[0]=(uint8_t)sv_badge_data;
			memcpy(&txData[1],&tmpBPar,sizeof(Badge_parameters_t));
			//comm->sendAnswer(f.frame.srcID,f.frame.cmd,txData,sizeof(Badge_parameters_t)+1);
			comm->sendAnswer(f.frame.srcID,f.frame.cmd,txData,sizeof(Badge_parameters_t)+1,f.frame.num);
			do_polling_ans=false;
			}
		else
			{
			// do polling
			do_polling_ans=true;
			}
		gd.deviceList[f.frame.srcID].badgeIDindex++;
		if(gd.deviceList[f.frame.srcID].badgeIDindex>gd.maxBadgeId) gd.deviceList[f.frame.srcID].badgeIDindex=1;
		}

	if(do_polling_ans)
		{
		// POLLING
		//gd.deviceList[f.frame.srcID].badgeIDindex=1;
		comm->sendAnswer(f.frame.srcID,HPROT_CMD_ACK,0,0,f.frame.num);
		//dbg->trace(DBG_DEBUG,"answer to polling from id %d", f.frame.srcID);
		}
	}
}

/**
 * handle network event
 */
void Slave::handleVarcoEvent()
{
Event_t ev;

memcpy(&ev,f.frameData,sizeof(Event_t));
dbg->trace(DBG_NOTIFY,"New event from master id %d (%d)",f.frame.srcID,ev.Parameters_s.eventCode);
switch(ev.Parameters_s.eventCode)
	{
	case transit:
	case checkin:
		db->setArea(ev.Parameters_s.idBadge,ev.Parameters_s.area);
		break; // not used because the event must be stored
	default:
		break;
	}
db->addEvent(ev);
comm->sendAnswer(f.frame.srcID,HPROT_CMD_ACK,0,0,f.frame.num);
}

/**
 * handle the bdge area request
 */
void Slave::handleRequestArea()
{
uint16_t bid=*(uint16_t*)f.frameData;

int area=db->getArea(bid);
if(area>=0)
	{
	txData[0]=(uint8_t)area;
	dbg->trace(DBG_NOTIFY,"Badge id %d found, current area: %d",bid,(int)area);
	comm->sendAnswer(f.frame.srcID,HPROT_CMD_SV_REQUEST_CURRENT_AREA,	txData,1,f.frame.num);
	}
else
	{
	dbg->trace(DBG_WARNING,"Badge id %d not found or some other error occurs",bid);
	comm->sendAnswer(f.frame.srcID,HPROT_CMD_NACK,0,0,f.frame.num);
	}
}

//-----------------------------------------------------------------------------
// FIRMWARE HANDLING MEMBERS
//-----------------------------------------------------------------------------
/**
 * load all firmware data already present in the update folder
 */
void Slave::firmwareLoadData()
{
vector<string> files;
fwData_t fw;
ifstream f;
string line,cmd;
vector<string> toks;

// take all data files
fileList(FIRMWARE_UPD_PATH,files,_S FIRMWARE_TXT_EXT,true);

firmwares.clear();
FOR_EACH(it,string,files)
	{
	if(startsWith(*it,FIRMWARE_VARCOLAN))
		{
		fw.fwMetadata.fw_type=DEVICE_TYPE_VARCOLAN;
		fw.name="varcolan";
		}
	else if(startsWith(*it,FIRMWARE_BL_VARCOLAN))
		{
		fw.fwMetadata.fw_type=DEVICE_TYPE_BL_VARCOLAN;
		fw.name="bl_varcolan";
		}
	else if(startsWith(*it,FIRMWARE_SUPPLY))
		{
		fw.fwMetadata.fw_type=DEVICE_TYPE_SUPPLY_MONITOR;
		fw.name="supply";
		}
	else if(startsWith(*it,FIRMWARE_DISPLAY))
		{
		fw.fwMetadata.fw_type=DEVICE_TYPE_DISPLAY;
		fw.name="display";
		}
	else
		{
		dbg->trace(DBG_WARNING,"unknown firmware firmware " + *it);
		}

	// compose filenames
	fw.firmwareTxt=FIRMWARE_UPD_PATH + fw.name + FIRMWARE_TXT_EXT;
	fw.firmwareBin=FIRMWARE_UPD_PATH + fw.name + FIRMWARE_BIN_EXT;

	// check MD5 validity
	if(firmwareCheckMD5(fw.fwMetadata.fw_type))
		{
		f.open(fw.firmwareTxt.c_str());
		if(f.is_open())
			{
			// read the file
			getline(f, line);	// md5
			getline(f, line);	// version
			toks.clear();
			Split(line,toks,"#.");
			fw.fwMetadata.version.major=atoi(toks[0].c_str());
			fw.fwMetadata.version.minor=atoi(toks[1].c_str());
			// get the file size
			FILE *fp;
			fp=fopen(fw.firmwareBin.c_str(),"rb");
			if(fp)
				{
				fw.fwMetadata.fw_size=filelength(fp);
				// calculates its crc32
				uint8_t b;
				fw.fwMetadata.fw_crc32=CRC32_INIT;
				int cnt=0,sz=0;
				while(!feof(fp))
					{
					cnt=fread(&b,1,1,fp);
					if(cnt)
						{
						sz++;
						crc32(b,&fw.fwMetadata.fw_crc32);
						}
					}
				TRACE(dbg,DBG_NOTIFY,"firmware %s size %lu bytes (crc32: %04X) ver. %02X.%02X",
						fw.firmwareBin.c_str(),
						fw.fwMetadata.fw_size,
						fw.fwMetadata.fw_crc32,
						(unsigned int)fw.fwMetadata.version.major,
						(unsigned int)fw.fwMetadata.version.minor
						);
				fclose(fp);
				firmwares.push_back(fw);
				}
			else
				{
				dbg->trace(DBG_ERROR,"unable to open firmware file " + fw.firmwareBin);
				}
			f.close();
			}
		else
			{
			dbg->trace(DBG_ERROR,"unable to open txt firmware data file " + fw.firmwareTxt);
			}
		}
	else
		{
		dbg->trace(DBG_WARNING,"corrupted files are removed");
		// remove all firmwares
		string cmd= "rm "+ _S FIRMWARE_UPD_PATH + fw.name + "*";
		remove(cmd.c_str());
		}
	}
dbg->trace(DBG_NOTIFY,"found %d firmwares",firmwares.size());
}


/**
 * check if there is a new firmware
 * @return type of new firmware (for which device)
 */
globals_st::dataEvent_t Slave::firmwareCheckForNew()
{
globals_st::dataEvent_t ev;
uint8_t type=DEVICE_TYPE_NONE;
vector<string> files;
string destpath,name,ext;
bool unpack=true, isBL=false;

fileList(FIRMWARE_NEW_PATH,files,_S FIRMWARE_PKG_EXT,true);

// NOTE: scan all files but takes only the first one
ev.dataEvent=globals_st::dataEvent_t::ev_none;
FOR_EACH(it,string,files)
	{
	if(startsWith(*it,FIRMWARE_VARCOLAN))
		{
		type=DEVICE_TYPE_VARCOLAN;
		name=FIRMWARE_VARCOLAN;
		ext=FIRMWARE_PKG_EXT;
		destpath=_S FIRMWARE_UPD_PATH;
		unpack=true;
		}
	else if(startsWith(*it,FIRMWARE_BL_VARCOLAN))
		{
		type=DEVICE_TYPE_BL_VARCOLAN;
		name=FIRMWARE_BL_VARCOLAN;
		ext=FIRMWARE_PKG_EXT;
		destpath=_S FIRMWARE_UPD_PATH;
		unpack=true;
		isBL=true;
		}
	else if(startsWith(*it,FIRMWARE_SUPPLY))
		{
		type=DEVICE_TYPE_SUPPLY_MONITOR;
		name=FIRMWARE_SUPPLY;
		ext=FIRMWARE_PKG_EXT;
		destpath=_S FIRMWARE_UPD_PATH;
		unpack=true;
		}
	else if(startsWith(*it,FIRMWARE_DISPLAY))
		{
		type=DEVICE_TYPE_DISPLAY;
		name=FIRMWARE_DISPLAY;
		ext=FIRMWARE_PKG_EXT;
		destpath=_S FIRMWARE_UPD_PATH;
		unpack=true;
		}
	else if(startsWith(*it,FIRMWARE_SUPERVISOR))
		{
		type=DEVICE_TYPE_SUPERVISOR;
		name=*it;
		ext=FIRMWARE_SV_PKG_EXT;
		destpath=_S FIRMWARE_SVUPD_PATH;
		unpack=false;
		}

	// move firmware to the right folder
	string cmd="mv " + _S FIRMWARE_NEW_PATH + *it + " " + destpath + name + ext;
	system(cmd.c_str());
#ifdef RASPBERRY
	cmd="sudo chmod 666 " + destpath + *it;
#else
	cmd="chmod 666 " + destpath + *it;
#endif
	system(cmd.c_str());
	system("sync");

	//----------------------
	// for other devices
	if(unpack)
		{
		// unpack the firmware
		if(firmwareUnpack(type))
			{
			// reload the entire firmware database
			firmwareLoadData();
			// search the data
			fwit_t it=find(firmwares.begin(),firmwares.end(),type);
			if(it!=firmwares.end())
				{
				// all OK
				(isBL) ? (ev.dataEvent=globals_st::dataEvent_t::ev_bl_new_version) : (ev.dataEvent=globals_st::dataEvent_t::ev_fw_new_version);
				ev.tag.fw_device_type=type;
				break;	// return!
				}
			else
				{
				dbg->trace(DBG_ERROR,"the new FW is not loaded in the database");
				}
			}
		}
	//----------------------
	// for me (Supervisor)
	else
		{
		// other install methods
		if(type==DEVICE_TYPE_SUPERVISOR)
			{
			putDBEvent(fw_installed,DEV_BASEID_SUPERVISOR);
			sleep(1);
			dbg->trace(DBG_WARNING,"Supervisor will be terminated and restarted to install new software...");
			gd.run=false;
			}
		else
			{
			dbg->trace(DBG_ERROR,"device type undefined and its update method too");
			}
		}
	}
return ev;
}

/**
 * unpack a packaged firmware and check its integrity (not supervisor)
 * @param type
 * @param pkgfname (optional) package filename (no extension) usefull if it has no standard name
 * @return true:ok
 */
bool Slave::firmwareUnpack(uint8_t type, string pkgfname)
{
bool ret=false;
string pkg,cmd;
if(pkg.empty())
	{
	switch(type)
		{
		case DEVICE_TYPE_VARCOLAN:
			dbg->trace(DBG_NOTIFY,"found new varcolan firmware: " + pkgfname);
			pkg=FIRMWARE_VARCOLAN;
			break;
		case DEVICE_TYPE_BL_VARCOLAN:
			dbg->trace(DBG_NOTIFY,"found new varcolan (bootloader) firmware: " + pkgfname);
			pkg=FIRMWARE_BL_VARCOLAN;
			break;
//		case DEVICE_TYPE_SUPERVISOR:
//			dbg->trace(DBG_NOTIFY,"found new supervisor firmware: " + pkgfname);
//			pkg=FIRMWARE_SUPERVISOR;
//			unpack=false;
//			break;
		case DEVICE_TYPE_SUPPLY_MONITOR:
			dbg->trace(DBG_NOTIFY,"found new supply monitor firmware: " + pkgfname);
			pkg=FIRMWARE_SUPPLY;
			break;
		case DEVICE_TYPE_DISPLAY:
			dbg->trace(DBG_NOTIFY,"found new display firmware: " + pkgfname);
			pkg=FIRMWARE_DISPLAY;
			break;
		}
	}
else
	{
	pkg=pkgfname;
	}

dbg->trace(DBG_NOTIFY,"unpacking firmware package " + pkg);
cmd=_S FIRMWARE_UPD_PATH + FIRMWARE_UNPACKER + " " + pkg + " " + RESULT_TXT_FILE + " rmpack";
TRACE(dbg,DBG_DEBUG,"executing " + cmd);
system(cmd.c_str());

// check the md5 test result
ifstream f;
string line;
string resfile=_S FIRMWARE_UPD_PATH + _S RESULT_TXT_FILE;
f.open(resfile.c_str());
if(f.is_open())
	{
	getline(f, line);
	size_t found=line.find(pkg);
	if(found!=string::npos)
		{
		found=line.find("OK");
		if(found!=string::npos)
			{
			dbg->trace(DBG_NOTIFY,"MD5 firmware: " + line);
			ret=true;
			}
		else
			{
			dbg->trace(DBG_ERROR,"invalid MD5 firmware -> remove all wrong files");
			cmd="rm " + _S FIRMWARE_UPD_PATH + pkg + "*";
			system(cmd.c_str());
			}
		}
	else
		{
		dbg->trace(DBG_ERROR,"wrong firmware: " + line);
		}
	f.close();
	}
remove(resfile.c_str());
return ret;
}

/**
 * check tne MD5 for thefirmware specified
 * @param type
 * @return true:ok
 */
bool Slave::firmwareCheckMD5(uint8_t type)
{
bool ret=false;
string name,cmd;
ifstream f;
string line;

switch(type)
	{
	case DEVICE_TYPE_VARCOLAN:
		name=FIRMWARE_VARCOLAN;
		break;
	case DEVICE_TYPE_BL_VARCOLAN:
		name=FIRMWARE_BL_VARCOLAN;
		break;
	case DEVICE_TYPE_SUPERVISOR:
		name=FIRMWARE_SUPERVISOR;
		break;
	case DEVICE_TYPE_SUPPLY_MONITOR:
		name=FIRMWARE_SUPPLY;
		break;
	case DEVICE_TYPE_DISPLAY:
		name=FIRMWARE_DISPLAY;
		break;
	}

// perform md5 check
cmd="cd " +  _S FIRMWARE_UPD_PATH + " && md5sum -c " + name + FIRMWARE_TXT_EXT + " > " + RESULT_TXT_FILE;
system(cmd.c_str());

// read the md5 test result
string resfile=_S FIRMWARE_UPD_PATH + _S RESULT_TXT_FILE;
f.open(resfile.c_str());
if(f.is_open())
	{
	getline(f, line);
	size_t found=line.find(name);
	if(found!=string::npos)
		{
		found=line.find("OK");
		if(found!=string::npos)
			{
			dbg->trace(DBG_NOTIFY,"firmware: " + line);
			ret=true;
			}
		else
			{
			dbg->trace(DBG_ERROR,"invalid MD5 firmware");
			//cmd="rm " + _S FIRMWARE_UPD_PATH + name + "*";
			//system(cmd.c_str());
			}
		}
	else
		{
		dbg->trace(DBG_ERROR,"wrong firmware: " + line);
		}
	f.close();
	}
remove(resfile.c_str());
return ret;
}

/**
 * check the firmware version, if it is < of the one stored in the supervisor -> true
 * @param type
 * @param major
 * @param minor
 * @return true if the device need to be updated; false: not or SV does not have the firmware
 */
bool Slave::firmwareCheckVersion(uint8_t type, uint8_t major, uint8_t minor)
{
ifstream f;
string line;
vector<string> toks;
int fwver_sv,fwver_dev;
bool ret=false;

fwit_t it=find(firmwares.begin(),firmwares.end(),type);
if(it==firmwares.end()) return ret;	// firmware not found

f.open(it->firmwareTxt.c_str());
if(f.is_open())
	{
	// read the file
	getline(f, line);	// md5
	getline(f, line);	// version
	Split(line,toks,"#.");
	//it->fwMetadata.version.major=atoi(toks[0].c_str());
	//it->fwMetadata.version.minor=atoi(toks[1].c_str());
	fwver_sv=it->fwMetadata.version.major*100+it->fwMetadata.version.minor;
	fwver_dev=major*100+minor;

#ifdef THREE_RELAYS
	if(fwver_sv < 200)
		{
		return false;
		}
#else
	if(fwver_sv >= 200)
		{
		return false;
		}
#endif

	if(fwver_sv>fwver_dev)
		{
		ret=true;
		}
	f.close();
	}
return ret;
}

/**
 * send a firmware chunk to the requester
 * @param type
 * @param startByte
 * @param amount
 * @return true: ok; false: firmware not found
 */
bool Slave::firmwareChunkSend(uint8_t type, uint32_t startByte,uint8_t amount)
{
bool ret=false;
FILE *fp;

// search firmware
if(actualType!=type)
	{
	actual_it=find(firmwares.begin(),firmwares.end(),type);
	if(actual_it==firmwares.end())
		{
		actualType=DEVICE_TYPE_NONE;
		return ret;	// firmware not found
		}
	actualType=type;
	}

if(amount>HPROT_PAYLOAD_SIZE)
	{
	dbg->trace(DBG_NOTIFY,"invalid amount request");
	return ret;
	}

fp=fopen(actual_it->firmwareBin.c_str(),"rb");
if(fp)
	{
	memcpy(txData,&startByte,sizeof(startByte));
	fseek(fp, startByte, SEEK_SET);
	memset(&txData[sizeof(startByte)],0xff,amount);
	size_t n=fread(&txData[sizeof(startByte)],1,amount,fp);
	if(n)
		{
		comm->sendAnswer(f.frame.srcID,HPROT_CMD_SV_FW_CHUNK_REQ,	txData,amount+sizeof(startByte),f.frame.num);
		unsigned int rem=(actual_it->fwMetadata.fw_size > (startByte+amount)) ? (actual_it->fwMetadata.fw_size-(startByte+amount)) : (0);
		dbg->trace(DBG_NOTIFY,"FW update: sent %d bytes to id %-3d addr 0x%05X, remaining %u bytes",amount,f.frame.srcID,startByte,rem);
		fclose(fp);
		}
	else
		{
		dbg->trace(DBG_NOTIFY,"No more data to read from " + actual_it->firmwareBin);
		comm->sendAnswer(f.frame.srcID,HPROT_CMD_NACK,0,0,f.frame.num);
		}
	ret=true;
	}
else
	{
	dbg->trace(DBG_ERROR,"unable to open firmware file " + actual_it->firmwareBin);
	}
return ret;
}

/**
 * handle the memory data dump of a master
 */
void Slave::handleMasterDump()
{
Weektime_parameters_t *wp;
Profile_parameters_t *pp;
Badge_parameters_t *bp;
char name[MAX_NAME_SIZE];
int tmp;

try
	{
	switch(f.frameData[0])
		{
		case WEEKTIME_TYPE:	// weektime data
			wp=(Weektime_parameters_t *)&f.frameData[1];
			dbg->trace(DBG_NOTIFY,"weektime %d data dump",wp->Parameters_s.weektimeId);
			sprintf(name,"Weektime%d",wp->Parameters_s.weektimeId);
			db->setWeektime(wp->Parameters_s.weektimeId,_S name,*wp);
			if(!db->updateWeektime(wp->Parameters_s.weektimeId,""))
				{
				dbg->trace(DBG_ERROR,"cannot update weektime %d in the database",wp->Parameters_s.weektimeId);
				throw false;
				}
			break;

		case PROFILE_TYPE:	// profile data
			pp=(Profile_parameters_t *)&f.frameData[1];
			dbg->trace(DBG_NOTIFY,"profile %d data dump",pp->Parameters_s.profileId);
			sprintf(name,"Profile%d",pp->Parameters_s.profileId);
			db->setProfile(pp->Parameters_s.profileId,_S name,*pp);
			if(!db->updateProfile(pp->Parameters_s.profileId,""))
				{
				dbg->trace(DBG_ERROR,"cannot update profile %d in the database",pp->Parameters_s.profileId);
				throw false;
				}
			break;

		case BADGE_TYPE:	// badge data
			bp=(Badge_parameters_t *)&f.frameData[1];
			dbg->trace(DBG_NOTIFY,"badge %d data dump",bp->Parameters_s.ID);
			tmp=db->setBadge(*bp);
			if(tmp<0)
				{
				dbg->trace(DBG_ERROR,"cannot add badge %d in the database",bp->Parameters_s.ID);
				throw false;
				}
			if(bp->Parameters_s.ID != tmp)
				{
				dbg->trace(DBG_WARNING,"badge ID is not equal: from master=%d, mine DB=%d",bp->Parameters_s.ID,tmp);
				}
			break;

		case STOP_TYPE: // end of dump
			if(gd.dumpFromMaster)
				{
				cfg->updateVariable("DUMP_FROM_MASTER", "0");	// ensure one-shot!
				gd.cfgChanged=true;
				gd.dumpFromMaster=false;
				}
			break;
		}
	comm->sendAnswer(f.frame.srcID,HPROT_CMD_ACK,0,0,f.frame.num);
	}
catch(...)
	{
	comm->sendAnswer(f.frame.srcID,HPROT_CMD_NACK,0,0,f.frame.num);
	}
}

/**
 * handle timed requests for each device
 */
void Slave::handleTimedRequests()
{
for(globals_st::iddevice_t it = gd.deviceList.begin();it != gd.deviceList.end();it++)
	{
	if(it->second.aliveCounter>0)	// if is alive..
		{
		if(it->second.timedReqCounter>0) it->second.timedReqCounter--;
		if(it->second.timedReqCounter==0)
			{
			switch(it->second.timedRequest)
				{
//				case globals_st::dataDevice_st::treq_none:
//					it->second.timedRequest=globals_st::dataDevice_st::treq_fwVersion;
//					break;

				//..............................................................................
				case globals_st::dataDevice_st::treq_fwVersion:
					if(!checkIfEventExists(it->first,globals_st::dataEvent_t::ev_fw_version_req))
						{
						globals_st::dataEvent_t de;
						de.dataEvent=globals_st::dataEvent_t::ev_fw_version_req;
						pushEvent(de,it->first);
						}

					it->second.timedRequest=globals_st::dataDevice_st::treq_active_wktimes;	// for the nest step
					break;

				//..............................................................................
				case globals_st::dataDevice_st::treq_active_wktimes:
					if(!checkIfEventExists(it->first,globals_st::dataEvent_t::ev_active_wktimes))
						{
						globals_st::dataEvent_t de;
						de.dataEvent=globals_st::dataEvent_t::ev_active_wktimes;
						pushEvent(de,it->first);
						}
					break;
				}
			// recharge for next step
			it->second.timedReqCounter=DEF_TIMED_REQUEST;
			}
		}
	}
}


/**
 * update alive counters for devices
 */
void Slave::updateAllAliveCounters()
{
for(globals_st::iddevice_t it = gd.deviceList.begin();it != gd.deviceList.end();it++)
	{
	if(it->second.aliveCounter>0) it->second.aliveCounter--;
	}
}

/**
 * reset alie counter to its maximum
 * @param id device (0 = all devices)
 * @param aliveValue count of the alive counter (seconds)
 */
void Slave::resetAliveCounter(int id, uint16_t aliveValue)
{
if(id==0)
	{
	for(globals_st::iddevice_t it = gd.deviceList.begin();it != gd.deviceList.end();it++)
		{
		it->second.aliveCounter=aliveValue;
		it->second.deadSignaled=false;
		}
	}
else
	{
	gd.deviceList[id].aliveCounter=aliveValue;
	gd.deviceList[id].deadSignaled=false;
	}
}

/**
 * check if the device is not alive
 * @param id (0 = all devices)
 * @param event if true (default) the dead is written to DB
 * @return true: is alive; false: dead
 */
bool Slave::checkAliveCounter(int id, bool event)
{
bool ret=true;
if(id==0)
	{
	for(globals_st::iddevice_t it = gd.deviceList.begin();it != gd.deviceList.end();it++)
		{
		ret &= (it->second.aliveCounter>0);
		if(it->second.aliveCounter==0 && !it->second.deadSignaled)
			{
			dbg->trace(DBG_WARNING,"device %d is dead?", it->first);
			it->second.timedReqCounter=DEF_TIMED_REQUEST_INIT;
			it->second.timedRequest=globals_st::dataDevice_st::treq_fwVersion;
			if(event)
				{
				putDBEvent(terminal_not_inline,it->first);
				}
			gd.deadCounter++;
			it->second.deadSignaled=true;
			}
		}
	}
else
	{
	ret=(gd.deviceList[id].aliveCounter>0);
	if(!ret  && !gd.deviceList[id].deadSignaled)
		{
		dbg->trace(DBG_WARNING,"device %d is dead?", id);
		gd.deviceList[id].timedReqCounter=DEF_TIMED_REQUEST_INIT;
		gd.deviceList[id].timedRequest=globals_st::dataDevice_st::treq_fwVersion;
		if(event)
			{
			putDBEvent(terminal_not_inline,id);
			}
		gd.deviceList[id].deadSignaled=true;
		}
	}
return ret;
}

/**
 * polling time modulator
 * @param queueSize actual size of the queue
 */
inline void Slave::pollingTimeModulator(size_t queueSize)
{
bool sendEvent=false;

if(gd.enableChangePollingTime && (queueSize > MSG_QUEUE_THRESHOLD_POLLING))
	{
	if(gd.masterQueueCounter<MSG_QUEUE_COUNTER_MAX) gd.masterQueueCounter++;
	if(gd.masterQueueCounter>MSG_QUEUE_CONGESTION_MAX)
		{
		if(gd.masterQueueDelayCnt>0)	// used to delay changes
			{
			gd.masterQueueDelayCnt--;
			}
		else
			{
			if(gd.masterPollingTime < MSG_MAX_POLLING_TIME)
				{
				gd.masterQueueDelayCnt=MSG_QUEUE_DELAY_CNT;
				gd.masterPollingTime += MSG_POLLING_TIME_INCREMENT;
				dbg->trace(DBG_WARNING,"queue: increment polling time to %d ms)", gd.masterPollingTime);
				sendEvent=true;
				}
			}
		}
	}
else
	{
	if(gd.masterQueueCounter>0)
		{
		gd.masterQueueCounter--;
		}
	else
		{
		if(gd.masterQueueDelayCnt>0) 	// used to delay changes
			{
			gd.masterQueueDelayCnt--;
			}
		else
			{
			// fold back
			if(gd.masterPollingTime > gd.masterPollingTime_def)
				{
				gd.masterQueueDelayCnt=MSG_QUEUE_DELAY_CNT;
				gd.masterPollingTime -= MSG_POLLING_TIME_INCREMENT;
				if(gd.masterPollingTime<gd.masterPollingTime_def) gd.masterPollingTime=gd.masterPollingTime_def;
				dbg->trace(DBG_NOTIFY,"queue: decrement polling time to %d ms)", gd.masterPollingTime);
				gd.masterQueueCounter=MSG_QUEUE_CONGESTION_MAX;	// this to avoid continuous decrements
				sendEvent=true;
				}
			}
		}
	}
if(sendEvent)
	{
	globals_st::dataEvent_t de;
	de.dataEvent=globals_st::dataEvent_t::ev_polling_time;
	de.tag.polling_time=gd.masterPollingTime;
	pushEvent(de,0);
	}

}

/**
 * get the actual system time (wrapper to use RTC or system time if PC)
 * @return time_t (epoch time)
 */
time_t Slave::getSystemTime()
{
time_t et;
#ifdef RASPBERRY
et=hwgpio->getRTC_datetime();
#else
et=time(NULL);
#endif
return et;
}

/**
 * check and run scheduled jobs
 */
void Slave::handleScheduledJobs()
{
if(!webCommandInProgress)
	{
	if(!sched->isEmpty())
		{
		char scriptResult[2000];
		time_t time_sched=sched->getInfoNextJob()->sched_time;
		time_t time_now=getSystemTime();
		if(time_sched>=0)	// fix: sometimes sched time is 0 (???) so this start immediately
			{
			if(time_sched<=time_now)
				{
				// execute command
				Scheduler::sched_t s=sched->popNextJob();
				dbg->trace(DBG_NOTIFY,"Sched: executing [" + s.command + "]");
				parseScriptLine(s.command,scriptResult);
				}
			}
		else
			{
			dbg->trace(DBG_WARNING,"Sched: schedule time cannot be < 0");
			sched->popNextJob();	// delete the job
			}
		}
	}
}


