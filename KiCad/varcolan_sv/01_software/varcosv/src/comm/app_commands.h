/**----------------------------------------------------------------------------

 * PROJECT: varcosv
 * PURPOSE:
 * 
 *-----------------------------------------------------------------------------  
 * CREATION: 4 Mar 2016
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

#ifndef COMM_APP_COMMANDS_H_
#define COMM_APP_COMMANDS_H_

#if ((defined TERMINAL) || (defined DISPLAY))

#include "prot6/hprot.h"
#include "prot6/hprot_stdcmd.h"

#else

#include "../common/prot6/hprot.h"
#include "../common/prot6/hprot_stdcmd.h"

#endif /* TERMINAL */
//-----------------------------------------------------------------------------
// COMMANDS
//-----------------------------------------------------------------------------
// base value is 0x20 (32)
//-----------------------------------------------------------------------------
// MASTER/SLAVE COMMANDS
//-----------------------------------------------------------------------------

/*
 * Request to check Full profile and weektime CRC
 * [0] device ID from which a device is enabled to answer if need
 * [1]: Profile CRC8
 * [2]: Weektime CRC8
 * [3]: Codes CRC8
 * [3]:Firmware major
 * [4]:Firmware minor
 * value min 0, max 0xffff
 * ans:
 * ACK/NACK
 */
#define HPROT_CMD_CHECK_CRC_P_W							HPROT_CMD_USRBASE+1		// 0x21

/*
 * Command send by slave after getting HPROT_CMD_CHECK_CRC_P_W in case Full weektimes CRC is not synced
 * No Data needed
 * ans:
 * ACK/NACK
 */
#define HPROT_CMD_WEEKTIME_ERROR						HPROT_CMD_USRBASE+2		// 0x22

/*
 * Command send by slave after getting HPROT_CMD_CHECK_CRC_P_W in case Full profiles CRC is not synced
 * No Data needed
 * ans:
 * ACK/NACK
 */
#define HPROT_CMD_PROFILE_ERROR							HPROT_CMD_USRBASE+3		// 0x23

/*
 * Command send by slave after getting HPROT_CMD_CHECK_CRC_P_W in case has one (or more)
 * event to be synced
 * [0..sizeof(Event)]: event to be stored
 * ans:
 * ACK/NACK
 */
#define HPROT_CMD_EVENT_TO_SYNC							HPROT_CMD_USRBASE+4		// 0x24

/*
 * Command send by slave after getting HPROT_CMD_CHECK_CRC_P_W in case
 * it needs to request data of specific badge
 * [0 ... 9] 20 nibble of badge
 * ans:
 * ACK/NACK
 */
#define HPROT_CMD_BADGE_REQUEST							HPROT_CMD_USRBASE+5		// 0x25

/*
 * Request to check each CRC of each profile
 * Total amount of profiles is too big and need to be divided in order to check each profile.
 * First byte of frame is stating profile ID and second byte is amount of next CRCs
 * [0]: first profile ID
 * [1]: amount of profiles
 * [2 - n]: list of 8 bit profile CRCs
 * ans:
 * [0]: true if there is a unsync profile, false if all CRCs are synced
 * [1]: ID of unsynced profile or DON'T CARE
 */
#define HPROT_CMD_PROFILE_CRC_LIST					HPROT_CMD_USRBASE+6 	// 0x26

/*
 * Command to force update of single Profile
 * [0 - sizeof(Profile)] whole profile structure to be update
 * ans:
 * ACK/NACK
 */
#define HPROT_CMD_UPDATE_PROFILE 						HPROT_CMD_USRBASE+7		// 0x27

/*
 * Request to check each CRC of each weektime inside frame
 * [0 - 64]: list of 8 bit weektime CRCs
 * ans:
 * [0]: true if there is a unsync weektime, false if all CRCs are synced
 * [1]: ID of unsynced weektime or DON'T CARE
 */
#define HPROT_CMD_WEEKTIME_CRC_LIST					HPROT_CMD_USRBASE+8 	// 0x28

