/*
 *-----------------------------------------------------------------------------
 * PROJECT: tstcont9
 * PURPOSE: see module DBWrapper.h file
 *-----------------------------------------------------------------------------
 */

#include "DBWrapper.h"
#include "../common/utils/Utils.h"
#include "../common/utils/Trace.h"
#include <stdio_ext.h>
extern "C"
{
#include "../crc.h"
}

extern struct globals_st gd;
extern Trace *dbg;

#ifdef RASPBERRY
#include "../HardwareGPIO.h"	// for watchdog
extern HardwareGPIO *hwgpio;
#endif

/**
 * ctor
 * @param svfname
 * @param evfname
 */
DBWrapper::DBWrapper(string svfname, string evfname) :
		Crc32()
{
lastBadgeID = 0;
this->svfname = svfname;
this->evfname = evfname;
weektimeActive = 0;
}

/**
 * dtor
 */
DBWrapper::~DBWrapper()
{
}

/**
 * save the database to a persistent location, but only if changed
 * @return
 */
bool DBWrapper::dbSave()
{
// check CRC32
__fpurge(stdout);
uint32_t tmp = CalculateFileCRC(gd.dbRamFolder + _S DB_VARCOLANEV);
if(tmp != dbEventCRC32)
	{
	dbg->trace(DBG_NOTIFY, "database changed -> saving it to persistent folder");
	string cmd = "cp " + gd.dbRamFolder + _S DB_VARCOLANEV + " " + gd.dbPersistFolder + " &";
	system(cmd.c_str());
	system("sync");
	dbEventCRC32 = tmp;
	}
else
	{
	dbg->trace(DBG_NOTIFY, "database unchanged -> backup skipped");
	}

//dbg->trace(DBG_NOTIFY, "saving database");
//int pid = fork() ;
//if( 0 == pid )
//     system("...") ;
////your normal code here..
return true;
}

/**
 * copy database to ram
 * @return
 */
bool DBWrapper::db2Ram()
{
string cmd;
__fpurge(stdout);
cmd = "cp " + gd.dbPersistFolder + _S DB_VARCOLANEV + " " + gd.dbRamFolder;
system(cmd.c_str());
cmd = "chmod 666 " + gd.dbRamFolder + _S DB_VARCOLANEV + "; sync";
system(cmd.c_str());

dbEventCRC32 = CalculateFileCRC(gd.dbRamFolder + _S DB_VARCOLANEV);
if(dbEventCRC32 == 0)
	{
	dbg->trace(DBG_FATAL, "database file open error, cannot calculate crc32");
	return false;
	}
return true;
}

/**
 * open the database
 * @return true: ok
 */
bool DBWrapper::open()
{
sqlite3_config(SQLITE_CONFIG_SERIALIZED);
int rc = sqlite3_open(svfname.c_str(), &svdb);
__fpurge(stdout);
if(rc)
	{
	dbg->trace(DBG_FATAL, "can't open database: %s", sqlite3_errmsg(svdb));
	sqlite3_close(svdb);
	return false;
	}
dbg->trace(DBG_NOTIFY, "varcosv database opened successfully");

rc = sqlite3_open(evfname.c_str(), &evdb);
__fpurge(stdout);
if(rc)
	{
	dbg->trace(DBG_FATAL, "can't open database: %s", sqlite3_errmsg(evdb));
	sqlite3_close(evdb);
	return false;
	}
dbg->trace(DBG_NOTIFY, "event database opened successfully");



// check if need to create tables
//char *zErrMsg = 0;
//string sql;
//
//sql="select count(type) from sqlite_master where type='table' and name='badge'";
//rc = sqlite3_exec(db, sql.c_str(), callback, 0, &zErrMsg);
//if (rc != SQLITE_OK)
//	{
//	dbg->trace(DBG_FATAL, "SQL error: %s\n", zErrMsg);
//	sqlite3_free(zErrMsg);
//	return false;
//	}
//else
//	{
//	dbg->trace(DBG_NOTIFY, "database table check ok");
//	}

return true;
}

/**
 * close database
 */
void DBWrapper::close()
{
sqlite3_close(svdb);
sqlite3_close(evdb);
dbg->trace(DBG_NOTIFY, "databases closed");
}

bool DBWrapper::loadSVConfig()
{
int rc;
char *zErrMsg = 0;
string sql, sres;
bool ret = false;
vector<string> toks;

sql = "SELECT * FROM supervisor LIMIT=1";
rc = sqlite3_exec(svdb, sql.c_str(), callback_res_monostring, &sres, &zErrMsg);
__fpurge(stdout);
if(rc != SQLITE_OK)
	{
	dbg->trace(DBG_FATAL, "SQL error: %s (%s)", zErrMsg, sql.c_str());
	sqlite3_free(zErrMsg);
	return false;
	}
else
	{
	if(sres != "NULL" && !sres.empty())
		{
		Split(sres, toks, ";=");
		for(unsigned int i = 0;i < toks.size();i++)
			{
			if(toks[i] == "mode")
				{
				i++;
				if(toks[i] == "SERVER")
					{
					gd.useSerial = false;
					gd.useSocketClient = false;
					gd.useSocketServer = true;
					}
				else if(toks[i] == "CLIENT")  // almost unused!
					{
					gd.useSerial = false;
					gd.useSocketClient = true;
					gd.useSocketServer = false;
					}
				else if(toks[i] == "SERIAL")
					{
					gd.useSerial = true;
					gd.useSocketClient = false;
					gd.useSocketServer = false;
					}
				}
			else if(toks[i] == "device")
				{
				i++;
				strcpy(gd.device, toks[i].c_str());
				}
			else if(toks[i] == "baud_rate")
				{
				i++;
				gd.brate = to_number<int>(toks[i]);
				}
			else if(toks[i] == "port")
				{
				i++;
				gd.port = to_number<int>(toks[i]);
				}
			else if(toks[i] == "has_reader")
				{
				i++;
				gd.has_reader = to_bool(toks[i]);
				}
			else if(toks[i] == "reader_port")
				{
				i++;
				strcpy(gd.reader_device, toks[i].c_str());
				}
			else if(toks[i] == "reader_baud_rate")
				{
				i++;
				gd.reader_brate = to_number<int>(toks[i]);
				}
			else if(toks[i] == "use_telnet")
				{
				i++;
				gd.useTelnet = to_bool(toks[i]);
				}
			else if(toks[i] == "telnet_port")
				{
				i++;
				strcpy(gd.telnetPort, toks[i].c_str());
				}
			else
				{
				dbg->trace(DBG_WARNING, "unhandled field " + toks[i]);
				i++;
				}
			}
		ret = true;
		}
	else
		{
		dbg->trace(DBG_ERROR, "DB configuration of the supervisor missing");
		ret = false;
		}
	}
return ret;
}

/**
 * set all profile to default
 * @param onlyMemory update only memory
 * @return true:ok
 */
bool DBWrapper::setProfileDefault(bool onlyMemory)
{
globals_st::profile_t p;
uint8_t *ptr;
p.params.Parameters_s.activeTerminal = 0;
p.params.Parameters_s.coercion = 0;
p.params.Parameters_s.type = 0;
p.params.Parameters_s.weektimeId = MAX_WEEKTIMES + 1;
for(int i = 0;i < MAX_PROFILES;i++)
	{
	p.params.Parameters_s.profileId = i;
	sprintf(p.name, "Profile%d", i);
	ptr = (uint8_t*) &p.params.Parameters_s;
	p.params.crc8 = CRC8calc(ptr, sizeof(Profile_parameters_t) - 1);

	memcpy(&gd.profiles[i], &p, sizeof(globals_st::profile_t));
	}
updateProfilesCRC();

// set the database
if(!onlyMemory)
	{
	int rc;
	char *zErrMsg = 0;
	string sql;
	string actTerm;
	uint64_t temp_at;

	temp_at = p.params.Parameters_s.activeTerminal;
	activeTerminal2string(temp_at, actTerm);
	p.params.Parameters_s.activeTerminal = temp_at;

	for(int i = 0;i < MAX_PROFILES;i++)
		{
		WATCHDOG_REFRESH();
		sql = "INSERT INTO profile "
				"(id,name,active_terminal,weektime_id,coercion,type,status) "
				"VALUES ("
				"'" + to_string((int) gd.profiles[i].params.Parameters_s.profileId) + "'" + "," + "'" + gd.profiles[i].name + "'" + "," + "'" + actTerm + "'" + "," + "'" + to_string((int) gd.profiles[i].params.Parameters_s.weektimeId) + "'" + "," + "'" + to_string((int) gd.profiles[i].params.Parameters_s.coercion) + "'" + "," + "'" + to_string((int) gd.profiles[i].params.Parameters_s.type) + "'" + "," + "'" + to_string(
		    (i) ? (0) : (1)) + "'" + ")";

		dbg->trace(DBG_DEBUG, "DBsql: " + sql);

		rc = sqlite3_exec(svdb, sql.c_str(), callback, 0, &zErrMsg);
		__fpurge(stdout);
		if(rc != SQLITE_OK)
			{
			dbg->trace(DBG_FATAL, "SQL error: %s (%s)", zErrMsg, sql.c_str());
			sqlite3_free(zErrMsg);
			return false;
			}
		else
			{
			dbg->trace(DBG_NOTIFY, "database profile %d added successfully", i);
			}
		}
	}
return true;
}

/**
 * utility to set a profile data in memory
 * WARN: call updateProfile to update the database
 * @param ndx profile to change
 * @param name
 * @param par
 * @return
 */
bool DBWrapper::setProfile(int ndx, string name, Profile_parameters_t& par)
{
bool ret = true;
//globals_st::profile_t p;

if(ndx < MAX_PROFILES)
	{
	strcpy(gd.profiles[ndx].name, name.c_str());
	memcpy(&gd.profiles[ndx].params, &par, sizeof(Profile_parameters_t));
	gd.profiles[ndx].params.crc8 = CRC8calc((uint8_t *) &gd.profiles[ndx].params.Parameters_s, sizeof(gd.profiles[ndx].params.Parameters_s));
	ret = true;
	}
else
	{
	ret = false;
	}
return ret;
}

/**
 * update a profile
 * @param ndx profile data index
 * @param name profile name, can be empty
 * @return true: ok
 */
bool DBWrapper::updateProfile(int ndx, string name)
{
int rc;
char *zErrMsg = 0;
string sql;
string actTerm;
uint64_t temp_at;

temp_at = gd.profiles[ndx].params.Parameters_s.activeTerminal;
activeTerminal2string(temp_at, actTerm);

if(!name.empty())
	{
	name.resize(MAX_NAME_SIZE);
	strcpy(gd.profiles[ndx].name, name.c_str());
	}

sql = (string) "UPDATE profile SET "
		"name = " + "'" + gd.profiles[ndx].name + "'" + "," + "active_terminal = " + "'" + actTerm + "'" + "," + "weektime_id = " + "'" + to_string((int) gd.profiles[ndx].params.Parameters_s.weektimeId) + "'" + "," + "coercion = " + "'" + to_string((int) gd.profiles[ndx].params.Parameters_s.coercion) + "'" + "," + "type = " + "'" + to_string((int) gd.profiles[ndx].params.Parameters_s.type) + "'" + " WHERE id = '" + to_string(ndx) + "'";

//dbg->trace(DBG_DEBUG, "DBsql: " + sql);

rc = sqlite3_exec(svdb, sql.c_str(), callback, 0, &zErrMsg);
__fpurge(stdout);
if(rc != SQLITE_OK)
	{
	dbg->trace(DBG_FATAL, "SQL error: %s (%s)", zErrMsg, sql.c_str());
	sqlite3_free(zErrMsg);
	return false;
	}
else
	{
	updateProfilesCRC();
	dbg->trace(DBG_NOTIFY, "database profile %d update successfully", ndx);
	}
return true;
}

/**
 * set all weektimes to default
 * @param onlyMemory only memory update
 * @return true:ok
 */
