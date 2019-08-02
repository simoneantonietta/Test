/**----------------------------------------------------------------------------
 * PROJECT: hprot51
 * PURPOSE:
 * here some standard commands that all applications shound answer to, but
 * it is not mandatory
 *-----------------------------------------------------------------------------  
 * CREATION: Jan 29, 2015
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

#ifndef HPROT5_SRC_PROTT_STDCMD_H_
#define HPROT5_SRC_PROT_HPROT_STDCMD_H_

#define HPROT_CMD_NONE										0x00	// INVALID COMMAND!!!

/*
 * ack/nack communication or answer
 */
#define HPROT_CMD_ACK											0x01	// Acknowledge/Yes/true
#define HPROT_CMD_NACK										0x02	// Not Acknowledge/NO/false

/*
 * used to check if a connection is active
 * ans: ACK
 */
#define HPROT_CMD_CHKLNK									0x03

/*
 * debug frame
 * send a debug string
 * destination must be the debug ID
 * ...
 * ans: none
 */
#define HPROT_CMD_DEBUG_MSG								0x04

/*
 * debug frame
 * destination must be the debug ID
 * send 2 bytes that means a message
 * [0..1] message number
 */
#define	HPROT_CMD_DEBUG_MAPPED_MSG				0x05

/*
 * debug frame
 * destination must be the debug ID
 * permits to send values of some mapped variables
 * [0] variable index
 * [1] type
 * [2] value
 * and so on....
 *
 * Multiple values are permitted
 * type can be:
 * 'b'	signed byte
 * 'B'	unsigned byte
 * 'w'	int 16bit
 * 'W'	unsigned int 16bit
 * 'i'	signed integer 32bit
 * 'I'	unsigned integer 32bit
 * 'f'	float (32bit)
 * 'd'	double
 */
#define	HPROT_CMD_DEBUG_VALUES_MSG				0x06

/*
 * sent the new crypto key
 * [0..3] xored key
 * encrypt: key XOR HPROT_CRYPT_DEFAULT_KEY)
 * decrypt: received_key XOR HPROT_CRYPT_DEFAULT_KEY
 * ans:
 * ack/nack
 */
#define HPROT_CMD_CRYPTO_NEW_KEY					0x07

/*
 * used with tokenized messages only!
 * when some device need to be resync to receive frames
 * It happens when a frame is lost so the frame can have a tokA field wrong
 * requires no data
 * this command must have tokA = 0
 * ans:
 * ack/nack
 */
#define HPROT_CMD_TOKEN_RESYNC						0x08

/*
 * send a chunk of data
 * [0]    data type (0x00, is the start; 0xff is end; others are available)
 * [1,2]  chunk number
 * the length of the chunk can be derived from length of the frame data as
 * chunk size=len-HPROT_DATA_TRANSFER_OVERHEAD;
 * the last frame (data type = 0xff) in the data field, contains the crc32 of all data
 *
 * ans:
 * if in progress:
 * 		[0]    expected data type (!= 0xff)
 * 		[1,2]  expected chunk number
 *
 *    the length of the chunk can be derived from length of the frame data as
 *    chunk size=len-HPROT_DATA_TRANSFER_OVERHEAD;
 *
 * 		or
 * 		nack if unsupported
 *
 * else
 * 		in case of the last frame (rx as data type=0xFF)
 * 		ack/nack
 *
 */
#define HPROT_CMD_DATA_TRANSFER						0x09
//=============================================================================
/*
 * offset where the user commands begin
 */
#define HPROT_CMD_USRBASE        					0x20		// 32

#endif /* HPROT5_SRC_PROT_HPROT_STDCMD_H_ */
