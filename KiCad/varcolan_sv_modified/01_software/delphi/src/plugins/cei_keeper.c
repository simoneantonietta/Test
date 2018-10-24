/*

programma utente:

centrale:
- liste classi 1 (centrale) e 8 (alimentazione)




hwtestserial -d /dev/ttyS2 -b 4800 -tx "at+ipr=4800" -txhex 0d -rx 1
hwtestserial -d /dev/ttyS2 -b 4800 -tx "at+csns=4" -txhex 0d -rx 1
hwtestserial -d /dev/ttyS2 -b 4800 -tx "at+clip=1" -txhex 0d -rx 1
hwtestserial -d /dev/ttyS2 -b 4800 -tx "at+crc=1" -txhex 0d -rx 1
hwtestserial -d /dev/ttyS2 -b 4800 -tx "at+cbst=6,0,1" -txhex 0d -rx 1
hwtestserial -d /dev/ttyS2 -b 4800 -tx "at+ifc=0,0" -txhex 0d -rx 1
hwtestserial -d /dev/ttyS2 -b 4800 -tx "ats0=1" -txhex 0d -rx 1
hwtestserial -d /dev/ttyS2 -b 4800 -tx "atd0392301655" -txhex 0d -rx 120
hwtestserial -d /dev/ttyS2 -b 4800 -tx "at" -txhex 0d -rx 120

16/11/2009 - Aggiunto il log del gsm su saet.log.2.

*/

#include "protocol.h"
#include "codec.h"
#include "serial.h"
#include "support.h"
#include "user.h"
#include "ronda.h"
#include "database.h"
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
#include <sys/ioctl.h>

//#define DEBUG

#define CEI	((CEI_data*)(dev->prot_data))
#define L7	L4+8
#define L7_len	L4_len-12

static pthread_mutex_t cei_evlistmutex = PTHREAD_MUTEX_INITIALIZER;
static ProtDevice *cei_current_dev = NULL;
static ProtDevice *cei_backup_dev = NULL;
static unsigned char cei_zone_secret[1 + n_ZS + n_ZI];

static int keeper_fuori_scansione = 0;
static int keeper_test_backup = 0;

/*****************************/

static short cei_inv_num;
static char cei_inv_file[32];

/*****************************/

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

void cei_fine_punzonatura(ProtDevice *dev, int elemento)
{
  cei_event_list *tevlist;
  CEI_allarme *ev;
  
  pthread_mutex_lock(&cei_evlistmutex);
  
  tevlist = malloc(sizeof(cei_event_list));
  tevlist->next = CEI->evlist;
  CEI->evlist = tevlist;
  tevlist->event = malloc(sizeof(CEI_allarme));
  
  ev = ((CEI_allarme*)(tevlist->event));
  
  ev->header.formato = 1;	// Formato ABI v.2
  ev->header.versione = 2;
  ev->header.ept = 0;
  ev->header.processo = 1;	// supervisione e controllo
  ev->header.tipoinfo = 1;	// allarme
  ev->header.numeromessaggio = CEI->L4_pktnum[0]++;
  ev->tempoaccadimento = cei_tempoaccadimento();
  
  ev->tipoevento = ALM_STATO_RIVELATORE;
  ev->transizione = ALM_FINE;
  ev->tiporivelatore = 0;
  ev->areaapplicativa = 0;
  ev->areaprovenienza = 0;
  ev->categoria = 1;
  
  ev->classeelemento = CLS_SENSORE;
  ev->numeroelementomsb = elemento >> 8;
  ev->numeroelementolsb = elemento & 0xff;
  
  tevlist->len = sizeof(CEI_allarme);
  pthread_mutex_unlock(&cei_evlistmutex);  
}

void cei_accoda_alarm(ProtDevice *dev, CEI_allarme *ev)
{
  cei_event_list *tevlist;
  
  pthread_mutex_lock(&cei_evlistmutex);
  
  tevlist = malloc(sizeof(cei_event_list));
  tevlist->next = CEI->evlist;
  CEI->evlist = tevlist;
  tevlist->event = malloc(sizeof(CEI_allarme));
  
  memcpy(tevlist->event, ev, sizeof(CEI_allarme));
  
  tevlist->len = sizeof(CEI_allarme);
  pthread_mutex_unlock(&cei_evlistmutex);  
}

void cei_invio_file(ProtDevice *dev)
{
  cei_event_list *tevlist, *tevhead;
  CEI_generico *ev;
  FILE *fp;
  int len, num;
  
  fp = fopen(cei_inv_file, "r");
  if(!fp) return;
  
  tevhead = malloc(sizeof(cei_event_list));
  tevhead->event = malloc(sizeof(CEI_generico)+17);
  
  ev = ((CEI_generico*)(tevhead->event));
  ev->header.formato = 1;	// Formato ABI v.2
  ev->header.versione = 2;
  ev->header.ept = 0;
  ev->header.processo = 4;
  ev->header.tipoinfo = 2;
  ev->header.numeromessaggio = CEI->L4_pktnum[3]++;
  *(short*)(ev->dati) = 0;
  *(unsigned char*)(ev->dati+4) = 200;
  memcpy(ev->dati+5, cei_inv_file+5, 12);
  
  tevhead->len = sizeof(CEI_generico)+17;
  
  tevlist = tevhead;
  
  len = 200;
  num = 1;
  while(len == 200)
  {
    tevlist->next = malloc(sizeof(cei_event_list));
    tevlist = tevlist->next;
    tevhead->event = malloc(sizeof(CEI_generico)+202);
    ev = ((CEI_generico*)(tevlist->event));
    ev->header.formato = 1;	// Formato ABI v.2
    ev->header.versione = 2;
    ev->header.ept = 0;
    ev->header.processo = 4;
    ev->header.tipoinfo = 2;
    ev->header.numeromessaggio = CEI->L4_pktnum[3]++;
    *(short*)(ev->dati) = num++;
    
    len = fread(ev->dati+2, 1, 200, fp);
    tevlist->len = len+2+sizeof(CEI_generico);
  }
  
  fclose(fp);
  
  ev = ((CEI_generico*)(tevhead->event));
  *(unsigned char*)(ev->dati+2) = num;
  
  pthread_mutex_lock(&cei_evlistmutex);
  
  tevlist->next = CEI->evlist;
  CEI->evlist = tevhead;
  
  pthread_mutex_unlock(&cei_evlistmutex);  
}

