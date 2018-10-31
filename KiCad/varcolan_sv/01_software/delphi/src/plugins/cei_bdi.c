#include "protocol.h"
#include "codec.h"
#include "serial.h"
#include "support.h"
#include "user.h"
#include "ronda.h"
#include "database.h"
#include "cei_bdi.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <byteswap.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>
#include <string.h>

#define DEBUG

#define CEI	((CEI_data*)(dev->prot_data))
#define L7	L4+8
#define L7_len	L4_len-12

static pthread_mutex_t cei_evlistmutex = PTHREAD_MUTEX_INITIALIZER;
static ProtDevice *cei_current_dev = NULL;
static unsigned char cei_zone_secret[1 + n_ZS + n_ZI];

static int cei_tempoaccadimento()
{
  struct tm tm;
  time_t t;
  
  memset(&tm, 0, sizeof(struct tm));
  tm.tm_mday = GIORNO;
  tm.tm_mon = MESE - 1;
  tm.tm_year = ANNO + 100;
  tm.tm_hour = ORE;
  tm.tm_min = MINUTI;
  tm.tm_sec = SECONDI;
  t = mktime(&tm);
  return bswap_32(t - BASESECONDS);
}

/* Funzione di configurazione dell'autenticazione.
   Ogni zona con segreto configurato richiede il codice al supervisore
   per eseguire le disattivazioni. */
   
void cei_segreto_zona(int zona, unsigned char segreto)
{
  if((zona >= 0) && (zona < (1 + n_ZS + n_ZI)) && ((segreto < n_SEGRETO) || (segreto == 0xff)))
    cei_zone_secret[zona] = segreto;
}

void cei_richiesta_segreto(int classe, int elemento, int cond)
{
  cei_event_list *tevlist;
  CEI_segnalazione_comandi_locali *ev;
  ProtDevice *dev = cei_current_dev;
  
  if(!cond) return;
  
  pthread_mutex_lock(&cei_evlistmutex);
  if(!cei_current_dev)
  {
    pthread_mutex_unlock(&cei_evlistmutex);
    return;
  }
  
  tevlist = malloc(sizeof(cei_event_list));
  tevlist->next = CEI->evlist;
  CEI->evlist = tevlist;
  tevlist->event = malloc(sizeof(CEI_segnalazione_comandi_locali));
  
  ev = ((CEI_segnalazione_comandi_locali*)(tevlist->event));
  
  ev->header.formato = 1;	// Formato ABI v.2
  ev->header.versione = 2;
  ev->header.ept = 0;
  ev->header.processo = 1;	// supervisione e controllo
  ev->header.tipoinfo = 5;	// segnalazione comandi locali
  ev->header.numeromessaggio = CEI->L4_pktnum[0]++;
  ev->tempoaccadimento = cei_tempoaccadimento();
  
  if(classe == CLS_ZONA)
    ev->codicecomando = 2;
  else
    ev->codicecomando = 4;
  ev->esito = 6;
  ev->classeelemento = classe;
  ev->numeroelementomsb = elemento >> 8;
  ev->numeroelementolsb = elemento & 0xff;
  
  tevlist->len = sizeof(CEI_segnalazione_comandi_locali);
  pthread_mutex_unlock(&cei_evlistmutex);  
}

void cei_annulla_sblocco(int fascia, int cond)
{
  int i;
  char cmd[16];
  
  if(!cond) return;
  
  /* va bene un consumatore qualunque */
  for(i=0; (i<MAX_NUM_CONSUMER) && !config.consumer[i].dev; i++);
  if(!config.consumer[i].dev) return; // non dovrebbe mai accadere
  
  sprintf(cmd, "/7%02d20000", fascia);
  codec_parse_cmd(cmd, 9, config.consumer[i].dev);
  sprintf(cmd, "/7%02d30000", fascia);
  codec_parse_cmd(cmd, 9, config.consumer[i].dev);
}

static unsigned short cei_CRC2(unsigned char *data, int len)
{
  unsigned short crc = 0;
  int i;
  
  for(i=0; i<len; i++)
    crc += data[i];
    
  return crc;
}

static unsigned short cei_CRC16(unsigned char *data, int len)
{
  unsigned short crc = 0xffff;
  int i, j;

  for (i = 0; i < len; i++)
  {
    crc ^= data[i];
    for (j = 0; j < 8; j++)
    {
      if (crc & 1)
      {
        crc >>= 1;
        crc ^= 0xa001;
      }
      else 
        crc >>= 1;
    }
  }
  
  return crc;
}

static int cei_byte_stuff(unsigned char *from, unsigned char *to, int len)
{
  int len2 = 0;
  int i;
  
  for(i=0; i<len; i++)
  {
    if((from[i] == STX) || (from[i] == ETX) || (from[i] == DLE))
    {
      to[len2++] = DLE;
      to[len2++] = from[i] | 0x80;
    }
    else
      to[len2++] = from[i];
  }
  
  return len2;
}

static int cei_L2_send_packet(ProtDevice *dev, unsigned char *pkt, int len)
{
  unsigned char *buf;
  unsigned short crc;
  
  if(CEI->L2out)
  {
    free(CEI->L2out);
    CEI->L2out = NULL;
    CEI->L2out_len = 0;
  }
  
  buf = (unsigned char*)malloc(len+8);
  if(!buf) return 0;
  
  CEI->L2out = (unsigned char*)malloc((len+10) * 2);
  if(!CEI->L2out)
  {
    free(buf);
    return 0;
  }
  
  CEI->L2out[0] = STX;

  buf[1] = CEI->Qout | CEI->dir | 0x08;
  CEI->Qout ^= CEI_L2_FLAG_Q;
  CEI->Qout &= ~CEI_L2_FLAG_NAK;
  buf[2] = config.DeviceID >> 8;
  buf[3] = config.DeviceID & 0xff;
  if(len < 249)
  {
    buf[0] = len;
    memcpy(buf + 4, pkt, len);
    switch(CEI->dir & 0x03)
    {
      case CEI_CRC2: crc = cei_CRC2(buf, len + 4); break;
      case CEI_CRC16: crc = cei_CRC16(buf, len + 4); break;
      default: len -= 2; break;
    }
    if(buf[0] == len)
    {
      buf[len+4] = crc >> 8;
      buf[len+5] = crc & 0xff;
    }
    CEI->L2out_len = cei_byte_stuff(buf, CEI->L2out + 1, len + 6) + 1;
  }
  else
  {
    buf[0] = 0xff;
    buf[4] = len >> 8;
    buf[5] = len & 0xff;
    memcpy(buf + 6, pkt, len);
    switch(CEI->dir & 0x03)
    {
      case CEI_CRC2: crc = cei_CRC2(buf, len + 6); break;
      case CEI_CRC16: crc = cei_CRC16(buf, len + 6); break;
      default: len -= 2; break;
    }
    if(buf[5] == (len & 0xff))
    {
      buf[len+6] = crc >> 8;
      buf[len+7] = crc & 0xff;
    }
    CEI->L2out_len = cei_byte_stuff(buf, CEI->L2out + 1, len + 8) + 1;
  }
  
  CEI->L2out[CEI->L2out_len++] = ETX;
  free(buf);
  
#ifdef DEBUG
{
int ret;
printf("TX:");
for(ret=0; ret<CEI->L2out_len; ret++)
printf("%02x ", CEI->L2out[ret]);
printf("\n");
}
#endif
  write(dev->fd, CEI->L2out, CEI->L2out_len);
  
  return 1;
}

static int cei_L2_get_packet_SM(ProtDevice *dev)
{
  int n, idx, state;
  fd_set fds;
  struct timeval to;
  
  idx = 0;
  state = 1;
  FD_ZERO(&fds);
  
  while(1)
  {
    if((state != 3) && (state != 5))
    {
      if(config.consumer[dev->consumer].configured == 5)
      {
        to.tv_sec = CEI_TIMEOUT/10;
        to.tv_usec = (CEI_TIMEOUT%10)*100000;
        FD_SET(dev->fd, &fds);
        n = select(dev->fd+1, &fds, NULL, NULL, &to);
        if(n > 0)
        {
          n = read(dev->fd, CEI->L2 + idx, 1);
          if(n <= 0)	// socket closed or error
            return -1;
        }
        else if(n == 0)	// timeout
          state = 5;
        else
          return -1;	// error
      }
      else
      {
        n = read(dev->fd, CEI->L2 + idx, 1);
        if(n <= 0) state = 5;	// timeout or error
      }
    }
    
    switch(state)
    {
      case 1:
        if(CEI->L2[0] == STX)
        {
          idx++;
          state = 2;
        }
        break;
      case 2:
        if(CEI->L2[idx] == STX)
          state = 5;
        else if(CEI->L2[idx] == ETX)
          state = 3;
        else if(CEI->L2[idx] == DLE)
          state = 4;
        else
          idx++;
        break;
      case 3:
        if(CEI->L2[1] != 0xff)
        {
          CEI->L4 = CEI->L2 + 5;
          CEI->L4_len = CEI->L2[1];
        }
        else
        {
          CEI->L4 = CEI->L2 + 7;
          CEI->L4_len = bswap_16(*(unsigned short*)(CEI->L2 + 5));
        }
        return 1;
        break;
      case 4:
        if((CEI->L2[idx] == (STX|0x80)) || (CEI->L2[idx] == (ETX|0x80)) || (CEI->L2[idx] == (DLE|0x80)))
        {
          CEI->L2[idx] &= 0x7f;
          idx++;
          state = 2;
        }
        else
          state = 5;
        break;
      case 5:
        CEI->L4 = NULL;
        CEI->L4_len = 0;
        return -2;
        break;
    }
  }
}

static int cei_L2_get_packet(ProtDevice *dev)
{
  int ret;

  ret = cei_L2_get_packet_SM(dev);
  
  if(ret < 0)
  {
    if(ret == -1) free(CEI->L2out);	// connection closed
    return ret;
  }
  
#ifdef DEBUG
printf("RX:");
for(ret=0; ret<((CEI->L4-CEI->L2)+CEI->L4_len+3); ret++)
printf("%02x ", CEI->L2[ret]);
printf("\n");
#endif
  
  if(!((CEI->L2[2] & CEI_L2_FLAG_DIR) ^ CEI->dir)) return 0;
  
  if((CEI->L2[2] & CEI_L2_FLAG_Q) != CEI->Q)
  {
    CEI->Q ^= CEI_L2_FLAG_Q;
  }
  
  if((((CEI->L2[2] & 0x03) == 0x00) &&
            (bswap_16(*((unsigned short*)(CEI->L4+CEI->L4_len))) != cei_CRC2(CEI->L2+1, CEI->L4-CEI->L2+CEI->L4_len-1))) ||
     (((CEI->L2[2] & 0x03) == 0x01) &&
            (bswap_16(*((unsigned short*)(CEI->L4+CEI->L4_len))) != cei_CRC16(CEI->L2+1, CEI->L4-CEI->L2+CEI->L4_len-1))))
  {
    CEI->Qout |= CEI_L2_FLAG_NAK;
    cei_L2_send_packet(dev, NULL, 0);
    return -2;
  }
    
  CEI->Q ^= CEI_L2_FLAG_Q;
  
  if(CEI->L2[2] & CEI_L2_FLAG_NAK)
  {
    if(CEI->L2out)
    {
#ifdef DEBUG
printf("TX:");
for(ret=0; ret<CEI->L2out_len; ret++)
printf("%02x ", CEI->L2out[ret]);
printf("\n");
#endif
      write(dev->fd, CEI->L2out, CEI->L2out_len);
    }
    return -2;
  }
  
  return 1;
}

