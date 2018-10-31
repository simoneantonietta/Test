#include "pxl.h"
#include "support.h"
#include "master.h"
#include "delphi.h"
#include <sys/ioctl.h>
#include <asm/saetpxl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>

static int pxl_fd = -1;
static pthread_t pxl_thd;
static pthread_t pxl_poll_thd;
static sem_t pxl_sem;

static unsigned char pxl_cmd_dim[] = {
	 1, 1, 1,17, 2, 2, 2, 2, 5, 1,
	 1, 1, 1, 3, 4, 8, 5, 7, 3, 4,
	 3, 3,10, 3, 1, 1, 2, 9, 2, 2,
	 5,18, 4, 4};

static unsigned char pxl_periph[16] = {
	0x22, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff};

static int pxl_status_sens = 0;

unsigned char swap(unsigned char data)
{
  unsigned char tmp;
  
  tmp = (data << 4) | (data >> 4);
  return tmp;
}

void pxl_parse(unsigned char *cmd)
{
  unsigned char reply[24];
  int i;

  sem_wait(&pxl_sem);
  
  switch(cmd[0])
  {
    case 1:
      reply[0] = 1;
      reply[1] = 0;
      write(pxl_fd, reply, 2);
      break;
    case 2:
      reply[0] = 2;
      reply[1] = 0x22;
      for(i=2; i<17; i++)
        reply[i] = 0xff;
      write(pxl_fd, reply, 17);
      break;
    case 3:
      for(i=0; i<16; i++)
      {
        if((cmd[i+1] & 0x0f) != (pxl_periph[i] & 0x0f))
        {
          if((pxl_periph[i] & 0x0f) == 0x0f)
          {
            /* manomissione */
            reply[0] = 4;
            reply[1] = i << 1;
            write(pxl_fd, reply, 2);
          }
          else
          {
            /* incongruente */
            reply[0] = 3;
            reply[1] = i << 1;
            reply[2] = 2;
            write(pxl_fd, reply, 3);
          }
        }
        if((cmd[i+1] & 0xf0) != (pxl_periph[i] & 0xf0))
        {
          if((pxl_periph[i] & 0xf0) == 0xf0)
          {
            /* manomissione */
            reply[0] = 4;
            reply[1] = (i << 1) + 1;
            write(pxl_fd, reply, 2);
          }
          else
          {
            /* incongruente */
            reply[0] = 3;
            reply[1] = (i << 1) + 1;
            reply[2] = 2;
            write(pxl_fd, reply, 3);
          }
        }
      }
      break;
    case 7:
      reply[0] = 7;
      reply[2] = 0x80;
      if(!cmd[1])
      {
        reply[1] = 0;
        reply[3] = pxl_status_sens << 1;
      }
      else
      {
        reply[1] = 1;
        reply[3] = pxl_status_sens >> 7;
      }
      write(pxl_fd, reply, 4);
      break;
    case 8:
      if(!cmd[1] && (cmd[2] & 0x80))
        ioctl(pxl_fd, IO_WRITEBITS, swap(cmd[3]));
      break;
    default:
      break;
  }

  sem_post(&pxl_sem);
}

void* pxl_emulate()
{
  unsigned char cmd[24];
  int res, len = 0;
  
  pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
  
  support_log("PXL start");
  
  cmd[0] = 1;
  pxl_parse(cmd);
  
  while(1)
  {
    pthread_testcancel();
    res = read(pxl_fd, cmd + len, sizeof(cmd) - len);
    pthread_testcancel();
    
    if(res > 0)
    {
      len += res;
      if(pxl_cmd_dim[cmd[0]] == len)
      {
        len = 0;
        pxl_parse(cmd);
      }
    }
  }

  return NULL;
}

void* pxl_poll()
{
  int status, oldstatus = 0;
  unsigned char reply[4];

  pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
  
  while(1)
  {
    status = ioctl(pxl_fd, _IO(SAETPXL_IOCTYPE, IO_READBITS), NULL);
    status ^= 0xc000;	// invert alarm state for switches
    
    if(status != oldstatus)
    {
      sem_wait(&pxl_sem);
      if((status & 0x00ff) != (oldstatus & 0x00ff))
      {
        reply[0] = 7;
        reply[1] = 0;
        reply[2] = 0x80;
        reply[3] = swap(status & 0x00ff);
        write(pxl_fd, reply, 4);
      }
      if((status & 0xff00) != (oldstatus & 0xff00))
      {
        reply[0] = 7;
        reply[1] = 1;
        reply[2] = 0x80;
        reply[3] = swap((status & 0xff00) >> 8);
        write(pxl_fd, reply, 4);
      }
      sem_post(&pxl_sem);
      oldstatus = status;
    }
    usleep(100000);
  }
  
  return NULL;
}

void pxl_start()
{
  pxl_fd = open(ADDROOTDIR("/dev/pxlmm"), O_RDWR);
  if(pxl_fd < 0) return;
  
  sem_init(&pxl_sem, 0, 1);
  pthread_create(&pxl_thd, NULL, (PthreadFunction)pxl_emulate, NULL);
  pthread_detach(pxl_thd);
  pthread_create(&pxl_poll_thd, NULL, (PthreadFunction)pxl_poll, NULL);
  pthread_detach(pxl_poll_thd);
}

void pxl_stop()
{
  if(pxl_fd < 0) return;

  pthread_cancel(pxl_thd);
  pthread_cancel(pxl_poll_thd);
  close(pxl_fd);
}
