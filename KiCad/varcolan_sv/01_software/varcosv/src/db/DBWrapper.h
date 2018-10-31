/**----------------------------------------------------------------------------
 * PROJECT:
 * PURPOSE:
 * wrapper for database (MYSQL) used as reporting system database
 *-----------------------------------------------------------------------------  
 * CREATION: 9 Feb 2015
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

#ifndef DB_DBWRAPPER_H_
#define DB_DBWRAPPER_H_

//#include <mysql++/mysql++.h>
#include "../global.h"
#include <string>
//#include <sqlite3.h>
#include "sqlite3.h"
#include "../common/utils/Trace.h"
#include "../common/utils/Utils.h"
#include "../Crc32.h"
extern "C"
{
#include "../crc.h"
}

#define WT_STRING_FORM_SIZE			175

extern struct globals_st gd;
extern Trace *dbg;

using namespace std;

class DBWrapper: public Crc32
{
public:
	typedef struct webCmd_st
	{
		unsigned long long int rowid;
		int status;
		string command;
		string result;
	} webCmd_t;
	webCmd_t webCommand;
	uint64_t weektimeActive;	// bit array that indicates if aweektime is used (name is present)

	DBWrapper(string svfname, string evfname);
	virtual ~DBWrapper();

	bool dbSave();
	bool db2Ram();

	bool open();
	void close();

	bool loadSVConfig();

	bool setProfileDefault(bool onlyMemory = false);
	bool loadProfiles();
	bool updateProfile(int ndx, string name);
	bool setProfile(int ndx, string name, Profile_parameters_t &par);

	bool setWeektimeDefault(bool onlyMemory = false);
	bool loadWeektimes();
	bool updateWeektime(int ndx, string name);
	bool setWeektime(int ndx, string name, Weektime_parameters_t &par);

	bool loadCausals();
	bool setCausalDefault();
	bool setCausal(int ndx, int cid, string desc);
	bool getCausal(int &ndx, int &cid, string &desc);

	bool setTerminalDefault(bool onlyMemory = false);
	bool loadTerminals();
	bool setTerminal(string name, int status, Terminal_parameters_to_send_t &par);
	bool cleanTerminalStatus(int status);
	bool cleanTerminalName(string name);
	bool setTerminalStatus(int terminalId, int status);
	int getTerminalStatus(int terminalId);
	bool getTerminal(int id, globals_st::terminal_t &par);
	bool removeTerminal(int id);

	void updateMostRecentBadgeTimestamp();
	int setBadge(Badge_parameters_t &par, time_t timestamp = 0, char *printedCode = NULL, char *contact = NULL);
	bool getBadgeFromTimestamp(time_t timestamp, Badge_parameters_t *par);
	int getBadgeId(uint8_t *uid);
	int getBadgeMaxId();
	bool getBadgeFromId(int id, Badge_parameters_t &par, bool debugMsg = true);
	bool getBadgeFromUID(uint8_t *uid, Badge_parameters_t &par);
	bool changeBadgePinFromId(int id, string newpin);
	uint64_t getNewBadgeId();
	void normalizeBadge(uint8_t *uid, int size, uint8_t *uid_norm, bool conv = false);

	bool setArea(int badgeID, int area);
	int getArea(int badgeID);

	//bool setUser(int badge_id,string &name1, string &name2);
	int getUserId(int badge_id);
	bool getUserName(int user_id, string &name1, string &name2);

	bool cleanTable(char which);

	bool addAdminUser(string username, string password, int role);
	bool addArea(int area_id, int current_area, string area_name);

	bool checkEventTable();
	bool addEvent(Event_t &ev);
	unsigned int getEvents(EventRequest_t *evreq, vector<Event_t> *evt);
	unsigned int getNEvents(time_t start_timestamp, vector<Event_t> *evt, int maxcount);
	unsigned int fillEventsData(vector<Event_t> *evt);

	bool getUnsyncedEvent(uint64_t &ev_rowid, Event_t &ev);
	bool setEventSyncStatus(uint64_t rowid, bool syncStatus);
	void updateUnsyncedEvents();

	time_t datetime2epoch(string dt);
	string epoch2datetime(time_t ep);

	string getHistoryDateLimits(char latest_earlier);

	bool getCommand();
	bool setResult(string res);
	void cleanCommands();



private:
	uint32_t lastBadgeID;
	uint32_t dbEventCRC32;

	string svfname;
	string evfname;
	sqlite3 *svdb;
	sqlite3 *evdb;
	vector<string> querydata;
	vector<string> toks;


	void updateProfilesCRC(void);
	void updateWeektimesCRC(void);
	void updateCausalCRC(void);

	void weektime2str(int ndx, char *data);
	bool exeSQL(sqlite3 *db, string sql);

	void fillProfileData();
	void fillWeektimeData();
	void fillCausalData();
	bool fillBadgeData(Badge_parameters_t &par);

	void activeTerminal2string(uint64_t &at, string &str);
	void string2activeTerminal(string &str, uint64_t &at);



	//-----------------------------------------------------------------------------
	// callbacks
	//-----------------------------------------------------------------------------

	/**
	 * called by sql.
	 * This creates a comma separated ';' string with fields and = to the value
	 * @param userdata
	 * @param argc
	 * @param argv
	 * @param azColName
	 * @return
	 */
	static int callback_vec_string(void *userdata, int argc, char **argv, char **azColName)
	{
	int i;
	vector<string> *vstr = (vector<string>*) userdata;
	string str;
	str = "";
	for(i = 0;i < argc;i++)
		{
		str += azColName[i] ? azColName[i] : "NULL";
		str += "=";
		if(argv[i] == NULL)
			str += "NULL";
		else
			{
			str += argv[i][0] ? argv[i] : "NULL";
			}
		str += ";";
		}
	vstr->push_back(str);
	return 0;
	}

	/**
	 * called by sql to retrieve only 1 string (no column returns)
	 * @param userdata result string
	 * @param argc
	 * @param argv
	 * @param azColName
	 * @return
	 */
	static int callback_res_monostring(void *userdata, int argc, char **argv, char **azColName)
	{
	int i;
	string *str = (string*) userdata;

	*str = "";
	for(i = 0;i < argc;i++)
		{
		if(argv[i] == NULL)
			*str += "NULL";
		else
			{
			*str += argv[i][0] ? argv[i] : "NULL";
			}

		/* Separator char */
		if(i < argc) *str += ";";
		}
	return 0;
	}

	/**
	 * called by sql
	 * @param userdata
	 * @param argc
	 * @param argv
	 * @param azColName
	 * @return
	 */
	static int callback(void *userdata, int argc, char **argv, char **azColName)
	{
	int i;
	//----------------
	// debug
	char tmp[200], s[2000];
	s[0] = 0;
	for(i = 0;i < argc;i++)
		{
		sprintf(tmp, "%s = %s;", azColName[i], argv[i] ? argv[i] : "NULL");
		strcat(s, tmp);
		}
	dbg->trace(DBG_NOTIFY, "%s = %s", azColName[i], argv[i] ? argv[i] : "NULL");
	//----------------
	return 0;
	}
};

//-----------------------------------------------
#endif /* DB_DBWRAPPER_H_ */