bool DBWrapper::setWeektimeDefault(bool onlyMemory)
{
globals_st::weektime_t w;
uint8_t *ptr;

for(int i = 0;i < MAX_WEEKTIMES;i++)
	{
	w.params.Parameters_s.weektimeId = i;
	sprintf(w.name, "Weektime%d", i);
	bool hour = true;
	for(unsigned int j = 0;j < sizeof(Weektime_parameters_t) - 2;j++)
		{
		if(hour)
			{
			w.params.Parameters_s.week.weekdays[j] = HOUR_MAX;
			hour = false;
			}
		else
			{
			w.params.Parameters_s.week.weekdays[j] = MINUTE_MAX;
			hour = true;
			}
		}
	ptr = (uint8_t*) &w.params.Parameters_s;
	w.params.crc8 = CRC8calc(ptr, sizeof(Weektime_parameters_t) - 1);

	memcpy(&gd.weektimes[i], &w, sizeof(globals_st::weektime_t));
	}
updateWeektimesCRC();

if(!onlyMemory)
	{
	// set the database
	int rc;
	char *zErrMsg = 0;
	string sql;
	char str[WT_STRING_FORM_SIZE];

	for(int i = 0;i < MAX_WEEKTIMES;i++)
		{
		WATCHDOG_REFRESH();
		// create one string
		weektime2str(i, str);

		sql = "INSERT INTO weektime "
				"(id,name,weektimedata) "
				"VALUES ("
				"'" + to_string((int) gd.weektimes[i].params.Parameters_s.weektimeId) + "'" + "," + "'" + gd.weektimes[i].name + "'" + "," + "'" + str + "'" + ")";

		dbg->trace(DBG_DEBUG, "DBsql: " + sql);

		rc = sqlite3_exec(svdb, sql.c_str(), callback, 0, &zErrMsg);
		__fpurge(stdout);
		if(rc != SQLITE_OK)
			{
			dbg->trace(DBG_FATAL, "SQL error: %s (%s)", zErrMsg, sql.c_str());
			sqlite3_free(zErrMsg);
			return false;
			}
		else
			{
			dbg->trace(DBG_NOTIFY, "database weektime %d added successfully", i);
			}
		}
	}
return true;
}


/**
 * utility to set a weektime data in memory
 * WARN: call updateWeektime to update the database
 * @param ndx weektime to change
 * @param name
 * @param par
 * @return
 */
bool DBWrapper::setWeektime(int ndx, string name, Weektime_parameters_t& par)
{
bool ret = true;

if(ndx < MAX_WEEKTIMES)
	{
	strcpy(gd.weektimes[ndx].name, name.c_str());
	memcpy(&gd.weektimes[ndx].params, &par, sizeof(Weektime_parameters_t));
	gd.weektimes[ndx].params.crc8 = CRC8calc((uint8_t *) &gd.weektimes[ndx].params.Parameters_s, sizeof(gd.weektimes[ndx].params.Parameters_s));
	ret = true;
	}
else
	{
	ret = false;
	}
return ret;
}


/**
 * update a weektime
 * @param ndx weektime data index
 * @param name weektime name
 * @return true:ok
 */
bool DBWrapper::updateWeektime(int ndx, string name)
{
int rc;
char *zErrMsg = 0;
string sql;
char str[WT_STRING_FORM_SIZE];

if(!name.empty())
	{
	name.resize(MAX_NAME_SIZE);
	strcpy(gd.weektimes[ndx].name, name.c_str());
	}

weektime2str(ndx, str);
sql = (string) "UPDATE weektime SET "
		"name = " + "'" + gd.weektimes[ndx].name + "'" + "," + "weektimedata = " + "'" + str + "'" + " WHERE id = '" + to_string(ndx) + "'";

//dbg->trace(DBG_DEBUG, "DBsql: " + sql);

rc = sqlite3_exec(svdb, sql.c_str(), callback, 0, &zErrMsg);
__fpurge(stdout);
if(rc != SQLITE_OK)
	{
	dbg->trace(DBG_FATAL, "SQL error: %s (%s)", zErrMsg, sql.c_str());
	sqlite3_free(zErrMsg);
	return false;
	}
else
	{
	updateWeektimesCRC();
	dbg->trace(DBG_NOTIFY, "database weektime %d update successfully", ndx);
	}
return true;
}

/**
 * clean the contents and set to default
 * @param which 'p' profiles; 'w' weektimes; 'b' badges; 'e' events; 't' terminals
 * @return true: ok
 */
bool DBWrapper::cleanTable(char which)
{
bool ret = true;
int rc;
sqlite3 *pers_evdb;
switch(which)
	{
	case 'p':
		dbg->trace(DBG_NOTIFY, "delete all profiles data");
		ret &= exeSQL(svdb, "DELETE FROM profile;VACUUM;");
		ret &= setProfileDefault();
		break;
	case 'w':
		dbg->trace(DBG_NOTIFY, "delete all weektimes data");
		ret &= exeSQL(svdb, "DELETE FROM weektime;VACUUM;");
		ret &= setWeektimeDefault();
		break;
	case 'b':
		dbg->trace(DBG_NOTIFY, "delete all badge data and area log");
		ret &= exeSQL(svdb, "DELETE FROM badge;VACUUM;");
		ret &= exeSQL(svdb, "DELETE FROM userbadge;VACUUM;");
		ret &= exeSQL(evdb, "DELETE FROM area;VACUUM;");
		gd.maxBadgeId = 0;
		updateMostRecentBadgeTimestamp();
		dbSave();
		break;
	case 'e':
		dbg->trace(DBG_NOTIFY, "delete all events data (temporary table)");
		ret &= exeSQL(evdb, "DELETE FROM history;VACUUM;");

		dbg->trace(DBG_NOTIFY, "delete all events data (persistent table)");
		rc = sqlite3_open((gd.dbPersistFolder + DB_VARCOLANEV).c_str(), &pers_evdb);
		__fpurge(stdout);
		if(rc)
			{
			dbg->trace(DBG_FATAL, "can't open database: %s", sqlite3_errmsg(pers_evdb));
			sqlite3_close(pers_evdb);
			return false;
			}
		dbg->trace(DBG_NOTIFY, "event database opened successfully");
		ret &= exeSQL(pers_evdb, "DELETE FROM history;VACUUM;");
		sqlite3_close(pers_evdb);
		break;
	case 't':
		dbg->trace(DBG_NOTIFY, "delete all terminals");
		// terminals cannot be deleted but can be set in deleted state
		cleanTerminalStatus(TERMINAL_STATUS_NOT_PRESENT);
		cleanTerminalName("");
		//ret&=exeSQL(evdb,"DELETE FROM terminal;VACUUM;");
		break;
	case 'u':
		dbg->trace(DBG_NOTIFY, "delete all users name data");
		ret &= exeSQL(svdb, "DELETE FROM user;VACUUM;");
		ret &= exeSQL(svdb, "DELETE FROM userbadge;VACUUM;");
		break;
	case 'j':
		dbg->trace(DBG_NOTIFY, "delete all jobs");
		ret &= exeSQL(svdb, "DELETE FROM scheduled_jobs;VACUUM;");
		break;
	case 'd':
		dbg->trace(DBG_NOTIFY, "delete all admin/users");
		ret &= exeSQL(svdb, "DELETE FROM adminusers;VACUUM;");
		addAdminUser("admin", "e10adc3949ba59abbe56e057f20f883e", 1);
		addAdminUser("view", "e10adc3949ba59abbe56e057f20f883e", 100);
		addAdminUser("user", "e10adc3949ba59abbe56e057f20f883e", 10);
		break;
	case 'a':
		dbg->trace(DBG_NOTIFY, "delete all area definitios");
		ret &= exeSQL(svdb, "DELETE FROM area;VACUUM;");
		addArea(0, 0, "Esterno");
		addArea(1, 0, "Interno");
		break;
	case 'q':
		dbg->trace(DBG_NOTIFY, "delete all custom queries");
		ret &= exeSQL(svdb, "DELETE FROM custom_queries;VACUUM;");
		break;
	case 'c':
			dbg->trace(DBG_NOTIFY, "delete all causal codes");
			ret &= exeSQL(svdb, "DELETE FROM causal_codes;VACUUM;");
			break;
	}
return ret;
}

/**
 * set default admin / users
 * @param username
 * @param password
 * @param role
 * @return
 */
bool DBWrapper::addAdminUser(string username, string password, int role)
{
int rc;
char *zErrMsg = 0;
string sql;

sql = "INSERT OR REPLACE INTO adminusers "
		"(username,password,role) "
		"VALUES ("
		"'" + username + "'" + "," + "'" + password + "'" + "," + "'" + to_string(role) + "'" + ")";

//dbg->trace(DBG_DEBUG, "DBsql: " + sql);

rc = sqlite3_exec(svdb, sql.c_str(), callback, 0, &zErrMsg);
__fpurge(stdout);
if(rc != SQLITE_OK)
	{
	dbg->trace(DBG_FATAL, "SQL error: %s (%s)", zErrMsg, sql.c_str());
	sqlite3_free(zErrMsg);
	return false;
	}
else
	{
	dbg->trace(DBG_NOTIFY, "adminuser " + username + " added successfully");
	}
return true;
}

/**
 * add an area definition
 * @param area_id
 * @param current_area
 * @param area_name
 * @return
 */
bool DBWrapper::addArea(int area_id, int current_area, string area_name)
{
int rc;
char *zErrMsg = 0;
string sql;

sql = "INSERT OR REPLACE INTO area "
		"(area_id,current_area,name) "
		"VALUES ("
		"'" + to_string(area_id) + "'" + "," + "'" + to_string(current_area) + "'" + "," + "'" + area_name + "'" + ")";

//dbg->trace(DBG_DEBUG, "DBsql: " + sql);

rc = sqlite3_exec(svdb, sql.c_str(), callback, 0, &zErrMsg);
__fpurge(stdout);
if(rc != SQLITE_OK)
	{
	dbg->trace(DBG_FATAL, "SQL error: %s (%s)", zErrMsg, sql.c_str());
	sqlite3_free(zErrMsg);
	return false;
	}
else
	{
	dbg->trace(DBG_NOTIFY, "area " + area_name + " added successfully");
	}
return true;
}

//-----------------------------------------------------------------------------
// PROFILES
//-----------------------------------------------------------------------------

/**
 * load profiles from database to memory
 */
bool DBWrapper::loadProfiles()
{
int rc;
char *zErrMsg = 0;
string sql;
bool ret = false;

sql = (string) "SELECT * FROM profile";

//dbg->trace(DBG_DEBUG, "DBsql: " + sql);

querydata.clear();
rc = sqlite3_exec(svdb, sql.c_str(), callback_vec_string, &querydata, &zErrMsg);
__fpurge(stdout);
if(rc != SQLITE_OK)
	{
	dbg->trace(DBG_FATAL, "SQL error: %s (%s)", zErrMsg, sql.c_str());
	sqlite3_free(zErrMsg);
	return false;
	}
else
	{
	dbg->trace(DBG_NOTIFY, "query to retrieve profiles success");
	if(querydata.size() == 0)
		{
		dbg->trace(DBG_NOTIFY, "SQL: profiles not found");
		ret = false;
		}
	else
		{
		fillProfileData();
		updateProfilesCRC();
		ret = true;
		}
	}
return ret;
}

/**
 * fill memory structure with data
 */
void DBWrapper::fillProfileData()
{
int ndx = -1;
globals_st::profile_t p;
toks.clear();
vector<string> toks1;
if(querydata.size() > 0)
	{
	FOR_EACH(it,string,querydata){
	Split(*it,toks,";=");
	for(unsigned int i=0;i<toks.size();i++)
		{
		if(toks[i]=="id")
			{
			i++;
			ndx=atoi(toks[i].c_str());
			p.params.Parameters_s.profileId=ndx;
			}
		else if(toks[i]=="name")
			{
			i++;
			strcpy(p.name,toks[i].c_str());
			}
		else if(toks[i]=="active_terminal")
			{
			uint64_t nat;
			string sat;
			i++;
			sat=toks[i];
			string2activeTerminal(sat,nat);
			p.params.Parameters_s.activeTerminal=nat;
			}
		else if(toks[i]=="weektime_id")
			{
			i++;
			p.params.Parameters_s.weektimeId=atoi(toks[i].c_str());
			}
		else if(toks[i]=="coercion")
			{
			i++;
			p.params.Parameters_s.coercion=atoi(toks[i].c_str());
			}
		else if(toks[i]=="type")
			{
			i++;
			p.params.Parameters_s.type=atoi(toks[i].c_str());
			}
		}
	if(ndx>=0)
		{
		memcpy(&gd.profiles[ndx],&p,sizeof(globals_st::profile_t));
		gd.profiles[ndx].params.crc8=CRC8calc((uint8_t *)&gd.profiles[ndx].params.Parameters_s,sizeof(Profile_parameters_t::Parameters_s));
		}
	else
		{
		dbg->trace(DBG_ERROR, "cannot find a valid ID");
		}
	}
}
}

