#include "tebe.h"
#include "master.h"
#include "codec.h"
#include "support.h"
#include "finger.h"

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <fcntl.h>

#define TLB 7

#define TEBE_INTERVALLO	20
#define TEBE_SEND_FINGER 6

static int tebe_idx = 0;
/* incici dei record da aggiornare nel ciclo normale */
static int tebe_tessera = 0;
static int tebe_profilo = 0;
static int tebe_terminale = 0;
static int tebe_fascia = 0;
static int tebe_festivo = 0;
/* indici dei record da aggiornare con priorita' */
static int tebe_tessera2 = -1;
static int tebe_profilo2 = -1;
static int tebe_terminale2 = -1;
static int tebe_fascia2 = -1;
static int tebe_festivo2 = -1;

/* gestione invio database impronte */
static int tebe_finger_curr = -1;
static int tebe_finger_last = -1;
static int tebe_finger_timer = 0;

static unsigned char term_is_tebe[LARA_N_TERMINALI+1];
static unsigned char term_is_lan[LARA_N_TERMINALI+1];
static unsigned char tebe_oos[LARA_N_TERMINALI+1];
static unsigned char tebe_oos_upd[LARA_N_TERMINALI+1] = {0, };
static unsigned char tebe_act[LARA_N_TERMINALI+1][2];
static unsigned char tebe_act_upd[LARA_N_TERMINALI+1] = {0, };
static timeout_t *tebe_oos_timer;
static timeout_t *tebe_act_timer;

int tebe_term_alive[LARA_N_TERMINALI] = {0, };

static void tebe_sensor_send(void *null, int mm);
static void tebe_actuator_send(void *null, int mm);

static int tebe_ethfd = -1;

static pthread_t tebe_pth[MM_NUM] = {0, };

static pthread_mutex_t tebe_escort_mutex = PTHREAD_MUTEX_INITIALIZER;
struct _escort {
  unsigned short visitor_id;
  unsigned short escort_id;
  unsigned int mask[(LARA_N_TERMINALI+1)/(sizeof(unsigned int)*8)];
  int retry;
  struct _escort *next;
} *escort_head = NULL, *escort_notify_head = NULL, *escort_notify_tail = NULL;
#define TEBE_ESCORT_UNLINK  0xffff

void tebe_escort_save()
{
  struct _escort *e;
  int fd;
  
  fd = open("/tmp/tebe_escort.bin", O_WRONLY|O_CREAT|O_TRUNC|O_SYNC, 0644);
  if(fd < 0) return;
  
  pthread_mutex_lock(&tebe_escort_mutex);
  for(e=escort_head; e; e=e->next)
    write(fd, e, 4);
  pthread_mutex_unlock(&tebe_escort_mutex);
  
  close(fd);
}

/* In caso di disassociazione non libero comunque l'id dalla lista
   in modo da mantenere lo stato corrente nel caso un terminale
   avesse bisogno di riallinearsi. Posso così inviare tutta la lista
   completa degli id visitatori noti. */
void tebe_escort_link(int vid, int eid, int save)
{
  struct _escort *e;
  int i, base;
  
  pthread_mutex_lock(&tebe_escort_mutex);
  /* Per sicurezza verifico che non ci sia già una associazione in corso,
     al fine di evitare duplicati difficili poi da gestire. */
  for(e=escort_head; e; e=e->next)
    if(e->visitor_id == vid)
    {
      e->escort_id = eid;
      break;
    }
  
  if(!e)
  {
    /* Inserisco l'associazione nella lista. */
    e = malloc(sizeof(struct _escort));
    e->visitor_id = vid;
    e->escort_id = eid;
    e->next = escort_head;
    escort_head = e;
  }
  
  /* Inserisco l'associazione anche nella lista notifiche
     (invio alla centrale che successivamente diffonde a
     tutti i terminali) */
  /* Devo però mantenere la sequenza temporale, quindi
     occorre agganciare sempre al fondo. */
  e = malloc(sizeof(struct _escort));
  e->visitor_id = vid;
  e->escort_id = eid;
  for(i=0; i<((LARA_N_TERMINALI+1)/(sizeof(unsigned int)*8)); i++)
    e->mask[i] = 0;
  for(base=0; (base<MM_NUM) && (MMPresente[base]!=MM_LARA); base++);
  base <<= 5;
  for(i=1; i<=LARA_N_TERMINALI; i++)
    if(StatoPeriferica[(base+i)>>3] & (1<<(i&7)))
      e->mask[i/(sizeof(unsigned int)*8)] |= (1<<(i&(sizeof(unsigned int)*8-1)));
  e->retry = 3;
  e->next = NULL;
  if(!escort_notify_head)
    escort_notify_head = escort_notify_tail = e;
  else
  {
    escort_notify_tail->next = e;
    escort_notify_tail = e;
  }
  pthread_mutex_unlock(&tebe_escort_mutex);
  
  if(save) tebe_escort_save();
}

void tebe_escort_load()
{
  struct {
    unsigned short vid;
    unsigned short eid;
  } e;
  int fd;
  
  fd = open("/tmp/tebe_escort.bin", O_RDONLY);
  if(fd < 0) return;
  
  while(read(fd, &e, 4) > 0)
    tebe_escort_link(e.vid, e.eid, 0);
  
  close(fd);
}

void tebe_escort_align(int node)
{
  /* Accodo le notifiche per tutte le scorte note,
     cercando di ottimizzare se più nodi si presentano
     assieme. */
  struct _escort *e, *n;
  int i;
  
  pthread_mutex_lock(&tebe_escort_mutex);
  for(e=escort_head; e; e=e->next)
  {
    /* Cerco l'associazione nella lista notifiche, se c'è
       aggiungo il nodo e reimposto i tentativi di trasmissione. */
    for(n=escort_notify_head; n; n=n->next)
      if(n->visitor_id == e->visitor_id)
      {
        /* l'associazione è già nella lista notifiche, la aggiorno. */
        n->mask[node/(sizeof(unsigned int)*8)] |= (1<<(node&(sizeof(unsigned int)*8-1)));
        n->retry = 3;
        break;
      }
    if(!n)
    {
      /* l'associazione NON è già nella lista notifiche, la aggiungo. */
      n = malloc(sizeof(struct _escort));
      n->visitor_id = e->visitor_id;
      n->escort_id = e->escort_id;
      for(i=0; i<((LARA_N_TERMINALI+1)/(sizeof(unsigned int)*8)); i++)
        n->mask[i] = 0;
      n->mask[node/(sizeof(unsigned int)*8)] |= (1<<(node&(sizeof(unsigned int)*8-1)));
      n->retry = 3;
      n->next = NULL;
      if(!escort_notify_head)
        escort_notify_head = escort_notify_tail = n;
      else
      {
        escort_notify_tail->next = n;
        escort_notify_tail = n;
      }
    }
  }
  pthread_mutex_unlock(&tebe_escort_mutex);
}

#ifdef TEBESA
#include "perif.h"
int tebe_send(perif_msg *msg);
#endif

void tebe_send_to(int mm, int node, unsigned char *msg)
{
  unsigned char buf[40];
  
  if(!msg || (msg[0]>31)) return;
  
#ifdef TEBESA
  if(node == 2)
  {
    memcpy(buf+2, msg, msg[0]+1);
    tebe_send((perif_msg*)buf);
    return;
  }
#endif

  if(mm >= 0)
  {
    /* LON */
    buf[0] = TEBE_OUT;
    buf[1] = 0;
    buf[2] = 2;
    buf[3] = node;
    memcpy(buf+4, msg, msg[0]+1);
    master_queue(mm, buf);
  }
  else
  {
    /* ETH */
    buf[0] = TEBE_OUT;
//    *(ushort*)(buf+1) = config.DeviceID;
    inttouchar(config.DeviceID, buf+1, 2);
    buf[3] = 126;
    buf[4] = node;
    memcpy(buf+5, msg+1, msg[0]);
    send(tebe_ethfd, buf, msg[0]+5, 0);
  }
}

void tebe_send_mm(int mm, unsigned char *msg)
{
  /* Invio broadcast */
  tebe_send_to(mm, 0xff, msg);
}

void tebe_send_lan(unsigned char *msg, int len)
{
//  *(ushort*)(msg+1) = config.DeviceID;
  inttouchar(config.DeviceID, msg+1, 2);
  send(tebe_ethfd, msg, len, 0);
}

