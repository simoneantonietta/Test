#include "lara.h"
#include "tebe.h"
#include "master.h"
#include "codec.h"
#include "command.h"
#include "database.h"
#include "support.h"
#include "finger.h"
#include "volto.h"
#include "delphi.h"
#include <stdlib.h>
#include <stdio.h>
#include <byteswap.h>
#include <limits.h>
#include <string.h>

#ifdef TEBESA
void tebe_load_config();
void tebe_term_init();
#endif

//#define DEBUG

#define SC8 2
#define TLB 7

int lara_NumBadge = 0;

static int lara_restart = 0;
static pthread_mutex_t lara_can_save = PTHREAD_MUTEX_INITIALIZER;

const char LaraFile[] = DATA_DIR "/lara.gz";
const char LaraFileTemp[] = DATA_DIR "/lara.tmp";
const char LaraFileTmp[] = "/tmp/lara.gz";
//static const char ref_LaraFileName[] = "/saet/saet.ref/lara.gz";

pthread_mutex_t laragz_mutex = PTHREAD_MUTEX_INITIALIZER;
lara_Config *_lara = NULL;
lara_TessereEx *_laraex = NULL;
unsigned char _lara_ingressi[64];
/* Questo array slega l'associazione tra i terminali e la posizione fisica del modulo master */
unsigned char lara_terminal_prog[64];
/* Contatori per ritardare la segnalazione della forzatura porta. */
unsigned int lara_forzatura_timer[64];

lara_Flags *_laraf = NULL;

static int lara_save_pending = 0;
static int list_nodes = 0;
static int lara_last_idx;

unsigned short lara_giustificativo[LARA_N_TERMINALI+1];
Lara_timbrato lara_timbrato[(LARA_N_TERMINALI+1)*2];

static void* lara_save_thread(void *null)
{
  gzFile *fp;
    
  pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);

#ifdef DEBUG
#warning !!!!!!!!!!!!!!!!!!!!!!
printf("\nSalvataggio in corso...\n\n");
#endif

  fp = gzopen(ADDROOTDIR(LaraFileTemp), "w");
  if(fp)
  {
    lara_save_pending = 0;
    gzwrite(fp, "TEBE", 4);
    gzwrite(fp, &lara_NumBadge, 4);
    gzwrite(fp, _lara, sizeof(lara_Config) - sizeof(lara_Tessere*));
    gzwrite(fp, _lara->tessera, lara_NumBadge * sizeof(lara_Tessere));
    gzwrite(fp, _laraex, lara_NumBadge * sizeof(lara_TessereEx));
    gzclose(fp);
    rename(LaraFileTemp, LaraFile);
    pthread_mutex_unlock(&lara_can_save);
  }
  
  return NULL;
}

static void lara_save_file()
{
  pthread_t pth;
  
  if(_lara && lara_save_pending && !pthread_mutex_trylock(&lara_can_save))
  {
    support_log("Salvataggio LARA");
    if(!pthread_create(&pth, NULL, lara_save_thread, NULL))
      pthread_detach(pth);
    else
    {
      lara_save_pending = 0;
      pthread_mutex_unlock(&lara_can_save);
    }
  }
}

void lara_save(int now)
{
  if((now == 2) && !lara_save_pending) return;
  lara_save_pending = 1;
  if(now) lara_save_file();
}

int lara_get_id_data(int id, int *secret, unsigned char *badge, int *group, int *stato)
{
  if(_lara && (id < lara_NumBadge))
  {
    if((_lara->tessera[id].stato.s.abil == BADGE_VUOTO) ||
      (_lara->tessera[id].stato.s.abil == BADGE_CANC))
    {
      *secret = 0;
      memset(badge, 0, 10);
      *group = 0;
      *stato = 0;
    }
    else
    {
      *secret = _lara->tessera[id].pincnt;
      memcpy(badge, _lara->tessera[id].badge, 10);
      *group = _lara->tessera[id].p.s.profilo;
      *stato = (_lara->tessera[id].stato.s.badge << 2) | (_lara->tessera[id].area?1:0) |
        (_lara->tessera[id].stato.s.abil == BADGE_ABIL?2:0);
      return id;
    }
  }

  return -1;
}

int lara_get_id_free()
{
  int idx;
  
  if(_lara)
  {
    for(idx=5; (idx<lara_NumBadge) &&
      ((_lara->tessera[idx].stato.s.abil == BADGE_ABIL) || (_lara->tessera[idx].stato.s.abil == BADGE_DISABIL)); idx++);
    if(idx < lara_NumBadge) return idx;
  }
  
  return -1;
}

int lara_load_badge(int id, unsigned char* badge, int secret, int profilo, int stato)
{
  if(_lara && (id < lara_NumBadge))
  {
    memcpy(_lara->tessera[id].badge, badge, 10);
    _lara->tessera[id].pincnt = secret;
    _lara->tessera[id].stato.s.badge = stato >> 2;
    _lara->tessera[id].p.s.profilo = profilo;
    _lara->tessera[id].area = stato & 0x01;
    _lara->tessera[id].stato.s.abil = (stato&0x02)?BADGE_ABIL:BADGE_DISABIL;
    _laraex[id].group = 0;
    _laraf->tessera[id].ag = 1;	// invalida
    _laraf->tessera_area_last_update[id] = time(NULL);
    lara_save(0);
    return id;
  }
  
  return -1;
}

void lara_set_abil(int id, int abil)
{
  unsigned char ev[8];
  
  if(_lara && (id < lara_NumBadge))
  {
    if((abil == BADGE_CANC) || (abil == BADGE_VUOTO))
    {
      /* Cancellazione anagrafica volto */
      volto_param req;
      req.tipo = VOLTO_DELETE;
      req.anagrafica.id = id;
      volto_req(&req);
      
      memset(_lara->tessera[id].badge, 0, 10);
      _lara->tessera[id].pincnt = 0;
      
      finger_delete(id);	// cancella le impronte associate alla tessera
    }
    else if((_lara->tessera[id].stato.s.abil == BADGE_CANC) ||
            (_lara->tessera[id].stato.s.abil == BADGE_VUOTO))
    {
      /* Non devo abilitare/disabilitare una tessera che non esiste */
      return;
    }
    
    _lara->tessera[id].stato.s.abil = abil;
    _laraf->tessera[id].ag = 1;	// invalida
    
    ev[0] = Evento_Esteso2;
    ev[1] = Ex2_Lara;
    if(abil == BADGE_ABIL)
      ev[2] = 33;
    else if(abil == BADGE_CANC)
    {
      ev[2] = 41;
      ev[5] = _lara->tessera[id].stato.b;
    }
    else
      ev[2] = 32;
    ev[3] = id & 0xff;
    ev[4] = id >> 8;
    codec_queue_event(ev);
    
    lara_save(0);
  }
}

void lara_set_area_counters(int mm, int area1, int area2)
{
    if(area1 < LARA_N_AREE)
    {
      _lara->presenza[area1].contatore--;
      if(area1 > 0)
      {
        database_lock();
        master_set_alarm((mm << 8) + area1 + 128,
          (_lara->presenza[area1].contatore>0)?0:1, 1);
        database_unlock();
      }
    }
    
    if(area2 < LARA_N_AREE)
    {
      _lara->presenza[area2].contatore++;
      if(area2 > 0)
      {
        database_lock();
        master_set_alarm((mm << 8) + area2 + 128, 0, 1);
        database_unlock();
      }
    }
}

void lara_set_area_group(int id, int area, int mm, time_t timeset)
{
  int i;
  unsigned char ev[8];
  
  if(_laraex[id].group)
  {
    for(i=1; i<lara_NumBadge; i++)
    {
      /* Per tutte le tessere appartenenti allo stesso gruppo dell'id
         indicato (quindi nella ricerca salto me stesso), imposto la
         nuova area per allineare il gruppo. */
      if((_laraex[i].group == _laraex[id].group) && (i != id))
      {
        lara_set_area_counters(mm, _lara->tessera[i].area, area);
        _lara->tessera[i].area = area;
        _laraf->tessera_area_last_update[i] = timeset;
        _laraf->tessera[i].ag = 1;	// invalida
        
        /* Segnalo una forzatura area per le tessere. */
        ev[0] = Evento_Esteso2;
        ev[1] = Ex2_Lara;
        ev[2] = 36;	// ID forzato in area
        ev[3] = i & 0xff;
        ev[4] = i >> 8;
        ev[5] = area;
        codec_queue_event(ev);
      }
    }
  }
}

void lara_set_area(int id, int area)
{
  unsigned char ev[8];
  int mm;
  
  if(_lara && (id < lara_NumBadge))
  {
    for(mm=0; (mm<MM_NUM) && (MMPresente[mm]!=MM_LARA); mm++);
    mm += 2;
    
    ev[0] = Evento_Esteso2;
    ev[1] = Ex2_Lara;
    ev[2] = 36;
    ev[3] = id & 0xff;
    ev[4] = id >> 8;
    ev[5] = area;
    codec_queue_event(ev);
    
#if 0
    if(_lara->tessera[id].area < LARA_N_AREE)
    {
      _lara->presenza[_lara->tessera[id].area].contatore--;
      if(_lara->tessera[id].area > 0)
      {
        database_lock();
        master_set_alarm((mm << 8) + _lara->tessera[id].area + 128,
          (_lara->presenza[_lara->tessera[id].area].contatore>0)?0:1, 1);
        database_unlock();
      }
    }
    
    _lara->tessera[id].area = area;
    _laraf->tessera_area_last_update[id] = time(NULL);
    
    if(_lara->tessera[id].area < LARA_N_AREE)
    {
      _lara->presenza[_lara->tessera[id].area].contatore++;
      if(_lara->tessera[id].area > 0)
      {
        database_lock();
        master_set_alarm((mm << 8) + _lara->tessera[id].area + 128, 0, 1);
        database_unlock();
      }
    }
#else
    lara_set_area_counters(mm, _lara->tessera[id].area, area);
    _lara->tessera[id].area = area;
    _laraf->tessera_area_last_update[id] = time(NULL);
    lara_set_area_group(id, area, mm, _laraf->tessera_area_last_update[id]);
#endif
    
    _laraf->tessera[id].ag = 1;	// invalida
    
    lara_save(0);
    
    /* Se forzo la tessera in area 0 libero anche le eventuali associazioni di scorta. */
    if(area == 0)
      tebe_escort_link(id, TEBE_ESCORT_UNLINK, 1);
  }
}

static int lara_queue(int mm, lara_tMsg *cmd)
{
#ifdef DEBUG
#warning !!!!!!!!!!!!!!!!!!!!!!
int i;
for(i=0; i<cmd->Length+5; i++)
printf("%02x", ((unsigned char*)cmd)[i]);
printf("\n");
#endif
  return master_queue(mm, (unsigned char*)cmd);
}

static void lara_badge_to_nibble(unsigned char *badge, unsigned char *nibblebadge)
{
  int i;

  memset(nibblebadge, 0xff, 10);
  
  for(i=0; badge[i] && (i<BADGE_LENGTH); i++)
    nibblebadge[i>>1] = (i & 1)?((nibblebadge[i>>1] & 0xf0) | (badge[i] & 0x0f)):((nibblebadge[i>>1] & 0x0f) | (badge[i] << 4));
}

static int lara_find_badge(unsigned char *badge, int direct)
{
  int i;
  unsigned char nibblebadge[10];
  
  if(!_lara) return -1;
  
  if(!direct)
    lara_badge_to_nibble(badge, nibblebadge);
  else
    memcpy(nibblebadge, badge, 10);
  
  for(i=0; i<lara_NumBadge; i++)
    if(((_lara->tessera[i].stato.s.abil == BADGE_ABIL) || (_lara->tessera[i].stato.s.abil == BADGE_DISABIL))
       && (_lara->tessera[i].area != TEBE_AREASELF)
       && !memcmp(nibblebadge, _lara->tessera[i].badge, 10)) return i;
  
  return -1;
}

static __inline void lara_nibble_to_badge(unsigned char *badge, unsigned char *nibblebadge)
{
  int i;
  
  for(i=0; i<10; i++)
  {
    nibblebadge[i<<1] = ((badge[i] >> 4) & 0x0f) + '0';
    nibblebadge[(i<<1)+1] = (badge[i] & 0x0f) + '0';
  }
}

static int lara_find_areaself_template(unsigned char *badge)
{
  int i, j, k, tmpl;
  unsigned char nibblebadge[20];
  
  if(!_lara) return -1;
  
  tmpl = 0;
  
  for(i=0; i<lara_NumBadge; i++)
    if(((_lara->tessera[i].stato.s.abil == BADGE_ABIL) || (_lara->tessera[i].stato.s.abil == BADGE_DISABIL))
       && (_lara->tessera[i].area == TEBE_AREASELF))
    {
      tmpl++;
      lara_nibble_to_badge(_lara->tessera[i].badge, nibblebadge);
      for(k=0; (k<BADGE_LENGTH) && (nibblebadge[k]=='='); k++);
      for(j=k; (j<BADGE_LENGTH) && (nibblebadge[j]<='9'); j++);
      /* Se le cifre significative corrispondono, il template è giusto. */
      if(!memcmp(badge+k, nibblebadge+k, j-k)) return i;
    }
  
  /* Se non ho trovato nessun template, verifico il default. */
  if(!tmpl)
  {
    /* Se la stringa inizia con una cifra numerica, allora è valido. */
    if(badge[0] <= '9') return 0;
  }
  
  return -1;
}

static int lara_test_abil_term(int idx, int node)
{
  time_t t, tf, tv;
  struct tm df;
  int i, fest, fo, t1, t2;
  
  /* ID 0000 sempre abilitato */
  if(idx == 0) return 0;
  
//  fest = *(int*)(_lara->terminale[node].fest);
  fest = uchartoint(_lara->terminale[node].fest);
  if(fest)
  {
    memset(&df, 0, sizeof(struct tm));
    df.tm_mday = GIORNO;
    df.tm_mon = MESE-1;
    df.tm_year = 100;
    tf = mktime(&df);
    df.tm_year = ANNO+100;
    tv = mktime(&df);
    
    for(i=0; i<LARA_N_FESTIVI; i++)
    {
      if((fest & 1) && _lara->festivo[i].tipofestivo)
      {
        df.tm_mday = _lara->festivo[i].periodo.inizio[0];
        df.tm_mon = _lara->festivo[i].periodo.inizio[1] - 1;
        if(!(_lara->festivo[i].tipofestivo & 0x04))
          df.tm_year = _lara->festivo[i].periodo.inizio[2] + 100;
        else
          df.tm_year = 100;
        t = mktime(&df);
        
        if(_lara->festivo[i].tipofestivo & 1)	// periodo
        {
          if(((_lara->festivo[i].tipofestivo & 0x04) && (t <= tf)) ||
             (!(_lara->festivo[i].tipofestivo & 0x04) && (t <= tv)))
          {
            df.tm_mday = _lara->festivo[i].periodo.fine[0];
            df.tm_mon = _lara->festivo[i].periodo.fine[1] - 1;
            if(!(_lara->festivo[i].tipofestivo & 0x04))
              df.tm_year = _lara->festivo[i].periodo.fine[2] + 100;
            t = mktime(&df);
            
            if(((_lara->festivo[i].tipofestivo & 0x04) && (t >= tf)) ||
               (!(_lara->festivo[i].tipofestivo & 0x04) && (t >= tv)))
            {
              return 14;
            }
          }
        }
        else	// singolo
        {
          if(((_lara->festivo[i].tipofestivo & 0x04) && (t == tf)) ||
             (!(_lara->festivo[i].tipofestivo & 0x04) && (t == tv)))
          {
              return 14;
          }
        }
      }
      fest >>= 1;
    }
  }
  
  if(_lara->terminale[node].fosett != 0)
  {
    fo = ORE * 60 + MINUTI;
    i = _lara->terminale[node].fosett - 1;
    
    if((_lara->fasciaoraria[i].fascia[GIORNO_S-1][0][0] == 0xff) &&
       (_lara->fasciaoraria[i].fascia[GIORNO_S-1][0][2] == 0xff) &&
       (_lara->fasciaoraria[i].fascia[GIORNO_S-1][1][0] == 0xff) &&
       (_lara->fasciaoraria[i].fascia[GIORNO_S-1][1][2] == 0xff))
      return 0;
    
    t1 = _lara->fasciaoraria[i].fascia[GIORNO_S-1][0][0] * 60 +
         _lara->fasciaoraria[i].fascia[GIORNO_S-1][0][1];
    if(t1 >= (0xff*60)) t1 = 0;
    t2 = _lara->fasciaoraria[i].fascia[GIORNO_S-1][0][2] * 60 +
         _lara->fasciaoraria[i].fascia[GIORNO_S-1][0][3];
    if(t2 >= (0xff*60)) t2 = INT_MAX;
    if(((t1 == 0) && (t2 == INT_MAX)) || (fo < t1) || (fo >= t2))
    {
      t1 = _lara->fasciaoraria[i].fascia[GIORNO_S-1][1][0] * 60 +
           _lara->fasciaoraria[i].fascia[GIORNO_S-1][1][1];
      if(t1 >= (0xff*60)) t1 = 0;
      t2 = _lara->fasciaoraria[i].fascia[GIORNO_S-1][1][2] * 60 +
           _lara->fasciaoraria[i].fascia[GIORNO_S-1][1][3];
      if(t2 >= (0xff*60)) t2 = INT_MAX;
      if(((t1 == 0) && (t2 == INT_MAX)) || (fo < t1) || (fo >= t2))
      {
        return 13;
      }
    }
  }
  
  return 0;
}

