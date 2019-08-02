/*
 * TestUtils.cpp
 *
 *  Created on: Mar 23, 2015
 *      Author: saet
 */

#include <fstream>
#include <vector>
#include "global.h"
#include "GlobalData.h"
#include "TestUtils.h"
#include "utils/Trace.h"
#include "utils/Utils.h"
#include "acq/AcqInterface.h"

#define CURRENT_FNAME						"current.gp"
#define CURRENT_MEAS_TIME				2			// [s]
#define CURRENT_MEAS_MIN_TIME		100		// [ms]

#define PROGRAMMING_LOGFILE			"prog.log"
#define RADIO_CALIBRATION_FREQ	868300000

extern GlobalData *gd;
extern Trace *dbg;
extern CommSC8R *sc8cal;
extern CommSC8R *sc8sen;
extern AcqInterface *acq;

TestUtils::TestUtils()
{
// TODO Auto-generated constructor stub

}

TestUtils::~TestUtils()
{
// TODO Auto-generated destructor stub
}

/**
 * programming of the device
 * @param fname firmware file
 * @param mode 'a'=all memory; 'm' only main memory
 * @return result : true: ok
 */
bool TestUtils::programDevice(string fname,char mode)
{
ifstream f;
string line;
int nline = 0;
bool done = false;
string erase;

switch(mode)
	{
	case 'a':
		erase="ERASE_ALL";
		break;
	case 'm':
		erase="ERASE_MAIN";
		break;
	}
dbg->trace(DBG_NOTIFY, "Start the programmer... (erase mode: " + erase + ")");
//string cmd = (string)FIRMWARE_FOLDER + "MSP430Flasher -w \"" + FIRMWARE_FOLDER+fname + "\" -v -g -z [VCC] | grep -i \"error\" > " + PROGRAMMING_LOGFILE;
//string cmd = (string) FIRMWARE_FOLDER + "MSP430Flasher -w \"" + FIRMWARE_FOLDER + fname + "\" -e -v -g | grep -i \"error\" > " + PROGRAMMING_LOGFILE;
string cmd = (string) FIRMWARE_FOLDER + "flash.sh " + fname + " " + erase + " " + PROGRAMMING_LOGFILE;
dbg->trace(DBG_NOTIFY, cmd);
system(cmd.c_str());

f.open(((string) FIRMWARE_FOLDER + (string) PROGRAMMING_LOGFILE).c_str());
if (f.good())
	{
	while (!f.eof() && !done)
		{
		getline(f, line);
		nline++;
		toLowerStr(line);
		dbg->trace(DBG_NOTIFY, "PROGRAMMER: " + line);

		// check for errors
#define NERROR_CHECK		3
		std::size_t found[NERROR_CHECK];
		bool fail=false;
		found[0] = line.find("# ERROR");
		found[1] = line.find("# error");
		found[2] = line.find("DebugStack not initialized");

		for(int e=0;e<NERROR_CHECK;e++)
			{
			if (found[e] != std::string::npos) fail=true;
			}

		if (fail)
			{
			// an error occur
			dbg->trace(DBG_NOTIFY, "programming ERROR");
			res = false;
			strRes = "programmer error";
			done = true;
			}
		else
			{
			std::size_t found = line.find("(no error)");
			if (found != std::string::npos)
				{
				dbg->trace(DBG_NOTIFY, "programming success");
				res = true;
				strRes = "";
				done = true;
				}
			}
		}
	if (!res)
		{
		dbg->trace(DBG_ERROR, "unable to program the device: take a look to 'tmpflash.log' file");
		res = false;
		strRes = "";
		}
	}
else
	{
	dbg->trace(DBG_ERROR, "unable to read prog.log file");
	res = false;
	strRes = "";
	}

return res;
}

/**
 * open SC8 communications
 * @param useCal select calibration device
 * @param useSen select sensitivity device
 * @return
 */
bool TestUtils::SC8_openComm(bool useCal, bool useSen)
{
if (useCal)
	{
	if (!sc8cal->openCommunication(gd->sc8rDeviceCal, SC8R_BAUDRATE))
		{
		dbg->trace(DBG_ERROR, "unable to open sc8 calibration %s port", gd->sc8rDeviceCal);
		return false;
		}
	dbg->trace(DBG_NOTIFY, "open %s port for SC8R calibration success", gd->sc8rDeviceCal);
	}
if (useSen)
	{
	if (!sc8sen->openCommunication(gd->sc8rDeviceSen, SC8R_BAUDRATE))
		{
		dbg->trace(DBG_ERROR, "unable to open sc8 sensitivity %s port", gd->sc8rDeviceSen);
		return false;
		}
	dbg->trace(DBG_NOTIFY, "open %s port for SC8R sensitivity success", gd->sc8rDeviceSen);
	}

return true;
}