/*
 * Command to force update of single Weektime
 * [0 - sizeof(Weektime)] whole weektime structure to be update
 * ans:
 * ACK/NACK
 */
#define HPROT_CMD_UPDATE_WEEKTIME 					HPROT_CMD_USRBASE+9		// 0x29

/*
 * Command to force update of terminal
 * [0 - sizeof(Terminal)] whole terminal structure to be update
 * ans:
 * ACK/NACK
 */
#define HPROT_CMD_UPDATE_TERMINAL						HPROT_CMD_USRBASE+10		// 0x2a

/*
 * Command to force update of time
 * [0 - sizeof(TIme)] whole time structure to be update
 * ans:
 * ACK/NACK
 */
#define HPROT_CMD_UPDATE_TIME									HPROT_CMD_USRBASE+11		// 0x2b

/*
 * Command to force a global deletion of all badge
 * ans:
 * ACK/NACK
 */
#define HPROT_CMD_GLOBAL_DELETION							HPROT_CMD_USRBASE+12		// 0x2c

/*
 * Request of inputs status from Master to Slave
 * ans:
 * [0]: 0 = tamper in alarm, 1 = tamper not in alarm
 * [1]: 0 = button 1 released, 1 = button 1 pressed
 * [2]: 0 = button 2 released, 1 = button 2 pressed
 * [3]:	0 = door 1 closed, 1 = door 1 open
 * [4]: 0 = door 2 closed, 1 = door 2 open
 */
#define HPROT_CMD_INPUT_STATUS_REQUEST				HPROT_CMD_USRBASE+13		// 0x2d

/*
 * Command to force update of single badge
 * [0 - sizeof(Badge)] whole badge structure to be update
 * ans:
 * ACK/NACK
 */
#define HPROT_CMD_UPDATE_BADGE								HPROT_CMD_USRBASE+14		// 0x2e

/*
 * Command used to request current area of specific badge from Slave to Master
 * [0 - sizeof(ID)] id of requested badge
 * ans:
 * ACK: request forwarded to Supervisor
 * NACK: Supervisor not connected
 * [1]: current area
 */
#define HPROT_CMD_REQUEST_CURRENT_AREA				HPROT_CMD_USRBASE+15		// 0x2f

/*
 * Command used to send current area of specific badge from Master to Slave
 * [0 .. 1]: id of requested badge
 * [2]: current area
 * ans:
 * ACK/NACK
 */
#define HPROT_CMD_CURRENT_AREA								HPROT_CMD_USRBASE+16		// 0x30

/*
 * Command send by slave after getting HPROT_CMD_CHECK_CRC_P_W in case FW Version is not synced
 * No Data needed
 * ans:
 * ACK/NACK
 */
#define HPROT_CMD_FW_VERSION_ERROR						HPROT_CMD_USRBASE+17		// 0x31

/*
 * Command send by master in order to reboot all slaves in bootloader mode
 * No Data needed
 * ans:
 * ACK/NACK
 */
#define HPROT_CMD_NEW_FW_AVAILABLE						HPROT_CMD_USRBASE+18		// 0x32

/*
 * Command used to send firmware metadata
 * [0 .. sizeof(Metadata)]
 * ans:
 * ACK/NACK
 */
#define HPROT_CMD_METADATA_FW									HPROT_CMD_USRBASE+19 	// 0x33

/*
 * Send in broadcast a firmware chunk from Master to Slave
 * [0] type (65=varcolan;100=supervisor;101=supply monitor;102=display)
 * [1..4] start point byte
 * [5] amount
 * [6..amount] firmware bytes
 * ans:
 * No Answer
 */
#define HPROT_CMD_FW_CHUNK										HPROT_CMD_USRBASE+20 	// 0x34

/*
 * Request from Master to Slave to check whole new firmware was got successfully
 * ans:
 * ACK: Got firmware successfully
 * NACK: Got some error
 */
#define HPROT_CMD_FW_CHECK										HPROT_CMD_USRBASE+21 	// 0x35

