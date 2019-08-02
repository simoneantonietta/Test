/*
 *-----------------------------------------------------------------------------
 * PROJECT: acqsrv
 * PURPOSE: see module CommSC8R.h file
 *-----------------------------------------------------------------------------
 */

#include "CommSC8R.h"

#include "GlobalData.h"
#include "global.h"
#include "TestUtils.h"
#include "utils/Trace.h"
#include "utils/Utils.h"

#define SC8R_STX			0x02
#define SC8R_ETX			0x03
#define SC8R_DLE			0x10
#define SC8R_ACK			0x06
#define SC8R_NACK			0x15


extern Trace *dbg;

/**
 * ctor
 */
CommSC8R::CommSC8R()
{
connected=false;
txBuff=new uint8_t[SC8R_MSG_MAX_SIZE];
rxBuff=new uint8_t[SC8R_MSG_MAX_SIZE];
}

/**
 * dtor
 */
CommSC8R::~CommSC8R()
{
delete [] txBuff;
delete [] rxBuff;
}

/**
 * open communication
 * @param dev
 * @param baud
 * @return true:ok
 */
bool CommSC8R::openCommunication(const string dev,int baud)
{
bool ret;
ser_fd=serialOpen(dev.c_str(),baud,0);
if(ser_fd>0)
	{
	serialSetBlocking(ser_fd,0,SC8R_DEF_TIMEOUT);// set blocking for 3 s
	connected=true;
	ret=true;
	}
else
	{
	dbg->trace(DBG_ERROR,"serial " + dev + " cannot be opened");
	ret=false;
	}
return ret;
}

/**
 * close the socket
 */
void CommSC8R::closeCommunication()
{
serialClose(ser_fd);
connected=false;
}

/**
 * send a message
 * @param type
 * @param addr
 * @param snPerif
 * @param data
 * @param len
 */
void CommSC8R::sendMsg(uint8_t type, uint8_t addr, uint8_t *snPerif, uint8_t *data, int len)
{
serialFlushRX(ser_fd);
int n=prepareMsg(txBuff,type,addr,snPerif,data,len);
printTx((char*)txBuff,n);
serialWrite(ser_fd,txBuff,n);
}

/**
 * read a message
 * @param nbytes number of expected bytes
 * @param to timeout (min 1s)
 * @return number of data really read
 */
int CommSC8R::readMsg(int to)
{
int n=0, ndestuff=0, i=0;
memset(rxBuff,0,SC8R_MSG_MAX_SIZE);
n=serialReadETX(ser_fd,rxBuff,SC8R_ETX,SC8R_MSG_MAX_SIZE,to);
//printRx((char*)rxBuff,n);
// destuffing :) overwriting the rx buffer
while(i<n)
	{
	rxBuff[ndestuff]=getByte(rxBuff,i);
	ndestuff++;
	}
printRx((char*)rxBuff,ndestuff);

if(rxBuff[ndestuff-1]!=SC8R_ETX)	// if it received the full message
	{
	dbg->trace(DBG_ERROR,"SC8R: message incomplete");
	n=0;	// to indicates an error
	ndestuff=n;
	}
return ndestuff;
}

/**
 * set the internal timeout (use only if you know what are you doing!)
 * @param to
 */
void CommSC8R::setTimeout(int to)
{
serialSetTimeout(ser_fd,to);
}
//-----------------------------------------------------------------------------
// PRIVATE MEMBERS
//-----------------------------------------------------------------------------
/**
 * prepare the message in a buffer
 * @param buff
 * @param type
 * @param addr
 * @param snPerif pointer to 4 bytes
 * @param data
 * @param len
 * @return number of bytes of the message to send
 */
int CommSC8R::prepareMsg(uint8_t* buff, uint8_t type, uint8_t addr, uint8_t *snPerif, uint8_t *data, int len)
{
int ndx=0;
//uint8_t cksum=0;

buff[ndx++]=SC8R_STX;
putByte(buff,ndx,type);
putByte(buff,ndx,addr);
if(snPerif!=0)
	{
	putByte(buff,ndx,snPerif[3]);
	putByte(buff,ndx,snPerif[2]);
	putByte(buff,ndx,snPerif[1]);
	putByte(buff,ndx,snPerif[0]);
	}
if(len>0)
	{
	for(int i=0;i<len;i++)
		{
		putByte(buff,ndx,data[i]);
		}
	}
buff[ndx++]=SC8R_ETX;
return ndx;
}

/**
 * write bytes taking in account the byte stuffing
 * @param dst
 * @param ndx
 * @param val
 */
void CommSC8R::putByte(uint8_t *dst, int &ndx,uint8_t val)
{
if(val==SC8R_STX || val==SC8R_ETX || val==SC8R_DLE)
	{
	dst[ndx++]=SC8R_DLE;
	dst[ndx++]=0x80+val;
	}
else
	{
	dst[ndx++]=val;
	}
}

/**
 * read bytes taking in account the byte stuffing
 * @param src
 * @param ndx index is modified accordingly
 * @param val
 * @return buffer "destuffed"
 */
uint8_t CommSC8R::getByte(uint8_t *src, int &ndx)
{
uint8_t v;
if(src[ndx]==SC8R_DLE)
	{
	ndx++;
	v=src[ndx++]-0x80;
	}
else
	{
	v=src[ndx++];
	}
return v;
}

/**
 * print in readable form the data received
 * @param buff
 * @param size
 */
void CommSC8R::printRx(char* buff, int size)
{
char hex[SC8R_MSG_MAX_SIZE*2];
Hex2AsciiHex(hex,(unsigned char*)buff,size,false,0);
dbg->trace(DBG_NOTIFY,"SC8R RX: (%d) %s",size,hex);
}

/**
 * print in readable form the data transmitted
 * @param buff
 * @param size
 */
void CommSC8R::printTx(char* buff, int size)
{
char hex[SC8R_MSG_MAX_SIZE*2];
Hex2AsciiHex(hex,(unsigned char*)buff,size,false,0);
dbg->trace(DBG_NOTIFY,"SC8R TX: (%d) %s",size,hex);
}

