//============================================================================
// Name        : supervisor.cpp
// Author      : L. Mini
// Version     :
// Copyright   : Your copyright notice
// Description : Varco supervisor
//============================================================================

#include <getopt.h>
#include <signal.h>
#include "version.h"
#include "comm/SerCommTh.h"
#include "RXHFrame.h"
#include "TXHFrame.h"
#include "BadgeReader.h"
#include "comm/SktCliCommTh.h"
#include "comm/SktSrvCommTh.h"
#include "comm/UDPCommTh.h"
#include "common/utils/Trace.h"
#include "common/utils/SimpleCfgFile.h"
#include "db/DBWrapper.h"
#include "Slave.h"
#include "TelnetServer.h"
#include "UDPProtocolDebug.h"
#include "Scheduler.h"
#include "master/MasterInterface.h"
#include "HardwareGPIO.h"
#include "Ntp_datetime.h"

using namespace std;

Trace *dbg;
SerCommTh *serComm;
SktCliCommTh *sktCliComm;
SktSrvCommTh *sktSrvComm;
UDPCommTh *udpSrvComm;
CommTh *comm;
RXHFrame *rf;
TXHFrame *tf;
BadgeReader *br;
struct globals_st gd;
SimpleCfgFile *cfg;
DBWrapper *db;
Slave *slave;
TelnetServer *tsrv;
UDPProtocolDebug *udpDbg;
Scheduler *sched;
MasterInterface *svm;

#ifdef RASPBERRY
HardwareGPIO *hwgpio;
#endif