//-----------------------------------------------------------------------------
// WEEKTIMES
//-----------------------------------------------------------------------------

/**
 * load profiles from database to memory
 */
bool DBWrapper::loadWeektimes()
{
int rc;
char *zErrMsg = 0;
string sql;
bool ret = false;

sql = (string) "SELECT * FROM weektime";

//dbg->trace(DBG_DEBUG, "DBsql: " + sql);

querydata.clear();
rc = sqlite3_exec(svdb, sql.c_str(), callback_vec_string, &querydata, &zErrMsg);
__fpurge(stdout);
if(rc != SQLITE_OK)
	{
	dbg->trace(DBG_FATAL, "SQL error: %s (%s)", zErrMsg, sql.c_str());
	sqlite3_free(zErrMsg);
	return false;
	}
else
	{
	dbg->trace(DBG_NOTIFY, "query to retrieve weektimes success");
	if(querydata.size() == 0)
		{
		dbg->trace(DBG_NOTIFY, "SQL: weektime not found");
		ret = false;
		}
	else
		{
		fillWeektimeData();
		updateWeektimesCRC();
		ret = true;
		}
	}
return ret;
}

/**
 * fill memory structure with data
 */
void DBWrapper::fillWeektimeData()
{
int ndx = -1;
globals_st::weektime_t w;
toks.clear();
vector<string> toks1;
if(querydata.size() > 0)
	{
	FOR_EACH(it,string,querydata){
	Split(*it,toks,";=");
	for(unsigned int i=0;i<toks.size();i++)
		{
		if(toks[i]=="id")
			{
			i++;
			ndx=atoi(toks[i].c_str());
			w.params.Parameters_s.weektimeId=ndx;
			}
		else if(toks[i]=="name")
			{
			i++;
			strcpy(w.name,toks[i].c_str());
			}
		else if(toks[i]=="weektimedata")
			{
			i++;
			if(toks[i]==_S "24:60|24:60|24:60|24:60|24:60|24:60|24:60|24:60|24:60|24:60|24:60|24:60|24:60|24:60|24:60|24:60|24:60|24:60|24:60|24:60|24:60|24:60|24:60|24:60|24:60|24:60|24:60|24:60|")
				{

				weektimeActive &= ~(1ULL<<ndx);
				}
			else
				{
				weektimeActive |= (1ULL<<ndx);
				}
			// split the giant string :)
			toks1.clear();
			Split(toks[i],toks1,"|:");
			for(unsigned int j = 0;j < sizeof(Weektime_parameters_t) - 2;j++)
				{
				w.params.Parameters_s.week.weekdays[j]=atoi(toks1[j].c_str());
				}
			memcpy(&w.params.Parameters_s.week,&w.params.Parameters_s.week,sizeof(w.params.Parameters_s.week));
			}
		}
	if(ndx>=0)
		{
		memcpy(&gd.weektimes[ndx],&w,sizeof(globals_st::weektime_t));
		gd.weektimes[ndx].params.crc8=CRC8calc((uint8_t *)&gd.weektimes[ndx].params.Parameters_s,sizeof(Weektime_parameters_t::Parameters_s));
		}
	else
		{
		dbg->trace(DBG_ERROR, "cannot find a valid ID");
		}
	}
}
}

//-----------------------------------------------------------------------------
// CAUSALS
//-----------------------------------------------------------------------------

/**
 * load causals from database to memory
 */
bool DBWrapper::loadCausals()
{
int rc;
char *zErrMsg = 0;
string sql;
bool ret = false;

sql = (string) "SELECT * FROM causal_codes";

//dbg->trace(DBG_DEBUG, "DBsql: " + sql);

querydata.clear();
rc = sqlite3_exec(svdb, sql.c_str(), callback_vec_string, &querydata, &zErrMsg);
__fpurge(stdout);
if(rc != SQLITE_OK)
	{
	dbg->trace(DBG_FATAL, "SQL error: %s (%s)", zErrMsg, sql.c_str());
	sqlite3_free(zErrMsg);
	return false;
	}
else
	{
	dbg->trace(DBG_NOTIFY, "query to retrieve causals success");
	if(querydata.size() == 0)
		{
		dbg->trace(DBG_NOTIFY, "SQL: causals not found");
		ret = false;
		}
	else
		{
		fillCausalData();
		updateCausalCRC();
		ret = true;
		}
	}
return ret;
}

/**
 * fill memory structure with data
 */
void DBWrapper::fillCausalData()
{
int ndx = -1, cid, dlen;
//CausalCodes_parameters_t c;
toks.clear();
//vector<string> toks1;
char desc[CAUSAL_CODES_DESCRIPTION_SIZE];
if(querydata.size() > 0)
	{
	FOR_EACH(it,string,querydata){
	Split(*it,toks,";=");
	for(unsigned int i=0;i<toks.size();i++)
		{
		if(toks[i]=="n")
			{
			i++;
			ndx=atoi(toks[i].c_str());
			}
		else if(toks[i]=="causal_id")
			{
			i++;
			cid=atoi(toks[i].c_str());
			}
		else if(toks[i]=="description")
			{
			i++;
			strcpy(desc,toks[i].c_str());
			dlen=toks[i].length();
			}
		}
	if(ndx>=0)
		{
		gd.causals[ndx-1].Parameters_s.causal_Id=cid;
		memcpy(gd.causals[ndx-1].Parameters_s.description,desc,dlen);
		gd.causals[ndx-1].crc8=CRC8calc((uint8_t *)&gd.causals[ndx-1].Parameters_s,sizeof(CausalCodes_parameters_t::Parameters_s));
		}
	else
		{
		TRACE(dbg,DBG_ERROR, "cannot find a valid index in causal");
		}
	}
}
}

/**
 * set a causal
 * @param ndx
 * @param id
 * @param desc
 * @return
 */
bool DBWrapper::setCausalDefault()
{
int rc;
char *zErrMsg = 0;
string sql;
char str[WT_STRING_FORM_SIZE];

for(int i = 0;i < MAX_CAUSAL_CODES;i++)
	{
	WATCHDOG_REFRESH();
	// create one string
	weektime2str(i, str);

//	sql="INSERT INTO causal_codes " \
//			"(n,causal_id,description) " \
//			"VALUES (" \
//			"'"+to_string(i+1)+"'" + "," +
//			"'"+to_string(100+i+1)+"'" + "," +
//			"'causale"+to_string(i+1)+"'" +
//			")";
	sql = "INSERT INTO causal_codes "
			"(n,causal_id,description) "
			"VALUES ("
			"'" + to_string(i + 1) + "'" + "," + "'" + to_string(NULL_CAUSAL_CODE) + "'" + "," + "''" + ")";

	dbg->trace(DBG_DEBUG, "DBsql: " + sql);

	rc = sqlite3_exec(svdb, sql.c_str(), callback, 0, &zErrMsg);
	__fpurge(stdout);
	if(rc != SQLITE_OK)
		{
		dbg->trace(DBG_FATAL, "SQL error: %s (%s)", zErrMsg, sql.c_str());
		sqlite3_free(zErrMsg);
		return false;
		}
	else
		{
		dbg->trace(DBG_NOTIFY, "causal default %d added successfully", i + 1);
		}
	}
return true;
}

/**
 * get a causal
 * @param ndx if<=0 use id to retrieve data
 * @param id if 0 use ndx to retrieve data
 * @param desc description
 * @return
 */
/*todo does not work Split(sres, toks, ";");*/
bool DBWrapper::getCausal(int &ndx, int &cid, string &desc)
{
int rc;
char *zErrMsg = 0;
string sql;
string sres;

if(ndx > 0 && ndx <= 10)
	{
	// use ndx
	sql = (string) "SELECT * WHERE n='" + to_string(ndx) + "' FROM causal_codes";
	}
else
	{
	// use id
	sql = (string) "SELECT * WHERE id='" + to_string(cid) + "' FROM causal_codes";
	}
//dbg->trace(DBG_DEBUG, "DBsql: " + sql);

rc = sqlite3_exec(evdb, sql.c_str(), callback_res_monostring, &sres, &zErrMsg);
__fpurge(stdout);
if(rc != SQLITE_OK)
	{
	TRACE(dbg, DBG_FATAL, "SQL error: %s (%s)", zErrMsg, sql.c_str());
	sqlite3_free(zErrMsg);
	return false;
	}
else
	{
	if(sres != "NULL" && !sres.empty())
		{
		toks.clear();
		Split(sres, toks, ";");
		int _ndx = to_number<int>(toks[0]);
		int _id = to_number<int>(toks[1]);
		TRACE(dbg, DBG_NOTIFY, "causal code %d: (%d) %s", _ndx, _id, toks[2].c_str());
		ndx = _ndx;
		cid = _id;
		desc = toks[2];
		}
	else
		{
		TRACE(dbg, DBG_ERROR, "Causal not found");
		return false;
		}
	}
return true;
}

/**
 * setup a causal
 * @param ndx
 * @param id
 * @param desc
 * @return
 */
bool DBWrapper::setCausal(int ndx, int cid, string desc)
{
int rc;
char *zErrMsg = 0;
string sql;
string actTerm;
uint64_t temp_at;

sql = (string) "UPDATE causal_codes SET "
		"causal_id = '" + to_string(cid) + "', " + "description = '" + desc + "'" + " WHERE n = '" + to_string(ndx) + "'";

//dbg->trace(DBG_DEBUG, "DBsql: " + sql);

rc = sqlite3_exec(svdb, sql.c_str(), callback, 0, &zErrMsg);
__fpurge(stdout);
if(rc != SQLITE_OK)
	{
	TRACE(dbg, DBG_FATAL, "SQL error: %s (%s)", zErrMsg, sql.c_str());
	sqlite3_free(zErrMsg);
	return false;
	}
else
	{
	TRACE(dbg, DBG_NOTIFY, "causal %d updated (%d, %s)", ndx, cid, desc.c_str());
	}
return true;
}
//-----------------------------------------------------------------------------
// BADGES
//-----------------------------------------------------------------------------

/**
 * calculates the new id of a badge. It derive it for the last ROWID in the database
 * @return new badge id
 */
uint64_t DBWrapper::getNewBadgeId()
{
int rc;
char *zErrMsg = 0;
string sql, sres;

sql = "SELECT MAX(id) FROM badge";
rc = sqlite3_exec(svdb, sql.c_str(), callback_res_monostring, &sres, &zErrMsg);
__fpurge(stdout);
if(rc != SQLITE_OK)
	{
	dbg->trace(DBG_FATAL, "SQL error: %s\n", zErrMsg);
	sqlite3_free(zErrMsg);
	return false;
	}
else
	{
	if(sres != "NULL" && !sres.empty())
		{
		lastBadgeID = to_number<unsigned int>(sres);
		dbg->trace(DBG_NOTIFY, "last badge id=%d the newer will be %d", lastBadgeID, lastBadgeID + 1);
		}
	else
		{
		lastBadgeID = 0;
		dbg->trace(DBG_ERROR, "last badge id cannot be found -> 0");
		}

	lastBadgeID++;
	if(lastBadgeID > 65536)
		{
		lastBadgeID = 65536;
		dbg->trace(DBG_ERROR, "new badge ID is greater oh the maximum allowed, it will be forced to %d", lastBadgeID);
		}
	}
return lastBadgeID;
}

/**
 * Update most recent badge timestamp.
 * It shall call whenever a badge is add/edit, in order to calculate it just after modification of badges
 */
