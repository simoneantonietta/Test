#include "database.h"
#include "master.h"
#include "delphi.h"
#include "support.h"
#include "codec.h"
#include "command.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

#ifdef GZIP
#include <zlib.h>
#else
#define gzopen fopen
#define gzclose fclose
#define gzread(a,b,c) fread(b,1,c,a)
#define gzwrite(a,b,c) gzwrite(fp, b,1,c,a)
#define gzputc(a,b) fputc(b,a)
#define gzgetc(a) gzgetc(a)
typedef FILE* gzFile;
#endif

unsigned int SE[n_SE];
unsigned char Zona_SE[n_SE];
unsigned char SEmu[n_SE];
unsigned char SEinvAlm[n_SE/8];
unsigned char SEanalog[n_SE];

time_t SE_time_alarm[n_SE];
time_t SE_time_failure[n_SE];
time_t SE_time_sabotage[n_SE];

char DVSSAM[2] = {0, };
char DVSSAG[2] = {0, };

char DEFPT[n_DEFPT][5];

unsigned short *TDTA[n_TDTA] = {0, };

unsigned char SA[n_SA];
char CONF_SA = 0;
char CNFG_SA[n_SA][4];
char SANanalogica[n_SA];

SogliaList *SogliaSingola = NULL;

volatile unsigned int AT[n_AT];
unsigned short *TPDAT = 0;

unsigned char ME[n_ME];
unsigned char ME2[n_ME];
int MI[n_MI];

unsigned char CO[n_CO];
unsigned short TCO[n_CO];

unsigned char TM[n_TM];
unsigned int TTM[n_TM];

unsigned char ZONA[1 + n_ZS + n_ZI];
unsigned char ZONAEX[1 + n_ZS + n_ZI];
short *SE_ZONA[1 + n_ZS + n_ZI];

unsigned char *TPDZT = 0;
unsigned char *TPDZI[n_ZI] = {0, };

unsigned char MIN_ZABIL = 0xff;
unsigned char MAX_ZABIL = 0xff;

unsigned short ZONA_R[1 + n_ZS];
unsigned short ZONA_R_timer[1 + n_ZS];
unsigned char ZONA_R_mode[1 + n_ZS];

//unsigned char FAS[n_FAS];
unsigned char *FASR[n_FAS];

unsigned char FAG[n_FAG];
unsigned char FAGR[n_FAG][4];

char SatHoliday = 0;

unsigned char FESTIVI[n_FESTIVI] = {0, };
unsigned char *TPDGF = NULL;
unsigned char *TPDGV = NULL;
unsigned char TIPO_GIORNO = 0;
unsigned char SECONDI;
unsigned char MINUTI;
unsigned char ORE;
unsigned char GIORNO_S;
unsigned char GIORNO;
unsigned char MESE;
unsigned char ANNO;
unsigned char old_ORE;
unsigned char old_GIORNO;
unsigned char old_MESE;
unsigned char old_ANNO;
unsigned char N_MINUTO = 0;
unsigned char N_ORA = 0;
unsigned char N_GIORNO = 0;
unsigned char N_MESE = 0;
unsigned char N_ANNO = 0;

short RICHIESTA_ERRATA_AT = -1;
short CHIAVE_TRASMESSA;
short CHIAVE_FALSA;
unsigned char CHIAVE[5];
unsigned char M_CHIAVE[32][5];
struct v_analog_t V_ANALOG;
struct v_oradata_t V_ORADATA;
char LS_ABILITATA[MAX_NUM_CONSUMER];
unsigned char STATO_MODEM = 0;
unsigned char NO_ATTI_ZONA = 0;
unsigned char RILEVATO_MM = 0xff;
unsigned short M_CONTENITORE = 0;
unsigned short M_LINEA = 0;
unsigned short GUASTO = 0xffff;
unsigned char SOSPESA_LINEA = 0xff;
unsigned char RIPRISTINO_LINEA = 0;

