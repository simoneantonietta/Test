#include "ronda.h"
#include "support.h"
#include "database.h"
#include "timeout.h"
#include "command.h"
#include "user.h"
#include "master.h"
#include <stdio.h>
#include <string.h>

/* Se definito impone la modalita' di visualizzazione ronda come sensori,
   il valore e' la zona di appartenenza dei sensori+2 attivi in ronda */
//#define RONDA_GESTIONE_SENSORE	200

static const char *RondaFile = "/saet/data/ronda.conf";

Ronda_percorso_t Ronda_percorso[RONDA_NUM][RONDA_STAZIONI];
Ronda_orario_t Ronda_orario[3][RONDA_ORARI];
unsigned char Ronda_orario_alarm[3][RONDA_ORARI];
unsigned short Ronda_stazione[100];
int Ronda_zonafiltro;

static int Ronda_timer_zona_disattivata[1+n_ZS+n_ZI];

static int ronda_chiave = 0;
static int ronda_percorso = -1;
static int ronda_percorso_manuale = -1;
static int ronda_tappa_percorso;
static short ronda_stato[40];
static int ronda_stato_idx_in = 0;
static int ronda_stato_idx_out = 0;
static int ronda_contatore_fuori_tempo;
static timeout_t *ronda_timeout = NULL;
static timeout_t *ronda_tempo_min = NULL;
static timeout_t *ronda_save_timeout = NULL;
static timeout_t *ronda_timer_timeout = NULL;

void ronda_timer(void *null1, int null2);
int ronda_chiamata();
int ronda_evento();

static void ronda_push_evento(short evento)
{
  ronda_stato[ronda_stato_idx_in++] = evento;
  ronda_stato_idx_in %= 40;
}

void ronda_load()
{
  FILE *fp;
  char buf[512], *tmp;
  int i, s, p, v, x;
  
  memset(Ronda_percorso, 0xff, sizeof(Ronda_percorso));
  memset(Ronda_orario, 0xff, sizeof(Ronda_orario));
  memset(Ronda_orario_alarm, 0, sizeof(Ronda_orario_alarm));
  memset(Ronda_stazione, 0xff, sizeof(Ronda_stazione));
  Ronda_zonafiltro = -1;
  
  fp = fopen(RondaFile, "r");
  if(fp)
  {
  
  while(fgets(buf, sizeof(buf), fp))
  {
    if((buf[0] == '#') || (buf[0] == '\r') || (buf[0] == '\n')) continue;
    if((sscanf(buf, "Station%d=%d", &i, &v) == 2) && (i < 100)) Ronda_stazione[i] = v;
    if((sscanf(buf, "Route%d[%d]=", &i, &s) == 2) && (i < RONDA_NUM) && (s < RONDA_STAZIONI))
    {
      for(x=7;buf[x]!='=';x++);
      tmp = support_delim(buf + x + 1);
      if(tmp)
      {
        p = v = -1;
        sscanf(tmp, "%d,%d", &p, &v);
        Ronda_percorso[i][s].stazione = p;
        Ronda_percorso[i][s].tempo = v;
      }
      for(x=0; x<4; x++)
      {
        tmp = support_delim(NULL);
        if(tmp)
        {
          p = v = -1;
          sscanf(tmp, "%d,%d", &p, &v);
          Ronda_percorso[i][s].disattivazione[x].zona = p;
          Ronda_percorso[i][s].disattivazione[x].tempo = v;
        }
      }
      for(x=0; x<4; x++)
      {
        tmp = support_delim(NULL);
        if(tmp)
        {
          p = -1;
          sscanf(tmp, "%d", &p);
          Ronda_percorso[i][s].attivazione[x].zona = p;
        }
      }
    }
    if((sscanf(buf, "StartTime%d[%d]=%d:%d;%d", &i, &p, &v, &x, &s) == 5) && (i < 3) && (p < RONDA_ORARI))
    {
      Ronda_orario[i][p].ora = v % 24;
      Ronda_orario[i][p].minuti = x % 60;
      Ronda_orario[i][p].percorso = s % RONDA_NUM;
    }
    if((sscanf(buf, "FilterZone=%d", &v) == 1) && (v < (1+n_ZS+n_ZI))) Ronda_zonafiltro = v;
  }
  
  fclose(fp);
  }
  
  ronda_timeout = timeout_init();
  ronda_tempo_min = timeout_init();
  ronda_save_timeout = timeout_init();
  ronda_timer_timeout = timeout_init();
  timeout_on(ronda_timer_timeout, ronda_timer, NULL, 0, 10);
}

