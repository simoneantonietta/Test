#include "command.h"
#include "database.h"
#include "codec.h"
#include "delphi.h"
#include "support.h"
#include "master.h"
#include "lara.h"
#include "tebe.h"
#include "version.h"
#include "timeout.h"
#include <time.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef __CRIS__
#include <asm/rtc.h>
#endif
#ifdef __arm__
#include <linux/rtc.h>
#endif

int cmd_comportamento_SYS = 0;

//#define ATTIVAZIONE_PARZIALE_ZONE

unsigned short cmd_memory_control_cks1 = 0;
unsigned short cmd_memory_control_cks2 = 0;

int cmd_set_saturday_type(unsigned char type)
{
  int i;
  
  SatHoliday = type;
  database_changed = 1;
  
  TIPO_GIORNO = cmd_get_day_type(GIORNO_S - 1);
  for(i=0; i<n_FESTIVI; i++) FESTIVI[i] = cmd_get_day_type((GIORNO_S + i)%7);
  delphi_update_days();
  
  return 0;
}

int cmd_get_day_type(int day)
{
  switch(day)
  {
    case 0:
      return DAY_HOLIDAY;
    case 6:
      return SatHoliday;
    default:
      return DAY_WORKDAY;
  }
}

int cmd_set_day_type(int day, unsigned char type)
{
  unsigned char ev[4];
  
  if((config.Variant == 1) && (FESTIVI[day] == DAY_HOLIDAY)) return 0;
  
  FESTIVI[day] = type;
  
  ev[0] = Variato_stato_Festivo;
  ev[1] = day+1;
  ev[2] = type;
  
  codec_queue_event(ev);
  
  return 1;
}

int cmd_set_date(unsigned char *val)
{
  time_t ltime;
  struct tm *datehour;
  int i;
#if defined __CRIS__ || defined __arm__
  int fd, res;
  struct rtc_time rt;

  fd = open("/dev/rtc", O_RDONLY);
  if(fd < 0) return -1;
  
  rt.tm_year = 0;
  rt.tm_mon = 0;
  rt.tm_mday = 0;
  rt.tm_hour = 0;
  rt.tm_min = 0;
  rt.tm_sec = 0;
#endif
  
  time(&ltime);
  datehour = localtime(&ltime);
  
  if((datehour->tm_mday != GVAL2(val+1)) ||
     (datehour->tm_mon != GVAL2(val+3)-1) ||
     (datehour->tm_year != GVAL2(val+5)+100))
  {
  
  datehour->tm_mday = GVAL2(val+1);
  datehour->tm_mon = GVAL2(val+3)-1;
  datehour->tm_year = GVAL2(val+5)+100;
  ltime = mktime(datehour);
  stime(&ltime);
  
  /* ignora il giorno della settimana impostato, viene ricavato dalla data corrente */
  datehour = localtime(&ltime);
  
  GIORNO_S = datehour->tm_wday + 1;
/*
  GIORNO = datehour->tm_mday;
  if(GIORNO != old_GIORNO)
    N_GIORNO |= bitAlarm;
  old_GIORNO = GIORNO;
*/
  N_GIORNO |= bitAlarm;
  GIORNO = old_GIORNO = datehour->tm_mday;
  MESE = old_MESE = datehour->tm_mon + 1;
  ANNO = old_ANNO = datehour->tm_year % 100;
  
  delphi_set_timer(datehour->tm_sec);
  TIPO_GIORNO = cmd_get_day_type(datehour->tm_wday);
  for(i=0; i<n_FESTIVI; i++) FESTIVI[i] = cmd_get_day_type((datehour->tm_wday + i + 1)%7);
  delphi_update_days();
  delphi_check_periods();
  
  }
  
  /* L'RTC lo aggiorno sempre */
#if defined __CRIS__ || defined __arm__
  datehour = gmtime(&ltime);

  res = ioctl(fd, RTC_RD_TIME, &rt);
#ifdef __CRIS__
  if(res < 0) ioctl(fd, RTC_RD_TIME_V1, &rt);
#endif
  
  rt.tm_year = datehour->tm_year;
  rt.tm_mon = datehour->tm_mon;
  rt.tm_mday = datehour->tm_mday;

  res = ioctl(fd, RTC_SET_TIME, &rt);
#ifdef __CRIS__
  if(res < 0) ioctl(fd, RTC_SET_TIME_V1, &rt);
#endif
  
  close(fd);

  return res;
#else
  return 0;
#endif
}

int cmd_get_time()
{
  unsigned char ev[4];
  time_t ltime;
  struct tm *datehour;
  
  time(&ltime);
  datehour = localtime(&ltime);

  ev[0] = Variazione_ora;
  ev[1] = datehour->tm_hour;
  ev[2] = datehour->tm_min;
  
  codec_queue_event(ev);
  
  return 0;
}

int cmd_set_time(unsigned char *val)
{
  unsigned char ev[4];
  time_t ltime;
  struct tm datehour;
#if defined __CRIS__ || defined __arm__
  int fd, res;
  struct rtc_time rt;

  fd = open("/dev/rtc", O_RDONLY);
  if(fd < 0) return -1;
  
  rt.tm_year = 0;
  rt.tm_mon = 0;
  rt.tm_mday = 0;
  rt.tm_hour = 0;
  rt.tm_min = 0;
  rt.tm_sec = 0;
#endif
  
  ev[0] = Variazione_ora;
  ev[1] = GVAL2(val);
  ev[2] = GVAL2(val+2);
  codec_queue_event(ev);
  
  time(&ltime);
  localtime_r(&ltime, &datehour);
  datehour.tm_hour = GVAL2(val);
  datehour.tm_min = GVAL2(val+2);
  if(val[4] != 0x0d)
    datehour.tm_sec = GVAL2(val+4);
  else
    datehour.tm_sec = 0;
  ltime = mktime(&datehour);
  stime(&ltime);

  MINUTI = datehour.tm_min;
  ORE = old_ORE = datehour.tm_hour;
  delphi_set_timer(datehour.tm_sec);
  
#if defined __CRIS__ || defined __arm__
  gmtime_r(&ltime, &datehour);

  res = ioctl(fd, RTC_RD_TIME, &rt);
#ifdef __CRIS__
  if(res < 0) ioctl(fd, RTC_RD_TIME_V1, &rt);
#endif
  
  rt.tm_hour = datehour.tm_hour;
  rt.tm_min = datehour.tm_min;
  rt.tm_sec = datehour.tm_sec;

  res = ioctl(fd, RTC_SET_TIME, &rt);
#ifdef __CRIS__
  if(res < 0) ioctl(fd, RTC_SET_TIME_V1, &rt);
#endif
  
  close(fd);
  
  delphi_check_timerange();
  
  return res;
#else
  return 0;
#endif
}

int cmd_set_timerange(int idx, unsigned char *val)
{
  unsigned char ev[8];
  int i;
  
  i = GVAL(val) << 1;
  
  if(i < 3)
  {
    FAGR[idx][0 + i] = GVAL2(val + 1);
    FAGR[idx][1 + i] = GVAL2(val + 3);
  }
  else
  {
    i -= 4;
    FAGR[idx][0 + i] = 255;
    FAGR[idx][1 + i] = 255;
  }
  
  database_changed = 1;
  
  ev[0] = Variata_fascia_oraria;
  ev[1] = idx;
  ev[2] = i >> 1;
  ev[3] = FAGR[idx][0 + i];
  ev[4] = FAGR[idx][1 + i];
  
  codec_queue_event(ev);
  
  return 0;
}

