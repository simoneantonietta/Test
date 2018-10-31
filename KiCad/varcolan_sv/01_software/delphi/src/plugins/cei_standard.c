/*

programma utente:

centrale:
- liste classi 1 (centrale) e 8 (alimentazione)

*/

#include "protocol.h"
#include "codec.h"
#include "serial.h"
#include "support.h"
#include "user.h"
#include "database.h"
#include "master.h"
#include "delphi.h"
#include "cei_common.h"
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
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <termios.h>

extern int libuser_started;

#undef ALM_PERSIST
#define ALM_PERSIST	ALM_INIZIO

#define CEI_VERSIONE	2

//#define DEBUG

#define CEI	((CEI_data*)(dev->prot_data))
#define L7	L4+8
#define L7_len	L4_len-12

static pthread_mutex_t cei_evlistmutex = PTHREAD_MUTEX_INITIALIZER;
static unsigned char cei_allarme_alimentazione[n_SE] = {0, };
static unsigned char cei_attuatore_configurato[n_AT] = {0, };

#include "cei_common.c"

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

#define ALM_		0
#define CLS_		0
#define NOT_IMPL	-1

struct _cei_event {char tipoevento, transizione, classeelemento, numbytes;};

static struct _cei_event cei_event_table[MAX_NUM_EVENT] = {
{ALM_ALLARME, ALM_INIZIO, CLS_SENSORE, 2}, /* Allarme Sensore */
{ALM_ALLARME, ALM_FINE, CLS_SENSORE, 2}, /* Ripristino Sensore */
{ALM_GUASTO, ALM_INIZIO, CLS_UNITA_SATELLITE, 2}, /* Guasto Periferica */
{ALM_GUASTO, ALM_FINE, CLS_UNITA_SATELLITE, 2}, /* Fine Guasto Periferica */
{ALM_MANOMISSIONE, ALM_INIZIO, CLS_SENSORE, 2}, /* Manomissione Dispositivo */
{ALM_MANOMISSIONE, ALM_FINE, CLS_SENSORE, 2}, /* Fine Manomis Dispositivo */
{ALM_MANOMISSIONE, ALM_INIZIO, CLS_UNITA_SATELLITE, 2}, /* Manomissione Contenitore */
{ALM_MANOMISSIONE, ALM_FINE, CLS_UNITA_SATELLITE, 2}, /* Fine Manomis Contenitore */
{ALM_DISINSERITO, ALM_FINE, CLS_ZONA, 1}, /* Attivata zona */
{ALM_DISINSERITO, ALM_INIZIO, CLS_ZONA, 1}, /* Disattivata zona */
{ALM_DISINSERITO, ALM_PERSIST, CLS_ZONA, 1}, /* Attivazione impedita */
{ALM_DISABILITATO, ALM_FINE, CLS_SENSORE, 2}, /* Sensore In Servizio */
{ALM_DISABILITATO, ALM_INIZIO, CLS_SENSORE, 2}, /* Sensore Fuori Servizio */
{ALM_DISABILITATO, ALM_FINE, CLS_ATTUATORE, 2}, /* Attuatore In Servizio */
{ALM_DISABILITATO, ALM_INIZIO, CLS_ATTUATORE, 2}, /* Attuatore Fuori Servizio */
{ALM_, ALM_, CLS_, NOT_IMPL}, /* Ricezione codice errato */
{ALM_, ALM_, CLS_, NOT_IMPL}, /* Tipo ModuloMaster */
{ALM_, ALM_, CLS_, NOT_IMPL}, /* Periferica Incongruente */
{ALM_MANOMISSIONE, ALM_INIZIO, CLS_UNITA_SATELLITE, 2}, /* Periferica Manomessa */
{ALM_MANOMISSIONE, ALM_FINE, CLS_UNITA_SATELLITE, 2}, /* Periferica Ripristino */
{ALM_, ALM_, CLS_, NOT_IMPL}, /* Sospesa attivita linea */
{ALM_, ALM_, CLS_, NOT_IMPL}, /* Chiave Falsa */
{ALM_DISABILITATO, ALM_FINE, CLS_ORGANO_COMANDO, 2}, /* Sentinella on */
{ALM_DISABILITATO, ALM_INIZIO, CLS_ORGANO_COMANDO, 2}, /* Sentinella off */
{ALM_DISABILITATO, ALM_INIZIO, CLS_ORGANO_COMANDO, 2}, /* Sentinella off timeout */
{ALM_, ALM_, CLS_, NOT_IMPL}, /* No ModuloMasterTx */
{ALM_, ALM_, CLS_, NOT_IMPL}, /* ErrRx ModuloMaster */
{ALM_, ALM_, CLS_, NOT_IMPL}, /* Errore messaggio host */
/* --- RONDA --- */
{ALM_, ALM_, CLS_, NOT_IMPL}, /* Segnalazione evento */
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
{ALM_, ALM_, CLS_, NOT_IMPL}, /* ON telecomando */
{ALM_, ALM_, CLS_, NOT_IMPL}, /* Parametri taratura */
{ALM_, ALM_, CLS_, NOT_IMPL}, /* Stato prova */
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
{ALM_, ALM_, CLS_, NOT_IMPL}, /* ON attuatore */
{ALM_, ALM_, CLS_, NOT_IMPL}, /* Transitato Ident */
{ALM_, ALM_, CLS_, NOT_IMPL}, /* Entrato Ident */
{ALM_, ALM_, CLS_, NOT_IMPL}, /* Uscito Ident */
{ALM_, ALM_, CLS_, NOT_IMPL}, /* Codice valido */
{ALM_, ALM_, CLS_, NOT_IMPL}, /* Chiave valida */
//{ALM_ACCESSO_OPER, ALM_INIZIO, CLS_OPERATORE, 1}, /* Operatore */
{ALM_, ALM_, CLS_, NOT_IMPL}, /* Operatore */
{ALM_, ALM_, CLS_, NOT_IMPL}, /* Punzonatura */
{ALM_, ALM_, CLS_, NOT_IMPL}, /* Spegnimento */
{ALM_, ALM_, CLS_, NOT_IMPL}, /* Reset fumo */
{ALM_, ALM_, CLS_, NOT_IMPL}, /* Livello abilitato */
{ALM_, ALM_, CLS_, NOT_IMPL}, /* Variato segreto */
{ALM_, ALM_, CLS_, NOT_IMPL}, /* Conferma ricezione modem */
{ALM_, ALM_, CLS_, NOT_IMPL}, /* N telefonico 1 */
{ALM_, ALM_, CLS_, NOT_IMPL}, /* N telefonico 2 */
{ALM_, ALM_, CLS_, NOT_IMPL}, /* StatoBadge */
{ALM_, ALM_, CLS_, NOT_IMPL}, /* Evento Sensore */
//{ALM_STATO, ALM_PERSIST, CLS_SENSORE, 2}, /* Lista MU */
{ALM_, ALM_, CLS_, NOT_IMPL}, /* Lista MU */
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
{ALM_DISINSERITO, ALM_FINE, CLS_UNITA_SATELLITE, 2}, /* Governo_linee_in_servizio */
{ALM_DISINSERITO, ALM_INIZIO, CLS_UNITA_SATELLITE, 2}, /* Governo_linee_fuori_servizio */
};

