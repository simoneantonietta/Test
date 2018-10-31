/*
 *-----------------------------------------------------------------------------
 * PROJECT: rasp_saetcom
 * PURPOSE: see module SaetComm.h file
 *-----------------------------------------------------------------------------
 */

#include "SaetComm.h"
#include <pthread.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <netinet/ether.h>
#include <linux/if_packet.h>
#include <netdb.h>
#include <sys/ioctl.h>

#define BASESECONDS	631148400

static const uint8_t ZeroBuf[16] = {0, };

SaetComm::SaetComm()
{
connected=false;
gemss_connected=false;
msgid_out=msgid_in=0;
useCrypto=false;
expectedAck=false;
}

SaetComm::~SaetComm()
{
if(startedRx)
	{
	joinThread('r');
	pthread_mutex_destroy(&mutexRx);
	}
if(startedTx)
	{
	joinThread('t');
	pthread_mutex_destroy(&mutexTx);
	}
}

//=============================================================================
/**
 * get credentials from a fixed server
 * @return
 */
bool SaetComm::getCredentials()
{
/*
 * REQUEST:
 * GET /api/plant/register HTTP/1.1
 * Host: 192.168.30.11:8081
 * plant_id: 123
 * mac: 00:50:c2:17:00:01
 * Cache-Control: no-cache
 *
 * ANSWER:
 * HTTP/1.1 200 OK
 * X-Powered-By: Express
 * Date: Tue, 03 May 2016 09:16:41 GMT
 * Connection: keep-alive
 * Content-Length: 65
 * {"register":"ok","endpoint":{"host":"192.168.30.11","port":4006}}
 * curl -X GET -H "plant_id: 123" -H "mac: 00:50:c2:17:00:01" http://192.168.30.11:8081/api/plant/register
 * {"register":"retry"}
 * {"register":"ok","endpoint":{"host":"192.168.30.11","port":4006}}
 *
 */

string mac,ip,msk,gw;
getNetworkData(gd.eth_interface,mac,ip,msk,gw);


int fd, i, n;
struct sockaddr_in sa;
struct hostent *host;
char *p, *par;

do
	{
	host = gethostbyname(gd.gemss.reg_server_ip.c_str());
	if(host)
		{
		sa.sin_family = AF_INET;
		sa.sin_port = htons(gd.gemss.reg_server_port);
		memcpy(&sa.sin_addr.s_addr, host->h_addr, 4);

		fd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
		if(!connect(fd, (struct sockaddr*) &sa, sizeof(struct sockaddr_in)))
			{
			p = (char*)malloc(1024);
			do
				{
				sprintf(p, "GET /api/plant/register HTTP/1.1\r\nHost: %s:%d\r\nplant_id: %d\r\n"
						"mac: %s\r\nCache-Control: no-cache\r\n\r\n",
						gd.gemss.reg_server_ip.c_str(), gd.gemss.reg_server_port, gd.plantID, mac.c_str());
				printf("\n\nTX %s", p);
				send(fd, p, strlen(p), 0);

				p[0] = 0;
				i = 0;
				do
					{
#warning TIMEOUT!!!!
					/* Usare select con timeout 10s */
					n = recv(fd, p + i, 1024 - i, 0);
					if(n > 0)
						{
						i += n;
						p[i] = 0;
						TRACE(dbg,DBG_DEBUG,"RX %s", p);
						}
					} while((n > 0) && !strstr(p, "}"));
				if(n > 0)
					{
					/**
					 * check for the credentials
					 */
					if(strstr(p, "\"register\"") && strstr(p, "\"retry\""))
						{
						// if missing retry
						sleep(1);
						n = 0;
						}
					else if(strstr(p, "\"register\"") && strstr(p, "\"endpoint\""))
						{
						// retrieve credentials
						vector<string> toks;
						Split(p,toks,"{\":,}");
						gd.gemss.ip=toks[4];
						gd.gemss.port=to_number<int>(toks[6]);

						host = gethostbyname(gd.gemss.ip.c_str());
						if(host) memcpy(&gd.gemss.addr_ip, host->h_addr, 4);
						}
					}
				else
				sleep(2);
				}
			while(n <= 0);

			free(p);
			}
		else
			{
			host = NULL;
			sleep(2);
			}
		close(fd);
		}
	else
		sleep(2);
	} while(!host);
TRACE(dbg,DBG_DEBUG,"Server credentials: %s %d\n", gd.gemss.ip.c_str(), gd.gemss.port);

return true;
}

/**
 * open the connection
 * @return
 */
bool SaetComm::openComm()
{
bool ret;

// generates the crypto table
aes_gen_tabs();

/*
 * socket: create the parent socket
 */
sockfd = socket(AF_INET, SOCK_DGRAM, 0);
if(sockfd < 0)
	{
	TRACE(dbg,DBG_ERROR, "opening UDP socket");
	ret=false;
	}
else
	{
	/* setsockopt: Handy debugging trick that lets
	 * us rerun the server immediately after we kill it;
	 * otherwise we have to wait about 20 secs.
	 * Eliminates "ERROR on binding: Address already in use" error.
	 */
	optval = 1;
	setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (const void *) &optval, sizeof(int));
	struct timeval tv;
	tv.tv_sec = 2;		// set timeout
	tv.tv_usec = 0;
	setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO,&tv,sizeof(tv));

	/*
	 * build the server's Internet address
	 */
	bzero((char *) &myaddr, sizeof(myaddr));
	myaddr.sin_family = AF_INET;
	myaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	myaddr.sin_port = htons((unsigned short) gd.myPort);

	hostaddrp = inet_ntoa(myaddr.sin_addr);
	/*
	 * bind: associate the parent socket with a port
	 */
	if(bind(sockfd, (struct sockaddr *) &myaddr, sizeof(myaddr)) < 0)
		{
		TRACE(dbg,DBG_ERROR, "on binding server addr %s @ port %d",hostaddrp,gd.myPort);
		ret=false;
		}
	else
		{
		memset(MAChost,0,sizeof(MAChost));
		getMACAddress("eth0");
		TRACE(dbg,DBG_NOTIFY, "My mac address: %02x.%02x.%02x.%02x.%02x.%02x",MAChost[0],MAChost[1],MAChost[2],MAChost[3],MAChost[4],MAChost[5]);
		//dbg->trace(DBG_NOTIFY, "listening server addr %s @ port %d",hostaddrp,param);
		TRACE(dbg,DBG_NOTIFY, "listening UDP server at port %d",gd.myPort);
		connected=true;
		clientlen = sizeof(clientaddr);
		ret=true;
		}
	}
