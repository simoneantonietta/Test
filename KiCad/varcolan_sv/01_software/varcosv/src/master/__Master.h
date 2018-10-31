/**----------------------------------------------------------------------------
 * PROJECT: varcosv
 * PURPOSE:
 * MAster functionality
 * WARN: functionality incomplete and not used!
 *-----------------------------------------------------------------------------  
 * CREATION: 9 Mar 2016
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

#ifndef DB_MASTER_H_
#define DB_MASTER_H_

#include "global.h"
#include <vector>
#include <algorithm>
#include "dataStructs.h"
#include "comm/app_commands.h"

using namespace std;

#define RETRY_CHECKNETWORK		2
#define DEVICE_DEAD_COUNT_MAX	20

class Master
{
public:
	typedef struct device_st
	{
	uint8_t type;
	uint8_t id;
	uint8_t timeoutCont;

	bool operator==(const int& id) const
		{
		return (this->id==id);
		}

	bool operator< (const device_st& other) const
		{
		return this->id < other.id;
		}
	} device_t;

	Master();
	virtual ~Master();

	void init();
	bool getDevices();
	void mainLoop();
	void deviceDataManagement();

	void sendBadgeData();
	void sendTerminalData();
	void sendWeektime();

private:
	typedef enum
	{
		ret_ok,
		ret_error,
		ret_no_answer,
	} ret_status_t;

	uint8_t txData[HPROT_BUFFER_SIZE];

//	vector<device_t> devices;
//	int maxIDValue;
//	int enabledDeviceNdx;
//	int dataManagementTimeout;


	int lastProfileBlock;
	ret_status_t profileManagement(int block);
	void sendProfileData(int ndx);

	ret_status_t eventManagement(int n);

	ret_status_t lostDeviceManagement(int id);

};

//-----------------------------------------------
#endif /* DB_MASTER_H_ */
