#include "user.h"
#include "master.h"
#include "command.h"
#include "badge.h"
#include "support.h"
#include "delphi.h"
#include "lara.h"
#include "version.h"
#include <unistd.h>
#include <string.h>

struct storico s;

int UserSystemStatus;

int ProdStorico(struct storico *msg)
{
  return codec_queue_event((unsigned char*)msg);
}

int ALCO(int x)
{
  if(!(CO[x] & bitAlarm)) return 0;
  CO[x] &= ~bitAlarm;
  return 1;
}

/*
	y = 0: attuatore off
	y = 1: attuatore lampeggio
	y = 3: attuatore on
*/
void ATTZI(int x, int y)
{
  switch(y)
  {
    case 0:
      if(AT[x] & (bitON | bitFlashing))
      {
        AT[x] &= ~(bitON | bitFlashing);
        AT[x] |= bitAbilReq;
      }
      break;
    case 1:
      if((AT[x] & (bitON | bitFlashing)) != (bitON | bitFlashing))
        AT[x] |= (bitON | bitFlashing | bitAbilReq);
      break;
    case 3:
      if((AT[x] & (bitON | bitFlashing)) != bitON)
      {
        AT[x] &= ~bitFlashing;
        AT[x] |= (bitON | bitAbilReq);
      }
      break;
    default:
      return;
  }
}

int tasto_SEN(int i, char t)
{
  if(t == 'T')
  {
    if(SE[(i<<3)+1] & 0x40)	// 0x40: tasto funzione TC
    {
      SE[(i<<3)+1] &= ~0x40;
      return 1;
    }
  }
  else
  {
    if(SE[(i<<3)+2] & (1<<(t-'A')))
    {
      SE[(i<<3)+2] &= ~(1<<(t-'A'));
      return 1;
    }      
  }
  
  return 0;
}

int cod_SEN(int i)
{
  int t;
  
  t = SE[(i<<3)+3];
  SE[(i<<3)+3] = 0;
  
  return t;
}

int str_SEN(int i)
{
  if(SE[(i<<3)+1] & 0x40)	// 0x40: codice numerico ricevuto
  {
    SE[(i<<3)+1] &= ~0x40;
    return 1;
  }
  
  return 0;
}

int rd_EVEN(int i)
{
  int ev;
  
  ev = EVENTO[i];
  EVENTO[i] = -1;
  
  return ev;
}

int fs(unsigned char *x)
{
  if(!(*x & bitLHF)) return 0;
  *x &= ~bitLHF;
  return 1;
}

int fd(unsigned char *x)
{
  if(!(*x & bitHLF)) return 0;
  *x &= ~bitHLF;
  return 1;
}

int fr(unsigned int *x, unsigned int bit)
{
  if(!(*x & bit)) return 0;
  *x &= ~bit;
  return 1;
}

void Doncon(int co, int val)
{
  if(CO[co] & bitAlarm) return;
  if(!(CO[co] & bitTiming))
  {
    TCO[co] = val;
    CO[co] |= bitTiming;
  }
  
  TCO[co] -= 0x1000;
  if(!(TCO[co] & 0xf000))
  {
    CO[co] |= bitAlarm | bitLHF;
    CO[co] &= ~bitTiming;
    TCO[co] = 0;
  }
}

void Doffco(int co)
{
  CO[co] &= ~(bitAlarm | bitTiming);
  TCO[co] = 0;
}

static int mess_ex2_abil(int code)
{
  int i;
  
  for(i=0; i<MAX_NUM_CONSUMER; i++)
    if(PrintEventEx2[Ex2_Delphi][i][code])
      return 1;
  return 0;
}

void Donanl(unsigned int *x)
{
  unsigned char ev[8];
  int at = x - AT;
  
  if((*x & bitOOS) || ((*x & (bitON | bitFlashing)) == bitON) ||
     (((TipoPeriferica[at>>3] < 3) || (TipoPeriferica[at>>3] == 8)) && (SE[at] & bitTest))) return;
  *x &= ~bitFlashing;
  *x |= bitON | bitLHF | bitAbilReq;
  
  if(mess_ex2_abil(Ex2_On_attuatore))
  {
    ev[0] = 247;
    ev[1] = 1;
    ev[2] = Ex2_On_attuatore;
//    *(unsigned short*)(ev+3) = at;
    inttouchar(at, ev+3, 2);
    ev[5] = 0;
    codec_queue_event(ev);
  }
}