int tebe_invalidate(mm)
{
  unsigned char cmd[8];
  int i;
  
  for(i=0; (i<BADGE_NUM) && (!(_laraf->tessera[i].ag&0x01)); i++);
  if(i<BADGE_NUM)
  {
    cmd[0] = 5;
    cmd[1] = CODE_INV_PASS;
//    *(unsigned short*)(cmd+2) = i;
    inttouchar(i, cmd+2, 2);
    for(; (i<BADGE_NUM) && (_laraf->tessera[i].ag&0x01); i++);
//    *(unsigned short*)(cmd+4) = i - 1;
    inttouchar(i-1, cmd+4, 2);
    tebe_send_mm(mm, cmd);
    return 1;
  }
  
  for(i=0; (i<LARA_N_PROFILI) && (!(_laraf->profilo[i]&0x01)); i++);
  if(i<LARA_N_PROFILI)
  {
    cmd[0] = 3;
    cmd[1] = CODE_INV_PROF;
    cmd[2] = i;
    for(; (i<LARA_N_PROFILI) && (_laraf->profilo[i]&0x01); i++);
    cmd[3] = i - 1;
    tebe_send_mm(mm, cmd);
    return 1;
  }
  
  for(i=0; (i<LARA_N_TERMINALI+1) && (!(_laraf->terminale[i]&0x01)); i++);
  if(i<LARA_N_TERMINALI+1)
  {
    cmd[0] = 3;
    cmd[1] = CODE_INV_TERM;
    cmd[2] = i;
    for(; (i<LARA_N_TERMINALI+1) && (_laraf->terminale[i]&0x01); i++);
    cmd[3] = i - 1;
    tebe_send_mm(mm, cmd);
    return 1;
  }
  
  for(i=0; (i<LARA_N_FASCE) && (!(_laraf->fasciaoraria[i]&0x01)); i++);
  if(i<LARA_N_FASCE)
  {
    cmd[0] = 3;
    cmd[1] = CODE_INV_FOS;
    cmd[2] = i;
    for(; (i<LARA_N_FASCE) && (_laraf->fasciaoraria[i]&0x01); i++);
    cmd[3] = i - 1;
    tebe_send_mm(mm, cmd);
    return 1;
  }
  
  for(i=0; (i<LARA_N_FESTIVI) && (!(_laraf->festivo[i]&0x01)); i++);
  if(i<LARA_N_FESTIVI)
  {
    cmd[0] = 3;
    cmd[1] = CODE_INV_FEST;
    cmd[2] = i;
    for(; (i<LARA_N_FESTIVI) && (_laraf->festivo[i]&0x01); i++);
    cmd[3] = i - 1;
    tebe_send_mm(mm, cmd);
    return 1;
  }
  
  return 0;
}

int tebe_send_db_pri(int mm)
{
  unsigned char buf[40];
  
  for(tebe_tessera2=0; (tebe_tessera2<BADGE_NUM)&&(_laraf->tessera[tebe_tessera2].ag != 2); tebe_tessera2++);
  if(tebe_tessera2 == BADGE_NUM) tebe_tessera2 = -1;
  
  if(tebe_tessera2 >= 0)
  {
    buf[0] = 23;
    buf[1] = CODE_BADGE_1;
//    *(unsigned short*)(buf+2) = tebe_tessera2;
    inttouchar(tebe_tessera2, buf+2, 2);
    memcpy(buf+4, (unsigned char*)&(_lara->tessera[tebe_tessera2]), 20);
    tebe_send_mm(mm, buf);
    buf[0] = sizeof(lara_Tessere)-20+3;
    buf[1] = CODE_BADGE_2;
    memcpy(buf+4, (unsigned char*)&(_lara->tessera[tebe_tessera2])+20, sizeof(lara_Tessere)-20);
    tebe_send_mm(mm, buf);
    return 1;
  }
  
  for(tebe_profilo2=0; (tebe_profilo2<LARA_N_PROFILI)&&(_laraf->profilo[tebe_profilo2] != 2); tebe_profilo2++);
  if(tebe_profilo2 == LARA_N_PROFILI) tebe_profilo2 = -1;
  
  if(tebe_profilo2 >= 0)
  {
    buf[0] = 2 + sizeof(lara_Profili);
    buf[1] = CODE_PROF;
    buf[2] = tebe_profilo2;
    memcpy(buf+3, &(_lara->profilo[tebe_profilo2]), sizeof(lara_Profili));
    tebe_send_mm(mm, buf);
    return 1;
  }
  
  for(tebe_terminale2=0; (tebe_terminale2<(LARA_N_TERMINALI+1))&&(_laraf->terminale[tebe_terminale2] != 2); tebe_terminale2++);
  if(tebe_terminale2 == (LARA_N_TERMINALI+1)) tebe_terminale2 = -1;
  
  if(tebe_terminale2 >= 0)
  {
    buf[0] = 2 + sizeof(lara_Terminali);
    buf[1] = CODE_TERM;
    buf[2] = tebe_terminale2;
    memcpy(buf+3, &(_lara->terminale[tebe_terminale2]), sizeof(lara_Terminali));
    tebe_send_mm(mm, buf);
    return 1;
  }
  
  for(tebe_fascia2=0; (tebe_fascia2<LARA_N_FASCE)&&(_laraf->fasciaoraria[tebe_fascia2] != 2); tebe_fascia2++);
  if(tebe_fascia2 == LARA_N_FASCE) tebe_fascia2 = -1;
  
  if(tebe_fascia2 >= 0)
  {
    buf[0] = 2 + 28;
    buf[1] = CODE_FOS_1;
    buf[2] = tebe_fascia2;
    memcpy(buf+3, &(_lara->fasciaoraria[tebe_fascia2]), 28);
    tebe_send_mm(mm, buf);
    buf[0] = 2 + sizeof(lara_FOSett) - 28;
    buf[1] = CODE_FOS_2;
    memcpy(buf+3, ((char*)&(_lara->fasciaoraria[tebe_fascia2]))+28, sizeof(lara_FOSett) - 28);
    tebe_send_mm(mm, buf);
    return 1;
  }
  
  for(tebe_festivo2=0; (tebe_festivo2<LARA_N_FESTIVI)&&(_laraf->festivo[tebe_festivo2] != 2); tebe_festivo2++);
  if(tebe_festivo2 == LARA_N_FESTIVI) tebe_festivo2 = -1;
  
  if(tebe_festivo2 >= 0)
  {
    buf[0] = 2 + sizeof(lara_Festivi);
    buf[1] = CODE_FEST;
    buf[2] = tebe_festivo2;
    memcpy(buf+3, &(_lara->festivo[tebe_festivo2]), sizeof(lara_Festivi));
    tebe_send_mm(mm, buf);
    return 1;
  }
  
  return 0;
}

int tebe_notify_escort(int mm)
{
  struct _escort *e;
  unsigned char buf[6+sizeof(escort_notify_head->mask)];
  
  if(escort_notify_head)
  {
    if(escort_notify_head->retry)
    {
      escort_notify_head->retry--;
      buf[0] = 4+sizeof(escort_notify_head->mask);
      buf[1] = CODE_ESCORT;
      memcpy(buf+2, escort_notify_head, 4);
      memcpy(buf+6, escort_notify_head->mask, sizeof(escort_notify_head->mask));
      tebe_send_mm(mm, buf);
      return 1;
    }
    else
    {
      /* Esauriti i tentativi elimino la notifica. */
      e = escort_notify_head;
      escort_notify_head = e->next;
      if(!escort_notify_head) escort_notify_tail = NULL;
      free(e);
    }
  }
  return 0;
}

