#include "delphi.h"
#include "protocol.h"
#include "database.h"
#include "master.h"
#include "command.h"
#include "serial.h"
#include "modem.h"
#include "gsm.h"
#include "timeout.h"
#include "support.h"
#include "nvmem.h"
#include "ethernet.h"
#include "strings.h"
#include "version.h"
#include "lara.h"
#include "tebe.h"
#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <dlfcn.h>
#include <dirent.h>

#ifdef __CRIS__
#include <asm/rtc.h>
#include <sys/ioctl.h>
#include <asm/etraxgpio.h>
#endif

#if defined __i386__ || defined __x86_64__
char RootDir[256];
#endif

int delphi_modo = DELPHICENTRALE;

int Restart = 0;
int RestartProgram = 0;
int delphi_restart_count = 0;

static int BoardAnubi = 1;
static sem_t delphi_sem;

extern int UserSystemStatus;

const char UserFileName[] = "/saet/libuser.so";
#if 0
#ifdef __CRIS__
const char NewUserFileName[] = "/var/tmp/libuser.so.new";
#endif
#ifdef __arm__
const char NewUserFileName[] = "/var/tmp/libuser.so.arm";
#endif
const char NewUserFileNameDelete[] = "/var/tmp/libuser.so.*";
#else
#ifdef __CRIS__
const char NewUserFileName[] = "/tmp/libuser.so.new";
#endif
#ifdef __arm__
const char NewUserFileName[] = "/tmp/libuser.so.arm";
#endif
#if defined __i386__ || defined __x86_64__
const char NewUserFileName[] = "/tmp/libuser.so.new";
#endif
const char NewUserFileNameDelete[] = "/tmp/libuser.so.*";
#endif
const char ConfFileName[] = "/saet/saet.conf";
const char ConfFileName2[] = "/saet/saet.var";
const char NVFileName[] = "/saet/saet.nv";
const char NVTmpFileName[] = "/saet/saet.nv.tmp";
const char ConsumerFileName[] = "/saet/consumer.conf";
const char StringsFileName[] = "/saet/strings.conf";
const char RestartFileName[] = "/saet/saet.restart";
const char StopClockFile[] = "/saet/stopclock.conf";
const char VersionFile[] = "/tmp/version";

const char ref_ConfFileName[] = "/saet/saet.ref/saet.conf";
const char ref_NVFileName[] = "/saet/saet.ref/saet.nv";
const char ref_StringsFileName[] = "/saet/saet.ref/strings.conf";
const char ref_ConsumerFileName[] = "/saet/saet.ref/consumer.conf";
const char ref_SprintFileName[] = "/saet/saet.ref/sprint.xml";

const char Boot2aFileName[] = "/var/tmp/saet.2a";
const char Boot2bFileName[] = "/var/tmp/saet.2b";

const char DirectCodesFileName[] = "/tmp/codes.1";
const char DirectCodeStringsFileName[] = "/tmp/codes.2";

unsigned char Boot1Error[] = "   \"ERRORE - \"saet.nv\": file corrotto.";
unsigned char Boot2Error[] = "   0ERRORE - \"libuser.so\": file corrotto -> rimosso.";

void *delphi_lang = NULL;
static char **delphi_str[] = {&str_d_0, &str_d_1, &str_d_2, &str_d_3, &str_d_4,
	&str_d_5, &str_d_6, &str_d_7, &str_d_8, &str_d_9, &str_d_10, &str_d_11,
	&str_d_12, &str_d_13, &str_d_14, &str_d_15, &str_d_16};

#if defined __i386__ || defined __x86_64__
#define ROOTDIRPOOLDIM 80
static char *rootdirpool[ROOTDIRPOOLDIM] = {NULL, };
char* addrootdir(const char *file)
{
  int i;
  
  for(i=0; (i<ROOTDIRPOOLDIM)&&rootdirpool[i]&&strcmp(rootdirpool[i]+strlen(RootDir),file); i++);
  if(i == ROOTDIRPOOLDIM) return NULL;
  if(!rootdirpool[i])
  {
    rootdirpool[i] = malloc(strlen(RootDir)+strlen(file)+1);
    strcpy(rootdirpool[i], RootDir);
    strcat(rootdirpool[i], file);
  }
  return rootdirpool[i];
}
#endif

static void delphi_update_day(unsigned char g, unsigned char m, unsigned char a, unsigned char *t)
{
  unsigned char gg, mm, aa, tt, *p;
  
  if(TPDGF)
  {
    p = TPDGF;
    while((gg = *p++) != 0xff)
    {
      mm = *p++;
      tt = *p++;
      
      if((gg == g) && (mm == m)) *t = tt;
    }
  }
  if(TPDGV)
  {
    p = TPDGV;
    while((gg = *p++) != 0xff)
    {
      mm = *p++;
      aa = *p++;
      tt = *p++;
      
      if((gg == g) && (mm == m) && (aa == a)) *t = tt;
    }
  }
}

void delphi_update_days()
{
  int i;
  time_t ltime;
  struct tm *day;
  
  time(&ltime);
  day = localtime(&ltime);  
  delphi_update_day(day->tm_mday, day->tm_mon + 1, day->tm_year % 100, &TIPO_GIORNO);
  /* Mi posiziono alle 12:00 per evitare il problema del passaggio da ora legale a ora solare */
  day->tm_hour = 12;
  ltime = mktime(day);
  
  for(i=0; i<n_FESTIVI; i++)
  {
    ltime += (24 * 3600);	// add one day
    day = localtime(&ltime);
    delphi_update_day(day->tm_mday, day->tm_mon + 1, day->tm_year % 100, &FESTIVI[i]);
    lara_update_day(day, &FESTIVI[i]);
  }
}