void Donala(unsigned int *x)
{
  unsigned char ev[8];
  int at = x - AT;
  
  if((*x & (bitOOS | bitFlashing)) ||
     (((TipoPeriferica[at>>3] < 3) || (TipoPeriferica[at>>3] == 8)) && (SE[at] & bitTest))) return;
  *x |= bitON | bitLHF | bitAbilReq | bitFlashing;
  
  if(mess_ex2_abil(Ex2_On_attuatore))
  {
    ev[0] = 247;
    ev[1] = 1;
    ev[2] = Ex2_On_attuatore;
//    *(unsigned short*)(ev+3) = at;
    inttouchar(at, ev+3, 2);
    ev[5] = 1;
    codec_queue_event(ev);
  }
}

void Doffa(unsigned int *x)
{
  unsigned char ev[8];
  int at = x - AT;
  
  if(!(*x & bitON) ||
     (((TipoPeriferica[at>>3] < 3) || (TipoPeriferica[at>>3] == 8)) && (SE[at] & bitTest))) return;
  *x &= ~(bitON | bitFlashing);
  *x |= bitHLF | bitAbilReq;
  
  if(mess_ex2_abil(Ex2_Off_attuatore))
  {
    ev[0] = 247;
    ev[1] = 1;
    ev[2] = Ex2_Off_attuatore;
//    *(unsigned short*)(ev+3) = at;
    inttouchar(at, ev+3, 2);
    codec_queue_event(ev);
  }
}

void Dontim(int ti, int val)
{
  if(TM[ti] & bitTiming) return;
  TTM[ti] = val;
  TM[ti] |= bitON | bitLHF | bitTiming;
}

void Dontir(int ti, int val)
{
  TTM[ti] = val;
  TM[ti] |= bitON | bitLHF | bitTiming;
}

void Doffti(int ti)
{
  TM[ti] &= ~(bitON | bitTiming);
  TTM[ti] = 0;
}
/*
void Dontil(int ti, int val)
{
  if(TL[ti] & bitTiming) return;
  TTL[ti] = val;
  TL[ti] |= bitON | bitLHF | bitTiming;
}

void Dofftl(int ti)
{
  TL[ti] &= ~(bitON | bitTiming);
  TTL[ti] = 0;
}
*/
/**** PROCEDURE DI RIPRESA */

int REFRESH_SENSORI()
{
  return master_sensors_refresh();
}

int REFRESH_ATTUATORI()
{
  return master_actuators_refresh();
}

int REFRESH_TARATURE()
{
  return master_settings_refresh();
}

/**** PROCEDURE VARIE */

int MemorizzaChiave(int idx)
{
  if((idx < 1) || (idx > (sizeof(M_CHIAVE)/5))) return 0;
  
  memcpy(M_CHIAVE[idx], CHIAVE, 5);
  return 1;
}

int RicevutaChiave()
{
  int i;
  
  if(CHIAVE_TRASMESSA)
    for(i=0; i<(sizeof(M_CHIAVE)/5); i++)
      if(!memcmp(M_CHIAVE[i], CHIAVE, 5)) return i+1;
      
  return 0;
}

int RicevutoSegreto(unsigned short v)
{
  int i;
  
  for(i=0; i<n_SEGRETO; i++)
    if(SEGRETO[i] == v) return i+1;
  
  return 0;
}

int Periferia_completa()
{
  int i;
  
  for(i=0; i<256; i++)
  {
    if((TipoPeriferica[i] != 0xff) &&
      !(master_periph_present(i))) return 0;
  }
  
  return 1;
}

int FuoriServizioAlr(int zone)
{
  int i, found = 0;
  
  for(i=0; SE_ZONA[zone][i] != -1; i++)
    if(!(SE[SE_ZONA[zone][i]] & bitOOS) && (SE[SE_ZONA[zone][i]] & bitAlarm))
    {
      if(cmd_sensor_off(SE_ZONA[zone][i])) SE[SE_ZONA[zone][i]] |= bitOOSAZ;
      found = 1;
    }
      
  return found;
}

