#ifndef GLOBAL_H_
#define GLOBAL_H_

//-----------------------------------------------------------------------------
// C section
//-----------------------------------------------------------------------------
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <queue>
#include <map>
#include "version.h"
#include "dataStructs.h"
#include "common/prot6/hprot.h"
#include "common/utils/Trace.h"
#include "common/utils/CircularBuffer.h"
#include "common/utils/TimeFuncs.h"

//#include "macros.h"

#define APP_NAME 						"VarcoSV"

//-----------------------------------------------------------------------------
// C++ section
//-----------------------------------------------------------------------------
#if __cplusplus
//-----------------------------------------------------------------------------

#include <string>

#define USE_UDP_PROTOCOL

#define MAX_MESSAGE_TRACES 	20
#define DBG_ERROR 					TRACECLASS_ERROR
#define DBG_WARNING					TRACECLASS_WARN
#define DBG_FATAL 					TRACECLASS_FATAL
#define DBG_NOTIFY 					TRACECLASS_NOTIFY
#define DBG_DEBUG						TRACECLASS_DEBUG
#define DBG_SVM_NOTIFY			TRACECLASS_CUSTOM_MIN+0
#define DBG_SVM_ERROR				TRACECLASS_CUSTOM_MIN+1
#define DBG_SVM_WARNING			TRACECLASS_CUSTOM_MIN+2
#define DBG_SVM_FATAL				TRACECLASS_CUSTOM_MIN+4
#define DBG_SVM_DEBUG				TRACECLASS_CUSTOM_MIN+5

#define RASPBERRY_WORKING_DIR				"/data"

#define CFG_FILE						"varcosv.cfg"
#define BAUDRATE						"115200"
#define PORT								"5000"
#define MYID 								"69"

#define DB_VARCOLANSV				"varcolan.db"
#define DB_VARCOLANEV				"varcolan_events.db"
#define DB_EVENT_BKP_PATH		"./database_bkp/"
#define DB_RESTORE_PATH			"/restore_db/"
#define MAX_EVENT_BACKUP_FILES		5

#define JOB_FILE						"/data/jobs.txt"

#define DEV_TYPE_UNKNOWN					0
#define DEV_TYPE_SOCKET						1
#define DEV_TYPE_SERIAL						2

#define MAX_NAME_SIZE							32

#define MAX_CIRC_BUFF_SIZE				10
#define MAX_EVENT_TAG_PARAMS			10

#define DEF_MASTER_POLLING_TIME			500		// [ms]
#define MSG_POLLING_TIME_INCREMENT	50		// [ms]
#define MSG_MAX_POLLING_TIME				1000	// [ms]

#define DEF_ALIVE_TIME						20		// [s] (altered by cfg file)
#define DEF_TIMED_REQUEST					120		// [s]
#define DEF_TIMED_REQUEST_INIT 		5			// [s] this is the first load to perform a request fast
/*
 * firmware archive filename format:
 * type_majorminor.tar
 * i.e.: varcolan_0102.tar
 */
#define FIRMWARE_CHECK_TIME		10				// [sec]
#define FIRMWARE_NEW_PATH			"./firmware_new/"
#define FIRMWARE_UPD_PATH			"./firmware/"
#define FIRMWARE_SVUPD_PATH		"./update/"
#define FIRMWARE_UTILS_PATH		"./utils/"
#define FIRMWARE_UNPACKER			"unpkg.sh"
#define FIRMWARE_PKG_EXT			".tar"
#define FIRMWARE_SV_PKG_EXT		".tar.gz"
#define FIRMWARE_BIN_EXT			".bin"
#define FIRMWARE_TXT_EXT			".txt"
#define FIRMWARE_VARCOLAN			"varcolan"
#define FIRMWARE_BL_VARCOLAN	"bl_varcolan"
#define FIRMWARE_SUPERVISOR		"varcosv"
#define FIRMWARE_SUPPLY				"supply"
#define FIRMWARE_DISPLAY			"display"