int cmd_set_holydays(unsigned char *val)
{
  memcpy(FESTIVI + (GVAL(val) << 4), val + 1, 16);
  
  if(config.Variant == 1)
  {
    int i, f;
    
    /* Le domeniche sono SEMPRE festive, non si può cambiare. */
    f = GVAL(val) << 4;
    for(i=0; i<16; i++)
    {
      /* GIORNO_S va da 1 a 7 (1=domenica) */
      if(!((GIORNO_S+f)%7)) FESTIVI[f] = DAY_HOLIDAY;
      f++;
    }
    
    /* Anche i giorni fissi non si possono cambiare. */
    delphi_update_days();
  }

  return 0;
}

static void cmd_oop_prepare(Event *ev)
{
  ev->MsgType = STX;

  ev->DeviceID[0] = config.DeviceID >> 8;
  ev->DeviceID[1] = config.DeviceID & 0xff;
  ev->NodeID = config.NodeID;
  ev->Status = 0;

  memset(ev->Event, 0, 32);

  ev->Event[2] = GIORNO;
  ev->Event[3] = MESE;
  ev->Event[4] = ANNO;
  ev->Event[5] = ORE;
  ev->Event[6] = MINUTI | 0xC0;
  ev->Event[7] = SECONDI;
  ev->Event[8] = InvioMemoria;
}

#warning ------------------------------------
#warning Con la ridondanza crea problemi.
#warning Spuntare il check su com90 nel caso.
#warning ------------------------------------
int cmd_send_version(ProtDevice *dev)
{
  Event ev;

  cmd_oop_prepare(&ev);
  ev.Len = 29;

  ev.Event[11] = 0x50;
  ev.Event[13] = 1;	// Anubi
  ev.Event[14] = 1;	// version major number
  
  version_data(ev.Event);
  
/* 18-09-2011 */
#if 0
  while(prot_recv(NULL, dev) && dev->cmd.MsgType != ENQ)
    codec_send_null(dev);
#else
  while(prot_recv(NULL, dev) && (dev->cmd.MsgType != ENQ))
    if(dev->cmd.MsgType == STX) codec_send_null(dev);
#endif
  codec_send_event(&ev, dev);
  
/* 18-09-2011 */
  dev->lastevent = NULL;
  return 0;
}

int cmd_send_internal_info(ProtDevice *dev)
{
  Event ev;
  
  cmd_oop_prepare(&ev);
  ev.Len = 31;
  
  ev.Event[13] = MINUTI;
  ev.Event[14] = ORE;
  ev.Event[15] = GIORNO_S;
  ev.Event[16] = GIORNO;
  ev.Event[17] = MESE;
  ev.Event[18] = ANNO;
  ev.Event[19] = TEST_ATTIVO;
  ev.Event[20] = PROVA;
  ev.Event[21] = CHIAVE[0];
  ev.Event[22] = CHIAVE[1];
  ev.Event[23] = CHIAVE[2];
  ev.Event[24] = CHIAVE[3];
  ev.Event[25] = CHIAVE[4];

/* 18-09-2011 */
#if 0
  while(prot_recv(NULL, dev) && dev->cmd.MsgType != ENQ)
    codec_send_null(dev);
#else
  while(prot_recv(NULL, dev) && (dev->cmd.MsgType != ENQ))
    if(dev->cmd.MsgType == STX) codec_send_null(dev);
#endif
  codec_send_event(&ev, dev);
  
  ev.Event[13] = CHIAVE_FALSA;
  ev.Event[14] = M_CONTENITORE;
  ev.Event[15] = M_LINEA;
  ev.Event[16] = RIPRISTINO_LINEA;
  ev.Event[17] = GUASTO;
  ev.Event[18] = GUASTO_NOTEST;
  ev.Event[19] = CHIAVE_TRASMESSA;
  ev.Event[20] = NO_ATTI_ZONA;
  ev.Event[21] = N_MINUTO;
  ev.Event[22] = N_ORA;
  ev.Event[23] = 0;
  ev.Event[24] = N_GIORNO;
  ev.Event[25] = N_MESE;
  ev.Event[26] = N_ANNO;
  ev.Event[27] = TIPO_GIORNO;
  ev.Event[28] = RICHIESTA_ERRATA_AT;
  
/* 18-09-2011 */
#if 0
  while(prot_recv(NULL, dev) && dev->cmd.MsgType != ENQ)
    codec_send_null(dev);
#else
  while(prot_recv(NULL, dev) && (dev->cmd.MsgType != ENQ))
    if(dev->cmd.MsgType == STX) codec_send_null(dev);
#endif
  codec_send_event(&ev, dev);
  
/* 18-09-2011 */
  dev->lastevent = NULL;
  
  return 0;
}

/*
  Abilita l'attivazione di una zona insieme o della zona totale
  allarmate solo da zone ritardate.
*/
static int cmd_on_zone_alm(int zone, int check_zs)
{
  unsigned char *list;
  
  if(zone && (zone <= n_ZS))
  {
    if(!check_zs || ((ZONA[zone] & bitStatusInactive) && !ZONA_R[zone])) return 0;
  }
  else
  {
    if(!zone)
      list = TPDZT;
    else
      list = TPDZI[zone-n_ZS-1];
    if(list)
    {
      while(*list != 0xff)
      {
        if(!cmd_on_zone_alm(*list, 1)) return 0;
        list++;
      }
    }
  }
  
  return 1;
}