/**
 * set the sc8 in test mode
 * @param sc8 which sc8 will be treated
 * @return true:ok
 */
bool TestUtils::SC8_testMode(CommSC8R *sc8)
{
bool ret = true;
uint8_t data[100];	// generic buffer
uint8_t *rxdata;
int n;
try
	{
	if (!sc8->isConnected())
		{
		dbg->trace(DBG_ERROR, "sc8 not connected cannot continue");
		throw false;
		}
	dbg->trace(DBG_NOTIFY, "SC8R: setup test mode");
	// set in test mode
	data[0] = 0x30;  // time (0x20)
	data[1] = 0x00;  // "
	sc8->sendMsg(SC8R_CMD_TESTMODE_PERIF, SC8R_ADDR, gd->codeIDexp, data, 2);
	n = sc8->readMsg(SC8R_DEF_TIMEOUT);
	rxdata = sc8->getRxData();
	if (n > 0)
		{
		if (rxdata[2] == 0x31)
			{
			dbg->trace(DBG_NOTIFY, "SC8R: test mode active");
			}
		else
			{
			dbg->trace(DBG_ERROR, "SC8R: test mode fail!");
			throw false;
			}
		}
	else
		{
		throw false;
		}
	}
catch (...)
	{
	ret = false;
	}
return ret;
}

/**
 * exit from the test mode of the sc8
 * @param sc8 which sc8 will be treated
 * @return true:ok
 */
bool TestUtils::SC8_testModeEnd(CommSC8R *sc8)
{
bool ret = true;
//uint8_t device[4]={0x00,0x00,0x00,DUT_TYPE};
uint8_t data[100];	// generic buffer
uint8_t *rxdata;
int n;
try
	{
	if (!sc8->isConnected())
		{
		dbg->trace(DBG_ERROR, "sc8 not connected cannot continue");
		throw false;
		}
	dbg->trace(DBG_NOTIFY, "SC8R: exit test mode");
	// set in test mode
	data[0] = 0x00;  // time
	data[1] = 0x00;  // "
	sc8->sendMsg(SC8R_CMD_TESTMODE_PERIF, SC8R_ADDR, gd->codeIDexp, data, 2);
	n = sc8->readMsg(SC8R_DEF_TIMEOUT);
	rxdata = sc8->getRxData();
	if (n > 0)
		{
		if (rxdata[2] == 0x31)
			{
			dbg->trace(DBG_NOTIFY, "SC8R: test mode disabled");
			}
		else
			{
			dbg->trace(DBG_ERROR, "SC8R: test mode exit fail!");
			throw false;
			}
		}
	else
		{
		throw false;
		}
	}
catch (...)
	{
	ret = false;
	}
return ret;
}


/**
 * set the sc8 for the device calibration
 * @return true:ok
 */