//-----------------------------------------------------------------------------
// PROTOTYPES
//-----------------------------------------------------------------------------
void endingApplication();
void printUsage();
void signal_brkpipe_callback_handler(int signum);
//=============================================================================
int main(int argc, char *argv[])
{
SET_FW_VERSION();
SET_FW_NAME();
HPROT_SET_FW_VERSION();
int c;
int ret=0;
string tmp;
TimeFuncs aliveTime;
unsigned int aliveHours=0;
Ntp_datetime ntpdt;

dbg=new Trace(MAX_MESSAGE_TRACES,APP_NAME,NULL);

dbg->addClassName(DBG_SVM_NOTIFY,"SVM_NOTIFY");
dbg->addClassName(DBG_SVM_ERROR,"SVM_ERROR");
dbg->addClassName(DBG_SVM_WARNING,"SVM_WARNING");
dbg->addClassName(DBG_SVM_FATAL,"SVM_FATAL");
dbg->addClassName(DBG_SVM_DEBUG,"SVM_DEBUG");

dbg->set_all(true);
dbg->setUseTimestamp(true);

dbg->trace(DBG_NOTIFY,"***STARTUP***");
dbg->trace(DBG_NOTIFY,"VarcoLAN SV - ver. %s",VERSION);

aliveTime.initTime_s();

// set some defaults
gd.cfgChanged=false;
gd.myID=0;
gd.brate=0;
gd.device[0]=0;
gd.port=5000;
gd.useSerial=true;
gd.useSocketClient=false;
gd.connected=false;
gd.badgeAutoSend=false;
gd.dbSaveTime=1200;			// [s]
gd.dumpFromMaster=false;
gd.dumpTriggerFromCfg=false;
gd.masterPollingTime=DEF_MASTER_POLLING_TIME;
gd.masterPollingTime_def=DEF_MASTER_POLLING_TIME;
gd.masterAliveTime=DEF_ALIVE_TIME;
gd.deadCounter=0;
gd.cklinkCounter=0;
gd.badgeAcq.waitingForBadge=false;
gd.globalErrors=0;
gd.globalErrors_th=GLOBAL_ERROR_THRESHOLD;
gd.forceReset=false;
gd.badgeAcq.wait_timeout=WAIT_FOR_BADGE_TIMEOUT;
gd.maxBadgeId=0;

gd.svm.keepalive_time=SVM_CONN_KEEP_ALIVE_TIME;
gd.svm.svcID=300;
gd.svmDebugMode=false;
//-----------------------------------------------------------------------------
// get configuration file
//-----------------------------------------------------------------------------
cfg=new SimpleCfgFile();
dbg->trace(DBG_NOTIFY,"\t\tI'm running in the folder:");
system("pwd");

#ifdef RASPBERRY
dbg->trace(DBG_NOTIFY,"set working directory to:");
chdir(RASPBERRY_WORKING_DIR);
system("pwd");
#endif

if(cfg->loadCfgFile(CFG_FILE))
	{
	string tmp;
	gd.myID=cfg->getValue_int("MY_ID",MYID);
	gd.defaultDest=(uint8_t)cfg->getValue_int("DEFAULT_DEST","1");

	tmp=cfg->getValueOf("DEVICE");
	strcpy(gd.device,tmp.c_str());

	gd.brate=cfg->getValue_int("BAUD_RATE",BAUDRATE);
	gd.port=cfg->getValue_int("PORT",PORT);

	gd.has_reader=cfg->getValue_bool("HAS_READER", "1");
	tmp=cfg->getValueOf("READER_PORT");
	strcpy(gd.reader_device,tmp.c_str());
	gd.reader_brate=cfg->getValue_int("READER_BAUD_RATE","1200");

	gd.terminalAsReader_id=cfg->getValue_int("TERMINAL_AS_READER_ID","0");

	//gd.devAnsTimeout=cfg->getValue_int("TIMEOUT_PER_DEVICE","10");
	gd.devAnsTimeout=ANSWER_TIME_EACH_DEVICE_MS;

	gd.usefifo=cfg->getValue_bool("USE_FIFO", "N");
	gd.fifoName=cfg->getValueOf("FIFO_FILE");

	gd.useTelnet=cfg->getValue_bool("USE_TELNET", "0");
	strcpy(gd.telnetPort,cfg->getValueOf("TELNET_PORT").c_str());
	gd.telnetLatency=cfg->getValue_int("TELNET_LATENCY","10");
	gd.telnetPwd=cfg->getValueOf("TELNET_PWD","-");
	if(gd.telnetPwd=="-")	// if password is not used
		{
		gd.telnetUsePwd=false;
		}
	else
		{
		gd.telnetUsePwd=true;
		}

	gd.dbRamFolder=cfg->getValueOf("DB_RAM_FOLDER");
	gd.dbPersistFolder=cfg->getValueOf("DB_PERSISTENT_FOLDER");
	gd.maxEventPerTable=cfg->getValue_int("MAX_EVENTS","5000");

	gd.dbSaveTime=cfg->getValue_int("DB_SAVE_TIME","12000");

	gd.dumpFromMaster=cfg->getValue_bool("DUMP_FROM_MASTER", "0");
	gd.dumpTriggerFromCfg=gd.dumpFromMaster;

	gd.loopLatency=cfg->getValue_int("LOOP_LATENCY","500")*1000;

	gd.enableChangePollingTime=cfg->getValue_bool("CHANGE_POLLING", "1");
	gd.masterPollingTime=(uint16_t)cfg->getValue_int("POLLING_TIME","500");
	gd.masterPollingTime_def=gd.masterPollingTime;
	gd.masterAliveTime=cfg->getValue_int("ALIVE_TIME","20");

	gd.badgeAutoSend=cfg->getValue_bool("BADGE_AUTO_SEND", "N");

	gd.useUdpDebug=cfg->getValue_bool("USE_UDP_DEBUG", "0");
	gd.udpDbg_ip=cfg->getValueOf("UDP_DEBUG_IP", "0.0.0.0");
	gd.udpDbg_port=cfg->getValue_int("UDP_DEBUG_PORT", "6001");

	gd.enableWatchdog=cfg->getValue_bool("ENABLE_WATCHDOG", "Y");
	gd.watchdogTime=cfg->getValue_int("WATCHDOG_TIME","120");
	gd.globalErrors_th=cfg->getValue_int("GLOBAL_ERROR_THRESHOLD","100");

#ifdef HPROT_USE_CRYPTO
	string ktmp=cfg->getValueOf("CRYPTO_KEY","default");
	if(ktmp=="default")
		{
		gd.hprotKey[0]=HPROT_CRYPT_DEFAULT_KEY0;
		gd.hprotKey[1]=HPROT_CRYPT_DEFAULT_KEY1;
		gd.hprotKey[2]=HPROT_CRYPT_DEFAULT_KEY2;
		gd.hprotKey[3]=HPROT_CRYPT_DEFAULT_KEY3;
		}
	else
		{
		AsciiHex2Hex(gd.hprotKey,(char *)cfg->getValueOf("CRYPTO_KEY").c_str(),HPROT_CRYPT_KEY_SIZE);
		}
	dbg->trace(DBG_NOTIFY,"key loaded: %02X%02X%02X%02X", gd.hprotKey[0],gd.hprotKey[1],gd.hprotKey[2],gd.hprotKey[3]);

	gd.enableCrypto=cfg->getValue_bool("ENABLE_CRYPTO","N");
	if(gd.enableCrypto)
		{
		if(hprotCheckKey(gd.hprotKey)==1)
			{
			dbg->trace(DBG_NOTIFY,"crypto enabled");
			}
		else
			{
			dbg->trace(DBG_NOTIFY,"crypto not enabled due to invalid key -> plain mode communication");
			}
		}
	else
		{
		dbg->trace(DBG_NOTIFY,"crypto disabled -> plain mode communication");
		}
#endif
	//...........................................................
	// SVM configs
	gd.useSVM=cfg->getValue_bool("USE_SVM", "N");
	if(gd.useSVM)
		{
		gd.svm.ip=cfg->getValueOf("SVM_IP_DEFAULT", "0.0.0.0");
		gd.svm.port=cfg->getValue_int("SVM_PORT","4006");
		gd.svm.plantID=cfg->getValue_int("SVM_PLANT_ID","1000");
		}
	//...........................................................
	gd.svmDebugMode=cfg->getValue_bool("SVM_DEBUG_MODE", "N");
	}
else
	{
	dbg->trace(DBG_ERROR,"configuration file not found");
	}

//-----------------------------------------------------------------------------
// Handle command line
//-----------------------------------------------------------------------------
while((c = getopt(argc, argv, "d:")) != -1)
	{
	switch(c)
		{
		case 'd':	// delete database table
			db->open();
			db->cleanTable(optarg[0]);
			db->close();
			exit(1);
			break;
//		case 'd':	// device
//			gd.serial_flag = true;
//			strcpy((char*) gd.device, optarg);
//			break;
		default:
			printUsage();
			exit(1);
		}
	}
//-----------------------------------------------------------------------------
// Other inits
//-----------------------------------------------------------------------------
rf=new RXHFrame();
tf=new TXHFrame();
br=new BadgeReader();
db=new DBWrapper(gd.dbPersistFolder+DB_VARCOLANSV,gd.dbRamFolder+DB_VARCOLANEV);
slave=new Slave();
udpDbg=new UDPProtocolDebug();
sched=new Scheduler(JOB_FILE);
#ifdef RASPBERRY
//export WIRINGPI_GPIOMEM=1
dbg->trace(DBG_NOTIFY,"Enabling Raspberry Pi HW GPIO");
hwgpio=new HardwareGPIO();
HW_GPIO_INIT();
#endif

//-----------------------------------------------------------------------------
// Get time from NTP server
//-----------------------------------------------------------------------------

time_t ntim=ntpdt.ntpdate();
if(ntim==0)
	{
	TRACE(dbg,DBG_WARNING, "NTP server not found");
	}
else
	{
	int year,month,day,hour,min,secs;
	char dt[20];// = "YYYYMMDDhhmmss";
	//TRACE(dbg,DBG_WARNING, "NTP server found, time update to %s",ctime(&ntim));
	ntim=ntpdt.calcLocalEpochTime(ntim,+1);
	TRACE(dbg,DBG_NOTIFY, "NTP server found, time update to %s",ctime(&ntim));
	ntpdt.splitTime(ntim,&year,&month,&day,&hour,&min,&secs);

#ifdef RASPBERRY
	//YYYYMMDDhhmmss
	sprintf(dt,"%04d%02d%02d%02d%02d%02d", year, month, day, hour, min, secs);
	hwgpio->setRTC_datetime(dt);
	system("sudo rtc_spi");
	sleep(1);
#endif
	}


#ifdef RASPBERRY
dbg->trace(DBG_NOTIFY,"started at "+aliveTime.datetime_fmt("%D %T",hwgpio->getRTC_datetime()));
#else
dbg->trace(DBG_NOTIFY,"started at "+aliveTime.datetime_now("%D %T"));
#endif

// database
db->db2Ram();
sleep(1);
db->open();
if(db->loadWeektimes()) TRACE(dbg,DBG_NOTIFY,"weektimes loaded");
if(db->loadProfiles()) TRACE(dbg,DBG_NOTIFY,"profiles loaded");
if(db->loadTerminals()) TRACE(dbg,DBG_NOTIFY,"terminals loaded");
if(db->loadCausals())
	{
	TRACE(dbg,DBG_NOTIFY,"causals loaded");
	}
else
	{
	TRACE(dbg,DBG_NOTIFY,"set some default causals");
	db->setCausalDefault();
	// reload
	db->loadCausals();
	}


db->updateUnsyncedEvents();
TRACE(dbg,DBG_NOTIFY,"unsynched event update");
db->updateMostRecentBadgeTimestamp();
TRACE(dbg,DBG_NOTIFY,"most recent badge timestamp update");

gd.maxBadgeId=db->getBadgeMaxId();
TRACE(dbg,DBG_NOTIFY,"Badge max ID = %d",gd.maxBadgeId);


// enable telnet interface
if(gd.useTelnet)
	{
	dbg->trace(DBG_NOTIFY,"Using Telnet for interacting");
	tsrv = new TelnetServer();
	tsrv->initServer(gd.telnetPort,gd.telnetLatency);
	}

// badge reader
if(gd.has_reader)
	{
	if(!br->init(gd.reader_device,gd.reader_brate))
		{
		dbg->trace(DBG_WARNING,"reader disabled");
		gd.has_reader=false;
		}
	}




//while(1)
//	{
//	if(gd.useSVM)
//		{
//		svm->mainLoop();
//		}
//	}


//hwgpio->setRTC_datetime("20000101010101");
//hwgpio->writeSysUpdTimeFile("20000101010101");
//hwgpio->getRTC_datetime();
//-----------------------------------------------------------------------------
// check for the communication channel
//-----------------------------------------------------------------------------
HW_BLINK_WAIT_NET();
serComm = new SerCommTh();
serComm->setNewKey(gd.hprotKey);
serComm->enableCrypto(gd.enableCrypto);
#ifdef USE_UDP_PROTOCOL
udpSrvComm = new UDPCommTh(gd.myID);
udpSrvComm->setNewKey(gd.hprotKey);
udpSrvComm->enableCrypto(gd.enableCrypto);
#else
sktSrvComm = new SktSrvCommTh();
sktSrvComm->setNewKey(gd.hprotKey);
sktSrvComm->enableCrypto(gd.enableCrypto);
#endif

try
	{
#if 1
	if(gd.usefifo)
		{
		// We expect write failures to occur but we want to handle them where
		// the error occurs rather than in a SIGPIPE handler.
		signal(SIGPIPE, signal_brkpipe_callback_handler);
		//signal(SIGPIPE, SIG_IGN);	// will ignore signal
		dbg->trace(DBG_NOTIFY,"opening fifo " + gd.fifoName);
		if(!serComm->openFifo(gd.fifoName))
			{
			dbg->trace(DBG_WARNING,"unable to open fifo " + gd.fifoName + ", sniffer is not present?");
			gd.usefifo=false;
			}
		else
			{
#ifdef USE_UDP_PROTOCOL
			udpSrvComm->setFifoHandler(serComm->getFifoHandler());
#else
			sktSrvComm->setFifoHandler(serComm->getFifoHandler());
#endif
			}
		}
#endif

	if(gd.useUdpDebug)
		{
		dbg->trace(DBG_NOTIFY,"opening Debug UDP @ip %s port %d", gd.udpDbg_ip.c_str(), gd.udpDbg_port);
		if(!udpDbg->openUdp(gd.udpDbg_ip,gd.udpDbg_port))
			{
			dbg->trace(DBG_WARNING,"unable to open the debug UDP interface");
			gd.usefifo=false;
			}
		else
			{
			udpDbg->startThread(NULL,'r');
#ifdef COMM_TH_UDP_DEBUG
			serComm->setUDPDebugInterface(udpDbg);
			serComm->setUseUDPDebug(true);

#ifdef USE_UDP_PROTOCOL
			udpSrvComm->setUDPDebugInterface(udpDbg);
			udpSrvComm->setUseUDPDebug(true);
#else
			sktSrvComm->setUDPDebugInterface(udpDbg);
			sktSrvComm->setUseUDPDebug(true);
#endif
#endif // COMM_TH_UDP_DEBUG
			}
		}

	// serial
	if(!serComm->openCommunication(gd.device, gd.brate))
		{
		TRACE(dbg,DBG_ERROR,"cannot open serial port");
#ifdef RASPBERRY
		throw false;
#else
		TRACE(dbg,DBG_WARNING,"Serial port will be ignored");
		gd.useSerial=false;
#endif
		}
	else
		{
		serComm->setMyId(gd.myID);
		serComm->clearLinkCheck();
		serComm->startThread(NULL,'r');	// start the listener thread
		}

	// server
#ifdef USE_UDP_PROTOCOL
	if(!udpSrvComm->openCommunication(gd.device, gd.port))
		{
		dbg->trace(DBG_ERROR,"cannot open IP socket");
		throw false;
		}
	else
		{
		udpSrvComm->setMyId(gd.myID);
		udpSrvComm->clearLinkCheck();
		udpSrvComm->startThread(NULL,'r');	// start the listener thread
		}
#else
	if(!sktSrvComm->openCommunication(gd.device, gd.port))
		{
		dbg->trace(DBG_ERROR,"cannot open IP socket");
		throw false;
		}
	else
		{
		sktSrvComm->setMyId(gd.myID);
		sktSrvComm->clearLinkCheck();
		sktSrvComm->startThread(NULL,'r');	// start the listener thread
		}
#endif
	dbg->trace(DBG_NOTIFY,"waiting for a CheckLink in Serial or LAN ...");

	// install the SVM interface if need
	if(gd.useSVM)
		{
		svm=new MasterInterface();
		if(svm->initComm(gd.svm.ip,gd.svm.port,gd.svm.plantID))
			{
			TRACE(dbg,DBG_SVM_NOTIFY,"SVM interface socket opened");
			}
		else
			{
			TRACE(dbg,DBG_SVM_ERROR,"cannot open socket for SVM");
			gd.useSVM=false;
			}
		}

	gd.useSerial=false;
	gd.useSocketClient=false;
	gd.useSocketServer=false;

	bool netFound=false;
	int _recoverycheckcnt=500;
	while(!netFound)
		{
		// this allow recovery in this phase
		if(_recoverycheckcnt==0)
			{
			slave->recoveryCheck();
			_recoverycheckcnt=500;
			}
		else
			{
			_recoverycheckcnt--;
			}

		// this to allow telnet in this phase
		if(gd.useTelnet)
			{
			tsrv->mainLoop();
			}

		if(serComm->getLinkCheckedStatus())
			{
			gd.useSerial=true;
			gd.useSocketClient=false;
			gd.useSocketServer=false;
			netFound=true;
			dbg->trace(DBG_NOTIFY,"serial communication found");
			}
#ifdef USE_UDP_PROTOCOL
		if(udpSrvComm->getLinkCheckedStatus())
			{
			gd.useSerial=false;
			gd.useSocketClient=false;
			gd.useSocketServer=true;
			netFound=true;
			dbg->trace(DBG_NOTIFY,"LAN communication (UDP) found");
			}
#else
		if(sktSrvComm->getLinkCheckedStatus())
			{
			gd.useSerial=false;
			gd.useSocketClient=false;
			gd.useSocketServer=true;
			netFound=true;
			dbg->trace(DBG_NOTIFY,"LAN communication (TCP) found");
			}
#endif
		usleep(10000);
		}

	// network found...

	if(!gd.useSerial)
		{
		// close serial
		serComm->setUseFifo(false);
		serComm->closeCommunication();
		//serComm->joinThread('r');
		//serComm->joinThread('t');
		dbg->trace(DBG_NOTIFY,"serial disconnected");
		}

	if(!gd.useSocketServer)
		{
#ifdef USE_UDP_PROTOCOL
		// close server
		udpSrvComm->setUseFifo(false);
		udpSrvComm->closeCommunication();
		//udpSrvComm->joinThread('r');
		//udpSrvComm->joinThread('t');
		dbg->trace(DBG_NOTIFY,"server disconnected");
#else
		sktSrvComm->setUseFifo(false);
		sktSrvComm->closeCommunication();
		sktSrvComm->joinThread('r');
		gd.connected=false;
		dbg->trace(DBG_NOTIFY,"server disconnected");
#endif
		}
	}
catch(bool res)
	{
	TRACE(dbg,DBG_NOTIFY,"cannot find network");
	gd.run=false;
	gd.usefifo=false;		// to avoid close of a fifo not yet opened
	endingApplication();
	return ret;
	}
//-----------------------------------------------------------------------------

tf->setCommInterface(&comm);

#if 0
if(gd.usefifo)
	{
	// We expect write failures to occur but we want to handle them where
	// the error occurs rather than in a SIGPIPE handler.
	signal(SIGPIPE, signal_brkpipe_callback_handler);
	//signal(SIGPIPE, SIG_IGN);	// will ignore signal
	dbg->trace(DBG_NOTIFY,"opening fifo " + gd.fifoName);
	if(!comm->openFifo(gd.fifoName))
		{
		TRACE(dbg,DBG_WARNING,"unable to open fifo " + gd.fifoName + ", sniffer is not present?");
		gd.usefifo=false;
		}
	}
#endif


slave->firmwareLoadData();
sched->loadJobs();

gd.run=true;
usleep(50000);

//-----------------------------------------------------------------------------
// main loop
//-----------------------------------------------------------------------------
#ifdef RASPBERRY
if(gd.enableWatchdog)
	{
	if(!hwgpio->watchdogStart(gd.watchdogTime))
		{
		TRACE(dbg,DBG_WARNING,"watchdog cannot be used -> will be disabled");
		gd.enableWatchdog=false;
		}
	}
else
	{
	TRACE(dbg,DBG_NOTIFY,"watchdog disabled");
	}
#endif

Badge_parameters_t b;
b.Parameters_s.ID = 1;
b.Parameters_s.badge[0].full = 0x12;
b.Parameters_s.badge[1].full = 0x23;
b.Parameters_s.badge_status = badge_enabled;
b.Parameters_s.profilesID[0] = 0;
b.Parameters_s.user_type = base;
b.Parameters_s.visitor = false;
b.crc8 = CRC8calc((uint8_t*)&b, sizeof(b.Parameters_s));

//db->setBadge(b);

//###################################################################à
HW_BLINK_ALIVE();
while(gd.run)
	{
	slave->mainLoop();
	if(gd.useTelnet)
		{
		tsrv->mainLoop();
		}

	// save cfg file if need
	if(gd.cfgChanged==true)
		{
		if(cfg->saveCfgFile())
			{
			dbg->trace(DBG_NOTIFY,"cfg file saved");
			}
		else
			{
			dbg->trace(DBG_ERROR,"cfg file cannot be saved");
			}
		system("sync");
		gd.cfgChanged=false;
		}

	if(aliveTime.getElapsedTime_s()>3600)
		{
		aliveTime.initTime_s();	// reset
		aliveHours++;
		dbg->trace(DBG_NOTIFY,"alive by %u hours (about %u days)",aliveHours,aliveHours/24);
		}

#ifdef RASPBERRY
	WATCHDOG_REFRESH();

	// check for global errors
	if(gd.globalErrors > 0)
		{
		if(gd.globalErrors > gd.globalErrors_th)
			{
			TRACE(dbg,DBG_NOTIFY,"Global Errors reach the threshold, the system will be reset soon");
			gd.forceReset=true;
			}
		}
#endif

	if(gd.useSVM)
		{
		svm->mainLoop();
		}

	usleep(gd.loopLatency);
	}
//-----------------------------------------------------------------------------
// ENDING
//-----------------------------------------------------------------------------
endingApplication();
return ret;
}

