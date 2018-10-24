#include "protocol.h"
#include "codec.h"
#include "serial.h"
#include "support.h"
#include "user.h"
#include "ronda.h"
#include "database.h"
#include "cei_imq.h"
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

#define DEBUG

char* cmdtoascii(unsigned char *cmd, int asc, int hex)
{
  static char buf[256];
  int i, j;
  
  memcpy(buf, cmd, asc);
  j = asc;
  for(i=0; i<hex; i++)
  {
    buf[j++] = '#';
    buf[j++] = (cmd[asc+i]>>4)>9?(cmd[asc+i]>>4)+'A'-'0':(cmd[asc+i]>>4)+'0';
    buf[j++] = (cmd[asc+i]&0xf)>9?(cmd[asc+i]&0xf)+'A'-'0':(cmd[asc+i]&0xf)+'0';
  }
  buf[j] = 0;
  
  return buf;
}

#define EPT

//#undef FEAL_MK_VAL
//#define FEAL_MK_VAL	"01234567"

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
  
  ev->header.formato = 1;	// Formato ABI v1.3
  ev->header.versione = 3;
  ev->header.ept = 0;
  ev->header.processo = 1;	// supervisione e controllo
  ev->header.sorgente = 4;	// sorgente CSC
  ev->header.tipoinfo = 5;	// segnalazione comandi locali
  ev->header.numeromessaggio = CEI->L4_pktnum[0]++;
  ev->tempoaccadimento = cei_tempoaccadimento();
  
  if(classe == CLS_ZONA)
    ev->codicecomando = 2;
  else
    ev->codicecomando = 4;
  ev->esito = 3;	// comando non eseguibile
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
#warning LOG
{
FILE *fp;
fp = fopen("/tmp/CEI_tlc.log", "a");
fprintf(fp, "%02d/%02d/%02d %02d:%02d:%02d - %s\n", GIORNO, MESE, ANNO, ORE, MINUTI, SECONDI, cmd);
fclose(fp);
}
  codec_parse_cmd(cmd, 9, config.consumer[i].dev);
  sprintf(cmd, "/7%02d30000", fascia);
#warning LOG
{
FILE *fp;
fp = fopen("/tmp/CEI_tlc.log", "a");
fprintf(fp, "%02d/%02d/%02d %02d:%02d:%02d - %s\n", GIORNO, MESE, ANNO, ORE, MINUTI, SECONDI, cmd);
fclose(fp);
}
  codec_parse_cmd(cmd, 9, config.consumer[i].dev);
}

#include "cei_common.c"

#define ALM_		0
#define CLS_		0
#define NOT_IMPL	-1

struct _cei_event {short tipoevento; char transizione, classeelemento, numbytes;};

static struct _cei_event cei_event_table[MAX_NUM_EVENT] = {
{ALM_ALLARME, ALM_INIZIO, CLS_SENSORE, 2}, /* Allarme Sensore */
{ALM_ALLARME, ALM_FINE, CLS_SENSORE, 2}, /* Ripristino Sensore */
{ALM_GUASTO, ALM_INIZIO, CLS_UNITA_SATELLITE, 1}, /* Guasto Periferica */
{ALM_GUASTO, ALM_FINE, CLS_UNITA_SATELLITE, 1}, /* Fine Guasto Periferica */
{ALM_MANOMISSIONE, ALM_INIZIO, CLS_SENSORE, 2}, /* Manomissione Dispositivo */
{ALM_MANOMISSIONE, ALM_FINE, CLS_SENSORE, 2}, /* Fine Manomis Dispositivo */
{ALM_MANOMISSIONE, ALM_INIZIO, CLS_UNITA_SATELLITE, 1}, /* Manomissione Contenitore */
{ALM_MANOMISSIONE, ALM_FINE, CLS_UNITA_SATELLITE, 1}, /* Fine Manomis Contenitore */
{ALM_DISINSERITO, ALM_FINE, CLS_ZONA, 1}, /* Attivata zona */
{ALM_DISINSERITO, ALM_INIZIO, CLS_ZONA, 1}, /* Disattivata zona */
{ALM_DISINSERITO, ALM_PERSIST, CLS_ZONA, 1}, /* Attivazione impedita */
{ALM_DISABILITATO, ALM_FINE, CLS_SENSORE, 2}, /* Sensore In Servizio */
{ALM_DISABILITATO, ALM_INIZIO, CLS_SENSORE, 2}, /* Sensore Fuori Servizio */
{ALM_DISABILITATO, ALM_FINE, CLS_ATTUATORE, 2}, /* Attuatore In Servizio */
{ALM_DISABILITATO, ALM_INIZIO, CLS_ATTUATORE, 2}, /* Attuatore Fuori Servizio */
#if 0
{ALM_ERRORE_PROGR, ALM_INIZIO, CLS_UNITA_SATELLITE, 1}, /* Ricezione codice errato */
#else
{ALM_, ALM_, CLS_, NOT_IMPL}, /* Ricezione codice errato */
#endif
{ALM_, ALM_, CLS_, NOT_IMPL}, /* Tipo ModuloMaster */
{ALM_, ALM_, CLS_, NOT_IMPL}, /* Periferica Incongruente */
{ALM_MANOMISSIONE, ALM_INIZIO, CLS_UNITA_SATELLITE, 1}, /* Periferica Manomessa */
{ALM_MANOMISSIONE, ALM_FINE, CLS_UNITA_SATELLITE, 1}, /* Periferica Ripristino */
{ALM_, ALM_, CLS_, NOT_IMPL}, /* Sospesa attivita linea */
{ALM_ACCESSO_NEGATO, ALM_INIZIO, CLS_ORGANO_COMANDO, 1}, /* Chiave Falsa */
{ALM_DISABILITATO, ALM_FINE, CLS_ORGANO_COMANDO, 1}, /* Sentinella on */
{ALM_DISABILITATO, ALM_INIZIO, CLS_ORGANO_COMANDO, 1}, /* Sentinella off */
{ALM_DISABILITATO, ALM_INIZIO, CLS_ORGANO_COMANDO, 1}, /* Sentinella off timeout */
#if 0
{ALM_GUASTO_IF, ALM_INIZIO, CLS_UNITA_SATELLITE, 1}, /* No ModuloMasterTx */
{ALM_GUASTO_IF, ALM_INIZIO, CLS_UNITA_SATELLITE, 1}, /* ErrRx ModuloMaster */
{ALM_ERRORE_PROGR, ALM_INIZIO, CLS_UNITA_CENTRALE, 0}, /* Errore messaggio host */
#else
{ALM_, ALM_, CLS_, NOT_IMPL}, /* No ModuloMasterTx */
{ALM_, ALM_, CLS_, NOT_IMPL}, /* ErrRx ModuloMaster */
{ALM_, ALM_, CLS_, NOT_IMPL}, /* Errore messaggio host */
#endif
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
#if 0
{ALM_ATTIVAZIONE, ALM_INIZIO, CLS_UNITA_CENTRALE, 2}, /* ON telecomando */
#else
{ALM_, ALM_, CLS_, NOT_IMPL}, /* ON telecomando */
#endif
{ALM_CONFIGURAZIONE, ALM_INIZIO, CLS_UNITA_SATELLITE, 1}, /* Parametri taratura */
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
{ALM_, ALM_, CLS_, NOT_IMPL}, /* ON attuatore */
{ALM_, ALM_, CLS_, NOT_IMPL}, /* Transitato Ident */
{ALM_, ALM_, CLS_, NOT_IMPL}, /* Entrato Ident */
{ALM_, ALM_, CLS_, NOT_IMPL}, /* Uscito Ident */
{ALM_, ALM_, CLS_, NOT_IMPL}, /* Codice valido */
{ALM_, ALM_, CLS_, NOT_IMPL}, /* Chiave valida */
#if 0
{ALM_ACCESSO_OPER, ALM_INIZIO, CLS_OPERATORE, 1}, /* Operatore */
#else
{ALM_, ALM_, CLS_, NOT_IMPL}, /* Operatore */
#endif
{ALM_, ALM_, CLS_, NOT_IMPL}, /* Punzonatura */
{ALM_, ALM_, CLS_, NOT_IMPL}, /* Spegnimento */
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
#warning --- A cosa servono? ---
{ALM_DISINSERITO, ALM_FINE, CLS_UNITA_SATELLITE, 1}, /* Governo_linee_in_servizio */
{ALM_DISINSERITO, ALM_INIZIO, CLS_UNITA_SATELLITE, 1}, /* Governo_linee_fuori_servizio */
};

static struct _cei_event cei_event_table_ex2[MAX_NUM_EVENT_EX2] = {
{ALM_TEST_ATTIVO, ALM_INIZIO, CLS_RAGGRUPPAMENTO, 1}, /* Inizio test attivo */
{ALM_TEST_ATTIVO, ALM_FINE, CLS_RAGGRUPPAMENTO, 1}, /* Fine test attivo */
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
{ALM_ATTIVAZIONE, ALM_INIZIO, CLS_ATTUATORE, 2}, /* On attuatore */
{ALM_ATTIVAZIONE, ALM_FINE, CLS_ATTUATORE, 2}, /* Off attuatore */
{ALM_, ALM_, CLS_, NOT_IMPL},
{ALM_, ALM_, CLS_, NOT_IMPL},
{ALM_, ALM_, CLS_, NOT_IMPL},
{ALM_STATO, ALM_, CLS_, 0},	/* Allineamento */
};