static int lara_test_abil(int idx, int node)
{
  time_t t, tf, tv;
  struct tm df;
  int i, fest, fo, t1, t2;
  unsigned char ev[8];
  int p, ret;
  
  /* ID 0000 sempre abilitato */
  if(idx == 0) return 0;
  
  ev[0] = Evento_Esteso2;
  ev[1] = Ex2_Lara;
  ev[3] = idx & 0xff;
  ev[4] = idx >> 8;
  ev[5] = node;
  
  if(_lara->tessera[idx].stato.s.abil == BADGE_DISABIL)
  {
    ev[2] = 12;
    codec_queue_event(ev);
    return ID_ZONA_9;
  }
  
  node--;
  
  if(_lara->tessera[idx].p.s.profilo == 0)
  {
    if((_lara->tessera[idx].p.s.term[node/8] & (1<<(node%8))) == 0)
    {
      ev[2] = 12;
      codec_queue_event(ev);
      return ID_NO_JOIN_LETT;
    }
    
//    fest = *(int*)(_lara->tessera[idx].p.s.fest);
    fest = uchartoint(_lara->tessera[idx].p.s.fest);
    if(fest)
    {
      memset(&df, 0, sizeof(struct tm));
      df.tm_mday = GIORNO;
      df.tm_mon = MESE-1;
      df.tm_year = 100;
      tf = mktime(&df);
      df.tm_year = ANNO+100;
      tv = mktime(&df);
      
      for(i=0; i<LARA_N_FESTIVI; i++)
      {
        if((fest & 1) && _lara->festivo[i].tipofestivo)
        {
          df.tm_mday = _lara->festivo[i].periodo.inizio[0];
          df.tm_mon = _lara->festivo[i].periodo.inizio[1] - 1;
          if(!(_lara->festivo[i].tipofestivo & 0x04))
            df.tm_year = _lara->festivo[i].periodo.inizio[2] + 100;
          else
            df.tm_year = 100;
          t = mktime(&df);
          
          if(_lara->festivo[i].tipofestivo & 1)	// periodo
          {
            if(((_lara->festivo[i].tipofestivo & 0x04) && (t <= tf)) ||
               (!(_lara->festivo[i].tipofestivo & 0x04) && (t <= tv)))
            {
              df.tm_mday = _lara->festivo[i].periodo.fine[0];
              df.tm_mon = _lara->festivo[i].periodo.fine[1] - 1;
              if(!(_lara->festivo[i].tipofestivo & 0x04))
                df.tm_year = _lara->festivo[i].periodo.fine[2] + 100;
              t = mktime(&df);

              if(((_lara->festivo[i].tipofestivo & 0x04) && (t >= tf)) ||
                 (!(_lara->festivo[i].tipofestivo & 0x04) && (t >= tv)))
              {
                ev[2] = 14;
                codec_queue_event(ev);
                return ID_DISAB;
              }
            }
          }
          else	// singolo
          {
            if(((_lara->festivo[i].tipofestivo & 0x04) && (t == tf)) ||
               (!(_lara->festivo[i].tipofestivo & 0x04) && (t == tv)))
            {
              ev[2] = 14;
              codec_queue_event(ev);
              return ID_DISAB;
            }
          }
        }
        fest >>= 1;
      }
    }
    
    if(_lara->tessera[idx].p.s.fosett != 0)
    {
      fo = ORE * 60 + MINUTI;
      i = _lara->tessera[idx].p.s.fosett - 1;
      
      if((_lara->fasciaoraria[i].fascia[GIORNO_S-1][0][0] != 0xff) ||
         (_lara->fasciaoraria[i].fascia[GIORNO_S-1][0][2] != 0xff) ||
         (_lara->fasciaoraria[i].fascia[GIORNO_S-1][1][0] != 0xff) ||
         (_lara->fasciaoraria[i].fascia[GIORNO_S-1][1][2] != 0xff))
      {
        t1 = _lara->fasciaoraria[i].fascia[GIORNO_S-1][0][0] * 60 +
             _lara->fasciaoraria[i].fascia[GIORNO_S-1][0][1];
        if(t1 >= (0xff*60)) t1 = 0;
        t2 = _lara->fasciaoraria[i].fascia[GIORNO_S-1][0][2] * 60 +
             _lara->fasciaoraria[i].fascia[GIORNO_S-1][0][3];
        if(t2 >= (0xff*60)) t2 = INT_MAX;
//printf("=1=> %d %d %d\n", fo, t1, t2);
        if(((t1 == 0) && (t2 == INT_MAX)) || (fo < t1) || (fo >= t2))
        {
          t1 = _lara->fasciaoraria[i].fascia[GIORNO_S-1][1][0] * 60 +
             _lara->fasciaoraria[i].fascia[GIORNO_S-1][1][1];
          if(t1 >= (0xff*60)) t1 = 0;
          t2 = _lara->fasciaoraria[i].fascia[GIORNO_S-1][1][2] * 60 +
             _lara->fasciaoraria[i].fascia[GIORNO_S-1][1][3];
          if(t2 >= (0xff*60)) t2 = INT_MAX;
//printf("=2=> %d %d %d\n", fo, t1, t2);
          if(((t1 == 0) && (t2 == INT_MAX)) || (fo < t1) || (fo >= t2))
          {
            ev[2] = 13;
            codec_queue_event(ev);
            return ID_DISAB;
          }
        }
      }
    }
    
    if((i = lara_test_abil_term(idx, node)))
    {
      ev[2] = i;
      codec_queue_event(ev);
      return ID_DISAB;
    }
  }
  else
  {
    ret = 0;
    ev[2] = 0;
    
    for(p=0; p<LARA_MULTIPROFILI; p++)
    {
      if(!_lara->tessera[idx].p.profilo[p])
      {
        p = LARA_MULTIPROFILI;
        break;
      }
      
    if((_lara->profilo[_lara->tessera[idx].p.profilo[p]-1].term[node/8] & (1<<(node%8))) == 0)
    {
      if(!ret)
      {
        ev[2] = 15;
        ev[5] = _lara->tessera[idx].p.profilo[p];
        ev[6] = node+1;
        ret = ID_NO_JOIN_LETT;
      }
      continue;	// prova il profilo successivo
    }

//    fest = *(int*)(_lara->profilo[_lara->tessera[idx].p.profilo[p]-1].fest);
    fest = uchartoint(_lara->profilo[_lara->tessera[idx].p.profilo[p]-1].fest);
    if(fest)
    {
      memset(&df, 0, sizeof(struct tm));
      df.tm_mday = GIORNO;
      df.tm_mon = MESE-1;
      df.tm_year = 100;
      tf = mktime(&df);
      df.tm_year = ANNO+100;
      tv = mktime(&df);
      
      for(i=0; i<LARA_N_FESTIVI; i++)
      {
        if((fest & 1) && _lara->festivo[i].tipofestivo)
        {
          df.tm_mday = _lara->festivo[i].periodo.inizio[0];
          df.tm_mon = _lara->festivo[i].periodo.inizio[1] - 1;
          if(!(_lara->festivo[i].tipofestivo & 0x04))
            df.tm_year = _lara->festivo[i].periodo.inizio[2] + 100;
          else
            df.tm_year = 100;
          t = mktime(&df);
          
          if(_lara->festivo[i].tipofestivo & 1)	// periodo
          {
            if(((_lara->festivo[i].tipofestivo & 0x04) && (t <= tf)) ||
               (!(_lara->festivo[i].tipofestivo & 0x04) && (t <= tv)))
            {
              df.tm_mday = _lara->festivo[i].periodo.fine[0];
              df.tm_mon = _lara->festivo[i].periodo.fine[1] - 1;
              if(!(_lara->festivo[i].tipofestivo & 0x04))
                df.tm_year = _lara->festivo[i].periodo.fine[2] + 100;
              t = mktime(&df);

              if(((_lara->festivo[i].tipofestivo & 0x04) && (t >= tf)) ||
                 (!(_lara->festivo[i].tipofestivo & 0x04) && (t >= tv)))
              {
//                if(!ret)
                {
                  ev[2] = 14;
                  ret = ID_DISAB;
                }
                
                i = LARA_N_FESTIVI;
              }
            }
          }
          else	// singolo
          {
            if(((_lara->festivo[i].tipofestivo & 0x04) && (t == tf)) ||
               (!(_lara->festivo[i].tipofestivo & 0x04) && (t == tv)))
            {
//              if(!ret)
              {
                ev[2] = 14;
                ret = ID_DISAB;
              }
              
              i = LARA_N_FESTIVI;
            }
          }
        }
        fest >>= 1;
      }
      
      if(i > LARA_N_FESTIVI)
        continue;	// prova il profilo successivo
    }
    
    if(_lara->profilo[_lara->tessera[idx].p.profilo[p]-1].fosett != 0)
    {
      fo = ORE * 60 + MINUTI;
      i = _lara->profilo[_lara->tessera[idx].p.profilo[p]-1].fosett - 1;
      
      if((_lara->fasciaoraria[i].fascia[GIORNO_S-1][0][0] != 0xff) ||
         (_lara->fasciaoraria[i].fascia[GIORNO_S-1][0][2] != 0xff) ||
         (_lara->fasciaoraria[i].fascia[GIORNO_S-1][1][0] != 0xff) ||
         (_lara->fasciaoraria[i].fascia[GIORNO_S-1][1][2] != 0xff))
      {
        t1 = _lara->fasciaoraria[i].fascia[GIORNO_S-1][0][0] * 60 +
             _lara->fasciaoraria[i].fascia[GIORNO_S-1][0][1];
        if(t1 >= (0xff*60)) t1 = 0;
        t2 = _lara->fasciaoraria[i].fascia[GIORNO_S-1][0][2] * 60 +
             _lara->fasciaoraria[i].fascia[GIORNO_S-1][0][3];
        if(t2 >= (0xff*60)) t2 = INT_MAX;
        if(((t1 == 0) && (t2 == INT_MAX)) || (fo < t1) || (fo >= t2))
        {
          t1 = _lara->fasciaoraria[i].fascia[GIORNO_S-1][1][0] * 60 +
             _lara->fasciaoraria[i].fascia[GIORNO_S-1][1][1];
          if(t1 >= (0xff*60)) t1 = 0;
          t2 = _lara->fasciaoraria[i].fascia[GIORNO_S-1][1][2] * 60 +
             _lara->fasciaoraria[i].fascia[GIORNO_S-1][1][3];
          if(t2 >= (0xff*60)) t2 = INT_MAX;
          if(((t1 == 0) && (t2 == INT_MAX)) || (fo < t1) || (fo >= t2))
          {
            if(!ret || (ev[2] > 14))
            {
              ev[2] = 13;
              ret = ID_DISAB;
            }
            continue;	// prova il profilo successivo
          }
        }
      }
    }
      /* Se arrivo fino a quì, il profilo è autorizzato. */
      break;
    }
    
    if(p < LARA_MULTIPROFILI)
    {
      /* Il profilo è abilitato, quindi l'unico errore
         ancora possibile è solo quello del terminale. */
      ret = lara_test_abil_term(idx, node);
      if(ret)
      {
        ev[2] = ret;
        ret = ID_DISAB;
      }
    }
    if(ret) codec_queue_event(ev);
    return ret;
  }
    
  return 0;
}

void lara_time(int mm)
{
  lara_tMsg cmd;
  time_t ltime;
  struct tm *datehour;
  
  cmd.Code = LARA_TIME;
  cmd.Address.domain = 0;
  cmd.Address.subnet = 2;
  cmd.Address.node = 255;
  cmd.Length = 7;
#if 0
  cmd.Msg[0] = SECONDI;
  cmd.Msg[1] = MINUTI;
  cmd.Msg[2] = ORE;
  cmd.Msg[3] = GIORNO;
  cmd.Msg[4] = MESE;
  cmd.Msg[5] = ANNO;
  cmd.Msg[6] = GIORNO_S-1;
#else
  time(&ltime);
  ltime++;	// cerco di compensare il ritardo in trasmissione
  datehour = localtime(&ltime);

  cmd.Msg[0] = datehour->tm_sec;
  cmd.Msg[1] = datehour->tm_min;
  cmd.Msg[2] = datehour->tm_hour;
  cmd.Msg[3] = datehour->tm_mday;
  cmd.Msg[4] = datehour->tm_mon + 1;
  cmd.Msg[5] = datehour->tm_year % 100;
  cmd.Msg[6] = datehour->tm_wday;
#endif
  
  lara_queue(mm, &cmd);
  
  /* Invia l'ora broadcast su LAN */
  tebe_send_lan((char*)&cmd, 12);
}

/* called once a second */
void lara_timer()
{
  int i, mm;
  unsigned char ev[4];
  
  /* una volta al secondo */
  /* se la routine di temporizzazione ritarda, ogni minuto viene corretta l'ora
     in base all'RTC, e si possono perdere i SECONDI bassi (0, 1, ecc). */

  if(SECONDI == 59)
  {
    for(i=0; i<MM_NUM; i++)
      if(MMPresente[i] == MM_LARA)
      {
        lara_time(i);
        i+=2;
      }
  }
  
  if(SECONDI == 50) lara_save_file();
  
  tebe_timer();
  
  for(mm=0; (mm<MM_NUM) && (MMPresente[mm]!=MM_LARA); mm++);
  for(i=0; i<64; i++)
  {
    if(lara_forzatura_timer[i])
    {
      lara_forzatura_timer[i]--;
      if(!lara_forzatura_timer[i])
      {
        /* Forzatura porta ritardata */
        database_lock();
        master_set_alarm((((mm << 5) + i) << 3) + 6, 1, 1);
//        master_set_alarm((((mm << 5) + i) << 3) + 6, 0, 1);
        database_unlock();
        if(!(SE[(((mm << 5) + i) << 3) + 6] & bitOOS))
        {
          ev[0] = Evento_Esteso2;
          ev[1] = Ex2_Lara;
          ev[2] = 21;
          ev[3] = i;
          codec_queue_event(ev);
          _lara_ingressi[i] |= 0x80;
        }
      }
    }
  }
}

