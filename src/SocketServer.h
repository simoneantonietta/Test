/**----------------------------------------------------------------------------
 * PROJECT: woodbusd
 * PURPOSE: generic class to handle socker server in non blocking mode
 * 
 *-----------------------------------------------------------------------------  
 * CREATION: 7 Oct 2013
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

#ifndef SOCKETSERVER_H_
#define SOCKETSERVER_H_

#include <stdio.h>
#include <string.h>   //strlen
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>   //close
#include <arpa/inet.h>    //close
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/time.h> //FD_SET, FD_ISSET, FD_ZERO macros
//#include "GlobalData.h"
#include "comm/prot6/hprot.h" // need only for size defines

#define DEBUG_SKT


#define SKSRV_MAX_CLIENTS						3
#define SKSRV_BUFFER_SIZE						HPROT_BUFFER_SIZE
#define SKSRV_MAX_PENDING_CONN			2
#if SKSRV_MAX_CLIENTS>99
#error SKSRV_MAX_CLIENTS too large, must be less then 100
#endif

class SocketServer
{
public:
	SocketServer();
	virtual ~SocketServer();

	bool createServer(int port, int to, bool txt, bool clearNLine=true);
	bool loop();

	bool readData(int client,char* data,int size);
	bool sendData(int client,char* data,int size);
	bool hasRxData(int client);
	int nRxData(int client);
	void clearNewLine();

	// SOCKET EVENTS
	virtual void onNewConnection(int client) {};
	virtual void onCloseConnection(int client) {};
	virtual void onDataReceived(int client) {};

private:
	struct ClientData
	{
	int clientSocket;
	char buffer[SKSRV_BUFFER_SIZE];		// data buffer
	int nDataAvailable;
	} clData[SKSRV_MAX_CLIENTS];


	bool textMode;
	bool clearNLine;
	int opt;
	int port;
	int to;
	timeval timeout;
	int masterSocket;
  //int clientSocket[SKSRV_MAX_CLIENTS];
  //char buffer[SKSRV_MAX_CLIENTS][SKSRV_BUFFER_SIZE];		// data buffer
  fd_set readfds;		// socket descriptor
  struct sockaddr_in address;
  int addrlen;
  //int nDataAvailable[SKSRV_MAX_CLIENTS];

  void printRx(int client, char *buff,int size);
  void printTx(int client, char *buff,int size);
};

//-----------------------------------------------
#endif /* SOCKETSERVER_H_ */