/*
 * Master dives specific slave's outputs
 * [0] 0 = gate 1 relay
 * 		 1 = gate 2 relay
 * 		 2 = alarm relay
 * 		 3 = led 1 OK
 * 		 4 = led 2 OK
 * 		 5 = led 1 ATT
 * 		 6 = led 2 ATT
 * [1] 0 = OFF
 * 		 1 = ON
 * ans:
 * ACK/NACK
 */
#define HPROT_CMD_DRIVE_OUTPUT										HPROT_CMD_USRBASE+22 	// 0x36

/*
 * Master send to specific slave MAC Address
 * [0..5] Mac Address in hex
 * ans:
 * ACK/NACK
 */
#define HPROT_CMD_MAC_ADDRESS										HPROT_CMD_USRBASE+23 	// 0x37

/*
 * Master send to specific slave IP Address
 * [0..3] IP Address in hex
 * ans:
 * ACK/NACK
 */
#define HPROT_CMD_IP_ADDRESS										HPROT_CMD_USRBASE+24 	// 0x38

/*
 * Command used to request terminal data from Master to specific slave
 * ans:
 * [0 .. sizeof(TerminalData)]
 */
#define HPROT_CMD_REQUEST_TERMINAL_DATA				HPROT_CMD_USRBASE+25		// 0x39

/*
 * Command to force a deletion of all events
 * ans:
 * ACK/NACK
 */
#define HPROT_CMD_LOG_DELETION								HPROT_CMD_USRBASE+26		// 0x3a

/*
 * Command to send to all slaves new crypto key
 * [0]: new crypto key 0
 * [1]: new crypto key 1
 * [2]: new crypto key 2
 * [3]: new crypto key 3
 * ans:
 * ACK/NACK
 */
#define HPROT_CMD_NEW_CRYPTO_KEY								HPROT_CMD_USRBASE+27		// 0x3b

/*
 * Command to force a reboot of all slaves
 * ans:
 * ACK/NACK
 */
#define HPROT_CMD_FORCE_REBOOT								HPROT_CMD_USRBASE+28		// 0x3c

/*
 * Command used from master to notify slave that required badge is used to
 * be stored into database. No log needed
 * ans:
 * ACK/NACK
 */
#define HPROT_CMD_BADGE_NEED_TO_BE_STORED				HPROT_CMD_USRBASE+29		// 0x3d

/*
 * Command used from master to send active weektime come from supervisor
 * [0 .. 7] 64 bit of active weektime. 0=not active, 1=active. bit 0=id 0
 * ans:
 * ACK/NACK
 */
#define HPROT_CMD_ACTIVE_WEEKTIME_LIST						HPROT_CMD_USRBASE+30		// 0x3e

/*
 * Command used from master to active or de-active terminal control
 * [0]: status of control. 0=OFF, 1=ON
 * ans:
 * ACK/NACK
 */
#define HPROT_CMD_TERMINAL_CONTROL						HPROT_CMD_USRBASE+31		// 0x3f

/*
 * Slave send to Master updated pin of specific badge
 * [0]: ID of badge
 * [1..4]: new pin
 * ans:
 * ACK/NACK
 */
#define HPROT_CMD_UPDATE_PIN										HPROT_CMD_USRBASE+32			// 0x40

/*
 * Command send by slave after getting HPROT_CMD_CHECK_CRC_P_W in case Codes CRC is not synced
 * No Data needed
 * ans:
 * ACK/NACK
 */
#define HPROT_CMD_CAUSAL_CODES_ERROR										HPROT_CMD_USRBASE+33		// 0x41

/*
 * Command to force update of codes
 * [0 - (sizeof(Codes) * MAX_CAUSAL_CODES)] codes structure of whole codes
 * ans:
 * ACK/NACK
 */
#define HPROT_CMD_UPDATE_CAUSAL_CODES 									HPROT_CMD_USRBASE+34		// 0x42

/*
 * Command send by slave to request event list to Master
 *	[0..(sizeof(EventRequest_t)] structure of request
 * ans:
 * ACK/NACK
 */