static struct _cei_event cei_event_table_ex2[MAX_NUM_EVENT_EX2] = {
{ALM_TEST_ATTIVO, ALM_INIZIO, CLS_RAGGRUPPAMENTO, 1}, /* Inizio test attivo */
{ALM_TEST_POSITIVO, ALM_INIZIO, CLS_RAGGRUPPAMENTO, 1}, /* Fine test attivo (positivo) */
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
{ALM_, ALM_, CLS_, NOT_IMPL},
{ALM_, ALM_, CLS_, NOT_IMPL},
{ALM_, ALM_, CLS_, NOT_IMPL},
{ALM_, ALM_, CLS_, NOT_IMPL},
{ALM_, ALM_, CLS_, NOT_IMPL},
{ALM_, ALM_, CLS_, NOT_IMPL},
{ALM_, ALM_, CLS_, NOT_IMPL},
{ALM_, ALM_, CLS_, NOT_IMPL},
{ALM_OROLOGIO_FUORI_SYN, ALM_INIZIO, CLS_UNITA_CENTRALE, 0},
{ALM_OROLOGIO_FUORI_SYN, ALM_FINE, CLS_UNITA_CENTRALE, 0},
{ALM_MANCARETE, ALM_INIZIO, CLS_UNITA_CENTRALE, 0},
{ALM_MANCARETE, ALM_FINE, CLS_UNITA_CENTRALE, 0},
{ALM_BATT_SCARICA, ALM_INIZIO, CLS_UNITA_CENTRALE, 0},
{ALM_BATT_SCARICA, ALM_FINE, CLS_UNITA_CENTRALE, 0},
{ALM_MANUTENZIONE, ALM_INIZIO, CLS_UNITA_CENTRALE, 0},
{ALM_MANUTENZIONE, ALM_FINE, CLS_UNITA_CENTRALE, 0},
{ALM_TEST_NEGATIVO, ALM_INIZIO, CLS_RAGGRUPPAMENTO, 1}, /* Fine test attivo (negativo) */
{ALM_STRAORDINARIO, ALM_INIZIO, CLS_ZONA, 0},
{ALM_STRAORDINARIO, ALM_FINE, CLS_ZONA, 0},
{ALM_INS_ANTICIPATO, ALM_INIZIO, CLS_UNITA_CENTRALE, 0},
{ALM_, ALM_, CLS_, NOT_IMPL},
{ALM_STATO, ALM_PERSIST, CLS_UNITA_CENTRALE, 0},	/* Stato centrale (per allineamento) */
{ALM_ATTIVAZIONE, ALM_INIZIO, CLS_ATTUATORE, 2}, /* On attuatore */
{ALM_ATTIVAZIONE, ALM_FINE, CLS_ATTUATORE, 2}, /* Off attuatore */
};

static int delphi_tastiera(int p)
{
  if(((MMPresente[p>>5] == 0) &&
      ((TipoPeriferica[p] == 7) ||
       (TipoPeriferica[p] == 10) ||
       (TipoPeriferica[p] == 6))) ||
      ((MMPresente[p>>5] == 2) &&
      ((TipoPeriferica[p] == 4) ||
       (TipoPeriferica[p] == 5) ||
       (TipoPeriferica[p] == 6) ||
       (TipoPeriferica[p] == 7))) ||
      ((MMPresente[p>>5] == 5) &&
      ((TipoPeriferica[p] == 7))) ||
      ((MMPresente[p>>5] == 4) &&
      ((TipoPeriferica[p] == 7)))
     )
    return 1;
  return 0;
}

