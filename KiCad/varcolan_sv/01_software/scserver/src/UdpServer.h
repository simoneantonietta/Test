/**----------------------------------------------------------------------------
 * PROJECT: scserver
 * PURPOSE:
 * 
 *-----------------------------------------------------------------------------  
 * CREATION: Dec 12, 2016
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

#ifndef UDPSERVER_H_
#define UDPSERVER_H_

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdexcept>

class UdpServer
{
public:
	UdpServer();
	UdpServer(const std::string& addr, int port);
	~UdpServer();

	int get_socket() const;
	int get_port() const;
	std::string get_addr() const;

	std::string get_lastClientAddr();
	int get_lastClientPort();

	bool openConnection(const std::string& addr, int port);
	int datarecv(char *msg, size_t max_size);
	int timed_datarecv(char *msg, size_t max_size, int max_wait_ms);
	int datasend(const char *msg, size_t size);

private:
	int f_socket;
	int f_port;
	std::string f_addr;
	struct addrinfo * f_addrinfo;

	struct sockaddr_in * lastclientaddr;
	unsigned int lastclientlen;
};

//-----------------------------------------------
#endif /* UDPSERVER_H_ */