bool TestUtils::SC8_devCalibration()
{
bool ret = true;
//uint8_t device[4]={0x00,0x00,0x00,DUT_TYPE};
uint8_t data[100];	// generic buffer
int ndx = 0;
uint8_t *rxdata;
int n;
CommSC8R *sc8;

if(gd->useSC8Rcal)
	{
	sc8=sc8cal;
	}
else
	{
	sc8=sc8sen;
	}

try
	{
	if (!sc8->isConnected())
		{
		dbg->trace(DBG_ERROR, "sc8 not connected cannot continue");
		throw false;
		}
	// calibration
	data[ndx++] = 0x32;
	data[ndx++] = 0x73;
	data[ndx++] = 0x80;
	data[ndx++] = 0x66;
	data[ndx++] = 0xC0;
	data[ndx++] = 0x67;
	data[ndx++] = 0x04;
	data[ndx++] = 0x04;
	data[ndx++] = 0x01;
	data[ndx++] = 0x0A;
	sc8->sendMsg(SC8R_CMD_CALIBRATION, SC8R_ADDR, gd->codeIDexp, data, ndx);

	n = sc8->readMsg(SC8R_CAL_TIMEOUT);
	rxdata = sc8->getRxData();
	if (n > N_RSSI_PNTS)
		{
		if (rxdata[1] == 0xf1)  // right message
			{
			// map radio info
			info = (radioInfo_st *) &rxdata[2];
			if (info->RSSIPeak < 70)
				{
				dbg->trace(DBG_ERROR, "peak rssi too low (%u)", info->RSSIPeak);
				}
			else
				{
				dbg->trace(DBG_NOTIFY, "peak rssi: %u", info->RSSIPeak);
				}
			// calculates other parameters
			unsigned int hbs = (info->band >> 5) & 1;
			info->band = info->band & 0x1F;
			unsigned int valid = info->flag & (1 << 6);
			unsigned int fpeak = (hbs + 1) * 10000000 * (24 + info->band) + (hbs + 1) * (info->stepFreqPeak * info->stepDelta + info->stepInit) * 156.25;
			int bandwidth = info->stepDelta * (hbs + 1) * 156.25 * info->stepWidthBand;
			int fdelta = fpeak - RADIO_CALIBRATION_FREQ;

			// write data file
			ofstream rssifile;
			rssifile.open("rssi.txt");
			for (int i = 0; i < N_RSSI_PNTS; i++)
				{
				rssifile << to_string(i) << "\t" << to_string((unsigned int) info->rssi[i]) << "\n";
				}
			rssifile.close();

			if (valid)
				{
				dbg->trace(DBG_NOTIFY, "frequency peak: %0.6f MHz", (float) fpeak / 1000000);
				dbg->trace(DBG_NOTIFY, "bandwidth: %d", bandwidth);
				dbg->trace(DBG_NOTIFY, "delta freq.: %0.6f MHz", (float) fdelta / 1000000);
				gd->res_freq = (uint32_t) (fpeak / 1000);
				gd->res_delta = (uint32_t) (fdelta / 1000);
				gd->res_bandwidth = (uint32_t) bandwidth;
				}
			else
				{
				dbg->trace(DBG_WARNING, "Invalid peak data");
				throw false;
				}
			}
		else
			{
			dbg->trace(DBG_WARNING, "unexpected answer from sc8");
			throw false;
			}
		}
	else
		{
		dbg->trace(DBG_WARNING, "unexpected answer from sc8");
		throw false;
		}
	}
catch (...)
	{
	ret = false;
	}
return ret;
}


/**
 * set the sc8 for the sensibility measurement
 * @return true:ok
 */
bool TestUtils::SC8_devMeasureSens()
{
bool ret = true;
//uint8_t device[4]={0x00,0x00,0x00,DUT_TYPE};
uint8_t data[100];	// generic buffer
int ndx = 0;
uint8_t *rxdata;
int n;

try
	{
	if (!sc8sen->isConnected())
		{
		dbg->trace(DBG_ERROR, "sc8 not connected cannot continue");
		throw false;
		}
	// measurement
	data[ndx++] = 0x31;
	data[ndx++] = 0x73;
	data[ndx++] = 0x80;
	data[ndx++] = 0x66;
	data[ndx++] = 0xC0;
	data[ndx++] = 0x67;
	data[ndx++] = 0x04;
	data[ndx++] = 0x04;
	data[ndx++] = 0x01;
	data[ndx++] = 0x0A;
	sc8sen->sendMsg(SC8R_CMD_CALIBRATION, SC8R_ADDR, gd->codeIDexp, data, ndx);

	n = sc8sen->readMsg(SC8R_CAL_TIMEOUT);
	rxdata = sc8sen->getRxData();
	if (n > N_RSSI_PNTS)
		{
		if (rxdata[1] == 0xf1)  // right message
			{
			// map radio info
			info = (radioInfo_st *) &rxdata[2];
			if (info->RSSIPeak < 70)
				{
				dbg->trace(DBG_ERROR, "peak rssi too low (%u dB)", info->RSSIPeak);
				}
			else
				{
				dbg->trace(DBG_ERROR, "peak rssi: %u dB", info->RSSIPeak);
				}
			// calculates other parameters
			unsigned int hbs = (info->band >> 5) & 1;
			info->band = info->band & 0x1F;
			unsigned int valid = info->flag & (1 << 6);
			unsigned int fpeak = (hbs + 1) * 10000000 * (24 + info->band) + (hbs + 1) * (info->stepFreqPeak * info->stepDelta + info->stepInit) * 156.25;
			int bandwidth = info->stepDelta * (hbs + 1) * 156.25 * info->stepWidthBand;
			int fdelta = fpeak - RADIO_CALIBRATION_FREQ;

			// write data file
			ofstream rssifile;
			rssifile.open("rssi.txt");
			for (int i = 0; i < N_RSSI_PNTS; i++)
				{
				rssifile << to_string(i) << "\t" << to_string((unsigned int) info->rssi[i]) << "\n";
				}
			rssifile.close();

			if (valid)
				{
				dbg->trace(DBG_NOTIFY, "frequency peak: %0.6f MHz", (float) fpeak / 1000000);
				dbg->trace(DBG_NOTIFY, "bandwidth: %d", bandwidth);
				dbg->trace(DBG_NOTIFY, "delta freq.: %0.6f MHz", (float) fdelta / 1000000);

				// start analysis
				if (SC8_rssiAnalisys(info->rssi, N_RSSI_PNTS, gd->parm_sensThreshold) < 0)
					{
					throw false;
					}
				}
			else
				{
				dbg->trace(DBG_WARNING, "Invalid peak data");
				ret = false;
				throw false;
				}
			}
		else
			{
			dbg->trace(DBG_WARNING, "unexpected answer from sc8");
			throw false;
			}
		}
	else
		{
		dbg->trace(DBG_WARNING, "unexpected answer from sc8");
		throw false;
		}
	}
catch (...)
	{
	ret = false;
	}
plotRssi();
return ret;
}

