/*
 *-----------------------------------------------------------------------------
 * PROJECT: woodbusd
 * PURPOSE: see module SocketServer.h file
 *-----------------------------------------------------------------------------
 */

#include "global.h"
#include "SocketServer.h"
#include "GlobalData.h"
#include "utils/Utils.h"

extern Trace *dbg;

/**
 * ctor
 */
SocketServer::SocketServer()
{
textMode = false;
clearNLine = false;
opt = 1;
//initialise all client_socket[] to 0 so not checked
for (int i = 0; i < SKSRV_MAX_CLIENTS; i++)
	{
	clData[i].clientSocket = 0;
	clData[i].nDataAvailable = 0;
	}
}

/**
 * dtor
 */
SocketServer::~SocketServer()
{
}

/**
 * open socket and create the server
 * @param port
 * @param to timeout in [us]
 * @param txt true: textual data protocol; false: binary data protocol
 * @param clearNLine if txt=true and this param is true eliminates the \n ant the end of the string received or transmitted
 * @return
 */
bool SocketServer::createServer(int port, int to, bool txt, bool clearNLine)
{
this->port = port;
this->to = to;
this->timeout.tv_sec = to / 1000000;
this->timeout.tv_usec = to - (timeout.tv_sec * 1000000);

this->textMode = txt;
this->clearNLine = clearNLine;

//create a master socket
if ((masterSocket = socket(AF_INET, SOCK_STREAM, 0)) == 0)
	{
	//perror("socket failed");
	dbg->trace(DBG_SKTSERVER, "ERROR: socket failed");
	return false;
	}

//set master socket to allow multiple connections , this is just a good habit, it will work without this
if (setsockopt(masterSocket, SOL_SOCKET, SO_REUSEADDR, (char *) &opt, sizeof(opt))
    < 0)
	{
	dbg->trace(DBG_SKTSERVER, "ERROR: setting socket options");
	return false;
	}

//type of socket created
address.sin_family = AF_INET;
address.sin_addr.s_addr = INADDR_ANY;
address.sin_port = htons(this->port);

//bind the socket to localhost port
if (bind(masterSocket, (struct sockaddr *) &address, sizeof(address)) < 0)
	{
	dbg->trace(DBG_SKTSERVER, "ERROR: bind failed");
	return false;
	}
dbg->trace(DBG_SKTSERVER, "Listener on port %d", this->port);

//try to specify maximum of 3 pending connections for the master socket
if (listen(masterSocket, SKSRV_MAX_PENDING_CONN) < 0)
	{
	dbg->trace(DBG_SKTSERVER, "ERROR on listener");
	return false;
	}

//accept the incoming connection
this->addrlen = sizeof(address);
dbg->trace(DBG_SKTSERVER, "Waiting for connections...");
return true;
}

/**
 * main loop of the server handling
 * Should run in a while(1) {...} loop
 * @return true: ok
 */
