/*
NOTE:
- Al massimo sono gestite due chiavette (che si registrano su sda1 e sdb1),
se si dovesse usare una terza chiavetta occorre resettare la centrale.
- Gli eventi che capitano mentre la chiavetta non è inserita vengono persi.
- Nel caso in cui venga rimossa la chiavetta, solo alla seconda timbratura
  viene riconosciuta la rimozione. Perciò se si rimuove la chiavetta, si
  esegue una sola timbratura e poi si reinserisce la stessa chiavetta,
  il log risultante presenta il record mancato tutto a zero. E' un problema
  per il fatto che il file risultante potrebbe non essere più leggibile,
  comunque i dati sono tutti presenti.
  Se invece si rimuove la chiavetta e la si sostituisce con un'altra,
  occorrono due timbrature prima che si avvii la registrazione del nuovo
  log (che partirebbe quindi dalla terza timbratura).
*/

#include "protocol.h"
#include "codec.h"
#include "event.h"
#include "database.h"
#include "support.h"
#include <unistd.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>

#define EVENT_STRING_BASE	207

static const char *EventString[] = {
"12x",
"12x",
"12x",
};

#define EVENT_STRING_BASE2	7

static const char *EventString2[] = {
"1xx3x1",
"1xx3xx",
"1xx3x13",
"1xx3xx3",
};

static char* event_format(const char *msg, unsigned char *ev)
{
  static char buf[256];
  int i, j, k, v;
  
  for(i=j=k=0; msg[i]; i++)
  {
    switch(msg[i])
    {
      case '1':
        v = ev[k++];
        sprintf(buf+j, "%03d", v);
        j += strlen(buf+j);
        break;
      case '2':
        v = ev[k++] * 256;
        v += ev[k++];
        sprintf(buf+j, "%04d", v);
        j += strlen(buf+j);
        break;
      case '3':
        v = ev[k++];
        v += ev[k++] * 256;
        sprintf(buf+j, "%05d", v);
        j += strlen(buf+j);
        break;
      case 'x':
        v = ev[k++];
        sprintf(buf+j, "%02X", v);
        j += strlen(buf+j);
        break;
      case 'X':
        v = ev[k++] * 256;
        v += ev[k++];
        sprintf(buf+j, "%04X", v);
        j += strlen(buf+j);
        break;
      default:
        break;
    }
  }
  
  buf[j++] = '\0';
  return buf;
}