static int delphi_check_time(unsigned char *range, int actual)
{
  int start, end;
  
  if((range[0] == 0xff) && (range[2] == 0xff)) return 0;
  
  start = range[0] * 60 + range[1];
  end = range[2] * 60 + range[3];
  
  if(range[0] == 0xff)
  {
    if(actual == end) return 2;
    if(actual == 0) return 1;
    return 0;
  }
  
  if(range[2] == 0xff)
  {
    if(actual == start) return 1;
    if(actual == 0) return 2;
    return 0;
  }
  
  if(start < end)
  {
    if((actual >= start) && (actual < end))
      return 1;
    else
      return 2;
  }

  if((actual >= start) || (actual < end))
    return 1;
  else
    return 2;  
}

void delphi_check_timerange()
{
  int i, t;
  
  t = ORE * 60 + MINUTI;
  
  /* We have to see if some FAG in alarm is not in the new FASR */
  if(N_GIORNO)
    for(i=0; i<n_FAG; i++) FAG[i] |= 0x80;

  if(FASR[GIORNO_S-1])
  {
    for(i=0; FASR[GIORNO_S-1][i] != 255; i++)
    {
      FAG[FASR[GIORNO_S-1][i]] &= ~0x80;
      switch(delphi_check_time(FAGR[FASR[GIORNO_S-1][i]], t))
      {
        case 1:
          database_set_alarm2(&(FAG[FASR[GIORNO_S-1][i]]));
          break;
        case 2:
          database_reset_alarm2(&(FAG[FASR[GIORNO_S-1][i]]));
          break;
        default:
          break;
      }
    }
  }

  if(N_GIORNO)
    for(i=0; i<n_FAG; i++)
    {
      if(FAG[i] & 0x80)
        database_reset_alarm2(&FAG[i]);
      FAG[i] &= ~0x80;
    }
}

void delphi_check_periods()
{
  int i, inizio, fine, oggi;
  
  for(i=0; i<32; i++)
  {
    inizio = periodo[i].mesei * 100 + periodo[i].giornoi;
    if(inizio)
    {
      fine = periodo[i].mesef * 100 + periodo[i].giornof;
      oggi = MESE * 100 + GIORNO;
      if(inizio < fine)
      {
        if((oggi >= inizio) && (oggi <= fine))
          database_set_alarm2(&ME[605+i]);
        else
          database_reset_alarm2(&ME[605+i]);
      }
      else
      {
        if((oggi < inizio) && (oggi > fine))
          database_reset_alarm2(&ME[605+i]);
        else
          database_set_alarm2(&ME[605+i]);
      }
    }
    else
      database_reset_alarm2(&ME[605+i]);
  }
}

#if defined __CRIS__ || defined __arm__
void delphi_restart()
{
  FILE *fp;
  int i;
  
  fp = fopen(ADDROOTDIR(DirectCodesFileName), "w");
  fwrite(RONDA, sizeof(RONDA), 1, fp);
  fclose(fp);
  fp = fopen(ADDROOTDIR(DirectCodeStringsFileName), "w");
  for(i=0; i<256; i++)
  {
    if(CodeName[i])
      fprintf(fp, "C%d=%s\n", i, CodeName[i]);
  }
  for(i=0; i<64; i++)
  {
    if(RadioName[i])
      fprintf(fp, "R%d=%s\n", i, RadioName[i]);
  }
  fclose(fp);
  
  fp = fopen(ADDROOTDIR(RestartFileName), "w");
  fclose(fp);
  if((GSM_Status[1] > 0) && (GSM_Status[1] <= 3))
  {
    GSM_Call_terminate(0);
  }
  else if(GSM_Status[1] > 3)
  {
    GSM_Call_terminate(1);
  }
  if((TMD_Status[1] > 0) && (TMD_Status[1] <= 3))
  {
    TMD_Call_terminate();
  }
  else if(TMD_Status[1] > 3)
  {
    TMD_Call_terminate();
  }
  RestartProgram = 10;
}
#endif

static void delphi_load_codes()
{
  FILE *fp;
  char buf[64];
  int i, idx;
  
  fp = fopen(ADDROOTDIR(DirectCodesFileName), "r");
  if(fp)
  {
    fread(RONDA, 1, sizeof(RONDA), fp);
    fclose(fp);
    
    database_save();
    unlink(ADDROOTDIR(DirectCodesFileName));
  }
  
  fp = fopen(ADDROOTDIR(DirectCodeStringsFileName), "r");
  if(fp)
  {
    while(fgets(buf, 64, fp))
    {
      switch(buf[0])
      {
        case 'C':
          idx = -1;
          sscanf(buf+1, "%d", &idx);
          if(idx < 0) continue;
          for(i=2; (buf[i]!='=') && (i<64); i++);
          if(i == 64) continue;
          if(CodeName[idx]) free(CodeName[idx]);
          CodeName[idx] = strdup(buf + i + 1);
          break;
        case 'R':
          idx = -1;
          sscanf(buf+1, "%d", &idx);
          if(idx < 0) continue;
          for(i=2; (buf[i]!='=') && (i<64); i++);
          if(i == 64) continue;
          if(RadioName[idx]) free(RadioName[idx]);
          RadioName[idx] = strdup(buf + i + 1);
          break;
        default:
          break;
      }
    }
    fclose(fp);
    
    string_save();
    unlink(ADDROOTDIR(DirectCodeStringsFileName));
  }
}