int InServizioAlr(int zone)
{
  int i, found = 0;
  
  for(i=0; SE_ZONA[zone][i] != -1; i++)
    if(SE[SE_ZONA[zone][i]] & bitOOSAZ)
    {
      cmd_sensor_on(SE_ZONA[zone][i]);
      found = 1;
    }
      
  return found;
}

int ChgOra(char *num)
{
  return cmd_set_time(num);
}


int ChgData(char *num)
{
  return cmd_set_date(num);
}


int ChgFasceOra(char *num)
{
  return cmd_set_timerange(GVAL2(num), num+2);
}


/**** PROCEDURE DI GESTIONE MEMORIA DI ACCETTAZIONE */

int SetMU(int x, int odt)
{
  int val, nval, mval;
  
  val = odt & 0xff;
  nval = 0;
  mval = 0;
  if(val & bitAlarm) {nval |= bitMUAlarm | bitGestAlarm; mval = bitSEmuAlarm;}
  if(val & bitSabotage) {nval |= bitMUSabotage | bitGestSabotage; mval = bitSEmuInSab;}
  if(val & bitFailure) {nval |= bitMUFailure | bitGestFailure; mval = bitSEmuInFail;}
  if(val & 0x20) nval |= bitMUNoAlarm;
  if(val & 0x40) nval |= bitMUNoSabotage;
  if(val & 0x80) nval |= bitMUNoFailure;
  nval |= (val & 0x06) << 26;
  
  if(odt & 0x100)
  {
    SE[x] &= ~(0x0fc0c000);
    SE[x] |= nval;
    SEmu[x] &= ~(bitSEmuAlarm|bitSEmuInSab|bitSEmuInFail);
    SEmu[x] |= mval;
  }
  else if(odt & 0x200)
  {
    SE[x] |= nval;
    SEmu[x] |= mval;
  }
  else if(odt & 0x400)
  {
    SE[x] &= (~(0x0fc0c000)|nval);
    SEmu[x] &= mval;
  }

  nval = 0;
  if(SE[x] & bitMUAlarm) nval |= bitAlarm;
  if(SE[x] & bitMUSabotage) nval |= bitSabotage;
  if(SE[x] & bitMUFailure) nval |= bitFailure;
  if(SE[x] & bitMUNoAlarm) nval |= 0x20;
  if(SE[x] & bitMUNoSabotage) nval |= 0x40;
  if(SE[x] & bitMUNoFailure) nval |= 0x80;
  nval |= (SE[x] & ((bitMUCounter << 1) | bitMUCounter)) >> 25;
  
  return nval & 0xff;
}

int SetMUrange(int x, int c, int val)
{
  int i, nval;
  
  nval = 0;
  if(val & bitAlarm) nval |= bitMUAlarm;
  if(val & bitSabotage) nval |= bitMUSabotage;
  if(val & bitFailure) nval |= bitMUFailure;
  if(val & 0x20) nval |= bitMUNoAlarm;
  if(val & 0x40) nval |= bitMUNoSabotage;
  if(val & 0x80) nval |= bitMUNoFailure;
  nval |= (val & 0x06) << 26;
  
  for(i=x; i<(x+c); i++)
  {
    SE[i] &= ~(0x0fc0c000);
    SE[i] |= nval;
  }
  return 0;
}


int ClrMUcnt()
{
  int i;
  
  for(i=0; i<n_SE; i++) SE[i] &= ~((bitMUCounter << 1) | bitMUCounter);
  return 0;
}

int GetMUcnt(int x)
{
  int cnt;
  
  cnt = ((SE[x] >> 26) & 0x03);
  if(cnt == 3) SE[x] &= ~((bitMUCounter << 1) | bitMUCounter);
  return cnt;
}

/**** PROCEDURE DI CONTROLLO PERIFERIA ANALOGICA (MM tipo 2) */