/**
 * set the device type (SG or Saet or other else)
 * @param typ defines the type 0=Saet; 1=SG
 * @return true: ok
 */
bool TestUtils::SC8_setDeviceType(int typ)
{
uint8_t parmSaet[]={0x00,0x00};
uint8_t parmSG[]={0x00,0x01};
uint8_t data[10];	// generic buffer
int ndx = 0;
uint8_t *rxdata;
int n;
bool ret = false;
CommSC8R *sc8;
uint8_t expected;

if(gd->useSC8Rcal)
	{
	sc8=sc8cal;
	}
else
	{
	sc8=sc8sen;
	}

try
	{
	if (!sc8->isConnected())
		{
		dbg->trace(DBG_ERROR, "sc8 not connected cannot continue");
		throw false;
		}
	// params
	switch(typ)
		{
		case DUT_TYPE_SAET:
			expected=DUT_TYPE_SAET;
			dbg->trace(DBG_NOTIFY, "set DUT type: SAET");
			memcpy(&data[ndx],parmSaet,sizeof(parmSaet));
			ndx+=sizeof(parmSaet);
			break;
		case DUT_TYPE_SG:
			expected=DUT_TYPE_SG;
			dbg->trace(DBG_NOTIFY, "set DUT type: SG");
			memcpy(&data[ndx],parmSG,sizeof(parmSG));
			ndx+=sizeof(parmSG);
			break;
		}
	sc8->sendMsg(SC8R_CMD_SET_TYPE, SC8R_ADDR,0, data, ndx);

	n = sc8->readMsg(SC8R_DEF_TIMEOUT);
	rxdata = sc8->getRxData();
	if (n > 0)
		{
		if (rxdata[2] == expected)
			{
			dbg->trace(DBG_NOTIFY, "SC8R: set type OK");
			ret = true;
			}
		else
			{
			dbg->trace(DBG_ERROR, "SC8R: set type fail!");
			throw false;
			}
		}
	else
		throw false;
	}
catch (...)
	{
	dbg->trace(DBG_ERROR, "generic error in send parameters");
	ret = false;
	}
return ret;
}

/**
 * set the device parameters (i.e. inertial sensibility)
 * @param paramSet defines the set of parameter to be used (0=default; 1=test set)
 * @return true: ok
 */
bool TestUtils::SC8_setDevParameters(int paramSet)
{
uint8_t parmDefault[]={0x0A,0x05,0x7E,0x0F,0xFF,0xFF,0xFF,0x04};
uint8_t parmForTest[]={0x0A,0x05,0x03,0x02,0xFF,0xFF,0xFF,0x04};
uint8_t params[8];
uint8_t data[100];	// generic buffer
int ndx = 0;
uint8_t *rxdata;
int n;
bool ret = false;
CommSC8R *sc8;

if(gd->useSC8Rcal)
	{
	sc8=sc8cal;
	}
else
	{
	sc8=sc8sen;
	}

try
	{
	if (!sc8->isConnected())
		{
		dbg->trace(DBG_ERROR, "sc8 not connected cannot continue");
		throw false;
		}
	// params
	switch(paramSet)
		{
		case PARAMSET_DEFAULT:
			dbg->trace(DBG_NOTIFY, "set DUT parameter: DEFAULT");
			memcpy(&data[ndx],parmDefault,sizeof(parmDefault));
			memcpy(params,parmDefault,sizeof(parmDefault));	// for check
			ndx+=sizeof(parmDefault);
			break;
		case PARAMSET_TESTING:
			dbg->trace(DBG_NOTIFY, "set DUT parameter: TESTING");
			memcpy(&data[ndx],parmForTest,sizeof(parmForTest));
			memcpy(params,parmForTest,sizeof(parmForTest));	// for check
			ndx+=sizeof(parmForTest);
			break;
		}
//	data[ndx++] = 0x0A;  // time [s] shutter
//	data[ndx++] = 0x05;  // pulses shutter
//	data[ndx++] = 0x03;  // sensibility crash (def 0x19 ?)
//	data[ndx++] = 0x02;  // sensibility movement	(def 0x0F)
//	data[ndx++] = 0xFF;
//	data[ndx++] = 0xFF;
//	data[ndx++] = 0xFF;
//	data[ndx++] = 0x04;
	sc8->sendMsg(SC8R_CMD_WRITE_PARAMS, SC8R_ADDR, gd->codeIDexp, data, ndx);

	n = sc8->readMsg(SC8R_DEF_TIMEOUT);
	rxdata = sc8->getRxData();
	if (n > 0)
		{
		if (rxdata[2] == 0x31)
			{
			dbg->trace(DBG_NOTIFY, "SC8R: send parameters OK");
			ret = true;
			}
		else
			{
			dbg->trace(DBG_ERROR, "SC8R: send parameters FAIL!");
			throw false;
			}
		}
	else
		throw false;

	// check the written params reading them
	SC8_devStatus();
	if(memcmp(gd->DUT_params,params,sizeof(parmDefault))==0)
		{
		dbg->trace(DBG_NOTIFY, "SC8R: parameters check OK");
		ret = true;
		}
	else
		{
		dbg->trace(DBG_ERROR, "SC8R: parameters check FAIL!");
		throw false;
		}
	}
catch (...)
	{
	dbg->trace(DBG_ERROR, "generic error in send parameters");
	ret = false;
	}
return ret;
}

