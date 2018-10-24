/*
 *-----------------------------------------------------------------------------
 * PROJECT: scserver
 * PURPOSE: see module UdpClient.h file
 *-----------------------------------------------------------------------------
 */

#include "UdpClient.h"
#include <errno.h>
#include "Global.h"

/** \brief Initialize a UDP client object.
 *
 * This function initializes the UDP client object using the address and the
 * port as specified.
 *
 * The port is expected to be a host side port number (i.e. 59200).
 *
 * The \p addr parameter is a textual address. It may be an IPv4 or IPv6
 * address and it can represent a host name or an address defined with
 * just numbers. If the address cannot be resolved then an error occurs
 * and constructor throws.
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
 * The server could not be initialized properly. Either the address cannot be
 * resolved, the port is incompatible or not available, or the socket could
 * not be created.
 *
 * \param[in] addr  The address to convert to a numeric IP.
 * \param[in] port  The port number.
 */
UdpClient::UdpClient(const std::string& addr, int port) :
		f_port(port), f_addr(addr)
{
char decimal_port[16];
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
	}
}

/** \brief Clean up the UDP client object.
 *
 * This function frees the address information structure and close the socket
 * before returning.
 */
UdpClient::~UdpClient()
{
freeaddrinfo(f_addrinfo);
close(f_socket);
}

/** \brief Retrieve a copy of the socket identifier.
 *
 * This function return the socket identifier as returned by the socket()
 * function. This can be used to change some flags.
 *
 * \return The socket used by this UDP client.
 */
int UdpClient::get_socket() const
{
return f_socket;
}

/** \brief Retrieve the port used by this UDP client.
 *
 * This function returns the port used by this UDP client. The port is
 * defined as an integer, host side.
 *
 * \return The port as expected in a host integer.
 */
int UdpClient::get_port() const
{
return f_port;
}

/** \brief Retrieve a copy of the address.
 *
 * This function returns a copy of the address as it was specified in the
 * constructor. This does not return a canonalized version of the address.
 *
 * The address cannot be modified. If you need to send data on a different
 * address, create a new UDP client.
 *
 * \return A string with a copy of the constructor input address.
 */
std::string UdpClient::get_addr() const
{
return f_addr;
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
int UdpClient::datarecv(char *msg, size_t max_size)
{
return ::recv(f_socket, msg, max_size, 0);
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
int UdpClient::timed_datarecv(char *msg, size_t max_size, int max_wait_ms)
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
int UdpClient::datasend(const char *msg, size_t size)
{
return sendto(f_socket, msg, size, 0, f_addrinfo->ai_addr, f_addrinfo->ai_addrlen);
}