static int cei_L3_send_packet(ProtDevice *dev, unsigned char *pkt, int len)
{
  /* Predisposizione al livello di rete */
  return cei_L2_send_packet(dev, pkt, len);
}

static int cei_L3_get_packet(ProtDevice *dev)
{
  /* Predisposizione al livello di rete */
  return cei_L2_get_packet(dev);
}

static int cei_L4_send_packet2(ProtDevice *dev, unsigned char *pkt, int len)
{
  int ret;
  
  if(!pkt) return cei_L3_send_packet(dev, NULL, 0);
  
  pkt[0] = 0;	// Ind. O
  pkt[1] = 0;
  pkt[2] = 0;	// Ind. D
  pkt[3] = 0;
  pkt[5] = 0;
  pkt[6] = CEI->S;	// S
  pkt[7] = CEI->R;	// R
  
  ret = cei_L3_send_packet(dev, pkt, len);
  return ret;
}

static int cei_L4_send_packet(ProtDevice *dev, void *pkt, int len)
{
  unsigned char tpkt[8];
  
  if(len == -1)
  {
    if(!CEI->lastpkt) CEI->lastpkt = malloc(8);
    if(CEI->flag & CEI_SYNCHRO)
    {
      CEI->lastpkt[4] = CEI_L4_FLAG_R;
      CEI->flag &= ~CEI_SYNCHRO;
      if(!CEI->lastlen) CEI->lastlen = 8;
    }
    else
      CEI->lastpkt[4] = 0;
    return cei_L4_send_packet2(dev, CEI->lastpkt, CEI->lastlen);
  }
  
//  if(pkt && !(CEI->flag & CEI_SENDREPLY))
  if(pkt && (len > 8))
    CEI->flag |= CEI_WAITREPLY;
  
  if(!pkt && (CEI->flag & CEI_SENDREPLY))
  {
    pkt = tpkt;
    len = 8;
  }
  
  CEI->flag &= ~CEI_SENDREPLY;
  
  free(CEI->lastpkt);
  CEI->lastpkt = NULL;
  CEI->lastlen = 0;
  
  if(!pkt) return cei_L4_send_packet2(dev, NULL, 0);
  if(len < 8) return 0;
  
  if(CEI->flag & CEI_SYNCHRO)
  {
    ((unsigned char*)pkt)[4] = CEI_L4_FLAG_R;
    CEI->flag &= ~CEI_SYNCHRO;
  }
  else
    ((unsigned char*)pkt)[4] &= 0x01;
  
  CEI->lastpkt = malloc(len);
  memcpy(CEI->lastpkt, pkt, len);
  CEI->lastlen = len;
  
  return cei_L4_send_packet2(dev, (unsigned char*)pkt, len);
}

static int cei_L4_get_packet(ProtDevice *dev)
{
  int ret;
  unsigned char pkt[32];
  
  if((ret = cei_L3_get_packet(dev)) <= 0)
  {
    /* se stavo aspettando una risposta, al timeout reinvio il messaggio */
    if((ret == -2) && (CEI->flag & CEI_WAITREPLY)) ret = -3;
    return ret;
  }
  
  if(CEI->L4_len)
  {
    /* controllo flusso di trasporto */
    
    if(CEI->L4[4] & CEI_L4_FLAG_R)
    {
      /* forzatura contatore S */
      CEI->S = CEI->L4[7]-1;
      /* forzatura contatore R */
      CEI->R = CEI->L4[6];
    }
    
#if 0
    if(CEI->flag & CEI_WAITREPLY)	/* attesa risposta */
    {
      if(CEI->L4[7] != (unsigned char)(CEI->S+1))
      {
        CEI->outoforder++;
        if(CEI->outoforder == 3)
        {
          /* alla terza risposta fuori sequenza, reinvio il messaggio forzando i contatori */
          CEI->flag |= CEI_SYNCHRO;
          return -3;
        }
        /* se non ricevo la conferma, al quarto tentativo chiudo e riparto */
        if(CEI->outoforder == 4) return -1;
        /* aspetto la conferma con la sequenza corretta */
        return -2;
      }
      CEI->S = CEI->L4[7];
      CEI->flag &= ~CEI_WAITREPLY;
    }
    else
    {
      CEI->flag |= CEI_SENDREPLY;
      if(CEI->L4[6] != CEI->R)
      {
        cei_L4_send_packet(dev, pkt, 8);
        return -2;
      }
      CEI->R++;
    }
#else
    if(CEI->flag & CEI_WAITREPLY)	/* attesa risposta */
    {
      if(CEI->L4[7] != (CEI->S+1))
      {
        CEI->outoforder++;
        if(CEI->outoforder == 3)
        {
          /* alla terza risposta fuori sequenza, reinvio il messaggio forzando i contatori */
          CEI->flag |= CEI_SYNCHRO;
          return -3;
        }
        /* se non ricevo la conferma, al quarto tentativo chiudo e riparto */
        if(CEI->outoforder == 4) return -1;
        /* aspetto la conferma con la sequenza corretta */
        return -2;
      }
      CEI->S = CEI->L4[7];
      CEI->flag &= ~CEI_WAITREPLY;
    }
    
    if(CEI->L4_len > 8)
    {
      CEI->flag |= CEI_SENDREPLY;
      if(CEI->L4[6] != CEI->R)
      {
        cei_L4_send_packet(dev, pkt, 8);
        return -2;
      }
      CEI->R++;
    }
#endif
    
    CEI->outoforder = 0;
        
    /* livello di protezione */
    if(CEI->L4[4] & CEI_L4_FLAG_LP)
    {
      pkt[4] = CEI_L4_FLAG_S;	// livello protezione degradata
      cei_L4_send_packet2(dev, pkt, 8);
      return -2;
    }
    
    if(CEI->L4[4] & CEI_L4_FLAG_TA)
    {
      /* dati di trasporto */
      pkt[4] = CEI_L4_FLAG_TA;
      switch(((CEI_trasporto*)(CEI->L4))->funzione)
      {
        case 1:
          ((CEI_trasporto*)pkt)->funzione = 255;
          ((CEI_trasporto*)pkt)->dati[0] = 1;
          ((CEI_trasporto*)pkt)->dati[1] = 4;
          cei_L4_send_packet(dev, pkt, sizeof(CEI_trasporto)+2);
          return -2;
        case 6:
          memcpy(CEI->idR1, ((CEI_trasporto*)(CEI->L4))->dati, 8);
          memcpy(CEI->idR2, "12345678", 8);
          ((CEI_trasporto*)pkt)->funzione = 7;
          memcpy(((CEI_trasporto*)pkt)->dati, CEI->idR1, 8);
          memcpy(((CEI_trasporto*)pkt)->dati+8, CEI->idR2, 8);
          cei_L4_send_packet(dev, pkt, sizeof(CEI_trasporto)+16);
          return -2;
        case 7:
          memcpy(CEI->idR2, ((CEI_trasporto*)(CEI->L4))->dati+8, 8);
          ((CEI_trasporto*)pkt)->funzione = 8;
          memcpy(((CEI_trasporto*)pkt)->dati, CEI->idR2, 8);
          cei_L4_send_packet(dev, pkt, sizeof(CEI_trasporto)+8);
          return -2;
        case 8:
//          if(memcmp(((CEI_trasporto*)pkt)->dati, CEI->idR2, 8))
//            return -2;
          pkt[4] = 0;
          cei_L4_send_packet(dev, pkt, 8);
          return -2;
        case 13:
          ((CEI_trasporto*)pkt)->funzione = 255;
          ((CEI_trasporto*)pkt)->dati[0] = 13;
          ((CEI_trasporto*)pkt)->dati[1] = 5;
          cei_L4_send_packet(dev, pkt, sizeof(CEI_trasporto)+2);
          return -2;
        default:
          return -2;
      }
    }
  }
  else if(CEI->flag & CEI_WAITREPLY)
    return -3;
  
  return CEI->L4_len;
}

int cei_configure(CEI_data *cei, int type, int prot_lvl)
{
  /* autenticazione e crittografia non ancora supportata */
  if(prot_lvl > 0) return 0;
  
  cei->dir = type & (CEI_L2_FLAG_DIR | 0x03);
  return 1;
}

#define ALM_		0
#define CLS_		0
#define NOT_IMPL	-1

struct _cei_event {char tipoevento, transizione, classeelemento, numbytes;};