static void cei_event_set_alarm(CEI_allarme *alarm, Event *ev, int tempoaccadimento)
{
  int elemento;
  
  alarm->header.formato = 1;	// Formato ABI v.2
  alarm->header.versione = CEI_VERSIONE;
  
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
  
  if(alarm->classeelemento == CLS_UNITA_CENTRALE) alarm->areaprovenienza = 64;
  
  switch(cei_event_table[ev->Event[8]-EV_OFFSET].numbytes)
  {
    case 1:
      alarm->numeroelementomsb = 0;
      alarm->numeroelementolsb = ev->Event[9];
      break;
    case 2:
      elemento = ev->Event[9]*256+ev->Event[10];
      
      alarm->numeroelementomsb = ev->Event[9];
      if((alarm->classeelemento == CLS_UNITA_SATELLITE) && !elemento)
      {
        alarm->classeelemento = CLS_UNITA_CENTRALE;
        alarm->numeroelementolsb = 1;
      }
      else if((alarm->classeelemento == CLS_UNITA_SATELLITE) && delphi_tastiera(elemento))
      {
        alarm->classeelemento = CLS_ORGANO_COMANDO;
        alarm->numeroelementolsb = ev->Event[10];
      }
      else
        alarm->numeroelementolsb = ev->Event[10];
      
      if((alarm->classeelemento == CLS_SENSORE) &&
         cei_allarme_alimentazione[elemento])
      {
        if(alarm->tipoevento == ALM_ALLARME)
          alarm->tipoevento = cei_allarme_alimentazione[elemento] & 0x7f;
        if(cei_allarme_alimentazione[elemento] & 0x80)
        {
          alarm->classeelemento = CLS_UNITA_CENTRALE;
          alarm->numeroelementolsb = 1;
        }
        else
          alarm->classeelemento = CLS_GRP_ALIMENTAZIONE;
      }
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
  alarm->header.versione = CEI_VERSIONE;
  
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
    
  if(alarm->classeelemento == CLS_UNITA_CENTRALE)
    alarm->areaprovenienza = 64;
  
  switch(cei_event_table_ex2[ev->Event[10]].numbytes)
  {
    case 1:
      alarm->numeroelementomsb = 0;
      alarm->numeroelementolsb = ev->Event[11];
      break;
    case 2:
      alarm->numeroelementomsb = ev->Event[12];
      alarm->numeroelementolsb = ev->Event[11];
      break;
    default:
      alarm->numeroelementomsb = 0;
      if(alarm->classeelemento == CLS_UNITA_CENTRALE)
        alarm->numeroelementolsb = 1;
      else
        alarm->numeroelementolsb = 0;
      break;
  }
}

static int cei_status_centrale_lut[] = {ALM_MANUTENZIONE, ALM_MANCARETE,
	ALM_BATT_SCARICA, ALM_OROLOGIO_FUORI_SYN, ALM_GUASTO, ALM_MANOMISSIONE};

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
  
  *pkt = NULL;
  
  if(!ev || !ev->Len) return 0;
  
  if(((ev->Event[8] < EV_OFFSET) || (ev->Event[8] >= (EV_OFFSET + MAX_NUM_EVENT)) || 
     (cei_event_table[ev->Event[8]-EV_OFFSET].numbytes == NOT_IMPL)) && (ev->Event[8] != Evento_Esteso2))
    return -1;
  
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
    if(cei_event_table_ex2[ev->Event[10]].tipoevento == ALM_STATO)
    {
      len = sizeof(CEI_stato_header) + sizeof(CEI_stato) * 6;
      header = calloc(1, len);
      if(!header) return -1;
      
      header->header.formato = 1;	// Formato ABI v.2
      header->header.versione = CEI_VERSIONE;
      header->header.processo = 2;	// trasferimento stati per allineamento
      header->header.numeromessaggio = CEI->L4_pktnum[1]++;
      header->header.tipoinfo = 2;	// allineamento
      header->tempoaccadimento = tempoaccadimento;
      
      stato = (void*)header + sizeof(CEI_stato_header);
      stato->transizione = 1;
      stato++;
      
      for(i=0; i<6; i++)
      {
        stato->transizione = (ev->Event[11] & (1<<i))?ALM_PERSIST:ALM_FINE;
        stato->tipoevento = cei_status_centrale_lut[i];
        stato->classeelemento = cei_event_table_ex2[ev->Event[10]].classeelemento;
        stato->numeroelementolsb = 1;
        stato++;
      }
      
      *pkt = (unsigned char*)header;
    }
    else
    {
      switch(ev->Event[9])
      {
        case Ex2_Delphi:
          len = sizeof(CEI_allarme);
          alarm = calloc(1, len);
          if(!alarm) return -1;
  
          cei_event_set_alarm_ex2(alarm, ev, tempoaccadimento);
          if((alarm->numeroelementolsb || alarm->numeroelementomsb) &&
             /* filtra gli eventi per gli attuatori non configurati */
             ((alarm->classeelemento != CLS_ATTUATORE) ||
              cei_attuatore_configurato[alarm->numeroelementomsb*256+alarm->numeroelementolsb]))
          {
            alarm->header.numeromessaggio = CEI->L4_pktnum[0]++;
            *pkt = (unsigned char*)alarm;
          }
          else
          {
            free(alarm);
            return -1;
          }
          break;
        default:
          return -1;
          break;
      }
    }
  }
  else if(cei_event_table[ev->Event[8]-EV_OFFSET].tipoevento == ALM_STATO)
  {
    if(ev->Event[8] == FineInvioMemoria)
      num = 1;
//    else if(ev->Event[8] == Lista_MU)
//      num = 24;
    else if(ev->Event[8] == Lista_zone)
      num = 8;
    else if(ev->Event[8] == Lista_attuatori)
      num = 16;
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
    header->header.versione = CEI_VERSIONE;
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
#if 0
      else if(ev->Event[8] == Lista_MU)
      {
        switch(i % 3)
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
          default:	// non puo' capitare
            bitmask = 0;
            tipoevento = 0;
            break;
        }
        stato->transizione = (ev->Event[11 + (i/3)] & bitmask)?ALM_PERSIST:ALM_FINE;
        stato->tipoevento = tipoevento;
        stato->classeelemento = cei_event_table[ev->Event[8]-EV_OFFSET].classeelemento;
        stato->numeroelementomsb = ev->Event[9];
        stato->numeroelementolsb = ev->Event[10];
      }