#define SCRIPT_CHECK_TIME								15				// [sec]
#define SCRIPT_PATH											"./scripts/"
#define SCRIPT_FILENAME									SCRIPT_PATH "runme.cmd"
#define SCRIPT_STATUS_FNAME							SCRIPT_PATH "status.cmd"
#define SCRIPT_RECOVERY_FNAME						"/tmp/mnt/sv_recovery.cmd"
#define SCRIPT_RECOVERY_STATUS_FNAME		"/tmp/mnt/status_recovery.cmd"

#define TERMINAL_STATUS_NOT_PRESENT		0
#define TERMINAL_STATUS_ON_LINE				1
#define TERMINAL_STATUS_OFF_LINE			2

#define GLOBAL_ERROR_THRESHOLD				50

#define WAIT_FOR_BADGE_TIMEOUT				30	// step 500 ms

using namespace std;

/*
 * global data
 */
struct globals_st
{
	int myID;
	int defaultDest;
	int port;
	int brate;
	bool useSerial, useSocketClient, useSocketServer;
	char device[80];	// contain serial port or the server IP address
	bool connected;
	bool run;
	int loopLatency;  // [us]

	bool enableChangePollingTime;
	uint16_t masterPollingTime_def;  // time in [ms] that a master use to poll me (default value)
	uint16_t masterPollingTime;  // time in [ms] that a master use to poll me
	unsigned int masterQueueCounter;	// counts the congestion
	unsigned int masterQueueDelayCnt;  // to delay changes

	int masterAliveTime;		// in [s]

	unsigned int globalErrors;	// if this variables rech a threshold the system is reset
	unsigned int globalErrors_th;  // trheshold to reset (0 disables the check)
	bool forceReset;						// set to true if you want to reset with watchdog

	bool dumpFromMaster;
	bool dumpTriggerFromCfg;
	bool cfgChanged;

	bool enableWatchdog;
	int watchdogTime;

	// fifo for sniffing
	bool usefifo;
	string fifoName;

	bool useUdpDebug;
	string udpDbg_ip;
	int udpDbg_port;

	uint8_t hprotKey[HPROT_CRYPT_KEY_SIZE];
	bool enableCrypto;

	// telnet for interaction
	bool useTelnet;
	char telnetPort[10];
	int telnetLatency;
	string telnetPwd;
	bool telnetUsePwd;
	int cmd_fromClient;

	// badge reader
	bool has_reader;
	char reader_device[80];
	int reader_brate;

	int terminalAsReader_id;	// if =0 use only the reader, else permits the badge load from a terminal with this id

	// some functional flags
	bool badgeAutoSend;

	// db handling
	string dbRamFolder, dbPersistFolder;
	int dbSaveTime;
	int maxEventPerTable;

	int maxBadgeId;			// contains the max id value (used in autosend)

	struct badgeAcquisition_st
	{
		bool waitingForBadge;
		char badge[BADGE_SIZE * 2 + 1];
		int badge_size;
		int wait_timeout;
	} badgeAcq;

	// SVM config data
	bool useSVM;
	struct svm_st
	{
		string ip;
		int port;
		int plantID;
		int svcID;
		unsigned int keepalive_time;
	} svm;

	bool svmDebugMode;	// this flag is usefull to debug in svm mode wit the webapp working on supervisor
	//............................
	// weektimes
	typedef struct
	{
		Weektime_parameters_t params;
		char name[MAX_NAME_SIZE];
	}__attribute__((packed)) weektime_t;
	weektime_t weektimes[MAX_WEEKTIMES];
	uint8_t globWeektimesCRC;
	//............................
	// profiles
	typedef struct
	{
		Profile_parameters_t params;
		char name[MAX_NAME_SIZE];
	}__attribute__((packed)) profile_t;
	profile_t profiles[MAX_PROFILES];
	uint8_t globProfilesCRC;

	//............................
	// badge
	typedef struct
	{
	Badge_parameters_t params;
	time_t timestamp :32;
	char printedCode[16];
	char contact[30]; // reference name for visitors
	} __attribute__((packed)) badge_t;

	//............................
	// Causals
	CausalCodes_parameters_t causals[MAX_CAUSAL_CODES];
	uint8_t globCausalsCRC;
	//............................
	// Badges
	time_t mostRecentBadgeTimestamp;
	//............................
	// terminals
	typedef struct
	{
		Terminal_parameters_to_send_t params;
		char name[MAX_NAME_SIZE];
		int status;			// indicates the status of the terminal in the database
	} terminal_t;
	terminal_t terminals[MAX_TERMINALS];
	//............................
	//Events
	int unsynckedEvent;
	//............................

