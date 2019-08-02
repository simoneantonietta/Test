//============================================================================
// Name        : acqsrv.cpp
// Author      : Luca Mini
// Version     :
// Copyright   : Your copyright notice
// Description : Hello World in C++, Ansi-style
//============================================================================

#include <cstdlib>
#include <iostream>
#include <getopt.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>

#include "acq/AcqInterface.h"
#include "comm/SerCommTh.h"
#include "GlobalData.h"
#include "global.h"
#include "ServerInterface.h"
#include "CommSC8R.h"
#include "TestUtils.h"
#include "utils/Trace.h"
#include "version.h"

using namespace std;

Trace *dbg;
GlobalData *gd;
ServerInterface *srvif;
SerCommTh *comm;
AcqInterface *acq;
TestUtils *tu;
CommSC8R *sc8cal;
CommSC8R *sc8sen;

//-----------------------------------------------------------------------------
// PRIVATE
//-----------------------------------------------------------------------------
static void printUsage();
static void signal_callback_handler(int signum);

//=============================================================================
int main(int argc, char *argv[])
{
int c;
gd=new GlobalData();
srvif=new ServerInterface();
acq=new AcqInterface();
tu=new TestUtils();

cout << "ACQSYS ver. " << VERSION << " by L. Mini - SAET srl" << endl;

string traceOutput(TRACEOUTPUT_STDOUT);	// default

// setup the trace system
dbg=new Trace(MAX_MESSAGE_TRACES,APP_NAME,traceOutput.c_str());
dbg->addClassName(DBG_SKTSERVER,"SOCKET");
dbg->addClassName(DBG_SKTSERVER_DATA,"SOCKETDATA");
dbg->addClassName(DBG_ACQ,"ACQSYS");
dbg->setUseTimestamp(true);
dbg->set_all(true);
dbg->set(false,DBG_SKTSERVER_DATA,-1);

// load some defaults
gd->serverPort=SERVER_PORT;
gd->brate=BAUDRATE;
strcpy(gd->sc8rDeviceCal,SC8R_CAL_PORT);
strcpy(gd->sc8rDeviceSen,SC8R_SEN_PORT);
strcpy(gd->portDevice,SERIAL_PORT);
gd->myID=MYID;
gd->useSerial=true;
gd->useSC8Rcal=false;
gd->useSC8Rsen=true;
gd->forceSend2Unknown=true;

strcpy(gd->codeID,"0000");
gd->codeIDexp[0]=DUT_TYPE;
gd->codeIDexp[1]=0x00;
gd->codeIDexp[2]=0x00;
gd->codeIDexp[3]=0x00;

gd->parm_sensThreshold=185;

acq->init();
acq->setSafeCondition();
acq->printSummary(1);

#if 0
// esempio con iteratore
AcqInterface::chan_it current = acq->getIter("current");
cout << (*current)->getValue().fvalue << endl;

cout << acq->channel[0]->getName() << ": " << acq->channel[0]->getValue().ivalue << endl;
cout << acq->channel[0]->getName() << ": " << acq->channel[0]->getValue().ivalue << endl;
cout << acq->channel[0]->getName() << ": " << acq->channel[0]->getValue().ivalue << endl;
cout << acq->channel[0]->getName() << ": " << acq->channel[0]->getValue().ivalue << endl;

cout << acq->channel[8]->getName() << ": " << acq->channel[8]->getValue().ivalue << endl;
cout << acq->channel[8]->getName() << ": " << acq->channel[8]->getValue().ivalue << endl;
cout << acq->channel[8]->getName() << ": " << acq->channel[8]->getValue().ivalue << endl;
cout << acq->channel[8]->getName() << ": " << acq->channel[8]->getValue().ivalue << endl;

acq_t v;
v.ivalue=0;
acq->channel[10]->setValue(v);
v.ivalue=1;
acq->channel[10]->setValue(v);
v.ivalue=0;
acq->channel[10]->setValue(v);
v.ivalue=1;
acq->channel[10]->setValue(v);
#endif

while ((c = getopt(argc, argv, "fsmd:b:p:n:l:c:")) != -1)
	switch (c)
		{
		case 'd':
			strcpy((char*) gd->portDevice, optarg);
			break;
		case 'l':
			strcpy((char*) gd->sc8rDeviceSen, optarg);
			break;
		case 'c':
			strcpy((char*) gd->sc8rDeviceCal, optarg);
			break;
		case 'b':
			gd->brate = atoi(optarg);
			break;
		case 'p':
			gd->serverPort = atoi(optarg);
			break;
		case 'n':
			gd->myID = atoi(optarg);
			break;
		case 's':
			gd->useSerial=false;
			break;
		case 'm':
			gd->useSC8Rsen=false;
			break;
		case 'k':
			gd->useSC8Rcal=false;
			break;
		case 'f':
			dbg->trace(DBG_NOTIFY,"frame to unknown devices are NOT sent");
			gd->forceSend2Unknown=false;
			break;

		default:
			printUsage();
			abort();
		}

cout << APP_NAME << " ver. " << VERSION << endl;
dbg->trace(DBG_NOTIFY,"My ID: %d",gd->myID);
if(gd->useSerial)
	{
	dbg->trace(DBG_NOTIFY,"using SERIAL %s at %d baud",gd->portDevice,gd->brate);
	}
else
	{
	dbg->trace(DBG_NOTIFY,"serial interface disabled");
	}

// serial interface
if(gd->useSerial)
	{
	comm = new SerCommTh();
	if(!comm->openCommunication(gd->portDevice, gd->brate))
		{
		dbg->trace(DBG_ERROR,"serial interface: exiting");
		return 1;
		}
	comm->setMyId(gd->myID);
	comm->start(NULL);	// start the listener thread
#if 1
	// check connection
	comm->sendRequest(SPECBRD_ID,HPROT_CMD_CHKLNK,0,NULL,true);
	comm->waitOperation();
	if(comm->getResult())
		{
		dbg->trace(DBG_NOTIFY,"spec board connection checked");
		}
	else
		{
		dbg->trace(DBG_ERROR,"communication: " + comm->getErrorMessage());
		}
#endif
	}

//tu->fitToCurveAnalisys(59,102,185);


if(gd->useSC8Rcal)
	{
	sc8cal = new CommSC8R();
	tu->SC8_openComm(true,false);
#if 0
	// for debug
	tu->SC8_testMode(sc8cal);
	// here must press tamper
	cout << "PRESS TAMPER" << endl;
	sleep(5);
	for(int c=0;c<3;c++)	// make 3 attempts
		{
		sleep(1);
		if(tu->SC8_devCalibration()) break;
		}
#endif
	}

if(gd->useSC8Rsen)
	{
	sc8sen = new CommSC8R();
	tu->SC8_openComm(false,true);
#if 0
	// for debug
	tu->SC8_testMode(sc8sen);
	// here must press tamper
	cout << "PRESS TAMPER" << endl;
	sleep(5);
	for(int c=0;c<3;c++)	// make 3 attempts
		{
		sleep(1);
		if(tu->SC8_devMeasureSens()) break;
		}
#endif
	}

// create the server interface
if(!srvif->createServer(gd->serverPort,0,false))
	{
	dbg->trace(DBG_ERROR,"socket interface: exiting");
	return 1;
	}

// Register signal and signal handler
signal(SIGINT, signal_callback_handler);

//=============================================================================
while (1)
	{
	srvif->loop();
	}

// pratically the server never reach this point!
exit(EXIT_SUCCESS);

if(gd->useSerial)
	{
	comm->closeCommunication();
	comm->endApplicationRequest();
	}
//=============================================================================
delete acq;
return 0;
}

