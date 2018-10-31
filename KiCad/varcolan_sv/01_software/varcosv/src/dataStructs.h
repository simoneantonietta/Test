/**----------------------------------------------------------------------------
 * PROJECT: varcosv
 * PURPOSE:
 * 
 *-----------------------------------------------------------------------------  
 * CREATION: 8 Mar 2016
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

#ifndef DATASTRUCTS_H_
#define DATASTRUCTS_H_

#include <stdint.h>
#include <time.h>

#if ((defined TERMINAL) || (defined DISPLAY))
#include <stdbool.h>
#include "prot6/hprot.h"
#else
#ifdef SAETCOM
// this to avoid issue on compiling
#define HPROT_CRYPT_KEY_SIZE 			4
#else
#include "common/prot6/hprot.h"
#endif
#endif

/**
 * ---------------------------------------------------------------------------------------------------------------------------
 *
 * Global system define(s)
 *
 * ---------------------------------------------------------------------------------------------------------------------------
 */
#define MAX_WEEKTIMES			64
#define MAX_PROFILES			250
#define MAX_TERMINALS			64
#define MAX_BADGE	 				65536
#define MAX_CAUSAL_CODES	10

#define DEVICE_TYPE_NONE						0
#define DEVICE_TYPE_SUPERVISOR			10
#define DEVICE_TYPE_VARCOLAN				11
#define DEVICE_TYPE_SUPPLY_MONITOR	12
#define DEVICE_TYPE_DISPLAY					13
#define DEVICE_TYPE_BL_VARCOLAN			14

#define ANSWER_TIME_EACH_DEVICE_MS 	10

#define FW_CHUNK_SIZE 128 /* byte */

#define APPLICATION_SIZE_MAX (0x00077FFF - 0x00018000)//(1024 * 32 * 12)

#define MAX_NUMER_OF_EVENTS_INTO_FRAME 10

#define REPORT_N_EVENTS_MAX			250

//...............................
// system addresses
//...............................
#define DEV_BASEID_TEST								65
#define DEV_BASEID_GPSNIF							66
#define DEV_BASEID_SPEC_BOARD					67

//...............................
// base addresses structure
//...............................
// max devices
#define DEV_MAX_N_VARCOLAN						64
#define DEV_MAX_N_SUPERVISOR					1
#define DEV_MAX_N_SUPPLY_MONITOR			20
#define DEV_MAX_N_DISPLAY							64
#define DEV_MAX_N_SVM									64979

#define DEV_BASEID_VARCOLAN						1
#define DEV_MAXID_VARCOLAN						(DEV_BASEID_VARCOLAN+DEV_MAX_N_VARCOLAN-1)

#define DEV_BASEID_SUPERVISOR					69
#define DEV_MAXID_SUPERVISOR					(DEV_BASEID_SUPERVISOR+DEV_MAX_N_SUPERVISOR-1)

#define DEV_BASEID_SUPPLY_MONITOR			70
#define DEV_MAXID_SUPPLY_MONITOR			(DEV_BASEID_SUPPLY_MONITOR+DEV_MAX_N_SUPPLY_MONITOR-1)

#define DEV_BASEID_DISPLAY						90
#define DEV_MAXID_DISPLAY							(DEV_BASEID_DISPLAY+DEV_MAX_N_DISPLAY-1)

#define DEV_BASEID_SVM								300
#define DEV_MAXID_SVM									(DEV_BASEID_SVM+DEV_MAX_N_SVM-1)
/**
 * ---------------------------------------------------------------------------------------------------------------------------
 *
 * Badge struct(s)
 *
 * ---------------------------------------------------------------------------------------------------------------------------
 */

#define BADGE_SIZE 10
#define ACTIVE_PROFILE_ARRAY_MAX 9
#define NULL_LINK 0xffff

