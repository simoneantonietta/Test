#include "protocol.h"
#include "support.h"
#include "serial.h"
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>

#define PELCO_STOP	0
#define PELCO_SX	1
#define PELCO_DX	2
#define PELCO_ALTO	3
#define PELCO_BASSO	4
#define PELCO_ZOOM_P	5
#define PELCO_ZOOM_M	6
#define PELCO_SALVA	7
#define PELCO_POS	8
#define PELCO_ATT	9
#define PELCO_DIS	10

static int pelco_fd;

void PELCO(int cond, int ind, int cmd, int param)
{
  unsigned char cmdbuf[8];
  int i;
  
  if(!cond) return;
  
  cmdbuf[0] = 0xff;	// sync
  cmdbuf[1] = ind;
  cmdbuf[2] = cmdbuf[3] = cmdbuf[4] = cmdbuf[5] = 0;
  
  switch(cmd)
  {
    case PELCO_STOP:
      break;
    case PELCO_SX:
      cmdbuf[3] = 0x04;
      cmdbuf[4] = 0x10;	// velocità 00-3F
      break;
    case PELCO_DX:
      cmdbuf[3] = 0x02;
      cmdbuf[4] = 0x10;	// velocità 00-3F
      break;
    case PELCO_ALTO:
      cmdbuf[3] = 0x10;
      cmdbuf[5] = 0x10;	// velocità 00-3F
      break;
    case PELCO_BASSO:
      cmdbuf[3] = 0x08;
      cmdbuf[5] = 0x10;	// velocità 00-3F
      break;
    case PELCO_ZOOM_P:
      cmdbuf[3] = 0x20;
      break;
    case PELCO_ZOOM_M:
      cmdbuf[3] = 0x40;
      break;
    case PELCO_SALVA:
      if((param < 1) || (param > 32)) return;
      cmdbuf[3] = 3;
      cmdbuf[5] = param;
      break;
    case PELCO_POS:
      if((param < 1) || (param > 32)) return;
      cmdbuf[3] = 7;
      cmdbuf[5] = param;
      break;
    case PELCO_ATT:
      cmdbuf[2] = 0x88;	// camera on
      break;
    case PELCO_DIS:
      cmdbuf[2] = 0x08;	// camera off
      break;
    default:
      return;
  }
  
  cmdbuf[6] = 0;
  for(i=1; i<6; i++) cmdbuf[6] += cmdbuf[i];
  
  write(pelco_fd, cmdbuf, 7);
}

static void pelco_loop(ProtDevice *dev)
{
  char buf[32];
  
  pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
  
  ser_setspeed(dev->fd, B4800, 0);
  pelco_fd = dev->fd;
  
  while(1)
  {
    if(read(pelco_fd, buf, 32) <= 0) return;
  }
}

void _init()
{
  printf("PELCO-D plugin: " __DATE__ " " __TIME__ "\n");
  prot_plugin_register("PEL", 0, NULL, NULL, (PthreadFunction)pelco_loop);
}

