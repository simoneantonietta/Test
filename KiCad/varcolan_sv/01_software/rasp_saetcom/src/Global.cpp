/*
 *-----------------------------------------------------------------------------
 * PROJECT: rasp_saetcom
 * PURPOSE: see module global.h file
 *-----------------------------------------------------------------------------
 */

#include "Global.h"

#include "SaetComm.h"
#include "SimpleClient.h"

Trace *dbg;
SimpleCfgFile *cfg;
Global gd;

// not so globals :D
SaetComm *sc;
SimpleClient *tc;
//=============================================================================

/**
 * ctor
 */
Global::Global()
{
eth_interface="eth0";
myPort=UDP_PORT_SV;
gemss.port=UDP_PORT_GEMSS;
plantID=1;	// default plant id
connectionExpireTime=30;
}

/**
 * dtor
 */
Global::~Global()
{

}
