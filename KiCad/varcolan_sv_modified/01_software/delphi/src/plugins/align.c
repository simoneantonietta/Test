#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "support.h"
#include "lara.h"
#include "database.h"
#include "codec.h"
#include "master.h"

static const unsigned char Ack[4] = {0x02, 0x00, 0x00, 0x00};

void align_set_area_counters(int mm, int area1, int area2)
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

/* Non mi serve, se ci sono associazioni di tessere mi arrivano
   anche le singole forzature, quindi tutte le tessere si allineano. */
#if 0
void align_set_area_group(int id, int area, int mm, time_t timeset)
{
  int i;
  
  if(_laraex[id].group)
  {
    for(i=1; i<lara_NumBadge; i++)
    {
      /* Per tutte le tessere appartenenti allo stesso gruppo dell'id
         indicato (quindi nella ricerca salto me stesso), imposto la
         nuova area per allineare il gruppo. */
      if((_laraex[i].group == _laraex[id].group) && (i != id))
      {
        align_set_area_counters(mm, _lara->tessera[i].area, area);
        _lara->tessera[i].area = area;
        _laraf->tessera_area_last_update[i] = timeset;
        _laraf->tessera[i].ag = 1;	// invalida
      }
    }
  }
}
#endif

void align_set_area(int id, int area, time_t timeset)
{
  int mm;
  unsigned char ev[8];
  
  for(mm=0; (mm<MM_NUM)&&(MMPresente[mm]!=MM_LARA); mm++);
  if(mm >= MM_NUM) return;
  
  if(timeset >= _laraf->tessera_area_last_update[id])
  {
    _laraf->tessera_area_last_update[id] = timeset;
    if(_lara->tessera[id].area != area)
    {
      align_set_area_counters(mm+2, _lara->tessera[id].area, area);
      _lara->tessera[id].area = area;
      _laraf->tessera[id].ag = 1;	// invalida
      
      //align_set_area_group(id, area, mm+2, timeset);
      lara_save(0);
          
      /* Segnalo una forzatura area per le tessere. */
      ev[0] = Evento_Esteso2;
      ev[1] = Ex2_Lara;
      ev[2] = 36;	// ID forzato in area
      ev[3] = id & 0xff;
      ev[4] = id >> 8;
      ev[5] = area;
      /* Questa funzione non è ancora esportata dal saet.new */
      //codec_queue_event_time(ev, timeset);
      codec_queue_event(ev);
    }
  }
}

static void align_parse_msg(Event *ev)
{
  struct tm tm;
  time_t t;
  int id, area;
  
  tm.tm_mday = ev->Event[2]&0x1f;
  tm.tm_mon = (ev->Event[3]&0x0f)-1;
  tm.tm_year = (ev->Event[4]&0x7f)+100;
  tm.tm_hour = ev->Event[5]&0x3f;
  tm.tm_min = ev->Event[6]&0x3f;
  tm.tm_sec = ev->Event[7];
  t = mktime(&tm);
  
  if((ev->Event[8] == 247) && (ev->Event[9] == 0))
  {
    /* Evento Tebe */
    switch(ev->Event[10])
    {
      case 7:
      case 9:
        id = ev->Event[11] + ev->Event[12]*256;
        area = ev->Event[14];
        align_set_area(id, area, t);
        break;
      case 36:
        id = ev->Event[11] + ev->Event[12]*256;
        area = ev->Event[13];
        align_set_area(id, area, t);
        break;
      default:
        break;
    }
  }
}

static void align_loop(ProtDevice *dev)
{
  int ret, i, evcurr, waitack, ibuf;
  Event ev;
  struct sockaddr_in addr;
  fd_set fds;
  struct timeval to;
  unsigned char buf[256];
  
  if(!dev) return;
  if(config.consumer[dev->consumer].configured != 5) return;
  
  pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
  
  if(config.consumer[dev->consumer].param)
  {
    support_log("Allineamento Init");
    dev->fd = -1;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(config.consumer[dev->consumer].data.eth.port);
    addr.sin_addr.s_addr = inet_addr(config.consumer[dev->consumer].param);
  }
  
  evcurr = waitack = 0;
  
  while(1)
  {
    if(config.consumer[dev->consumer].param)
    {
      /* Master - è la centrale che attiva la connessione */
      if(dev->fd < 0) dev->fd = socket(PF_INET, SOCK_STREAM, 0);
      ret = connect(dev->fd, (struct sockaddr*)&addr, sizeof(struct sockaddr_in));
      if(ret)
      {
        /* Riprova */
        sleep(1);
        continue;
      }
    }
    else
    {
      /* Slave - è la centrale che attende la connessione */
    }
    
    support_log("Allineamento connect");
    
    FD_ZERO(&fds);
    ibuf = 0;
    
    /* Le due centrali sono ora connesse tra loro */
    while(1)
    {
      FD_SET(dev->fd, &fds);
      to.tv_sec = 0;
      to.tv_usec = 100000;
      ret = select(dev->fd+1, &fds, NULL, NULL, &to);
      
      if(ret)
      {
        /* Ho ricevuto un messaggio */
        ret = read(dev->fd, buf+ibuf, sizeof(buf)-ibuf);
        if(ret <= 0)
        {
          /* Chiusura connessione */
          support_log("Allineamento close");
          close(dev->fd);
          if(config.consumer[dev->consumer].param)
          {
            dev->fd = -1;
            break;	// master
          }
          else
            return;	// slave
        }
        
        ibuf += ret;
        
        /* Interpreto la ricezione (potrei ricevere un evento ed un ACK insieme) */
        while(1)
        {
          /* Assicuro l'allineamento STX */
          for(i=0; (i<ibuf)&&(buf[i]!=0x02); i++);
          if(i)
          {
            if(i != ibuf)
            {
              memmove(buf, buf+i, ibuf-i);
              ibuf -= i;
            }
            else
              ibuf = 0;
          }
          if((ibuf > 3) && (ibuf >= (buf[3]+4)))
          {
            /* Gestisce il messaggio */
            if(!buf[3])
            {
              /* E' una conferma ricezione */
              waitack = 0;
              evcurr = 0;
            }
            else
            {
              /* Interpreta il messaggio */
              align_parse_msg((Event*)buf);
              /* Invia la conferma */
              write(dev->fd, Ack, sizeof(Ack));
            }
            
            /* Scarta il messaggio ricevuto */
            i = buf[3]+4;
            memmove(buf, buf+i, ibuf-i);
            ibuf -= i;
          }
          else
            break;
        }
      }
      else
        if(waitack) waitack--;
      
      if(!waitack)
      {
        if(evcurr <= 0)
          while(!(evcurr = codec_get_event(&ev, dev)));
        if(evcurr > 0)
        {
          /* Ho un evento da spedire */
          /* Per ora solo eventi Tebe */
          if((ev.Event[8] == 247) && (ev.Event[9] == 0))
          {
            /* Non invio il chksum, via TCP è inutile */
            write(dev->fd, &ev, ev.Len+4);
            waitack = 10;
          }
          else
            evcurr = 0;
        }
      }
    }
  }
}

void _init()
{
  printf("Allineamento (plugin): " __DATE__ " " __TIME__ "\n");
  prot_plugin_register("ALIGN", 0, NULL, NULL, (PthreadFunction)align_loop);
}