void lara_load(int mm, gzFile fp)
{
  int i, ind;
  time_t ltime;
  struct tm *day;
  char term[8];
  
  gzread(fp, _lara, sizeof(lara_Config) - sizeof(lara_Tessere*));
  gzread(fp, _lara->tessera, lara_NumBadge * sizeof(lara_Tessere));
  
  if(_lara->param & LARA_PARAM_EXTRA_ID)
    gzread(fp, _laraex, lara_NumBadge * sizeof(lara_TessereEx));
  
  gzclose(fp);
  
  _lara->param |= LARA_PARAM_EXTRA_ID;
  
  if(!(_lara->param & LARA_PARAM_MULTIPROFILE))
  {
    /* Assicura che i profili associati alle tessere siano corretti */
    for(i=0; i<lara_NumBadge; i++)
    {
      /* Se è impostato un profilo, azzera la restante struttura dati */
      if(_lara->tessera[i].p.s.profilo)
        memset(_lara->tessera[i].p.profilo+1, 0, LARA_MULTIPROFILI-1);
    }
    _lara->param |= LARA_PARAM_MULTIPROFILE;	// anagrafica tessere multiprofilo
  }
  
  memset(_lara_ingressi, 0, sizeof(_lara_ingressi));
  for(i=0; i<63; i++)
    if(TipoPeriferica[(mm<<5)+i+1] == TLB)
      lara_terminal_prog[i] = 1;
    else
      lara_terminal_prog[i] = 0;
  
  /* inizializza profilo 1 con i terminali previsti */
  for(i=0; i<8; i++) term[i] = 0xff;
  if(!memcmp(_lara->profilo[0].term, term, 8))
  {
    for(i=0; i<8; i++) _lara->profilo[0].term[i] = 0;
    for(i=0; i<63; i++)
      if(lara_terminal_prog[i])
        _lara->profilo[0].term[i>>3] |= (1<<(i&0x07));
  }
  
  /* inizializza gli allarmi zone vuote in base alla confiugurazione */
  
  ind = ((mm+2) << 8) + 128 + 1;
  // L'area 0 non e' sensorizzata
  database_lock();
  for(i=1; i<LARA_N_AREE; i++)
  {
    master_set_alarm(ind, (_lara->presenza[i].contatore>0)?0:1, 1);
    ind++;
  }
  database_unlock();
  
  /* sostituisce le fasce orarie Delphi con quelle Lara */
  
//  lara_substitute_timings();
  
  /* inizializza i giorni festivi */
  
  time(&ltime);
  day = localtime(&ltime);
  for(i=0; i<n_FESTIVI; i++)
  {
    ltime += (24 * 3600);	// add one day
    day = localtime(&ltime);
    lara_update_day(day, &FESTIVI[i]);
  }
  
  if(config.TebeBadgeDisableOnErr)
    _lara->param |= LARA_PARAM_BADGEDISABLE;
}

void lara_reload(int mm)
{
  gzFile fp;
  int i;
  char buf[256];
  
  fp = gzopen(ADDROOTDIR(LaraFileTmp), "r");
  if(!fp) return;
  gzread(fp, &i, 4);
  if(i == 0x45424554)
  {
    gzread(fp, &i, 4);
    if(i != lara_NumBadge)
    {
      lara_NumBadge = i;
      
      free(_lara->tessera);
      free(_laraf->tessera);
      free(_laraf->tessera_area_last_update);
      _lara->tessera = (lara_Tessere*)calloc(1, (lara_NumBadge * sizeof(lara_Tessere)) + 1);
      _laraf->tessera = calloc(lara_NumBadge, sizeof(r_pass_flag));
      _laraf->tessera_area_last_update = calloc(lara_NumBadge, sizeof(int));
      if(!_lara->tessera || !_laraf->tessera || !_laraf->tessera_area_last_update)
      {
        support_log("Errore allocazione Lara (reload)");
        gzclose(fp);
        sprintf(buf, "mv %s %s", LaraFileTmp, LaraFile);
        system(buf);
        exit(0);
      }
    }
  }
  else
  {
    gzclose(fp);
    return;
  }
  
  lara_load(mm, fp);
  
  /* Occorre invalidare tutto il db */
  memset(_laraf->profilo, 1, sizeof(_laraf->profilo));
  memset(_laraf->terminale, 1, sizeof(_laraf->terminale));
  memset(_laraf->fasciaoraria, 1, sizeof(_laraf->fasciaoraria));
  memset(_laraf->festivo, 1, sizeof(_laraf->festivo));
  for(i=0; i<BADGE_NUM; i++) _laraf->tessera[i].ag = 1;
}

int lara_init(int mm)
{
  gzFile fp;
  lara_tMsg cmd;
  int i;
  char msg[64];
  
  if(!_lara)
  {
    fp = gzopen(ADDROOTDIR(LaraFile), "r");
    if(!fp)
      lara_NumBadge = 4000;
    else
    {
    gzread(fp, &i, 4);
    if(i == 0x45424554)
    {
      gzread(fp, &lara_NumBadge, 4);
      lara_NumBadge &= 0xffff;
    }
    else
    {
      lara_NumBadge = 4000;
      gzrewind(fp);
    }
    }
  
  sprintf(msg, "Tebe: %d tessere", lara_NumBadge);
  support_log(msg);
  
  _lara = (lara_Config*)calloc(1, sizeof(lara_Config));
  if(!_lara)
  {
    support_log("Errore allocazione Lara");
    return -1;
  }
  
  /* viene allocato un byte in piu' per permettere l'accesso al contatore su 3 byte
     come intero su 4 byte, senza accedere fuori della memoria allocata. */
  _lara->tessera = (lara_Tessere*)calloc(1, (lara_NumBadge * sizeof(lara_Tessere)) + 1);
  if(!_lara->tessera)
  {
    free(_lara);
    support_log("Errore allocazione Lara");
    return -1;
  }
  
  _laraf = (lara_Flags*)calloc(1, sizeof(lara_Flags));
  if(!_laraf)
  {
    free(_lara->tessera);
    free(_lara);
    support_log("Errore allocazione Lara");
    return -1;
  }
  
  _laraf->tessera = calloc(lara_NumBadge, sizeof(r_pass_flag));
  _laraf->tessera_area_last_update = calloc(lara_NumBadge, sizeof(int));
  
  if(!_laraf->tessera_area_last_update || !_laraf->tessera)
  {
    free(_lara->tessera);
    free(_lara);
    free(_laraf->tessera_area_last_update);
    free(_laraf->tessera);
    free(_laraf);
    support_log("Errore allocazione Lara");
    return -1;
  }
  
  _laraex = calloc(lara_NumBadge, sizeof(lara_TessereEx));
  if(!_laraex)
  {
    free(_lara->tessera);
    free(_lara);
    free(_laraf->tessera_area_last_update);
    free(_laraf->tessera);
    free(_laraf);
    support_log("Errore allocazione Lara");
    return -1;
  }
  
  if(!fp)
  {
    _lara->param = LARA_PARAM_MULTIPROFILE;	// anagrafica tessere multiprofilo
    if(config.TebeBadgeDisableOnErr)
    {
      _lara->param |= LARA_PARAM_BADGEDISABLE;
      return -2;
    }
  }
  
  lara_load(mm, fp);
  }
  
  tebe_init(mm);
  
#ifdef TEBESA
  tebe_load_config();
  tebe_term_init();
#endif
  
  if(lara_restart)
  {
    cmd.Code = LARA_NODES;
    cmd.Address.domain = 0;
    cmd.Address.subnet = 2;
    cmd.Address.node = 126;
    cmd.Length = 32;
    memset(cmd.Msg, 0, 32);
    cmd.Msg[0] = 2;
    cmd.Msg[16] = 2;
    lara_queue(mm, &cmd);

    lara_restart = 0;
  }
  
  if(master_behaviour == MASTER_NORMAL)
  {
    /* 20091105 */
#if 0
    /* 20081117 */
    /* Forza i nodi previsti */
    cmd.Code = LARA_NODES;
    cmd.Address.domain = 0;
    cmd.Address.subnet = 2;
    cmd.Address.node = 126;
    cmd.Length = 32;
    memset(cmd.Msg, 0, 32);
    for(i=0; i<64; i++)
    {
      if(TipoPeriferica[i+(mm*32)] == TLB)
      {
        cmd.Msg[i/8] |= (1<<(i&7));
        cmd.Msg[(i/8)+16] |= (1<<(i&7));
      }
    }
    lara_queue(mm, &cmd);
    /************/
#endif
    
    list_nodes = 1;
    
    cmd.Code = LARA_RQ;
    cmd.Address.domain = 0;
    cmd.Address.subnet = 2;
    cmd.Address.node = 126;
    cmd.Length = 1;
    cmd.Msg[0] = NODES;
    
    lara_queue(mm, &cmd);
    
    lara_time(mm);
  }
  else if(master_behaviour == MASTER_ACTIVE)
  {
    /* forza master */
    cmd.Code = TEBE_SET_MASTER;
    cmd.Length = 0;
    
    lara_queue(mm, &cmd);
  }
  else if(master_behaviour == MASTER_STANDBY)
  {
    /* attende la segnalazione slave */
  }
  
//  lara_time(mm);
  
  volto_init();
  
  return 0;
}

static void lara_nodes(unsigned char *bitmap, int pos, int type, int list)
{
  int i;
  unsigned char s, ev[16];
  
#ifdef TEBESA
  int mmbase, node;
  
  mmbase = pos >> 5;
  node = pos & 0x1f;
  if((mmbase >= 1) && (MMPresente[mmbase-1] == MM_LARA)) {node += 32; mmbase--;}
  if((mmbase >= 1) && (MMPresente[mmbase-1] == MM_LARA)) {node += 32; mmbase--;}
  if(node >= 64) node += 36;
  
  /* Nodo 2 sempre presente */
  if(node == 0) bitmap[0] |= 0x04;
#endif
  
  s = *bitmap;
  for(i=0; i<8; i++)
  {
    if(s & 0x01)
    {
      if(TipoPeriferica[pos+i] == type)
      {
        if(!(StatoPeriferica[pos>>3] & (1<<i)))
        {
#if 0
          ev[0] = Periferica_Ripristino;
#ifdef GEST16MM
          ev[1] = (pos+i)>>8;
          ev[2] = pos+i;
#else
          ev[1] = pos+i;
#endif
          codec_queue_event(ev);
#endif
          StatoPeriferica[pos>>3] |= (1<<i);
          tebe_ripristino(pos+i);
        }
      }
      else
      {
        StatoPeriferica[pos>>3] &= ~(1<<i);
        ev[0] = Periferica_Incongruente;
#ifdef GEST16MM
        ev[1] = (pos+i)>>8;
        ev[2] = pos+i;
        ev[3] = type;
#else
        ev[2] = type;
#endif
        codec_queue_event(ev);
      }
    }
    else
    {
      if((TipoPeriferica[pos+i] != 0xff) && (StatoPeriferica[pos>>3] & (1<<i)))
      {
        StatoPeriferica[pos>>3] &= ~(1<<i);
#if 0
        ev[0] = Periferica_Manomessa;
#ifdef GEST16MM
        ev[1] = (pos+i)>>8;
        ev[2] = pos+i;
#else
        ev[1] = pos+i;
#endif
        codec_queue_event(ev);
#endif
        tebe_manomissione(pos+i);
      }
    }
    s >>= 1;
  }
  
  s = *(bitmap+1);
  for(i=0; i<8; i++)
  {
    if(s & 0x01)
    {
      if(TipoPeriferica[pos+i+8] == type)
      {
        if(!(StatoPeriferica[(pos>>3)+1] & (1<<i)))
        {
#if 0
          ev[0] = Periferica_Ripristino;
#ifdef GEST16MM
          ev[1] = (pos+i+8)>>8;
          ev[2] = pos+i+8;
#else
          ev[1] = pos+i+8;
#endif
          codec_queue_event(ev);
#endif
          StatoPeriferica[(pos>>3)+1] |= (1<<i);
          tebe_ripristino(pos+i+8);
        }
      }
      else
      {
        StatoPeriferica[(pos>>3)+1] &= ~(1<<i);
        ev[0] = Periferica_Incongruente;
#ifdef GEST16MM
        ev[1] = (pos+i+8)>>8;
        ev[2] = pos+i+8;
        ev[3] = type;
#else
        ev[2] = type;
#endif
        codec_queue_event(ev);
      }
    }
    else
    {
      if((TipoPeriferica[pos+i+8] != 0xff) && (StatoPeriferica[(pos>>3)+1] & (1<<i)))
      {
        StatoPeriferica[(pos>>3)+1] &= ~(1<<i);
#if 0
        ev[0] = Periferica_Manomessa;
#ifdef GEST16MM
        ev[1] = (pos+i+8)>>8;
        ev[2] = pos+i+8;
#else
        ev[1] = pos+i+8;
#endif
        codec_queue_event(ev);
#endif
        tebe_manomissione(pos+i+8);
      }
    }
    s >>= 1;
  }
  
#ifdef GEST16MM
  if(list)
  {
    ev[0] = Periferiche_presenti;
    ev[1] = pos >> 8;
    ev[2] = pos;
    s = *bitmap;
    for(i=0; i<4; i++)
    {
      if(s & 0x01)
      {
        ev[i+3] = type;
        tebe_ripristino(pos+(i<<1));
      }
      else
        ev[i+3] = 0x0f;
      s >>= 1;
      if(s & 0x01)
      {
        ev[i+3] |= (type << 4);
        tebe_ripristino(pos+(i<<1)+1);
      }
      else
        ev[i+3] |= 0xf0;
      s >>= 1;
    }
    s = *(bitmap+1);
    for(i=4; i<8; i++)
    {
      if(s & 0x01)
      {
        ev[i+3] = type;
        tebe_ripristino(pos+(i<<1));
      }
      else
        ev[i+3] = 0x0f;
      s >>= 1;
      if(s & 0x01)
      {
        ev[i+3] |= (type << 4);
        tebe_ripristino(pos+(i<<1)+1);
      }
      else
        ev[i+3] |= 0xf0;
      s >>= 1;
    }
      
    codec_queue_event(ev);
  }
#else
  if(list)
  {
    ev[0] = Periferiche_presenti;
    ev[1] = pos;
    s = *bitmap;
    for(i=0; i<4; i++)
    {
      if(s & 0x01)
      {
        ev[i+2] = type;
        tebe_ripristino(pos+(i<<1));
      }
      else
        ev[i+2] = 0x0f;
      s >>= 1;
      if(s & 0x01)
      {
        ev[i+2] |= (type << 4);
        tebe_ripristino(pos+(i<<1)+1);
      }
      else
        ev[i+2] |= 0xf0;
      s >>= 1;
    }
    s = *(bitmap+1);
    for(i=4; i<8; i++)
    {
      if(s & 0x01)
      {
        ev[i+2] = type;
        tebe_ripristino(pos+(i<<1));
      }
      else
        ev[i+2] = 0x0f;
      s >>= 1;
      if(s & 0x01)
      {
        ev[i+2] |= (type << 4);
        tebe_ripristino(pos+(i<<1)+1);
      }
      else
        ev[i+2] |= 0xf0;
      s >>= 1;
    }
      
    codec_queue_event(ev);
  }
#endif
}

static void lara_set_id_struct(int idx, int node, lara_t_id *msg)
{
  msg->op_code = lara_test_abil(idx, node);
  
  msg->id = bswap_16(idx);
  msg->attrib.gruppo = _lara->tessera[idx].p.s.profilo;
  msg->attrib.g_resp = _lara->tessera[idx].stato.s.supervisor ? 4 : 0;
  msg->attrib.area = _lara->tessera[idx].area;
  msg->segreto = bswap_16(_lara->tessera[idx].pincnt);
  msg->attrib.abil = msg->op_code?0:1;
}

