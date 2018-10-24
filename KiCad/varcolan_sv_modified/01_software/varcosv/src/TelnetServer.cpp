/*
 *-----------------------------------------------------------------------------
 * PROJECT: varcosv
 * PURPOSE: see module TelnetServer.h file
 *-----------------------------------------------------------------------------
 */

#include "global.h"
#include "TelnetServer.h"
#include "common/utils/Trace.h"
#include "common/utils/Utils.h"
#include "Slave.h"

extern struct globals_st gd;
extern Trace *dbg;
extern Slave *slave;

/**
 * ctor
 */
TelnetServer::TelnetServer()
{
resetClient(-1);
}

/**
 * dtor
 */
TelnetServer::~TelnetServer()
{
}
/**
 * on connection event
 * @param client
 * @param descr
 * @param host
 * @param port
 */
void TelnetServer::onConnection(int client, int descr, char* host, char* port)
{
dbg->trace(DBG_NOTIFY,"connection from telnet interface, client %d", client);
string wellcome="varcolan supervisor ver " + _S VERSION + "\n";
this->writeData(client,wellcome.c_str(),wellcome.length());
}

/**
 * on command received
 * @param client
 * @param data
 * @param count
 */
void TelnetServer::onDataRead(int client, char* data, int count)
{
string line(data);
vector<string> toks;
char result[5000];
char reqid[20];
//this->writeData(client,data,count);
trim(line);
result[0]=0;
//for(unsigned int i=0;i<strlen(data);i++)
//	{
//	if(data[i]=='\r') data[i]=0;
//	if(data[i]=='\n') data[i]=0;
//	}
data[count]=0;
Split(data,toks,";\n\r");
for(unsigned int i=0;i<toks.size();i++)
	{
	if(gd.telnetUsePwd)
		{
		if(!getAuthentication(client))
			{
			if(toks[i]==gd.telnetPwd)
				{
				setAuthentication(client,true);
				this->writeData(client,"auth ok\n",sizeof("auth ok\n"));
				continue;
				}
			else
				{
				dbg->trace(DBG_WARNING,"wrong password");
				this->writeData(client,"ERROR PWD\n",sizeof("ERROR PWD\n"));
				return;
				}
			}
		}
	dbg->trace(DBG_NOTIFY,"telnet command: " + toks[i]);
	bool ret=slave->parseScriptLine(toks[i],result,reqid);
//	if(strcmp(reqid,"saetcom")==0)
//		{
//		setClientIdentifier(client,SS_CLIENT_SAETCOM);
//		TRACE(dbg,DBG_NOTIFY,"client %d has been identified as: %s",client,reqid);
//		gd.saetcomInterface=true;
//		gd.saetcomClient=client;
//		}

	if(strlen(result)>0)
		{
		strcat(result,"\n");
		this->writeData(client,result,strlen(result));
		}
	else // automatic answer
		{
		if(ret)
			{
			this->writeData(client,"OK\n",sizeof("OK\n"));
			}
		else
			{
			this->writeData(client,"ERROR\n",sizeof("ERROR\n"));
			}
		}
	}
}


/**
 * utility to generate ascii data for the saetcom process
 * output format:
 * preamble asciihex_data
 * @param preamble string that anticipate the following asciihex data
 * @param data
 * @param output contains the output. This maust be deallocated buy the user with delete []
 * @return number of data in output
 */
int TelnetServer::composeTelnetData(string preamble,uint8_t *data,int len,char *output)
{
int p=preamble.length();
output=new char[p+len*2+1];
strcpy(output,preamble.c_str());
strcpy(output+p,"|");
p++;
Hex2AsciiHex(output+p,data,len,false,0);
p+=len*2;
return p;
}


/**
 * write data to telnet
 * @param client
 * @param data
 * @param count
 */
void TelnetServer::writeData(int client, const char* data, int count)
{
if(clientIsConnected(client))
	{
	SimpleServer::writeData(client,data,count);
	}
}

/**
 * on close connection
 * @param client
 */
void TelnetServer::onCloseConn(int client)
{
resetClient(client);
dbg->trace(DBG_NOTIFY,"connection from telnet interface CLOSED");
}



