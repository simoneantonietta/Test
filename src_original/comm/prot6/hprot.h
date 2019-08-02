/**----------------------------------------------------------------------------
 * PROJECT: prot6
 * PURPOSE:
 * communication protocol
 *
 * FORMAT
 * format: HDR|NUM|SRC|DST|CMD|LEN|data|CRC8
 *
 * if tokenized version is used:
 * format: HDR|NUM|tokA|tokN|SRC|DST|CMD|LEN|data|CRC8
 *
 * if id extended
 * format: HDR|NUM|SRCl|SRCh|DSTl|DSTh|CMD|LEN|data|CRC8
 * format: HDR|NUM|tokA|tokN|SRCl|SRCh|DSTl|DSTh|CMD|LEN|data|CRC8
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
 * L. Mini               20/06/16      added debug macro and some debug messages
 * L. Mini               07/07/16      changed the frame format: number is near the header to put
 *                                     more entropy in crypted messages
 * L. Mini               31/10/16      added string for HPROT_VERSION
 * L. Mini               31/10/16      add the compile option for HPROT_TOKENIZED_MESSAGES to improve
 *                                     the security
 *                                     it works so:
 *                                     first frame sent:           toksA=0  tokN=v1
 *                                     frame recv must have:       toksA=v1 tokN=v2
 *                                     frame sent:                 toksA=v2 tokN=v3
 *                                     and so on.
 *                                     - toksN is calculated by the sender (topically from tokA)
 *                                     - toksA=0 is always accepted
 *                                     - receiver must check if tokA == tokN, if yes -> ok else msg discarded
 *
 *                                     NOTE1: if too many errors occurs it needs to resync with TOK_RESYNC command
 *                                     with tokA=0
 *                                     NOTE2: if you use tokenized messages you must have a copy of protocol data for
 *                                            each device connected or a buffer to store all tokN
 *                                     NOTE3: chklnk may be performed with tokA=0;tokN=valid
 *                                     NOTE4: in broadcast messages, tokA=0;tokN=valid
 *                                     NOTE5: in debug messages: tokA=0,tokN=0
 *                                     NOTE6: in service messages: tokA=0,tokN=0
 * L. Mini               19/12/16      bugfix on token handling for special id
 * L. Mini               13/02/17      added extended ID version
 *-----------------------------------------------------------------------------
 */

#ifndef HPROT_H_
#define HPROT_H_

#include <stdint.h>
#include <string.h>

#ifdef HPROT_DEBUG
#include <stdio.h>
#endif

//=============================================================================
/*
 * please set this define globally to have a tokenized version of the protocol
 *
 * HPROT_TOKENIZED_MESSAGES
 * HPROT_EXTENDED_ID
 */
//#define HPROT_TOKENIZED_MESSAGES
//#define HPROT_EXTENDED_ID
//=============================================================================


#define HPROT_VERSION_MAJOR						"06"
#define HPROT_VERSION_MINOR						"02"
#define HPROT_VERSION_MAIN            HPROT_VERSION_MAJOR "." HPROT_VERSION_MINOR

/* MACROs to write version into the code (hardcoded)*/
#define HPROT_SET_FW_VERSION()				const volatile char __hprotversion__[]="hprot_version[" HPROT_VERSION_MAJOR "." HPROT_VERSION_MINOR "]"

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
 *
 * to avoid frame injection, enable
 * HPROT_TOKENIZED_MESSAGES
 * and implements function to check tokens
 *
 * to enable src/dst as a greater number (2bytes), enable
 * HPROT_EXTENDED_ID
 *
 */

#if defined HPROT_RX_RAW_BUFFER_PLAIN && defined HPROT_RX_RAW_BUFFER_CRYPTO
#error "HPROT_RX_RAW_BUFFER_PLAIN and HPROT_RX_RAW_BUFFER_CRYPTO cannot be declared together"
#endif

#ifdef HPROT_DEBUG
#define HPROTDBG(...)		printf(__VA_ARGS__)
#else
#define HPROTDBG(...)
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
#ifdef HPROT_TOKENIZED_MESSAGES
	#define HPROT_HDR_COMMAND				0x45
	#define HPROT_HDR_REQUEST				0x46
	#define HPROT_HDR_ANSWER				0x47
#else
	#define HPROT_HDR_COMMAND				0x40
	#define HPROT_HDR_REQUEST				0x41
	#define HPROT_HDR_ANSWER				0x42
#endif

// for byte stuffing (used in crypto)
#define HPROT_BS_DLE							0x10

