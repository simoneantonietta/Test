//============================================================================
// Name        : scserver.cpp
// Author      : 
// Version     :
// Copyright   : Your copyright notice
// Description : server to test saetcom udp communication with supervisor
//============================================================================

#include <iostream>

#include "Global.h"
#include "version.h"
#include "SaetCommSrv.h"

using namespace std;

//-----------------------------------------------------------------------------
// PROTOTYPES
//-----------------------------------------------------------------------------
void endingApplication();
void printUsage();

//-----------------------------------------------------------------------------
// GLOBALS
//-----------------------------------------------------------------------------
extern SaetCommSrv *scsrv;

//=============================================================================

int main(int argc, char **argv)
{
dbg=new Trace(MAX_MESSAGE_TRACES,"SCSIM",NULL);
dbg->set_all(true);
dbg->setUseTimestamp(true);

scsrv=new SaetCommSrv();

//dbg->trace(DBG_NOTIFY,"***STARTUP***");
dbg->trace(DBG_NOTIFY,"SaetCom server simulator - ver. %s",VERSION);

if(argc<4)
	{
	printUsage();
	return 0;
	}

gd.client.addr=(string) argv[1];
gd.client.port=atoi(argv[2]);
gd.client.plantId=atoi(argv[3]);

scsrv->initComm(gd.client.addr,gd.client.port,gd.client.plantId);

while(gd.run)
	{
	sleep(1);
	}

endingApplication();
}

//=============================================================================

/**
 * routine to be called before ending the application
 */
void endingApplication()
{
TRACE(dbg,DBG_NOTIFY,"ending application");
sleep(1);

delete cfg;
delete dbg;
}


/**
 * print the usage of the utility
 */
void printUsage()
{
cout << APPLICATION_NAME << " usage:\n";
cout << "params: <addr> <port> <plant_id>\n";
cout << endl;
}