static struct _cei_event cei_event_table[MAX_NUM_EVENT] = {
{ALM_ALLARME, ALM_INIZIO, CLS_SENSORE, 2}, /* Allarme Sensore */
{ALM_ALLARME, ALM_FINE, CLS_SENSORE, 2}, /* Ripristino Sensore */
{ALM_GUASTO, ALM_INIZIO, CLS_UNITA_SATELLITE, 1}, /* Guasto Periferica */
{ALM_GUASTO, ALM_FINE, CLS_UNITA_SATELLITE, 1}, /* Fine Guasto Periferica */
{ALM_MANOMISSIONE, ALM_INIZIO, CLS_SENSORE, 2}, /* Manomissione Dispositivo */
{ALM_MANOMISSIONE, ALM_FINE, CLS_SENSORE, 2}, /* Fine Manomis Dispositivo */
{ALM_MANOMISSIONE, ALM_INIZIO, CLS_UNITA_SATELLITE, 1}, /* Manomissione Contenitore */
{ALM_MANOMISSIONE, ALM_FINE, CLS_UNITA_SATELLITE, 1}, /* Fine Manomis Contenitore */
{ALM_DISINSERITO, ALM_INIZIO, CLS_ZONA, 1}, /* Attivata zona */
{ALM_DISINSERITO, ALM_FINE, CLS_ZONA, 1}, /* Disattivata zona */
{ALM_DISINSERITO, ALM_PERSIST, CLS_ZONA, 1}, /* Attivazione impedita */
{ALM_DISABILITATO, ALM_FINE, CLS_SENSORE, 2}, /* Sensore In Servizio */
{ALM_DISABILITATO, ALM_INIZIO, CLS_SENSORE, 2}, /* Sensore Fuori Servizio */
{ALM_DISABILITATO, ALM_FINE, CLS_ATTUATORE, 2}, /* Attuatore In Servizio */
{ALM_DISABILITATO, ALM_INIZIO, CLS_ATTUATORE, 2}, /* Attuatore Fuori Servizio */
{ALM_ERRORE_PROGR, ALM_INIZIO, CLS_UNITA_SATELLITE, 1}, /* Ricezione codice errato */
{ALM_, ALM_, CLS_, NOT_IMPL}, /* Tipo ModuloMaster */
{ALM_, ALM_, CLS_, NOT_IMPL}, /* Periferica Incongruente */
{ALM_MANOMISSIONE, ALM_INIZIO, CLS_UNITA_SATELLITE, 1}, /* Periferica Manomessa */
{ALM_MANOMISSIONE, ALM_FINE, CLS_UNITA_SATELLITE, 1}, /* Periferica Ripristino */
{ALM_INTERR_LINEA, ALM_INIZIO, CLS_UNITA_SATELLITE, 1}, /* Sospesa attivita linea */
{ALM_CHIAVE, ALM_FINE, CLS_UNITA_CENTRALE, 1}, /* Chiave Falsa */
{ALM_DISABILITATO, ALM_FINE, CLS_ORGANO_COMANDO, 1}, /* Sentinella on */
{ALM_DISABILITATO, ALM_INIZIO, CLS_ORGANO_COMANDO, 1}, /* Sentinella off */
{ALM_DISABILITATO, ALM_INIZIO, CLS_ORGANO_COMANDO, 1}, /* Sentinella off timeout */
{ALM_GUASTO_IF, ALM_INIZIO, CLS_UNITA_SATELLITE, 1}, /* No ModuloMasterTx */
{ALM_GUASTO_IF, ALM_INIZIO, CLS_UNITA_SATELLITE, 1}, /* ErrRx ModuloMaster */
{ALM_ERRORE_PROGR, ALM_INIZIO, CLS_UNITA_CENTRALE, 0}, /* Errore messaggio host */
/* --- RONDA --- */
{ALM_RONDA, ALM_INIZIO, CLS_UNITA_CENTRALE, 2}, /* Segnalazione evento */
/* ------------- */
{ALM_, ALM_, CLS_, NOT_IMPL}, /* Periferiche presenti */
{ALM_STATO, ALM_INIZIO, CLS_ZONA, 1}, /* Lista zone */
{ALM_STATO, ALM_INIZIO, CLS_SENSORE, 2}, /* Lista sensori */
{ALM_STATO, ALM_INIZIO, CLS_ATTUATORE, 2}, /* Lista attuatori */
{ALM_GUASTO, ALM_INIZIO, CLS_SENSORE, 2}, /* Guasto Sensore */
{ALM_, ALM_, CLS_, NOT_IMPL}, /* Variazione ora */
{ALM_, ALM_, CLS_, NOT_IMPL}, /* Codici controllo */
{ALM_, ALM_, CLS_, NOT_IMPL}, /* Stato QRA Ronda */
{ALM_, ALM_, CLS_, NOT_IMPL}, /* Accettato allarme Sensore */
{ALM_, ALM_, CLS_, NOT_IMPL}, /* Mancata punzonatura */
{ALM_ATTIVAZIONE, ALM_INIZIO, CLS_UNITA_CENTRALE, 2}, /* ON telecomando */
{ALM_, ALM_, CLS_, NOT_IMPL}, /* Parametri taratura */
{ALM_TESTMANUALE, ALM_, CLS_UNITA_CENTRALE, 0}, /* Stato prova */
{ALM_, ALM_, CLS_, NOT_IMPL}, /* Test attivo in corso */
{ALM_, ALM_, CLS_, NOT_IMPL}, /* Risposta SCS */
{ALM_GUASTO, ALM_FINE, CLS_SENSORE, 2}, /* Fine Guasto Sensore */
{ALM_, ALM_, CLS_, NOT_IMPL}, /* Invio codice segreto */
{ALM_, ALM_, CLS_, NOT_IMPL}, /* Perifere previste */
{ALM_ALLARME, ALM_PERSIST, CLS_SENSORE, 2}, /* Sensore in Allarme */
{ALM_, ALM_, CLS_, NOT_IMPL}, /* Variata fascia oraria */
{ALM_, ALM_, CLS_, NOT_IMPL}, /* Invio host no storico */
{ALM_, ALM_, CLS_, NOT_IMPL}, /* Invio Memoria */
{ALM_STATO, ALM_FINE, 0, 0}, /* Fine Invio Memoria */
{ALM_, ALM_, CLS_, NOT_IMPL}, /* Liste codici */
{ALM_, ALM_, CLS_, NOT_IMPL}, /* Orari ronda */
{ALM_, ALM_, CLS_, NOT_IMPL}, /* Giorni festivi */
{ALM_, ALM_, CLS_, NOT_IMPL}, /* Fasce orarie */
{ALM_ATTIVAZIONE, ALM_INIZIO, CLS_ATTUATORE, 2}, /* ON attuatore */
{ALM_, ALM_, CLS_, NOT_IMPL}, /* Transitato Ident */
{ALM_, ALM_, CLS_, NOT_IMPL}, /* Entrato Ident */
{ALM_, ALM_, CLS_, NOT_IMPL}, /* Uscito Ident */
{ALM_CODICE, ALM_PERSIST, CLS_UNITA_CENTRALE, 1}, /* Codice valido */
{ALM_CHIAVE, ALM_INIZIO, CLS_UNITA_CENTRALE, 1}, /* Chiave valida */
{ALM_ACCESSO_OPER, ALM_INIZIO, CLS_OPERATORE, 1}, /* Operatore */
{ALM_, ALM_, CLS_, NOT_IMPL}, /* Punzonatura */
{ALM_ATTIVAZIONE, ALM_FINE, CLS_ATTUATORE, 2}, /* Spegnimento */
{ALM_, ALM_, CLS_, NOT_IMPL}, /* Reset fumo */
{ALM_, ALM_, CLS_, NOT_IMPL}, /* Livello abilitato */
{ALM_, ALM_, CLS_, NOT_IMPL}, /* Variato segreto */
{ALM_, ALM_, CLS_, NOT_IMPL}, /* Conferma ricezione modem */
{ALM_, ALM_, CLS_, NOT_IMPL}, /* N telefonico 1 */
{ALM_, ALM_, CLS_, NOT_IMPL}, /* N telefonico 2 */
{ALM_, ALM_, CLS_, NOT_IMPL}, /* StatoBadge */
{ALM_ACC_ALLARME, ALM_INIZIO, CLS_SENSORE, 2}, /* Evento Sensore */
{ALM_STATO, ALM_PERSIST, CLS_SENSORE, 2}, /* Lista MU */
{ALM_, ALM_, CLS_, NOT_IMPL}, /* Lista ZS */
{ALM_, ALM_, CLS_, NOT_IMPL}, /* Str Descr Disp */
{ALM_, ALM_, CLS_, NOT_IMPL}, /* Str Descr Zona */
{ALM_, ALM_, CLS_, NOT_IMPL}, /* Variato stato Festivo */
{ALM_, ALM_, CLS_, NOT_IMPL}, /* Valori analogici */
{ALM_, ALM_, CLS_, NOT_IMPL}, /* Badge letto */
{ALM_, ALM_, CLS_, NOT_IMPL},
{ALM_, ALM_, CLS_, NOT_IMPL},
{ALM_, ALM_, CLS_, NOT_IMPL},
{ALM_, ALM_, CLS_, NOT_IMPL},
{ALM_, ALM_, CLS_, NOT_IMPL},
{ALM_, ALM_, CLS_, NOT_IMPL},
{ALM_, ALM_, CLS_, NOT_IMPL},
{ALM_, ALM_, CLS_, NOT_IMPL},
{ALM_, ALM_, CLS_, NOT_IMPL},
{ALM_, ALM_, CLS_, NOT_IMPL},
{ALM_, ALM_, CLS_, NOT_IMPL},
{ALM_, ALM_, CLS_, NOT_IMPL},
{ALM_, ALM_, CLS_, NOT_IMPL},
{ALM_, ALM_, CLS_, NOT_IMPL},
{ALM_, ALM_, CLS_, NOT_IMPL},
{ALM_, ALM_, CLS_, NOT_IMPL},
{ALM_, ALM_, CLS_, NOT_IMPL},
{ALM_, ALM_, CLS_, NOT_IMPL},
{ALM_, ALM_, CLS_, NOT_IMPL},
{ALM_, ALM_, CLS_, NOT_IMPL},
{ALM_, ALM_, CLS_, NOT_IMPL},
{ALM_, ALM_, CLS_, NOT_IMPL},
{ALM_, ALM_, CLS_, NOT_IMPL},
{ALM_, ALM_, CLS_, NOT_IMPL},
{ALM_DISINSERITO, ALM_FINE, CLS_UNITA_SATELLITE, 1}, /* Governo_linee_in_servizio */
{ALM_DISINSERITO, ALM_INIZIO, CLS_UNITA_SATELLITE, 1}, /* Governo_linee_fuori_servizio */
};

static struct _cei_event cei_event_table_ex2[MAX_NUM_EVENT_EX2] = {
{ALM_TEST_ATTIVO, ALM_INIZIO, CLS_UNITA_CENTRALE, 1}, /* Inizio test attivo */
{ALM_TEST_ATTIVO, ALM_FINE, CLS_UNITA_CENTRALE, 1}, /* Fine test attivo */
};

static void cei_event_set_alarm(CEI_allarme *alarm, Event *ev, int tempoaccadimento)
{
  alarm->header.formato = 1;	// Formato ABI v.2
  alarm->header.versione = 2;
  
  alarm->header.processo = 1;	// supervisione e controllo
  alarm->header.tipoinfo = 1;	// allarmi e stati
  
  alarm->tempoaccadimento = tempoaccadimento;
  alarm->areaapplicativa = 0;
  alarm->tiporivelatore = 0;
  alarm->categoria = 1;	// non richiede azioni da CSC
  alarm->areaprovenienza = 0;
  
  alarm->transizione = cei_event_table[ev->Event[8]-EV_OFFSET].transizione;
  alarm->tipoevento = cei_event_table[ev->Event[8]-EV_OFFSET].tipoevento;
  alarm->classeelemento = cei_event_table[ev->Event[8]-EV_OFFSET].classeelemento;
    
  switch(cei_event_table[ev->Event[8]-EV_OFFSET].numbytes)
  {
    case 1:
      alarm->numeroelementomsb = 0;
      alarm->numeroelementolsb = ev->Event[9];
      break;
    case 2:
      alarm->numeroelementomsb = ev->Event[9];
      alarm->numeroelementolsb = ev->Event[10];
      break;
    default:
      alarm->numeroelementomsb = 0;
      alarm->numeroelementolsb = 0;
      break;
  }
}

