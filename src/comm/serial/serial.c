/*
 *-----------------------------------------------------------------------------
 * PROJECT:
 * PURPOSE: see module serial.h file
 *-----------------------------------------------------------------------------
 */

#include <errno.h>
//#include <termios.h>
#include <termio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <linux/serial.h>
#include <sys/time.h>
#include "serial.h"

/*
 * timeout in 100ms used for read operation.
 * All timeouts are affected by this
 */
#define SERIAL_INTERNAL_TIMEOUT		10

static int set_interface_attribs(int fd, int speed, int parity);
static speed_t serialGetBaudRate(int br);

// store locally the configuration
static uint8_t __serialAct_nbytes;
static uint8_t __serialAct_to;
//=============================================================================
/**
 * open the serial port
 * @param portname
 * @param speed
 * @param parity
 * @return descriptor
 */
int serialOpen(const char* portname, int speed, int parity)
{
//int fd = open(portname, O_RDWR | O_NOCTTY | O_SYNC);
int fd = open(portname, O_RDWR | O_NOCTTY);
if (fd < 0)
	{
	printf("error %d opening %s: %s\n", errno, portname, strerror(errno));
	return -1;
	}

//set_interface_attribs (fd, B115200, 0);  // set speed to 115,200 bps, 8n1 (no parity)
set_interface_attribs(fd, speed, parity);  // set speed to 115,200 bps, 8n1 (no parity)
serialSetBlocking(fd, 0, SERIAL_INTERNAL_TIMEOUT);	// set blocking for 1 s (default)
return fd;
}

/**
 * close the serial port
 * @param fd descriptor
 */
void serialClose(int fd)
{
close(fd);
}

/**
 * send data
 * @param fd
 * @param buf
 * @param ndata
 * @return data sent
 */
int serialWrite(int fd, uint8_t *buf, int ndata)
{
return write(fd, buf, ndata);
}

/**
 * read the expected data
 * @param fd
 * @param buf
 * @param ndata data expected
 * @param to timeout: if 0 disable timeout, else is in [ms]
 * NOTE: this time must be > of the timeout set at the beginning or the one set by serialSetTimeout or serialSetBlocking
 * @return n data read
 */
int serialRead(int fd, uint8_t *buf, int ndata, int to)
{
int n=0;
struct timeval tv;
unsigned int ms1,ms2,tmp;

if(to>0 && to<SERIAL_INTERNAL_TIMEOUT)
	{
	printf("error: timeout must be greater and, better, multiple of %d\n", SERIAL_INTERNAL_TIMEOUT);
	return 0;
	}

// this to handle a more versatile timeout
gettimeofday(&tv, NULL);
tmp=tv.tv_usec/1000;
ms1=tmp + (((tv.tv_usec-tmp*1000)>=500) ? (1) : (0));	// to round better
ms1=tv.tv_sec*1000+ms1;
// wait for the all data expected
do
	{
	n+=read(fd, &buf[n], ndata-n);

	gettimeofday(&tv, NULL);
	tmp=tv.tv_usec/1000;
	ms2=tmp + (((tv.tv_usec-tmp*1000)>=500) ? (1) : (0));	// to round better
	ms2=tv.tv_sec*1000+ms2;
	if(to==0) break;
	if(to>0)
		{
		if((ms2-ms1)>=(to*100)) break; // timeout
		}
	} while(n<ndata);
return n;
}

/**
 * read data and wait for the end char ("byte stuffing" type protocols)
 * @param fd
 * @param buf
 * @param etx
 * @param maxSize maximum number of chars to be read
 * @param to timeout: if 0 use the timeout at the first byte else setup the time in 100 ms steps
 * NOTE: this time must be > of the timeout set at the beginning or the one set by serialSetTimeout or serialSetBlocking
 * @return bytes effectively read
 */
