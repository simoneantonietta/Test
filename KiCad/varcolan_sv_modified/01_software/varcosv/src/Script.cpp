/*
 *-----------------------------------------------------------------------------
 * PROJECT: varcosv
 * PURPOSE: see module Script.h file
 *-----------------------------------------------------------------------------
 */

#include "Script.h"
#include "dataStructs.h"
#include "comm/app_commands.h"
#include "comm/CommTh.h"
#include "comm/UDPCommTh.h"
#include "common/utils/Trace.h"
#include "common/utils/Utils.h"
#include "common/utils/SimpleCfgFile.h"
#include "db/DBWrapper.h"
#include "TelnetServer.h"
#include "HardwareGPIO.h"
#include "BadgeReader.h"
#include "Scheduler.h"
#include "Slave.h"



extern struct globals_st gd;
extern CommTh *comm;
extern Trace *dbg;
extern DBWrapper *db;
extern TelnetServer *tsrv;
extern UDPCommTh *udpSrvComm;
extern BadgeReader *br;
extern Scheduler *sched;
extern SimpleCfgFile *cfg;
extern Slave *slave;

#ifdef COMM_TH_UDP_DEBUG
extern UDPProtocolDebug *udpDbg;
#endif

#ifdef RASPBERRY
extern HardwareGPIO *hwgpio;
#endif

extern void endingApplication();

Script::Script()
{
// TODO Auto-generated constructor stub

}

Script::~Script()
{
// TODO Auto-generated destructor stub

}

/**
 * executes a script (not bash script but a proprietary one)
 * @param fname
 * @param forceRun
 * @return true: ok
 */
bool Script::runScriptFile(string script_fname, string status_fname, bool forceRun)
{
bool ret = true;
ifstream f;
fstream s;
string line;
bool run = false;
char output[100];
// TODO use database to trigger the script

// check if it must execute the script
if(!forceRun)
	{
	s.open(status_fname.c_str(), ios::in);
	if(s.is_open())
		{
		// read the file
		while(!s.eof())
			{
			nline++;
			getline(s, line);
			trim(line);
			if(line.empty()) continue;
			if(line[0] == '#') continue;
			if(line == "1")
				{
				run = true;
				s.close();
				// reopen in writing mode
				s.open(status_fname.c_str(), ios::out);
				if(s.is_open())
					{
					s << "0" << endl;
					}
				}
			break;
			}
		}
	s.close();
	}
else
	{
	run = true;
	}

nline = 0;
// execute the script
if(run)
	{
	TRACE(dbg,DBG_NOTIFY, "script execution begin");
	f.open(script_fname.c_str());
	if(f.is_open())
		{
		// read the file
		while(!f.eof())
			{
			getline(f, line);
			trim(line);
			ret = parseScriptLine(line,output);
			TRACE(dbg,DBG_NOTIFY,"command result: %s",output);
			}
		f.close();
		}
	TRACE(dbg,DBG_NOTIFY,"script execution end");
	}
return ret;
}

/**
 * parse a line and executes commands
 * @param cmd script command
 * if format is:
 *
 * $(n) script_line
 *
 * where n indicates client number or name (new format)
 * else is old format
 * @param output optional string for output
 * @param reqid if found else NULL
 * @return true: ok
 */