static void cei_event_set_alarm(CEI_allarme *alarm, Event *ev, int tempoaccadimento)
{
  alarm->header.formato = 1;	// Formato ABI v1.3
  alarm->header.versione = 3;
  
  alarm->header.processo = 1;	// supervisione e controllo
  alarm->header.sorgente = 0;
  alarm->header.tipoinfo = 1;	// allarmi e stati
  
  alarm->tempoaccadimento = tempoaccadimento;
  alarm->areaapplicativa = 0;
  alarm->tiporivelatore = 0;
  alarm->categoria = 1;	// non richiede azioni da CSC
  alarm->areaprovenienza = 0;
  
  alarm->transizione = cei_event_table[ev->Event[8]-EV_OFFSET].transizione;
  alarm->tipoeventomsb = cei_event_table[ev->Event[8]-EV_OFFSET].tipoevento >> 8;
  alarm->tipoeventolsb = cei_event_table[ev->Event[8]-EV_OFFSET].tipoevento;
  alarm->classeelemento = cei_event_table[ev->Event[8]-EV_OFFSET].classeelemento;
  
#ifdef EPT
  if(cei_event_table[ev->Event[8]-EV_OFFSET].tipoevento >= 4000)
    alarm->header.ept = 1;
  else
#endif
    alarm->header.ept = 0;
  
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
  
  /* Le segnalazione della periferica 0 diventano segnalazioni della centrale. */
  if((alarm->classeelemento == CLS_UNITA_SATELLITE) &&
     !alarm->numeroelementolsb && !alarm->numeroelementomsb)
    alarm->classeelemento = CLS_UNITA_CENTRALE;
  
  /* La classe centrale forza il numero di elemento a 1. */
  if((alarm->classeelemento == CLS_UNITA_CENTRALE) &&
     /*(alarm->tipoevento != ALM_TEST_ATTIVO) &&*/
     ((alarm->tipoeventomsb*256+alarm->tipoeventolsb) != ALM_RONDA))
    alarm->numeroelementolsb = 1;
}

static void cei_event_set_alarm_ex2(CEI_allarme *alarm, Event *ev, int tempoaccadimento)
{
  alarm->header.formato = 1;	// Formato ABI v1.3
  alarm->header.versione = 3;
  
  alarm->header.processo = 1;	// supervisione e controllo
  alarm->header.sorgente = 0;
  alarm->header.tipoinfo = 1;	// allarmi e stati
  
  alarm->tempoaccadimento = tempoaccadimento;
  alarm->areaapplicativa = 0;
  alarm->tiporivelatore = 0;
  alarm->categoria = 1;	// non richiede azioni da CSC
  alarm->areaprovenienza = 0;
  
  alarm->transizione = cei_event_table_ex2[ev->Event[10]].transizione;
  alarm->tipoeventomsb = cei_event_table_ex2[ev->Event[10]].tipoevento >> 8;
  alarm->tipoeventolsb = cei_event_table_ex2[ev->Event[10]].tipoevento;
  alarm->classeelemento = cei_event_table_ex2[ev->Event[10]].classeelemento;
    
#ifdef EPT
  if(cei_event_table_ex2[ev->Event[10]].tipoevento >= 4000)
    alarm->header.ept = 1;
  else
#endif
    alarm->header.ept = 0;
  
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
  
  if((alarm->tipoeventomsb*256+alarm->tipoeventolsb) == ALM_TEST_ATTIVO)
  {
    alarm->numeroelementolsb++;
    if(!alarm->numeroelementolsb) alarm->numeroelementomsb++;
  }
  else if(((alarm->tipoeventomsb*256+alarm->tipoeventolsb) == ALM_ATTIVAZIONE) &&
          !alarm->numeroelementomsb && !alarm->numeroelementolsb)
  /*
     Le movimentazioni dell'attuatore 0 (buzzer di centrale) vengono
     fatte comunque passare ma in classe SEGNALAZIONE.
  */
    alarm->classeelemento = CLS_SEGNALAZIONE;
}

static int cei_event_to_l4cei(ProtDevice *dev, Event *ev, unsigned char **pkt)
{
  int i, len, num, tempoaccadimento;
  unsigned char bitmask;
  short tipoevento;
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
  if((ev->Event[8] == Evento_Esteso2) && (cei_event_table_ex2[ev->Event[10]].numbytes == NOT_IMPL))
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
    if(ev->Event[9] == Ex2_Delphi)
    {
      if(ev->Event[10] < MAX_NUM_EVENT_EX2)
      {
        if(cei_event_table_ex2[ev->Event[10]].tipoevento == ALM_STATO)
        {
          if(ev->Event[11] == 0)	// fine lista
            num = 1;
          else if(ev->Event[11] == 1)	// sensori
            num = 56;
          else if(ev->Event[11] == 2)	// zone
            num = 32;
          else if(ev->Event[11] == 3)	// attuatori
            num = 16;
          else
            num = 0;
          
          len = sizeof(CEI_stato_header) + sizeof(CEI_stato) * num;
          if(!CEI->lista)
          {
            len += sizeof(CEI_stato);
            CEI->lista = 1;	// inizio allineamento
          }
          header = calloc(1, len);
          if(!header) return -1;
          stato = (void*)header + sizeof(CEI_stato_header);
    
          header->header.formato = 1;	// Formato ABI v1.3
          header->header.versione = 3;
          header->header.processo = 2;	// trasferimento stati per allineamento
          header->header.numeromessaggio = CEI->L4_pktnum[1]++;
          header->header.sorgente = 0;
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
            if(ev->Event[11] == 0)
            {
              CEI->lista = 0;	// fine allineamento
              break;
            }
            else if(ev->Event[11] == 1)	// sensori
            {
#ifdef EPT
              header->header.ept = 1;
#endif
              switch(i % 7)
              {
                case 0:
                  bitmask = bitSyncAlarm;
                  tipoevento = ALM_ALLARME;
                  break;
                case 1:
                  bitmask = bitSyncSabotage;
                  tipoevento = ALM_MANOMISSIONE;
                  break;
                case 2:
                  bitmask = bitSyncFailure;
                  tipoevento = ALM_GUASTO;
                  break;
                case 3:
                  bitmask = bitSyncOOS;
                  tipoevento = ALM_DISABILITATO;
                  break;
                case 4:
                  bitmask = bitSyncMuAlarm;
                  tipoevento = ALM_ACC_ALLARME;
                  break;
                case 5:
                  bitmask = bitSyncMuSabotage;
                  tipoevento = ALM_ACC_MANOMISSIONE;
                  break;
                case 6:
                  bitmask = bitSyncMuFailure;
                  tipoevento = ALM_ACC_GUASTO;
                  break;
                default:
                  bitmask = 0;
                  tipoevento = 0;
                  break;
              }
              stato->transizione = (ev->Event[14 + (i/7)] & bitmask)?ALM_PERSIST:ALM_FINE;
              stato->tipoeventomsb = tipoevento >> 8;
              stato->tipoeventolsb = tipoevento & 0xff;
              stato->classeelemento = CLS_SENSORE;
              stato->numeroelementomsb = ev->Event[13];
              stato->numeroelementolsb = ev->Event[12];
            }
            else if(ev->Event[11] == 2)	// zone
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
                default:
                  bitmask = 0;
                  tipoevento = 0;
                  break;
              }
              if((i%4) == 3)
                stato->transizione = (ev->Event[14 + (i/4)] & bitmask)?ALM_FINE:ALM_PERSIST;
              else
                stato->transizione = (ev->Event[14 + (i/4)] & bitmask)?ALM_PERSIST:ALM_FINE;
              
              stato->tipoeventomsb = tipoevento>>8;
              stato->tipoeventolsb = tipoevento;
              stato->classeelemento = CLS_ZONA;
              stato->numeroelementomsb = ev->Event[13];
              stato->numeroelementolsb = ev->Event[12];
            }
            else if(ev->Event[11] == 3)	// attuatori
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
                default:
                  bitmask = 0;
                  tipoevento = 0;
                  break;
              }
              stato->transizione = (ev->Event[14 + (i/2)] & bitmask)?ALM_PERSIST:ALM_FINE;
              stato->tipoeventomsb = tipoevento>>8;
              stato->tipoeventolsb = tipoevento;
              stato->classeelemento = CLS_ATTUATORE;
              stato->numeroelementomsb = ev->Event[13];
              stato->numeroelementolsb = ev->Event[12];
            }
            
            if(num > 1) stato->numeroelementolsb += i / (num/8);
        
            /* Il sensore 0 e la zona 0 non producono stato. */
            if(stato->numeroelementolsb || stato->numeroelementomsb)
              stato++;
            else
              len -= sizeof(CEI_stato);
          }
          *pkt = (unsigned char*)header;
        }
        else
        {
          len = sizeof(CEI_allarme);
          alarm = calloc(1, len);
          if(!alarm) return -1;
          
          cei_event_set_alarm_ex2(alarm, ev, tempoaccadimento);
          
          if((alarm->classeelemento == CLS_SEGNALAZIONE) ||
             alarm->numeroelementolsb || alarm->numeroelementomsb)
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
      }
      else
        return -1;
    }
    else
      return -1;
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
    
    header->header.formato = 1;	// Formato ABI v1.3
    header->header.versione = 3;
    header->header.processo = 2;	// trasferimento stati per allineamento
    header->header.numeromessaggio = CEI->L4_pktnum[1]++;
    header->header.sorgente = 0;
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
        stato[i].tipoeventomsb = 0;
        stato[i].tipoeventolsb = 0;
        CEI->lista = 0;	// fine allineamento
      }
      else if(ev->Event[8] == Lista_MU)
      {
#ifdef EPT
        header->header.ept = 1;
#endif
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
          default:
            bitmask = 0;
            tipoevento = 0;
            break;
        }
        stato->transizione = (ev->Event[11 + (i/3)] & bitmask)?ALM_PERSIST:ALM_FINE;
        stato->tipoeventomsb = tipoevento >> 8;
        stato->tipoeventolsb = tipoevento & 0xff;
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
            default:
              bitmask = 0;
              tipoevento = 0;
              break;
          }
