/**----------------------------------------------------------------------------
 * PROJECT: varcolan
 * PURPOSE:
 * 
 *-----------------------------------------------------------------------------  
 * CREATION: 5 Jan 2016
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

#ifndef SRC_CRC_H_
#define SRC_CRC_H_

#define CRC8_INIT							0x00
#define CRC16_INIT						0x00
#define CRC32_INIT						0x00

#include <stdint.h>

uint32_t ChecksumCalc(uint8_t * buffer, uint32_t size);

uint32_t CRC32calc(uint8_t * buffer, uint32_t size);
uint16_t CRC16calc(uint8_t * buffer, uint16_t size);
uint8_t CRC8calc(uint8_t * buffer, uint8_t size);

uint32_t crc32(uint8_t ch, uint32_t *crc);
uint16_t crc16(uint8_t ch, uint16_t *crc);
uint8_t crc8(uint8_t ch, uint8_t *crc);

#endif /* SRC_CRC_H_ */