#if 0
static void lara_parse_LARA_CMD(int mm, int node, lara_t_cmd *msg)
{
  int i;
  lara_tMsg cmd;
  char buf[32];
  unsigned char ev[8];
  
#ifdef DEBUG
#warning !!!!!!!!!!!!!!!!!!!!!!  
  printf("op_code: %d\n", msg->op_code);
  for(i=0; i<28; i++) printf("%d ", ((unsigned char*)msg)[i]);
  printf("\n---------------------\n");
#endif
  
  cmd.Code = LARA_CMD;
  cmd.Address.domain = 0;
  cmd.Address.subnet = 2;
  cmd.Address.node = node;
  cmd.Length = sizeof(lara_t_cmd);
  
  switch(msg->op_code)
  {
    case V_ORA:
      sprintf(buf, "%02d%02d%02d", msg->tipo.data.hour, msg->tipo.data.min, msg->tipo.data.sec);
      cmd_set_time(buf);
      lara_time(mm);
      break;
    case V_DATA:
      sprintf(buf, "%d%02d%02d%02d", msg->tipo.data.dow, msg->tipo.data.day, msg->tipo.data.mon, msg->tipo.data.year);
      cmd_set_date(buf);
      lara_time(mm);
      break;
    case FASCIA_RQ:
      ((lara_t_cmd*)(cmd.Msg))->op_code = DATI_FASCIA;
      ((lara_t_cmd*)(cmd.Msg))->tipo.fasce.fascia = msg->tipo.fasce.fascia--;
      ((lara_t_cmd*)(cmd.Msg))->tipo.fasce.ora_inizio = _lara->fasciaoraria[msg->tipo.fasce.fascia].fascia[0][0][0];        
      ((lara_t_cmd*)(cmd.Msg))->tipo.fasce.ora_fine = _lara->fasciaoraria[msg->tipo.fasce.fascia].fascia[0][0][1];        
      ((lara_t_cmd*)(cmd.Msg))->tipo.fasce.min_inizio = _lara->fasciaoraria[msg->tipo.fasce.fascia].fascia[0][0][2];        
      ((lara_t_cmd*)(cmd.Msg))->tipo.fasce.min_fine = _lara->fasciaoraria[msg->tipo.fasce.fascia].fascia[0][0][3];        
      lara_queue(mm, &cmd);
      break;
    case PROG_FASCE:
      ev[0] = Evento_Esteso2;
      ev[1] = Ex2_Lara;
      ev[2] = 54;
      ev[3] = msg->tipo.fasce.fascia--;
      for(i=0; i<7; i++)
      {
        _lara->fasciaoraria[msg->tipo.fasce.fascia].fascia[i][0][0] = msg->tipo.fasce.ora_inizio;
        _lara->fasciaoraria[msg->tipo.fasce.fascia].fascia[i][0][1] = msg->tipo.fasce.ora_fine;
        _lara->fasciaoraria[msg->tipo.fasce.fascia].fascia[i][0][2] = msg->tipo.fasce.min_inizio;
        _lara->fasciaoraria[msg->tipo.fasce.fascia].fascia[i][0][3] = msg->tipo.fasce.min_fine;
      }
      _laraf->fasciaoraria[msg->tipo.fasce.fascia] = 1;	// invalida
      ev[2] = _lara->fasciaoraria[msg->tipo.fasce.fascia].fascia[0][0][0];
      ev[3] = _lara->fasciaoraria[msg->tipo.fasce.fascia].fascia[0][0][1];
      ev[4] = _lara->fasciaoraria[msg->tipo.fasce.fascia].fascia[0][0][2];
      ev[5] = _lara->fasciaoraria[msg->tipo.fasce.fascia].fascia[0][0][3];
      codec_queue_event(ev);
      lara_save(0);
      break;
    case FEST_RQ: /* Messaggio non usato sul terminale intermedio */
/*
      ((lara_t_cmd*)(cmd.Msg))->op_code = DATI_FESTIVI;
//#warning Specificare le festività
      ((lara_t_cmd*)(cmd.Msg))->tipo.fest.fissi = 0;
      ((lara_t_cmd*)(cmd.Msg))->tipo.fest.mobili = 0;
      lara_queue(mm, &cmd);
*/
      break;
    case PROG_FEST: /* Messaggio non usato sul terminale intermedio */
//#warning Programmare le festività
/*
      ev[0] = 115;
      ev[1] = msg->tipo.fest.fissi;
      ev[2] = msg->tipo.fest.mobili;
      codec_queue_event(ev);
*/
      break;
    case GROUP_RQ:
      ((lara_t_cmd*)(cmd.Msg))->op_code = DATI_GROUP;
      ((lara_t_cmd*)(cmd.Msg))->tipo.group.gruppo = msg->tipo.group.gruppo;
      ((lara_t_cmd*)(cmd.Msg))->tipo.group.stato = _lara->profilo[msg->tipo.group.gruppo].stato.b;
      ((lara_t_cmd*)(cmd.Msg))->tipo.group.fasce = 1 << (_lara->profilo[msg->tipo.group.gruppo].fosett-1);
      memcpy(((lara_t_cmd*)(cmd.Msg))->tipo.group.lettori, _lara->profilo[msg->tipo.group.gruppo].term, 8);
      ((lara_t_cmd*)(cmd.Msg))->tipo.group.telecomandi[0] = 0;
      lara_queue(mm, &cmd);
      break;
    case JOIN_GROUP_TEL: /* Messaggio non usato sul terminale intermedio */
//#warning Definire funzionalità telecomandi
      break;
    case JOIN_GROUP_LET:
      memcpy(_lara->profilo[((lara_t_cmd*)(cmd.Msg))->tipo.group.gruppo].term, msg->tipo.group.lettori, 8);
      _laraf->profilo[((lara_t_cmd*)(cmd.Msg))->tipo.group.gruppo] = 1;	// invalida
      ev[0] = Evento_Esteso2;
      ev[1] = Ex2_Lara;
      ev[2] = 44;
      ev[3] = ((lara_t_cmd*)(cmd.Msg))->tipo.group.gruppo;
      codec_queue_event(ev);
      lara_save(0);
      break;
    case JOIN_GROUP_FAS:
      for(i=0; !(msg->tipo.group.fasce & 1) && (i<8); i++) msg->tipo.group.fasce >>= 1;
      if(i < 8)
        _lara->profilo[((lara_t_cmd*)(cmd.Msg))->tipo.group.gruppo].fosett = i + 1;
      else
        _lara->profilo[((lara_t_cmd*)(cmd.Msg))->tipo.group.gruppo].fosett = 0;
      _laraf->profilo[((lara_t_cmd*)(cmd.Msg))->tipo.group.gruppo] = 1;	// invalida
      lara_save(0);
      break;
    case LETT_RQ:
      ((lara_t_cmd*)(cmd.Msg))->op_code = DATI_LETT;
      ((lara_t_cmd*)(cmd.Msg))->tipo.lett.address = msg->tipo.lett.address;
      ((lara_t_cmd*)(cmd.Msg))->tipo.lett.stato = _lara->terminale[msg->tipo.lett.address].c.s.filtro;
      ((lara_t_cmd*)(cmd.Msg))->tipo.lett.area[0] = _lara->terminale[msg->tipo.lett.address].area[0];
      ((lara_t_cmd*)(cmd.Msg))->tipo.lett.area[1] = _lara->terminale[msg->tipo.lett.address].area[1];
      lara_queue(mm, &cmd);
      break;
    case JOIN_LET_TEL: /* Messaggio non usato sul terminale intermedio */
//#warning Definire funzionalità telecomandi
      break;
    case JOIN_LET_AREE:
      _lara->terminale[msg->tipo.lett.address].area[0] = msg->tipo.lett.area[0];
      _lara->terminale[msg->tipo.lett.address].area[1] = msg->tipo.lett.area[1];
      _laraf->terminale[msg->tipo.lett.address] = 1;	// invalida
      ev[0] = Evento_Esteso2;
      ev[1] = Ex2_Lara;
      ev[2] = 48;
      ev[3] = msg->tipo.lett.address;
      ev[4] = msg->tipo.lett.area[0];
      ev[5] = msg->tipo.lett.area[1];
      codec_queue_event(ev);
      lara_save(0);
      break;
    case JOIN_LET_FILTER:
      _lara->terminale[msg->tipo.lett.address].c.s.filtro = msg->tipo.lett.stato;
      _laraf->terminale[msg->tipo.lett.address] = 1;	// invalida
      lara_save(0);
      break;
    case JOIN_ALL_TEL: /* Messaggio non usato sul terminale intermedio */
//#warning Definire funzionalità telecomandi
      break;
    default:
      break;
  }
}
#endif

void lara_input(int mm, int node, int ingressi, int lock)
{
  int idx;
  unsigned char ev[8];
      
      ev[0] = Evento_Esteso2;
      ev[1] = Ex2_Lara;
      ev[2] = 0;
      ev[3] = node;
      idx = ((mm << 5) + node) << 3;
      
  if(lock) database_lock();
      
      /* Manomissione contenitore */
      master_set_sabotage(idx, ingressi, Manomissione_Contenitore);
      
      idx += 2;
      /* Pulsante apriporta */
      if((ingressi & ~_lara_ingressi[node]) & 0x02)
      {
        master_set_alarm(idx, ingressi >> 1, 1);
        master_set_alarm(idx, 0, 1);
      }
      
      idx++;
      /* Sensore porta aperta */
      master_set_alarm(idx, ingressi >> 2, 1);
      if(!(SE[idx] & bitOOS))
      {
        if((ingressi & 0x04) && !(_lara_ingressi[node] & 0x04))
        {
          /* la forzatura e' gestita dall'evento specifico */
          /* qui genero solo l'evento di apertura */
          ev[2] = 67;
        }
        else if(!(ingressi & 0x04) && (_lara_ingressi[node] & 0x04))
        {
          /* evento di chiusura */
          ev[2] = 68;
          codec_queue_event(ev);
          ev[2] = 0;
          /* fine forzatura/apertura prolungata */
          if(_lara_ingressi[node] & 0x80) ev[2] = 23;
          _lara_ingressi[node] &= 0x7f;
          lara_forzatura_timer[node] = 0;
          master_set_alarm((((mm << 5) + node) << 3) + 6, 0, 1);
          master_set_alarm((((mm << 5) + node) << 3) + 7, 0, 1);
        }
      }
      if(ev[2]) codec_queue_event(ev);
      ev[2] = 0;
      
      idx++;
      /* Ingresso ausiliario */
      master_set_alarm(idx, ingressi >> 3, 1);
      if(!(SE[idx] & bitOOS))
      {
        if((ingressi & 0x08) && !(_lara_ingressi[node] & 0x08))
          ev[2] = 24;
        else if(!(ingressi & 0x08) && (_lara_ingressi[node] & 0x08))
          ev[2] = 25;
      }
      if(ev[2]) codec_queue_event(ev);
      ev[2] = 0;
      
      idx++;
      /* Batteria bassa */
      master_set_alarm(idx, ingressi >> 4, 1);
      if(!(SE[idx] & bitOOS))
      {
        if((ingressi & 0x10) && !(_lara_ingressi[node] & 0x10))
          ev[2] = 26;
        else if(!(ingressi & 0x10) && (_lara_ingressi[node] & 0x10))
          ev[2] = 27;
      }
      if(ev[2]) codec_queue_event(ev);
  
  if(lock) database_unlock();
  
      _lara_ingressi[node] = (_lara_ingressi[node] & 0x80) | (ingressi & 0x7f);
}

void lara_set_secret(int id, int secret, int term)
{
  unsigned char ev[8];
  int i, mask = 0;
  
  if(_lara && (id < lara_NumBadge))
  {
    ev[0] = Evento_Esteso2;
    ev[1] = Ex2_Lara;
    ev[2] = 31;
    ev[3] = id & 0xff;
    ev[4] = id >> 8;
    ev[5] = term;
    
    _lara->tessera[id].pincnt = secret;
    if(secret)
    {
      _lara->tessera[id].stato.s.badge |= SEGR_PROGR;
      if((_lara->tessera[id].stato.s.abil == BADGE_VUOTO) || (_lara->tessera[id].stato.s.abil == BADGE_CANC))
      {
        /* Creazione anagrafica volto */
        volto_param req;
        req.tipo = VOLTO_INSERT;
        req.anagrafica.id = id;
        volto_req(&req);
        
        _lara->tessera[id].stato.s.abil = BADGE_ABIL;
        
        _lara->tessera[id].stato.s.coerc = 0;
        _lara->tessera[id].stato.s.apbk = 0;
        _lara->tessera[id].area = 0;
        _lara->tessera[id].p.s.profilo = 1;
        for(i=0; i<LARA_N_FESTIVI; i++)
        {
          if(_lara->festivo[i].tipofestivo)
            mask |= (1L << i);
        }
//        *(int*)_lara->tessera[id].p.s.fest = mask;
//        *(int*)_lara->tessera[id].p.s.term = 1;
//        *(int*)(_lara->tessera[id].p.s.term + 4) = 0;
        inttouchar(mask, _lara->tessera[id].p.s.fest, sizeof(int));
        inttouchar(1, _lara->tessera[id].p.s.term, sizeof(int));
        inttouchar(0, _lara->tessera[id].p.s.term+4, sizeof(int));
        _lara->tessera[id].p.s.fosett = 0;
        _laraf->tessera_area_last_update[id] = time(NULL);
        ev[2] = 34;
      }
    }
    else
    {
      _lara->tessera[id].stato.s.badge &= ~SEGR_PROGR;
      if(_lara->tessera[id].stato.s.badge == 0)
      {
        _lara->tessera[id].stato.s.abil = BADGE_VUOTO;
        ev[2] = 32;
      }
    }
    
    _laraf->tessera[id].ag = 1;	// invalida
    
    codec_queue_event(ev);
    lara_save(0);
  }
}

void lara_set_counter(int id, int counter)
{
  unsigned char ev[8];
  
  if(_lara && (id < lara_NumBadge))
  {
    _lara->tessera[id].pincnt = counter;
    _laraf->tessera[id].ag = 1;	// invalida
    ev[0] = Evento_Esteso2;
    ev[1] = Ex2_Lara;
    ev[2] = 42;
    ev[3] = id & 0xff;
    ev[4] = id >> 8;
    codec_queue_event(ev);
    lara_save(0);
  }
}

void lara_set_profile(int id, int profile)
{
  unsigned char ev[8];
  
  if(_lara && (id < lara_NumBadge))
  {
    if(_lara->tessera[id].p.s.profilo == profile) return;
    
    if(_lara->tessera[id].p.s.profilo && !profile)
    {
      _lara->tessera[id].stato.s.coerc = _lara->profilo[_lara->tessera[id].p.s.profilo-1].stato.s.coerc;
      _lara->tessera[id].stato.s.apbk = _lara->profilo[_lara->tessera[id].p.s.profilo-1].stato.s.apbk;
      memcpy(_lara->tessera[id].p.s.term, _lara->profilo[_lara->tessera[id].p.s.profilo-1].term, 8);
      _lara->tessera[id].p.s.fosett = _lara->profilo[_lara->tessera[id].p.s.profilo-1].fosett;
      memcpy(_lara->tessera[id].p.s.fest, _lara->profilo[_lara->tessera[id].p.s.profilo-1].fest, 4);
    }
    else if(!_lara->tessera[id].p.s.profilo)
      memset(_lara->tessera[id].p.profilo, 0, LARA_MULTIPROFILI);
  
    _lara->tessera[id].p.s.profilo = profile;
    _laraf->tessera[id].ag = 1;	// invalida
    ev[0] = Evento_Esteso2;
    ev[1] = Ex2_Lara;
    ev[2] = 37;
    ev[3] = id & 0xff;
    ev[4] = id >> 8;
    ev[5] = profile;
    codec_queue_event(ev);
    lara_save(0);
  }
}

void lara_add_profile_sec(int id, int profile)
{
  unsigned char ev[8];
  int i;
  
  if(_lara && (id < lara_NumBadge))
  {
    for(i=0; (i<LARA_MULTIPROFILI) && _lara->tessera[id].p.profilo[i] &&
      (_lara->tessera[id].p.profilo[i] != profile); i++);
    if((i<LARA_MULTIPROFILI) && !_lara->tessera[id].p.profilo[i])
    {
      _lara->tessera[id].p.profilo[i] = profile;
      _laraf->tessera[id].ag = 1;	// invalida
      ev[0] = Evento_Esteso2;
      ev[1] = Ex2_Lara;
      ev[2] = 63;
      ev[3] = id & 0xff;
      ev[4] = id >> 8;
      ev[5] = profile;
      codec_queue_event(ev);
      lara_save(0);
    }
  }
}

