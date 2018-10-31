#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <ctype.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include "codec.h"
#include "database.h"
#include "command.h"
#include "list.h"
#include "master.h"
#include "badge.h"
#include "delphi.h"
#include "gsm.h"
#include "modem.h"
#include "user.h"
#include "lara.h"
#include "ronda.h"
#include "support.h"

//#define DEBUG_SAETNET
//#define DEBUG_CMD

static Event NullEvent = {STX, 0, 0, 0};

#if 0
static int EventInIdx = 0;
/* Il consumatore 8 e' utilizzato per liberare i buffer
   allocati nell'accodamento degli eventi, quando non
   sono piu' necessari. */
static int EventOutIdx[MAX_NUM_CONSUMER + 1] = {0, };
static unsigned char EventBuf[DIM_EVENT_BUFFER];
#else
static struct _events {
  int Init;
  int nConsumers;
  int EventInIdx;
  int EventOutIdx[MAX_NUM_CONSUMER + 1];
  unsigned char EventBuf[DIM_EVENT_BUFFER];
} *Events = NULL;
#endif

static pthread_mutex_t EventMutex = PTHREAD_MUTEX_INITIALIZER;
timeout_t *timer_accept_actuators = NULL;

static InitFunction codec_sync = NULL;
static ProtDevice *codec_syncdev = NULL;
static int codec_sync_cont = 0;

static const unsigned char EventDim[] = {
	 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	//   0
	 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	//  10
	 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	//  20
	 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	//  30
	 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	//  40
	 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	//  50
	 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	//  60
	 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	//  70
	 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	//  80
	 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	//  90
	 
	 /* Lara */
	 1, 1, 4, 4, 4, 3, 2, 3, 3, 2,	// 100
	 5, 2, 3, 3, 2, 2, 3, 3, 3, 3,	// 110
	 1, 1, 1, 1, 3, 4, 1, 1, 3, 4,	// 120
	 0, 1, 6, 6, 2, 4, 4, 4, 1, 1,	// 130
	 1, 1, 1, 1, 1, 1, 1, 1, 0, 0,	// 140
     
     /* Delphi */
#ifndef GEST16MM
	 3, 3, 1, 1, 2, 2, 1, 1, 1, 1,	// 150
	 1, 2, 2, 2, 2, 1, 2, 2, 1, 1,	// 160
	 1, 1, 1, 1, 1, 1, 1, 0, 2, 9,	// 170
	 9,10,10, 2, 2, 6, 3, 2, 1, 2,	// 180
	 3, 1, 1, 0, 2, 3, 9, 3, 4, 1,	// 190
	20, 0,17,13,17, 9, 2, 3, 3, 3,	// 200
	 1, 1, 1, 2, 2, 2, 1, 0, 1, 9,	// 210
	 9,17, 4,10,10,17,17, 2, 9,11,	// 220
#else
	 3, 3, 2, 2, 2, 2, 2, 2, 1, 1,	// 150
	 1, 2, 2, 2, 2, 1, 2, 3, 2, 2,	// 160
	 1, 2, 2, 2, 2, 1, 1, 0, 2,10,	// 170
	 9,10,10, 2, 2, 6, 3, 2, 2, 2,	// 180
	 4, 1, 1, 0, 2, 3,10, 3, 4, 1,	// 190
	20, 0,17,13,17, 9, 2, 4, 4, 4,	// 200
	 1, 1, 1, 2, 2, 2, 1, 0, 1, 9,	// 210
	 9,18, 4,10,10,17,17, 2,10,11,	// 220
#endif
	 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	// 230
// GC20070207
//#define EP2 2+sizeof(char*)
#define EP2 2
	 0, 0, 0, 0, 0, 0, 0,EP2,0, 0,	// 240
#define EP3 3+sizeof(char*)
	 0, 0, 0, EP3, 1, 1		// 250
};

static const unsigned char EventDimEx2[][MAX_NUM_EVENT_EX2] = {
  /* Nuovo Lara/Tebe */
  { 4,31,15,15, 3,57, 8, 4, 4, 6,
    6, 3, 3, 3, 3, 4, 3, 2, 1, 0,
    0, 1, 1, 1, 1, 1, 1, 1, 0, 0,
    0, 2, 2, 2, 3, 2, 3, 3, 3, 2,
    2, 3, 2, 2, 1, 1, 2, 2, 3, 2,
    1, 1, 4, 2, 1, 1, 0, 0, 0, 0,
    2, 5, 3, 3, 3, 3, 3, 1, 1},
  /* Delphi */
  { 1, 1, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    1, 0, 0, 0, 1, 1, 3, 2, 0, 2,
    2,11, 1},
};

static char Secret[MAX_NUM_CONSUMER][DIM_SECRET];
static char SecretSet[MAX_NUM_CONSUMER] = {0, };

static int codec_get_bytes(unsigned char *ev, int *consumer, int num);