static void delphi_timer()
{
  static char tick = 0;
  static int stopclock;
  
  time_t ltime;
  struct tm *datehour;
  int i, t;
#ifdef __CRIS__
  int fd;
#endif

  /* LIN timeout */
  
  for(i=0; i<MAX_NUM_CONSUMER; i++)
  {
    if(LIN_TO[i])
    {
      LIN_TO[i]--;
      if(!LIN_TO[i])
      {
        if(config.consumer[i].dev && config.consumer[i].dev->modem)
        {
          char m[] = "Timeout linea 0";
          m[14] = '0' + i;
          support_log(m);
          if(config.consumer[i].dev->modem == MODEM_GSM)
            GSM_Call_terminate(1);
          else
            TMD_Call_terminate();
        }
        else if(config.consumer[i].dev && config.consumer[i].dev->type == protETH)
        {
          t = config.consumer[i].dev->fd;
          config.consumer[i].dev->fd = -1;
          shutdown(t, SHUT_RD);
          close(t);
        }
        LIN[i] = 0;
      }
    }
  }
  
  /* 0.1s */

#ifdef __CRIS__
  /* check for test button press -> Anubi power on */
  
  fd = open(ADDROOTDIR(DevGpioA), O_RDONLY);
  if(fd >= 0)
  {
    i = ioctl(fd, _IO(ETRAXGPIO_IOCTYPE, IO_READBITS), NULL);
    close(fd);
    if((i >= 0) && !(i & 0x02))
    {
      delphi_restart_count++;
      if(delphi_restart_count > 5)
      {
        support_log("Restart button");
        delphi_restart();
      }
    }
    else
      delphi_restart_count = 0;
  }
#endif
  if(RestartProgram)
  {
    RestartProgram--;
    if(!RestartProgram)
    {
      struct timeval tv;
      while(TMD_Status[1] || GSM_Status[1])
      {
        tv.tv_sec = 0;
        tv.tv_usec = 100000;		/* 0.1s */
        select(0, NULL, NULL, NULL, &tv);
        timeout_check();
      }
      /* If modem is present reset baudrate to initial value before exiting to
         permit probing at restart */
      if(TMD_Status[0] != 99)
      {
        i = support_find_serial_consumer(PROT_MODEM, 0);
        modem_setspeed(config.consumer[i].data.serial.baud1);
      }
      /* GSM has autobauding capability, so resetting the baudrate is not necessary */
      exit(0);
    }
    return;
  }
  
  timeout_check();
  
  for(i=0; i<n_TM; i++)
  {
    if(TTM[i])
    {
      TTM[i]--;
      if(!TTM[i])
      {
        TM[i] &= ~(bitTiming | bitON);
        TM[i] |= bitHLF;
      }
    }
  }
  
  database_lock();
  for(i=1; i<(n_ZS+1); i++)
  {
    if(ZONA_R_timer[i])
    {
      ZONA_R_timer[i]--;
      if(!ZONA_R_timer[i])
      {
#if 0
        if(cmd_zone_on(i, ZONA_R_mode[i] | 0x80))	// forza l'attivazione
          cmd_zone_refresh(1);
        else
        {
          // attivazione impedita: si attiva con allarme
          cmd_zone_on(i, 0x80 | 1);
          cmd_zone_refresh(1);
        }
#else
        cmd_zone_on(i, ZONA_R_mode[i] | 0x80);
        cmd_zone_refresh(1);
#endif
        cmd_actuator_refresh();
      }
    }
  }
  database_unlock();
  
  N_MINUTO = 0;
  N_ORA = 0;
  N_GIORNO = 0;
  N_MESE = 0;
  N_ANNO = 0;
  
//  if(timer_hook) timer_hook();
  
  tick++;
  if(tick < 10)
  {
    /* refresh zones and actuate */  
    master_do();
    return;
  }
  tick = 0;
  
  /* 1s */

  if(!unlink(ADDROOTDIR(RestartFileName)))
  {
    support_log("Restart FTP");
#if defined __CRIS__ || defined __arm__
    delphi_restart();
#endif
  }
  
  for(i=0; i<n_CO; i++)
  {
    if(TCO[i] & 0xfff)
    {
      TCO[i]--;
      if(!(TCO[i] & 0xfff))
      {
        CO[i] &= ~bitTiming;
        TCO[i] = 0;
      }
    }
  }

  lara_timer();
  gsm_line_status();

  if(stopclock)
  {
    stopclock--;
    if(stopclock)
    {
      /* refresh zones and actuate */  
      master_do();
      return;
    }
    else
    {
#ifdef __CRIS__
      struct rtc_time rt;
      time_t ltime;
      struct tm *datehour;
      
      if(SECONDI <= 30) SECONDI = 31;
      
      time(&ltime);
      datehour = localtime(&ltime);
      
      fd = open("/dev/rtc", O_RDONLY);
      if(fd)
      {
        i = ioctl(fd, RTC_RD_TIME, &rt);
        if(i < 0) ioctl(fd, RTC_RD_TIME_V1, &rt);
        rt.tm_sec = SECONDI;
        i = ioctl(fd, RTC_SET_TIME, &rt);
        if(i < 0) ioctl(fd, RTC_SET_TIME_V1, &rt);
        close(fd);
      }
#endif
      
      datehour->tm_sec = SECONDI;
      ltime = mktime(datehour);
      stime(&ltime);
    }
  }
  
#ifdef __CRIS__
  if((ORE == 0) && (MINUTI == 0) && (SECONDI == 30))
  {
    FILE *fp;
    
#ifdef TEBESA
    stopclock = 0;	// default: 0 s
#else
    if(BoardAnubi)
      stopclock = 5;	// default: 5 s
    else
      stopclock = 0;	// default: 0 s
#endif
    fp = fopen(ADDROOTDIR(StopClockFile), "r");
    if(fp)
    {
      fscanf(fp, "%d", &stopclock);
      fclose(fp);
    }
    
    if(stopclock) support_log("Compensazione RTC");
  }
#endif

#if 1
#ifdef __CRIS__
  /* Aggiornamento orologio di sistema da RTC ogni ora */
  if(/*(ORE == 0) &&*/ (MINUTI == 0) && (SECONDI == 30))
  {
    struct rtc_time rt;
    time_t ltime;
    struct tm *datehour;
    
    fd = open("/dev/rtc", O_RDONLY);
    if(fd < 0)
      SECONDI++;
    else
    {
      time(&ltime);
      datehour = localtime(&ltime);
    
      i = ioctl(fd, RTC_RD_TIME, &rt);
      if(i < 0) ioctl(fd, RTC_RD_TIME_V1, &rt);
      SECONDI = rt.tm_sec;
      close(fd);
      
      datehour->tm_sec = SECONDI;
      ltime = mktime(datehour);
      stime(&ltime);
    }
  }
  else
#endif
#endif

  SECONDI++;
  if(SECONDI < 60)
  {
    /* refresh zones and actuate */  
    master_do();
    return;
  }
  SECONDI = 0;
  
  /* 1 min */
      
  time(&ltime);
  datehour = localtime(&ltime);

  MINUTI = datehour->tm_min;
  ORE = datehour->tm_hour;
  SECONDI = datehour->tm_sec;
  GIORNO_S = datehour->tm_wday + 1;
  GIORNO = datehour->tm_mday;
  MESE = datehour->tm_mon + 1;
  ANNO = datehour->tm_year % 100;

  N_MINUTO |= bitAlarm;

  if(ORE != old_ORE)
  {
    N_ORA |= bitAlarm;
    old_ORE = ORE;
    
    if(GIORNO != old_GIORNO)
    {
      N_GIORNO |= bitAlarm;
      old_GIORNO = GIORNO;
      
      TIPO_GIORNO = FESTIVI[0];

      for(i=1; i<n_FESTIVI; i++)
        FESTIVI[i-1] = FESTIVI[i];
      FESTIVI[i-1] = cmd_get_day_type((GIORNO_S + n_FESTIVI - 1) % 7);
      delphi_update_days();
      delphi_check_periods();
      
      if(MESE != old_MESE)
      {
        N_MESE |= bitAlarm;
        old_MESE = MESE;
        
        if(ANNO != old_ANNO)
        {
          N_ANNO |= bitAlarm;
          old_ANNO = ANNO;
        }
      }
    }
  }
  
  delphi_check_timerange();
  
  /* refresh zones and actuate */  
  master_do();
}