/*
  mode:
  0: if zone in alarm don't activate
  1: if zone in alarm activate with sensor alarm
  2: if zone in alarm activate with sensor out of service
  0x8X: attiva immediatamente la zona con la modalita' X.
*/
int cmd_zone_do_on(int zone, int mode)
{
  unsigned char ev[8];
  unsigned char *zlist;
  int i, ret;

  if(ZONA[zone] & bitActive) return 0;
  if(ZONAEX[zone] & bitLockDisactive) return -1;
  
  ret = 1;
  
  if(zone && (zone < (n_ZS+1)) && !cmd_comportamento_SYS)
  {
    ev[0] = Evento_Sensore;
    ev[4] = 3;
    
    for(i=0; SE_ZONA[zone][i] != -1; i++)
    {
      ev[1] = SE_ZONA[zone][i] >> 8;
      ev[2] = SE_ZONA[zone][i] & 0xff;
      
      if((SE[SE_ZONA[zone][i]] & bitMUAlarm) && !(SE[SE_ZONA[zone][i]] & bitAlarm) &&
         !(SE[SE_ZONA[zone][i]] & bitMUNoAlarm))
      {
        SE[SE_ZONA[zone][i]] &= ~(bitMUAlarm|(bitMUCounter<<1)|bitMUCounter);
        ev[3] = 1;
        codec_queue_event(ev);
      }
      if((SE[SE_ZONA[zone][i]] & bitMUSabotage) && !(SE[SE_ZONA[zone][i]] & bitSabotage) &&
         !(SE[SE_ZONA[zone][i]] & bitMUNoSabotage))
      {
        SE[SE_ZONA[zone][i]] &= ~(bitMUSabotage|(bitMUCounter<<1)|bitMUCounter);
        ev[3] = 2;
        codec_queue_event(ev);
      }
      if((SE[SE_ZONA[zone][i]] & bitMUFailure) && !(SE[SE_ZONA[zone][i]] & bitFailure) &&
         !(SE[SE_ZONA[zone][i]] & bitMUNoFailure))
      {
        SE[SE_ZONA[zone][i]] &= ~(bitMUFailure|(bitMUCounter<<1)|bitMUCounter);
        ev[3] = 3;
        codec_queue_event(ev);
      }
    }
  }
  
  if((zone && (zone < (n_ZS+1))) && ZONA_R[zone] && !(mode & 0x80))
  // zona semplice ritardata
  {
    if(!ZONA_R_timer[zone])
    {
      /* La zona ritardata si deve attivare sempre, anche in allarme se è il caso. */
      if(!mode)
        ZONA_R_mode[zone] = 1;
      else
        ZONA_R_mode[zone] = mode;
      ZONA_R_timer[zone] = ZONA_R[zone];
      
#if 0
      ev[0] = Evento_Esteso2;
      ev[1] = Ex2_Delphi;
      ev[2] = Ex2_Attivata_zona_rit;
      ev[3] = zone;
      codec_queue_event(ev);
#endif
    }
  }
  else if(ZONA[zone] & bitStatusInactive)
  {
    mode &= 0x7f;
    
    if((mode > 0) || cmd_on_zone_alm(zone, 0))
    {
      ZONA[zone] |= bitActive;      
      ev[0] = Attivata_zona;
      ev[1] = zone;
      codec_queue_event(ev);
      if(zone && (zone <= n_ZS))
      {
        for(i=0; SE_ZONA[zone][i]!=-1; i++)
        {
          ev[1] = SE_ZONA[zone][i] >> 8;
          ev[2] = SE_ZONA[zone][i] & 0xff;
          
          if(!(SE[SE_ZONA[zone][i]] & bitOOS) && (SE[SE_ZONA[zone][i]] & bitAlarm))
          {
            if(mode == 1)
            {
              if(!(SE[SE_ZONA[zone][i]] & bitNoAlarm))
              {
                ev[0] = Allarme_Sensore;
                ev[3] = zone;
                codec_queue_event(ev);
              
                if(database_testandset_MU(SE_ZONA[zone][i], bitMUAlarm))
                {
                  ev[0] = Evento_Sensore;
                  ev[3] = 1;
                  ev[4] = 1;
                  codec_queue_event(ev);
                }
              }
              ZONA[zone] |= bitAlarm | bitLHF;
              SE[SE_ZONA[zone][i]] |= bitGestAlarm | bitLHF;
            }
            else if(mode == 2)
            {
              if(cmd_sensor_off(SE_ZONA[zone][i]))
                SE[SE_ZONA[zone][i]] |= bitOOSAZ;
            }
          }
        }
      }
    }
    else
    {
      ret = -1;
      NO_ATTI_ZONA |= bitAlarm;
      ev[0] = Attivazione_impedita;
      ev[1] = zone;
      codec_queue_event(ev);
      if(zone && (zone <= n_ZS))
      {
        ev[0] = Sensore_in_Allarme;
        ev[3] = zone;
        for(i=0; SE_ZONA[zone][i]!=-1; i++)
        {
          if(!(SE[SE_ZONA[zone][i]] & bitOOS) && (SE[SE_ZONA[zone][i]] & bitAlarm))
          {
            ev[1] = SE_ZONA[zone][i] >> 8;
            ev[2] = SE_ZONA[zone][i] & 0xff;
            codec_queue_event(ev);
          }
        }
      }
    }
  }
  else
  {
    ZONA[zone] |= bitActive;
    ev[0] = Attivata_zona;
    ev[1] = zone;
    codec_queue_event(ev);
  }

  /* Activate sub-zones */
  
  if((zone == 0) && TPDZT)
  {
    zlist = TPDZT;
    while(*zlist != 0xff)
    {
#ifndef ATTIVAZIONE_PARZIALE_ZONE
      if((ret > 0) || ((ZONA[*zlist] & bitStatusInactive) && (!*zlist || (*zlist>n_ZS) || !ZONA_R[*zlist])))
#endif
        cmd_zone_do_on(*zlist, mode);
      zlist++;
    }
  }
  else if((zone > n_ZS) && TPDZI[zone - n_ZS - 1])
  {
    zlist = TPDZI[zone - n_ZS - 1];
    while(*zlist != 0xff)
    {
#ifndef ATTIVAZIONE_PARZIALE_ZONE
      if((ret > 0) || ((ZONA[*zlist] & bitStatusInactive) && (!*zlist || (*zlist>n_ZS) || !ZONA_R[*zlist])))
#endif
        cmd_zone_do_on(*zlist, mode);
      zlist++;
    }
  }
  
  return ret;
}

int cmd_zone_on(int zone, int mode)
{
  int ret;
  if((ret = cmd_zone_do_on(zone, mode)))
  {
    cmd_zone_refresh(1);
    cmd_actuator_refresh();
    if(ret < 0) return 0;
    return 1;
  }
  return 0;
}

int cmd_zone_do_off(int zone, int no_check)
{
  unsigned char ev[4];
  unsigned char *zlist;
  int i;
  
  if(zone && (zone < (n_ZS+1))) ZONA_R_timer[zone] = 0;
  
  if((ZONA[zone] & bitActive) && !(ZONA[zone] & bitLockActive))
  {
    ZONA[zone] &= ~(bitActive | bitAlarm | bitLHF | bitHLF);
    ev[0] = Disattivata_zona;
    ev[1] = zone;
    codec_queue_event(ev);
    
    /* Se disattivo una zona semplice, rimetto in servizio i sensori esclusi
       in modo automatico e segnalo il ripristino per i sensori in allarme. */
    if(zone && (zone < (n_ZS+1)))
      for(i=0; SE_ZONA[zone][i]!=-1; i++)
      {
        if(SE[SE_ZONA[zone][i]] & bitOOSAZ)
          cmd_sensor_on(SE_ZONA[zone][i]);
        else if((SE[SE_ZONA[zone][i]] & bitAlarm) &&
                !(SE[SE_ZONA[zone][i]] & bitOOS) &&
                !(SE[SE_ZONA[zone][i]] & bitNoAlarm) &&
                !cmd_comportamento_SYS)
        {
          if(!(SE[SE_ZONA[zone][i]] & bitMUNoAlarm) || !(SE[SE_ZONA[zone][i]] & bitMUAlarm))
          {
            SE[SE_ZONA[zone][i]] &= ~bitGestAlarm;
            ev[0] = Ripristino_Sensore;
            ev[1] = SE_ZONA[zone][i] >> 8;
            ev[2] = SE_ZONA[zone][i] & 0xff;
            ev[3] = zone;
            codec_queue_event(ev);
          }
        }
      }
  }
  else if(!no_check)
    return 0;

  if((zone == 0) && TPDZT)
  {
    zlist = TPDZT;
    while(*zlist != 0xff)
    {
      cmd_zone_do_off(*zlist, no_check);
      zlist++;
    }
  }
  else if((zone > n_ZS) && TPDZI[zone - n_ZS - 1])
  {
    zlist = TPDZI[zone - n_ZS - 1];
    while(*zlist != 0xff)
    {
      cmd_zone_do_off(*zlist, no_check);
      zlist++;
    }
  }

  return 1;
}

int cmd_zone_off(int zone, int no_check)
{
  if(cmd_zone_do_off(zone, no_check))
  {
    cmd_zone_refresh(0);
    cmd_actuator_refresh();
    return 1;
  }
  return 0;
}

int cmd_zone_refresh(int onoff)
{
  int i;
  unsigned char ev[4];
  unsigned char *zlist;
  int update = 1;
  int all_on;
  
  while(update)
  {
    update = 0;
    for(i=0; i<n_ZI; i++)
    {
      all_on = 1;
      
      if((zlist = TPDZI[i]) &&
        (((onoff) && !(ZI[i] & bitActive)) || ((!onoff) && (ZI[i] & bitActive))))
      {
        while(*zlist != 0xff)
        {
//          if(!(ZONA[*zlist] & bitActive)) all_on = 0;
          /* La zona semplice in corso di attivazione ritardata conta come zona attiva */
          if(!(ZONA[*zlist] & bitActive) && (!*zlist || (*zlist > n_ZS) || !ZONA_R_timer[*zlist])) all_on = 0;
          zlist++;
        }
      }
      
      if(TPDZI[i] && *TPDZI[i] != 0xff)  // una lista vuota non cambia lo stato della zona
      {
        if(onoff && all_on && !(ZI[i] & bitActive))
        {
          update = 1;
          ZI[i] |= bitActive;
          ev[0] = Attivata_zona;
          ev[1] = i + 1 + n_ZS;
          codec_queue_event(ev);
        }
        else if(!onoff && !all_on && (ZI[i] & bitActive))
        {
          update = 1;
          ZI[i] &= ~bitActive;
          ev[0] = Disattivata_zona;
          ev[1] = i + 1 + n_ZS;
          codec_queue_event(ev);
        }
      }
    }
  }
  
  if((zlist = TPDZT) && (*zlist != 0xff))
  {
    all_on = 1;
    while(*zlist != 0xff)
    {
      if(!(ZONA[*zlist] & bitActive)) all_on = 0;
      zlist++;
    }

    if(onoff && all_on && !(ZT & bitActive))
    {
      ZT |= bitActive;
      ev[0] = Attivata_zona;
      ev[1] = 0;
      codec_queue_event(ev);
    }
    else if(!onoff && !all_on && (ZT & bitActive))
    {
      ZT &= ~bitActive;
      ev[0] = Disattivata_zona;
      ev[1] = 0;
      codec_queue_event(ev);
    }
  }
  
  return 0;
}

