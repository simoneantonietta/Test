/**----------------------------------------------------------------------------
 * PROJECT: rasp_saetcom
 * PURPOSE:
 * simple tcp client
 *-----------------------------------------------------------------------------  
 * CREATION: Nov 16, 2016
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

#ifndef SIMPLECLIENT_H_
#define SIMPLECLIENT_H_

#include <string>
#include <iostream>
#include <stdio.h>
#include	<sys/socket.h>    //socket
#include	<arpa/inet.h> 		//inet_addr

#define SIMPLECLIENT_RX_BUFF_SIZE		512
//#define SIMPLECLIENT_RX_BLOCKING


using namespace std;
/*
 *
 */
class SimpleClient
{
public:
	SimpleClient();
	virtual ~SimpleClient();

	bool openConnection(string addr, int port);
	void mainLoop();
	void closeConnection();

	virtual void writeData(const char *data, int count);
	virtual void onDataRead(char *data, int count) {cout << data << endl;}

private:
	int sock;
	struct sockaddr_in server;
	string server_addr;
	int server_port;
	fd_set rset, wset;
	struct timeval tval;
	bool connected;

	ssize_t rxcount;
	char rxbuf[SIMPLECLIENT_RX_BUFF_SIZE];

};

//-----------------------------------------------
#endif /* SIMPLECLIENT_H_ */
