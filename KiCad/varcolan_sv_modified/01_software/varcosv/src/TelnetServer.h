/**----------------------------------------------------------------------------
 * PROJECT: varcosv
 * PURPOSE:
 * 
 *-----------------------------------------------------------------------------  
 * CREATION: 14 Apr 2016
 * Author: Luca Mini
 * 
 * LICENCE: please see LICENCE.TXT file
 * 
 * HISTORY (of the module):
 *-----------------------------------------------------------------------------
 * Author              | Date        | Description
 *-----------------------------------------------------------------------------
 *-----------------------------------------------------------------------------
 */

#ifndef TELNETSERVER_H_
#define TELNETSERVER_H_

#include "SimpleServer.h"

#define SS_CLIENT_SAETCOM			1

class TelnetServer: public SimpleServer
{
public:
	TelnetServer();
	virtual ~TelnetServer();

	void onConnection(int client,int descr,char *host,char *port);
	void onDataRead(int client,char *data, int count);
	void writeData(int client, const char *data, int count);
	void onCloseConn(int client);

	int composeTelnetData(string preamble,uint8_t *data,int len,char *output);
private:

};

//-----------------------------------------------
#endif /* TELNETSERVER_H_ */