int cmd_periph_set(int idx, int status)
{
  /* TO DO */
  return 0;
}

int cmd_update_tunings(unsigned char idx, unsigned char time, unsigned char sens)
{
  /* TO DO */
  return 0;
}

int cmd_actuator_on(int idx)
{
  unsigned char ev[4];
  
  if(idx > n_AT) return -1;
  
  if(AT[idx] & bitOOS)
  {
    AT[idx] = 0;
    ev[0] = Attuatore_In_Servizio;
    ev[1] = idx >> 8;
    ev[2] = idx & 0xff;
    codec_queue_event(ev);
    if(MMPresente[idx>>8] == MM_LARA)
    {
      tebe_actuator_on(idx);
      AT[idx] = bitAbilReq;
    }
    return 1;
  }
  
  return 0;
}

int cmd_actuator_off(int idx)
{
  unsigned char ev[4];
  
  if(idx > n_AT) return -1;
  
  if(!(AT[idx] & bitOOS))
  {
    AT[idx] = bitOOS | bitAbilReq;
    ev[0] = Attuatore_Fuori_Servizio;
    ev[1] = idx >> 8;
    ev[2] = idx & 0xff;
    codec_queue_event(ev);
    if(MMPresente[idx>>8] == MM_LARA) tebe_actuator_off(idx);
    return 1;
  }
  
  return 0;
}

int cmd_actuator_refresh()
{
  int i;
  
  for(i=0; i<n_AT; i++)
  {
    if(MMPresente[i>>8] == 0xff)
    {
      i += 255;
    }
    else if(TipoPeriferica[i>>3] == 0xff)
    {
      i += 7;
    }
    else if(Zona_SE[i] != 0xff)
    {
      if((TipoPeriferica[i>>3] < 2) || (TipoPeriferica[i>>3] == 6))
      {
      if(ZONA[Zona_SE[i]] & bitActive)
      {
        if(!(AT[i] & bitAbil))
          AT[i] |= bitAbil | bitAbilReq;
        else
          continue;
      }
      else
      {
        if(AT[i] & bitAbil)
        {
          AT[i] &= ~bitAbil;
          AT[i] |= bitAbilReq;
        }
        else
          continue;
      }
      }
      
      if((i & 7) || (!(DEFPT[i>>3][0] || DEFPT[i>>3][1] || DEFPT[i>>3][2])))
      {
        if((Zona_SE[i] >= MIN_ZABIL) && (Zona_SE[i] <= MAX_ZABIL))
        {
          if(AT[i] & bitAbil)
            AT[i] |= bitON | bitLHF;
          else
          {
            AT[i] &= ~bitON;
            AT[i] |= bitHLF;
          }
        }
      }
    }
  }
  
  return 0;
}

int cmd_sensor_on(int idx)
{
  unsigned char ev[4], cmd[2];
  int node;
  
  if(idx > n_SE) return -1;
  
  if(SE[idx] & bitOOS)
  {
    SE[idx] &= (bitNoAlarm|bitMUNoAlarm|bitMUNoSabotage|bitMUNoFailure|bitAlarm /*|bitGestAlarm*/);
    ev[0] = Sensore_In_Servizio;
    ev[1] = idx >> 8;
    ev[2] = idx & 0xff;
    codec_queue_event(ev);
    
    if(SE[idx] & bitAlarm)
    {
      SE[idx] &= ~bitAlarm;
      master_set_alarm(idx, 1, 1);
    }
    
    if(MMPresente[idx >> 8] == MM_LARA)
    {
      tebe_sensor_on(idx);
      node = (idx>>3) & 0x1f;
      if((idx >= 0x0100) && (MMPresente[(idx >> 8) - 1] == MM_LARA)) node += 32;
      if((idx >= 0x0200) && (MMPresente[(idx >> 8) - 2] == MM_LARA)) return 0;	// actuators
      cmd[0] = _lara_ingressi[node];
      if(((idx & 0x07) > 0) && ((idx & 0x07) < 6))
        _lara_ingressi[node] &= ~(1 << ((idx - 1) & 0x07));
      lara_input(idx >> 8, node, cmd[0], 0);
    }
    else
    {
      if((MMPresente[idx >> 8] == 0) &&
              (TipoPeriferica[idx>>3] == 0xf /*virt SC8R2 868*/))
      {
        /* Lo stato sensori va richiesto al concentratore radio */
        void (*radio868_refresh_p)(int idx);
        radio868_refresh_p = dlsym(NULL, "radio868_refresh");
        if(radio868_refresh_p) radio868_refresh_p(idx);
      }
      else if(!master_periph_present(idx>>3))
      {
        if((idx & 0x07) == 1)
        {
          SEmu[idx] |= bitSEmuLineSab;
          SE[idx] |= bitSabotage | bitLineSabotage;
          if(database_testandset_MU(idx, bitMUSabotage))
          {
            SE[idx] |= bitGestSabotage;
          }
        }
        
/* Il bit bitMUNoSabotage risulta sempre a 0 e quindi il guasto non torna.
   Ma a Rosso va meglio così e quindi lascio commentato. */
//        if(SE[(idx&~0x07)+1] & bitMUNoSabotage)
//          master_set_failure(idx, 1, Guasto_Sensore);
      }
      else if((MMPresente[idx >> 8] != 0) ||
              ((TipoPeriferica[idx>>3] != 6 /*TAS*/) &&
               (TipoPeriferica[idx>>3] != 7 /*SEN*/) &&
               (TipoPeriferica[idx>>3] != 0xf /*virt SC8R2 868*/)))
      {
        /* Chiedo lo stato sensori, ma solo se la periferica non è una tastiera CL. */
        cmd[0] = 7;
        cmd[1] = (idx >> 3) & 0x1f;
        master_queue(idx >> 8, cmd);
      }
    }
    
    return 1;
  }
  
  return 0;
}