int tebe_send_db(int mm)
{
  unsigned char buf[40];
  
  if(tebe_finger_curr >= 0)
  {
    finger_send(mm, tebe_finger_curr);
    return 1;
  }
  
  if(tebe_notify_escort(mm)) return 0;
  if(tebe_invalidate(mm)) return 0;
  if(tebe_send_db_pri(mm)) return 1;
  
  if(tebe_idx < TEBE_INTERVALLO)
  {
    buf[0] = 23;
    buf[1] = CODE_BADGE_1;
//    *(unsigned short*)(buf+2) = tebe_tessera;
    inttouchar(tebe_tessera, buf+2, 2);
    memcpy(buf+4, (unsigned char*)&(_lara->tessera[tebe_tessera]), 20);
    tebe_send_mm(mm, buf);
    buf[0] = sizeof(lara_Tessere)-20+3;
    buf[1] = CODE_BADGE_2;
    memcpy(buf+4, (unsigned char*)&(_lara->tessera[tebe_tessera])+20, sizeof(lara_Tessere)-20);
    tebe_send_mm(mm, buf);
    return 1;
  }
  switch(tebe_idx)
  {
    case TEBE_INTERVALLO:
      buf[0] = 2 + sizeof(lara_Profili);
      buf[1] = CODE_PROF;
      buf[2] = tebe_profilo;
      memcpy(buf+3, &(_lara->profilo[tebe_profilo]), sizeof(lara_Profili));
      tebe_send_mm(mm, buf);
      break;
    case TEBE_INTERVALLO+1:
      buf[0] = 2 + sizeof(lara_Terminali);
      buf[1] = CODE_TERM;
      buf[2] = tebe_terminale;
      memcpy(buf+3, &(_lara->terminale[tebe_terminale]), sizeof(lara_Terminali));
      tebe_send_mm(mm, buf);
      break;
    case TEBE_INTERVALLO+2:
      buf[0] = 2 + 28;
      buf[1] = CODE_FOS_1;
      buf[2] = tebe_fascia;
      memcpy(buf+3, &(_lara->fasciaoraria[tebe_fascia]), 28);
      tebe_send_mm(mm, buf);
      buf[0] = 2 + sizeof(lara_FOSett) - 28;
      buf[1] = CODE_FOS_2;
      memcpy(buf+3, ((char*)&(_lara->fasciaoraria[tebe_fascia]))+28, sizeof(lara_FOSett) - 28);
      tebe_send_mm(mm, buf);
      break;
    case TEBE_INTERVALLO+3:
      buf[0] = 2 + sizeof(lara_Festivi);
      buf[1] = CODE_FEST;
      buf[2] = tebe_festivo;
      memcpy(buf+3, &(_lara->festivo[tebe_festivo]), sizeof(lara_Festivi));
      tebe_send_mm(mm, buf);
      break;
  }
  return 1;
}

void tebe_inc_db()
{
  if(tebe_finger_curr >= 0)
  {
    tebe_finger_timer = TEBE_SEND_FINGER;
    tebe_finger_last = tebe_finger_curr;
    tebe_finger_curr = -1;
    return;
  }
    
  if(tebe_finger_timer)
  {
    tebe_finger_timer--;
    if(!tebe_finger_timer)
    {
      tebe_finger_curr = finger_next_valid(tebe_finger_last);
      return;
    }
  }
  
  if(tebe_tessera2 >= 0)
  {
    _laraf->tessera[tebe_tessera2].ag = 0;
    tebe_tessera2 = -1;
    return;
  }
  if(tebe_profilo2 >= 0)
  {
    _laraf->profilo[tebe_profilo2] = 0;
    tebe_profilo2 = -1;
    return;
  }
  if(tebe_terminale2 >= 0)
  {
    _laraf->terminale[tebe_terminale2] = 0;
    tebe_terminale2 = -1;
    return;
  }
  if(tebe_fascia2 >= 0)
  {
    _laraf->fasciaoraria[tebe_fascia2] = 0;
    tebe_fascia2 = -1;
    return;
  }
  if(tebe_festivo2 >= 0)
  {
    _laraf->festivo[tebe_festivo2] = 0;
    tebe_festivo2 = -1;
    return;
  }
  
  if(tebe_idx < TEBE_INTERVALLO)
  {
    tebe_tessera++;
    tebe_tessera %= BADGE_NUM;
  }
  else
  {
    switch(tebe_idx)
    {
      case TEBE_INTERVALLO:
        tebe_profilo++;
        tebe_profilo %= LARA_N_PROFILI;
        break;
      case TEBE_INTERVALLO+1:
        tebe_terminale++;
        tebe_terminale %= (LARA_N_TERMINALI+1);
        break;
      case TEBE_INTERVALLO+2:
        tebe_fascia++;
        tebe_fascia %= LARA_N_FASCE;
        break;
      case TEBE_INTERVALLO+3:
        tebe_festivo++;
        tebe_festivo %= LARA_N_FESTIVI;
        break;
    }
  }
  tebe_idx++;
  tebe_idx %= (TEBE_INTERVALLO+4);
}

void tebe_timer()
{
  int i, inc = 0;
//  char ev[4];
  
#if 0
  /* Terminali LON */
  for(i=0; (i<(LARA_N_TERMINALI+1))&&(!term_is_tebe[i] || term_is_lan[i]) ; i++);
  if(i <= LARA_N_TERMINALI)
  {
    for(i=0; i<MM_NUM; i++)
      if(MMPresente[i] == MM_LARA)
      {
        
        i+=2;
      }
  }
  
  /* Terminali LAN */
  for(i=0; (i<(LARA_N_TERMINALI+1))&&(!term_is_tebe[i] || !term_is_lan[i]) ; i++);
  if(i <= LARA_N_TERMINALI)
    ;
#endif

  /* Terminali LON */
  for(i=0; (i<(LARA_N_TERMINALI+1))&&(!term_is_tebe[i] || term_is_lan[i]) ; i++);
  if(i <= LARA_N_TERMINALI)
  {
    for(i=0; i<MM_NUM; i++)
      if(MMPresente[i] == MM_LARA)
      {
        inc = tebe_send_db(i);
        i+=2;
      }
  }
  
  /* Terminali LAN */
  for(i=0; (i<(LARA_N_TERMINALI+1))&&(!term_is_tebe[i] || !term_is_lan[i]) ; i++);
  if(i <= LARA_N_TERMINALI)
    inc = tebe_send_db(-1);
  
  if(inc) tebe_inc_db();
  
  for(inc=0; (inc<MM_NUM) && (MMPresente[inc]!=MM_LARA); inc++);
  inc <<= 5;
  for(i=0; i<LARA_N_TERMINALI; i++)
  {
    if(term_is_lan[i+1] && tebe_term_alive[i])
    {
      tebe_term_alive[i]--;
      if(!tebe_term_alive[i])
      {
        /* Fuori linea */
        StatoPeriferica[(inc+i+1)>>3] &= ~(1<<((i+1)&0x07));
#if 0
        ev[0] = Periferica_Manomessa;
#ifdef GEST16MM
        ev[1] = (inc+i+1)>>8;
        ev[2] = inc+i+1;
#else
        ev[1] = inc+i+1;
#endif
        codec_queue_event(ev);
#endif
        tebe_manomissione(inc+i+1);
      }
    }
  }
}

int tebe_find_badge(unsigned char *badge)
{
  int i;
  
  for(i=0; i<BADGE_NUM; i++)
    if(((_lara->tessera[i].stato.s.abil == BADGE_ABIL) || (_lara->tessera[i].stato.s.abil == BADGE_DISABIL))
       && !memcmp(badge, _lara->tessera[i].badge, 10)) return i;
  
  return -1;
}

void tebe_set_area(int id, int area, int mm, int time)
{
  if(time < _laraf->tessera_area_last_update[id]) return;
  _laraf->tessera_area_last_update[id] = time;
  
#if 0
  _lara->presenza[_lara->tessera[id].area].contatore--;
  if((_lara->tessera[id].area > 0) && (_lara->tessera[id].area < LARA_N_AREE))
    master_set_alarm(((mm+2) << 8) + _lara->tessera[id].area + 128,
    (_lara->presenza[_lara->tessera[id].area].contatore>0)?0:1, 1);
  _lara->tessera[id].area = area;
  _lara->presenza[_lara->tessera[id].area].contatore++;
  if((_lara->tessera[id].area > 0) && (_lara->tessera[id].area < LARA_N_AREE))
    master_set_alarm(((mm+2) << 8) + _lara->tessera[id].area + 128, 0, 1);
  _laraf->tessera[id].ag = 1;	// invalida
#else
  lara_set_area_counters(mm+2, _lara->tessera[id].area, area);
  _lara->tessera[id].area = area;
  _laraf->tessera[id].ag = 1;	// invalida
#endif

  lara_set_area_group(id, area, mm+2, time);
  lara_save(0);
}