bool SocketServer::loop()
{
int sd, max_sd, activity, newSocket, valread;

//clear the socket set
FD_ZERO(&readfds);

//add master socket to set
FD_SET(masterSocket, &readfds);
max_sd = masterSocket;

//add child sockets to set
for (int i = 0; i < SKSRV_MAX_CLIENTS; i++)
	{
	//socket descriptor
	sd = clData[i].clientSocket;

	//if valid socket descriptor then add to read list
	if (sd > 0)
	FD_SET(sd, &readfds);

	//highest file descriptor number, need it for the select function
	if (sd > max_sd) max_sd = sd;
	}

//wait for an activity on one of the sockets , if timeout is NULL , so wait indefinitely
if (to == 0)
	{
	activity = select(max_sd + 1, &readfds, NULL, NULL, NULL);
	}
else
	{
	activity = select(max_sd + 1, &readfds, NULL, NULL, &timeout);
	}

if ((activity < 0) && (errno != EINTR))
	{
	dbg->trace(DBG_SKTSERVER, "select error");
	}

//If something happened on the master socket , then its an incoming connection
if (FD_ISSET(masterSocket, &readfds))
	{
	if ((newSocket = accept(masterSocket, (struct sockaddr *) &address, (socklen_t*) &addrlen))
	    < 0)
		{
		dbg->trace(DBG_SKTSERVER, "ERROR: accept");
		return false;
		}

	//inform user of socket number - used in send and receive commands
	//dbg->trace(DBG_SKTSERVER,"New connection, socket fd is %d, ip is : %s, port : %d" , newSocket , inet_ntoa(address.sin_addr) , ntohs(address.sin_port));
	dbg->trace(DBG_SKTSERVER, "New connection, socket fd is %d, ip is : %s, port : %d", newSocket, inet_ntoa(address.sin_addr), port);
	//add new socket to array of sockets
	bool found = false;
	for (int i = 0; i < SKSRV_MAX_CLIENTS; i++)
		{
		//if position is empty
		if (clData[i].clientSocket == 0)
			{
			found = true;
			clData[i].clientSocket = newSocket;
			dbg->trace(DBG_SKTSERVER, "Adding to list of sockets as client ID  %d", i);
			onNewConnection(i);
			break;
			}
		}
	if (!found) dbg->trace(DBG_SKTSERVER, "ERROR: too many client attached");
	}
else  // else its some IO operation on some other socket
	{
	for (int i = 0; i < SKSRV_MAX_CLIENTS; i++)
		{
		sd = clData[i].clientSocket;

		if (FD_ISSET(sd, &readfds))
			{
			//Check if it was for closing , and also read the incoming message
			if ((valread = read(sd, clData[i].buffer, SKSRV_BUFFER_SIZE)) == 0)
				{
				//Somebody disconnected , get his details and print
				getpeername(sd, (struct sockaddr*) &address, (socklen_t*) &addrlen);
				dbg->trace(DBG_SKTSERVER, "Host disconnected, client ID %d, ip %s , port %d", i, inet_ntoa(address.sin_addr), port);
				onCloseConnection(i);
				//Close the socket and mark as 0 in list for reuse
				close(sd);
				clData[i].clientSocket = 0;
				}
			else	// is some data!
				{
				//set the string terminating NULL byte on the end of the data read
				if (textMode)
					{
					clData[i].buffer[valread] = '\0';
					if (clearNLine)
						{
						for (int j = strlen(clData[i].buffer); j > 0; j--)
							{
							if (clData[i].buffer[j] == '\n' || clData[i].buffer[j] == '\r')
								{
								clData[i].buffer[j] = '\0';
								valread--;
								}
							}
						}
					}
				clData[i].nDataAvailable = valread;
#ifdef DEBUG_SKT
				printRx(i, clData[i].buffer, valread);
#endif
				onDataReceived(i);
				//sendData(i,valread,buffer[i]); // echo
				}
			}
		}
	}
return true;
}

/**
 * send data to client
 * @param client client descriptor
 * @param data data to send
 * @param size bytes to send
 * @return true: ok
 */
bool SocketServer::sendData(int client, char* data, int size)
{
//send new connection greeting message
int nbytes = send(clData[client].clientSocket, data, size, 0);
if (nbytes != size)
	{
	dbg->trace(DBG_SKTSERVER, "ERROR: not all data are sent (%d/%d)",nbytes,size);
	return false;
	}
#ifdef DEBUG_SKT
printTx(client, data, size);
#endif
return true;
}

/**
 * tell if a client has data
 * @param client
 * @return true: has data available
 */
bool SocketServer::hasRxData(int client)
{
return (clData[client].nDataAvailable > 0) ? (true) : (false);
}

/**
 * tell how many data is available
 * @param client
 * @return number of bytes available
 */
int SocketServer::nRxData(int client)
{
return clData[client].nDataAvailable;
}

/**
 * get the data available
 * @param client
 * @param size destination buffer (not the data available)
 * @param data buffer data readed
 * @return false: no data available; true: returned some data
 */
