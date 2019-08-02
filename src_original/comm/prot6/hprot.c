#include <stdlib.h>
#include "hprot.h"


//=============================================================================


//-----------------------------------------------------------------------------
// PROTOTYPES
//-----------------------------------------------------------------------------
static uint8_t CRCcalc(uint8_t ch,uint8_t *crc);
#ifdef HPROT_TOKENIZED_MESSAGES
static uint8_t calcNextToken(protocolData_t *protData);
static void setNextToken(protocolData_t *protData, uint8_t tokN, int bcast);
static uint8_t getNextToken(protocolData_t *protData);
#endif

#ifdef HPROT_USE_CRYPTO
static uint8_t hprot_crypt(uint8_t plain, uint8_t prev, uint8_t key);
static uint8_t hprot_decrypt(uint8_t crypt, uint8_t prev, uint8_t key);
static uint8_t hprot_BStuffEncode(uint8_t ch,uint8_t *b1, uint8_t *b2);
static uint8_t hprot_BStuffDecode(uint8_t ch, uint8_t *res, uint8_t *st);

const uint8_t hprotDefaultKey[HPROT_CRYPT_KEY_SIZE]={HPROT_CRYPT_DEFAULT_KEY0,HPROT_CRYPT_DEFAULT_KEY1,HPROT_CRYPT_DEFAULT_KEY2,HPROT_CRYPT_DEFAULT_KEY3};
#endif

#ifdef HPROT_DEBUG
char * hprotHex2AsciiHex(char *dst,unsigned char *src, int len, int sep);
#endif

//=============================================================================

/**
 * protocol initializer
 * @param protData
 * @param callback_onRxInit
 * @param callback_onDataBuffer
 * @param callback_onFrame
 */
void hprotInit(	protocolData_t *protData,
								void (*callback_onRxInit)(frame_t *f),
								void (*callback_onDataBuffer)(frame_t *f,uint8_t ch),
								commandError_t (*callback_onFrame)(frame_t *f),
								void (*callback_onFrameGen)(void *data))
{
HPROTDBG("HProt (%s)\n",__FUNCTION__);
protData->flags.isCrypto=0;		// must be always initialised
protData->flags.invalidKey=0;
protData->flags.rawPlain=HPROT_RXRAWDATA_PLAIN;

#ifdef HPROT_TOKENIZED_MESSAGES
protData->tokBuffSize=0;
protData->tokOffsetID=0;
protData->tokens=NULL;
protData->tokN=0;
#endif

#ifdef HPROT_USE_CRYPTO
int i;
for(i=0;i<HPROT_CRYPT_KEY_SIZE;i++)
	{
	protData->cryptoData.key[i]=hprotDefaultKey[i];
	}
if(hprotCheckKey(protData->cryptoData.key)==0)
	{
	protData->flags.invalidKey=1;
	protData->flags.isCrypto=0;		// if is invalid disable crypto
	}
#endif

hprotReset(protData);

// setup the callback
protData->callback_onRxInit=callback_onRxInit;
protData->callback_onDataBuffer=callback_onDataBuffer;
protData->callback_onFrame=callback_onFrame;
protData->callback_onFrameGen=callback_onFrameGen;
}

#ifdef HPROT_USE_CRYPTO
/**
 * enable cryptography (only itf the key is valid one)
 * @param protData
 */
void hprotEnableCrypto(protocolData_t *protData)
{
HPROTDBG("HProt (%s): enable\n",__FUNCTION__);
if(protData->flags.invalidKey==0)
	{
	protData->flags.isCrypto=1;
	}
else
	{
	protData->flags.isCrypto=0;
	}
}

/**
 * disable cryptography
 * @param protData
 */
void hprotDisableCrypto(protocolData_t *protData)
{
HPROTDBG("HProt (%s): disable\n",__FUNCTION__);
if(protData->flags.invalidKey==0)
	{
	protData->flags.isCrypto=0;	// if key is invalid, disable crypto
	}
}

/**
 * tell the status of cryptography
 * @param protData
 * @return 1=enabled; 0=disabled
 */
int hprotGetCryptoStatus(protocolData_t *protData)
{
return protData->flags.isCrypto;
}

#endif
/**
 * prepare the frame
 * @param f
 * @param databuff data buffer for the frame data
 */
void hprotFrameSetup(frame_t *f, uint8_t *databuff)
{
HPROTDBG("HProt (%s)\n",__FUNCTION__);
f->data=databuff;
f->len=0;
}

/**
 * reset the protocol
 * @param protData protocol data structure
 */
void hprotReset(protocolData_t *protData)
{
HPROTDBG("HProt (%s)\n",__FUNCTION__);
protData->crc=HPROT_CRC8_INIT;
protData->flags.ansPending=0;
protData->flags.reqPending=0;
protData->frameNumber=0;
protData->protCondition=pc_idle;
protData->requestNumber=0;
protData->rxInternalStatus=ipsm_IDLE;
protData->rxNDataCount=0;
protData->rxDataNdx=0;

#ifdef HPROT_ENABLE_RX_RAW_BUFFER
		protData->rxRawBufferCount=0;
#endif
#ifdef HPROT_USE_CRYPTO
		protData->cryptoData.keyptr=HPROT_CRYPT_KPTR_INITIAL;
		protData->cryptoData.cryprev=HPROT_CRYPT_INITIAL;
		if(protData->flags.invalidKey==1) protData->flags.isCrypto=0;	// if key is invalid, disable crypto
		protData->cryptoData.bs_status=0;
#endif
}

