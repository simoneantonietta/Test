#include "database.h"
#include "serial.h"
#include "codec.h"
#include "support.h"
#include <stdio.h>
#include <unistd.h>

/* Timeout 1s */
#define EB_TIMEOUT 10

#define EB_BASE_SENS_AM (0+255)	// allarme memorizzato
#define EB_BASE_SENS_M (4096+255)	// manomissione
#define EB_BASE_SENS_G (8192+255)	// guasto
#define EB_BASE_SENS_S (12288+255)	// fuori servizio
/* #define EB_BASE_SENS_A (16384+255)	// stato di allarme sensore */
#define EB_BASE_ZONE_S (20480+255)	// attivazione zona
#define EB_BASE_ZONE_A (20992+255)	// allarme zona
#define EB_BASE_ZONE_P (21504+255)	// zona non pronta all'inserimento

static unsigned char ebus_zone_a[256] = {0, };

static int ebus_convert(Event *ev, unsigned char *msg)
{
  int num;
  
  msg[0] = 85;
  msg[1] = 128;
  msg[4] = 0;
  num = 0;
  
  switch(ev->Event[8])
  {
    case 154:	// manomissione sensore
      num = EB_BASE_SENS_M + ((ev->Event[9]*256+ev->Event[10])*2);
      break;
    case 155:	// fine manomissione sensore
      num = EB_BASE_SENS_M + ((ev->Event[9]*256+ev->Event[10])*2) + 1;
      break;
    case 158:	// attivata zona
      num = EB_BASE_ZONE_S + (ev->Event[9]*2);
      break;
    case 159:	// disattivata zona
      num = EB_BASE_ZONE_S + (ev->Event[9]*2) + 1;
      break;
    case 161:	// in servizio sensore
      num = EB_BASE_SENS_S + ((ev->Event[9]*256+ev->Event[10])*2);
      break;
    case 162:	// fuori servizio sensore
      num = EB_BASE_SENS_S + ((ev->Event[9]*256+ev->Event[10])*2) + 1;
      break;
    case 183:	// guasto sensore
      num = EB_BASE_SENS_G + ((ev->Event[9]*256+ev->Event[10])*2);
      break;
    case 194:	// fine guasto sensore
      num = EB_BASE_SENS_G + ((ev->Event[9]*256+ev->Event[10])*2) + 1;
      break;
    case 222:
//printf("%d %d %d %d %d\n", ev->Event[8], ev->Event[9], ev->Event[10], ev->Event[11], ev->Event[12]);
      if(ev->Event[11] == 1)	// allarme memorizzato
      {
        int sens = ev->Event[9]*256+ev->Event[10];
        if(ev->Event[12] == 1)	// inizio
        {
          num = EB_BASE_SENS_AM + (sens*2);
          if(!ebus_zone_a[Zona_SE[sens]]) ebus_zone_a[Zona_SE[sens]] = 1;
        }
        else if((ev->Event[12] == 3) || (ev->Event[12] == 5))	// fine
        {
          num = EB_BASE_SENS_AM + (sens*2) + 1;
          if(!(ZONA[Zona_SE[sens]] & bitAlarm)) ebus_zone_a[Zona_SE[sens]] = 3;
        }
      }
      break;
    default:
      break;
  }
  
  msg[2] = num/255;
  msg[3] = num%255 + 1;
  return msg[2];
}

static void ebus_loop(ProtDevice *dev)
{
  unsigned char buf[6], msg[6];
  Event ev;
  int n, i, res;
  int eb_timeout, eb_retry, eb_last_NS;
  
  if(!dev) return;
  
  pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
  
  ser_setspeed(dev->fd, config.consumer[dev->consumer].data.serial.baud1, 1);
  config.consumer[dev->consumer].type = PROT_SAETNET_OLD;
  LS_ABILITATA[dev->consumer] = 0x03;
  
  support_log("EchoBus Init");
  
  eb_timeout = 0;
  eb_retry = 0;
  eb_last_NS = 0;
  
  while(1)
  {
    n = read(dev->fd, buf+dev->i, 1);
    if(n < 0) return;
    
    if(n)
    {
      if(dev->i || (buf[0] == 85) || ((buf[0] >= 180) && (buf[0] <= 183)))
        dev->i++;
      else
        dev->i = 0;
      
      if(dev->i == 5)
      {
//printf("Ricevuto messaggio (%d %d %d %d %d)\n", buf[0], buf[1], buf[2], buf[3], buf[4]);
        dev->i = 0;
        
        if((buf[1] == 70) && (buf[2] == eb_last_NS))
        {
          eb_last_NS = 0;
        }
        else if(buf[1] == 128)
        {
          /* Parse comando */
          
          /* Invio ACK */
          buf[1] = 70;
          buf[3] = 0;
          buf[4] = 0;
          write(dev->fd, buf, 5);
        }
        n = 0;
      }
    }
    
    if(!n)
    {
      dev->i = 0;
      if(eb_last_NS)
      {
        eb_timeout++;
        if(eb_timeout == EB_TIMEOUT)
        {
          eb_timeout = 0;
          eb_retry++;
          if(eb_retry < 10)
          {
            /* Reinvio messaggio */
            write(dev->fd, msg, 5);
            continue;
          }
          else
          {
            eb_retry = 0;
            /* Scarto l'evento */
            eb_last_NS = 0;
          }
        }
        else
          continue;
      }
      
      msg[0] = 85;
      msg[1] = 128;
      msg[4] = 0;
      n = 0;
      
      for(i=0; i<(1+n_ZS+n_ZI); i++)
      {
        if(ebus_zone_a[i] == 1)
        {
          ebus_zone_a[i] = 2;	// zona in allarme
          n = EB_BASE_ZONE_A + (i*2);
          break;
        }
        else if(ebus_zone_a[i] == 3)
        {
          ebus_zone_a[i] = 0;	// zona a riposo
          n = EB_BASE_ZONE_A + (i*2) + 1;
          break;
        }
        else if((ebus_zone_a[i] != 4) && (ZONA[i] & bitStatusInactive))
        {
          ebus_zone_a[i] = 4;	// zona non pronta all'inserimento
          n = EB_BASE_ZONE_P + (i*2);
          break;
        }
        else if((ebus_zone_a[i] == 4) && !(ZONA[i] & bitStatusInactive))
        {
          ebus_zone_a[i] = 0;	// zona pronta all'inserimento
          n = EB_BASE_ZONE_P + (i*2) + 1;
          break;
        }
      }
      
      if(n)
      {
        msg[2] = n/255;
        msg[3] = n%255 + 1;
        eb_last_NS = msg[2];
        write(dev->fd, msg, 5);
        continue;
      }
      
      do
      {
        while(!(res = codec_get_event(&ev, dev)));
        if(res > 0)
        {
          eb_last_NS = ebus_convert(&ev, msg);
          
          /* Invio messaggio */
          if(eb_last_NS)
            write(dev->fd, msg, 5);
          else
            res = 0;
        }
      }
      while(!res);
    }
  }
}

void _init()
{
  printf("EchoBus plugin: " __DATE__ " " __TIME__ "\n");
  prot_plugin_register("ECHOBUS", 0, NULL, NULL, (PthreadFunction)ebus_loop);
}