void DBWrapper::updateMostRecentBadgeTimestamp()
{
int rc;
char *zErrMsg = 0;
string sql;
string sres, toks;
time_t ret_val = 0;

sql = (string) "SELECT MAX(timestamp) from badge";

//dbg->trace(DBG_DEBUG, "DBsql: " + sql);

rc = sqlite3_exec(svdb, sql.c_str(), callback_res_monostring, &sres, &zErrMsg);
__fpurge(stdout);
if(rc != SQLITE_OK)
	{
	TRACE(dbg, DBG_FATAL, "SQL error: %s (%s)", zErrMsg, sql.c_str());
	sqlite3_free(zErrMsg);
	}
else
	{
	if(sres != "NULL" && !sres.empty())
		{
		TRACE(dbg, DBG_NOTIFY, "most recent badge has timestamp " + sres);
		ret_val = to_number<time_t>(sres);
		}
	}
gd.mostRecentBadgeTimestamp = ret_val;
}

/**
 * get badge parameter from timestamp
 * @param timestamp
 * @param  par pointer to badge parameter struct to be filled
 * return true if badge exist, false otherwise
 */
bool DBWrapper::getBadgeFromTimestamp(time_t timestamp, Badge_parameters_t *par)
{
/* TODO */
return true;
}

/**
 * set a badge entry to the database
 * @param par badge parameters
 * @return -1 error; id of the badge
 */
int DBWrapper::setBadge(Badge_parameters_t &par, time_t timestamp, char *printedCode, char *contact)
{
int rc;
char *zErrMsg = 0;
string sql;
string badge = "";
string et_str = "";
char tmp[100];
string _printedCode;
string _contact;

for(int i = 0;i < BADGE_SIZE;i++)
	{
	sprintf(tmp, "%02X", par.Parameters_s.badge[i].full);
	badge += tmp;
	}

sprintf(tmp, "%04d-%02d-%02d", (int) par.Parameters_s.start_year + 2000, (int) par.Parameters_s.start_month, (int) par.Parameters_s.start_day);
string date_vstart(tmp);
sprintf(tmp, "%04d-%02d-%02d", (int) par.Parameters_s.stop_year + 2000, (int) par.Parameters_s.stop_month, (int) par.Parameters_s.stop_day);
string date_vend(tmp);

/* Time stamp */
time_t et;
#ifdef RASPBERRY
et = (timestamp == 0) ? hwgpio->getRTC_datetime() : timestamp;
#else
et=time(NULL);
#endif
et_str = to_string<time_t>(et);

/* Printed Char */
if(printedCode != NULL)
	_printedCode = printedCode;
else
	_printedCode = "";

/* Contact */
if(contact != NULL)
	_contact = contact;
else
	_contact = "";

// check the presence of a badge
int id = getBadgeId((uint8_t*) par.Parameters_s.badge);
if(id == -1)
	{
	id = getNewBadgeId();
	}
par.Parameters_s.ID = id;

sql = "INSERT OR REPLACE INTO badge "
		"(id,badge_num,visitor,status,pin,validity_start,validity_stop,timestamp,printed_code,contact) "
		"VALUES ("
		"'" + to_string((int) par.Parameters_s.ID) + "'" + "," + "'" + badge + "'" + "," + "'" + (
    (par.Parameters_s.visitor == 1) ? +"1" : "0") + "'" + "," + "'" + to_string((int) par.Parameters_s.badge_status) + "'" + "," + "'" + to_string((int) par.Parameters_s.pin) + "'" + "," + "'" + date_vstart + "'" + "," + "'" + date_vend + "'" + "," + "'" + et_str + "'" + "," + "'" + _printedCode + "'" + "," + "'" + _contact + "'" + ")";

//dbg->trace(DBG_DEBUG, "DBsql: " + sql);
rc = sqlite3_exec(svdb, sql.c_str(), callback, 0, &zErrMsg);
__fpurge(stdout);
if(rc != SQLITE_OK)
	{
	dbg->trace(DBG_FATAL, "SQL error: %s (%s)", zErrMsg, sql.c_str());
	sqlite3_free(zErrMsg);
	return false;
	}
else
	{
	// create an entry in the area table too
	setArea(par.Parameters_s.ID, 0);
	dbg->trace(DBG_NOTIFY, "badge id=%d add/update success", id);
	}

updateMostRecentBadgeTimestamp();
return id;
}

/**
 * change the pin number of a badge (found with its id)
 * @param id
 * @param newpin
 * @return true: ok
 */
bool DBWrapper::changeBadgePinFromId(int id, string newpin)
{
bool ret = true;
Badge_parameters_t par;
if(getBadgeFromId(id, par))
	{
	// badge found
	par.Parameters_s.pin = to_number<int>(newpin);
	// write back
	if(setBadge(par) < 0)
		{
		ret = false;
		}
	}
else
	{
	ret = false;
	}
if(ret) dbg->trace(DBG_NOTIFY, "badge id=%d pin changed successfully", id);
return ret;
}

/**
 * retrieve badge information from its id
 * @param id
 * @param par
 * @return false: not found or error
 */
bool DBWrapper::getBadgeFromId(int id, Badge_parameters_t &par, bool debugMsg)
{
bool ret = false;
int rc;
char *zErrMsg = 0;
string sql;

sql = (string) "SELECT * FROM badge WHERE id='" + to_string(id) + "'";

//dbg->trace(DBG_DEBUG, "DBsql: " + sql);
querydata.clear();
rc = sqlite3_exec(svdb, sql.c_str(), callback_vec_string, &querydata, &zErrMsg);
__fpurge(stdout);
if(rc != SQLITE_OK)
	{
	gd.globalErrors++;
	dbg->trace(DBG_FATAL, "SQL error: %s (%s)", zErrMsg, sql.c_str());
	sqlite3_free(zErrMsg);
	return false;
	}
else
	{
	if(debugMsg) dbg->trace(DBG_NOTIFY, "query to retrieve badge data success");
	if(querydata.size() == 0)
		{
		if(debugMsg) TRACE(dbg, DBG_NOTIFY, "SQL: badge id %d not found", id);
		ret = false;
		}
	else
		{
		ret = fillBadgeData(par);
		}
	}
return ret;
}

/**
 * retrieve badge information from its id
 * @param uid unique id of the badge (normalized)
 * @param par
 * @return false: not found or error
 */
bool DBWrapper::getBadgeFromUID(uint8_t *uid, Badge_parameters_t &par)
{
bool ret = false;
int rc;
char *zErrMsg = 0;
string sql;
char badge[30];

Hex2AsciiHex(badge, uid, BADGE_SIZE, false, 0);
//strcpy(badge,(char*)uid);
sql = (string) "SELECT * FROM badge WHERE badge_num='" + badge + "'";

//dbg->trace(DBG_DEBUG, "DBsql: " + sql);
querydata.clear();
rc = sqlite3_exec(svdb, sql.c_str(), callback_vec_string, &querydata, &zErrMsg);
__fpurge(stdout);
if(rc != SQLITE_OK)
	{
	gd.globalErrors++;
	dbg->trace(DBG_FATAL, "SQL error: %s (%s)", zErrMsg, sql.c_str());
	sqlite3_free(zErrMsg);
	return false;
	}
else
	{
	dbg->trace(DBG_NOTIFY, "query to retrieve badge data success");
	if(querydata.size() == 0)
		{
		dbg->trace(DBG_NOTIFY, "SQL: badge %s not found", badge);
		ret = false;
		}
	else
		{
		ret = fillBadgeData(par);
		}
	}
return ret;
}

/**
 * check if a badge is present
 * @param par
 * @return -1 not present; else returnits id
 */
int DBWrapper::getBadgeId(uint8_t *uid)
{
int rc;
char *zErrMsg = 0;
string sql, sres;
char badge[30];
int id = -1;

Hex2AsciiHex(badge, uid, BADGE_SIZE, false, 0);

sql = "SELECT id FROM badge WHERE badge_num='" + _S badge + "'";
rc = sqlite3_exec(svdb, sql.c_str(), callback_res_monostring, &sres, &zErrMsg);
__fpurge(stdout);
if(rc != SQLITE_OK)
	{
	gd.globalErrors++;
	dbg->trace(DBG_FATAL, "SQL error: %s (%s)", zErrMsg, sql.c_str());
	sqlite3_free(zErrMsg);
	return false;
	}
else
	{
	if(sres != "NULL" && !sres.empty())
		{
		id = to_number<int>(sres);
		}
	}
return id;
}

/**
 * retrun the max id of a badge database
 * @return -1 not present; else return the max id
 */
int DBWrapper::getBadgeMaxId()
{
int rc;
char *zErrMsg = 0;
string sql, sres;
char badge[30];
int id = -1;

sql = "SELECT MAX(id) FROM badge";
rc = sqlite3_exec(svdb, sql.c_str(), callback_res_monostring, &sres, &zErrMsg);
__fpurge(stdout);
if(rc != SQLITE_OK)
	{
	gd.globalErrors++;
	TRACE(dbg, DBG_FATAL, "SQL error: %s (%s)", zErrMsg, sql.c_str());
	sqlite3_free(zErrMsg);
	return false;
	}
else
	{
	if(sres != "NULL" && !sres.empty())
		{
		id = to_number<int>(sres);
		}
	else
		{
		TRACE(dbg, DBG_NOTIFY, "no badge present, so the first ID is 1");
		id = 1;
		}
	}
return id;
}

/**
 * normalize data badge to BADGE_SIZE, adding 'f' at the end
 * @param uid original
 * @param size original size
 * @param uid_norm normalized
 * @param conv if true convert it to string (false=default)
 */
void DBWrapper::normalizeBadge(uint8_t *uid, int size, uint8_t *uid_norm, bool conv)
{
for(int i = 0;i < BADGE_SIZE;i++)
	{
	if(i < size)
		{
		uid_norm[i] = uid[i];
		}
	else
		{
		uid_norm[i] = 0xff;
		}
	}
if(conv)
	{
	Hex2AsciiHex((char *) uid_norm, uid, BADGE_SIZE, false, 0);
	}
}

/**
 * parse query result to fill the data structure
 * @param par
 * @return
 */
bool DBWrapper::fillBadgeData(Badge_parameters_t &par)
{
toks.clear();
vector<string> toks1;
if(querydata.size() > 0)
	{
	if(querydata.size() > 1)
		{
		dbg->trace(DBG_WARNING, "badge ID is not unique, take the first");
		}
	Split(querydata[0], toks, ";=");
	for(unsigned int i = 0;i < toks.size();i++)
		{
		if(toks[i] == "id")
			{
			i++;
			par.Parameters_s.ID = atoi(toks[i].c_str());
			}
		else if(toks[i] == "badge_num")
			{
			i++;
			if(toks[i].size() > (BADGE_SIZE * 2))
				{
				dbg->trace(DBG_ERROR, "badge size too big?");
				return false;
				}
			else
				{
				int val = 0;
				for(unsigned int k = 0, j = 0;k < toks[i].size();k++)
					{
					(toks[i][k] >= 'A') ? (val = toks[i][k] - 'A' + 10) : (val = toks[i][k] - '0');
					if(k & 1)
						{
						par.Parameters_s.badge[j].part.low = val;
						j++;
						}
					else
						{
						par.Parameters_s.badge[j].part.high = val;
						}
					}
				}
			}
		else if(toks[i] == "visitor")
			{
			i++;
			par.Parameters_s.visitor = atoi(toks[i].c_str());
			}
		else if(toks[i] == "status")
			{
			i++;
			par.Parameters_s.badge_status = (Badge_Status_t) atoi(toks[i].c_str());
			}
		else if(toks[i] == "pin")
			{
			i++;
			par.Parameters_s.pin = atoi(toks[i].c_str());
			}
		else if(toks[i] == "user_type")
			{
			i++;
			par.Parameters_s.user_type = (User_type_e) atoi(toks[i].c_str());
			}
		else if(toks[i] == "validity_start")
			{
			i++;
			toks1.clear();
			if(toks[i] != "NULL")
				{
				Split(toks[i], toks1, "-");
				par.Parameters_s.start_year = to_number<int>(toks1[0]) - 2000;
				par.Parameters_s.start_month = to_number<int>(toks1[1]);
				par.Parameters_s.start_day = to_number<int>(toks1[2]);
				}
			}
		else if(toks[i] == "validity_stop")
			{
			i++;
			toks1.clear();
			if(toks[i] != "NULL")
				{
				Split(toks[i], toks1, "-");
				par.Parameters_s.stop_year = to_number<int>(toks1[0]) - 2000;
				par.Parameters_s.stop_month = to_number<int>(toks1[1]);
				par.Parameters_s.stop_day = to_number<int>(toks1[2]);
				}
			}
		else if(startsWith(toks[i], "profile_id"))
			{
			volatile char c = toks[i][sizeof("profile_id") - 1];
			unsigned int ndx = c - 0x30;
			i++;
			par.Parameters_s.profilesID[ndx] = atoi(toks[i].c_str());
			}
		}
	}
return true;
}