unsigned char RONDA[128][16];
unsigned int QRA1;
unsigned int QRA2;
unsigned int TIPO_RONDA;

unsigned char PROVA = 0;
unsigned char TEST_ATTIVO = 0;
unsigned short GUASTO_NOTEST = 0xffff;

unsigned char EndListEnable = 1;

unsigned short SEGRETO[n_SEGRETO];
short EVENTO[n_EVENTO];

unsigned char PrintEvent[MAX_NUM_CONSUMER][MAX_NUM_EVENT];
unsigned char PrintEventEx[MAX_NUM_CONSUMER][MAX_NUM_EVENT_EX];
unsigned char PrintEventEx2[2][MAX_NUM_CONSUMER][MAX_NUM_EVENT_EX2];

int EventNum = MAX_NUM_EVENT;
int EventNumEx = MAX_NUM_EVENT_EX;
int EventNumEx2 = MAX_NUM_EVENT_EX2;

/* In realtà basterebbe MAX_NUM_MM, ma ci sono problemi
   di compatibilità in caso di aggiornamento del saet.new,
   alla partenza segnalerebbe "saet.nv corrotto". */
unsigned char AlarmSabotage[MAX_NUM_MM * 32];
/* Questo array invece è corretto perché è un byte per periferica. */
unsigned char AlarmSabotageInv[MAX_NUM_MM * 32];

unsigned char Barcode[MAX_NUM_MM * 32][10];

Periodo periodo[32];

/**********************************/

int database_changed = 0;

static unsigned short database_terminator = 0xffff;
static pthread_mutex_t database_mutex = PTHREAD_MUTEX_INITIALIZER;

void inline database_lock()
{
  pthread_mutex_lock(&database_mutex);
}

void inline database_unlock()
{
  pthread_mutex_unlock(&database_mutex);
}

