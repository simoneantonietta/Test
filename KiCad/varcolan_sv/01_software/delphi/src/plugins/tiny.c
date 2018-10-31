#include "protocol.h"
#include "codec.h"
#include "command.h"
#include "database.h"
#include "support.h"
#include "user.h"
#include <unistd.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>

static void tiny_loop(ProtDevice *dev)
{
  Event ev;
  int res, n, z;
  unsigned char buf[80];
  fd_set fds;
  struct timeval to;

  if(!dev) return;
  
  pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
  
  sprintf(buf, "Tiny connect [%d]", getpid());
  support_log(buf);
  
  FD_ZERO(&fds);
  to.tv_sec = 0;
  to.tv_usec = 100000;
  
  while(1)
  {
    FD_SET(dev->fd, &fds);
    res = select(dev->fd+1, &fds, NULL, NULL, &to);
    if(res)
    {
      /* Ricezione comandi */
      if(read(dev->fd, buf, 80) <= 0)
      {
        support_log("Tiny end");
        return;
      }
      
      switch(buf[0])
      {
        case 'A':	// attiva zona
          buf[4] = 0;
          n = atoi(buf+1);
          database_lock();
          cmd_zone_on(n, 0);
          database_unlock();
          break;
        case 'B':	// attiva zona con allarme
          buf[4] = 0;
          n = atoi(buf+1);
          database_lock();
          cmd_zone_on(n, 1);
          database_unlock();
          break;
        case 'C':	// disattiva zona
          buf[4] = 0;
          n = atoi(buf+1);
          database_lock();
          cmd_zone_off(n, 0);
          database_unlock();
          break;
        case 'D':	// in servizio sensore
          buf[5] = 0;
          n = atoi(buf+1);
          database_lock();
          cmd_sensor_on(n);
          database_unlock();
          break;
        case 'E':	// fuori servizio sensore
          buf[5] = 0;
          n = atoi(buf+1);
          database_lock();
          cmd_sensor_off(n);
          database_unlock();
          break;
        case 'F':	// attivazione uscita
          buf[5] = 0;
          n = atoi(buf+1);
          database_lock();
          Donanl(&AT[n]);
          database_unlock();
          break;
        case 'G':	// disattivazione uscita
          buf[5] = 0;
          n = atoi(buf+1);
          database_lock();
          Doffa(&AT[n]);
          database_unlock();
          break;
        case 'Z':	// associa sensore a zona
          buf[8] = 0;
          z = atoi(buf+5);
          buf[5] = 0;
          n = atoi(buf+1);
          database_lock();
          Zona_SE[n] = z;
          database_changed = 1;
          database_unlock();
          break;
        default:
          break;
      }
    }
    else
    {
      to.tv_sec = 0;
      to.tv_usec = 100000;
      
      /* Controllo eventi */
      do
      {
        while(!(res = codec_get_event(&ev, dev)));
        if(res > 0)
        {
          sprintf(buf, "%04d%02d%02d%02d%02d%02d%02d", ev.Event[0]*256+ev.Event[1],
            ev.Event[2], ev.Event[3], ev.Event[4], 
            ev.Event[5], ev.Event[6]&0x3f, ev.Event[7]);
          
          /* Invia l'evento */
          switch(ev.Event[8])
          {
            case 150:
              buf[16] = 'A';
              buf[17] = 'I';
              sprintf(buf+18, "%04d", ev.Event[9]*256+ev.Event[10]);
              sprintf(buf+22, "%03d", ev.Event[11]);
              n = 25;
              break;
            case 151:
              buf[16] = 'A';
              buf[17] = 'F';
              sprintf(buf+18, "%04d", ev.Event[9]*256+ev.Event[10]);
              sprintf(buf+22, "%03d", ev.Event[11]);
              n = 25;
              break;
            case 154:
              buf[16] = 'B';
              buf[17] = 'I';
              sprintf(buf+18, "%04d", ev.Event[9]*256+ev.Event[10]);
              n = 22;
              break;
            case 155:
              buf[16] = 'B';
              buf[17] = 'F';
              sprintf(buf+18, "%04d", ev.Event[9]*256+ev.Event[10]);
              n = 22;
              break;
            case 183:
              buf[16] = 'C';
              buf[17] = 'I';
              sprintf(buf+18, "%04d", ev.Event[9]*256+ev.Event[10]);
              n = 22;
              break;
            case 194:
              buf[16] = 'C';
              buf[17] = 'F';
              sprintf(buf+18, "%04d", ev.Event[9]*256+ev.Event[10]);
              n = 22;
              break;
            case 168:
              buf[16] = 'D';
              buf[17] = 'I';
              sprintf(buf+18, "%03d", ev.Event[9]*256+ev.Event[10]);
              n = 21;
              break;
            case 169:
              buf[16] = 'D';
              buf[17] = 'F';
              sprintf(buf+18, "%03d", ev.Event[9]*256+ev.Event[10]);
              n = 21;
              break;
            case 158:
              buf[16] = 'E';
              buf[17] = 'I';
              sprintf(buf+18, "%03d", ev.Event[9]);
              n = 21;
              break;
            case 159:
              buf[16] = 'E';
              buf[17] = 'F';
              sprintf(buf+18, "%03d", ev.Event[9]);
              n = 21;
              break;
            case 160:
              buf[16] = 'E';
              buf[17] = 'E';
              sprintf(buf+18, "%03d", ev.Event[9]);
              n = 21;
              break;
            default:
              /* Evento non gestito */
              n = 0;
              break;
          }
          
          if(n) write(dev->fd, buf, n);
        }
      }
      while(res > 0);
    }
  }
}

void _init()
{
  printf("Tiny plugin: " __DATE__ " " __TIME__ "\n");
  prot_plugin_register("TNY", 0, NULL, NULL, (PthreadFunction)tiny_loop);
}