return ret;
}

/**
 * check and set the status of the connection (timeout)
 * @return true: ok, no timeout
 */
bool SaetComm::checkConnectionTimeout()
{
if(connTimeout.getElapsedTime_s()>gd.connectionExpireTime)
	{
	gemss_connected=false;
	return false;
	}
return true;
}

#if 0
//=============================================================================
#define BITRESEND				0x01
#define BITCONNECT			0x08
#define BITKEEPALIVE		0x20
#define BITACK					0x40
#define BITCOMMAND			0x80

static const unsigned char ZeroBuf[16] = {0};
#define BUF_SIZ sizeof(struct ether_header)+sizeof(struct iphdr)+sizeof(struct udphdr)+30+6
struct UdpConf
{
	//int idx, Id, Port, Retries, Debug;
	int Port;
	int fdraw, eth0running;
	uint32_t IPsv;
	int lport;
	unsigned char MAChost[6], MACgw[6];
	unsigned char chall[16];
};
struct UdpConf udpconfig;

typedef struct
{
	unsigned short msgid;
	unsigned char status;
	unsigned short plant;
	unsigned char node;
	unsigned int datetime;
	unsigned char len;
	unsigned char extension;
	unsigned char event[256];
}__attribute__ ((packed)) UDP_event;

#define BASESECONDS	631148400

enum UdpState
{
	UDP_NULL = 0,
	UDP_REQUEST,
	UDP_CONFIRM,
	UDP_SECRET_SET,
	UDP_SECRET_CONF,
	UDP_CONNECTED,
	UDP_REQREG,
	UDP_CHALLENGE,
	UDP_REGISTER
};
/**
 * registration request
 * @return
 */
bool SaetComm::registrationRequest()
{
int v;
UDP_event umr;

umr.msgid = 0;
umr.status = BITCONNECT;
umr.plant = gd.plantID;
umr.node = 0;
umr.datetime = time(NULL) - BASESECONDS;
umr.len = 2+16+6+sizeof(UDP_event)-256;
umr.extension = 1;
umr.event[0] = UDP_REQREG;
umr.event[1] = 0;

/* Calcolo un challenge diverso da zero. */
do
{
  v = rand();
  memcpy(udpconfig.chall, &v, 4);
  v = rand();
  memcpy(udpconfig.chall+4, &v, 4);
  v = rand();
  memcpy(udpconfig.chall+8, &v, 4);
  v = rand();
  memcpy(udpconfig.chall+12, &v, 4);
}
while(!memcmp(udpconfig.chall, ZeroBuf, 16));

memcpy(umr.event+2, udpconfig.chall, 16);

/* Aggiungo il MAC address per identificare la centrale. */
memcpy(umr.event+2+16, udpconfig.MAChost, 6);

//debug(udpconfig.Debug & 1, idx, "TX", (unsigned char*)&umr, umr.len);
//support_log("Invio ReqReg");
sendto(sockfd, &umr, umr.len, 0, (struct sockaddr*)&serveraddr, sizeof(struct sockaddr_in));
return true;
}
#endif
//-----------------------------------------------------------------------------
// PRIVATE
//-----------------------------------------------------------------------------
/**
 * build a datagram
 * @param type
 * @param status
 * @param len data len (0 for header only)
 * @param data
 * @param dg		output datagram
 * @param dg_req (opt) if the datagram to build is an ack, this is used to clone the header
 */