#define BADGE_DEFAULT_PROFILE_ID 			0
#define BADGE_DEFAULT_PIN 						0
#define BADGE_DEFAULT_STATUS	 				badge_enabled
#define BADGE_DEFAULT_START_VALIDITY	0
#define BADGE_DEFAULT_STOP_VALIDITY		0
#define BADGE_DEFAULT_LINK						NULL_LINK


typedef enum
{
	badge_disabled = 0,
	badge_enabled = 1,
	badge_deleted = 2,
} Badge_Status_t; /* Status of badge */

typedef union
{
	struct
	{
		uint8_t low :4;
		uint8_t high :4;
	} part;
	uint8_t full;
} Nibble_t; /* UID code of badge */

typedef enum
{
	base = 0,
	installer = 1,
	admin = 2,
} User_type_e; /* Type of user who uses specific badge */
/**
 * Badge Parameters
 */
typedef struct
{
	uint8_t crc8; /* crc of Parameters_s struct */
	uint16_t link; /* Link to next badge in memory management */
	struct
	{
		Nibble_t badge[BADGE_SIZE]; /* UID of badge */
		struct
		{
			Badge_Status_t badge_status :2;
			User_type_e user_type :2;
			uint32_t visitor :1;
			uint32_t pin :17; /* 5 Digit -> 17 bit are enough*/
		}__attribute__ ((packed));
		uint16_t ID;
		uint8_t profilesID[ACTIVE_PROFILE_ARRAY_MAX];
		union
		{
			struct
			{
				uint16_t start_year :7;
				uint16_t start_month :4;
				uint16_t start_day :5;
			};
			uint16_t startValidity; /* Temporary start */
		};
		union
		{
			struct
			{
				uint16_t stop_year :7;
				uint16_t stop_month :4;
				uint16_t stop_day :5;
			};
			uint16_t stopValidity; /* Temporary stop */
		};
	}__attribute__ ((packed)) Parameters_s;
}__attribute__ ((packed)) Badge_parameters_t;

//-----------------------------------------------------------------------------
// Sizes
//-----------------------------------------------------------------------------
#define CRC_SIZE 1
#define MAC_SIZE 6
#define IP_SIZE 4
#define CODED_AUTH_SIZE 30

//-----------------------------------------------------------------------------
// Memory Dump Types
//-----------------------------------------------------------------------------
#define WEEKTIME_TYPE 0x01
#define PROFILE_TYPE 	0x02
#define BADGE_TYPE 		0x03
#define STOP_TYPE 		0xff

/**
 * ---------------------------------------------------------------------------------------------------------------------------
 *
 * Terminal struct(s)
 *
 * ---------------------------------------------------------------------------------------------------------------------------
 */

typedef enum
{
	access_none = 0,
	access_badge_only = 1,
	access_badge_or_id_and_pin = 2,
	access_badge_and_pin = 3,
} AccessType_t;

/* Firmware Version definition */
typedef struct
{
	uint8_t major;
	uint8_t minor;
}__attribute__ ((packed)) FWversion_t;

/* Firmware metadata definition */
typedef struct
{
	uint8_t fw_type;
	uint32_t fw_crc32;
	uint32_t fw_size;
	FWversion_t version;
}__attribute__ ((packed)) FWMetadata_t;

typedef enum
{
	no_connection = 0,
	bus_lan = 1,
	bus_RS485 = 2,
} SVBus_t;

typedef struct
{
	uint32_t networkCRCError; /* Used to count how may CRC Error occur in RS485 Network communication */
	uint32_t reservedCRCError; /* Used to count how may CRC Error occur in RS485 Reserved communication */
	uint32_t lanCRCError; /* Used to count how may CRC Error occur in LAN communication */
	uint32_t SVCommTimeout; /* Used to count how may timeout occur during SV communication */
}__attribute__ ((packed)) TerminalStats_t;

typedef enum
{
	hw_2rele = 0,
	hw_3rele = 1,
} HWVersion_e;

typedef enum
{
	bidirectional_door = 0,
	monodirectional_doors = 1,
	enabling = 2
} EntranceType_t;