int cmd_sensor_off(int idx)
{
  unsigned char ev[8];
  
  if(idx > n_SE) return -1;
  
  if(!(SE[idx] & bitOOS))
  {
    ev[1] = idx >> 8;
    ev[2] = idx & 0xff;

    if((SE[idx] & (bitGestAlarm|bitMUNoAlarm)) == (bitGestAlarm|bitMUNoAlarm))
    {
      if(SE[idx] & bitMUAlarm)
      {
        ev[0] = Evento_Sensore;
        ev[3] = 1;
        ev[4] = MU_acc;
        codec_queue_event(ev);
      }
      
      SE[idx] &= ~(bitAlarm|bitGestAlarm);
      SE[idx] |= bitHLF;
      ev[0] = Ripristino_Sensore;
      ev[3] = Zona_SE[idx];
      codec_queue_event(ev);
    }
    if((SE[idx] & bitAlarm) && !(SE[idx] & bitNoAlarm) &&
       (Zona_SE[idx] != 0xff) && (ZONA[Zona_SE[idx]] & bitActive))
    {
      SE[idx] &= ~bitGestAlarm;
      ev[0] = Ripristino_Sensore;
      ev[3] = Zona_SE[idx];
      codec_queue_event(ev);
    }
    if(SE[idx] & bitInputSabotage)
    {
      if(SE[idx] & bitMUSabotage)
      {
        ev[0] = Evento_Sensore;
        ev[3] = 2;
        ev[4] = MU_acc;
        codec_queue_event(ev);
      }
      
      SE[idx] |= bitInputSabotageHLF;
      ev[0] = Fine_Manomis_Dispositivo;
      codec_queue_event(ev);
    }
    if(SE[idx] & bitInputFailure)
    {
      if(SE[idx] & bitMUFailure)
      {
        ev[0] = Evento_Sensore;
        ev[3] = 3;
        ev[4] = MU_acc;
        codec_queue_event(ev);
      }
      
      SE[idx] |= bitInputFailureHLF;
      ev[0] = Fine_Guasto_Sensore;
      codec_queue_event(ev);
    }
    if(!(idx & 0x07) && (SE[idx] & bitPackSabotage))
    {
      SE[idx] |= bitPackHLF;
      ev[0] = Fine_Manomis_Contenitore;
#ifdef GEST16MM
      ev[1] = idx >> 11;
      ev[2] = idx >> 3;
#else
      ev[1] = idx >> 3;
#endif
      codec_queue_event(ev);
      ev[1] = idx >> 8;
      ev[2] = idx & 0xff;
    }
/*
Se viene messo fuori servizio il sensore non do il fine manomissione periferica
perche' non sarebbe corretto. Quando il sensore viene rimesso in servizio
ripristino lo stato di manomissione del sensore ma senza fronti.
    if(((idx & 0x07) == 1) && (SE[idx] & bitLineSabotage))
    {
    }
*/
    if(((idx & 0x07) == 2) && (SE[idx] & bitGenFailure))
    {
      SE[idx] |= bitGenHLF;
      ev[0] = Fine_Guasto_Periferica;
#ifdef GEST16MM
      ev[1] = idx >> 11;
      ev[2] = idx >> 3;
#else
      ev[1] = idx >> 3;
#endif
      codec_queue_event(ev);
      ev[1] = idx >> 8;
      ev[2] = idx & 0xff;
    }
    
    if(MMPresente[idx>>8] == MM_LARA)
    {
      SE[idx] &= (bitSabotage|bitNoAlarm|bitMUNoAlarm|bitMUNoSabotage|bitMUNoFailure|bitAlarm|
                  bitHLF|bitInputSabotageHLF|bitInputFailureHLF|bitPackHLF|bitGenHLF/*|bitGestAlarm*/);
      tebe_sensor_off(idx);
    }
    else
      SE[idx] &= (bitNoAlarm|bitMUNoAlarm|bitMUNoSabotage|bitMUNoFailure|bitAlarm|
                  bitHLF|bitInputSabotageHLF|bitInputFailureHLF|bitPackHLF|bitGenHLF/*|bitGestAlarm*/);
    SE[idx] |= bitOOS;
    ev[0] = Sensore_Fuori_Servizio;
    codec_queue_event(ev);
    
    return 1;
  }
  
  return 0;
}

int cmd_modem_status(unsigned char *val, int retstatus)
{
  unsigned char ev[4];

  STATO_MODEM = val[0] - '0';
  ev[0] = Conferma_ricezione_modem;
  ev[1] = retstatus;
  codec_queue_event(ev);
  
  return 0;
}

int cmd_accept_actuators(int grp)
{
  int i, ini, num, alr;
  
  if(grp)
  {
    ini = (grp - 1) * 64;
    num = 64;
  }
  else
  {
    ini = 0;
    num = n_AT;
  }
  
  for(i=ini; i<num; i++)
  {
    if(i & 7)
    {
      alr = AT[i] & bitAlarm;
      AT[i] &= ~(bitON | bitLHF | bitHLF | bitFlashing);
      if(alr) AT[i] |= bitAbilReq;
    }
    else
    {
      if((DEFPT[i>>3][2] == 1) && !DEFPT[i>>3][3] && !DEFPT[i>>3][4])
      {
        i += 7;
      }
      else
      {
        alr = AT[i] & bitAlarm;
        AT[i] &= ~(bitON | bitLHF | bitHLF | bitFlashing);
        if(alr) AT[i] |= bitAbilReq;
      }
    }
  }
  
  return 1;
}

int cmd_accept_act_timers()
{
  int i, alr;
  
  if(!TPDAT) return 0;
  for(i=0; TPDAT[i]!=0xffff; i++)
  {
    alr = AT[TPDAT[i]] & bitAlarm;
    AT[TPDAT[i]] &= ~(bitON | bitLHF | bitHLF);
    if(alr) AT[TPDAT[i]] |= bitAbilReq;
  }
  
  return 1;
}

int cmd_accept_mem(int grp)
{
  if(grp)
    memset(ME + (grp << 4), 0, 16);
  else
    memset(ME, 0, n_ME);
  
  return 0;
}

int cmd_accept_counters(int grp)
{
  if(grp)
  {
    memset(CO + (grp << 3), 0, 8);
    memset(TCO + (grp << 3) * sizeof(short), 0, 8 * sizeof(short));
  }
  else
  {
    memset(CO, 0, sizeof(CO));
    memset(TCO, 0, sizeof(TCO));
  }

  return 0;
}

int cmd_accept_timers(int grp)
{
  if(grp)
  {
    memset(TM + (grp << 3), 0, 8);
    memset(TTM + (grp << 3) * sizeof(int), 0, 8 * sizeof(int));
  }
  else
  {
    memset(TM, 0, sizeof(TM));
    memset(TTM, 0, sizeof(TTM));
  }
  
  return 0;
}

