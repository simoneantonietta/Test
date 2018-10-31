/**----------------------------------------------------------------------------
 * PROJECT: hpsnif
 * PURPOSE:
 * 
 *-----------------------------------------------------------------------------  
 * CREATION: 28 Jan 2016
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

#ifndef COMM_HANDLEFRAME_H_
#define COMM_HANDLEFRAME_H_

#include <string>
#include <map>
#include <stdint.h>
#include <queue>
#include "common/prot6/hprot.h"
#include "global.h"
#include "comm/CommTh.h"

#define ABSOLUTE_MAX_QUEUE 					250

using namespace std;

class RXHFrame
{
public:
	typedef struct qdata_st
	{
		char dir;
		uint8_t size;
		uint8_t data[HPROT_BUFFER_SIZE];
		int fr_error;
	} qdata_t;
	queue<qdata_t> qframes;

	RXHFrame();
	virtual ~RXHFrame();

//	void setCommInterface();

	void setPrintParam(bool enableParse,bool parse_stdcmd,bool timestamp_f,bool hideCrc_f,int dumpTrigger)
	{
	this->parse_stdcmd_f=parse_stdcmd;
	this->parseEnable=enableParse;
	this->dumpTrigger=dumpTrigger;
	this->timestamp_f=timestamp_f;
	this->hideCrc_f=hideCrc_f;

	getTimestampMS(true);
	this->penable=false;
	}

	void setPrintEnable(bool en) {this->penable=en;};
	bool getPrintEnable() {return this->penable;};
	void setfilterEnable(bool en) {this->filterEnable=en;}
	bool getfilterEnable() {return this->filterEnable;}

	void putQueue(char dir,uint8_t *buff, int size, int fr_error=pc_idle);
	void getQueue(qdata_t *q);
	size_t depthQueue() {return qframes.size();}
	bool isEmptyQueue() {return qframes.empty();}
	void clearQueue() {while(!qframes.empty()) qframes.pop();}

	void printFrame(char dir,uint8_t *buff, int size, int fr_error);

	bool loadDbgMessageMap(string fname);
	bool loadDbgVariableMap(string fname);

	void setLogFilename(string fname) {this->logFilename=fname;}
	void setEnableLog(bool en) {this->enableLog=en;}
	bool logToFile(uint8_t *buff, int size, bool txt=false);
	void setShowDbgOnly(bool en) {this->onlyDebugMessages=en;}

private:
	CommTh *comm;

	bool penable;
	bool parseEnable;
	bool parse_stdcmd_f;
	bool timestamp_f;
	bool hideCrc_f;
	bool filterEnable;
	bool enableLog;
	bool onlyDebugMessages;

	int dumpTrigger;
	char str[10000];
	char strVar[10000];
	string logFilename;

	typedef union value_u
	{
		int8_t sbValue;
		uint8_t ubValue;
		int16_t swValue;
		uint16_t uwValue;
		int32_t siValue;
		uint32_t uiValue;
		float fValue;
		double dValue;
	} value_t;

	map<int,string> messageMap;
	map<int,string> variableMap;

	string getTimestamp();
	long long milliseconds_init;
	string getTimestampMS(bool initFlag=false);
	string parseDbgVarFrame(uint8_t *databuf, int size);
};

//-----------------------------------------------
#endif /* COMM_HANDLEFRAME_H_ */