void SaetComm::buildDatagram(datagramType_e type,uint8_t len, uint8_t *data, SC_datagram_t *dg,SC_datagram_t *dg_req)
{
uint16_t mid=msgid_out;

memset(dg,0,sizeof(SC_datagram_t));
dg->msgid=mid;
switch(type)
	{
	//.........................................
	case dgtype_event:
		dg->status.ev_cmd=SC_MSG_IS_EVENT;	// as event
		dg->status.ack=0;
		dg->status.keepAlive=1;
		dg->status.dataType=0;
		dg->status.retryFlag=0;
		dg->len=SC_MSG_OVERHEAD+len;
		dg->plant=gd.plantID;
		dg->node=0;
		dg->datetime=time(NULL) - BASESECONDS;
		dg->ext=1;
		memcpy(dg->data,data,len);
		break;
		//.........................................
	case dgtype_command:
		break;
		//.........................................
	case dgtype_ack:
		memcpy(dg,dg_req,sizeof(SC_datagram_t));
		dg->len=0;
		dg->status.ack=1;
		break;
		//.........................................
	case dgtype_keepalive:
		dg->status.ev_cmd=SC_MSG_IS_EVENT;	// as event
		dg->status.ack=0;
		dg->status.keepAlive=1;
		dg->status.dataType=0;
		dg->status.retryFlag=0;
		dg->len=0;
		dg->plant=gd.plantID;
		dg->node=0;
		dg->datetime=time(NULL) - BASESECONDS;
		dg->ext=1;
		break;
	//.........................................
	case dgtype_connection:
		dg->status.ev_cmd=SC_MSG_IS_EVENT;	// as event
		dg->status.ack=0;
		dg->status.keepAlive=0;
		dg->status.dataType=SC_MSG_IS_CONNECT;	// connection bit
		dg->status.retryFlag=0;
		dg->len=SC_MSG_OVERHEAD+len;
		dg->plant=gd.plantID;
		dg->node=0;
		dg->datetime=time(NULL) - BASESECONDS;
		dg->ext=1;
		memcpy(dg->data,data,len);
		break;
	//.........................................
	case dgtype_reqreg:
		msgid_out=0;
		dg->msgid=msgid_out;

		dg->status.ev_cmd=SC_MSG_IS_EVENT;	// as event
		dg->status.ack=0;
		dg->status.keepAlive=0;
		dg->status.dataType=SC_MSG_IS_CONNECT;	// connection bit
		dg->status.retryFlag=0;
		dg->len=SC_MSG_OVERHEAD+16+2;
		dg->plant=gd.plantID;
		dg->node=0;
		dg->datetime=time(NULL) - BASESECONDS;
		dg->ext=1;
		memcpy(dg->data,data,len);
		// challenge calculation
		do
			{
			int v = rand();
			memcpy(challenge_reqreg, &v, 4);
			v = rand();
			memcpy(challenge_reqreg+4, &v, 4);
			v = rand();
			memcpy(challenge_reqreg+8, &v, 4);
			v = rand();
			memcpy(challenge_reqreg+12, &v, 4);
			} while(!memcmp(challenge_reqreg, ZeroBuf, 16));

		dg->data[0]=SC_CONN_REQREG;
		dg->data[1]=0;
		memcpy(&dg->data[2], challenge_reqreg, 16);
		memcpy(&dg->data[2+16], MAChost, 6);	// mac address to identify the supervisor
		break;
	//.........................................
	case dgtype_register:
		dg->status.ev_cmd=SC_MSG_IS_EVENT;	// as event
		dg->status.ack=0;
		dg->status.keepAlive=0;
		dg->status.dataType=SC_MSG_IS_CONNECT;	// connection bit
		dg->status.retryFlag=0;
		dg->len=SC_MSG_OVERHEAD+len+1;
		dg->plant=gd.plantID;
		dg->node=0;
		dg->datetime=time(NULL) - BASESECONDS;
		dg->ext=1;
		dg->data[0]=SC_CONN_REGISTER;
		memcpy(&dg->data[1],data,len); // challenge data
		break;
	}
}

/**
 * build an ack starting from a previous message
 * @param dg
 * @param noData (default=true) if false maintain data on dg
 */
void SaetComm::buildAck(SC_datagram_t *dg, bool noData)
{
dg->status.ack=1;
if(noData)
	{
	dg->len=SC_MSG_OVERHEAD;	// cut out all data
	}
}

/**
 * send an event to gemss
 * @param code
 * @param data
 * @param sz size of data
 * @param dg datagram (output)
 */
void SaetComm::buildEvent(uint8_t code, uint8_t *data, int sz, SC_datagram_t *dg)
{
tmpbuff[SC_EVT_FIXED_ID]=247;
tmpbuff[SC_EVT_TYPE]=SC_MY_TYPE;
tmpbuff[SC_EVT_CODE]=code;
memcpy(&tmpbuff[SC_EVT_DATA],data,sz);
buildDatagram(dgtype_event,sz+3,tmpbuff,dg);
}

/**
 * prepare for answer
 * @param expectedAck
 * @param dg
 */
void SaetComm::setExpectedAns(bool expectedAck,SC_datagram_t *dg)
{
this->expectedAck = expectedAck;
if(dg!=NULL) lastTxDatagram=*dg;
if(expectedAck) ansTimeout.startTimeout(GEMSS_ANS_TIMEOUT);
}

/**
 * registration request
 * @return true: registered
 */
bool SaetComm::registrationRequest()
{
SC_datagram_t dg;
buildDatagram(dgtype_reqreg,0,tmpbuff,&dg);
sendDatagram(&dg);
return true;
}

/**
 * checking for the datagram validity
 * @param dg
 */
