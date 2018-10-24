/**----------------------------------------------------------------------------
 * PROJECT: protest
 * PURPOSE:
 * 
 *-----------------------------------------------------------------------------  
 * CREATION: 4 Feb 2015
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

#ifndef GLOBAL_H_
#define GLOBAL_H_

#include <string.h>
#include <string>
//#include <stdint.h>

//---------------------------------------------------------
/*
 * comment this for production
 */
//#define ACQ_SIMUL
//---------------------------------------------------------

#define APP_NAME						"ACQSrv_test"
#define MYID								1

#define SPECBRD_ID					2

#define MAX_MESSAGE_TRACES 	20
#define DBG_ERROR 					TRACECLASS_ERROR
#define DBG_WARNING					TRACECLASS_WARN
#define DBG_FATAL 					TRACECLASS_FATAL
#define DBG_NOTIFY 					TRACECLASS_NOTIFY
#define DBG_DEBUG						TRACECLASS_DEBUG
#define DBG_SKTCLIENT				TRACECLASS_CUSTOM_MIN+0
#define DBG_SKTCLIENT_DATA	TRACECLASS_CUSTOM_MIN+1
#define DBG_SKTSERVER				TRACECLASS_CUSTOM_MIN+2
#define DBG_SKTSERVER_DATA	TRACECLASS_CUSTOM_MIN+3
#define DBG_ACQ							TRACECLASS_CUSTOM_MIN+4

#define SERIAL_PORT					"/dev/ttySpecBrd"
#define BAUDRATE						115200
#define SERVER_PORT					5000


//-----------------------------------------------------------------------------
// MACROS
//-----------------------------------------------------------------------------
#define STATIC_SIMPLE_CALLBACK(classname,name) 	static void s_##name(void* data) {((classname*)data)->name(data);} \
																								void name(void* data);

#define STATIC_SIMPLE_CALLBACK_NAME(name) 			s_##name
//=============================================================================

/*
 * generic result class
 */
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

	void reset() {status=is_unknown;value=0;message[0]=0;}
	void setStatus(status_e st) {status=st;}
	status_e getStatus() {return status;}

	void setError(int val=0) {status=is_error;value=val;}
	void setError(const char* msg,int val=0)
		{
		status=is_error;
		strcpy(message,msg);
		value=val;
		}
	void setOK(const char *msg,int val=0)
		{
		status=is_ok;
		strcpy(message,msg);
		value=val;
		}
	void setOK(int val=0) {status=is_ok;value=val;}
	void setWaiting(int val=0) {status=is_waiting;value=val;}

	bool isError() {return (status==is_error) ? (true) : (false);}
	bool isOK() {return (status==is_ok) ? (true) : (false);}
	bool isUnknown() {return (status==is_unknown) ? (true) : (false);}
	bool isWaiting() {return (status==is_waiting) ? (true) : (false);}
};


//-----------------------------------------------
#endif /* GLOBAL_H_ */