/**
 * print the usage of the application
 */
void printUsage()
{
cout << APP_NAME << " [opt]\n";
cout << "where [opt]:\n";
cout << "-d[serial]  serial device (i.e. /dev/ttyUSB0)\n";
cout << "-b[baud]    set the baudrate value\n";
cout << "-p[port]    socket port\n";
cout << "-n[myid]    server ID\n";
cout << "-l[serial]  serial device for SC8R sensitivity (i.e. /dev/ttyUSB1)\n";
cout << "-c[serial]  serial device for SC8R calibration (i.e. /dev/ttyUSB2)\n";
cout << "-s          disable serial port usage\n";
cout << "-m          disable SC8R sensitivity usage\n";
cout << "-k          disable SC8R calibration usage\n";
cout << "-f          not force to send frame to unknown devices ID\n";
cout << "\n";
cout << endl;
}

//-----------------------------------------------------------------------------
/**
 * Define the function to be called when ctrl-c (SIGINT) signal is sent to process
 * @param signum
 */
void signal_callback_handler(int signum)
{
printf("Caught signal %d\n", signum);
if(gd->useSerial)
	{
	comm->endApplicationRequest();
	comm->join();
	comm->closeCommunication();
	}

// Cleanup and close up stuff here
cout << "--TERMINATED--" << endl;

delete dbg;
delete srvif;
if(gd->useSerial) delete comm;
delete gd;
delete acq;
if(gd->useSC8Rcal) delete sc8cal;
if(gd->useSC8Rsen) delete sc8sen;

// Terminate program
exit(signum);
}