typedef struct
{
	uint8_t crc8; /* crc of Parameters_s struct */
	struct
	{

		uint8_t crypto_key[HPROT_CRYPT_KEY_SIZE];
		uint32_t fw_checksum;
		uint8_t mac[MAC_SIZE];
		union
		{
			uint8_t terminal_ip[IP_SIZE];
			uint32_t terminal_ip_32;
		};
		union
		{
			uint8_t netmask_ip[IP_SIZE];
			uint32_t netmask_ip_32;
		};
		union
		{
			uint8_t gateway_ip[IP_SIZE];
			uint32_t gateway_ip_32;
		};
		union
		{
			uint8_t sv_ip[IP_SIZE];
			uint32_t sv_ip_32;
		};

		char codedAuthorisation[CODED_AUTH_SIZE];
		hprot_idtype_t terminal_id;

		/* Application Metadata */
		struct
		{
			uint8_t new_firmware_available :1;
			uint8_t dhcp :1;
			uint8_t antipassback :1;
			uint8_t isMaster :1;
			SVBus_t SVConnection :2;
			AccessType_t access_door1 :2;
		}__attribute__ ((packed));

		struct
		{
			uint8_t new_fw_update_succesfully :1;
			EntranceType_t entranceType :3;
			AccessType_t access_door2 :2;
			uint8_t area1_reader2;
			uint8_t area2_reader2;

		}__attribute__ ((packed));

		uint8_t weektimeId;
		uint8_t openDoorTime;
		uint8_t openDoorTimeout;
		uint8_t area1_reader1;
		uint8_t area2_reader1;

		union
		{
			uint8_t spare_buffer[8];
			uint64_t spare64;
		};
	}__attribute__ ((packed)) Parameters_s;
}__attribute__ ((packed)) Terminal_parameters_t;

/**
 * ---------------------------------------------------------------------------------------------------------------------------
 *
 * Terminal (to send) struct(s)
 *
 * ---------------------------------------------------------------------------------------------------------------------------
 */
typedef struct
{
	uint8_t terminal_id;
	struct
	{
		bool antipassback :1;
		AccessType_t access_door1 :2;
		EntranceType_t entranceType :3;
		AccessType_t access_door2 :2;
		HWVersion_e terminal_hw_ver :2;
	}__attribute__ ((packed));

	union
	{
		uint8_t terminal_ip[IP_SIZE];
		uint32_t terminal_ip_32;
	};

	uint8_t mac[MAC_SIZE];
	uint8_t weektimeId;
	uint8_t openDoorTime;
	uint8_t openDoorTimeout;
	uint8_t area1_reader1;
	uint8_t area2_reader1;
	uint8_t area1_reader2;
	uint8_t area2_reader2;
	FWversion_t terminal_fw_ver;
	FWversion_t SM_fw_ver;
	FWversion_t display_fw_ver;
	FWversion_t spare_fw_ver;
}__attribute__ ((packed)) Terminal_parameters_to_send_t;

/**
 * ---------------------------------------------------------------------------------------------------------------------------
 *
 * Causal codes struct(s)
 *
 * ---------------------------------------------------------------------------------------------------------------------------
 */
#define CAUSAL_CODES_DESCRIPTION_SIZE 20

typedef struct
{
	uint8_t crc8; /* crc of Parameters_s struct */
	struct
	{
		uint16_t causal_Id;
		char description[CAUSAL_CODES_DESCRIPTION_SIZE];
	}__attribute__ ((packed)) Parameters_s;
}__attribute__ ((packed)) CausalCodes_parameters_t;

/**
 * ---------------------------------------------------------------------------------------------------------------------------
 *
 * Profile struct(s)
 *
 * ---------------------------------------------------------------------------------------------------------------------------
 */