void tebe_set_fe_alarm(int id, int fe, int mm, int node, int evtime)
{
  if(((time(NULL) - evtime) > 60) || (evtime < _laraf->tessera_area_last_update[id])) return;
  _laraf->tessera_area_last_update[id] = evtime;
  
  database_lock();
  lara_timbrato[(node<<1)+fe].id = id;
  lara_timbrato[(node<<1)+fe].profilo = _lara->tessera[id].p.s.profilo;
  lara_timbrato[(node<<1)+fe].time = time(NULL);
  ME[(node<<1)+fe+640] = _lara->tessera[id].p.s.profilo;
  master_set_alarm((mm<<8)+(node<<3)+fe, 1, 1);
  master_set_alarm((mm<<8)+(node<<3)+fe, 0, 1);
  database_unlock();
}

int tebe_event(unsigned char *msg, int node, unsigned char *ev, int mm2, int evtime)
{
  int id, mm;
  unsigned char buf[8];
  
  if(mm2 < 0)
    for(mm=0; (mm<MM_NUM)&&(MMPresente[mm]!=MM_LARA); mm++);
  else
    mm = mm2;
  
  ev[0] = 247;
  ev[1] = 0;
  ev[2] = msg[6];
  
  switch(msg[6])
  {
    case 9:
//      id = *(ushort*)(msg+7);
      id = uchartoshort(msg+7);
      tebe_set_area(id, msg[9], mm, evtime);
      lara_giustificativo[node] = uchartoshort(msg+10);
      if(msg[9] == _lara->terminale[node].area[0])	// Fe 1
        tebe_set_fe_alarm(id, 1, mm, node, evtime);
      else	// Fe 0
        tebe_set_fe_alarm(id, 0, mm, node, evtime);
      /* Invio lo sblocco tessera */
      buf[0] = 5;
      buf[1] = CODE_BLKBADGE;
      buf[2] = id & 0xff;
      buf[3] = id >> 8;
      /* Sblocco */
      /* Aziché 0 indico 1/10 di secondo per sicurezza. */
      buf[4] = 1;
      buf[5] = 0;
      tebe_send_mm(mm2, buf);
    case 10:
      ev[3] = msg[7];
      ev[4] = msg[8];
      ev[5] = node;
      ev[6] = msg[9];
      ev[7] = msg[10];
      ev[8] = msg[11];
//      id = *(ushort*)(msg+7);
      id = uchartoshort(msg+7);
      if(_lara->param & LARA_PARAM_COUNTER)
      {
//        _lara->tessera[id].pincnt -= *(int*)(msg+12);
        _lara->tessera[id].pincnt -= uchartoint(msg+12);
        _laraf->tessera[id].ag = 1;	// invalida
        lara_save(0);
      }
      if(msg[6] == 10)
      {
        lara_giustificativo[node] = uchartoshort(msg+10);
        tebe_set_fe_alarm(id, msg[9], mm, node, evtime);
      }
      break;
    case 7:
//      id = *(ushort*)(msg+7);
      id = uchartoshort(msg+7);
      tebe_set_area(id, msg[9], mm, evtime);
      lara_giustificativo[node] = 0xffff;
      if(msg[9] == _lara->terminale[node].area[0])	// Fe 1
        tebe_set_fe_alarm(id, 1, mm, node, evtime);
      else	// Fe 0
        tebe_set_fe_alarm(id, 0, mm, node, evtime);
      /* Invio lo sblocco tessera */
      buf[0] = 5;
      buf[1] = CODE_BLKBADGE;
      buf[2] = id & 0xff;
      buf[3] = id >> 8;
      /* Sblocco */
      /* Aziché 0 indico 1/10 di secondo per sicurezza. */
      buf[4] = 1;
      buf[5] = 0;
      tebe_send_mm(mm2, buf);
    case 8:
//      id = *(ushort*)(msg+7);
      id = uchartoshort(msg+7);
      if(_lara->param & LARA_PARAM_COUNTER)
      {
//        _lara->tessera[id].pincnt -= *(int*)(msg+12);
        _lara->tessera[id].pincnt -= uchartoint(msg+12);
        _laraf->tessera[id].ag = 1;	// invalida
        lara_save(0);
      }
      if(msg[6] == 8)
      {
        lara_giustificativo[node] = 0xffff;
        tebe_set_fe_alarm(id, msg[9], mm, node, evtime);
      }
    case 11:
    case 12:
    case 13:
    case 14:
    case 16:
    case 17:
      ev[3] = msg[7];
      ev[4] = msg[8];
      ev[5] = node;
      ev[6] = msg[9];
      break;
    case 15:
      ev[3] = msg[7];
      ev[4] = msg[8];
      ev[5] = msg[9];
      ev[6] = node;
      break;
    case 18:
      ev[3] = node;
      break;
    case 21:
      if((time(NULL) - evtime) < 60)
        master_set_alarm((mm<<8)+(node<<3)+6, 1, 1);
      ev[3] = node;
      break;
    case 22:
      if((time(NULL) - evtime) < 60)
        master_set_alarm((mm<<8)+(node<<3)+7, 1, 1);
      ev[3] = node;
      break;
    case 23:
      if((time(NULL) - evtime) < 60)
      {
        master_set_alarm((mm<<8)+(node<<3)+6, 0, 1);
        master_set_alarm((mm<<8)+(node<<3)+7, 0, 1);
      }
      ev[3] = node;
      break;
    case 24:
      if((time(NULL) - evtime) < 60)
        master_set_alarm((mm<<8)+(node<<3)+4, 1, 1);
      ev[3] = node;
      break;
    case 25:
      if((time(NULL) - evtime) < 60)
        master_set_alarm((mm<<8)+(node<<3)+4, 0, 1);
      ev[3] = node;
      break;
    case 32:
//      id = *(ushort*)(msg+7);
      id = uchartoshort(msg+7);
      lara_set_abil(id, BADGE_DISABIL);
      return 0;
    case 61:
      ev[3] = msg[7];
      ev[4] = msg[8];
      ev[5] = node;
      ev[6] = msg[10];
      ev[7] = msg[11];
      break;
    case 67:
    case 68:
      ev[3] = node;
      break;
    default:
      return 0;
  }
  
  return 1;
}

void tebe_term_status(int mm, int node, unsigned char status)
{
  int idx;
  
#if 0
  if(mm == -1)
    idx = (32 + node) << 3;
  else
    idx = ((mm << 5) + node) << 3;
#else
  if(mm == -1)
    for(mm=0; (mm<MM_NUM)&&(MMPresente[mm]!=MM_LARA); mm++);
  idx = ((mm << 5) + node) << 3;
#endif
            
  /* Manomissione contenitore */
  master_set_sabotage(idx, status >> 7, Manomissione_Contenitore);
      
  idx += 3;
  /* Porta aperta */
  master_set_alarm(idx, status >> 5, 1);
  
  idx ++;
  /* Ingresso ausiliario */
  master_set_alarm(idx, status >> 6, 1);
  idx++;
  /* Coercizione */
  master_set_alarm(idx, status >> 2, 1);
  idx++;
  /* Forzatura porta */
  master_set_alarm(idx, status, 1);
  idx++;
  /* Apertura prolungata */
  master_set_alarm(idx, status >> 1, 1);
}