#define HPROT_CMD_EVENT_LIST_REQUEST										HPROT_CMD_USRBASE+35		// 0x43


/*
 * Command send by master to answer to event list request
 * [0..1] id of badge
 * [2] total amount of found events
 * [3] start index of total amount
 * [4]: number of event in this frame
 * [5..n*sizeof(event)] n events, specified in [4
 *
 * Warning
 * if "total amount of found events" is > REPORT_N_EVENTS_MAX means there are 250+ events
 *
 * amount of event cannot be greater than MAX_NUMER_OF_EVENTS_INTO_FRAME
 * ans:
 * ACK/NACK
 */
#define HPROT_CMD_EVENT_LIST														HPROT_CMD_USRBASE+36		// 0x44

//-----------------------------------------------------------------------------
// SUPERVISOR COMMANDS
//-----------------------------------------------------------------------------
#define HPROT_SUPERVISOR_CMD_BASE							(HPROT_CMD_USRBASE+48)	// 0x50
/*
 * (for Supervisor)
 * Request to check Full profile and weektime CRC
 * [0]: Profile CRC8
 * [1]: Weektime CRC8
 * [2]: Causal codes CRC8
 * ans:
 * ACK if aligned
 * [0] 01=weektime data error;
 *     02=profile data error;
 *     03=update badge;
 *     04=update terminal data;
 *     05=new time;
 *     06=request FW version (used to update the fw);
 *     07=force new firmware;
 *     08=global deletion:
 *     09=request present terminals (after bus scan)
 *     10=drive outputs
 *     11=mac address
 *     12=ip address
 *     13=memory dump
 *     14=request terminal(s) data
 *     15=polling time
 *     16=log deletion
 *     17=force new bootloader fw
 *     18=update crypto key
 *     19=stats request
 *     20=stats clear
 *     21=active weektime
 *     22=terminal control
 *     23=causal data error
 *     253=spare
 *     254=reboot request
 *     255=busy
 * where if
 * 03: [1..sizeof(badge)] new badge data
 * 04: [1..sizeof(terminal)] new terminal data
 * 05: [1..sizeof(time)] current time
 * 10: [1] terminal ID
 * 		 [2] 0 = gate 1 relay
 * 		 		 1 = gate 2 relay
 * 		 		 2 = alarm relay
 * 		 		 3 = led 1 OK
 * 		 		 4 = led 2 OK
 * 		 		 5 = led 1 ATT
 * 		 		 6 = led 2 ATT
 * 		 		 7 = SM no supply relay
 * 		 		 8 = SM battery issue relay
 * 		 [3] 0 = OFF
 * 		 		 1 = ON
 * 11: [0..5] Mac Address in hex
 * 12: [0..3] Ip Address in hex
 * 14: [1..n] n bytes. Each byte is terminal ID required
 * 15: [1..2]: polling time expressed in millisecond (uint16)
 * 18: [1]: new crypto key 0
 * 		 [2]: new crypto key 1
 * 		 [3]: new crypto key 2
 * 		 [4]: new crypto key 3
 * 21: [1..8] 64 bit of active weektime. 0=not active, 1=active. bit 0=id 0.
 * 22: [1]: terminal id
 * 		 [2]: status of control - 0=OFF, 1=ON
 * 23: [0..(sizeof(CausalCodes) * MAX_CODES)]: Data of all codes stored
 * 253: [1] spare byte
 *
 */
#define HPROT_CMD_SV_CHECK_CRC_P_W						HPROT_SUPERVISOR_CMD_BASE+0 	//  0x50

/*
 * (for Supervisor)
 * Request to check each CRC of each weektime inside frame
 * [0 - 64]: list of 8 bit weektime CRCs
 * ans:
 * ACK if aligned
 * [0..sizeof(Weektime)]: weektime data
 */
#define HPROT_CMD_SV_WEEKTIME_CRC_LIST				HPROT_SUPERVISOR_CMD_BASE+1 	// 0x51

