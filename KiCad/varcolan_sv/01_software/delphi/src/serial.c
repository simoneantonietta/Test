#include "serial.h"
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <linux/serial.h>
#include <stdlib.h>

int ser_open(const char *dev, int baud, int timeout)
{
  int fd;
  struct serial_struct info;

  fd = open(dev, O_RDWR | O_NOCTTY);
  if(fd < 0) return fd;

  ser_setspeed(fd, baud, timeout);
  
  ioctl(fd, TIOCGSERIAL, &info);
  info.closing_wait = ASYNC_CLOSING_WAIT_NONE;
  ioctl(fd, TIOCSSERIAL, &info);
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

  tio.c_cflag = baud /* | CRTSCTS */ | CS8 | CLOCAL | CREAD;
  tio.c_iflag = IGNPAR | IGNBRK;
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

#include <errno.h>

char* ser_send_recv(int fdser, char *cmd, char *buf)
{
  char tbuf[256];
  char *p, *s;
  int n, error = 0;
#ifdef __arm__
  fd_set fds;
  struct timeval to;
#endif
  
  if(buf) buf[0] = 0;

  n = write(fdser, cmd, strlen(cmd));
  if(n < (int)strlen(cmd)) return buf;
  
  p = buf;
  if(!p) p = tbuf;
  s = p;
  
#ifdef __arm__
  FD_ZERO(&fds);
  FD_SET(fdser, &fds);
#endif
  while(!((((p-s) > 3) && !strncmp(p - 3, "OK\r\n", 4)) ||
       (((p-s) > 6) && !strncmp(p - 6, "ERROR\r\n", 7)) ||
       (error && ((p-s) > 1) && !strncmp(p - 1, "\r\n", 2))))
  {
#ifdef __arm__
    /* La read con timeout 1s in realtà viene interrotta da SIGALM
       10 volte al secondo e di fatto la read non esce più a meno
       che non riceva effettivamente qualcosa.
       Con la select supero il problema ma devo ricavare il timeout
       dall'impostazione della seriale. */
    to.tv_sec = 1;
    to.tv_usec = 0;
    do
      n = select(fdser+1, &fds, NULL, NULL, &to);
    while((n<0) && (errno == EINTR) && to.tv_usec);
    if(n > 0)
#endif
    n = read(fdser, p+1, 1);

    /* Se n = 0 e' uscito con timeout e quindi siamo in fase di inizializzazione.
       Se il timeout non e' impostato, n = 1 oppure errore e quindi n < 0. */
    if(n < 1) break;
    if((p == s) && ((*(p+1) == '\r') || (*(p+1) == '\n')))
      continue;
    else
      p += n;

    if(((p-s) > 5) && !strncmp(p - 5, "ERROR:", 6)) error++;
  }
  
  usleep(20000);
  
  *(p+1) = 0;
  if(buf)
  {
    if(p == s) return buf;
    
    if(buf[1] == '\r')	/* we don't have echo */
      return buf + 3;
    else
      return buf + strlen(cmd) + 3;
  }
  else
    return NULL;
}

