//============================================================================
// Name        : varcolansv_arm.cpp
// Author      : 
// Version     :
// Copyright   : Your copyright notice
// Description : Hello World in C++, Ansi-style
//============================================================================

/*
 * algorithm:
 * main State Machine:
 * 1. init
 * 2. connection telnet
 * 3. connection saetcom
 * 4. listen
 * 		4.1. listen telnet
 * 			{
 * 			}
 * 		4.2. listen gems
 * 			{
 * 			}
 * 		4.3. check saetcom connection (keepalive)
 * 			if not goto 3
 * 		4.4. check telnet connection (keepalive)
 * 			if not goto 2
 *
 */


#include <signal.h>
#include <sys/time.h>
#include <semaphore.h>

#include "Global.h"
#include "version.h"

#include "SaetComm.h"
#include "TelnetInterface.h"

//-----------------------------------------------------------------------------
// GLOBALS
//-----------------------------------------------------------------------------
extern SaetComm *sc;
extern TelnetInterface *tc;
//-----------------------------------------------------------------------------
// PROTOTYPES
//-----------------------------------------------------------------------------
void endingApplication();
void printUsage();
//-----------------------------------------------------------------------------
// LOCALS
//-----------------------------------------------------------------------------

//=============================================================================
int main()
{
SET_FW_VERSION();
SET_FW_NAME();

//----------------------------------
// main status
enum mainsm_e
	{
	main_init,

	main_telnet_connection,
	main_telnet_listen,
	main_telnet_check_com,

	main_saetcom_connection,
	main_saetcom_listen,
	main_saetcom_check_com,

	main_error,
	} mainStatus=main_init;
//----------------------------------

bool run=true;

dbg=new Trace(MAX_MESSAGE_TRACES,"SAETCOM",NULL);
dbg->set_all(true);
dbg->setUseTimestamp(true);

//dbg->trace(DBG_NOTIFY,"***STARTUP***");
dbg->trace(DBG_NOTIFY,"VarcoLAN SV SAETCOM module - ver. %s",VERSION);

sc=new SaetComm();
tc=new TelnetInterface();

gd.gemss.ip="192.168.30.242";
gd.gemss.port=4006;

gd.plantID=130;

// saetcom interface opening
if(sc->openComm())
	{
	sc->startThread(NULL,'r');
	}
else
	{
	endingApplication();
	return 1;	// exit!
	}

// saetcom telnet opening
if(tc->openConnection(TELNET_SV_ADDRESS,TELNET_SV_PORT))
	{
	tc->startThread(NULL,'r');
	TRACE(dbg,DBG_NOTIFY,"telnet interface opened");
	}
else
	{
	endingApplication();
	return 1;	// exit!
	}

//=============================================================
// main loop
while(run)
	{
	sc->checkConnectionTimeout();

	switch(mainStatus)
		{
		//......................................
		case main_init:
			TRACE(dbg,DBG_NOTIFY,"starting communications...");

			// saetcom registration
			sc->registrationRequest();
			mainStatus=main_telnet_connection;
			break;
		//......................................
		case main_telnet_connection:
			// check the first connection
			tc->sendData("$(saetcom) test",sizeof("$(saetcom) test"));
			if(tc->waitAnswer(TELNET_SV_ANS_TIMEOUT))
				{
				char tcans[SIMPLECLIENT_RX_BUFF_SIZE];
				tc->getRXData(tcans);
				TRACE(dbg,DBG_NOTIFY,"sv answer: %s",tcans);

				mainStatus=main_saetcom_connection;
				}
			else
				{
				TRACE(dbg,DBG_ERROR,"no answer received, sv communication shutdown");
				mainStatus=main_error;
				}

			break;
		//......................................
		case main_saetcom_connection:
			if(sc->isGemmssConnected_cripto())
				{
				mainStatus=main_telnet_listen;
				}
			break;

		//......................................
		case main_telnet_listen:
			mainStatus=main_saetcom_listen;
			// TODO gestisce coda telnet
			break;
		//......................................
		case main_saetcom_listen:
			mainStatus=main_telnet_check_com;
			// TODO gestisce coda SC
			break;
		//......................................
		case main_telnet_check_com:
			// TODO verifica connessione telnet
			mainStatus=main_saetcom_check_com;
			break;
		//......................................
		case main_saetcom_check_com:
			// TODO verifica connessione SC
			mainStatus=main_telnet_listen;
			break;

		//......................................
		case main_error:
			mainStatus=main_saetcom_connection;
			break;
		}
	}
//=============================================================

#if 0
while(1)
	{

	sleep(1);
	sc->checkConnectionTimeout();

	tc->mainLoop();

	// check TX message queue
	if(!sc->datagramQueueTX.empty())
		{
		SC_datagram_t dg;
		dg=sc->datagramQueueTX.front();
		sc->datagramQueueTX.pop();
		// todo: transmit event to gemms
		}

	// check for an expected answer, and retry if none
	if(sc->isWaitingAnswer() && sc->ansTimeout.checkTimeout())
		{
		// todo handle retry
		}

	}
//sc->registrationRequest();
//sc->getCredentials();
#endif
endingApplication();
return 0;
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
cout << endl;
}