bool SocketServer::readData(int client, char* data, int size)
{
if (clData[client].nDataAvailable > 0)
	{
	if (size < clData[client].nDataAvailable)
	  dbg->trace(DBG_SKTSERVER, "WARN: destination data buffer smaller than the available data");
	memcpy(data, clData[client].buffer, size);
	clData[client].nDataAvailable = 0;
	return true;
	}
dbg->trace(DBG_SKTSERVER, "WARN: attempt to read with no data available");
return false;
}

//-----------------------------------------------------------------------------
// PRIVATE MEMBERS
//-----------------------------------------------------------------------------

/**
 * print in readable form the data received
 * @param client
 * @param buff
 * @param size
 */
void SocketServer::printRx(int client, char* buff, int size)
{
if (textMode)
	{
	dbg->trace(DBG_SKTSERVER_DATA, "RX: %s", buff);
	}
else
	{
	//char hex[SKSRV_BUFFER_SIZE * 2];
	//char prn[SKSRV_BUFFER_SIZE + 10];
	//Hex2AsciiHex(hex, (unsigned char*) buff, size, false);
	//printableBuffer(prn, (unsigned char*) buff, size);
	//dbg->trace(DBG_SKTSERVER_DATA,"RX: (%d) %s\n[%s]",size,hex,prn);
	//dbg->trace(DBG_SKTSERVER_DATA, "RX: (%d) %s", size, hex);
	//dbg->trace(DBG_SKTSERVER_DATA, "    (%d) [%s]", size, prn);
	string str="";
	hexdumpStr(str,buff,size);
	dbg->trace(DBG_SKTSERVER_DATA, "RX: client %d (%d bytes)\n--------------------------------------------------------------------------\n%s--------------------------------------------------------------------------",client,size,str.c_str());
	}
}

/**
 * print in readable form the data transmitted
 * @param client
 * @param buff
 * @param size
 */
void SocketServer::printTx(int client, char* buff, int size)
{
if (textMode)
	{
	dbg->trace(DBG_SKTSERVER_DATA, "TX: %s", buff);
	}
else
	{
	//char hex[SKSRV_BUFFER_SIZE * 2];
	//char prn[SKSRV_BUFFER_SIZE + 10];
	//Hex2AsciiHex(hex, (unsigned char*) buff, size, false);
	//printableBuffer(prn, (unsigned char*) buff, size);
	//dbg->trace(DBG_SKTSERVER_DATA,"TX: (%d) %s\n[%s]",size,hex,prn);
	//dbg->trace(DBG_SKTSERVER_DATA, "TX: (%d) %s", size, hex);
	//dbg->trace(DBG_SKTSERVER_DATA, "    (%d) [%s]", size, prn);
	string str="";
	hexdumpStr(str,buff,size);
	dbg->trace(DBG_SKTSERVER_DATA, "TX: client %d (%d bytes)\n--------------------------------------------------------------------------\n%s--------------------------------------------------------------------------",client,size,str.c_str());
	}
}


#if 0

/**
 Handle multiple socket connections with select and fd_set on Linux

 Silver Moon ( m00n.silv3r@gmail.com)
 */

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

#define TRUE   1
#define FALSE  0
#define PORT 8888