void database_save()
{
  gzFile fp;
  int i;
  unsigned char ev[4];
  unsigned short len;
  SogliaList *sl;

  fp = gzopen(ADDROOTDIR(NVTmpFileName), "w");
  if(!fp) return;
  
  database_changed = 0;
  
  ev[0] = 247;
  ev[1] = 1;
  ev[2] = Ex2_SalvataggioDB;
  codec_queue_event(ev);
  
  gzwrite(fp, "SAET10", 6);
  
  database_lock();
  
  gzputc(fp, 0);
  len = sizeof(TipoPeriferica);
  gzwrite(fp, &len, sizeof(unsigned short));
  gzwrite(fp, TipoPeriferica, len);

  gzputc(fp, 1);
  len = sizeof(DVSSAM);
  gzwrite(fp, &len, sizeof(unsigned short));
  gzwrite(fp, DVSSAM, len);
  
  gzputc(fp, 2);
  len = sizeof(DVSSAG);
  gzwrite(fp, &len, sizeof(unsigned short));
  gzwrite(fp, DVSSAG, len);
  
  gzputc(fp, 3);
  len = sizeof(DEFPT);
  gzwrite(fp, &len, sizeof(unsigned short));
  gzwrite(fp, DEFPT, len);

  for(i=0; i<n_TDTA; i++)
  {
    gzputc(fp, 4);
    len = 0;
    if(TDTA[i])
      for(; TDTA[i][len] != 0xffff; len++);
    else
      TDTA[i] = &database_terminator;
    len++;
    len *= sizeof(unsigned short);
    len++;
    gzwrite(fp, &len, sizeof(unsigned short));
    gzputc(fp, i);
    gzwrite(fp, TDTA[i], len-1);
  }

  gzputc(fp, 5);
  len = sizeof(CONF_SA);
  gzwrite(fp, &len, sizeof(unsigned short));
  gzwrite(fp, &CONF_SA, len);
  
  gzputc(fp, 6);
  len = sizeof(CNFG_SA);
  gzwrite(fp, &len, sizeof(unsigned short));
  gzwrite(fp, CNFG_SA, len);
  
  gzputc(fp, 7);
  len = 0;
  if(TPDZT)
    for(; TPDZT[len] != 0xff; len++);
  else
    TPDZT = (unsigned char*)&database_terminator;
  len++;
  len++;
  gzwrite(fp, &len, sizeof(unsigned short));
  gzputc(fp, 0);
  gzwrite(fp, TPDZT, len-1);
  
  for(i=0; i<n_ZI; i++)
  {
    gzputc(fp, 7);
    len = 0;
    if(TPDZI[i])
      for(; TPDZI[i][len] != 0xff; len++);
    else
      TPDZI[i] = (unsigned char*)&database_terminator;
    len++;
    len++;      
    gzwrite(fp, &len, sizeof(unsigned short));
    gzputc(fp, i + 1);
    gzwrite(fp, TPDZI[i], len-1);
  }
  
  gzputc(fp, 8);
  len = sizeof(Zona_SE);
  gzwrite(fp, &len, sizeof(unsigned short));
  gzwrite(fp, Zona_SE, len);

  gzputc(fp, 9);
  len = sizeof(MIN_ZABIL);
  gzwrite(fp, &len, sizeof(unsigned short));
  gzwrite(fp, &MIN_ZABIL, len);

  gzputc(fp, 10);
  len = sizeof(MAX_ZABIL);
  gzwrite(fp, &len, sizeof(unsigned short));
  gzwrite(fp, &MAX_ZABIL, len);

  gzputc(fp, 11);
  len = sizeof(FAGR);
  gzwrite(fp, &len, sizeof(unsigned short));
  gzwrite(fp, FAGR, len);
  
  for(i=0; i<n_FAS; i++)
  {
    gzputc(fp, 12);
    len = 0;
    if(FASR[i])
      for(; FASR[i][len] != 0xff; len++);
    else
      FASR[i] = (unsigned char*)&database_terminator;
    len++;
    len++;
    gzwrite(fp, &len, sizeof(unsigned short));
    gzputc(fp, i);
    gzwrite(fp, FASR[i], len-1);
  }

  gzputc(fp, 13);
  len = sizeof(SatHoliday);
  gzwrite(fp, &len, sizeof(unsigned short));
  gzwrite(fp, &SatHoliday, len);
  
  if(TPDGF)
  {
    gzputc(fp, 14);
    for(len=0; TPDGF[len]!=0xff; len++);
    len++;
    gzwrite(fp, &len, sizeof(unsigned short));
    gzwrite(fp, TPDGF, len);
  }
  
  gzputc(fp, 15);
  len = 0;
  if(TPDAT)
    for(; TPDAT[len] != 0xffff; len++);
  else
    TPDAT = &database_terminator;
  len++;
  len *= sizeof(unsigned short);
  gzwrite(fp, &len, sizeof(unsigned short));
  gzwrite(fp, TPDAT, len);
  
  gzputc(fp, 16);
  len = sizeof(EndListEnable);
  gzwrite(fp, &len, sizeof(unsigned short));
  gzwrite(fp, &EndListEnable, len);

  for(i=0; i<MAX_NUM_CONSUMER; i++)
  {
    gzputc(fp, 17);
    len = MAX_NUM_EVENT + 1;
    gzwrite(fp, &len, sizeof(unsigned short));
    gzputc(fp, i);
    gzwrite(fp, PrintEvent[i], MAX_NUM_EVENT);

    gzputc(fp, 18);
    len = MAX_NUM_EVENT_EX + 1;
    gzwrite(fp, &len, sizeof(unsigned short));
    gzputc(fp, i);
    gzwrite(fp, PrintEventEx[i], MAX_NUM_EVENT_EX);
    
    gzputc(fp, 28);
    len = MAX_NUM_EVENT_EX2 + 2;
    gzwrite(fp, &len, sizeof(unsigned short));
    gzputc(fp, 0);
    gzputc(fp, i);
    gzwrite(fp, PrintEventEx2[0][i], MAX_NUM_EVENT_EX2);
    
    gzputc(fp, 28);
    len = MAX_NUM_EVENT_EX2 + 2;
    gzwrite(fp, &len, sizeof(unsigned short));
    gzputc(fp, 1);
    gzputc(fp, i);
    gzwrite(fp, PrintEventEx2[1][i], MAX_NUM_EVENT_EX2);
  }

  gzputc(fp, 19);
  len = sizeof(AlarmSabotage);
  gzwrite(fp, &len, sizeof(unsigned short));
  gzwrite(fp, AlarmSabotage, len);

  gzputc(fp, 20);
  len = sizeof(RONDA);
// + sizeof(ORA_RONDA) + sizeof(RONDAext);
  gzwrite(fp, &len, sizeof(unsigned short));
  gzwrite(fp, RONDA, len);
  
  sl = SogliaSingola;
  while(sl)
  {
    gzputc(fp, 21);
    len = 7;
    gzwrite(fp, &len, sizeof(unsigned short));
    gzwrite(fp, sl, len);
    sl = sl->next;
  }
  
  gzputc(fp, 22);
  len = sizeof(SEGRETO);
  gzwrite(fp, &len, sizeof(unsigned short));
  gzwrite(fp, SEGRETO, len);

  for(i=0; i<32; i++)
  {
    gzputc(fp, 23);
    len = sizeof(Periodo) + 1;
    gzwrite(fp, &len, sizeof(unsigned short));
    gzputc(fp, i);
    gzwrite(fp, &(periodo[i]), sizeof(Periodo));
  }
  
  if(TPDGV)
  {
    gzputc(fp, 24);
    len = 0;
    for(len=0; TPDGV[len]!=0xff; len++);
    len++;
    gzwrite(fp, &len, sizeof(unsigned short));
    gzwrite(fp, TPDGV, len);
  }
  
  gzputc(fp, 25);
  len = sizeof(ZONA_R);
  gzwrite(fp, &len, sizeof(unsigned short));
  gzwrite(fp, ZONA_R, len);

  gzputc(fp, 26);
  len = sizeof(SANanalogica);
  gzwrite(fp, &len, sizeof(unsigned short));
  gzwrite(fp, SANanalogica, len);

  gzputc(fp, 27);
  len = sizeof(MMTexas);
  gzwrite(fp, &len, sizeof(unsigned short));
  gzwrite(fp, MMTexas, len);

  gzputc(fp, 29);
  len = sizeof(AlarmSabotageInv);
  gzwrite(fp, &len, sizeof(unsigned short));
  gzwrite(fp, AlarmSabotageInv, len);

  gzputc(fp, 30);
  len = sizeof(SEinvAlm);
  gzwrite(fp, &len, sizeof(unsigned short));
  gzwrite(fp, SEinvAlm, len);

  gzputc(fp, 31);
  len = sizeof(master_SC8R2_uniqueid);
  gzwrite(fp, &len, sizeof(unsigned short));
  gzwrite(fp, &master_SC8R2_uniqueid, len);
  
  database_unlock();
  
  gzputc(fp, 0xff);
  
  gzclose(fp);
  
  rename(ADDROOTDIR(NVTmpFileName), ADDROOTDIR(NVFileName));
  sync();
}