#if defined HPROT_ENABLE_RX_RAW_BUFFER
/**
 * set the raw buffer mode: plain or crypto
 * @param protData protocol data structure
 * @param mode 1 = plain; 0 = crypto
 */
void hprotSetRxRawPlain(protocolData_t *protData,uint8_t mode)
{
HPROTDBG("HProt (%s): %s\n",__FUNCTION__,(mode) ? ("plain") : ("crypto"));
protData->flags.rawPlain=mode;
}
#endif

/**
 * parse multiple data
 * @param protData protocol data structure
 * @param f frame struct to fill (memory for data is allocated automatically, but the users must delete it)
 * @param buf	buffer containing data to be parsed
 * @param ndata number of bytes still present in the buffer
 */
unsigned int hprotFrameParserNData (protocolData_t *protData, frame_t *f, uint8_t *buf, uint8_t ndata)
{
int n,np;

HPROTDBG("HProt (%s): start at status %d, ndata %d\n",__FUNCTION__,(int)protData->rxInternalStatus, ndata);

//-------------
// DEBUG
//char _tmp[HPROT_BUFFER_SIZE*2];
//hprotHex2AsciiHex(_tmp,buf,ndata,1);
//printf("RX Frame: %s\n",_tmp);
//-------------

for(n=0,np=0;n<ndata;n++)
	{
	np += hprotFrameParserByte(protData,f,buf[n]);
	if(protData->protCondition==pc_ok || protData->protCondition==pc_error_crc || protData->protCondition==pc_error_cmd)
		{
		if(ndata > (n+1))	// has more data than parsed, save the remaining for next step
			{
			n++;
			HPROTDBG("HProt (%s): data exceed of %d bytes\n",__FUNCTION__,ndata-n);
			memmove(buf,&buf[n-1],ndata-n);
			break;
			}
		}
	}
//-------------
// DEBUG
#ifdef HPROT_DEBUG
//HPROTDBG("HProt (%s): parsed %d/%d bytes \n",__FUNCTION__,n,ndata);
if(protData->protCondition==pc_error_crc)
	{
	char _tmp[HPROT_BUFFER_SIZE*2];
	hprotHex2AsciiHex(_tmp,buf,ndata,1);
	HPROTDBG("HProt (%s): CRC ndata=%d; parser=%d (id %d init st: %d): %s\n",__FUNCTION__,ndata,n,protData->destDeviceID,(int)protData->rxInternalStatus,_tmp);
	}
else if(protData->protCondition==pc_inprogress)
	{
	HPROTDBG("HProt (%s): exiting in status pc_inprogress\n",__FUNCTION__);
	}
else if(protData->protCondition==pc_error_cmd)
	{
	HPROTDBG("HProt (%s): command error %02X h\n",__FUNCTION__,f->cmd);
	}
#endif
//-------------
n=ndata-n;
return n;
}

/**
 * parser of a frame
 * @param protData protocol data structure
 * @param f frame struct to fill (memory for data is allocated automatically, but the users must delete it)
 * @param ch byte received
 * @return 0 if no char is parsed correctly, 1 if a char is recognized
 */
