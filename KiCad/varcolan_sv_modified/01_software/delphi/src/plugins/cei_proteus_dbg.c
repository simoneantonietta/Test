/*

programma utente:

centrale:
- liste classi 1 (centrale) e 8 (alimentazione)

*/

#include "protocol.h"
#include "codec.h"
#include "serial.h"
#include "support.h"
#include "database.h"
#include "master.h"
#include "gsm.h"
#include "delphi.h"
#include "user.h"
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
#include <ctype.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <netdb.h>

/* Numero di polling in connessione backup senza eventi da trasmettere,
   scaduti i quali la connessione viene disattivata. */
#define BACKUP_POLL_EXIT 30

//#undef FEAL_MK_VAL
//#define FEAL_MK_VAL	"01234567"

extern int libuser_started;
int GSM_Squillo();
int GSM_Call_refuse();

#define CEI_VERSIONE	2

#define DEBUG

#define CEI	((CEI_data*)(dev->prot_data))
#define L7	L4+8
#define L7_len	L4_len-12

static pthread_mutex_t cei_evlistmutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t cei_backupmutex = PTHREAD_MUTEX_INITIALIZER;
/* Indica se la connessione LAN è attiva o meno */
static int cei_connesso = 0;
static ProtDevice *dev_main = NULL;
static ProtDevice *dev_backup = NULL;
static int cei_numeri = 0;
static char *cei_numero = NULL;
static int cei_chiama = -1;
static int cei_numero_corrente = -1;
static int cei_numero_timeout = -1;

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
{ALM_ATTIVAZIONE, ALM_INIZIO, CLS_ORGANO_COMANDO, 2}, /* Sentinella on */
{ALM_ATTIVAZIONE, ALM_FINE, CLS_ORGANO_COMANDO, 2}, /* Sentinella off */
{ALM_ATTIVAZIONE, ALM_FINE, CLS_ORGANO_COMANDO, 2}, /* Sentinella off timeout */
{ALM_, ALM_, CLS_, NOT_IMPL}, /* No ModuloMasterTx */
{ALM_, ALM_, CLS_, NOT_IMPL}, /* ErrRx ModuloMaster */
{ALM_, ALM_, CLS_, NOT_IMPL}, /* Errore messaggio host */
{ALM_RIPARTENZA_PROGR, ALM_INIZIO, CLS_UNITA_CENTRALE, 0}, /* Segnalazione evento */
{ALM_, ALM_, CLS_, NOT_IMPL}, /* Periferiche presenti */
{ALM_STATO, ALM_INIZIO, CLS_ZONA, 1}, /* Lista zone */
{ALM_STATO, ALM_INIZIO, CLS_SENSORE, 2}, /* Lista sensori */
//{ALM_STATO, ALM_INIZIO, CLS_ATTUATORE, 2}, /* Lista attuatori */
{ALM_, ALM_, CLS_, NOT_IMPL}, /* Lista attuatori */
{ALM_GUASTO, ALM_INIZIO, CLS_SENSORE, 2}, /* Guasto Sensore */
{ALM_, ALM_, CLS_, NOT_IMPL}, /* Variazione ora */
{ALM_, ALM_, CLS_, NOT_IMPL}, /* Codici controllo */
{ALM_, ALM_, CLS_, NOT_IMPL}, /* Stato QRA Ronda */
{ALM_, ALM_, CLS_, NOT_IMPL}, /* Accettato allarme Sensore */
{ALM_, ALM_, CLS_, NOT_IMPL}, /* Mancata punzonatura */
{ALM_COMANDO_LOCALE, ALM_INIZIO, CLS_SEGNALAZIONE, 2}, /* ON telecomando */
{ALM_, ALM_, CLS_, NOT_IMPL}, /* Parametri taratura */
{ALM_, ALM_, CLS_, NOT_IMPL}, /* Stato prova */
{ALM_TEST_MANUALE, ALM_INIZIO, CLS_SENSORE, 1}, /* Test attivo in corso */
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
{ALM_ACCESSO_OPER, ALM_INIZIO, CLS_OPERATORE, 2}, /* Entrato Ident */
{ALM_ACCESSO_OPER, ALM_FINE, CLS_OPERATORE, 2}, /* Uscito Ident */
{ALM_, ALM_, CLS_, NOT_IMPL}, /* Codice valido */
{ALM_, ALM_, CLS_, NOT_IMPL}, /* Chiave valida */
{ALM_ACCESSO_OPER, ALM_INIZIO, CLS_OPERATORE, 1}, /* Operatore */
{ALM_ACCESSO_OPER, ALM_INIZIO, CLS_OPERATORE, 2}, /* Punzonatura */
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
{ALM_TEST_MANUALE, ALM_INIZIO, CLS_SENSORE, 1}, /* Inizio test attivo */
{ALM_TEST_POSITIVO, ALM_INIZIO, CLS_SENSORE, 1}, /* Fine test attivo (positivo) */
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
{ALM_TEST_NEGATIVO, ALM_INIZIO, CLS_SENSORE, 1}, /* Fine test attivo (negativo) */
{ALM_STRAORDINARIO, ALM_INIZIO, CLS_ZONA, 0},
{ALM_STRAORDINARIO, ALM_FINE, CLS_ZONA, 0},
{ALM_INS_ANTICIPATO, ALM_INIZIO, CLS_UNITA_CENTRALE, 0},
{ALM_, ALM_, CLS_, NOT_IMPL},
{ALM_STATO, ALM_PERSIST, CLS_UNITA_CENTRALE, 0},	/* Stato centrale (per allineamento) */
{ALM_ATTIVAZIONE, ALM_INIZIO, CLS_ATTUATORE, 2}, /* On attuatore */
{ALM_ATTIVAZIONE, ALM_FINE, CLS_ATTUATORE, 2}, /* Off attuatore */
{ALM_VAR_PARAMETRI, ALM_INIZIO, CLS_UNITA_CENTRALE, 0}, /* Salvataggio database */
{ALM_AUTOESCLUSO, ALM_INIZIO, CLS_SENSORE, 2}, /* Fuori servizio sensore (autoesclusione) */
{ALM_AUTOESCLUSO, ALM_FINE, CLS_SENSORE, 2}, /* In servizio sensore (autoesclusione) */
};

