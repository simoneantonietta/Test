/*
 *-----------------------------------------------------------------------------
 * PROJECT: rasp_saetcom
 * PURPOSE: see module SimpleClient.h file
 *-----------------------------------------------------------------------------
 */

#include "SimpleClient.h"
#include <netdb.h>
#include <sys/fcntl.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

/**
 * ctor
 */
SimpleClient::SimpleClient()
{
connected=false;
}

/**
 * dtor
 */
SimpleClient::~SimpleClient()
{
}

/**
 * open a connection on the server specified
 * @param addr
 * @param port
 * @return
 */
bool SimpleClient::openConnection(string addr, int port)
{
server_addr=addr;
server_port=port;

int flags, n, error;
socklen_t len;
struct hostent *h;
struct sockaddr_in temp;

temp.sin_family = AF_INET;
temp.sin_port = htons(server_port);
h = gethostbyname(server_addr.c_str());
if (h == 0)
	{
	perror("Gethostbyname failed");
	return false;
	}
bcopy(h->h_addr, &temp.sin_addr, h->h_length);
sock = socket(AF_INET, SOCK_STREAM, 0);

flags = fcntl(sock, F_GETFL, 0);
#ifdef SIMPLECLIENT_RX_BLOCKING
// blocking
fcntl(sock, F_SETFL, flags);
#else
// non blocking
fcntl(sock, F_SETFL, flags | O_NONBLOCK);
#endif

error = 0;
if((n = connect(sock, (struct sockaddr*) &temp, sizeof(temp))) < 0)
	{
	if(errno != EINPROGRESS)
		{
		perror("connect");
		return false;
		}
	}

/* Do whatever we want while the connect is taking place. */

if (n == 0)
	goto done;	/* connect completed immediately */

FD_ZERO(&rset);
FD_SET(sock, &rset);
wset = rset;
tval.tv_sec = 2;
tval.tv_usec = 0;

if ((n = select(sock + 1, &rset, &wset, NULL, &tval)) == 0)
	{
	close (sock); /* timeout */
	errno = ETIMEDOUT;
	return false;
	}

if (FD_ISSET(sock, &rset) || FD_ISSET(sock, &wset))
	{
	len = sizeof(error);
	if (getsockopt(sock, SOL_SOCKET, SO_ERROR, &error, &len) < 0) return false; /* Solaris pending error */
	}
else
	{
	perror("select");
	}

done: fcntl(sock, F_SETFL, flags); /* restore file status flags */

if (error)
	{
	close (sock); /* just in case */
	errno = error;
	return false;
	}

// update values to handle incoming messages
tval.tv_sec = 1;
tval.tv_usec = 0;
connected=true;
return true;
}

/**
 * close the connection
 */
void SimpleClient::closeConnection()
{
close(sock);
}

/**
 * main loop (must run in loop like while(1) or thread)
 */
void SimpleClient::mainLoop()
{
#ifdef SIMPLECLIENT_RX_BLOCKING
// blocking
rxcount=recv(sock,rxbuf,SIMPLECLIENT_RX_BUFF_SIZE,MSG_WAITFORONE);

#else
int n;
// non blocking
FD_ZERO(&rset);
FD_SET(sock, &rset);

// timeout for the select
tval.tv_sec=1;
tval.tv_usec=0;

if ((n = select(sock + 1, &rset, &wset, NULL, &tval)) == 0)
	{
	// NOTE: this timeout does not means necessarily an error

	//resetRxFrame();
	//result.setError();
	//result.message="select timeout";
	errno = ETIMEDOUT;
	rxcount=0;
	}

rxcount=recv(sock,rxbuf,SIMPLECLIENT_RX_BUFF_SIZE,MSG_DONTWAIT);
if(rxcount<0)
	{
	rxcount=0;
	}
if(rxcount>0)
	{
	onDataRead(rxbuf,rxcount);
	}
#endif
}

/**
 * send data to the socket
 * @param data
 * @param count
 */
void SimpleClient::writeData(const char* data, int count)
{
if(connected)
	{
	if(send(sock, data, count, 0) < 0)
		{
		puts("Send failed");
		}
	}
}