//-----------------------------------------------------------------------------
// AREA
//-----------------------------------------------------------------------------
/**
 * set the current area of a badge
 * @param badgeID
 * @param area
 * @return
 */
bool DBWrapper::setArea(int badgeID, int area)
{
int rc;
char *zErrMsg = 0;
string sql;

sql = "INSERT OR REPLACE INTO area "
		"(badge_id,current_area) "
		"VALUES ("
		"'" + to_string(badgeID) + "'" + "," + "'" + to_string(area) + "'" + ")";

//dbg->trace(DBG_DEBUG, "DBsql: " + sql);

rc = sqlite3_exec(evdb, sql.c_str(), callback, 0, &zErrMsg);
__fpurge(stdout);
if(rc != SQLITE_OK)
	{
	dbg->trace(DBG_FATAL, "SQL error: %s (%s)", zErrMsg, sql.c_str());
	sqlite3_free(zErrMsg);
	return false;
	}
else
	{
	dbg->trace(DBG_NOTIFY, "area add/update success");
	}
return true;
}

/**
 * get the current area of a badge
 * @param badgeID
 * @return -1 on error
 */
int DBWrapper::getArea(int badgeID)
{
int rc;
char *zErrMsg = 0;
string sql, sres;
int area = -1;

sql = "SELECT current_area FROM area WHERE badge_id='" + to_string(badgeID) + "'";
rc = sqlite3_exec(evdb, sql.c_str(), callback_res_monostring, &sres, &zErrMsg);
__fpurge(stdout);
if(rc != SQLITE_OK)
	{
	gd.globalErrors++;
	dbg->trace(DBG_FATAL, "SQL error: %s (%s)", zErrMsg, sql.c_str());
	sqlite3_free(zErrMsg);
	return false;
	}
else
	{
	if(sres != "NULL" && !sres.empty())
		{
		area = (uint8_t) to_number<int>(sres);
		}
	else
		{
		dbg->trace(DBG_ERROR, "badge id not found in area table");
		area = -1;
		}
	}
return area;
}
//-----------------------------------------------------------------------------
// USERS
//-----------------------------------------------------------------------------

///**
// * set the user in the user database
// * @param badge_id
// * @param name1
// * @param name2
// * @return
// */
//bool DBWrapper::setUser(int badge_id,string &name1, string &name2)
//{
//int rc;
//char *zErrMsg = 0;
//string sql;
//
//
//sql="INSERT OR REPLACE INTO user " \
//		"(id,first_name,second_name,badge_id) " \
//		"VALUES (" \
//		"'" + _S "0" + "'" + "," +
//		"'" + name1 + "'" +
//		"'" + name2 + "'" +
//		"'" + to_string(badge_id) + "'" +
//		")";
//
////dbg->trace(DBG_DEBUG, "DBsql: " + sql);
//
//rc = sqlite3_exec(svdb, sql.c_str(), callback, 0, &zErrMsg);
//__fpurge(stdout);
//if (rc != SQLITE_OK)
//	{
//	dbg->trace(DBG_FATAL, "SQL error: %s (%s)", zErrMsg,sql.c_str());
//	sqlite3_free(zErrMsg);
//	return false;
//	}
//else
//	{
//	dbg->trace(DBG_NOTIFY, "user add/update success");
//	}
//return true;
//}

/**
 * retrieve the user id from badge id
 * @param badge_id
 * @return user_id or -1 of not found
 */
int DBWrapper::getUserId(int badge_id)
{
int rc;
char *zErrMsg = 0;
string sql;
string sres;
int user_id = -1;

sql = (string) "SELECT user_id FROM userbadge WHERE badge_id='" + to_string(badge_id) + "'";

//dbg->trace(DBG_DEBUG, "DBsql: " + sql);

rc = sqlite3_exec(svdb, sql.c_str(), callback_res_monostring, &sres, &zErrMsg);
__fpurge(stdout);
if(rc != SQLITE_OK)
	{
	dbg->trace(DBG_FATAL, "SQL error: %s (%s)", zErrMsg, sql.c_str());
	sqlite3_free(zErrMsg);
	return false;
	}
else
	{
	dbg->trace(DBG_NOTIFY, "query to retrieve user id from badge data success");
	if(sres != "NULL" && !sres.empty())
		{
		user_id = to_number<int>(sres);
		}
	else
		{
		dbg->trace(DBG_ERROR, "user id not found");
		user_id = -1;
		}
	}
return user_id;
}

/**
 * retrieve the user names from user id
 * @param user_id
 * @param name1
 * @param name2
 * @return
 */
bool DBWrapper::getUserName(int user_id, string &name1, string &name2)
{
int rc;
char *zErrMsg = 0;
bool found = false;
string sql;
vector<string> userdata, toks;

sql = (string) "SELECT first_name,second_name FROM user WHERE id='" + to_string(user_id) + "'";

//dbg->trace(DBG_DEBUG, "DBsql: " + sql);

rc = sqlite3_exec(svdb, sql.c_str(), callback_vec_string, &userdata, &zErrMsg);
__fpurge(stdout);
if(rc != SQLITE_OK)
	{
	dbg->trace(DBG_FATAL, "SQL error: %s (%s)", zErrMsg, sql.c_str());
	sqlite3_free(zErrMsg);
	return false;
	}
else
	{
	dbg->trace(DBG_NOTIFY, "query to retrieve badge data success");
	FOR_EACH(it,string,userdata){
	toks.clear();
	Split(*it,toks,";=");
	for(unsigned int i=0;i<toks.size();i++)
		{
		if(toks[i]=="first_name")
			{
			i++;
			name1=toks[i];
			found=true;
			}
		if(toks[i]=="second_name")
			{
			i++;
			found=true;
			name2=toks[i];
			}
		}
	}
}
return found;
}

//-----------------------------------------------------------------------------
// TERMINALS
//-----------------------------------------------------------------------------
/**
 * set all terminals to default
 * @param onlyMemory only memory update
 * @return true:ok
 */
bool DBWrapper::setTerminalDefault(bool onlyMemory)
{
// not yet implemented
return false;
}

/**
 * retrieve all terminal data
 * @param id
 * @param par
 * @return
 */
bool DBWrapper::loadTerminals()
{
int rc;
char *zErrMsg = 0;
string sql;
vector<string> userdata, toks;
int cnt = 0;

sql = (string) "SELECT * FROM terminal";

//dbg->trace(DBG_DEBUG, "DBsql: " + sql);

rc = sqlite3_exec(svdb, sql.c_str(), callback_vec_string, &userdata, &zErrMsg);
__fpurge(stdout);
if(rc != SQLITE_OK)
	{
	gd.globalErrors++;
	dbg->trace(DBG_FATAL, "SQL error: %s (%s)", zErrMsg, sql.c_str());
	sqlite3_free(zErrMsg);
	return false;
	}
else
	{
	dbg->trace(DBG_NOTIFY, "query to retrieve terminal data success");
	if(userdata.size() > MAX_TERMINALS)
		{
		dbg->trace(DBG_ERROR, "too many terminals, excess will be ignored");
		}
	dbg->trace(DBG_NOTIFY, "%d terminals defined", userdata.size());
	if(userdata.size() > 0)
		{
		//TRACE(dbg,DBG_DEBUG,"query result: %d records",userdata.size());
		// find ID
		int id = 0;
		unsigned int i = 0;
		bool found = false;
		FOR_EACH(it,string,userdata){
		found=false;
		toks.clear();
		Split(*it,toks,";=");
		i=0;
		//TRACE(dbg,DBG_DEBUG,*it);
		for(i=0;i<toks.size();i++)
			{
			if(toks[i]=="id")
				{
				i++;
				id=to_number<int>(toks[i])-1;
				gd.terminals[id].params.terminal_id=id+1;
				found=true;
				break;
				}
			}

		//cout << *it << endl;
		// retrive other data
		if(found && id>=0 && id<=MAX_TERMINALS)
			{
			for(i=0;i<toks.size();i++)
				{
				if(toks[i]=="name")
					{
					i++;
					if(toks[i].empty() || toks[i]=="NULL")	// if it has not a name, assign it to a default
						{
						char tmp[100];
						sprintf(tmp,"Terminal%d",id+1);
						strcpy(gd.terminals[id].name,tmp);
						}
					else
						{
						strcpy(gd.terminals[id].name,toks[i].c_str());
						}
					}
				else if(toks[i]=="status")
					{
					i++;
					gd.terminals[id].status=to_number<int>(toks[i]);
					// check if it was connected, if yes delete it
//						if(gd.terminals[id].status==TERMINAL_STATUS_NOT_PRESENT)
//							{
//							globals_st::iddevice_t dit=gd.deviceList.find(id);
//							if(dit != gd.deviceList.end())
//								{
//								TRACE(dbg,DBG_NOTIFY,"terminal %d that was connected, will be erased",id+1);
//								gd.deviceList.erase(dit);
//								}
//							}
					}
				else if(toks[i]=="antipassback")
					{
					i++;
					gd.terminals[id].params.antipassback=(uint8_t)to_number<int>(toks[i]);
					}
				else if(toks[i]=="access1")
					{
					i++;
					gd.terminals[id].params.access_door1=(AccessType_t)to_number<int>(toks[i]);
					}
				else if(toks[i]=="access2")
					{
					i++;
					gd.terminals[id].params.access_door2=(AccessType_t)to_number<int>(toks[i]);
					}
				else if(toks[i]=="entrance_type")
					{
					i++;
					gd.terminals[id].params.entranceType=(EntranceType_t)to_number<int>(toks[i]);
					}
				else if(toks[i]=="weektime_id")
					{
					i++;
					gd.terminals[id].params.weektimeId=(uint8_t)to_number<int>(toks[i]);
					}
				else if(toks[i]=="open_door_time")
					{
					i++;
					gd.terminals[id].params.openDoorTime=(uint8_t)to_number<int>(toks[i]);
					}
				else if(toks[i]=="open_door_timeout")
					{
					i++;
					gd.terminals[id].params.openDoorTimeout=(uint8_t)to_number<int>(toks[i]);
					}
				else if(toks[i]=="area1_reader1")
					{
					i++;
					gd.terminals[id].params.area1_reader1=(uint8_t)to_number<int>(toks[i]);
					}
				else if(toks[i]=="area2_reader1")
					{
					i++;
					gd.terminals[id].params.area2_reader1=(uint8_t)to_number<int>(toks[i]);
					}
				else if(toks[i]=="area1_reader2")
					{
					i++;
					gd.terminals[id].params.area1_reader2=(uint8_t)to_number<int>(toks[i]);
					}
				else if(toks[i]=="area2_reader2")
					{
					i++;
					gd.terminals[id].params.area2_reader2=(uint8_t)to_number<int>(toks[i]);
					}
				else if(toks[i]=="MAC_addr")
					{
					i++;
					if(!toks[i].empty())
						{
						sscanf(toks[i].c_str(),"%2hhx.%2hhx.%2hhx.%2hhx.%2hhx.%2hhx", &gd.terminals[id].params.mac[0],
								&gd.terminals[id].params.mac[1],
								&gd.terminals[id].params.mac[2],
								&gd.terminals[id].params.mac[3],
								&gd.terminals[id].params.mac[4],
								&gd.terminals[id].params.mac[5]);
						}
					}
				else if(toks[i]=="IP_addr")
					{
					i++;
					if(!toks[i].empty())
						{
						sscanf(toks[i].c_str(),"%hhd.%hhd.%hhd.%hhd", &gd.terminals[id].params.terminal_ip[0],
								&gd.terminals[id].params.terminal_ip[1],
								&gd.terminals[id].params.terminal_ip[2],
								&gd.terminals[id].params.terminal_ip[3]);
						}
					}
				else if(toks[i]=="fw_ver")
					{
					i++;
					vector<string> ver;
					Split(toks[i],ver,".");
					if(ver.size()==2)
						{
						gd.terminals[id].params.terminal_fw_ver.major=(uint8_t)to_number<int>(ver[0]);
						gd.terminals[id].params.terminal_fw_ver.minor=(uint8_t)to_number<int>(ver[1]);
						}
					}
				else if(toks[i]=="hw_ver")
					{
					i++;
					gd.terminals[id].params.terminal_hw_ver=(HWVersion_e)to_number<int>(toks[i]);
					}
				}
			}
		}
	}
}
return true;
}