void delphi_set_timer(int sec)
{
  if(sec)
    SECONDI = sec - 1;
  else
    SECONDI = 59;
}

static int delphi_baud(int baud)
{
  switch(baud)
  {
    case 600: return B600;
    case 1200: return B1200;
    case 2400: return B2400;
    case 4800: return B4800;
    case 9600: return B9600;
    case 19200: return B19200;
    case 38400: return B38400;
    case 57600: return B57600;
    case 115200: return B115200;
    default: return 0;
  }
}

void delphi_save_conf()
{
  FILE *fp;
  int i;

  fp=fopen(ADDROOTDIR(ConfFileName), "w");
  if(fp)
  {
    fprintf(fp, "DeviceID=%d\n", config.DeviceID);
    if(delphi_modo == DELPHITIPO)
      fprintf(fp, "NodeID=0\n");
    else
      fprintf(fp, "NodeID=%d\n", config.NodeID);
    fprintf(fp, "\n");
    fprintf(fp, "Rings=%d\n", config.Rings);
    if(config.Retries) fprintf(fp, "Retries=%d\n", config.Retries);
    if(config.Interval) fprintf(fp, "Interval=%d\n", config.Interval/10);
    fprintf(fp, "\n");
    fprintf(fp, "PhonePrefix=%s\n", config.PhonePrefix);
    for(i=0; i<PBOOK_DIM; i++)
    {
      if(config.PhoneBook[i].Phone[0])
      {
      fprintf(fp, "PhoneNum%d=%s;", i+1, config.PhoneBook[i].Phone);
      if(config.PhoneBook[i].Name) fprintf(fp, "%s", config.PhoneBook[i].Name);
      fprintf(fp, "%s", (config.PhoneBook[i].Abil & PB_RX)?";1":";0");
      fprintf(fp, "%s", (config.PhoneBook[i].Abil & PB_TX)?";1":";0");
      fprintf(fp, "%s", (config.PhoneBook[i].Abil & PB_SMS_RX)?";1":";0");
      fprintf(fp, "%s", (config.PhoneBook[i].Abil & PB_SMS_TX)?";1":";0");
      fprintf(fp, "%s", (config.PhoneBook[i].Abil & PB_GSM_RX)?";1":";0");
      fprintf(fp, "%s", (config.PhoneBook[i].Abil & PB_GSM_TX)?";1":";0");
      fprintf(fp, "\n");
      }
    }
    fprintf(fp, "\n");
    if(!config.Log) fprintf(fp, "Log=%d\n", config.Log);
    if(config.LogLimit != 102400) fprintf(fp, "LogLimit=%d\n", config.LogLimit);
    if(config.Variant) fprintf(fp, "Variant=%d\n", config.Variant);
    fclose(fp);
    sync();
  }
  
  fp=fopen(ADDROOTDIR(ref_ConfFileName), "r");
  if(fp)
    fclose(fp);
  else
  {
    /* Se il file saet.conf non esiste in saet.ref, copia quello appena creato. */
    char cmd[128];
    sprintf(cmd, "cp %s %s", ADDROOTDIR(ConfFileName), ADDROOTDIR(ref_ConfFileName));
    system(cmd);
  }
}

