/**----------------------------------------------------------------------------
 * PROJECT: acqsrv
 * PURPOSE:
 * 
 *-----------------------------------------------------------------------------  
 * CREATION: 27 Feb 2015
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

#ifndef PROTCOMMANDS_H_
#define PROTCOMMANDS_H_

/*
 * application specific commands
 */

/*
 * set a servo position
 * [0] servo
 * [1] position [0..100]
 */
#define HPROT_CMD_SETSERVO				HPROT_CMD_USRBASE+0			// 0x20

/*
 * set a channel output
 * [0] channel
 * [1..] acq_t
 */
#define HPROT_CMD_SETACQCH				HPROT_CMD_USRBASE+1			// 0x21

/*
 * get a channel
 * [0] channel
 * and as answer:
 * [1..] acq_t
 * ans:
 * ACK
 */
#define HPROT_CMD_GETACQCH				HPROT_CMD_USRBASE+2			// 0x22

/*
 * program the device
 * [0] erase mode 'a' all memory, 'm' only main memory
 * [1..] filename
 * ans:
 * ACK, NACK
 */
#define HPROT_CMD_PGMDEV					HPROT_CMD_USRBASE+3			// 0x23

/*
 * change UID
 * [0..3] old uid
 * [4..7] new uid
 * ans:
 * ACK, NACK
 */
#define HPROT_CMD_NEWID						HPROT_CMD_USRBASE+4			// 0x24

/*
 * start calibration
 * [0..3] uid
 * ans:
 * [0..3] (int) freq in kHz
 * [4..7] (int) delta in kHz
 */
#define HPROT_CMD_CALIB						HPROT_CMD_USRBASE+5			// 0x25

/*
 * sensibility test
 * [0..3] uid
 * [4] threshold
 * ans:
 * RSSI value measured
 */
#define HPROT_CMD_SENS						HPROT_CMD_USRBASE+6			// 0x26

/*
 * set the device in test/run mode
 * [0] 0: calibration; 1: sensitivity
 * [1] 1: test mode on; 0: end test mode
 * ans:
 * ACK, NACK
 */
#define HPROT_CMD_DEVMODE					HPROT_CMD_USRBASE+7			// 0x27

/*
 * get the device status (alarms etc.)
 * [0..3] uid
 * ans:
 * [0] status
 * [1] vbat
 */
#define HPROT_CMD_DEVSTATUS				HPROT_CMD_USRBASE+8			// 0x28

/*
 * set device ID
 * [0..3] codeIDDefaultExp expanded 4 bytes
 * [4..7] codeID 4 chars (the label UID)
 * [8..11] codeIDexp expanded 4 bytes
 * ans:
 * ACK, NACK
 */
#define HPROT_CMD_SETCODEID				HPROT_CMD_USRBASE+9			// 0x29

/*
 * set device parameters
 * [0] parameters set (0 is always the default one)
 * ans:
 * ACK, NACK
 */
#define HPROT_CMD_SETDEVPARAMS		HPROT_CMD_USRBASE+10	// 0x2A

/*
 * request the version
 * ans:
 * string that represents the version
 * NACK
 */
#define HPROT_CMD_GETVERSION			HPROT_CMD_USRBASE+11	// 0x2B

/*
 * run the current profile check
 * [0] minimum current to identify a transmission [mA]
 * [1] maximum current allowed [mA]
 * ans:
 * [0] current max value [mA]
 */
#define HPROT_CMD_CURRPROFILE			HPROT_CMD_USRBASE+12	// 0x2C

/*
 * set the DUT type
 * [0] 0=Saet;1=SG
 * ans:
 * ACK, NACK
 */
#define HPROT_CMD_SETDUTTYPE			HPROT_CMD_USRBASE+13	// 0x2D

//-----------------------------------------------
#endif /* PROTCOMMANDS_H_ */