#endif
      else
      {
        if(ev->Event[8] == Lista_zone)
        {
#if 0
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
            default:	// non puo' capitare
              bitmask = 0;
              tipoevento = 0;
              break;
          }
          if((i%4) == 3)
            stato->transizione = (ev->Event[10 + (i/4)] & bitmask)?ALM_FINE:ALM_PERSIST;
          else
            stato->transizione = (ev->Event[10 + (i/4)] & bitmask)?ALM_PERSIST:ALM_FINE;
          stato->tipoevento = tipoevento;
#else
          stato->transizione = (ev->Event[10+i] & bitOOS)?ALM_FINE:ALM_PERSIST;
          stato->tipoevento = ALM_DISINSERITO;
#endif
          stato->classeelemento = cei_event_table[ev->Event[8]-EV_OFFSET].classeelemento;
          stato->numeroelementolsb = ev->Event[9];
        }
        else if(ev->Event[8] == Lista_sensori)
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
              tipoevento = ALM_DISABILITATO;
              break;
            default:	// non puo' capitare
              bitmask = 0;
              tipoevento = 0;
              break;
          }
          if(!cei_allarme_alimentazione[ev->Event[9]*256+ev->Event[10]+(i/4)])
            stato->transizione = (ev->Event[11 + (i/4)] & bitmask)?ALM_PERSIST:ALM_FINE;
          else
            stato->transizione = ALM_FINE;
          stato->tipoevento = tipoevento;
          stato->classeelemento = cei_event_table[ev->Event[8]-EV_OFFSET].classeelemento;
          stato->numeroelementomsb = ev->Event[9];
          stato->numeroelementolsb = ev->Event[10];
        }
        else	// attuatori
        {
          switch(i % 2)
          {
            case 0:
              bitmask = bitON;
              tipoevento = ALM_ATTIVAZIONE;
              break;
            case 1:
              bitmask = bitOOS;
              tipoevento = ALM_DISABILITATO;
              break;
            default:	// non puo' capitare
              bitmask = 0;
              tipoevento = 0;
              break;
          }
          stato->transizione = (ev->Event[11 + (i/2)] & bitmask)?ALM_PERSIST:ALM_FINE;
          stato->tipoevento = tipoevento;
          stato->classeelemento = cei_event_table[ev->Event[8]-EV_OFFSET].classeelemento;
          stato->numeroelementomsb = ev->Event[9];
          stato->numeroelementolsb = ev->Event[10];
        }
      }
      
      if(num > 1)
      {
        stato->numeroelementolsb += i / (num/8);
        /* Elimino dalla lista gli elementi con indice 0 */
        if(!stato->numeroelementomsb && !stato->numeroelementolsb)
        {
          memset(stato, 0, sizeof(CEI_stato));
          stato--;
          len -= sizeof(CEI_stato);
        }
        /* Elimino dalla lista zone le zone non configurate */
        else if((stato->classeelemento == CLS_ZONA) &&
           (!SE_ZONA[stato->numeroelementolsb] || (SE_ZONA[stato->numeroelementolsb][0] == -1)))
        {
          memset(stato, 0, sizeof(CEI_stato));
          stato--;
          len -= sizeof(CEI_stato);
        }
        /* Elimino dalla lista sensori i sensori non configurati o configurati come alimentazione */
        else if(stato->classeelemento == CLS_SENSORE)
        {
          int elemento;
          elemento = stato->numeroelementomsb * 256 + stato->numeroelementolsb;
          if(cei_allarme_alimentazione[elemento] || (Zona_SE[elemento] == 0xff))
          {
            memset(stato, 0, sizeof(CEI_stato));
            stato--;
            len -= sizeof(CEI_stato);
          }
        }
        /* Elimino dalla lista attuatori gli attuatori non configurati */
        else if(stato->classeelemento == CLS_ATTUATORE)
        {
          int elemento;
          elemento = stato->numeroelementomsb * 256 + stato->numeroelementolsb;
          if(!cei_attuatore_configurato[elemento])
          {
            memset(stato, 0, sizeof(CEI_stato));
            stato--;
            len -= sizeof(CEI_stato);
          }
        }
      }
      stato++;
    }
    
    if(len == sizeof(CEI_stato_header))
    {
      len = 0;
      *pkt = NULL;
      free(header);
      return -1;
    }
    else
      *pkt = (unsigned char*)header;
  }
  else
  {
    len = sizeof(CEI_allarme);
    alarm = calloc(1, len);
    if(!alarm) return -1;

    cei_event_set_alarm(alarm, ev, tempoaccadimento);
    if((alarm->numeroelementolsb || alarm->numeroelementomsb) &&
       /* filtra gli eventi per gli attuatori non configurati */
       ((alarm->classeelemento != CLS_ATTUATORE) ||
        cei_attuatore_configurato[alarm->numeroelementomsb*256+alarm->numeroelementolsb]))
    {
      alarm->header.numeromessaggio = CEI->L4_pktnum[0]++;
      *pkt = (unsigned char*)alarm;
    }
    else
    {
      free(alarm);
      return -1;
    }
  }
  
  return len;
}

static CEI_stato_header* cei_allineamento_header(ProtDevice *dev, int stati, int tempoaccadimento, int *len)
{
  CEI_stato_header *header;
  
  *len = sizeof(CEI_stato_header) + sizeof(CEI_stato) * stati;
  header = calloc(1, *len);
  if(!header) return NULL;
  
  header->header.formato = 1;	// Formato ABI v.2
  header->header.versione = CEI_VERSIONE;
  header->header.processo = 2;	// trasferimento stati per allineamento
  header->header.numeromessaggio = CEI->L4_pktnum[1]++;
  header->header.tipoinfo = 2;	// allineamento
  header->tempoaccadimento = tempoaccadimento;
  
  return header;
}

static void cei_allineamento_periferiche(ProtDevice *dev)
{
  int p, i, len;
  CEI_stato_header *header;
  CEI_stato *stato;
  int tempoaccadimento;
  cei_event_list *tevlist, *tail;
  
  tempoaccadimento = cei_tempoaccadimento();
  
  header = cei_allineamento_header(dev, 17, tempoaccadimento, &len);
  if(!header) return;
  
  stato = (void*)header + sizeof(CEI_stato_header);
  stato->transizione = 1;
  stato++;
  
  i = 0;
  for(p=1; p<(MM_NUM*32); p++)
  {
    if((TipoPeriferica[p] != 0xff) && !delphi_tastiera(p))
    {
      stato->transizione = (StatoPeriferica[p>>3]&(1<<(p&7)))?ALM_FINE:ALM_PERSIST;
      stato->tipoevento = ALM_MANOMISSIONE;
      stato->classeelemento = CLS_UNITA_SATELLITE;
      stato->numeroelementomsb = p/256;
      stato->numeroelementolsb = p;
      stato++;
      stato->transizione = (SE[p*8+2]&bitGenFailure)?ALM_PERSIST:ALM_FINE;
      stato->tipoevento = ALM_GUASTO;
      stato->classeelemento = CLS_UNITA_SATELLITE;
      stato->numeroelementomsb = p/256;
      stato->numeroelementolsb = p;
      stato++;
      i++;
      if(i == 8)
      {
        pthread_mutex_lock(&cei_evlistmutex);
        tevlist = malloc(sizeof(cei_event_list));
        if(!tevlist)
        {
          pthread_mutex_unlock(&cei_evlistmutex);
          return;
        }
        if(!CEI->evlist)
        {
          CEI->evlist = tevlist;
        }
        else
        {
          tail = CEI->evlist;
          while(tail->next) tail = tail->next;
          tail->next = tevlist;
        }
        tevlist->event = (void*)header;
        tevlist->len = len;
        tevlist->next = NULL;
        pthread_mutex_unlock(&cei_evlistmutex);
        
        header = cei_allineamento_header(dev, 16, tempoaccadimento, &len);
        stato = (void*)header + sizeof(CEI_stato_header);
        
        i = 0;      
      }
    }
  }
  
  /* Occorre accodare il fine lista ed inviare l'evento attuale opportunamente ridimensionato */
  len -= ((8-i)*2 - 1)*sizeof(CEI_stato);
  
  pthread_mutex_lock(&cei_evlistmutex);
  tevlist = malloc(sizeof(cei_event_list));
  if(!tevlist)
  {
    pthread_mutex_unlock(&cei_evlistmutex);
    return;
  }
  if(!CEI->evlist)
  {
    CEI->evlist = tevlist;
  }
  else
  {
    tail = CEI->evlist;
    while(tail->next) tail = tail->next;
    tail->next = tevlist;
  }
  tevlist->event = (void*)header;
  tevlist->len = len;
  tevlist->next = NULL;
  pthread_mutex_unlock(&cei_evlistmutex);
}