static void cei_event_set_alarm_ex2(CEI_allarme *alarm, Event *ev, int tempoaccadimento)
{
  alarm->header.formato = 1;	// Formato ABI v.2
  alarm->header.versione = 2;
  
  alarm->header.processo = 1;	// supervisione e controllo
  alarm->header.tipoinfo = 1;	// allarmi e stati
  
  alarm->tempoaccadimento = tempoaccadimento;
  alarm->areaapplicativa = 0;
  alarm->tiporivelatore = 0;
  alarm->categoria = 1;	// non richiede azioni da CSC
  alarm->areaprovenienza = 0;
  
  alarm->transizione = cei_event_table_ex2[ev->Event[10]].transizione;
  alarm->tipoevento = cei_event_table_ex2[ev->Event[10]].tipoevento;
  alarm->classeelemento = cei_event_table_ex2[ev->Event[10]].classeelemento;
    
  switch(cei_event_table_ex2[ev->Event[10]].numbytes)
  {
    case 1:
      alarm->numeroelementomsb = 0;
      alarm->numeroelementolsb = ev->Event[11];
      break;
    case 2:
      alarm->numeroelementomsb = ev->Event[11];
      alarm->numeroelementolsb = ev->Event[12];
      break;
    default:
      alarm->numeroelementomsb = 0;
      alarm->numeroelementolsb = 0;
      break;
  }
}

static int cei_event_to_l4cei(ProtDevice *dev, Event *ev, unsigned char **pkt)
{
  int i, len, num, tempoaccadimento;
  unsigned char bitmask, tipoevento;
  CEI_allarme *alarm;
  CEI_stato_header *header;
  CEI_stato *stato;
  struct tm tm;
  time_t t;
  
#if 0
#warning !!!!!!!!!!!!!!!
printf("Evento: ");
for(i=0;i<ev->Len;i++) printf("%02x ", ev->Event[i]);
printf("\n");
#endif
  
  if(!ev || !ev->Len)
  {
    *pkt = NULL;
    return 0;
  }
  
  if(((ev->Event[8] < EV_OFFSET) || (ev->Event[8] >= (EV_OFFSET + MAX_NUM_EVENT)) || 
     (cei_event_table[ev->Event[8]-EV_OFFSET].numbytes == NOT_IMPL)) && (ev->Event[8] != Evento_Esteso2))
  {
    *pkt = NULL;
    return -1;
  }
  
  memset(&tm, 0, sizeof(struct tm));
  tm.tm_mday = ev->Event[2];
  tm.tm_mon = ev->Event[3] - 1;
  tm.tm_year = ev->Event[4] + 100;
  tm.tm_hour = ev->Event[5];
  tm.tm_min = ev->Event[6] & 0x3f;
  tm.tm_sec = ev->Event[7];
  t = mktime(&tm);
  tempoaccadimento = bswap_32(t - BASESECONDS);
  
  if(ev->Event[8] == Evento_Esteso2)
  {
    switch(ev->Event[9])
    {
      case Ex2_Delphi:
        len = sizeof(CEI_allarme);
        alarm = calloc(1, len);
        if(!alarm) return -1;

        alarm->header.numeromessaggio = CEI->L4_pktnum[0]++;
        cei_event_set_alarm_ex2(alarm, ev, tempoaccadimento);
        *pkt = (unsigned char*)alarm;
        break;
      default:
        *pkt = NULL;
        return -1;
        break;
    }
  }
  else if(cei_event_table[ev->Event[8]-EV_OFFSET].tipoevento == ALM_STATO)
  {
    if(ev->Event[8] == FineInvioMemoria)
      num = 1;
    else if(ev->Event[8] == Lista_MU)
      num = 24;
    else
      num = 32;
    len = sizeof(CEI_stato_header) + sizeof(CEI_stato) * num;
    if(!CEI->lista)
    {
      len += sizeof(CEI_stato);
      CEI->lista = 1;	// inizio allineamento
    }
    header = calloc(1, len);
    if(!header) return -1;
    stato = (void*)header + sizeof(CEI_stato_header);
    
    header->header.formato = 1;	// Formato ABI v.2
    header->header.versione = 2;
    header->header.processo = 2;	// trasferimento stati per allineamento
    header->header.numeromessaggio = CEI->L4_pktnum[1]++;
    header->header.tipoinfo = 2;	// allineamento
    header->tempoaccadimento = tempoaccadimento;
    
    if(CEI->lista == 1)
    {
      stato->transizione = 1;
      CEI->lista = 2;
      stato++;
    }
    
    for(i=0; i<num; i++)
    {
      if(ev->Event[8] == FineInvioMemoria)
      {
        stato[i].tipoevento = 0;
        CEI->lista = 0;	// fine allineamento
      }
      else if(ev->Event[8] == Lista_MU)
      {
        switch(i % 3)
        {
          case 0:
            bitmask = bitAlarm;
            tipoevento = ALM_ACC_ALLARME;
            break;
          case 1:
            bitmask = bitSabotage;
            tipoevento = ALM_ACC_MANOMISSIONE;
            break;
          case 2:
            bitmask = bitFailure;
            tipoevento = ALM_ACC_GUASTO;
            break;
        }
        stato->transizione = (ev->Event[11 + (i/3)] & bitmask)?2:0;
        stato->tipoevento = tipoevento;
        stato->classeelemento = cei_event_table[ev->Event[8]-EV_OFFSET].classeelemento;
        stato->numeroelementomsb = ev->Event[9];
        stato->numeroelementolsb = ev->Event[10];
      }
      else
      {
        if(ev->Event[8] == Lista_zone)
        {
          switch(i % 4)
          {
            case 0:
              bitmask = bitAlarm;
              tipoevento = ALM_ALLARME;
              break;
            case 1:
              bitmask = bitSabotage;
              tipoevento = ALM_MANOMISSIONE;
              break;
            case 2:
              bitmask = bitFailure;
              tipoevento = ALM_GUASTO;
              break;
            case 3:
              bitmask = bitOOS;
              tipoevento = ALM_DISINSERITO;
              break;
          }
          if((i%4) == 3)
            stato->transizione = (ev->Event[10 + (i/4)] & bitmask)?0:2;
          else
            stato->transizione = (ev->Event[10 + (i/4)] & bitmask)?2:0;
          stato->tipoevento = tipoevento;
          stato->classeelemento = cei_event_table[ev->Event[8]-EV_OFFSET].classeelemento;
          stato->numeroelementolsb = ev->Event[9];
        }
        else
        {
          switch(i % 4)
          {
            case 0:
              if(ev->Event[8] == Lista_sensori)
              {
                bitmask = bitAlarm;
                tipoevento = ALM_ALLARME;
              }
              else
              {
                bitmask = bitON;
                tipoevento = ALM_ATTIVAZIONE;
              }
              break;
            case 1:
              bitmask = bitSabotage;
              tipoevento = ALM_MANOMISSIONE;
              break;
            case 2:
              bitmask = bitFailure;
              tipoevento = ALM_GUASTO;
              break;
            case 3:
              bitmask = bitOOS;
              tipoevento = ALM_DISABILITATO;
              break;
          }
          stato->transizione = (ev->Event[11 + (i/4)] & bitmask)?2:0;
          stato->tipoevento = tipoevento;
          stato->classeelemento = cei_event_table[ev->Event[8]-EV_OFFSET].classeelemento;
          stato->numeroelementomsb = ev->Event[9];
          stato->numeroelementolsb = ev->Event[10];
        }
      }
      
      if(num > 1) stato->numeroelementolsb += i / (num/8);
      stato++;
    }
    *pkt = (unsigned char*)header;
  }
  else
  {
    len = sizeof(CEI_allarme);
    alarm = calloc(1, len);
    if(!alarm) return -1;

    alarm->header.numeromessaggio = CEI->L4_pktnum[0]++;
    
    cei_event_set_alarm(alarm, ev, tempoaccadimento);
    if(ev->Event[8] == Stato_prova)
      alarm->transizione = ev->Event[9];
    
    if(ev->Event[8] == Evento_Sensore)
    {
      alarm->tipoevento = ALM_ACC_ALLARME - 1 + ev->Event[11];
      if((ev->Event[12] == 3) || (ev->Event[12] == 5)) alarm->transizione = ALM_FINE;
      if(ev->Event[12] == 4) alarm->transizione = ALM_PERSIST;
    }
    else if(ev->Event[8] == Segnalazione_evento)
    {
      alarm->areaapplicativa = 7;	// ronda
      alarm->transizione = 1;
      
      num = (alarm->numeroelementomsb<<8) + alarm->numeroelementolsb;
      alarm->numeroelementomsb = 0;
      alarm->numeroelementolsb = 0;
      
      switch(num)
      {
        case 1000:
          alarm->tipoevento = 1;	// preallarme
          alarm->areaapplicativa = 0;
          break;
        case 0:
          alarm->tipoevento = ALM_R_GIRO;
          alarm->transizione = 0;
          break;
        case 1:
          alarm->tipoevento = ALM_R_GIRO;
          break;
        case 2:
          alarm->tipoevento = ALM_R_PARTENZA;
          alarm->transizione = 0;
          break;
        default:
          alarm->classeelemento = CLS_UNITA_SATELLITE;
          alarm->numeroelementolsb = num % 100;
          switch(num/100)
          {
            case 1:
              alarm->tipoevento = ALM_R_PARTENZA;
              break;
            case 2:
              alarm->tipoevento = ALM_R_PUNZONATURA;
              break;
            case 3:
              alarm->tipoevento = ALM_R_MANCATA_PUNZ;
              break;
            case 4:
              alarm->tipoevento = ALM_R_RIP_MANC_PUNZ;
              break;
            case 5:
              alarm->tipoevento = ALM_R_FUORI_TEMPO;
              break;
            case 6:
              alarm->tipoevento = ALM_R_SEQUENZA;
              break;
            case 7:
              alarm->tipoevento = ALM_R_CHIAVE_FALSA;
              break;
            case 8:
              alarm->tipoevento = ALM_R_SOS_RONDA;
              break;
            default:
              free(alarm);
              *pkt = NULL;
              return -1;    
              break;
          }
          break;
      }
    }
    
    *pkt = (unsigned char*)alarm;
  }
  
  return len;
}

