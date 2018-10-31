/*
 *-----------------------------------------------------------------------------
 * PROJECT:
 * PURPOSE: see module wigand.h file
 *-----------------------------------------------------------------------------
 */

#include "wiegand.h"

#define WIEGAND_SIZE	26

static uint8_t wieg_buff[WIEGAND_SIZE];
static uint8_t wieg_ndx;

/**
 * reset the buffer (after each decode)
 */
void wiegandReset(void)
{
for(wieg_ndx=0;wieg_ndx<WIEGAND_SIZE;wieg_ndx++) wieg_buff[wieg_ndx]=0;
wieg_ndx=0;
}

/**
 * add a bit into the bit buffer
 * @param b
 * @return 1 if a full buffer is available for decoding; 0 in progress or empty
 */
int wiegandAddBit(uint8_t b)
{
wieg_buff[wieg_ndx]=b;
if(wieg_ndx<WIEGAND_SIZE)
	{
	wieg_ndx++;
	return 0;
	}
else
	{
	return 1;	// indicates that a full buffure is available
	}
}

/**
 * decode the wiegand data from a stream of bits stored as bytes
 * @param type 'c' card read; 'k' key digit
 * @param input
 * @param output
 * @return 0 on error; 1 ok
 */
int wiegandDecode(char type,uint8_t *input,wiegand26_t *output)
{
int i,par;

if(type=='c')
	{
	// CARD READER
	// check parities
	par=0;
	for(i=0;i<13;i++)
		{
		par += input[i];
		}
	if(par & 1)	// even parity
		{
		return 0;
		}

	par=0;
	for(i=13;i<25;i++)
		{
		par += input[i];
		}
	if(!(par & 1))	// odd parity
		{
		return 0;
		}

	// get facility
	output->flds.facility=0x00;
	for(i=0;i<8;i++)
		{
		output->flds.facility |= (input[1+i] << i);
		}
	// get value
	output->flds.data=0x0000;
	for(i=0;i<16;i++)
		{
		output->flds.data |= (input[1+i] << i);
		}
	}
else
	{
	// KEYBOARD DIGIT
	// 4 bit per digit
	output->digit=0;
	for(i=0;i<4;i++)
		{
		output->digit |= input[i] << i;
		}
	}
	wiegandReset();
return 1;
}