void lara_del_profile_sec(int id, int profile)
{
  unsigned char ev[8];
  int i;
  
  if(_lara && (id < lara_NumBadge))
  {
    for(i=1; (i<LARA_MULTIPROFILI) && _lara->tessera[id].p.profilo[i] &&
      (_lara->tessera[id].p.profilo[i] != profile); i++);
    if((i<LARA_MULTIPROFILI) && (_lara->tessera[id].p.profilo[i] == profile))
    {
      memmove(_lara->tessera[id].p.profilo+i,
              _lara->tessera[id].p.profilo+i+1,
              LARA_MULTIPROFILI-i-1);
      _lara->tessera[id].p.profilo[LARA_MULTIPROFILI-1] = 0;
      _laraf->tessera[id].ag = 1;	// invalida
      ev[0] = Evento_Esteso2;
      ev[1] = Ex2_Lara;
      ev[2] = 64;
      ev[3] = id & 0xff;
      ev[4] = id >> 8;
      ev[5] = profile;
      codec_queue_event(ev);
      lara_save(0);
    }
  }
}

void lara_set_term_din(int term, int din)
{
  unsigned char ev[4];
  
  if(_lara && (term < LARA_N_TERMINALI) && (din >= 0))
  {
#warning
//    (unsigned short)(_lara->terminale[term].dincnt) = din;
    (_lara->terminale[term].dincnt) = (unsigned short)din;
    _laraf->terminale[term] = 1;	// invalida
    ev[0] = Evento_Esteso2;
    ev[1] = Ex2_Lara;
    ev[2] = 51;
    ev[3] = term+1;
    codec_queue_event(ev);
    lara_save(0);
  }
}

void lara_set_term_counter(int term, int counter)
{
  unsigned char ev[4];
  
  if(_lara && (term < LARA_N_TERMINALI) && (counter >= 0))
  {
    _lara->terminale[term].dincnt = counter;
    _laraf->terminale[term] = 1;	// invalida
    ev[0] = Evento_Esteso2;
    ev[1] = Ex2_Lara;
    ev[2] = 51;
    ev[3] = term+1;
    codec_queue_event(ev);
    lara_save(0);
  }
}

void lara_set_term_times(int term, int timeopen, int timeopentimeout, int tapbk)
{
  unsigned char ev[8];
  
  if(_lara && (term < LARA_N_TERMINALI))
  {
    _lara->terminale[term].timeopen = timeopen;
    _lara->terminale[term].timeopentimeout = timeopentimeout;
    _lara->terminale[term].conf.s.tapbk = tapbk;
    _laraf->terminale[term] = 1;	// invalida
    lara_save(0);
    
    ev[0] = 247;
    ev[1] = 0;
    ev[2] = 52;
    ev[3] = term+1;
    ev[4] = timeopen;
    ev[5] = timeopentimeout;
    ev[6] = tapbk;
    codec_queue_event(ev);
  }
}

static void lara_parse_LARA_ID(int mm, int node, lara_t_id *msg)
{
  int idx, direct; //, i;
  lara_tMsg cmd;
  unsigned char ev[16];
  
#ifdef DEBUG
#warning !!!!!!!!!!!!!!!!!!!!!!
  static int cont = 0;
  
  printf("op_code: %d\n", msg->op_code);
  printf("service_code: %d\n", bswap_16(msg->service_code));
  printf("attrib.g_resp: %d\n", msg->attrib.g_resp);
  printf("attrib.area: %d\n", msg->attrib.area);
  printf("attrib.abil: %d\n", msg->attrib.abil);
  printf("attrib.gruppo: %d\n", msg->attrib.gruppo);
  printf("attrib.fessura: %d\n", msg->attrib.fessura);
  printf("attrib.spare: %d\n", msg->attrib.spare);
  printf("ingressi: %d\n", msg->ingressi);
  printf("id: %d\n", bswap_16(msg->id));
  printf("segreto: %d\n", bswap_16(msg->segreto));
  printf("badge: %s\n", msg->badge);
  printf("-------------------%d--\n", cont++);
#endif
  
  /* Gestione configurazione Nuovo Varco.
     I nuovi moduli varco non gestiscono tutta l'anagrafica come i TermTebe,
     però usano la configurazione centralizzata dei dati del terminale (tempi).
     Quindi se arriva un LARA_ID si tratta di un modulo varco, e se il terminale
     corrispondente risulta modificato, prima di mandare ogni altra cosa mando
     la nuova configurazione. In questo modo il modulo varco risulta sempre
     allineato senza che questo debba preoccuparsi di richiedere alcunché e
     allo stesso tempo senza che la centrale inizi a mandare configurazioni
     non necessarie sul bus in stile TermTebe. */
  if(_laraf->terminale[node-1])
  {
    unsigned char buf[sizeof(lara_Terminali)+7];
    
    /* Il modulo varco è solo LON */
    buf[0] = TEBE_OUT;
    buf[1] = 0;
    buf[2] = 2;
    buf[3] = node;
    buf[4] = 2 + sizeof(lara_Terminali);
    buf[5] = CODE_TERM;
    buf[6] = node-1;
    memcpy(buf+7, &(_lara->terminale[node-1]), sizeof(lara_Terminali));
    master_queue(mm, buf);
    
    _laraf->terminale[node-1] = 0;
  }
  
  cmd.Code = LARA_ID;
  cmd.Address.domain = 0;
  cmd.Address.subnet = 2;
  cmd.Address.node = node;
  cmd.Length = sizeof(lara_t_id);
  
  ev[0] = Evento_Esteso2;
  ev[1] = Ex2_Lara;
  
  direct = msg->op_code & 0xc0;
  msg->op_code = msg->op_code & 0x3f;
  
  switch(msg->op_code)
  {
    case ID_RQ:
      idx = bswap_16(msg->id);
      if((idx < 0) || (idx >= lara_NumBadge) ||
         (_lara->tessera[idx].stato.s.abil == BADGE_VUOTO) ||
         (_lara->tessera[idx].stato.s.abil == BADGE_CANC))
      {
        msg->op_code = NO_ID;
        memcpy(cmd.Msg, msg, sizeof(lara_t_id));
        lara_queue(mm, &cmd);
        break;
      }
    case SET_SEGR_ID:
      idx = bswap_16(msg->id);
      if((idx < 0) || (idx >= lara_NumBadge))
      {
        msg->op_code = NO_ID;
        memcpy(cmd.Msg, msg, sizeof(lara_t_id));
        lara_queue(mm, &cmd);
      }
      else
      {
/* non devo verificare l'id perche' potrei volerlo creare nuovo */
        lara_set_id_struct(idx, node, msg);
        msg->op_code = DATI_ID;
        memcpy(cmd.Msg, msg, sizeof(lara_t_id));
        lara_queue(mm, &cmd);
      }
      break;
    case ACC_ID:
      idx = bswap_16(msg->id);
      if(idx && (SE[(mm<<8)+(node<<3)+msg->attrib.fessura] & bitOOS))
      {
        ev[3] = idx & 0xff;
        ev[4] = idx >> 8;
        if(_lara->tessera[idx].p.s.profilo)
        {
          ev[2] = 15;
          ev[5] = _lara->tessera[idx].p.s.profilo;
          ev[6] = node;
        }
        else
        {
          ev[2] = 12;
          ev[5] = node;
        }
        codec_queue_event(ev);
/* il codice messaggio e' relativo ad un'altra situazione
   ma stampa "Terminale non permesso" che puo' andare bene */
        msg->op_code = ID_NO_JOIN_LETT;
      }
      else if((idx < 0) || (idx >= lara_NumBadge) ||
              (_lara->tessera[idx].stato.s.abil == BADGE_VUOTO) ||
              (_lara->tessera[idx].stato.s.abil == BADGE_CANC))
      {
        msg->op_code = NO_ID;
      }
      else
      {
        lara_set_id_struct(idx, node, msg);
        if(msg->op_code)
        {
          msg->segreto = 0;
        }
        else if(_lara->tessera[idx].stato.s.badge & BADGE_PROGR)
        {
          if((_lara->tessera[idx].stato.s.badge & SEGR_PROGR) &&
          ((((msg->attrib.fessura == 0) && ((_lara->terminale[node-1].conf.s.filtro & 0x3) == 2)) ||
          ((msg->attrib.fessura == 1) && ((_lara->terminale[node-1].conf.s.filtro & 0xc) == 8)))))
          {
            msg->op_code = SEGR_RQ;
          }
          else
          {
            msg->op_code = BADGE_REQUIRED;
          }
        }
        else if(_lara->tessera[idx].stato.s.badge & SEGR_PROGR)
        {
          msg->op_code = SEGR_RQ;
        }
        else
        {
          msg->op_code = NO_SEGR;
        }
      }
      memcpy(cmd.Msg, msg, sizeof(lara_t_id));
      lara_queue(mm, &cmd);
      break;
    case BADGE_RQ:
    case SET_SEGR_BADGE:
      idx = lara_find_badge(msg->badge, direct);
      if(idx < 0)
      {
        ev[2] = 18;
        ev[3] = node;
        codec_queue_event(ev);
        msg->op_code = NO_BADGE;
      }
      else
      {
        lara_set_id_struct(idx, node, msg);
        msg->op_code = DATI_ID;
        lara_last_idx = idx;
      }
      memcpy(cmd.Msg, msg, sizeof(lara_t_id));
      lara_queue(mm, &cmd);
      break;
    case ACC_BADGE:
    case ACC_SEGR:
      if(msg->op_code == ACC_BADGE)
      {
        /* Gestione AreaSelf */
        if(_lara->terminale[node-1].area[0] == TEBE_AREASELF)
        {
          idx = lara_find_areaself_template(msg->badge);
          if(idx < 0)
          {
            ev[2] = 18;	// id assente
            ev[3] = node;
            codec_queue_event(ev);
            msg->op_code = NO_BADGE;
          }
          else if(SE[(mm<<8)+(node<<3)+msg->attrib.fessura] & bitOOS)
          {
            ev[2] = 12;	// non abilitato
            ev[3] = 0;
            ev[4] = 0;
            ev[5] = node;
            codec_queue_event(ev);
/* il codice messaggio e' relativo ad un'altra situazione
   ma stampa "Terminale non permesso" che puo' andare bene */
            msg->op_code = ID_NO_JOIN_LETT;
          }
          else
          {
            database_lock();
            ME[(node<<1)+msg->attrib.fessura+640] = 0;
            lara_timbrato[(node<<1)+msg->attrib.fessura].id = idx;
            lara_timbrato[(node<<1)+msg->attrib.fessura].profilo = 0;
            lara_timbrato[(node<<1)+msg->attrib.fessura].time = time(NULL);
            master_set_alarm((mm<<8)+(node<<3)+msg->attrib.fessura, 1, 1);
            master_set_alarm((mm<<8)+(node<<3)+msg->attrib.fessura, 0, 1);
            database_unlock();
            //msg->id = 0;
            msg->id = bswap_16(idx);
            msg->attrib.gruppo = 0;
            msg->attrib.g_resp = 0;
            msg->attrib.area = 0;
            msg->segreto = 0;
            msg->attrib.abil = 1;
            msg->op_code = TIMBRATO;
          }
          msg->attrib.spare = 0;
          memcpy(cmd.Msg, msg, sizeof(lara_t_id));
          lara_queue(mm, &cmd);
          break;
        }
        else
          idx = lara_find_badge(msg->badge, direct);
      }
      else
        idx = bswap_16(msg->id);
      
      if((idx >= 0) && (SE[(mm<<8)+(node<<3)+msg->attrib.fessura] & bitOOS))
      {
        ev[3] = idx & 0xff;
        ev[4] = idx >> 8;
        if(_lara->tessera[idx].p.s.profilo)
        {
          ev[2] = 15;	// non abilitato
          ev[5] = _lara->tessera[idx].p.s.profilo;
          ev[6] = node;
        }
        else
        {
          ev[2] = 12;	// non abilitato
          ev[5] = node;
        }
        codec_queue_event(ev);
/* il codice messaggio e' relativo ad un'altra situazione
   ma stampa "Terminale non permesso" che puo' andare bene */
        msg->op_code = ID_NO_JOIN_LETT;
      }
      else if((idx < 0) || (idx >= lara_NumBadge) ||
              (_lara->tessera[idx].stato.s.abil == BADGE_VUOTO) ||
              (_lara->tessera[idx].stato.s.abil == BADGE_CANC))
      {
        ev[2] = 18;	// id assente
        ev[3] = node;
        codec_queue_event(ev);
        if(msg->op_code == ACC_BADGE)
          msg->op_code = NO_BADGE;
        else
          msg->op_code = NO_ID;
      }
#if 0
      else if((msg->op_code == ACC_BADGE) && (idx < NUM_BADGE_COLLETTIVI))
      {
        msg->id = bswap_16(idx);
        lara_set_id_struct(idx, node, msg);
        if(!msg->op_code)
        {
          database_lock();
          ME[(node<<1)+msg->attrib.fessura+640] = _lara->tessera[bswap_16(msg->id)].profilo;
          master_set_alarm((mm<<8)+(node<<3)+msg->attrib.fessura, 1, 1);
          master_set_alarm((mm<<8)+(node<<3)+msg->attrib.fessura, 0, 1);
          database_unlock();
          msg->op_code = TRANS;
          msg->attrib.area = _lara->terminale[node-1].area[1-msg->attrib.fessura];
        }
      }
#endif
      else
      {
        unsigned char tmp_op_code = msg->op_code; // msg->op_code modified by lara_set_id_struct()
        
        msg->id = bswap_16(idx);
        lara_set_id_struct(idx, node, msg);

        if(_lara->tessera[idx].stato.s.supervisor)
        {
          ev[2] = 16;	// abilita grado supervisore
          ev[3] = idx & 0xff;
          ev[4] = idx >> 8;
          ev[5] = node;
          codec_queue_event(ev);
          msg->op_code = SET_G_RESP;
        }
        else if(msg->attrib.abil)
        {
/*
printf("%d %d %d term apbk:%d area tess:%d area fess:%d\n",
  _lara->tessera[idx].p.s.profilo,
  _lara->tessera[idx].stato.s.apbk,
  _lara->profilo[_lara->tessera[idx].p.s.profilo-1].stato.s.apbk,
  _lara->terminale[node-1].stato.s.apbk,
  _lara->tessera[idx].area,
  _lara->terminale[node-1].area[msg->attrib.fessura]);
*/
          if(((!_lara->tessera[idx].p.s.profilo && !_lara->tessera[idx].stato.s.apbk) ||
              (_lara->tessera[idx].p.s.profilo && !_lara->profilo[_lara->tessera[idx].p.s.profilo-1].stato.s.apbk)) &&
             (!_lara->terminale[node-1].stato.s.apbk) &&
             (_lara->tessera[idx].area != _lara->terminale[node-1].area[msg->attrib.fessura]))
          {
            ev[2] = 11;	// fuori area
            ev[3] = idx & 0xff;
            ev[4] = idx >> 8;
            ev[5] = node;
            codec_queue_event(ev);
            msg->op_code = NOT_IN_ZONE;
          }
          else if((tmp_op_code == ACC_BADGE) && (_lara->tessera[idx].stato.s.badge & SEGR_PROGR) &&
          ((((msg->attrib.fessura == 0) && ((_lara->terminale[node-1].conf.s.filtro & 0x3) != 1)) ||
          ((msg->attrib.fessura == 1) && ((_lara->terminale[node-1].conf.s.filtro & 0xc) != 4)))))
          {
            msg->op_code = SEGR_RQ;
          }
          else if(_lara->terminale[node-1].stato.s.volto && !msg->attrib.spare)
          {
            /* Riconoscimento volto non bloccante */
            volto_param req;
            req.tipo = VOLTO_IDFY_LARA;
            req.lara.mm = mm;
            req.lara.nodo = node;
            msg->op_code = tmp_op_code;
            memcpy(&req.lara.msg, msg, sizeof(lara_t_id));
            volto_req(&req);
            break;
          }
          else if(msg->attrib.spare > 1)
          {
            /* Nota: msg->attrib.spare vale sempre 0 quando il messaggio arriva
               direttamente dal terminale. Se è diverso da 0, il valore indica
               il risultato del riconoscimento del volto. */
            /* Il valore 2 indica errore di riconoscimento. */
            msg->op_code = GENERIC_ERROR;
            
            if(msg->attrib.spare == 2)
              ev[2] = 65;	// volto non riconosciuto
            else
              ev[2] = 66;	// volto non presente in anagrafica
            ev[3] = idx & 0xff;
            ev[4] = idx >> 8;
            ev[5] = node;
            codec_queue_event(ev);
          }
          else if(_lara->terminale[node-1].area[0] == _lara->terminale[node-1].area[1])
          {
            lara_giustificativo[node] = bswap_16(msg->service_code);
            database_lock();
            lara_timbrato[(node<<1)+msg->attrib.fessura].id = bswap_16(msg->id);
            lara_timbrato[(node<<1)+msg->attrib.fessura].profilo =
              _lara->tessera[bswap_16(msg->id)].p.s.profilo;
            lara_timbrato[(node<<1)+msg->attrib.fessura].time = time(NULL);
            ME[(node<<1)+msg->attrib.fessura+640] = _lara->tessera[bswap_16(msg->id)].p.s.profilo;
            master_set_alarm((mm<<8)+(node<<3)+msg->attrib.fessura, 1, 1);
            master_set_alarm((mm<<8)+(node<<3)+msg->attrib.fessura, 0, 1);
            database_unlock();
            msg->op_code = TIMBRATO;
          }
          else
          {
            lara_giustificativo[node] = bswap_16(msg->service_code);
            database_lock();
            lara_timbrato[(node<<1)+msg->attrib.fessura].id = bswap_16(msg->id);
            lara_timbrato[(node<<1)+msg->attrib.fessura].profilo =
              _lara->tessera[bswap_16(msg->id)].p.s.profilo;
            lara_timbrato[(node<<1)+msg->attrib.fessura].time = time(NULL);
            ME[(node<<1)+msg->attrib.fessura+640] = _lara->tessera[bswap_16(msg->id)].p.s.profilo;
            master_set_alarm((mm<<8)+(node<<3)+msg->attrib.fessura, 1, 1);
            master_set_alarm((mm<<8)+(node<<3)+msg->attrib.fessura, 0, 1);
            database_unlock();
            msg->op_code = TRANS;
            msg->attrib.area = _lara->terminale[node-1].area[1-msg->attrib.fessura];
          }
        }
      }
      /* Rimetto a posto lo spare, altrimenti lo trasmetto al lettore
         e alla timbratura successiva mi torna indietro. */
      msg->attrib.spare = 0;
      memcpy(cmd.Msg, msg, sizeof(lara_t_id));
      lara_queue(mm, &cmd);
      break;
    case TRANS_OK:
      if(msg->service_code == -1)
        ev[2] = 7;
      else
      {
        ev[2] = 9;
        /* msg->service_code e' invertito */
        ev[7] = msg->service_code >> 8;
        ev[8] = msg->service_code & 0xff;
      }
      ev[3] = msg->id >> 8;
      ev[4] = msg->id & 0xff;
      ev[5] = node;
      ev[6] = msg->attrib.area;
      codec_queue_event(ev);
      
/*
printf("ID:%d  area:%d (%d) -> area:%d (%d)\n", bswap_16(msg->id),
  _lara->tessera[bswap_16(msg->id)].area,
  _lara->presenza[_lara->tessera[bswap_16(msg->id)].area].contatore,
  msg->attrib.area,
  _lara->presenza[msg->attrib.area].contatore);
*/
      
#if 0
      if(_lara->tessera[bswap_16(msg->id)].area < LARA_N_AREE)
      {
        _lara->presenza[_lara->tessera[bswap_16(msg->id)].area].contatore--;
        if(_lara->tessera[bswap_16(msg->id)].area > 0)
        {
          database_lock();
          master_set_alarm(((mm+2) << 8) + _lara->tessera[bswap_16(msg->id)].area + 128,
            (_lara->presenza[_lara->tessera[bswap_16(msg->id)].area].contatore>0)?0:1, 1);
          database_unlock();
        }
      }
      
      _lara->tessera[bswap_16(msg->id)].area = msg->attrib.area;
      
      if(_lara->tessera[bswap_16(msg->id)].area < LARA_N_AREE)
      {
        _laraf->tessera_area_last_update[bswap_16(msg->id)] = time(NULL);
        _lara->presenza[_lara->tessera[bswap_16(msg->id)].area].contatore++;
        if(_lara->tessera[bswap_16(msg->id)].area > 0)
        {
          database_lock();
          master_set_alarm(((mm+2) << 8) + _lara->tessera[bswap_16(msg->id)].area + 128, 0, 1);
          database_unlock();
        }
      }
#else
/*
printf("ID:%d  area:%d (%d) -> area:%d (%d)\n", bswap_16(msg->id),
  _lara->tessera[bswap_16(msg->id)].area,
  _lara->presenza[_lara->tessera[bswap_16(msg->id)].area].contatore,
  msg->attrib.area,
  _lara->presenza[msg->attrib.area].contatore);
*/      
      lara_set_area_counters(mm+2, _lara->tessera[bswap_16(msg->id)].area, msg->attrib.area);
      _laraf->tessera_area_last_update[bswap_16(msg->id)] = time(NULL);
      _lara->tessera[bswap_16(msg->id)].area = msg->attrib.area;
      lara_set_area_group(bswap_16(msg->id), msg->attrib.area, mm+2,
        _laraf->tessera_area_last_update[bswap_16(msg->id)]);
#endif
      
      _laraf->tessera[bswap_16(msg->id)].ag = 1;	// invalida
      lara_save(0);
      break;
    case TIMBR_OK:
      if(msg->service_code == -1)
        ev[2] = 8;
      else
      {
        ev[2] = 10;
        /* msg->service_code e' invertito */
        ev[7] = msg->service_code >> 8;
        ev[8] = msg->service_code & 0xff;
      }
      ev[3] = msg->id >> 8;
      ev[4] = msg->id & 0xff;
      ev[5] = node;
      ev[6] = msg->attrib.fessura;
      codec_queue_event(ev);
      break;
/*
    case V_AREA:
    case V_AREA_L:
      idx = bswap_16(msg->id);
      if((idx < 0) || (idx >= lara_NumBadge) ||
         (_lara->tessera[idx].stato.s.abil == BADGE_VUOTO) ||
         (_lara->tessera[idx].stato.s.abil == BADGE_CANC))
      {
        msg->op_code = NO_ID;
        memcpy(cmd.Msg, msg, sizeof(lara_t_id));
        lara_queue(mm, &cmd);
      }
      else
        lara_set_area(bswap_16(msg->id), msg->attrib.area);
      break;
*/
    case SET_N_SEGR:
      if(msg->segreto == -1)
      {
        msg->op_code = ERR_SEGR;
        memcpy(cmd.Msg, msg, sizeof(lara_t_id));
        lara_queue(mm, &cmd);
      }
      else
        lara_set_secret(bswap_16(msg->id), bswap_16(msg->segreto), node);
      break;
    case ID_FREE_RQ:
      idx = bswap_16(msg->id);
      if(idx >= lara_NumBadge)
      {
        msg->op_code = NO_ID;
      }
      else if((_lara->tessera[idx].stato.s.abil == BADGE_VUOTO) || (_lara->tessera[idx].stato.s.abil == BADGE_CANC) ||
              (_lara->tessera[idx].stato.s.badge != 0))
      {
        msg->op_code = DATI_ID;
      }
      else
      {
        msg->op_code = ID_BUSY;
      }
      memcpy(cmd.Msg, msg, sizeof(lara_t_id));
      lara_queue(mm, &cmd);
      break;
    case JOIN_ID_GROUP:
      idx = bswap_16(msg->id);
      if((idx < 0) || (idx >= lara_NumBadge) ||
         (_lara->tessera[idx].stato.s.abil == BADGE_VUOTO) ||
         (_lara->tessera[idx].stato.s.abil == BADGE_CANC))
      {
        ev[2] = 18;
        ev[3] = node;
        codec_queue_event(ev);
        msg->op_code = NO_ID;
        memcpy(cmd.Msg, msg, sizeof(lara_t_id));
        lara_queue(mm, &cmd);
      }
      else
        lara_set_profile(idx, msg->attrib.gruppo);
      break;
    case C_BADGE_L:
      for(idx=1; (idx<lara_NumBadge) &&
        (!((_lara->tessera[idx].stato.s.abil == BADGE_VUOTO) || (_lara->tessera[idx].stato.s.abil == BADGE_CANC))); idx++);
      msg->id = bswap_16(idx);
    case C_BADGE_F:
      idx = bswap_16(msg->id);
      if(idx >= lara_NumBadge)
      {
        msg->op_code = NO_ID;
      }
      else if((_lara->tessera[idx].stato.s.abil != BADGE_ABIL) && (_lara->tessera[idx].stato.s.abil != BADGE_DISABIL))
      {
        if(lara_find_badge(msg->badge, direct) < 0)
        {
          /* Creazione anagrafica volto */
          volto_param req;
          req.tipo = VOLTO_INSERT;
          req.anagrafica.id = idx;
          volto_req(&req);
          
          if(direct)
            memcpy(_lara->tessera[idx].badge, msg->badge, 10);
          else
            lara_badge_to_nibble(msg->badge, _lara->tessera[idx].badge);
          msg->op_code = DATI_ID;
          _lara->tessera[idx].stato.s.badge = BADGE_PROGR;
          _lara->tessera[idx].stato.s.coerc = 0;
          _lara->tessera[idx].stato.s.apbk = 0;
          _lara->tessera[idx].area = 0;
          _lara->tessera[idx].pincnt = 0;
          memset(_lara->tessera[idx].p.profilo, 0, LARA_MULTIPROFILI);
//          if(msg->attrib.gruppo)
//            _lara->tessera[idx].p.s.profilo = msg->attrib.gruppo;
//          else
            _lara->tessera[idx].p.s.profilo = 1;
          _lara->tessera[idx].stato.s.abil = BADGE_ABIL;
/*
Impostando il profilo 1 per default, queste impostazioni non sono utilizzate.
Inoltre, con i multiprofili queste impostazioni vanno in conflitto con i profili secondari.
          if(!_lara->tessera[idx].p.s.profilo)
          {
            int mask = 0;
            
            for(i=0; i<LARA_N_FESTIVI; i++)
            {
              if(_lara->festivo[i].tipofestivo)
                mask |= (1L << i);
            }
            *(int*)_lara->tessera[idx].p.s.fest = mask;
            *(int*)_lara->tessera[idx].p.s.term = 1;
            *(int*)(_lara->tessera[idx].p.s.term + 4) = 0;
            _lara->tessera[idx].p.s.fosett = 0;
          }
*/
          _laraf->tessera[idx].ag = 1;	// invalida
          _laraf->tessera_area_last_update[idx] = time(NULL);
          ev[0] = Evento_Esteso2;
          ev[1] = Ex2_Lara;
          ev[2] = 34;
          ev[3] = idx & 0xff;
          ev[4] = idx >> 8;
          ev[5] = node;
          codec_queue_event(ev);
          lara_save(0);
        }
        else
          msg->op_code = BADGE_EXIST;
      }
      else
      {
        msg->op_code = ID_BUSY;
      }
      memcpy(cmd.Msg, msg, sizeof(lara_t_id));
      lara_queue(mm, &cmd);
      break;
    case C_MASK:
      break;
    case DOOR_TIMEOUT:
      database_lock();
      master_set_alarm((((mm << 5) + node) << 3) + 7, 1, 1);
//      master_set_alarm((((mm << 5) + node) << 3) + 7, 0, 1);
      database_unlock();
      if(!(SE[(((mm << 5) + node) << 3) + 7] & bitOOS))
      {
        ev[2] = 22;
        ev[3] = node;
        codec_queue_event(ev);
        _lara_ingressi[node] |= 0x80;
      }
      break;
    case FORZATURA:
      if(_lara->terminale[node-1].stato.s.ritforz)
      {
        lara_forzatura_timer[node] = _lara->terminale[node-1].timeopentimeout;
      }
      else
      {
        database_lock();
        master_set_alarm((((mm << 5) + node) << 3) + 6, 1, 1);
//        master_set_alarm((((mm << 5) + node) << 3) + 6, 0, 1);
        database_unlock();
        if(!(SE[(((mm << 5) + node) << 3) + 6] & bitOOS))
        {
          ev[2] = 21;
          ev[3] = node;
          codec_queue_event(ev);
          _lara_ingressi[node] |= 0x80;
        }
      }
      break;
    case CHANGE_INPUT:
      /* il campo input e' valido su tutti gli eventi */
      if(!(msg->ingressi & 0x04) && !(_lara_ingressi[node] & 0x04) &&
         !(SE[(((mm << 5) + node) << 3) + 3] & bitOOS))
      {
        ev[2] = 7;
        ev[3] = 0;
        ev[4] = 0;
        ev[5] = node;
        ev[6] = _lara->terminale[node-1].area[0];
        codec_queue_event(ev);
      }
      break;
    default:
      break;
  }

  /* Gestione ingressi */
  msg->ingressi ^= 0x14;
  lara_input(mm, node, msg->ingressi, 1);
}