static int cei_L4_parse_command(ProtDevice *dev, int abilitato)
{
  int i, j, len, ret = -1;
  int elemento, valore;
  CEI_risposta_telecomando risp;
  time_t t;
  struct tm *datehour;
  char cmd[24], cmd2[16];
  void *pkt;
  unsigned char orai, mini, oraf, minf;
  
  cmd[0] = 0;
  len = 1;
  
  switch(((CEI_header_ex*)(CEI->L4))->processo)
  {
    case 1: // Supervisione e controllo
      if(((CEI_header_ex*)(CEI->L4))->tipoinfo == 3)	// Telecomandi
      {
        elemento = (((CEI_telecomando*)(CEI->L4))->numeroelementomsb << 8) |
                   ((CEI_telecomando*)(CEI->L4))->numeroelementolsb;
                   
        if(abilitato)
        {
          switch(((CEI_telecomando*)(CEI->L4))->comando)
          {
            case 1:
              if(((CEI_telecomando*)(CEI->L4))->classeelemento == CLS_ZONA)
              {
                if(ME[637])
                {
                  ME[637] = 0;
                  sprintf(cmd, "Y1%03d", elemento);
                  len = 5;
                }
                else
                {
                  sprintf(cmd, "I%03d", elemento);
                  len = 4;
                }
              }
              break;
            case 2:
              if((((CEI_telecomando*)(CEI->L4))->classeelemento == CLS_ZONA) && (elemento < (1 + n_ZS + n_ZI)))
              {
                if(cei_zone_secret[elemento] == 0xff)
                {
                  sprintf(cmd, "J%03d", elemento);
                  len = 4;
                }
                else
                {
                  cei_richiesta_segreto(CLS_ZONA, elemento, 1);
                  ret = 0;
                }
              }
              break;
            case 3:
              if(((CEI_telecomando*)(CEI->L4))->classeelemento == CLS_SENSORE)
              {
                sprintf(cmd, "D%04d", elemento);
                len = 5;
              }
              else if(((CEI_telecomando*)(CEI->L4))->classeelemento == CLS_ATTUATORE)
              {
                sprintf(cmd, "B%04d", elemento);
                len = 5;
              }
              break;
            case 4:
              if(((CEI_telecomando*)(CEI->L4))->classeelemento == CLS_SENSORE)
              {
                if((Zona_SE[elemento] == 0xff) || (cei_zone_secret[Zona_SE[elemento]] == 0xff))
                {
                  sprintf(cmd, "C%04d", elemento);
                  len = 5;
                }
                else
                {
                  cei_richiesta_segreto(CLS_SENSORE, elemento, 1);
                  ret = 0;
                }
              }
              else if(((CEI_telecomando*)(CEI->L4))->classeelemento == CLS_ATTUATORE)
              {
                sprintf(cmd, "A%04d", elemento);
                len = 5;
              }
              break;
            case 13:
              if(((CEI_telecomando*)(CEI->L4))->classeelemento == CLS_UNITA_CENTRALE)
                cmd[0] = 'L';
              break;
            case 14:
              if(((CEI_telecomando*)(CEI->L4))->classeelemento == CLS_UNITA_CENTRALE)
                cmd[0] = 'O';
              break;
            case 20:
              if(((CEI_telecomando*)(CEI->L4))->classeelemento == CLS_ATTUATORE)
              {
                if(!elemento)
                  cmd[0] = 'G';
                else if(elemento == 1)
                  cmd[0] = 'H';
                else
                  ret = 0;
              }
              break;
            case 21:
              if(((CEI_telecomando*)(CEI->L4))->classeelemento == CLS_SENSORE)
              {
                memcpy(cmd, "/389", 4);
                len = 4;
              }
              break;
            case 201:
              if(((CEI_telecomando*)(CEI->L4))->classeelemento == CLS_UNITA_CENTRALE)
              {
                sprintf(cmd, "K%04d", elemento);
                len = 4;
              }
              break;
            case 203:
              if(((CEI_telecomando*)(CEI->L4))->classeelemento == CLS_SENSORE)
              {
                memcpy(cmd, "/381", 4);
                cmd[4] = ((CEI_telecomando*)(CEI->L4))->numeroelementomsb;
                cmd[5] = ((CEI_telecomando*)(CEI->L4))->numeroelementolsb;
                len = 6;
              }
              break;
            case 204:
              if(((CEI_telecomando*)(CEI->L4))->classeelemento == CLS_SENSORE)
              {
                memcpy(cmd, "/382", 4);
                cmd[4] = ((CEI_telecomando*)(CEI->L4))->numeroelementomsb;
                cmd[5] = ((CEI_telecomando*)(CEI->L4))->numeroelementolsb;
                len = 6;
              }
              break;
            case 205:
              if(((CEI_telecomando*)(CEI->L4))->classeelemento == CLS_SENSORE)
              {
                memcpy(cmd, "/383", 4);
                cmd[4] = ((CEI_telecomando*)(CEI->L4))->numeroelementomsb;
                cmd[5] = ((CEI_telecomando*)(CEI->L4))->numeroelementolsb;
                len = 6;
              }
              break;
            case 206:
              if(((CEI_telecomando*)(CEI->L4))->classeelemento == CLS_SENSORE)
                memcpy(cmd, "/35", 3);
              else if(((CEI_telecomando*)(CEI->L4))->classeelemento == CLS_ATTUATORE)
                memcpy(cmd, "/36", 3);
              else if(((CEI_telecomando*)(CEI->L4))->classeelemento == CLS_UNITA_SATELLITE)
                memcpy(cmd, "/37", 3);
              len = 3;
              break;
            
            /* Telecomandi ronda */
            case 11:
              if(((CEI_telecomando*)(CEI->L4))->classeelemento == CLS_UNITA_CENTRALE)
              {
                memcpy(cmd, "/550", 4);
                cmd[4] = '0' + ((CEI_telecomando*)(CEI->L4))->numeroelementomsb;
                len = 5;
              }
              break;
            case 209:
              if(((CEI_telecomando*)(CEI->L4))->classeelemento == CLS_UNITA_CENTRALE)
              {
                memcpy(cmd, "/551", 4);
                cmd[4] = '0' + ((CEI_telecomando*)(CEI->L4))->numeroelementomsb;
                len = 5;
              }
              break;
            case 210:
              if(((CEI_telecomando*)(CEI->L4))->classeelemento == CLS_UNITA_CENTRALE)
              {
                memcpy(cmd, "/552", 4);
                cmd[4] = '0' + ((CEI_telecomando*)(CEI->L4))->numeroelementomsb;
                len = 5;
              }
              break;
            case 211:
              if(((CEI_telecomando*)(CEI->L4))->classeelemento == CLS_UNITA_CENTRALE)
              {
                memcpy(cmd, "K638", 4);
                len = 4;
              }
              break;
            case 212:
              if(((CEI_telecomando*)(CEI->L4))->classeelemento == CLS_UNITA_CENTRALE)
              {
                memcpy(cmd, "K639", 4);
                len = 4;
              }
              break;
          }
          if(cmd[0]) ret = codec_parse_cmd(cmd, len, dev);
        }
        else
          ret = -2;
        
        risp.header.formato = 1;	// Formato ABI v.2
        risp.header.versione = 2;
        risp.header.ept = 0;
        risp.header.processo = 1;	// supervisione e controllo
        risp.header.tipoinfo = 4;	// risposta al telecomando
        risp.header.numeromessaggio = CEI->L4_pktnum[0]++;
        switch(ret)
        {
          case 0:
            risp.esito = 3; break;
          case 1:
            risp.esito = 2; break;
          case -2:
            risp.esito = 1; break;
          default:
            risp.esito = 5; break;
        }
        risp.tempoaccadimento = cei_tempoaccadimento();
        risp.numeroorigine = ((CEI_telecomando*)(CEI->L4))->header.numeromessaggio;
        cei_L4_send_packet(dev, &risp, sizeof(CEI_risposta_telecomando));
      }
      else if(((CEI_header_ex*)(CEI->L4))->tipoinfo == 6)	// Telecomandi con valori
      {
        elemento = (((CEI_telecomando*)(CEI->L4))->numeroelementomsb << 8) |
                   ((CEI_telecomando*)(CEI->L4))->numeroelementolsb;
                   
        valore = (((CEI_telecomando_valori*)(CEI->L4))->valoremsb << 8) +
                 ((CEI_telecomando_valori*)(CEI->L4))->valorelsb;
                
        if(abilitato)
        {
          switch(((CEI_telecomando_valori*)(CEI->L4))->comando)
          {
            case 2:
              if((((CEI_telecomando_valori*)(CEI->L4))->classeelemento == CLS_ZONA) && (elemento < (1 + n_ZS + n_ZI)))
              {
                if((cei_zone_secret[elemento] == 0xff) || (SEGRETO[cei_zone_secret[elemento]] == valore))
                {
                  sprintf(cmd, "J%03d", elemento);
                  len = 4;
                }
                else
                  ret = 0;
              }
              break;
            case 4:
              if(((CEI_telecomando*)(CEI->L4))->classeelemento == CLS_SENSORE)
              {
                if((Zona_SE[elemento] == 0xff) || (cei_zone_secret[Zona_SE[elemento]] == 0xff)
                    || (SEGRETO[0] == valore))
                {
                  sprintf(cmd, "C%04d", elemento);
                  len = 5;
                }
                else
                  ret = 0;
              }
              break;
            case 207:
              if(((CEI_telecomando_valori*)(CEI->L4))->classeelemento == CLS_UNITA_CENTRALE)
              {
                elemento = (((CEI_telecomando_valori*)(CEI->L4))->secondovaloremsb << 8) +
                            ((CEI_telecomando_valori*)(CEI->L4))->secondovalorelsb;
                if(elemento != SEGRETO[((CEI_telecomando_valori*)(CEI->L4))->numeroelementolsb])
                  ret = 0;
                else
                {
                  memcpy(cmd, "/52", 3);
                  cmd[3] = ((CEI_telecomando_valori*)(CEI->L4))->numeroelementolsb + '0';
                  cmd[4] = ((CEI_telecomando_valori*)(CEI->L4))->valorelsb;
                  cmd[5] = ((CEI_telecomando_valori*)(CEI->L4))->valoremsb;
                  len = 6;
                }
              }
              break;
            case 208:
              if(((CEI_telecomando_valori*)(CEI->L4))->classeelemento == CLS_UNITA_CENTRALE)
              {
                memcpy(cmd, "/53", 3);
                cmd[3] = ((CEI_telecomando_valori*)(CEI->L4))->numeroelementolsb + '0';
                cmd[4] = ((CEI_telecomando_valori*)(CEI->L4))->valorelsb;
                cmd[5] = ((CEI_telecomando_valori*)(CEI->L4))->valoremsb;
                len = 6;
              }
              break;
          }
          if(cmd[0]) ret = codec_parse_cmd(cmd, len, dev);
        }
        else
          ret = -2;
        
        risp.header.formato = 1;	// Formato ABI v.2
        risp.header.versione = 2;
        risp.header.ept = 0;
        risp.header.processo = 1;	// supervisione e controllo
        risp.header.tipoinfo = 4;	// risposta al telecomando
        risp.header.numeromessaggio = CEI->L4_pktnum[0]++;
        switch(ret)
        {
          case 0:
            risp.esito = 3; break;
          case 1:
            risp.esito = 2; break;
          case -2:
            risp.esito = 1; break;
          default:
            risp.esito = 5; break;
        }
        risp.tempoaccadimento = cei_tempoaccadimento();
        risp.numeroorigine = ((CEI_telecomando*)(CEI->L4))->header.numeromessaggio;
        cei_L4_send_packet(dev, &risp, sizeof(CEI_risposta_telecomando));
      }
      break;
    case 2: // Trasferimento stati per allineamento CA/CSC
      if(((CEI_header_ex*)(CEI->L4))->tipoinfo == 1)	// Richiesta allineamento
      {
        if(abilitato)
        {
          switch(((CEI_richiesta_allineamento*)(CEI->L4))->comando)
          {
            case 1:
              if(((CEI_telecomando*)(CEI->L4))->classeelemento == CLS_SENSORE)
                cmd[0] = 'd';
              else if(((CEI_telecomando*)(CEI->L4))->classeelemento == CLS_ATTUATORE)
                cmd[0] = 'e';
              else if(((CEI_telecomando*)(CEI->L4))->classeelemento == CLS_ZONA)
                cmd[0] = 'c';
              break;
            case 2:
              if(((CEI_telecomando*)(CEI->L4))->classeelemento == CLS_SENSORE)
              {
                memcpy(cmd, "/380", 4);
                len = 4;
              }
              break;
          }
          if(cmd[0]) ret = codec_parse_cmd(cmd, len, dev);
        }
        cei_L4_send_packet(dev, NULL, 0);
      }
      break;
    case 3: // Trasferimento parametri di gestione impianto
      switch(((CEI_header_ex*)(CEI->L4))->tipoinfo)
      {
        case 1:	// Trasmissione data e ora        
          t = bswap_32(((CEI_trasmissione_dataora*)(CEI->L4))->tempo) + BASESECONDS - timezone;
          datehour = gmtime(&t);
          sprintf(cmd, "U%d%02d%02d%02d", datehour->tm_wday, datehour->tm_mday, datehour->tm_mon+1, datehour->tm_year-100);
          sprintf(cmd2, "W%02d%02d%02d", datehour->tm_hour, datehour->tm_min, datehour->tm_sec);
          codec_parse_cmd(cmd, 8, dev);
          cei_L4_send_packet(dev, NULL, 0);
          ret = codec_parse_cmd(cmd2, 7, dev);
          break;
        case 3: // Trasmissione festivita' fisse
          t = time(NULL);
          for(i=0; i<n_FESTIVI; i++)
          {
            t += 86400;
            datehour = localtime(&t);
            FESTIVI[i] = 0;
            for(j=0; j<16; j++)
            {
              if(((CEI_trasmissione_festivita*)(CEI->L4))->fest[j].tipo &&
                 (((CEI_trasmissione_festivita*)(CEI->L4))->fest[j].giorno == datehour->tm_mday) &&
                 (((CEI_trasmissione_festivita*)(CEI->L4))->fest[j].mese == (datehour->tm_mon+1)))
              {
                if((3-((CEI_trasmissione_festivita*)(CEI->L4))->fest[j].tipo) > FESTIVI[i])
                  FESTIVI[i] = 3 - ((CEI_trasmissione_festivita*)(CEI->L4))->fest[j].tipo;
                break;
              }
            }
          }
          memcpy(cmd, "/540", 4);
          memcpy(cmd+4, FESTIVI, 16);
          codec_parse_cmd(cmd, 20, dev);
          memcpy(cmd, "/541", 4);
          memcpy(cmd+4, FESTIVI+16, 16);
          ret = codec_parse_cmd(cmd, 20, dev);
          cei_L4_send_packet(dev, NULL, 0);
          break;
        case 5: // Trasmissione festivita' variabili per blocchi
          {
            CEI_festivita_variabili *fest = ((CEI_trasmissione_festivita_variabili*)(CEI->L4))->fest;
            for(i=0; i<32; i++)
            {
              sprintf(cmd, "T%02d%d%02d%02d%02d%02d", i, fest[i].tipo,
                      fest[i].mesei, fest[i].giornoi, fest[i].mesef, fest[i].giornof);
              ret = codec_parse_cmd(cmd, 12, dev);
            }
          }
          cei_L4_send_packet(dev, NULL, 0);
          break;
        case 6: // Trasmissione fasce orarie
          for(i=0;i<7;i++)
          {
            for(j=0;j<4;j++)
            {
              if(((CEI_trasmissione_fasce*)(CEI->L4))->fascia[i][j].ignora_inizio)
              {
                if(((CEI_trasmissione_fasce*)(CEI->L4))->fascia[i][j].ora_inizio == 0x7f)
                {
                  sprintf(cmd, "/7%02d20000", i*4+j);
                  codec_parse_cmd(cmd, 9, dev);
                }
              }
              else
              {
                sprintf(cmd, "/7%02d0%02d%02d", i*4+j,
                  ((CEI_trasmissione_fasce*)(CEI->L4))->fascia[i][j].ora_inizio,
                  ((CEI_trasmissione_fasce*)(CEI->L4))->fascia[i][j].minuti_inizio);
                codec_parse_cmd(cmd, 9, dev);
              }
              
              if(((CEI_trasmissione_fasce*)(CEI->L4))->fascia[i][j].ignora_fine)
              {
                if(((CEI_trasmissione_fasce*)(CEI->L4))->fascia[i][j].ora_fine == 0x7f)
                {
                  sprintf(cmd, "/7%02d30000", i*4+j);
                  codec_parse_cmd(cmd, 9, dev);
                }
              }
              else
              {
                sprintf(cmd, "/7%02d1%02d%02d", i*4+j,
                  ((CEI_trasmissione_fasce*)(CEI->L4))->fascia[i][j].ora_fine,
                  ((CEI_trasmissione_fasce*)(CEI->L4))->fascia[i][j].minuti_fine);
                codec_parse_cmd(cmd, 9, dev);
              }
            }
          }
          cei_L4_send_packet(dev, NULL, 0);
          break;
        case 7: // Trasmissione fasce di sblocco orario
          // prime 24 ore
          orai = ((CEI_trasmissione_fasce_sblocco*)(CEI->L4))->fascia[0].ora_inizio;
          mini = ((CEI_trasmissione_fasce_sblocco*)(CEI->L4))->fascia[0].minuti_inizio;
          oraf = ((CEI_trasmissione_fasce_sblocco*)(CEI->L4))->fascia[0].ora_fine;
          minf = ((CEI_trasmissione_fasce_sblocco*)(CEI->L4))->fascia[0].minuti_fine;
          if(((CEI_trasmissione_fasce_sblocco*)(CEI->L4))->fascia[0].ignora_inizio)
          {
          }
          
          if(orai == 0x7f)
          {
              sprintf(cmd, "/7%02d20000", ((GIORNO_S-1)<<1)+28);
              codec_parse_cmd(cmd, 9, dev);
              sprintf(cmd, "/7%02d30000", ((GIORNO_S-1)<<1)+28);
              codec_parse_cmd(cmd, 9, dev);
              sprintf(cmd, "/7%02d20000", ((GIORNO_S%7)<<1)+29);
              codec_parse_cmd(cmd, 9, dev);
              sprintf(cmd, "/7%02d30000", ((GIORNO_S%7)<<1)+29);
              codec_parse_cmd(cmd, 9, dev);
          }
          else if(orai >= ORE)
          {
            if(oraf < orai)
            {
              // scavalcamento mezzanotte (prima fascia giorno attuale e seconda fascia giorno successivo)
              sprintf(cmd, "/7%02d0%02d%02d", ((GIORNO_S-1)<<1)+28, orai, mini);
              codec_parse_cmd(cmd, 9, dev);
              sprintf(cmd, "/7%02d30000", ((GIORNO_S-1)<<1)+28);
              codec_parse_cmd(cmd, 9, dev);
              sprintf(cmd, "/7%02d20000", ((GIORNO_S%7)<<1)+29);
              codec_parse_cmd(cmd, 9, dev);
              sprintf(cmd, "/7%02d1%02d%02d", ((GIORNO_S%7)<<1)+29, oraf, minf);
              codec_parse_cmd(cmd, 9, dev);
            }
            else
            {
              // sera giorno attuale (prima fascia giorno attuale)
              sprintf(cmd, "/7%02d0%02d%02d", ((GIORNO_S-1)<<1)+28, orai, mini);
              codec_parse_cmd(cmd, 9, dev);
              sprintf(cmd, "/7%02d1%02d%02d", ((GIORNO_S-1)<<1)+28, oraf, minf);
              codec_parse_cmd(cmd, 9, dev);
              sprintf(cmd, "/7%02d20000", ((GIORNO_S%7)<<1)+29);
              codec_parse_cmd(cmd, 9, dev);
              sprintf(cmd, "/7%02d30000", ((GIORNO_S%7)<<1)+29);
              codec_parse_cmd(cmd, 9, dev);
            }
          }
          else
          {
            // mattino giorno successivo (seconda fascia giorno successivo)
            sprintf(cmd, "/7%02d20000", ((GIORNO_S-1)<<1)+28);
            codec_parse_cmd(cmd, 9, dev);
            sprintf(cmd, "/7%02d30000", ((GIORNO_S-1)<<1)+28);
            codec_parse_cmd(cmd, 9, dev);
            sprintf(cmd, "/7%02d0%02d%02d", ((GIORNO_S%7)<<1)+29, orai, mini);
            codec_parse_cmd(cmd, 9, dev);
            sprintf(cmd, "/7%02d1%02d%02d", ((GIORNO_S%7)<<1)+29, oraf, minf);
            codec_parse_cmd(cmd, 9, dev);
          }
          
          // seconde 24 ore
          orai = ((CEI_trasmissione_fasce_sblocco*)(CEI->L4))->fascia[1].ora_inizio;
          mini = ((CEI_trasmissione_fasce_sblocco*)(CEI->L4))->fascia[1].minuti_inizio;
          oraf = ((CEI_trasmissione_fasce_sblocco*)(CEI->L4))->fascia[1].ora_fine;
          minf = ((CEI_trasmissione_fasce_sblocco*)(CEI->L4))->fascia[1].minuti_fine;
          
          
          if(orai == 0x7f)
          {
              sprintf(cmd, "/7%02d20000", ((GIORNO_S%7)<<1)+28);
              codec_parse_cmd(cmd, 9, dev);
              sprintf(cmd, "/7%02d30000", ((GIORNO_S%7)<<1)+28);
              codec_parse_cmd(cmd, 9, dev);
              sprintf(cmd, "/7%02d20000", (((GIORNO_S+1)%7)<<1)+29);
              codec_parse_cmd(cmd, 9, dev);
              sprintf(cmd, "/7%02d30000", (((GIORNO_S+1)%7)<<1)+29);
              codec_parse_cmd(cmd, 9, dev);
          }
          else if(orai >= ORE)
          {
            if(oraf < orai)
            {
              // scavalcamento mezzanotte (prima fascia giorno attuale e seconda fascia giorno successivo)
              sprintf(cmd, "/7%02d0%02d%02d", ((GIORNO_S%7)<<1)+28, orai, mini);
              codec_parse_cmd(cmd, 9, dev);
              sprintf(cmd, "/7%02d30000", ((GIORNO_S%7)<<1)+28);
              codec_parse_cmd(cmd, 9, dev);
              sprintf(cmd, "/7%02d20000", (((GIORNO_S+1)%7)<<1)+29);
              codec_parse_cmd(cmd, 9, dev);
              sprintf(cmd, "/7%02d1%02d%02d", (((GIORNO_S+1)%7)<<1)+29, oraf, minf);
              codec_parse_cmd(cmd, 9, dev);
            }
            else
            {
              // sera giorno attuale (prima fascia giorno attuale)
              sprintf(cmd, "/7%02d0%02d%02d", ((GIORNO_S%7)<<1)+28, orai, mini);
              codec_parse_cmd(cmd, 9, dev);
              sprintf(cmd, "/7%02d1%02d%02d", ((GIORNO_S%7)<<1)+28, oraf, minf);
              codec_parse_cmd(cmd, 9, dev);
              sprintf(cmd, "/7%02d20000", (((GIORNO_S+1)%7)<<1)+29);
              codec_parse_cmd(cmd, 9, dev);
              sprintf(cmd, "/7%02d30000", (((GIORNO_S+1)%7)<<1)+29);
              codec_parse_cmd(cmd, 9, dev);
            }
          }
          else
          {
            // mattino giorno successivo (seconda fascia giorno successivo)
            sprintf(cmd, "/7%02d20000", ((GIORNO_S%7)<<1)+28);
            codec_parse_cmd(cmd, 9, dev);
            sprintf(cmd, "/7%02d30000", ((GIORNO_S%7)<<1)+28);
            codec_parse_cmd(cmd, 9, dev);
            sprintf(cmd, "/7%02d0%02d%02d", (((GIORNO_S+1)%7)<<1)+29, orai, mini);
            codec_parse_cmd(cmd, 9, dev);
            sprintf(cmd, "/7%02d1%02d%02d", (((GIORNO_S+1)%7)<<1)+29, oraf, minf);
            codec_parse_cmd(cmd, 9, dev);
          }

          cei_L4_send_packet(dev, NULL, 0);
          break;
        case 8: // Trasmissione fasce orarie per periodi
          for(i=0;i<32;i++)
          {
              if(((CEI_trasmissione_fasce_periodo*)(CEI->L4))->fascia[i].ignora_inizio)
              {
                if(((CEI_trasmissione_fasce_periodo*)(CEI->L4))->fascia[i].ora_inizio == 0x7f)
                {
                  sprintf(cmd, "/7%02d20000", 42+i);
                  codec_parse_cmd(cmd, 9, dev);
                }
              }
              else
              {
                sprintf(cmd, "/7%02d0%02d%02d", 42+i,
                  ((CEI_trasmissione_fasce_periodo*)(CEI->L4))->fascia[i].ora_inizio,
                  ((CEI_trasmissione_fasce_periodo*)(CEI->L4))->fascia[i].minuti_inizio);
                codec_parse_cmd(cmd, 9, dev);
              }
              
              if(((CEI_trasmissione_fasce_periodo*)(CEI->L4))->fascia[i].ignora_fine)
              {
                if(((CEI_trasmissione_fasce_periodo*)(CEI->L4))->fascia[i].ora_fine == 0x7f)
                {
                  sprintf(cmd, "/7%02d30000", 42+i);
                  codec_parse_cmd(cmd, 9, dev);
                }
              }
              else
              {
                sprintf(cmd, "/7%02d1%02d%02d", 42+i,
                  ((CEI_trasmissione_fasce_periodo*)(CEI->L4))->fascia[i].ora_fine,
                  ((CEI_trasmissione_fasce_periodo*)(CEI->L4))->fascia[i].minuti_fine);
                codec_parse_cmd(cmd, 9, dev);
              }
          }
          cei_L4_send_packet(dev, NULL, 0);
          break;
        case 15: // Richiesta parametri
          pkt = NULL;
          switch(((CEI_richiesta_parametri*)(CEI->L4))->info_richiesta)
          {
            case 1:
              ret = sizeof(CEI_trasmissione_dataora);
              pkt = calloc(1, ret);
              if(pkt)
              {
                ((CEI_trasmissione_dataora*)pkt)->header.versione = 1;
                ((CEI_trasmissione_dataora*)pkt)->header.formato = 2;
                ((CEI_trasmissione_dataora*)pkt)->header.processo = 3;
                ((CEI_trasmissione_dataora*)pkt)->header.tipoinfo = 1;
                ((CEI_trasmissione_dataora*)pkt)->header.numeromessaggio = CEI->L4_pktnum[2]++;
                ((CEI_trasmissione_dataora*)pkt)->tempo = cei_tempoaccadimento();
              }
              else
                ret = 0;
              break;
            case 3:
              ret = sizeof(CEI_trasmissione_festivita);
              pkt = calloc(1, ret);
              if(pkt)
              {
                ((CEI_trasmissione_festivita*)pkt)->header.versione = 1;
                ((CEI_trasmissione_festivita*)pkt)->header.formato = 2;
                ((CEI_trasmissione_festivita*)pkt)->header.processo = 3;
                ((CEI_trasmissione_festivita*)pkt)->header.tipoinfo = 3;
                ((CEI_trasmissione_festivita*)pkt)->header.numeromessaggio = CEI->L4_pktnum[2]++;
                t = time(NULL);
                for(i=j=0; (i<n_FESTIVI) && (j<16); i++)
                {
                  t += 86400;
                  if(FESTIVI[i])
                  {
                    datehour = localtime(&t);
                    ((CEI_trasmissione_festivita*)pkt)->fest[j].tipo = 3 - FESTIVI[i];
                    ((CEI_trasmissione_festivita*)pkt)->fest[j].giorno = datehour->tm_mday;
                    ((CEI_trasmissione_festivita*)pkt)->fest[j].mese = datehour->tm_mon + 1;
                    j++;
                  }
                }
              }
              else
                ret = 0;
              break;
            case 5:
              ret = sizeof(CEI_trasmissione_festivita_variabili);
              pkt = calloc(1, ret);
              if(pkt)
              {
                ((CEI_trasmissione_fasce*)pkt)->header.versione = 1;
                ((CEI_trasmissione_fasce*)pkt)->header.formato = 2;
                ((CEI_trasmissione_fasce*)pkt)->header.processo = 3;
                ((CEI_trasmissione_fasce*)pkt)->header.tipoinfo = 5;
                ((CEI_trasmissione_fasce*)pkt)->header.numeromessaggio = CEI->L4_pktnum[2]++;
                memcpy(((CEI_trasmissione_festivita_variabili*)(CEI->L4))->fest, periodo, sizeof(CEI_festivita_variabili) * 32);
              }
              else
                ret = 0;
              break;
            case 6:
              ret = sizeof(CEI_trasmissione_fasce);
              pkt = calloc(1, ret);
              if(pkt)
              {
                ((CEI_trasmissione_fasce*)pkt)->header.versione = 1;
                ((CEI_trasmissione_fasce*)pkt)->header.formato = 2;
                ((CEI_trasmissione_fasce*)pkt)->header.processo = 3;
                ((CEI_trasmissione_fasce*)pkt)->header.tipoinfo = 6;
                ((CEI_trasmissione_fasce*)pkt)->header.numeromessaggio = CEI->L4_pktnum[2]++;
                memcpy(((CEI_trasmissione_fasce*)pkt)->fascia, FAGR, sizeof(CEI_fascia) * 4 * 7);
              }
              else
                ret = 0;
              break;
            case 8:
              ret = sizeof(CEI_trasmissione_fasce_periodo);
              pkt = calloc(1, ret);
              if(pkt)
              {
                ((CEI_trasmissione_fasce_periodo*)pkt)->header.versione = 1;
                ((CEI_trasmissione_fasce_periodo*)pkt)->header.formato = 2;
                ((CEI_trasmissione_fasce_periodo*)pkt)->header.processo = 3;
                ((CEI_trasmissione_fasce_periodo*)pkt)->header.tipoinfo = 8;
                ((CEI_trasmissione_fasce_periodo*)pkt)->header.numeromessaggio = CEI->L4_pktnum[2]++;
                memcpy(((CEI_trasmissione_fasce_periodo*)pkt)->fascia, FAGR[42], sizeof(CEI_fascia) * 32);
              }
              else
                ret = 0;
              break;
            default:
              ret = 0;
              break;
          }
          
          cei_L4_send_packet(dev, pkt, ret);
          if(pkt)
          {
            free(pkt);
            ret = 1;
          }
          else
            ret = 0;
          break;
      }
      break;
    case 4: // Trasferimento parametri di configurazione
      switch(((CEI_header_ex*)(CEI->L4))->tipoinfo)
      {
        case 1:	// richiesta parametri
          switch(((CEI_parametri_configurazione*)(CEI->L4))->tipodati)
          {
            case 1:
              pkt = calloc(1, sizeof(CEI_parametri_configurazione) + 32);
              if(pkt)
              {
                ((CEI_parametri_configurazione*)pkt)->header.versione = 1;
                ((CEI_parametri_configurazione*)pkt)->header.formato = 2;
                ((CEI_parametri_configurazione*)pkt)->header.processo = 4;
                ((CEI_parametri_configurazione*)pkt)->header.tipoinfo = 2;
                ((CEI_parametri_configurazione*)pkt)->header.numeromessaggio = CEI->L4_pktnum[3]++;
                ((CEI_parametri_configurazione*)pkt)->tipodati = 1;
                ((CEI_parametri_configurazione*)pkt)->indice = ((CEI_parametri_configurazione*)(CEI->L4))->indice;
                memcpy(((CEI_parametri_configurazione*)(CEI->L4))->dati,
                       Zona_SE + (32 * ((CEI_parametri_configurazione*)(CEI->L4))->indice), 32);
                cei_L4_send_packet(dev, pkt, sizeof(CEI_parametri_configurazione) + 32);
                free(pkt);
                ret = 1;
              }
              else
                ret = 0;
              break;
            case 2:
              ret = 0;
              if((((CEI_parametri_configurazione*)(CEI->L4))->indice < RONDA_NUM) && Ronda_percorso_p)
              {
                pkt = calloc(1, sizeof(CEI_parametri_configurazione) + 608);
                if(pkt)
                {
                  ((CEI_parametri_configurazione*)pkt)->header.versione = 1;
                  ((CEI_parametri_configurazione*)pkt)->header.formato = 2;
                  ((CEI_parametri_configurazione*)pkt)->header.processo = 4;
                  ((CEI_parametri_configurazione*)pkt)->header.tipoinfo = 2;
                  ((CEI_parametri_configurazione*)pkt)->header.numeromessaggio = CEI->L4_pktnum[3]++;
                  ((CEI_parametri_configurazione*)pkt)->tipodati = 2;
                  ((CEI_parametri_configurazione*)pkt)->indice = ((CEI_parametri_configurazione*)(CEI->L4))->indice;
                  memcpy(((CEI_parametri_configurazione*)(CEI->L4))->dati,
                         Ronda_percorso_p + ((CEI_parametri_configurazione*)(CEI->L4))->indice *
                           RONDA_STAZIONI * sizeof(Ronda_percorso_t), 608);
                  cei_L4_send_packet(dev, pkt, sizeof(CEI_parametri_configurazione) + 608);
                  free(pkt);
                  ret = 1;
                }
              }
              break;
            case 3:
              ret = 0;
              if((((CEI_parametri_configurazione*)(CEI->L4))->indice < 3) && Ronda_orario_p)
              {
                pkt = calloc(1, sizeof(CEI_parametri_configurazione) + 96);
                if(pkt)
                {
                  ((CEI_parametri_configurazione*)pkt)->header.versione = 1;
                  ((CEI_parametri_configurazione*)pkt)->header.formato = 2;
                  ((CEI_parametri_configurazione*)pkt)->header.processo = 4;
                  ((CEI_parametri_configurazione*)pkt)->header.tipoinfo = 2;
                  ((CEI_parametri_configurazione*)pkt)->header.numeromessaggio = CEI->L4_pktnum[3]++;
                  ((CEI_parametri_configurazione*)pkt)->tipodati = 3;
                  ((CEI_parametri_configurazione*)pkt)->indice = ((CEI_parametri_configurazione*)(CEI->L4))->indice;
                  memcpy(((CEI_parametri_configurazione*)(CEI->L4))->dati,
                         Ronda_orario_p + ((CEI_parametri_configurazione*)(CEI->L4))->indice * 
                           RONDA_ORARI * sizeof(Ronda_orario_t), 96);
                  cei_L4_send_packet(dev, pkt, sizeof(CEI_parametri_configurazione) + 96);
                  free(pkt);
                  ret = 1;
                }
              }
              break;
          }
          break;
        case 3: // caricamento parametri
          switch(((CEI_parametri_configurazione*)(CEI->L4))->tipodati)
          {
            case 1:
              cmd[0] = 'Z';
              for(i=0; i<4; i++)
              {
                *(short*)(cmd+1) = ((((CEI_parametri_configurazione*)(CEI->L4))->indice << 2) + i) << 3;
                memcpy(cmd+3, ((CEI_parametri_configurazione*)(CEI->L4))->dati + (i<<3), 8);
                ret = codec_parse_cmd(cmd, 11, dev);
              }
              cei_L4_send_packet(dev, NULL, 0);
              break;
            case 2:
              for(i = 0; i<RONDA_STAZIONI; i++)
              {
                j = i * 19;
                
                memcpy(cmd, "/560", 4);
                cmd[4] = '0'+((CEI_parametri_configurazione*)(CEI->L4))->indice;
                cmd[5] = i;
                cmd[6] = ((CEI_parametri_configurazione*)(CEI->L4))->dati[j];
                cmd[7] = ((CEI_parametri_configurazione*)(CEI->L4))->dati[j+1];
                cmd[8] = ((CEI_parametri_configurazione*)(CEI->L4))->dati[j+2];
                codec_parse_cmd(cmd, 9, dev);
                
                memcpy(cmd, "/561", 4);
                cmd[4] = '0'+((CEI_parametri_configurazione*)(CEI->L4))->indice;
                cmd[5] = i;
                memcpy(cmd+6, ((CEI_parametri_configurazione*)(CEI->L4))->dati + j + 3, 12);
                codec_parse_cmd(cmd, 18, dev);
                
                memcpy(cmd, "/562", 4);
                cmd[4] = '0'+((CEI_parametri_configurazione*)(CEI->L4))->indice;
                cmd[5] = i;
                memcpy(cmd+6, ((CEI_parametri_configurazione*)(CEI->L4))->dati + j + 15, 4);
                ret = codec_parse_cmd(cmd, 10, dev);
              }
              cei_L4_send_packet(dev, NULL, 0);
              break;
            case 3:
              for(i = 0; i<RONDA_ORARI; i++)
              {
                memcpy(cmd, "/563", 4);
                cmd[4] = ((CEI_parametri_configurazione*)(CEI->L4))->indice;
                cmd[5] = i;
                memcpy(cmd+6, ((CEI_parametri_configurazione*)(CEI->L4))->dati + i*3, 3);
                ret = codec_parse_cmd(cmd, 9, dev);
              }
              cei_L4_send_packet(dev, NULL, 0);
              break;
            default:
              break;
          }
          break;
      }
      break;
  }
  
  return ret;
}