bool SaetComm::preCheckDatagram(uint8_t *dg, int sz)
{
bool ret=true;
SC_datagram_t *d=(SC_datagram_t*)dg;

needRealign=false;
needRetry=false;
try
	{
	if((unsigned int)sz < SC_MSG_OVERHEAD) throw false;	// if too small..
	//if(d->status.ack && d->status.retryFlag)	 throw false;
	//if(d->status._reserved || d->status._spare2 || d->status._spare4)  throw false;
	//if(d->node!=0)  throw false;
	//if(d->ext!=1)  throw false;
	if(d->status.dataType==SC_MSG_IS_CONNECT)	// is a connect
		{
		/* Poichè questo è il codice per la centrale, posso ricevere
		 solo i due comandi per le fasi 1 e 3, con bitCommand
		 necessariamente impostato.
		 Non devono essere presenti altri bit, a meno dell'Ack.
		 In caso di Ack, possono essere solo relativi alle fasi 2 e 4,
		 senza bitCommand impostato. */
		/* Nei messaggi di connessione non eseguo il controllo sul msgid
		 per permettere la connessione con i contatori sicuramente
		 disallienati. */
		if(d->status.ack!=SC_MSG_IS_ACK)
			{
			if(	(d->status.ev_cmd != SC_MSG_IS_COMMAND) ||
					(d->status.bStatus & ~((1 << SC_STATUS_FLD_DATATYPE) | (1 << SC_STATUS_FLD_TYPE) | (1 << SC_STATUS_FLD_RETRYFLAG))) ||
					(d->len==0))
				throw false;

			if(	(d->data[0] != SC_CONN_PHASE1) &&
					(d->data[0] != SC_CONN_PHASE3) &&
					(d->data[0] != SC_CONN_CHALLENGE))
			  throw false;

			if(	(d->data[0] == SC_CONN_PHASE1) &&
					(d->len != 5 + SC_MSG_OVERHEAD))
			  throw false;

			if(	(d->data[0] == SC_CONN_PHASE3) &&
					(d->len != 25 + SC_MSG_OVERHEAD))
			  throw false;

			if(	(d->data[0] == SC_CONN_CHALLENGE) &&
					(d->len != 17 + 16 + SC_MSG_OVERHEAD))
			  throw false;
			}
		else	// is ACK
			{
			//if((stato & bitCommand) || (stato & ~(bitConnect | bitAck | bitResend)) || !msg->len)
			if(	(d->status.ev_cmd == SC_MSG_IS_COMMAND) ||
					(d->status.bStatus & ~((1 << SC_STATUS_FLD_DATATYPE) | (1 << SC_STATUS_FLD_ACK) | (1 << SC_STATUS_FLD_RETRYFLAG))) ||
					(d->len==0))
				throw false;
			if( (d->data[0] != SC_CONN_PHASE2) &&
					(d->data[0] != SC_CONN_PHASE4) &&
					(d->data[0] != SC_CONN_REQREG) &&
					(d->data[0] != SC_CONN_REGISTER))
				throw false;

			if(	(d->data[0] == SC_CONN_PHASE2) &&
					(d->len != 5 + SC_MSG_OVERHEAD))
			  throw false;

			if(	(d->data[0] == SC_CONN_PHASE4) &&
					(d->len != 5 + SC_MSG_OVERHEAD))
				throw false;

			if(	(d->data[0] == SC_CONN_REQREG) &&
					(d->len != 2 + 16 + SC_MSG_OVERHEAD))
				throw false;

			if(	(d->data[0] == SC_CONN_REGISTER) &&
					(d->len != 17 + SC_MSG_OVERHEAD))
				throw false;
			}
		}
	else	// not a connect
		{
		/* Per i messaggi di Ack, il controllo sul msgid avviene nel loop principale
		 ed è riferito al contatore 'out', non al contatore 'in'. */
		//if(!(stato & bitAck))
		if(d->status.ack != SC_MSG_IS_ACK)	// se non è un ack
			{
			if(d->msgid != msgid_in)	// if not aligned
				{
				if(d->status.keepAlive == SC_MSG_IS_KEEPALIVE)
					{
					/* Il keepalive riceve l'ack anche se è fuori sequenza,
					 ma non modifica il contatore 'in' atteso.
					 Imposta però il bit Resend per segnalare la necessità
					 di un riallineamento attraverso una procedura di
					 connessione da parte del supervisore. */
					needRealign=true;
					}
				//else if((stato & bitResend) && (msg->msgid == (*msgid - 1)))
				else if((d->status.retryFlag==SC_MSG_IS_RETRY) && (d->msgid==msgid_in-1))
					{
					/* Se ricevo un messaggio con il bit Resend e contatore
					 pari all'ultimo messaggio ricevuto, significa che il
					 messaggio di Ack è andato perso e lo devo reinviare,
					 ma il messaggio era già stato trattato. */
					needRetry=true;
					}
				else
					{
					throw false;
					}
				}
	// GC 20080509
			else if(d->status.keepAlive==SC_MSG_IS_KEEPALIVE)
				{
				/* L'Ack non deve avere il bit di resend impostato se non per forzare il riallineamento. */
				//msg->stato &= ~bitResend;
				}
			//msgid_in++;
			}
		else
			{
			// ACK messages check
			}
		}
	}
catch(bool res)
	{
	ret=false;
	}
return ret;
}

/**
 * get the MAC address only
 * @param ethif ethernet interface name (i.e: "eth0")
 */
void SaetComm::getMACAddress(const char *ethif)
{
struct ifreq s;
int fd = socket(PF_INET, SOCK_DGRAM, IPPROTO_IP);

strcpy(s.ifr_name, ethif);
if(0 == ioctl(fd, SIOCGIFHWADDR, &s))
	{
	memcpy(MAChost, s.ifr_hwaddr.sa_data, 6);
	}
}

/**
 * get all the network data for the specified interface
 * @param dev
 * @param mac
 * @param ip
 * @param msk
 * @param gw
 */
void SaetComm::getNetworkData(string dev, string &mac, string &ip, string &msk, string &gw)
{
vector<string> toks;
string cmd;
FILE * netdata;
char buffer[100],*line_p;
// mac
cmd="cat /sys/class/net/" + dev +"/address";
netdata=popen(cmd.c_str(), "r");

line_p = fgets(buffer, sizeof(buffer), netdata);
for(unsigned int i=0;i<strlen(line_p);i++) if(line_p[i]=='\n') line_p[i]=0;
string _mac(line_p);

// ip & C.
cmd="ifconfig " + dev + " | grep \"inet \"";
netdata=popen(cmd.c_str(), "r");
// result: inet addr:192.168.10.100  Bcast:192.168.10.255  Mask:255.255.255.0
line_p = fgets(buffer, sizeof(buffer), netdata);
for(unsigned int i=0;i<strlen(line_p);i++) if(line_p[i]=='\n') line_p[i]=0;
string netinfo(line_p);
toks.clear();
Split(netinfo,toks,": \t");
string _ip=toks[2];
string _msk=toks[6];
// get the default gateway
netdata=popen("route -n | grep 'UG[ \t]' | awk '{print $2}'", "r");
line_p = fgets(buffer, sizeof(buffer), netdata);
string _gw(line_p);
TRACE(dbg,DBG_NOTIFY,"network data ip: %s msk: %s gw: %s mac: %s",_ip.c_str(),_msk.c_str(),_gw.c_str(),_mac.c_str());

ip=_ip;
msk=_msk;
gw=_gw;
mac=_mac;
}