int hprotFrameParserByte (protocolData_t *protData, frame_t *f, uint8_t ch)
{
uint8_t chp=ch;
int ret=0;
//............................
#ifdef HPROT_USE_CRYPTO
uint8_t dec;
if(hprotGetCryptoStatus(protData))
	{
#if defined HPROT_ENABLE_RX_RAW_BUFFER
	if(!protData->flags.rawPlain)
		{
		protData->rxRawBuffer[protData->rxRawBufferCount]=ch;
		protData->rxRawBufferCount++;
		}
#endif
	// if not header pre-elaborate the byte
	if(!((protData->rxInternalStatus == ipsm_IDLE) || (protData->rxInternalStatus == ipsm_HEADER)))
		{
		//---------------------
		// TODO DEBUG
		//---------------------
		if(hprot_BStuffDecode(ch,&dec,&protData->cryptoData.bs_status)==0)
			{
			return ret;	// need of the next byte to decode
			}
		chp=hprot_decrypt(dec,protData->cryptoData.cryprev,protData->cryptoData.key[protData->cryptoData.keyptr]);
		protData->cryptoData.cryprev=dec;
		protData->cryptoData.keyptr++;
		protData->cryptoData.keyptr %= HPROT_CRYPT_KEY_SIZE;
		}
	}
#endif
//............................
switch(protData->rxInternalStatus)
	{
	//--------------------------------------------------------------------
	case ipsm_IDLE:
	//--------------------------------------------------------------------
	case ipsm_HEADER:
		if(chp==HPROT_HDR_COMMAND || chp==HPROT_HDR_REQUEST  || chp==HPROT_HDR_ANSWER)
			{
#if defined HPROT_ENABLE_RX_RAW_BUFFER
			if(protData->flags.rawPlain)
				{
				protData->rxRawBufferCount=0;
				protData->rxRawBuffer[protData->rxRawBufferCount]=chp;
				protData->rxRawBufferCount++;
				}
#endif

#ifdef HPROT_USE_CRYPTO
			protData->cryptoData.cryprev=HPROT_CRYPT_INITIAL;
			protData->cryptoData.keyptr=HPROT_CRYPT_KPTR_INITIAL;
			protData->cryptoData.bs_status=0;
#endif

			protData->protCondition=pc_inprogress;
      f->hdr=chp;
      protData->crc=HPROT_CRC8_INIT;
      protData->rxDataNdx=0;
      CRCcalc(chp,&protData->crc);
      ret=1;
			//-------------
      if(protData->callback_onRxInit!=0) protData->callback_onRxInit(f);			// at the beginning of the frame call
			//-------------
			protData->rxInternalStatus=ipsm_NUM;
			}
		break;
	//--------------------------------------------------------------------
	case ipsm_NUM:
#if defined HPROT_ENABLE_RX_RAW_BUFFER
		if(protData->flags.rawPlain)
			{
			protData->rxRawBuffer[protData->rxRawBufferCount]=chp;
			protData->rxRawBufferCount++;
			}
#endif
		CRCcalc(chp,&protData->crc);
		if(f->hdr==HPROT_HDR_REQUEST) protData->requestNumber=chp;
		f->num=chp;
		ret=1;
#ifdef HPROT_TOKENIZED_MESSAGES
		protData->rxInternalStatus=ipsm_TOKA;
#else
		protData->rxInternalStatus=ipsm_SRCID;
#endif
		break;

	//**********************************************************************
	#ifdef HPROT_TOKENIZED_MESSAGES
	//--------------------------------------------------------------------
	case ipsm_TOKA:
	#if defined HPROT_ENABLE_RX_RAW_BUFFER
			if(protData->flags.rawPlain)
				{
				protData->rxRawBuffer[protData->rxRawBufferCount]=chp;
				protData->rxRawBufferCount++;
				}
	#endif

			CRCcalc(chp,&protData->crc);
			f->tokA=chp;	// check is demanded to the application
				{
				protData->rxInternalStatus=ipsm_TOKN;
				ret=1;
				}
			break;
		//--------------------------------------------------------------------
		case ipsm_TOKN:
	#if defined HPROT_ENABLE_RX_RAW_BUFFER
			if(protData->flags.rawPlain)
				{
				protData->rxRawBuffer[protData->rxRawBufferCount]=chp;
				protData->rxRawBufferCount++;
				}
	#endif
			CRCcalc(chp,&protData->crc);
			f->tokN=chp;
			protData->rxInternalStatus=ipsm_SRCID;
			break;
	#endif
	//**********************************************************************

	//--------------------------------------------------------------------
	case ipsm_SRCID:
#if defined HPROT_ENABLE_RX_RAW_BUFFER
		if(protData->flags.rawPlain)
			{
			protData->rxRawBuffer[protData->rxRawBufferCount]=chp;
			protData->rxRawBufferCount++;
			}
#endif
		CRCcalc(chp,&protData->crc);
		f->srcID=chp;
		ret=1;
#ifdef HPROT_EXTENDED_ID
		protData->rxInternalStatus=ipsm_SRCID_H;
#else
		protData->rxInternalStatus=ipsm_DSTID;
#endif
		break;

#ifdef HPROT_EXTENDED_ID
	//--------------------------------------------------------------------
	case ipsm_SRCID_H:
	#if defined HPROT_ENABLE_RX_RAW_BUFFER
			if(protData->flags.rawPlain)
				{
				protData->rxRawBuffer[protData->rxRawBufferCount]=chp;
				protData->rxRawBufferCount++;
				}
	#endif
			CRCcalc(chp,&protData->crc);
			f->srcID |= ((hprot_idtype_t)chp << 8);
			ret=1;
			protData->rxInternalStatus=ipsm_DSTID;
			break;
#endif
	//--------------------------------------------------------------------
	case ipsm_DSTID:
#if defined HPROT_ENABLE_RX_RAW_BUFFER
		if(protData->flags.rawPlain)
			{
			protData->rxRawBuffer[protData->rxRawBufferCount]=chp;
			protData->rxRawBufferCount++;
			}
#endif
		CRCcalc(chp,&protData->crc);
		f->dstID=chp;
		protData->destDeviceID=f->srcID;
		ret=1;
#ifdef HPROT_EXTENDED_ID
		protData->rxInternalStatus=ipsm_DSTID_H;
#else
		protData->rxInternalStatus=ipsm_CMD;
#endif
		break;
#ifdef HPROT_EXTENDED_ID
	//--------------------------------------------------------------------
	case ipsm_DSTID_H:
	#if defined HPROT_ENABLE_RX_RAW_BUFFER
			if(protData->flags.rawPlain)
				{
				protData->rxRawBuffer[protData->rxRawBufferCount]=chp;
				protData->rxRawBufferCount++;
				}
	#endif
			CRCcalc(chp,&protData->crc);
			f->dstID |= ((hprot_idtype_t)chp << 8);
			ret=1;
			protData->rxInternalStatus=ipsm_CMD;
			break;
#endif

	//--------------------------------------------------------------------
	case ipsm_CMD:
#if defined HPROT_ENABLE_RX_RAW_BUFFER
		if(protData->flags.rawPlain)
			{
			protData->rxRawBuffer[protData->rxRawBufferCount]=chp;
			protData->rxRawBufferCount++;
			}
#endif
		CRCcalc(chp,&protData->crc);
		f->cmd=chp;
		ret=1;
		protData->rxInternalStatus=ipsm_LEN;
		break;
	//--------------------------------------------------------------------
	case ipsm_LEN:
#if defined HPROT_ENABLE_RX_RAW_BUFFER
		if(protData->flags.rawPlain)
			{
			protData->rxRawBuffer[protData->rxRawBufferCount]=chp;
			protData->rxRawBufferCount++;
			}
#endif
		CRCcalc(chp,&protData->crc);
		f->len=chp;
		ret=1;
		if(f->len > HPROT_PAYLOAD_SIZE)	//too many data
			{
			// in this case overwrites the size and it will generate probably a crc error
			f->len=1;
			}
		protData->rxNDataCount=f->len;	// to receive data (will be zero at the end)
		if(f->len==0)
			{
			protData->rxInternalStatus=ipsm_CKCRC;
			}
		else
			{
			protData->rxInternalStatus=ipsm_DATA;
			}
		break;
	//--------------------------------------------------------------------
	case ipsm_DATA:
		if(protData->rxNDataCount>0)
			{
#if defined HPROT_ENABLE_RX_RAW_BUFFER
			if(protData->flags.rawPlain)
				{
				protData->rxRawBuffer[protData->rxRawBufferCount]=chp;
				protData->rxRawBufferCount++;
				}
#endif
			CRCcalc(chp,&protData->crc);
			//.......................
			if(protData->callback_onDataBuffer!=0) protData->callback_onDataBuffer(f,chp);			// at every data received
			//.......................
			protData->rxNDataCount--;
			f->data[protData->rxDataNdx++]=chp; // store data
			ret=1;
			if(protData->rxNDataCount==0)
				{
				protData->rxInternalStatus=ipsm_CKCRC;
				}
			}
		else		// no data
			{
			protData->rxInternalStatus=ipsm_CKCRC;
			}
		break;
	//--------------------------------------------------------------------
	case ipsm_CKCRC:
		ret=1;
#if defined HPROT_ENABLE_RX_RAW_BUFFER
		if(protData->flags.rawPlain)
			{
			protData->rxRawBuffer[protData->rxRawBufferCount]=chp;
			protData->rxRawBufferCount++;
			}
#endif
    if(protData->crc!=chp)	// if crc are not equals
      {
    	// CRC Error
    	protData->protCondition=pc_error_crc; // crc error
      }
		else
			{
			//CRC OK
			f->crc=chp;
			protData->protCondition=pc_ok;	// for now it is right, but subject to further verifications

			if(protData->callback_onFrame!=0)	// if is defined
				{
				if(protData->callback_onFrame(f)!=cmd_OK)				// exe command (-1 on unrecognized command)
					{
					protData->protCondition=pc_error_cmd;
					}
				}
			if(protData->callback_onFrameGen!=0)	// if is defined
				{
				protData->callback_onFrameGen(protData->uData);
				}
			}
    protData->rxInternalStatus=ipsm_IDLE;
		break;
	}
return ret;
}