static void database_init()
{
  int i, j;
  
  memset(SE, 0, sizeof(SE));
  memset(Zona_SE, 0xff, sizeof(Zona_SE));
  memset(SEmu, 0, sizeof(SEmu));
  memset(AT, 0, sizeof(AT));
  memset(ME, 0, sizeof(ME));
  memset(ME2, 0, sizeof(ME2));
  memset(MI, 0, sizeof(MI));
  memset(CO, 0, sizeof(CO));
  memset(TCO, 0, sizeof(TCO));
  memset(DEFPT, 0, sizeof(DEFPT));
  memset(SA, 0, sizeof(SA));
  memset(CNFG_SA, 0, sizeof(CNFG_SA));
  memset(FASR, 0, sizeof(FASR));
  memset(FAG, 0, sizeof(FAG));
  memset(FAGR, 0xff, sizeof(FAGR));
  memset(TM, 0, sizeof(TM));
  memset(TTM, 0, sizeof(TTM));
  memset(ZONA, 0, sizeof(ZONA));
  memset(ZONAEX, 0, sizeof(ZONAEX));
  memset(SE_ZONA, 0, sizeof(SE_ZONA));
  memset(ZONA_R, 0, sizeof(ZONA_R));
  memset(ZONA_R_timer, 0, sizeof(ZONA_R_timer));
  memset(M_CHIAVE, 0, sizeof(M_CHIAVE));
  memset(SEGRETO, 0xff, sizeof(SEGRETO));
  memset(EVENTO, 0xff, sizeof(EVENTO));
  memset(RONDA, 0, sizeof(RONDA));
//  memset(ORA_RONDA, 0, sizeof(ORA_RONDA));
//  memset(RONDAext, 0, sizeof(RONDAext));
  memset(AlarmSabotage, 0, sizeof(AlarmSabotage));
  memset(AlarmSabotageInv, 0, sizeof(AlarmSabotageInv));
  memset(periodo, 0, sizeof(periodo));
  memset(SANanalogica, 0, sizeof(SANanalogica));
  memset(MMTexas, 0, sizeof(MMTexas));
  memset(SEinvAlm, 0, sizeof(SEinvAlm));
  
  V_ANALOG.ind = -1;
  
  support_load_status();
  
  memset(LS_ABILITATA, 0, sizeof(LS_ABILITATA));
  memset(PrintEvent, 1, sizeof(PrintEvent));
  memset(PrintEventEx, 1, sizeof(PrintEventEx));
  memset(PrintEventEx2, 1, sizeof(PrintEventEx2));
  
  for(i=0; i<MAX_NUM_CONSUMER; i++)
  {
    /* Eventi di porta aperta normalmente disattivi */
    PrintEventEx2[Ex2_Lara][i][67] = 0;
    PrintEventEx2[Ex2_Lara][i][68] = 0;
    
    for(j=95; j<101; j++)
      PrintEventEx2[Ex2_Delphi][i][j] = 0;
  }
}