/**
 * remove a terminal
 * @param id
 * @return
 * NOT USED: use setTerminalStatus
 */
bool DBWrapper::removeTerminal(int id)
{
int rc;
char *zErrMsg = 0;
string sql;
vector<string> userdata;

sql = (string) "DELETE FROM terminal * WHERE id='" + to_string(id) + "'";

//dbg->trace(DBG_DEBUG, "DBsql: " + sql);

rc = sqlite3_exec(svdb, sql.c_str(), callback, &userdata, &zErrMsg);
__fpurge(stdout);
if(rc != SQLITE_OK)
	{
	dbg->trace(DBG_FATAL, "SQL error: %s (%s)", zErrMsg, sql.c_str());
	sqlite3_free(zErrMsg);
	return false;
	}
else
	{
	dbg->trace(DBG_NOTIFY, "query to remove terminal success");
	// FIX: handle data!
	}
return true;
}

/**
 * set the terminal status
 * @param terminalId
 * @param status status that I want to set for all and can be TERMINAL_STATUS_NOT_PRESENT,TERMINAL_STATUS_ON_LINE,TERMINAL_STATUS_OFF_LINE
 * @return
 */
bool DBWrapper::cleanTerminalStatus(int status)
{
int rc;
char *zErrMsg = 0;
string sql;
string actTerm;
uint64_t temp_at;

sql = (string) "UPDATE terminal SET "
		"status = " + "'" + to_string(status) + "'" + " WHERE status > '" + to_string(TERMINAL_STATUS_NOT_PRESENT) + "'";

//dbg->trace(DBG_DEBUG, "DBsql: " + sql);

rc = sqlite3_exec(svdb, sql.c_str(), callback, 0, &zErrMsg);
__fpurge(stdout);
if(rc != SQLITE_OK)
	{
	TRACE(dbg, DBG_FATAL, "SQL error: %s (%s)", zErrMsg, sql.c_str());
	sqlite3_free(zErrMsg);
	return false;
	}
else
	{
	char ststr[][15] =
		{
		"NotPresent", "OnLine", "OffLine"
		};
	TRACE(dbg, DBG_NOTIFY, "all terminals status set to [%s] success", ststr[status]);
	}
return true;
}

/**
 * clean the terminal name
 * @param terminalId
 * @param name name used to clean
 * @return
 */
bool DBWrapper::cleanTerminalName(string name)
{
int rc;
char *zErrMsg = 0;
string sql;
string actTerm;
uint64_t temp_at;

sql = (string) "UPDATE terminal SET "
		"name = " + "'" + name + "'";

//dbg->trace(DBG_DEBUG, "DBsql: " + sql);

rc = sqlite3_exec(svdb, sql.c_str(), callback, 0, &zErrMsg);
__fpurge(stdout);
if(rc != SQLITE_OK)
	{
	TRACE(dbg, DBG_FATAL, "SQL error: %s (%s)", zErrMsg, sql.c_str());
	sqlite3_free(zErrMsg);
	return false;
	}
else
	{
	char ststr[][15] =
		{
		"NotPresent", "OnLine", "OffLine"
		};
	TRACE(dbg, DBG_NOTIFY, "all terminals name set");
	}
return true;
}

/**
 * set the terminal status
 * @param terminalId
 * @param status can be TERMINAL_STATUS_NOT_PRESENT,TERMINAL_STATUS_ON_LINE,TERMINAL_STATUS_OFF_LINE
 * @return
 */
bool DBWrapper::setTerminalStatus(int terminalId, int status)
{
int rc;
char *zErrMsg = 0;
string sql;
string actTerm;
uint64_t temp_at;

if(terminalId == 0)
	{
	TRACE(dbg, DBG_WARNING, "invalid terminal ID (0)");
	return false;
	}

sql = (string) "UPDATE terminal SET "
		"status = " + "'" + to_string(status) + "'" + " WHERE id = '" + to_string(terminalId) + "'";

//dbg->trace(DBG_DEBUG, "DBsql: " + sql);

rc = sqlite3_exec(svdb, sql.c_str(), callback, 0, &zErrMsg);
__fpurge(stdout);
if(rc != SQLITE_OK)
	{
	TRACE(dbg, DBG_FATAL, "SQL error: %s (%s)", zErrMsg, sql.c_str());
	sqlite3_free(zErrMsg);
	return false;
	}
else
	{
	TRACE(dbg, DBG_NOTIFY, "terminal status update success");
	}
return true;
}

/**
 * retrieve terminal data
 * @param terminalId
 * @return status
 */
int DBWrapper::getTerminalStatus(int terminalId)
{
int rc;
char *zErrMsg = 0;
string sql;
string sres, toks;
int st = -1;

if(terminalId == 0)
	{
	TRACE(dbg, DBG_WARNING, "invalid terminal ID (0)");
	return -1;
	}

sql = (string) "SELECT status WHERE terminal='" + to_string(terminalId) + "' FROM terminal";

//dbg->trace(DBG_DEBUG, "DBsql: " + sql);

rc = sqlite3_exec(evdb, sql.c_str(), callback_res_monostring, &sres, &zErrMsg);
__fpurge(stdout);
if(rc != SQLITE_OK)
	{
	TRACE(dbg, DBG_FATAL, "SQL error: %s (%s)", zErrMsg, sql.c_str());
	sqlite3_free(zErrMsg);
	return -1;
	}
else
	{
	if(sres != "NULL" && !sres.empty())
		{
		st = to_number<int>(sres);
		char ststr[][15] =
			{
			"NotPresent", "OnLine", "OffLine"
			};
		TRACE(dbg, DBG_NOTIFY, "terminal %d current status %s", terminalId, ststr[st]);
		}
	}
return st;
}

/**
 * set the terminal data
 * @param id
 * @param name
 * @param par
 * @return
 */
bool DBWrapper::setTerminal(string name, int status, Terminal_parameters_to_send_t& par)
{
int rc;
char *zErrMsg = 0;
string sql;
char mac[20], ip[20];

if(par.terminal_id == 0)
	{
	dbg->trace(DBG_WARNING, "invalid terminal ID (0)");
	return false;
	}

sprintf(mac, "%02x.%02x.%02x.%02x.%02x.%02x", par.mac[0], par.mac[1], par.mac[2], par.mac[3], par.mac[4], par.mac[5]);
sprintf(ip, "%d.%d.%d.%d", par.terminal_ip[0], par.terminal_ip[1], par.terminal_ip[2], par.terminal_ip[3]);

if(name.empty() || name == "")
	{
	sql = "UPDATE terminal SET "
			"status = '" + to_string((int) status) + "'" + "," + "antipassback = '" + to_string((int) par.antipassback) + "'" + "," + "access1 = '" + to_string((int) par.access_door1) + "'" + "," + "access2 = '" + to_string((int) par.access_door2) + "'" + "," + "entrance_type = '" + to_string((int) par.entranceType) + "'" + "," + "weektime_id = '" + to_string((int) par.weektimeId) + "'" + "," + "open_door_time = '" + to_string((int) par.openDoorTime) + "'" + "," + "open_door_timeout = '" + to_string((int) par.openDoorTimeout) + "'" + "," + "area1_reader1 = '" + to_string((int) par.area1_reader1) + "'" + "," + "area2_reader1 = '" + to_string((int) par.area2_reader1) + "'" + "," + "area1_reader2 = '" + to_string((int) par.area1_reader2) + "'" + "," + "area2_reader2 = '" + to_string((int) par.area2_reader2) + "'" + "," + "MAC_addr = '" + mac + "'" + "," + "IP_addr = '" + ip + "'" + "," + "fw_ver = '" + to_string((int) par.terminal_fw_ver.major) + "." + to_string((int) par.terminal_fw_ver.minor) + "'" + "," + "hw_ver = '" + to_string((int) par.terminal_hw_ver) + "'" + " WHERE id = '" + to_string((int) par.terminal_id) + "'";
	}
else
	{
	sql = "UPDATE terminal SET "
			"name = '" + name + "'" + "," + "status = '" + to_string((int) status) + "'" + "," + "antipassback = '" + to_string((int) par.antipassback) + "'" + "," + "access1 = '" + to_string((int) par.access_door1) + "'" + "," + "access2 = '" + to_string((int) par.access_door2) + "'" + "," + "entrance_type = '" + to_string((int) par.entranceType) + "'" + "," + "weektime_id = '" + to_string((int) par.weektimeId) + "'" + "," + "open_door_time = '" + to_string((int) par.openDoorTime) + "'" + "," + "open_door_timeout = '" + to_string((int) par.openDoorTimeout) + "'" + "," + "area1_reader1 = '" + to_string((int) par.area1_reader1) + "'" + "," + "area2_reader1 = '" + to_string((int) par.area2_reader1) + "'" + "," + "area1_reader2 = '" + to_string((int) par.area1_reader2) + "'" + "," + "area2_reader2 = '" + to_string((int) par.area2_reader2) + "'" + "," + "MAC_addr = '" + mac + "'" + "," + "IP_addr = '" + ip + "'" + "," + "fw_ver = '" + to_string((int) par.terminal_fw_ver.major) + "." + to_string((int) par.terminal_fw_ver.minor) + "'" + "," + "hw_ver = '" + to_string((int) par.terminal_hw_ver) + "'" + " WHERE id = '" + to_string((int) par.terminal_id) + "'";
	}

//dbg->trace(DBG_DEBUG, "DBsql: " + sql);
TRACE(dbg, DBG_NOTIFY, "terminal %d firmware ver. %s", (int )par.terminal_id, (to_string((int )par.terminal_fw_ver.major) + "." + to_string((int )par.terminal_fw_ver.minor)).c_str());
string hw_ver;
hw_ver = ((int )par.terminal_hw_ver == 0) ? "2rele" : "3rele";
TRACE(dbg, DBG_NOTIFY, "terminal %d hardware ver. %s", (int )par.terminal_id, hw_ver.c_str());
rc = sqlite3_exec(svdb, sql.c_str(), callback, 0, &zErrMsg);
__fpurge(stdout);
if(rc != SQLITE_OK)
	{
	dbg->trace(DBG_FATAL, "SQL error: %s (%s)", zErrMsg, sql.c_str());
	sqlite3_free(zErrMsg);
	return false;
	}
else
	{
	dbg->trace(DBG_NOTIFY, "terminal add/update success");
	}
return true;
}

/**
 * retrieve terminal data
 * @param id
 * @param par
 * @return
 */
bool DBWrapper::getTerminal(int id, globals_st::terminal_t& par)
{
int rc;
char *zErrMsg = 0;
string sql;
vector<string> userdata, toks;

sql = (string) "SELECT * WHERE terminal='" + to_string(id) + "' FROM terminal";

//dbg->trace(DBG_DEBUG, "DBsql: " + sql);

rc = sqlite3_exec(svdb, sql.c_str(), callback_vec_string, &userdata, &zErrMsg);
__fpurge(stdout);
if(rc != SQLITE_OK)
	{
	dbg->trace(DBG_FATAL, "SQL error: %s (%s)", zErrMsg, sql.c_str());
	sqlite3_free(zErrMsg);
	return false;
	}
else
	{
	dbg->trace(DBG_NOTIFY, "query to retrieve terminal %d data success", id);
	if(userdata.size() != 1)
		{
		dbg->trace(DBG_ERROR, "terminal not found or not unique");
		}
	else
		{
		Split(userdata[0], toks, ";=");
		for(unsigned int i = 0;i < toks.size();i++)
			{
			if(toks[i] == "name")
				{
				i++;
				strcpy(par.name, toks[i].c_str());
				}
			else if(toks[i] == "antipassback")
				{
				i++;
				par.params.antipassback = to_number<uint8_t>(toks[i]);
				}
			else if(toks[i] == "access")
				{
				i++;
				par.params.access_door1 = (AccessType_t) to_number<uint8_t>(toks[i]);
				}
			else if(toks[i] == "weektime_id")
				{
				i++;
				par.params.weektimeId = to_number<uint8_t>(toks[i]);
				}
			// TODO complete this function
			}
		}
	}
dbg->trace(DBG_WARNING, "%s not fully implemented", __FUNCTION__);
return true;
}

