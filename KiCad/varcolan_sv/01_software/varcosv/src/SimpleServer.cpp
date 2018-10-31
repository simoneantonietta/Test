/*
 *-----------------------------------------------------------------------------
 * PROJECT: varcosv
 * PURPOSE: see module SimpleServer.h file
 *-----------------------------------------------------------------------------
 */

#include "SimpleServer.h"

#define MAXEVENTS 64

/**
 * ctor
 */
SimpleServer::SimpleServer()
{
actualClient=-1;
}

/**
 * dtor
 */
SimpleServer::~SimpleServer()
{
}

/**
 * start the server
 * @param port
 * @param latency_ms (-1 indefinite - blocking)
 * @return
 */
int SimpleServer::initServer(char* port, int latency_ms)
{
this->latency_ms=latency_ms;

sfd = create_and_bind(port);
if(sfd == -1) abort();

s = make_socket_non_blocking(sfd);
if(s == -1) abort();

s = listen(sfd, SOMAXCONN);
if(s == -1)
	{
	perror("listen");
	abort();
	}

efd = epoll_create1(0);
if(efd == -1)
	{
	perror("epoll_create");
	abort();
	}

event.data.fd = sfd;
event.events = EPOLLIN | EPOLLET;
s = epoll_ctl(efd, EPOLL_CTL_ADD, sfd, &event);
if(s == -1)
	{
	perror("epoll_ctl");
	abort();
	}

/* Buffer where events are returned */
events = (struct epoll_event *)calloc(MAXEVENTS, sizeof event);
return EXIT_SUCCESS;
}

/**
 * close connection
 * @param client
 */
void SimpleServer::closeClientConnection(int client)
{
if(clients[client].connected)
	{
	//int fd=getClientFD(client);
	close(clients[client].fd);
	clients[client].connected=false;
	clients[client].fd=-1;
	}
}


/**
 * close and free resources
 */
void SimpleServer::closeServer()
{
free(events);
if(sfd >= 0)
	{
	int err = 1;
	socklen_t len = sizeof err;
	if(-1 == getsockopt(sfd, SOL_SOCKET, SO_ERROR, (char *) &err, &len))
		{
	  perror("error on close socket");
		}
	if(err)
		{
		errno = err;              // set errno to the socket SO_ERROR
		}
	//return err;

	if(shutdown(sfd, SHUT_RDWR) < 0)  // secondly, terminate the 'reliable' delivery
		{
	  if(errno != ENOTCONN && errno != EINVAL)  // SGI causes EINVAL
	  	{
	    perror("shutdown");
	  	}
		}
	if(close(sfd) < 0)  // finally call close()
		{
	  perror("close");
		}
	}
}

/**
 * main loop (must run in loop like while(1))
 */
void SimpleServer::mainLoop()
{
int n, i;

n = epoll_wait(efd, events, MAXEVENTS, 10);
for(i = 0;i < n;i++)
	{
	if((events[i].events & EPOLLERR) || (events[i].events & EPOLLHUP) || (!(events[i].events & EPOLLIN)))
		{
		/* An error has occured on this fd, or the socket is not
		 ready for reading (why were we notified then?) */
		fprintf(stderr, "epoll error\n");
		close(events[i].data.fd);
		continue;
		}

	else if(sfd == events[i].data.fd)
		{
		/* We have a notification on the listening socket, which
		 means one or more incoming connections. */
		while(1)
			{
			struct sockaddr in_addr;
			socklen_t in_len;
			int infd;
			char hbuf[NI_MAXHOST], sbuf[NI_MAXSERV];

			in_len = sizeof in_addr;
			infd = accept(sfd, &in_addr, &in_len);
			if(infd == -1)
				{
				if((errno == EAGAIN) || (errno == EWOULDBLOCK))
					{
					/* We have processed all incoming connections. */
					break;
					}
				else
					{
					perror("accept");
					break;
					}
				}

			s = getnameinfo(&in_addr, in_len, hbuf, sizeof hbuf, sbuf, sizeof sbuf,
			NI_NUMERICHOST | NI_NUMERICSERV);
			if(s == 0)
				{
				actualClient=addClient(infd);
				if(actualClient==-1)
					{
					perror("error telnet socket: too many clients?");
					return;
					}
				onConnection(actualClient,infd,hbuf,sbuf);
				//printf("Accepted connection on descriptor %d (host=%s, port=%s)\n", infd, hbuf, sbuf);
				}

			/* Make the incoming socket non-blocking and add it to the
			 list of fds to monitor. */
			s = make_socket_non_blocking(infd);
			if(s == -1) abort();

			event.data.fd = infd;
			event.events = EPOLLIN | EPOLLET;
			s = epoll_ctl(efd, EPOLL_CTL_ADD, infd, &event);
			if(s == -1)
				{
				perror("epoll_ctl");
				abort();
				}
			}
		continue;
		}
	else
		{
		/* We have data on the fd waiting to be read. Read and
		 display it. We must read whatever data is available
		 completely, as we are running in edge-triggered mode
		 and won't get a notification again for the same
		 data. */
		int done = 0;
		actualClient=findClient(events[i].data.fd);

		while(1)
			{
			rxcount = read(events[i].data.fd, rxbuf, sizeof rxbuf);
			if(rxcount == -1)
				{
				/* If errno == EAGAIN, that means we have read all
				 data. So go back to the main loop. */
				if(errno != EAGAIN)
					{
					perror("read");
					done = 1;
					}
				break;
				}
			else if(rxcount == 0)
				{
				/* End of file. The remote has closed the
				 connection. */
				done = 1;
				break;
				}
			onDataRead(actualClient,rxbuf,rxcount);
			}

		if(done)
			{
			onCloseConn(actualClient);
			closeClientConnection(actualClient);
			/* Closing the descriptor will make epoll remove it
			 from the set of descriptors which are monitored. */
			close(events[i].data.fd);
			}
		}
	}
}

