/**----------------------------------------------------------------------------
 * PROJECT: scserver
 * PURPOSE:
 * WARNING: never used for now...
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

#ifndef UDPCLIENT_H_
#define UDPCLIENT_H_

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdexcept>

/*
 *
 */
class UdpClient
{
public:
	UdpClient(const std::string& addr, int port);
	~UdpClient();

	int get_socket() const;
	int get_port() const;
	std::string get_addr() const;

	int datarecv(char *msg, size_t max_size);
	int timed_datarecv(char *msg, size_t max_size, int max_wait_ms);
	int datasend(const char *msg, size_t size);

private:
	int f_socket;
	int f_port;
	std::string f_addr;
	struct addrinfo * f_addrinfo;
};

//-----------------------------------------------
#endif /* UDPCLIENT_H_ */