#ifdef HPROT_DEBUG
/**
 * convert a byte vector to a string of hexadecimals
 * @param dst buffer for the string result
 * @param src buffer of the values to be converted
 * @param len number of data in src
 * @return dst pointer
 */
char * hprotHex2AsciiHex(char *dst,unsigned char *src, int len, int sep)
{
int i;
*dst=(char) NULL;
char tmp[3];
for(i=0; i<len;i++)
	{
	sprintf(tmp,"%02X",((unsigned int)*(src+i) & 0xFF));
	if(sep) strcat(tmp," ");
	strcat(dst,tmp);
	}
return(dst);
}
#endif

/**
 * build a frame in memory from parameters
 * @param protData protocol data structure
 * @param buff where to build the frame to send
 * @param hdr specifies type: command, request or answer
 * @param srcId ID of the sender (usually my ID)
 * @param dstId ID of the destination
 * @param cmd command type
 * @param data  ptr to data
 * @param len len of the data buffer
 * @param fnumber frame number. If < 0 (HPROT_SET_FNUMBER_DEFAULT) use default, else set the the frame number at the value passed
 * @return frame len in bytes (0 if error)
 */
unsigned int hprotFrameBuild(protocolData_t *protData,uint8_t *buff, uint8_t hdr, hprot_idtype_t srcId, hprot_idtype_t dstId, uint8_t cmd, uint8_t *data, uint8_t len, int fnumber)
{
uint8_t ndat,hdr1,ntok;
uint8_t crc=HPROT_CRC8_INIT;
int buffndx=0,i;

#ifdef HPROT_DEBUG
char h;
h="CRA"[hdr-HPROT_HDR_COMMAND];
HPROTDBG("HProt (%s): hdr %c src %d dst %d cmd %d (%02X) len %d\n",__FUNCTION__,h,srcId,dstId,cmd,cmd,len);
#endif
// perform some checks for coherence
if((dstId==HPROT_BROADCAST_ID) || (dstId==HPROT_DEBUG_ID))	// in these cases cannot be a request or answer
	{
	hdr1=HPROT_HDR_COMMAND;
	}
else
	{
	hdr1=hdr;
	}

if(len>HPROT_PAYLOAD_SIZE)
	{
	// error: too many data
	return 0;
	}
// build buffer in memory
ndat=len;
buff[buffndx++]=hdr1;
if(fnumber<=HPROT_SET_FNUMBER_DEFAULT)
	{
	if(hdr==HPROT_HDR_ANSWER)
		{
		buff[buffndx++]=protData->requestNumber;
		}
	else
		{
		buff[buffndx++]=protData->frameNumber;
		protData->frameNumber++;
		}
	}
else
	{
	buff[buffndx++]=fnumber;
	}

protData->destDeviceID=dstId; // LM 02_11_2016 store last handled destination ID

//--------------------------------------
#ifdef HPROT_TOKENIZED_MESSAGES
switch(dstId)
	{
	// . . . . . . . . . . . . . . . . . . . . . . .
	case HPROT_BROADCAST_ID:
		buff[buffndx++]=0;						// tokA
		ntok=calcNextToken(protData);	// tokN
		setNextToken(protData,ntok,HPROT_BROADCAST_ID);
		buff[buffndx++]=ntok;
		break;
	case HPROT_SERVICE_ID:
		buff[buffndx++]=0;						// tokA
		ntok=calcNextToken(protData);	// tokN
		setNextToken(protData,ntok,HPROT_ID_NONE);
		buff[buffndx++]=ntok;
		break;
	// . . . . . . . . . . . . . . . . . . . . . . .
	case HPROT_DEBUG_ID:
		buff[buffndx++]=0;				// tokA
		buff[buffndx++]=0;				// tokN
		break;

	// . . . . . . . . . . . . . . . . . . . . . . .
	default:
		buff[buffndx++]=getNextToken(protData);	// tokA
		ntok=calcNextToken(protData);						// tokN
		setNextToken(protData,ntok,HPROT_ID_NONE);
		buff[buffndx++]=ntok;
		break;
	}
#endif
//--------------------------------------

*(hprot_idtype_t*) &buff[buffndx]=srcId;
buffndx += sizeof(hprot_idtype_t);
*(hprot_idtype_t*) &buff[buffndx]=dstId;
buffndx += sizeof(hprot_idtype_t);
buff[buffndx++]=cmd;
buff[buffndx++]=ndat;	// len

uint8_t *d=data;
for(;ndat>0;ndat--)
	{
	buff[buffndx++]=*d;
	d++;
	}

// calculate CRC
for(i=0;i<buffndx;i++)
	{
	CRCcalc(buff[i],&crc);
	}
buff[buffndx++]=crc;

HPROTDBG("HProt (%s): plain data frame size = %d\n",__FUNCTION__,buffndx);

//............................
#ifdef HPROT_USE_CRYPTO
if(protData->flags.isCrypto)
	{
	buffndx=hprotBuffCrypt(protData,buff,buffndx);
	}
HPROTDBG("HProt (%s): crypted data frame size = %d\n",__FUNCTION__,buffndx);
#endif
//............................

return buffndx;
}