static void cei_allineamento_tastiere(ProtDevice *dev)
{
  int p, i, len;
  CEI_stato_header *header;
  CEI_stato *stato;
  int tempoaccadimento;
  cei_event_list *tevlist, *tail;
  
  tempoaccadimento = cei_tempoaccadimento();
  
  header = cei_allineamento_header(dev, 25, tempoaccadimento, &len);
  if(!header) return;
  
  stato = (void*)header + sizeof(CEI_stato_header);
  stato->transizione = 1;
  stato++;
  
  i = 0;
  for(p=0; p<(MM_NUM*32); p++)
  {
    if(delphi_tastiera(p))
    {
      stato->transizione = (StatoPeriferica[p>>3]&(1<<(p&7)))?ALM_FINE:ALM_PERSIST;
      stato->tipoevento = ALM_MANOMISSIONE;
      stato->classeelemento = CLS_ORGANO_COMANDO;
      stato->numeroelementomsb = p/256;
      stato->numeroelementolsb = p;
      stato++;
      stato->transizione = (SE[p*8+2]&bitGenFailure)?ALM_PERSIST:ALM_FINE;
      stato->tipoevento = ALM_GUASTO;
      stato->classeelemento = CLS_ORGANO_COMANDO;
      stato->numeroelementomsb = p/256;
      stato->numeroelementolsb = p;
      stato++;
      stato->transizione = (SE[p*8+1]&bitAlarm)?ALM_FINE:ALM_PERSIST;
      stato->tipoevento = ALM_DISABILITATO;
      stato->classeelemento = CLS_ORGANO_COMANDO;
      stato->numeroelementomsb = p/256;
      stato->numeroelementolsb = p;
      stato++;
      i++;
      if(i == 8)
      {
        pthread_mutex_lock(&cei_evlistmutex);
        tevlist = malloc(sizeof(cei_event_list));
        if(!tevlist)
        {
          pthread_mutex_unlock(&cei_evlistmutex);
          return;
        }
        if(!CEI->evlist)
        {
          CEI->evlist = tevlist;
        }
        else
        {
          tail = CEI->evlist;
          while(tail->next) tail = tail->next;
          tail->next = tevlist;
        }
        tevlist->event = (void*)header;
        tevlist->len = len;
        tevlist->next = NULL;
        pthread_mutex_unlock(&cei_evlistmutex);
        
        header = cei_allineamento_header(dev, 16, tempoaccadimento, &len);
        stato = (void*)header + sizeof(CEI_stato_header);
        
        i = 0;      
      }
    }
  }
  
  /* Occorre accodare il fine lista ed inviare l'evento attuale opportunamente ridimensionato */
  len -= ((8-i)*3 - 1)*sizeof(CEI_stato);
  
  pthread_mutex_lock(&cei_evlistmutex);
  tevlist = malloc(sizeof(cei_event_list));
  if(!tevlist)
  {
    pthread_mutex_unlock(&cei_evlistmutex);
    return;
  }
  if(!CEI->evlist)
  {
    CEI->evlist = tevlist;
  }
  else
  {
    tail = CEI->evlist;
    while(tail->next) tail = tail->next;
    tail->next = tevlist;
  }
  tevlist->event = (void*)header;
  tevlist->len = len;
  tevlist->next = NULL;
  pthread_mutex_unlock(&cei_evlistmutex);
}

static void cei_allineamento_alimentazioni(ProtDevice *dev)
{
  int p, i, len;
  CEI_stato_header *header;
  CEI_stato *stato;
  int tempoaccadimento;
  cei_event_list *tevlist, *tail;
  
  tempoaccadimento = cei_tempoaccadimento();
  
  header = cei_allineamento_header(dev, 26, tempoaccadimento, &len);
  if(!header) return;
  
  stato = (void*)header + sizeof(CEI_stato_header);
  stato->transizione = 1;
  stato++;
  
  i = 0;
  for(p=1; p<n_SE; p++)
  {
    /* Solo i sensori di alimentazione non di centrale. */
    if(cei_allarme_alimentazione[p] && !(cei_allarme_alimentazione[p] & 0x80))
    {
      stato->transizione = (SE[p] & bitGestSabotage)?ALM_PERSIST:ALM_FINE;
      stato->tipoevento = ALM_MANOMISSIONE;
      stato->classeelemento = CLS_GRP_ALIMENTAZIONE;
      stato->numeroelementomsb = p/256;
      stato->numeroelementolsb = p;
      stato++;
      stato->transizione = (SE[p] & bitGestFailure)?ALM_PERSIST:ALM_FINE;
      stato->tipoevento = ALM_GUASTO;
      stato->classeelemento = CLS_GRP_ALIMENTAZIONE;
      stato->numeroelementomsb = p/256;
      stato->numeroelementolsb = p;
      stato++;
      stato->transizione = (SE[p] & bitOOS)?ALM_PERSIST:ALM_FINE;
      stato->tipoevento = ALM_DISABILITATO;
      stato->classeelemento = CLS_GRP_ALIMENTAZIONE;
      stato->numeroelementomsb = p/256;
      stato->numeroelementolsb = p;
      stato++;
      if(cei_allarme_alimentazione[p] == ALM_MANCARETE)
        stato->transizione = (SE[p] & bitGestAlarm)?ALM_PERSIST:ALM_FINE;
      else
        stato->transizione = ALM_FINE;
      stato->tipoevento = ALM_MANCARETE;
      stato->classeelemento = CLS_GRP_ALIMENTAZIONE;
      stato->numeroelementomsb = p/256;
      stato->numeroelementolsb = p;
      stato++;
      if(cei_allarme_alimentazione[p] == ALM_BATT_SCARICA)
        stato->transizione = (SE[p] & bitGestAlarm)?ALM_PERSIST:ALM_FINE;
      else
        stato->transizione = ALM_FINE;
      stato->tipoevento = ALM_BATT_SCARICA;
      stato->classeelemento = CLS_GRP_ALIMENTAZIONE;
      stato->numeroelementomsb = p/256;
      stato->numeroelementolsb = p;
      stato++;
      i++;
      if(i == 5)
      {
        pthread_mutex_lock(&cei_evlistmutex);
        tevlist = malloc(sizeof(cei_event_list));
        if(!tevlist)
        {
          pthread_mutex_unlock(&cei_evlistmutex);
          return;
        }
        if(!CEI->evlist)
        {
          CEI->evlist = tevlist;
        }
        else
        {
          tail = CEI->evlist;
          while(tail->next) tail = tail->next;
          tail->next = tevlist;
        }
        tevlist->event = (void*)header;
        tevlist->len = len;
        tevlist->next = NULL;
        pthread_mutex_unlock(&cei_evlistmutex);
        
        header = cei_allineamento_header(dev, 16, tempoaccadimento, &len);
        stato = (void*)header + sizeof(CEI_stato_header);
        
        i = 0;      
      }
    }
  }
  
  /* Occorre accodare il fine lista ed inviare l'evento attuale opportunamente ridimensionato */
  len -= ((5-i)*5 - 1)*sizeof(CEI_stato);
  
  pthread_mutex_lock(&cei_evlistmutex);
  tevlist = malloc(sizeof(cei_event_list));
  if(!tevlist)
  {
    pthread_mutex_unlock(&cei_evlistmutex);
    return;
  }
  if(!CEI->evlist)
  {
    CEI->evlist = tevlist;
  }
  else
  {
    tail = CEI->evlist;
    while(tail->next) tail = tail->next;
    tail->next = tevlist;
  }
  tevlist->event = (void*)header;
  tevlist->len = len;
  tevlist->next = NULL;
  pthread_mutex_unlock(&cei_evlistmutex);
}