/*
 * (for Supervisor)
 * Request to check each CRC of each profile
 * Total amount of profiles is too big and need to be divided in order to check each profile.
 * First byte of frame is starting profile ID and second byte is amount of next CRCs
 * [0]: first profile ID
 * [1]: amount of profiles
 * [2 - n]: list of 8 bit profile CRCs
 * ans:
 * ACK if aligned
 * [0..sizeof(Profile)]: profile data
 */
#define HPROT_CMD_SV_PROFILE_CRC_LIST					HPROT_SUPERVISOR_CMD_BASE+2 	// 0x52

/*
 * (for Supervisor)
 * Command used to send new event to the supervisor
 * [0 - sizeof(event)] new event data
 * ans:
 * ACK/NACK
 */
#define HPROT_CMD_SV_NEW_EVENT								HPROT_SUPERVISOR_CMD_BASE+3 	// 0x53

/*
 * (for Supervisor)
 * Command used to request current area of specific badge to the supervisor
 * [0 - sizeof(ID)] id of requested badge
 * ans:
 * NACK: badge not present in database
 * [1]: current area
 */
#define HPROT_CMD_SV_REQUEST_CURRENT_AREA			HPROT_SUPERVISOR_CMD_BASE+4 	// 0x54

/*
 * (for Supervisor)
 * Command used to send firmware version
 * [0] type (65=varcolan;100=supervisor;101=supply monitor;102=display)
 * [1] FW_VER_MAJ
 * [2] FW_VER_MIN
 * ans:
 * ACK/NACK
 */
#define HPROT_CMD_SV_FW_VERSION								HPROT_SUPERVISOR_CMD_BASE+5 	// 0x55

/*
 * (for Supervisor)
 * Command used to request firmware metadata
 * [0] type (65=varcolan;100=supervisor;101=supply monitor;102=display)
 * ans:
 * [0..size(metadata)] firmware metadata
 * or NACK
 */
#define HPROT_CMD_SV_METADATA_FW							HPROT_SUPERVISOR_CMD_BASE+6 	// 0x56

/*
 * (for Supervisor)
 * send a firmware chunk
 * master request the chunk
 * [0] type (65=varcolan;100=supervisor;101=supply monitor;102=display)
 * [1..4] start point byte
 * [5] amount
 * ans:
 * [0..3] start point byte
 * [4..amount] firmware bytes
 * or nack
 */
#define HPROT_CMD_SV_FW_CHUNK_REQ							HPROT_SUPERVISOR_CMD_BASE+7 	// 0x57

/*
 * (for Supervisor)
 * master needs to request data of specific badge
 * [0]: id of terminal need badge data
 * [1 ... BadgeSize] number of digits of badge
 * ans:
 * badge data or NACK
 */
#define HPROT_CMD_SV_BADGE_REQUEST						HPROT_SUPERVISOR_CMD_BASE+8			// 0x58

/*
 * (for Supervisor)
 * master sent to supervisor list of present terminals in
 * Master-Slave bus
 * [0 .. 63] 64 byte each filled with
 * 		1 if related terminal is present
 * 		0 if related terminal is not present
 * ans:
 * ACK/NACK
 */
#define HPROT_CMD_SV_PRESENT_TERMINALS					HPROT_SUPERVISOR_CMD_BASE+9			// 0x59

/*
 * (for Supervisor)
 * master sent to supervisor dump of memory element by element
 * [0]: 01=weektime data;
 *     	02=profile data;
 *     	03=badge data;
 *     	0xff=stop
 * [1..sizeof(data)]: data of element
 * ans:
 * ACK/NACK
 */
#define HPROT_CMD_SV_MEMORY_DUMP								HPROT_SUPERVISOR_CMD_BASE+10			// 0x5a

/*
 * (for Supervisor)
 * master send to supervisor the terminal(s) data
 * [0..sizeof(terminal_0_params) .. n..sizeof(terminal_n_params ]: data of element(s)
 * ans:
 * ACK/NACK
 */