#ifdef HPROT_USE_CRYPTO
/**
 * utility to crypt a buffer
 * @param protData
 * @param buff (must be correctly sized)
 * @param size
 * @return size of the crypted buffer
 */
unsigned int hprotBuffCrypt(protocolData_t *protData,uint8_t *buff,unsigned int size)
{
int i,len,n,cryp;
uint8_t tmpbuff[HPROT_PLAIN_BUFFER_SIZE];

if(size>HPROT_PLAIN_BUFFER_SIZE) return 0;

memcpy(tmpbuff,buff,size);
protData->cryptoData.cryprev=HPROT_CRYPT_INITIAL;
protData->cryptoData.keyptr=HPROT_CRYPT_KPTR_INITIAL;
for(i=1,len=1;i<size;i++)		// header is not encrypted
	{
	cryp=hprot_crypt(tmpbuff[i],protData->cryptoData.cryprev,protData->cryptoData.key[protData->cryptoData.keyptr]);
	protData->cryptoData.cryprev=cryp;
	protData->cryptoData.keyptr++;
	protData->cryptoData.keyptr %= HPROT_CRYPT_KEY_SIZE;
	n = hprot_BStuffEncode(cryp,&buff[len],&buff[len+1]);
	len += n;
	}
return len;
}

/**
 * utility to decrypt a buffer (WARN: not used in hprotFrameParseByte!)
 * @param protData
 * @param buff
 * @param size size of the crypted buffer
 * @return size of the decrypted buffer
 */