	uint16_t devAnsTimeout;  // time for each device to answer
//	frame_t frame;
//	uint8_t frameData[HPROT_PAYLOAD_SIZE];

	//----------------------------------------------
	// protocol message queue
	typedef struct frameData_st
	{
		enum channel_e
		{
			chan_unk,
			chan_serial,
			chan_ethernet
		} channel;
		frame_t frame;
		uint8_t frameData[HPROT_PAYLOAD_SIZE];
		int client;

		frameData_st()
		{
		channel = chan_unk;
		client = -1;
		frame.data = frameData;
		}
	} frameData_t;

	pthread_mutex_t mutexFrameQueue;
	queue<frameData_t> frameQueue;

	pthread_mutex_t mutexSVCFrameQueue;
	queue<frameData_t> SVCframeQueue;

	//----------------------------------------------
	// data events, and queue for each master connected
	typedef struct dataEvent_st
	{
		enum dataEventType_e
		{
			ev_none = sv_none,
			ev_weektime_data_error = sv_weektime_data_error,
			ev_profile_data_error = sv_profile_data_error,
			ev_causal_codes_data_error = sv_codes_data_error,
			ev_badge_data = sv_badge_data,
			ev_terminal_data_upd = sv_terminal_data_upd,
			ev_new_time = sv_new_time,
			ev_fw_version_req = sv_fw_version_req,
			ev_fw_new_version = sv_fw_new_version,
			ev_bl_new_version = sv_bl_new_version,
			ev_global_deletion = sv_global_deletion,
			ev_terminal_list = sv_present_terminals_req,
			ev_set_output = sv_drive_outputs,
			ev_mac_address = sv_mac_address,
			ev_ip_address = sv_ip_address,
			ev_memory_dump = sv_memory_dump,
			ev_terminal_data_req = sv_terminal_data_req,
			ev_polling_time = sv_polling_time,
			ev_log_deletion = sv_log_deletion,
			ev_reset_device = sv_reboot_request,
			ev_change_crypto_key = sv_update_crypto_key,
			ev_stats_req = sv_stats_req,
			ev_stats_clear = sv_stats_clear,
			ev_spare = sv_spare,
			ev_active_wktimes = sv_active_wt,
			ev_terminal_control = sv_terminal_control,

			ev_varco_event = 0x50,
			ev_all_terminal_data = 0x51,

		} dataEvent;
		/*
		 * tag data: each event can transport some simple data
		 */
#define EV_FLD_EVENT			0
#define EV_FLD_BADGE_ID		1
#define EV_FLD_AREA				2
#define EV_FLD_CODE				3
#define EV_FLD_DEVICE_ID	4

		union
		{
			int badge_id;
			int weektime_id;
			int terminal_id;
			uint16_t polling_time;
			uint8_t fw_device_type;
			uint16_t varco_event[5];	//used to generate varco events to be stored in the history database
			Terminal_parameters_to_send_t term_par;
			uint8_t params[MAX_EVENT_TAG_PARAMS];  // general parameters used i.e. for output direct drive
			time_t epochtime;
		} tag;

		dataEvent_st()
		{
		dataEvent = ev_none;
		}
	} dataEvent_t;

	//................................
	// main state machine
	enum slaveStates_e
	{
		sl_unk,
		sl_idle,
		sl_terminal_request,
		sl_newkey_send,
	};

	//................................
	// workers various states. Put here all states of all workers
	enum workerStates_e
	{
		wkr_unk,
		wkr_idle,

		wkr_nterm_init,
		wkr_nterm_req,
		wkr_nterm_wait,
		wkr_nterm_param_req,
		wkr_nterm_param_wait,
		wkr_nterm_param_end,

		wkr_newkey_init,
		wkr_newkey_send,
		wkr_newkey_done
	};
	//................................