static void delphi_read_conf()
{
  FILE *fp;
  char buf[64], *val, *tmp;
  int i;

  fp=fopen(ADDROOTDIR(ConfFileName), "r");
  if(fp)
  {
    while(fgets(buf, 64, fp))
    {
      if(!strncmp(buf, "NodeID=", 7)) if(sscanf(buf+7, "%d", &i)) config.NodeID = i;
      if(!strncmp(buf, "DeviceID=", 9)) if(sscanf(buf+9, "%d", &i)) config.DeviceID = i;
      if(!strncmp(buf, "Rings=", 6)) if(sscanf(buf+6, "%d", &i)) config.Rings = i;
      if(!strncmp(buf, "Retries=", 8))
        if(sscanf(buf+8, "%d", &i)) config.Retries = i;
      if(!strncmp(buf, "Interval=", 9))
        if(sscanf(buf+9, "%d", &i)) config.Interval = i * 10;
      if(!strncmp(buf, "PhoneNum", 8))
      {
        if(sscanf(buf+8, "%d", &i))
        {
          val = strtok(buf + 10 + (i>9?1:0), "\r\n");
          if(val)
          {
            tmp = support_delim(val);
            if(tmp) strcpy(config.PhoneBook[i-1].Phone, tmp);
            free(config.PhoneBook[i-1].Name);
            tmp = support_delim(NULL);
            if(tmp) config.PhoneBook[i-1].Name = strdup(tmp);
            tmp = support_delim(NULL);
            if(tmp && tmp[0] == '1') config.PhoneBook[i-1].Abil |= PB_RX;
            tmp = support_delim(NULL);
            if(tmp && tmp[0] == '1') config.PhoneBook[i-1].Abil |= PB_TX;
            tmp = support_delim(NULL);
            if(tmp && tmp[0] == '1') config.PhoneBook[i-1].Abil |= PB_SMS_RX;
            tmp = support_delim(NULL);
            if(tmp && tmp[0] == '1') config.PhoneBook[i-1].Abil |= PB_SMS_TX;
            tmp = support_delim(NULL);
            if(tmp && tmp[0] == '1') config.PhoneBook[i-1].Abil |= PB_GSM_RX;
            tmp = support_delim(NULL);
            if(tmp && tmp[0] == '1') config.PhoneBook[i-1].Abil |= PB_GSM_TX;
          }
        }
      }
      if(!strncmp(buf, "PhonePrefix=", 12)) sscanf(buf+12, "%7s", config.PhonePrefix);
      if(!strncmp(buf, "Log=", 4))            
        if(sscanf(buf+4, "%d", &i)) config.Log = i;
      if(!strncmp(buf, "LogLimit=", 9))            
        if(sscanf(buf+9, "%d", &i)) config.LogLimit = i;
      if(!strncmp(buf, "Variant=", 8))            
        if(sscanf(buf+8, "%d", &i)) config.Variant = i;
      if(!strncmp(buf, "Debug=", 6))            
        if(sscanf(buf+6, "%d", &i)) config.debug = i;
    }
    fclose(fp);

    if(!config.PhonePrefix[0]) strcpy(config.PhonePrefix, "+39");
  }
  
  fp=fopen(ADDROOTDIR(ConfFileName2), "r");
  if(fp)
  {
    while(fgets(buf, 64, fp))
    {
      if(!strncmp(buf, "VoltoServer=", 12)) config.VoltoServerName = strdup(buf+12);
      if(!strncmp(buf, "VoltoMaxConn=", 13))
        if(sscanf(buf+13, "%d", &i)) config.VoltoMaxConn = i;
      if(!strncmp(buf, "VoltoMinConn=", 13))
        if(sscanf(buf+13, "%d", &i)) config.VoltoMinConn = i;
      if(!strncmp(buf, "VoltoServerUpdate=", 18))
        if(sscanf(buf+18, "%d", &i)) config.VoltoServerUpdate = i;
      if(!strncmp(buf, "TebeBadgeDisable=", 17))
        if(sscanf(buf+17, "%d", &i)) config.TebeBadgeDisableOnErr = i;
    }
    fclose(fp);
  }
}