int cmd_accept_fronts()
{
  int i,j,k;
  static const char master_868_lut[32] = {
    -1,-1,0,-1,-1,-1,-1,1,-1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,2,-1,-1,-1,-1,3,-1,-1,-1,-1,-1,-1,-1,-1,};
  
  for(i=0; i<MAX_NUM_MM ; i++)
  {
    for(j=0; j<32; j++)
    {
      if(MMPresente[i] == 0)
      {
        switch(TipoPeriferica[(i<<5)+j])
        {
          case SS: case SC1:
          case TAS: case SEN: case QR: case ATt:
            SE[(i<<8)+(j<<3)+1] &= ~(bitLHF | bitHLF | bitPackLHF | bitPackHLF | bitInputSabotageLHF |
                     bitInputSabotageHLF | bitTestFailureLHF | bitTestFailureHLF |
                     bitInputFailureLHF | bitInputFailureHLF);
//          case SS: case SC1:
            SE[(i<<8)+(j<<3)] &= ~(bitLHF | bitHLF | bitPackLHF | bitPackHLF | bitInputSabotageLHF |
                     bitInputSabotageHLF | bitTestFailureLHF | bitTestFailureHLF |
                     bitInputFailureLHF | bitInputFailureHLF);
            for(k=(i<<8)+(j<<3); k<(i<<8)+(j<<3)+8; k++) AT[k] &= ~(bitLHF | bitHLF);
            break;
          case SC8: case SAN: case TS2: case SC8C:
            for(k=(i<<8)+(j<<3); k<(i<<8)+(j<<3)+8; k++)
            {
              SE[k] &= ~(bitLHF | bitHLF | bitPackLHF | bitPackHLF | bitInputSabotageLHF |
                       bitInputSabotageHLF | bitTestFailureLHF | bitTestFailureHLF |
                       bitInputFailureLHF | bitInputFailureHLF);
              AT[k] &= ~(bitLHF | bitHLF);
            }
            break;
          case SC8R2:
            for(k=(i<<8)+(j<<3); k<(i<<8)+(j<<3)+8; k++)
            {
              SE[k] &= ~(bitLHF | bitHLF | bitPackLHF | bitPackHLF | bitInputSabotageLHF |
                       bitInputSabotageHLF | bitTestFailureLHF | bitTestFailureHLF |
                       bitInputFailureLHF | bitInputFailureHLF);
              AT[k] &= ~(bitLHF | bitHLF);
            }
            /* Accetta i fronti dei sensori radio 868 */
            if(master_868_lut[j] >= 0)
              master_radio868_accept_fronts((i*4)+master_868_lut[j]);
            break;
          default:
            break;
        }
      }
      else if(MMPresente[i] == MM_LARA)
      {
        if(TipoPeriferica[(i<<5)+j] != 0xff)
          for(k=(i<<8)+(j<<3); k<(i<<8)+(j<<3)+8; k++)
          {
            SE[k] &= ~(bitLHF | bitHLF | bitPackLHF | bitPackHLF | bitInputSabotageLHF |
                       bitInputSabotageHLF | bitTestFailureLHF | bitTestFailureHLF |
                       bitInputFailureLHF | bitInputFailureHLF);
            AT[k] &= ~(bitLHF | bitHLF);
          }
      }
      else if(MMPresente[i] != 0xff)
      {
        switch(TipoPeriferica[(i<<5)+j])
        {
          case SSA: case SST:
          case TST: case TSK: case TLK: case TLB:
          case ATA:
            SE[(i<<8)+(j<<3)+1] &= ~(bitLHF | bitHLF | bitPackLHF | bitPackHLF | bitInputSabotageLHF |
                     bitInputSabotageHLF | bitTestFailureLHF | bitTestFailureHLF |
                     bitInputFailureLHF | bitInputFailureHLF);
            SE[(i<<8)+(j<<3)] &= ~(bitLHF | bitHLF | bitPackLHF | bitPackHLF | bitInputSabotageLHF |
                     bitInputSabotageHLF | bitTestFailureLHF | bitTestFailureHLF |
                     bitInputFailureLHF | bitInputFailureHLF);
            for(k=(i<<8)+(j<<3); k<(i<<8)+(j<<3)+8; k++) AT[k] &= ~(bitLHF | bitHLF);
            break;
          case SA8: case SAN:
            for(k=(i<<8)+(j<<3); k<(i<<8)+(j<<3)+8; k++)
            {
              SE[k] &= ~(bitLHF | bitHLF | bitPackLHF | bitPackHLF | bitInputSabotageLHF |
                       bitInputSabotageHLF | bitTestFailureLHF | bitTestFailureHLF |
                       bitInputFailureLHF | bitInputFailureHLF);
              AT[k] &= ~(bitLHF | bitHLF);
            }
            break;
          default:
            break;
        }
      }
    }
  
  }
  
//  for(i=0; i<n_AT; i++) AT[i] &= ~(bitLHF | bitHLF);
  for(i=0; i<n_ME; i++) ME[i] &= ~(bitLHF | bitHLF);
  for(i=0; i<n_CO; i++) CO[i] &= ~(bitLHF | bitHLF);
  for(i=0; i<n_TM; i++) TM[i] &= ~(bitLHF | bitHLF);
  for(i=0; i<(1+n_ZS+n_ZI); i++) ZONA[i] &= ~(bitLHF | bitHLF);
  for(i=0; i<n_FAG; i++) FAG[i] &= ~(bitLHF | bitHLF);
  
  return 0;
}

/*
int cmd_command_check_test(int s)
{
  return 0;
}
*/

int cmd_command_send_noise(int idx)
{
  AT[idx<<3] |= bitAbilReq | bitNoise;
  
  return 0;
}

int cmd_command_reset_noise(int idx)
{
  if(AT[idx<<3] & bitNoise)
  {
    AT[idx<<3] &= ~bitNoise;
    AT[idx<<3] |= bitAbilReq;
  }
  
  return 0;
}

int cmd_command_send_noise_all()
{
  int i;
  
  for(i=0; i<(MM_NUM*32); i++)
    if(DEFPT[i][0] || DEFPT[i][1] || DEFPT[i][2]) cmd_command_send_noise(i);
    
  return 0;
}

int cmd_command_reset_noise_all()
{
  int i;
  
  for(i=0; i<(MM_NUM*32); i++)
    cmd_command_reset_noise(i);
    
  return 0;
}

static timeout_t *cmd_test_to = NULL;

static void cmd_test_timeout(void *null, int grp)
{
  int i;
  
  for(i=0; TDTA[grp][i] != 0xffff; i++)
  {
    AT[TDTA[grp][i]+1] &= ~bitON;
    AT[TDTA[grp][i]+1] |= bitHLF | bitAbilReq;
  }
}

int cmd_command_start_test(int grp)
{
  unsigned char ev[4];
  int i;
  
  if((grp < 0) || (grp > n_TDTA-1)) return -1;
  if(!TDTA[grp]) return -2;
  
  if(grp > 15)
  {
    if(!cmd_test_to)
    {
      cmd_test_to = timeout_init();
      if(!cmd_test_to) return -3;
    }
    timeout_on(cmd_test_to, cmd_test_timeout, NULL, grp, 70);
  }
  
  ev[0] = Test_attivo_in_corso;
  ev[1] = grp;
  codec_queue_event(ev);

  TEST_ATTIVO |= bitLHF | bitON;
  
  for(i=0; TDTA[grp][i] != 0xffff; i++)
  {
    if(!(SE[TDTA[grp][i]] & bitOOS))
    {
      if(grp > 15)	// gruppo microonde
      {
#if 0
        if(AT[TDTA[grp][i]] & bitON)
          AT[TDTA[grp][i]+1] |= bitTemp;
        else
          AT[TDTA[grp][i]] |= bitLHF | bitON;
#else
        if(AT[TDTA[grp][i]] & bitON)
        {
          AT[TDTA[grp][i]+1] |= bitTemp;
          AT[TDTA[grp][i]] |= bitHLF;
          AT[TDTA[grp][i]] &= ~bitON;
        }
#endif
        AT[TDTA[grp][i]+1] |= bitLHF | bitON | bitAbilReq;
      }
      SE[TDTA[grp][i]] |= bitTest;
      AT[TDTA[grp][i]] |= bitAbilReq;
      SEmu[TDTA[grp][i]] |= bitSEmuTest;
    }
  }
  
  return 0;
}

int cmd_command_end_test(int grp)
{
//  unsigned char ev[4];
  int i;
  
  if((grp < 0) || (grp > n_TDTA-1)) return -1;
  if(!TDTA[grp]) return -2;
  
  TEST_ATTIVO = (TEST_ATTIVO & ~bitON) | bitHLF;
  
  if(cmd_test_to) timeout_off(cmd_test_to);
  
  for(i=0; TDTA[grp][i] != 0xffff; i++)
  {
    if(SE[TDTA[grp][i]] & bitTest)
    {
      SE[TDTA[grp][i]] &= ~bitTest;
      AT[TDTA[grp][i]] |= bitAbilReq;
      if(!(SE[TDTA[grp][i]] & bitFailure))
      {
        master_set_failure(TDTA[grp][i], 1, Guasto_Sensore);
//        GUASTO_NOTEST = TDTA[grp][i];
      }
      GUASTO_NOTEST = TDTA[grp][i];
    }
    
    if(grp > 15)	// gruppo microonde
    {
#if 0
      if(AT[TDTA[grp][i]+1] & bitTemp)
      {
        AT[TDTA[grp][i]+1] &= ~bitTemp;
      }
      else
      {
        AT[TDTA[grp][i]] &= ~bitON;
        AT[TDTA[grp][i]] |= bitHLF | bitAbilReq;
        if(AT[TDTA[grp][i]+1] & bitON)
        {
          AT[TDTA[grp][i]+1] &= ~bitON;
          AT[TDTA[grp][i]+1] |= bitHLF | bitAbilReq;
        }
      }
#else
      if(AT[TDTA[grp][i]+1] & bitTemp)
      {
        AT[TDTA[grp][i]+1] &= ~bitTemp;
        AT[TDTA[grp][i]] |= bitON | bitLHF | bitAbilReq;
      }
      if(AT[TDTA[grp][i]+1] & bitON)
      {
        AT[TDTA[grp][i]+1] &= ~bitON;
        AT[TDTA[grp][i]+1] |= bitHLF | bitAbilReq;
      }
#endif
    }
    SEmu[TDTA[grp][i]] &= ~bitSEmuTest;
  }
  
  return 0;
}

