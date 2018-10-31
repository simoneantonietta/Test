/**----------------------------------------------------------------------------
 * PROJECT:
 * PURPOSE:
 * handle wiegand protocol (26 bit for  now) and keyboard (4bit for each digit)
 * stdby: 1 level both signal
 * '1' -> goes down D1
 * '1' -> goes down D0
 * data pulse: 50 us
 * data period: 2 ms
 *-----------------------------------------------------------------------------  
 * CREATION: Oct 27, 2016
 * Author: Luca Mini
 * 
 * LICENCE: please see LICENCE.TXT file
 * 
 * HISTORY (of the module):
 *-----------------------------------------------------------------------------
 * Author              | Date        | Description
 *-----------------------------------------------------------------------------
 *-----------------------------------------------------------------------------
 *-----------------------------------------------------------------------------
 */

#ifndef WIEGAND_H_
#define WIEGAND_H_

typedef union wiegand26_u
{
	struct
	{
		uint8_t facility;
		uint16_t data;
	} flds;
	uint32_t data32;
	uint8_t dataVec[3];
	uint8_t digit;
} wiegand26_t;

void wiegandReset(void);
int wiegandAddBit(uint8_t b);
int wiegandDecode(char type,uint8_t *input,wiegand26_t *output);

//-----------------------------------------------
#endif /* WIEGAND_H_ */