	/*
	 * this struct hold all data for each device connected that
	 * must be in relation with a single device
	 * index is the ID of the device -1
	 */
	typedef struct dataDevice_st
	{
		bool deadSignaled;			// true if the dead is already signaled
		uint16_t aliveCounter;

		uint8_t timedReqCounter;	// when reach 0 some a request is performed (i.e.: fw version) [s]
		enum timedRequests_e
		{
			treq_none,
			treq_fwVersion,
			treq_active_wktimes,
		} timedRequest;

		TimeFuncs globTimeout, reqTimeout;
		int retry;

		bool timeout_started;
		circular_buffer<globals_st::dataEvent_t> *dataEventQueue;  // each device has its queue
		enum slaveStates_e mainState;			// main state machine
		enum workerStates_e workerState;	// worker state machine

		// application specific data
		int badgeIDindex;  // use to sync all badges
		struct terminalsData_st
		{
			uint8_t terminalsPresent[MAX_TERMINALS];
			int ndx;
			int prev_ndx;		// used to retry
		} terminalsData;

		bool newKeySet;

		/*
		 * ctor
		 */
		dataDevice_st()
		{
		timedReqCounter = DEF_TIMED_REQUEST_INIT;
		timedRequest = treq_fwVersion;
		deadSignaled = false;
		aliveCounter = DEF_ALIVE_TIME;
		timeout_started = false;
		badgeIDindex = 1;
		mainState = sl_idle;
		workerState = wkr_idle;
		memset(terminalsData.terminalsPresent, 0, MAX_TERMINALS);
		newKeySet = false;
		//dataEventQueue=new circular_buffer<globals_st::dataEvent_t>(MAX_CIRC_BUFF_SIZE);
		}
	} dataDevice_t;

	// global status, reflect the status of all masters, if equal, but if differs them contains xxx_unk status
	enum slaveStates_e glob_mainState, expected_glob_mainState;
	enum workerStates_e glob_workerState, expected_glob_workerState;

	// active master list
	map<int, dataDevice_st> deviceList;
	typedef map<int, dataDevice_t>::iterator iddevice_t;
	unsigned int deadCounter;		// cont the dead events
	unsigned int cklinkCounter;  // cont the chklinks
	//----------------------------------------------

	/**
	 * ctor of global data
	 */
	globals_st()
	{
	}
};

/*
 * generic result class
 */
#define RESULT_STRUCT
struct Result
{
public:
#define MAX_MESSAGE_SIZE		256
	enum status_e
	{
		is_unknown,
		is_waiting,
		is_ok,
		is_error
	} status;

	int value;
	//std::string message;
	char message[MAX_MESSAGE_SIZE];

	void reset()
	{
	status = is_unknown;
	value = 0;
	message[0] = 0;
	}
	void setStatus(status_e st)
	{
	status = st;
	}
	status_e getStatus()
	{
	return status;
	}

	void setError(int val = 0)
	{
	status = is_error;
	value = val;
	}
	void setError(const char* msg, int val = 0)
	{
	status = is_error;
	strcpy(message, msg);
	value = val;
	}
	void setOK(const char *msg, int val = 0)
	{
	status = is_ok;
	strcpy(message, msg);
	value = val;
	}
	void setOK(int val = 0)
	{
	status = is_ok;
	value = val;
	}
	void setWaiting(int val = 0)
	{
	status = is_waiting;
	value = val;
	}

	bool isError()
	{
	return (status == is_error) ? (true) : (false);
	}
	bool isOK()
	{
	return (status == is_ok) ? (true) : (false);
	}
	bool isUnknown()
	{
	return (status == is_unknown) ? (true) : (false);
	}
	bool isWaiting()
	{
	return (status == is_waiting) ? (true) : (false);
	}
};

//-----------------------------------------------------------------------------
// PROTOTYPES
//-----------------------------------------------------------------------------
int checkDeviceType(const char *dev);

//-----------------------------------------------------------------------------
// GLOBAL DATA
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// MACROS
//-----------------------------------------------------------------------------
#ifdef RASPBERRY
#define WATCHDOG_REFRESH() 	if(gd.enableWatchdog) \
															{ \
															if(!gd.forceReset) \
																{ \
																hwgpio->watchdogRefresh(); \
																} \
															}
#else
#define WATCHDOG_REFRESH()
#endif
//...............
#endif

//-----------------------------------------------
#endif /* GLOBAL_H_ */