int serialReadETX(int fd, uint8_t *buf, int etx, int maxSize, int to)
{
int n=0;
struct timeval tv;
unsigned int ms1,ms2,tmp;
int ndata;

if(to>0 && to<SERIAL_INTERNAL_TIMEOUT)
	{
	printf("error: timeout must be greater and, better, multiple of %d\n", SERIAL_INTERNAL_TIMEOUT);
	return 0;
	}

// this to handle a more versatile timeout
gettimeofday(&tv, NULL);
tmp=tv.tv_usec/1000;
ms1=tmp + (((tv.tv_usec-tmp*1000)>=500) ? (1) : (0));	// to round better
ms1=tv.tv_sec*1000+ms1;
// wait for the all data expected
do
	{
	if(n<maxSize)
		{
		n+=read(fd, &buf[n], 1);
		}
	else
		{
		return n;	// too many data, so exit!
		}

	gettimeofday(&tv, NULL);
	tmp=tv.tv_usec/1000;
	ms2=tmp + (((tv.tv_usec-tmp*1000)>=500) ? (1) : (0));	// to round better
	ms2=tv.tv_sec*1000+ms2;
	if(to>0)
		{
		if((ms2-ms1)>=(to*100)) break; // timeout
		}
	} while(buf[n-1]!=etx);
return n;
}
/**
 * flush the stream
 * @param fd
 */
void serialFlushRX(int fd)
{
#if 0
	sleep(2); // this need to avoid a kernel issue
	tcflush(fd,TCIOFLUSH);
#else
	uint8_t _buf[256];
	int n;
	uint8_t __serialOld_nbytes=__serialAct_nbytes;
	uint8_t __serialOld_to=__serialAct_to;

	serialSetBlocking(fd,0,0);	// set as non blocking
	do
		{
		n=serialRead(fd,_buf,256,0);
		} while(n>0);
	// recovery of the timeout
	serialSetBlocking(fd,__serialOld_nbytes,__serialOld_to);	// set as non blocking
#endif
}

/**
 * set the read blocking or not
 * @param fd
 * @param nbytes	number of bytes expected (usually 1)
 * @param to in intervals of 100 ms (i.e. 10 = 1 s) max 255 (25.5 s)
 *
 * @note
 *  nbytes=0;to=0 -> non blocking
 *  nbytes=0;to>0 -> wait for some data and return
 */
void serialSetBlocking(int fd, uint8_t nbytes, uint8_t to)
{
struct termios tty;

// store actual values
__serialAct_nbytes=nbytes;
__serialAct_to=to;

memset(&tty, 0, sizeof tty);
if (tcgetattr(fd, &tty) != 0)
	{
	printf("error %d from tggetattr\n", errno);
	return;
	}
/*
 VMIN = 0 and VTIME = 0
 This is a completely non-blocking read - the call is satisfied immediately
 directly from the driver's input queue. If data are available, it's transferred
 to the caller's buffer up to nbytes and returned. Otherwise zero is immediately
 returned to indicate "no data". We'll note that this is "polling" of the serial
 port, and it's almost always a bad idea. If done repeatedly, it can consume
 enormous amounts of processor time and is highly inefficient. Don't use this
 mode unless you really, really know what you're doing.
 VMIN = 0 and VTIME > 0
 This is a pure timed read. If data are available in the input queue, it's
 transferred to the caller's buffer up to a maximum of nbytes, and returned
 immediately to the caller. Otherwise the driver blocks until data arrives,
 or when VTIME tenths expire from the start of the call. If the timer expires
 without data, zero is returned. A single byte is sufficient to satisfy this read
 call, but if more is available in the input queue, it's returned to the caller.
 Note that this is an overall timer, not an intercharacter one.
 VMIN > 0 and VTIME > 0
 A read() is satisfied when either VMIN characters have been transferred to the
 caller's buffer, or when VTIME tenths expire between characters. Since this
 timer is not started until the first character arrives, this call can block
 indefinitely if the serial line is idle. This is the most common mode of operation,
 and we consider VTIME to be an intercharacter timeout, not an overall one.
 This call should never return zero bytes read.
 VMIN > 0 and VTIME = 0
 This is a counted read that is satisfied only when at least VMIN characters have
 been transferred to the caller's buffer - there is no timing component involved.
 This read can be satisfied from the driver's input queue (where the call could
 return immediately), or by waiting for new data to arrive: in this respect the
 call could block indefinitely. We believe that it's undefined behavior if nbytes
 is less then VMIN.
 */
tty.c_cc[VMIN] = nbytes;
tty.c_cc[VTIME] = to;            // 0.1 seconds step

if (tcsetattr(fd, TCSANOW, &tty) != 0)
  printf("error %d setting term attributes\n", errno);
}