/**
 * status request: get the alarm status, tamper ecc
 * @param dev device
 * @return byte that represents the status
 */
uint8_t TestUtils::SC8_devStatus()
{
uint8_t data[100];	// generic buffer
int ndx = 0;
uint8_t *rxdata;
int n;
uint8_t status = 0xFF;
CommSC8R *sc8;

if(gd->useSC8Rcal)
	{
	sc8=sc8cal;
	}
else
	{
	sc8=sc8sen;
	}

try
	{
	if (!sc8->isConnected())
		{
		dbg->trace(DBG_ERROR, "sc8 not connected cannot continue");
		throw false;
		}
	// status
	//data[ndx++]=0xFF;
	//data[ndx++]=0xFF;
	//data[ndx++]=0xFF;
	//data[ndx++]=0xFF;
	//data[ndx++]=0xFF;
	uint8_t device[4] =
		{
		0xff, 0xff, 0xff, 0xff
		};	// last peripheral in test mode

	data[ndx++] = 0x02;
	sc8->sendMsg(SC8R_CMD_STATUS_PERIF, SC8R_ADDR, device, data, ndx);

	n = sc8->readMsg(SC8R_DEF_TIMEOUT);
	rxdata = sc8->getRxData();
	if (n > 8)
		{
		if (rxdata[1] == 0x89)  // right message
			{
			status = rxdata[7];
			dbg->trace(DBG_NOTIFY, "status: 0x%02X", status);
			gd->res_ReadedVBat=rxdata[41];
			// other info
			dbg->trace(DBG_NOTIFY, "DUT FW version: %02X%02X%02X%02X",rxdata[24],rxdata[23],rxdata[22],rxdata[21]);
			dbg->trace(DBG_NOTIFY, "DUT Battery voltage: %0.2f V (%02X)",(float)rxdata[41]*19.52/1000,rxdata[41]);
			// parameters
			memcpy(gd->DUT_params,&rxdata[11],8);

/*
028900000004FF15231E
030A057E0FFFFFFF0400
00100100000000000004
00000000000000000000
00 EB 0400FF0065001603
*/
			}
		}
	else
		throw false;
	}
catch (...)
	{
	dbg->trace(DBG_ERROR, "generic error in status request");
	}
return status;
}

/**
 * set the new interna unique ID
 * @param oldId
 * @param newId
 * @return true:ok
 */
bool TestUtils::SC8_devSetUID(uint8_t *oldId, uint8_t *newId)
{
uint8_t data[100];	// generic buffer
int ndx = 0;
uint8_t *rxdata;
int n;
bool ret = false;
CommSC8R *sc8;

if(gd->useSC8Rcal)
	{
	sc8=sc8cal;
	}
else
	{
	sc8=sc8sen;
	}

try
	{
	if (!sc8->isConnected())
		{
		dbg->trace(DBG_ERROR, "sc8 not connected cannot continue");
		throw false;
		}
	// new id
	data[ndx++] = newId[0];
	data[ndx++] = newId[1];
	data[ndx++] = newId[2];
	data[ndx++] = newId[3];

	sc8->sendMsg(SC8R_CMD_SETID, SC8R_ADDR, oldId, data, ndx);

	n = sc8->readMsg(SC8R_DEF_TIMEOUT);
	rxdata = sc8->getRxData();
	if (n > 0)
		{
		if (rxdata[2] == 0x31)
			{
			dbg->trace(DBG_NOTIFY, "SC8R: change UID ok");
			// store it
			gd->codeIDexp[0] = newId[3];
			gd->codeIDexp[1] = newId[2];
			gd->codeIDexp[2] = newId[1];
			gd->codeIDexp[3] = newId[0];

			ret = true;
			}
		else
			{
			dbg->trace(DBG_ERROR, "SC8R: change UID fail!");
			throw false;
			}
		}
	}
catch (...)
	{
	dbg->trace(DBG_ERROR, "generic error in change UID");
	ret = false;
	}
return ret;
}