//-----------------------------------------------------------------------------
// PRIVATE
//-----------------------------------------------------------------------------
/**
 * return the FD of a client
 * @param client
 * @return -1 not found, >=0 if found
 */
int SimpleServer::getClientFD(int client)
{
return (clients[client].connected) ? (clients[client].fd) : (-1);
}

/**
 * search the client form the fd
 * @param fd
 * @return -1 not found, >=0 if found
 */
int SimpleServer::findClient(int fd)
{
for(int i=0;i<SS_MAX_CLIENTS;i++)
	{
	if(clients[i].connected && clients[i].fd==fd) return i;
	}
return -1;
}

/**
 * add a client and return itd id
 * @param fd
 * @return id of the client (client number); -1 if full
 */
int SimpleServer::addClient(int fd)
{
//search for free
for(int i=0;i<SS_MAX_CLIENTS;i++)
	{
	if(!clients[i].connected)
		{
		clients[i].connected=true;
		clients[i].fd=fd;
		return i;
		}
	}
return -1;
}

/**
 * make non blocking socket
 * @param sfd
 * @return
 */
int SimpleServer::make_socket_non_blocking(int sfd)
{
int flags, s;

flags = fcntl(sfd, F_GETFL, 0);
if(flags == -1)
	{
	perror("fcntl");
	return -1;
	}

flags |= O_NONBLOCK;
s = fcntl(sfd, F_SETFL, flags);
if(s == -1)
	{
	perror("fcntl");
	return -1;
	}

return 0;
}

/**
 * creates the bind
 * @param port
 * @return
 */
int SimpleServer::create_and_bind(char *port)
{
struct addrinfo hints;
struct addrinfo *result, *rp;
int s, sfd;
int optval; /* flag value for setsockopt */

memset(&hints, 0, sizeof(struct addrinfo));
hints.ai_family = AF_UNSPEC; /* Return IPv4 and IPv6 choices */
hints.ai_socktype = SOCK_STREAM; /* We want a TCP socket */
hints.ai_flags = AI_PASSIVE; /* All interfaces */

s = getaddrinfo(NULL, port, &hints, &result);
if(s != 0)
	{
	fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
	return -1;
	}

for(rp = result;rp != NULL;rp = rp->ai_next)
	{
	sfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
	if(sfd == -1) continue;

	/* setsockopt: Handy debugging trick that lets
	 * us rerun the server immediately after we kill it;
	 * otherwise we have to wait about 20 secs.
	 * Eliminates "ERROR on binding: Address already in use" error.
	 */
	optval = 1;
	setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval , sizeof(int));

	s = bind(sfd, rp->ai_addr, rp->ai_addrlen);
	if(s == 0)
		{
		/* We managed to bind successfully! */
		break;
		}

	close(sfd);
	}

if(rp == NULL)
	{
	fprintf(stderr, "Could not bind\n");
	return -1;
	}

freeaddrinfo(result);

return sfd;
}