void tebe_parse(int mm, int node, tebe_msg *msg)
{
  unsigned char buf[40];
  int i, idx; //, mask;
  
  if((mm >= 0) && (TipoPeriferica[(mm<<5)+node] != TLB) && (msg->code != CODE_TERM_PROGRAM))
  {
    buf[0] = 1;
    buf[1] = CODE_STOP;
    //tebe_send_to(mm, node, buf);
    
    return;
  }
  
  switch(msg->code)
  {
    case CODE_BADGE_REQ:
      idx = tebe_find_badge(msg->msg);
      if(idx < 0)
      {
        buf[0] = 11;
        buf[1] = CODE_BADGE_INV;
        memcpy(buf+2, msg->msg, 10);
        tebe_send_mm(mm, buf);
      }
      else
      {
        buf[0] = 23;
        buf[1] = CODE_BADGE_1;
//        *(unsigned short*)(buf+2) = idx;
        inttouchar(idx, buf+2, 2);
        memcpy(buf+4, (unsigned char*)&(_lara->tessera[idx]), 20);
        tebe_send_mm(mm, buf);
        buf[0] = sizeof(lara_Tessere)-20+3;
        buf[1] = CODE_BADGE_2;
        memcpy(buf+4, (unsigned char*)&(_lara->tessera[idx])+20, sizeof(lara_Tessere)-20);
        tebe_send_mm(mm, buf);
      }
      break;
    case CODE_ID_REQ:
//      idx = *(unsigned short*)(msg->msg);
//      *(unsigned short*)(buf+2) = idx;
      idx = uchartoshort(msg->msg);
      inttouchar(idx, buf+2, 2);
      if(idx >= BADGE_NUM)
      {
        buf[0] = 3;
        buf[1] = CODE_ID_INV;
        tebe_send_mm(mm, buf);
        break;
      }
      buf[0] = 23;
      buf[1] = CODE_BADGE_1;
      memcpy(buf+4, (unsigned char*)&(_lara->tessera[idx]), 20);
      tebe_send_mm(mm, buf);
      buf[0] = sizeof(lara_Tessere)-20+3;
      buf[1] = CODE_BADGE_2;
      memcpy(buf+4, (unsigned char*)&(_lara->tessera[idx])+20, sizeof(lara_Tessere)-20);
      tebe_send_mm(mm, buf);
      _laraf->tessera[idx].ag = 0;
      break;
    case CODE_TERM_REQ:
      buf[0] = 2 + sizeof(lara_Terminali);
      buf[1] = CODE_TERM;
      buf[2] = msg->msg[0];
      memcpy(buf+3, &(_lara->terminale[msg->msg[0]]), sizeof(lara_Terminali));
      tebe_send_mm(mm, buf);
      _laraf->terminale[msg->msg[0]] = 0;
      break;
    case CODE_PROF_REQ:
      buf[0] = 2 + sizeof(lara_Profili);
      buf[1] = CODE_PROF;
      buf[2] = msg->msg[0];
      memcpy(buf+3, &(_lara->profilo[msg->msg[0]]), sizeof(lara_Profili));
      tebe_send_mm(mm, buf);
      _laraf->profilo[msg->msg[0]] = 0;
      break;
    case CODE_FOS_REQ:
      buf[0] = 2 + 28;
      buf[1] = CODE_FOS_1;
      buf[2] = msg->msg[0];
      memcpy(buf+3, &(_lara->fasciaoraria[msg->msg[0]]), 28);
      tebe_send_mm(mm, buf);
      buf[0] = 2 + sizeof(lara_FOSett) - 28;
      buf[1] = CODE_FOS_2;
      memcpy(buf+3, ((char*)&(_lara->fasciaoraria[msg->msg[0]]))+28, sizeof(lara_FOSett) - 28);
      tebe_send_mm(mm, buf);
      _laraf->fasciaoraria[msg->msg[0]] = 0;
      break;
    case CODE_FEST_REQ:
      buf[0] = 2 + sizeof(lara_Festivi);
      buf[1] = CODE_FEST;
      buf[2] = msg->msg[0];
      memcpy(buf+3, &(_lara->festivo[msg->msg[0]]), sizeof(lara_Festivi));
      tebe_send_mm(mm, buf);
      _laraf->festivo[msg->msg[0]] = 0;
      break;
    case CODE_CREATE_BADGE_REQ:
//      idx = *(int*)(msg->msg+10);
      idx = uchartoint(msg->msg+10);
      for(;(idx<BADGE_NUM) &&
           ((_lara->tessera[idx].stato.s.abil == BADGE_ABIL) ||
            (_lara->tessera[idx].stato.s.abil == BADGE_DISABIL)); idx++);
      if(idx<BADGE_NUM)
      {
        memcpy(_lara->tessera[idx].badge, msg->msg, 10);
        _lara->tessera[idx].stato.s.badge = BADGE_PROGR;
        _lara->tessera[idx].stato.s.coerc = 0;
        _lara->tessera[idx].stato.s.apbk = 0;
        _lara->tessera[idx].area = 0;
        _lara->tessera[idx].pincnt = 0;
        memset(_lara->tessera[idx].p.profilo, 0, LARA_MULTIPROFILI);
        _lara->tessera[idx].p.s.profilo = 1;
        _lara->tessera[idx].stato.s.abil = BADGE_ABIL;
        
/*
Impostando il profilo 1 per default, queste impostazioni non sono utilizzate.
Inoltre, con i multiprofili queste impostazioni vanno in conflitto con i profili secondari.
        mask = 0;
          
        for(i=0; i<LARA_N_FESTIVI; i++)
        {
          if(_lara->festivo[i].tipofestivo)
            mask |= (1L << i);
        }
        *(int*)_lara->tessera[idx].p.s.fest = mask;
        *(int*)_lara->tessera[idx].p.s.term = 1;
        *(int*)(_lara->tessera[idx].p.s.term + 4) = 0;
        _lara->tessera[idx].p.s.fosett = 0;
*/        
        _laraf->tessera[idx].ag = 1;	// invalida
        
        buf[0] = Evento_Esteso2;
        buf[1] = Ex2_Lara;
        buf[2] = 34;
        buf[3] = idx & 0xff;
        buf[4] = idx >> 8;
        buf[5] = node;
        codec_queue_event(buf);
        lara_save(0);
        
        buf[0] = 16;
        buf[1] = CODE_RESULT;
        buf[2] = RESULT_CREATE_BADGE;
        memcpy(buf+3, msg->msg, 10);
//        *(int*)(buf+13) = idx;
        inttouchar(idx, buf+13, sizeof(int));
        tebe_send_to(mm, node, buf);
      }
      break;
    case CODE_CREATE_PIN_REQ:
//      idx = *(int*)(msg->msg+2);
      idx = uchartoint(msg->msg+2);
      for(;(idx<BADGE_NUM) &&
           ((_lara->tessera[idx].stato.s.abil == BADGE_ABIL) ||
            (_lara->tessera[idx].stato.s.abil == BADGE_DISABIL)); idx++);
      if(idx<BADGE_NUM)
      {
        memcpy(_lara->tessera[idx].badge, msg->msg, 10);
        _lara->tessera[idx].stato.s.badge = SEGR_PROGR;
        _lara->tessera[idx].stato.s.coerc = 0;
        _lara->tessera[idx].stato.s.apbk = 0;
        _lara->tessera[idx].area = 0;
//        _lara->tessera[idx].pincnt = *(unsigned short*)(msg->msg);
        _lara->tessera[idx].pincnt = uchartoshort(msg->msg);
        memset(_lara->tessera[idx].p.profilo, 0, LARA_MULTIPROFILI);
        _lara->tessera[idx].p.s.profilo = 1;
        _lara->tessera[idx].stato.s.abil = BADGE_ABIL;
        
/*
Impostando il profilo 1 per default, queste impostazioni non sono utilizzate.
Inoltre, con i multiprofili queste impostazioni vanno in conflitto con i profili secondari.
        mask = 0;
          
        for(i=0; i<LARA_N_FESTIVI; i++)
        {
          if(_lara->festivo[i].tipofestivo)
            mask |= (1L << i);
        }
        *(int*)_lara->tessera[idx].p.s.fest = mask;
        *(int*)_lara->tessera[idx].p.s.term = 1;
        *(int*)(_lara->tessera[idx].p.s.term + 4) = 0;
        _lara->tessera[idx].p.s.fosett = 0;
*/
        
        _laraf->tessera[idx].ag = 1;	// invalida
        
        buf[0] = Evento_Esteso2;
        buf[1] = Ex2_Lara;
        buf[2] = 34;
        buf[3] = idx & 0xff;
        buf[4] = idx >> 8;
        buf[5] = node;
        codec_queue_event(buf);
        lara_save(0);
        
        buf[0] = 8;
        buf[1] = CODE_RESULT;
        buf[2] = RESULT_CREATE_PIN;
//        *(unsigned short*)(buf+3) = *(unsigned short*)(msg->msg);
        memcpy(buf+3, msg->msg, 2);
        inttouchar(idx, buf+5, sizeof(int));
        tebe_send_to(mm, node, buf);
      }
      break;
    case CODE_CHANGE_PIN_REQ:
//      lara_set_secret(*(unsigned short*)(msg->msg), *(unsigned short*)(msg->msg+2), node);
      lara_set_secret(uchartoshort(msg->msg), uchartoshort(msg->msg+2), node);
      buf[0] = 6;
      buf[1] = CODE_RESULT;
      buf[2] = RESULT_CHANGE_PIN;
      memcpy(buf+3, msg->msg, 4);
      tebe_send_to(mm, node, buf);
      break;
    case CODE_CHANGE_DINCONT_REQ:
      lara_set_term_counter(node-1, uchartoint(msg->msg));
      buf[0] = 2;
      buf[1] = CODE_RESULT;
      buf[2] = RESULT_CHANGE_DINCONT;
      tebe_send_to(mm, node, buf);
      break;
    case CODE_CHANGE_BLKBADGE_REQ:
      _lara->terminale[node-1].stato.s.blkbadge = msg->msg[0];
      _laraf->terminale[node-1] = 1;	// invalida
      lara_save(0);
      buf[0] = 2;
      buf[1] = CODE_RESULT;
      buf[2] = RESULT_CHANGE_BLKBADGE;
      tebe_send_to(mm, node, buf);
      break;
    case CODE_CHANGE_TAPBK_REQ:
      lara_set_term_times(node-1, _lara->terminale[node-1].timeopen,
//        _lara->terminale[node-1].timeopentimeout, *(int*)(msg->msg));
        _lara->terminale[node-1].timeopentimeout, uchartoint(msg->msg));
      buf[0] = 2;
      buf[1] = CODE_RESULT;
      buf[2] = RESULT_CHANGE_TAPBK;
      tebe_send_to(mm, node, buf);
      break;
    case CODE_CHANGE_TAP_REQ:
      lara_set_term_times(node-1, _lara->terminale[node-1].timeopen,
//        *(int*)(msg->msg), _lara->terminale[node-1].conf.s.tapbk);
        uchartoint(msg->msg), _lara->terminale[node-1].conf.s.tapbk);
      buf[0] = 2;
      buf[1] = CODE_RESULT;
      buf[2] = RESULT_CHANGE_TAP;
      tebe_send_to(mm, node, buf);
      break;
    case CODE_CHANGE_ONRELE_REQ:
//      lara_set_term_times(node-1, *(int*)(msg->msg),
      lara_set_term_times(node-1, uchartoint(msg->msg),
        _lara->terminale[node-1].timeopentimeout,
        _lara->terminale[node-1].conf.s.tapbk);
      buf[0] = 2;
      buf[1] = CODE_RESULT;
      buf[2] = RESULT_CHANGE_ONRELE;
      tebe_send_to(mm, node, buf);
      break;
    case CODE_INV_PASS_OK:
//      for(i=(*(unsigned short*)(msg->msg));i<(*(unsigned short*)(msg->msg+2))+1;i++)
      for(i=(uchartoshort(msg->msg));i<(uchartoshort(msg->msg+2))+1;i++)
        _laraf->tessera[i].ag = 2;
      break;
    case CODE_INV_PROF_OK:
      memset(_laraf->profilo + msg->msg[0], 2, msg->msg[1] - msg->msg[0] + 1);
      break;
    case CODE_INV_TERM_OK:
      memset(_laraf->terminale + msg->msg[0], 2, msg->msg[1] - msg->msg[0] + 1);
      break;
    case CODE_INV_FOS_OK:
      memset(_laraf->fasciaoraria + msg->msg[0], 2, msg->msg[1] - msg->msg[0] + 1);
      break;
    case CODE_INV_FEST_OK:
      memset(_laraf->festivo + msg->msg[0], 2, msg->msg[1] - msg->msg[0] + 1);
      break;
    case CODE_EVENT:
//      if(!tebe_event(msg->msg, node, buf, mm, *(int*)(msg->msg+2)) ||
//         codec_try_queue_event(buf, *(int*)(msg->msg+2)))
      if(!tebe_event(msg->msg, node, buf, mm, uchartoint(msg->msg+2)) ||
         codec_try_queue_event(buf, uchartoint(msg->msg+2)))
        buf[1] = CODE_EVENT_OK;
      else
        buf[1] = CODE_EVENT_DELAY;
      buf[0] = 5;
//      *(int*)(buf+2) = *(unsigned short*)(msg->msg);
      inttouchar(uchartoshort(msg->msg), buf+2, sizeof(int));
      tebe_send_to(mm, node, buf);
      break;
    case CODE_STATUS:
      /* se i sensori sono fuori servizio vengono mascherati sulla centrale */
      tebe_term_status(mm, node, msg->msg[0]);
      buf[0] = 2;
      buf[1] = CODE_STATUS_OK;
      buf[2] = msg->msg[0];
      tebe_send_to(mm, node, buf);
      if(!term_is_tebe[node])
      {
        /* viene forzato l'aggiornamento dello stato di fuori servizio
           cosi' da mascherare gli allarmi anche sul terminale */
        term_is_tebe[node] = 1;
        tebe_sensor_send(NULL, mm);
        tebe_actuator_send(NULL, mm);
      }
      break;
    case CODE_SENSORS_OK:
      if(msg->msg[0] == tebe_oos[node])
        tebe_oos_upd[node] = 0;
      break;
    case CODE_ACTUATORS_OK:
//      if(*(ushort*)(msg->msg) == *(ushort*)(tebe_act[node]))
      if(uchartoshort(msg->msg) == uchartoshort(tebe_act[node]))
        tebe_act_upd[node] = 0;
      break;
    case CODE_PARAM_REQ:
      buf[0] = 5;
      buf[1] = CODE_PARAM;
//      *(int*)(buf+2) = _lara->param;
      inttouchar(_lara->param, buf+2, sizeof(int));
      tebe_send_mm(mm, buf);
      /* Accoda anche l'allineamento delle associazioni scorta. */
      tebe_escort_align(node);
      break;
    case CODE_TERM_PROGRAM:
      buf[0] = 2;
      buf[1] = CODE_TERM_PROGRAM_OK;
      buf[2] = msg->msg[0];
      tebe_send_mm(mm, buf);
#if 0
      if(mm == -1) mm = 1;
#else
  if(mm == -1)
    for(mm=0; (mm<MM_NUM)&&(MMPresente[mm]!=MM_LARA); mm++);
#endif
      lara_terminal_prog[msg->msg[0]-1] = 1;
      tebe_term_alive[msg->msg[0]-1] = KEEPALIVE_TIMEOUT;
      TipoPeriferica[(mm<<5)+msg->msg[0]] = TLB;
//      StatoPeriferica[((mm<<5)+msg->msg[0])/8] |= (1<<(((mm<<5)+msg->msg[0])&0x07));
      database_changed = 1;
      break;
    case CODE_KEEPALIVE:
      tebe_term_alive[node-1] = KEEPALIVE_TIMEOUT;
      break;
    case CODE_FINGER_REQ:
//      if(*(unsigned short*)(msg->msg) == 0xffff)
      if(uchartoshort(msg->msg) == 0xffff)
      {
        buf[0] = 3;
        buf[1] = CODE_FINGER;
        buf[2] = 0xff;
        buf[3] = 0xff;
        tebe_send_mm(mm, buf);
        if(!tebe_finger_timer)
          tebe_finger_timer = 10;
        tebe_finger_last = 0;
      }
//      else if(*(unsigned short*)(msg->msg) != 0x0000)
//        finger_send(mm, *(unsigned short*)(msg->msg));
      else if(uchartoshort(msg->msg) != 0x0000)
        finger_send(mm, uchartoshort(msg->msg));
      break;
    case CODE_FINGER:
      buf[0] = 5;
      buf[1] = CODE_FINGER_ACK;
//      *(int*)(buf+2) = *(int*)(msg->msg);
      memcpy(buf+2, msg->msg, sizeof(int));
      tebe_send_to(mm, node, buf);
      if(finger_store(msg->msg))
//        finger_send(mm, *(unsigned short*)(msg->msg));
        finger_send(mm, uchartoshort(msg->msg));
      break;
    case CODE_ESCORT:
      /* Comunicazione da parte di un terminale di avvenuta
         associazione tra tessera e scorta (o tessera liberata).
         Occorre diffondere questa comunicazione a tutti i terminali
         in linea e la centrale deve anche mantenere copia delle
         associazioni da reinoltrare ai terminali che rientrano in
         linea. */
      tebe_escort_link(msg->msg[0]+msg->msg[1]*256, msg->msg[2]+msg->msg[3]*256, 1);
      break;
    case CODE_ESCORT_OK:
      /* Elimino il nodo dai destinatari della notifica globale,
         se la maschera nodi si azzera allora elimino la notifica dalla coda */
      if(!memcmp(msg->msg, escort_notify_head, 4))
      {
        escort_notify_head->mask[node/(sizeof(unsigned int)*8)] &= ~(1<<(node&(sizeof(unsigned int)*8-1)));
        for(i=0; i<((LARA_N_TERMINALI+1)/(sizeof(unsigned int)*8)); i++)
          if(escort_notify_head->mask[i]) break;
        if(i == ((LARA_N_TERMINALI+1)/(sizeof(unsigned int)*8)))
        {
          /* Tutti i terminali hanno confermato */
          struct _escort *e;
          e = escort_notify_head;
          escort_notify_head = e->next;
          if(!escort_notify_head) escort_notify_tail = NULL;
          free(e);
        }
      }
      break;
    case CODE_BLKBADGE:
      /* Un badge ha timbrato su un terminale configurato per il blocco,
         rimbalzo la notifica in broadcast indicando il tempo di blocco.
         Per ora il tempo non è configurabile, ma dovesse diventarlo questo
         è il punto dove viene comunicato. */
      /* Il blocco va inviato se esistono terminali configurati per il blocco,
         ma se ricevo questo messaggio vuol dire che almeno un terminale lo è.
         Potrebbe essere l'unico, ma avrebbe poco senso per cui assumo ce ne
         sia almeno un altro. */
      buf[0] = 5;
      buf[1] = CODE_BLKBADGE;
      buf[2] = msg->msg[0];
      buf[3] = msg->msg[1];
      /* Blocco per 2 minuti */
      buf[4] = (2*60*10) & 0xff;
      buf[5] = (2*60*10) >> 8;
      tebe_send_mm(mm, buf);
      break;
    default:
      break;
  }
}