/**
 * serial timeout for non blocking read (internal granularity)
 * Please before change this, take in account the coherence with the value
 * set in the read operation
 * @param fd
 * @param to in intervals of 100 ms (i.e. 10 = 1 s) max 255 (25.5 s)
 */
void serialSetTimeout(int fd, int to)
{
struct termios tty;
if(to>255)
	{
	printf("error timeout is too big (max 255)\n");
	return;
	}
memset(&tty, 0, sizeof tty);
if (tcgetattr(fd, &tty) != 0)
	{
	printf("error %d from tcgetattr\n", errno);
	return;
	}

tty.c_cc[VTIME] = to;            // 0.1 seconds steps

if (tcsetattr(fd, TCSANOW, &tty) != 0)
  printf("error %d setting term attributes\n", errno);
}

/**
 * set how many chars must expect for read operation
 * @param fd
 * @param nbytes
 */
void serialNBytesExp(int fd, uint8_t nbytes)
{
struct termios tty;
memset(&tty, 0, sizeof tty);
if (tcgetattr(fd, &tty) != 0)
	{
	printf("error %d from tggetattr\n", errno);
	return;
	}

tty.c_cc[VMIN] = nbytes;

if (tcsetattr(fd, TCSANOW, &tty) != 0)
  printf("error %d setting term attributes\n", errno);
}

//-----------------------------------------------------------------------------
// PRIVATE
//-----------------------------------------------------------------------------

/**
 * get the baudrate constants
 * @param br can be an integer
 * @return
 */
speed_t serialGetBaudRate(int br)
{
speed_t baud;
switch (br)
	{
#ifdef B0
	case 0:
		baud = B0;
		break;
#endif
#ifdef B50
	case 50:
		baud = B50;
		break;
#endif
#ifdef B75
	case 75:
		baud = B75;
		break;
#endif
#ifdef B110
	case 110:
		baud = B110;
		break;
#endif
#ifdef B134
	case 134:
		baud = B134;
		break;
#endif
#ifdef B150
	case 150:
		baud = B150;
		break;
#endif
#ifdef B200
	case 200:
		baud = B200;
		break;
#endif
#ifdef B300
	case 300:
		baud = B300;
		break;
#endif
#ifdef B600
	case 600:
		baud = B600;
		break;
#endif
#ifdef B1200
	case 1200:
		baud = B1200;
		break;
#endif
#ifdef B1800
	case 1800:
		baud = B1800;
		break;
#endif
#ifdef B2400
	case 2400:
		baud = B2400;
		break;
#endif
#ifdef B4800
	case 4800:
		baud = B4800;
		break;
#endif
#ifdef B7200
		case 7200: baud = B7200; break;
#endif
#ifdef B9600
	case 9600:
		baud = B9600;
		break;
#endif
#ifdef B14400
		case 14400: baud = B14400; break;
#endif
#ifdef B19200
	case 19200:
		baud = B19200;
		break;
#endif
#ifdef B28800
		case 28800: baud = B28800; break;
#endif
#ifdef B57600
	case 57600:
		baud = B57600;
		break;
#endif
#ifdef B76800
		case 76800: baud = B76800; break;
#endif
#ifdef B38400
	case 38400:
		baud = B38400;
		break;
#endif
#ifdef B115200
	case 115200:
		baud = B115200;
		break;
#endif
#ifdef B128000
		case 128000: baud = B128000; break;
#endif
#ifdef B153600
		case 153600: baud = B153600; break;
#endif
#ifdef B230400
	case 230400:
		baud = B230400;
		break;
#endif
#ifdef B256000
		case 256000: baud = B256000; break;
#endif
#ifdef B460800
	case 460800:
		baud = B460800;
		break;
#endif
#ifdef B921600
	case 921600:
		baud = B921600;
		break;
#endif
#ifdef B1000000
	case 1000000:
		baud = B1000000;
		break;
#endif
#ifdef B1152000
	case 1152000:
		baud = B1152000;
		break;
#endif
#ifdef B1500000
	case 1500000:
		baud = B1500000;
		break;
#endif
#ifdef B2000000
	case 2000000:
		baud = B2000000;
		break;
#endif
#ifdef B2500000
	case 2500000:
		baud = B2500000;
		break;
#endif
#ifdef B3000000
	case 3000000:
		baud = B3000000;
		break;
#endif
#ifdef B3500000
	case 3500000:
		baud = B3500000;
		break;
#endif
#ifdef B4000000
	case 4000000:
		baud = B4000000;
		break;
#endif
	default:
		// ca calculate a custom baud rate
		baud = 0;
	}
return baud;
}