void ronda_save_func(void *null1, int null2)
{
  FILE *fp;
  int i, j, x;
  
  fp = fopen(RondaFile, "w");
  if(!fp) return;
  
  for(i=0; i<100; i++)
  {
    if(Ronda_stazione[i] != 0xffff)
      fprintf(fp, "Station%d=%d\n", i, Ronda_stazione[i]);
  }
  for(i=0; i<RONDA_NUM; i++)
  {
    for(j=0; (j<RONDA_STAZIONI) && (Ronda_percorso[i][j].stazione!=0xff); j++)
    {
      fprintf(fp, "Route%d[%d]=%d,%d", i, j, Ronda_percorso[i][j].stazione, Ronda_percorso[i][j].tempo);
      for(x=0; x<4; x++)
      {
        fprintf(fp, ";");
        if(Ronda_percorso[i][j].disattivazione[x].zona != 0xff) fprintf(fp, "%d,%d",
          Ronda_percorso[i][j].disattivazione[x].zona, Ronda_percorso[i][j].disattivazione[x].tempo);
      }
      for(x=0; x<4; x++)
      {
        fprintf(fp, ";");
        if(Ronda_percorso[i][j].attivazione[x].zona != 0xff) fprintf(fp, "%d",
          Ronda_percorso[i][j].attivazione[x].zona);
      }
      fprintf(fp, "\n");
    }
  }
  for(i=0; i<3; i++)
  {
    for(j=0; j<RONDA_ORARI; j++)
    {
      if(Ronda_orario[i][j].ora!=0xff) fprintf(fp, "StartTime%d[%d]=%d:%d;%d\n", i, j,
        Ronda_orario[i][j].ora, Ronda_orario[i][j].minuti, Ronda_orario[i][j].percorso);
    }
  }
  
  if(Ronda_zonafiltro >= 0)
    fprintf(fp, "FilterZone=%d\n", Ronda_zonafiltro);
  
  fclose(fp);
}

void ronda_save()
{
  timeout_on(ronda_save_timeout, ronda_save_func, NULL, 0, 10);
}

void ronda_timer(void *null1, int null2)
{
  int i;
  unsigned char ev[4];
  
  for(i=0; i<(1+n_ZS+n_ZI); i++)
  {
    if(Ronda_timer_zona_disattivata[i] > 0)
    {
      Ronda_timer_zona_disattivata[i]--;
      if(!Ronda_timer_zona_disattivata[i])
        cmd_zone_on(i, 1);
    }
  }
  
  if(ronda_timeout)	/* ronda configurata */
  {
    ev[0] = Segnalazione_evento;
    ronda_chiamata();
    do
    {
      i = ronda_evento();
      if(i >= 0)
      {
        ev[1] = i >> 8;
        ev[2] = i & 0xff;
        codec_queue_event(ev);
      }
    }
    while(i >= 0);
  }
  
  timeout_on(ronda_timer_timeout, ronda_timer, NULL, 0, 10);
}

void ronda_mancata_partenza(void *prima_stazione, int timeout)
{
  ronda_push_evento(RONDA_EVENTO_MANCATA_PARTENZA);
//  master_set_alarm((Ronda_stazione[(int)prima_stazione]<<3)+2, 1, 1);
//  master_set_alarm((Ronda_stazione[(int)prima_stazione]<<3)+2, 0, 1);
  FSA((Ronda_stazione[(int)prima_stazione]<<3)+6, 1);
  timeout_on(ronda_timeout, ronda_mancata_partenza, NULL, timeout, timeout * 10);
}

void ronda_mancata_punzonatura(void *null, int stazione)
{
  if(!ronda_contatore_fuori_tempo)
    ronda_push_evento(RONDA_EVENTO_MANCATA_PUNZON + stazione);
  else
    ronda_push_evento(RONDA_EVENTO_RIP_MANC_PUNZON + stazione);
  ronda_contatore_fuori_tempo++;
  master_set_alarm((Ronda_stazione[stazione]<<3)+2, 1, 1);
  master_set_alarm((Ronda_stazione[stazione]<<3)+2, 0, 1);
  timeout_on(ronda_timeout, ronda_mancata_punzonatura, NULL, stazione,
    Ronda_percorso[ronda_percorso][ronda_tappa_percorso].tempo * 10);
}