typedef struct
{
	uint8_t crc8; /* crc of Parameters_s struct */
	struct
	{
		uint8_t profileId;
		uint64_t activeTerminal;
		uint8_t weektimeId;
		struct
		{
			uint8_t coercion :1;
			uint8_t type :7;
		}__attribute__ ((packed));
	}__attribute__ ((packed)) Parameters_s;
}__attribute__ ((packed)) Profile_parameters_t;

#define HOUR_MAX 24
#define MINUTE_MAX 60

#define WT_ARRAY_SIZE 		(7*4*2)
typedef struct
{
	uint8_t hour;
	uint8_t minute;
} Daily_time;

typedef struct
{
	Daily_time begin;
	Daily_time end;
} Limits_t;

typedef struct
{
	Limits_t first;
	Limits_t second;
} Numbers_t;

/**
 * ---------------------------------------------------------------------------------------------------------------------------
 *
 * Weektime struct(s)
 *
 * ---------------------------------------------------------------------------------------------------------------------------
 */
typedef struct
{
	uint8_t crc8;
	struct
	{
		uint8_t weektimeId;
		union
		{
			struct
			{
				Numbers_t monday;
				Numbers_t tuesday;
				Numbers_t wednesday;
				Numbers_t thursday;
				Numbers_t friday;
				Numbers_t saturday;
				Numbers_t sunday;
			};
			uint8_t weekdays[WT_ARRAY_SIZE];
		} week;
	}__attribute__ ((packed)) Parameters_s;
}__attribute__ ((packed)) Weektime_parameters_t;

/**
 * ---------------------------------------------------------------------------------------------------------------------------
 *
 * Event struct(s)
 *
 * ---------------------------------------------------------------------------------------------------------------------------
 */
typedef enum
{
	pin_changed = 3,
	badges_deleted = 4,
	start_enabling = 5,
	stop_enabling = 6,
	transit = 7,
	checkin = 8,
	transit_with_code = 9,
	checkin_with_code = 10,
	out_of_area = 11,
	disable_in_terminal = 12,
	no_checkin_weektime = 13,
	no_valid_visitor = 14,
	coercion = 17,
	not_present_in_terminal = 18,
	terminal_not_inline = 19,
	terminal_inline = 20,
	forced_gate = 21,
	opendoor_timeout = 22,
	stop_opendoor_timeout = 23,
	alarm_aux = 24,
	stop_alarm_aux = 25,
	low_voltge = 26,
	stop_low_voltge = 27,
	start_tamper = 28,
	stop_tamper = 29,
	wrong_pin = 30,
	broken_terminal = 31,
	SM_low_battery = 32,
	SM_critical_battery = 33,
	SM_no_supply = 34,
	SM_supply_ok = 35,
	transit_button = 36,
	fw_installed = 37,
	wrong_access = 38,
	SM_start_tamper = 39,
	SM_stop_tamper = 40,
	SM_no_battery = 41,
	recovery_factory_settings = 42,
	update_profile = 43,
	update_weektime = 44,
	update_causal_codes = 45,
	update_terminal = 46,
	update_area = 47
} EventCode_t; /* Codes of event */

#define NULL_CAUSAL_CODE 0

typedef struct
{
	uint8_t crc8; /* crc of Parameters_s struct */
	struct
	{
		bool isSynced :1;
		uint16_t id;
		time_t timestamp :32;
		hprot_idtype_t terminalId;
		EventCode_t eventCode :8;
		uint16_t idBadge;
		uint8_t area;
		uint16_t causal_code;

	}__attribute__ ((packed)) Parameters_s;
}__attribute__ ((packed)) Event_t;

/* Relay(s) */
enum
{
	gate1_relay = 0,
	gate2_relay = 1,
	alarm_relay = 2
};

/**
 * ---------------------------------------------------------------------------------------------------------------------------
 *
 * Supervisor polling answer
 *
 * ---------------------------------------------------------------------------------------------------------------------------
 */