/**
 * setup the interface atributes
 * @param fd
 * @param speed
 * @param parity
 * @return
 */
int set_interface_attribs(int fd, int speed, int parity)
{
struct termios tty;
struct serial_struct serinfo;
speed_t baudrate;

memset(&tty, 0, sizeof tty);
if (tcgetattr(fd, &tty) != 0)
	{
	printf("error %d from tcgetattr\n", errno);
	return -1;
	}

baudrate = serialGetBaudRate(speed);
if (baudrate == 0)  // if need to calculate a custom baud rate
	{
	serinfo.reserved_char[0] = 0;
	if (ioctl(fd, TIOCGSERIAL, &serinfo) < 0) return -1;
	serinfo.flags &= ~ASYNC_SPD_MASK;
	serinfo.flags |= ASYNC_SPD_CUST;
	serinfo.custom_divisor = (serinfo.baud_base + (speed / 2)) / speed;
	if (serinfo.custom_divisor < 1) serinfo.custom_divisor = 1;
	if (ioctl(fd, TIOCSSERIAL, &serinfo) < 0) return -1;
	if (ioctl(fd, TIOCGSERIAL, &serinfo) < 0) return -1;
	if (serinfo.custom_divisor * speed != serinfo.baud_base)
		{
		//warnx("actual baudrate is %d / %d = %f", serinfo.baud_base, serinfo.custom_divisor, (float) serinfo.baud_base / serinfo.custom_divisor);
		printf("actual baudrate is %d / %d = %f", serinfo.baud_base, serinfo.custom_divisor, (float) serinfo.baud_base / serinfo.custom_divisor);
		}
	}

cfsetospeed(&tty, baudrate);
cfsetispeed(&tty, baudrate);

tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;     // 8-bit chars
// disable IGNBRK for mismatched speed tests; otherwise receive break
// as \000 chars
tty.c_iflag &= ~IGNBRK;         // disable break processing
tty.c_lflag = 0;                // no signaling chars, no echo,
// no canonical processing
tty.c_oflag = 0;                // no remapping, no delays
tty.c_cc[VMIN] = 0;            // read doesn't block
tty.c_cc[VTIME] = 5;            // 0.5 seconds read timeout

tty.c_iflag &= ~(IXON | IXOFF | IXANY);  // shut off xon/xoff ctrl

tty.c_cflag |= (CLOCAL | CREAD);  // ignore modem controls,
// enable reading
tty.c_cflag &= ~(PARENB | PARODD);      // shut off parity
tty.c_cflag |= parity;
tty.c_cflag &= ~CSTOPB;
tty.c_cflag &= ~CRTSCTS;

cfmakeraw(&tty);	// raw it!
tcflush( fd, TCIFLUSH );
if (tcsetattr(fd, TCSANOW, &tty) != 0)
	{
	printf("error %d from tcsetattr\n", errno);
	return -1;
	}
return 0;
}



#if 0
...
char *portname = "/dev/ttyUSB1"
...
int fd = open (portname, O_RDWR | O_NOCTTY | O_SYNC);
if (fd < 0)
	{
	error_message ("error %d opening %s: %s", errno, portname, strerror (errno));
	return;
	}

set_interface_attribs (fd, B115200, 0);  // set speed to 115,200 bps, 8n1 (no parity)
set_blocking (fd, 0);// set no blocking

write (fd, "hello!\n", 7);// send 7 character greeting

usleep ((7 + 25) * 100);// sleep enough to transmit the 7 plus
// receive 25:  approx 100 uS per char transmit
char buf[100];
int n = read(fd, buf, sizeof buf);// read up to 100 characters if ready to read
}
#endif