/**
 * Analysis the rssi
 * Algorithm:
 * 1. set a threshold level, let it is vth
 * 2. check the cth intersections with rssi
 * 3. if nothing intersect and if max(RSSI) < vth -> fail
 * 4. if 2 points intersect, all data over it is stored in buff; goto 6
 * 5. if intersect more than 2 points reject and retry from the acquisition, if the case
 * 6. calculates the first derivative of buff
 * 7. count the zero crossing points, these should be 1 else -> retry
 * 8. if too many retry -> fail
 *
 * @param rssi
 * @param nPnt number of points
 * @param vth threshold
 * @return peak RSSI; negative numbers represents error codes
 */
int TestUtils::SC8_rssiAnalisys(uint8_t *rssi, int nPnt, uint8_t vth)
{
int ret = 0;
uint8_t max = 0;
int der;
try
	{
	// find the max value
	for (int i = 0; i < nPnt; i++)
		{
		if (max < rssi[i])
			{
			max = rssi[i];
			}
		}
	gd->res_sensRSSI = max;

	// check threshold, if greater than the maximum -> fail
	if (vth > max)
		{
		dbg->trace(DBG_ERROR, "threshold not reached");
		throw -1;
		}

	// search intersections
	int ndx_up = -1, ndx_dwn = -1;
	char st = 'u';
	int n_inters = 0;
	for (int i = 1; i < nPnt; i++)
		{
		der = rssi[i] - rssi[i - 1];
		switch (st)
			{
			case 'u':  // search threshold
				if (rssi[i] >= vth && der > 0)
					{
					st = 'd';
					n_inters++;
					ndx_up = i;
					}
				break;
			case 'd':
				if (rssi[i] <= vth && der < 0)
					{
					st = 'u';
					n_inters++;
					ndx_dwn = i;
					}
				break;
			}
		}

	dbg->trace(DBG_DEBUG, "intersection found at indexes: up=%d; dwn=%d", ndx_up, ndx_dwn);
	if (n_inters != 2)
		{
		dbg->trace(DBG_ERROR, "must find 2 intersections, but found %d", n_inters);
		throw -2;
		}

	// write extracted peak data file
	ofstream rssipkfile;
	rssipkfile.open("rssi_pk.txt");
	for (int i = ndx_up; i <= ndx_dwn; i++)
		{
		rssipkfile << to_string(i) << "\t" << to_string((unsigned int) info->rssi[i]) << "\n";
		}
	rssipkfile.close();

	int dev = fitToCurveAnalisys(ndx_up, ndx_dwn, vth);
	gd->res_sensDev = dev;
	if (dev > 0)
		{
		dbg->trace(DBG_ERROR, "curve deviation %u", dev);
		}
	else
		{
		ret = -4;
		}
	}
catch (int errorCode)
	{
	ret = errorCode;
	}
return ret;
}

//-----------------------------------------------------------------------------
// PRIVATE MEMBERS
//-----------------------------------------------------------------------------

/**
 * plot the RSSI in pdf running an external script
 */
void TestUtils::plotRssi()
{
char cmd[50];
sprintf(cmd, "./plot_rssi.sh %s %d", gd->codeID, gd->parm_sensThreshold);
system(cmd);
}

/**
 * plot the RSSI in pdf running an external script
 */
void TestUtils::plotCurr()
{
char cmd[50];
sprintf(cmd, "./plot_curr.sh %s %d %d", gd->codeID, gd->parm_txCurrentMin, gd->parm_txCurrentMax);
system(cmd);
}

/**
 * this routine attempts to fit the curve peak measured with the analitical one
 * to find the deviation from it
 * @param rssi rssi data vector
 * @param start,stop point to be considered
 * @param vth threshold
 * @param devmax maximum deviation for a valid curve
 * @return >0 the deviation; <0 errors
 */