void lara_parse(int mm, unsigned char *cmd)
{
  lara_tMsg *lcmd;
  unsigned char ev[8];
  int i, mod;
  
  lcmd = (lara_tMsg*)cmd;

#ifdef DEBUG
#warning !!!!!!!!!!!!!!!!!!!!!!
printf("-> code:%d node:%d len:%d\n", lcmd->Code, lcmd->Address.node, lcmd->Length);
#endif
  
  if(lcmd->Address.node < 64)
  {
    if((TipoPeriferica[(mm<<5)+lcmd->Address.node] == TLB) &&
       !(StatoPeriferica[((mm<<5)+lcmd->Address.node)>>3] & (1<<(lcmd->Address.node&0x07))))
    {
      StatoPeriferica[((mm<<5)+lcmd->Address.node)>>3] |= (1<<(lcmd->Address.node&0x07));
#ifdef GEST16MM
      ev[1] = ((mm<<5)+lcmd->Address.node)>>8;
      ev[2] = (mm<<5)+lcmd->Address.node;
#else
      ev[1] = (mm<<5)+lcmd->Address.node;
#endif
//      codec_queue_event(ev);
//      tebe_ripristino((mm<<5)+lcmd->Address.node);
      master_periph_restore(((mm<<5)+lcmd->Address.node)<<3, ev);
    }
  }
  else if((lcmd->Address.node >= 100) && (lcmd->Address.node <= 115))
  {
    if((TipoPeriferica[(mm<<5)+lcmd->Address.node-36] == SC8) &&
       !(StatoPeriferica[((mm<<5)+lcmd->Address.node-36)>>3] & (1<<((lcmd->Address.node-36)&0x07))))
    {
      StatoPeriferica[((mm<<5)+lcmd->Address.node-36)>>3] |= (1<<((lcmd->Address.node-36)&0x07));
#ifdef GEST16MM
      ev[1] = ((mm<<5)+lcmd->Address.node-36)>>8;
      ev[2] = (mm<<5)+lcmd->Address.node-36;
#else
      ev[1] = (mm<<5)+lcmd->Address.node-36;
#endif
//      codec_queue_event(ev);
//      tebe_ripristino((mm<<5)+lcmd->Address.node-36);
      master_periph_restore(((mm<<5)+lcmd->Address.node-36)<<3, ev);
    }
  }

  switch(lcmd->Code)
  {
//    case LARA_TIME:
//      break;
    case LARA_PC_RQ:
      ev[0] = Evento_Esteso2;
      ev[1] = Ex2_Lara;
      ev[2] = 61;
      ev[3] = lara_last_idx & 0xff;
      ev[4] = lara_last_idx >> 8;
      ev[5] = lcmd->Address.node;
      ev[6] = lcmd->Msg[0];
      ev[7] = 0;
      codec_queue_event(ev);
      
//printf("Req: %d\n", lcmd->Msg[0]);
/* Nodo >= 100 (attuatore):
   req = 2 -> manomissione contenitore a riposo
   req = 3 -> manomissione contenitore in allarme
*//*
      if(lcmd->Address.node >= 100)
      {
        database_lock();
        master_set_sabotage(((mm << 5) + node) << 3, lcmd->Msg[0], Manomissione_Contenitore);
        database_unlock();
      }
*/
      break;
//    case LARA_DISPLAY:
//      break;
//    case LARA_ATTUA:
//      break;
//    case LARA_IN_ATT:
//      break;
//    case LARA_RQ:
//      break;
//    case LARA_STAT_ATT:
//      break;
    case LARA_ID:
      lara_parse_LARA_ID(mm, lcmd->Address.node, (lara_t_id*)lcmd->Msg);
      break;
    case LARA_CMD:
#ifdef DEBUG
printf("**** LARA_CMD ****\n");
#endif
//      lara_parse_LARA_CMD(mm, lcmd->Address.node, (lara_t_cmd*)lcmd->Msg);
      break;
    case LARA_NODES:
      /* 20091105 */
      /* Forza i nodi previsti, preservando i nodi presenti */
      lcmd->Code = LARA_NODES;
      lcmd->Length = 32;
      mod = 0;
      for(i=0; i<64; i++)
      {
        if(TipoPeriferica[i+(mm*32)] == TLB)
        {
          if(!(lcmd->Msg[i/8] & (1<<(i&7))))
          {
            lcmd->Msg[i/8] |= (1<<(i&7));
            mod = 1;
          }
        }
        else if(lcmd->Msg[i/8] & (1<<(i&7)))
        {
          lcmd->Msg[i/8] &= ~(1<<(i&7));
          mod = 1;
        }
      }
      if(mod)
      {
        /* 20100412 */
        /* Se varia la lista nodi previsti, forzo tutti i nodi presenti. */
        for(i=0; i<8; i++) lcmd->Msg[i+16] = lcmd->Msg[i];
        /************/
        lara_queue(mm, lcmd);
      }
      /************/
      
      /* Riabilito l'invio dei comandi normali */
      MMSuspend[mm] = 0;
      MMSuspend[mm+1] = 0;
      MMSuspend[mm+2] = 0;
      
      lara_nodes(lcmd->Msg+16, (mm<<5), TLB, list_nodes);
      lara_nodes(lcmd->Msg+18, (mm<<5)+16, TLB, list_nodes);
      lara_nodes(lcmd->Msg+20, ((mm+1)<<5), TLB, list_nodes);
      lara_nodes(lcmd->Msg+22, ((mm+1)<<5)+16, TLB, list_nodes);
      ev[0] = ((*(lcmd->Msg+28)>>4)&0x0f) | ((*(lcmd->Msg+29)<<4)&0xf0);
      ev[1] = ((*(lcmd->Msg+29)>>4)&0x0f) | ((*(lcmd->Msg+30)<<4)&0xf0);
      lara_nodes(ev, ((mm+2)<<5), SC8, list_nodes);
      list_nodes = 0;
      break;
    case TEBE_IN:
      tebe_parse(mm, lcmd->Address.node, (tebe_msg*)lcmd->Msg);
      break;
    case TEBE_BACKUP:
      master_behaviour = 2 - cmd[1];
      if(!master_switch[mm])
      {
        master_switch[mm] = 1;
        
    /* 20091105 */
#if 0
    /* 20081117 */
    /* Forza i nodi previsti */
    lcmd->Code = LARA_NODES;
    lcmd->Address.domain = 0;
    lcmd->Address.subnet = 2;
    lcmd->Address.node = 126;
    lcmd->Length = 32;
    memset(lcmd->Msg, 0, 32);
    for(i=0; i<64; i++)
    {
      if(TipoPeriferica[i+(mm*32)] == TLB)
      {
        lcmd->Msg[i/8] |= (1<<(i&7));
        lcmd->Msg[(i/8)+16] |= (1<<(i&7));
      }
    }
    lara_queue(mm, lcmd);
    /************/
#endif
        
        list_nodes = 1;
        lcmd->Code = LARA_RQ;
        lcmd->Address.domain = 0;
        lcmd->Address.subnet = 2;
        lcmd->Address.node = 126;
        lcmd->Length = 1;
        lcmd->Msg[0] = NODES;
        lara_queue(mm, lcmd);
        lara_time(mm);
      }
      else
      {
        master_behaviour += 2;
        if(master_behaviour == MASTER_STANDBY)
          exit(0);
      }
      break;
    default:
      break;
  }
}