#include "cei_keeper_common.c"

#define ALM_		0
#define CLS_		0
#define NOT_IMPL	-1

struct _cei_event {char tipoevento, transizione, classeelemento, numbytes;};

static struct _cei_event cei_event_table[MAX_NUM_EVENT] = {
{ALM_ALLARME, ALM_INIZIO, CLS_SENSORE, 2}, /* Allarme Sensore */
{ALM_ALLARME, ALM_FINE, CLS_SENSORE, 2}, /* Ripristino Sensore */
////{ALM_GUASTO, ALM_INIZIO, CLS_UNITA_SATELLITE, 1}, /* Guasto Periferica */
////{ALM_GUASTO, ALM_FINE, CLS_UNITA_SATELLITE, 1}, /* Fine Guasto Periferica */
{ALM_GUASTO, ALM_INIZIO, CLS_UNITA_SATELLITE, 2}, /* Guasto Periferica */
{ALM_GUASTO, ALM_FINE, CLS_UNITA_SATELLITE, 2}, /* Fine Guasto Periferica */
{ALM_MANOMISSIONE, ALM_INIZIO, CLS_SENSORE, 2}, /* Manomissione Dispositivo */
{ALM_MANOMISSIONE, ALM_FINE, CLS_SENSORE, 2}, /* Fine Manomis Dispositivo */
////{ALM_MANOMISSIONE, ALM_INIZIO, CLS_UNITA_SATELLITE, 1}, /* Manomissione Contenitore */
////{ALM_MANOMISSIONE, ALM_FINE, CLS_UNITA_SATELLITE, 1}, /* Fine Manomis Contenitore */
{ALM_MANOMISSIONE, ALM_INIZIO, CLS_UNITA_SATELLITE, 2}, /* Manomissione Contenitore */
{ALM_MANOMISSIONE, ALM_FINE, CLS_UNITA_SATELLITE, 2}, /* Fine Manomis Contenitore */
{ALM_DISINSERITO, ALM_FINE, CLS_ZONA, 1}, /* Attivata zona */
{ALM_DISINSERITO, ALM_INIZIO, CLS_ZONA, 1}, /* Disattivata zona */
{ALM_DISINSERITO, ALM_PERSIST, CLS_ZONA, 1}, /* Attivazione impedita */
{ALM_DISABILITATO, ALM_FINE, CLS_SENSORE, 2}, /* Sensore In Servizio */
{ALM_DISABILITATO, ALM_INIZIO, CLS_SENSORE, 2}, /* Sensore Fuori Servizio */
{ALM_DISABILITATO, ALM_FINE, CLS_ATTUATORE, 2}, /* Attuatore In Servizio */
{ALM_DISABILITATO, ALM_INIZIO, CLS_ATTUATORE, 2}, /* Attuatore Fuori Servizio */
{ALM_ERRORE_PROGR, ALM_INIZIO, CLS_UNITA_SATELLITE, 1}, /* Ricezione codice errato */
{ALM_, ALM_, CLS_, NOT_IMPL}, /* Tipo ModuloMaster */
{ALM_, ALM_, CLS_, NOT_IMPL}, /* Periferica Incongruente */
////{ALM_MANOMISSIONE, ALM_INIZIO, CLS_UNITA_SATELLITE, 1}, /* Periferica Manomessa */
////{ALM_MANOMISSIONE, ALM_FINE, CLS_UNITA_SATELLITE, 1}, /* Periferica Ripristino */
{ALM_MANOMISSIONE, ALM_INIZIO, CLS_UNITA_SATELLITE, 2}, /* Periferica Manomessa */
{ALM_MANOMISSIONE, ALM_FINE, CLS_UNITA_SATELLITE, 2}, /* Periferica Ripristino */
////{ALM_INTERR_LINEA, ALM_INIZIO, CLS_UNITA_SATELLITE, 1}, /* Sospesa attivita linea */
{ALM_INTERR_LINEA, ALM_INIZIO, CLS_UNITA_SATELLITE, 2}, /* Sospesa attivita linea */
////{ALM_CHIAVE, ALM_FINE, CLS_UNITA_CENTRALE, 1}, /* Chiave Falsa */
{ALM_CHIAVE, ALM_FINE, CLS_UNITA_CENTRALE, 2}, /* Chiave Falsa */
////{ALM_DISABILITATO, ALM_FINE, CLS_ORGANO_COMANDO, 1}, /* Sentinella on */
////{ALM_DISABILITATO, ALM_INIZIO, CLS_ORGANO_COMANDO, 1}, /* Sentinella off */
////{ALM_DISABILITATO, ALM_INIZIO, CLS_ORGANO_COMANDO, 1}, /* Sentinella off timeout */
{ALM_DISABILITATO, ALM_FINE, CLS_ORGANO_COMANDO, 2}, /* Sentinella on */
{ALM_DISABILITATO, ALM_INIZIO, CLS_ORGANO_COMANDO, 2}, /* Sentinella off */
{ALM_DISABILITATO, ALM_INIZIO, CLS_ORGANO_COMANDO, 2}, /* Sentinella off timeout */
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
{ALM_TEST_MANUALE, ALM_, CLS_UNITA_CENTRALE, 0}, /* Stato prova */
{ALM_, ALM_, CLS_, NOT_IMPL}, /* Test attivo in corso */
{ALM_, ALM_, CLS_, NOT_IMPL}, /* Risposta SCS */
{ALM_GUASTO, ALM_FINE, CLS_SENSORE, 2}, /* Fine Guasto Sensore */
{ALM_, ALM_, CLS_, NOT_IMPL}, /* Invio codice segreto */
{ALM_, ALM_, CLS_, NOT_IMPL}, /* Perifere previste */
{ALM_, ALM_, CLS_, NOT_IMPL}, /* Sensore in Allarme */
{ALM_, ALM_, CLS_, NOT_IMPL}, /* Variata fascia oraria */
{ALM_, ALM_, CLS_, NOT_IMPL}, /* Invio host no storico */
{ALM_, ALM_, CLS_, NOT_IMPL}, /* Invio Memoria */
{ALM_STATO, ALM_FINE, 0, 0}, /* Fine Invio Memoria */
{ALM_, ALM_, CLS_, NOT_IMPL}, /* Liste codici */
{ALM_, ALM_, CLS_, NOT_IMPL}, /* Orari ronda */
{ALM_, ALM_, CLS_, NOT_IMPL}, /* Giorni festivi */
{ALM_, ALM_, CLS_, NOT_IMPL}, /* Fasce orarie */
//{ALM_ATTIVAZIONE, ALM_INIZIO, CLS_ATTUATORE, 2}, /* ON attuatore */
{ALM_, ALM_, CLS_, NOT_IMPL}, /* ON attuatore */
{ALM_, ALM_, CLS_, NOT_IMPL}, /* Transitato Ident */
{ALM_, ALM_, CLS_, NOT_IMPL}, /* Entrato Ident */
{ALM_, ALM_, CLS_, NOT_IMPL}, /* Uscito Ident */
////{ALM_CODICE, ALM_PERSIST, CLS_UNITA_CENTRALE, 1}, /* Codice valido */
{ALM_CODICE, ALM_PERSIST, CLS_UNITA_CENTRALE, 2}, /* Codice valido */
////{ALM_CHIAVE, ALM_INIZIO, CLS_UNITA_CENTRALE, 1}, /* Chiave valida */
{ALM_CHIAVE, ALM_INIZIO, CLS_UNITA_CENTRALE, 2}, /* Chiave valida */
{ALM_ACCESSO_OPER, ALM_INIZIO, CLS_OPERATORE, 1}, /* Operatore */
{ALM_STATO_RIVELATORE, ALM_INIZIO, CLS_SENSORE, 2}, /* Punzonatura */
//{ALM_ATTIVAZIONE, ALM_FINE, CLS_ATTUATORE, 2}, /* Spegnimento */
{ALM_, ALM_, CLS_, NOT_IMPL}, /* Spegnimento */
{ALM_, ALM_, CLS_, NOT_IMPL}, /* Reset fumo */
{ALM_, ALM_, CLS_, NOT_IMPL}, /* Livello abilitato */
{ALM_, ALM_, CLS_, NOT_IMPL}, /* Variato segreto */
{ALM_, ALM_, CLS_, NOT_IMPL}, /* Conferma ricezione modem */
{ALM_, ALM_, CLS_, NOT_IMPL}, /* N telefonico 1 */
{ALM_, ALM_, CLS_, NOT_IMPL}, /* N telefonico 2 */
{ALM_, ALM_, CLS_, NOT_IMPL}, /* StatoBadge */
//{ALM_, ALM_, CLS_, NOT_IMPL}, /* Evento Sensore */
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
////{ALM_DISINSERITO, ALM_FINE, CLS_UNITA_SATELLITE, 1}, /* Governo_linee_in_servizio */
////{ALM_DISINSERITO, ALM_INIZIO, CLS_UNITA_SATELLITE, 1}, /* Governo_linee_fuori_servizio */
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
{ALM_BATT_SCOLLEGATA, ALM_INIZIO, CLS_UNITA_CENTRALE, 0},
{ALM_BATT_SCOLLEGATA, ALM_FINE, CLS_UNITA_CENTRALE, 0},
{ALM_MANUTENZIONE, ALM_INIZIO, CLS_UNITA_CENTRALE, 0},
{ALM_MANUTENZIONE, ALM_FINE, CLS_UNITA_CENTRALE, 0},
{ALM_TEST_NEGATIVO, ALM_INIZIO, CLS_RAGGRUPPAMENTO, 1}, /* Fine test attivo (negativo) */
{ALM_STRAORDINARIO, ALM_INIZIO, CLS_ZONA, 0},
{ALM_STRAORDINARIO, ALM_FINE, CLS_ZONA, 0},
{ALM_INS_ANTICIPATO, ALM_INIZIO, CLS_UNITA_CENTRALE, 0},
};