int main(int argc , char *argv[])
	{
	int opt = TRUE;
	int master_socket , addrlen , new_socket , client_socket[30] , max_clients = 30 , activity, i , valread , sd;
	int max_sd;
	struct sockaddr_in address;

	char buffer[1025];  //data buffer of 1K

	//set of socket descriptors
	fd_set readfds;

	//a message
	char *message = "ECHO Daemon v1.0 \r\n";

	//initialise all client_socket[] to 0 so not checked
	for (i = 0; i < max_clients; i++)
		{
		client_socket[i] = 0;
		}

	//create a master socket
	if( (master_socket = socket(AF_INET , SOCK_STREAM , 0)) == 0)
		{
		perror("socket failed");
		exit(EXIT_FAILURE);
		}

	//set master socket to allow multiple connections , this is just a good habit, it will work without this
	if( setsockopt(master_socket, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)) < 0 )
		{
		perror("setsockopt");
		exit(EXIT_FAILURE);
		}

	//type of socket created
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons( PORT );

	//bind the socket to localhost port 8888
	if (bind(master_socket, (struct sockaddr *)&address, sizeof(address))<0)
		{
		perror("bind failed");
		exit(EXIT_FAILURE);
		}
	printf("Listener on port %d \n", PORT);

	//try to specify maximum of 3 pending connections for the master socket
	if (listen(master_socket, 3) < 0)
		{
		perror("listen");
		exit(EXIT_FAILURE);
		}

	//accept the incoming connection
	addrlen = sizeof(address);
	puts("Waiting for connections ...");

	while(TRUE)
		{
		//clear the socket set
		FD_ZERO(&readfds);

		//add master socket to set
		FD_SET(master_socket, &readfds);
		max_sd = master_socket;

		//add child sockets to set
		for ( i = 0; i < max_clients; i++)
			{
			//socket descriptor
			sd = client_socket[i];

			//if valid socket descriptor then add to read list
			if(sd > 0)
			FD_SET( sd , &readfds);

			//highest file descriptor number, need it for the select function
			if(sd > max_sd)
			max_sd = sd;
			}

		//wait for an activity on one of the sockets , timeout is NULL , so wait indefinitely
		activity = select( max_sd + 1 , &readfds , NULL , NULL , NULL);

		if ((activity < 0) && (errno!=EINTR))
			{
			printf("select error");
			}

		//If something happened on the master socket , then its an incoming connection
		if (FD_ISSET(master_socket, &readfds))
			{
			if ((new_socket = accept(master_socket, (struct sockaddr *)&address, (socklen_t*)&addrlen))<0)
				{
				perror("accept");
				exit(EXIT_FAILURE);
				}

			//inform user of socket number - used in send and receive commands
			printf("New connection , socket fd is %d , ip is : %s , port : %d \n" , new_socket , inet_ntoa(address.sin_addr) , ntohs(address.sin_port));

			//send new connection greeting message
			if( send(new_socket, message, strlen(message), 0) != strlen(message) )
				{
				perror("send");
				}

			puts("Welcome message sent successfully");

			//add new socket to array of sockets
			for (i = 0; i < max_clients; i++)
				{
				//if position is empty
				if( client_socket[i] == 0 )
					{
					client_socket[i] = new_socket;
					printf("Adding to list of sockets as %d\n" , i);

					break;
					}
				}
			}

		//else its some IO operation on some other socket :)
		for (i = 0; i < max_clients; i++)
			{
			sd = client_socket[i];

			if (FD_ISSET( sd , &readfds))
				{
				//Check if it was for closing , and also read the incoming message
				if ((valread = read( sd , buffer, 1024)) == 0)
					{
					//Somebody disconnected , get his details and print
					getpeername(sd , (struct sockaddr*)&address , (socklen_t*)&addrlen);
					printf("Host disconnected , ip %s , port %d \n" , inet_ntoa(address.sin_addr) , ntohs(address.sin_port));

					//Close the socket and mark as 0 in list for reuse
					close( sd );
					client_socket[i] = 0;
					}

				//Echo back the message that came in
				else
					{
					//set the string terminating NULL byte on the end of the data read
					buffer[valread] = '\0';
					send(sd , buffer , strlen(buffer) , 0 );
					}
				}
			}
		}

	return 0;
	}
#endif

#if 0
#include "sockhelp.h"
#include <ctype.h>
#include <sys/time.h>
#include <fcntl.h>

int sock; /* The socket file descriptor for our "listening"
 socket */
int connectlist[5]; /* Array of connected sockets so we know who
 we are talking to */
fd_set socks; /* Socket file descriptors we want to wake
 up for, using select() */
