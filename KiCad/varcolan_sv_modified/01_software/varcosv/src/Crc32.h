/**----------------------------------------------------------------------------
 * PROJECT: varcosv
 * PURPOSE:
 * 
 *-----------------------------------------------------------------------------  
 * CREATION: 20 Apr 2016
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

#ifndef CRC32_H_
#define CRC32_H_

class Crc32
{
public:
	Crc32() {BuildCRC32Table();}
	~Crc32() {};




	/*
	* This routine calculates the CRC for a block of data using the
	* table lookup method. It accepts an original value for the crc,
	* and returns the updated value.
	*/
	unsigned long CalculateBufferCRC(unsigned int count, unsigned long crc, void *buffer)
	{
	unsigned char *p;
	unsigned long temp1;
	unsigned long temp2;

	p = (unsigned char*) buffer;
	while(count-- != 0)
		{
		temp1 = (crc >> 8) & 0x00FFFFFFL;
		temp2 = CRC32Table[((int) crc ^ *p++) & 0xff];
		crc = temp1 ^ temp2;
		}
	return (crc);
	}


	/*
	* This routine is responsible for actually performing the
	* calculation of the 32 bit CRC for the entire file.  We
	* precondition the CRC value with all 1's, then invert every bit
	* after the entire file has been done.  This gives us a CRC value
	* that corresponds with the values calculated by PKZIP and ARJ.
	* The actual calculation consists of reading in blocks from the
	* file, then updating the CRC with the value for that block.  The
	* CRC work is done by another the CalculateBufferCRC routine.
	*/
	unsigned long CalculateFileCRC(string fname)
	{
	unsigned long crc;
	int count;
	unsigned char buffer[512];
	int i;
	FILE * file;
	file=fopen(fname.c_str(),"rb");
	if(!file) return 0;

	crc = 0xFFFFFFFFL;
	i = 0;
	for(;;)
		{
		count = fread(buffer, 1, 512, file);
		//if((i++ % 32) == 0) putc('.', stdout);
		if(count == 0) break;
		crc = CalculateBufferCRC(count, crc, buffer);
		}
	fclose(file);
	return (crc ^= 0xFFFFFFFFL);
	}

private:
	unsigned long CRC32Table[ 256 ];
	/*
		* Instead of performing a straightforward calculation of the 32 bit
		* CRC using a series of logical operations, this program uses the
		* faster table lookup method.  This routine is called once when the
		* program starts up to build the table which will be used later
		* when calculating the CRC values.
		*/
		#define CRC32_POLYNOMIAL     0xEDB88320L
		void BuildCRC32Table()
		{
		int i;
		int j;
		unsigned long crc;

		for(i = 0;i <= 255;i++)
			{
			crc = i;
			for(j = 8;j > 0;j--)
				{
				if(crc & 1)
					crc = (crc >> 1) ^ CRC32_POLYNOMIAL;
				else
					crc >>= 1;
				}
			CRC32Table[i] = crc;
			}
		}

};

//-----------------------------------------------
#endif /* CRC32_H_ */
