/**----------------------------------------------------------------------------
 * PROJECT: rasp_varcosv
 * PURPOSE:
 * perform a a time an operation
 *-----------------------------------------------------------------------------  
 * CREATION: 26 May 2016
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

#ifndef SCHEDULER_H_
#define SCHEDULER_H_

#include "global.h"
#include <vector>
#include <algorithm>    // std::sort

using namespace std;

class Scheduler
{
public:
	typedef struct sched_st
	{
		time_t sched_time;
		string command;

		sched_st() : sched_time(0), command("") {};
		bool operator<(const sched_st &tother) const
		{
		return this->sched_time < tother.sched_time;
		}
	} sched_t;

	typedef vector<sched_t>::iterator sched_it;


	Scheduler(string fname) {jobFilename=fname;}
	virtual ~Scheduler() {};


	/**
	 * add a scheduled job
	 * @param t		epoch time
	 * @param actualSystemTime actual epoch time
	 * @param cmd	command
	 * @return number of jobs scheduled
	 */
	int add(time_t t,time_t actualSystemTime,string cmd)
	{
	int timeleft;
	sched_t s;
	s.sched_time=t;
	s.command=cmd;
	schedJobs.push_back(s);
	sort(schedJobs.begin(),schedJobs.end());
	saveJobs();

	timeleft=t-(int)actualSystemTime;
	if(timeleft<0)
		{
		TRACE(dbg,DBG_NOTIFY,"scheduled job start in no more than 1 min", timeleft);
		}
	else
		{
		TRACE(dbg,DBG_NOTIFY,"scheduled job start in less than %d min", timeleft/60);
		}

	return schedJobs.size();
	}

	/**
	 * clear all jobs
	 */
	void clearAllJobs()
	{
	schedJobs.clear();
	remove(JOB_FILE);
	}


	/**
	 * get info about the more near job to be executed
	 * @return iterator to the element
	 */
	sched_it getInfoNextJob()
	{
	if(!schedJobs.empty())
		{
		return schedJobs.begin();
		}
	else
		{
		return schedJobs.end();
		}
	}

	/**
	 * get the next element and pop it!
	 * @return elment scheduled
	 */
	sched_t popNextJob()
	{
	sched_t s;
	if(!schedJobs.empty())
		{
		s=schedJobs[0];
		schedJobs.erase(schedJobs.begin());
		saveJobs();
		}
	else
		{
		s=*schedJobs.end();
		}
	return s;
	}

	/**
	 * tell if the job list is empty
	 * @return true: empty
	 */
	bool isEmpty()
	{
	return schedJobs.empty();
	}

	/**
	 * save jobs in a text file
	 * @param fname
	 * @return
	 */
	bool saveJobs()
	{
	ofstream f;

	f.open(jobFilename.c_str(),ofstream::out);
	if(f.good())
		{
		FOR_EACH(it,sched_t,schedJobs)
			{
			f << it->sched_time << "#" << it->command << "\n";
			}
		f.close();
		}
	else
		return false;
	return true;
	}

	/**
	 * load some saved jobs
	 * @param fname
	 * @return true ok;
	 */
	bool loadJobs()
	{
	ifstream f;
	string line;
	vector<string> toks;
	int nline=0;
	sched_t job;

	if(fileExists(jobFilename))
		{
		schedJobs.clear();
		f.open(jobFilename.c_str(),ofstream::in);
		while(f.good())
			{
			toks.clear();
			getline(f,line);
			trim(line);
			if(line.empty()) continue;
			nline++;
			Split(line,toks,"#\n");
			if(toks.size()==2)
				{
				job.sched_time=to_number<time_t>(toks[0]);
				job.command=toks[1];
				schedJobs.push_back(job);
				}
			else
				{
				TRACE(dbg,DBG_ERROR,"invalid job format at line %d", nline);
				return false;
				}
			TRACE(dbg,DBG_NOTIFY,"loaded %d scheduled jobs", schedJobs.size());
			f.close();
			}
		}
	else
		{
		TRACE(dbg,DBG_NOTIFY,"job file %s not found, assume no scheduled jobs programmed",jobFilename.c_str());
		}
	return true;
	}

private:
	string jobFilename;
	vector<sched_t> schedJobs;
};

//-----------------------------------------------
#endif /* SCHEDULER_H_ */
