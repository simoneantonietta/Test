#include "list.h"
#include "database.h"
#include "codec.h"
#include "master.h"
#include <string.h>

#define LIST_ALL_SENS

void list_oos_sensors()
{
  unsigned char ev[4];
  int i;
  
  ev[0] = Sensore_Fuori_Servizio;
  
  for(i=0; i<n_SE; i++)
    if(SE[i] & bitOOS)
    {
      ev[1] = i >> 8;
      ev[2] = i & 0xff;
      codec_queue_event(ev);
    }
}

void list_oos_actuators()
{
  unsigned char ev[4];
  int i;
  
  ev[0] = Attuatore_Fuori_Servizio;
  
  for(i=0; i<n_AT; i++)
    if(AT[i] & bitOOS)
    {
      ev[1] = i >> 8;
      ev[2] = i & 0xff;
      codec_queue_event(ev);
    }
}

void list_status_zone()
{
  unsigned char ev[16], zone[1+n_ZS+n_ZI];
  int i, j, inlist;
  
  ev[0] = Lista_zone;
  
  memset(zone, 0, 1 + n_ZS + n_ZI);
  for(i=0; i<n_SE; i++)
    if(Zona_SE[i] != 0xff) zone[Zona_SE[i]] = 1;
  for(i=0; i<n_ZI; i++)
    if(TPDZI[i] && (TPDZI[i][0] != 0xff)) zone[n_ZS+1+i] = 1;
  if(TPDZT && (TPDZT[0] != 0xff)) zone[0] = 1;
  
  for(i=0; i<(1 + n_ZS + n_ZI); i+=8)
  {
    inlist = 0;
    for(j=i; (j<(i+8)) && (j<(1+n_ZS+n_ZI)); j++)
      inlist |= zone[j];
      
    if(inlist)
    {
      ev[1] = i;
      memcpy(ev+2, ZONA + i, 8);
      if(i > (1 + n_ZS + n_ZI - 8)) memset(ev+2 + (1 + n_ZS + n_ZI) - i, 0, 8 - (1 + n_ZS + n_ZI) + i);
      codec_queue_event(ev);
    }
  }
}

/*
static void list_status(unsigned int *per, int num, unsigned char event)
{
  unsigned char ev[16];
  int i, j;
  
  ev[0] = event;
  
  for(i=0; i<num;)
  {
#ifdef LIST_ALL_SENS
    if(TipoPeriferica[i>>3] != 0xff)
#else
    if(master_periph_present(i>>3))
#endif
    {
      ev[1] = i >> 8;
      ev[2] = i & 0xff;
      for(j=3; j<11; j++,i++) ev[j] = (unsigned char)per[i];
      codec_queue_event(ev);
    }
    else
      i += 8;
  }
}
*/

void list_status_sensors()
{
//  list_status(SE, n_SE, Lista_sensori);
  
  unsigned char ev[16];
  int i, j;
  
  ev[0] = Lista_sensori;
  
  for(i=0; i<n_SE;)
  {
#ifdef LIST_ALL_SENS
    if(TipoPeriferica[i>>3] != 0xff)
#else
    if(master_periph_present(i>>3))
#endif
    {
      ev[1] = i >> 8;
      ev[2] = i & 0xff;
      for(j=3; j<11; j++,i++)
      {
        ev[j] = (unsigned char)SE[i];
        if(SE[i] & bitMUNoAlarm)
        {
          if(SE[i] & bitGestAlarm)
            ev[j] |= bitAlarm;
          else
            ev[j] &= ~bitAlarm;
        }
        if((SE[i] & bitMUNoSabotage) && (SE[i] & bitGestSabotage)) ev[j] |= bitSabotage;
        if((SE[i] & bitMUNoFailure) && (SE[i] & bitGestFailure)) ev[j] |= bitFailure;
      }
      codec_queue_event(ev);
    }
    else
      i += 8;
  }
}

void list_status_actuators()
{
//  list_status(AT, n_AT, Lista_attuatori);
  
  unsigned char ev[16];
  int i, j;
  
  ev[0] = Lista_attuatori;
  
  for(i=0; i<n_AT;)
  {
#ifdef LIST_ALL_SENS
    if(TipoPeriferica[i>>3] != 0xff)
#else
    if(master_periph_present(i>>3))
#endif
    {
      ev[1] = i >> 8;
      ev[2] = i & 0xff;
      for(j=3; j<11; j++,i++) ev[j] = (unsigned char)AT[i];
      codec_queue_event(ev);
    }
    else
      i += 8;
  }
}

void list_calibrations()
{
  unsigned char ev[8];
  int i;

  ev[0] = Parametri_taratura;
#ifdef GEST16MM
  for(i=0; i<n_DEFPT; i++)
#else
  for(i=0; i<256; i++)
#endif
  {
    if((DEFPT[i][2] == 1) && !DEFPT[i][3] && !DEFPT[i][4])	// MM type 0 - SC1
    {
#ifdef GEST16MM
      ev[1] = i>>8;
      ev[2] = i;
      ev[3] = DEFPT[i][0];
      ev[4] = DEFPT[i][1];
#else
      ev[1] = i;
      ev[2] = DEFPT[i][0];
      ev[3] = DEFPT[i][1];
#endif
      codec_queue_event(ev);
    }
  }
}