unsigned int hprotBuffDecrypt(protocolData_t *protData,uint8_t *buff,unsigned int size)
{
int i,j=0;
uint8_t dec,ch;

if(size>HPROT_CRYPTO_BUFFER_SIZE) return 0;

for(i=0;i<size;i++)
	{
	ch=buff[i];
	if(protData->flags.isCrypto)
		{
		// in this case pre-elaborate the byte
		if(i>0)
			{
			if(hprot_BStuffDecode(ch,&dec,&protData->cryptoData.bs_status)==0)
				{
				continue;
				}
			ch=hprot_decrypt(dec,protData->cryptoData.cryprev,protData->cryptoData.key[protData->cryptoData.keyptr]);
			protData->cryptoData.cryprev=dec;
			protData->cryptoData.keyptr++;
			protData->cryptoData.keyptr %= HPROT_CRYPT_KEY_SIZE;
			j++;
			}
		else
			{
			protData->cryptoData.keyptr=HPROT_CRYPT_KPTR_INITIAL;
			protData->cryptoData.cryprev=HPROT_CRYPT_INITIAL;
			protData->cryptoData.bs_status=0;
			}
		}
	else
		{
		j++;
		}
	buff[j]=ch;	// replace
	}
return j;
}
#endif

//-----------------------------------------------------------------------------
// PRIVATE
//-----------------------------------------------------------------------------
/**
 * calculate the crc8
 * @param ch byte to add in the calculation
 * @param crc reference to the previous or crc initial value
 * @return current crc value
 */
static uint8_t CRCcalc(uint8_t ch,uint8_t *crc)
{
uint8_t bit_counter;
uint8_t data;
uint8_t feedback_bit;
uint8_t vcrc;

data=ch; // temporary store
bit_counter = 8;
vcrc=*crc;

do
	{
	feedback_bit = (vcrc ^ data) & 0x01;

	if (feedback_bit == 0x01)
		vcrc = vcrc ^ HPROT_CRC8_POLY;

	vcrc = (vcrc >> 1) & 0x7F;

	if (feedback_bit == 0x01)
		vcrc = vcrc | 0x80;

	data = data >> 1;
	bit_counter--;
	} while (bit_counter > 0);
*crc=vcrc;

return vcrc;
}

//-----------------------------------------------------------------------------
// CIPHER
//-----------------------------------------------------------------------------
// implements CBC algorithm (Cipher Block Chaining)
#ifdef HPROT_USE_CRYPTO
/**
 * set the key
 * @param protData
 * @param key buffer containing HPROT_CRYPT_KEY_SIZE bytes
 */
void hprotSetKey(protocolData_t *protData,uint8_t *key)
{
int i;
for(i=0;i<HPROT_CRYPT_KEY_SIZE;i++)
	{
	protData->cryptoData.key[i]=key[i];
	}
protData->flags.invalidKey=0;
if(hprotCheckKey(protData->cryptoData.key)==0)
	{
	protData->flags.invalidKey=1;
	protData->flags.isCrypto=0;		// if is invalid disable crypto
	protData->flags.rawPlain=HPROT_RXRAWDATA_PLAIN;
	}
hprotReset(protData);
}

/**
 * check the key validity
 * @param protData
 * @return 0: key is invalid -> crypto will be disabled; 1: key ok
 */
int hprotCheckKey(uint8_t *key)
{
int i, nz=0;
for(i=0;i<HPROT_CRYPT_KEY_SIZE;i++)
	{
	if(key[i]==HPROT_CRYPT_INVALID_KEY)
		{
		nz++;
		}
	}
if(nz==HPROT_CRYPT_KEY_SIZE)	//check if it must disable crypto
	{
	return 0;
	}
return 1;
}

/**
 * encode/decode the key, used to tranfer the key with command HPROT_CMD_CRYPTO_NEW_KEY
 * @param key_plain
 * @param key_encdec
 */
void hprotEncDecKey(uint8_t *key_plain,uint8_t *key_encdec)
{
int i;
for(i=0;i<HPROT_CRYPT_KEY_SIZE;i++)
	{
	key_encdec[i]=key_plain[i] ^ hprotDefaultKey[i];
	}
}

/**
 * crypt algorithm
 * @param plain
 * @param prev
 * @param key
 * @return
 */
static uint8_t hprot_crypt(uint8_t plain, uint8_t prev, uint8_t key)
{
uint8_t out;
out = plain^prev;
out ^= key;
return out;
}

/**
 * decrypt algorithm
 * @param crypt
 * @param prev
 * @param key
 * @return
 */
static uint8_t hprot_decrypt(uint8_t crypt, uint8_t prev, uint8_t key)
{
uint8_t out;
out = crypt^key;
out ^= prev;
return out;
}

/**
 * encode for the byte stuffing
 * @param ch data to check
 * @param b0 data stuffed (first) (can be DLE if return 2)
 * @param b1 data stuffed (first) (can be void if the byte is not stuffed)
 * @return 1 or 2 (if stuffed)
 */