static void tebe_sensor_send(void *null, int mm)
{
  int i, base, inviato;
  char cmd[4];
  
  if(tebe_oos_timer->active) return;
  
#if 0
  if(mm == -1)
    base = 32;
  else
    base = mm << 5;
#else
  if(mm == -1)
  {
    for(i=0; (i<MM_NUM)&&(MMPresente[i]!=MM_LARA); i++);
    base = i << 5;
  }
  else
    base = mm << 5;
#endif
  
  inviato = 0;
  for(i=1; i<LARA_N_TERMINALI+1; i++)
  {
    if(master_periph_present(base+i))
    {
      if(term_is_tebe[i] && tebe_oos_upd[i])
      {
        if(term_is_lan[i]) mm = -1;
        cmd[0] = 2;
        cmd[1] = CODE_SENSORS;
        cmd[2] = tebe_oos[i];
        tebe_send_to(mm, i, cmd);
        inviato++;
      }
    }
    else if(term_is_tebe[i])
    {
      /* se scollego un terminale tebe e ne collego uno lara, evito di tentare in continuazione
         di mandare lo stato di fuori servizio dato che non avra' risposta. Quando un terminale
         tebe viene riconnesso, invia lo stato degli ingressi dichiarandosi di tipo tebe e
         rischedulando l'invio degli stati di fuori servizio */
      term_is_tebe[i] = 0;
    }
  }
  if(inviato) timeout_on(tebe_oos_timer, tebe_sensor_send, NULL, mm, 6);
}