void database_calc_zones()
{
  int n, num[1+n_ZS+n_ZI];
  unsigned char *list;
  
  memset(num, 0, sizeof(num));
  for(n=0; n<n_SE; n++) if(Zona_SE[n] < (1+n_ZS)) num[Zona_SE[n]]++;
  for(n=1; n<=n_ZS; n++)
  {
    free(SE_ZONA[n]);
    SE_ZONA[n] = malloc((num[n]+1)*sizeof(short));
    SE_ZONA[n][num[n]] = -1;
    num[n] = 0;
  }
  for(n=0; n<n_SE; n++)
    if(Zona_SE[n] < (1+n_ZS))
    {
      SE_ZONA[Zona_SE[n]][num[Zona_SE[n]]] = n;
      num[Zona_SE[n]]++;
    }
  for(n=0; n<n_ZI; n++)
  {
    list = TPDZI[n];
    while(*list != 0xff)
    {
      num[n+n_ZS+1] += num[*list];
      list++;
    }
    free(SE_ZONA[n+n_ZS+1]);
    SE_ZONA[n+n_ZS+1] = malloc((num[n+n_ZS+1]+1)*sizeof(short));
    SE_ZONA[n+n_ZS+1][num[n+n_ZS+1]] = -1;
    list = TPDZI[n];
    num[n+n_ZS+1] = 0;
    while(*list != 0xff)
    {
      memcpy(&(SE_ZONA[n+n_ZS+1][num[n+n_ZS+1]]), SE_ZONA[*list], num[*list]*sizeof(short));
      num[n+n_ZS+1] += num[*list];
      list++;
    }
  }
  list = TPDZT;
  while(*list != 0xff)
  {
    num[0] += num[*list];
    list++;
  }
  free(SE_ZONA[0]);
  SE_ZONA[0] = malloc((num[0]+1)*sizeof(short));
  SE_ZONA[0][num[0]] = -1;
  list = TPDZT;
  num[0] = 0;
  while(*list != 0xff)
  {
    memcpy(&(SE_ZONA[0][num[0]]), SE_ZONA[*list], num[*list]*sizeof(short));
    num[0] += num[*list];
    list++;
  }
}

