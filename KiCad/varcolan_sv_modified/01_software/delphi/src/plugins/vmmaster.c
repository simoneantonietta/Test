#include "protocol.h"
#include "database.h"
#include "support.h"
#include "user.h"
#include "master.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <errno.h>
#include <byteswap.h>

unsigned char perif[32];
unsigned char pstatus[32][3] = {{0,},};

#define SC8R_INTERVAL 500000
#define SC8R_MAX_PERIF	80

static void vmmaster_loop(ProtDevice *dev)
{
  int i, ret, mm, fdmm[2], fdl, fd, s;
  unsigned char buf[256];
  struct sockaddr_in addr;
  
  fd_set fds;
  int fdmax;
  struct timeval to;
  
  struct {
    int acq;
    int idx;
    int num;
  } sc8r2[32];
  
  if((config.consumer[dev->consumer].configured != 5) ||
     !config.consumer[dev->consumer].param)
  {
    sleep(1);
    return;
  }
  
  /* lettura parametri - n.modulo */
  ret = sscanf(config.consumer[dev->consumer].param, "%d", &mm);
  if(!ret || (mm > 15) || (mm < 0))
  {
    free(dev->prot_data);
    sleep(1);
    return;
  }
  
  sprintf((char*)buf, "Virtual mmaster loop [%d] - mm%d", getpid(), mm);
  support_log((char*)buf);
  
  /* Occorre assicurare che il processo saet abbia già creato i file
     descriptor per i moduli reali, altrimenti si crea prima il virtuale
     che viene sovrascritto dal modulo reale. */
  sleep(1);
  
  socketpair(AF_LOCAL, SOCK_STREAM, 0, fdmm);
  master_register_module(mm, fdmm[0]);
  
  write(fdmm[1], "\033\013\001\000\000\001\000", 7);	// versione 11.0.0.0 e modulo master tipo 0
  
  fdl = socket(PF_INET, SOCK_STREAM, 0);
  i = 1;
  setsockopt(fdl, SOL_SOCKET, SO_REUSEADDR, &i, sizeof(int));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(config.consumer[dev->consumer].data.eth.port);
  addr.sin_addr.s_addr = INADDR_ANY;
  bind(fdl, (struct sockaddr*)&addr, sizeof(struct sockaddr));
  listen(fdl, 0);
  
  fd = -1;
  fdmax = 0;
  to.tv_sec = 0;
  to.tv_usec = SC8R_INTERVAL;
  
  memset(sc8r2, 0, sizeof(sc8r2));
  for(i=0; i<32; i++)
  {
    sc8r2[i].idx = rand() % 34;
    sc8r2[i].num = sc8r2[i].idx;
  }
  
  while(1)
  {
    FD_ZERO(&fds);
    
    FD_SET(fdmm[1], &fds);
    fdmax = fdmm[1];
    FD_SET(fdl, &fds);
    if(fdl > fdmax) fdmax = fdl;
    if(fd >= 0)
    {
      FD_SET(fd, &fds);
      if(fd > fdmax) fdmax = fd;
    }
    
    ret = select(fdmax+1, &fds, NULL, NULL, &to);
    if(!ret)
    {
      for(i=mm*4; i<mm*4+4; i++)
      {
        if(sc8r2[i].acq == 1)
        {
          if(!sc8r2[i].idx)
            sc8r2[i].idx = 1;
          else if((sc8r2[i].idx&0x3f) < 34)
          {
            const addr[4] = {2,7,18,23};
            // 1d028700000001ffff01
            buf[0] = 0x1d;
            buf[1] = addr[i&0x3];
            buf[2] = sc8r2[i].idx;
            buf[3] = 0;
            buf[4] = 0;
            buf[5] = 0;
            buf[6] = 1;	// mando solo contatti
            buf[7] = 0x14;	// rssi
            buf[8] = 0xff;
            buf[9] = 0;
            write(fdmm[1], buf, 10);
            
            sc8r2[i].idx++;
            sc8r2[i].num++;
            if(sc8r2[i].num >= SC8R_MAX_PERIF)
            {
              sc8r2[i].idx = 0;
              sc8r2[i].num = 0;
            }
          }
          else
          {
            sc8r2[i].idx &= ~0x3f;
            sc8r2[i].idx += 0x40;
          }
        }
      }
      to.tv_usec = SC8R_INTERVAL;
    }
    else if(ret > 0)
    {
      if(FD_ISSET(fdmm[1], &fds))
      {
        ret = read(fdmm[1], buf, 256);
        if(!ret) break;
        
        switch(buf[0])
        {
          case 2:
            buf[0] = 2;
            memset(buf+1, 0xff, 16);
            write(fdmm[1], buf, 17);
            break;
          case 3:
            for(i=0; i<16; i++)
            {
              perif[i*2] = buf[i+1] & 0x0f;
              perif[(i*2)+1] = buf[i+1] >> 4;
            }
            break;
          case 0x27:
            i = mm*4 + ((buf[1]&0x10)?2:0) + (((buf[1]&0xf)==2)?0:1);
            sc8r2[i].acq = buf[2];
            //sc8r2[i].num = 0;
            break;
          default:
            break;
        }
      }
      if((fd >= 0) && FD_ISSET(fd, &fds))
      {
        ret = read(fd, buf, 256);
        if(!ret)
        {
          close(fd);
          fd = -1;
        }
        else
        {
          buf[ret] = 0;
          ret = sscanf(buf, "%*c %d", &i);
          if((ret == 1) && ((i/256) == mm))
          {
            i &= 0xff;
            switch(buf[0])
            {
              case 'A':
                pstatus[i/8][0] |= (1<<(i&7));
                break;
              case 'a':
                pstatus[i/8][0] &= ~(1<<(i&7));
                break;
              case 'M':
                pstatus[i/8][1] |= (1<<(i&7));
                break;
              case 'm':
                pstatus[i/8][1] &= ~(1<<(i&7));
                break;
              case 'G':
                pstatus[i/8][2] |= (1<<(i&7));
                break;
              case 'g':
                pstatus[i/8][2] &= ~(1<<(i&7));
                break;
              default:
                break;
            }
            
            if(perif[i/8] == 2)
            {
              buf[0] = 7;
              buf[1] = i/8;
              buf[2] = 0;
              buf[3] = 0;
              for(s=0; s<4; s++)
              {
                if(pstatus[i/8][0] & (1<<s))
                {
                  /* allarme */
                  buf[3] |= (1<<(s+4));
                }
              }
              for(s=4; s<8; s++)
              {
                if(pstatus[i/8][0] & (1<<s))
                {
                  /* allarme */
                  buf[3] |= (1<<(s-4));
                }
              }
              write(fdmm[1], buf, 4);
            }
            else if(perif[i/8] == 0)
            {
              buf[0] = 7;
              buf[1] = i/8;
              buf[2] = 0;
              buf[3] = 0;
              if(pstatus[i/8][0] & 1) buf[2] = 2;
              write(fdmm[1], buf, 4);
            }
            else if(perif[i/8] == 8)
            {
              buf[0] = 8;
              buf[1] = i/8;
              buf[2] = 0;
              buf[3] = 0;
              buf[4] = 0;
              buf[5] = 0;
              for(s=0; s<8; s++)
              {
                if(pstatus[i/8][2] & (1<<s))
                {
                  /* guasto */
                  buf[3] |= (1<<s);
                  buf[4] |= (1<<s);
                  buf[5] |= (1<<s);
                }
                else if(pstatus[i/8][1] & (1<<s))
                {
                  if(pstatus[i/8][0] & (1<<s))
                  {
                    /* allarme + tamper */
                    buf[3] |= (1<<s);
                    buf[5] |= (1<<s);
                  }
                  else
                  {
                    /* tamper */
                    buf[4] |= (1<<s);
                    buf[5] |= (1<<s);
                  }
                }
                else if(pstatus[i/8][0] & (1<<s))
                {
                  /* allarme */
                  buf[3] |= (1<<s);
                }
              }
              write(fdmm[1], buf, 6);
            }
          }
        }
      }
      if(FD_ISSET(fdl, &fds))
      {
        if(fd >= 0) close(fd);
        fd = accept(fdl, NULL, NULL);
      }
    }
    else
      return;
  }
}

void _init()
{
  printf("Virtual mmaster (plugin): " __DATE__ " " __TIME__ "\n");
  prot_plugin_register("VMM", 0, NULL, NULL, (PthreadFunction)vmmaster_loop);
}