int highsock; /* Highest #'d file descriptor, needed for select() */

void setnonblocking(sock)
int sock;
	{
	int opts;

	opts = fcntl(sock,F_GETFL);
	if (opts < 0)
		{
		perror("fcntl(F_GETFL)");
		exit(EXIT_FAILURE);
		}
	opts = (opts | O_NONBLOCK);
	if (fcntl(sock,F_SETFL,opts) < 0)
		{
		perror("fcntl(F_SETFL)");
		exit(EXIT_FAILURE);
		}
	return;
	}

void build_select_list()
	{
	int listnum; /* Current item in connectlist for for loops */

	/* First put together fd_set for select(), which will
	 consist of the sock veriable in case a new connection
	 is coming in, plus all the sockets we have already
	 accepted. */

	/* FD_ZERO() clears out the fd_set called socks, so that
	 it doesn't contain any file descriptors. */

	FD_ZERO(&socks);

	/* FD_SET() adds the file descriptor "sock" to the fd_set,
	 so that select() will return if a connection comes in
	 on that socket (which means you have to do accept(), etc. */

	FD_SET(sock,&socks);

	/* Loops through all the possible connections and adds
	 those sockets to the fd_set */

	for (listnum = 0; listnum < 5; listnum++)
		{
		if (connectlist[listnum] != 0)
			{
			FD_SET(connectlist[listnum],&socks);
			if (connectlist[listnum] > highsock)
			highsock = connectlist[listnum];
			}
		}
	}

