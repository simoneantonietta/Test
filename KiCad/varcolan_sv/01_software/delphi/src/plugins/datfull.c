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

//#define LOG_EVENTO

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
#ifndef MNT_DETACH
#define MNT_DETACH 2
#endif
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>

#define EVENT_STRING_BASE	150

static const char *EventString[] = {
"12z",	// 150
"12z",
"1p",
"1p",
"12",
"12",
"1p",
"1p",
"1z",
"1z",
"1z",	// 160
"11",
"11",
"11",
"11",
"1d",
"1dd",
"1pd",
"1p",
"1p",
"1d",	// 170
"1p",
"1p",
"1p",
"1p",
"1d",
"1d",
"1",
"12",
"1pPPPPPPPP",
"11ssssssss",	// 180
"12ssssssss",
"12ssssssss",
"12",
"1H",
"1XXX",
"1ddd",
"12",
"1p",
"12",
"1pdd",	// 190
"1d",
"11",
"1",
"12",
"12d",
"1pPPPPPPPP",
"12z",
"14FH",
"11",
"1",	// 200
"1",
"1",	// ronda
"1",	// ronda
"1",	// ronda
"14HHHH",
"12",
"12x",	// 207
"12x",
"12x",
"11",	// 210
"11",
"11",
"12",
"12",
"12",
"1d",
"1",
"11",
"1",
"1",	// 220
"1",
"12dd",
"12ssssssss",
"12zzzzzzzz",
"1",
"1",
"1",
"1211111111",	// valori analogici
"1",
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
        j += 3;
        break;
      case '2':
        v = ev[k++] * 256;
        v += ev[k++];
        sprintf(buf+j, "%04d", v);
        j += 4;
        break;
      case '3':
        v = ev[k++];
        v += ev[k++] * 256;
        sprintf(buf+j, "%05d", v);
        j += 5;
        break;
      case '4':
        v = ev[k++];
        sprintf(buf+j, "%02d", v);
        j += 2;
        break;
      case 'd':
        v = ev[k++];
        sprintf(buf+j, "%d", v);
        j += strlen(buf+j);
        break;
      case 'F':
        v = ev[k++];
        if(v)
          sprintf(buf+j, " fine ");
        else
          sprintf(buf+j, " inizio ");
        j += strlen(buf+j);
        break;
      case 'H':
        sprintf(buf+j, "%02d:", ev[k++]);
        sprintf(buf+j+3, "%02d", ev[k++]);
        j += 5;
        break;
      case 'p':
/*
        v = ev[k++] * 256;
        v += ev[k++];
*/
        v = ev[k++];
        sprintf(buf+j, "%0d-%02d", v>>5, v&31);
        j += strlen(buf+j);
        break;
      case 'P':
        v = ev[k++];
        if((v&0xf) == 0xf)
          buf[j] = '-';
        else if((v&0xf) == 0xe)
          buf[j] = '*';
        else if((v&0xf) > 9)
          buf[j] = ((v&0xf)-10) + 'A';
        else
          buf[j] = (v&0xf) + '0';
        j++;
        v >>= 4;
        if(v == 0xf)
          buf[j] = '-';
        else if(v == 0xe)
          buf[j] = '*';
        else if(v > 9)
          buf[j] = ((v&0xf)-10) + 'A';
        else
          buf[j] = v + '0';
        j++;
        break;
      case 's':
        v = ev[k++];
        strcpy(buf+j, "----");
        if(v & bitOOS) buf[j] = 'S';
        if(v & bitFailure) buf[j+1] = 'G';
        if(v & bitSabotage) buf[j+2] = 'M';
        if(v & bitAlarm) buf[j+3] = 'A';
        j += 4;
        break;
      case 'x':
        v = ev[k++];
        sprintf(buf+j, "%02X", v);
        j += 2;
        break;
      case 'X':
        v = ev[k++] * 256;
        v += ev[k++];
        sprintf(buf+j, "%04X", v);
        j += 4;
        break;
      case 'z':
        v = ev[k++];
        if(!v)
          sprintf(buf+j, "Tot.");
        else if(v < 216)
          sprintf(buf+j, "S%03d", v);
        else if(v == 255)
          sprintf(buf+j, " -- ");
        else
          sprintf(buf+j, "I %02d", v-216);
        j += 4;
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
  char *p, buf[256], filename[64], buf2[256];
  FILE *fp, *fp2;
  time_t t;
  struct tm day;
  int curr_day;
  int mount_timer, umount_timer;
  
  if(!dev) return;
  
  pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
  
  support_log("DAT Init");
  
  mkdir("/tmp/storage", 0777);
  fp2 = NULL;
  curr_day = mount_timer = umount_timer = 0;
  
  /* Controllo se alla partenza la chiavetta è già montata. */
  mounted = 0;
  fp = fopen("/proc/mounts", "r");
  while(fgets(buf, 255, fp))
    if(strstr(buf, "/tmp/storage"))
    {
      t = time(NULL);
      localtime_r(&t, &day);
      sprintf(filename, "/tmp/storage/storico_%02d-%02d-%02d.dat", day.tm_year%100, day.tm_mon+1, day.tm_mday);
      fp2 = fopen(filename, "a");
      if(!fp2)
        umount2("/tmp/storage", MNT_DETACH);
      else
      {
        mounted = 1;
        support_log("Disco USB montato.");
#ifdef LOG_EVENTO
        buf[0] = Evento_Esteso;
        buf[1] = Ex_Stringa;
        buf[2] = 0;
        strcpy(buf+4, "Disco USB montato.");
        buf[3] = strlen(buf+4);
        codec_queue_event(buf);
#endif
      }
    }
  fclose(fp);
  
  fp = fopen("/tmp/log1.dat", "a");
  log = 0;
  
  while(1)
  {
    /* Monto la chiavetta (tentativi ogni 10s) */
    if(!mounted && (mount_timer > 100))
    {
      mount_timer = 0;
      
      if(!mount("/dev/sda1", "/tmp/storage", "vfat", 0, "") ||
         !mount("/dev/sdb1", "/tmp/storage", "vfat", 0, "") ||
         !mount("/dev/sda", "/tmp/storage", "vfat", 0, "") ||
         !mount("/dev/sdb", "/tmp/storage", "vfat", 0, ""))
      {
        support_log("Montato disco USB.");
#ifdef LOG_EVENTO
        buf[0] = Evento_Esteso;
        buf[1] = Ex_Stringa;
        buf[2] = 0;
        strcpy(buf+4, "Montato disco USB.");
        buf[3] = strlen(buf+4);
        codec_queue_event(buf);
#endif
//printf("Montato\n");
        t = time(NULL);
        localtime_r(&t, &day);
        sprintf(filename, "/tmp/storage/storico_%02d-%02d-%02d.dat", day.tm_year%100, day.tm_mon+1, day.tm_mday);
        fp2 = fopen(filename, "a");
        if(fp2)
        {
          mounted = 1;
          //support_log("Aperto file di storico.");
#ifdef LOG_EVENTO
          buf[0] = Evento_Esteso;
          buf[1] = Ex_Stringa;
          buf[2] = 0;
          strcpy(buf+4, "Aperto file di storico.");
          buf[3] = strlen(buf+4);
          codec_queue_event(buf);
#endif
        }
        else
        {
          support_log("Errore apertura file di storico.");
#ifdef LOG_EVENTO
          buf[0] = Evento_Esteso;
          buf[1] = Ex_Stringa;
          buf[2] = 0;
          strcpy(buf+4, "Errore apertura file di storico.");
          buf[3] = strlen(buf+4);
          codec_queue_event(buf);
#endif
        }
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
    {
      usleep(100000);
/* 23/03/2011 */
      if(!mounted) mount_timer++;
      umount_timer++;
      if(umount_timer >= 400)
      {
//printf("*\n");
        /* Ogni 40s circa... */
        umount_timer = 0;
        t = time(NULL);
        localtime_r(&t, &day);
        if((day.tm_hour == 0) && (day.tm_min == 20))
        {
                if(fp2) fclose(fp2);
                res = umount2("/tmp/storage", MNT_DETACH);
//printf("Smonta (%d)\n", res);
                if(!res)
                  mounted = 0;
                else
                  curr_day = 0;	// forzo la riapertura del file
                support_log("Smontato disco USB.");
#ifdef LOG_EVENTO
                buf[0] = Evento_Esteso;
                buf[1] = Ex_Stringa;
                buf[2] = 0;
                strcpy(buf+4, "Smontato disco USB.");
                buf[3] = strlen(buf+4);
                codec_queue_event(buf);
#endif
        }
      }
/**************/
    }
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
      else if(ev.Event[8] == 253)
      {
        if((ev.Event[9] == 0) || (ev.Event[9] == 1))
        {
          ev.Event[12 + ev.Event[11]] = 0;
          sprintf(buf2, "2530%d%02d %s", ev.Event[9], ev.Event[10], ev.Event+12);
          p = buf2;
        }
        else if(ev.Event[9] == 2)
        {
          ev.Event[11 + ev.Event[10]] = 0;
          sprintf(buf2, "25302%s", ev.Event+11);
          p = buf2;
        }
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
            sprintf(filename, "/tmp/storage/storico_%02d-%02d-%02d.dat", day.tm_year%100, day.tm_mon+1, day.tm_mday);
            fp2 = fopen(filename, "a");
          }
          
          if(!fp2 || !(res=fwrite(buf, 1, strlen(buf), fp2)))
          {
//printf("Scrittura fallita.\n");
            if(fp2) fclose(fp2);
            res = umount2("/tmp/storage", MNT_DETACH);
            if(!res)
              mounted = 0;
            else
              curr_day = 0;	// Forzo la riapertura del file
            support_log("Smontato disco USB.");
#ifdef LOG_EVENTO
            buf[0] = Evento_Esteso;
            buf[1] = Ex_Stringa;
            buf[2] = 0;
            strcpy(buf+4, "Smontato disco USB.");
            buf[3] = strlen(buf+4);
            codec_queue_event(buf);
#endif
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
                res = umount2("/tmp/storage", MNT_DETACH);
                if(!res)
                  mounted = 0;
                else
                  curr_day = 0;	// Forzo la riapertura del file
                support_log("Smontato disco USB.");
#ifdef LOG_EVENTO
                buf[0] = Evento_Esteso;
                buf[1] = Ex_Stringa;
                buf[2] = 0;
                strcpy(buf+4, "Smontato disco USB.");
                buf[3] = strlen(buf+4);
                codec_queue_event(buf);
#endif
              }
            }
            else
            {
              res = fsync(fileno(fp2));
              if(res)
              {
                {
//printf("Fsync fallito.\n");
                  if(fp2) fclose(fp2);
                  res = umount2("/tmp/storage", MNT_DETACH);
                  if(!res)
                    mounted = 0;
                  else
                    curr_day = 0;	// Forzo la riapertura del file
                  support_log("Smontato disco USB.");
#ifdef LOG_EVENTO
                  buf[0] = Evento_Esteso;
                  buf[1] = Ex_Stringa;
                  buf[2] = 0;
                  strcpy(buf+4, "Smontato disco USB.");
                  buf[3] = strlen(buf+4);
                  codec_queue_event(buf);
#endif
                }
              }
//else
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
  config.consumer[dev.consumer].type = PROT_SAETNET_OLD;
  config.consumer[dev.consumer].progr = 1;
  dat_loop(&dev);
}

void _init()
{
  pthread_t pth;
  
  printf("Com90 .dat FULL (plugin): " __DATE__ " " __TIME__ "\n");
//  prot_plugin_register("DAT", 0, NULL, NULL, (PthreadFunction)dat_loop);
  
  pthread_create(&pth,  NULL, (PthreadFunction)dat_loop2, NULL);
  pthread_detach(pth);
}