static void tebe_actuator_send(void *null, int mm)
{
  int i, base, inviato;
  char cmd[4];
  
  if(tebe_act_timer->active) return;
  
#if 0
  if(mm == -1)
    base = 32;
  else
    base = mm << 5;
#else
  if(mm == -1)
  {
    for(i=0; (i<MM_NUM)&&(MMPresente[i]!=MM_LARA); i++);
    base = i << 5;
  }
  else
    base = mm << 5;
#endif
  
  inviato = 0;
  for(i=1; i<LARA_N_TERMINALI+1; i++)
  {
    if(master_periph_present(base+i))
    {
#ifdef TEBESA
      if((term_is_tebe[i] || (i == 2)) && tebe_act_upd[i])
#else
      if(term_is_tebe[i] && tebe_act_upd[i])
#endif
      {
        if(term_is_lan[i]) mm = -1;
        cmd[0] = 3;
        cmd[1] = CODE_ACTUATORS;
        cmd[2] = tebe_act[i][0];
        cmd[3] = tebe_act[i][1];
        tebe_send_to(mm, i, cmd);
#ifdef TEBESA
        if(i == 2)
          tebe_act_upd[2] = 0;
        else
#endif
        inviato++;
      }
    }
    else if(term_is_tebe[i])
    {
      term_is_tebe[i] = 0;
    }
  }
  if(inviato) timeout_on(tebe_act_timer, tebe_actuator_send, NULL, mm, 6);
}

void* tebe_udp_recv(void *pmm)
{
  struct sockaddr_in sa;
  int fdi, len, mm;
  unsigned char buf[64]; //, ev[4];
  
  pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
  
  debug_pid[DEBUG_PID_TEBEU] = support_getpid();
  sprintf(buf, "Tebe UDP recv [%d]", debug_pid[DEBUG_PID_TEBEU]);
  support_log(buf);
  
  sa.sin_family = AF_INET;
  sa.sin_port = htons(4100);
  sa.sin_addr.s_addr = INADDR_ANY;
  fdi = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
  bind(fdi, (struct sockaddr*)&sa, sizeof(struct sockaddr));
  
  mm = (int)pmm;
  
  while(1)
  {
    len = recv(fdi, buf, 64, 0);
//    if((buf[0] == TEBE_OUT) && (*(ushort*)(buf+1) == config.DeviceID) && (buf[4] == 126))
    if((buf[0] == TEBE_OUT) && (uchartoshort(buf+1) == config.DeviceID) && (buf[4] == 126))
    {
      term_is_lan[buf[3]] = 1;
      
/*
      for(i=0; i<LARA_N_TERMINALI; i++)
      {
        if(term_is_lan[i+1] && tebe_term_alive[i])
          tebe_term_alive[i] = KEEPALIVE_TIMEOUT;
      }
*/
      
      if((TipoPeriferica[(mm<<5)+buf[3]] == TLB) &&
         !(StatoPeriferica[((mm<<5)+buf[3])>>3] & (1<<(buf[3]&0x07))))
      {
        StatoPeriferica[((mm<<5)+buf[3])>>3] |= (1<<(buf[3]&0x07));
        tebe_term_alive[buf[3]-1] = KEEPALIVE_TIMEOUT;
#if 0
        ev[0] = Periferica_Ripristino;
#ifdef GEST16MM
        ev[1] = ((mm<<5)+buf[3])>>8;
        ev[2] = (mm<<5)+buf[3];
#else
        ev[1] = (mm<<5)+buf[3];
#endif
        codec_queue_event(ev);
#endif
        tebe_ripristino((mm<<5)+buf[3]);
      }
      tebe_parse(-1, buf[3], (tebe_msg*)(buf+5));
    }
  }
  
  return NULL;
}

void tebe_init(int mm)
{
  int t, s, base;
  struct sockaddr_in sa;
  struct ifreq ifr;
  static int tebe_active = 0;
  
  if(tebe_active) return;
  tebe_active = 1;
  
  memset(tebe_oos, 0, sizeof(tebe_oos));
  memset(tebe_act, 0, sizeof(tebe_act));
  memset(term_is_tebe, 0, sizeof(term_is_tebe));
  memset(term_is_lan, 0, sizeof(term_is_lan));
  for(t=1; t<(LARA_N_TERMINALI+1); t++)
  {
    base = (mm<<8)+(t<<3);
    for(s=0; s<8; s++)
    {
      if(SE[base+s] & bitOOS) tebe_oos[t] |= (1<<s);
      if(AT[base+s] & bitON) tebe_act[t][0] |= (1<<s);
      if(AT[base+s] & bitOOS) tebe_act[t][1] |= (1<<s);
    }
  }
  tebe_oos_timer = timeout_init();
  tebe_act_timer = timeout_init();
  
  finger_init();
  
  sa.sin_family = AF_INET;
  sa.sin_port = htons(4100);
  tebe_ethfd = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
  strcpy(ifr.ifr_name, "eth0");
  ioctl(tebe_ethfd, SIOCGIFBRDADDR, &ifr);
  sa.sin_addr.s_addr = ((struct sockaddr_in*)(&(ifr.ifr_broadaddr)))->sin_addr.s_addr;
  s = 1;
  setsockopt(tebe_ethfd, SOL_SOCKET, SO_BROADCAST, &s, sizeof(int));
  connect(tebe_ethfd, (struct sockaddr*)&sa, sizeof(struct sockaddr));
  
  pthread_create(&tebe_pth[mm], NULL, tebe_udp_recv, (void*)mm);
  pthread_detach(tebe_pth[mm]);
  
  tebe_escort_load();
}