void ronda_led(void *null, int stazione)
{
  ONAT((Ronda_stazione[stazione]<<3) + RONDA_LED_TEMPO_OK, 1);
}

/* ritorna -1 se non ci sono ronde da chiamare o il numero della ronda da chiamare (0-9) */
int ronda_chiamata()
{
  int i, j;
  
  for(j=0; j<3; j++)
    for(i=0; i<RONDA_ORARI; i++)
    {
      if((Ronda_orario_alarm[j][i] == RONDA_CONCLUSA) &&
         (Ronda_orario[j][i].minuti != MINUTI))
        {
          Ronda_orario_alarm[j][i] = RONDA_CHIUSA;
        }
    }
    
  if(ronda_percorso != -1) return -1;
  if((Ronda_zonafiltro >= 0) && !(ZONA[Ronda_zonafiltro] & bitActive)) return -1;
  
  for(i=0; i<RONDA_ORARI; i++)
  {
    if((Ronda_orario[TIPO_GIORNO][i].ora == ORE) && (Ronda_orario[TIPO_GIORNO][i].minuti == MINUTI))
    {
      if(Ronda_orario_alarm[TIPO_GIORNO][i] == RONDA_CHIUSA)
      {
        Ronda_orario_alarm[TIPO_GIORNO][i] = RONDA_CHIAMATA;
        ronda_tappa_percorso = -1;
        ronda_contatore_fuori_tempo = 0;
        timeout_on(ronda_timeout, ronda_mancata_partenza,
          (void*)((int)Ronda_percorso[Ronda_orario[TIPO_GIORNO][i].percorso][0].stazione),
          Ronda_percorso[Ronda_orario[TIPO_GIORNO][i].percorso][0].tempo,
          Ronda_percorso[Ronda_orario[TIPO_GIORNO][i].percorso][0].tempo * 10);
        ronda_push_evento(RONDA_EVENTO_RICHIESTA_GIRO + Ronda_orario[TIPO_GIORNO][i].percorso);
        ONAT(1, 1);
        ONME(595 + Ronda_orario[TIPO_GIORNO][i].percorso, 1);
#ifdef RONDA_GESTIONE_SENSORE
{
  int s;
  for(s=0; Ronda_percorso[Ronda_orario[TIPO_GIORNO][i].percorso][s].stazione!=0xff; s++)
    Zona_SE[(Ronda_stazione[Ronda_percorso[Ronda_orario[TIPO_GIORNO][i].percorso][s].stazione]<<3)+2] = RONDA_GESTIONE_SENSORE;
  cmd_zone_on(RONDA_GESTIONE_SENSORE, 0);
}
#endif
        return Ronda_orario[TIPO_GIORNO][i].percorso;
      }
    }
  }
  
  return -1;
}

int ronda_partenza(int percorso)
{
  int i, j;
  
  if((percorso < 0) || (percorso >= RONDA_NUM)) return 0;
  
  if(ronda_percorso_manuale != percorso)
  {
    for(j=0; j<3; j++)
    {
      for(i=0; i<RONDA_ORARI; i++)
      {
        if((Ronda_orario_alarm[j][i] == RONDA_CHIAMATA) &&
           (Ronda_orario[j][i].percorso == percorso))
        {
          Ronda_orario_alarm[j][i] = RONDA_IN_CORSO;
          break;
        }
      }
      if(i != RONDA_ORARI) break;
    }
    if(j == 3) return 0;
  }
  
  OFFA(1, 1);
  
  /* attivazioni */
  for(i=0; i<4; i++)
  {
    if(Ronda_percorso[percorso][0].attivazione[i].zona != 0xff)
      cmd_zone_on(Ronda_percorso[percorso][0].attivazione[i].zona, 1);
  }
  
  /* disattivazioni */
  for(i=0; i<4; i++)
  {
    if(Ronda_percorso[percorso][0].disattivazione[i].zona != 0xff)
    {
      cmd_zone_off(Ronda_percorso[percorso][0].disattivazione[i].zona, 1);
      Ronda_timer_zona_disattivata[Ronda_percorso[percorso][0].disattivazione[i].zona] =
        Ronda_percorso[percorso][0].disattivazione[i].tempo;
    }
  }
  
  ronda_percorso = percorso;
  ronda_tappa_percorso = 1;
  
  timeout_on(ronda_timeout, ronda_mancata_punzonatura, NULL,
    Ronda_percorso[percorso][1].stazione,
    Ronda_percorso[percorso][1].tempo * 10);
  timeout_on(ronda_tempo_min, ronda_led, NULL,
    Ronda_percorso[percorso][1].stazione, RONDA_TEMPO_MIN);
  
  ronda_push_evento(RONDA_EVENTO_INIZIO_GIRO);
  return 1;
}