/**
 * state machine of the protocol
 * @param dg received datagram
 */
void SaetComm::rxParseStateMachine(SC_datagram_t *dg)
{
bool stop=false;
SC_datagram_t tmpdg;

while(!stop)
	{
	stop=true;
	switch(scStatus)
		{
		//................................................................
		case scs_idle:
			// inverse connection (challenge phase)
			if((dg->status.ev_cmd==SC_MSG_IS_EVENT) && (dg->status.dataType==SC_MSG_IS_CONNECT) && (dg->data[0]==SC_CONN_CHALLENGE))
				{
				scStatus=scs_conn_challenge;
				stop=false;	// reparse
				}
			// connection
			else if((dg->status.ev_cmd==SC_MSG_IS_COMMAND) && (dg->status.dataType==SC_MSG_IS_CONNECT) && (dg->data[0]==SC_CONN_PHASE1))
				{
				scStatus=scs_conn_phase1;
				stop=false;	// reparse
				}
			else if((dg->status.ev_cmd==SC_MSG_IS_COMMAND) && (dg->status.dataType==SC_MSG_IS_CONNECT) && (dg->data[0]==SC_CONN_PHASE3))
				{
				scStatus=scs_conn_crypt1;
				stop=false;	// reparse
				}
			// keepalive
			else if(dg->status.keepAlive==SC_MSG_IS_KEEPALIVE)
				{
				scStatus=scs_keepalive;
				stop=false;	// reparse
				}
			// command
			else if((dg->status.ev_cmd==SC_MSG_IS_COMMAND) && (dg->status.dataType==SC_MSG_IS_CMD_EVT))
				{
				scStatus=scs_command;
				stop=false;	// reparse
				}
			// ack
			else if(dg->status.ack==SC_MSG_IS_ACK)
				{
				scStatus=scs_ack;
				stop=false;	// reparse
				}
			// event
			else if(dg->status.ev_cmd==SC_MSG_IS_EVENT)
				{
				scStatus=scs_ack;
				stop=false;	// reparse
				}
			//.  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .

			// if a good datagram is received restart the timeout
			if(connTimeout.getDeltaTime_s()<gd.connectionExpireTime)
				{
				connTimeout.initTime_s();
				}
			break;
		//................................................................
		case scs_conn_challenge:
			// check if challenge is good
			if(useCrypto) aes_decrypt(&ctx, &dg->data[17], &dg->data[17]);
			if(	memcmp(&dg->data[17],challenge_reqreg,SC_REQREG_CHALL_SIZE) || !memcmp(&dg->data[17], ZeroBuf, SC_REQREG_CHALL_SIZE))
				{
				// challenge mismatch-> ignore datagram
				}
			else
				{
				// proceed with ack and registration messages
				memset(challenge_reqreg, 0, SC_REQREG_CHALL_SIZE);
				// send ack
				buildAck(dg,true);
				sendDatagram(dg,false);
				buildDatagram(dgtype_register,SC_REQREG_CHALL_SIZE,&dg->data[1],dg,dg);
				datagramEncrypt(&ctx, &dg->data[1], SC_REQREG_CHALL_SIZE, &dg->data[1]);
				sendDatagram(dg,false);
				}
			break;
		//................................................................
		case scs_conn_phase1:
			/*
			 * if it was already connected to a sv
			 * if yes: 	accept only the new connection from the same IP
			 * 					else refuse it
			 * if not:  accept the new connection
			 */
			if(gemss_connected)
				{
				if(memcmp(&clientaddr.sin_addr,&gemssaddr.sin_addr,sizeof(gemssaddr.sin_addr))==0 )
					{
					// is the previous SV address -> accept
					scStatus=scs_conn_phase1_ack;
					stop=false;	// reparse
					}
				}
			else
				{
				// is the previous SV address -> accept
				scStatus=scs_conn_phase1_ack;
				stop=false;	// reparse
				}
			break;
		//................................................................
		case scs_conn_phase1_ack:
			buildAck(dg,false);
			sendDatagram(dg);
			scStatus=scs_conn_phase2;
			stop=false;	// go immediately to the next phase
			break;
		//................................................................
		case scs_conn_phase2:
			// generates a message with 4 random bytes
			tmpbuff[0]=SC_CONN_PHASE2;
			tmpbuff[1]=(uint8_t) (rand() % 256);
			tmpbuff[2]=(uint8_t) (rand() % 256);
			tmpbuff[3]=(uint8_t) (rand() % 256);
			tmpbuff[4]=(uint8_t) (rand() % 256);
			memcpy(challenge,&tmpbuff[1],4);	// store for the check in the nest phase
			buildDatagram(dgtype_connection,5,tmpbuff,&tmpdg);
			sendDatagram(&tmpdg);
			scStatus=scs_conn_phase2_ack;
			break;
		//................................................................
		case scs_conn_phase2_ack:
			// ack receive from gemss
			if(dg->status.ack)
				{
				// check for the data
				if(dg->data[0]==SC_CONN_PHASE2)
					{
					if(memcmp(challenge,&dg->data[1],4)==0)
						{
						// ok! accepted
						memcpy(&gemssaddr.sin_addr,&clientaddr.sin_addr,sizeof(gemssaddr.sin_addr));	// store gemss socket data
						msgid_in=msgid_out=0; // reset
						gemss_connected=true;
						connTimeout.initTime_s();
						TRACE(dbg,DBG_NOTIFY,"GEMMS registering ok");
						}
					}
				}
			else
				{
				TRACE(dbg,DBG_NOTIFY,"unexpected datagram (should be an ack to the registration");
				}
			scStatus=scs_idle;
			break;
		//................................................................
		case scs_conn_crypt1:	// key setup
			memcpy(aeskey,&dg->data[1],SC_KEY_SIZE);
			scStatus=scs_conn_crypt1_ack;
			stop=false;	// go immediately to the next phase
			break;
		//................................................................
		case scs_conn_crypt1_ack:
			buildAck(dg,false);
			sendDatagram(dg);
			scStatus=scs_conn_crypt2;
			stop=false;	// go immediately to the next phase
			break;
		//................................................................
		case scs_conn_crypt2:
			// generates a message with 4 random bytes
			tmpbuff[0]=SC_CONN_PHASE4;
			tmpbuff[1]=(uint8_t) (rand() % 256);
			tmpbuff[2]=(uint8_t) (rand() % 256);
			tmpbuff[3]=(uint8_t) (rand() % 256);
			tmpbuff[4]=(uint8_t) (rand() % 256);
			memcpy(challenge,&tmpbuff[1],4);	// store for the check in the nest phase
			buildDatagram(dgtype_connection,5,tmpbuff,&tmpdg);
			sendDatagram(&tmpdg);
			scStatus=scs_conn_crypt2_ack;
			break;
		//................................................................
		case scs_conn_crypt2_ack:
			// ack receive from gemss
			if(dg->status.ack)
				{
				// check for the data
				if(dg->data[0]==SC_CONN_PHASE4)
					{
					if(memcmp(challenge,&dg->data[1],4)==0)
						{
						// ok! key accepted
						aes_set_key(&ctx, aeskey, SC_KEY_SIZE);
						useCrypto=true;
						msgid_in=msgid_out=0; // reset
						TRACE(dbg,DBG_NOTIFY,"crypto key setup -> crypto comm enabled");
						}
					}
				}
			else
				{
				TRACE(dbg,DBG_NOTIFY,"unexpected datagram (should be an ack to the crypto)");
				}
			scStatus=scs_idle;
			break;
		//................................................................
		case scs_ack:
			// check for expected msgid
			if(isWaitingAnswer())
				{
				if(dg->msgid==msgid_in)
					{
					TRACE(dbg,DBG_DEBUG,"ack received");
					msgid_in++;
					setExpectedAns(false);
					// todo
					}
				else
					{
					TRACE(dbg,DBG_DEBUG,"ack received with unexpected msgid (%d, expected %d)",dg->msgid,msgid_in);
					}
				}
			else
				{
				TRACE(dbg,DBG_DEBUG,"unexpected ack received");
				}
			break;
		//................................................................
		case scs_command:
			//TRACE(dbg,DBG_DEBUG,"command received");
			exeCommand(dg);
			break;
		//................................................................
		case scs_event:
			if(dg->msgid==msgid_in)
				{
				TRACE(dbg,DBG_DEBUG,"event from GEMSS received");
				eventConverter(dg);

				buildAck(dg);
				sendDatagram(dg);
				msgid_in++;
				}
			else
				{
				TRACE(dbg,DBG_DEBUG,"event from GEMSS received with unexpected msgid (%d, expected %d)",dg->msgid,msgid_in);
				}
			break;
		//................................................................
		case scs_keepalive:
			if(dg->msgid==msgid_in)
				{
				TRACE(dbg,DBG_DEBUG,"keepalive received");
				buildAck(dg);
				sendDatagram(dg);
				msgid_in++;
				}
			else
				{
				TRACE(dbg,DBG_DEBUG,"keepalive received with unexpected msgid (%d, expected %d)",dg->msgid,msgid_in);
				}
			break;
		}
	}
}