//*****************************************************************************

//-----------------------------------------------------------------------------
// PRIVATE
//-----------------------------------------------------------------------------
/**
 * convert into string the uint64_t booleans
 * @param at
 * @param str
 */
void DBWrapper::activeTerminal2string(uint64_t &at, string &str)
{
uint64_t msk = 1ULL << 63;
str.clear();
char c;
for(int i = 0;i < 64;i++)
	{
	(at & msk) ? (c = 'A') : (c = 'I');
	str += c;
	msk >>= 1;
	}
}

/**
 *  * convert uint64_t booleans the string
 * @param str
 * @param at
 */
void DBWrapper::string2activeTerminal(string &str, uint64_t &at)
{
uint64_t msk = 1ULL << 63;
at = 0ULL;
for(unsigned int i = 0;i < str.length();i++)
	{
	(str.at(i) == 'A') ? (at |= msk) : (at &= ~msk);
	msk >>= 1;
	}
}

/**
 * convert in a long strings for the database
 * @param ndx
 * @param data
 */
void DBWrapper::weektime2str(int ndx, char *data)
{
char tmp[5], sep;

// create one string
data[0] = 0;
for(int d = 0;d < WT_ARRAY_SIZE;d++)
	{
	((d % 2) == 0) ? (sep = ':') : (sep = '|');
	sprintf(tmp, "%02d%c", gd.weektimes[ndx].params.Parameters_s.week.weekdays[d], sep);
	strcat(data, tmp);
	}
}

/**
 * Update full CRC of Profiles
 */
void DBWrapper::updateProfilesCRC(void)
{
gd.globProfilesCRC = CRC8_INIT;
for(int i = 0;i < MAX_PROFILES;i++)
	{
	gd.globProfilesCRC = crc8(gd.profiles[i].params.crc8, &gd.globProfilesCRC);
	}
dbg->trace(DBG_DEBUG, "Profiles CRC = %02X", (unsigned int) gd.globProfilesCRC);
}

/**
 * Update full CRC of Weektime
 */
void DBWrapper::updateWeektimesCRC(void)
{
gd.globWeektimesCRC = CRC8_INIT;
for(int i = 0;i < MAX_WEEKTIMES;i++)
	{
	gd.globWeektimesCRC = crc8(gd.weektimes[i].params.crc8, &gd.globWeektimesCRC);
	asm("nop");
	}
dbg->trace(DBG_DEBUG, "Weektimes CRC = %02X", (unsigned int) gd.globWeektimesCRC);
}

/**
 * Update full CRC of Causal
 */
void DBWrapper::updateCausalCRC(void)
{
gd.globCausalsCRC = CRC8_INIT;
for(int i = 0;i < MAX_CAUSAL_CODES;i++)
	{
	gd.globCausalsCRC = crc8(gd.causals[i].crc8, &gd.globCausalsCRC);
	asm("nop");
	}
dbg->trace(DBG_DEBUG, "Causals CRC = %02X", (unsigned int) gd.globCausalsCRC);
}

/**
 * this routine check the max number of rows and if > of a threshold, copy the entire database into a backup
 * and clear the event table
 * @param ev
 * @return
 */
bool DBWrapper::checkEventTable()
{
int rc;
char *zErrMsg = 0;
string sql, sres;

sql = "SELECT COUNT(*) FROM history";
rc = sqlite3_exec(evdb, sql.c_str(), callback_res_monostring, &sres, &zErrMsg);
__fpurge(stdout);
if(rc != SQLITE_OK)
	{
	dbg->trace(DBG_FATAL, "SQL error: %s (%s)", zErrMsg, sql.c_str());
	sqlite3_free(zErrMsg);
	return false;
	}
else
	{
	if(sres != "NULL" && !sres.empty())
		{
		int n;
		string bkpfname = "", bkpfname_old = "", cmd;
		int nevents = to_number<int>(sres);
		dbg->trace(DBG_NOTIFY, "events stored: %d", nevents);
		if(nevents > gd.maxEventPerTable)
			{
			TRACE(dbg, DBG_NOTIFY, "too many events -> backup and clear the actual table...");

			// close and backup
			sqlite3_close(evdb);
			__fpurge(stdout);

			// search file name to be used
			for(n = 0;n < MAX_EVENT_BACKUP_FILES;n++)
				{
				bkpfname = _S DB_EVENT_BKP_PATH + _S DB_VARCOLANEV + _S "." + to_string(n);
				if(!fileExists(bkpfname))
					{
					break;
					}
				else
					{
					if(n == (MAX_EVENT_BACKUP_FILES - 1))
						{
						// restart the count
						n = 0;
						bkpfname = _S DB_EVENT_BKP_PATH + _S DB_VARCOLANEV + _S "." + to_string(n);
						}
					}
				}
			// determines the file to be deleted for circular buffer
			n++;
			n %= MAX_EVENT_BACKUP_FILES;
			bkpfname_old = _S DB_EVENT_BKP_PATH + _S DB_VARCOLANEV + _S "." + to_string(n);
			if(fileExists(bkpfname_old))
				{
				TRACE(dbg, DBG_NOTIFY, "removing the older backup file " + bkpfname_old);
				cmd = "rm " + bkpfname_old;
				}

			cmd = "cp " + evfname + " " + bkpfname;
			system(cmd.c_str());
			// reopen and clear events table
			rc = sqlite3_open(evfname.c_str(), &evdb);
			__fpurge(stdout);
			if(rc)
				{
				dbg->trace(DBG_FATAL, "can't open database: %s", sqlite3_errmsg(evdb));
				sqlite3_close(evdb);
				return false;
				}
			TRACE(dbg, DBG_NOTIFY, "event database reopened successfully");
#if 0
			// delete all
			TRACE(dbg,DBG_NOTIFY, "delete all events data...");
			exeSQL(evdb,"DELETE FROM history;VACUUM;");
#else
			// delete about 1/3 of the oldest data
			string earlier = getHistoryDateLimits('e');
			string latest = getHistoryDateLimits('l');
			//TRACE(dbg,DBG_NOTIFY, "earlier event: " + earlier + "; latest event: " + latest);
			time_t ear = datetime2epoch(earlier);
			time_t lat = datetime2epoch(latest);
			time_t mid = lat / 2 + ear / 2;
			string limit = epoch2datetime(mid);
			TRACE(dbg, DBG_NOTIFY, "delete events older than " + limit);
			exeSQL(evdb, "DELETE FROM history WHERE timestamp <= '" + limit + "';");
			exeSQL(evdb, "VACUUM;");
#endif
			}
		}
	else
		{
		dbg->trace(DBG_NOTIFY, "no entry found in history table");
		}
	}
return true;
}

/**
 * datetime as in sqlite3 to epoch time converter
 * @param dt datetime string (i.e. "2015-12-09 23:59:59")
 * @return epoch time
 */
time_t DBWrapper::datetime2epoch(string dt)
{
vector<string> toks1;

//"2015-12-09 23:59:59"
toks1.clear();
Split(dt, toks1, "- :");
struct tm t =
	{
	0
	};
t.tm_year = to_number<int>(toks1[0]) - 1900;
t.tm_mon = to_number<int>(toks1[1]) - 1;
t.tm_mday = to_number<int>(toks1[2]);
t.tm_hour = to_number<int>(toks1[3]);
t.tm_min = to_number<int>(toks1[4]);
t.tm_sec = to_number<int>(toks1[5]);
return mktime(&t);
}

/**
 * epoch time to datetime as in sqlite3 converter
 * @param ep epoch time
 * @return string as in sqlite3 (i.e. "2015-12-09 23:59:59")
 */
string DBWrapper::epoch2datetime(time_t ep)
{
struct tm * timeinfo;
char sqlTime[80];

timeinfo = localtime(&ep);
strftime(sqlTime, 80, "%Y-%m-%d %H:%M:%S", timeinfo);
return _S sqlTime;
}

/**
 * return the latest or the earlier date in the history
 * @param latest_earlier
 * @return
 */
string DBWrapper::getHistoryDateLimits(char latest_earlier)
{
int rc;
char *zErrMsg = 0;
string sql;
string sres, toks;
char choice = toupper(latest_earlier);
if(choice == 'E')
	{
	sql = (string) "SELECT MIN(timestamp) from history";
	}
else
	{
	sql = (string) "SELECT MAX(timestamp) from history";
	}

//dbg->trace(DBG_DEBUG, "DBsql: " + sql);

rc = sqlite3_exec(evdb, sql.c_str(), callback_res_monostring, &sres, &zErrMsg);
__fpurge(stdout);
if(rc != SQLITE_OK)
	{
	TRACE(dbg, DBG_FATAL, "SQL error: %s (%s)", zErrMsg, sql.c_str());
	sqlite3_free(zErrMsg);
	return "";
	}
else
	{
	if(sres != "NULL" && !sres.empty())
		{
		if(choice == 'E')
			{
			TRACE(dbg, DBG_NOTIFY, "earlier event in history is at " + sres);
			}
		else
			{
			TRACE(dbg, DBG_NOTIFY, "latest event in history is at " + sres);
			}
		}
	}
return sres;
}

/**
 * add a new event in the history
 * @param ev
 * @return
 */
bool DBWrapper::addEvent(Event_t &ev)
{
int rc;
char *zErrMsg = 0;
string sql;
int user_id;
string name1, name2;

//time_t rawtime=ev.Parameters_s.timestamp;
//struct tm * timeinfo;
//char sqlTime [80];
//timeinfo = localtime ( &rawtime );
//strftime (sqlTime,80,"%Y-%m-%d %H:%M:%S",timeinfo);
string sqlTime = epoch2datetime(ev.Parameters_s.timestamp);

user_id = getUserId(ev.Parameters_s.idBadge);
if(user_id >= 0)
	{
	getUserName(user_id, name1, name2);
	TRACEF(dbg, DBG_NOTIFY, "event from user id %d (%s %s)", user_id, name1.c_str(), name2.c_str());
	}
else
	{
	name1 = name2 = "";
	}

sql = "INSERT INTO history "
		"(timestamp,terminal_id,event,badge_id,area,causal_code,user_first_name,user_second_name,synchronised) "
		"VALUES ("
		"'" + _S sqlTime + "'" + "," + "'" + to_string((int) ev.Parameters_s.terminalId) + "'" + "," + "'" + to_string((int) ev.Parameters_s.eventCode) + "'" + "," + "'" + to_string((int) ev.Parameters_s.idBadge) + "'" + "," + "'" + to_string((int) ev.Parameters_s.area) + "'" + "," + "'" + to_string((int) ev.Parameters_s.causal_code) + "'" + "," + "'" + name1 + "'" + "," + "'" + name2 + "'" + "," + "'" + (
    (ev.Parameters_s.isSynced) ? (_S "1") : (_S "0")) + "'" + ")";

//dbg->trace(DBG_DEBUG, "DBsql: " + sql);

rc = sqlite3_exec(evdb, sql.c_str(), callback, 0, &zErrMsg);
__fpurge(stdout);
if(rc != SQLITE_OK)
	{
	dbg->trace(DBG_FATAL, "SQL error: %s (%s)", zErrMsg, sql.c_str());
	sqlite3_free(zErrMsg);
	return false;
	}
else
	{
	dbg->trace(DBG_NOTIFY, "event history add success");
	}

/* Update evento to sync counter variable */
updateUnsyncedEvents();

return true;
}

