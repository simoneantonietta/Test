/*
 *-----------------------------------------------------------------------------
 * PROJECT: rasp_saetcom
 * PURPOSE: see module global.h file
 *-----------------------------------------------------------------------------
 */

#include "Global.h"
#include "UdpServer.h"
#include "SaetCommSrv.h"

Trace *dbg;
SimpleCfgFile *cfg;
Global gd;
SaetCommSrv *scsrv;

//=============================================================================

/**
 * ctor
 */
Global::Global()
{
run=true;
}

/**
 * dtor
 */
Global::~Global()
{

}