//=============================================================================

/**
 * routine to be called before ending the application
 */
void endingApplication()
{
if(gd.usefifo)
	{
	comm->closeFifo();
	}

dbg->trace(DBG_NOTIFY,"ending application");
sleep(1);
db->close();
db->dbSave();
serComm->closeCommunication();
udpDbg->closeComm();
delete serComm;
#ifdef USE_UDP_PROTOCOL
udpSrvComm->closeCommunication();
if(gd.useSocketServer) delete udpSrvComm;
#else
if(gd.useSocketClient)
	{
	sktCliComm->closeCommunication();
	delete sktCliComm;
	}
if(gd.useSocketServer)
	{
	sktSrvComm->closeCommunication();
	delete sktSrvComm;
	}
#endif
delete cfg;
delete rf;
delete dbg;
delete br;
delete db;
delete slave;
delete udpDbg;
delete sched;

if(gd.useSVM)
	{
	delete svm;
	}

#ifdef RASPBERRY
if(gd.enableWatchdog)
	{
	hwgpio->watchdogStop();
	}
hwgpio->closeHw();
delete hwgpio;
#endif

//delete master;
}


/**
 * print the usage of the utility
 */
void printUsage()
{
cout << "-d[p|w|b|e]      delete all data in the table specified\n";
cout << "                 p=profile;w=weektime;b=badge;e=event\n";
//cout << "-p[port]    serial device (i.e. /dev/ttyUSB0)\n";
//cout << "-b[baud]    set the baudrate value\n";
//cout << "-k[ip:port] set the server IP:port i.e. localhost:5000, 192.168.1.10:5000\n";
//cout << "-f[file]    file containing commands\n";
//
//cout << "\nexamples in interactive mode:\n";
//cout << "r.03.01.03\n";
//cout << "c.02.01.21.x[12.34.5.de]d[12.45.99]s[helpme]\n";
//cout << "\nNOTE: if socket mode (param -k) is used, serial parameters are ignored\n";
cout << endl;
}

/* Catch Signal Handler functio */
/**
 * signal handler for broken pipe (tipically used for FIFO in this case
 * @param signum
 */
void signal_brkpipe_callback_handler(int signum)
{
dbg->trace(DBG_WARNING,"Caught signal SIGPIPE %d - fifo turn-off", signum);
gd.usefifo=false;
comm->closeFifo();
}