void lara_actuators(int ind, unsigned char att, unsigned char mask)
{
  int i, mmbase, node;
  lara_tMsg cmd;
  
#if 0
  node = ind & 0x1f;
  mmbase = ind >> 5;
  if((mmbase >= 1) && (MMPresente[mmbase-1] == MM_LARA)) {node += 32; mmbase--;}
  if((mmbase >= 1) && (MMPresente[mmbase-1] == MM_LARA)) {node += 32; mmbase--;}
  if(node >= 64) node += 36;
#else
  node = ind & 0x1f;
  mmbase = ind >> 5;
  for(i=0; i<MAX_NUM_MM; i++)
  {
    if(MMPresente[i] == MM_LARA)
    {
      if((mmbase >= i) && (mmbase < (i+3)))
      {
        node += 32*(mmbase-i);
        if(node >= 64) node += 36;
        
        mmbase = i;
        break;
      }
      else
        i += 2;
    }
  }
#endif
  
  if(tebe_actuators(mmbase, node, att)) return;
  
  cmd.Code = LARA_ATTUA;
  cmd.Address.domain = 0;
  cmd.Address.subnet = 2;
  cmd.Address.node = node;
#ifdef DEBUG
#warning !!!!!!!!!!!!!!
printf("Attuatore (mm:%d node:%d) %d %02x %02x\n", mmbase, cmd.Address.node, ind, att, mask);
#endif
  cmd.Length = sizeof(lara_tActuator);
  memset(((lara_tActuator*)(cmd.Msg)), 0, sizeof(lara_tActuator));
  
  for(i=0; i<8; i++)
  {
    if(mask & 0x01)
    {
      if(att & 0x01)
        ((lara_tActuator*)(cmd.Msg))->comandi[i] = ON_T;
      else
        ((lara_tActuator*)(cmd.Msg))->comandi[i] = OFF_T;
    }
    att >>= 1;
    mask >>= 1;
  }
  
  /* gestione apriporta terminali Lara. */
  if(cmd.Address.node < 64)
  {
    /* l'attuatore 0 sui terminali e' impulsivo */
    AT[ind<<3] &= ~bitON;
    if(((lara_tActuator*)(cmd.Msg))->comandi[1] == ON_T)
    {
      ((lara_tActuator*)(cmd.Msg))->comandi[0] = ON_T;
    }
    else if(((lara_tActuator*)(cmd.Msg))->comandi[0] == ON_T)
    {
      if(!(AT[(ind<<3)+1] & bitON))
        ((lara_tActuator*)(cmd.Msg))->comandi[0] = TIMING;
      else
        ((lara_tActuator*)(cmd.Msg))->comandi[0] = NOOP;
    }
    else
    {
      ((lara_tActuator*)(cmd.Msg))->comandi[0] = OFF_T;
    }
    ((lara_tActuator*)(cmd.Msg))->comandi[1] = NOOP;
  }
  
  lara_queue(mmbase, &cmd);
}

int lara_update_day(struct tm *day, unsigned char *daytype)
{
  int i;
  struct tm df;
  time_t t, tf, tv;
  
  if(_lara)
  {
    memset(&df, 0, sizeof(struct tm));
    df.tm_mday = day->tm_mday;
    df.tm_mon = day->tm_mon;
    df.tm_year = 100;
    tv = mktime(&df);
    df.tm_year = day->tm_year;
    tf = mktime(&df);

    for(i=0; i<LARA_N_FESTIVI; i++)
    {
      if(_lara->festivo[i].tipofestivo)
      {
        df.tm_mday = _lara->festivo[i].periodo.inizio[0];
        df.tm_mon = _lara->festivo[i].periodo.inizio[1] - 1;
        if(_lara->festivo[i].tipofestivo & 0x04)
          df.tm_year = _lara->festivo[i].periodo.inizio[2] + 100;
        else
          df.tm_year = 100;
        t = mktime(&df);

        if(_lara->festivo[i].tipofestivo & 0x01)		// periodo
        {
          if(((_lara->festivo[i].tipofestivo & 0x04) && (tf >= t)) ||
             (!(_lara->festivo[i].tipofestivo & 0x04) && (tv >= t)))
          {
            df.tm_mday = _lara->festivo[i].periodo.fine[0];
            df.tm_mon = _lara->festivo[i].periodo.fine[1] - 1;
            if(_lara->festivo[i].tipofestivo & 0x04)
              df.tm_year = _lara->festivo[i].periodo.fine[2] + 100;
            t = mktime(&df);
            if(((_lara->festivo[i].tipofestivo & 0x04) && (tf < t)) ||
              (!(_lara->festivo[i].tipofestivo & 0x04) && (tv < t)))
            {
              if(daytype) *daytype = DAY_HOLIDAY;
              return DAY_HOLIDAY;
            }
          }
        }
        else	// singolo
        {
          if(((_lara->festivo[i].tipofestivo & 0x04) && (t == tf)) ||
             (!(_lara->festivo[i].tipofestivo & 0x04) && (t == tv)))
          {
            if(daytype) *daytype = DAY_HOLIDAY;
            return DAY_HOLIDAY;
          }
        }
      }
    }
  }
  
  return DAY_WORKDAY;
}

/*
void lara_reset_periph()
{
  char cmd[128];
  
  sprintf(cmd, "cp %s %s", ref_LaraFileName, LaraFile);
  system(cmd);

  lara_restart = 1;
}
*/

void lara_check_tessera(int idx, lara_Tessere *nuovo)
{
  unsigned char ev[8];
  
  ev[0] = Evento_Esteso2;
  ev[1] = Ex2_Lara;
  ev[3] = idx & 0xff;
  ev[4] = idx >> 8;
  
  if((_lara->tessera[idx].stato.s.abil == BADGE_VUOTO) || (_lara->tessera[idx].stato.s.abil == BADGE_CANC))
  {
    if((nuovo->stato.s.abil == BADGE_ABIL) || (nuovo->stato.s.abil == BADGE_DISABIL))
    {
      /* Creazione anagrafica volto */
      volto_param req;
      req.tipo = VOLTO_INSERT;
      req.anagrafica.id = idx;
      volto_req(&req);
      
      ev[2] = 35;	// ID inserito su centrale
      codec_queue_event(ev);
      
      /* Se creo la tessera in area 0, i contatori presenza sono già a posto.
         Se invece indico fin da subito un'area di appartenenza, devo gestire
         i contatori presenza. */
      if(nuovo->area && (nuovo->area < LARA_N_AREE))
      {
        int mm;
        for(mm=0; (mm<MM_NUM) && (MMPresente[mm]!=MM_LARA); mm++);
        mm += 2;
#if 0
        _lara->presenza[0].contatore--;
        _lara->presenza[nuovo->area].contatore++;
        database_lock();
        master_set_alarm(((mm+2) << 8) + nuovo->area + 128, 0, 1);
        database_unlock();
#else
        lara_set_area_counters(mm, 0, nuovo->area);
#endif
        _laraf->tessera_area_last_update[idx] = time(NULL);
      }
    }
  }
  else
  {
    if(_lara->tessera[idx].pincnt != nuovo->pincnt)
    {
      if(_lara->param & LARA_PARAM_COUNTER)
        ev[2] = 42;	// ID variato contatore
      else
        ev[2] = 31;	// ID variato PIN
      codec_queue_event(ev);
    }
    if(_lara->tessera[idx].stato.b != nuovo->stato.b)
    {
      if(_lara->tessera[idx].stato.s.abil != nuovo->stato.s.abil)
      {
        switch(nuovo->stato.s.abil)
        {
          case BADGE_DISABIL:
            ev[2] = 32; codec_queue_event(ev); break;	// ID disabilitato
          case BADGE_ABIL:
            ev[2] = 33; codec_queue_event(ev); break;	// ID abilitato
        }
      }
      if(((_lara->tessera[idx].stato.s.abil != nuovo->stato.s.abil) && 
          ((nuovo->stato.s.abil == BADGE_VUOTO) || (nuovo->stato.s.abil == BADGE_CANC))) ||
         ((_lara->tessera[idx].stato.b & 0x07) != (nuovo->stato.b & 0x07)))
      {
        ev[2] = 41;	// ID variato stato
        ev[5] = nuovo->stato.b;
        codec_queue_event(ev);
      }
    }
    if(_lara->tessera[idx].area != nuovo->area)
    {
      int mm;
      
      ev[2] = 36;	// ID forzato in area
      ev[5] = nuovo->area;
      codec_queue_event(ev);
      
      for(mm=0; (mm<MM_NUM) && (MMPresente[mm]!=MM_LARA); mm++);
      mm += 2;
      
      if(_lara->tessera[idx].area < LARA_N_AREE)
      {
#if 0
        _lara->presenza[_lara->tessera[idx].area].contatore--;
        if(_lara->tessera[idx].area > 0)
        {
          database_lock();
          master_set_alarm((mm << 8) + _lara->tessera[idx].area + 128,
            (_lara->presenza[_lara->tessera[idx].area].contatore>0)?0:1, 1);
          database_unlock();
        }
        
        _laraf->tessera_area_last_update[idx] = time(NULL);
        
        _lara->presenza[nuovo->area].contatore++;
        if(nuovo->area > 0)
        {
          database_lock();
          master_set_alarm((mm << 8) + nuovo->area + 128, 0, 1);
          database_unlock();
        }
#endif        
        lara_set_area_counters(mm, _lara->tessera[idx].area, nuovo->area);
        _laraf->tessera_area_last_update[idx] = time(NULL);
        lara_set_area_group(idx, nuovo->area, mm, _laraf->tessera_area_last_update[idx]);
        
        /* Se forzo la tessera in area 0 libero anche le eventuali associazioni di scorta. */
        if(nuovo->area == 0)
          tebe_escort_link(idx, TEBE_ESCORT_UNLINK, 1);
      }
    }
    
    if(!nuovo->p.s.profilo)
    {
      /* Impostato profilo individuale */
      if(_lara->tessera[idx].p.s.profilo != nuovo->p.s.profilo)
      {
        ev[2] = 37;	// ID associato a profilo
        ev[5] = nuovo->p.s.profilo;
        codec_queue_event(ev);
      }
      if(_lara->tessera[idx].p.s.profilo || (_lara->tessera[idx].p.s.fosett != nuovo->p.s.fosett))
      {
        ev[2] = 38;	// ID associato a F.O.Sett
        ev[5] = nuovo->p.s.fosett;
        codec_queue_event(ev);
      }
      if(_lara->tessera[idx].p.s.profilo || memcmp(_lara->tessera[idx].p.s.term, nuovo->p.s.term, 8))
      {
        ev[2] = 39;	// ID variata associazione terminali
        codec_queue_event(ev);
      }
      if(_lara->tessera[idx].p.s.profilo || memcmp(_lara->tessera[idx].p.s.fest, nuovo->p.s.fest, 4))
      {
        ev[2] = 40;	// ID variata associazione festività
        codec_queue_event(ev);
      }
    }
    else
    {
      int i, j;
      
      if(!_lara->tessera[idx].p.s.profilo || /* Da profilo individuale a profilo multiplo */
        memcmp(_lara->tessera[idx].p.profilo, nuovo->p.profilo, LARA_MULTIPROFILI)) /* Variazione del profilo multiplo */
      {
        /* Indico solo i profili variati */
        if(_lara->tessera[idx].p.s.profilo != nuovo->p.s.profilo)
        {
          ev[2] = 37;	// ID associato a profilo
          ev[5] = nuovo->p.s.profilo;
          codec_queue_event(ev);
        }
        for(i=1; i<LARA_MULTIPROFILI; i++)
        {
          if(nuovo->p.profilo[i])
          {
            for(j=1; (j<LARA_MULTIPROFILI)&&(nuovo->p.profilo[i]!=_lara->tessera[idx].p.profilo[j]); j++);
            if(j == LARA_MULTIPROFILI)
            {
              ev[2] = 63;	// ID associato a profilo secondario
              ev[5] = nuovo->p.profilo[i];
              codec_queue_event(ev);
            }
          }
        }
        for(i=1; i<LARA_MULTIPROFILI; i++)
        {
          if(_lara->tessera[idx].p.profilo[i])
          {
            for(j=1; (j<LARA_MULTIPROFILI)&&(nuovo->p.profilo[j]!=_lara->tessera[idx].p.profilo[i]); j++);
            if(j == LARA_MULTIPROFILI)
            {
              ev[2] = 64;	// ID cancellato profilo secondario
              ev[5] = _lara->tessera[idx].p.profilo[i];
              codec_queue_event(ev);
            }
          }
        }
      }
#if 0
      /* Indico tutti i profili */
      for(i=0; i<LARA_MULTIPROFILI; i++)
      {
        if(nuovo->p.profilo[i])
        {
          ev[2] = 37;
          ev[5] = nuovo->p.profilo[i];
          codec_queue_event(ev);
        }
      }
#endif
    }
  }
  
  if((nuovo->stato.s.abil == BADGE_CANC) || (nuovo->stato.s.abil == BADGE_VUOTO))
  {
    /* Cancellazione anagrafica volto */
    volto_param req;
    req.tipo = VOLTO_DELETE;
    req.anagrafica.id = idx;
    volto_req(&req);
    
    finger_delete(idx);	// cancella le impronte associate alla tessera
  }
}