#define HPROT_CMD_SV_TERMINAL_DATA							HPROT_SUPERVISOR_CMD_BASE+11			// 0x5b

/*
 * (for Supervisor and used by the debugger)
 * send a script command line that will be executed
 * [0 ... ] command script string
 * ans:
 * [0 ... ] result from script OR
 * ACK/NACK
 */
#define HPROT_CMD_SV_SCRIPT_STRING							HPROT_SUPERVISOR_CMD_BASE+12			// 0x5c

/*
 * (for Supervisor)
 * -------------------------
 * WARNING: Protocol Violation
 * -------------------------
 * Supervisor send a forced a polling request to Master.
 * This command shall send by Supervisor without any request from master.
 * It is a protocol violation but necessary in case a new polling is required in special case
 * ans:
 * ACK/NACK
 */
#define HPROT_CMD_SV_FORCE_POLLING_REQ						HPROT_SUPERVISOR_CMD_BASE+13			// 0x5d

/*
 * (for Supervisor)
 * Master send communication statistics to SV
 * [0..sizeof(TerminalStats)]: Terminal Statistics struct
 * ans:
 * ACK/NACK
 */
#define HPROT_CMD_SV_TERMINAL_STATS								HPROT_SUPERVISOR_CMD_BASE+14			// 0x5e

/*
 * (for Supervisor)
 * Master send updated pin of specific badge
 * [0]: ID of badge
 * [1..4]: new pin
 * ans:
 * ACK/NACK
 */
#define HPROT_CMD_SV_UPDATE_PIN										HPROT_SUPERVISOR_CMD_BASE+15			// 0x5f

/*
 * (for Supervisor)
 * Master send updated causal codes
 * [0..(sizeof(CausalCodes) * MAX_CAUSAL_CODES)]: Data of all codes stored
 * ans:
 * ACK/NACK
 */
#define HPROT_CMD_SV_UPDATE_CAUSAL_CODES										HPROT_SUPERVISOR_CMD_BASE+16			// 0x60

/*
 * (for Supervisor)
 * Master send updated badge
 * [0..(sizeof(Badge_parameter)]: Data of badge parameters
 * ans:
 * ACK/NACK
 */
#define HPROT_CMD_SV_UPDATE_BADGE														HPROT_SUPERVISOR_CMD_BASE+17			// 0x61

/*
 * (for Supervisor)
 * Master send a request of event list to be displayed in Display peripheral
 *	[0..(sizeof(EventRequest_t)] structure of request
 * ans:
 * [0..1] id of badge
 * [2] total amount of found events
 * [3] start index of total amount
 * [4]: number of event in this frame
 * [5..n*sizeof(event)] n events, specified in [4]
 *
 * Warning
 * if "total amount of found events" is > REPORT_N_EVENTS_MAX means there are 250+ events
 *
 * amount of event cannot be greater than MAX_NUMER_OF_EVENTS_INTO_FRAME
 */
#define HPROT_CMD_SV_EVENT_LIST_REQUEST										HPROT_SUPERVISOR_CMD_BASE+18			// 0x62


//-----------------------------------------------------------------------------
// SUPPLY MONITOR COMMANDS
//-----------------------------------------------------------------------------
#define HPROT_SUPPLY_MONITOR_CMD_BASE							(HPROT_CMD_USRBASE+80)						// 0x70

/*
 * Terminal send polling request to Supply Monitor
 * ans:
 * [0]: Battery status
 * 	0 = No supply error
 *	1 = 220V OK
 *	2 = 220V not_available
 *	3 = Low_Battery
 *	4 = Battery_OK
 *	5 = Critical level Battery
 *	6 = Battery not available
 *	7 = Tamper OK,
 *	8 = tamper in alarm
 */
#define HPROT_CMD_SM_POLLING												HPROT_SUPPLY_MONITOR_CMD_BASE+0	// 0x70

/*
 * Terminal send command in order to get/lose control of SM relays
 * [0] 0 = control OFF
 * 		 1 = control ON
 * ans:
 * ACK/NACK
 */
