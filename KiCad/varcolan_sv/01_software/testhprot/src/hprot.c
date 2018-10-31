#include "hprot.h"


//=============================================================================


//-----------------------------------------------------------------------------
// PROTOTYPES
//-----------------------------------------------------------------------------
static uint8_t CRCcalc(uint8_t ch,uint8_t *crc);

#ifdef HPROT_USE_CRYPTO
static uint8_t hprot_crypt(uint8_t plain, uint8_t prev, uint8_t key);
static uint8_t hprot_decrypt(uint8_t crypt, uint8_t prev, uint8_t key);
static uint8_t hprot_BStuffEncode(uint8_t ch,uint8_t *b1, uint8_t *b2);
static uint8_t hprot_BStuffDecode(uint8_t ch, uint8_t *res, uint8_t *st);

const uint8_t hprotDefaultKey[HPROT_CRYPT_KEY_SIZE]={HPROT_CRYPT_DEFAULT_KEY0,HPROT_CRYPT_DEFAULT_KEY1,HPROT_CRYPT_DEFAULT_KEY2,HPROT_CRYPT_DEFAULT_KEY3};
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
protData->flags.isCrypto=0;		// must be always initialised
protData->flags.invalidKey=0;

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
f->data=databuff;
f->len=0;
}

/**
 * reset the protocol
 * @param protData protocol data structure
 */
