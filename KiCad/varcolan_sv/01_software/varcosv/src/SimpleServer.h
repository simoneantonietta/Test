/**----------------------------------------------------------------------------
 * PROJECT:
 * PURPOSE:
 * Simple server tcp
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

#ifndef SIMPLESERVER_H_
#define SIMPLESERVER_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <errno.h>

#define SS_MAX_CLIENTS   	10
#define SS_RX_BUFF_SIZE		512

#define SS_CLIENT_UNKNOWN	-1

class SimpleServer
{
public:
	SimpleServer();
	virtual ~SimpleServer();

	int initServer(char* port, int latency_ms=10);
	void mainLoop();
	void closeClientConnection(int client);
	void closeServer();

	int getActualClient() {return actualClient;}
	bool clientIsConnected(int client) {return clients[client].connected;}
	void setClientIdentifier(int client,int id) {clients[client].identifier=id;}
	//-----------------------------------------------------------------------------
	// EVENTS
	//-----------------------------------------------------------------------------
	/**
	 * write data to socket
	 * rewrite freely it BUT you must call this!
	 * @param client
	 * @param data
	 * @param count
	 */
	virtual void writeData(int client, const char *data, int count)
	{
	write(getClientFD(client), data, count);
	}

	/**
	 * please rewrite it
	 * @param client
	 * @param descr
	 * @param host
	 * @param port
	 */
	virtual void onConnection(int client,int descr,char *host,char *port)
	{
	printf("Accepted connection on sk %d descriptor %d (host=%s, port=%s)\n", client, descr, host, port);
	}

	/**
	 * please rewrite for your use
	 * @param client
	 * @param data
	 * @param count
	 */
	virtual void onDataRead(int client,char *data, int count)
	{
		/* Write the buffer to standard output */
		s = write(1, data, count);
		if(s == -1)
			{
			perror("write");
			abort();
			}
	}

	/**
	 * please rewrite for your use
	 * @param client
	 */
	virtual void onCloseConn(int client)
	{
	printf("Closed connection on client %d\n", client);
	}

	/**
	 * return the authentication status (if used)
	 * @param client
	 * @return
	 */
	bool getAuthentication(int client) {return clients[client].authenticated;}

	/**
	 * set the authentication status (if used)
	 * @param client
	 * @return
	 */
	void setAuthentication(int client, bool st) {clients[client].authenticated=st;}

	/**
	 * reset of client info
	 * @param client
	 */
	void resetClient(int client)
	{
	if(client<0)
		{
		for(int i=0;i<SS_MAX_CLIENTS;i++)
			{
			clients[i].authenticated=false;
			clients[i].fd=-1;
			clients[i].connected=false;
			clients[i].identifier=SS_CLIENT_UNKNOWN;
			}
		}
	else
		{
		clients[client].authenticated=false;
		clients[client].fd=-1;
		clients[client].connected=false;
		clients[client].identifier=SS_CLIENT_UNKNOWN;
		}
	}

private:
	struct clientData_st
	{
	bool connected;
	int fd;
	bool authenticated;
	int identifier;		// identifier if any, else -1

	clientData_st()
		{
		fd=-1;
		authenticated=false;
		connected=false;
		identifier=SS_CLIENT_UNKNOWN;
		}
	} clients[SS_MAX_CLIENTS];
	int actualClient;	// last read from...

	int latency_ms;

	ssize_t rxcount;
	char rxbuf[SS_RX_BUFF_SIZE];

	int sfd, s;
	int efd;
	struct epoll_event event;
	struct epoll_event *events;

	int make_socket_non_blocking(int sfd);
	int create_and_bind(char *port);
	int findClient(int fd);
	int addClient(int fd);
	int getClientFD(int client);

};

//-----------------------------------------------
#endif /* SIMPLESERVER_H_ */