/**
 * send the datagram physically
 * @param dg
 * @param forcePlain force to send in plan even if crypto is enabled
 * @return
 */
bool SaetComm::sendDatagram(SC_datagram_t *dg,bool forcePlain)
{
int sz;
printRXTX('t',(uint8_t *)dg);
if(useCrypto)
	{
	sz=datagramEncrypt(&ctx,(uint8_t *)dg,dg->len,txBuff);
	}
else
	{
	memcpy(txBuff,dg,sizeof(SC_datagram_t));
	sz=dg->len;
	}
clientaddr.sin_family=AF_INET;
inet_pton(AF_INET, gd.gemss.ip.c_str(), &(gemssaddr.sin_addr));
gemssaddr.sin_port=htons(gd.gemss.port);
hostaddrp = inet_ntoa(gemssaddr.sin_addr);
//TRACE(dbg,DBG_DEBUG,"send datagram to %s",hostaddrp);
int txndata=sendto(sockfd, txBuff, sz, 0, (struct sockaddr*)&gemssaddr, clientlen);
if(txndata<0)
	{
	TRACE(dbg,DBG_ERROR,"in sendto");
	txndata=0;
	return false;
	}

setExpectedAns(true,dg);
msgid_out++;

return true;
}

/**
 * execute a command from gemss
 * @param dg
 * @return true :ok; false: unrecognized
 */
bool SaetComm::exeCommand(SC_datagram_t *dg)
{
bool ret=true;

if(dg->data[SC_CMD_TYPE]==SC_MY_TYPE)
	{
	switch(dg->data[SC_CMD_CODE])
		{
		//........................................................
		case SC_CMDGEMSS_SET_DATE:		// date setup
			TRACE(dbg,DBG_DEBUG,"date setup: %d/%d/%d",(int)dg->data[SC_CMD_DATA],(int)dg->data[SC_CMD_DATA+1],(int)dg->data[SC_CMD_DATA+2]+2000);
			break;
		//........................................................
		case SC_CMDGEMSS_SET_TIME:		// time setup
			TRACE(dbg,DBG_DEBUG,"time setup: %d:%d %d",(int)dg->data[SC_CMD_DATA],(int)dg->data[SC_CMD_DATA+1],(int)dg->data[SC_CMD_DATA+2]);
			break;

		//........................................................
		//........................................................
		default:
			TRACE(dbg,DBG_DEBUG,"Unrecognised command %d", (int)dg->data[SC_CMD_CODE]);
			ret=false;
			break;
		}

	// send ack if right
	if(ret)
		{
		buildAck(dg);
		sendDatagram(dg);
		}
	}
return ret;
}

