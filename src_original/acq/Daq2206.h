/**----------------------------------------------------------------------------
 * PROJECT: acqsrv
 * PURPOSE:
 * class for the handling of Adlink Daq2206 board
 * (specialization for this project)
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

#ifndef ACQ_DAQ2206_H_
#define ACQ_DAQ2206_H_

#include <stdint.h>
#include "../global.h"

#include "ChannelBase.h"
#include "../utils/Trace.h"

#ifndef ACQ_SIMUL
#include "d2kdask.h"
#endif

// if commented uses the internal board sampling mode. This is usefull to sample with slow timings
//#define USE_SOFTWARE_SAMPLING

#define TIMEBASE						40000000
#define SAMPLING_FREQ				1000
#define CNT1								(TIMEBASE/SAMPLING_FREQ)
#define RANGE								AD_B_5_V // AD_B_10_V
#define MILLISEC2SAMPLES(v) (v*(SAMPLING_FREQ/1000))

extern Trace *dbg;


//=============================================================================
/*
 * analog channel
 */
class Daq2206_analog : public ChannelBase
{
public:
	Daq2206_analog() {};
	virtual ~Daq2206_analog()
	{
#ifndef ACQ_SIMUL
	D2K_Release_Card (info.card);
#endif
	}

	typedef struct chinfo_st
	{
		uint16_t nMeans;			// number of means
		uint16_t card;
		uint16_t Id;					// buffer id
		uint16_t sampleRate;
		int16_t value;				// value as read/written
		uint16_t cfg;					// internal configuration
	} chinfo_t;
	chinfo_t info;


	/**
	 * channel setup
	 * @param name
	 * @param type
	 * @param data
	 * @return
	 */
	bool setup(const char* name, chtype_e type, void *data=NULL)
	{
	bool ret=false;
	info.value=0;
#ifndef ACQ_SIMUL
	D2K_AI_CH_Config(info.card, channel, info.cfg);
#endif
	// as default is converted in float t.f. =1
	ret |= ChannelBase::setup(name,type,data);
	setupLinearConversion(1.0,0.0);
	return !ret;
	}

	/**
	 * read analog channel routine
	 * @param channel
	 * @param data
	 * @return raw value
	 */
	acq_t readCh(int channel, void *data=NULL)
	{
	uint16_t err, id;
	uint16_t tmp;
	long int acc;
	acq_t val;
	val.ivalue=0;
	switch(type)
		{
		//......................................
		case ch_analog_in:
			try
				{
				// multiple read
				if (info.nMeans > 1)
					{
//..............
#ifdef USE_SOFTWARE_SAMPLING
					short *ai_buf = 0;
					ai_buf = new short[info.nMeans];
#ifndef ACQ_SIMUL
					err = D2K_AI_ContBufferSetup(info.card, ai_buf, info.nMeans, &id);
#endif
					if (err != 0)
						{
						printf("AI_ContBufferSetup error=%d", err);
						return val;
						}

					ai_buf = new short[info.nMeans];
#ifndef ACQ_SIMUL
					err = D2K_AI_ContReadChannel(info.card, channel, id, info.nMeans, info.sampleRate, 0, SYNCH_OP);
#endif
					if (err != 0)
						{
						dbg->trace(DBG_ERROR, "read channel %d (%s) error=%d", channel, getName().c_str(), err);
						throw false;
						}
					for (int i = 0; i < info.nMeans; i++)
						{
						info.value += ai_buf[i];
						}
					delete ai_buf;
//..............
#else
//..............
//internal sampling
					acc=0;
					for(int i=0;i<info.nMeans;i++)
						{
#ifndef ACQ_SIMUL
						err = D2K_AI_ReadChannel(info.card, channel, &tmp);
#endif
						if (err != 0)
							{
							dbg->trace(DBG_ERROR, "read channel %d (%s) error=%d", channel, getName().c_str(), err);
							throw false;
							}
						// NOTE: this is in 2 complement, and if unipolar is centered!
						acc+=tmp;
						usleep(1000000/info.sampleRate);
						}
#endif
					// NOTE: this is in 2 complement, and if unipolar is centered!
					info.value = acc/info.nMeans;
					}
				else
					{
					// single read
#ifndef ACQ_SIMUL
					err = D2K_AI_ReadChannel(info.card, channel, &tmp);
#endif
					if (err != 0)
						{
						dbg->trace(DBG_ERROR, "read channel %d (%s) error=%d", channel, getName().c_str(), err);
						throw false;
						}
					// NOTE: this is in 2 complement, and if unipolar is centered!
					info.value = tmp;
					}
				val.raw=info.value;
				return val;
				}
			catch (...)
				{
				dbg->trace(DBG_ERROR, "some error on reading AI channel %d (%s)", channel, getName().c_str());
				}
			break;
		//......................................
		case ch_analog_out:
			// value is the last write one, so it is in info.value
			break;
		}
	return val;
	}

	/**
	 * write analog channel routine
	 * @param channel
	 * @param rv
	 * @param data
	 */
	void writeCh(int channel, acq_t rv,void *data=NULL)
	{
	switch(type)
		{
		//......................................
		case ch_digital_in:
		case ch_analog_in:
			dbg->trace(DBG_ERROR, "input channel cannot be written %d (%s)", channel, getName().c_str());
			break;
		//......................................
		case ch_unknown:
		case ch_analog_out:
			dbg->trace(DBG_ERROR, "not yet implemented");
			// value is the last write one, so it is in info.value
			break;
		//......................................
		case ch_digital_out:
			dbg->trace(DBG_ERROR, "attempt a digital output to analog output routine");
			break;
		}
	}