static void delphi_read_consumer()
{
  FILE *fp;
  char buf[64], *val;
  int i;
  
  fp=fopen(ADDROOTDIR(ConsumerFileName), "r");
  if(fp)
  {
    while(fgets(buf, 64, fp))
    {
      if(buf[0] == '#') continue;
      
      val = support_delim(buf);
      sscanf(val, "%d", &i);
      val = support_delim(NULL);
      if(val && (i < MAX_NUM_CONSUMER))
      {
        if(!strncmp(val, "ETH", 3))
        {
          config.consumer[i].configured = 5;
          val = support_delim(NULL);
          if(val) sscanf(val, "%d", &config.consumer[i].data.eth.port);
          if(config.consumer[i].data.eth.port)
          {
            /* old style: x;ETH;port */
            config.consumer[i].type = PROT_SAETNET_OLD;
          }
          else
          {
            /* new style: x;ETH;prot;port */
            if(val) config.consumer[i].type = prot_type(val);
            val = support_delim(NULL);
            if(val) sscanf(val, "%d", &config.consumer[i].data.eth.port);
          }
        }
        else
        {
#ifndef TEBESA
          if(!strncmp(val, "COM", 3))
          {
            config.consumer[i].configured = val[3] - '0';
#ifdef __arm__
            /* Per la scheda Athena la COM4 non esiste. */
            if(config.consumer[i].configured == 4)
            {
              config.consumer[i].configured = 0;
              continue;
            }
#endif
          }
          else
#endif
          if(!strncmp(val, "PTY", 3))
            config.consumer[i].configured = val[3] - '0' + 6;
          else if(!strncmp(val, "USB", 3))
            config.consumer[i].configured = val[3] - '0' + 12;
          val = support_delim(NULL);
          if(val) config.consumer[i].type = prot_type(val);
          val = support_delim(NULL);
          if(val) sscanf(val, "%d", &config.consumer[i].data.serial.baud1);
          config.consumer[i].data.serial.baud1 = delphi_baud(config.consumer[i].data.serial.baud1);
          val = support_delim(NULL);
          if(val) sscanf(val, "%d", &config.consumer[i].data.serial.baud2);
          config.consumer[i].data.serial.baud2 = delphi_baud(config.consumer[i].data.serial.baud2);
        }
        val = support_delim(NULL);
        if(val) config.consumer[i].param = strdup(val);
        config.consumer[i].progr = 1;
      }
    }
    fclose(fp);
  }
}

static void delphi_closeall()
{
  int i;
  ProtDevice *dev;
  
  support_log("Exit");
  support_log2("Exit");
  support_logmm("Exit");
  
  /* Salvo eventuali pendenze Tebe */
  lara_save(2);
  
  if(master_behaviour != MASTER_NORMAL)
  {
    signal(SIGALRM, SIG_IGN);
    
    cluster_elimina_if();
    
    for(i=0; i<MAX_NUM_CONSUMER; i++)
    {
      dev = config.consumer[i].dev;
      if(dev)
      {
        if(dev->th_plugin) pthread_cancel(dev->th_plugin);
        if(dev->th_eth) pthread_cancel(dev->th_eth);
        prot_close(dev);
      }
    }
    tebe_stop();
    master_stop();
    while(1) sleep(1);
  }
}

static void delphi_exit(int sig)
{
  static int exiting = 0;
  char str[24];
  int i, pid, cont;
  struct timeval tv;
  
  if(!exiting)
  {
    exiting = 1;
    
    pid = support_getpid();
    for(i=0; (i<DEBUG_PID_MAX)&&(debug_pid[i]!=pid); i++);
    sprintf(str, "Exit (%d [%d])", sig, pid);
    switch(i)
    {
      case DEBUG_PID_MAIN: strcat(str, " [Main]"); break;
      case DEBUG_PID_MSEND: strcat(str, " [Master send]"); break;
      case DEBUG_PID_MRECV: strcat(str, " [Master recv]"); break;
      case DEBUG_PID_MLOOP: sprintf(str+strlen(str), " [Master loop (%d)]", debug_mloop_phase); break;
      case DEBUG_PID_TEBEU: strcat(str, " [Tebe UDP]"); break;
      case DEBUG_PID_VOLTO: strcat(str, " [Volto]"); break;
      default:
        if(i<8) sprintf(str+strlen(str), " [Consumer %d]", i);
        break;
    }
    support_log(str);
    support_log2(str);
    support_logmm(str);
    
#if defined __CRIS__ || defined __arm__
    if((GSM_Status[1] > 0) && (GSM_Status[1] <= 3))
      GSM_Call_terminate(0);
    else if(GSM_Status[1] > 3)
      GSM_Call_terminate(1);
    TMD_Call_terminate();
    cont = 0;
    while(TMD_Status[1] || GSM_Status[1])
    {
      tv.tv_sec = 0;
      tv.tv_usec = 100000;		/* 0.1s */
      select(0, NULL, NULL, NULL, &tv);
      timeout_check();
      
      /* Timeout 10 secondi, poi esce forzatamente */
      cont++;
      if(cont > 100) break;
    }
    /* If modem is present reset baudrate to initial value before exiting to
       permit probing at restart */
    if(TMD_Status[0] != 99)
    {
      i = support_find_serial_consumer(PROT_MODEM, 0);
      modem_setspeed(config.consumer[i].data.serial.baud1);
    }
    /* GSM has autobauding capability, so resetting the baudrate is not necessary */
#endif
  
    /* for a clean exit */
    delphi_closeall();
    
    abort();
  }
}

static void delphi_null(int sig)
{
  /* Serve a sbloccare le read(), in particolare sulle connessioni modem */
}