bool Script::parseScriptLine(string cmd, char *output, char *reqid)
{
bool ret=true;
vector<string> toks;
Badge_parameters_t bpar;
string line;
string _reqid;		// requester id

if(cmd.empty()) return ret;
if(cmd[0] == '#') return ret;

if(startsWith(cmd,"$("))
	{
  size_t pos = cmd.find(" ");
  if (pos == std::string::npos)
    return -1;
  string tmpreqid=cmd.substr(0,pos);
  _reqid=extractBetween(tmpreqid,"$(",")");
  if(reqid!=NULL)
  	{
  	strcpy(reqid,_reqid.c_str());
  	}
  line=cmd.substr(pos, std::string::npos);
  trim(line);
  TRACE(dbg,DBG_DEBUG,"command from %s: %s",_reqid.c_str(),line.c_str());
	}
else
	{
	// old format
  if(reqid!=NULL)
  	{
  	_reqid[0]=0;
  	}
	line=cmd;
	}

gd.expected_glob_mainState=globals_st::sl_unk;
gd.expected_glob_workerState=globals_st::wkr_unk;

// parse commands
try
	{
	//......................................................
	if(startsWith(line,"fw"))
		{
		/*
		 * force firmware upload
		 * fw force varcolan|supply|touch
		 */
		toks.clear();
		Split(line,toks," \t");
		if(toks[1]=="force")
			{
			TRACE(dbg,DBG_NOTIFY,"SCRIPT: force update firmware for " + toks[2]);
			if(toks[2]=="varcolan")
				{
				// check if file exists
				string fn=_S FIRMWARE_UPD_PATH + _S FIRMWARE_VARCOLAN;
				if(fileExists(fn+FIRMWARE_BIN_EXT) && fileExists(fn+FIRMWARE_TXT_EXT))
					{
					globals_st::dataEvent_t de;
					de.dataEvent=globals_st::dataEvent_t::ev_fw_new_version;
					de.tag.fw_device_type=DEVICE_TYPE_VARCOLAN;
					pushEvent(de,0);
					}
				else
					{
					throw 3;
					}
				}
			else
				{
				throw 1;
				}
			}
		else
			{
			throw 100;	// generic syntax error
			}
		}
	//......................................................
	else if(startsWith(line,"badge"))// add or modify a badge
		{
		/*
		 * syntax:
		 *       --------------------  vi st pin    vstart vstop     prof1 prof2 etc
		 * badge 1234567890abcdefffff (0, 0, 00000, now,   tomorrow, 0,    NULL, NULL,NULL,NULL,NULL,NULL,NULL,NULL)
		 * vstart can be "now" or YYYY-MM-DD
		 * vstop can be "tomorrow" or YYYY-MM-DD
		 * profileID the first must be declared, the others are optional
		 *
		 * badge 312622993FFFFFFFFFFF (0, 0, 00000, now,   tomorrow, 0,    NULL, NULL,NULL,NULL,NULL,NULL,NULL,NULL)
		 *
		 */
		int i=0;
		vector<string> toks1;
		string tmp;
		toks.clear();
		Split(line,toks," \t,()");

		if(toks.size()<8)
			{
			throw 4;
			}
		// badge number
		i++;// next field
		if(toks[i].size()!=(BADGE_SIZE*2))
			{
			string spc((BADGE_SIZE*2)-toks[i].size(),'f');
			toks[i]=toks[i]+spc;
			//throw 5;
			}
		int val=0;
		toUpperStr(toks[i]);
		for(unsigned int k=0,j=0;k<toks[i].size();k++)
			{
			(toks[i][k]>='A') ? (val=toks[i][k]-'A'+10) : (val=toks[i][k]-'0');
			if(k&1)
				{
				bpar.Parameters_s.badge[j].part.low=val;
				j++;
				}
			else
				{
				bpar.Parameters_s.badge[j].part.high=val;
				}
			}

		// visitor flag
		i++;// next field
		bpar.Parameters_s.visitor=to_number<int>(toks[i]);
		// badge status
		i++;// next field
		bpar.Parameters_s.badge_status=(Badge_Status_t)to_number<int>(toks[i]);
		//badge bin
		i++;// next field
		bpar.Parameters_s.pin=to_number<int>(toks[i]);
		// validity start
		i++;// next field
		if(toks[i]=="now" || toks[i]=="today")
			{
			time_t rawtime;
			struct tm * timeinfo;
			char buffer [80];

			time (&rawtime);
			timeinfo = localtime (&rawtime);

			strftime (buffer,80,"%F",timeinfo);
			tmp=_S buffer;
			}
		else
			{
			tmp=toks[i];
			}
		toks1.clear();
		Split(tmp,toks1,"-");
		bpar.Parameters_s.start_year=to_number<int>(toks1[0])-2000;
		bpar.Parameters_s.start_month=to_number<int>(toks1[1]);
		bpar.Parameters_s.start_day=to_number<int>(toks1[2]);

		// validity stop
		i++;// next field
		if(toks[i]=="tomorrow")
			{
			time_t rawtime;
			struct tm * timeinfo;
			char buffer [80];

			time (&rawtime);
			rawtime += (60*60*24);	// get tomorrow
			timeinfo = localtime (&rawtime);

			strftime (buffer,80,"%F",timeinfo);
			tmp=_S buffer;
			}
		else
			{
			tmp=toks[i];
			}
		toks1.clear();
		Split(tmp,toks1,"-");
		bpar.Parameters_s.stop_year=to_number<int>(toks1[0])-2000;
		bpar.Parameters_s.stop_month=to_number<int>(toks1[1]);
		bpar.Parameters_s.stop_day=to_number<int>(toks1[2]);

		// profile
		for(int p=0;p<ACTIVE_PROFILE_ARRAY_MAX;p++) bpar.Parameters_s.profilesID[p]=MAX_PROFILES;
		i++;// next field
		int np=0;
		for(int p=toks.size()-i;p>0;p--)
			{
			bpar.Parameters_s.profilesID[np]=to_number<int>(toks[i]);
			i++;
			np++;
			}
		TRACE(dbg,DBG_NOTIFY,"SCRIPT: add/modify badge %s",toks[1].c_str());
		int id=db->setBadge(bpar);

		// generate event to propagate the new badge
		globals_st::dataEvent_t de;
		de.dataEvent=globals_st::dataEvent_t::ev_badge_data;
		de.tag.badge_id=id;
		pushEvent(de,0);
		}
	//......................................................
	else if(startsWith(line,"exe"))
		{
		/*
		 * exe scriptname [params..]
		 */
		if(toks.size()<2) throw 2;
		string cmd=toks[1];
		TRACE(dbg,DBG_NOTIFY,"SCRIPT: execute shell script " + toks[1]);
		for(unsigned int i=1;i<toks.size();i++)  // concatenate parameters
			{
			cmd+=" " + toks[i];
			}
		system(cmd.c_str());
		}
	//......................................................
	else if(startsWith(line,"causal"))
		{
		/*
		 * modify a causal
		 * causal <ndx> <causal_id> <description>
		 */
		toks.clear();
		Split(line,toks," \t");

		if(toks.size()==4)
			{
			int _ndx=to_number<int>(toks[1]);
			int _cid=to_number<int>(toks[2]);
			db->setCausal(_ndx,_cid,toks[3]);
			}
		else
			{
			throw 6;
			}
		}
	//......................................................
	else if(startsWith(line,"clean"))
		{
		globals_st::dataEvent_t de;
		/**
		 * clean all_badges|all_logs|ID logs
		 *
		 * all_badges delete all badges to all devices
		 * ID logs delete log to a specified ID
		 * all_logs delete log to all
		 */
		toks.clear();
		Split(line,toks," \t");

		if(toks.size()==2)
			{
			if(toks[1]=="all_badges")
				{
				de.dataEvent=globals_st::dataEvent_t::ev_global_deletion;
				}
			else if(toks[1]=="all_logs")
				{
				de.dataEvent=globals_st::dataEvent_t::ev_log_deletion;
				}
			else
				{
				throw 100;
				}
			pushEvent(de,0);
			}
		else if(toks.size()==3)
			{
			if(toks[2]=="badge")
				{
				de.dataEvent=globals_st::dataEvent_t::ev_global_deletion;
				}
			else if(toks[2]=="logs")
				{
				de.dataEvent=globals_st::dataEvent_t::ev_log_deletion;
				}
			else
				{
				throw 100;
				}
			int id=atoi(toks[1].c_str());	// profile index
			pushEvent(de,id);
			}
		else
			{
			throw 6;
			}
		}
	//......................................................
	else if(startsWith(line,"vacuum"))
		{
		/*
		 * restore to factory configuration. all data will be deleted
		 * on all devices and supervisor
		 */
		globals_st::dataEvent_t de;
		de.dataEvent=globals_st::dataEvent_t::ev_global_deletion;
		pushEvent(de,0);
		de.dataEvent=globals_st::dataEvent_t::ev_log_deletion;
		pushEvent(de,0);

		WATCHDOG_REFRESH();
		db->cleanTable('b');
		WATCHDOG_REFRESH();
		db->cleanTable('e');
		WATCHDOG_REFRESH();
		db->cleanTable('p');
		WATCHDOG_REFRESH();
		db->cleanTable('w');
		WATCHDOG_REFRESH();
		db->cleanTable('t');
		WATCHDOG_REFRESH();
		db->cleanTable('u');
		WATCHDOG_REFRESH();
		db->cleanTable('j');
		WATCHDOG_REFRESH();
		db->cleanTable('d');
		WATCHDOG_REFRESH();
		db->cleanTable('a');
		WATCHDOG_REFRESH();
		db->cleanTable('q');
		WATCHDOG_REFRESH();
		db->cleanTable('c');
		WATCHDOG_REFRESH();

		strcpy(output,"ok");
		}
	//......................................................
	else if(startsWith(line,"disable"))
		{
		/*
		 * disable badgeautosend
		 */
		toks.clear();
		Split(line,toks," \t,()");

		if(toks.size()<2) throw 6;
		if(toks[1]=="badgeautosend")
			{
			gd.badgeAutoSend=false;
			}
		else if(toks[1]=="crypto")
			{
			gd.enableCrypto=false;
			comm->enableCrypto(false);
			}
		else if(toks[1]=="watchdog")
			{
			gd.enableWatchdog=false;
#ifdef RASPBERRY
		if(gd.enableWatchdog)
			{
			hwgpio->watchdogStop();
			}
#endif

			}
		else
			{
			throw 100;
			}
		}
	//......................................................
	else if(startsWith(line,"enable"))
		{
		/*
		 * enable badgeautosend|webcommand
		 */
		toks.clear();
		Split(line,toks," \t,()");

		if(toks.size()<2) throw 6;
		if(toks[1]=="badgeautosend")
			{
			gd.badgeAutoSend=true;
			}
		else if(toks[1]=="crypto")
			{
			gd.enableCrypto=true;
			comm->enableCrypto(true);
			}
		else if(toks[1]=="watchdog")
			{
			gd.enableWatchdog=true;
			}
		else
			{
			throw 100;
			}
		}
	//......................................................
	else if(startsWith(line,"reload"))
		{
		/*
		 * reload profiles|causals|weektimes|terminals
		 */
		bool res=true;
		toks.clear();
		Split(line,toks," \t,()");

		if(toks.size()<2) throw 6;
		if(toks[1]=="profiles")
			{
			res=db->loadProfiles();
			slave->putDBEvent(update_profile,gd.myID);
			}
		else if(toks[1]=="weektimes")
			{
			res=db->loadWeektimes();
			globals_st::dataEvent_t de;
			de.dataEvent=globals_st::dataEvent_t::ev_active_wktimes;
			pushEvent(de,0);
			TRACE(dbg,DBG_NOTIFY, "active weektime update to all");
			slave->putDBEvent(update_weektime,gd.myID);
			}
		else if(toks[1]=="causals")
			{
			res=db->loadCausals();
			globals_st::dataEvent_t de;
			TRACE(dbg,DBG_NOTIFY, "causals reloaded");
			slave->putDBEvent(update_causal_codes,gd.myID);
			}
		else if(toks[1]=="terminals")
			{
			res=db->loadTerminals();
			}
		else if(toks[1]=="terminal")
			{
			if(toks.size()==3)
				{
				res=db->loadTerminals();

				globals_st::dataEvent_t de;
				de.dataEvent=globals_st::dataEvent_t::ev_terminal_data_upd;
				memcpy(&de.tag.term_par,&gd.terminals[to_number<int>(toks[2])-1].params,sizeof(Terminal_parameters_to_send_t));
				//pushEvent(de,to_number<int>(toks[2]));
				pushEvent(de,0);
				TRACE(dbg,DBG_NOTIFY, "terminal %s data update",toks[2].c_str());
				strcpy(output,"ok");
				}
			else
				{
				strcpy(output,"error");
				throw 6;
				}
			}
		else if(toks[1]=="badge")
			{
			if(toks.size()==4)
				{
				// follow the toks[2]=id, toks[3]=uid
				globals_st::dataEvent_t de;
				de.dataEvent=globals_st::dataEvent_t::ev_badge_data;
				de.tag.badge_id=to_number<int>(toks[2]);
				pushEvent(de,0);
				TRACE(dbg,DBG_NOTIFY, "badge %d (%s) propagating",de.tag.badge_id,toks[3].c_str());
				strcpy(output,"ok");
				db->dbSave(); // make a backup because an entry area is added
				// update max id
				gd.maxBadgeId=db->getBadgeMaxId();
				TRACE(dbg,DBG_NOTIFY,"Badge max ID = %d",gd.maxBadgeId);
				}
			else
				{
				strcpy(output,"error");
				throw 6;
				}
			}
		else
			{
			throw 100;
			}

		if(output!=NULL)
			{
			(res) ? (strcpy(output,"ok")) : (strcpy(output,"error"));
			}
		// do nothing
		}
	//......................................................
	else if(startsWith(line,"profile"))
		{
		/*
		 * profile 0 active|inactive 0,20,50
		 */
		unsigned int n,ter,ndx;
		toks.clear();
		Split(line,toks," \t,()");

		if(toks.size()<4) throw 6;
		ndx=atoi(toks[1].c_str());	// profile index
		if(ndx<MAX_PROFILES)
			{
			n=3;
			while(n<toks.size())
				{
				ter=atoi(toks[n].c_str());
				if(ter<MAX_TERMINALS)
					{
					if(toks[2]=="active" || toks[2]=="enable")
						{
						gd.profiles[ndx].params.Parameters_s.activeTerminal |= (1UL<<ter);
						}
					else if(toks[2]=="inactive" || toks[2]=="disable")
						{
						gd.profiles[ndx].params.Parameters_s.activeTerminal &= ~(1UL<<ter);
						}
					}
				n++;
				}
			gd.profiles[ndx].params.crc8=CRC8calc((uint8_t *)&gd.profiles[ndx].params.Parameters_s,sizeof(Profile_parameters_t::Parameters_s));
			db->updateProfile(ndx,gd.profiles[ndx].name);	// update in database
			}
		}
	//......................................................
	else if(startsWith(line,"weektime"))
		{
		/**
		 * weektime 0 mon|tue|wed|thu|fri|sat|sun 10:00,12:00 14:00,17:00
		 */
		unsigned int n,ndx;
		uint8_t t;
		toks.clear();
		Split(line,toks," \t,():.");

		if(toks.size()<11) throw 6;
		ndx=atoi(toks[1].c_str());	// weektime index
		if(ndx<MAX_WEEKTIMES)
			{
			int day=dayMap(toks[2]);
			if(day>=0)
				{
				int wd=day*4*2;
				n=3;
				while(n<toks.size())
					{
					t=atoi(toks[n++].c_str());
					gd.weektimes[ndx].params.Parameters_s.week.weekdays[wd++]=t;
					}
				}
			gd.weektimes[ndx].params.crc8=CRC8calc((uint8_t *)&gd.weektimes[ndx].params.Parameters_s,sizeof(Weektime_parameters_t::Parameters_s));
			db->updateWeektime(ndx,gd.weektimes[ndx].name);	// update in database
			}
		}
	//......................................................
	else if(startsWith(line,"logevent"))
		{
		/*
		 * add a log event to the event database
		 * logevent <event_code>
		 */
		toks.clear();
		Split(line,toks," \t,():.");
		if(toks.size() == 2)
			{
			slave->putDBEvent((EventCode_t)to_number<int>(toks[1]),DEV_BASEID_SUPERVISOR);
			}
		}
	//......................................................
	else if(startsWith(line,"sleep"))
		{
		/*
		 * wait for the specified amount of seconds (n)
		 * sleep n
		 */
		toks.clear();
		Split(line,toks," \t,():.");

		if(toks.size() == 2)
			{
			sleep(to_number<int>(toks[1]));
			}
		}
	//......................................................
	else if(startsWith(line,"sync"))
		{
		/*
		 * executes a sync to write changes
		 */
		sync();
		}
	//......................................................
	else if(startsWith(line,"change_pin"))
		{
		/*
		 * change_pin badge_id new_pin
		 * change the pin to the badge specified by its id
		 * new_pin is a 5 digit maximum integer number [0...99999]
		 */
		toks.clear();
		Split(line,toks," \t,():.");

		if(toks.size() == 3)
			{
			if((toks[2].length() > 0) && (toks[2].length() <= 5))
				{
				if(!db->changeBadgePinFromId(to_number<int>(toks[1]),toks[2]))
					{
					strcpy(output,"error");
					}
				}
			else
				{
				throw 9;
				}
			}
		else
			{
			throw 6;
			}
		}
	//......................................................
	else if(startsWith(line,"set"))
		{
		/*
		 * see HPROT_SUPERVISOR_CMD_BASE data to identify ports
		 * set ID n_port 0|1
		 */
		toks.clear();
		Split(line,toks," \t,():.");

		if(toks.size() == 4)
			{
			globals_st::dataEvent_t de;
			de.dataEvent=globals_st::dataEvent_t::ev_set_output;
			de.tag.params[0]=(uint8_t)to_number<int>(toks[1]);
			de.tag.params[1]=(uint8_t)to_number<int>(toks[2]);
			de.tag.params[2]=(uint8_t)to_number<int>(toks[3]);
			pushEvent(de,0);
			}
		else
			{
			throw 6;
			}
		}
	//......................................................
	else if(startsWith(line,"terminalctrl"))
		{
		/*
		 * terminalctrl id on|off
		 *
		 * enable/disable the control of a terminal
		 */
		toks.clear();
		Split(line,toks," \t,():.");

		if(toks.size() == 3)
			{
			globals_st::dataEvent_t de;
			de.dataEvent=globals_st::dataEvent_t::ev_terminal_control;
			int _id=(uint8_t)to_number<int>(toks[1]);
			de.tag.params[0]=_id;
			de.tag.params[1]=(toks[2]=="on") ? (1) : (0);
			pushEvent(de,0);
			}
		else
			{
			throw 6;
			}
		}
	//......................................................
	else if(startsWith(line,"spare"))
		{
		/*
		 * send a spare command
		 * spare ID value
		 *
		 */
		toks.clear();
		Split(line,toks," \t,():.");

		if(toks.size() == 3)
			{
			globals_st::dataEvent_t de;
			int _id=(uint8_t)to_number<int>(toks[1]);
			de.dataEvent=globals_st::dataEvent_t::ev_spare;
			de.tag.params[0]=(uint8_t)to_number<int>(toks[2]);
			pushEvent(de,_id);
			}
		else
			{
			throw 6;
			}
		}
	//......................................................
	else if(startsWith(line,"mac"))
		{
		/*
		 * set the mac address to the ID device
		 * mac ID xx.xx.xx.xx.xx.xx
		 */
		toks.clear();
		Split(line,toks," \t.:");

		if(toks.size() == 8)
			{
			globals_st::dataEvent_t de;
			de.dataEvent=globals_st::dataEvent_t::ev_mac_address;
			de.tag.params[0]=(uint8_t)to_number<int>(toks[1]);	// id
			de.tag.params[1]=(uint8_t)strtol(toks[2].c_str(),NULL,16);
			de.tag.params[2]=(uint8_t)strtol(toks[3].c_str(),NULL,16);
			de.tag.params[3]=(uint8_t)strtol(toks[4].c_str(),NULL,16);
			de.tag.params[4]=(uint8_t)strtol(toks[5].c_str(),NULL,16);
			de.tag.params[5]=(uint8_t)strtol(toks[6].c_str(),NULL,16);
			de.tag.params[6]=(uint8_t)strtol(toks[7].c_str(),NULL,16);
			pushEvent(de,de.tag.params[0]);
			}
		else
			{
			throw 6;
			}
		}
	//......................................................
	else if(startsWith(line,"ip"))
		{
		/*
		 * set the ip address to the ID device
		 * ip ID ddd.ddd.ddd.ddd
		 */
		toks.clear();
		Split(line,toks," \t.:");

		if(toks.size() == 6)
			{
			globals_st::dataEvent_t de;
			de.dataEvent=globals_st::dataEvent_t::ev_ip_address;
			de.tag.params[0]=(uint8_t)to_number<int>(toks[1]);	// id
			de.tag.params[1]=(uint8_t)to_number<int>(toks[2]);
			de.tag.params[2]=(uint8_t)to_number<int>(toks[3]);
			de.tag.params[3]=(uint8_t)to_number<int>(toks[4]);
			pushEvent(de,de.tag.params[0]);
			}
		else
			{
			throw 6;
			}
		}
	//......................................................
	else if(startsWith(line,"masterdump"))
		{
		/*
		 * start the master ID data dump
		 * masterdump ID
		 */
		toks.clear();
		Split(line,toks," \t");

		if(toks.size() == 2)
			{
			globals_st::dataEvent_t de;
			de.dataEvent=globals_st::dataEvent_t::ev_memory_dump;
			de.tag.params[0]=(uint8_t)to_number<int>(toks[1]);	// id
			pushEvent(de,de.tag.params[0]);
			}
		else
			{
			throw 6;
			}
		}
	//......................................................
	else if(startsWith(line,"termlist"))
		{
		/*
		 * force the request of terminal list
		 */

		globals_st::dataEvent_t de;
		de.dataEvent=globals_st::dataEvent_t::ev_terminal_list;
		pushEvent(de,0);
		}
	//......................................................
	else if(startsWith(line,"terminals"))
		{
		/*
		 * start all procedure to retrieve list and data
		 */
		globals_st::dataEvent_t de;
		de.dataEvent=globals_st::dataEvent_t::ev_all_terminal_data;
		gd.expected_glob_mainState=globals_st::sl_terminal_request;
		gd.expected_glob_workerState=globals_st::wkr_nterm_param_end;
		slave->webCmdTimeout.startTimeout((WKR_TIMEOUT_TERMINALS + WKR_GLOBAL_TIMEOUT_INC)*1000);
		gd.cmd_fromClient=tsrv->getActualClient();
		TRACE(dbg,DBG_NOTIFY,"retrieve all terminals data, request from web -> client %d",gd.cmd_fromClient);
		strcpy(output,"wait");
		db->cleanTerminalStatus(TERMINAL_STATUS_OFF_LINE);
		pushEvent(de,0);
		}
	//......................................................
	else if(startsWith(line,"termdata"))
		{
		/*
		 * force the request of terminal data
		 * termdata ID
		 */
		toks.clear();
		Split(line,toks," \t");

		if(toks.size() >= 2)
			{
			globals_st::dataEvent_t de;
			de.dataEvent=globals_st::dataEvent_t::ev_terminal_data_req;
			for(int i=0;i<MAX_EVENT_TAG_PARAMS;i++)
				{
				if(i<(toks.size()-1))
					{
					de.tag.params[i]=(uint8_t)to_number<int>(toks[i+1]);	// id
					}
				else
					{
					de.tag.params[i]=0;
					}
				}
			pushEvent(de,0);
			}
		else
			{
			throw 6;
			}
		}
	//......................................................
	else if(startsWith(line,"list"))
		{
		/*
		 * request of some listings for info
		 * list <master>
		 */
		toks.clear();
		Split(line,toks," \t");

		if(toks.size() == 2 && output != NULL)
			{
			if(toks[1]=="master")
				{
				char tmp[50];
				int alive_cnt=0,dead_cnt=0;
				strcat(output,"list: ");
				for(globals_st::iddevice_t it = gd.deviceList.begin();it != gd.deviceList.end();it++)
					{
					if(it->second.aliveCounter>0)	// if is alive
						{
						sprintf(tmp,"%d.",it->first);
						strcat(output,tmp);
						alive_cnt++;
						}
					else // dead masters
						{
						sprintf(tmp,"!%d.",it->first);
						strcat(output,tmp);
						dead_cnt++;
						}
					}
				sprintf(tmp,"\nalive = %d\ndead  = %d\ndead counter = %d\ncheckLinks = %d\n",alive_cnt,dead_cnt,gd.deadCounter,gd.cklinkCounter);
				strcat(output,tmp);
				}
			else if(toks[1]=="stats")
				{
				char tmp1[50];
				sprintf(tmp1,"connections: %d\n",comm->getStats(COMM_STATS_GLOBAL_CONNECTION));
				strcat(output,tmp1);
				sprintf(tmp1,"disconnections: %d\n",comm->getStats(COMM_STATS_GLOBAL_DISCONNECTION));
				strcat(output,tmp1);
				sprintf(tmp1,"crc errors: %d\n",comm->getStats(COMM_STATS_GLOBAL_CRC_ERROR));
				strcat(output,tmp1);
				sprintf(tmp1,"cmd errors: %d\n",comm->getStats(COMM_STATS_GLOBAL_CMD_ERROR));
				strcat(output,tmp1);
				sprintf(tmp1,"answers from invalid id: %d\n",comm->stats.ans_invalid_id);
				strcat(output,tmp1);

				if(gd.useSerial)
					{
					int crc,cmd;
					for(CommTh::stats_st::devCommError_it it = comm->stats.devErrors.begin();it != comm->stats.devErrors.end();it++)
						{
						crc=comm->getStats(COMM_STATS_DEV_CRC_ERROR,it->first);
						cmd=comm->getStats(COMM_STATS_DEV_CMD_ERROR,it->first);
						if((crc>0) || (cmd>0))
							{
							sprintf(tmp1,"device %d errors: crc=%-4d; cmd=%-3d\n",it->first,it->second. crc,cmd);
							strcat(output,tmp1);
							}
						}
					}
				else if(gd.useSocketServer)
					{
					for(int i = 0;i < UDP_MAX_CLIENTS;i++)
						{
						if((udpSrvComm->clients[i].error_crc>0) || (udpSrvComm->clients[i].error_cmd>0))
							{
							sprintf(tmp1,"client %-2d (ip:%d.%d.%d.%-3d) (id %-3d) errors: crc=%-4d; cmd=%-3d\n"	,
																																								i,
																																								udpSrvComm->clients[i].clientIP.ip_fld[0],
																																								udpSrvComm->clients[i].clientIP.ip_fld[1],
																																								udpSrvComm->clients[i].clientIP.ip_fld[2],
																																								udpSrvComm->clients[i].clientIP.ip_fld[3],
																																								udpSrvComm->clients[i].id,
																																								udpSrvComm->clients[i].error_crc,
																																								udpSrvComm->clients[i].error_cmd
																																								);
							strcat(output,tmp1);
							}
						}
					}
				}
			}
		}
	//......................................................
	else if(startsWith(line,"get badge"))
		{
		/*
		 * put the system in read badge mode
		 */
		TRACE(dbg,DBG_NOTIFY,"waiting for a badge -> client %d",gd.cmd_fromClient);
		if(gd.has_reader)
			{
			if(br->checkPresence(true))
				{
				TRACE(dbg,DBG_NOTIFY,"reader available");
				}
			else
				{
				TRACE(dbg,DBG_WARNING,"reader no more available");
				}
			TRACE(dbg,DBG_NOTIFY,"use reader or a terminal to load the badge");
			br->reset();
			br->startThread(NULL);
			}
		else
			{
			if(gd.terminalAsReader_id != 0)
				{
				TRACE(dbg,DBG_NOTIFY,"no reader available, use terminal %d", (int)gd.terminalAsReader_id);
				}
			else
				{
				throw 10;
				}
			}
		gd.badgeAcq.waitingForBadge=true;
		gd.cmd_fromClient=tsrv->getActualClient();
		strcpy(output,"wait");
//		sleep(2);
//		strcpy(output,"1234567890");
		}
	//......................................................
	else if(startsWith(line,"clear"))
		{
		/**
		 * clear statistics
		 * clear stats|devstats <id>
		 */
		toks.clear();
		Split(line,toks," \t");

		if(toks.size() == 2)
			{
			if(toks[1]=="stats")
				{
				comm->clearStats();
				if(gd.useSocketServer)
					{
					for(int i = 0;i < UDP_MAX_CLIENTS;i++)
						{
						udpSrvComm->clients[i].error_crc=0;
						udpSrvComm->clients[i].error_cmd=0;
						}
					}
				gd.deadCounter=0;
				gd.cklinkCounter=0;
				}
			}
		else if(toks.size()==3)
			{
			if(toks[1]=="devstats")
				{
				globals_st::dataEvent_t de;
				de.dataEvent=globals_st::dataEvent_t::ev_stats_clear;
				de.tag.terminal_id=to_number<int>(toks[2]);
				pushEvent(de,to_number<int>(toks[2]));
				}
			}
		else
			{
			throw 100;
			}
		}
	//......................................................
	else if(startsWith(line,"time rtc"))
		{
		/*
		 * set the rtc time (and system)
		 * time rtc YYYYMMDDhhmmss
		 *
		 */
		toks.clear();
		Split(line,toks," \t");

		if(toks.size() == 3)
			{
#ifdef RASPBERRY
			hwgpio->setRTC_datetime(toks[2]);
			//hwgpio->writeSysUpdTimeFile(toks[2]);

			// propagates to masters
			int year, month, day, hour, minute, second;
			struct tm time_requested;
			globals_st::dataEvent_t de;
			de.dataEvent=globals_st::dataEvent_t::ev_new_time;

			sscanf(toks[2].c_str(), "%4d%2d%2d%2d%2d%2d", &year, &month, &day, &hour, &minute, &second);

			/* Validate that the input date and time is basically sensible */
			if((year < 2000) || (year > 2099) || (month < 1) || (month > 12) || (day < 1) || (day > 31) || (hour < 0) || (hour > 23) || (minute < 0) || (minute > 59) || (second < 0) || (second > 59))
				{
				printf("Incorrect date and time specified.\nRun as:\nrtc_spi\nor\nrtc_spi CCYYMMDDHHMMSS\n");
				throw 9;
				}

			/* Finally convert to it to EPOCH time, ie the number of seconds since January 1st 1970, and set the system time */
			time_requested.tm_sec = second;
			time_requested.tm_min = minute;
			time_requested.tm_hour = hour;
			time_requested.tm_mday = day;
			time_requested.tm_mon = month - 1;
			time_requested.tm_year = year - 1900;
			time_requested.tm_wday = 0; /* not used */
			time_requested.tm_yday = 0; /* not used */
			time_requested.tm_isdst = -1; /* determine daylight saving time from the system */

			de.tag.epochtime = mktime(&time_requested);

			pushEvent(de,0);

			system("sudo rtc_spi");

#else
			throw 7;
#endif
			}
		else
			{
			throw 6;
			}
		}
	//......................................................
	else if(startsWith(line,"polling"))
		{
		/*
		 * set the polling time of all or specified device
		 * polling ID value (0=all)
		 * value is in ms
		 */
		toks.clear();
		Split(line,toks," \t");

		if(toks.size() == 3)
			{
			globals_st::dataEvent_t de;
			de.dataEvent=globals_st::dataEvent_t::ev_polling_time;
			de.tag.polling_time=to_number<uint16_t>(toks[2]);
			pushEvent(de,to_number<int>(toks[1]));
			}
		else
			{
			throw 6;
			}
		}
	//......................................................
	else if(startsWith(line,"killme"))
		{
		TRACE(dbg,DBG_NOTIFY,"kill request!");
		db->dbSave();
		tsrv->closeClientConnection(tsrv->getActualClient());
		gd.run=false;
		sleep(1);
		}
	//......................................................
	else if(startsWith(line,"logout"))
		{
		tsrv->closeClientConnection(tsrv->getActualClient());
		}
	//......................................................
	else if(startsWith(line,"use_svm"))
		{
		/*
		 * return a value that indicates id has svm configured
		 */
		if(output!=NULL)
			{
			if(gd.useSVM && !gd.svmDebugMode)
				{
				strcpy(output,"yes");
				}
			else
				{
				strcpy(output,"no");
				}
			}
		}
	//......................................................
	else if(startsWith(line,"test"))
		{
		if(output!=NULL)
			{
			strcpy(output,"ok");
			}
		}
	//......................................................
	else if(startsWith(line,"time?"))
		{
		#ifdef RASPBERRY
		TRACE(dbg,DBG_NOTIFY,"actual time "+tf.datetime_fmt("%D %T",hwgpio->getRTC_datetime()));
		strcpy(output,tf.datetime_fmt("%D %T",hwgpio->getRTC_datetime()).c_str());
		#else
		TRACE(dbg,DBG_NOTIFY,"started at "+tf.datetime_now("%D %T"));
		#endif
		}
	//......................................................
	else if(startsWith(line,"sched"))
		{
		/**
		 * scheduled operation
		 * sched <epoch_time> {<command>}
		 */
		toks.clear();
		Split(line,toks,"{}");

		if(toks.size() == 2)
			{
			vector<string> toks1;
			Split(toks[0],toks1," \t");
			if(toks1.size()==2)
				{
				time_t et;
				time_t t=to_number<time_t>(toks1[1]);
				#ifdef RASPBERRY
				et=hwgpio->getRTC_datetime();
				#else
				et=time(NULL);
				#endif

				sched->add(t,et,toks[1]);
				}
			else
				{
				throw 6;
				}
			}
		else
			{
			throw 6;
			}
		}
	//......................................................
	else if(startsWith(line,"delete jobs"))
		{
		/**
		 * delete all scheduled operations
		 * delete jobs
		 */
		sched->clearAllJobs();
		}
	//......................................................
	else if(startsWith(line,"system"))
		{
		/**
		 * execute a shell command
		 * system [<cmd>]
		 */
		toks.clear();
		Split(line,toks,"[]");

		if(toks.size() == 2)
			{
			string cmd=trim(toks[1]);
			system(cmd.c_str());
			}
		else
			{
			throw 6;
			}
		}
	//......................................................
	else if(startsWith(line,"reset"))
		{
		/**
		 * perform a reset of the id device, if 0 ->all
		 * reset <id>
		 */
		toks.clear();
		Split(line,toks," \t");

		if(toks.size() == 2)
			{
			globals_st::dataEvent_t de;
			de.dataEvent=globals_st::dataEvent_t::ev_reset_device;
			de.tag.terminal_id=to_number<int>(toks[1]);
			pushEvent(de,to_number<int>(toks[1]));
			}
		else
			{
			throw 6;
			}
		}
	//......................................................
	else if(startsWith(line,"poll_request"))
		{
		/**
		 * perform a command to force an immediate polling
		 * poll_request <id>
		 */
		toks.clear();
		Split(line,toks," \t");

		if(toks.size() == 2)
			{
			int id=to_number<int>(toks[1]);
			comm->sendCommand(id,HPROT_CMD_SV_FORCE_POLLING_REQ,0,0);
			}
		else
			{
			throw 6;
			}
		}
	//......................................................
	else if(startsWith(line,"devstats"))
		{
		/**
		 * request a device statistics (i.e. communication crc error)
		 * devstats <id>
		 */
		toks.clear();
		Split(line,toks," \t");

		if(toks.size() == 2)
			{
			globals_st::dataEvent_t de;
			de.dataEvent=globals_st::dataEvent_t::ev_stats_req;
			de.tag.terminal_id=to_number<int>(toks[1]);
			pushEvent(de,to_number<int>(toks[1]));
			}
		else
			{
			throw 6;
			}
		}
	//......................................................
	else if(startsWith(line,"newkey"))
		{
		/**
		 * perform a change of crypto key
		 * newkey <key>
		 * i.e:
		 * newkey 12345678
		 */
		toks.clear();
		Split(line,toks," \t");

		if(toks.size() == 2)
			{
			globals_st::dataEvent_t de;
			de.dataEvent=globals_st::dataEvent_t::ev_change_crypto_key;
			if(toks[1]=="default")
				{
				de.tag.params[0]=HPROT_CRYPT_DEFAULT_KEY0;
				de.tag.params[1]=HPROT_CRYPT_DEFAULT_KEY1;
				de.tag.params[2]=HPROT_CRYPT_DEFAULT_KEY2;
				de.tag.params[3]=HPROT_CRYPT_DEFAULT_KEY3;
				}
			else
				{
				for(int i=0;i<HPROT_CRYPT_KEY_SIZE*2;i++)
					{
					if(!isxdigit(toks[1][i]))
						{
						throw 8;
						}
					AsciiHex2Hex(de.tag.params,(char *)toks[1].c_str(),HPROT_CRYPT_KEY_SIZE);
					}
				}

			slave->webCmdTimeout.startTimeout(WKR_NEWKEY_GLOBAL_TIMEOUT*1000);
			gd.expected_glob_mainState=globals_st::sl_newkey_send;
			gd.expected_glob_workerState=globals_st::wkr_newkey_done;

			for(globals_st::iddevice_t it = gd.deviceList.begin();it != gd.deviceList.end();it++)
				{
				it->second.newKeySet=false;
				}
			clearEvent(0);		// clear all events..
			pushEvent(de,0);	// and push this new one
			TRACE(dbg,DBG_NOTIFY, "key change to " + toks[1]);
			strcpy(output,"ok");
			}
		else
			{
			throw 6;
			}
		}
	//......................................................
	else if(startsWith(line,"mykey"))
		{
		/*
		 * forces the new key but not for devices, only for me
		 * mykey <key>
		 */
		toks.clear();
		Split(line,toks," \t");

		if(toks.size() == 2)
			{
			if(toks[1]=="default")
				{
				gd.hprotKey[0]=HPROT_CRYPT_DEFAULT_KEY0;
				gd.hprotKey[1]=HPROT_CRYPT_DEFAULT_KEY1;
				gd.hprotKey[2]=HPROT_CRYPT_DEFAULT_KEY2;
				gd.hprotKey[3]=HPROT_CRYPT_DEFAULT_KEY3;
				}
			else
				{
				for(int i=0;i<HPROT_CRYPT_KEY_SIZE*2;i++)
					{
					if(!isxdigit(toks[1][i]))
						{
						throw 8;
						}
					AsciiHex2Hex(gd.hprotKey,(char *)toks[1].c_str(),HPROT_CRYPT_KEY_SIZE);
					}
				}
			comm->setNewKey(gd.hprotKey);
			comm->enableCrypto(true);

#ifdef COMM_TH_UDP_DEBUG
			if(gd.useUdpDebug)
				{
				udpDbg->setNewKey(gd.hprotKey);
				udpDbg->enableCrypto(true);
				}
#endif
			if(hprotCheckKey(gd.hprotKey)==0)
				{
				TRACE(dbg,DBG_NOTIFY,"plain communication mode due to invalid key");
				}
			// save to the config file
			char _tmpstr[10];
			Hex2AsciiHex(_tmpstr,gd.hprotKey,HPROT_CRYPT_KEY_SIZE,false,0);
			cfg->updateVariable("CRYPTO_KEY",_S _tmpstr);
			gd.cfgChanged=true;
			}
		else
			{
			throw 6;
			}
		}
	//......................................................
	else if(startsWith(line,"myip"))
		{
		/*
		 * set the new IP of the supervisor
		 * myip <xxx.xxx.xxx.xxx> <nnn.nnn.nnn.nnn>
		 * <xxx.xxx.xxx.xxx> ip address
		 * <nnn.nnn.nnn.nnn> netmask
		 */
		toks.clear();
		Split(line,toks," \t");

		if(toks.size() == 3 || toks.size() == 4)
			{
			string cmd;
			//sudo ifconfig eth0   192.168.0.1 netmask 255.255.255.0
			//string cmd="sudo ifconfig eth0 " + toks[1] + " netmask " + toks[2];
			//sed -i '/address*/c\address 192.168.10.100' /data/interfaces
			cmd="sed -i '/address*/c\\address " + toks[1] + "' /data/sys/interfaces";
			system(cmd.c_str());
			cmd="sed -i '/netmask*/c\\netmask " + toks[2] + "' /data/sys/interfaces";
			system(cmd.c_str());
			sleep(1);
			system("sync");

			if(toks.size() == 4)
				{
				if(toks[3]=="noreboot")
					{
					TRACE(dbg,DBG_NOTIFY, "no reboot, please do it manually to get the changes effective");
					}
				}
			else
				{
				//cmd="sudo /etc/init.d/networking restart";
				TRACE(dbg,DBG_NOTIFY, "now I reboot...bye bye");
				cmd="sudo reboot";
				system(cmd.c_str());
				}
			strcpy(output,"ok");
			TRACE(dbg,DBG_NOTIFY, "set new ip " + toks[1] + " netmask " + toks[2] + " for eth0");
			}
		else
			{
			throw 6;
			}
		}
	//......................................................
	else if(startsWith(line,"mygateway"))
		{
		/*
		 * set the new default gateway of the supervisor
		 * myip <ggg.ggg.ggg.ggg>
		 * <ggg.ggg.ggg.ggg> gateway address
		 */
		toks.clear();
		Split(line,toks," \t");

		if(toks.size() == 2)
			{
			//sudo route add default gw 192.168.0.253 eth0
			string cmd="sudo route add default gw " + toks[1] + " eth0";
			system(cmd.c_str());
			strcpy(output,"ok");
			TRACE(dbg,DBG_NOTIFY, "new gateway for eth0 set to " + toks[1]);
			}
		else
			{
			throw 6;
			}
		}
	//......................................................
	else if(startsWith(line,"mydns"))
		{
		/*
		 * set the new dns
		 * mydns <ddd.ddd.ddd.ddd> <<eee.eee.eee.eee>>
		 * the second param is not madatory
		 */
		toks.clear();
		Split(line,toks," \t");

		if(toks.size() == 2 || toks.size() == 3)
			{
			system("sudo mv /etc/resolv.conf /etc/~resolv.conf");
			system("echo \"# Generated by sv\" > /etc/resolv.conf");
			string cmd="echo \"nameserver \"" + toks[1] + " >> /etc/resolv.conf";
			system(cmd.c_str());
			TRACE(dbg,DBG_NOTIFY, "set new dns " + toks[1]);
			if(toks.size() == 3)
				{
				cmd="echo \"nameserver \"" + toks[2] + " >> /etc/resolv.conf";
				system(cmd.c_str());
				TRACE(dbg,DBG_NOTIFY, "set new dns " + toks[2]);
				}
			strcpy(output,"ok");
			}
		else
			{
			throw 6;
			}
		}
	//......................................................
	else if(startsWith(line,"get mynetdata"))
		{
		/*
		 * send data as output
		 * get mynetdata <ip> <netmask> [<gateway> <dns1> <dns1>]
		 */
		FILE * netdata=popen("ifconfig eth0 | grep \"inet \"", "r");
		// result: inet addr:192.168.10.100  Bcast:192.168.10.255  Mask:255.255.255.0
		char buffer[100];
		char *line_p = fgets(buffer, sizeof(buffer), netdata);
		for(unsigned int i=0;i<strlen(line_p);i++) if(line_p[i]=='\n') line_p[i]=0;
		string netinfo(line_p);
		toks.clear();
		Split(netinfo,toks,": \t");
		string _tmp=toks[2] + " " + toks[6];
		// get the default gateway
		netdata=popen("route -n | grep 'UG[ \t]' | awk '{print $2}'", "r");
		line_p = fgets(buffer, sizeof(buffer), netdata);
		string gw(line_p);
		_tmp += " " + gw;
		TRACE(dbg,DBG_NOTIFY,"network data " + _tmp);
		strcpy(output,_tmp.c_str());
		}
	//......................................................
	else if(startsWith(line,"alter"))
		{
		/**
		 * perform a change of a paramenter in the .cfg file
		 * alter <param> = <value>
		 * i.e:
		 * alter TERMINAL_AS_READER_ID = 0
		 */
		toks.clear();
		Split(line,toks," \t=");

		if(toks.size() == 3)
			{
			if(cfg->updateVariable(toks[1],toks[2]))
				{
				cfg->saveCfgFile();
				TRACE(dbg,DBG_NOTIFY,"alter " + toks[1] + " = " + toks[2] + " success (restart app to make the changes effective)");
				strcpy(output,"ok");
				}
			else
				{
				strcpy(output,"error");
				TRACE(dbg,DBG_ERROR,"alter " + toks[1] + " = " + toks[2] + " failed");
				}
			}
		else
			{
			throw 6;
			}
		}
	//......................................................
//	else if(startsWith(line,"timed_op"))
//		{
//		/**
//		 * creates some timed operations and can be:
//		 * timed_op sql [query] [ftp_addr:user:pwd]
//		 */
//		toks.clear();
//		Split(line,toks," \t[]:");
//
//		if(toks.size() == 3)
//			{
////			$ ftp -n <<EOF
////			open ftp.example.com
////			user user secret
////			put my-local-file.txt
////			EOF
///
/// 			oppure
///
/// 		curl -T my-local-file.txt ftp://ftp.example.com --user user:secret
//			}
//		}
	//......................................................
	else if(startsWith(line,"database"))
		{
		/**
		 * delete all data in the specified table
		 * database clean profiles|weektimes|badges|events
		 * i.e:
		 * database clean badges
		 */
		toks.clear();
		Split(line,toks," \t");

		if(toks.size() == 3)
			{
			WATCHDOG_REFRESH();
			if(toks[1]=="clean")
				{
				if(toks[2]=="profiles")
					{
					db->cleanTable('p');
					db->setProfileDefault();
					strcpy(output,"ok");
					}
				else if(toks[2]=="weektimes")
					{
					db->cleanTable('w');
					db->setWeektimeDefault();
					strcpy(output,"ok");
					}
				else if(toks[2]=="badges")
					{
					db->cleanTable('b');
					strcpy(output,"ok");
					}
				else if(toks[2]=="events")
					{
					db->cleanTable('e');
					strcpy(output,"ok");
					}
				else if(toks[2]=="terminals")
					{
					db->cleanTable('t');
					strcpy(output,"ok");
					}
				else if(toks[2]=="users")
					{
					db->cleanTable('u');
					strcpy(output,"ok");
					}
				else if(toks[2]=="jobs")
					{
					db->cleanTable('j');
					strcpy(output,"ok");
					}
				else
					{
					throw 9;
					}
				WATCHDOG_REFRESH();
				}
			}
		else
			{
			throw 6;
			}
		}
	//......................................................
	else if(startsWith(line,"restoredb"))
		{
		string cmd;
		/*
		 * start the DB restore:
		 */
			// check if is a valid db file
			// file varcolan.db | grep "SQLite 3" | wc -l
#ifdef RASPBERRY
		if(fileExists(_S RASPBERRY_WORKING_DIR + _S DB_RESTORE_PATH + _S DB_VARCOLANSV))
			{
			cmd="file " + _S RASPBERRY_WORKING_DIR + _S DB_RESTORE_PATH + _S DB_VARCOLANSV + " | grep \"SQLite 3\" | wc -l";
#else
		if(fileExists(_S DB_RESTORE_PATH + _S DB_VARCOLANSV))
			{
			cmd="file " + _S DB_RESTORE_PATH + _S DB_VARCOLANSV + " | grep \"SQLite 3\" | wc -l";
#endif

			FILE * ckfile=popen(cmd.c_str(), "r");
			// result= 0 or 1
			char buffer[100];
			char *line_p = fgets(buffer, sizeof(buffer), ckfile);
			for(unsigned int i=0;i<strlen(line_p);i++) if(line_p[i]=='\n') line_p[i]=0;
			//TRACE(dbg,DBG_DEBUG,"cmd %s ->result %s",cmd.c_str(),line_p);
			if(line_p[0]=='1')
				{
				db->dbSave();	// save event db

				TRACE(dbg,DBG_NOTIFY,"now kill me -- bye bye!");
				tsrv->closeClientConnection(tsrv->getActualClient());
				gd.run=false;
				sleep(1);
				}
			else
				{
				cmd="rm -rf " + _S DB_RESTORE_PATH + _S DB_VARCOLANSV;
				system(cmd.c_str());
				TRACE(dbg,DBG_ERROR,"database not restored: invalid file");
				}
			strcpy(output,"ok");
			}

		}

	//......................................................
	//......................................................
	else
		{
		throw 100;	// generic syntax error
		}
	}
catch(int error_no)
	{
	ret=false;
	switch(error_no)
		{
		case 1:
		TRACE(dbg,DBG_ERROR,"SCRIPT: unknown device to update @line %d",nline);
		break;
		case 2:
		TRACE(dbg,DBG_ERROR,"SCRIPT: SYNTAX: exe without script name @line %d",nline);
		break;
		case 3:
		TRACE(dbg,DBG_ERROR,"SCRIPT: firmware not found");
		break;
		case 4:
		TRACE(dbg,DBG_ERROR,"SCRIPT: not enough fields in badge definition @line %d",nline);
		break;
		case 5:
		TRACE(dbg,DBG_ERROR,"SCRIPT: wrong badge number length  @line %d",nline);
		break;
		case 6:
		TRACE(dbg,DBG_ERROR,"SCRIPT: not enough fields (%d)",nline,toks.size());
		break;
		case 7:
		TRACE(dbg,DBG_ERROR,"SCRIPT: function not implemented in this configuration @line %d",nline);
		break;
		case 8:
		TRACE(dbg,DBG_ERROR,"SCRIPT: invalid key");
		break;
		case 9:
		TRACE(dbg,DBG_ERROR,"SCRIPT: invalid data field");
		break;
		case 10:
		TRACE(dbg,DBG_ERROR,"no reader available, this operation cannot be performed");
		break;

		case 100:
			TRACE(dbg,DBG_ERROR,"SCRIPT: SYNTAX error @line %d in [%s]",nline,line.c_str());
		break;
		}
	}
return ret;
}

//-----------------------------------------------------------------------------
// PRIVATE
//-----------------------------------------------------------------------------
int Script::dayMap(string day)
{
//mon|tue|wed|thu|fri|sat|sun
if(startsWith(day,"mon") || startsWith(day,"lu"))
	{
	return 0;
	}
else if(startsWith(day,"tue") || startsWith(day,"ma"))
	{
	return 1;
	}
else if(startsWith(day,"wed") || startsWith(day,"me"))
	{
	return 2;
	}
else if(startsWith(day,"thu") || startsWith(day,"gi"))
	{
	return 3;
	}
else if(startsWith(day,"fri") || startsWith(day,"ve"))
	{
	return 4;
	}
else if(startsWith(day,"sat") || startsWith(day,"sa"))
	{
	return 5;
	}
else if(startsWith(day,"sun") || startsWith(day,"do"))
	{
	return 6;
	}
else
	return -1;
}