/**
 * counts the unsynchronised events
 * @return counts of events
 */
void DBWrapper::updateUnsyncedEvents()
{
unsigned int cnt = 0;
int rc;
char *zErrMsg = 0;
string sql, sres;

sql = "SELECT COUNT(*) FROM history WHERE synchronised = 0";

rc = sqlite3_exec(evdb, sql.c_str(), callback_res_monostring, &sres, &zErrMsg);
__fpurge(stdout);
if(rc != SQLITE_OK)
	{
	dbg->trace(DBG_FATAL, "SQL error: %s (%s)", zErrMsg, sql.c_str());
	sqlite3_free(zErrMsg);
	}
else
	{
	if(sres != "NULL" && !sres.empty())
		{
		cnt = to_number<unsigned int>(sres);
		dbg->trace(DBG_NOTIFY, "%d unsynchronised events found", cnt);
		}
	}

gd.unsynckedEvent = cnt;
}

/**
 * return the first found unsynchronised event
 * @param ev_rowid row id for the event
 * @param ev event
 * @return conts of unsynchronised events
 */
bool DBWrapper::getUnsyncedEvent(uint64_t &ev_rowid, Event_t &ev)
{
int rc;
char *zErrMsg = 0;
struct tm * timeinfo;
char sqlTime[80];
time_t rawtime;
string sql, sres;

// convert epoch times

sql = (string) "SELECT rowid,timestamp,terminal_id,event,badge_id,area,causal_code,user_first_name,user_second_name FROM history WHERE synchronised = 0 LIMIT 1";
//dbg->trace(DBG_DEBUG, "DBsql: " + sql);

rc = sqlite3_exec(evdb, sql.c_str(), callback_res_monostring, &sres, &zErrMsg);
__fpurge(stdout);
if(rc != SQLITE_OK)
	{
	TRACE(dbg, DBG_FATAL, "SQL error: %s (%s)", zErrMsg, sql.c_str());
	sqlite3_free(zErrMsg);
	return false;
	}
else
	{
	if(sres != "NULL" && !sres.empty())
		{
		toks.clear();
		Split(sres, toks, ";");

		ev_rowid = to_number<uint64_t>(toks[0]);

		ev.Parameters_s.timestamp = datetime2epoch(toks[1]);
		ev.Parameters_s.terminalId = to_number<int>(toks[2]);
		ev.Parameters_s.eventCode = (EventCode_t) to_number<int>(toks[3]);
		ev.Parameters_s.idBadge = to_number<int>(toks[4]);
		ev.Parameters_s.area = to_number<int>(toks[5]);
		ev.Parameters_s.causal_code = to_number<int>(toks[6]);
		}
	else
		{
		TRACE(dbg, DBG_NOTIFY, "no unsynchronised events found");
		}
	}
return true;
}

/**
 * set the synch flag
 * @param rowid
 * @param syncStatus
 * @return
 */
bool DBWrapper::setEventSyncStatus(uint64_t rowid, bool syncStatus)
{
// TODO
int rc;
char *zErrMsg = 0;
string sql;

sql = (string) "UPDATE history SET synchronised = '" + to_string((int) syncStatus) + "'" + " WHERE rowid = '" + to_string((int) rowid) + "'";

rc = sqlite3_exec(evdb, sql.c_str(), callback, 0, &zErrMsg);
__fpurge(stdout);
if(rc != SQLITE_OK)
	{
	TRACE(dbg, DBG_FATAL, "SQL error: %s (%s)", zErrMsg, sql.c_str());
	sqlite3_free(zErrMsg);
	return false;
	}
else
	{
	TRACE(dbg, DBG_NOTIFY, "terminal status update success");
	}

updateUnsyncedEvents();
return true;

}

/**
 * retrieve events from the database
 * @param evreq event struct containing the request data
 * @param evt result output
 * @return total events found in the specified range by evreq data
 */
unsigned int DBWrapper::getEvents(EventRequest_t *evreq, vector<Event_t> *evt)
{
unsigned int totev = 0;

int rc;
char *zErrMsg = 0;
struct tm * timeinfo;
char sqlTime[80];
time_t rawtime;
string sql;

// convert epoch times
string starttime = epoch2datetime(evreq->epochtime_start);
string stoptime = epoch2datetime(evreq->epochtime_stop);

sql = (string) "SELECT timestamp,terminal_id,event,badge_id,area,causal_code FROM history WHERE badge_id='" + to_string(evreq->id_badge) + "' AND timestamp BETWEEN '" + starttime + "' AND '" + stoptime + "'";
//dbg->trace(DBG_DEBUG, "DBsql: " + sql);

querydata.clear();
rc = sqlite3_exec(evdb, sql.c_str(), callback_vec_string, &querydata, &zErrMsg);
__fpurge(stdout);
if(rc != SQLITE_OK)
	{
	dbg->trace(DBG_FATAL, "SQL error: %s (%s)", zErrMsg, sql.c_str());
	sqlite3_free(zErrMsg);
	return false;
	}
else
	{
	dbg->trace(DBG_NOTIFY, "query to retrieve events success");
	if(querydata.size() == 0)
		{
		dbg->trace(DBG_NOTIFY, "SQL: no events found");
		totev = 0;
		}
	else
		{
		totev = querydata.size();
		fillEventsData(evt);
		}
	}

return totev;
}

/**
 * retrieve maxcount events from the database
 * @param start_timestamp timestamp of the first event returnedo or after that
 * @param evt event vector
 * @param maxcount max number of events
 * @return total events effectiverly retrieved
 */
unsigned int DBWrapper::getNEvents(time_t start_timestamp, vector<Event_t> *evt, int maxcount)
{
unsigned int totev = 0;

int rc;
char *zErrMsg = 0;
struct tm * timeinfo;
char sqlTime[80];
time_t rawtime;
string sql;

// convert epoch times
string starttime = epoch2datetime(start_timestamp);

sql = (string) "SELECT timestamp,terminal_id,event,badge_id,area,causal_code FROM history WHERE timestamp >= '" + starttime + "' LIMIT " + to_string(maxcount);
//dbg->trace(DBG_DEBUG, "DBsql: " + sql);

querydata.clear();
rc = sqlite3_exec(evdb, sql.c_str(), callback_vec_string, &querydata, &zErrMsg);
__fpurge(stdout);
if(rc != SQLITE_OK)
	{
	dbg->trace(DBG_FATAL, "SQL error: %s (%s)", zErrMsg, sql.c_str());
	sqlite3_free(zErrMsg);
	return false;
	}
else
	{
	dbg->trace(DBG_NOTIFY, "query to retrieve events success");
	if(querydata.size() == 0)
		{
		dbg->trace(DBG_NOTIFY, "SQL: no events found");
		totev = 0;
		}
	else
		{
		totev = querydata.size();
		fillEventsData(evt);
		}
	}

return totev;
}

/**
 * fill events data
 */
unsigned int DBWrapper::fillEventsData(vector<Event_t> *evt)
{
Event_t e;
vector<string> toks1;

toks.clear();
evt->clear();
e.Parameters_s.isSynced = true;
if(querydata.size() > 0)
	{
	FOR_EACH(it,string,querydata){
	Split(*it,toks,";=");
	for(unsigned int i=0;i<toks.size();i++)
		{
		//timestamp,terminal_id,event,badge_id,area,causal_code
		if(toks[i]=="timestamp")
			{
			i++;
			//"2015-12-09 23:59:59"
			toks1.clear();
			Split(toks[i],toks1,"- :");
			struct tm t =
				{0};
			t.tm_year=to_number<int>(toks1[0])-1900;
			t.tm_mon=to_number<int>(toks1[1])-1;
			t.tm_mday=to_number<int>(toks1[2]);
			t.tm_hour=to_number<int>(toks1[3]);
			t.tm_min=to_number<int>(toks1[4]);
			t.tm_sec=to_number<int>(toks1[5]);
			e.Parameters_s.timestamp = mktime(&t);
			}
		else if(toks[i]=="terminal_id")
			{
			i++;
			e.Parameters_s.terminalId=(uint8_t)to_number<int>(toks[i]);
			}
		else if(toks[i]=="event")
			{
			i++;
			e.Parameters_s.eventCode=(EventCode_t)(uint8_t)to_number<int>(toks[i]);
			}
		else if(toks[i]=="badge_id")
			{
			i++;
			e.Parameters_s.idBadge=(uint16_t)to_number<int>(toks[i]);
			}
		else if(toks[i]=="area")
			{
			i++;
			e.Parameters_s.area=(uint8_t)to_number<int>(toks[i]);
			}
		else if(toks[i]=="causal_code")
			{
			i++;
			e.Parameters_s.causal_code=(uint16_t)to_number<int>(toks[i]);
			}
		else if(toks[i]=="synchronised")
			{
			i++;
			e.Parameters_s.isSynced=to_number<int>(toks[i]);
			}
		}
	evt->push_back(e);
	}
}
return querydata.size();
}

/**
 * executes a generic sql command
 * @param db database where to act
 * @param sql
 * @param specifies data to be retrieved
 * @return true:ok
 */
bool DBWrapper::exeSQL(sqlite3 *db, string sql)
{
int rc;
char *zErrMsg = 0;

dbg->trace(DBG_DEBUG, "DBsql: " + sql);
rc = sqlite3_exec(db, sql.c_str(), callback, 0, &zErrMsg);
__fpurge(stdout);
if(rc != SQLITE_OK)
	{
	dbg->trace(DBG_FATAL, "SQL error: %s (%s)", zErrMsg, sql.c_str());
	sqlite3_free(zErrMsg);
	return false;
	}
else
	{
	dbg->trace(DBG_NOTIFY, "sql command executed successfully");
	}
return true;
}

#if 0
/**
 * get a command from the database as web actions
 * @return true: has a command
 */
bool DBWrapper::getCommand()
	{
	int rc;
	char *zErrMsg = 0;
	string command;
	string sql;

	sql="SELECT rowid, command FROM actions WHERE status=1 ORDER BY rowid LIMIT 1";
	rc = sqlite3_exec(evdb, sql.c_str(), callback_res_monostring, &command, &zErrMsg);
	__fpurge(stdout);
	if (rc != SQLITE_OK)
		{
		dbg->trace(DBG_FATAL, "SQL error: %s (%s)", zErrMsg,sql.c_str());
		sqlite3_free(zErrMsg);
		return false;
		}
	else
		{
		if(!command.empty())
			{
			// there are some commands to be executed
			toks.clear();
			Split(command,toks,";=");
			for(unsigned int i=0;i<toks.size();i++)
				{
				if(toks[i]=="command")
					{
					i++;
					webCommand.command=toks[i];
					webCommand.result="";
					webCommand.status=1;
					}
				else if(toks[i]=="rowid")
					{
					i++;
					webCommand.rowid=to_number<int>(toks[1]);
					}
				}
			}
		else
			{
			return false;  // no commands
			}
		}
	return true;
	}

/**
 * set the result for a command
 * @param rowid
 * @param res
 * @return
 */
bool DBWrapper::setResult(string res)
	{
	int rc;
	char *zErrMsg = 0;
	string sql;

	if(webCommand.rowid==0)
		{
		dbg->trace(DBG_DEBUG, "WEB result simulated (rowid=0)");
		gd.simWeb_trigger=false;
		return true;
		}

	sql=(string) "UPDATE actions SET "
	"status = '0' ," +
	"result = " + "'"+ res + "'" +
	" WHERE rowid = '" + to_string(webCommand.rowid) + "'";

	webCommand.status=0;

//dbg->trace(DBG_DEBUG, "DBsql: " + sql);

	rc = sqlite3_exec(evdb, sql.c_str(), callback, 0, &zErrMsg);
	__fpurge(stdout);
	if (rc != SQLITE_OK)
		{
		dbg->trace(DBG_FATAL, "SQL error: %s (%s)", zErrMsg,sql.c_str());
		sqlite3_free(zErrMsg);
		return false;
		}
	else
		{
		updateProfilesCRC();
		dbg->trace(DBG_NOTIFY, "database command result written");
		}
	return true;
	}

/**
 * clean al commands form actions table
 */
void DBWrapper::cleanCommands()
	{
	cleanTable('c');
	}
#endif
