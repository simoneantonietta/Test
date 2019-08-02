/*
 * TestUtils.h
 *
 *  Created on: Mar 23, 2015
 *      Author: saet
 */

#ifndef TESTUTILS_H_
#define TESTUTILS_H_


#include <string>
#include "CommSC8R.h"

#define N_RSSI_PNTS					500
#define PARAMSET_DEFAULT		0
#define PARAMSET_TESTING		1

#define DUT_TYPE_SAET				0
#define DUT_TYPE_SG					1

using namespace std;

class TestUtils
{
public:
	TestUtils();
	virtual ~TestUtils();

	typedef struct status_st
	{
		uint8_t alarm1			:1;
		uint8_t alarm2			:1;
		uint8_t tamper			:1;
		uint8_t battery_low	:1;
		uint8_t fail				:1;
		uint8_t live1				:1;
		uint8_t live2				:1;
	} status_t;

	bool programDevice(string fname,char mode='a');
	bool SC8_openComm(bool useCal, bool useSen);
	bool SC8_testMode(CommSC8R *sc8);
	bool SC8_testModeEnd(CommSC8R *sc8);
	bool SC8_devCalibration();
	bool SC8_devMeasureSens();
	int currentProfile(int minCurrent,int maxCurrent);
	int SC8_rssiAnalisys(uint8_t *rssi, int nPnt, uint8_t vth);
	int fitToCurveAnalisys(int start, int end, uint8_t vth);
	uint8_t SC8_devStatus();
	bool SC8_devSetUID(uint8_t *oldId,uint8_t *newId);
	bool SC8_setDevParameters(int paramSet);
	bool SC8_setDeviceType(int typ);

private:
	struct radioInfo_st
		{
			uint8_t radio_id[4];
			uint8_t rssi[N_RSSI_PNTS];
			uint8_t cmd;
			uint8_t band;
			uint16_t stepInit;
			uint8_t stepDelta;
			uint8_t flag;
			uint8_t sweep;
			uint8_t delayMeas;
			uint16_t stepFreqPeak;
			uint8_t stepWidthBand;
			uint8_t RSSIPeak;
		} *info;

	bool res;
	string strRes;

	void plotRssi();
	void plotCurr();
	bool loadRSSICurve(string fname, int *rssi, unsigned int *npt);
	bool loadGPCurve(string fname, float *xi, float *yi, unsigned int *npt);
	void fft(unsigned int logN, double *real, double *im);
};

#endif /* TESTUTILS_H_ */