static void delphi_set_time()
{
  time_t ltime;
  struct tm *datehour;
  
  /**** Set time ****/
  time(&ltime);
  datehour = localtime(&ltime);

  SECONDI = datehour->tm_sec;
  MINUTI = datehour->tm_min;
  ORE = datehour->tm_hour;
  GIORNO_S = datehour->tm_wday + 1;
  GIORNO = datehour->tm_mday;
  MESE = datehour->tm_mon + 1;
  ANNO = datehour->tm_year % 100;

  old_ORE = ORE;
  old_GIORNO = GIORNO;
  old_MESE = MESE;
  old_ANNO = ANNO;
}

void delphi_init()
{
  struct stat st;
  
  /**** Check if Anubi or Iside ****/
  if(!stat("/etc/board", &st)) BoardAnubi = 0;
  
  /**** Set time ****/
  delphi_set_time();
  
  /**** Set signals ****/
  signal(SIGHUP, delphi_exit);
  signal(SIGINT, delphi_exit);
  signal(SIGQUIT, delphi_exit);
  signal(SIGKILL, delphi_exit);
  signal(SIGSEGV, delphi_exit);
  signal(SIGTERM, delphi_exit);
  
  signal(SIGUSR2, delphi_null);

  signal(SIGPIPE, SIG_IGN);
  
  signal(SIGILL, delphi_exit);
  signal(SIGABRT, delphi_exit);
  signal(SIGFPE, delphi_exit);
  signal(SIGBUS, delphi_exit);
  
  /**** Init configuration parameters ****/
  
  memset(&config, 0, sizeof(Config_t));
  config.DeviceID = 1;
  config.Log = 1;
  config.LogLimit = 102400;
  config.debug = -1;
}

static void delphi_load_ref()
{
  char cmd[128];
  
  unlink(ADDROOTDIR(ConsumerFileName));
  unlink(ADDROOTDIR(ConfFileName));
  unlink(ADDROOTDIR(NVFileName));
  unlink(ADDROOTDIR(StringsFileName));
  
  sprintf(cmd, "cp %s %s", ADDROOTDIR(ref_ConsumerFileName), ADDROOTDIR(ConsumerFileName));
  system(cmd);
  sprintf(cmd, "cp %s %s", ADDROOTDIR(ref_ConfFileName), ADDROOTDIR(ConfFileName));
  system(cmd);
  sprintf(cmd, "cp %s %s", ADDROOTDIR(ref_NVFileName), ADDROOTDIR(NVFileName));
  system(cmd);
  sprintf(cmd, "cp %s %s", ADDROOTDIR(ref_StringsFileName), ADDROOTDIR(StringsFileName));
  system(cmd);
  sprintf(cmd, "[ -f %s ] && cp %s %s; rm %s", ADDROOTDIR(NewUserFileName), ADDROOTDIR(NewUserFileName), ADDROOTDIR(UserFileName), ADDROOTDIR(NewUserFileNameDelete));
  system(cmd);
  sync();
}

void delphi_load_lang()
{
  DIR *dir;
  struct dirent *lang;
  char buf[64];
  
  dir = opendir(ADDROOTDIR("/saet/data"));
  if(dir)
  {
    while((lang = readdir(dir)))
    {
      if(!strncmp(lang->d_name, "lang", 4))
      {
        sprintf(buf, "/saet/data/%s", lang->d_name);
        delphi_lang = dlopen(ADDROOTDIR(buf), RTLD_LAZY);
        break;
      }
    }
  }
}

void delphi_set_lang(char *app, int num, char **str[])
{
  void (*setlang)(int, char***);
  
  if(delphi_lang)
  {
    setlang = dlsym(delphi_lang, app);
    if(setlang) (*setlang)(num, str);
  }
}

static void delphi_alarm(int sig)
{
  sem_post(&delphi_sem);
  sem_getvalue(&delphi_sem, &sig);
  if(sig > 50)
  {
    support_log("Main loop bloccato!");
    exit(0);
  }
}

extern unsigned short cmd_memory_control_cks2;