// CRC8 definitions
#define HPROT_CRC8_INIT						0x00
#define HPROT_CRC8_POLY						0x18

// ID definitions
#define HPROT_ID_NONE							0
#define HPROT_INITIAL_ID					0
#define HPROT_INVALID_ID					0
#define HPROT_BROADCAST_ID				255
#define HPROT_DEBUG_ID						254
#define HPROT_SERVICE_ID					253

#define HPROT_PLAIN_BUFFER_SIZE		256			// size of header+crc plain (no payload)

#ifdef HPROT_USE_CRYPTO
#define HPROT_CRYPTO_BUFFER_SIZE	(HPROT_PLAIN_BUFFER_SIZE*2)	// size of header+crc crypto (no payload)
#define HPROT_BUFFER_SIZE					HPROT_CRYPTO_BUFFER_SIZE	// size of header+crc crypto (no payload)
#else
#define HPROT_BUFFER_SIZE					HPROT_PLAIN_BUFFER_SIZE	// size of header+crc (no payload)
#endif

//#ifdef HPROT_TOKENIZED_MESSAGES
//	#define HPROT_OVERHEAD_SIZE			9	// size of header+crc (no payload)
//	#define HPROT_RANDOM_TOKEN				// if defined tokens are random values else are counter based
//#else
//	#define HPROT_OVERHEAD_SIZE			7	// size of header+crc (no payload)
//#endif

#ifdef HPROT_TOKENIZED_MESSAGES
	#define __OH1										2		// additional overhead
	#define __OFF1									2		// offset for field specifier
	#define HPROT_RANDOM_TOKEN				// if defined tokens are random values else are counter based
#else
	#define __OH1										0		// additional overhead
	#define __OFF1									0		// offset for field specifier
#endif

#ifdef HPROT_EXTENDED_ID
	#define __OH2										2		// additional overhead
	#define __OFF2									1		// offset for field specifier
#else
	#define __OH2										0		// dditional overhead
	#define __OFF2									0	// offset for field specifier
#endif

#define HPROT_OVERHEAD_SIZE				(7 + __OH1 + __OH2)	// size of header+crc (no payload)
#define HPROT_PAYLOAD_SIZE				(HPROT_PLAIN_BUFFER_SIZE-HPROT_OVERHEAD_SIZE)

#define HPROT_SET_FNUMBER_DEFAULT	-1

// fields in the frame
#define HPROT_FLD_HDR				0
#define HPROT_FLD_NUM				1
#ifdef HPROT_TOKENIZED_MESSAGES
	#define HPROT_FLD_TOKA		2
	#define HPROT_FLD_TOKN		3
#endif
#ifdef HPROT_EXTENDED_ID
	#define HPROT_FLD_SRC_L		(2 + __OFF1)
	#define HPROT_FLD_SRC_H		(HPROT_FLD_SRC_L + 1)
	#define HPROT_FLD_DST_L		(HPROT_FLD_SRC_H + 1)
	#define HPROT_FLD_DST_H		(HPROT_FLD_DST_L + 1)
#endif
#define HPROT_FLD_SRC				(2 + __OFF1)
#define HPROT_FLD_DST				(3 + __OFF1 + __OFF2)
#define HPROT_FLD_CMD				(4 + __OFF1 + __OFF2 +__OFF2)
#define HPROT_FLD_LEN				(5 + __OFF1 + __OFF2 +__OFF2)
#define HPROT_FLD_DATA			(6 + __OFF1 + __OFF2 +__OFF2)

#define HPROT_RXRAWDATA_CRYPT			0
#define HPROT_RXRAWDATA_PLAIN			1

#define HPROT_MF_DATA_TRANSFER_OVERHEAD		3
#define HPROT_MF_DATA_TYPE_FLD						0
#define HPROT_MF_DATA_CHUNK_NUM_FLD				1
#define HPROT_MF_DATA_FLD									(HPROT_MF_DATA_TRANSFER_OVERHEAD-1)
#define HPROT_MF_DATA_CRC									1
#define HPROT_MF_MAX_CHUNK_SIZE						(200) //(HPROT_PAYLOAD_SIZE-HPROT_MF_DATA_TRANSFER_OVERHEAD)
#define HPROT_MF_START_MULTI_FRAME				0x00
#define HPROT_MF_END_MULTI_FRAME					0xFF
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

#ifdef HPROT_EXTENDED_ID
	typedef uint16_t hprot_idtype_t;