void codec_init()
{
  int fd, idx, i;
  Event ev;
  union {
    void *p;
    unsigned char b[sizeof(char*)];
  } tmp;
  
  timer_accept_actuators = timeout_init();
  
  if(Restart) unlink(ADDROOTDIR("/tmp/delphi.events"));
  fd = open(ADDROOTDIR("/tmp/delphi.events"), O_RDWR|O_CREAT/*|O_EXCL|O_TRUNC*/, 0600);
  if(fd >= 0)
  {
    ftruncate(fd, sizeof(struct _events));
    Events = mmap(NULL, sizeof(struct _events), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
    close(fd);
  }
  if(!Events) Events = calloc(1, sizeof(struct _events));
  if(Events)
  {
    /* Se dovesse cambiare il numero di consumatori invalido
       il file eventualmente caricato. Se non ho caricato un
       file Init è già zero. */
    if(Events->nConsumers != MAX_NUM_CONSUMER)
      Events->Init = 0;
    
    if(!Events->Init)
    {
      Events->Init = 1;
      Events->nConsumers = MAX_NUM_CONSUMER;
    }
    
    /* Tutti gli eventi stringa hanno perso il puntamento, devo sostituirlo
       con una nuova stringa opportuna. */
    pthread_mutex_lock(&EventMutex);
    idx = Events->EventOutIdx[MAX_NUM_CONSUMER];
    while(idx != Events->EventInIdx)
    {
      codec_get_bytes(ev.Event, &idx, 9);
      if(ev.Event[8] == Evento_Esteso)
      {
        /* L'evento esteso prevede ancora 3 byte + 4 di puntatore.
           Anziché estrarli tutti estraggo solo i primi 2, azzero
           la lunghezza e sostituisco il puntatore. */
        codec_get_bytes(ev.Event + 9, &idx, 2);
        Events->EventBuf[idx++] = 6;  // forzo la lunghezza (ev.Event[11])
        if(idx == DIM_EVENT_BUFFER) idx = 0;
        tmp.p = (void*)strdup("(****)"); // indica che la stringa originale è andata persa
        for(i=0; i<sizeof(char*); i++)
        {
          Events->EventBuf[idx++] = tmp.b[i];
          if(idx == DIM_EVENT_BUFFER) idx = 0;
        }
      }
      else
      {
        codec_get_bytes(ev.Event + 9, &idx, EventDim[ev.Event[8]]);
        if(ev.Event[8] == Evento_Esteso2)
           codec_get_bytes(ev.Event + 11, &idx, EventDimEx2[ev.Event[9]][ev.Event[10]]);
      }
    }
    pthread_mutex_unlock(&EventMutex);
    
  }
  else
  {
    support_log("Errore allocazione memoria eventi");
    exit(0);
  }
}

int codec_send_null(ProtDevice *dev)
{
/*
  Event ev;

  memcpy(&ev, &NullEvent, sizeof(Event));
  ev.NodeID = config.NodeID;
  return prot_send(&ev, dev);
*/
  NullEvent.NodeID = config.NodeID;
  return prot_send(&NullEvent, dev);
}

int codec_send_event(Event *ev, ProtDevice *dev)
{
  return prot_send(ev, dev);
}

static int codec_get_bytes(unsigned char *ev, int *consumer, int num)
{
  if((*consumer <= (Events->EventInIdx - num)) ||
    ((*consumer > Events->EventInIdx) && (*consumer <= (DIM_EVENT_BUFFER - num))))
  {
    memcpy(ev, Events->EventBuf + *consumer, num);
    *consumer += num;
    return 0;
  }

  if((*consumer > Events->EventInIdx) &&
    (Events->EventInIdx >= (num - (DIM_EVENT_BUFFER - *consumer))))
  {
    memcpy(ev, Events->EventBuf + *consumer, (DIM_EVENT_BUFFER - *consumer));
    memcpy(ev + (DIM_EVENT_BUFFER - *consumer), Events->EventBuf, (num - (DIM_EVENT_BUFFER - *consumer)));
    *consumer += num - DIM_EVENT_BUFFER;
    return 0;
  }

  return -1;
}

/* returns:
-1	no event queued => event null
0	event not printable
1	event printable
*/
int codec_get_event(Event *ev, ProtDevice *dev)
{
  int res, i;
  
  memcpy(ev, &NullEvent, sizeof(Event));
  ev->NodeID = config.NodeID;
  
  if(!libuser_started) return -1;

  /* Il polling sul consumatore ridondato elegge la centrale master */
  if(codec_syncdev && config.consumer[codec_syncdev->consumer].param &&
     (dev == config.consumer[config.consumer[codec_syncdev->consumer].param[0] - '0'].dev))
    master_set_active();
  
  /* Dopo il primo switch, se ricevo il polling sulla standby non devo rispondere.
     Il polling deve essere fatto sulla Delphi attiva. */
  if(master_behaviour == MASTER_STANDBY) return -1;
  
  pthread_mutex_lock(&EventMutex);
  if(Events->EventOutIdx[dev->consumer] != Events->EventInIdx)
  {
    codec_get_bytes(ev->Event, &Events->EventOutIdx[dev->consumer], 9);
    codec_get_bytes(ev->Event + 9, &Events->EventOutIdx[dev->consumer], EventDim[ev->Event[8]]);
    if(ev->Event[8] == Evento_Esteso)
    {
      char *ptr;
      
      /* La verifica deve essere fatta subito poichè se l'evento è Ex_Stringa
         il valore di ev->Event[10] viene alterato. */
      res = PrintEventEx[dev->consumer][ev->Event[10]];
      if(ev->Event[9] == Ex_Stringa)
      {
        ev->Len = 2 + 9 + 2 + ev->Event[11];
        ev->Event[10] = ev->Event[11];
        ptr = (char*)((ev->Event[12]) | ((ev->Event[13])<<8) | ((ev->Event[14])<<16) | ((ev->Event[15])<<24));
        memcpy((char*)(&(ev->Event[11])), ptr, ev->Event[10]);
      }
      else
      {
        ev->Len = 2 + 9 + 3 + ev->Event[11];
        ptr = *((char**)(&(ev->Event[12])));
        memcpy((char*)(&(ev->Event[12])), ptr, ev->Event[11]);
      }
    }
    else if(ev->Event[8] == Evento_Esteso2)
    {
// GC20070207
//      char *ptr;
      
      ev->Len = 2 + 9 + 2 + EventDimEx2[ev->Event[9]][ev->Event[10]];
//      ptr = *((char**)(&(ev->Event[11])));
//      memcpy((char*)(&(ev->Event[11])), ptr, EventDimEx2[ev->Event[9]][ev->Event[10]]);
      codec_get_bytes(ev->Event + 11, &Events->EventOutIdx[dev->consumer], EventDimEx2[ev->Event[9]][ev->Event[10]]);
      /* In realtà la centrale genera solo eventi di tipo 0 (Lara/Tebe) e 1 (Delphi) */
      if(ev->Event[9] < 2)
        res = PrintEventEx2[ev->Event[9]][dev->consumer][ev->Event[10]];
      else
        res = 1;
    }
    else
    {
      ev->Len = 2 + 9 + EventDim[ev->Event[8]];
      if(ev->Event[8] >= EV_OFFSET)
        res = PrintEvent[dev->consumer][ev->Event[8] - EV_OFFSET];
      else
        res = 1; /* Lara events always printed */
    }
    
    pthread_mutex_unlock(&EventMutex);
    
    ev->DeviceID[0] = config.DeviceID >> 8;
    ev->DeviceID[1] = config.DeviceID & 0xff;
    
    if(!(LS_ABILITATA[dev->consumer] & 0x01)) res = 0;
    
    if(res)
    {
      ev->Event[0] = config.consumer[dev->consumer].progr >> 8;
      ev->Event[1] = config.consumer[dev->consumer].progr & 0xff;
    
      config.consumer[dev->consumer].progr++;
      if(config.consumer[dev->consumer].progr > 9999) config.consumer[dev->consumer].progr = 0;
      
      if((config.consumer[dev->consumer].type == PROT_SAETNET_OLD) ||
         (config.consumer[dev->consumer].type == PROT_GSM) ||
         (config.consumer[dev->consumer].type == PROT_MODEM))
      {
        switch(ev->Event[8])
        {
          case Guasto_Periferica:
          case Fine_Guasto_Periferica:
          case Manomissione_Contenitore:
          case Fine_Manomis_Contenitore:
          case Periferica_Manomessa:
          case Periferica_Ripristino:
          case Chiave_Falsa:
          case Sentinella_on:
          case Sentinella_off:
          case Sentinella_off_timeout:
          case Mancata_punzonatura:
            ev->Event[9] = ev->Event[10];
            ev->Len--;
            break;
          case Transitato_Ident:
          case Entrato_Ident:
          case Uscito_Ident:
//            ev->Event[9] = ev->Event[10];
            ev->Len--;
            break;
          case Periferica_Incongruente:
            ev->Event[9] = ev->Event[10];
            ev->Event[10] = ev->Event[11];
            ev->Len--;
            break;
          case Periferiche_presenti:
          case Periferiche_previste:
          case Parametri_taratura:
          case Valori_analogici:
            for(i=9; i<18; i++)
              ev->Event[i] = ev->Event[i+1];
            ev->Len--;
            break;
          case StatoBadge:
            for(i=9; i<26; i++)
              ev->Event[i] = ev->Event[i+1];
            ev->Len--;
            break;
          default:
            break;
        }
      }
    }
    
    if(codec_sync && config.consumer[codec_syncdev->consumer].param &&
      ((config.consumer[codec_syncdev->consumer].param[0] - '0') == dev->consumer) &&
      (ev->Event[8] != Ricezione_codice_errato) &&
      (ev->Event[8] != No_ModuloMasterTx) &&
      (ev->Event[8] != ErrRx_ModuloMaster) &&
      /* Gli eventi stringa per gsm, se hanno indice da 100 in poi sono
         eventi generati in risposta all'interazione con il modem, quindi la
         slave non è in grado di produrli e non devono essere consumati. */
      ((ev->Event[8] != Evento_Esteso) || (ev->Event[9] != Ex_Stringa_GSM) || (ev->Event[10] < 100)))
        codec_sync(codec_syncdev, 0);
#ifdef DEBUG_SAETNET
#warning !!!!!!!!!!!!
if(dev->consumer == 0)
{
char buf[64];
sprintf(buf, "EV%d ", res);
for(i=0; (i<ev->Len-2)&&(i<24); i++) sprintf(buf+4+i*2, "%02x", ev->Event[i]);
if(i<ev->Len-2) sprintf(buf+4+i*2, "...");
//printf("%s\n", buf);
support_log(buf);
}
#endif
    return res;
  }
  
  pthread_mutex_unlock(&EventMutex);
  return -1;
}

static int codec_drop_event(int consumer)
{
  unsigned char ev[24];
  
  if(Events->EventOutIdx[consumer] > (DIM_EVENT_BUFFER - 9))
  {
    if((Events->EventBuf[Events->EventOutIdx[consumer] + 8 - DIM_EVENT_BUFFER] == Evento_Esteso) && (consumer == MAX_NUM_CONSUMER))
    {
      codec_get_bytes(ev, &Events->EventOutIdx[consumer], EventDim[Events->EventBuf[Events->EventOutIdx[consumer] + 8 - DIM_EVENT_BUFFER]] + 9);
      free(*((char**)(&(ev[12]))));
    }
// GC20070207
//    else if((Events->EventBuf[Events->EventOutIdx[consumer] + 8 - DIM_EVENT_BUFFER] == Evento_Esteso2) && (consumer == MAX_NUM_CONSUMER))
    else if(Events->EventBuf[Events->EventOutIdx[consumer] + 8 - DIM_EVENT_BUFFER] == Evento_Esteso2)
    {
      codec_get_bytes(ev, &Events->EventOutIdx[consumer], EventDim[Events->EventBuf[Events->EventOutIdx[consumer] + 8 - DIM_EVENT_BUFFER]] + 9);
//      free(*((char**)(&(ev[11]))));
      Events->EventOutIdx[consumer] += EventDimEx2[ev[9]][ev[10]];
    }
    else
      Events->EventOutIdx[consumer] += EventDim[Events->EventBuf[Events->EventOutIdx[consumer] + 8 - DIM_EVENT_BUFFER]] + 9;
  }
  else
  {
    if((Events->EventBuf[Events->EventOutIdx[consumer] + 8] == Evento_Esteso) && (consumer == MAX_NUM_CONSUMER))
    {
      codec_get_bytes(ev, &Events->EventOutIdx[consumer], EventDim[Events->EventBuf[Events->EventOutIdx[consumer] + 8]] + 9);
      free(*((char**)(&(ev[12]))));
    }
// GC20070207
//    else if((Events->EventBuf[Events->EventOutIdx[consumer] + 8] == Evento_Esteso2) && (consumer == MAX_NUM_CONSUMER))
    else if(Events->EventBuf[Events->EventOutIdx[consumer] + 8] == Evento_Esteso2)
    {
      codec_get_bytes(ev, &Events->EventOutIdx[consumer], EventDim[Events->EventBuf[Events->EventOutIdx[consumer] + 8]] + 9);
//      free(*((char**)(&(ev[11]))));
      Events->EventOutIdx[consumer] += EventDimEx2[ev[9]][ev[10]];
    }
    else
      Events->EventOutIdx[consumer] += EventDim[Events->EventBuf[Events->EventOutIdx[consumer] + 8]] + 9;
  }
    
  if(Events->EventOutIdx[consumer] >= DIM_EVENT_BUFFER)
    Events->EventOutIdx[consumer] -= DIM_EVENT_BUFFER;
  
  return 0;
}

static int codec_put_bytes(unsigned char *ev, int num)
{
  int i, dropped;
  
  for(i=0; i<=MAX_NUM_CONSUMER; i++)
  {    
    do
    {
      dropped = 0;
      if((Events->EventInIdx < Events->EventOutIdx[i]) &&
          ((Events->EventInIdx + num) >= Events->EventOutIdx[i]))
      {
        codec_drop_event(i);
        dropped = 1;
        config.consumer[i].progr = 0;
      }
      else if((Events->EventInIdx > Events->EventOutIdx[i]) &&
               ((Events->EventInIdx + num) >= (Events->EventOutIdx[i] + DIM_EVENT_BUFFER)))
      {
        codec_drop_event(i);
        dropped = 1;
        config.consumer[i].progr = 0;
      }
    }
    while(dropped);
  }
  
  if(Events->EventInIdx < (DIM_EVENT_BUFFER - num))
  {
    memcpy(Events->EventBuf + Events->EventInIdx, ev, num);
    Events->EventInIdx += num;
  }
  else
  {
    memcpy(Events->EventBuf + Events->EventInIdx, ev, DIM_EVENT_BUFFER - Events->EventInIdx);
    memcpy(Events->EventBuf, ev + DIM_EVENT_BUFFER - Events->EventInIdx, num - (DIM_EVENT_BUFFER - Events->EventInIdx));
    Events->EventInIdx += num - DIM_EVENT_BUFFER;
  }
  
  return 0;
}

int codec_queue_event_time(unsigned char *ev, int time)
{
  unsigned char hdr[8];
  struct tm dt;
#ifdef DEBUG_SAETNET
char buf[64];
int i;
#endif
  
  hdr[0] = 0;
  hdr[1] = 0;
  
  if(time == -1)
  {
    hdr[2] = GIORNO;
    hdr[3] = MESE;
    hdr[4] = ANNO;
    hdr[5] = ORE;
    hdr[6] = MINUTI | 0xC0;
    hdr[7] = SECONDI;
  }
  else
  {
    memcpy(&dt, localtime((time_t*)&time), sizeof(struct tm));
    hdr[2] = dt.tm_mday;
    hdr[3] = dt.tm_mon+1;
    hdr[4] = dt.tm_year % 100;
    hdr[5] = dt.tm_hour;
    hdr[6] = dt.tm_min | 0xC0;
    hdr[7] = dt.tm_sec;
  }
  
  codec_put_bytes(hdr, 8);
#ifdef DEBUG_SAETNET
#warning !!!!!!!!!!!!
sprintf(buf, "PUT ");
for(i=0; i<8; i++) sprintf(buf+4+i*2, "%02x", hdr[i]);
#endif
  if(ev[0] == Evento_Esteso)
  {
    char *ptr;
    ptr = malloc(ev[3]);
    if(!ptr) return 0;
    memcpy(ptr, ev+4, ev[3]);
    codec_put_bytes(ev, 4);
    codec_put_bytes((unsigned char*)&ptr, sizeof(char*));
#ifdef DEBUG_SAETNET
#warning !!!!!!!!!!!!
for(i=0; (i<4+ev[3])&&(i<16); i++) sprintf(buf+20+i*2, "%02x", ev[i]);
if(i<4+ev[3]) sprintf(buf+20+i*2, "...");
#endif
  }
  else if(ev[0] == Evento_Esteso2)
  {
// GC20070207
//    char *ptr;
//    ptr = malloc(EventDimEx2[ev[1]][ev[2]]);
//    if(!ptr) return 0;
//    memcpy(ptr, ev+3, EventDimEx2[ev[1]][ev[2]]);
//    codec_put_bytes(ev, 3);
//    codec_put_bytes((unsigned char*)&ptr, sizeof(char*));
    codec_put_bytes(ev, 3+EventDimEx2[ev[1]][ev[2]]);
#ifdef DEBUG_SAETNET
#warning !!!!!!!!!!!!
for(i=0; i<3+EventDimEx2[ev[1]][ev[2]]; i++) sprintf(buf+20+i*2, "%02x", ev[i]);
#endif
  }
  else
  {
    codec_put_bytes(ev, 1 + EventDim[ev[0]]);
#ifdef DEBUG_SAETNET
#warning !!!!!!!!!!!!
for(i=0; i<1+EventDim[ev[0]]; i++) sprintf(buf+20+i*2, "%02x", ev[i]);
#endif
  }
#ifdef DEBUG_SAETNET
support_log(buf);
#endif
  
  return 1;
}

int codec_queue_event(unsigned char *ev)
{
  int ret;
  
  pthread_mutex_lock(&EventMutex);
  ret = codec_queue_event_time(ev, -1);
  pthread_mutex_unlock(&EventMutex);
  return ret;
}

int codec_try_queue_event(unsigned char *ev, int time)
{
  int i, len, avail;
  
  if(ev[0] == Evento_Esteso)
    len = 12 + sizeof(char*);
  else if(ev[0] == Evento_Esteso2)
// GC20070207
//    len = 11 + sizeof(char*);
    len = 11 + EventDimEx2[ev[1]][ev[2]];
  else
    len = 9 + EventDim[ev[0]];
  
  pthread_mutex_lock(&EventMutex);
  
  /* cerco se c'e' almeno un consumatore con sufficiente spazio */
  avail = 0;
  for(i=0; (!avail) && (i<MAX_NUM_CONSUMER); i++)
  {    
    if(!((((Events->EventInIdx < Events->EventOutIdx[i]) &&
          ((Events->EventInIdx + len) >= Events->EventOutIdx[i]))) ||
          (((Events->EventInIdx > Events->EventOutIdx[i]) &&
               ((Events->EventInIdx + len) >= (Events->EventOutIdx[i] + DIM_EVENT_BUFFER))))))
      avail = 1;
  }
  
  if(!avail)
  {
    pthread_mutex_unlock(&EventMutex);
    return 0;
  }
  
  i = codec_queue_event_time(ev, time);
  pthread_mutex_unlock(&EventMutex);
  
  return i;
}

static int codec_secret(unsigned char *cmd, int consumer)
{
  int i;
  
  if(!SecretSet[consumer]) return 0;
  
  for(i=0;i<DIM_SECRET;i++) if(!isdigit(cmd[i])) break;
  if(i == DIM_SECRET)
  {
    return strncmp(Secret[consumer], cmd, DIM_SECRET);
  }
  return 0;
}

static void codec_timeout_send(FILE **fp, int null)
{
  char ev[1];
  
  fclose(*fp);
  *fp = NULL;

  if(!unlink(ADDROOTDIR(LaraFileTmp)))
    pthread_mutex_unlock(&laragz_mutex);
  
  ev[0] = Errore_messaggio_host;
  codec_queue_event(ev);
}

static void codec_accetta_sensore_CEI(int i)
{
  Event ev;
  
  if((EVENTO[3] > 0) &&
     ((SE[i] & bitGestAlarm) || (SE[i] & bitGestFailure) || (SE[i] & bitGestSabotage)))
    EVENTO[3] = -1;
  
  if(SE[i] & bitGestAlarm)
  {
    ev.Event[0] = Evento_Sensore;
    ev.Event[1] = i >> 8;
    ev.Event[2] = i & 0xff;
    ev.Event[3] = 1;
    ev.Event[4] = MU_acc;
    codec_queue_event(ev.Event);
    SE[i] &= ~(bitMUAlarm|bitGestAlarm);
    SEmu[i] &= ~bitSEmuAlarm;
    ev.Event[0] = Ripristino_Sensore;
    ev.Event[3] = Zona_SE[i];
    codec_queue_event(ev.Event);
    SE[i] |= bitHLF;
    if(SE[i] & bitAlarm)
    {
      SE[i] &= ~bitAlarm;
      master_set_alarm(i, 1, 1);
    }
  }
  
  if(SE[i] & bitGestFailure)
  {
    ev.Event[0] = Evento_Sensore;
    ev.Event[1] = i >> 8;
    ev.Event[2] = i & 0xff;
    ev.Event[3] = 3;
    ev.Event[4] = MU_acc;
    codec_queue_event(ev.Event);
    
    if(SEmu[i] & bitSEmuInFail)
    {
      SE[i] |= bitInputFailureHLF;
      SE[i] &= ~(bitMUFailure|bitGestFailure);
      SEmu[i] &= ~bitSEmuInFail;
      ev.Event[0] = Fine_Guasto_Sensore;
      codec_queue_event(ev.Event);
      if(SE[i] & bitInputFailure)
      {
        SE[i] &= ~bitInputFailure;
        master_set_failure(i, 1, Guasto_Sensore);
      }
    }
    
                          if(((i & 0x03) == 2) && (SEmu[i] & bitSEmuGenFail))
                          {
                            SE[i] |= bitGenHLF;
                            SE[i] &= ~(bitMUFailure|bitGestFailure);
                            SEmu[i] &= ~bitSEmuGenFail;
                            ev.Event[0] = Fine_Guasto_Periferica;
#ifdef GEST16MM
                            ev.Event[1] = i >> 11;
                            ev.Event[2] = i >> 3;
#else
                            ev.Event[1] = i >> 3;
#endif
                            codec_queue_event(ev.Event);
                            if(SE[i] & bitGenFailure)
                            {
                              SE[i] &= ~bitGenFailure;
                              master_set_failure(i-2, 1, Guasto_Periferica);
                            }
                          }
                        }
                        
                        if(SE[i] & bitGestSabotage)
                        {
                          ev.Event[0] = Evento_Sensore;
                          ev.Event[1] = i >> 8;
                          ev.Event[2] = i & 0xff;
                          ev.Event[3] = 2;
                          ev.Event[4] = MU_acc;
                          codec_queue_event(ev.Event);
                          
                          SE[i] &= ~bitMUSabotage;
                          if(((i & 0x03) == 0) && (SEmu[i] & bitSEmuPackSab))
                          {
                            SE[i] |= bitPackHLF;
                            SE[i] &= ~bitGestSabotage;
                            SEmu[i] &= ~bitSEmuPackSab;
                            ev.Event[0] = Fine_Manomis_Contenitore;
#ifdef GEST16MM
                            ev.Event[1] = i >> 11;
                            ev.Event[2] = i >> 3;
#else
                            ev.Event[1] = i >> 3;
#endif
                            codec_queue_event(ev.Event);
                            
                            if(SE[i] & bitPackSabotage)
                            {
                              SE[i] &= ~bitPackSabotage;
                              master_set_sabotage(i, 1, Manomissione_Contenitore);
                            }
                          }
                          else if(((i & 0x03) == 1) && (SEmu[i] & bitSEmuLineSab))
                          {
                            SE[i] |= bitLineHLF;
                            SE[i] &= ~bitGestSabotage;
                            SEmu[i] &= ~bitSEmuLineSab;
                            ev.Event[0] = Periferica_Ripristino;
#ifdef GEST16MM
                            ev.Event[1] = i >> 11;
                            ev.Event[2] = i >> 3;
#else
                            ev.Event[1] = i >> 3;
#endif
                            codec_queue_event(ev.Event);
                            
                            if(SE[i] & bitLineSabotage)
                            {
                              int tmp, j;
                              
                              ev.Event[0] = Periferica_Manomessa;
                              
                              if(!(SE[i] & bitOOS))
                              {
                                SEmu[i] |= bitSEmuLineSab;
                                SE[i] |= bitSabotage | bitLineSabotage;
                                tmp = database_testandset_MU(i, bitMUSabotage);
                              }
                              else
                                tmp = 0;
                              if(tmp)
                              {
                                codec_queue_event(ev.Event);
                                SE[i] |= bitGestSabotage;
                                ev.Event[0] = Evento_Sensore;
                                ev.Event[1] = i >> 8;
                                ev.Event[2] = i & 0xff;
                                ev.Event[3] = MU_sabotage;
                                ev.Event[4] = MU_ini;
                                codec_queue_event(ev.Event);
                              }
                              else
                              {
                                if(!(SE[i] & bitMUNoSabotage))
                                  codec_queue_event(ev.Event);
                              }
                              if(SE[i] & bitMUNoSabotage)
                              {
                                tmp = i-1;
                                for(j=0; j<master_sensor_valid(tmp); j++)
                                master_set_failure(tmp+j, 1, Guasto_Sensore);
                              }
                            }
                          }
                      
                          if(SEmu[i] & bitSEmuInSab)
                          {
                            SE[i] |= bitInputSabotageHLF;
                            SE[i] &= ~bitGestSabotage;
                            SEmu[i] &= ~bitSEmuInSab;
                            ev.Event[0] = Fine_Manomis_Dispositivo;
                            ev.Event[1] = i >> 8;
                            codec_queue_event(ev.Event);
                            if(SE[i] & bitInputSabotage)
                            {
                              SE[i] &= ~bitInputSabotage;
                              master_set_sabotage(i, 1, Manomissione_Dispositivo);
                            }
                          }
                        }
}

int codec_parse_cmd(unsigned char *cmd, int len, ProtDevice *dev)
{
  if(codec_sync)
  {
    codec_syncdev->cmd.Len = len;
    memcpy(codec_syncdev->cmd.Command, cmd, DIMBUF);
    codec_sync(codec_syncdev, 1);
  }
  
/*
  if(config.debug == dev->consumer)
  {
    int i;
    
    printf("parse: ");
    for(i=0; i<len; i++)
      printf("%02x", cmd[i]);
    printf("\n");
  }
*/
if(config.debug == dev->consumer)
{
  char log[256];
  int xx;
  memcpy(log, "CMD:", 4);
  for(xx=0; xx<len; xx++)
    sprintf(log+xx*2, "%02x", cmd[xx]);
  support_log(log);
}
  
  return codec_parse_cmd2(cmd, dev);
}

int codec_parse_cmd2(unsigned char *cmd, ProtDevice *dev)
{
  int i, ret;
  unsigned char *cmd2;
  unsigned char t;
  Event ev;
  static FILE *fp = NULL;
  static unsigned short mem_expected = 0;
  static unsigned short mem_error = 0;
  static unsigned short mem_sprint = 0;
  static unsigned short mem_file = 0;
  
  ret = 1;
  
  for(i=0;i<DIM_SECRET;i++) if(!isdigit(cmd[i])) break;
  cmd2 = &(cmd[i]);

  if (libuser_started && ((LS_ABILITATA[dev->consumer] & 0x02) || (cmd2[0] == '@')))
  {    
    switch (cmd2[0])
    {
    /*************** MODEM **************/
      case '@':
        cmd_modem_status(cmd2 + 1, config.consumer[dev->consumer].configured);
        break;
    /********** LIST MANAGEMENT *********/
      case 'a':
        list_oos_sensors();
        break;
      case 'b':
        list_oos_actuators();
        break;
      case 'c':
        list_status_zone();
        break;
      case 'd':
        list_status_sensors();
        break;
      case 'e':
        list_status_actuators();
        break;
      case 'g':
        list_calibrations();
        break;
      case 'k':
        list_holydays();
        break;
      case 'n':
        list_periph();
        break;
      case 'q':
        list_rounds_QRA();
        break;
      case 'r':
        list_rounds_list(6);
        list_rounds_timings(0);
        break;
      case 's':
        list_rounds_list(0);
        list_rounds_timings(6);
        break;
      case 't':
        cmd_memory_control();
        break;
      case 'u':
        list_daily_timings();
        break;
      case 'v':
        list_rounds_keys();
        break;
      case 'w':
        list_sensors_zone();
        break;
      case 'x':
        list_status_sync();
        break;
    
    /************** COMMANDS *************/
      case '>':
        ev.Event[0] = Risposta_SCS;
        codec_queue_event(ev.Event);
        break;
      case 'A':
        database_lock();
        ret = cmd_actuator_off(atoi(cmd2 + 1));
        database_unlock();
        break;
      case 'B':
        database_lock();
        ret = cmd_actuator_on(atoi(cmd2 + 1));
        database_unlock();
        break;
      case 'C':
        database_lock();
        ret = cmd_sensor_off(atoi(cmd2 + 1));
        database_unlock();
        break;
      case 'D':
        database_lock();
        ret = cmd_sensor_on(atoi(cmd2 + 1));
        database_unlock();
        break;
      case 'G':
        database_lock();
        ret = cmd_accept_act_timers();
        database_unlock();
        break;
      case 'H':
        if(!timer_accept_actuators->active)
        {
          database_lock();
          ret = cmd_accept_actuators(0);
          timeout_on(timer_accept_actuators, NULL, NULL, 0, 50);
          database_unlock();
        }
        break;
      case 'I':
        database_lock();
        ret = cmd_zone_on(atoi(cmd2 + 1), 0);
        database_unlock();
#ifdef DEBUG_CMD
{
  char m[64];
  sprintf(m, "CMD: Attivata zona %d da consumatore %d", atoi(cmd2 + 1), dev->consumer);
  support_log(m);
}
#endif
        break;
      case 'J':
        database_lock();
        ret = cmd_zone_off(atoi(cmd2 + 1), 0);
        database_unlock();
#ifdef DEBUG_CMD
{
  char m[64];
  sprintf(m, "CMD: Disattivata zona %d da consumatore %d", atoi(cmd2 + 1), dev->consumer);
  support_log(m);
}
#endif
        break;
      case 'K':
        database_lock();
        database_set_alarm2(&ME[atoi(cmd2 + 1)]);
        database_unlock();
        break;
      case 'L':
        SProva(1);
        break;
      case 'M':
        if(PROVA & bitAlarm) cmd_command_send_noise(atoi(cmd2 + 1) / 8);
        break;
      case 'N':
        if(PROVA & bitAlarm) cmd_command_send_noise_all();
        break;
      case 'O':
        SProva(0);
        break;
      case 'P':
        cmd2[5] &= 0x3;
        i = atoi(cmd2 + 1) / 8;
        DEFPT[i][1] = cmd2[5];
        DEFPT[i][2] = 1;
        DEFPT[i][3] = 0;
        DEFPT[i][4] = 0;
        master_set_setting(i);
        database_changed = 1;
        ev.Event[0] = Parametri_taratura;
#ifndef GEST16MM
        ev.Event[1] = i;
        ev.Event[2] = DEFPT[i][0];
        ev.Event[3] = DEFPT[i][1];      
#else
        ev.Event[1] = i >> 8;
        ev.Event[2] = i;
        ev.Event[3] = DEFPT[i][0];
        ev.Event[4] = DEFPT[i][1];      
#endif
        codec_queue_event(ev.Event);
        break;
      case 'Q':
        cmd2[5] &= 0x7;
        i = atoi(cmd2 + 1) / 8;
        DEFPT[i][0] = cmd2[5];
        DEFPT[i][2] = 1;
        DEFPT[i][3] = 0;
        DEFPT[i][4] = 0;
        master_set_setting(i);
        database_changed = 1;
        ev.Event[0] = Parametri_taratura;
#ifndef GEST16MM
        ev.Event[1] = i;
        ev.Event[2] = DEFPT[i][0];
        ev.Event[3] = DEFPT[i][1];      
#else
        ev.Event[1] = i >> 8;
        ev.Event[2] = i;
        ev.Event[3] = DEFPT[i][0];
        ev.Event[4] = DEFPT[i][1];      
#endif
        codec_queue_event(ev.Event);
        break;
      case 'R':
        cmd_set_day_type(GVAL2(cmd2+2), cmd2[1] - '0');
        break;
      case 'T':
        database_lock();
        i = GVAL2(cmd2+1);
        periodo[i].tipo = GVAL(cmd2+3);
        periodo[i].mesei = GVAL2(cmd2+4);
        periodo[i].giornoi = GVAL2(cmd2+6);
        periodo[i].mesef = GVAL2(cmd2+8);
        periodo[i].giornof = GVAL2(cmd2+10);
        database_unlock();
        database_changed = 1;
        delphi_check_periods();
        break;
      case 'U':
        cmd_set_date(cmd2 + 1);
        for(i=0; i<MM_NUM; i++)
          if(MMPresente[i] == MM_LARA)
          {
            lara_time(i);
            i+=2;
          }
        break;
      case 'V':
        cmd_get_time();
        break;
      case 'W':
        support_log("Variata ora da supervisione");
        cmd_set_time(cmd2 + 1);
        for(i=0; i<MM_NUM; i++)
          if(MMPresente[i] == MM_LARA)
          {
            lara_time(i);
            i+=2;
          }
        break;
      case 'X':
        database_lock();
        memcpy(Secret[dev->consumer], cmd2 + 1, 6);
        SecretSet[dev->consumer] = 1;
        ev.Event[0] = Variato_segreto;
        codec_queue_event(ev.Event);
        database_unlock();
        break;
      case 'Y':
        database_lock();
        ret = cmd_zone_on(atoi(cmd2 + 2), cmd2[1] - '0');
        database_unlock();
#ifdef DEBUG_CMD
{
  char m[64];
  sprintf(m, "CMD: Attivata zona %d (%d) da consumatore %d", atoi(cmd2 + 2), cmd2[1]-'0', dev->consumer);
  support_log(m);
}
#endif
        break;
      case 'Z':
        database_lock();
//        memcpy(Zona_SE + *(unsigned short*)(cmd2+1), cmd2 + 3, 8);
        memcpy(Zona_SE + uchartoshort(cmd2+1), cmd2 + 3, 8);
        /* Ricalcola l'associazione zona/sensore */
        database_calc_zones();
        database_unlock();
        database_changed = 1;
        break;
      case '/':
        switch (cmd2[1])
        {
          case '3':
            switch(cmd2[2])
            {
              case '5':
                master_sensors_refresh();
                break;
              case '6':
                master_actuators_refresh();
                break;            
              case '7':
                master_settings_refresh();
                break;            
              case '8':
                switch(cmd2[3])
                {
                  case '0':
                    list_memory_MU();
                    list_end();
                    break;
                  case '1':
                    database_lock();
                    ret = cmd_status_accept(*((short *)(cmd2+4)), bitMUAlarm, MU_alarm);
                    database_unlock();
                    break;
                  case '2':
                    database_lock();
                    ret = cmd_status_accept(*((short *)(cmd2+4)), bitMUSabotage, MU_sabotage);
                    database_unlock();
                    break;
                  case '3':
                    database_lock();
                    ret = cmd_status_accept(*((short *)(cmd2+4)), bitMUFailure, MU_failure);
                    database_unlock();
                    break;
                  case '9':
                    database_lock();
                    for(i=0; i<n_SE; i++) SE[i] &= ~(0x0fc0c000);
                    database_unlock();
                    break;
                }
                break;            
            }
            break;
          case '7':
            cmd_set_timerange(GVAL2(cmd2 + 2), cmd2 + 4);
            break;
          case 'a':
            switch(cmd2[2])
            {
              case 'R':
                cmd_request_analog(atoi(cmd2+3));
                break;
              case 'W':
                t = cmd2[6];
                cmd2[6] = 0;
                cmd_send_threshold(atoi(cmd2+3), t, cmd2+7);
                break;
            }
            break;
          case 'B':
            switch(cmd2[2])
            {
              case '1':
                badge_request(cmd2[3]);
                break;
              case '2':
                t = cmd2[4];
                cmd2[4] = cmd2[5];
                cmd2[5] = t;
//                badge_manage(cmd2[3], *(unsigned short*)&cmd2[4], cmd2[6]);
                badge_manage(cmd2[3], uchartoshort(cmd2+4), cmd2[6]);
                break;
              case '3':
                t = cmd2[4];
                cmd2[4] = cmd2[5];
                cmd2[5] = t;
//                badge_acquire(cmd2[3], *(unsigned short*)&cmd2[4]);
                badge_acquire(cmd2[3], uchartoshort(cmd2+4));
                break;
              case '4':
                badge_load(&cmd2[3]);
                break;
            }
            break;
          case 'A':
            switch(cmd2[2])
            {
              case 'G':
                switch(cmd2[3])
                {
                  case 'S':
                    GSM_Status_event(cmd2[4] - '0');
                    break;
                }
                break;
              case 'M':
                switch(cmd2[3])
                {
                  case 'S':
                    TMD_Status_event(cmd2[4] - '0');
                    break;
                }
                break;
            }
            break;
          case 'L':
            lara_conf_cmd(cmd2[2]-'a', cmd2+3);
            break;
          case 'M':
            if(cmd2[2] == 'R')
            {
              support_log("Invio Restart");
              /* To avoid incoherency */
              if((mem_file) && (!mem_sprint)) unlink(ADDROOTDIR(ref_SprintFileName));
#if defined __CRIS__ || defined __arm__
              delphi_restart();
#endif
              break;
            }
//            else if(*(unsigned short*)(cmd2+3) == 0)
            else if(uchartoshort(cmd2+3) == 0)
            {
              /* reset the load */
              if(fp) fclose(fp);
              fp = NULL;
              mem_expected = 0;
              mem_error = 0;
              switch(cmd2[2])
              {
                case 'U':
                  mem_file = 1;
                  fp = fopen(ADDROOTDIR(NewUserFileName), "w");
                  support_log("Invio libuser.so");
                  break;
                case 'C':
                  mem_file = 1;
                  fp = fopen(ADDROOTDIR(ref_ConfFileName), "w");
                  support_log("Invio saet.conf");
                  break;
                case 'N':
                  mem_file = 1;
                  fp = fopen(ADDROOTDIR(ref_NVFileName), "w");
                  support_log("Invio saet.nv");
                  break;
                case 'P':
                  mem_file = 1;
                  fp = fopen(ADDROOTDIR(ref_ConsumerFileName), "w");
                  support_log("Invio consumer.conf");
                  break;
                case 'S':
                  mem_file = 1;
                  fp = fopen(ADDROOTDIR(ref_StringsFileName), "w");
                  support_log("Invio strings.conf");
                  break;
                case 'X':
                  mem_sprint = 1;
                  fp = fopen(ADDROOTDIR(ref_SprintFileName), "w");
                  support_log("Invio sprint.xml");
                  break;
                case 'L':
                  /* Blocco la gestione del campo Tebe per il tempo
                     necessario a ricevere la nuova anagrafica. */
                  pthread_mutex_lock(&laragz_mutex);
                  fp = fopen(ADDROOTDIR(LaraFileTmp), "w");
                  support_log("Invio lara.gz");
                  break;
              }
            }
//            if(*(unsigned short*)(cmd2+3) != mem_expected)
            if(uchartoshort(cmd2+3) != mem_expected)
            {
              char m[64];
//              sprintf(m, "Errore sequenza (arrivato %d, atteso %d)", *(unsigned short*)(cmd2+3), mem_expected);
              sprintf(m, "Errore sequenza (arrivato %d, atteso %d)", uchartoshort(cmd2+3), mem_expected);
              support_log(m);
              mem_error = 1;
            }
            else
//              if(fp) fwrite(cmd2+7, *(unsigned short*)(cmd2+5), 1, fp);
              if(fp) fwrite(cmd2+7, uchartoshort(cmd2+5), 1, fp);
            mem_expected++;
            timeout_on(dev->send_timeout, (timeout_func)codec_timeout_send, &fp, 0, 50);
//            if(*(unsigned short*)(cmd2+5) == 0)	// last packet
            if(uchartoshort(cmd2+5) == 0)	// last packet
            {
              timeout_off(dev->send_timeout);
              if(fp) fclose(fp);
              fp = NULL;
              mem_expected = 0;
              if(mem_error)
              {
                ev.Event[0] = Errore_messaggio_host;
                codec_queue_event(ev.Event);
                mem_error = 0;
              }
              else if(cmd2[2] == 'L')
              {
                /* Carico i dati del lara.gz arrivato, non c'è bisogno
                   di riavviare la centrale. */
                for(i=0; i<MM_NUM; i++)
                  if(MMPresente[i] == MM_LARA)
                  {
                    lara_reload(i);
                    break;
                  }
                /* Ripristino l'attività del campo Tebe */
                pthread_mutex_unlock(&laragz_mutex);
                lara_save(0);
                unlink(ADDROOTDIR(LaraFileTmp));
              }
              /* Serve solo per l'Athena, ma su Iside male non fa.
                 I file non vengono scritti subito su flash se non si
                 fa il sync, e se si spegne subito la centrale gli
                 aggiornamenti vanno persi. */
              sync();
            }
            break;
          case '5':
            switch(cmd2[2])
            {
#if 1
/* Utilizzati per l'invio dei codici da com90 (non usa lo /52). */
              case '0':
                memcpy(RONDA[GVAL(cmd2 + 3)], cmd2 + 4, 16);
                database_changed = 1;
                break;
              case '1':
                memcpy(RONDA[12] + ((GVAL(cmd2 + 3)) * 24), cmd2 + 4, 24);
                database_changed = 1;
                break;
#endif
              case '2':
                if(GVAL(cmd2 + 3) < 10)
                  SEGRETO[GVAL(cmd2 + 3)] = *((unsigned short *)(cmd2 + 4));
                else
                  ((unsigned short *)RONDA)[GVAL(cmd2 + 3) - 10] = *((unsigned short *)(cmd2 + 4));
                database_changed = 1;
                break;
              case '3':
                database_lock();
//                EVENTO[GVAL(cmd2 + 3)] = *((short*)(cmd2 + 4));
                EVENTO[GVAL(cmd2 + 3)] = uchartoshort(cmd2 + 4);
                switch(GVAL(cmd2 + 3))
                {
                  case 0:
                    if((EVENTO[0] >= 0) && (EVENTO[0] < n_SE) && (SE[EVENTO[0]] & bitGestAlarm))
                    {
                      if((SE[EVENTO[0]] & ((bitMUCounter << 1) | bitMUCounter)) == ((bitMUCounter << 1) | bitMUCounter))
                        SE[EVENTO[0]] &= ~((bitMUCounter << 1) | bitMUCounter);
                      SE[EVENTO[0]] += bitMUCounter;
                      ev.Event[0] = Evento_Sensore;
                      ev.Event[1] = EVENTO[0] >> 8;
                      ev.Event[2] = EVENTO[0] & 0xff;
                      ev.Event[3] = 1;
                      if((SE[EVENTO[0]] & ((bitMUCounter << 1) | bitMUCounter)) == ((bitMUCounter << 1) | bitMUCounter))
                        ev.Event[4] = MU_3ac;
                      else
                        ev.Event[4] = MU_acc;
                      codec_queue_event(ev.Event);
                      SE[EVENTO[0]] &= ~bitMUAlarm;
                      SEmu[EVENTO[0]] &= ~bitSEmuAlarm;
                      if(!(SE[EVENTO[0]] & bitAlarm) || !(ZONA[Zona_SE[EVENTO[0]]] & bitActive))
                      {
                        SE[EVENTO[0]] |= bitHLF;
                        SE[EVENTO[0]] &= ~bitGestAlarm;
                        ev.Event[0] = Ripristino_Sensore;
                        ev.Event[3] = Zona_SE[EVENTO[0]];
                        codec_queue_event(ev.Event);
                      }
                      EVENTO[0] = 0xffff;
                    }
                    break;
                  case 1:
                    if((EVENTO[1] >= 0) && (EVENTO[1] < n_SE) && (SE[EVENTO[1]] & bitGestSabotage))
                    {
                      if((SE[EVENTO[1]] & ((bitMUCounter << 1) | bitMUCounter)) == ((bitMUCounter << 1) | bitMUCounter))
                        SE[EVENTO[1]] &= ~((bitMUCounter << 1) | bitMUCounter);
                      SE[EVENTO[1]] += bitMUCounter;
                      ev.Event[0] = Evento_Sensore;
                      ev.Event[1] = EVENTO[1] >> 8;
                      ev.Event[2] = EVENTO[1] & 0xff;
                      ev.Event[3] = 2;
                      if((SE[EVENTO[1]] & ((bitMUCounter << 1) | bitMUCounter)) == ((bitMUCounter << 1) | bitMUCounter))
                        ev.Event[4] = MU_3ac;
                      else
                        ev.Event[4] = MU_acc;
                      codec_queue_event(ev.Event);
                      SE[EVENTO[1]] &= ~bitMUSabotage;
                      if(((EVENTO[1] & 0x03) == 0) &&
                         !(SE[EVENTO[1]] & bitPackSabotage) && (SEmu[EVENTO[1]] & bitSEmuPackSab))
                      {
                        SE[EVENTO[1]] |= bitPackHLF;
                        SE[EVENTO[1]] &= ~bitGestSabotage;
                        SEmu[EVENTO[1]] &= ~bitSEmuPackSab;
                        ev.Event[0] = Fine_Manomis_Contenitore;
#ifdef GEST16MM
                        ev.Event[1] = EVENTO[1] >> 11;
                        ev.Event[2] = EVENTO[1] >> 3;
#else
                        ev.Event[1] = EVENTO[1] >> 3;
#endif
                        codec_queue_event(ev.Event);
                      }
                      else if(((EVENTO[1] & 0x03) == 1) &&
                              !(SE[EVENTO[1]] & bitLineSabotage) && (SEmu[EVENTO[1]] & bitSEmuLineSab))
                      {
                        SE[EVENTO[1]] |= bitLineHLF;
                        SE[EVENTO[1]] &= ~bitGestSabotage;
                        SEmu[EVENTO[1]] &= ~bitSEmuLineSab;
                        ev.Event[0] = Periferica_Ripristino;
#ifdef GEST16MM
                        ev.Event[1] = EVENTO[1] >> 11;
                        ev.Event[2] = EVENTO[1] >> 3;
#else
                        ev.Event[1] = EVENTO[1] >> 3;
#endif
                        codec_queue_event(ev.Event);
                      }
                      if(!(SE[EVENTO[1]] & bitInputSabotage) && (SEmu[EVENTO[1]] & bitSEmuInSab))
                      {
                        SE[EVENTO[1]] |= bitInputSabotageHLF;
                        SE[EVENTO[1]] &= ~bitGestSabotage;
                        SEmu[EVENTO[1]] &= ~bitSEmuInSab;
                        ev.Event[0] = Fine_Manomis_Dispositivo;
                        ev.Event[1] = EVENTO[1] >> 8;
                        codec_queue_event(ev.Event);
                      }
                      EVENTO[1] = 0xffff;
                    }
                    break;
                  case 2:
                    if((EVENTO[2] >= 0) && (EVENTO[2] < n_SE) && (SE[EVENTO[2]] & bitGestFailure))
                    {
                      if((SE[EVENTO[2]] & ((bitMUCounter << 1) | bitMUCounter)) == ((bitMUCounter << 1) | bitMUCounter))
                        SE[EVENTO[2]] &= ~((bitMUCounter << 1) | bitMUCounter);
                      SE[EVENTO[2]] += bitMUCounter;
                      ev.Event[0] = Evento_Sensore;
                      ev.Event[1] = EVENTO[2] >> 8;
                      ev.Event[2] = EVENTO[2] & 0xff;
                      ev.Event[3] = 3;
                      if((SE[EVENTO[2]] & ((bitMUCounter << 1) | bitMUCounter)) == ((bitMUCounter << 1) | bitMUCounter))
                        ev.Event[4] = MU_3ac;
                      else
                        ev.Event[4] = MU_acc;
                      codec_queue_event(ev.Event);
                      SE[EVENTO[2]] &= ~bitMUFailure;
                      if(!(SE[EVENTO[2]] & bitInputFailure) && (SEmu[EVENTO[2]] & bitSEmuInFail))
                      {
                        SE[EVENTO[2]] |= bitInputFailureHLF;
                        SE[EVENTO[2]] &= ~bitGestFailure;
                        SEmu[EVENTO[2]] &= ~bitSEmuInFail;
                        ev.Event[0] = Fine_Guasto_Sensore;
                        codec_queue_event(ev.Event);
                      }
                      if(((EVENTO[2] & 0x03) == 2) &&
                         !(SE[EVENTO[2]] & bitGenFailure) && (SEmu[EVENTO[2]] & bitSEmuGenFail))
                      {
                        SE[EVENTO[2]] |= bitGenHLF;
                        SE[EVENTO[2]] &= ~bitGestFailure;
                        SEmu[EVENTO[2]] &= ~bitSEmuGenFail;
                        ev.Event[0] = Fine_Guasto_Periferica;
#ifdef GEST16MM
                        ev.Event[1] = EVENTO[2] >> 11;
                        ev.Event[2] = EVENTO[2] >> 3;
#else
                        ev.Event[1] = EVENTO[2] >> 3;
#endif
                        codec_queue_event(ev.Event);
                      }
                      EVENTO[2] = 0xffff;
                    }
                    break;
                  case 3:
//                    if(EVENTO[3] == -1)
                    if(EVENTO[3] == 9999)
                    {
                      for(i=0; i<n_SE; i++)
                        codec_accetta_sensore_CEI(i);
                    }
                    else if((EVENTO[3] >= 0) && (EVENTO[3] < n_SE))
                    {
                      codec_accetta_sensore_CEI(EVENTO[3]);
                    }
                    break;
                }
                database_unlock();
                break;
              case '4':
                switch(cmd2[3])
                {
                  case '0':
                  case '1':
                    cmd_set_holydays(cmd2 + 3);
                    break;
                  case '2':
                    database_lock();
                    cmd_TPDGF_reset();
                    database_unlock();
                    break;
                  case '3':
                    database_lock();
                    cmd_TPDGF_add(cmd2 + 4);
                    database_unlock();
                    break;
                  case '4':
                    database_lock();
                    cmd_TPDGV_reset();
                    database_unlock();
                    break;
                  case '5':
                    database_lock();
                    cmd_TPDGV_add(cmd2 + 4);
                    database_unlock();
                    break;
                  case '6':
                    database_lock();
                    cmd_set_saturday_type(GVAL(cmd2 + 4));
                    database_unlock();
                    break;
                }
                break;
              case '5':
                switch(cmd2[3])
                {
                  case '0':
                    if(ronda_partenza_manuale_p) ret = ronda_partenza_manuale_p(GVAL(cmd2+4));
                    break;
                  case '1':
                    if(ronda_partenza_p) ret = ronda_partenza_p(GVAL(cmd2+4));
                    break;
                  case '2':
                    if(ronda_chiudi_p) ret = ronda_chiudi_p(GVAL(cmd2+4));
                    break;
                }
                break;
              case '6':
                switch(cmd2[3])
                {
                  case '0':
                    if(Ronda_percorso_p)
                      memcpy((char*)Ronda_percorso_p + (cmd2[4]*RONDA_STAZIONI + (cmd2[5]))*19, cmd2+6, 3);
                    break;
                  case '1':
                    if(Ronda_percorso_p)
                      memcpy((char*)Ronda_percorso_p + (cmd2[4]*RONDA_STAZIONI + (cmd2[5]))*19 + 3, cmd2+6, 12);
                    break;
                  case '2':
                    if(Ronda_percorso_p)
                      memcpy((char*)Ronda_percorso_p + (cmd2[4]*RONDA_STAZIONI + (cmd2[5]))*19 + 15, cmd2+6, 4);
                    break;
                  case '3':
                    if(Ronda_orario_p)
                      memcpy((char*)Ronda_orario_p + (cmd2[4]*RONDA_ORARI + (cmd2[5]))*3, cmd2+6, 3);
                    break;
                  case '4':
                    if(Ronda_stazione_p)
                      ((unsigned short*)Ronda_stazione_p)[cmd2[4]] = cmd2[5] + cmd2[6]*256;
                    break;
                  case '5':
                    if(Ronda_zonafiltro_p)
                      memcpy(((int*)Ronda_zonafiltro_p), cmd2+5, sizeof(int));
                    break;
                }
                if(ronda_save_p) ronda_save_p();
                break;
              case '7':
                /* ATTENZIONE: Anche se questi comandi sono stati implementati in SaetNet,
                   di fatto non sono utilizzabili perché implicherebbero numerose limitazioni
                   nell'uso degli attuatori nel programma utente. In pratica il comportamento
                   non sarebbe facilmente predicibile. Quindi si può pensare che siano
                   utilizzabili solo in assenza di programma utente o simili. */
                i = atoi(cmd2+4);
                if((i > 0) && (i < n_AT))
                {
                  database_lock();
                  switch(cmd2[3])
                  {
                    case '0':
                      Donanl((unsigned int*)&AT[i]);
                      break;
                    case '1':
                      Donala((unsigned int*)&AT[i]);
                      break;
                    case '2':
                      Doffa((unsigned int*)&AT[i]);
                      break;
                  }
                  database_unlock();
                }
                break;

    /********** SPECIAL COMMANDS *********/

	      case '9':
	        switch (cmd2[3])
                {
                  case '2':
                    cmd_send_internal_info(dev);
                    break;
                  case '3':
                    if(cmd2[4] == 80 && cmd2[5] == 0) cmd_send_version(dev);
                    break;
                }
                break;
	    }
	    break;
        }
        break;
    }
    
    if((cmd2[0] >= 'a') && (cmd2[0] <= 'w')) list_end();
  }

  return ret;
}

int codec_parse_command(Command *cmd, ProtDevice *dev)
{
  int res = -1;
  //static Event ev;
  unsigned char cks, errev = Errore_messaggio_host;
  
  switch(cmd->MsgType)
  {
    case STX:
      cks = prot_calc_cks((Event*)cmd);
      if(cmd->Command[cmd->Len-2] != cks)
      {
        prot_send_NAK(dev);
        return -2;
      }
      if((delphi_modo == DELPHITIPO) && (cmd->NodeID != DELPHI_FORCE_NODEID))
      {
        prot_send_NAK(dev);
        return -2;
      }
      if((delphi_modo != DELPHITIPO) && (cmd->NodeID != 100) && (cmd->NodeID != 37))
      {
//        prot_send_NAK(dev);
        return -2;
      }
      if(IS_COMMAND(cmd) && (cmd->Len))
      {
        res = codec_send_null(dev);
        if(!codec_secret(cmd->Command, dev->consumer))
        {
          cmd->Command[cmd->Len-2] = 0;
//          if(!(cmd->Status & 0x1) || memcmp(cmd->Command, dev->lastCommand, cmd->Len - 2))
            codec_parse_cmd(cmd->Command, cmd->Len, dev);
        }
      }
      else
      {
        codec_queue_event(&errev);
        res = codec_send_null(dev);
      }
      break;
    case ENQ:
      while(!codec_get_event(&(dev->event), dev));
      res = codec_send_event(&(dev->event), dev);
      break;
    case ETB:
      res = codec_send_null(dev);
      break;
  }

  return res;
}

int codec_drop_events(int consumer, int dim)
{
  int dim2;
    
  pthread_mutex_lock(&EventMutex);

  dim2 = Events->EventInIdx - Events->EventOutIdx[consumer];
  if(dim2 < 0) dim2 += DIM_EVENT_BUFFER;
  
  while(dim2 > dim)
  {
    codec_drop_event(consumer);
    dim2 = Events->EventInIdx - Events->EventOutIdx[consumer];
    if(dim2 < 0) dim2 += DIM_EVENT_BUFFER;
  }
  
  pthread_mutex_unlock(&EventMutex);
  
  return 0;
}

int codec_do_consume_event(int consumer)
{
  /* tutto questo perche' voglio che il contatore degli eventi della
     centrale slave sia allineato con il contatore master. */
  
  Event ev;
  int res;
  
  pthread_mutex_lock(&EventMutex);
  
  if(Events->EventOutIdx[consumer] != Events->EventInIdx)
  {
    codec_get_bytes(ev.Event, &Events->EventOutIdx[consumer], 9);
    codec_get_bytes(ev.Event + 9, &Events->EventOutIdx[consumer], EventDim[ev.Event[8]]);
    if(ev.Event[8] == Evento_Esteso)
    {
      res = PrintEventEx[consumer][ev.Event[10]];
    }
    else if(ev.Event[8] == Evento_Esteso2)
    {
      res = PrintEventEx2[ev.Event[9]][consumer][ev.Event[10]];
      codec_get_bytes(ev.Event + 11, &Events->EventOutIdx[consumer], EventDimEx2[ev.Event[9]][ev.Event[10]]);
    }
    else
    {
      if(ev.Event[8] >= EV_OFFSET)
        res = PrintEvent[consumer][ev.Event[8] - EV_OFFSET];
      else
        res = 1; /* Lara events always printed */
    }
    
    pthread_mutex_unlock(&EventMutex);
    
    if(!(LS_ABILITATA[consumer] & 0x01)) res = 0;
    
    if(res)
    {
      config.consumer[consumer].progr++;
      if(config.consumer[consumer].progr > 9999) config.consumer[consumer].progr = 0;
    }
    
    return res;
  }
  
  pthread_mutex_unlock(&EventMutex);
  return -1;
}

int codec_consume_event_needed()
{
  if(!codec_syncdev) return 0;
  
  while(codec_sync_cont &&
        (codec_do_consume_event(config.consumer[codec_syncdev->consumer].param[0]-'0') >= 0))
    codec_sync_cont--;
  
  return codec_sync_cont;
}

int codec_consume_event(int consumer)
{
  /* Per evitare di non riuscire a consumare un evento che non si
     è ancora generato sulla centrale slave, gestisco un contatore
     di richieste.
     Ogni volta che si deve consumare un evento, consumo anche tutti
     quelli che sono rimasti non consumati.
     Al termine della procedura, codec_sync_count contiene il numero
     di eventi che non si è riuscito a consumare.
     Allo switch, consumo eventuali residui per essere completamente
     allineato.
  */
  codec_sync_cont++;
  while(codec_sync_cont && (codec_do_consume_event(consumer) >= 0)) codec_sync_cont--;
#if 0
{
FILE *fp;
fp=fopen("/tmp/count", "w");
fprintf(fp, "%d\n", codec_sync_cont);
fclose(fp);
}
#endif
  return codec_sync_cont;
}

void codec_sync_register(ProtDevice *dev, InitFunction sync)
{
  codec_syncdev = dev;
  codec_sync = sync;
}