int ronda_chiudi(int percorso)
{
  int i, j;
  
  if((percorso < 0) || (percorso >= RONDA_NUM)) return 0;

  if(ronda_percorso == -1)
  {
    if(ronda_percorso_manuale >= 0)
      ronda_percorso = ronda_percorso_manuale;
    else
    {
      for(j=0; (j<3)&&(ronda_percorso==-1); j++)
        for(i=0; i<RONDA_ORARI; i++)
          if(Ronda_orario_alarm[j][i] == RONDA_CHIAMATA)
          {
            ronda_percorso = Ronda_orario[j][i].percorso;
            i = RONDA_ORARI;
          }
      if(ronda_percorso == -1) return 0;
    }
  }
  
  if(percorso == ronda_percorso)
  {
    timeout_off(ronda_timeout);
    timeout_off(ronda_tempo_min);
    if((ronda_tappa_percorso > 0) && (Ronda_percorso[ronda_percorso][ronda_tappa_percorso].stazione != 0xff))
      OFFA((Ronda_stazione[Ronda_percorso[ronda_percorso][ronda_tappa_percorso].stazione]<<3) + RONDA_LED_TEMPO_OK, 1);
    if(ronda_percorso_manuale == -1)
    {
      for(j=0; j<3; j++)
        for(i=0; i<RONDA_ORARI; i++)
          if(((Ronda_orario_alarm[j][i] == RONDA_CHIAMATA) ||
              (Ronda_orario_alarm[j][i] == RONDA_IN_CORSO)) &&
             (Ronda_orario[j][i].percorso == ronda_percorso))
            Ronda_orario_alarm[j][i] = RONDA_CONCLUSA;
    }
    for(i=0; i<(1+n_ZS+n_ZI); i++)
    {
      if(Ronda_timer_zona_disattivata[i])
      {
        cmd_zone_on(i, 1);
        Ronda_timer_zona_disattivata[i] = 0;
      }
    }
    
    if(ronda_tappa_percorso < 1) ronda_tappa_percorso = 1;
    for(i=ronda_tappa_percorso; (i<(RONDA_STAZIONI-1))&&(Ronda_percorso[percorso][i+1].stazione!=0xff); i++)
    {
      ronda_push_evento(RONDA_EVENTO_MANCATA_PUNZON + Ronda_percorso[percorso][i].stazione);
      master_set_alarm((Ronda_stazione[Ronda_percorso[percorso][i].stazione]<<3)+2, 1, 1);
      master_set_alarm((Ronda_stazione[Ronda_percorso[percorso][i].stazione]<<3)+2, 0, 1);
    }
    if(Ronda_percorso[percorso][i].stazione != 0xff)
      ronda_push_evento(RONDA_EVENTO_PUNZONATURA + Ronda_percorso[percorso][i].stazione);
      
    ronda_push_evento(RONDA_EVENTO_FINE_GIRO);
    ronda_percorso = -1;
    ronda_percorso_manuale = -1;
    
    /* disattivo l'attuatore in caso di rifiuto della chiamata */
    OFFA(1, 1);
#ifdef RONDA_GESTIONE_SENSORE
    for(i=0; i<100 ; i++)
      if(Ronda_stazione[i] != 0xffff)
      {
        cmd_zone_off(RONDA_GESTIONE_SENSORE, 0);
        Zona_SE[(Ronda_stazione[i]<<3)+2] = 0xff;
        cmd_sensor_on((Ronda_stazione[i]<<3)+2);
      }
#endif
    
    return 1;
  }
  else
    return 0;
}

void ronda_set_chiave(int codice)
{
  ronda_chiave = codice;
}