/**
 * convert an event in the SV format and will be pushed in the queue
 * @param dg
 * @return true :ok; false: unrecognized
 */
bool SaetComm::eventConverter(SC_datagram_t *dg)
{
bool ret=true;

if(dg->data[SC_EVT_FIXED_ID]==SC_EVT_FIXED_ID_VALUE && dg->data[SC_EVT_TYPE]==SC_MY_EVENT_TYPE)
	{
	switch(dg->data[SC_EVT_CODE])
		{
		//........................................................
//		case SC_CMDGEMSS_SET_DATE:
//			break;

		//........................................................
		//........................................................
		default:
			TRACE(dbg,DBG_DEBUG,"Unrecognised event %d", (int)dg->data[SC_EVT_CODE]);
			ret=false;
			break;
		}

	// send ack if right
	if(ret)
		{
		buildAck(dg);
		sendDatagram(dg);
		}
	}
return ret;
}

//-----------------------------------------------------------------------------
// RX THREAD
//-----------------------------------------------------------------------------
/**
 * create thread
 * @param detach
 * @param size
 * @param which can be 't' for tx or 'r' for rx
 * @return
 */
bool SaetComm::createThread(bool detach, int size,char which)
{
int ret;
bool res=true;
pthread_attr_t *_attr;
if(which=='r')
	{
	_attr=&_attrRx;
	}
else
	{
	_attr=&_attrTx;
	}

if ((ret = pthread_attr_init(_attr)) != 0)
	{
	cout << strerror(ret) << endl;
	throw "Error";
	}
if (detach)
	{
	if ((ret = pthread_attr_setdetachstate(_attr, PTHREAD_CREATE_DETACHED)) != 0)
		{
		cout << strerror(ret) << endl;
		throw "Error";
		}
	}
if (size >= PTHREAD_STACK_MIN)
	{
	if ((ret = pthread_attr_setstacksize(_attr, size)) != 0)
		{
		cout << strerror(ret) << endl;
		res=false;
		//throw "Error";
		}
	}
return res;
}

//-----------------------------------------------------------------------------
// THREADS
//-----------------------------------------------------------------------------

/**
 * THREAD RX JOB
 * which is executed in the thread
 */
void SaetComm::runRx()
{
//while (!endApplicationStatus())
while(1)
	{
	int n = recvfrom(sockfd, internal_buf, UDP_BUFSIZE, 0, (struct sockaddr *) &clientaddr, (unsigned int *)&clientlen);
	if(n > 0)
		{
		// determine who sent the datagram
	//	hostp = gethostbyaddr((const char *) &clientaddr.sin_addr.s_addr, sizeof(clientaddr.sin_addr.s_addr), AF_INET);
	//	if(hostp == NULL)
	//		{
	//		dbg->trace(DBG_ERROR,"on gethostbyaddr");
	//		}
		hostaddrp = inet_ntoa(clientaddr.sin_addr);
		//TRACE(dbg,DBG_DEBUG,"some data received from %s",hostaddrp);
		if(hostaddrp == NULL)
			{
			TRACE(dbg,DBG_ERROR,"on inet_ntoa");
			}

		if(useCrypto)	// decrypt
			{
			if(datagramDecrypt(&ctx,internal_buf,n,rxBuf)==false)
				{
				TRACE(dbg,DBG_DEBUG,"invalid size of the crypted datagram");
				}
			}
		else
			{
			memcpy(rxBuf,internal_buf,n);
			}
		printRXTX('r',(uint8_t *)rxBuf);

		if(preCheckDatagram(rxBuf,n))
			{
			//TRACE(dbg,DBG_DEBUG,"datagram ok");
			rxParseStateMachine((SC_datagram_t *)rxBuf);
			}
		else
			{
			TRACE(dbg,DBG_DEBUG,"datagram error");
			}
		}
	}
startedRx=false;
destroyThread('r');
cout << "Communication RX tread stopped" << endl;
}

/**
 * THREAD TX JOB
 * which is executed in the thread
 */
void SaetComm::runTx()
{

startedTx=false;
destroyThread('t');
cout << "Communication TX tread stopped" << endl;
}

/**
 * start the execution of the thread
 * @param arg (Uses default argument: arg = NULL)
 * @param which can be 't' for tx or 'r' for rx
 */
bool SaetComm::startThread(void *arg,char which)
{
bool ret=true;
bool *_started;
void *_arg;
pthread_t *_id;

if(which=='r')
	{
	_started=&startedRx;
	_arg=this->argRx;
	_id=&_idRx;
	}
else
	{
	_started=&startedTx;
	_arg=this->argTx;
	_id=&_idTx;
	}

if (!*_started)
	{
	*_started = true;
	_arg = arg;
	/*
	 * Since pthread_create is a C library function, the 3rd
	 * argument is a global function that will be executed by
	 * the thread. In C++, we emulate the global function using
	 * the static member function that is called exec. The 4th
	 * argument is the actual argument passed to the function
	 * exec. Here we use this pointer, which is an instance of
	 * the Thread class.
	 */
	if(which=='r')
		{
		if ((ret = pthread_create(_id, NULL, &execRx, this)) != 0)
			{
			cout << strerror(ret) << endl;
			//throw "Error";
			ret=false;
			}
		}
	else
		{
		if ((ret = pthread_create(_id, NULL, &execTx, this)) != 0)
			{
			cout << strerror(ret) << endl;
			//throw "Error";
			ret=false;
			}
		}
	}
return ret;
}