#define HPROT_CMD_SM_OUTPUT_CONTROL										HPROT_SUPPLY_MONITOR_CMD_BASE+1 	// 0x71

/*
 * Terminal drives specific SM's outputs
 * [0] 0 = Output 0 (no supply relay)
 * 		 1 = Output 1	(battery issue relay)
 * [1] 0 = OFF
 * 		 1 = ON
 * ans:
 * ACK/NACK
 */
#define HPROT_CMD_SM_DRIVE_OUTPUT											HPROT_SUPPLY_MONITOR_CMD_BASE+2 	// 0x72

/*
 * Firmare version request
 * ans:
 * [0 .. sizeof(FW_version)]
 */
#define HPROT_CMD_SM_GET_FW_VERSION											HPROT_SUPPLY_MONITOR_CMD_BASE+3 	// 0x73


//-----------------------------------------------------------------------------
// DISPLAY COMMANDS
//-----------------------------------------------------------------------------
#define HPROT_DISPLAY_CMD_BASE													(HPROT_CMD_USRBASE+96)						// 0x80

/*
 * Terminal sends polling request to Display
 * ans:
 * ACK if aligned
 * [0]: (display_pollingAns_t) Polling answer
 *	00 = none
 *	01 = causal_req
 *	02 = causal_set
 *	03 = badge_param_req_from_badge
 *	04 = badge_param_req_from_ID
 *	05 = badge_param_set
 *	06 = time_req
 *	07 = time_set
 *	08 = terminal_param_req
 *	09 = terminal_param_set
 *	10 = causal_and_badge_reader
 *	11 = event_req
 *	12 = first free badge entry
 * where if
 *	02: [1..(sizeof(CausalCodes) * MAX_CAUSAL_CODES)]: Data of all codes stored
 *	03: [1..BADGE_SIZE]	badge
 *	04: [1..2]: id of specific badge
 *	05: [1..sizeof(badge_param)] parameter of badge
 *	07: [1..sizeof(time)]	epochtime
 *	08: [1..sizeof(terminal_param)]	parameter of terminal
 *	10:
 *		[1..3] causal
 *		[4..BADGE_SIZE]	badge
 *	11:
 *		[1..(sizeof(EventRequest_t)] structure of request
 *
 */
#define HPROT_CMD_DISPLAY_POLLING												HPROT_DISPLAY_CMD_BASE+0	// 0x80

/*
 * Terminal sends whole causal block
 * [0..(sizeof(CausalCodes) * MAX_CODES)]: Data of all codes stored
 * ans:
 * ACK/NACK
 */
#define HPROT_CMD_DISPLAY_CAUSAL												HPROT_DISPLAY_CMD_BASE+1	// 0x81

/*
 * Terminal sends badge parameter
 * [00..sizeof(badge_param)]
 * ans:
 * ACK/NACK
 */
#define HPROT_CMD_DISPLAY_BADGE_PARAM													HPROT_DISPLAY_CMD_BASE+2	// 0x82

/*
 * Terminal sends current time
 * [00..sizeof(time)] epochtime
 * ans:
 * ACK/NACK
 */
#define HPROT_CMD_DISPLAY_TIME													HPROT_DISPLAY_CMD_BASE+3	// 0x83

/*
 * Terminal sends its own parameter
 * [00..sizeof(terminal_param)]
 * ans:
 * ACK/NACK
 */
#define HPROT_CMD_DISPLAY_TEMINAL_PARAM													HPROT_DISPLAY_CMD_BASE+4	// 0x84

/*
 * Terminal sends event of specific id of badge
 * [0..1] id of badge
 * [2] total amount of found events
 * [3] start index of total amount
 * [4]: number of event in this frame
 * [5..n*sizeof(event)] n events, specified in [4]
 *
 * Warning
 * if "total amount of found events" is > REPORT_N_EVENTS_MAX means there are 250+ events
 * ans:
 * ACK/NACK
 *
 * amount of event cannot be greater than MAX_NUMER_OF_EVENTS_INTO_FRAME
 */