int main(int argc, char *argv[])
{
  int ret, fd, consumer;
  int modem_present;
  char log[256];
  
#if defined __i386__ || defined __x86_64__
  getcwd(RootDir, 256);
  strcat(RootDir, "/");
  if(argv[0][0] != '/') strcat(RootDir, argv[0]);
  for(ret=strlen(RootDir)-1; RootDir[ret]!='/'; ret--);
  strcpy(RootDir+ret, "/..");
  printf("Base: %s\n", RootDir);
#endif
  
  if((argc > 1) && !strcmp(argv[1], "-v"))
  {
    char *err;
    
    /* I plugin potrebbero impostare il Variant, quindi è
       necessario che la configurazione caricata sia valida. */
    delphi_init();
    delphi_read_conf();
    
    printf("%s\n", version_get());
    prot_plugin_load();
    delphi_load_lang();
    dlopen(ADDROOTDIR(UserFileName), RTLD_NOW);
    err = dlerror();
    if(err)
      printf("%s\n", err);
    else
    {
      cmd_memory_control_init();
      printf("Libuser checksum: %04x\n\n", cmd_memory_control_cks2);
    }
    printf("\n");
    cmd_memory_control_init();
    
    return 0;
  }
  
  cluster_init();
    
  /* Nota: la chiamata system() brucia due PID all'avvio.
     Per questo motivo con ps si trova il buco. */
  /* Uno lo brucia per l'esecuzione del comando,
     l'altro invece non so perché. Comunque non per la
     scrittura sul file version (provato). */
  sprintf(log, "%s -v > %s", argv[0], ADDROOTDIR(VersionFile));
  system(log);
  
  memset(debug_pid, 0, DEBUG_PID_MAX);
  debug_pid[DEBUG_PID_MAIN] = support_getpid();
  
  atexit(delphi_closeall);
  
  if(!mkdir(ADDROOTDIR(DATA_DIR), 0777))
  {
    rename(ADDROOTDIR("/saet/status.nv"), ADDROOTDIR(StatusFileName));
    rename(ADDROOTDIR("/saet/lara.gz"), ADDROOTDIR(LaraFile));
  }
  
  support_save_last_log();
  
  delphi_init();
  
  if((argc > 2) && !strcmp(argv[1], "-p"))
  {
    config.debug = atoi(argv[2]);
    printf("Debug di protocollo attivo su consumatore %d\n", config.debug);
  }
    
  ret = unlink(ADDROOTDIR(RestartFileName));
  if(!ret)
  {
#ifdef __CRIS__
    /* check for test button press -> Anubi power on */
  
    fd = open(ADDROOTDIR(DevGpioA), O_RDONLY);
    if(fd >= 0)
    {
      while(!(ioctl(fd, _IO(ETRAXGPIO_IOCTYPE, IO_READBITS), NULL) & 0x02)) usleep(100000);
      close(fd);
    }
#endif

    Restart = 1;
    support_reset_status();
    nvmem_reset();
    delphi_load_ref();
//    lara_reset_periph();
   
    UserSystemStatus = 1;
    
    sprintf(log, "Delphi Restart [%d]", support_getpid());
    support_log(log);
  }
  else
  {
    UserSystemStatus = 2;
    sprintf(log, "Delphi Reset [%d]", support_getpid());
    support_log(log);
  }
  
  /**** First init the event buffer ****/
  codec_init();
  
  /**** Load first configurations ****/
  delphi_read_conf();
  
  delphi_load_lang();
  delphi_set_lang("DELPHI", sizeof(delphi_str)/sizeof(char*), delphi_str);
  
  string_init();
  string_load();
  
  /**** Load NV database ****/
  database_load();

  prot_plugin_load();
  delphi_read_consumer();
  
  /**** Carica la configurazione dei codici eventualmente salvata ****/
  delphi_load_codes();
  
  /* check for modem */
  modem_present = 0;
  consumer = support_find_serial_consumer(PROT_MODEM, 0);
  if(consumer >= 0)
  {
#ifdef __arm__
#define MODEM_IOCTYPE 45
#define IO_MODEM_ONOFF      _IO(MODEM_IOCTYPE, 0x10) /*  */
#define IO_MODEM_SHUTDOWN   _IO(MODEM_IOCTYPE, 0x11) /*  */
    /* Accensione modem */
    fd = open("/dev/gpio", O_RDWR);
    ioctl(fd, IO_MODEM_SHUTDOWN, 0);
    ioctl(fd, IO_MODEM_ONOFF, 0);
    close(fd);
#endif
  
    modem_serial[9] = '0' + config.consumer[consumer].configured - 1;
    modem_present = prot_test_modem(modem_serial);
  }
  
  if(modem_present == MODEM_CLASSIC)
    modem_init();
  else
    TMD_Status[0] = 99;
  
  /* GSM probe delayed */
  GSM_Status[0] = 99;
  gsm_init(MODEM_GSM);
  
  prot_start();
//  support_load_status();	// spostato in master_init()
  master_init();
  
  /**** Refresh internal time ****/
  delphi_set_time();

#if 0
  while(1)
  {
    struct timeval tv;
    
    tv.tv_sec = 0;
    tv.tv_usec = 100000;		/* 0.1s */
    select(0, NULL, NULL, NULL, &tv);
    delphi_timer();
  }
#else
  {
  struct itimerval tv;
  
  sem_init(&delphi_sem, 0, 0);
  signal(SIGALRM, delphi_alarm);
  
  tv.it_value.tv_sec = 0;
  tv.it_value.tv_usec = 100000;		/* 0.1s */
  tv.it_interval.tv_sec = 0;
  tv.it_interval.tv_usec = 100000;		/* 0.1s */
  setitimer(ITIMER_REAL, &tv, NULL);
  
  while(1)
  {
    sem_wait(&delphi_sem);
    delphi_timer();
  }
  
  }
#endif

  return 0;
}

void delphi_restart_network()
{
#if defined __i386__ || defined __x86_64__
  return;
#else
  system("cp /tmp/network.conf /etc/network/network.conf");
  unlink("/tmp/network.conf");
      
  if(support_get_board() == BOARD_ANUBI)
  {
    system("route del default");
    system("/usr/init.d/network");
  }
  else if(support_get_board() == BOARD_ATHENA)
  {
    system("SYSCFG_IFACE0=y INTERFACE0=eth0 /etc/rc.d/init.d/network restart");
  }
  else
    system("/etc/init.d/net.eth0 start");
  
  if((GSM_Status[1] > 0) && (GSM_Status[1] <= 3))
  {
    GSM_Call_terminate(0);
  }
  else if(GSM_Status[1] > 3)
  {
    GSM_Call_terminate(1);
  }
  if((TMD_Status[1] > 0) && (TMD_Status[1] <= 3))
  {
    TMD_Call_terminate();
  }
  else if(TMD_Status[1] > 3)
  {
    TMD_Call_terminate();
  }
  RestartProgram = 10;
#endif
}