void list_holydays()
{
  unsigned char ev[20];
  int i;
  
  ev[0] = Giorni_festivi;
  
  ev[1] = 0;
  memcpy(ev+2, FESTIVI, 16);
  codec_queue_event(ev);

  ev[1] = 1;
  memcpy(ev+2, FESTIVI+16, 16);
  codec_queue_event(ev);

  ev[1] = 2;
  ev[2] = TIPO_GIORNO;
  for(i=3;i<19;ev[i++]=0xff);
  codec_queue_event(ev);
}

void list_periph()
{
  unsigned char evs[16], evp[16];
  int j, k, z;
  
  evs[0] = Periferiche_previste;
  evp[0] = Periferiche_presenti;

  for(k=0;k<(MM_NUM*2);k++)
    if(MMPresente[k/2] != 0xff)
    {
#ifdef GEST16MM
      evs[1] = evp[1] = k>>4;
      evs[2] = evp[2] = z = k<<4;
      for (j=3;j<11;j++)
#else
      evs[1] = evp[1] = z = k<<4;
      for (j=2;j<10;j++)
#endif
      {
        evs[j] = (TipoPeriferica[z] & 0xf) | ((TipoPeriferica[z+1] & 0xf) << 4);
        evp[j] = (master_periph_present(z++)!=0?0x0E:0x0F) |
                 (master_periph_present(z++)!=0?0xE0:0xF0);
      }

      codec_queue_event(evs);
      codec_queue_event(evp);
    }
}

void list_rounds_QRA()
{
  unsigned char ev[4];
  
  ev[0] = Stato_QRA_Ronda;
  ev[1] = QRA1;
  ev[2] = QRA2;
  ev[3] = TIPO_RONDA;
  
  codec_queue_event(ev);
}

void list_rounds_list(int start)
{
  unsigned char ev[20];
  int i;
  
  ev[0] = Liste_ronda;
  
  for(i=start; i<start+6; i++)
  {
    ev[1] = i;
    memcpy(ev+2, RONDA[i], 16);
    codec_queue_event(ev);
  }
}

void list_rounds_timings(int start)
{
  unsigned char ev[20];
  unsigned char *rtim;
  int i;
  
  ev[0] = Orari_ronda;
  rtim = RONDA[12];
  
  for(i=start; i<start+6; i++)
  {
    ev[1] = i;
    memcpy(ev+2, rtim + (24*(i>>1)) + (12*(i&1)), 12);
    codec_queue_event(ev);
  }
}

void list_rounds_keys()
{
  unsigned char ev[20];
  int i, j;
  
  ev[0] = Liste_ronda;
  
//  for(i=j=0; i<((sizeof(RONDA)+sizeof(ORA_RONDA))/16); i++,j++)
#warning Quando Com90 gestirà 256 chiavi sarà da usare questo for
//  for(i=j=0; i<(sizeof(RONDA)/16); i++,j++)
  for(i=j=0; i<21; i++,j++)
  {
    ev[1] = j;
    memcpy(ev+2, RONDA[i], 16);
    codec_queue_event(ev);
  }
}

void list_daily_timings()
{
  unsigned char ev[16];
  int i;
  
  ev[0] = Fasce_orarie;
  
  for(i=0; i<n_FAG; i+=2)
  {
    if((FAGR[i][0] != 0xff) || (FAGR[i][2] != 0xff) ||
      ((i != 254) && ((FAGR[i][4] != 0xff) || (FAGR[i][6] != 0xff))))
    {
      ev[1] = i;
      memcpy(ev+2, FAGR[i], 8);
      if(i > (n_FAG - 2)) memset(ev + 6, 0xff, 4);
      codec_queue_event(ev);
    }
  }
}

void list_sensors_zone()
{
  unsigned char ev[12];
  int i, j;

  ev[0] = Lista_ZS;
  for(i=0; i<(MAX_NUM_MM << 5); i++)
  {
#ifdef LIST_ALL_SENS
    if(TipoPeriferica[i] != 0xff)
#else
    if(master_periph_present(i))
#endif
    {
      ev[1] = i >> 5;
      ev[2] = i << 3;
      for(j=0; j<8; j++) ev[j+3] = Zona_SE[(i<<3) + j];
      codec_queue_event(ev);
    }
  }
}