#define HPROT_CMD_DISPLAY_EVENT_LIST													HPROT_DISPLAY_CMD_BASE+5	// 0x85

/*
 * Terminal sends firmware version request
 * ans:
 * [0 .. sizeof(FW_version)]
 */
#define HPROT_CMD_DISPLAY_FW_VER_REQ															HPROT_DISPLAY_CMD_BASE+6	// 0x86

/*
 * Terminal sends badge struct with default parameter and
 * first free ID available
 * [0..sizeof(badge_param)]
 * ans:
 * ACK/NACK
 */
#define HPROT_CMD_DISPLAY_FIRST_FREE_BADGE_ENTRY									HPROT_DISPLAY_CMD_BASE+7	// 0x87

//-----------------------------------------------------------------------------
// SVC commands
//-----------------------------------------------------------------------------
#define HPROT_SVC_CMD_BASE																				(HPROT_CMD_USRBASE+176)						// 0xB0 (208)

/*
 * Polling from SVM to SVC
 * [0]: Profile CRC8
 * [1]: Weektime CRC8
 * [2]: Causal codes CRC8
 * [3..6]: most recent timestamp of badges
 * ans:
 * ACK if aligned
 * [0] 01=profile data error;
 *     02=weektime data error;
 *     03=causal data error
 *     04=update badge;
 * where if
 * 03: [1..(sizeof(CausalCodes) * MAX_CODES)]: Data of all codes stored
 * 04: [1..sizeof(badge)] new badge data
 *
 */
#define HPROT_CMD_SVC_POLLING																			HPROT_SVC_CMD_BASE+0 	//  0x50

/*
 * Request to check each CRC of each profile
 * Total amount of profiles is too big and need to be divided in order to check each profile.
 * First byte of frame is starting profile ID and second byte is amount of next CRCs
 * [0]: first profile ID
 * [1]: amount of profiles
 * [2 - n]: list of 8 bit profile CRCs
 * ans:
 * ACK if aligned
 * [0..sizeof(Profile)-1]: profile data
 * [sizeof(Profile)..MAX_NAME_SIZE] name of profile
 */
#define HPROT_CMD_SVC_PROFILE_CRC_LIST															HPROT_SVC_CMD_BASE+1 	// 0xB1

/*
 * Request to check each CRC of each weektime inside frame
 * [0 - 64]: list of 8 bit weektime CRCs
 * ans:
 * ACK if aligned
 * [0..sizeof(Weektime)-1]: weektime data
 * [sizeof(Weektime)..MAX_NAME_SIZE] name of weektime
 */
#define HPROT_CMD_SVC_WEEKTIME_CRC_LIST														HPROT_SVC_CMD_BASE+2 	// 0xB2

/*
 * Command used to send new event
 * [0 - sizeof(event)] new event data
 * ans:
 * ACK/NACK
 */
#define HPROT_CMD_SVC_NEW_EVENT																		HPROT_SVC_CMD_BASE+3 	// 0xB3

/*
 * Command used to send a request from SVC to SVM
 * [0]: request type
 * 			0 = area
 * where if
 * [0] == area
 * 			-> [1..2]: id of badge need area
 *
 * ans:
 * ACK/NACK
 * if request type == area
 * 			-> [0]: id of required area (Nack if badge not present)
 */
#define HPROT_CMD_SVC_REQ																					HPROT_SVC_CMD_BASE+4 	// 0xB4

/*
 * Command used to send a setting from SVC to SVM
 * [0]: setting type
 * 			0 = area
 * 			1 = download file
 * where if
 * [0] == area
 * 			-> [1..2]: id of badge need area
 * 				 [3]	 : new area
 * [0] == download file
 * 			-> [1..sizeof(url)] URL of file to be downloaded
 *
 * ans:
 * ACK/NACK
 */
#define HPROT_CMD_SVC_SET																					HPROT_SVC_CMD_BASE+5 	// 0xB5
//-----------------------------------------------
#endif /* COMM_APP_COMMANDS_H_ */