static void cei_event_set_alarm(CEI_allarme *alarm, Event *ev, int tempoaccadimento)
{
  int elemento;
  
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
  
  if(alarm->classeelemento == CLS_UNITA_CENTRALE) alarm->areaprovenienza = 64;
  
  switch(cei_event_table[ev->Event[8]-EV_OFFSET].numbytes)
  {
    case 1:
      alarm->numeroelementomsb = 0;
      if((alarm->classeelemento == CLS_UNITA_SATELLITE) && !ev->Event[9])
      {
        alarm->classeelemento = CLS_UNITA_CENTRALE;
        alarm->numeroelementolsb = 1;
      }
      else
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
      else
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
    
  if(alarm->classeelemento == CLS_UNITA_CENTRALE) alarm->areaprovenienza = 64;
  
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
      if(alarm->classeelemento == CLS_UNITA_CENTRALE)
        alarm->numeroelementolsb = 1;
      else
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
  
  if(!ev || !ev->Len)
  {
    *pkt = NULL;
    return 0;
  }
  
#if 0
#warning !!!!!!!!!!!!!!!
printf("Evento: ");
for(i=0;i<ev->Len;i++) printf("%02x ", ev->Event[i]);
printf("\n");
#endif
  
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
          default:	// non puo' capitare
            bitmask = 0;
            tipoevento = 0;
            break;
        }
        stato->transizione = (ev->Event[11 + (i/3)] & bitmask)?1:0;
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
            default:	// non puo' capitare
              bitmask = 0;
              tipoevento = 0;
              break;
          }
          if((i%4) == 3)
            stato->transizione = (ev->Event[10 + (i/4)] & bitmask)?0:1;
          else
            stato->transizione = (ev->Event[10 + (i/4)] & bitmask)?1:0;
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
            default:	// non puo' capitare
              bitmask = 0;
              tipoevento = 0;
              break;
          }
          stato->transizione = (ev->Event[11 + (i/4)] & bitmask)?1:0;
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
/*
   Il tentativo di accettazione fallito per sensore in anomalia
   deve generare un fine ed un inizio per quel sensore.
*/
/*
      alarm->tipoevento = ALM_ACC_ALLARME - 1 + ev->Event[11];
      if((ev->Event[12] == 3) || (ev->Event[12] == 5)) alarm->transizione = ALM_FINE;
      if(ev->Event[12] == 4) alarm->transizione = ALM_PERSIST;
*/
      if(ev->Event[12] == 4)
      {
        alarm->tipoevento = ALM_ACC_ALLARME - 1 + ev->Event[11];
        // accoda l'inizio e restituisce subito la fine
        alarm->transizione = ALM_INIZIO;
        cei_accoda_alarm(dev, alarm);
        alarm->transizione = ALM_FINE;
      }
      else
      {
        free(alarm);
        *pkt = NULL;
        return -1;
      }
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
    else if(ev->Event[8] == Punzonatura)
    {
      cei_fine_punzonatura(dev, (alarm->numeroelementomsb<<8) + alarm->numeroelementolsb);
    }
    
    *pkt = (unsigned char*)alarm;
  }
  
  return len;
}