//	typedef union
//	{
//		struct
//		{
//		uint16_t id_l :8;
//		uint16_t id_h :8;
//		};
//		uint16_t id;
//	} __attribute__ ((__packed__)) hprot_idtype_t;
#else
	typedef uint8_t hprot_idtype_t;
#endif


/*
 * internal parser state machine
 */
typedef enum internalRxParserSM_e
{
	ipsm_IDLE,
	ipsm_HEADER,
	ipsm_NUM,
#ifdef HPROT_TOKENIZED_MESSAGES
	ipsm_TOKA,
	ipsm_TOKN,
#endif
	ipsm_CMD,
	ipsm_LEN,
	ipsm_SRCID,
	ipsm_DSTID,
#ifdef HPROT_EXTENDED_ID
	ipsm_SRCID_H,
	ipsm_DSTID_H,
#endif
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
hprot_idtype_t srcID; 	// sender (from)
hprot_idtype_t dstID;	// receiver (to)
uint8_t num;
#ifdef HPROT_TOKENIZED_MESSAGES
uint8_t tokA,tokN;
#endif
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
			uint8_t rawPlain				:1;	// if 1 the raw data is stored in crypto mode; else in plain mode
			uint8_t invalidKey			:1;	// if 1 the crypto is disabled due to the invalid key
		} flags;
	hprot_idtype_t myID;
	internalRxParserSM_t rxInternalStatus;

	uint8_t rxDataNdx;
	uint8_t rxNDataCount;
	uint8_t frameNumber;
	uint8_t requestNumber;
	hprot_idtype_t destDeviceID;
	uint8_t crc;

#ifdef HPROT_TOKENIZED_MESSAGES
	/*
	 * to handle subsequent messages: this in crypto mode is usefull to avoid the
	 * injection of previous messages and interpreted as correct one
	 * HPROT_SERVICE_ID skip this check anyway
	 */
	//uint8_t tokA;			// actual token
	uint8_t tokN;			// next token
	/*
	 * next pointer is  used when you have varius device but only one pd.
	 * this point to an a bytes array, where its index is the ID of the device.
	 * If not used must be set to NULL
	 */
	uint8_t *tokens;
	uint8_t tokBuffSize;	// size of the tokens buffer
	hprot_idtype_t tokOffsetID;	// set the id offset in the buffer (typically 0 or 1, but if your system
												// start with id=40 it is better use it to avoid allocation of unused memory)
#endif

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
int hprotFrameParserByte (protocolData_t *protData, frame_t *f, uint8_t ch);
unsigned int hprotFrameParserNData (protocolData_t *protData, frame_t *f, uint8_t *buf, uint8_t ndata);
unsigned int hprotFrameBuild(protocolData_t *protData,uint8_t *buff, uint8_t hdr, hprot_idtype_t srcId, hprot_idtype_t dstId, uint8_t cmd, uint8_t *data, uint8_t len, int fnumber);

#ifdef HPROT_TOKENIZED_MESSAGES
void hprotSetTokensBuffer(protocolData_t *protData,uint8_t *tb,int sz,int offsetId);
void hprotResetToken(protocolData_t *protData,int id);
int hprotCheckToken(protocolData_t *protData,frame_t *f);
void hprotUpdateToken(protocolData_t *protData, frame_t *f);
#endif

#ifdef HPROT_ENABLE_RX_RAW_BUFFER
void hprotSetRxRawPlain(protocolData_t *protData,uint8_t mode);
#endif

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


#ifdef USE_HPROT_UTIL_CKSUM
uint32_t hprot_ChecksumCalc(uint8_t * buffer, uint32_t size);
#endif

#ifdef USE_HPROT_UTIL_CRC8
#define CRC8_INIT						0x00

uint8_t hprot_CRC8(uint8_t ch, uint8_t *crc);
uint8_t hprot_CRC8calc(uint8_t * buffer, uint8_t size);
#endif

#ifdef USE_HPROT_UTIL_CRC16
#define CRC16_INIT					0x0000

uint16_t hprot_CRC16(uint8_t ch, uint16_t *crc);
uint16_t hprot_CRC16calc(uint8_t * buffer, uint16_t size);
#endif

#ifdef USE_HPROT_UTIL_CRC32
#define CRC32_INIT					0x00000000

uint32_t hprot_CRC32(uint8_t ch, uint32_t *crc);
uint32_t hprot_CRC32calc(uint8_t * buffer, uint32_t size);
#endif


#ifdef __cplusplus
}
#endif

#endif /*HPROT_H_ */