static int cei_L4_parse_command(ProtDevice *dev, int abilitato)
{
  int i, len, ret = -1;
  int elemento;
  CEI_risposta_telecomando risp;
  time_t t, dt;
  struct tm *datehour;
  char cmd[24], cmd2[16];
  
  cmd[0] = 0;
  len = 1;
  
  memset(&risp, 0, sizeof(CEI_risposta_telecomando));
  
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
              else if(((CEI_telecomando*)(CEI->L4))->classeelemento == CLS_ATTUATORE)
              {
                if(!elemento)
                {
                  for(i=0; i<n_AT; i++)
                    if(cei_attuatore_configurato[i])
                    {
                      sprintf(cmd, "/570%04d", i);
                      len = 8;
                      codec_parse_cmd(cmd, len, dev);
                    }
                  cmd[0] = 0;
                  ret = 1;
                }
                else
                {
                  if(cei_attuatore_configurato[elemento])
                  {
                    sprintf(cmd, "/570%04d", elemento);
                    len = 8;
                  }
                }
              }
              break;
            case 2:
              if((((CEI_telecomando*)(CEI->L4))->classeelemento == CLS_ZONA) &&
                 (elemento < (1 + n_ZS + n_ZI)))
              {
                sprintf(cmd, "J%03d", elemento);
                len = 4;
              }
              else if(((CEI_telecomando*)(CEI->L4))->classeelemento == CLS_ATTUATORE)
              {
                if(!elemento)
                {
                  for(i=0; i<n_AT; i++)
                    if(cei_attuatore_configurato[i])
                    {
                      sprintf(cmd, "/572%04d", i);
                      len = 8;
                      codec_parse_cmd(cmd, len, dev);
                    }
                  cmd[0] = 0;
                  ret = 1;
                }
                else
                {
                  if(cei_attuatore_configurato[elemento])
                  {
                    sprintf(cmd, "/572%04d", elemento);
                    len = 8;
                  }
                }
              }
              break;
            case 3:
              if(((CEI_telecomando*)(CEI->L4))->classeelemento == CLS_SENSORE)
              {
                sprintf(cmd, "D%04d", elemento);
                len = 5;
              }
              else if(((CEI_telecomando*)(CEI->L4))->classeelemento == CLS_GRP_ALIMENTAZIONE)
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
                sprintf(cmd, "C%04d", elemento);
                len = 5;
              }
              else if(((CEI_telecomando*)(CEI->L4))->classeelemento == CLS_GRP_ALIMENTAZIONE)
              {
                sprintf(cmd, "C%04d", elemento);
                len = 5;
              }
              else if(((CEI_telecomando*)(CEI->L4))->classeelemento == CLS_ATTUATORE)
              {
                sprintf(cmd, "A%04d", elemento);
                len = 5;
              }
              break;
            case 5:
              if(((CEI_telecomando*)(CEI->L4))->classeelemento == CLS_UNITA_CENTRALE)
              {
                memcpy(cmd, "/533", 4);
                *(short*)(cmd+4) = 9999;
                len = 6;
              }
              break;
            case 6:
              if(((CEI_telecomando*)(CEI->L4))->classeelemento == CLS_RAGGRUPPAMENTO)
              {
                // Test automatico
/*
Il numero elemento rappresenta il gruppo:
0 - test generale
1 - microfoni fissi
2 - microfoni mobili
3 - sensori volumetrici
*/
                if(elemento < 32)
                {
                  sprintf(cmd, "K%03d", 512 + elemento);
                  len = 5;
                }
              }
              break;
            case 15:
              if(((CEI_telecomando*)(CEI->L4))->classeelemento == CLS_UNITA_CENTRALE)
              {
                support_log("Invio Restart CEI");
#ifdef __CRIS__
                delphi_restart();
#endif
                ret = 1;
              }
              break;
            case 18:
              if(((CEI_telecomando*)(CEI->L4))->classeelemento == CLS_UNITA_CENTRALE)
              {
                memcpy(cmd, "K590", 4);
                len = 4;
              }
              break;
            case 19:
              if(((CEI_telecomando*)(CEI->L4))->classeelemento == CLS_UNITA_CENTRALE)
              {
                memcpy(cmd, "K589", 4);
                len = 4;
              }
              break;
            case 20:
              if(((CEI_telecomando*)(CEI->L4))->classeelemento == CLS_ATTUATORE)
              {
                cmd[0] = 'H';
              }
              break;
            case 21:
#if 0
              if(((CEI_telecomando*)(CEI->L4))->classeelemento == CLS_SENSORE)
              {
                // "/530ww" -> accettazione allarme
                // "/531ww" -> accettazione manomissione
                // "/532ww" -> accettazione guasto
                // "/533ww (=9999)" -> accettazione completa tutti
                // "/533ww" -> accettazione completa
//                memcpy(cmd, "/389", 4);
//                len = 4;
                memcpy(cmd, "/533", 4);
                if(elemento)
                  *(unsigned short*)(cmd+4) = elemento;
                else
                  *(unsigned short*)(cmd+4) = 9999;
                len = 6;
              }
              else if(((CEI_telecomando*)(CEI->L4))->classeelemento == CLS_GRP_ALIMENTAZIONE)
              {
                memcpy(cmd, "/533", 4);
                if(elemento)
                  *(unsigned short*)(cmd+4) = elemento;
                else
                  *(unsigned short*)(cmd+4) = 9999;
                len = 6;
              }