int database_load()
{
  gzFile fp;
  int c, n, error; //, num[1+n_ZS+n_ZI];
  short int len;
  void *p;
  char buf[8];
//  unsigned char *list;

  database_init();
  
  fp = gzopen(ADDROOTDIR(NVFileName), "r");
  if(!fp)
  {
    database_save();
    fp = gzopen(ADDROOTDIR(NVFileName), "r");
    if(!fp) return -1;
  }
  gzread(fp, buf, 6);
  if(strncmp(buf, "SAET10", 6))	// SAET v1.0
  {
    gzclose(fp);
    return -1;
  }
  
  database_lock();
  
  error = 0;
  p = NULL;
  
  while(!error && ((c=gzgetc(fp)) != EOF))
  {
    if(c == 255) break;
    
    n = gzread(fp, &len, sizeof(len));
    if(n != sizeof(len))
    {
      error = 1;
      break;
    }
    
    switch(c)
    {
      case 0:
        p = TipoPeriferica;
        if(len > sizeof(TipoPeriferica)) error = 1;
        break;
      case 1:
        p = DVSSAM;
        if(len > sizeof(DVSSAM)) error = 1;
        break;
      case 2:
        p = DVSSAG;
        if(len > sizeof(DVSSAG)) error = 1;
        break;
      case 3:
        p = DEFPT;
        if(len > sizeof(DEFPT)) error = 1;
        break;
      case 4:
        c = gzgetc(fp);
        if(c < n_TDTA)
        {
          p = malloc(--len);
          TDTA[c] = p;
        }
        else
          error = 1;
        break;
      case 5:
        p = &CONF_SA;
        if(len > sizeof(CONF_SA)) error = 1;
        break;
      case 6:
        p = CNFG_SA;
        if(len > sizeof(CNFG_SA)) error = 1;
        break;
      case 7:
        c = gzgetc(fp);
        if(c <= n_ZI)
        {
          p = malloc(--len);
          if(!c)
            TPDZT = p;
          else
            TPDZI[c-1] = p;
        }
        else
          error = 1;
        break;
      case 8:
        p = Zona_SE;
        if(len > sizeof(Zona_SE)) error = 1;
        break;
      case 9:
        p = &MIN_ZABIL;
        if(len > sizeof(MIN_ZABIL)) error = 1;
        break;
      case 10:
        p = &MAX_ZABIL;
        if(len > sizeof(MAX_ZABIL)) error = 1;
        break;
      case 11:
        p = FAGR;
        if(len > sizeof(FAGR)) error = 1;
        break;
      case 12:
        c = gzgetc(fp);
        if(c < n_FAS)
        {
          p = malloc(--len);
          FASR[c] = p;
        }
        else
          error = 1;
        break;
      case 13:
        p = &SatHoliday;
        if(len > sizeof(SatHoliday)) error = 1;
        break;
      case 14:
        p = malloc(len);
        TPDGF = p;
        break;
      case 15:
        p = malloc(len);
        TPDAT = p;
        break;
      case 16:
        p = &EndListEnable;
        if(len > sizeof(EndListEnable)) error = 1;
        break;
      case 17:
        c = gzgetc(fp);
        if(c < MAX_NUM_CONSUMER)
        {
          len--;
          p = PrintEvent[c];
          if(len > MAX_NUM_EVENT) error = 1;
        }
        else
          error = 1;
        break;
      case 18:
        c = gzgetc(fp);
        if(c < MAX_NUM_CONSUMER)
        {
          len--;
          p = PrintEventEx[c];
          if(len > MAX_NUM_EVENT_EX) error = 1;
        }
        else
          error = 1;
        break;
      case 19:
        p = AlarmSabotage;
        if(len > sizeof(AlarmSabotage)) error = 1;
        break;
      case 20:
        p = RONDA;
//        if(len > (sizeof(RONDA) + sizeof(ORA_RONDA) + sizeof(RONDAext))) error = 1;
        if(len > sizeof(RONDA)) error = 1;
        break;
      case 21:
        p = malloc(sizeof(SogliaList));
        ((SogliaList*)p)->next = SogliaSingola;
        SogliaSingola = (SogliaList*)p;
        if(len > 7) error = 1;
        break;
      case 22:
        p = SEGRETO;
        if(len > sizeof(SEGRETO)) error = 1;
        break;
      case 23:
        c = gzgetc(fp);
        if(c < 32)
        {
          len--;
          p = &(periodo[c]);
          if(len > sizeof(Periodo)) error = 1;
        }
        else
          error = 1;
        break;
      case 24:
        p = malloc(len);
        TPDGV = p;
        break;
      case 25:
        p = ZONA_R;
        break;
      case 26:
        p = SANanalogica;
        break;
      case 27:
        p = MMTexas;
        break;
      case 28:
        n = gzgetc(fp);
        c = gzgetc(fp);
        if(c < MAX_NUM_CONSUMER)
        {
          len-=2;
          p = PrintEventEx2[n][c];
          if(len > MAX_NUM_EVENT_EX2) error = 1;
        }
        else
          error = 1;
        break;
      case 29:
        p = AlarmSabotageInv;
        if(len > sizeof(AlarmSabotageInv)) error = 1;
        break;
      case 30:
        p = SEinvAlm;
        if(len > sizeof(SEinvAlm)) error = 1;
        break;
      case 31:
        p = &master_SC8R2_uniqueid;
        if(len > sizeof(master_SC8R2_uniqueid)) error = 1;
        break;
      default:
        /* skip */
        gzseek(fp, len, SEEK_CUR);
        continue;
    }
    
    if(!error)
    {
      if(p)
      {
        n = gzread(fp, p, len);
        if(len && (n != len)) error = 1;
        
        if((p == TDTA[c]) && (TDTA[c][(len>>1)-1] != 0xffff))
        {
          free(TDTA[c]);
          TDTA[c] = NULL;
          error = 1;
        }
        if((p == TPDZT) && (TPDZT[len-1] != 0xff))
        {
          free(TPDZT);
          TPDZT = NULL;
          error = 1;
        }
        if((p == TPDZI[c]) && (TPDZI[c][len-1] != 0xff))
        {
          free(TPDZI[c]);
          TPDZI[c] = NULL;
          error = 1;
        }
        if((p == TPDGF) && (TPDGF[len-1] != 0xff))
        {
          free(TPDGF);
          TPDGF = NULL;
          error = 1;
        }
        if((p == TPDAT) && (TPDAT[(len>>1)-1] != 0xffff))
        {
          free(TPDAT);
          TPDAT = NULL;
          error = 1;
        }
        if((p == FASR[c]) && (FASR[c][len-1] != 0xff))
        {
          free(FASR[c]);
          FASR[c] = NULL;
          error = 1;
        }
        if((p == TPDGV) && (TPDGV[len-1] != 0xff))
        {
          free(TPDGV);
          TPDGV = NULL;
          error = 1;
        }
      }
      else
        error = 1;
    }
  }
  
  for(n=0; n<n_TDTA; n++) if(!TDTA[n]) TDTA[n] = &database_terminator;
  if(!TPDZT) TPDZT = (unsigned char*)&database_terminator;
  for(n=0; n<n_ZI; n++) if(!TPDZI[n]) TPDZI[n] = (unsigned char*)&database_terminator;
  if(!TPDAT) TPDAT = &database_terminator;
  
  /* Costruisco la lista dei sensori appartenenti alle varie zone */
  database_calc_zones();
  /*
  memset(num, 0, sizeof(num));
  for(n=0; n<n_SE; n++) if(Zona_SE[n] < (1+n_ZS)) num[Zona_SE[n]]++;
  for(n=1; n<=n_ZS; n++)
  {
    SE_ZONA[n] = malloc((num[n]+1)*sizeof(short));
    SE_ZONA[n][num[n]] = -1;
    num[n] = 0;
  }
  for(n=0; n<n_SE; n++)
    if(Zona_SE[n] < (1+n_ZS))
    {
      SE_ZONA[Zona_SE[n]][num[Zona_SE[n]]] = n;
      num[Zona_SE[n]]++;
    }
  for(n=0; n<n_ZI; n++)
  {
    list = TPDZI[n];
    while(*list != 0xff)
    {
      num[n+n_ZS+1] += num[*list];
      list++;
    }
    SE_ZONA[n+n_ZS+1] = malloc((num[n+n_ZS+1]+1)*sizeof(short));
    SE_ZONA[n+n_ZS+1][num[n+n_ZS+1]] = -1;
    list = TPDZI[n];
    num[n+n_ZS+1] = 0;
    while(*list != 0xff)
    {
      memcpy(&(SE_ZONA[n+n_ZS+1][num[n+n_ZS+1]]), SE_ZONA[*list], num[*list]*sizeof(short));
      num[n+n_ZS+1] += num[*list];
      list++;
    }
  }
  list = TPDZT;
  while(*list != 0xff)
  {
    num[0] += num[*list];
    list++;
  }
  SE_ZONA[0] = malloc((num[0]+1)*sizeof(short));
  SE_ZONA[0][num[0]] = -1;
  list = TPDZT;
  num[0] = 0;
  while(*list != 0xff)
  {
    memcpy(&(SE_ZONA[0][num[0]]), SE_ZONA[*list], num[*list]*sizeof(short));
    num[0] += num[*list];
    list++;
  }
  */
  
  TIPO_GIORNO = cmd_get_day_type(GIORNO_S-1);
  for(n=0; n<n_FESTIVI; n++) FESTIVI[n] = cmd_get_day_type((GIORNO_S + n)%7);
  delphi_update_days();

  for(n=0; n<MAX_NUM_CONSUMER; n++)
  {
    /* Eventi di porta aperta normalmente disattivi */
    PrintEventEx2[Ex2_Lara][n][67] = 0;
    PrintEventEx2[Ex2_Lara][n][68] = 0;
  }
  
  database_unlock();
  gzclose(fp);
  
  if(error)
  {
    support_log("Errore caricamento saet.nv");
    Boot1Error[0] = Evento_Esteso;
    Boot1Error[1] = Ex_Stringa;
    codec_queue_event(Boot1Error);
  }
  
  return -error;  
}

void database_set_alarm(unsigned int *sens)
{
  if((*sens)&bitAlarm) return;
  *sens |= bitAlarm | bitLHF;
}

void database_set_alarm2(unsigned char *sens)
{
  if((*sens)&bitAlarm) return;
  // azzero tutto tranne il fronte di discesa
  *sens &= bitHLF;
  // allarme e fronte di salita
  *sens |= bitAlarm | bitLHF;
}

void database_reset_alarm(unsigned int *sens)
{
  if(!((*sens)&bitAlarm)) return;
  *sens &= ~bitAlarm;
  *sens |= bitHLF;
}

void database_reset_alarm2(unsigned char *sens)
{
  if(!((*sens)&bitAlarm)) return;
  // azzero tutto tranne il fronte di salita
  *sens &= bitLHF;
  // fronte di discesa
  *sens |= bitHLF;
}

int database_testandset_MU(int index, unsigned int bit)
{
  if(SE[index]&bit) return 0;
  SE[index] |= bit;
  return 1;
}