int RIAnalog(int i)
{
  unsigned char cmd[2];
  
  if(MMPresente[i>>5] != 0xff)
  {
    cmd[0] = 26;
    cmd[1] = i & 0x1f;
    master_queue(i>>5, cmd);
    return 0;
  }
  return -1;
}


/**** PROCEDURE DI CONTROLLO LETTORE DI BADGE (MM tipo 2) */

int ISTime(int i)
{
  unsigned char cmd[10];
  
  if(MMPresente[i>>5] != 0xff)
  {
    cmd[0] = 27;
    cmd[1] = i & 0x1f;
    cmd[2] = SECONDI;
    cmd[3] = MINUTI;
    cmd[4] = ORE;
    cmd[5] = GIORNO_S;
    cmd[6] = GIORNO;
    cmd[7] = MESE;
    cmd[8] = ANNO % 100;
    master_queue(i>>5, cmd);
    return 0;
  }

  return -1;
}

int RTime(int i)
{
  unsigned char cmd[2];
  
  if(MMPresente[i>>5] != 0xff)
  {
    cmd[0] = 28;
    cmd[1] = i & 0x1f;
    master_queue(i>>5, cmd);
    return 0;
  }

  return -1;
}

int GBadge(int i, int bdgID, int x)
{
  return badge_manage(i, bdgID, x);
}

/**** PROCEDURE DI CONTROLLO */

void SProva(int s)
{
  unsigned char ev[2];
  
  ev[0] = Stato_prova;
  ev[1] = s;
  codec_queue_event(ev);
  if(s)
  {
    database_set_alarm2(&PROVA);
  }
  else
  {
    database_reset_alarm2(&PROVA);
#if 1
    OFFRUMORE_TUTTI();
#endif
  }
}

int serial_write(int consumer, char *str, int len)
{
  if(config.consumer[consumer].dev && (config.consumer[consumer].dev->type == protGEN))
    return write(config.consumer[consumer].dev->fd, str, len);
  else
    return -1;
}

int serial_read(int consumer, char *str, int len)
{
  if(config.consumer[consumer].dev && (config.consumer[consumer].dev->type == protGEN))
    return read(config.consumer[consumer].dev->fd, str, len);
  else
    return -1;
}

void Invent(char gg, char mm, char aa)
{
  int c;
  unsigned short mlinea;
  
  if (rd_EVEN(9)!=-1)		   
  {
    s.code=185;
//    *(unsigned short*)&s.dati[0] = version_get_short();
//    *(unsigned short*)&s.dati[2] = mm | aa<<4 | gg<<8;
    inttouchar(version_get_short(), s.dati, 2);
    inttouchar(mm | aa<<4 | gg<<8, s.dati+2, 2);
    
    mlinea = 0xffff;
    for(c=0; c<(MM_NUM*32); c++)
    {
      if((TipoPeriferica[c] != 0xff) && !(StatoPeriferica[c>>3] & (1<<(c&0x07))))
        mlinea = c;
    }
    s.dati[4] = mlinea >> 8;
    s.dati[5] = mlinea & 0xff;

    ProdStorico(&s);
  }
}

void ImpostaChiave(int idx, int key)
{
  if(idx < (sizeof(RONDA)/2))
  {
    ((short*)RONDA)[idx] = key;
    database_changed = 1;
  }
}

int SystemPowerON()
{
  if(UserSystemStatus == 2)
  {
    UserSystemStatus = 0;
    return 1;
  }
  return 0;
}

int SystemRestart()
{
  if(UserSystemStatus == 1)
  {
    UserSystemStatus = 0;
    return 1;
  }
  return 0;
}

void RitardoZona(int zona, int ritardo)
{
  if((zona < 1) || (zona > n_ZS)) return;
  
  ZONA_R[zona] = ritardo * 10;
  database_changed = 1;
}

void SetVariante(int var)
{
  if(config.Variant != var)
  {
    config.Variant = var;
    delphi_save_conf();
  }
}

void SetSEinvAlm(int se)
{
  SEinvAlm[se>>3] |= (1<<(se&7));
  database_changed = 1;
}

unsigned short GiustificativoTebe(int term)
{
  if(term < LARA_N_TERMINALI+1)
    return lara_giustificativo[term];
  else
    return 0xffff;
}