void hprotReset(protocolData_t *protData)
{
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

/**
 * parser of a frame
 * @param protData protocol data structure
 * @param f frame struct to fill (memory for data is allocated automatically, but the users must delete it)
 * @param ch byte received
 */
void hprotFrameParserByte (protocolData_t *protData, frame_t *f, uint8_t ch)
{
//............................
#ifdef HPROT_USE_CRYPTO
uint8_t dec;
if(hprotGetCryptoStatus(protData))
	{
#if defined HPROT_ENABLE_RX_RAW_BUFFER && defined HPROT_RX_RAW_BUFFER_CRYPTO
	protData->rxRawBuffer[protData->rxRawBufferCount]=ch;
	protData->rxRawBufferCount++;
#endif
	// in this case pre-elaborate the byte
	if(!((protData->rxInternalStatus == ipsm_IDLE) || (protData->rxInternalStatus == ipsm_HEADER)))
		{
		if(hprot_BStuffDecode(ch,&dec,&protData->cryptoData.bs_status)==0)
			{
			return;	// need of the next byte to decode
			}
		ch=hprot_decrypt(dec,protData->cryptoData.cryprev,protData->cryptoData.key[protData->cryptoData.keyptr]);
		protData->cryptoData.cryprev=dec;
		protData->cryptoData.keyptr++;
		protData->cryptoData.keyptr %= HPROT_CRYPT_KEY_SIZE;
		}
	else
		{
		protData->cryptoData.keyptr=HPROT_CRYPT_KPTR_INITIAL;
		protData->cryptoData.cryprev=HPROT_CRYPT_INITIAL;
		protData->cryptoData.bs_status=0;
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
		if(ch==HPROT_HDR_COMMAND || ch==HPROT_HDR_REQUEST  || ch==HPROT_HDR_ANSWER)
			{
#if defined HPROT_ENABLE_RX_RAW_BUFFER
			if(!protData->flags.isCrypto)
				{
				protData->rxRawBufferCount=0;
				protData->rxRawBuffer[protData->rxRawBufferCount]=ch;
				protData->rxRawBufferCount++;
				}
#endif

#ifdef HPROT_USE_CRYPTO
			protData->cryptoData.cryprev=HPROT_CRYPT_INITIAL;
			protData->cryptoData.keyptr=HPROT_CRYPT_KPTR_INITIAL;
#endif

			protData->protCondition=pc_inprogress;
      f->hdr=ch;
      protData->crc=HPROT_CRC8_INIT;
      protData->rxDataNdx=0;
      CRCcalc(ch,&protData->crc);
			//-------------
      if(protData->callback_onRxInit!=0) protData->callback_onRxInit(f);			// at the beginning of the frame call
			//-------------
			protData->rxInternalStatus=ipsm_SRCID;
			}
		break;
	//--------------------------------------------------------------------
	case ipsm_SRCID:
#if defined HPROT_ENABLE_RX_RAW_BUFFER
		if(!protData->flags.isCrypto)
			{
			protData->rxRawBuffer[protData->rxRawBufferCount]=ch;
			protData->rxRawBufferCount++;
			}
#endif
		CRCcalc(ch,&protData->crc);
		f->srcID=ch;
		protData->rxInternalStatus=ipsm_DSTID;
		break;
	//--------------------------------------------------------------------
	case ipsm_DSTID:
#if defined HPROT_ENABLE_RX_RAW_BUFFER
		if(!protData->flags.isCrypto)
			{
			protData->rxRawBuffer[protData->rxRawBufferCount]=ch;
			protData->rxRawBufferCount++;
			}
#endif
		CRCcalc(ch,&protData->crc);
		f->dstID=ch;
		protData->destDeviceID=f->srcID;
		protData->rxInternalStatus=ipsm_NUM;
		break;
	//--------------------------------------------------------------------
	case ipsm_NUM:
#if defined HPROT_ENABLE_RX_RAW_BUFFER
		if(!protData->flags.isCrypto)
			{
			protData->rxRawBuffer[protData->rxRawBufferCount]=ch;
			protData->rxRawBufferCount++;
			}
#endif
		CRCcalc(ch,&protData->crc);
		if(f->hdr==HPROT_HDR_REQUEST) protData->requestNumber=ch;
		f->num=ch;
		protData->rxInternalStatus=ipsm_CMD;
		break;
	//--------------------------------------------------------------------
	case ipsm_CMD:
#if defined HPROT_ENABLE_RX_RAW_BUFFER
		if(!protData->flags.isCrypto)
			{
			protData->rxRawBuffer[protData->rxRawBufferCount]=ch;
			protData->rxRawBufferCount++;
			}
#endif
		CRCcalc(ch,&protData->crc);
		f->cmd=ch;
		protData->rxInternalStatus=ipsm_LEN;
		break;
	//--------------------------------------------------------------------
	case ipsm_LEN:
#if defined HPROT_ENABLE_RX_RAW_BUFFER
		if(!protData->flags.isCrypto)
			{
			protData->rxRawBuffer[protData->rxRawBufferCount]=ch;
			protData->rxRawBufferCount++;
			}
#endif
		CRCcalc(ch,&protData->crc);
		f->len=ch;
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
			if(!protData->flags.isCrypto)
				{
				protData->rxRawBuffer[protData->rxRawBufferCount]=ch;
				protData->rxRawBufferCount++;
				}
#endif
			CRCcalc(ch,&protData->crc);
			//.......................
			if(protData->callback_onDataBuffer!=0) protData->callback_onDataBuffer(f,ch);			// at every data received
			//.......................
			protData->rxNDataCount--;
			f->data[protData->rxDataNdx++]=ch; // store data
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
#if defined HPROT_ENABLE_RX_RAW_BUFFER
		if(!protData->flags.isCrypto)
			{
			protData->rxRawBuffer[protData->rxRawBufferCount]=ch;
			protData->rxRawBufferCount++;
			}
#endif
    if(protData->crc!=ch)	// if crc are not equals
      {
    	// CRC Error
    	protData->protCondition=pc_error_crc; // crc error
      }
		else
			{
			//CRC OK
			protData->protCondition=pc_ok;	// for now it is right, but subject to further verifications

			if(protData->callback_onFrame!=0)	// if is defined
				{
				if(protData->callback_onFrame(f)==cmd_OK)				// exe command (-1 on unrecognized command)
					{
					protData->protCondition=pc_ok;
					}
				else			// unrecognised command
					{
					protData->protCondition=pc_error_cmd;
					}
				}
			if(protData->callback_onFrameGen!=0)	// if is defined
				{
				protData->callback_onFrame(protData->uData);
				protData->protCondition=pc_ok;
				}
			}
    protData->rxInternalStatus=ipsm_IDLE;
#ifdef HPROT_USE_CRYPTO
    protData->cryptoData.bs_status=0;
#endif
		break;
	}
}

#include <stdio.h>
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

/**
 * parse multiple data
 * @param protData protocol data structure
 * @param f frame struct to fill (memory for data is allocated automatically, but the users must delete it)
 * @param buf	buffer containing data to be parsed
 * @param ndata number of bytes still present in the buffer
 */
unsigned int hprotFrameParserNData (protocolData_t *protData, frame_t *f, uint8_t *buf, uint8_t ndata)
{
int i,n;

//-------------
// DEBUG
//char _tmp[HPROT_BUFFER_SIZE*2];
//hprotHex2AsciiHex(_tmp,buf,ndata,1);
//printf("RX Frame: %s\n",_tmp);
//-------------

for(i=0,n=0;i<ndata;i++)
	{
	hprotFrameParserByte(protData,f,buf[i]);
	n++;
	if(protData->protCondition==pc_ok || protData->protCondition==pc_error_crc || protData->protCondition==pc_error_cmd)
		{
		if(ndata > n)	// has more data than parsed, save the remaining for next step
			{
			memmove(buf,&buf[i],ndata-n);
			break;
			}
		}
	}
return ndata -= n;
}

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
unsigned int hprotFrameBuild(protocolData_t *protData,uint8_t *buff, uint8_t hdr, uint8_t srcId, uint8_t dstId, uint8_t cmd, uint8_t *data, uint8_t len, int fnumber)
{
uint8_t ndat,hdr1;
uint8_t crc=HPROT_CRC8_INIT;
int buffndx=0,i;

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
buff[buffndx++]=srcId;
buff[buffndx++]=dstId;
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

//............................
#ifdef HPROT_USE_CRYPTO
if(protData->flags.isCrypto)
	{
	buffndx=hprotBuffCrypt(protData,buff,buffndx);
	}
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
int i,j,cryp;
uint8_t tmpbuff[HPROT_PLAIN_BUFFER_SIZE];

if(size>HPROT_PLAIN_BUFFER_SIZE) return 0;

memcpy(tmpbuff,buff,size);
protData->cryptoData.cryprev=HPROT_CRYPT_INITIAL;
protData->cryptoData.keyptr=HPROT_CRYPT_KPTR_INITIAL;
for(i=1,j=1;i<size;i++)		// header is not encrypted
	{
	cryp=hprot_crypt(tmpbuff[i],protData->cryptoData.cryprev,protData->cryptoData.key[protData->cryptoData.keyptr]);
	protData->cryptoData.cryprev=cryp;
	protData->cryptoData.keyptr++;
	protData->cryptoData.keyptr %= HPROT_CRYPT_KEY_SIZE;
	j += hprot_BStuffEncode(cryp,&buff[j],&buff[j+1]);
	}
return j;
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
uint8_t CRCcalc(uint8_t ch,uint8_t *crc)
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
	case HPROT_BS_ETX:
	case HPROT_BS_STX:
		*b0=HPROT_BS_DLE;
		*b1=(uint8_t) ((0x80+ch) & 0xFF);
		printf("*dle:%02X -> %02X\n",ch, *b1);
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
	printf("dle:%02X\n",*res);
	*st=0;
	ret=1;
	}
return ret;
}
#endif
