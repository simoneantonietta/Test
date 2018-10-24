/**----------------------------------------------------------------------------
 * PROJECT: prot6
 * PURPOSE:
 * communication protocol
 *
 * FORMAT
 * format: HDR|SRC|DST|NUM|CMD|LEN|data|CRC8
 *
 * each field is a byte
 * HDR can be a command (no answer required) or request (answer required) or ans answer (like a command)
 * NUM is a progressive number for the frame, incremented every frame is sent, but if
 * a request is performed, its answer must have the same number of the request
 * A command or request with another number during a request procedure will be ignored
 * IMPORTANT: tipically everu communication between two devices must begin with a CHECK_LINK request
 * This perform a registration of the id, without it nothing can know the presence of a specific device id
 *-----------------------------------------------------------------------------
 * CREATION: 28 jan 2015
 * Author: Luca Mini
 *
 * LICENCE: please see LICENCE.TXT file
 *
 * HISTORY (of the module):
 *-----------------------------------------------------------------------------
 * Author              | Date        | Description
 *-----------------------------------------------------------------------------
 * L. Mini               02/02/16      some corrections on parse algorithm (goto eliminated)
 *                                     raw buffer counter to 16 bit to avoid overflow
 * L. Mini               27/05/16      added cryptographic algorithm
 *-----------------------------------------------------------------------------
 */

#ifndef HPROT_H_
#define HPROT_H_

#include <stdint.h>
#include <string.h>

/* DEFINES:
 * -------------------------------------------------
 * define this to compile crypto algorithm
 * #define HPROT_USE_CRYPTO
 *
 * define this in your build environment to have raw buffer
 * #define HPROT_ENABLE_RX_RAW_BUFFER
 *
 * if you need raw buffer (HPROT_ENABLE_RX_RAW_BUFFER) and crypto,
 * you have to specify if the raw buffer must be plain or crypted,
 * so you need to specify
 * #define HPROT_RX_RAW_BUFFER_PLAIN
 * or
 * #define HPROT_RX_RAW_BUFFER_CRYPTO
 */

#if defined HPROT_RX_RAW_BUFFER_PLAIN && defined HPROT_RX_RAW_BUFFER_CRYPTO
#error "HPROT_RX_RAW_BUFFER_PLAIN and HPROT_RX_RAW_BUFFER_CRYPTO cannot be declared together"
#endif

#define HPROT_CRYPT_DEFAULT_KEY0	0x4c
#define HPROT_CRYPT_DEFAULT_KEY1	0x77
#define HPROT_CRYPT_DEFAULT_KEY2	0x37
#define HPROT_CRYPT_DEFAULT_KEY3	0x30

#define HPROT_CRYPT_INVALID_KEY		0x00	// the key composed of this byte is invalid (disable crypto)

#define HPROT_CRYPT_INITIAL				0x5A
#define HPROT_CRYPT_KPTR_INITIAL	0x00	// can be 0x00..0x03
#define HPROT_CRYPT_KEY_SIZE			4



// header definitions
#define HPROT_HDR_COMMAND					0x40
#define HPROT_HDR_REQUEST					0x41
#define HPROT_HDR_ANSWER					0x42

// for byte stuffing (used in crypto)
#define HPROT_BS_DLE							0x10
#define HPROT_BS_STX							0x02
#define HPROT_BS_ETX							0x03

// CRC8 definitions
#define HPROT_CRC8_INIT						0x00
#define HPROT_CRC8_POLY						0x18

// ID definitions
#define HPROT_INITIAL_ID					0
#define HPROT_INVALID_ID					0
#define HPROT_BROADCAST_ID				255
#define HPROT_DEBUG_ID						254

#define HPROT_PLAIN_BUFFER_SIZE		256			// size of header+crc plain (no payload)

#ifdef HPROT_USE_CRYPTO
#define HPROT_CRYPTO_BUFFER_SIZE	(HPROT_PLAIN_BUFFER_SIZE*2)	// size of header+crc crypto (no payload)
#define HPROT_BUFFER_SIZE					HPROT_CRYPTO_BUFFER_SIZE	// size of header+crc crypto (no payload)
#else
#define HPROT_BUFFER_SIZE					HPROT_PLAIN_BUFFER_SIZE	// size of header+crc (no payload)
#endif
#define HPROT_OVERHEAD_SIZE				7	// size of header+crc (no payload)
#define HPROT_PAYLOAD_SIZE				HPROT_BUFFER_SIZE-HPROT_OVERHEAD_SIZE

#define HPROT_SET_FNUMBER_DEFAULT	-1

// fields in the frame
#define HPROT_FLD_HDR			0
#define HPROT_FLD_SRC			1
#define HPROT_FLD_DST			2
#define HPROT_FLD_NUM			3
#define HPROT_FLD_CMD			4
#define HPROT_FLD_LEN			5
#define HPROT_FLD_DATA		6