static int cei_init(ProtDevice *dev)
{
  if(dev->prot_data) return 1;
  
  dev->prot_data = malloc(sizeof(CEI_data));
  if(!dev->prot_data) return 0;
  
  CEI->L2out = NULL;
  CEI->L2out_len = 0;
  CEI->Qout = 0;
  CEI->S = 0;
  CEI->R = 0;
  CEI->outoforder = 0;
  CEI->flag = 0;
  CEI->Q = 0;
  CEI->L4 = NULL;
  CEI->L4_len = 0;
  CEI->L4_pktnum[0] = 0;
  CEI->L4_pktnum[1] = 0;
  CEI->L4_pktnum[2] = 0;
  CEI->L4_pktnum[3] = 0;
  CEI->dir = CEI_CSC;
  CEI->lista = 0;
  CEI->lastpkt = NULL;
  CEI->lastlen = 0;
  CEI->evlist = NULL;
  
  if(config.consumer[dev->consumer].configured != 5)
    ser_setspeed(dev->fd, config.consumer[dev->consumer].data.serial.baud1, CEI_TIMEOUT);
  
  return 1;
}

static void cei_free(ProtDevice *dev)
{
  free(dev->prot_data);
  dev->prot_data = NULL;
}

static void cei_ca_loop(ProtDevice *dev)
{
  Event ev;
  int ret, num;
  unsigned char *pkt;
  
  if(!dev || !cei_init(dev)) return;
  
  support_log("CEI Connect");
  
//  cei_configure(CEI, CEI_CA | CEI_CRC16, CEI_PROT_NONE);
  cei_configure(CEI, CEI_CA | CEI_CRC2, CEI_PROT_NONE);
  pthread_mutex_lock(&cei_evlistmutex);
  if(!cei_current_dev) cei_current_dev = dev;
  pthread_mutex_unlock(&cei_evlistmutex);
  
  while(1)
  {
#if 0
    while((ret = cei_L4_get_packet(dev)) == -2);
#else
    num = 15;	// 30 secondi
    while(num && ((ret = cei_L4_get_packet(dev)) == -2)) num--;
#endif
    
    if(ret == -3)
    {
      cei_L4_send_packet(dev, NULL, -1);
      continue;
    }
    
    if(ret < 0)
    {
      if(config.consumer[dev->consumer].configured == 5)
      {
        cei_event_list *elt;
        /* evita la chiusura del plugin durante un accodamento
           di un evento di richiesta segreto e viceversa */
        pthread_mutex_lock(&cei_evlistmutex);
        cei_current_dev = NULL;
        pthread_mutex_unlock(&cei_evlistmutex);
        while(CEI->evlist)
        {
          elt = CEI->evlist;
          CEI->evlist = CEI->evlist->next;
          free(elt->event);
          free(elt);
        }
        cei_free(dev);
        
        LIN[dev->consumer] = 0;
        support_log("CEI Close");
        return;	// TCP connection closed
      }
      else
      {
        // restart communication on serial line
        
        continue;
      }
    }
    
    /* Gestione LIN come per protocollo SaetNet. */
    LIN[dev->consumer] = bitAlarm;
  
    if(CEI->L4_len > 8)
    {
      cei_L4_parse_command(dev, LS_ABILITATA[dev->consumer] & 0x02);
    }
    else	// polling
    {
      ret = -1;
      pthread_mutex_lock(&cei_evlistmutex);
      if(CEI->evlist)
      {
        cei_event_list *tevlist;
        
        pkt = CEI->evlist->event;
        ret = CEI->evlist->len;
        tevlist = CEI->evlist;
        CEI->evlist = CEI->evlist->next;
        free(tevlist);
      }
      pthread_mutex_unlock(&cei_evlistmutex);
      while(ret < 0)
      {
        while(!codec_get_event(&ev, dev));
#warning -------------------------
#warning Gestione 16 moduli master
#warning -------------------------
//#error
        ret = cei_event_to_l4cei(dev, &ev, &pkt);
      }
      
      cei_L4_send_packet(dev, pkt, ret);
      free(pkt);
    }
  }
}