/*
          if((i%4) == 3)
            stato->transizione = (ev->Event[10 + (i/4)] & bitmask)?ALM_INIZIO:ALM_PERSIST;
          else
            stato->transizione = (ev->Event[10 + (i/4)] & bitmask)?ALM_PERSIST:ALM_INIZIO;
*/
          if((i%4) == 3)
            stato->transizione = (ev->Event[10 + (i/4)] & bitmask)?ALM_FINE:ALM_PERSIST;
          else
            stato->transizione = (ev->Event[10 + (i/4)] & bitmask)?ALM_PERSIST:ALM_FINE;
          
          stato->tipoeventomsb = tipoevento>>8;
          stato->tipoeventolsb = tipoevento;
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
            default:
              bitmask = 0;
              tipoevento = 0;
              break;
          }
//          stato->transizione = (ev->Event[11 + (i/4)] & bitmask)?ALM_PERSIST:ALM_INIZIO;
          stato->transizione = (ev->Event[11 + (i/4)] & bitmask)?ALM_PERSIST:ALM_FINE;
          stato->tipoeventomsb = tipoevento>>8;
          stato->tipoeventolsb = tipoevento;
          stato->classeelemento = cei_event_table[ev->Event[8]-EV_OFFSET].classeelemento;
          stato->numeroelementomsb = ev->Event[9];
          stato->numeroelementolsb = ev->Event[10];
        }
      }
      
      if(num > 1) stato->numeroelementolsb += i / (num/8);
      
      /* Il sensore 0 e la zona 0 non producono stato. */
      if(stato->numeroelementolsb || stato->numeroelementomsb) stato++;
    }
    *pkt = (unsigned char*)header;
  }
  else
  {
    len = sizeof(CEI_allarme);
    alarm = calloc(1, len);
    if(!alarm) return -1;

    cei_event_set_alarm(alarm, ev, tempoaccadimento);
    if(ev->Event[8] == Stato_prova)
      alarm->transizione = ev->Event[9];
    
    if(ev->Event[8] == Evento_Sensore)
    {
      alarm->tipoeventomsb = (ALM_ACC_ALLARME - 1 + ev->Event[11]) >> 8;
      alarm->tipoeventolsb = (ALM_ACC_ALLARME - 1 + ev->Event[11]) & 0xff;
      if((ev->Event[12] == 3) || (ev->Event[12] == 5)) alarm->transizione = ALM_FINE;
      if(ev->Event[12] == 4) alarm->transizione = ALM_PERSIST;
    }
    else if(ev->Event[8] == Segnalazione_evento)
    {
      cei_event_list *tevlist;
      
      alarm->areaapplicativa = 7;	// ronda
      alarm->transizione = ALM_INIZIO;
      
      num = (alarm->numeroelementomsb<<8) + alarm->numeroelementolsb;
      alarm->numeroelementomsb = 0;
      alarm->numeroelementolsb = 1;
      
      switch(num)
      {
        case 1000:
          /* Preallarme ridondanza? */
          alarm->tipoeventomsb = 0;
          alarm->tipoeventolsb = 1;	// preallarme
          alarm->areaapplicativa = 0;
          break;
        case 0:
          alarm->tipoeventomsb = 0;
          alarm->tipoeventolsb = ALM_R_GIRO;
          alarm->transizione = ALM_FINE;
          /* Carica il percorso per cui era stata richiesta la partenza. */
          alarm->numeroelementolsb = CEI->percorso;
          break;
        case 1:
          /* Carica il percorso per cui era stata richiesta la partenza. */
          alarm->numeroelementolsb = CEI->percorso;
          
          /* Crea l'evento di inizio giro (accodato) */
          pthread_mutex_lock(&cei_evlistmutex);
          tevlist = malloc(sizeof(cei_event_list));
          tevlist->next = CEI->evlist;
          CEI->evlist = tevlist;
          tevlist->event = malloc(len);
          memcpy(tevlist->event, alarm, len);
          ((CEI_allarme*)(tevlist->event))->tipoeventomsb = 0;
          ((CEI_allarme*)(tevlist->event))->tipoeventolsb = ALM_R_GIRO;
          tevlist->len = len;
          pthread_mutex_unlock(&cei_evlistmutex);
          
          /* Crea l'evento di fine richiesta partenza */
          alarm->tipoeventomsb = 0;
          alarm->tipoeventolsb = ALM_R_PARTENZA;
          alarm->transizione = ALM_FINE;
          break;
        case 2:
          /* Carica il percorso per cui era stata richiesta la partenza. */
          alarm->numeroelementolsb = CEI->percorso;
          
          alarm->tipoeventomsb = 0;
          alarm->tipoeventolsb = ALM_R_PARTENZA;
          alarm->transizione = ALM_PERSIST;
          break;
        default:
          alarm->classeelemento = CLS_STAZIONE_RONDA;
          alarm->numeroelementolsb = (num%100)+1;
          alarm->tipoeventomsb = 0;
          switch(num/100)
          {
            case 1:
              alarm->tipoeventolsb = ALM_R_PARTENZA;
              alarm->classeelemento = CLS_UNITA_CENTRALE;
              CEI->percorso = alarm->numeroelementolsb;
              break;
            case 2:
              alarm->tipoeventolsb = ALM_R_PUNZONATURA;
              break;
            case 3:
              alarm->tipoeventolsb = ALM_R_MANCATA_PUNZ;
              break;
            case 4:
              alarm->tipoeventomsb = ALM_R_RIP_MANC_PUNZ >> 8;
              alarm->tipoeventolsb = ALM_R_RIP_MANC_PUNZ & 0xff;
              break;
            case 5:
              alarm->tipoeventolsb = ALM_R_FUORI_TEMPO;
              break;
            case 6:
              alarm->tipoeventolsb = ALM_R_SEQUENZA;
              break;
            case 7:
              alarm->tipoeventolsb = ALM_R_CHIAVE_FALSA;
              break;
            case 8:
              alarm->tipoeventolsb = ALM_R_SOS_RONDA;
              break;
            default:
              free(alarm);
              return -1;    
              break;
          }
          break;
      }
    }
    else if(ev->Event[8] == Parametri_taratura)
    {
      cei_event_list *tevlist;
      
      pthread_mutex_lock(&cei_evlistmutex);
    
      /* Crea l'evento di fine configurazione */
      tevlist = malloc(sizeof(cei_event_list));
      tevlist->next = CEI->evlist;
      CEI->evlist = tevlist;
      tevlist->event = malloc(len);
      memcpy(tevlist->event, alarm, len);
      ((CEI_allarme*)(tevlist->event))->transizione = 0;
      tevlist->len = len;
      
      pthread_mutex_unlock(&cei_evlistmutex);
    }
    
    if(!alarm->numeroelementolsb && !alarm->numeroelementomsb)
    {
      free(alarm);
      return -1;
    }
    
    alarm->header.numeromessaggio = CEI->L4_pktnum[0]++;
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
              if((((CEI_telecomando*)(CEI->L4))->classeelemento == CLS_ZONA) &&
                 (elemento < (1 + n_ZS + n_ZI)))
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
              if((((CEI_telecomando*)(CEI->L4))->classeelemento == CLS_ZONA) &&
                 (elemento < (1 + n_ZS + n_ZI)))
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
              else if(((CEI_telecomando*)(CEI->L4))->classeelemento == CLS_SEGNALAZIONE)
              {
                memcpy(cmd, "K279", 5);
                len = 4;
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
            case 12:
              if(((CEI_telecomando*)(CEI->L4))->classeelemento == CLS_OPERATORE)
              {
                memcpy(cmd, "K051", 5);
                len = 4;
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
                  cmd[0] = 'H';
                else
                  ret = 0;
              }
              break;
            case 21:
              if(((CEI_telecomando*)(CEI->L4))->classeelemento == CLS_SENSORE)
              {
                memcpy(cmd, "/389", 5);
                len = 4;
              }
              else if(((CEI_telecomando*)(CEI->L4))->classeelemento == CLS_ATTUATORE)
              {
                if(!elemento)
                  cmd[0] = 'G';
                else
                  ret = 0;
              }
              else if(((CEI_telecomando*)(CEI->L4))->classeelemento == CLS_OPERATORE)
              {
                memcpy(cmd, "K050", 5);
                len = 4;
              }
              break;
            case 201:
              if(((CEI_telecomando*)(CEI->L4))->classeelemento == CLS_UNITA_CENTRALE)
              {
                sprintf(cmd, "K%04d", elemento);
                len = 5;
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
                cmd[4] = '0' - 1 + ((CEI_telecomando*)(CEI->L4))->numeroelementolsb;
                len = 5;
              }
              break;
            case 209:
              if(((CEI_telecomando*)(CEI->L4))->classeelemento == CLS_UNITA_CENTRALE)
              {
                memcpy(cmd, "/551", 4);
                cmd[4] = '0' - 1 + ((CEI_telecomando*)(CEI->L4))->numeroelementolsb;
                len = 5;
              }
              break;
            case 210:
              if(((CEI_telecomando*)(CEI->L4))->classeelemento == CLS_UNITA_CENTRALE)
              {
                memcpy(cmd, "/552", 4);
                cmd[4] = '0' - 1 + ((CEI_telecomando*)(CEI->L4))->numeroelementolsb;
                len = 5;
              }
              break;
            case 211:
              if(((CEI_telecomando*)(CEI->L4))->classeelemento == CLS_UNITA_CENTRALE)
              {
                memcpy(cmd, "K638", 5);
                len = 4;
              }
              break;
            case 212:
              if(((CEI_telecomando*)(CEI->L4))->classeelemento == CLS_UNITA_CENTRALE)
              {
                memcpy(cmd, "K639", 5);
                len = 4;
              }
              break;
          }
          if(cmd[0])
          {
#warning LOG
{
FILE *fp;
fp = fopen("/tmp/CEI_tlc.log", "a");
cmd[len] = 0;
fprintf(fp, "%02d/%02d/%02d %02d:%02d:%02d - %s\n", GIORNO, MESE, ANNO, ORE, MINUTI, SECONDI, cmd);
fclose(fp);
}
            ret = codec_parse_cmd(cmd, len, dev);
          }
        }
        else
          ret = -2;
        
        risp.header.formato = 1;	// Formato ABI v1.3
        risp.header.versione = 3;
        risp.header.ept = 0;
        risp.header.processo = 1;	// supervisione e controllo
        risp.header.sorgente = 0;
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
          if(cmd[0])
          {
#warning LOG
{
FILE *fp;
fp = fopen("/tmp/CEI_tlc.log", "a");
cmd[len] = 0;
fprintf(fp, "%02d/%02d/%02d %02d:%02d:%02d - %s\n", GIORNO, MESE, ANNO, ORE, MINUTI, SECONDI, cmd);
fclose(fp);
}
            ret = codec_parse_cmd(cmd, len, dev);
          }
        }
        else
          ret = -2;
        
        risp.header.formato = 1;	// Formato ABI v1.3
        risp.header.versione = 3;
        risp.header.ept = 0;
        risp.header.processo = 1;	// supervisione e controllo
        risp.header.sorgente = 0;
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
              if(((CEI_telecomando*)(CEI->L4))->classeelemento == CLS_NONE)
                cmd[0] = 'x';
              else if(((CEI_telecomando*)(CEI->L4))->classeelemento == CLS_SENSORE)
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
          if(cmd[0])
          {
#warning LOG
{
FILE *fp;
fp = fopen("/tmp/CEI_tlc.log", "a");
cmd[len] = 0;
fprintf(fp, "%02d/%02d/%02d %02d:%02d:%02d - %s\n", GIORNO, MESE, ANNO, ORE, MINUTI, SECONDI, cmd);
fclose(fp);
}
            ret = codec_parse_cmd(cmd, len, dev);
          }
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
#warning LOG
{
FILE *fp;
fp = fopen("/tmp/CEI_tlc.log", "a");
fprintf(fp, "%02d/%02d/%02d %02d:%02d:%02d - %s\n", GIORNO, MESE, ANNO, ORE, MINUTI, SECONDI, cmd);
fprintf(fp, "%02d/%02d/%02d %02d:%02d:%02d - %s\n", GIORNO, MESE, ANNO, ORE, MINUTI, SECONDI, cmd2);
fclose(fp);
}
          codec_parse_cmd(cmd, 8, dev);
          cei_L4_send_packet(dev, NULL, 0);
          ret = codec_parse_cmd(cmd2, 7, dev);
          break;
        case 3: // Trasmissione festivita' fisse