#define MAX_INTERP_PNTS 	1000
int TestUtils::fitToCurveAnalisys(int start, int end, uint8_t vth)
{
int dev = 0;
float tmpl_xi[MAX_INTERP_PNTS], tmpl_yi[MAX_INTERP_PNTS],
    tmpl_dat[MAX_INTERP_PNTS];
unsigned int tmpl_npts, rssi_npts, rssi_n;
float rssi_dat[MAX_INTERP_PNTS];
int rssi[MAX_INTERP_PNTS];

// load the rssi data
if (!loadRSSICurve("rssi.txt", rssi, &tmpl_npts))
	{
	dbg->trace(DBG_ERROR, "unable to open 'rssi_pk.txt' file");
	return -1;
	}
// load the template
if (!loadGPCurve("interp.dat", tmpl_xi, tmpl_yi, &tmpl_npts))
	{
	dbg->trace(DBG_ERROR, "unable to open 'interp.dat'");
	return -2;
	}

// take only useful data
int j = 0;
for (int i = start; i <= end; i++)
	{
	rssi[j] = rssi[i];
	j++;
	}
rssi_npts = j;

// resample peak curve to fit template
if (rssi_npts < tmpl_npts)  // must downsample
	{
	// normalize rssi data
	for (unsigned int i = 0; i < rssi_npts; i++)
		{
		rssi_dat[i] = rssi[i] - vth;
		}

	// downsampling of the template to fit the rssi curve
	int step = roundf((float) tmpl_npts / (float) rssi_npts);
	float stepErr = ((float) tmpl_npts / (float) rssi_npts) - (float) step;
	float errAcc = 0, rem;
	unsigned int k = 0;
	for (unsigned int i = 0; i < rssi_npts; i++)
		{
		tmpl_dat[i] = tmpl_yi[k];
		k += step;
		errAcc += stepErr;
		if (fabs(errAcc) >= 1.0)
			{
			rem = (k + errAcc);
			k = roundf(k + errAcc);
			errAcc = rem - k;
			}
		if (k >= tmpl_npts) k = tmpl_npts - 1;
		}
	tmpl_npts = rssi_npts;	// update npoints

	// find min/max value in the template
	float tmplmin = 10000000000, tmplmax = 0;
	for (unsigned int i = 0; i < tmpl_npts; i++)
		{
		if (tmplmax < tmpl_dat[i]) tmplmax = tmpl_dat[i];
		if (tmplmin > tmpl_dat[i]) tmplmin = tmpl_dat[i];
		}
	// find min/max value in the rssi (minimum is intrinsic in the vth but for a general algo it search for it)
	float rssimin = 10000000000, rssimax = 0;
	for (unsigned int i = 0; i < rssi_npts; i++)
		{
		if (rssimax < rssi_dat[i]) rssimax = rssi_dat[i];
		if (rssimin > rssi_dat[i]) rssimin = rssi_dat[i];
		}
	// scale rssi curve
	float gain = (tmplmax - tmplmin) / (rssimax - rssimin);
	//float off=tmplmin;
	float off = (tmpl_dat[0] + tmpl_dat[tmpl_npts - 1]) / 2;	// mean of extremes
	for (unsigned int i = 0; i < rssi_npts; i++)
		{
		rssi_dat[i] = gain * (float) rssi_dat[i] + off;
		}

	// write resample/normalized rssi file
	ofstream devfile;
	devfile.open("dev.txt");
	for (unsigned int i = 0; i < rssi_npts; i++)
		{
		devfile << to_string(i) << "\t" << to_string(rssi_dat[i]) << "\t" << to_string(tmpl_dat[i]) << "\n";
		}
	devfile.close();

	// find deviation
	for (unsigned int i = 0; i < rssi_npts; i++)
		{
		dev += ceil(fabs(rssi_dat[i] - tmpl_dat[i]));
		}
	dbg->trace(DBG_DEBUG, "deviation: %u", (unsigned int) dev);
	}
else
	{
	dbg->trace(DBG_ERROR, "template points must be greater or equals the rssi points");
	}
return dev;
}

/**
 * load a curve data file created from gnuplot
 * i.e.:
 * set table "interp.dat"
 * plot [-1:1] (1-exp(x**2)**3)
 *
 * @param fname name of the file
 * @param rssi ptr to the destination vector
 * @param npt number of points
 * @return true:ok
 */
bool TestUtils::loadRSSICurve(string fname, int *rssi, unsigned int *npt)
{
bool ret = true;
string line;
vector<string> toks;
ifstream datfile;
int ndata = 0, nline = 0;

datfile.open(fname.c_str());
if (!datfile.good()) return false;

while (!datfile.eof())
	{
	getline(datfile, line);
	nline++;
	trim(line);
	if (line.empty()) continue;
	if (line[0] == '#') continue;
	toks.clear();

	Split(line, toks, " \t");
	rssi[ndata] = atoi(toks[1].c_str());
	ndata++;
	}
datfile.close();
*npt = ndata;

return ret;
}