void tebe_stop()
{
  int i;
  
  for(i=0; i<MM_NUM; i++)
  {
    if(tebe_pth[i])
      pthread_cancel(tebe_pth[i]);
  }
}

void tebe_ripristino(int ind)
{
  int node, mm;
  unsigned char ev[4];
  
#ifdef GEST16MM
  ev[1] = ind >> 8;
  ev[2] = ind;
#else
  ev[1] = ind;
#endif
  master_periph_restore(ind<<3, ev);
  
  node = ind & 0x1f;
  mm = ind >> 5;
  if((mm >= 1) && (MMPresente[mm-1] == MM_LARA)) {node += 32; mm--;}
  if((mm >= 1) && (MMPresente[mm-1] == MM_LARA)) return;	// actuators
  
  /* invio dello stato di fuori servizio sensori */
  tebe_oos_upd[node] = 1;
  tebe_sensor_send(NULL, mm);
  /* invio dello stato di fuori servizio attuatori */
  tebe_act_upd[node] = 1;
  tebe_actuator_send(NULL, mm);
}

void tebe_manomissione(int ind)
{
  int node, mm;
  unsigned char ev[4];
  int basedev, tmp;
  
  basedev = (ind<<3)+1;
  ev[0] = Periferica_Manomessa;
#ifdef GEST16MM
  ev[1] = ind >> 8;
  ev[2] = ind;
#else
  ev[1] = ind;
#endif
  M_LINEA = ind;
  database_lock();
  if(!(SE[basedev] & bitOOS))
  {
    SEmu[basedev] |= bitSEmuLineSab;
    SE[basedev] |= bitSabotage | bitLineSabotage;
    tmp = database_testandset_MU(basedev, bitMUSabotage);
  }
  else
    tmp = 0;
  if(tmp)
  {
    SE[basedev] |= bitGestSabotage | bitLineLHF;
    codec_queue_event(ev);
    master_queue_sensor_event(basedev, MU_sabotage);          
  }
  else
  {
    if(!(SE[basedev] & bitMUNoSabotage))
    {
      SE[basedev] |= bitGestSabotage | bitLineLHF;
      codec_queue_event(ev);
    }
  }
  database_unlock();
  
  node = ind & 0x1f;
  mm = ind >> 5;
  if((mm >= 1) && (MMPresente[mm-1] == MM_LARA)) {node += 32; mm--;}
  if((mm >= 1) && (MMPresente[mm-1] == MM_LARA)) return;	// actuators
  
  term_is_tebe[node] = 0;
  term_is_lan[node-1] = 0;
}

void tebe_sensor_on(int idx)
{
  int node, mm;
  
  node = (idx>>3) & 0x1f;
  mm = idx >> 8;
  if((mm >= 1) && (MMPresente[mm-1] == MM_LARA)) {node += 32; mm--;}
  if((mm >= 1) && (MMPresente[mm-1] == MM_LARA)) return;	// actuators
  
  tebe_oos[node] &= ~(1<<(idx&0x07));
  tebe_oos_upd[node] = 1;
  tebe_sensor_send(NULL, mm);
}

void tebe_sensor_off(int idx)
{
  int node, mm;
  
  node = (idx>>3) & 0x1f;
  mm = idx >> 8;
  if((mm >= 1) && (MMPresente[mm-1] == MM_LARA)) {node += 32; mm--;}
  if((mm >= 1) && (MMPresente[mm-1] == MM_LARA)) return;	// actuators
  
  tebe_oos[node] |= (1<<(idx&0x07));
  tebe_oos_upd[node] = 1;
  tebe_sensor_send(NULL, mm);
}

int tebe_actuators(int mm, int nodo, unsigned char att)
{
#ifdef TEBESA
  if((nodo>LARA_N_TERMINALI) || ((nodo != 2) && !term_is_tebe[nodo])) return 0;
#else
  if((nodo>LARA_N_TERMINALI) || !term_is_tebe[nodo]) return 0;
#endif
  
  tebe_act[nodo][0] = att;
  tebe_act_upd[nodo] = 1;
  tebe_actuator_send(NULL, mm);
  
  return 1;
}

void tebe_actuator_on(int idx)
{
  /* la chiamata a questa funzione implica la successiva chiamata a tebe_actuators() */
  int node, mm;
  
  node = (idx>>3) & 0x1f;
  mm = idx >> 8;  
  if((mm >= 1) && (MMPresente[mm-1] == MM_LARA)) {node += 32; mm--;}
  if((mm >= 1) && (MMPresente[mm-1] == MM_LARA)) return;	// actuator;
  
  tebe_act[node][1] &= ~(1<<(idx&0x07));
}

void tebe_actuator_off(int idx)
{
  /* la chiamata a questa funzione implica la successiva chiamata a tebe_actuators() */
  int node, mm;
  
  node = (idx>>3) & 0x1f;
  mm = idx >> 8;
  if((mm >= 1) && (MMPresente[mm-1] == MM_LARA)) {node += 32; mm--;}
  if((mm >= 1) && (MMPresente[mm-1] == MM_LARA)) return;	// actuators
  
  tebe_act[node][1] |= (1<<(idx&0x07));
}

void tebe_result(unsigned char *res)
{
  int mm;
  unsigned char cmd[32];
  
  if(!res[2] || (res[2] > LARA_N_TERMINALI)) return;
  
#ifdef TEBESA
  if(res[2] == 2)
    mm = -1;
  else
#endif
  if(term_is_lan[res[2]])
    mm = -1;
  else if(term_is_tebe[res[2]])
  {
    for(mm=0; (mm<MM_NUM)&&(MMPresente[mm]!=MM_LARA); mm++);
    if(mm == MM_NUM) return;
  }
  else
    return;
  
  cmd[2] = 27;
  cmd[3] = CODE_RESULT;
  cmd[4] = RESULT_DATA_REQUEST;
  memcpy(cmd+5, res, 2+1+2+(4*5));
#ifdef TEBESA
  if(res[2] == 2)
  {
    tebe_send((perif_msg*)cmd);
    return;
  }
  else
#endif
  tebe_send_to(mm, res[2], cmd+2);
}

void tebe_emergency(int term, int emergenza)
{
  int mm;
  unsigned char cmd[32];
  
  if((term != 0xff) && (!term || (term > LARA_N_TERMINALI))) return;
  
  cmd[2] = 3;
  cmd[3] = CODE_RESULT;
  cmd[4] = RESULT_EMERGENCY;
  cmd[5] = emergenza;
  
  if(term == 0xff)
  {
#ifdef TEBESA
    tebe_send((perif_msg*)cmd);
#endif
    
    tebe_send_to(-1, term, cmd+2);
    for(mm=0; (mm<MM_NUM)&&(MMPresente[mm]!=MM_LARA); mm++);
    if(mm == MM_NUM) return;
    tebe_send_to(mm, term, cmd+2);
  }
  else
#ifdef TEBESA
  if(term == 2)
    mm = -1;
  else
#endif
  if(term_is_lan[term])
    mm = -1;
  else if(term_is_tebe[term])
  {
    for(mm=0; (mm<MM_NUM)&&(MMPresente[mm]!=MM_LARA); mm++);
    if(mm == MM_NUM) return;
  }
  else
    return;
  
#ifdef TEBESA
  if(term == 2)
  {
    tebe_send((perif_msg*)cmd);
    return;
  }
  else
#endif
  tebe_send_to(mm, term, cmd+2);
}

void tebe_send_finger(int id)
{
  int i;
  
  /* Terminali LON */
  for(i=0; (i<(LARA_N_TERMINALI+1))&&(!term_is_tebe[i] || term_is_lan[i]) ; i++);
  if(i <= LARA_N_TERMINALI)
  {
    for(i=0; i<MM_NUM; i++)
      if(MMPresente[i] == MM_LARA)
      {
        finger_send(i, id);
        i+=2;
      }
  }
  
  /* Terminali LAN */
  for(i=0; (i<(LARA_N_TERMINALI+1))&&(!term_is_tebe[i] || !term_is_lan[i]) ; i++);
  if(i <= LARA_N_TERMINALI) finger_send(-1, id);
}
