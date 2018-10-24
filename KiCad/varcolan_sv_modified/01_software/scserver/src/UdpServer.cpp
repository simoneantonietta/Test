/*
 *-----------------------------------------------------------------------------
 * PROJECT: scserver
 * PURPOSE: see module UdpServer.h file
 *-----------------------------------------------------------------------------
 */

#include "UdpServer.h"
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include "Global.h"

/**
 * ctor
 */
UdpServer::UdpServer()
{

}

/**
 * ctor
 */
UdpServer::UdpServer(const std::string& addr, int port)
{
openConnection(addr,port);
}


/** \brief Clean up the UDP server.
 *
 * This function frees the address info structures and close the socket.
 */
UdpServer::~UdpServer()
{
freeaddrinfo(f_addrinfo);
close(f_socket);
}

/** \brief The socket used by this UDP server.
 *
 * This function returns the socket identifier. It can be useful if you are
 * doing a select() on many sockets.
 *
 * \return The socket of this UDP server.
 */
int UdpServer::get_socket() const
{
return f_socket;
}

/** \brief The port used by this UDP server.
 *
 * This function returns the port attached to the UDP server. It is a copy
 * of the port specified in the constructor.
 *
 * \return The port of the UDP server.
 */
int UdpServer::get_port() const
{
return f_port;
}

/** \brief Return the address of this UDP server.
 *
 * This function returns a verbatim copy of the address as passed to the
 * constructor of the UDP server (i.e. it does not return the canonalized
 * version of the address.)
 *
 * \return The address as passed to the constructor.
 */
std::string UdpServer::get_addr() const
{
return f_addr;
}

/**
 * get the address of the last client that sent a message
 * @return ip address
 */
std::string UdpServer::get_lastClientAddr()
{
//	hostp = gethostbyaddr((const char *) &clientaddr.sin_addr.s_addr, sizeof(clientaddr.sin_addr.s_addr), AF_INET);
//	if(hostp == NULL)
//		{
//		dbg->trace(DBG_ERROR,"on gethostbyaddr");
//		}
char *cliaddrp = inet_ntoa(lastclientaddr->sin_addr);
return (std::string) cliaddrp;
}

/**
 * get the port of the last client that sent a message
 * @return port
 */
int UdpServer::get_lastClientPort()
{
int cliportp = ntohs(lastclientaddr->sin_port);
return cliportp;
}

/**
 * open the connection
 * This function initializes a UDP server object making it ready to
 * receive messages.
 *
 * The server address and port are specified in the constructor so
 * if you need to receive messages from several different addresses
 * and/or port, you'll have to create a server for each.
 *
 * The address is a string and it can represent an IPv4 or IPv6
 * address.
 *
 * Note that this function calls connect() to connect the socket
 * to the specified address. To accept data on different UDP addresses
 * and ports, multiple UDP servers must be created.
 *
 * \note
 * The socket is open in this process. If you fork() or exec() then the
 * socket will be closed by the operating system.
 *
 * \warning
 * We only make use of the first address found by getaddrinfo(). All
 * the other addresses are ignored.
 *
 * \exception udp_client_server_runtime_error
 * The udp_client_server_runtime_error exception is raised when the address
 * and port combinaison cannot be resolved or if the socket cannot be
 * opened.
 *
 * \param[in] addr  The address we receive on.
 * \param[in] port  The port we receive from.
 * @return
 */
