//============================================================================
// Name        : varcolansv_arm.cpp
// Author      : 
// Version     :
// Copyright   : Your copyright notice
// Description : Hello World in C++, Ansi-style
//============================================================================

#if 1
#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include "common/serial/serial.h"

using namespace std;

int main()
{
int ser_fd;	// serial fd
unsigned char buff[10];
ser_fd=serialOpen("/dev/ttyS0",57600,0);
if(ser_fd>0)
	{
	serialSetBlocking(ser_fd,0,0);
	while(1)
		{
		int rxndata=serialRead(ser_fd,buff,1,10);
		//usleep(10000);
		}
	}
else
	{
	cout << "cannot open serial port" << endl;
	}

serialClose(ser_fd);
return 0;
}
#else
#include <stdio.h>
#include <unistd.h>
#include <linux/serial.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>

int ser_setspeed(int fdser, int baud, int timeout);

int ser_open(const char *dev, int baud, int timeout)
{
  int fd, custom;
  struct serial_struct info;

  fd = open(dev, O_RDWR | O_NOCTTY);
  if(fd < 0) return fd;

  ioctl(fd, TIOCGSERIAL, &info);
  info.closing_wait = ASYNC_CLOSING_WAIT_NONE;
  ioctl(fd, TIOCSSERIAL, &info);

  ser_setspeed(fd, baud, timeout);

  return fd;
}

int ser_close(int fdser)
{
  return close(fdser);
}

int ser_setspeed(int fdser, int baud, int timeout)
{
  struct termios tio;

  memset(&tio, 0, sizeof(tio));

  tio.c_cflag = baud | CS8 | CLOCAL | CREAD;
  tio.c_iflag = IGNPAR;
  tio.c_oflag = 0;
  tio.c_lflag = 0;

  tio.c_cc[VINTR] = 0;
  tio.c_cc[VQUIT] = 0;
  tio.c_cc[VERASE] = 0;
  tio.c_cc[VKILL] = 0;
  tio.c_cc[VEOF] = 4;
  tio.c_cc[VTIME] = timeout;
  tio.c_cc[VMIN] = timeout?0:1;
  tio.c_cc[VSWTC] = 0;
  tio.c_cc[VSTART] = 0;
  tio.c_cc[VSTOP] = 0;
  tio.c_cc[VSUSP] = 0;
  tio.c_cc[VEOL] = 0;
  tio.c_cc[VREPRINT] = 0;
  tio.c_cc[VDISCARD] = 0;
  tio.c_cc[VWERASE] = 0;
  tio.c_cc[VLNEXT] = 0;
  tio.c_cc[VEOL2] = 0;

  tcflush(fdser, TCIOFLUSH);
  return tcsetattr(fdser, TCSANOW, &tio);
}

int main(int argc, char *argv[])
{
  int fd, i, s;
  unsigned char buf[256];
  struct timeval tv;

  fd = ser_open("/dev/ttyS0", B57600, 0);

  while(1)
  {
    s = read(fd, buf, 256);
    if(s)
    {
      gettimeofday(&tv, NULL);

      fprintf(stdout, ">> %4d %d.%06d ", s, tv.tv_sec, tv.tv_usec);
      for(i=0; i<s; i++) fprintf(stdout, "%02x", buf[i]);
      fprintf(stdout, "\n");
      fflush(stdout);
    }
  }
}

#endif