static int cei_L4_parse_command(ProtDevice *dev, int abilitato)
{
  int i, len, ret = -1;
//  int j;
  int elemento, valore;
  CEI_risposta_telecomando risp;
  time_t t, dt;
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
                   
#if 0
{
char buf[256];
sprintf(buf, "parse %d %d %d", abilitato, ((CEI_telecomando*)(CEI->L4))->comando, ((CEI_telecomando*)(CEI->L4))->classeelemento);
support_log(buf);
}
#endif
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
                ret = 1;
              }
              break;
            case 7:
              if(((CEI_telecomando*)(CEI->L4))->classeelemento == CLS_ZONA)
              {
                // Straordinario -> ME[592]
                memcpy(cmd, "K592", 5);
                len = 5;
              }
              break;
            case 8:
              if(((CEI_telecomando*)(CEI->L4))->classeelemento == CLS_ZONA)
              {
                // Inserimento anticipato -> ME[591]
                memcpy(cmd, "K591", 5);
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
                // Inizio manutenzione -> ME[590]
                memcpy(cmd, "K590", 5);
                len = 5;
              }
              break;
            case 19:
              if(((CEI_telecomando*)(CEI->L4))->classeelemento == CLS_UNITA_CENTRALE)
              {
                // Fine manutenzione -> ME[589]
                memcpy(cmd, "K589", 5);
                len = 5;
              }
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
            case 202:
              if(((CEI_telecomando*)(CEI->L4))->classeelemento == CLS_UNITA_CENTRALE)
              {
                /* Keeper: test linea commutata */
                keeper_test_backup = 1;
                ret = 1;
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
              else if(((CEI_telecomando*)(CEI->L4))->classeelemento == CLS_UNITA_CENTRALE)
              {
                /* Keeper: fuori scansione */
                keeper_fuori_scansione = 1;
                ret = 1;
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
              {
                memcpy(cmd, "/380", 4);
                len = 4;
              }
              else if(((CEI_telecomando*)(CEI->L4+i))->classeelemento == CLS_ATTUATORE)
                cmd[0] = 'e';
              else if(((CEI_telecomando*)(CEI->L4+i))->classeelemento == CLS_ZONA)
                cmd[0] = 'c';
              break;
            case 2:
              if(((CEI_telecomando*)(CEI->L4+i))->classeelemento == CLS_SENSORE)
                cmd[0] = 'd';
              break;
          }
          if(cmd[0]) ret = codec_parse_cmd(cmd, len, dev);
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
            codec_parse_cmd("K594", 5, dev);
          }
          else
          {
            datehour = gmtime(&t);
            
            /* On ME[593] */
            codec_parse_cmd("K593", 5, dev);
            
            sprintf(cmd, "U%d%02d%02d%02d", datehour->tm_wday, datehour->tm_mday, datehour->tm_mon+1, datehour->tm_year-100);
            sprintf(cmd2, "W%02d%02d%02d", datehour->tm_hour, datehour->tm_min, datehour->tm_sec);
            codec_parse_cmd(cmd, 8, dev);
            ret = codec_parse_cmd(cmd2, 7, dev);
          }
          cei_L4_send_packet(dev, NULL, 0);
          break;
        case 3: // Trasmissione festivita' fisse
          {
            CEI_festivita *fest = ((CEI_trasmissione_festivita*)(CEI->L4))->fest;
            
            codec_parse_cmd("/542", 4, dev);
            for(i=0;i<16;i++)
            {
              if(fest[i].tipo && (fest[i].tipo != 3))
              {
                if(fest[i].tipo == 4)
                  fest[i].tipo = 0;
                else
                  fest[i].tipo = 2 - fest[i].tipo;
                sprintf(cmd, "/543%02d%02d%d", fest[i].giorno, fest[i].mese, fest[i].tipo);
                codec_parse_cmd(cmd, 9, dev);
              }
            }
          }
          cei_L4_send_packet(dev, NULL, 0);
          break;
        case 4: // Trasmissione festivita' variabili
          {
            CEI_festivita *fest = ((CEI_trasmissione_festivita*)(CEI->L4))->fest;
            int anno;
            
            codec_parse_cmd("/544", 4, dev);
            for(i=0;i<16;i++)
            {
              if(fest[i].tipo && (fest[i].tipo != 3))
              {
                if(fest[i].tipo == 4)
                  fest[i].tipo = 0;
                else
                  fest[i].tipo = 2 - fest[i].tipo;
                anno = ANNO;
                if((fest[i].mese < MESE) || ((fest[i].mese == MESE) && (fest[i].giorno <= GIORNO))) anno++;
                sprintf(cmd, "/545%02d%02d%02d%d", fest[i].giorno, fest[i].mese, anno, fest[i].tipo);
                codec_parse_cmd(cmd, 11, dev);
              }
            }
          }
          cei_L4_send_packet(dev, NULL, 0);
          break;
        case 5: // Trasmissione festivita' variabili per blocchi
          {
            CEI_festivita_blocchi *fest = ((CEI_trasmissione_festivita_blocchi*)(CEI->L4))->fest;
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
          memcpy(FAGR, ((CEI_trasmissione_fasce*)(CEI->L4))->fascia, sizeof(CEI_fascia) * 4 * 7);
          for(i=0;i<(4*7);i++)
          {
            if(FAGR[i][0] != 0xff)
              sprintf(cmd, "/7%02d0%02d%02d", i, FAGR[i][0], FAGR[i][1]);
            else
              sprintf(cmd, "/7%02d20000", i);
            codec_parse_cmd(cmd, 9, dev);
            if(FAGR[i][2] != 0xff)
              sprintf(cmd, "/7%02d1%02d%02d", i, FAGR[i][2], FAGR[i][3]);            
            else
              sprintf(cmd, "/7%02d30000", i);
            ret = codec_parse_cmd(cmd, 9, dev);                                          
          }
          cei_L4_send_packet(dev, NULL, 0);
          break;
        case 7: // Trasmissione fasce di sblocco orario
          // prime 24 ore
          orai = ((CEI_trasmissione_fasce_sblocco*)(CEI->L4))->fascia[0].ora_inizio;
          mini = ((CEI_trasmissione_fasce_sblocco*)(CEI->L4))->fascia[0].minuti_inizio;
          oraf = ((CEI_trasmissione_fasce_sblocco*)(CEI->L4))->fascia[0].ora_fine;
          minf = ((CEI_trasmissione_fasce_sblocco*)(CEI->L4))->fascia[0].minuti_fine;
          if(orai == 0xff)
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
          if(orai == 0xff)
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
          memcpy(FAGR[42], ((CEI_trasmissione_fasce_periodo*)(CEI->L4))->fascia, sizeof(CEI_fascia) * 32);
          for(i=42;i<(42+32);i++)
          {
            if(FAGR[i][0] != 0xff)
              sprintf(cmd, "/7%02d0%02d%02d", i, FAGR[i][0], FAGR[i][1]);
            else
              sprintf(cmd, "/7%02d20000", i);
            codec_parse_cmd(cmd, 9, dev);
            if(FAGR[i][2] != 0xff)
              sprintf(cmd, "/7%02d1%02d%02d", i, FAGR[i][2], FAGR[i][3]);            
            else
              sprintf(cmd, "/7%02d30000", i);
            ret = codec_parse_cmd(cmd, 9, dev);                                          
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
                
                memset(((CEI_trasmissione_festivita*)pkt)->fest, 0, sizeof(CEI_festivita)*16);
                if(TPDGF)
                {
                  unsigned char *p = TPDGF;
                  for(i=0; i<16; i++)
                  {
                    if(*p != 0xff)
                    {
                      ((CEI_trasmissione_festivita*)pkt)->fest[i].giorno = *p++;
                      ((CEI_trasmissione_festivita*)pkt)->fest[i].mese = *p++;
                      if(*p == 0)
                        ((CEI_trasmissione_festivita*)pkt)->fest[i].tipo = 4;
                      else
                        ((CEI_trasmissione_festivita*)pkt)->fest[i].tipo = 2 - *p;
                      p++;
                    }
                    else
                      i = 16;
                  }
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
                ((CEI_trasmissione_festivita*)pkt)->header.versione = 1;
                ((CEI_trasmissione_festivita*)pkt)->header.formato = 2;
                ((CEI_trasmissione_festivita*)pkt)->header.processo = 3;
                ((CEI_trasmissione_festivita*)pkt)->header.tipoinfo = 4;
                ((CEI_trasmissione_festivita*)pkt)->header.numeromessaggio = CEI->L4_pktnum[2]++;
                
                memset(((CEI_trasmissione_festivita*)pkt)->fest, 0, sizeof(CEI_festivita)*16);
                if(TPDGV)
                {
                  unsigned char *p = TPDGV;
                  for(i=0; i<16; i++)
                  {
                    if(*p != 0xff)
                    {
                      ((CEI_trasmissione_festivita*)pkt)->fest[i].giorno = *p++;
                      ((CEI_trasmissione_festivita*)pkt)->fest[i].mese = *p++;
                      p++;	// salta l'anno
                      if(*p == 0)
                        ((CEI_trasmissione_festivita*)pkt)->fest[i].tipo = 4;
                      else
                        ((CEI_trasmissione_festivita*)pkt)->fest[i].tipo = 2 - *p;
                      p++;
                    }
                    else
                      i = 16;
                  }
                }
              }
              else
                ret = 0;
              break;
            case 5:
              ret = sizeof(CEI_trasmissione_festivita_blocchi);
              pkt = calloc(1, ret);
              if(pkt)
              {
                ((CEI_trasmissione_fasce*)pkt)->header.versione = 1;
                ((CEI_trasmissione_fasce*)pkt)->header.formato = 2;
                ((CEI_trasmissione_fasce*)pkt)->header.processo = 3;
                ((CEI_trasmissione_fasce*)pkt)->header.tipoinfo = 5;
                ((CEI_trasmissione_fasce*)pkt)->header.numeromessaggio = CEI->L4_pktnum[2]++;
                memcpy(((CEI_trasmissione_festivita_blocchi*)(CEI->L4))->fest, periodo, sizeof(CEI_festivita_blocchi) * 32);
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
#if 0
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
#endif
          {
            char name[16];
            
            cei_inv_num = *(short*)(CEI->L4+14);
            memcpy(name, CEI->L4+17, 12);
            name[12] = 0;
            sprintf(cei_inv_file, "/tmp/%s", name);
            
            cei_invio_file(dev);
          }
          
          break;
        case 3: // caricamento parametri
#if 0
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
#endif
          if(*(short*)(CEI->L4+12) == 0)
          {
            char name[16];
            FILE *fp;
            
            cei_inv_num = *(short*)(CEI->L4+14);
            memcpy(name, CEI->L4+17, 12);
            name[12] = 0;
            sprintf(cei_inv_file, "/tmp/%s", name);
            fp = fopen(cei_inv_file, "w");
            fclose(fp);
          }
          else
          {
            FILE *fp;
            
            fp = fopen(cei_inv_file, "a");
            fwrite(CEI->L4+14, CEI->L4_len-14, 1, fp);
            fclose(fp);
          }
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
  time_t lastev;
  
  pkt = (char*)PrintEventEx2;
  pkt += Ex2_Delphi*MAX_NUM_CONSUMER*EventNumEx2;
  pkt += dev->consumer*EventNumEx2;
  pkt[Ex2_Allineamento] = 0;
  
  support_log("CEI Connect");
  
  pthread_mutex_lock(&cei_evlistmutex);
  if(!cei_current_dev) cei_current_dev = dev;
  pthread_mutex_unlock(&cei_evlistmutex);
  
  lastev = time(NULL);
  
  while(1)
  {
    if((dev == cei_backup_dev) && ((time(NULL) - lastev) > 60))
    {
      /* Timeout comunicazione modem */
      
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
      
      sleep(1);
      write(dev->fd, "+++", 3);
      while(read(dev->fd, dev->buf, 256));
      write(dev->fd, "ATH\r", 4);
// per test
//      write(dev->fd, "\r\nNO CARRIER\r\n", 14);
      
      support_log("CEI Close");
      return;
    }
    
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
/* Che sia via LAN o via seriale (modem), pulisce tutto. */
//      if(config.consumer[dev->consumer].configured == 5)
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
        
        /* Se la connessione è quella di backup modem, non libera la memoria del consumatore,
           la libera solo se è la connessione LAN a chiudersi. */
        if((dev != cei_backup_dev) && (config.consumer[dev->consumer].configured == 5)) cei_free(dev);
        
        support_log("CEI Close");
        return;	// TCP connection closed
      }
/*
      else
      {
        support_log("CEI Close");
        return;	// serial connection closed
      }
*/
    }
    
    keeper_fuori_scansione = 0;
    
    /* Se una connessione LAN ha successo durante una sessione di backup,
       la LAN prende il sopravvento. */
    pthread_mutex_lock(&cei_evlistmutex);
    if(dev != cei_backup_dev) cei_current_dev = dev;
    pthread_mutex_unlock(&cei_evlistmutex);
    
    if(CEI->L4_len > 8)
    {
      cei_L4_parse_command(dev, 1); // LS_ABILITATA[dev->consumer] & 0x02);
      lastev = time(NULL);
    }
    else	// polling
    {
      if(CEI->identvalid &&
         /* Invio un nuovo evento solo se la connessione attiva
            è l'unica o se è la primaria (LAN) */
         /* (nel loop della connessione LAN) || (l'unico loop e quello backup) */
         ((dev != cei_backup_dev) || (cei_current_dev == cei_backup_dev)))
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
          if(pkt)
          {
            ((char*)pkt)[4] |= (CEI->L4[4] & CEI_L4_FLAG_S);
            lastev = time(NULL);
          }
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

static void cei_ca_loop_none(ProtDevice *dev)
{
  if(!dev || !cei_init(dev)) return;
  cei_configure(CEI, CEI_CA | CEI_CRC16, CEI_PROT_NONE);
  
  cei_ca_loop(dev);
}

static void cei_ca_loop_feal(ProtDevice *dev)
{
  if(!dev || !cei_init(dev)) return;
  cei_configure(CEI, CEI_CA | CEI_CRC16, CEI_PROT_FEAL);
//  cei_configure(CEI, CEI_CA | CEI_CRC16, CEI_PROT_NONE);
//  cei_configure(CEI, CEI_CA | CEI_CRC2, CEI_PROT_NONE);
  
  cei_ca_loop(dev);
}

static void cei_backup(ProtDevice *dev)
{
  /*
  
  Questa linea deve controllare il GSM, quando riceve la stringa CONNECT
  attiva il loop di comunicazione. La coda eventi è sempre quella del
  consumatore ETH, la coda di questo consumatore viene usata solo per
  verificare la presenza di eventi da trasmettere in caso di connessione
  di rete non disponibile.
  
  L'estrazione di un evento dalla coda ETH durante un polling regolare
  deve consumare lo stesso evento anche sulla coda del consumatore di backup.
  
  Attenzione: se il consumatore ETH non è attivo, questo consumatore estrae
  dalla propria coda eventi che non devono più essere scartati in quanto
  già rimossi. Verificare l'opportunità di una funziona codec_peek_event.
  
  Se la connessione ETH non è disponibile e viene rilevato un evento da
  dover trasmettere, basta inviare sulla seriale il comando AT di chiamata.
  Eventualmente occorre gestire il BUSY, il NO CARRIER o il NO ANSWER.
  
  La ripresa della comunicazione ETH deve far cadere la comunicazione modem.
  
  Intercettare il NO CARRIER nel loop di ricezione CEI per chiudere
  immediatamente la comunicazione, senza dover attendere 30 secondi la
  scadenza del timeout. Occorre però modificare cei_common!!!
  
  Semplificazione: le specifiche richiedono che la connessione modem si
  attivi solo in caso di allarme. Anziché estrarre gli eventi, controllo
  semplicemente che la zona 1 o la zona 2 sia in allarme.
  
  */
  
  int n, ret, backup, phonenum, connecting, answering, keepon, testonoff, timeout_ATD, tipo;
  unsigned char *pkt;
  char buf[256];
  Event ev;
  
  if(!dev || !cei_init(dev)) return;
  cei_configure(CEI, CEI_CA | CEI_CRC16, CEI_PROT_FEAL);
  
  pkt = (char*)PrintEventEx2;
  pkt += Ex2_Delphi*MAX_NUM_CONSUMER*EventNumEx2;
  pkt += dev->consumer*EventNumEx2;
  pkt[Ex2_Allineamento] = 0;
  
  backup = dev->consumer;
  phonenum = 1;
  if(config.consumer[dev->consumer].param)
    sscanf(config.consumer[dev->consumer].param, "%d:%d", &backup, &phonenum);
  if((phonenum < 1) || (phonenum > PBOOK_DIM)) phonenum = 1;
  
  pthread_mutex_lock(&cei_evlistmutex);
  cei_backup_dev = malloc(sizeof(ProtDevice));
  memcpy(cei_backup_dev, dev, sizeof(ProtDevice));
  cei_backup_dev->consumer = backup;
  pthread_mutex_unlock(&cei_evlistmutex);
  
  connecting = 0;
  answering = 0;
  keepon = 0;
  testonoff = 1;
  timeout_ATD = 0;
  
  sprintf(buf, "/dev/ttyS%d", config.consumer[dev->consumer].configured-1);
  tipo = prot_test_modem(buf);
  if(tipo == MODEM_CLASSIC)
  {
    ser_setspeed(dev->fd, B19200|CRTSCTS, CEI_TIMEOUT);
    write(dev->fd, "ATE0X3&G5\r", 10);
  }
  else
  {
    /* Imposto il timeout ma soprattutto elimino il controllo di flusso */
//    ser_setspeed(dev->fd, config.consumer[dev->consumer].data.serial.baud1, CEI_TIMEOUT);
    ser_setspeed(dev->fd, B4800, CEI_TIMEOUT);
    write(dev->fd, "ATE0;+IPR=4800;+CBST=6,0,1;+IFC=0,0;+CSNS=4\r", 44);
  }
  
  while(1)
  {
    n = read(dev->fd, dev->buf+dev->i, 1);
//printf("a) %p %d %d %d %d\n", cei_current_dev, keeper_fuori_scansione, connecting, keeper_test_backup, n);
    if(n && (dev->i < (DIMBUF-1)))
    {
      testonoff = 0;
      if(dev->i > 1) timeout_ATD = 0;
#if 0
{
struct timeval tv;
gettimeofday(&tv, NULL);
printf("%d.%02d ", tv.tv_sec, tv.tv_usec/10000);
if(dev->buf[dev->i] == '\r')
printf("%d CR\n", dev->i);
else if(dev->buf[dev->i] == '\n')
printf("%d LF\n", dev->i);
else
printf("%d %c\n", dev->i, dev->buf[dev->i]);
}
#endif
      dev->i++;
      if(dev->buf[dev->i-1] != '\r')
      {
        if(dev->buf[dev->i-1] == '\n') dev->i--;
        continue;
      }
      dev->buf[dev->i] = 0;
/* GC 091116 */
if(dev->i) support_log2(dev->buf);
      
      if(answering && (dev->buf[0] != '\r') && (dev->buf[0] != '\n'))
      {
//printf("errore\n");
//printf("answ 0\n");
        answering = 0;
        sleep(1);
      }
      
      if(!strncmp(dev->buf, "RING", 4))
      {
//printf("answ 1\n");
        support_log("Backup: RING");
        support_log2("Backup: RING");
        answering = 1;
        write(dev->fd, "ATA\r", 4);
      }
      else if(!strncmp(dev->buf, "CONNECT", 7))
      {
        support_log("Backup: CONNECT");
        support_log2("Backup: CONNECT");
//printf("loop start\n");
        /* La connessione deve cadere dopo 60 secondi
           dall'ultimo evento inviato. */
        memset(dev->prot_data, 0, sizeof(CEI_data));
        cei_ca_loop_feal(cei_backup_dev);
//printf("loop end\n");
        /* Dopo aver concluso la connessione di backup,
           svuoto completamente la coda del consumatore
           in attesa di un nuovo evento da inviare, per
           evitare di richiamare il centro solo perché
           la centrale risulta ancora in allarme. */
        while(codec_get_event(&ev, dev) >= 0);
        connecting = 0;
        answering = 0;
        sleep(5);
      }
      
      /* Qualunque cosa ricevuta durante un tentativo di
         connessione implica un errore e quindi occorre
         ripetere il tentativo, anche se la zona non è
         più in allarme. */
      if(connecting && (dev->buf[0] != '\r') && (dev->buf[0] != '\n'))
      {
//printf("errore\n");
        connecting = -1;
        sleep(1);
      }
    }
    else if(!connecting && !answering)
    {
//printf("--- %d %d %d %d\n", keepon, testonoff, connecting, answering);
      /* Timeout seriale, ma non in fase di connessione. */
      if(testonoff)
      {
        if(tipo == MODEM_CLASSIC)
          write(dev->fd, "ATE0X3&G5\r", 10);
        else
        {
          /* E' andato in timeout un keepalive AT, riaccendo il GSM */
          n = TIOCM_RTS;
          n = ioctl(dev->fd, TIOCMBIC, &n);
          usleep(100000);
          n = TIOCM_RTS;
          n = ioctl(dev->fd, TIOCMBIS, &n);
          sleep(5);
          
          write(dev->fd, "ATE0;+IPR=4800;+CBST=6,0,1;+IFC=0,0;+CSNS=4\r", 44);
        }
      }
      
      keepon++;
      if(keepon >= 30)
      {
        keepon = 0;
        testonoff = 1;
        write(dev->fd, "AT\r", 3);
      }
    }
    else if(timeout_ATD)
    {
      timeout_ATD--;
      if(!timeout_ATD)
      {
        /* Verifico se il gsm è ancora acceso */
        keepon = 0;
        testonoff = 1;
        write(dev->fd, "AT\r", 3);
        
        /* Perdo l'evento che ha scatenato la chiamata, ma meglio che
           rimanere con il gsm bloccato. */
        connecting = 0;
      }
    }
    
//printf("b) %p %d %d %d\n", cei_current_dev, keeper_fuori_scansione, connecting, keeper_test_backup);
    
    /* Devo anche attendere l'esito del testonoff prima di attivare una eventuale chiamata */
    if(!cei_current_dev && !keeper_fuori_scansione && !testonoff)
    {
      int call = 0;
      
      if((connecting < 0) || keeper_test_backup)
        call = 1;
      else
      {
//        if(!connecting && ((ZONA[1] & bitAlarm) || (ZONA[2] & bitAlarm)))
        if(!connecting)
        {
//          /* La centrale è in allarme, attivo la connessione di backup se ho eventi da spedire */
          /* Cerco se ci sono eventi sensibili da inviare.
             Gli eventi sensibili sono allarmi, manomissioni e guasti. */
          ret = 0;
          while(ret == 0)
          {
            while(!(ret = codec_get_event(&ev, dev)));
            
            // qui ret o è -1 o è > 0
            // se è -1 esco senza fare nulla
            
            if(ret > 0)
            {
              ret = cei_event_to_l4cei(dev, &ev, &pkt);
              if(ret < 0) ret = 0;
              if(ret)
              {
                /* Verifica il tipo di evento */
                if((((CEI_header_ex*)pkt)->processo != 1) ||
                   (((CEI_header_ex*)pkt)->tipoinfo != 1) ||
                   (((CEI_allarme*)pkt)->transizione != ALM_INIZIO) ||
                   ((((CEI_allarme*)pkt)->tipoevento != ALM_ALLARME) &&
                    (((CEI_allarme*)pkt)->tipoevento != ALM_GUASTO) &&
                    (((CEI_allarme*)pkt)->tipoevento != ALM_MANOMISSIONE)))
                  ret = 0;
                free(pkt);
              }
            }
            
            // qui ret è -1 da prima (ed esco), 0 perché l'evento non è "sensibile"
            // (e provo con un nuovo evento, se c'è), o > 0 se ho tradotto l'evento.
            // Se ho tradotto, call va a 1 ed esco perché ret comunque non è 0.
            if(ret > 0)
            {
              /* C'è un evento in coda da spedire. */
              call = 1;
            }
          }
        }
      }
      
      if(call)
      {
/*
Attenzione: se il GSM non risponde in nessun modo al comando ATD, il sistema
rimane appeso per sempre!!! Ad esempio: GSM spento ed invio ATD, oppure ATD
non ricevuto correttamente dal GSM.
*/
        support_log("Backup: Call");
        support_log2("Backup: Call");
        connecting = 1;
        timeout_ATD = 60;	// timeout 120s
        keeper_test_backup = 0;
        sprintf(buf, "ATD%s\r", config.PhoneBook[phonenum-1].Phone);
#if 0
{
struct timeval tv;
gettimeofday(&tv, NULL);
printf("%d.%02d ", tv.tv_sec, tv.tv_usec/10000);
printf("call %s\n", buf);
}
#endif
        write(dev->fd, buf, strlen(buf));
      }
    }
    else if(cei_current_dev)
    {
      /* Se nel frattempo torna la connessione primaria,
         interrompo i tentativi di chiamata. */
      if(connecting) connecting = 0;
      
      /* Se sono connesso via LAN, svuoto man mano la coda per evitare che
         se durante la connessione LAN vengono generati eventi che implicano
         la chiamata modem, questi rimangano accodati e alla caduta della
         connessione LAN generino una chiamata a vuoto. */
      while(codec_get_event(&ev, dev) >= 0);
    }
    
    dev->i = 0;
  }
}

void _init()
{
  int i;
  
  for(i=0; i<(1 + n_ZS + n_ZI); i++) cei_zone_secret[i] = 0xff;
  
  printf("CEI-ABI (plugin, Keeper): " __DATE__ " " __TIME__ "\n");
  prot_plugin_register("CEI", 0, NULL, NULL, (PthreadFunction)cei_ca_loop_none);
  prot_plugin_register("CEI-F", 0, NULL, NULL, (PthreadFunction)cei_ca_loop_feal);
  prot_plugin_register("CEI-B", 0, NULL, NULL, (PthreadFunction)cei_backup);
}