#warning LOG
{
FILE *fp;
fp = fopen("/tmp/CEI_tlc.log", "a");
fprintf(fp, "%02d/%02d/%02d %02d:%02d:%02d - /542\n", GIORNO, MESE, ANNO, ORE, MINUTI, SECONDI);
fclose(fp);
}
          codec_parse_cmd("/542", 4, dev);
          memcpy(cmd, "/543", 4);
          for(i=0; i<16; i++)
          {
            sprintf(cmd+4, "%02d%02d%d", ((CEI_trasmissione_festivita*)(CEI->L4))->fest[i].giorno,
              ((CEI_trasmissione_festivita*)(CEI->L4))->fest[i].mese,
              ((CEI_trasmissione_festivita*)(CEI->L4))->fest[i].tipo?
                3-((CEI_trasmissione_festivita*)(CEI->L4))->fest[i].tipo:0);
#warning LOG
{
FILE *fp;
fp = fopen("/tmp/CEI_tlc.log", "a");
fprintf(fp, "%02d/%02d/%02d %02d:%02d:%02d - %s\n", GIORNO, MESE, ANNO, ORE, MINUTI, SECONDI, cmd);
fclose(fp);
}
            codec_parse_cmd(cmd, 9, dev);
          }
          cei_L4_send_packet(dev, NULL, 0);
          break;
        case 4: // Trasmissione festivita' variabili
#warning LOG
{
FILE *fp;
fp = fopen("/tmp/CEI_tlc.log", "a");
fprintf(fp, "%02d/%02d/%02d %02d:%02d:%02d - /544\n", GIORNO, MESE, ANNO, ORE, MINUTI, SECONDI);
fclose(fp);
}
          codec_parse_cmd("/544", 4, dev);
          memcpy(cmd, "/545", 4);
          for(i=0; i<16; i++)
          {
            j = ANNO;
            if((((CEI_trasmissione_festivita*)(CEI->L4))->fest[i].mese < MESE) ||
               ((((CEI_trasmissione_festivita*)(CEI->L4))->fest[i].mese == MESE) &&
                (((CEI_trasmissione_festivita*)(CEI->L4))->fest[i].giorno <= GIORNO)))
               j++;
            sprintf(cmd+4, "%02d%02d%02d%d", ((CEI_trasmissione_festivita*)(CEI->L4))->fest[i].giorno,
              ((CEI_trasmissione_festivita*)(CEI->L4))->fest[i].mese, j,
              ((CEI_trasmissione_festivita*)(CEI->L4))->fest[i].tipo?
                3-((CEI_trasmissione_festivita*)(CEI->L4))->fest[i].tipo:0);
#warning LOG
{
FILE *fp;
fp = fopen("/tmp/CEI_tlc.log", "a");
fprintf(fp, "%02d/%02d/%02d %02d:%02d:%02d - %s\n", GIORNO, MESE, ANNO, ORE, MINUTI, SECONDI, cmd);
fclose(fp);
}
            codec_parse_cmd(cmd, 11, dev);
          }
          cei_L4_send_packet(dev, NULL, 0);
          break;
        case 5: // Trasmissione festivita' variabili per blocchi
          {
            CEI_festivita_variabili *fest = ((CEI_trasmissione_festivita_variabili*)(CEI->L4))->fest;
            for(i=0; i<32; i++)
            {
              sprintf(cmd, "T%02d%d%02d%02d%02d%02d", i, fest[i].tipo,
                      fest[i].mesei, fest[i].giornoi, fest[i].mesef, fest[i].giornof);
#warning LOG
{
FILE *fp;
fp = fopen("/tmp/CEI_tlc.log", "a");
fprintf(fp, "%02d/%02d/%02d %02d:%02d:%02d - %s\n", GIORNO, MESE, ANNO, ORE, MINUTI, SECONDI, cmd);
fclose(fp);
}
              ret = codec_parse_cmd(cmd, 12, dev);
            }
          }
          cei_L4_send_packet(dev, NULL, 0);
          break;
        case 11: // Trasmissione fasce orarie
          for(i=0;i<7;i++)
          {
            for(j=0;j<4;j++)
            {
              if(((CEI_trasmissione_fasce*)(CEI->L4))->fascia[i][j].ignora_inizio)
              {
                if(((CEI_trasmissione_fasce*)(CEI->L4))->fascia[i][j].ora_inizio == 0x7f)
                {
                  sprintf(cmd, "/7%02d20000", i*4+j);
#warning LOG
{
FILE *fp;
fp = fopen("/tmp/CEI_tlc.log", "a");
fprintf(fp, "%02d/%02d/%02d %02d:%02d:%02d - %s\n", GIORNO, MESE, ANNO, ORE, MINUTI, SECONDI, cmd);
fclose(fp);
}
                  codec_parse_cmd(cmd, 9, dev);
                }
              }
              else
              {
                sprintf(cmd, "/7%02d0%02d%02d", i*4+j,
                  ((CEI_trasmissione_fasce*)(CEI->L4))->fascia[i][j].ora_inizio,
                  ((CEI_trasmissione_fasce*)(CEI->L4))->fascia[i][j].minuti_inizio);
#warning LOG
{
FILE *fp;
fp = fopen("/tmp/CEI_tlc.log", "a");
fprintf(fp, "%02d/%02d/%02d %02d:%02d:%02d - %s\n", GIORNO, MESE, ANNO, ORE, MINUTI, SECONDI, cmd);
fclose(fp);
}
                codec_parse_cmd(cmd, 9, dev);
              }
              
              if(((CEI_trasmissione_fasce*)(CEI->L4))->fascia[i][j].ignora_fine)
              {
                if(((CEI_trasmissione_fasce*)(CEI->L4))->fascia[i][j].ora_fine == 0x7f)
                {
                  sprintf(cmd, "/7%02d30000", i*4+j);
#warning LOG
{
FILE *fp;
fp = fopen("/tmp/CEI_tlc.log", "a");
fprintf(fp, "%02d/%02d/%02d %02d:%02d:%02d - %s\n", GIORNO, MESE, ANNO, ORE, MINUTI, SECONDI, cmd);
fclose(fp);
}
                  codec_parse_cmd(cmd, 9, dev);
                }
              }
              else
              {
                sprintf(cmd, "/7%02d1%02d%02d", i*4+j,
                  ((CEI_trasmissione_fasce*)(CEI->L4))->fascia[i][j].ora_fine,
                  ((CEI_trasmissione_fasce*)(CEI->L4))->fascia[i][j].minuti_fine);
#warning LOG
{
FILE *fp;
fp = fopen("/tmp/CEI_tlc.log", "a");
fprintf(fp, "%02d/%02d/%02d %02d:%02d:%02d - %s\n", GIORNO, MESE, ANNO, ORE, MINUTI, SECONDI, cmd);
fclose(fp);
}
                codec_parse_cmd(cmd, 9, dev);
              }
            }
          }
          cei_L4_send_packet(dev, NULL, 0);
          break;
        case 12: // Trasmissione fasce di sblocco orario
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
#warning LOG
{
FILE *fp;
fp = fopen("/tmp/CEI_tlc.log", "a");
fprintf(fp, "%02d/%02d/%02d %02d:%02d:%02d - %s\n", GIORNO, MESE, ANNO, ORE, MINUTI, SECONDI, cmd);
fclose(fp);
}
              codec_parse_cmd(cmd, 9, dev);
              sprintf(cmd, "/7%02d30000", ((GIORNO_S-1)<<1)+28);