/**
 * load a curve data file created from gnuplot
 * i.e.:
 * set table "interp.dat"
 * plot [-1:1] (1-exp(x**2)**3)
 *
 * @param fname name of the .dat file
 * @param xi ptr to the destination x vector
 * @param yi ptr to the destination y vector
 * @param npt number of points
 * @return true:ok
 */
bool TestUtils::loadGPCurve(string fname, float *xi, float *yi, unsigned int *npt)
{
bool ret = true;
string line;
vector<string> toks;
ifstream datfile;
int ndata = 0, nline = 0;

datfile.open(fname.c_str());
if (!datfile.good()) return false;

while (!datfile.eof())
	{
	getline(datfile, line);
	nline++;
	trim(line);
	if (line.empty()) continue;
	if (line[0] == '#') continue;
	toks.clear();

	Split(line, toks, " \t");
	xi[ndata] = atof(toks[0].c_str());
	yi[ndata] = atof(toks[1].c_str());
	ndata++;
	}
datfile.close();
*npt = ndata;

return ret;
}

/**
 * take the current profile, useful to check transmission current need
 * @param minCurrent validity limit used to identify the transmission chunk
 * @param maxCurrent validity limit
 * @return current max value [mA]; 0 = error
 */
int TestUtils::currentProfile(int minCurrent,int maxCurrent)
{
unsigned int ndx=0;
unlink(CURRENT_FNAME); // delete the previous file if exists

gd->parm_txCurrentMin=minCurrent;
gd->parm_txCurrentMax=maxCurrent;

//acq_t v;
//v.ivalue=1;
//acq->channel[acq->getIndex("current_scale")]->setValue(v);
for(int i=0;i<(CURRENT_MEAS_TIME*1000)/CURRENT_MEAS_MIN_TIME;i++)
	{
	ndx=acq->channel[acq->getIndex("current_mA")]->readBufferedValue(MILLISEC2SAMPLES(CURRENT_MEAS_MIN_TIME),string(CURRENT_FNAME),ndx);
	}
ifstream vfile;
string line;
vector<string> tok;
int nline=0;
vfile.open(CURRENT_FNAME);
bool validity=false;
float imax=0.0;

while(!vfile.eof())
	{
	getline(vfile,line);
	nline++;
	tok.clear();
	Split(line, tok, " \t");
	float v=strtof(tok[1].c_str(),NULL);
	if(imax<v) imax=v;	// max search
	if(v>(float)minCurrent)	// check for validity
		{
		validity=true;
		}
	}
if(!validity)
	{
	dbg->trace(DBG_WARNING, "transmission chunk not found");
	imax=0.0;
	}
else
	{
	dbg->trace(DBG_NOTIFY, "current peak: %d mA",(int)imax);
	}
vfile.close();

plotCurr();

return (int)imax;
}

/**
 * fft calculation
 * @param logN
 * @param real
 * @param im
 */
#define MINPI 3.1415926	// to be verified, perhaps have a minus
void TestUtils::fft(unsigned int logN, double *real, double *im)  // logN is base 2 log(N)
{
unsigned int n = 0, nspan, span, submatrix, node;
unsigned int N = 1 << logN;
double temp, primitive_root, angle, realtwiddle, imtwiddle;

for (span = N >> 1; span; span >>= 1)      // loop over the FFT stages
	{
	primitive_root = MINPI / span;     // define MINPI in the header

	for (submatrix = 0; submatrix < (N >> 1) / span; submatrix++)
		{
		for (node = 0; node < span; node++)
			{
			nspan = n + span;
			temp = real[n] + real[nspan];       // additions & subtractions
			real[nspan] = real[n] - real[nspan];
			real[n] = temp;
			temp = im[n] + im[nspan];
			im[nspan] = im[n] - im[nspan];
			im[n] = temp;

			angle = primitive_root * node;      // rotations
			realtwiddle = cos(angle);
			imtwiddle = sin(angle);
			temp = realtwiddle * real[nspan] - imtwiddle * im[nspan];
			im[nspan] = realtwiddle * im[nspan] + imtwiddle * real[nspan];
			real[nspan] = temp;

			n++;   // not forget to increment n

			}  // end of loop over nodes

		n = (n + span) & (N - 1);   // jump over the odd blocks

		}  // end of loop over submatrices

	}  // end of loop over FFT stages

}  // end of FFT function