void list_memory_MU()
{
  unsigned char ev[24], nval;
  int i, j;

  ev[0] = Lista_MU;
  for(i=0; i<n_SE;)
  {
#ifdef LIST_ALL_SENS
    if(TipoPeriferica[i>>3] != 0xff)
#else
    if(master_periph_present(i>>3))
#endif
    {
      ev[1] = i >> 8;
      ev[2] = i & 0xff;
      for(j=0; j<8; j++, i++)
      {
        nval = 0;
        if(SE[i] & bitMUAlarm) nval |= bitAlarm;
        if(SE[i] & bitMUSabotage) nval |= bitSabotage;
        if(SE[i] & bitMUFailure) nval |= bitFailure;
        if(SE[i] & bitMUNoAlarm) nval |= 0x20;
        if(SE[i] & bitMUNoSabotage) nval |= 0x40;
        if(SE[i] & bitMUNoFailure) nval |= 0x80;
        nval |= (SE[i] & ((bitMUCounter << 1) | bitMUCounter)) >> 25;
        ev[j+3] = nval;
      }
      codec_queue_event(ev);
    }
    else
      i += 8;
  }
}

void list_status_sync()
{
  unsigned char ev[16], zone[1+n_ZS+n_ZI];
  int i, j, inlist;
  
  ev[0] = Evento_Esteso2;
  ev[1] = Ex2_Delphi;
  ev[2] = Ex2_Allineamento;
  
  /* Allineamento zone */
  ev[3] = 2;
  
  memset(zone, 0, 1 + n_ZS + n_ZI);
  for(i=0; i<n_SE; i++)
    if(Zona_SE[i] != 0xff) zone[Zona_SE[i]] = 1;
  for(i=0; i<n_ZI; i++)
    if(TPDZI[i] && (TPDZI[i][0] != 0xff)) zone[n_ZS+1+i] = 1;
  if(TPDZT && (TPDZT[0] != 0xff)) zone[0] = 1;
  
  for(i=0; i<(1+n_ZS+n_ZI);)
  {
    inlist = 0;
    for(j=i; (j<(i+8)) && (j<(1+n_ZS+n_ZI)); j++)
      inlist |= zone[j];
      
    if(inlist)
    {
//      *(short*)(ev+4) = i;
      inttouchar(i, ev+4, sizeof(short));
      for(j=6; j<14; j++,i++)
      {
        ev[j] = 0;
        if(i < (1+n_ZS+n_ZI))
        {
          if(ZONA[i] & bitAlarm) ev[j] |= bitSyncAlarm;
          if(ZONA[i] & bitSabotage) ev[j] |= bitSyncSabotage;
          if(ZONA[i] & bitFailure) ev[j] |= bitSyncFailure;
          if(ZONA[i] & bitActive) ev[j] |= bitSyncOOS;
        }
      }
      codec_queue_event(ev);
    }
    else
      i += 8;
  }
  
  /* Allineamento sensori */
  ev[3] = 1;
  
  for(i=0; i<n_SE;)
  {
#ifdef LIST_ALL_SENS
    if(TipoPeriferica[i>>3] != 0xff)
#else
    if(master_periph_present(i>>3))
#endif
    {
//      *(short*)(ev+4) = i;
      inttouchar(i, ev+4, sizeof(short));
      for(j=6; j<14; j++,i++)
      {
        ev[j] = 0;
        if(SE[i] & bitAlarm) ev[j] |= bitSyncAlarm;
        if(SE[i] & bitSabotage) ev[j] |= bitSyncSabotage;
        if(SE[i] & bitFailure) ev[j] |= bitSyncFailure;
        if(SE[i] & bitOOS) ev[j] |= bitSyncOOS;
        if(SE[i] & bitMUAlarm) ev[j] |= bitSyncMuAlarm;
        if(SE[i] & bitMUSabotage) ev[j] |= bitSyncMuSabotage;
        if(SE[i] & bitMUFailure) ev[j] |= bitSyncMuFailure;
        
        if(SE[i] & bitMUNoAlarm)
        {
          if(SE[i] & bitGestAlarm)
            ev[j] |= bitSyncAlarm;
          else
            ev[j] &= ~bitSyncAlarm;
        }
        if((SE[i] & bitMUNoSabotage) && (SE[i] & bitGestSabotage)) ev[j] |= bitSyncSabotage;
        if((SE[i] & bitMUNoFailure) && (SE[i] & bitGestFailure)) ev[j] |= bitSyncFailure;
      }
      codec_queue_event(ev);
    }
    else
      i += 8;
  }
  
  /* Allineamento attuatori */
  ev[3] = 3;
  
  for(i=0; i<n_AT;)
  {
#ifdef LIST_ALL_SENS
    if(TipoPeriferica[i>>3] != 0xff)
#else
    if(master_periph_present(i>>3))
#endif
    {
//      *(short*)(ev+4) = i;
      inttouchar(i, ev+4, sizeof(short));
      for(j=6; j<14; j++,i++)
      {
        ev[j] = 0;
        if(AT[i] & bitON) ev[j] |= bitSyncAlarm;
        if(AT[i] & bitOOS) ev[j] |= bitSyncOOS;
      }
      codec_queue_event(ev);
    }
    else
      i += 8;
  }
  
  /* Fine allineamento */
  memset(ev+3, 0, 11);
  codec_queue_event(ev);
}

void list_end()
{
  unsigned char ev;
  
  if(EndListEnable)
  {
    ev = FineInvioMemoria;
    codec_queue_event(&ev);
  }
}