#warning LOG
{
FILE *fp;
fp = fopen("/tmp/CEI_tlc.log", "a");
fprintf(fp, "%02d/%02d/%02d %02d:%02d:%02d - %s\n", GIORNO, MESE, ANNO, ORE, MINUTI, SECONDI, cmd);
fclose(fp);
}
              codec_parse_cmd(cmd, 9, dev);
              sprintf(cmd, "/7%02d20000", ((GIORNO_S%7)<<1)+29);
#warning LOG
{
FILE *fp;
fp = fopen("/tmp/CEI_tlc.log", "a");
fprintf(fp, "%02d/%02d/%02d %02d:%02d:%02d - %s\n", GIORNO, MESE, ANNO, ORE, MINUTI, SECONDI, cmd);
fclose(fp);
}
              codec_parse_cmd(cmd, 9, dev);
              sprintf(cmd, "/7%02d30000", ((GIORNO_S%7)<<1)+29);
#warning LOG
{
FILE *fp;
fp = fopen("/tmp/CEI_tlc.log", "a");
fprintf(fp, "%02d/%02d/%02d %02d:%02d:%02d - %s\n", GIORNO, MESE, ANNO, ORE, MINUTI, SECONDI, cmd);
fclose(fp);
}
              codec_parse_cmd(cmd, 9, dev);
          }
          else if(orai >= ORE)
          {
            if(oraf < orai)
            {
              // scavalcamento mezzanotte (prima fascia giorno attuale e seconda fascia giorno successivo)
              sprintf(cmd, "/7%02d0%02d%02d", ((GIORNO_S-1)<<1)+28, orai, mini);
#warning LOG
{
FILE *fp;
fp = fopen("/tmp/CEI_tlc.log", "a");
fprintf(fp, "%02d/%02d/%02d %02d:%02d:%02d - %s\n", GIORNO, MESE, ANNO, ORE, MINUTI, SECONDI, cmd);
fclose(fp);
}
              codec_parse_cmd(cmd, 9, dev);
              sprintf(cmd, "/7%02d30000", ((GIORNO_S-1)<<1)+28);
#warning LOG
{
FILE *fp;
fp = fopen("/tmp/CEI_tlc.log", "a");
fprintf(fp, "%02d/%02d/%02d %02d:%02d:%02d - %s\n", GIORNO, MESE, ANNO, ORE, MINUTI, SECONDI, cmd);
fclose(fp);
}
              codec_parse_cmd(cmd, 9, dev);
              sprintf(cmd, "/7%02d20000", ((GIORNO_S%7)<<1)+29);
#warning LOG
{
FILE *fp;
fp = fopen("/tmp/CEI_tlc.log", "a");
fprintf(fp, "%02d/%02d/%02d %02d:%02d:%02d - %s\n", GIORNO, MESE, ANNO, ORE, MINUTI, SECONDI, cmd);
fclose(fp);
}
              codec_parse_cmd(cmd, 9, dev);
              sprintf(cmd, "/7%02d1%02d%02d", ((GIORNO_S%7)<<1)+29, oraf, minf);
#warning LOG
{
FILE *fp;
fp = fopen("/tmp/CEI_tlc.log", "a");
fprintf(fp, "%02d/%02d/%02d %02d:%02d:%02d - %s\n", GIORNO, MESE, ANNO, ORE, MINUTI, SECONDI, cmd);
fclose(fp);
}
              codec_parse_cmd(cmd, 9, dev);
            }
            else
            {
              // sera giorno attuale (prima fascia giorno attuale)
              sprintf(cmd, "/7%02d0%02d%02d", ((GIORNO_S-1)<<1)+28, orai, mini);
#warning LOG
{
FILE *fp;
fp = fopen("/tmp/CEI_tlc.log", "a");
fprintf(fp, "%02d/%02d/%02d %02d:%02d:%02d - %s\n", GIORNO, MESE, ANNO, ORE, MINUTI, SECONDI, cmd);
fclose(fp);
}
              codec_parse_cmd(cmd, 9, dev);
              sprintf(cmd, "/7%02d1%02d%02d", ((GIORNO_S-1)<<1)+28, oraf, minf);
#warning LOG
{
FILE *fp;
fp = fopen("/tmp/CEI_tlc.log", "a");
fprintf(fp, "%02d/%02d/%02d %02d:%02d:%02d - %s\n", GIORNO, MESE, ANNO, ORE, MINUTI, SECONDI, cmd);
fclose(fp);
}
              codec_parse_cmd(cmd, 9, dev);
              sprintf(cmd, "/7%02d20000", ((GIORNO_S%7)<<1)+29);
#warning LOG
{
FILE *fp;
fp = fopen("/tmp/CEI_tlc.log", "a");
fprintf(fp, "%02d/%02d/%02d %02d:%02d:%02d - %s\n", GIORNO, MESE, ANNO, ORE, MINUTI, SECONDI, cmd);
fclose(fp);
}
              codec_parse_cmd(cmd, 9, dev);
              sprintf(cmd, "/7%02d30000", ((GIORNO_S%7)<<1)+29);
#warning LOG
{
FILE *fp;
fp = fopen("/tmp/CEI_tlc.log", "a");
fprintf(fp, "%02d/%02d/%02d %02d:%02d:%02d - %s\n", GIORNO, MESE, ANNO, ORE, MINUTI, SECONDI, cmd);
fclose(fp);
}
              codec_parse_cmd(cmd, 9, dev);
            }
          }
          else
          {
            // mattino giorno successivo (seconda fascia giorno successivo)
            sprintf(cmd, "/7%02d20000", ((GIORNO_S-1)<<1)+28);
#warning LOG
{
FILE *fp;
fp = fopen("/tmp/CEI_tlc.log", "a");
fprintf(fp, "%02d/%02d/%02d %02d:%02d:%02d - %s\n", GIORNO, MESE, ANNO, ORE, MINUTI, SECONDI, cmd);
fclose(fp);
}
            codec_parse_cmd(cmd, 9, dev);
            sprintf(cmd, "/7%02d30000", ((GIORNO_S-1)<<1)+28);
#warning LOG
{
FILE *fp;
fp = fopen("/tmp/CEI_tlc.log", "a");
fprintf(fp, "%02d/%02d/%02d %02d:%02d:%02d - %s\n", GIORNO, MESE, ANNO, ORE, MINUTI, SECONDI, cmd);
fclose(fp);
}
            codec_parse_cmd(cmd, 9, dev);
            sprintf(cmd, "/7%02d0%02d%02d", ((GIORNO_S%7)<<1)+29, orai, mini);
#warning LOG
{
FILE *fp;
fp = fopen("/tmp/CEI_tlc.log", "a");
fprintf(fp, "%02d/%02d/%02d %02d:%02d:%02d - %s\n", GIORNO, MESE, ANNO, ORE, MINUTI, SECONDI, cmd);
fclose(fp);
}
            codec_parse_cmd(cmd, 9, dev);
            sprintf(cmd, "/7%02d1%02d%02d", ((GIORNO_S%7)<<1)+29, oraf, minf);
#warning LOG
{
FILE *fp;
fp = fopen("/tmp/CEI_tlc.log", "a");
fprintf(fp, "%02d/%02d/%02d %02d:%02d:%02d - %s\n", GIORNO, MESE, ANNO, ORE, MINUTI, SECONDI, cmd);
fclose(fp);
}
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
#warning LOG
{
FILE *fp;
fp = fopen("/tmp/CEI_tlc.log", "a");
fprintf(fp, "%02d/%02d/%02d %02d:%02d:%02d - %s\n", GIORNO, MESE, ANNO, ORE, MINUTI, SECONDI, cmd);
fclose(fp);
}
              codec_parse_cmd(cmd, 9, dev);
              sprintf(cmd, "/7%02d30000", ((GIORNO_S%7)<<1)+28);
#warning LOG
{
FILE *fp;
fp = fopen("/tmp/CEI_tlc.log", "a");
fprintf(fp, "%02d/%02d/%02d %02d:%02d:%02d - %s\n", GIORNO, MESE, ANNO, ORE, MINUTI, SECONDI, cmd);
fclose(fp);
}
              codec_parse_cmd(cmd, 9, dev);
              sprintf(cmd, "/7%02d20000", (((GIORNO_S+1)%7)<<1)+29);
#warning LOG
{
FILE *fp;
fp = fopen("/tmp/CEI_tlc.log", "a");
fprintf(fp, "%02d/%02d/%02d %02d:%02d:%02d - %s\n", GIORNO, MESE, ANNO, ORE, MINUTI, SECONDI, cmd);
fclose(fp);
}
              codec_parse_cmd(cmd, 9, dev);
              sprintf(cmd, "/7%02d30000", (((GIORNO_S+1)%7)<<1)+29);
#warning LOG
{
FILE *fp;
fp = fopen("/tmp/CEI_tlc.log", "a");
fprintf(fp, "%02d/%02d/%02d %02d:%02d:%02d - %s\n", GIORNO, MESE, ANNO, ORE, MINUTI, SECONDI, cmd);
fclose(fp);
}
              codec_parse_cmd(cmd, 9, dev);
          }
          else if(orai >= ORE)
          {
            if(oraf < orai)
            {
              // scavalcamento mezzanotte (prima fascia giorno attuale e seconda fascia giorno successivo)
              sprintf(cmd, "/7%02d0%02d%02d", ((GIORNO_S%7)<<1)+28, orai, mini);
#warning LOG
{
FILE *fp;
fp = fopen("/tmp/CEI_tlc.log", "a");
fprintf(fp, "%02d/%02d/%02d %02d:%02d:%02d - %s\n", GIORNO, MESE, ANNO, ORE, MINUTI, SECONDI, cmd);
fclose(fp);
}
              codec_parse_cmd(cmd, 9, dev);
              sprintf(cmd, "/7%02d30000", ((GIORNO_S%7)<<1)+28);
#warning LOG
{
FILE *fp;
fp = fopen("/tmp/CEI_tlc.log", "a");
fprintf(fp, "%02d/%02d/%02d %02d:%02d:%02d - %s\n", GIORNO, MESE, ANNO, ORE, MINUTI, SECONDI, cmd);
fclose(fp);
}
              codec_parse_cmd(cmd, 9, dev);
              sprintf(cmd, "/7%02d20000", (((GIORNO_S+1)%7)<<1)+29);
#warning LOG
{
FILE *fp;
fp = fopen("/tmp/CEI_tlc.log", "a");
fprintf(fp, "%02d/%02d/%02d %02d:%02d:%02d - %s\n", GIORNO, MESE, ANNO, ORE, MINUTI, SECONDI, cmd);
fclose(fp);
}
              codec_parse_cmd(cmd, 9, dev);
              sprintf(cmd, "/7%02d1%02d%02d", (((GIORNO_S+1)%7)<<1)+29, oraf, minf);
#warning LOG
{
FILE *fp;
fp = fopen("/tmp/CEI_tlc.log", "a");
fprintf(fp, "%02d/%02d/%02d %02d:%02d:%02d - %s\n", GIORNO, MESE, ANNO, ORE, MINUTI, SECONDI, cmd);
fclose(fp);
}
              codec_parse_cmd(cmd, 9, dev);
            }
            else
            {
              // sera giorno attuale (prima fascia giorno attuale)
              sprintf(cmd, "/7%02d0%02d%02d", ((GIORNO_S%7)<<1)+28, orai, mini);
#warning LOG
{
FILE *fp;
fp = fopen("/tmp/CEI_tlc.log", "a");
fprintf(fp, "%02d/%02d/%02d %02d:%02d:%02d - %s\n", GIORNO, MESE, ANNO, ORE, MINUTI, SECONDI, cmd);
fclose(fp);
}
              codec_parse_cmd(cmd, 9, dev);
              sprintf(cmd, "/7%02d1%02d%02d", ((GIORNO_S%7)<<1)+28, oraf, minf);
#warning LOG
{
FILE *fp;
fp = fopen("/tmp/CEI_tlc.log", "a");
fprintf(fp, "%02d/%02d/%02d %02d:%02d:%02d - %s\n", GIORNO, MESE, ANNO, ORE, MINUTI, SECONDI, cmd);
fclose(fp);
}
              codec_parse_cmd(cmd, 9, dev);
              sprintf(cmd, "/7%02d20000", (((GIORNO_S+1)%7)<<1)+29);
#warning LOG
{
FILE *fp;
fp = fopen("/tmp/CEI_tlc.log", "a");
fprintf(fp, "%02d/%02d/%02d %02d:%02d:%02d - %s\n", GIORNO, MESE, ANNO, ORE, MINUTI, SECONDI, cmd);
fclose(fp);
}
              codec_parse_cmd(cmd, 9, dev);
              sprintf(cmd, "/7%02d30000", (((GIORNO_S+1)%7)<<1)+29);
#warning LOG
{
FILE *fp;
fp = fopen("/tmp/CEI_tlc.log", "a");
fprintf(fp, "%02d/%02d/%02d %02d:%02d:%02d - %s\n", GIORNO, MESE, ANNO, ORE, MINUTI, SECONDI, cmd);
fclose(fp);
}
              codec_parse_cmd(cmd, 9, dev);
            }
          }
          else
          {
            // mattino giorno successivo (seconda fascia giorno successivo)
            sprintf(cmd, "/7%02d20000", ((GIORNO_S%7)<<1)+28);
#warning LOG
{
FILE *fp;
fp = fopen("/tmp/CEI_tlc.log", "a");
fprintf(fp, "%02d/%02d/%02d %02d:%02d:%02d - %s\n", GIORNO, MESE, ANNO, ORE, MINUTI, SECONDI, cmd);
fclose(fp);
}
            codec_parse_cmd(cmd, 9, dev);
            sprintf(cmd, "/7%02d30000", ((GIORNO_S%7)<<1)+28);
#warning LOG
{
FILE *fp;
fp = fopen("/tmp/CEI_tlc.log", "a");
fprintf(fp, "%02d/%02d/%02d %02d:%02d:%02d - %s\n", GIORNO, MESE, ANNO, ORE, MINUTI, SECONDI, cmd);
fclose(fp);
}
            codec_parse_cmd(cmd, 9, dev);
            sprintf(cmd, "/7%02d0%02d%02d", (((GIORNO_S+1)%7)<<1)+29, orai, mini);
#warning LOG
{
FILE *fp;
fp = fopen("/tmp/CEI_tlc.log", "a");
fprintf(fp, "%02d/%02d/%02d %02d:%02d:%02d - %s\n", GIORNO, MESE, ANNO, ORE, MINUTI, SECONDI, cmd);
fclose(fp);
}
            codec_parse_cmd(cmd, 9, dev);
            sprintf(cmd, "/7%02d1%02d%02d", (((GIORNO_S+1)%7)<<1)+29, oraf, minf);
#warning LOG
{
FILE *fp;
fp = fopen("/tmp/CEI_tlc.log", "a");
fprintf(fp, "%02d/%02d/%02d %02d:%02d:%02d - %s\n", GIORNO, MESE, ANNO, ORE, MINUTI, SECONDI, cmd);
fclose(fp);
}
            codec_parse_cmd(cmd, 9, dev);
          }

          cei_L4_send_packet(dev, NULL, 0);
          break;
        case 13: // Trasmissione fasce orarie per periodi
          for(i=0;i<32;i++)
          {
              if(((CEI_trasmissione_fasce_periodo*)(CEI->L4))->fascia[i].ignora_inizio)
              {
                if(((CEI_trasmissione_fasce_periodo*)(CEI->L4))->fascia[i].ora_inizio == 0x7f)
                {
                  sprintf(cmd, "/7%02d20000", 42+i);
#warning LOG
{
FILE *fp;
fp = fopen("/tmp/CEI_tlc.log", "a");
fprintf(fp, "%02d/%02d/%02d %02d:%02d:%02d - %s\n", GIORNO, MESE, ANNO, ORE, MINUTI, SECONDI, cmd);
fclose(fp);
}
                  codec_parse_cmd(cmd, 9, dev);
                }
              }
              else
              {
                sprintf(cmd, "/7%02d0%02d%02d", 42+i,
                  ((CEI_trasmissione_fasce_periodo*)(CEI->L4))->fascia[i].ora_inizio,
                  ((CEI_trasmissione_fasce_periodo*)(CEI->L4))->fascia[i].minuti_inizio);
#warning LOG
{
FILE *fp;
fp = fopen("/tmp/CEI_tlc.log", "a");
fprintf(fp, "%02d/%02d/%02d %02d:%02d:%02d - %s\n", GIORNO, MESE, ANNO, ORE, MINUTI, SECONDI, cmd);
fclose(fp);
}
                codec_parse_cmd(cmd, 9, dev);
              }
              
              if(((CEI_trasmissione_fasce_periodo*)(CEI->L4))->fascia[i].ignora_fine)
              {
                if(((CEI_trasmissione_fasce_periodo*)(CEI->L4))->fascia[i].ora_fine == 0x7f)
                {
                  sprintf(cmd, "/7%02d30000", 42+i);
#warning LOG
{
FILE *fp;
fp = fopen("/tmp/CEI_tlc.log", "a");
fprintf(fp, "%02d/%02d/%02d %02d:%02d:%02d - %s\n", GIORNO, MESE, ANNO, ORE, MINUTI, SECONDI, cmd);
fclose(fp);
}
                  codec_parse_cmd(cmd, 9, dev);
                }
              }
              else
              {
                sprintf(cmd, "/7%02d1%02d%02d", 42+i,
                  ((CEI_trasmissione_fasce_periodo*)(CEI->L4))->fascia[i].ora_fine,
                  ((CEI_trasmissione_fasce_periodo*)(CEI->L4))->fascia[i].minuti_fine);
#warning LOG
{
FILE *fp;
fp = fopen("/tmp/CEI_tlc.log", "a");
fprintf(fp, "%02d/%02d/%02d %02d:%02d:%02d - %s\n", GIORNO, MESE, ANNO, ORE, MINUTI, SECONDI, cmd);
fclose(fp);
}
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
                ((CEI_trasmissione_dataora*)pkt)->header.versione = 3;
                ((CEI_trasmissione_dataora*)pkt)->header.formato = 1;
                ((CEI_trasmissione_dataora*)pkt)->header.processo = 3;
                ((CEI_trasmissione_dataora*)pkt)->header.sorgente = 0;
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
                unsigned char *tpdgf = TPDGF;
                
                ((CEI_trasmissione_festivita*)pkt)->header.versione = 3;
                ((CEI_trasmissione_festivita*)pkt)->header.formato = 1;
                ((CEI_trasmissione_festivita*)pkt)->header.processo = 3;
                ((CEI_trasmissione_festivita*)pkt)->header.sorgente = 0;
                ((CEI_trasmissione_festivita*)pkt)->header.tipoinfo = 3;
                ((CEI_trasmissione_festivita*)pkt)->header.numeromessaggio = CEI->L4_pktnum[2]++;
                for(i=0; (i<16)&&(tpdgf)&&(*tpdgf!=0xff); i++)
                {
                  ((CEI_trasmissione_festivita*)pkt)->fest[i].giorno = *tpdgf++;
                  ((CEI_trasmissione_festivita*)pkt)->fest[i].mese = *tpdgf++;
                  ((CEI_trasmissione_festivita*)pkt)->fest[i].tipo = *tpdgf++;
                  if(((CEI_trasmissione_festivita*)pkt)->fest[i].tipo == 3)
                    ((CEI_trasmissione_festivita*)pkt)->fest[i].tipo = 0;
                }
              }
              else
                ret = 0;
              break;
            case 4:
              ret = sizeof(CEI_trasmissione_festivita);
              pkt = calloc(1, ret);
              if(pkt)
              {
                unsigned char *tpdgv = TPDGV;
                
                ((CEI_trasmissione_festivita*)pkt)->header.versione = 3;
                ((CEI_trasmissione_festivita*)pkt)->header.formato = 1;
                ((CEI_trasmissione_festivita*)pkt)->header.processo = 3;
                ((CEI_trasmissione_festivita*)pkt)->header.sorgente = 0;
                ((CEI_trasmissione_festivita*)pkt)->header.tipoinfo = 4;
                ((CEI_trasmissione_festivita*)pkt)->header.numeromessaggio = CEI->L4_pktnum[2]++;
                for(i=0; (i<16)&&(tpdgv)&&(*tpdgv!=0xff); i++)
                {
                  ((CEI_trasmissione_festivita*)pkt)->fest[i].giorno = *tpdgv++;
                  ((CEI_trasmissione_festivita*)pkt)->fest[i].mese = *tpdgv++;
                  tpdgv++;	// salta l'anno
                  ((CEI_trasmissione_festivita*)pkt)->fest[i].tipo = *tpdgv++;
                  if(((CEI_trasmissione_festivita*)pkt)->fest[i].tipo == 3)
                    ((CEI_trasmissione_festivita*)pkt)->fest[i].tipo = 0;
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
                ((CEI_trasmissione_fasce*)pkt)->header.versione = 3;
                ((CEI_trasmissione_fasce*)pkt)->header.formato = 1;
                ((CEI_trasmissione_fasce*)pkt)->header.processo = 3;
                ((CEI_trasmissione_fasce*)pkt)->header.sorgente = 0;
                ((CEI_trasmissione_fasce*)pkt)->header.tipoinfo = 5;
                ((CEI_trasmissione_fasce*)pkt)->header.numeromessaggio = CEI->L4_pktnum[2]++;
                memcpy(((CEI_trasmissione_festivita_variabili*)(CEI->L4))->fest, periodo, sizeof(CEI_festivita_variabili) * 32);
              }
              else
                ret = 0;
              break;
            case 11:
              ret = sizeof(CEI_trasmissione_fasce);
              pkt = calloc(1, ret);
              if(pkt)
              {
                ((CEI_trasmissione_fasce*)pkt)->header.versione = 3;
                ((CEI_trasmissione_fasce*)pkt)->header.formato = 1;
#ifdef EPT
                ((CEI_trasmissione_fasce*)pkt)->header.ept = 1;
#else
                ((CEI_trasmissione_fasce*)pkt)->header.ept = 0;
#endif
                ((CEI_trasmissione_fasce*)pkt)->header.processo = 3;
                ((CEI_trasmissione_fasce*)pkt)->header.sorgente = 0;
                ((CEI_trasmissione_fasce*)pkt)->header.tipoinfo = 136;
                ((CEI_trasmissione_fasce*)pkt)->header.numeromessaggio = CEI->L4_pktnum[2]++;
                memcpy(((CEI_trasmissione_fasce*)pkt)->fascia, FAGR, sizeof(CEI_fascia) * 4 * 7);
              }
              else
                ret = 0;
              break;
            case 12:
              ret = sizeof(CEI_trasmissione_fasce_periodo);
              pkt = calloc(1, ret);
              if(pkt)
              {
                ((CEI_trasmissione_fasce_periodo*)pkt)->header.versione = 3;
                ((CEI_trasmissione_fasce_periodo*)pkt)->header.formato = 1;
#ifdef EPT
                ((CEI_trasmissione_fasce_periodo*)pkt)->header.ept = 1;
#else
                ((CEI_trasmissione_fasce_periodo*)pkt)->header.ept = 0;
#endif
                ((CEI_trasmissione_fasce_periodo*)pkt)->header.processo = 3;
                ((CEI_trasmissione_fasce_periodo*)pkt)->header.sorgente = 0;
                ((CEI_trasmissione_fasce_periodo*)pkt)->header.tipoinfo = 138;
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
                ((CEI_parametri_configurazione*)pkt)->header.versione = 3;
                ((CEI_parametri_configurazione*)pkt)->header.formato = 1;
#ifdef EPT
                ((CEI_parametri_configurazione*)pkt)->header.ept = 1;
#else
                ((CEI_parametri_configurazione*)pkt)->header.ept = 0;
#endif
                ((CEI_parametri_configurazione*)pkt)->header.processo = 4;
                ((CEI_parametri_configurazione*)pkt)->header.sorgente = 0;
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
                  ((CEI_parametri_configurazione*)pkt)->header.versione = 3;
                  ((CEI_parametri_configurazione*)pkt)->header.formato = 1;
#ifdef EPT
                  ((CEI_parametri_configurazione*)pkt)->header.ept = 1;
#else
                  ((CEI_parametri_configurazione*)pkt)->header.ept = 0;
#endif
                  ((CEI_parametri_configurazione*)pkt)->header.processo = 4;
                  ((CEI_parametri_configurazione*)pkt)->header.sorgente = 0;
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
                  ((CEI_parametri_configurazione*)pkt)->header.versione = 3;
                  ((CEI_parametri_configurazione*)pkt)->header.formato = 1;
#ifdef EPT
                  ((CEI_parametri_configurazione*)pkt)->header.ept = 1;
#else
                  ((CEI_parametri_configurazione*)pkt)->header.ept = 0;
#endif
                  ((CEI_parametri_configurazione*)pkt)->header.processo = 4;
                  ((CEI_parametri_configurazione*)pkt)->header.sorgente = 0;
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
            case 4:
              pkt = calloc(1, sizeof(CEI_parametri_configurazione) + 1);
              if(pkt)
              {
                ((CEI_parametri_configurazione*)pkt)->header.versione = 3;
                ((CEI_parametri_configurazione*)pkt)->header.formato = 1;
#ifdef EPT
                ((CEI_parametri_configurazione*)pkt)->header.ept = 1;
#else
                ((CEI_parametri_configurazione*)pkt)->header.ept = 0;
#endif
                ((CEI_parametri_configurazione*)pkt)->header.processo = 4;
                ((CEI_parametri_configurazione*)pkt)->header.sorgente = 0;
                ((CEI_parametri_configurazione*)pkt)->header.tipoinfo = 2;
                ((CEI_parametri_configurazione*)pkt)->header.numeromessaggio = CEI->L4_pktnum[3]++;
                ((CEI_parametri_configurazione*)pkt)->tipodati = 4;
                ((CEI_parametri_configurazione*)pkt)->indice = ((CEI_parametri_configurazione*)(CEI->L4))->indice;
                ((CEI_parametri_configurazione*)pkt)->dati[0] = SatHoliday;
                cei_L4_send_packet(dev, pkt, sizeof(CEI_parametri_configurazione) + 1);
                free(pkt);
                ret = 1;
              }
              else
                ret = 0;
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
#warning LOG
{
FILE *fp;
fp = fopen("/tmp/CEI_tlc.log", "a");
fprintf(fp, "%02d/%02d/%02d %02d:%02d:%02d - %s\n", GIORNO, MESE, ANNO, ORE, MINUTI, SECONDI, cmdtoascii(cmd, 1, 10));
fclose(fp);
}
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
#warning LOG
{
FILE *fp;
fp = fopen("/tmp/CEI_tlc.log", "a");
fprintf(fp, "%02d/%02d/%02d %02d:%02d:%02d - %s\n", GIORNO, MESE, ANNO, ORE, MINUTI, SECONDI, cmdtoascii(cmd, 4, 5));
fclose(fp);
}
                codec_parse_cmd(cmd, 9, dev);
                
                memcpy(cmd, "/561", 4);
                cmd[4] = '0'+((CEI_parametri_configurazione*)(CEI->L4))->indice;
                cmd[5] = i;
                memcpy(cmd+6, ((CEI_parametri_configurazione*)(CEI->L4))->dati + j + 3, 12);
#warning LOG
{
FILE *fp;
fp = fopen("/tmp/CEI_tlc.log", "a");
fprintf(fp, "%02d/%02d/%02d %02d:%02d:%02d - %s\n", GIORNO, MESE, ANNO, ORE, MINUTI, SECONDI, cmdtoascii(cmd, 4, 14));
fclose(fp);
}
                codec_parse_cmd(cmd, 18, dev);
                
                memcpy(cmd, "/562", 4);
                cmd[4] = '0'+((CEI_parametri_configurazione*)(CEI->L4))->indice;
                cmd[5] = i;
                memcpy(cmd+6, ((CEI_parametri_configurazione*)(CEI->L4))->dati + j + 15, 4);
#warning LOG
{
FILE *fp;
fp = fopen("/tmp/CEI_tlc.log", "a");
fprintf(fp, "%02d/%02d/%02d %02d:%02d:%02d - %s\n", GIORNO, MESE, ANNO, ORE, MINUTI, SECONDI, cmdtoascii(cmd, 4, 6));
fclose(fp);
}
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
#warning LOG
{
FILE *fp;
fp = fopen("/tmp/CEI_tlc.log", "a");
fprintf(fp, "%02d/%02d/%02d %02d:%02d:%02d - %s\n", GIORNO, MESE, ANNO, ORE, MINUTI, SECONDI, cmdtoascii(cmd, 4, 5));
fclose(fp);
}
                ret = codec_parse_cmd(cmd, 9, dev);
              }
              cei_L4_send_packet(dev, NULL, 0);
              break;
            case 4:
              memcpy(cmd, "/546", 4);
              cmd[4] = '0' + ((CEI_parametri_configurazione*)(CEI->L4))->dati[0];
#warning LOG
{
FILE *fp;
fp = fopen("/tmp/CEI_tlc.log", "a");
fprintf(fp, "%02d/%02d/%02d %02d:%02d:%02d - %s\n", GIORNO, MESE, ANNO, ORE, MINUTI, SECONDI, cmdtoascii(cmd, 4, 1));
fclose(fp);
}
              ret = codec_parse_cmd(cmd, 5, dev);
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
  
  dev->prot_data = calloc(1, sizeof(CEI_data));
  if(!dev->prot_data) return 0;
  
  if(config.consumer[dev->consumer].configured != 5)
    ser_setspeed(dev->fd, config.consumer[dev->consumer].data.serial.baud1, CEI_TIMEOUT);
  
  CEI->ind_CSC = 0xffff;
  if(config.consumer[dev->consumer].param)
    sscanf(config.consumer[dev->consumer].param, "%d", &(CEI->ind_CSC));
  
  return 1;
}

static void cei_free(ProtDevice *dev)
{
  free(dev->prot_data);
  dev->prot_data = NULL;
}

static struct {
  unsigned char *pkt;
  int len;
} cei_last_event[MAX_NUM_CONSUMER] = {{NULL, 0}, };

static void cei_ca_loop(ProtDevice *dev)
{
  Event ev;
  int ret, num;
  unsigned char *pkt;
  
  PrintEventEx2[1][dev->consumer][Ex2_Inizio_test] = 1;
  PrintEventEx2[1][dev->consumer][Ex2_Fine_test] = 1;
  PrintEventEx2[1][dev->consumer][Ex2_On_attuatore] = 1;
  PrintEventEx2[1][dev->consumer][Ex2_Off_attuatore] = 1;
  
  support_log("CEI Connect");
  
  pthread_mutex_lock(&cei_evlistmutex);
  if(!cei_current_dev) cei_current_dev = dev;
  pthread_mutex_unlock(&cei_evlistmutex);
  
  ret = 0;
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
    
    /* Libero l'ultimo messaggio nel momento in cui ricevo
       un messaggio di livello 4, dato che significa conferma
       di trasmissione. */
    if((cei_last_event[dev->consumer].pkt) && (CEI->L4_len >= 8))
    {
      free(cei_last_event[dev->consumer].pkt);
      cei_last_event[dev->consumer].pkt = NULL;
      cei_last_event[dev->consumer].len = 0;
    }
    
    if(CEI->L4_len > 8)
    {
      cei_L4_parse_command(dev, LS_ABILITATA[dev->consumer] & 0x02);
    }
    else	// polling
    {
      ret = -1;
      if(cei_last_event[dev->consumer].pkt)
      {
        pkt = cei_last_event[dev->consumer].pkt;
        ret = cei_last_event[dev->consumer].len;
      }
      else
      {
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
      }
      while(ret < 0)
      {
        while(!codec_get_event(&ev, dev));
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

static char cei_evento_riavvio_flag[MAX_NUM_CONSUMER] = {0, };

void cei_evento_riavvio(ProtDevice *dev)
{
  cei_event_list *tevlist;
  CEI_allarme *alarm;
  
  /* L'evento di riavvio deve essere generato solo una volta,
     non ad ogni connessione. */
  if(cei_evento_riavvio_flag[dev->consumer]) return;
  cei_evento_riavvio_flag[dev->consumer] = 1;
  
  pthread_mutex_lock(&cei_evlistmutex);
  
  tevlist = malloc(sizeof(cei_event_list));
  tevlist->next = CEI->evlist;
  CEI->evlist = tevlist;
  tevlist->event = malloc(sizeof(CEI_allarme));
  
  alarm = ((CEI_allarme*)(tevlist->event));
  
  alarm->header.formato = 1;	// Formato ABI v1.3
  alarm->header.versione = 3;
  
  alarm->header.processo = 1;	// supervisione e controllo
  alarm->header.sorgente = 0;
  alarm->header.tipoinfo = 1;	// allarmi e stati
  
  alarm->tempoaccadimento = bswap_32(time(NULL) - BASESECONDS);
  alarm->areaapplicativa = 0;
  alarm->tiporivelatore = 0;
  alarm->categoria = 1;	// non richiede azioni da CSC
  alarm->areaprovenienza = 0;
  
  alarm->transizione = 1;
  alarm->tipoeventomsb = 0;
  alarm->tipoeventolsb = ALM_ALTERAZIONE;
  alarm->classeelemento = CLS_UNITA_CENTRALE;
  alarm->numeroelementomsb = 0;
  alarm->numeroelementolsb = 1;
  
  tevlist->len = sizeof(CEI_allarme);
  pthread_mutex_unlock(&cei_evlistmutex);  
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
       semplicemente il loop CA. */
    if(!cei_init(dev)) return;
    cei_evento_riavvio(dev);
    
//    cei_configure(CEI, CEI_CA | CEI_CRC16, CEI_PROT_NONE);
//    cei_configure(CEI, CEI_CA | CEI_CRC16, CEI_PROT_AUTH);
    cei_configure(CEI, CEI_CA | CEI_CRC16, CEI_PROT_FEAL);
//    cei_configure(CEI, CEI_CA | CEI_CRC16, CEI_PROT_NONE);
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
        cei_evento_riavvio(dev);
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
  
  printf("CEI-ABI rev1.3 (plugin): " __DATE__ " " __TIME__ "\n");
  prot_plugin_register("CEI", 0, NULL, NULL, (PthreadFunction)cei_listen);
//  prot_plugin_register("CEI-CA", 0, NULL, NULL, (PthreadFunction)cei_ca_loop);
//  prot_plugin_register("CEI-CSC", 0, NULL, NULL, (PthreadFunction)cei_csc_loop);
}