void lara_check_profilo(int idx, lara_Profili *nuovo)
{
  unsigned char ev[8];
  
  ev[0] = Evento_Esteso2;
  ev[1] = Ex2_Lara;
  ev[3] = idx+1;
  
  if(_lara->profilo[idx].fosett != nuovo->fosett)
  {
    ev[2] = 43;
    ev[4] = nuovo->fosett;
    codec_queue_event(ev);
  }
  if(memcmp(_lara->profilo[idx].term, nuovo->term, 8))
  {
    ev[2] = 44;
    codec_queue_event(ev);
  }
  if(memcmp(_lara->profilo[idx].fest, nuovo->fest, 4))
  {
    ev[2] = 45;
    codec_queue_event(ev);
  }
  if(_lara->profilo[idx].stato.b != nuovo->stato.b)
  {
    ev[2] = 46;
    ev[4] = nuovo->stato.b;
    codec_queue_event(ev);
  }
}

void lara_check_term(int idx, lara_Terminali *nuovo)
{
  unsigned char ev[8];
  
  ev[0] = Evento_Esteso2;
  ev[1] = Ex2_Lara;
  ev[3] = idx+1;
  
  if(_lara->terminale[idx].conf.s.filtro != nuovo->conf.s.filtro)
  {
    ev[2] = 47;
    ev[4] = nuovo->conf.s.filtro;
    codec_queue_event(ev);
  }
  if((_lara->terminale[idx].area[0] != nuovo->area[0]) || (_lara->terminale[idx].area[1] != nuovo->area[1]))
  {
    ev[2] = 48;
    ev[4] = nuovo->area[0];
    ev[5] = nuovo->area[1];
    codec_queue_event(ev);
  }
  if(_lara->terminale[idx].fosett != nuovo->fosett)
  {
    ev[2] = 49;
    ev[4] = nuovo->fosett;
    codec_queue_event(ev);
  }
  if(memcmp(_lara->terminale[idx].fest, nuovo->fest, 4))
  {
    ev[2] = 50;
    codec_queue_event(ev);
  }
  if(_lara->terminale[idx].dincnt != nuovo->dincnt)
  {
    ev[2] = 51;
    codec_queue_event(ev);
  }
  if((_lara->terminale[idx].timeopen != nuovo->timeopen) ||
     (_lara->terminale[idx].timeopentimeout != nuovo->timeopentimeout) ||
     (_lara->terminale[idx].conf.s.tapbk != nuovo->conf.s.tapbk))
  {
    ev[2] = 52;
    ev[4] = nuovo->timeopen;
    ev[5] = nuovo->timeopentimeout;
    ev[6] = nuovo->conf.s.tapbk;
    codec_queue_event(ev);
  }
  if(_lara->terminale[idx].stato.b != nuovo->stato.b)
  {
    ev[2] = 53;
    ev[4] = nuovo->stato.b;
    codec_queue_event(ev);
  }
}

void lara_check_fosett(int idx, lara_FOSett *nuovo)
{
  unsigned char ev[8];
  
  if(memcmp(&(_lara->fasciaoraria[idx]), nuovo, sizeof(lara_FOSett)))
  {
    ev[0] = Evento_Esteso2;
    ev[1] = Ex2_Lara;
    ev[2] = 54;
    ev[3] = idx+1;
    codec_queue_event(ev);
  }
}

void lara_check_fest(int idx, lara_Festivi *nuovo)
{
  unsigned char ev[8];
  
  if(memcmp(&(_lara->festivo[idx]), nuovo, sizeof(lara_Festivi)))
  {
    ev[0] = Evento_Esteso2;
    ev[1] = Ex2_Lara;
    ev[2] = 55;
    ev[3] = idx+1;
    codec_queue_event(ev);
  }
}

void lara_conf_cmd(int cmd, unsigned char *data)
{
  unsigned char ev[64];
  
  if(!_lara) return;
  
  ev[0] = Evento_Esteso2;
  ev[1] = Ex2_Lara;
  ev[3] = data[0];
  
  switch(cmd)
  {
    /* Lettura */
    
    case 0:
//      if(*(unsigned short*)(data) >= BADGE_NUM) return;
      if(uchartoshort(data) >= BADGE_NUM) return;
      ev[2] = 1;
      ev[4] = data[1];
//      if(_lara->tessera[*(unsigned short*)(data)].stato.s.abil == BADGE_VUOTO)
      if(_lara->tessera[uchartoshort(data)].stato.s.abil == BADGE_VUOTO)
      {
//        memset(&(_lara->tessera[*(unsigned short*)(data)]), 0xff, 10);
//        memset((char*)&(_lara->tessera[*(unsigned short*)(data)])+10, 0, sizeof(lara_Tessere)-10);
        memset(&(_lara->tessera[uchartoshort(data)]), 0xff, 10);
        memset((char*)&(_lara->tessera[uchartoshort(data)])+10, 0, sizeof(lara_Tessere)-10);
      }
//      memcpy(ev+5, &(_lara->tessera[*(unsigned short*)(data)]), sizeof(lara_Tessere));
      memcpy(ev+5, &(_lara->tessera[uchartoshort(data)]), sizeof(lara_Tessere));
      break;
    case 1:
      if(!data[0] || (data[0] > LARA_N_PROFILI)) return;
      ev[2] = 2;
      memcpy(ev+4, &(_lara->profilo[data[0]-1]), sizeof(lara_Profili));
      break;
    case 2:
      if(!data[0] || (data[0] > LARA_N_TERMINALI)) return;
      ev[2] = 3;
      memcpy(ev+4, &(_lara->terminale[data[0]-1]), sizeof(lara_Terminali));
      break;
    case 3:
      if(data[0] >= LARA_N_AREE) return;
      ev[2] = 4;
      memcpy(ev+4, &(_lara->presenza[data[0]]), sizeof(lara_Presenze));
      break;
    case 4:
      if(!data[0] || (data[0] > LARA_N_FASCE)) return;
      ev[2] = 5;
      memcpy(ev+4, &(_lara->fasciaoraria[data[0]-1]), sizeof(lara_FOSett));
      break;
    case 5:
      if(!data[0] || (data[0] > LARA_N_FESTIVI)) return;
      ev[2] = 6;
      memcpy(ev+4, &(_lara->festivo[data[0]-1]), sizeof(lara_Festivi));
      break;
    
    /* Scrittura */
    
    case 6:
      ev[2] = 0;
      ev[4] = 1;
      ev[5] = data[0];
      ev[6] = data[1];
//      if(*(unsigned short*)(data) >= BADGE_NUM)
      if(uchartoshort(data) >= BADGE_NUM)
        ev[3] = 1;
      else
      {
        ev[3] = 0;
//        lara_check_tessera(*(unsigned short*)(data), (lara_Tessere*)(data+2));
//        memcpy(&(_lara->tessera[*(unsigned short*)(data)]), data + 2, sizeof(lara_Tessere));
//        _laraf->tessera[*(unsigned short*)(data)].ag = 1;	// invalida
        lara_check_tessera(uchartoshort(data), (lara_Tessere*)(data+2));
        memcpy(&(_lara->tessera[uchartoshort(data)]), data + 2, sizeof(lara_Tessere));
        _laraf->tessera[uchartoshort(data)].ag = 1;	// invalida
        lara_save(0);
      }
      break;
    case 7:
      ev[2] = 0;
      ev[4] = 2;
      ev[5] = data[0];
      ev[6] = 0;
      if(!data[0] || (data[0] > LARA_N_PROFILI))
        ev[3] = 1;
      else
      {
        ev[3] = 0;
        lara_check_profilo(data[0]-1, (lara_Profili*)(data+1));
        memcpy(&(_lara->profilo[data[0]-1]), data + 1, sizeof(lara_Profili));
        _laraf->profilo[data[0]-1] = 1;	// invalida
        lara_save(0);
      }
      break;
    case 8:
      ev[2] = 0;
      ev[4] = 3;
      ev[5] = data[0];
      ev[6] = 0;
      if(!data[0] || (data[0] > LARA_N_TERMINALI))
        ev[3] = 1;
      else if(!lara_terminal_prog[data[0]-1])
        ev[3] = 3;
      else
      {
        ev[3] = 0;
        lara_check_term(data[0]-1, (lara_Terminali*)(data+1));
        memcpy(&(_lara->terminale[data[0]-1]), data + 1, sizeof(lara_Terminali));
        _laraf->terminale[data[0]-1] = 1;	// invalida
        lara_save(0);
        volto_init();
      }
      break;
    case 9:
      ev[2] = 0;
      ev[4] = 4;
      ev[5] = data[0];
      ev[6] = 0;
      if(data[0] >= LARA_N_AREE)
        ev[3] = 1;
      else
      {
        ev[3] = 0;
        memcpy(&(_lara->presenza[data[0]]), data + 1, sizeof(lara_Presenze));
        lara_save(0);
      }
      break;
    case 10:
      ev[2] = 0;
      ev[4] = 5;
      ev[5] = data[0];
      ev[6] = 0;
      if(!data[0] || (data[0] > LARA_N_FASCE))
        ev[3] = 1;
      else
      {
        ev[3] = 0;
        lara_check_fosett(data[0]-1, (lara_FOSett*)(data+1));
        memcpy(&(_lara->fasciaoraria[data[0]-1]), data + 1, sizeof(lara_FOSett));
        _laraf->fasciaoraria[data[0]-1] = 1;	// invalida
//        lara_substitute_timings();
        lara_save(0);
      }
      break;
    case 11:
      ev[2] = 0;
      ev[4] = 6;
      ev[5] = data[0];
      ev[6] = 0;
      if(!data[0] || (data[0] > LARA_N_FESTIVI))
        ev[3] = 1;
      else
      {
        ev[3] = 0;
        lara_check_fest(data[0]-1, (lara_Festivi*)(data+1));
        memcpy(&(_lara->festivo[data[0]-1]), data + 1, sizeof(lara_Festivi));
        _laraf->festivo[data[0]-1] = 1;	// invalida
        lara_save(0);
      }
      break;
    case 12:
      tebe_result(data);
      ev[2] = 0;
      ev[3] = 0;
      ev[4] = 7;
      ev[5] = data[0];
      ev[6] = data[1];
      break;
    case 13:
      tebe_emergency(data[0], data[1]);
      ev[2] = 0;
      ev[3] = 0;
      ev[4] = 8;
      ev[5] = data[1];
      ev[6] = 0;
      break;
   
   /* Nota per evitare confusione in futuro:
      Le lettere sono quelle dei comandi SaetNet /L, e il
      numero è l'offset rispetto alla lettera 'a'.
      Il comando UDP invece è +1, quindi il comando UDP 15
      corrisponde alla creazione ID (14) e così via. */
      
    case 14:	// crea ID		o
      {
      volto_param req;
      req.tipo = VOLTO_INSERT;
//      req.anagrafica.id = *(unsigned short*)(data);
      req.anagrafica.id = uchartoshort(data);
      req.anagrafica.id2 = 0;
      req.anagrafica.imp = 0;
      req.anagrafica.term = 0;
      volto_req(&req);
      
      ev[2] = 0;
      ev[3] = 0;
      ev[4] = 9;
      ev[5] = req.anagrafica.id & 0xff;
      ev[6] = req.anagrafica.id >> 8;
      }
      break;
    case 15:	// cancella ID		p
      {
      volto_param req;
      req.tipo = VOLTO_DELETE;
//      req.anagrafica.id = *(unsigned short*)(data);
      req.anagrafica.id = uchartoshort(data);
      req.anagrafica.id2 = 0;
      req.anagrafica.imp = 0;
      req.anagrafica.term = 0;
      volto_req(&req);
      
      ev[2] = 0;
      ev[3] = 0;
      ev[4] = 10;
      ev[5] = req.anagrafica.id & 0xff;
      ev[6] = req.anagrafica.id >> 8;
      }
      break;
    case 16:	// predisposizione enroll	q
      {
      volto_param req;
      req.tipo = VOLTO_ENROLL;
//      req.anagrafica.id = *(unsigned short*)(data);
      req.anagrafica.id = uchartoshort(data);
      req.anagrafica.id2 = 0;
      req.anagrafica.imp = config.DeviceID;
      req.anagrafica.term = *(data+2);
      volto_req(&req);
      
      ev[2] = 0;
      ev[3] = 0;
      ev[4] = 11;
      ev[5] = req.anagrafica.id & 0xff;
      ev[6] = req.anagrafica.id >> 8;
      }
      break;
    case 17:	// annullamento enroll		r
      {
      volto_param req;
      req.tipo = VOLTO_ENR_CANC;
//      req.anagrafica.id = *(unsigned short*)(data);
      req.anagrafica.id = uchartoshort(data);
      req.anagrafica.id2 = 0;
      req.anagrafica.imp = config.DeviceID;
      req.anagrafica.term = *(data+2);
      volto_req(&req);
      
      ev[2] = 0;
      ev[3] = 0;
      ev[4] = 12;
      ev[5] = req.anagrafica.id & 0xff;
      ev[6] = req.anagrafica.id >> 8;
      }
      break;
    case 18:	// associazione ID-ID		s
      {
      volto_param req;
      req.tipo = VOLTO_JOIN;
//      req.anagrafica.id = *(unsigned short*)(data);
//      req.anagrafica.id2 = *(unsigned short*)(data+2);
      req.anagrafica.id = uchartoshort(data);
      req.anagrafica.id2 = uchartoshort(data+2);
      req.anagrafica.imp = 0;
      req.anagrafica.term = 0;
      volto_req(&req);
      
      ev[2] = 0;
      ev[3] = 0;
      ev[4] = 13;
      ev[5] = req.anagrafica.id & 0xff;
      ev[6] = req.anagrafica.id >> 8;
      }
      break;
    
    /* Creato per la gestione CEI ABI di AllSystem */
    case 19:
      lara_set_abil(*(unsigned short*)(data), data[2]);
      /* L'evento lo crea il lara_set_abil() */
      return;
      
    default:
      return;
  }
  codec_queue_event(ev);
}

void lara_join_id(int id1, int id2)
{
  if((id1 < lara_NumBadge) && (id2 < lara_NumBadge))
  {
    /* Definisce il gruppo */
    _laraex[id2].group = id2;
    /* Associa il secondo id al gruppo. */
    _laraex[id1].group = id2;
    /* Per il momento considero queste informazioni gestite dal
       ciclo normale delprogramma utente, quindi non salvo.
       Quando il GEMSS sarà in grado di gestirle pure lui, allora
       la macro potrà essere usata solo in inizializzazione o
       nemmeno più usata, tanto c'è il GEMSS che imposta.
       Ok, con questi presupposti non ha senso che si salvi mai
       attraverso questa macro... */
    //lara_save(0);
  }
}

