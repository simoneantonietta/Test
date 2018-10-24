//============================================================================
// Name        : testhprot.cpp
// Author      : 
// Version     :
// Copyright   : Your copyright notice
// Description : Hello World in C++, Ansi-style
//============================================================================

#include <iostream>
#include <stdio.h>
#include <stdint.h>
#include "hprot.h"
using namespace std;

#if 0
/**
 * crypt algorithm
 * @param plain
 * @param prev
 * @param key
 * @return
 */
static uint8_t _hprot_crypt(uint8_t plain, uint8_t prev, uint8_t key)
{
uint8_t out;
out = plain^prev;
out ^= key;
return out;
//return plain;
}

/**
 * decrypt algorithm
 * @param crypt
 * @param prev
 * @param key
 * @return
 */
static uint8_t _hprot_decrypt(uint8_t crypt, uint8_t prev, uint8_t key)
{
uint8_t out;
out = crypt^key;
out ^= prev;
return out;
//return crypt;
}

/**
 * encode for the byte stuffing
 * @param ch data to check
 * @param b0 data stuffed (first) (can be DLE if return 2)
 * @param b1 data stuffed (first) (can be void if the byte is not stuffed)
 * @return 1 or 2 (if stuffed)
 */
static uint8_t _hprot_BStuffEncode(uint8_t ch,uint8_t *b0, uint8_t *b1)
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
		printf("dle+%02X\n",*b1);
		n=2;
		break;
	default:
		*b0=ch;
		printf("%02X\n",ch);
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
static uint8_t _hprot_BStuffDecode(uint8_t ch, uint8_t *res, uint8_t *st)
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

unsigned int _hprotBuffCrypt(protocolData_t *protData,uint8_t *buff,unsigned int size)
{
int i,j,cryp;
uint8_t tmpbuff[HPROT_PLAIN_BUFFER_SIZE];

if(size>HPROT_PLAIN_BUFFER_SIZE) return 0;

memcpy(tmpbuff,buff,size);
protData->cryptoData.cryprev=HPROT_CRYPT_INITIAL;
protData->cryptoData.keyptr=HPROT_CRYPT_KPTR_INITIAL;
for(i=1,j=1;i<size;i++)		// header is not encrypted
	{
	cryp=_hprot_crypt(tmpbuff[i],protData->cryptoData.cryprev,protData->cryptoData.key[protData->cryptoData.keyptr]);
	protData->cryptoData.cryprev=cryp;
	protData->cryptoData.keyptr++;
	protData->cryptoData.keyptr %= HPROT_CRYPT_KEY_SIZE;
	j += _hprot_BStuffEncode(cryp,&buff[j],&buff[j+1]);
	printf("*orig:%02X -> ",cryp);
	}
return j;
}

unsigned int _hprotBuffDecrypt(protocolData_t *protData,uint8_t *buff,unsigned int size)
{
int i,j=0;
uint8_t dec,ch;

if(size>HPROT_CRYPTO_BUFFER_SIZE) return 0;

for(i=0;i<size;i++)
	{
	ch=buff[i];
	printf("initial: %02X\n",ch);
	if(protData->flags.isCrypto)
		{
		// in this case pre-elaborate the byte
		if(i>0)
			{
			if(_hprot_BStuffDecode(ch,&dec,&protData->cryptoData.bs_status)==0)
				{
				continue;
				}
			printf("decoded: %02X\n",dec);
			ch=_hprot_decrypt(dec,protData->cryptoData.cryprev,protData->cryptoData.key[protData->cryptoData.keyptr]);
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

int main()
{
uint8_t buff1[HPROT_CRYPTO_BUFFER_SIZE],buff2[HPROT_CRYPTO_BUFFER_SIZE];
int i,j,sze,szd;
uint8_t v1,v2;
protocolData_t pd;

hprotInit(&pd,NULL,NULL,NULL,NULL);
hprotEnableCrypto(&pd);

// fill
for(i=0;i<HPROT_PLAIN_BUFFER_SIZE;i++)
	{
	buff1[i]=i;
	buff2[i]=i;
	}

sze=hprotBuffCrypt(&pd,buff2,HPROT_PLAIN_BUFFER_SIZE);
printf("encrypt size %d\n",sze);

szd=hprotBuffDecrypt(&pd,buff2,sze);
printf("decrypt size %d\n",szd);

// compare
int n=0;
for(i=0;i<HPROT_PLAIN_BUFFER_SIZE;i++)
	{
	if(buff1[i] != buff2[i])
		{
		printf("diff @%i v1=%02X v2=%02X\n",i,buff1[i],buff2[i]);
		n++;
		//return 1;
		}
	else
		{
		//printf("ok @%i v1=%02X v2=%02X\n",i,buff1[i],buff2[i]);
		}
	}
if(n==0) printf("NO DIFFS\n");
puts("done");
return 0;
}
