/**----------------------------------------------------------------------------
 * PROJECT:
 * PURPOSE:
 * The values for speed are B115200, B230400, B9600, B19200, B38400, B57600, B1200, B2400, B4800, etc.
 * The values for parity are 0 (meaning no parity),
 * PARENB|PARODD (enable parity and use odd),
 * PARENB (enable parity and use even),
 * PARENB|PARODD|CMSPAR (mark parity),
 * PARENB|CMSPAR (space parity).
 * 
 * Blocking" sets whether a read() on the port waits for the specified number of
 * characters to arrive. Setting no blocking means that a read() returns however
 * many characters are available without waiting for more, up to the buffer limit.
 *-----------------------------------------------------------------------------  
 * CREATION: 4 Feb 2015
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

#ifndef COMM_SERIAL_SERIAL_H_
#define COMM_SERIAL_SERIAL_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

int serialOpen(const char* portname,int speed,int parity);
void serialSetBlocking(int fd, uint8_t nbytes, uint8_t to);
void serialClose(int fd);
int serialWrite(int fd, uint8_t *buf, int ndata);
int serialRead(int fd,uint8_t *buf,int ndata,int to);
int serialReadETX(int fd, uint8_t *buf, int etx, int maxSize, int to);
void serialSetTimeout(int fd, int to);
void serialNBytesExp(int fd, uint8_t nbytes);
void serialFlushRX(int fd);

#ifdef __cplusplus
}
#endif
//-----------------------------------------------
#endif /* COMM_SERIAL_SERIAL_H_ */