int cmd_keyboard_send_secret(int idx, unsigned char *secret)
{
  unsigned char cmd[8];
  
  if(MMPresente[idx >> 5] != 0xff)
  {
    cmd[0] = 16;
    cmd[1] = idx & 0x1f;
    cmd[2] = secret[0];
    cmd[3] = secret[1];
    cmd[4] = secret[2];
    master_queue(idx >> 5, cmd);
  }
  return 0;
}

int cmd_keyboard_send_key(int idx, unsigned char *key)
{
  unsigned char cmd[8];
  
  if(MMPresente[idx >> 5] != 0xff)
  {
    cmd[0] = 17;
    cmd[1] = idx & 0x1f;
    cmd[2] = key[0];
    cmd[3] = key[1];
    cmd[4] = key[2];
    cmd[5] = key[3];
    cmd[6] = key[4];
    master_queue(idx >> 5, cmd);
  }
  return 0;
}

int cmd_keyboard_set_state(int idx, int cond, int timeout)
{
  unsigned char cmd[4];
  
  if(MMPresente[idx >> 5] != 0xff)
  {
    cmd[0] = 19;
    cmd[1] = idx & 0x1f;
    cmd[2] = cond;
    cmd[3] = timeout;
    master_queue(idx >> 5, cmd);
  }
  return 0;
}

int cmd_keyboard_set_mode(int idx, int cond)
{
  unsigned char cmd[4];

  if(MMPresente[idx >> 5] != 0xff)
  {
    cmd[0] = 20;
    cmd[1] = idx & 0x1f;
    cmd[2] = cond;
    master_queue(idx >> 5, cmd);
  }
  return 0;
}

int cmd_keyboard_set_echo(int idx, int cond)
{
  unsigned char cmd[4];
  
  if(MMPresente[idx >> 5] != 0xff)
  {
    cmd[0] = 13;
    cmd[1] = idx & 0x1f;
    cmd[2] = cond;
    master_queue(idx >> 5, cmd);
  }
  return 0;
}

int cmd_display_message(int idx, int pos, char *msg)
{
  unsigned char cmd[16];
  int i, j;
  
  if(MMPresente[idx >> 5] != 0xff)
  {
    cmd[0] = 22;
    cmd[1] = idx & 0x1f;
    
    if(pos >= 0) cmd_display_set_cursor_pos(idx, pos);
    
    for(i=0; i<strlen(msg); i+=8)
    {
      strncpy(cmd + 2, msg + i, 8);
      for(j=2; j<10; j++) if(!cmd[j]) cmd[j] = 4;
      master_queue(idx >> 5, cmd);
    }
  }    
  return 0;
}

int cmd_display_set_mode(int idx, int mode)
{
  unsigned char cmd[4];
  
  if(MMPresente[idx >> 5] != 0xff)
  {
    cmd[0] = 23;
    cmd[1] = idx & 0x1f;
    cmd[2] = mode;
    master_queue(idx >> 5, cmd);
  }
  return 0;
}

int cmd_display_set_cursor_pos(int idx, int pos)
{
  unsigned char cmd[4];
  
  if(MMPresente[idx >> 5] != 0xff)
  {
    cmd[0] = 21;
    cmd[1] = idx & 0x1f;
    cmd[2] = pos |= 0x80;
    master_queue(idx >> 5, cmd);
  }
  return 0;
}

int cmd_memory_control_init()
{
  FILE *fp;
  int c, l;
  unsigned char buf[1024];
  
#ifdef TEBESA
  fp = fopen(ADDROOTDIR("/saet/tebe_sa"), "r");
#else
  fp = fopen(ADDROOTDIR("/saet/saet"), "r");
#endif
  cmd_memory_control_cks1 = 0;
  
  /* La lettura byte per byte era pazzescamente lenta. */
  //while((c = fgetc(fp)) != EOF) cmd_memory_control_cks1 += c;
  while((l = fread(buf, 1, 1024, fp)) > 0)
  {
    for(c=0; c<l; c++) cmd_memory_control_cks1 += buf[c];
  }
  fclose(fp);
  
  fp = fopen(ADDROOTDIR(UserFileName), "r");
  cmd_memory_control_cks2 = 0;
  if(fp)
  {
    //while((c = fgetc(fp)) != EOF) cmd_memory_control_cks2 += c;
    while((l = fread(buf, 1, 1024, fp)) > 0)
    {
      for(c=0; c<l; c++) cmd_memory_control_cks2 += buf[c];
    }
    fclose(fp);
  }
  
  return 0;
}

int cmd_memory_control()
{
  unsigned char ev[16], d;
  unsigned short cks3;
  int c, i;
  
  ev[0] = Codici_controllo;

  ev[1] = cmd_memory_control_cks1 >> 8;
  ev[2] = cmd_memory_control_cks1 & 0xff;
  
  ev[3] = cmd_memory_control_cks2 >> 8;
  ev[4] = cmd_memory_control_cks2 & 0xff;
  
  cks3 = 0xffff;
  
  if(config.Variant == 1)
  {
    for(i=0; i<n_DEFPT*5; i++)
    {
      d = ((unsigned char*)DEFPT)[i];
      for(c=0; c<8; c++)
      {
        if((d ^ cks3) & 0x0001)
          cks3 = (cks3 >> 1) ^ 0xa001;
        else
          cks3 >>= 1;
        d >>= 1;
      }
    }
  }
  else
  {
    for(c=0; c<(MM_NUM*32); c++)
    {
      if((TipoPeriferica[c] != 0xff) && !(StatoPeriferica[c>>3] & (1<<(c&0x07))))
        cks3 = c;
    }
  }
  
  ev[5] = cks3 >> 8;
  ev[6] = cks3 & 0xff;
  
  codec_queue_event(ev);
  
#ifdef __arm__
  ev[0] = Evento_Esteso;
  ev[1] = Ex_Stringa;
  ev[2] = 0;
  strcpy(ev+4, "Athena");
  ev[3] = strlen(ev+4);
  codec_queue_event(ev);
#endif
  
  return 0;
}

static unsigned char cmd_status_acceptable(short idx, int bit)
{
  int bitSE = 0;

  if(!(SE[idx] & bit)) return MU_nac;
  
  switch(bit)
  {
    case bitMUAlarm:
      bitSE = bitAlarm;
      break;
    case bitMUFailure:
      bitSE = bitFailure;
      break;
    case bitMUSabotage:
      bitSE = bitSabotage;
      break;
  }
  
  if(!(SE[idx] & bitSE) || (SE[idx] & bitOOS) ||
    (!cmd_comportamento_SYS && (
     ((bit == bitMUAlarm) && !(ZONA[Zona_SE[idx]] & bitActive)) ||
     ((bit == bitMUAlarm) && (SE[idx] & bitNoAlarm)))))
  {
    if((SE[idx] & ((bitMUCounter << 1) | bitMUCounter)) == ((bitMUCounter << 1) | bitMUCounter))
      SE[idx] &= ~((bitMUCounter << 1) | bitMUCounter);
    SE[idx] += bitMUCounter;
    
    if((SE[idx] & ((bitMUCounter << 1) | bitMUCounter)) == ((bitMUCounter << 1) | bitMUCounter))
      return MU_3ac;
    else
      return MU_acc;    
  }
  else
    return MU_nac;
}