static void dat_loop(ProtDevice *dev)
{
  Event ev;
  int res, log, mounted;
  char *p, buf[256], filename[64];
  FILE *fp, *fp2;
  time_t t;
  struct tm day;
  int curr_day;
  
  if(!dev) return;
  
  pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
  
  support_log("DAT Init");
  
  mkdir("/tmp/storage", 0777);
  fp2 = NULL;
  curr_day = 0;
  
  /* Controllo se alla partenza la chiavetta è già montata. */
  mounted = 0;
  fp = fopen("/proc/mounts", "r");
  while(fgets(buf, 255, fp))
    if(strstr(buf, "/tmp/storage"))
    {
      t = time(NULL);
      localtime_r(&t, &day);
      sprintf(filename, "/tmp/storage/timbrature_%02d-%02d-%02d.dat", day.tm_year%100, day.tm_mon+1, day.tm_mday);
      fp2 = fopen(filename, "a");
      if(!fp2)
        umount("/tmp/storage");
      else
        mounted = 1;
    }
  fclose(fp);
  
  fp = fopen("/tmp/log1.dat", "a");
  log = 0;
  
  while(1)
  {
    /* Monto la chiavetta */
    if(!mounted)
    {
      if(!mount("/dev/sda1", "/tmp/storage", "vfat", 0, "") ||
         !mount("/dev/sdb1", "/tmp/storage", "vfat", 0, "") ||
         !mount("/dev/sda", "/tmp/storage", "vfat", 0, "") ||
         !mount("/dev/sdb", "/tmp/storage", "vfat", 0, ""))
      {
        t = time(NULL);
        localtime_r(&t, &day);
        sprintf(filename, "/tmp/storage/timbrature_%02d-%02d-%02d.dat", day.tm_year%100, day.tm_mon+1, day.tm_mday);
        fp2 = fopen(filename, "a");
        if(fp2)
          mounted = 1;
      }
    }
    
#if 0
/* 02/08/2006 */
    if(N_ORA)
    {
      system("cp /tmp/log1.dat /saet/data/timbrature1.dat");
      system("cp /tmp/log2.dat /saet/data/timbrature2.dat");
    }
/**************/
#endif
    while(!(res = codec_get_event(&ev, dev)));
    if(res < 0)	/* no events queued */
      usleep(100000);
    else
    {
      res = ev.Event[8] - EVENT_STRING_BASE;
      if((res >= 0) && (res < (sizeof(EventString) / sizeof(char*))))
        p = event_format(EventString[res], ev.Event + 8);
      else if((ev.Event[8] == 247) && (ev.Event[9] == 0))	// Tebe
      {
        res = ev.Event[10] - EVENT_STRING_BASE2;
        if((res >= 0) && (res < (sizeof(EventString2) / sizeof(char*))))
          p = event_format(EventString2[res], ev.Event + 8);
        else
          p = NULL;
      }
      else
        p = NULL;
      
      if(p)
      {
        sprintf(buf, "  %04d %04d %02d-%02d-%02d %02d:%02d.%02d %s\r\n",
          ev.DeviceID[0] * 256 + ev.DeviceID[1],
          ev.Event[0] * 256 + ev.Event[1],
          ev.Event[2], ev.Event[3], ev.Event[4], ev.Event[5], ev.Event[6] & 0x3f, ev.Event[7],
          p);
        fwrite(buf, 1, strlen(buf), fp);
//printf(buf);
        fflush(fp);
        log++;
        if(log == 1000)
        {
          fclose(fp);
          rename("/tmp/log1.dat", "/tmp/log2.dat");
          fp = fopen("/tmp/log1.dat", "a");
          log = 0;
        }
        
        if(mounted)
        {
          t = time(NULL);
          localtime_r(&t, &day);
          if(curr_day != day.tm_mday)
          {
//printf("Nuovo log.\n");
            curr_day = day.tm_mday;
            fclose(fp2);
            sprintf(filename, "/tmp/storage/timbrature_%02d-%02d-%02d.dat", day.tm_year%100, day.tm_mon+1, day.tm_mday);
            fp2 = fopen(filename, "a");
          }
          
          if(!fp2 || !(res=fwrite(buf, 1, strlen(buf), fp2)))
          {
//printf("Scrittura fallita.\n");
            if(fp2) fclose(fp2);
            umount("/tmp/storage");
            mounted = 0;
          }
          else
          {
//printf("Sync (%d)\n", res);
            res = fflush(fp2);
//printf("OK (%d)\n", res);
            if(res)
            {
              {
//printf("Flush fallito.\n");
                if(fp2) fclose(fp2);
                umount("/tmp/storage");
                mounted = 0;
              }
            }
            else
            {
              res = fsync(fileno(fp2));
//printf("OK (%d)\n", res);
            }
          }
        }
      }
    }
  }
}

static void dat_loop2(void *null)
{
  ProtDevice dev;
  
  dev.consumer = 7;
  LS_ABILITATA[7] = 0x03;
  dat_loop(&dev);
}

void _init()
{
  pthread_t pth;
  
  printf("Com90 .dat plugin: " __DATE__ " " __TIME__ "\n");
//  prot_plugin_register("DAT", 0, NULL, NULL, (PthreadFunction)dat_loop);
  
  pthread_create(&pth,  NULL, (PthreadFunction)dat_loop2, NULL);
  pthread_detach(pth);
}