	/**
	 * perform a fast acquisition into a buffer to be read later (sampling time Ts=1 ms)
	 * @param buff	buffer containing data
	 * @param sample	n samples
	 * @param filename filename where to store data (gnuplot-table)
	 * @param startIndex the beginning of the index
	 * return index of the last data written
	 */
	unsigned int readChBuffered(int channel, unsigned int samples, string filename, unsigned int startIndex)
	{
	unsigned long int mem;

	D2K_AI_InitialMemoryAllocated(info.card,&mem);
	try
		{
		dataIndex=startIndex;
		if((unsigned long int)samples > (mem*1024/2))
			{
			cout << "ERROR: too many samples" << endl;
			throw 0;
			}

		ai_buf=new short[samples];
		err = D2K_AI_ContBufferSetup(info.card, ai_buf, samples, &info.Id);
		if (err != 0)
			{
			printf("AI_ContBufferSetup error=%d\n", err);
			throw 1;
			}

		err = D2K_AI_ContReadChannel(info.card, channel, info.Id, samples, CNT1, CNT1, SYNCH_OP);

		if (err != 0)
			{
			printf("AI_ContScanChannels error=%d\n", err);
			throw 2;
			}

		// conversion and save to file
		//float t;
		float v;
		if(!filename.empty())
			{
			vfile.open(filename.c_str(),fstream::app);	//::app for append mode, ::out for write mode overwriting
			}
		for(unsigned int i=0;i<samples;i++)
			{
			//t=(float)i*(1000.0/(float)SAMPLING_FREQ);	// [ms]
			//D2K_AI_VoltScale(info.card,info.cfg,ai_buf[i],&v); // [V]
			v=applyLinearConversion(ai_buf[i]);
			if(vfile.good())
				{
				vfile << to_string(dataIndex) << "\t" << to_string<float>((float)v) << "\n";
				}
			dataIndex++;
			}
		if(vfile.good())
			{
			vfile.close();
			}
		}
	catch(int ret)
		{
		}
	delete [] ai_buf;
	return dataIndex;
	}

private:
	short *ai_buf;
	int err;
	ofstream vfile;
	unsigned int dataIndex;

	/**
	 *performs the mean son a signal
	 * @param buff buffer containing data (is overwritten)
	 * @param samples total samples
	 * @param n mean on n samples
	 * @return ndata
	 */
	unsigned int meanFilter(float *buff, unsigned int samples, int n)
	{
	int i=0;
	float m;
	while(i<(samples-n))
		{
		m=0.0;
		for(int j=0;j<n;j++)
			{
			m+=buff[i+j];
			}
		m/=n;
		buff[i]=m;
		i++;
		}
	return i;
	}

};


//=============================================================================
/*
 * digital channel (bit)
 */
class Daq2206_digital : public ChannelBase
{
public:
	Daq2206_digital() {};
	virtual ~Daq2206_digital()
	{
#ifndef ACQ_SIMUL
	D2K_Release_Card (info.card);
#endif
	};

	typedef struct chinfo_st
	{
		uint16_t card;
		uint16_t port;
		uint8_t *portValue;				// value as read/written
	} chinfo_t;
	chinfo_t info;

	/**
	 * channel setup
	 * @param name
	 * @param type
	 * @param data
	 * @return true: ok
	 */
	bool setup(const char* name, ChannelBase::chtype_e type, void *data=NULL)
	{
	bool ret=false;
	*info.portValue=0;
	switch(type)
		{
		//......................................
		case ch_digital_in:
#ifndef ACQ_SIMUL
			D2K_DIO_PortConfig(info.card, info.port, INPUT_PORT);
#endif
			break;
		//......................................
		case ch_digital_out:
#ifndef ACQ_SIMUL
			D2K_DIO_PortConfig(info.card, info.port, OUTPUT_PORT);
#endif
			break;

		default:
			ret=true;
		}
	ret |= ChannelBase::setup(name,type,data);
	return !ret;
	}

	/**
	 * read a digital value from an input port (bit)
	 * @param ch channel
	 * @return value read [0,1]; -1 error!
	 */
	acq_t readCh(int channel, void *data)
	{
	uint32_t inPort;
	acq_t val;

#ifndef ACQ_SIMUL
	D2K_DI_ReadPort(info.card, info.port, (unsigned long int *)&inPort);
#endif
	*info.portValue = inPort & 0xFF;
	val.raw=(*info.portValue & (1 << channel)) ? (1) : (0);
	return val;
	}

	/**
	 * write a digital value in an output port (bit)
	 * @param channel
	 * @param v bit value
	 * @param data
	 */
	void writeCh(int channel, acq_t v, void *data)
	{
	if (v.ivalue)
		{
		*info.portValue = (*info.portValue) | (1 << channel);
		}
	else
		{
		*info.portValue = (*info.portValue) & ~(1 << channel);
		}
#ifndef ACQ_SIMUL
	D2K_DO_WritePort(info.card, info.port, *info.portValue);
#endif
	}
private:

};

//-----------------------------------------------
#endif /* ACQ_DAQ2206_H_ */