void ronda_punzonatura(int codice, int periferica)
{
  int i, j, p, stazione;
  
  if(!ronda_tempo_min || ronda_tempo_min->active) return;
  
  for(i=0;(i<100)&&(Ronda_stazione[i]!=periferica);i++);
  if(i==100)
    return;
  else
    stazione = i;
  
  if(codice == ronda_chiave)
  {
    if(ronda_percorso >= 0)
    {
      if(Ronda_percorso[ronda_percorso][ronda_tappa_percorso].stazione == stazione)
      {
        /* punzonatura */
        OFFA((Ronda_stazione[Ronda_percorso[ronda_percorso][ronda_tappa_percorso].stazione]<<3) + RONDA_LED_TEMPO_OK, 1);
#ifdef RONDA_GESTIONE_SENSORE
        cmd_sensor_off((Ronda_stazione[Ronda_percorso[ronda_percorso][ronda_tappa_percorso].stazione]<<3) + 2);
#endif
        if(ronda_contatore_fuori_tempo)
        {
          ronda_push_evento(RONDA_EVENTO_FUORI_TEMPO + stazione);
          master_set_alarm((periferica<<3)+5, 1, 1);
          master_set_alarm((periferica<<3)+5, 0, 1);
        }
        else
          ronda_push_evento(RONDA_EVENTO_PUNZONATURA + stazione);
        ronda_contatore_fuori_tempo = 0;
        
        /* attivazioni */
        for(i=0; i<4; i++)
        {
          if(Ronda_percorso[ronda_percorso][ronda_tappa_percorso].attivazione[i].zona != 0xff)
          {
            cmd_zone_on(Ronda_percorso[ronda_percorso][ronda_tappa_percorso].attivazione[i].zona, 1);
            Ronda_timer_zona_disattivata[Ronda_percorso[ronda_percorso][ronda_tappa_percorso].attivazione[i].zona] = 0;
          }
        }
        
        /* disattivazioni */
        for(i=0; i<4; i++)
        {
          if(Ronda_percorso[ronda_percorso][ronda_tappa_percorso].disattivazione[i].zona != 0xff)
          {
            cmd_zone_off(Ronda_percorso[ronda_percorso][ronda_tappa_percorso].disattivazione[i].zona, 1);
            Ronda_timer_zona_disattivata[Ronda_percorso[ronda_percorso][ronda_tappa_percorso].disattivazione[i].zona] =
              Ronda_percorso[ronda_percorso][ronda_tappa_percorso].disattivazione[i].tempo;
          }
        }
        
        ronda_tappa_percorso++;
        if((ronda_tappa_percorso == RONDA_STAZIONI) ||
           (Ronda_percorso[ronda_percorso][ronda_tappa_percorso].stazione == 0xff))
        {
          /* fine ronda */
          ronda_chiudi(ronda_percorso);
        }
        else
        {
          timeout_on(ronda_timeout, ronda_mancata_punzonatura, NULL,
            Ronda_percorso[ronda_percorso][ronda_tappa_percorso].stazione,
            Ronda_percorso[ronda_percorso][ronda_tappa_percorso].tempo * 10);
          timeout_on(ronda_tempo_min, ronda_led, NULL,
            Ronda_percorso[ronda_percorso][ronda_tappa_percorso].stazione, RONDA_TEMPO_MIN);
        }
      }
      else
      {
        /* fuori sequenza */
        for(i=0; (i<RONDA_STAZIONI)&&(Ronda_percorso[ronda_percorso][i].stazione!=0xff); i++);
        if(Ronda_percorso[ronda_percorso][--i].stazione == stazione)
        {
          /* fine ronda */
          ronda_chiudi(ronda_percorso);
        }
        else
        {
          ronda_push_evento(RONDA_EVENTO_FUORI_SEQUENZA + stazione);
          master_set_alarm((periferica<<3)+6, 1, 1);
          master_set_alarm((periferica<<3)+6, 0, 1);
        }
      }
    }
    else
    {
      if(ronda_percorso_manuale >= 0)
      {
        for(i=0; (i<RONDA_STAZIONI)&&(Ronda_percorso[ronda_percorso_manuale][i].stazione!=0xff); i++);
        if(Ronda_percorso[ronda_percorso_manuale][--i].stazione == stazione)
        {
          /* annullo giro */
          ronda_tappa_percorso = 1;
          ronda_chiudi(ronda_percorso_manuale);
        }
      }
      for(j=0; j<3; j++)
        for(i=0; i<RONDA_ORARI; i++)
          if(Ronda_orario_alarm[TIPO_GIORNO][i] == RONDA_CHIAMATA)
          {
            p = Ronda_orario[TIPO_GIORNO][i].percorso;
            for(i=0; (i<RONDA_STAZIONI)&&(Ronda_percorso[p][i].stazione!=0xff); i++);
            if(Ronda_percorso[p][--i].stazione == stazione)
            {
              /* annullo giro */
              ronda_tappa_percorso = 1;
              ronda_chiudi(p);
            }
            i = RONDA_ORARI;
            j = 3;
          }
    }
  }
  else
    ronda_push_evento(RONDA_EVENTO_CHIAVE_FALSA + stazione);
}