#else
              /* A differenza di Sepa e Proteus e forse Keeper, in questo caso
                 si vuole che l'andamento dell'allarme segua quello dell'ingresso,
                 quindi in stile Banca d'Italia.
                 L'accettazione viene fatta in modo automatico dal supervisore
                 a fronte della segnalazione, e se l'allarme fosse ancora in corso
                 lo ripresenterei dando inizio ad un ping pong di eventi e comandi. */
              if(((CEI_telecomando*)(CEI->L4))->classeelemento == CLS_SENSORE)
              {
                // "/530ww" -> accettazione allarme
                // "/531ww" -> accettazione manomissione
                // "/532ww" -> accettazione guasto
                // "/533ww (=9999)" -> accettazione completa tutti
                // "/533ww" -> accettazione completa
                if(elemento)
                {
                  *(unsigned short*)(cmd+4) = elemento;
                  memcpy(cmd, "/530", 4);
                  codec_parse_cmd(cmd, 6, dev);
                  memcpy(cmd, "/531", 4);
                  codec_parse_cmd(cmd, 6, dev);
                  memcpy(cmd, "/532", 4);
                  codec_parse_cmd(cmd, 6, dev);
                }
                else
                {
                  memcpy(cmd, "/530", 4);
                  for(elemento=0; elemento<n_SE; elemento++)
                  {
                    *(unsigned short*)(cmd+4) = elemento;
                    cmd[3] = '0';
                    codec_parse_cmd(cmd, 6, dev);
                    cmd[3] = '1';
                    codec_parse_cmd(cmd, 6, dev);
                    cmd[3] = '2';
                    codec_parse_cmd(cmd, 6, dev);
                  }
                }
                cmd[0] = 0;
                ret = 1;
              }
              else if(((CEI_telecomando*)(CEI->L4))->classeelemento == CLS_GRP_ALIMENTAZIONE)
              {
                if(elemento)
                {
                  *(unsigned short*)(cmd+4) = elemento;
                  memcpy(cmd, "/530", 4);
                  codec_parse_cmd(cmd, 6, dev);
                  memcpy(cmd, "/531", 4);
                  codec_parse_cmd(cmd, 6, dev);
                  memcpy(cmd, "/532", 4);
                  codec_parse_cmd(cmd, 6, dev);
                }
                else
                {
                  memcpy(cmd, "/530", 4);
                  for(elemento=0; elemento<n_SE; elemento++)
                  {
                    *(unsigned short*)(cmd+4) = elemento;
                    cmd[3] = '0';
                    codec_parse_cmd(cmd, 6, dev);
                    cmd[3] = '1';
                    codec_parse_cmd(cmd, 6, dev);
                    cmd[3] = '2';
                    codec_parse_cmd(cmd, 6, dev);
                  }
                }
                cmd[0] = 0;
                ret = 1;
              }
#endif
              break;
            default:
              break;
          }
          if(cmd[0]) ret = codec_parse_cmd(cmd, len, dev);
        }
        else
          ret = -2;
        
        risp.header.formato = 1;	// Formato ABI v.2
        risp.header.versione = CEI_VERSIONE;
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
        if(ret && !CEI->skvalid) ((char*)&risp)[4] |= CEI_L4_FLAG_S;
        cei_L4_send_packet(dev, &risp, sizeof(CEI_risposta_telecomando));
      }
      break;
    case 2: // Trasferimento stati per allineamento CA/CSC
      for(i=0; i<(CEI->L4_len-10); i+=5)
      {
        cmd[0] = 0;
        if(((CEI_header_ex*)(CEI->L4+i))->tipoinfo == 1)	// Richiesta allineamento
        {
          if(abilitato)
          {
            switch(((CEI_richiesta_allineamento*)(CEI->L4+i))->comando)
            {
              case 1:
                if(((CEI_telecomando*)(CEI->L4+i))->classeelemento == CLS_SENSORE)
                  cmd[0] = 'd';
/*
                {
                  memcpy(cmd, "/380", 4);
                  len = 4;
                }
*/
                else if(((CEI_telecomando*)(CEI->L4+i))->classeelemento == CLS_ATTUATORE)
                  cmd[0] = 'e';
                else if(((CEI_telecomando*)(CEI->L4+i))->classeelemento == CLS_ZONA)
                  cmd[0] = 'c';
                else if(((CEI_telecomando*)(CEI->L4+i))->classeelemento == CLS_UNITA_CENTRALE)
                {
                  memcpy(cmd, "K588", 4);
                  len = 4;
                }
                else if(((CEI_telecomando*)(CEI->L4+i))->classeelemento == CLS_UNITA_SATELLITE)
                {
                  cei_allineamento_periferiche(dev);
                }
                else if(((CEI_telecomando*)(CEI->L4+i))->classeelemento == CLS_ORGANO_COMANDO)
                {
                  cei_allineamento_tastiere(dev);
                }
                else if(((CEI_telecomando*)(CEI->L4+i))->classeelemento == CLS_GRP_ALIMENTAZIONE)
                {
                  cei_allineamento_alimentazioni(dev);
                }
                break;
            }
            if(cmd[0])
              ret = codec_parse_cmd(cmd, len, dev);
          }
        }
      }
      cei_L4_send_packet(dev, NULL, 0);
      break;
    case 3: // Trasferimento parametri di gestione impianto
      switch(((CEI_header_ex*)(CEI->L4))->tipoinfo)
      {
        case 1:	// Trasmissione data e ora        
          t = time(NULL);
          datehour = localtime(&t);
          datehour->tm_isdst = 0;
          dt = mktime(datehour)-timezone;
          
          t = bswap_32(((CEI_trasmissione_dataora*)(CEI->L4))->tempo) + BASESECONDS - timezone;
          
          dt -= t;
          if(dt < 0) dt = -dt;
//printf("Disallineamento: %ds (ora desiderata %02d:%02d:%02d)\n", dt, datehour->tm_hour, datehour->tm_min, datehour->tm_sec);
          if(dt > 300)
          {
            /* On ME[594] */
            codec_parse_cmd("K594", 4, dev);
          }
          else
          {
            datehour = gmtime(&t);
            
            /* On ME[593] */
            codec_parse_cmd("K593", 4, dev);
            
            sprintf(cmd, "U%d%02d%02d%02d", datehour->tm_wday, datehour->tm_mday, datehour->tm_mon+1, datehour->tm_year-100);
            sprintf(cmd2, "W%02d%02d%02d", datehour->tm_hour, datehour->tm_min, datehour->tm_sec);
            codec_parse_cmd(cmd, 8, dev);
            ret = codec_parse_cmd(cmd2, 7, dev);
          }
          cei_L4_send_packet(dev, NULL, 0);
          break;
      }
      break;
    case 4: // Trasferimento parametri di configurazione
      switch(((CEI_header_ex*)(CEI->L4))->tipoinfo)
      {
        case 1:	// richiesta parametri
          break;
      }
      break;
  }
  
  return ret;
}