static uint8_t hprot_BStuffEncode(uint8_t ch,uint8_t *b0, uint8_t *b1)
{
uint8_t n=0;
switch(ch)
	{
	case HPROT_HDR_ANSWER:
	case HPROT_HDR_COMMAND:
	case HPROT_HDR_REQUEST:
	case HPROT_BS_DLE:
		*b0=HPROT_BS_DLE;
		*b1=(uint8_t) ((0x80+ch) & 0xFF);
		n=2;
		break;
	default:
		*b0=ch;
		n=1;
		break;
	}
return n;
}

/**
 * decode for the byte stuffing
 * @param ch byte to decode
 * @param res byte decoded
 * @param st status variable (INIT to 0)
 * @return 0 byte not decoded (res is invalid); 1 byte decoded (res is valid)
 */
static uint8_t hprot_BStuffDecode(uint8_t ch, uint8_t *res, uint8_t *st)
{
uint8_t ret;

if(*st==0)	// previous was normal data
	{
	if(ch==HPROT_BS_DLE)
		{
		*st=1;
		ret=0;
		}
	else
		{
		*res=ch;
		ret=1;
		}
	}
else	// previous was DLE
	{
	*res=ch-0x80;
	*st=0;
	ret=1;
	}
return ret;
}
#endif

//-----------------------------------------------------------------------------
// TOKENIZED MESSAGES
//-----------------------------------------------------------------------------
#ifdef HPROT_TOKENIZED_MESSAGES
/**
 * set the token buffer.
 * @param protData
 * @param tb pointer to the butter or NULL if you use a protocol data for each device ID connected
 * @param sz size of the buffer (typically 250)
 * @param offsetId memory offset ffrom the id specified
 */
void hprotSetTokensBuffer(protocolData_t *protData,uint8_t *tb,int sz,int offsetId)
{
int i;
protData->tokens=tb;
protData->tokBuffSize=sz;
protData->tokOffsetID=offsetId;
for(i=0;i<sz;i++) tb[i]=0;
}

/**
 * perform the reset of tokens generation, useful to start or to resync a communication
 * run this before build a new frame
 * @param protData
 * @param id device to reset, if id=broadcast reset all
 */
void hprotResetToken(protocolData_t *protData,int id)
{
if(protData->tokens!=NULL)
	{
	if(id==HPROT_BROADCAST_ID)
		{
		memset(protData->tokens,0,protData->tokBuffSize);
		}
	else
		{
		protData->tokens[id-protData->tokOffsetID]=0;
		}
	}
protData->tokN=0;
}

/**
 * check the token of the frame
 * Usually used in the onFrame callback together the destination ID check
 * @param protData
 * @param f
 * @return 0 fail;1 ok
 */
int hprotCheckToken(protocolData_t *protData,frame_t *f)
{
int ret=0;
uint8_t t;
switch(f->dstID)
	{
	// . . . . . . . . . . . . . . . . . .
	case HPROT_DEBUG_ID:
		ret=1;
		break;
	// . . . . . . . . . . . . . . . . . .
	case HPROT_SERVICE_ID:
	case HPROT_BROADCAST_ID:
		if(f->tokA==0)
			{
			ret=1;
			}
		break;

	// . . . . . . . . . . . . . . . . . .
	default:
		t=getNextToken(protData);
		if((f->tokA==0) || (f->tokA==t)) ret=1;
		break;
	}
return ret;
}

/**
 * application level function to update the token
 * Use this after the correct identification of the destination and the validity of the frame
 * (aka in the onFrame, after filtering for the destination)
 * @param protData
 * @param f
 */
void hprotUpdateToken(protocolData_t *protData, frame_t *f)
{
switch(f->dstID)
	{
	// . . . . . . . . . . . . . . . . . .
	case HPROT_DEBUG_ID:
		// do nothing
		break;
	// . . . . . . . . . . . . . . . . . .
	case HPROT_SERVICE_ID:
	case HPROT_BROADCAST_ID:
	default:
		setNextToken(protData,f->tokN,HPROT_ID_NONE);
		break;
	}
}

/**
 * generates a new token for the message to be sent
 * token must be always != 0, because 0 is the accepted in init communication only
 * @param protData
 * @return
 */
static uint8_t calcNextToken(protocolData_t *protData)
{
uint8_t ntok;
#ifdef HPROT_RANDOM_TOKEN
ntok=0;
while(ntok==0)
	{
	ntok=rand() % 256;
	}
#else
ntok=getNextToken(protData);
ntok++;
if(ntok==0) ntok++;
#endif
return ntok;
}


/**
 * set the next expected token taking in account the fact that you can have a vector that contains
 * tokens for the various devices connected and not a protocol data for each one.
 * @param protData
 * @param tok N next token
 * @param bcast =1 if broadcast set to all devices
 */
static void setNextToken(protocolData_t *protData, uint8_t tokN, int bcast)
{
protData->tokN=tokN;

if(protData->tokens!=NULL)
	{
	protData->tokens[protData->destDeviceID-protData->tokOffsetID]=protData->tokN;
	if(bcast==HPROT_BROADCAST_ID)
		{
		memset(protData->tokens,tokN,protData->tokBuffSize);
		}
	}
}

/**
 * get the next exepected token taking in account the fact that you can have a vector that contains
 * tokens for the various devices connected and not a protocol data for each one.
 * @param protData
 */