int ronda_evento()
{
  int s;
  
  if(ronda_stato_idx_in != ronda_stato_idx_out)
  {
    s = ronda_stato[ronda_stato_idx_out++];
    ronda_stato_idx_out %= 40;
    return s;
  }
  else
    return RONDA_EVENTO_NONE;
}

int ronda_sos(int allarme, int periferica)
{
  int i;
  
  /* se non e' una periferica di ronda non emette l'SOS */
  for(i=0;(i<100)&&(Ronda_stazione[i]!=periferica);i++);
  if(i==100) return 0;
  
  /* se la ronda non e' partita non emette l'SOS */
  if((ronda_percorso < 0) || !(allarme & 0x01)) return 1;
  
  master_set_alarm((periferica<<3)+0, 1, 1);
  master_set_alarm((periferica<<3)+0, 0, 1);
  
  /* segnala la richiesta di SOS */
  ronda_push_evento(RONDA_EVENTO_SOS + i);
  return 1;
}

int ronda_chiave_falsa(int periferica)
{
  int i;
  
  /* se non e' una periferica di ronda gestisce la chiave falsa anti-intrusione */
  for(i=0;(i<100)&&(Ronda_stazione[i]!=periferica);i++);
  if(i==100) return 0;
  /* se la ronda non e' partita non segnala la chiave falsa */
  if(ronda_percorso < 0) return 1;
  /* segnala la chiave falsa */
  ronda_push_evento(RONDA_EVENTO_CHIAVE_FALSA + i);
  master_set_alarm((periferica<<3)+7, 1, 1);
  master_set_alarm((periferica<<3)+7, 0, 1);
  return 1;
}

int ronda_in_corso()
{
  return ronda_percorso;
}

int ronda_partenza_manuale(int percorso)
{
  if((percorso < 0) || (percorso >= RONDA_NUM) || (Ronda_percorso[percorso][0].stazione == 0xff)) return 0;
  ronda_tappa_percorso = -1;
  ronda_contatore_fuori_tempo = 0;
  ronda_percorso_manuale = percorso;
  timeout_on(ronda_timeout, ronda_mancata_partenza, NULL,
    Ronda_percorso[percorso][0].tempo, Ronda_percorso[percorso][0].tempo * 10);
  ronda_push_evento(RONDA_EVENTO_RICHIESTA_GIRO + percorso);
  ONAT(1, 1);
  ONME(595 + percorso, 1);
#ifdef RONDA_GESTIONE_SENSORE
{
  int s;
  for(s=0; Ronda_percorso[percorso][s].stazione!=0xff; s++)
    Zona_SE[(Ronda_stazione[Ronda_percorso[percorso][s].stazione]<<3)+2] = RONDA_GESTIONE_SENSORE;
  cmd_zone_on(RONDA_GESTIONE_SENSORE, 0);
}
#endif
  return 1;
}

void ronda_chiama(int chiama)
{
  int stazione;
  
  if(chiama)
  {
    for(stazione=0; (stazione < 100) && (Ronda_stazione[stazione] != 0xffff); stazione++)
      ONAT((Ronda_stazione[stazione]<<3) + RONDA_LED_CHIAMATA, 1);
  }
  else
  {
    for(stazione=0; (stazione < 100) && (Ronda_stazione[stazione] != 0xffff); stazione++)
      OFFA((Ronda_stazione[stazione]<<3) + RONDA_LED_CHIAMATA, 1);
  }
}

void _init()
{
  printf("Ronda (plugin): " __DATE__ " " __TIME__ "\n");
  ronda_punzonatura_p = ronda_punzonatura;
  ronda_sos_p = ronda_sos;
  ronda_chiave_falsa_p = ronda_chiave_falsa;
  ronda_partenza_p = ronda_partenza;
  ronda_partenza_manuale_p = ronda_partenza_manuale;
  ronda_chiudi_p = ronda_chiudi;
  ronda_save_p = ronda_save;

  Ronda_percorso_p = (void*)Ronda_percorso;
  Ronda_orario_p = (void*)Ronda_orario;
  Ronda_stazione_p = (void*)Ronda_stazione;
  Ronda_zonafiltro_p = (void*)&Ronda_zonafiltro;
  
  ronda_load();
}