int cmd_status_accept(short idx, int bit, char atype)
{
  unsigned char ev[8];
  int bitNoX, bitGest, bitSEmu;
  
  ev[0] = Evento_Sensore;
  ev[1] = idx >> 8;
  ev[2] = idx & 0xff;
  ev[3] = atype;
  ev[4] = cmd_status_acceptable(idx, bit);
  if(ev[4] != MU_nac) SE[idx] &= ~bit;
  codec_queue_event(ev);
  
  switch(bit)
  {
    case bitMUAlarm:
      ev[0] = Ripristino_Sensore;
      ev[3] = Zona_SE[idx];
      bitSEmu = bitSEmuAlarm;
      bitNoX = bitMUNoAlarm;
      bitGest = bitGestAlarm;      
      break;
    case bitMUFailure:
      if(((idx & 0x07) == 2) && (SEmu[idx] & bitSEmuGenFail))
      {
        ev[0] = Fine_Guasto_Periferica;
#ifdef GEST16MM
        ev[1] = idx >> 11;
        ev[2] = idx >> 3;
#else
        ev[1] = idx >> 3;
#endif
        bitSEmu = bitSEmuGenFail;
      }
      else
      {
        ev[0] = Fine_Guasto_Sensore;
        bitSEmu = bitSEmuInFail;
      }
      bitNoX = bitMUNoFailure;
      bitGest = bitGestFailure;
      break;
    case bitMUSabotage:
      if(((idx & 0x07) == 1) && (SEmu[idx] & bitSEmuLineSab))
      {
        ev[0] = Periferica_Ripristino;
#ifdef GEST16MM
        ev[1] = idx >> 11;
        ev[2] = idx >> 3;
#else
        ev[1] = idx >> 3;
#endif
        bitSEmu = bitSEmuLineSab;
      }
      else if(((idx & 0x07) == 0) && (SEmu[idx] & bitSEmuPackSab))
      {
        ev[0] = Fine_Manomis_Contenitore;
#ifdef GEST16MM
        ev[1] = idx >> 11;
        ev[2] = idx >> 3;
#else
        ev[1] = idx >> 3;
#endif
        bitSEmu = bitSEmuPackSab;
      }
      else
      {
        ev[0] = Fine_Manomis_Dispositivo;
        bitSEmu = bitSEmuInSab;
      }
      bitNoX = bitMUNoSabotage;
      bitGest = bitGestSabotage;
      break;
  }
  
  if((ev[4] != MU_nac) && (SE[idx] & bitNoX) && (SE[idx] & bitGest))
  {
    SE[idx] &= ~bitGest;
    SEmu[idx] &= ~bitSEmu;
    codec_queue_event(ev);
  }
  
  return (ev[4] != MU_nac);
}

int cmd_request_analog(int ind)
{
  unsigned char cmd[4];
  
  if(MMPresente[ind >> 5] != 0xff)
  {
    cmd[0] = 26;
    cmd[1] = ind & 0x1f;
    master_queue(ind >> 5, cmd);
  }
  return 1;
}

int cmd_send_threshold(int ind, unsigned char ch, unsigned char *threshold)
{
  unsigned char cmd[8];
  int sens;
  SogliaList *sl;
  
  if(((ch > 3) && (ch < 8)) || (ch > 15)) return 0;
  
  if(MMPresente[ind >> 5] != 0xff)
  {
    cmd[0] = 15;
    cmd[1] = ind & 0x1f;
    cmd[2] = ch;
    memcpy(cmd+3, threshold, 5);
    master_queue(ind >> 5, cmd);
  }
  
  switch(ch)
  {
    case 0:
      memcpy(DEFPT[ind], threshold, 5);
      break;
    case 1:
      DVSSAM[0] = threshold[0];
      DVSSAM[1] = threshold[3];
      break;
    case 2:
      DVSSAG[0] = threshold[0];
      DVSSAG[1] = threshold[3];
      break;
    case 3:
      if(CONF_SA)
      {
        for(sens=1; sens<n_SA; sens++) 
          memcpy(CNFG_SA[sens], CNFG_SA[0], 4);
        memcpy(CNFG_SA[ind], threshold, 4);
        CONF_SA = 0;
      }
      else
        memcpy(CNFG_SA[ind], threshold, 4);
      break;
    default:
      sens = (ind << 3) + ch - 8;
      for(sl = SogliaSingola; sl && sl->sens != sens; sl = sl->next);
      if(!sl)
      {
        sl = malloc(sizeof(SogliaList));
        if(!sl) return 0;
        sl->next = SogliaSingola;
        SogliaSingola = sl;
      }
      sl->sens = sens;
      memcpy(sl->so, threshold, 5);
      break;
  }
  
  database_changed = 1;
  return 1;
}

int cmd_TPDGF_reset()
{
  int i;
  
  if(TPDGF)
  {
    free(TPDGF);
    TPDGF = NULL;
    database_changed = 1;
  }
  
  TIPO_GIORNO = cmd_get_day_type(GIORNO_S - 1);
  for(i=0; i<n_FESTIVI; i++) FESTIVI[i] = cmd_get_day_type((GIORNO_S + i)%7);
  delphi_update_days();
  return 1;
}

int cmd_TPDGF_add(unsigned char *day)
{
  unsigned char *tpdgf;
  int len, i;
  
  len = 0;
  if(TPDGF) for(; TPDGF[len]!=0xff; len++);
  tpdgf = malloc(len + 4);
  if(!tpdgf) return 0;
  if(TPDGF)
  {
    memcpy(tpdgf, TPDGF, len);
    free(TPDGF);
  }
  tpdgf[len] = GVAL2(day);
  tpdgf[len+1] = GVAL2(day+2);
  tpdgf[len+2] = GVAL(day+4);
  tpdgf[len+3] = 0xff;
  TPDGF = tpdgf;
  database_changed = 1;
  
  TIPO_GIORNO = cmd_get_day_type(GIORNO_S - 1);
  for(i=0; i<n_FESTIVI; i++) FESTIVI[i] = cmd_get_day_type((GIORNO_S + i)%7);
  delphi_update_days();
  
  return 1;
}

int cmd_TPDGV_reset()
{
  int i;
  
  if(TPDGV)
  {
    free(TPDGV);
    TPDGV = NULL;
    database_changed = 1;
  }
  
  TIPO_GIORNO = cmd_get_day_type(GIORNO_S - 1);
  for(i=0; i<n_FESTIVI; i++) FESTIVI[i] = cmd_get_day_type((GIORNO_S + i)%7);
  delphi_update_days();
  return 1;
}

int cmd_TPDGV_add(unsigned char *day)
{
  unsigned char *tpdgv;
  int len, i;
  
  len = 0;
  if(TPDGV) for(; TPDGV[len]!=0xff; len++);
  tpdgv = malloc(len + 5);
  if(!tpdgv) return 0;
  if(TPDGV)
  {
    memcpy(tpdgv, TPDGV, len);
    free(TPDGV);
  }
  tpdgv[len] = GVAL2(day);
  tpdgv[len+1] = GVAL2(day+2);
  tpdgv[len+2] = GVAL2(day+4);
  tpdgv[len+3] = GVAL(day+6);
  tpdgv[len+4] = 0xff;
  TPDGV = tpdgv;
  database_changed = 1;
  
  TIPO_GIORNO = cmd_get_day_type(GIORNO_S - 1);
  for(i=0; i<n_FESTIVI; i++) FESTIVI[i] = cmd_get_day_type((GIORNO_S + i)%7);
  delphi_update_days();
  
  return 1;
}