//-----------------------------------------------------------------------------
// MACRO
//-----------------------------------------------------------------------------
// set my ID
#define HPROT_SETMYID(pdata,id) 			(pdata)->myID=id
// set the function pointers
#define HPROT_SETFUNC(funcptr,realfunc)	HProt->funcptr=realfunc;


/*
 * parser conditions
 */
typedef enum rxParserCondition_e
{
	pc_idle=0,
	pc_ok=1,
	pc_inprogress=2,
	pc_error_crc=3,
	pc_error_cmd=4,
} rxParserCondition_t;

typedef enum commandError_e {cmd_OK,cmd_ERROR} commandError_t;	// used to return the execute command status

/*
 * internal parser state machine
 */
typedef enum internalRxParserSM_e
{
	ipsm_IDLE,
	ipsm_HEADER,
	ipsm_NUM,
	ipsm_CMD,
	ipsm_LEN,
	ipsm_SRCID,
	ipsm_DSTID,
	ipsm_DATA,
	ipsm_CKCRC
} internalRxParserSM_t;
/*
 * frame structure
 */
typedef struct frame_st
{
uint8_t hdr;
uint8_t cmd;
uint8_t len;
uint8_t srcID; 	// sender (from)
uint8_t dstID;	// receiver (to)
uint8_t num;
uint8_t crc;
uint8_t *data;
}__attribute__ ((__packed__)) frame_t;

/**
 * contains all temporary data for the protocol
 */
typedef struct protocolData_st
{
	struct flags_st
		{
			uint8_t reqPending			:1;
			uint8_t ansPending			:1;
			uint8_t isCrypto				:1;	// must be always present (not under conditional compilation)
			uint8_t invalidKey			:1;	// if 1 the crypto is disabled due to the invalid key
		} flags;
	uint8_t myID;
	internalRxParserSM_t rxInternalStatus;
	uint8_t rxDataNdx;
	uint8_t rxNDataCount;
	uint8_t frameNumber;
	uint8_t requestNumber;
	uint8_t destDeviceID;
	uint8_t crc;

	rxParserCondition_t protCondition;	// result of the parser

#ifdef HPROT_ENABLE_RX_RAW_BUFFER
	uint8_t rxRawBuffer[HPROT_PLAIN_BUFFER_SIZE];
	uint16_t rxRawBufferCount;
#endif
	void *uData;				// data ptr used for callback_onFrameGen
	//-----------
	// callbacks
	//-----------
	void (*callback_onRxInit)(frame_t *f);
	void (*callback_onDataBuffer)(frame_t *f,uint8_t ch);
	commandError_t (*callback_onFrame)(frame_t *f);
	void (*callback_onFrameGen)(void *data);	// Useful for c++ or to send user data (it uses uData as parameter)

	#ifdef HPROT_USE_CRYPTO
	struct cryptoData_st
	{
		uint8_t cryprev,keyptr;
		uint8_t key[HPROT_CRYPT_KEY_SIZE];
		uint8_t bs_status;
	} cryptoData;
	#endif

} protocolData_t;


//-----------------------------------------------------------------------------
// PROTOTYPES
//-----------------------------------------------------------------------------
#ifdef __cplusplus
extern "C"
{
#endif

void hprotInit(protocolData_t *protData,void (*callback_onRxInit)(frame_t *f),void (*callback_onDataBuffer)(frame_t *f,uint8_t ch),commandError_t (*callback_onFrame)(frame_t *f),void (*callback_onFrameGen)(void *data));
void hprotFrameSetup(frame_t *f, uint8_t *databuff);
void hprotReset(protocolData_t *protData);
void hprotFrameParserByte (protocolData_t *protData, frame_t *f, uint8_t ch);
unsigned int hprotFrameParserNData (protocolData_t *protData, frame_t *f, uint8_t *buf, uint8_t ndata);
unsigned int hprotFrameBuild(protocolData_t *protData,uint8_t *buff, uint8_t hdr, uint8_t srcId, uint8_t dstId, uint8_t cmd, uint8_t *data, uint8_t len, int fnumber);

#ifdef HPROT_USE_CRYPTO
void hprotSetKey(protocolData_t *protData,uint8_t *key);
int hprotCheckKey(uint8_t *key);
void hprotEncDecKey(uint8_t *key_plain,uint8_t *key_encdec);
unsigned int hprotBuffCrypt(protocolData_t *protData,uint8_t *buff,unsigned int size);
unsigned int hprotBuffDecrypt(protocolData_t *protData,uint8_t *buff,unsigned int size);
void hprotEnableCrypto(protocolData_t *protData);
void hprotDisableCrypto(protocolData_t *protData);
int hprotGetCryptoStatus(protocolData_t *protData);

extern const uint8_t hprotDefaultKey[HPROT_CRYPT_KEY_SIZE];
#endif

#ifdef __cplusplus
}
#endif

#endif /*HPROT_H_ */
