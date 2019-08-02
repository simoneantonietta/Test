/**----------------------------------------------------------------------------
 * PROJECT: acqsrv
 * PURPOSE:
 * Thi class have to be adapted to the HW used
 *-----------------------------------------------------------------------------  
 * CREATION: 11 Mar 2015
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

#ifndef ACQINTERFACE_H_
#define ACQINTERFACE_H_

#include <vector>
#include <memory>
#include "../global.h"
#include "../GlobalData.h"
#include "../utils/Utils.h"

#include "ChannelBase.h"
#include "Daq2206.h"

#ifndef ACQ_SIMUL

#define ACQ_D2K2206_CARD_TYPE		DAQ_2206
#define ACQ_D2K2206_CARD_NUMB		0

#else
// SIMULATED
#define ACQ_D2K2206_CARD_TYPE		1
#define ACQ_D2K2206_CARD_NUMB		0
#define AD_B_10_V 							(1<<0)
#define AI_RSE									(1<<1)
#define Channel_P1A							0
#define Channel_P1B							1
#define Channel_P1C							2
#endif

extern Trace *dbg;
extern GlobalData *gd;

using namespace std;



class AcqInterface
{
public:

	typedef vector<ChannelBase*>::iterator chan_it;

	// HW related variables: please adapt on your board
	uint8_t port1A, port1B, port1CL,  port1CH;
	uint16_t card1;

	/*
	 * channel database
	 */
	vector<ChannelBase*> channel;
	//................................................
	AcqInterface() {};
	virtual ~AcqInterface()
	{
	FOR_EACH(it,ChannelBase*,channel)
		{
		delete *it;
		}
#ifndef ACQ_SIMUL
	D2K_Release_Card (card1);
#endif
	};


	/**
	 * get the iterator of the channel with a specified name, so you can use it to handle the channel
	 * @param name
	 * @return iterator to pointer of the channel class, or end() if not found
	 */
	chan_it getIter(string name)
	{
	FOR_EACH(it,ChannelBase*,channel)
		{
		if((*it)->getName()==name) return it;
		}
	return channel.end();
	}
	/**
	 * get the index of the channel with a specified name, so you can use it to handle the channel
	 * @param name
	 * @return index, -1 if not found
	 */
	int getIndex(string name)
	{
	FOR_EACH(it,ChannelBase*,channel)
		{
		if((*it)->getName()==name) return GET_NDXFROMITERATOR(channel,it);
		}
	return -1;
	}

	/**
	 * routine to report the declared channels
	 * @param mode 0 normal text; 1 as define; 2 as enum
	 *
	 */
	void printSummary(int mode)
	{
	int ch=0;
	string ty;

	printf("ACQ Channels definitions summary:\n");
	FOR_EACH(it,ChannelBase*,channel)
		{
		switch(mode)
			{
			case 0:
					switch((*it)->type)
						{
						case ChannelBase::ch_analog_in:
							ty="AI";
							break;
						case ChannelBase::ch_analog_out:
							ty="AO";
							break;
						case ChannelBase::ch_digital_in:
							ty="DI";
							break;
						case ChannelBase::ch_digital_out:
							ty="DO";
							break;
						case ChannelBase::ch_unknown:
							ty="??";
							break;
						}
					printf("%02d (0x%02X)\t%s\t%s\n",ch,ch,ty.c_str(),(*it)->getName().c_str());
				break;
			case 1:
				switch((*it)->type)
						{
						case ChannelBase::ch_analog_in:
							ty="AI";
							break;
						case ChannelBase::ch_analog_out:
							ty="AO";
							break;
						case ChannelBase::ch_digital_in:
							ty="DI";
							break;
						case ChannelBase::ch_digital_out:
							ty="DO";
							break;
						case ChannelBase::ch_unknown:
							ty="??";
							break;
						}
				printf("#define ACQCH_%s_%s\t0x%02X // %d\n",ty.c_str(),(*it)->getName().c_str(),ch,ch);
				break;
			case 2:
				printf("enum acqch_e\n{\n");
				switch((*it)->type)
						{
						case ChannelBase::ch_analog_in:
							ty="AI";
							break;
						case ChannelBase::ch_analog_out:
							ty="AO";
							break;
						case ChannelBase::ch_digital_in:
							ty="DI";
							break;
						case ChannelBase::ch_digital_out:
							ty="DO";
							break;
						case ChannelBase::ch_unknown:
							ty="??";
							break;
						}
				printf("\t ACQCH_%s_%s = 0x%02X, // %d\n",ty.c_str(),(*it)->getName().c_str(),ch,ch);
				break;
			}
		ch++;
		}
	if(mode==2)
		{
		printf("};\n");
		}
	}

	//=============================================================================
	/**
	 * Initialise all the acq interfaces an channels
	 * This member is specific for the HW used
	 * @return
	 */
	bool init()
	{
	bool ret = false;
	Daq2206_digital *dch;
	Daq2206_analog *ach;

	try
		{
		//************************
		// setup of the card
		// card number depends on the system
#ifndef ACQ_SIMUL
		if ((card1 = D2K_Register_Card(ACQ_D2K2206_CARD_TYPE, ACQ_D2K2206_CARD_NUMB)) < 0)
			{
			dbg->trace(DBG_ERROR, "Register_Card error=%d", card1);
			throw false;
			}
#endif
		//............................................
		// setup the ANALOG INPUT channels used
		ach=new Daq2206_analog;
		ach->info.card=card1;
		ach->info.nMeans=10;
		ach->info.sampleRate=10;	// [Hz]
		ach->info.value=0;
		ach->info.cfg=AD_U_10_V | AI_RSE;
		ach->channel=0;
		ach->setup("current",ChannelBase::ch_analog_in);
		//ach->setupLinearConversion(-30665,-12303,0.95,9.5);	// most sensible
		ach->setupLinearConversion(-32767,-30969,0.0001,0.0362);	// most sensible
		channel.push_back((ChannelBase*) ach);

		// setup the ANALOG INPUT channels used (virtual: the same as over but whith a different scale)
		ach=new Daq2206_analog;
		ach->info.card=card1;
		ach->info.nMeans=10;
		ach->info.sampleRate=10;	// [Hz]
		ach->info.value=0;
		ach->info.cfg=AD_U_10_V | AI_RSE;
		ach->channel=0;
		ach->setup("current_mA",ChannelBase::ch_analog_in);
		//ach->setupLinearConversion(-29854,-4028,9.75,95.9);	// less sensible
		ach->setupLinearConversion(-32679,-24000,0.360,34.59);	// less sensible
		channel.push_back((ChannelBase*) ach);

		ach=new Daq2206_analog;
		ach->info.card=card1;
		ach->info.nMeans=0;
		ach->info.sampleRate=0;
		ach->info.value=0;
		ach->info.cfg=AD_U_10_V | AI_RSE;
		ach->channel=1;
		ach->setup("led",ChannelBase::ch_analog_in);
		ach->setupLinearConversion(0x0000,0x7fff,5,10.0);
		channel.push_back((ChannelBase*) ach);

		ach=new Daq2206_analog;
		ach->info.card=card1;
		ach->info.nMeans=0;
		ach->info.sampleRate=0;
		ach->info.value=0;
		ach->info.cfg=AD_U_10_V | AI_RSE;
		ach->channel=2;
		ach->setup("tamper",ChannelBase::ch_analog_in);
		channel.push_back((ChannelBase*) ach);

		ach=new Daq2206_analog;
		ach->info.card=card1;
		ach->info.nMeans=0;
		ach->info.sampleRate=0;
		ach->info.value=0;
		ach->info.cfg=AD_U_10_V | AI_RSE;
		ach->channel=3;
		ach->setup("ampout",ChannelBase::ch_analog_in);
		//ach->setupLinearConversion(0x0000,0x7fff,5,10.0);
		float gain_U8A=5.04;
		ach->setupLinearConversion(0x0000,0x7fff,5/gain_U8A,10.0/gain_U8A);
		channel.push_back((ChannelBase*) ach);

		ach=new Daq2206_analog;
		ach->info.card=card1;
		ach->info.nMeans=0;
		ach->info.sampleRate=0;
		ach->info.value=0;
		ach->info.cfg=AD_U_10_V | AI_RSE;
		ach->channel=4;
		ach->setup("sens_d",ChannelBase::ch_analog_in);
		//ach->setupLinearConversion(0x0000,0x7fff,5,10.0);
		float gain_U7B=5.04;
		ach->setupLinearConversion(0x0000,0x7fff,5/gain_U7B,10.0/gain_U7B);
		channel.push_back((ChannelBase*) ach);

		ach=new Daq2206_analog;
		ach->info.card=card1;
		ach->info.nMeans=0;
		ach->info.sampleRate=0;
		ach->info.value=0;
		ach->info.cfg=AD_U_10_V | AI_RSE;
		ach->channel=5;
		ach->setup("sens_s",ChannelBase::ch_analog_in);
		//ach->setupLinearConversion(0x0000,0x7fff,5,10.0);
		float gain_U7A=5.04;
		ach->setupLinearConversion(0x0000,0x7fff,5/gain_U7A,10.0/gain_U7A);
		channel.push_back((ChannelBase*) ach);

		//............................................
		// setup the DIGITAL INPUT channels used
		dch=new Daq2206_digital;
		dch->info.portValue=&port1B;
		dch->info.card=card1;
		dch->info.port=Channel_P1B;
		dch->channel=0;
		dch->setup("usw1",ChannelBase::ch_digital_in);
		channel.push_back((ChannelBase*) dch);

		dch=new Daq2206_digital;
		dch->info.card=card1;
		dch->info.port=Channel_P1B;
		dch->info.portValue=&port1B;
		dch->channel=1;
		dch->setup("usw2",ChannelBase::ch_digital_in);
		channel.push_back((ChannelBase*) dch);

		//............................................
		// setup the DIGITAL OUTPUT channels used
		dch=new Daq2206_digital;
		dch->info.card=card1;
		dch->info.port=Channel_P1A;
		dch->info.portValue=&port1A;
		dch->channel=3;
		dch->setup("rele0",ChannelBase::ch_digital_out);	// power
		channel.push_back((ChannelBase*) dch);

		dch=new Daq2206_digital;
		dch->info.card=card1;
		dch->info.port=Channel_P1A;
		dch->info.portValue=&port1A;
		dch->channel=1;
		dch->setup("rele1",ChannelBase::ch_digital_out);	// pgm_power
		channel.push_back((ChannelBase*) dch);

		dch=new Daq2206_digital;
		dch->info.card=card1;
		dch->info.port=Channel_P1A;
		dch->info.portValue=&port1A;
		dch->channel=5;
		dch->setup("rele2",ChannelBase::ch_digital_out);	// spare
		channel.push_back((ChannelBase*) dch);

		dch=new Daq2206_digital;
		dch->info.card=card1;
		dch->info.port=Channel_P1A;
		dch->info.portValue=&port1A;
		dch->channel=7;
		dch->setup("rele3",ChannelBase::ch_digital_out);	// spare
		channel.push_back((ChannelBase*) dch);

		dch=new Daq2206_digital;
		dch->info.card=card1;
		dch->info.port=Channel_P1A;
		dch->info.portValue=&port1A;
		dch->channel=0;
		dch->setup("rele4",ChannelBase::ch_digital_out);	// current scale
		channel.push_back((ChannelBase*) dch);

		dch=new Daq2206_digital;
		dch->info.card=card1;
		dch->info.port=Channel_P1CL;
		dch->info.portValue=&port1CL;
		dch->channel=0;
		dch->setup("lamp",ChannelBase::ch_digital_out);
		channel.push_back((ChannelBase*) dch);

		}
	catch (...)
		{
		ret = false;
		}
	return ret;
	}

	/**
	 * set a safe condition for all outputs and inputs
	 */
	void setSafeCondition()
	{
	acq_t v;

	v.ivalue=0;
	channel[0x0A]->setValue(v);
	channel[0x0B]->setValue(v);
	channel[0x0C]->setValue(v);
	}

	//=============================================================================
};

//-----------------------------------------------
#endif /* ACQINTERFACE_H_ */