static int cei_init(ProtDevice *dev)
{
  if(dev->prot_data) return 1;
  
  dev->prot_data = calloc(1, sizeof(CEI_data));
  if(!dev->prot_data) return 0;
  
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
  int num, ret = 0;
  unsigned char *pkt;
  
  /* Attiva le segnalazioni automatiche di on/off attuatore */
  pkt = (char*)PrintEventEx2;
  pkt += Ex2_Delphi*MAX_NUM_CONSUMER*EventNumEx2;
  pkt += dev->consumer*EventNumEx2;
  pkt[Ex2_On_attuatore] = 1;
  pkt[Ex2_Off_attuatore] = 1;
  pkt[Ex2_Allineamento] = 0;
  
  support_log("CEI Connect");
  
  while(1)
  {
    num = 15;	// 30 secondi
    while(num && ((ret = cei_L4_get_packet(dev)) == -2)) num--;
    
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
        while(CEI->evlist)
        {
          elt = CEI->evlist;
          CEI->evlist = CEI->evlist->next;
          free(elt->event);
          free(elt);
        }
        cei_free(dev);
        
        support_log("CEI Close");
        return;	// TCP connection closed
      }
      else
      {
        // restart communication on serial line
        
        continue;
      }
    }
    
    if(CEI->L4_len > 8)
    {
      cei_L4_parse_command(dev, LS_ABILITATA[dev->consumer] & 0x02);
    }
    else	// polling
    {
      if(CEI->identvalid)
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
          ret = cei_event_to_l4cei(dev, &ev, &pkt);
          if(pkt) ((char*)pkt)[4] |= (CEI->L4[4] & CEI_L4_FLAG_S);
        }
      }
      else
      {
        pkt = NULL;
        ret = 0;
      }
      cei_L4_send_packet(dev, pkt, ret);
      free(pkt);
    }
  }
}

static void cei_listen(ProtDevice *dev)
{
  struct sockaddr_in sa;
  static int one = 1;
  
  if(!dev) return;
  
  if(!config.consumer[dev->consumer].param)
  {
    /* Se viene chiamata senza parametri, il consumatore è
       configurato in modo tradizionale e quindi eseguo
       semplicemente il loop CA in chiaro. */
    if(!cei_init(dev)) return;
  
    cei_configure(CEI, CEI_CA | CEI_CRC16, CEI_PROT_NONE);
    cei_ca_loop(dev);
    return;
  }
  
  dev->fd_eth = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
  if(dev->fd_eth >= 0)
  {
    setsockopt(dev->fd_eth, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(int));
    sa.sin_family = AF_INET;
    sa.sin_port = htons(config.consumer[dev->consumer].data.eth.port);
    sa.sin_addr.s_addr = INADDR_ANY;
    if(bind(dev->fd_eth, (struct sockaddr*)&sa, sizeof(struct sockaddr_in)))
    {
      close(dev->fd_eth);
      return;
    }
  }
  
  listen(dev->fd_eth, 0);
  
  while(1)
  {
    dev->fd = accept(dev->fd_eth, NULL, NULL);
    if(dev->fd >= 0)
    {
      dev->protocol_active = 1;
      
      if(cei_init(dev))
      {
        if(strstr(config.consumer[dev->consumer].param, "feal"))
          cei_configure(CEI, CEI_CA | CEI_CRC16, CEI_PROT_FEAL);
        else
          cei_configure(CEI, CEI_CA | CEI_CRC16, CEI_PROT_NONE);
        cei_ca_loop(dev);
      }
      
      if(dev->fd >= 0) close(dev->fd);
      dev->fd = -1;
    }
    else
    {
      support_log("ETH Close");
      return;
    }
  }
}

void _init()
{
  printf("CEI-ABI standard (plugin, server): " __DATE__ " " __TIME__ "\n");
  prot_plugin_register("CEI", 0, NULL, NULL, (PthreadFunction)cei_listen);
}

#if 0
static int cei_inizio_lista_alim = 0;
static void cei_allarme_set_evento(int sens, int code)
{
  FILE *fp;
  
  if(sens < n_SE)
  {
    cei_allarme_evento[sens] = code;
    fp = fopen("/saet/data/cei_alimentazione.nv", cei_inizio_lista_alim?"a":"w");
    if(fp)
    {
      cei_inizio_lista_alim = 1;
      fwrite(cei_allarme_evento, 1, n_SE, fp);
      fclose(fp);
    }
  }
}
#endif

/* La configurazione dei sensori tramite queste macro, specializza
   gli eventi CEI generati. Un allarme relativo a questi sensori
   viene rimappato nella classe 8 (CLS_GRP_ALIMENTAZIONE) se il
   sensore è >= 8, nella classe 1 (CLS_UNITA_CENTRALE) se il sensore
   è < 8 (periferica di centrale). In entrambi i casi, l'evento
   di allarme viene tradotto nell'evento configurato, il numero di
   elemento viene impostato a 1 nel caso di classe CLS_UNITA_CENTRALE.
   Per gli eventi di manomissione e guasto, la conversione coinvolge
   la classe ed il numero elemento, mentre l'evento rimane lo stesso.
*/

// #define CEI_manca_rete(s) cei_allarme_manca_rete(s,0)
void cei_allarme_manca_rete(int sens, int centrale)
{
  if(sens < n_SE) cei_allarme_alimentazione[sens] = ALM_MANCARETE | (centrale<<7);
}

// #define CEI_guasto_batteria(s) cei_allarme_batteria(s,0)
void cei_allarme_batteria(int sens, int centrale)
{
  if(sens < n_SE) cei_allarme_alimentazione[sens] = ALM_BATT_SCARICA | (centrale<<7);
}

void cei_attuatore(int att)
{
  if(att < n_AT) cei_attuatore_configurato[att] = 1;
}