bool UdpServer::openConnection(const std::string& addr, int port)
{
char decimal_port[16];
f_port=port;
f_addr=addr;

snprintf(decimal_port, sizeof(decimal_port), "%d", f_port);
decimal_port[sizeof(decimal_port) / sizeof(decimal_port[0]) - 1] = '\0';
struct addrinfo hints;
memset(&hints, 0, sizeof(hints));
hints.ai_family = AF_UNSPEC;
hints.ai_socktype = SOCK_DGRAM;
hints.ai_protocol = IPPROTO_UDP;
int r(getaddrinfo(addr.c_str(), decimal_port, &hints, &f_addrinfo));
if(r != 0 || f_addrinfo == NULL)
	{
	TRACE(dbg,DBG_ERROR,"invalid address or port %s %d",addr.c_str(),port);
	}
f_socket = socket(f_addrinfo->ai_family, SOCK_DGRAM | SOCK_CLOEXEC, IPPROTO_UDP);
if(f_socket == -1)
	{
	freeaddrinfo(f_addrinfo);
	TRACE(dbg,DBG_ERROR,"could not create socket for %s %d",addr.c_str(),port);
	return false;
	}
r = bind(f_socket, f_addrinfo->ai_addr, f_addrinfo->ai_addrlen);
if(r != 0)
	{
	freeaddrinfo(f_addrinfo);
	close(f_socket);
	TRACE(dbg,DBG_ERROR,"could not bind socket for %s %d",addr.c_str(),port);
	return false;
	}
return true;
}

/** \brief Wait on a message.
 *
 * This function waits until a message is received on this UDP server.
 * There are no means to return from this function except by receiving
 * a message. Remember that UDP does not have a connect state so whether
 * another process quits does not change the status of this UDP server
 * and thus it continues to wait forever.
 *
 * Note that you may change the type of socket by making it non-blocking
 * (use the get_socket() to retrieve the socket identifier) in which
 * case this function will not block if no message is available. Instead
 * it returns immediately.
 *
 * \param[in] msg  The buffer where the message is saved.
 * \param[in] max_size  The maximum size the message (i.e. size of the \p msg buffer.)
 *
 * \return The number of bytes read or -1 if an error occurs.
 */
int UdpServer::datarecv(char *msg, size_t max_size)
{
//return ::recv(f_socket, msg, max_size, 0);
return ::recvfrom(f_socket, msg, max_size, 0, (struct sockaddr *) &lastclientaddr, (unsigned int *)&lastclientlen);
}

/** \brief Wait for data to come in.
 *
 * This function waits for a given amount of time for data to come in. If
 * no data comes in after max_wait_ms, the function returns with -1 and
 * errno set to EAGAIN.
 *
 * The socket is expected to be a blocking socket (the default,) although
 * it is possible to setup the socket as non-blocking if necessary for
 * some other reason.
 *
 * This function blocks for a maximum amount of time as defined by
 * max_wait_ms. It may return sooner with an error or a message.
 *
 * \param[in] msg  The buffer where the message will be saved.
 * \param[in] max_size  The size of the \p msg buffer in bytes.
 * \param[in] max_wait_ms  The maximum number of milliseconds to wait for a message.
 *
 * \return -1 if an error occurs or the function timed out, the number of bytes received otherwise.
 */
int UdpServer::timed_datarecv(char *msg, size_t max_size, int max_wait_ms)
{
fd_set s;
FD_ZERO(&s);
FD_SET(f_socket, &s);
struct timeval timeout;
timeout.tv_sec = max_wait_ms / 1000;
timeout.tv_usec = (max_wait_ms % 1000) * 1000;
int retval = select(f_socket + 1, &s, &s, &s, &timeout);
if(retval == -1)
	{
	// select() set errno accordingly
	return -1;
	}
if(retval > 0)
	{
	// our socket has data
	return ::recv(f_socket, msg, max_size, 0);
	}

// our socket has no data
errno = EAGAIN;
return -1;
}

/** \brief Send a message through this UDP client.
 *
 * This function sends \p msg through the UDP client socket. The function
 * cannot be used to change the destination as it was defined when creating
 * the UdpClient object.
 *
 * The size must be small enough for the message to fit. In most cases we
 * use these in Snap! to send very small signals (i.e. 4 bytes commands.)
 * Any data we would want to share remains in the Cassandra database so
 * that way we can avoid losing it because of a UDP message.
 *
 * \param[in] msg  The message to send.
 * \param[in] size  The number of bytes representing this message.
 *
 * \return -1 if an error occurs, otherwise the number of bytes sent. errno
 * is set accordingly on error.
 */
int UdpServer::datasend(const char *msg, size_t size)
{
return sendto(f_socket, msg, size, 0, f_addrinfo->ai_addr, f_addrinfo->ai_addrlen);
}