static void cei_event_set_alarm(CEI_allarme *alarm, Event *ev, int tempoaccadimento)
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
  
  alarm->transizione = cei_event_table[ev->Event[8]-EV_OFFSET].transizione;
  alarm->tipoevento = cei_event_table[ev->Event[8]-EV_OFFSET].tipoevento;
  alarm->classeelemento = cei_event_table[ev->Event[8]-EV_OFFSET].classeelemento;
  
  if(alarm->classeelemento == CLS_UNITA_CENTRALE) alarm->areaprovenienza = 64;
  
  switch(cei_event_table[ev->Event[8]-EV_OFFSET].numbytes)
  {
    case 1:
      alarm->numeroelementomsb = 0;
      if(alarm->tipoevento == ALM_TEST_MANUALE)
        alarm->numeroelementolsb = 7;
      else
        alarm->numeroelementolsb = ev->Event[9];
      break;
    case 2:
      alarm->numeroelementomsb = ev->Event[9];
      if((alarm->classeelemento == CLS_UNITA_SATELLITE) && !ev->Event[9] && !ev->Event[10])
      {
        alarm->classeelemento = CLS_UNITA_CENTRALE;
        alarm->numeroelementolsb = 1;
        /* La manomissione contenitore della periferica 0 viene segnalata
           come "apertura portello" della centrale. */
        if((ev->Event[8] == Manomissione_Contenitore) ||
           (ev->Event[8] == Fine_Manomis_Contenitore))
          alarm->tiporivelatore = 2;
      }
      else
        alarm->numeroelementolsb = ev->Event[10];
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
    
  if(alarm->classeelemento == CLS_UNITA_CENTRALE) alarm->areaprovenienza = 64;
  
  switch(cei_event_table_ex2[ev->Event[10]].numbytes)
  {
    case 1:
      alarm->numeroelementomsb = 0;
      if((alarm->tipoevento == ALM_TEST_MANUALE) ||
         (alarm->tipoevento == ALM_TEST_POSITIVO) ||
         (alarm->tipoevento == ALM_TEST_NEGATIVO))
        alarm->numeroelementolsb = 7;
      else
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

static void cei_event_set_alarm_Tebe(CEI_allarme *alarm, Event *ev, int tempoaccadimento)
{
  struct {
    unsigned char giustificativomsb;
    unsigned char giustificativolsb;
    unsigned char terminale;
    unsigned char area;
  } *valori;
  
  alarm->header.formato = 1;	// Formato ABI v.2
  alarm->header.versione = CEI_VERSIONE;
  
  alarm->header.processo = 1;	// supervisione e controllo
  alarm->header.tipoinfo = 2;	// allarmi e stati con valore
  
  alarm->tempoaccadimento = tempoaccadimento;
  alarm->areaapplicativa = 0;
  alarm->tiporivelatore = 0;
  alarm->categoria = 1;	// non richiede azioni da CSC
  alarm->areaprovenienza = 0;
  
  /* Per ora gestisco solo le timbrature */
  /* 2013-03-25 Aggiunta la gestione delle abilitazioni */
  switch(ev->Event[10])
  {
    case 32:
      alarm->transizione = ALM_INIZIO;
      alarm->tipoevento = ALM_DISABILITATO;
      break;
    case 33:
      alarm->transizione = ALM_FINE;
      alarm->tipoevento = ALM_DISABILITATO;
      break;
    default:
      alarm->transizione = ALM_INIZIO;
      alarm->tipoevento = ALM_ACCESSO_OPER;
      break;
  }
  alarm->classeelemento = CLS_OPERATORE;
  
  alarm->numeroelementomsb = ev->Event[12];
  alarm->numeroelementolsb = ev->Event[11];
  
  if(ev->Event[10] > 10)
  {
    alarm->header.tipoinfo = 1;	// allarmi e stati
    return;
  }
  
  /* L'area valori è al fondo della struttura allarme. */
  valori = (void*)(alarm + 1);
  
  if(ev->Event[10] < 9)
  {
    valori->giustificativomsb = 0xff;
    valori->giustificativolsb = 0xff;
  }
  else
  {
    valori->giustificativomsb = ev->Event[16];
    valori->giustificativolsb = ev->Event[15];
  }
  valori->terminale = ev->Event[13];
  if((ev->Event[10] == 7) || (ev->Event[10] == 9))
    valori->area = ev->Event[14];
  else
    valori->area = 0xff;
}

static CEI_segnalazione_comandi_locali* cei_comando_locale_header(ProtDevice *dev, int sorgente, int tempoaccadimento, int *len)
{
  CEI_segnalazione_comandi_locali *event;
  
  *len = sizeof(CEI_segnalazione_comandi_locali);
  event = calloc(1, sizeof(CEI_segnalazione_comandi_locali));
  if(!event)
  {
    *len = -1;
    return NULL;
  }
  
  event->header.formato = 1;	// Formato ABI v.2
  event->header.versione = CEI_VERSIONE;
  event->header.processo = 1;	// allarmi e stati
  event->header.numeromessaggio = CEI->L4_pktnum[1]++;
  event->header.tipoinfo = 5 | (sorgente << 4);	// segnalazione comandi locali
  event->tempoaccadimento = tempoaccadimento;
  
  return event;
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
  CEI_segnalazione_comandi_locali *local;
  struct tm tm;
  time_t t;
  
  *pkt = NULL;
  
  if(!ev || !ev->Len) return 0;
  
  if(dev->consumer == config.debug)
  {
    char *buf;
    int ret;
    buf = malloc(10+ev->Len*2);
    if(buf)
    {
      sprintf(buf, "* Ev *: ");
      for(ret=0; ret<ev->Len-10; ret++)
        sprintf(buf+8+ret*2, "%02x", ev->Event[8+ret]);
      support_log(buf);
      free(buf);
    }
  }
  
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
    if((ev->Event[9] == Ex2_Delphi) &&
       (cei_event_table_ex2[ev->Event[10]].tipoevento == ALM_STATO))
    {
      /* Stato centrale */
      len = sizeof(CEI_stato_header) + sizeof(CEI_stato) * 7; // 8; // salta fuori sincronismo orologio
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
        if(i == 3) continue;	// salta fuori sincronismo orologio

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
          if(alarm->numeroelementolsb || alarm->numeroelementomsb)
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
        case Ex2_Lara:
          /* Per ora gestisco solo le timbrature */
          /* 2013-03-25 Aggiunta la gestione delle abilitazioni */
          if((ev->Event[10] < 7) ||
             ((ev->Event[10] > 10) && !((ev->Event[10] == 32)||(ev->Event[10] == 33))))
            return -1;
          
          /* Aggiungo anche i 4 byte per i valori senza alterare
             il file cei_common.h per la definizione della struttura
             specifica. */
          len = sizeof(CEI_allarme)+4;
          alarm = calloc(1, len);
          if(!alarm) return -1;
  
          cei_event_set_alarm_Tebe(alarm, ev, tempoaccadimento);
          if(alarm->numeroelementolsb || alarm->numeroelementomsb)
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
          /*
             Le liste sensori normali presentano lo stato di allarme del sensore
             anche se la zona non è attiva. Occorre quindi filtrare.
          */
          if((tipoevento == ALM_ALLARME) && (Zona_SE[ev->Event[9]*256+ev->Event[10]+(i/4)] == 0xff))
            stato->transizione = ALM_FINE;
          else
            stato->transizione = (ev->Event[11 + (i/4)] & bitmask)?ALM_PERSIST:ALM_FINE;
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
           (!SE_ZONA[stato->numeroelementolsb] ||
            /* 03/04/2013 - Su richiesta di Maffiotti, ora si vuole di nuovo che
               le zone insieme vengano segnalate. Nota: SE_ZONA vale anche per
               le zone insieme, include tutti i sensori definiti per le zone
               semplici incluse nella zona insieme. */
            //(stato->numeroelementolsb >= 216) ||
            (SE_ZONA[stato->numeroelementolsb][0] == -1)))
        {
          memset(stato, 0, sizeof(CEI_stato));
          stato--;
          len -= sizeof(CEI_stato);
        }
      }
      stato++;
    }
    *pkt = (unsigned char*)header;
  }
  else if(cei_event_table[ev->Event[8]-EV_OFFSET].tipoevento == ALM_COMANDO_LOCALE)
  {
    if(!ev->Event[9] && !ev->Event[10])	// On telecomando 0
    {
      local = cei_comando_locale_header(dev, 2, tempoaccadimento, &len);
      if(local)
      {
        local->codicecomando = 5;
        local->esito = 2;
        local->classeelemento = cei_event_table[ev->Event[8]-EV_OFFSET].classeelemento;
        local->numeroelementomsb = 0;
        local->numeroelementolsb = 0;
      }
      *pkt = (unsigned char*)local;
    }
    else
      return -1;
  }
  else
  {
    len = sizeof(CEI_allarme);
    alarm = calloc(1, len);
    if(!alarm) return -1;

    cei_event_set_alarm(alarm, ev, tempoaccadimento);
#if 0
    if((alarm->numeroelementolsb || alarm->numeroelementomsb) &&
       /* Non segnalare attivazione/disattivazione zone insieme */
       !((alarm->classeelemento == CLS_ZONA) && (alarm->numeroelementolsb >= 216)))
#else
       /* 03/04/2013 - Su richiesta di Maffiotti, ora si vuole di nuovo che
          le zone insieme vengano segnalate. */
    if(alarm->numeroelementolsb || alarm->numeroelementomsb)
#endif
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
  
  if(dev->consumer == config.debug)
    support_log("* Ev *: -> CEI");
  
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
    if(TipoPeriferica[p] != 0xff)
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
  
  header = cei_allineamento_header(dev, 17, tempoaccadimento, &len);
  if(!header) return;
  
  stato = (void*)header + sizeof(CEI_stato_header);
  stato->transizione = 1;
  stato++;
  
  i = 0;
  for(p=0; p<(MM_NUM*32); p++)
  {
    if(TipoPeriferica[p] == 7)
    {
      stato->transizione = (SE[p*8+1]&bitAlarm)?ALM_PERSIST:ALM_FINE;
      stato->tipoevento = ALM_ATTIVAZIONE;
      stato->classeelemento = CLS_ORGANO_COMANDO;
      stato->numeroelementomsb = p/256;
      stato->numeroelementolsb = p;
      stato++;
      i++;
      if(i == 16)
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
  len -= ((16-i) - 1)*sizeof(CEI_stato);
  
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
  FILE *fp;
  
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
                   
//#warning DEBUG
//printf("Comando %d, classe %d, elemento %d\n", ((CEI_telecomando*)(CEI->L4))->comando, ((CEI_telecomando*)(CEI->L4))->classeelemento, elemento);
        if(abilitato)
        {
          switch(((CEI_telecomando*)(CEI->L4))->comando)
          {
            case 1:
              if(((CEI_telecomando*)(CEI->L4))->classeelemento == CLS_ZONA)
              {
                if(!elemento)
                {
                  memcpy(cmd, "K513", 4);
                  len = 4;
                }
                else
                {
                  sprintf(cmd, "Y1%03d", elemento);
                  len = 5;
                }
              }
              else if(((CEI_telecomando*)(CEI->L4))->classeelemento == CLS_ATTUATORE)
              {
                if((elemento >= 0xff0) && (elemento <= 0xff7))
                {
                  sprintf(cmd, "K%03d", 528 + elemento - 0xff0);
                  len = 4;
                }
                else if(elemento < n_AT)
                {
                  memcpy(cmd, "K516", 4);
                  *(short*)(ME2+516) = elemento;
                  len = 4;
                }
              }
              break;
            case 2:
              if((((CEI_telecomando*)(CEI->L4))->classeelemento == CLS_ZONA) &&
                 (elemento < (1 + n_ZS + n_ZI)))
              {
                if(!elemento)
                {
                  memcpy(cmd, "K514", 4);
                  len = 4;
                }
                else
                {
                  sprintf(cmd, "J%03d", elemento);
                  len = 4;
                }
              }
              else if(((CEI_telecomando*)(CEI->L4))->classeelemento == CLS_ATTUATORE)
              {
                if((elemento >= 0xff0) && (elemento <= 0xff7))
                {
                  sprintf(cmd, "K%03d", 536 + elemento - 0xff0);
                  len = 4;
                }
                else if(elemento < n_AT)
                {
                  memcpy(cmd, "K517", 4);
                  *(short*)(ME2+516) = elemento;
                  len = 4;
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
              else if(((CEI_telecomando*)(CEI->L4))->classeelemento == CLS_OPERATORE)
              {
                memcpy(cmd, "/Lt", 3);
                cmd[3] = elemento;
                cmd[4] = elemento>>8;
                cmd[5] = 6;	// BADGE_ABIL
                len = 6;
              }
              break;
            case 4:
              if(((CEI_telecomando*)(CEI->L4))->classeelemento == CLS_SENSORE)
              {
                sprintf(cmd, "C%04d", elemento);
                len = 5;
              }
              else if(((CEI_telecomando*)(CEI->L4))->classeelemento == CLS_ATTUATORE)
              {
                sprintf(cmd, "A%04d", elemento);
                len = 5;
              }
              else if(((CEI_telecomando*)(CEI->L4))->classeelemento == CLS_OPERATORE)
              {
                memcpy(cmd, "/Lt", 3);
                cmd[3] = elemento;
                cmd[4] = elemento>>8;
                cmd[5] = 4;	// BADGE_DISABIL
                len = 6;
              }
              break;
            case 5:
              if(((CEI_telecomando*)(CEI->L4))->classeelemento == CLS_UNITA_CENTRALE)
              {
                memcpy(cmd, "/533", 4);
                *(short*)(cmd+4) = 9999;
                len = 6;
              }
              else if(((CEI_telecomando*)(CEI->L4))->classeelemento == CLS_SENSORE)
              {
                memcpy(cmd, "K515", 4);
                len = 4;
              }
              else if(((CEI_telecomando*)(CEI->L4))->classeelemento == CLS_SEGNALAZIONE)
              {
                memcpy(cmd, "K000", 4);
                len = 4;
              }
              break;
            case 6:
              if(((CEI_telecomando*)(CEI->L4))->classeelemento == CLS_ZONA)
              {
                // Sblocco ritardo Bancomat e ripristino antirapina
                memcpy(cmd, "K512", 4);
                len = 4;
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
              if(((CEI_telecomando*)(CEI->L4))->classeelemento == CLS_SENSORE)
              {
                memcpy(cmd, "/389", 4);
                len = 4;
              }
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
#if 0
          if(dt > 300)
          {
            /* On ME[594] */
            codec_parse_cmd("K594", 4, dev);
          }
          else
#endif
          {
            datehour = gmtime(&t);
            
#if 0
            /* On ME[593] */
            codec_parse_cmd("K593", 4, dev);
#endif
            
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
//        case 1:	// richiesta parametri
//          break;
        case 3:	// caricamento parametri di configurazione
          if(((CEI_parametri_configurazione*)(CEI->L4))->dati[0] == 0xbe)
          {
            if(((CEI_parametri_configurazione*)(CEI->L4))->dati[1] == 0x55)
            {
              pthread_mutex_lock(&cei_backupmutex);
              cei_chiama = ((CEI_parametri_configurazione*)(CEI->L4))->dati[2]*256 +
                           ((CEI_parametri_configurazione*)(CEI->L4))->dati[3];
#ifdef DEBUG
//printf("Chiama tra: %d\n", cei_chiama);
#endif
              cei_chiama *= 2;	// il timer viene chiamato 2 volte al secondo
              pthread_mutex_unlock(&cei_backupmutex);
            }
            else if((((CEI_parametri_configurazione*)(CEI->L4))->dati[1] == 0x1f) &&
                    (((CEI_parametri_configurazione*)(CEI->L4))->dati[2] == 0x60) &&
                    (((CEI_parametri_configurazione*)(CEI->L4))->dati[3] == 'T'))
            {
              if(cei_numeri) free(cei_numero);
              
              i = CEI->L4_len - 16;
              cei_numeri = i / 16;
#ifdef DEBUG
//printf("Numeri telefonici: %d\n", cei_numeri);
#endif
              cei_numero = malloc(i);
              if(!cei_numero)
              {
                cei_numeri = 0;
                break;
              }
              memcpy(cei_numero, CEI->L4+16, i);
              fp = fopen("/saet/data/cei_numeri.nv", "w");
              if(fp)
              {
                fwrite(&cei_numeri, sizeof(int), 1, fp);
                fwrite(&i, sizeof(int), 1, fp);
                fwrite(cei_numero, 1, i, fp);
                fclose(fp);
              }
            }
          }
          cei_L4_send_packet(dev, NULL, 0);
          break;
        default:
          cei_L4_send_packet(dev, NULL, 0);
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
  
#ifdef DEBUG
  config.debug = dev->consumer;
  config.LogLimit = 204800;
#endif
  
  return 1;
}

/*
static void cei_free(ProtDevice *dev)
{
  free(dev->prot_data);
  dev->prot_data = NULL;
}
*/

static int cei_callback_connect = 0;
static int cei_recall = 0;

static void cei_backup_gprs_recall(void *null1, int null2)
{
  cei_recall = 1;
}

static int cei_callback_gprs(int fd, void *null)
{
  struct termios tio, ttio;
  
  if(fd < 0)
  {
    /* Errore connessione GPRS */
    pthread_mutex_lock(&cei_backupmutex);
    dev_main->fd = -1;
    pthread_mutex_unlock(&cei_backupmutex);
    cei_callback_connect = 0;
    return 1;
  }
  
  support_log("CEI Connect (backup)");
  pthread_mutex_lock(&cei_backupmutex);
  tcgetattr(fd, &tio);
  memcpy(&ttio, &tio, sizeof(struct termios));
  ttio.c_cc[VTIME] = 0;
  ttio.c_cc[VMIN] = 0;
  tcsetattr(fd, TCSANOW, &ttio);
  dev_main->fd = fd;
  pthread_mutex_unlock(&cei_backupmutex);
  
  while(cei_callback_connect) sleep(1);
  
  pthread_mutex_lock(&cei_backupmutex);
  dev_main->fd = -1;
  tcsetattr(fd, TCSANOW, &tio);
  pthread_mutex_unlock(&cei_backupmutex);
  
  return 1;
}

static void* cei_backup_loop_gprs(void *null)
{
  Event ev;
  unsigned char *pkt;
  int squillo;
  char *server_addr;
  int server_port;
  timeout_t *recall;
  char buf[64];
  
  ProtDevice dev;
  
  dev.consumer = support_find_serial_consumer(PROT_GSM, 0);
  /* Se non è configurato il GSM, inutile proseguire. */
  if(dev.consumer < 0) return NULL;
  
  dev.fd = -1;
  dev_backup = &dev;
  recall = timeout_init();
  
////  PrintEventEx2[Ex2_Delphi][dev.consumer][Ex2_On_attuatore] = 1;
////  PrintEventEx2[Ex2_Delphi][dev.consumer][Ex2_Off_attuatore] = 1;
//  PrintEventEx2[Ex2_Delphi][dev.consumer][Ex2_SalvataggioDB] = 1;
//  PrintEventEx2[Ex2_Delphi][dev.consumer][Ex2_Allineamento] = 0;
  pkt = (char*)PrintEventEx2;
  pkt += Ex2_Delphi*MAX_NUM_CONSUMER*EventNumEx2;
  pkt += dev.consumer*EventNumEx2;
  pkt[Ex2_SalvataggioDB] = 1;
  pkt[Ex2_Allineamento] = 0;
  
  /* Il backup parte se è rilevato il GPRS e dopo che il programma
     utente si è attivato */
  while(!gsm_is_gprs || !libuser_started) sleep(2);
  
  gsm_ring_as_voice = 1;
  LS_ABILITATA[dev.consumer] = 0x01;
  
  sleep(10);
  
  sprintf(buf, "CEI Connessione di backup GPRS pronta (consumatore %d).", dev_main->consumer);
  support_log(buf);
  
  /* L'IP è lo stesso configurato per la connessione LAN,
     la porta invece può essere specifica. */
  /* Se la stringa di indirizzo contiene il ^M, il relativo comando AT per il GPRS fallisce.
     Occorre ripulire la stringa. */
  server_addr = strdup(config.consumer[dev_main->consumer].param);
  for(squillo=strlen(server_addr)-1; !isdigit(server_addr[squillo]); squillo--);
  server_addr[squillo+1] = 0;
  
  if(config.consumer[dev.consumer].param)
  {
    server_port = atoi(config.consumer[dev.consumer].param);
    if(!server_port) server_port = config.consumer[dev_main->consumer].data.eth.port;
  }
  else
    server_port = config.consumer[dev_main->consumer].data.eth.port;
  
  while(1)
  {
//printf("*** squillo\n");
/* Occorre mettere qualche impostazione nel saet.new per abilitare lo squillo dati
   senza che si attivi la connessione SaetNet. */
    squillo = GSM_Squillo();
    pthread_mutex_lock(&cei_backupmutex);
//printf("*** lock\n");
    
/* Se trovo cei_chiama > 0 imposto un timeout con callback, poi imposto subito cei_chiama = -1.
   Al callback imposto una qualche variabile che mi fa partire la connessione.
   Se trovo cei_chiama = 0 imposto subito la variabile per innescare la connessione. */
    if(cei_chiama == 0)
    {
//printf("*** chiama subito\n");
      /* Chiama subito */
      cei_recall = 1;
      cei_chiama = -1;
    }
    else if(cei_chiama > 0)
    {
//printf("*** attiva timer\n");
      timeout_on(recall, cei_backup_gprs_recall, NULL, 0, cei_chiama*5);
      cei_chiama = -1;
    }
  
//printf("*** test backup\n");
    if((dev_main->fd == -1) && !cei_callback_connect)
    {
      /* La connessione principale non è attiva, verifico se
         si deve attivare la connessione di backup. */
//printf("*** verifica evento (cons:%d abil:%02x)\n", dev.consumer, LS_ABILITATA[dev.consumer]);
      while(!codec_get_event(&ev, &dev));
      if(squillo || cei_recall || ev.Len)
      {
//printf("*** attivazione\n");
        /* Attivare la connessione di backup */
        support_log("CEI Connessione GPRS per allarme.");
        cei_callback_connect = 1;
        cei_recall = 0;
        gsm_gprs_connect(cei_callback_gprs, server_addr, server_port, NULL);
        pthread_mutex_unlock(&cei_backupmutex);
      }
      else
      {
        pthread_mutex_unlock(&cei_backupmutex);
        sleep(1);
      }
    }
    else
    {
      pthread_mutex_unlock(&cei_backupmutex);
      sleep(1);
    }
  }
  
  return NULL;
}

static void cei_ca_loop(ProtDevice *dev)
{
  Event ev;
  int num, ret = 0;
  unsigned char *pkt;
  char log[64];
  
  pthread_t pth;
  
  char *server_addr;
  int server_port;
  struct sockaddr_in sa;
  FILE *fp;
  
  int backup = 0;
  int backup_conn_exit = 0;
  int backup_poll = 0;
  
  if(!dev || !cei_init(dev)) return;
  
#if 0
Proteus non gestisce minimamente gli attuatori.

  /* Attiva le segnalazioni automatiche di on/off attuatore */
  PrintEventEx2[Ex2_Delphi][dev->consumer][Ex2_On_attuatore] = 1;
  PrintEventEx2[Ex2_Delphi][dev->consumer][Ex2_Off_attuatore] = 1;
#endif
  pkt = (char*)PrintEventEx2;
  pkt += Ex2_Delphi*MAX_NUM_CONSUMER*EventNumEx2;
  pkt += dev->consumer*EventNumEx2;
  pkt[Ex2_Stato_centrale] = 1;
  pkt[Ex2_SalvataggioDB] = 1;
  pkt[Ex2_Fuori_servizio_auto] = 1;
  pkt[Ex2_In_servizio_auto] = 1;
  
  if(!config.consumer[dev->consumer].param) return;
  
  server_addr = config.consumer[dev->consumer].param;
  server_port = config.consumer[dev->consumer].data.eth.port;
  
  /* Solo il primo consumatore CEI ottiene il backup GPRS */
  if(!dev_main)
  {
    backup = 1;
    dev_main = dev;
    pthread_create(&pth, NULL, (PthreadFunction)cei_backup_loop_gprs, NULL);
  }
  
//  cei_configure(CEI, CEI_CA | CEI_CRC16, CEI_PROT_FEAL);
  cei_configure(CEI, CEI_CA | CEI_CRC16, CEI_PROT_NONE);
  
  fp = fopen("/saet/data/cei_numeri.nv", "r");
  if(fp)
  {
    fread(&cei_numeri, sizeof(int), 1, fp);
    fread(&num, sizeof(int), 1, fp);
    
    cei_numero = malloc(num);
    if(cei_numero)
      fread(cei_numero, 1, num, fp);
    else
      cei_numeri = 0;
    fclose(fp);
  }
  
  while(!libuser_started) sleep(1);
  
  while(1)
  {
    if(config.consumer[dev->consumer].configured == 5)
    {
      if(backup) pthread_mutex_lock(&cei_backupmutex);
      if(dev->fd == -1)
      {
#if 1
        dev->fd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
        sa.sin_family = AF_INET;
        sa.sin_port = htons(server_port);
        sa.sin_addr.s_addr = inet_addr(server_addr);
        if(connect(dev->fd, (struct sockaddr*)&sa, sizeof(struct sockaddr)))
        {
          close(dev->fd);
          dev->fd = -1;
          if(backup) pthread_mutex_unlock(&cei_backupmutex);
          sleep(1);
          continue;
        }
#else
#warning DEBUG solo GPRS
          if(backup) pthread_mutex_unlock(&cei_backupmutex);
          sleep(1);
          continue;
#endif        
        /* Solo se connesso sulla linea principale */
        cei_connesso = 1;
        sprintf(log, "CEI (%d) Connect", dev->consumer);
        support_log(log);
      }
      if(backup) pthread_mutex_unlock(&cei_backupmutex);
    }
    
    if(backup_conn_exit)
    {
//printf("**** Uscita\n");
      ret = -1;
      backup_conn_exit = 0;
      backup_poll = 0;
    }
    else
    {
      num = 15;	// 30 secondi
      while(num && ((ret = cei_L4_get_packet(dev)) == -2)) num--;
    }
    
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
        
        sprintf(log, "CEI (%d) Close", dev->consumer);
        support_log(log);
        
        if(backup)
        {
          /* Svuoto completamente lo storico backup, devo ripartire
             alla ricerca di un nuovo evento significativo. */
          if(dev_backup)
          {
            Event evt;
            pthread_mutex_lock(&cei_backupmutex);
            while(codec_get_event(&evt, dev_backup) >= 0);
            pthread_mutex_unlock(&cei_backupmutex);
          }

          if(cei_callback_connect)
          {
            /* Sblocca la connessione GPRS */
            pthread_mutex_lock(&cei_backupmutex);
            cei_callback_connect = 0;
            cei_connesso = 0;
            CEI->identvalid = 0;
            pthread_mutex_unlock(&cei_backupmutex);
            while(dev->fd != -1) sleep(0);
          }
          else if(cei_numero_corrente != -1)
          {
            /* La connessione potrebbe essersi chiusa per una chiamata di
               backup, anche se la connessione di rete rimane possibile.
               Occorre quindi verificare questa condizione ed evitare che
               si ripristini immediatamente la connessione di rete. */
            pthread_mutex_lock(&cei_backupmutex);
            dev->fd = -1;
            cei_connesso = 0;
            CEI->identvalid = 0;
            pthread_mutex_unlock(&cei_backupmutex);
            while(dev->fd == -1) sleep(0);
          }
          else if(dev_backup && (dev->fd == dev_backup->fd))
          {
            /* Occorre chiudere la connessione di backup
            */
            sleep(1);
            write(dev_backup->fd, "+++", 3);
            sleep(1);
            write(dev_backup->fd, "ATH\r", 4);
            
            pthread_mutex_lock(&cei_backupmutex);
            dev->fd = -1;
            cei_connesso = 0;
            CEI->identvalid = 0;
            pthread_mutex_unlock(&cei_backupmutex);
/* Bisogna fare in modo che dopo la chiusura della connessione di backup,
   la connessione di rete venga effettuata prima che il processo di backup
   possa riattivarsi. */
          }
          else
          {
            pthread_mutex_lock(&cei_backupmutex);
            close(dev->fd);
            dev->fd = -1;
            cei_connesso = 0;
            CEI->identvalid = 0;
            pthread_mutex_unlock(&cei_backupmutex);
          }
        }
        else
        {
          close(dev->fd);
          dev->fd = -1;
          cei_connesso = 0;
          CEI->identvalid = 0;
        }
        continue;	// Connection closed
      }
      else
      {
        // restart communication on serial line
        continue;
      }
    }
    
    if(CEI->L4_len > 8)
    {
//printf("Comando\n");
      cei_L4_parse_command(dev, LS_ABILITATA[dev->consumer] & 0x02);

      if(backup && cei_callback_connect) support_log("CEI comando");

      backup_poll = 0;
    }
    else	// polling
    {
//printf("Polling\n");

      if(backup && cei_callback_connect)
      {
//printf("**** %d\n", backup_poll);
        backup_poll++;
        sprintf(log, "CEI poll %d", backup_poll);
        support_log(log);
        if(backup_poll > BACKUP_POLL_EXIT) backup_conn_exit = 1;
      }

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
        }
        if(pkt)
        {
          ((char*)pkt)[4] |= (CEI->L4[4] & CEI_L4_FLAG_S);
          backup_poll = 0;
          if(backup && cei_callback_connect) support_log("CEI evento");
        }
      }
      else
      {
        pkt = NULL;
        ret = 0;
      }
      cei_L4_send_packet(dev, pkt, ret);
      if(pkt)
      {
        free(pkt);
        pkt = NULL;
      }
    }
  }
}

static int cei_backup_timer(ProtDevice *dev, int param)
{
  char buf[32];
  
  if(cei_chiama >= 0)
  {
    cei_chiama--;
    if(cei_chiama < 0)
    {
      /* Attivare la chiamata gsm */
      support_log("CEI Chiamata GSM su richiesta del centro.");
      
      if(cei_numeri)
      {
        cei_numero_corrente = 0;
        sprintf(buf, "ATD%s", cei_numero);
        write(dev->fd, buf, strlen(buf));
        cei_numero_timeout = 120;
      }
    }
  }
  
  if(cei_numero_timeout >= 0)
  {
    cei_numero_timeout--;
    if(cei_numero_timeout < 0)
    {
      /* Attivare la chiamata gsm */
      support_log("CEI Timeout chiamata GSM.");
      
      if(cei_numeri)
      {
        sprintf(buf, "ATD%s", cei_numero+16*cei_numero_corrente);
        write(dev->fd, buf, strlen(buf));
//printf("-> %s\n", buf);
        cei_numero_timeout = 120;
      }
    }
  }
  return 0;
}

static void cei_backup_loop(ProtDevice *dev)
{
  struct termios tio;
  Event ev;
  char buf[128];
  int i, n;
  unsigned char *pkt;
  
  if(!dev) return;
  
////  PrintEventEx2[Ex2_Delphi][dev->consumer][Ex2_On_attuatore] = 1;
////  PrintEventEx2[Ex2_Delphi][dev->consumer][Ex2_Off_attuatore] = 1;
//  PrintEventEx2[Ex2_Delphi][dev->consumer][Ex2_SalvataggioDB] = 1;
//  PrintEventEx2[Ex2_Delphi][dev->consumer][Ex2_Allineamento] = 0;
  pkt = (char*)PrintEventEx2;
  pkt += Ex2_Delphi*MAX_NUM_CONSUMER*EventNumEx2;
  pkt += dev->consumer*EventNumEx2;
  pkt[Ex2_SalvataggioDB] = 1;
  pkt[Ex2_Allineamento] = 0;
  
  dev_backup = dev;
//printf("Avvio.\n");
  
  if(config.consumer[dev->consumer].configured < 5)
  {
    gsm_poweron(dev->fd);
    
    tcgetattr(dev->fd, &tio);
    tio.c_cc[VTIME] = 5;
    tio.c_cc[VMIN] = 0;
    tcsetattr(dev->fd, TCSANOW, &tio);
    
    write(dev->fd, "ATE0S0=1\r", 9);
    
    i = 0;
    while((n = read(dev->fd, buf+i, 128-i))) i += n;
    if((i < 4) || strncmp(buf+i-4, "OK\r\n", 4)) return;
    
    tcgetattr(dev->fd, &tio);
    tio.c_cc[VTIME] = 0;
    tio.c_cc[VMIN] = 0;
    tcsetattr(dev->fd, TCSANOW, &tio);
  }
  else
    return;
  
//printf("Modem OK.\n");
  while(!libuser_started) sleep(1);
  sleep(10);
  
  if(!dev_main) return;
  
  tcflush(dev->fd, TCIFLUSH);
  support_log("CEI Connessione di backup pronta.");
//printf("Pronto.\n");
  
  while(1)
  {
    if(dev_main->fd != dev->fd)	// se non sono connesso su linea di backup
    {
      i = 0;
      buf[0] = 0;
      while((n = read(dev->fd, buf+i, 128-i)) && (buf[i+n-1] != '\n'))
      {
        i += n;
        usleep(10000);
      }
      i += n;
      if(i)
      {
        buf[i] = 0;
        for(i=0; buf[i]<' '; i++);
        
        cei_numero_timeout = -1;
        
//printf("%s\n", buf+i);
        /* Controllare cosa è stato eventualmente ricevuto */
        if(!strncmp(buf+i, "ERROR", 5) ||
           !strncmp(buf+i, "BUSY", 4) ||
           !strncmp(buf+i, "NO CARRIER", 10))
        {
          if(cei_numero_corrente >= 0)
          {
            cei_numero_corrente++;
            if(cei_numero_corrente == cei_numeri) cei_numero_corrente = 0;
            if(cei_numeri == 1) sleep(10);
            sprintf(buf, "ATD%s", cei_numero+16*cei_numero_corrente);
            write(dev->fd, buf, strlen(buf));
            cei_numero_timeout = 120;
          }
        }
        else if(!strncmp(buf+i, "CONNECT", 7))
        {
          /* Impostazione fasulla, solo per evitare
             la riconnessione immediata via lan. */
          if(cei_numero_corrente < 0)
            cei_numero_corrente = 0;
          
          if(dev_main->fd >= 0)
          {
            close(dev_main->fd);
            while(dev_main->fd != -1) sleep(0);
          }
          dev_main->fd = dev->fd;
          cei_numero_corrente = -1;
          support_log("CEI Connessione di backup.");
        }
      }
    }
    
    pthread_mutex_lock(&cei_backupmutex);
    if((dev_main->fd == -1) && cei_numeri && (cei_numero_corrente == -1))
    {
//printf("check allarmi\n");
      while(!codec_get_event(&ev, dev));
      if(ev.Len)
      {
        /* Attivare la connessione di backup */
        support_log("CEI Chiamata GSM per allarme.");

/* Oltre a comporre il primo numero, segnarsi che è in corso una chiamata di backup.
   In caso abbia successo (CONNECT) si ripristina tutto, altrimenti in caso di errore
   si sa che si deve procedere con i numeri successivi. */
        cei_numero_corrente = 0;
        sprintf(buf, "ATD%s", cei_numero);
//printf("-> %s\n", buf);
        write(dev->fd, buf, strlen(buf));
        pthread_mutex_unlock(&cei_backupmutex);
      }
      else
      {
        pthread_mutex_unlock(&cei_backupmutex);
        sleep(1);
      }
    }
    else
    {
      pthread_mutex_unlock(&cei_backupmutex);
      sleep(1);
    }
  }
}

/***********************************************
	CONTACT ID
***********************************************/
#define CIDDIM 100
static unsigned char MACaddr[6];
static unsigned char CIDev[CIDDIM][16] = {{0, }, };
static unsigned char CIDlastev[16] = {0, };
static pthread_mutex_t CIDmux = PTHREAD_MUTEX_INITIALIZER;
static int CIDin = 0;
static int CIDout = 0;
static int CIDseq = 0;
//static int CIDkeepalivebase = 1200;	// 2 min
static int CIDkeepalivebase = 3000;	// 5 min	// 2013-03-04
static int CIDkeepalive = 0;
static int CIDwatchdog = 0;
static int CID_sending;
static int CID_send_to;
static int CID_send_retry;
static int CIDbackup = 0;
static int CID_gprs_fail = 0;	// 2013-03-02

char *CID_server_addr[2];
int CID_server_port;

#define CID_CH_LAN	0
#define CID_CH_GPRS	1
#define CID_CH_SMS	2
#define CID_CH_CALLBACK	3

int CID_AS_message(int cond, int num, int evento, int area, int pointid)
{
  unsigned char buf[16];
  char m[64];
  int i;
  
  CIDbackup = num;
  
  if(!cond) return 0;
  
  if((evento > 9999)||(area > 99)||(pointid > 999)) return -1;
  if((evento < 0)||(area < 0)||(pointid < 0)) return -1;
  
  sprintf(buf, "%04d18%04d%02d%03d", config.DeviceID, evento, area, pointid);
  
  sprintf(m, "ContactID (AS): invio %s ", buf);
  
  for(i=0; i<15; i++)
  {
    if(buf[i] == '0')
    {
      buf[i] = 'A';
      buf[15] += 10;
    }
    else
      buf[15] += (buf[i]-'0');
  }
  buf[15] %= 15;
  buf[15] = 15-buf[15];
  if(!buf[15]) buf[15] = 0x0F;
  
  if(buf[15] < 10)
    m[37] = buf[15] = buf[15]+'0';
  else
    m[37] = buf[15] = buf[15]+'A'-10;
  support_log(m);
  
  pthread_mutex_lock(&CIDmux);
  if(((CIDin+1)%CIDDIM) != CIDout)
  {
    memcpy(&(CIDev[CIDin]), buf, 16);
    CIDin++;
    if(CIDin >= CIDDIM) CIDin = 0;
  }
  pthread_mutex_unlock(&CIDmux);
  
  return 0;
}

static int cid_parse(unsigned char *data, int ch)
{
  int crc;
  
#if 0
#warning debug
printf("RX: ");
for(crc=0; crc<16; crc++) printf("%02x", data[crc]);
printf("\n");
#endif
#if 1
  char m[64];
  int i;
  sprintf(m, "ContactID (AS): recv (%d) ", ch);
  crc = strlen(m);
  for(i=0; i<16; i++) sprintf(m+crc+i*2, "%02x", data[i]);
  support_log(m);
#endif

  crc = cei_CRC16(data, 15);
  crc = ((crc>>8)+crc) & 0xff;
  if(data[15] != crc)
  {
    support_log("ContactID (AS): CRC error");
    return 0;
  }
  
  CIDwatchdog = 0;
  CID_gprs_fail = 0;
  
  if(!CIDlastev[8] && (data[9] == 0x16) && (data[10] == 0xa2))
  {
    /* E' una conferma di stato in vita ricevuta in modo inatteso. La scarto. */
  }
  else if(!memcmp(data, CIDlastev, 16))
  {
    support_log("ContactID (AS): conferma");
    memset(CIDlastev, 0, 16);
    CID_send_to = CID_send_retry = 0;
    if((CID_sending == 1) && (ch == CID_CH_LAN)) CID_sending = 0;
    CIDkeepalive = CIDkeepalivebase;
  }
  else
  {
    /* Comando generico */
    
    /* Verifica MAC address */
    if(memcmp(data, MACaddr, 6))
    {
      support_log("ContactID (AS): MAC error");
      return 0;
    }
      /* Sfrutto la stessa codifica degli eventi, ma con "event qualifier" extra
         specifiche: 8=inizio, 9=fine.
         Es: attivazione zona B
             0001 18 9 573 02 000 X
             fuori servizio sensore 4
             0001 18 8 570 00 004 X
      */
    crc = (data[9]<<8)|data[10];
    /* Questo è il comando */
    switch(crc)
    {
      case 0x86a2:	// impostazione periodo stato in vita
        crc = (data[12]<<4)|(data[13]>>4);
        if((crc & 0xf00) == 0xa00) crc &= 0x0ff;
        if((crc & 0x0f0) == 0x0a0) crc &= 0xf0f;
        if((crc & 0x00f) == 0x00a) crc &= 0xff0;
        crc = (crc>>8)*100 + ((crc>>4)&0xf)*10 + (crc&0xf);
        CIDkeepalivebase = crc * 600;
        break;
      case 0x857a:	// fuori servizio sensore
        crc = (data[12]<<4)|(data[13]>>4);
        if((crc & 0xf00) == 0xa00) crc &= 0x0ff;
        if((crc & 0x0f0) == 0x0a0) crc &= 0xf0f;
        if((crc & 0x00f) == 0x00a) crc &= 0xff0;
        crc = (crc>>8)*100 + ((crc>>4)&0xf)*10 + (crc&0xf);
        cmd_sensor_off(crc);
        break;
      case 0x957a:	// in servizio sensore
        crc = (data[12]<<4)|(data[13]>>4);
        if((crc & 0xf00) == 0xa00) crc &= 0x0ff;
        if((crc & 0x0f0) == 0x0a0) crc &= 0xf0f;
        if((crc & 0x00f) == 0x00a) crc &= 0xff0;
        crc = (crc>>8)*100 + ((crc>>4)&0xf)*10 + (crc&0xf);
        cmd_sensor_on(crc);
        break;
      case 0x8573:	// disattivazione zona
#if 0
        crc = data[11] & 0x0f;
        if(crc == 0xa) crc = 0;
#else
        crc = data[11];
        if((crc & 0xf0) == 0xa0) crc &= 0x0f;
        if((crc & 0x0f) == 0x0a) crc &= 0xf0;
        crc = ((crc>>4)&0xf)*10 + (crc&0xf);
#endif
        if(crc < n_ZS+1)
        {
          if(crc) crc += n_ZS+1;
          cmd_zone_off(crc, 0);
        }
        break;
      case 0x9573:	// attivazione zona (con FS)
#if 0
        crc = data[11] & 0x0f;
        if(crc == 0xa) crc = 0;
#else
        crc = data[11];
        if((crc & 0xf0) == 0xa0) crc &= 0x0f;
        if((crc & 0x0f) == 0x0a) crc &= 0xf0;
        crc = ((crc>>4)&0xf)*10 + (crc&0xf);
#endif
        if(crc < n_ZS+1)
        {
          if(crc) crc += n_ZS+1;
          cmd_zone_on(crc, 2);
        }
        break;
      case 0x852a:	// fuori servizio attuatore
        crc = (data[12]<<4)|(data[13]>>4);
        if((crc & 0xf00) == 0xa00) crc &= 0x0ff;
        if((crc & 0x0f0) == 0x0a0) crc &= 0xf0f;
        if((crc & 0x00f) == 0x00a) crc &= 0xff0;
        crc = (crc>>8)*100 + ((crc>>4)&0xf)*10 + (crc&0xf);
        cmd_actuator_off(crc);
        break;
      case 0x952a:	// in servizio attuatore
        crc = (data[12]<<4)|(data[13]>>4);
        if((crc & 0xf00) == 0xa00) crc &= 0x0ff;
        if((crc & 0x0f0) == 0x0a0) crc &= 0xf0f;
        if((crc & 0x00f) == 0x00a) crc &= 0xff0;
        crc = (crc>>8)*100 + ((crc>>4)&0xf)*10 + (crc&0xf);
        cmd_actuator_on(crc);
        break;
      case 0x832a:	// on attuatore
        crc = (data[12]<<4)|(data[13]>>4);
        if((crc & 0xf00) == 0xa00) crc &= 0x0ff;
        if((crc & 0x0f0) == 0x0a0) crc &= 0xf0f;
        if((crc & 0x00f) == 0x00a) crc &= 0xff0;
        crc = (crc>>8)*100 + ((crc>>4)&0xf)*10 + (crc&0xf);
        Donanl((unsigned int*)&AT[crc]);
        break;
      case 0x932a:	// off attuatore
        crc = (data[12]<<4)|(data[13]>>4);
        if((crc & 0xf00) == 0xa00) crc &= 0x0ff;
        if((crc & 0x0f0) == 0x0a0) crc &= 0xf0f;
        if((crc & 0x00f) == 0x00a) crc &= 0xff0;
        crc = (crc>>8)*100 + ((crc>>4)&0xf)*10 + (crc&0xf);
        Doffa((unsigned int*)&AT[crc]);
        break;
      case 0x86a5:	// richiesta stato sensore
        crc = (data[12]<<4)|(data[13]>>4);
        if((crc & 0xf00) == 0xa00) crc &= 0x0ff;
        if((crc & 0x0f0) == 0x0a0) crc &= 0xf0f;
        if((crc & 0x00f) == 0x00a) crc &= 0xff0;
        crc = (crc>>8)*100 + ((crc>>4)&0xf)*10 + (crc&0xf);
        {
          if(SE[crc] & bitOOS)
            CID_AS_message(1, CIDbackup, 1570, 0, crc);
          else
            CID_AS_message(1, CIDbackup, 3570, 0, crc);
          if(SE[crc] & bitAlarm)
            CID_AS_message(1, CIDbackup, 1130, Zona_SE[crc], crc);
          else
            CID_AS_message(1, CIDbackup, 3130, Zona_SE[crc], crc);
          if(SE[crc] & bitSabotage)
            CID_AS_message(1, CIDbackup, 1137, 0, crc);
          else
            CID_AS_message(1, CIDbackup, 3137, 0, crc);
          if(SE[crc] & bitFailure)
            CID_AS_message(1, CIDbackup, 1380, 0, crc);
          else
            CID_AS_message(1, CIDbackup, 3380, 0, crc);
        }
        break;
      default:
        /* Rinfresco il timeout di chiusura, in realtà il modulo chiude prima. */
        //if(allsystem_state.gprsclose) allsystem_state.gprsclose = GPRS_TIMEOUT;
        /* Ma poi non invio l'echo di conferma. */
support_log("ContactID (AS): sconosciuto");
        return 0;
        /* E' capitato infatti di ricevere una doppia conferma ad un evento, il
           primo è stato effettivamente preso come conferma, il secondo come comando.
           Comando peraltro non gestito essendo a tutti gli effetti un evento, ma
           al quale ho inviato la conferma che a sua volta il centro ha considerato
           come un evento inviando la relativa conferma. In pratica si è innescato
           un loop di messaggi. Evitando di restituire la conferma a ciò che non
           è corretto ricevere interrompo il loop. */
    }
    
support_log("ContactID (AS): Invio conferma");
    /* Invio la conferma al comando eseguito. */
    return 1;
  }
  
  return 0;
}

void CID_strtobin(char *ev)
{
  unsigned char hex[4];
  int i, n;
  
  hex[2] = 0;
  for(i=0; i<8; i++)
  {
    hex[0] = ev[i*2];
    hex[1] = ev[i*2+1];
    sscanf(hex, "%x", &n);
    CIDlastev[6+i] = n;
  }
}

void CID_create_ev(void)
{
  int c;
  
  memcpy(CIDlastev, MACaddr, 6);
  CID_strtobin(CIDev[CIDout]);
  CIDlastev[14] = CIDseq++;
  
  c = cei_CRC16(CIDlastev, 15);
  CIDlastev[15] = (c>>8)+c;
  
  CIDout++;
  if(CIDout >= CIDDIM) CIDout = 0;
}

int CID_AS_invio(int fd, void *data)
{
  int s, i, idx, connto;
  unsigned char rec[20];
  
  /* Se fd = -1 allora devo chiudere tutto, sono finiti i tentativi di connessione */
  if(fd < 0)
  {
    if((CID_sending == 2) && CID_server_addr[1])
    {
      /* Fallito l'invio al primo IP del centro, provo sul secondo. */
      support_log("ContactID (AS): Attivazione backup GPRS (2)");
      CID_sending = 3;	// backup gprs 2
      CID_send_retry = 0;
      gsm_gprs_connect_ex(CID_AS_invio, CID_server_addr[1], CID_server_port, GPRS_INIT_UDP, NULL);
    }
    else
    {
      /* Non riesco a connettermi via GPRS, backup SMS */
      support_log("ContactID (AS): connessione gprs fallita");
      CID_sending = 4;
      CID_send_retry = 0;
      
      CID_gprs_fail++;
      if(CID_gprs_fail >= 3)
      {
        CID_gprs_fail = 0;
        GSM_OnOff(-1);	// spengo e riaccendo il modulo GSM
      }
    }
    return 1;
  }
  
  support_log("ContactID (AS): gprs");
  
  /* Svuoto la ricezione, dovrebbe essere rimasto uno '\n' da consumare. */
  while(read(fd, rec, 16) > 0);
  
  idx = 0;
  
#if 1
{
  char m[64];
  int i;
  sprintf(m, "ContactID (AS): send (GPRS.1) ");
  s = strlen(m);
  for(i=0; i<16; i++) sprintf(m+s+i*2, "%02x", CIDlastev[i]);
  support_log(m);
}
#endif
  
  if((CIDlastev[9] == 0x16) && (CIDlastev[10] == 0xa2))
  {
    if(CIDlastev[11] != 0xa3) // non è callback
    {
      CIDlastev[11] = 0xa1;	// gprs
      /* Ricalcolo cks ContactID e crc messaggio */
      for(i=0; i<8; i++)
        sprintf(rec+i*2, "%02X", CIDlastev[i+6]);
      rec[15] = 0;
      for(i=0; i<15; i++)
      {
        if(rec[i] == 'A')
          rec[15] += 10;
        else
          rec[15] += (rec[i]-'0');
      }
      rec[15] %= 15;
      rec[15] = 15-rec[15];
      if(!rec[15]) rec[15] = 0x0F;
      
      if(rec[15] < 10)
        rec[15] = rec[15]+'0';
      else
        rec[15] = rec[15]+'A'-10;
      CID_strtobin(rec);
      
      i = cei_CRC16(CIDlastev, 15);
      CIDlastev[15] = (i>>8)+i;
    }
  }
  
  connto = 605;
  
  while(connto)
  {
    if(!CIDlastev[8] && (CIDout != CIDin) /*CIDev[CIDout][4]*/)
    {
      CID_create_ev();
      
#if 1
{
  char m[64];
  int i;
  sprintf(m, "ContactID (AS): send (GPRS.2) ");
  s = strlen(m);
  for(i=0; i<16; i++) sprintf(m+s+i*2, "%02x", CIDlastev[i]);
  support_log(m);
}
#endif
      if((CIDlastev[9] == 0x16) && (CIDlastev[10] == 0xa2))
      {
        if(CIDlastev[11] != 0xa3) // non è callback
        {
          CIDlastev[11] = 0xa1;	// gprs
          /* Ricalcolo cks ContactID e crc messaggio */
          for(i=0; i<8; i++)
            sprintf(rec+i*2, "%02X", CIDlastev[i+6]);
          rec[15] = 0;
          for(i=0; i<15; i++)
          {
            if(rec[i] == 'A')
              rec[15] += 10;
            else
              rec[15] += (rec[i]-'0');
          }
          rec[15] %= 15;
          rec[15] = 15-rec[15];
          if(!rec[15]) rec[15] = 0x0F;
          
          if(rec[15] < 10)
            rec[15] = rec[15]+'0';
          else
            rec[15] = rec[15]+'A'-10;
          CID_strtobin(rec);
          
          i = cei_CRC16(CIDlastev, 15);
          CIDlastev[15] = (i>>8)+i;
        }
      }
      
    }
    
    if(!CID_send_to && CIDlastev[8])
    {
      if(CID_send_retry < 2)
      {
        support_log("ContactID (AS): gprs invio");
        write(fd, CIDlastev, 16);
        //if((CIDlastev[9] != 0x16) || (CIDlastev[10] != 0xa2))
        {
          /* Se non è uno stato in vita... */
          /* Devo aspettare 30 secondi la risposta e nel caso fare 3 tentativi. */
          CID_send_to = 300;
          if(connto < 305) connto = 305;
          CID_send_retry++;
        }
        //else
        //  memset(CIDlastev, 0, 16);
      }
      else if((CID_sending == 2) && CID_server_addr[1])
      {
        /* Fallito l'invio al primo IP del centro, provo sul secondo. */
        support_log("ContactID (AS): Attivazione backup GPRS (2)");
        CID_sending = 3;	// backup gprs 2
        CID_send_retry = 0;
        gsm_gprs_connect_ex(CID_AS_invio, CID_server_addr[1], CID_server_port, GPRS_INIT_UDP, NULL);
        return 1;
      }
      else if((CIDlastev[9] == 0x16) && (CIDlastev[10] == 0xa2))
      {
        /* Lo stato in vita viene buttato via, non attiva il backup SMS */
        /* Però chiudo la connessione */
        memset(CIDlastev, 0, 16);
        CID_sending = 0;
        CID_send_retry = 0;
        support_log("ContactID (AS): GPRS disconnessione");
      
        CID_gprs_fail++;
        if(CID_gprs_fail >= 3)
        {
          CID_gprs_fail = 0;
          GSM_OnOff(-1);	// spengo e riaccendo il modulo GSM
        }
        return 1;
      }
      else
      {
        /* Superati i 3 tentativi, invio via SMS */
        CID_sending = 4;
        CID_send_retry = 0;
        support_log("ContactID (AS): GPRS disconnessione (sms)");
      
        CID_gprs_fail++;
        if(CID_gprs_fail >= 3)
        {
          CID_gprs_fail = 0;
          GSM_OnOff(-1);	// spengo e riaccendo il modulo GSM (prima però partono tutti gli sms)
        }
        return 1;
      }
    }
    
      s = read(fd, rec+idx, 16-idx);
      if(!s)
      {
if(idx) support_log("ContactID (AS): reset read");
        idx = 0;
        if(CID_send_to) CID_send_to--;
        if(connto) connto--;
      }
      else
      {
        idx += s;
        
#if 0
#warning debug
        for(s=0; s<idx; s++) printf("%02x", rec[s]);
        printf("\n");
#endif
#if 1
{
  char m[64];
  strcpy(m, "ContactID (AS): rx ");
  for(s=0; s<idx; s++) sprintf(m+19+s*2, "%02x", rec[s]);
  support_log(m);
}
#endif
        
        if(idx == 16)
        {
          if(cid_parse(rec, CID_CH_GPRS))
            write(fd, rec, 16);
          idx = 0;
          connto = 605;
        }
      }
  }
  
  support_log("ContactID (AS): GPRS disconnessione (ok)");
  /* Ritorno 1 se è andato tutto bene o 0 se devo ripetere la connessione GPRS. */
  CID_sending = 0;
  return 1;
}

/*
static int cid_timer(ProtDevice *dev, int param)
{
  return 0;
}
*/

#define CID_SMS_LIMIT 100

static void cid_loop(ProtDevice *dev)
{
  struct sockaddr_in sa;
  struct ifreq ifr;
  struct hostent *host;
  int n, fd[2];
  unsigned char buf[40];
  struct timeval to;
  fd_set fds;
  int CID_SMS_limit;
  time_t t;
  
  if(!dev || !config.consumer[dev->consumer].param) return;
  CID_server_addr[0] = config.consumer[dev->consumer].param;
  CID_server_port = config.consumer[dev->consumer].data.eth.port;
  
  /* Devo ripulire l'indirizzo o la connessione GPRS fallisce. */
  for(n=strlen(CID_server_addr[0])-1; !isdigit(CID_server_addr[0][n]); n--);
  CID_server_addr[0][n+1] = 0;
  /* Verifico se ho un secondo IP di backup */
  for(n=strlen(CID_server_addr[0])-1; (n>=0)&&(CID_server_addr[0][n]!=':'); n--);
  if(n >= 0)
  {
    CID_server_addr[1] = CID_server_addr[0]+n+1;
    CID_server_addr[0][n] = 0;
    /* Se il secondo IP è vuoto, è come se non fosse impostato.
       Es:  X;ETH;CIDAS;4101;192.168.10.200:
    */
    if(strlen(CID_server_addr[1]) == 0) CID_server_addr[1] = NULL;
  }
  else
    CID_server_addr[1] = NULL;
  
  CID_sending = CID_send_to = CID_send_retry = 0;
  CID_SMS_limit = 0;
  
  strcpy(ifr.ifr_name, "eth0");
  fd[0] = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
  ioctl(fd[0], SIOCGIFHWADDR, &ifr);
  memcpy(MACaddr, ifr.ifr_hwaddr.sa_data, 6);
  
  /* Attende 10 secondi all'avvio prima di iniziare a spedire,
     in modo da stabilizzare rete e DHCP. */
  sleep(10);
  
  sa.sin_family = AF_INET;
  sa.sin_port = htons(CID_server_port);
  //sa.sin_addr.s_addr = INADDR_ANY;
  //bind(fd[0], (struct sockaddr*)&sa, sizeof(struct sockaddr));
  host = gethostbyname(CID_server_addr[0]);
  //sa.sin_addr.s_addr = inet_addr(CID_server_addr[0]);
  sa.sin_addr = *(struct in_addr*)(host->h_addr);
  connect(fd[0], (struct sockaddr*)&sa, sizeof(struct sockaddr));
  
  if(CID_server_addr[1])
  {
    fd[1] = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
    //bind(fd[1], (struct sockaddr*)&sa, sizeof(struct sockaddr));
    //sa.sin_addr.s_addr = inet_addr(CID_server_addr[1]);
    host = gethostbyname(CID_server_addr[1]);
    sa.sin_addr = *(struct in_addr*)(host->h_addr);
    connect(fd[1], (struct sockaddr*)&sa, sizeof(struct sockaddr));
  }
  else
    fd[1] = -1;
  
  to.tv_sec = 0;
  to.tv_usec = 100000;
  FD_ZERO(&fds);
  while(1)
  {
    FD_SET(fd[0], &fds);
    if(fd[1] >= 0)
    {
      FD_SET(fd[1], &fds);
      n = select(fd[1]+1, &fds, NULL, NULL, &to);
    }
    else
      n = select(fd[0]+1, &fds, NULL, NULL, &to);
    
    if(!n)
    {
      /* Alla mezzanotte di ogni giorno azzero il limite sms */
      t = time(NULL);
      if((t % 86400) == 0) CID_SMS_limit = 0;
      
      to.tv_sec = 0;
      to.tv_usec = 100000;
      
      if((CID_sending == 1) && CID_send_to)
      {
        /* Gestione invio LAN */
        CID_send_to--;
        if(!CID_send_to)
        {
          if(CID_send_retry < 2)
          {
            CID_send_retry++;
            CID_send_to = 20;
            send(fd[0], CIDlastev, 16, 0);
          }
          else if(CID_server_addr[1] && (CID_send_retry < 6))
          {
            CID_send_retry++;
            CID_send_to = 20;
            send(fd[1], CIDlastev, 16, 0);
          }
          else
          {
            /* Attiva il backup GPRS */
support_log("ContactID (AS): Attivazione backup GPRS da LAN");
            CID_send_retry = 0;
            //if(CID_sending != 2)
            {
              CID_sending = 2;	// backup gprs
              gsm_gprs_connect_ex(CID_AS_invio, CID_server_addr[0], CID_server_port, GPRS_INIT_UDP, NULL);
            }
          }
        }
      }
      else if(CID_sending == 4)
      {
        /* E' richiesto l'invio dell'evento via SMS */
        
        /* Se non è già stato codificato (es: manca LAN e GPRS non si connette)
           lo devo codificare ora per poter avanzare il contatore CIDout. */
        if(!CIDlastev[8] && (CIDout != CIDin) /*CIDev[CIDout][4]*/)
        {
          CID_create_ev();
        }
        
        /* Tranne per gli stati in vita! */
        if((CIDbackup > 0) && (CIDlastev[9] != 0x16) && (CIDlastev[10] != 0xa2) &&
           /* Sono capitati degli invii di sms con testo nullo */
           (CIDlastev[9] != 0) && (CIDlastev[10] != 0) && CID_SMS_limit < CID_SMS_LIMIT)
        {
          support_log("ContactID (AS): backup sms");
          for(n=0; n<16; n++)
            sprintf(buf+n*2, "%02X", CIDlastev[n]);
          GSM_SMS_send(CIDbackup, buf);
          CID_SMS_limit++;
        }
        else if((CIDlastev[9] == 0) && (CIDlastev[10] == 0))
        {
          support_log("ContactID (AS): backup sms NULLO!");
        }
        memset(CIDlastev, 0, 16);
        CID_sending = 0;
      }
      
      if(CIDkeepalive) CIDkeepalive--;
      /* Non invio un nuovo stato in vita se sto già tentanto di inviarne uno. */
      if(!CIDkeepalive && ((CIDlastev[9] != 0x16) || (CIDlastev[10] != 0xa2)))
      {
        CID_AS_message(1, CIDbackup, 1602, CID_CH_LAN, CIDkeepalivebase/600);
        CIDkeepalive = CIDkeepalivebase;
      }
      
      CIDwatchdog++;
      if(CIDwatchdog > CIDkeepalivebase*6)
      {
        /* RESET CENTRALE */
        support_log("ContactID (AS): WATCHDOG");
        exit(0);
      }
    }
    else if(n > 0)
    {
      if(FD_ISSET(fd[0], &fds))
      {
        n = recv(fd[0], buf, 16, 0);
        if((n == 16) && cid_parse(buf, CID_CH_LAN))
          send(fd[0], buf, 16, 0);
      }
      if((fd[1] >= 0) && FD_ISSET(fd[1], &fds))
      {
        n = recv(fd[1], buf, 16, 0);
        if((n == 16) && cid_parse(buf, CID_CH_LAN))
          send(fd[1], buf, 16, 0);
      }
    }
    
    if(GSM_Squillo_sconosciuto())
    {
      /* Devo buttare giù la linea ed inviare uno stato in vita via GPRS. */
      support_log("ContactID (AS): callback");
      GSM_Call_refuse();
      CID_AS_message(1, CIDbackup, 1602, CID_CH_CALLBACK, CIDkeepalivebase/600);
      CIDkeepalive = CIDkeepalivebase;
      if(CID_sending != 2)
      {
        CID_sending = 2;	// backup gprs
        /* Se non ho già un evento in corso di invio, preparo quello appena accodato. */
        if(!CIDlastev[8]) CID_create_ev();
        gsm_gprs_connect_ex(CID_AS_invio, CID_server_addr[0], CID_server_port, GPRS_INIT_UDP, NULL);
      }
    }
    
    pthread_mutex_lock(&CIDmux);
    if(!CID_sending && (CIDout != CIDin))
    {
      CID_create_ev();
      
      ioctl(fd[0], SIOCGIFFLAGS, &ifr);
      if(ifr.ifr_flags & IFF_RUNNING)
      {
        /* LAN connessa */
        send(fd[0], CIDlastev, 16, 0);
        CID_sending = 1;
        CID_send_to = 20;
      }
      else
      {
        /* Attivo subito il GPRS */
support_log("ContactID (AS): Attivazione immediata backup GPRS");
        CID_sending = 2;	// backup gprs
        gsm_gprs_connect_ex(CID_AS_invio, CID_server_addr[0], CID_server_port, GPRS_INIT_UDP, NULL);
      }
    }
    pthread_mutex_unlock(&CIDmux);
  }
}

void _init()
{
  printf("CEI-ABI (plugin, Proteus GPRS): " __DATE__ " " __TIME__ "\n");
  prot_plugin_register("CEI", 0, NULL, NULL, (PthreadFunction)cei_ca_loop);
  /* Se configuro il consumatore GSM normale, il backup avviene via GPRS,
     se configuro il consumatore CEIB, il backup avviene su connessione dati punto punto,
     se non configuro nulla allora niente backup. */
  prot_plugin_register("CEIB", 0, (InitFunction)cei_backup_timer, NULL, (PthreadFunction)cei_backup_loop);
  
  /* Unifico in un unico plugin tutte le personalizzazioni AllSystem */
  /* Quindi aggiungo le funzioni ContactID su UDP del loro protocollo. Il consumatore
     è necessario per definire IP e porta del centro. */
  prot_plugin_register("CIDAS", 0, /*(InitFunction)cid_timer*/NULL, NULL, (PthreadFunction)cid_loop);
}

