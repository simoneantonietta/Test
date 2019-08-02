/**----------------------------------------------------------------------------
 * PROJECT: acqsrv
 * PURPOSE:
 * 
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

#ifndef ACQ_CHANNELBASE_H_
#define ACQ_CHANNELBASE_H_

#include <string>
#include <cstring>

#define ACQ_CHANNEL_NAME_SIZE			32


//=============================================================================

/*
 * type used for values
 */
typedef union acq_u
{
	float fvalue;
	int ivalue;
	int raw;
} acq_t;


//=============================================================================

/*
 * base class for channel
 */
class ChannelBase
{
public:
	ChannelBase()
	{
	x1=x2=y1=y2=0;
	m=q=0.0;
	}

	virtual ~ChannelBase() {};

	enum chtype_e {ch_analog_in,ch_analog_out,ch_digital_in,ch_digital_out, ch_unknown} type;

	int channel;

	//=============================================================================
	// PURE VIRTUALS
	/**
	 * low level acquisition of the channel, to be implemented for the specific HW
	 * use rawValue to store the read/written channel
	 * @param data user data
	 * @return raw value
	 */
	virtual acq_t readCh(int channel, void *data=NULL) = 0;
	/**
	 * low level write of the channel, to be implemented for the specific HW
	 * use rawValue to store the read/written channel
	 * @param channel number of the channel
	 * @param rv raw value
	 * @param data
	 */
	virtual void writeCh(int channel, acq_t v,void *data=NULL) = 0;


	//=============================================================================

	/**
	 * setup of the channel
	 * @param channel number of the channel
	 * @param data user data
	 * @return true: setup ok
	 */
	virtual bool setup(const char* name, chtype_e type, void *data=NULL)
	{
	strncpy(this->name,name,ACQ_CHANNEL_NAME_SIZE);
	this->name[ACQ_CHANNEL_NAME_SIZE-1]=0;
	value.ivalue=0;
	this->type=type;
	useScaledValue=false;

	return true;
	}

	/**
	 * read continuously a channel
	 * @param channel
	 * @param samples n samples to acquire
	 * @param filename file where store data
	 * @param startIndex the first index
	 * @return index of the last data written
	 */
	virtual unsigned int readChBuffered(int channel, unsigned int samples, string filename, unsigned int startIndex) {};

	/**
	 * set the channel value
	 * @param value
	 */
	virtual void setValue(acq_t value, void *data=NULL)
	{
	this->value=value;
	writeCh(channel,value,data);
	}

	/**
	 * get the channel value
	 * @return
	 */
	virtual acq_t getValue(void *data=NULL)
	{
	rawValue=readCh(channel,data);
	switch(type)
		{
		case ch_analog_in:
			value.fvalue=_Value().fvalue;
			break;
		case ch_analog_out:
			value.fvalue=_Value().fvalue;
			break;
		case ch_digital_in:
			value=rawValue;
			break;
		case ch_digital_out:
			value=rawValue;
			break;
		case ch_unknown:
			break;
		}
	return value;
	}

	/**
	 * wrapper for the real function
	 * @param buff
	 * @param samples
	 */
	virtual unsigned int readBufferedValue(unsigned int samples, string fname,unsigned int startIndex)
	{
	return readChBuffered(channel,samples,fname,startIndex);
	}

	/**
	 * setup of the linear conversion
	 * @param x1
	 * @param x2
	 * @param y1
	 * @param y2
	 */
	void setupLinearConversion(float x1,float x2, float y1, float y2)
	{
	if(x1!=0 || x2!=0 || y1!=0 || y2!=0)
		{
		m=(y2-y1)/(x2-x1);
		q=y1-m*x1;
		useScaledValue=true;
		}
	}
	/**
	 * setup the linear conversion with plinear parameter directly
	 * @param m
	 * @param q
	 */
	void setupLinearConversion(float m,float q)
	{
	this->m=m;
	this->q=q;
	useScaledValue=true;
	}

	/**
	 * utility to apply to a value the linear conversion for the channel
	 * @param v
	 * @return converted value
	 */
	float applyLinearConversion(int v)
	{
	return m*(float)v+q;
	}

	/**
	 * return the raw value
	 * @return raw value
	 */
	acq_t getRaw()
	{
	return rawValue;
	}

	/**
	 * get the name of the channel
	 * @return name
	 */
	std::string getName()
	{
	return std::string(name);
	}

private:
	char name[ACQ_CHANNEL_NAME_SIZE];
	acq_t rawValue;
	acq_t value;
	bool useScaledValue;
	float x1,x2,y1,y2; // if all are =0 no coversion is done
	float m;
	float q;


	/**
	 * calculates the linear conversion (or bypass)
	 * @return value
	 */
	acq_t _Value()
	{
	acq_t v;
	if(useScaledValue)
		{
		//printf("* raw value=%d\n",rawValue.raw);
		v.fvalue=m*rawValue.raw+q;
		}
	else
		{
		v.ivalue=rawValue.raw;
		}
	return v;
	}
};

//-----------------------------------------------
#endif /* ACQ_CHANNELBASE_H_ */