#if 0
       #include <sys/types.h>
       #include <sys/socket.h>
       #include <netinet/in.h>
       #include <arpa/inet.h>

static void cei_csc_loop(ProtDevice *dev)
{
  struct sockaddr_in addr;
  int ret;
  
  if(!dev || !cei_init(dev)) return;
  cei_configure(CEI, CEI_CSC, CEI_PROT_NONE);
  
  dev->fd = socket(PF_INET, SOCK_STREAM, 0);
  addr.sin_family = AF_INET;
  addr.sin_port = htons(4000);
  inet_aton("192.168.10.212", &addr.sin_addr);
  ret = connect(dev->fd, (struct sockaddr*)&addr, sizeof(struct sockaddr_in));
  
  while(1)
  {
    cei_L4_send_packet(dev, NULL, 0);
    while((ret = cei_L4_get_packet(dev)) == -2);
    if(ret < 0) return;
    if(CEI->L4_len <= 8) sleep(1);
  }
}
#endif

void _init()
{
  int i,j;
  
  for(i=0; i<(1 + n_ZS + n_ZI); i++) cei_zone_secret[i] = 0xff;
  
  support_log("Impostazione fasce CEI");
  /* Inizializza la struttura delle fasce orarie secondo il formato CEI */
  /* Sono 4 fasce orarie normali piu' 2 per la gestione di sblocco orario */
  for(i=0; i<n_FAS; i++)
  {
    free(FASR[i]);
    FASR[i] = malloc(4+2+32+1);
    if(FASR[i])
    {
      FASR[i][0] = (i<<2) + 0;
      FASR[i][1] = (i<<2) + 1;
      FASR[i][2] = (i<<2) + 2;
      FASR[i][3] = (i<<2) + 3;
      FASR[i][4] = (i<<1) + 28;
      FASR[i][5] = (i<<1) + 29;
      for(j=0; j<32; j++) FASR[i][6+j] = 42+j;
      FASR[i][38] = 0xff;
    }
  }
  
  printf("CEI-ABI (plugin, v.1): " __DATE__ " " __TIME__ "\n");
  prot_plugin_register("CEI", 0, NULL, NULL, (PthreadFunction)cei_ca_loop);
//  prot_plugin_register("CEI-CA", 0, NULL, NULL, (PthreadFunction)cei_ca_loop);
//  prot_plugin_register("CEI-CSC", 0, NULL, NULL, (PthreadFunction)cei_csc_loop);
}

