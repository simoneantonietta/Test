/*
 *-----------------------------------------------------------------------------
 * PROJECT: scserver
 * PURPOSE: see module SaetCommSrv.h file
 *-----------------------------------------------------------------------------
 */

#include "SaetCommSrv.h"


#define BASESECONDS	631148400


/**
 * ctor
 */
SaetCommSrv::SaetCommSrv()
{
}

/**
 * dtor
 */
SaetCommSrv::~SaetCommSrv()
{
}

/**
 * begin a communication
 * @param addr
 * @param port
 * @return
 */
bool SaetCommSrv::initComm(const std::string& addr, int port, int plantID)
{
udpsrv=new UdpServer();

if(udpsrv->openConnection(addr,port))
	{
	TRACE(dbg,DBG_NOTIFY,"server socket opened");
	}
else
	{
	return false;
	}
return true;
}

/**
 * prepare for answer
 * @param expectedAck
 * @param dg
 */
void SaetCommSrv::setExpectedAns(bool expectedAck,SC_datagram_t *dg)
{
this->expectedAck = expectedAck;
if(dg!=NULL) lastTxDatagram=*dg;
if(expectedAck) ansTimeout.startTimeout(GEMSS_ANS_TIMEOUT);
}

/**
 * send the datagram physically
 * @param dg
 * @return
 */
bool SaetCommSrv::sendDatagram(SC_datagram_t *dg)
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

udpsrv->datasend((char *)txBuff,sz);

setExpectedAns(true,dg);
msgid_out++;

return true;
}
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
void SaetCommSrv::buildDatagram(datagramType_e type,uint8_t len, uint8_t *data, SC_datagram_t *dg,SC_datagram_t *dg_req)
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
		dg->len=len+sizeof(SC_datagram_t);
		dg->plant=gd.client.plantId;
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
		dg->plant=gd.client.plantId;
		dg->node=0;
		dg->datetime=time(NULL) - BASESECONDS;
		dg->ext=1;
		break;
		//.........................................
	case dgtype_connection:
		dg->status.ev_cmd=SC_MSG_IS_EVENT;	// as event
		dg->status.ack=0;
		dg->status.keepAlive=0;
		dg->status.dataType=1;	// connection bit
		dg->status.retryFlag=0;
		dg->len=len+sizeof(SC_datagram_t);
		dg->plant=gd.client.plantId;
		dg->node=0;
		dg->datetime=time(NULL) - BASESECONDS;
		dg->ext=1;
		memcpy(dg->data,data,len);
		break;
	}
}

/**
 * build an ack starting from a previous message
 * @param dg
 * @param noData (default=true) if false maintain data on dg
 */
void SaetCommSrv::buildAck(SC_datagram_t *dg, bool noData)
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
void SaetCommSrv::buildEvent(uint8_t code, uint8_t *data, int sz, SC_datagram_t *dg)
{
tmpbuff[SC_EVT_FIXED_ID]=247;
tmpbuff[SC_EVT_TYPE]=SC_MY_TYPE;
tmpbuff[SC_EVT_CODE]=code;
memcpy(&tmpbuff[SC_EVT_DATA],data,sz);
buildDatagram(dgtype_event,sz+3,tmpbuff,dg);
}



/**
 * checking for the datagram validity
 * @param dg
 */
bool SaetCommSrv::preCheckDatagram(uint8_t *dg, int sz)
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
	if(d->ext!=1)  throw false;
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
			msgid_in++;
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
bool SaetCommSrv::datagramDecrypt(struct aes_ctx *ctx, uint8_t *data, int sz, uint8_t *out)
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
int SaetCommSrv::datagramEncrypt(struct aes_ctx *ctx, uint8_t *data, int sz, uint8_t *out)
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
// THREADS
//-----------------------------------------------------------------------------

/**
 * THREAD RX JOB
 * which is executed in the thread
 */
void SaetCommSrv::runRx()
{
//while (!endApplicationStatus())
while(1)
	{
	int n = udpsrv->datarecv((char*)internal_buf,SC_MSG_BUFFER_SIZE);
	//int n = recvfrom(sockfd, internal_buf, UDP_BUFSIZE, 0, (struct sockaddr *) &clientaddr, (unsigned int *)&clientlen);
	if(n > 0)
		{
		TRACE(dbg,DBG_DEBUG,"some data received from %s port %d",udpsrv->get_lastClientAddr().c_str(),udpsrv->get_lastClientPort());
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
			TRACE(dbg,DBG_DEBUG,"datagram ok");
			//rxParseStateMachine((SC_datagram_t *)rxBuf);
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
void SaetCommSrv::runTx()
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
bool SaetCommSrv::startThread(void *arg,char which)
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
void SaetCommSrv::joinThread(char which)
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
void SaetCommSrv::destroyThread(char which)
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
void *SaetCommSrv::execRx(void *thr)
{
reinterpret_cast<SaetCommSrv *>(thr)->runRx();
return NULL;
}

/**
 * Function that is used to be executed by the thread
 * @param thr
 */
void *SaetCommSrv::execTx(void *thr)
{
reinterpret_cast<SaetCommSrv *>(thr)->runTx();
return NULL;
}

//-----------------------------------------------------------------------------
// OTHERS
//-----------------------------------------------------------------------------

/**
 * used to debug: print all the datagrams
 * @param dir r|t
 * @param dat
 */
void SaetCommSrv::printRXTX(char dir,uint8_t *dat)
{
string msg="";
SC_datagram_t *dg;
char tmp[1024];

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

Hex2AsciiHex(tmp,dg->data,dg->len-SC_MSG_OVERHEAD,true,' ');
msg += _S"data=" + tmp;

TRACE(dbg,DBG_DEBUG,msg);
}