/**
 * Allow the thread to wait for the termination status
 * @param which can be 't' for tx or 'r' for rx
 */
void SaetComm::joinThread(char which)
{
if(which=='r')
	{
	if(startedRx)
		{
		pthread_join(_idRx, NULL);
		}
	}
else
	{
	if(startedTx)
		{
		pthread_join(_idTx, NULL);
		}
	}
}

/**
 * destroy the thread
 * @param which can be 't' for tx or 'r' for rx
 */
void SaetComm::destroyThread(char which)
{
int ret;
pthread_t *_id;
if(which=='r')
	{
	_id=&_idRx;
	}
else
	{
	_id=&_idTx;
	}

pthread_detach(*_id);
pthread_exit(0);

if(!noattrRx)
	{
	if((ret = pthread_attr_destroy(&_attrRx)) != 0)
		{
		cout << strerror(ret) << endl;
		throw "Error";
		}
	}
}

/**
 * Function that is used to be executed by the thread
 * @param thr
 */
void *SaetComm::execRx(void *thr)
{
reinterpret_cast<SaetComm *>(thr)->runRx();
return NULL;
}

/**
 * Function that is used to be executed by the thread
 * @param thr
 */
void *SaetComm::execTx(void *thr)
{
reinterpret_cast<SaetComm *>(thr)->runTx();
return NULL;
}

//-----------------------------------------------------------------------------
// CRYPT
//-----------------------------------------------------------------------------
/**
 * decrypt the received datagram
 * @param ctx
 * @param data
 * @param sz
 * @param out if specified copies data into this buffer else it will overwrite data
 * @return
 */
bool SaetComm::datagramDecrypt(struct aes_ctx *ctx, uint8_t *data, int sz, uint8_t *out)
{
int i;
uint8_t *d;

if(sz & 0x0f) return false;	// must be a multiple of 16

if(out==NULL)
	{
	d=data;
	}
else
	{
	memcpy(out,data,sz);
	d=out;
	}

for(i = 0;i < sz;i += 16)
	{
	aes_decrypt(ctx, d + i, d + i);
	}
return true;
}

/**
 * encrypt the datagram to be transmitted
 * @param ctx
 * @param data
 * @param sz
 * @param out if specified copies data into this buffer else it will overwrite data
 * @return number of bytes encrypted
 */
int SaetComm::datagramEncrypt(struct aes_ctx *ctx, uint8_t *data, int sz, uint8_t *out)
{
int i,n;
uint8_t *d;

if(out==NULL)
	{
	d=data;
	}
else
	{
	memcpy(out,data,sz);
	d=out;
	}

n = i = sz;
// padding
n += 15;
n = (n & ~0x0f);
for(;i < n;i++)
	{
	d[i] = 0;
	}
for(i = 0;i < n;i += 16)
	{
	aes_encrypt(ctx, d + i, d + i);
	}
return n;
}

//-----------------------------------------------------------------------------
// OTHERS
//-----------------------------------------------------------------------------

/**
 * used to debug: print all the datagrams
 * @param dir r|t
 * @param dat
 */
void SaetComm::printRXTX(char dir,uint8_t *dat)
{
string msg="";
SC_datagram_t *dg;
char tmp[UDP_BUFSIZE*2];

dg=(SC_datagram_t *)dat;
if(dir=='t')
	{
	msg += "SC_TX: ";
	}
else
	{
	msg += "SC_RX: ";
	}
sprintf(tmp,"%04X",(unsigned int)dg->msgid);
msg += _S"n=" + tmp + " ";

sprintf(tmp,"%02X",(unsigned int)dg->status.bStatus);
msg += _S"s=" + tmp + " ";
//sprintf(tmp,"\n");
//if(dg->status.retryFlag) sprintf(tmp,"\t(retry)\n");
//(dg->status.dataType) ? (sprintf(tmp,"\t(conn)\n")) : (sprintf(tmp,"\t(cmd_evt)\n"));
//if(dg->status.keepAlive) sprintf(tmp,"\t(keepalive)\n");
//if(dg->status.ack) sprintf(tmp,"\t(ack)\n");
//(dg->status.ev_cmd) ? (sprintf(tmp,"\t(cmd)\n")) : (sprintf(tmp,"\t(evt)\n"));
//msg += _S tmp;
//.................................................
// print in human mode the status
if(dg->status.dataType==SC_MSG_IS_CONNECT)
	{
	msg += _S"<con> " ;
	}

if(dg->status.ev_cmd==SC_MSG_IS_EVENT)
	{
	msg += _S"<ev> " ;
	}
else
	{
	msg += _S"<cmd> " ;
	}


if(dg->status.keepAlive==SC_MSG_IS_KEEPALIVE)
	{
	msg += _S"<keepalive> " ;
	}

if(dg->status.retryFlag==SC_MSG_IS_RETRY)
	{
	msg += _S"<<retry>> " ;
	}


if(dg->status.ack)
	{
	msg += _S"<ack> " ;
	}
//.................................................

sprintf(tmp,"%d",(unsigned int)dg->plant);
msg += _S"p=" + tmp + " ";

//sprintf(tmp,"%d",(unsigned int)dg->datetime);
//msg += _S"dt=" + tmp + " ";

sprintf(tmp,"%d(%d)",(unsigned int)dg->len,(unsigned int)dg->len-SC_MSG_OVERHEAD);
msg += _S"L=" + tmp + " ";

if(dg->len==SC_MSG_OVERHEAD)
	{
	msg +="data=NONE";
	}
else
	{
	Hex2AsciiHex(tmp,dg->data,dg->len-SC_MSG_OVERHEAD,true,' ');
	msg += _S"data=" + tmp;
	}
TRACE(dbg,DBG_DEBUG,msg);
}