void handle_new_connection()
	{
	int listnum; /* Current item in connectlist for for loops */
	int connection; /* Socket file descriptor for incoming connections */

	/* We have a new connection coming in!  We'll
	 try to find a spot for it in connectlist. */
	connection = accept(sock, NULL, NULL);
	if (connection < 0)
		{
		perror("accept");
		exit(EXIT_FAILURE);
		}
	setnonblocking(connection);
	for (listnum = 0; (listnum < 5) && (connection != -1); listnum ++)
	if (connectlist[listnum] == 0)
		{
		printf("\nConnection accepted:   FD=%d; Slot=%d\n",
				connection,listnum);
		connectlist[listnum] = connection;
		connection = -1;
		}
	if (connection != -1)
		{
		/* No room left in the queue! */
		printf("\nNo room left for new client.\n");
		sock_puts(connection,"Sorry, this server is too busy.  "
				Try again later!\r\n");
				close(connection);
				}
			}

		void deal_with_data(
				int listnum /* Current item in connectlist for for loops */
		)
			{
			char buffer[80]; /* Buffer for socket reads */
			char *cur_char; /* Used in processing buffer */

			if (sock_gets(connectlist[listnum],buffer,80) < 0)
				{
				/* Connection closed, close this end
				 and free up entry in connectlist */
				printf("\nConnection lost: FD=%d;  Slot=%d\n",
						connectlist[listnum],listnum);
				close(connectlist[listnum]);
				connectlist[listnum] = 0;
				}
			else
				{
				/* We got some data, so upper case it
				 and send it back. */
				printf("\nReceived: %s; ",buffer);
				cur_char = buffer;
				while (cur_char[0] != 0)
					{
					cur_char[0] = toupper(cur_char[0]);
					cur_char++;
					}
				sock_puts(connectlist[listnum],buffer);
				sock_puts(connectlist[listnum],"\n");
				printf("responded: %s\n",buffer);
				}
			}

		void read_socks()
			{
			int listnum; /* Current item in connectlist for for loops */

			/* OK, now socks will be set with whatever socket(s)
			 are ready for reading.  Lets first check our
			 "listening" socket, and then check the sockets
			 in connectlist. */

			/* If a client is trying to connect() to our listening
			 socket, select() will consider that as the socket
			 being 'readable'. Thus, if the listening socket is
			 part of the fd_set, we need to accept a new connection. */

			if (FD_ISSET(sock,&socks))
			handle_new_connection();
			/* Now check connectlist for available data */

			/* Run through our sockets and check to see if anything
			 happened with them, if so 'service' them. */

			for (listnum = 0; listnum < 5; listnum++)
				{
				if (FD_ISSET(connectlist[listnum],&socks))
				deal_with_data(listnum);
				} /* for (all entries in queue) */
			}

		int main (argc, argv)
		int argc;
		char *argv[];
			{
			char *ascport; /* ASCII version of the server port */
			int port; /* The port number after conversion from ascport */
			struct sockaddr_in server_address; /* bind info structure */
			int reuse_addr = 1; /* Used so we can re-bind to our port
			 while a previous connection is still
			 in TIME_WAIT state. */
			struct timeval timeout; /* Timeout for select */
			int readsocks; /* Number of sockets ready for reading */

			/* Make sure we got a port number as a parameter */
			if (argc < 2)
				{
				printf("Usage: %s PORT\r\n",argv[0]);
				exit(EXIT_FAILURE);
				}

			/* Obtain a file descriptor for our "listening" socket */
			sock = socket(AF_INET, SOCK_STREAM, 0);
			if (sock < 0)
				{
				perror("socket");
				exit(EXIT_FAILURE);
				}
			/* So that we can re-bind to it without TIME_WAIT problems */
			setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &reuse_addr,
					sizeof(reuse_addr));

			/* Set socket to non-blocking with our setnonblocking routine */
			setnonblocking(sock);

			/* Get the address information, and bind it to the socket */
			ascport = argv[1]; /* Read what the user gave us */
			port = atoport(ascport); /* Use function from sockhelp to
			 convert to an int */
			memset((char *) &server_address, 0, sizeof(server_address));
			server_address.sin_family = AF_INET;
			server_address.sin_addr.s_addr = htonl(INADDR_ANY);
			server_address.sin_port = port;
			if (bind(sock, (struct sockaddr *) &server_address,
							sizeof(server_address)) < 0 )
				{
				perror("bind");
				close(sock);
				exit(EXIT_FAILURE);
				}

			/* Set up queue for incoming connections. */
			listen(sock,5);

			/* Since we start with only one socket, the listening socket,
			 it is the highest socket so far. */
			highsock = sock;
			memset((char *) &connectlist, 0, sizeof(connectlist));

			while (1)
				{ /* Main server loop - forever */
				build_select_list();
				timeout.tv_sec = 1;
				timeout.tv_usec = 0;

				/* The first argument to select is the highest file
				 descriptor value plus 1. In most cases, you can
				 just pass FD_SETSIZE and you'll be fine. */

				/* The second argument to select() is the address of
				 the fd_set that contains sockets we're waiting
				 to be readable (including the listening socket). */

				/* The third parameter is an fd_set that you want to
				 know if you can write on -- this example doesn't
				 use it, so it passes 0, or NULL. The fourth parameter
				 is sockets you're waiting for out-of-band data for,
				 which usually, you're not. */

				/* The last parameter to select() is a time-out of how
				 long select() should block. If you want to wait forever
				 until something happens on a socket, you'll probably
				 want to pass NULL. */

				readsocks = select(highsock+1, &socks, (fd_set *) 0,
						(fd_set *) 0, &timeout);

				/* select() returns the number of sockets that had
				 things going on with them -- i.e. they're readable. */

				/* Once select() returns, the original fd_set has been
				 modified so it now reflects the state of why select()
				 woke up. i.e. If file descriptor 4 was originally in
				 the fd_set, and then it became readable, the fd_set
				 contains file descriptor 4 in it. */

				if (readsocks < 0)
					{
					perror("select");
					exit(EXIT_FAILURE);
					}
				if (readsocks == 0)
					{
					/* Nothing ready to read, just show that
					 we're alive */
					printf(".");
					fflush(stdout);
					}
				else
				read_socks();
				} /* while(1) */
			} /* main */

#endif
