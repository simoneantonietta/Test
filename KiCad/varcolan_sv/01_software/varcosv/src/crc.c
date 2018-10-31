/*
 *-----------------------------------------------------------------------------
 * PROJECT: varcolan
 * PURPOSE: see module crc8.h file
 *-----------------------------------------------------------------------------
 */

#include "crc.h"

// CRC8 definitions
#define CRC8_POLY						0x18
#define CRC16_POLY					0x1021
#define CRC32_POLY					0x04C11DB7

/**
 * calculate the crc8
 * @param ch byte to add in the calculation
 * @param crc reference to the previous or crc initial value
 * @return current crc value
 */
uint8_t crc8(uint8_t ch, uint8_t *crc)
{
uint8_t bit_counter;
uint8_t data;
uint8_t feedback_bit;
uint8_t vcrc;

data = ch;  // temporary store
bit_counter = 8;
vcrc = *crc;

do
	{
	feedback_bit = (vcrc ^ data) & 0x01;

	if (feedback_bit == 0x01) vcrc = vcrc ^ CRC8_POLY;

	vcrc = (vcrc >> 1) & 0x7F;

	if (feedback_bit == 0x01) vcrc = vcrc | 0x80;

	data = data >> 1;
	bit_counter--;
	}
while (bit_counter > 0);
*crc = vcrc;

return vcrc;
}

/**
 * calculate the crc16
 * @param ch byte to add in the calculation
 * @param crc reference to the previous or crc initial value
 * @return current crc value
 */
uint16_t crc16(uint8_t ch, uint16_t *crc)
{
uint16_t bit_counter;
uint16_t data;
uint16_t feedback_bit;
uint16_t vcrc;

data = ch;  // temporary store
bit_counter = 16;
vcrc = *crc;

do
	{
	feedback_bit = (vcrc ^ data) & 0x01;

	if (feedback_bit == 0x01) vcrc = vcrc ^ CRC16_POLY;

	vcrc = (vcrc >> 1) & 0x7FFF;

	if (feedback_bit == 0x01) vcrc = vcrc | 0x8000;

	data = data >> 1;
	bit_counter--;
	}
while (bit_counter > 0);
*crc = vcrc;

return vcrc;
}

/**
 * calculate the crc32
 * @param ch byte to add in the calculation
 * @param crc reference to the previous or crc initial value
 * @return current crc value
 */
uint32_t crc32(uint8_t ch, uint32_t *crc)
{
uint32_t bit_counter;
uint32_t data;
uint32_t feedback_bit;
uint32_t vcrc;

data = ch;  // temporary store
bit_counter = 32;
vcrc = *crc;

do
	{
	feedback_bit = (vcrc ^ data) & 0x01;

	if (feedback_bit == 0x01) vcrc = vcrc ^ CRC32_POLY;

	vcrc = (vcrc >> 1) & 0x7FFFFFFF;

	if (feedback_bit == 0x01) vcrc = vcrc | 0x80000000;

	data = data >> 1;
	bit_counter--;
	}
while (bit_counter > 0);
*crc = vcrc;

return vcrc;
}

/**
 * @brief Calculate CRC of an uint8_t array buffer till 256 bytes
 * @param buffer - buffer array pointer
 * @param size - size of array
 * @return crc8 calculation
 */
uint8_t CRC8calc(uint8_t * buffer, uint8_t size)
{
uint8_t crc = CRC8_INIT;
int i;
for (i = 0; i < size; i++)
	{
	crc8(buffer[i], &crc);
	}
return crc;
}

/**
 * @brief Calculate CRC of an uint8_t array buffer but for size till 65536 bytes
 * @param buffer - buffer array pointer
 * @param size - size of array
 * @return crc8 calculation
 */
uint16_t CRC16calc(uint8_t * buffer, uint16_t size)
{
uint16_t crc = CRC16_INIT;
int i;
for (i = 0; i < size; i++)
	{
	crc16(buffer[i], &crc);
	}
return crc;
}

/**
 * @brief Calculate CRC of an uint8_t array buffer but for size till 2^32 bytes
 * @param buffer - buffer array pointer
 * @param size - size of array
 * @return crc8 calculation
 */
uint32_t CRC32calc(uint8_t * buffer, uint32_t size)
{
uint32_t crc = CRC32_INIT;
uint32_t i;
for (i = size; i > 0; i--)
	{
	crc32(buffer[size-i], &crc);
	}
return crc;
}



/**
 * @brief Calculate checksum of an uint8_t array buffer but for size till 2^32 bytes
 * @param buffer - buffer array pointer
 * @param size - size of array
 * @return checksum calculation
 */
uint32_t ChecksumCalc(uint8_t * buffer, uint32_t size)
{
uint32_t checksum = 0;
uint32_t i;
for (i = size; i > 0; i--)
	{
	checksum += buffer[i];
	}
return checksum;
}