typedef enum
{
	sv_none = 0x00,
	sv_weektime_data_error = 0x01,
	sv_profile_data_error = 0x02,
	sv_badge_data = 0x03,
	sv_terminal_data_upd = 0x04,
	sv_new_time = 0x05,
	sv_fw_version_req = 0x06,
	sv_fw_new_version = 0x07,
	sv_global_deletion = 0x08,
	sv_present_terminals_req = 0x09,
	sv_drive_outputs = 0x0a,
	sv_mac_address = 0x0b,
	sv_ip_address = 0x0c,
	sv_memory_dump = 0x0d,
	sv_terminal_data_req = 0x0e,
	sv_polling_time = 0x0f,
	sv_log_deletion = 0x10,
	sv_bl_new_version = 0x11,
	sv_update_crypto_key = 0x12,
	sv_stats_req = 0x13,
	sv_stats_clear = 0x14,
	sv_active_wt = 0x15,
	sv_terminal_control = 0x16,
	sv_codes_data_error = 0x17,
	sv_spare = 0xfd,
	sv_reboot_request = 0xfe,
	sv_busy = 0xff,
} sv_DataTypeCode_t;

/**
 * ---------------------------------------------------------------------------------------------------------------------------
 *
 * Supply monitor polling answer
 *
 * ---------------------------------------------------------------------------------------------------------------------------
 */
typedef enum
{
	SM_Status_None = 0,
	SM_Status_220V_OK,
	SM_Status_220V_not_available,
	SM_Status_Low_Battery,
	SM_Status_Battery_OK,
	SM_Status_Critical_level_Battery,
	SM_Status_Battery_not_available,
	SM_Status_tamper_OK,
	SM_Status_tamper_in_alarm,
} sm_SupplyStatus_t;

/**
 * ---------------------------------------------------------------------------------------------------------------------------
 *
 * Display polling answer
 *
 * ---------------------------------------------------------------------------------------------------------------------------
 */
typedef enum
{
	dis_none = 0,
	dis_causal_req,
	dis_causal_set,
	dis_badge_param_req_from_badge,
	dis_badge_param_req_from_ID,
	dis_badge_param_set,
	dis_time_req,
	dis_time_set,
	dis_terminal_param_req,
	dis_terminal_param_set,
	dis_causal_and_badge_reader,
	dis_event_req,
	dis_first_free_id_req
} display_pollingAns_t;


/**
 * ---------------------------------------------------------------------------------------------------------------------------
 *
 * Event Request Struct
 *
 * ---------------------------------------------------------------------------------------------------------------------------
 */
typedef struct
{
	uint16_t id_badge;				/* id of badge event need */
	uint8_t start_event;			/* index of event to start - defaut=0*/
	time_t epochtime_start;		/* epochtime of start range */
	time_t epochtime_stop;		/* epochtime of end of range */
}__attribute__ ((packed)) EventRequest_t;

/* WARNING
*
*/


/**
 * SVM Polling Struct
 */

typedef struct
{
	uint8_t globProfilesCRC;
	uint8_t globWeektimesCRC;
	uint8_t globCausalsCRC;
	time_t badgeTimestamp :32;
} __attribute__ ((packed)) SVMPolling_t;

/**
 * ---------------------------------------------------------------------------------------------------------------------------
 *
 * SVC polling answer
 *
 * ---------------------------------------------------------------------------------------------------------------------------
 */
typedef enum
{
	svc_none = 0x00,
	svc_profile_data_error = 0x01,
	svc_weektime_data_error = 0x02,
	svc_causal_data_error = 0x03,
	svc_update_badge = 0x04,
} svc_pollingAns_t;

/**
 * ---------------------------------------------------------------------------------------------------------------------------
 *
 * SVC set command
 *
 * ---------------------------------------------------------------------------------------------------------------------------
 */
typedef enum
{
	svc_update_area = 0x00,
	svc_download_file = 0x01
} svc_setCommand_t;
//-----------------------------------------------
#endif /* DATASTRUCTS_H_ */