static uint8_t getNextToken(protocolData_t *protData)
{
if(protData->tokens!=NULL)
	{
	return protData->tokens[protData->destDeviceID-protData->tokOffsetID];
	}
else
	{
	return protData->tokN;
	}
}
#endif


//-----------------------------------------------------------------------------
// CRC UTILITIES
//-----------------------------------------------------------------------------

#ifdef USE_HPROT_UTIL_CRC8

#define CRC8_POLY							0x18


/**
 * calculate the crc8
 * @param ch byte to add in the calculation
 * @param crc reference to the previous or crc initial value
 * @return current crc value
 */
uint8_t hprot_CRC8(uint8_t ch, uint8_t *crc)
{
uint8_t bit_counter;
uint8_t data;
uint8_t feedback_bit;
uint8_t vcrc;

data = ch;  // temporary store
bit_counter = 8;
vcrc = *crc;

do
	{
	feedback_bit = (vcrc ^ data) & 0x01;

	if (feedback_bit == 0x01) vcrc = vcrc ^ CRC8_POLY;

	vcrc = (vcrc >> 1) & 0x7F;

	if (feedback_bit == 0x01) vcrc = vcrc | 0x80;

	data = data >> 1;
	bit_counter--;
	}
while (bit_counter > 0);
*crc = vcrc;

return vcrc;
}

/**
 * @brief Calculate CRC of an uint8_t array buffer till 256 bytes
 * @param buffer - buffer array pointer
 * @param size - size of array
 * @return crc8 calculation
 */
uint8_t hprot_CRC8calc(uint8_t * buffer, uint8_t size)
{
uint8_t crc = CRC8_INIT;
int i;
for (i = 0; i < size; i++)
	{
	crc8(buffer[i], &crc);
	}
return crc;
}
#endif


#ifdef USE_HPROT_UTIL_CRC16

#define CRC16_POLY						0x1021

/**
 * calculate the crc16
 * @param ch byte to add in the calculation
 * @param crc reference to the previous or crc initial value
 * @return current crc value
 */
uint16_t hprot_CRC16(uint8_t ch, uint16_t *crc)
{
uint16_t bit_counter;
uint16_t data;
uint16_t feedback_bit;
uint16_t vcrc;

data = ch;  // temporary store
bit_counter = 16;
vcrc = *crc;

do
	{
	feedback_bit = (vcrc ^ data) & 0x01;

	if (feedback_bit == 0x01) vcrc = vcrc ^ CRC16_POLY;

	vcrc = (vcrc >> 1) & 0x7FFF;

	if (feedback_bit == 0x01) vcrc = vcrc | 0x8000;

	data = data >> 1;
	bit_counter--;
	}
while (bit_counter > 0);
*crc = vcrc;

return vcrc;
}

/**
 * @brief Calculate CRC of an uint8_t array buffer but for size till 65536 bytes
 * @param buffer - buffer array pointer
 * @param size - size of array
 * @return crc8 calculation
 */
uint16_t hprot_CRC16calc(uint8_t * buffer, uint16_t size)
{
uint16_t crc = CRC16_INIT;
int i;
for (i = 0; i < size; i++)
	{
	crc16(buffer[i], &crc);
	}
return crc;
}

#endif


#ifdef USE_HPROT_UTIL_CRC32

#define CRC32_POLY						0x04C11DB7

/**
 * calculate the crc32
 * @param ch byte to add in the calculation
 * @param crc reference to the previous or crc initial value
 * @return current crc value
 */
uint32_t hprot_CRC32(uint8_t ch, uint32_t *crc)
{
uint32_t bit_counter;
uint32_t data;
uint32_t feedback_bit;
uint32_t vcrc;

data = ch;  // temporary store
bit_counter = 32;
vcrc = *crc;

do
	{
	feedback_bit = (vcrc ^ data) & 0x01;

	if (feedback_bit == 0x01) vcrc = vcrc ^ CRC32_POLY;

	vcrc = (vcrc >> 1) & 0x7FFFFFFF;

	if (feedback_bit == 0x01) vcrc = vcrc | 0x80000000;

	data = data >> 1;
	bit_counter--;
	}
while (bit_counter > 0);
*crc = vcrc;

return vcrc;
}



/**
 * @brief Calculate CRC of an uint8_t array buffer but for size till 2^32 bytes
 * @param buffer - buffer array pointer
 * @param size - size of array
 * @return crc8 calculation
 */
uint32_t hprot_CRC32calc(uint8_t * buffer, uint32_t size)
{
uint32_t crc = CRC32_INIT;
uint32_t i;
for (i = size; i > 0; i--)
	{
	crc32(buffer[size-i], &crc);
	}
return crc;
}

#endif

#ifdef USE_HPROT_UTIL_CKSUM
/**
 * @brief Calculate checksum of an uint8_t array buffer but for size till 2^32 bytes
 * @param buffer - buffer array pointer
 * @param size - size of array
 * @return checksum calculation
 */
uint32_t hprot_ChecksumCalc(uint8_t * buffer, uint32_t size)
{
uint32_t checksum = 0;
uint32_t i;
for (i = size; i > 0; i--)
	{
	checksum += buffer[i];
	}
return checksum;
}
#endif
