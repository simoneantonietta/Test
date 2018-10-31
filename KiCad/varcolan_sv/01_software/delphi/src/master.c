#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <dlfcn.h>
#include <byteswap.h>
#include <string.h>
#include <stdlib.h>
#include "master.h"
#include "database.h"
#include "codec.h"
#include "nvmem.h"
#include "pxl.h"
#include "delphi.h"
#include "support.h"
#include "lara.h"
#include "command.h"
#include "ronda.h"
#include "config.h"
#include "strings.h"
#include "gsm.h"
#include <sys/ioctl.h>
#if defined __CRIS__ || defined __arm__
#include <asm/saetmm.h>
#endif
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <sys/socket.h>
#include <linux/if.h>
#include <netdb.h>

//#define VARIANTE_SCORRANO

/* Numero di periferiche effettivamente gestite */
#define MASTER_LIMIT_TIPO	16

unsigned char MMPresente[MM_NUM] = {[0 ... MM_NUM-1] = 0xff};
unsigned char MMSuspend[MM_NUM] = {0, };
unsigned char MMSuspendDelay[MM_NUM] = {0, };

/* Periferiche previste */
unsigned char TipoPeriferica[MM_NUM*32] = {[0 ... MM_NUM*32-1] = 0xff};
/* Periferiche presenti */
unsigned char StatoPeriferica[MM_NUM*32/8] = {0, };

unsigned char MMTexas[MM_NUM];

int libuser_started = 0;
int master_behaviour = MASTER_NORMAL;
void (*master_ridon_sync)() = NULL;

int master_switch[MM_NUM] = {0, };

/* Per default non invia le soglie analogiche agli SC8 LON.
   L'attivazione dell'invio avviene tramite apposito plugin. */
int master_send_threshold = 0;

static pthread_t master_thd_s;
static pthread_t master_thd;
static pthread_t master_loop_thd;

/* Attenzione, il comando 36 in effetti è quello che imposta
   la funzione Texas del modulo master, solo che il 13-10-2011
   è stato per sbaglio ridefinito come il comando 35 prioritario.
   Per evitare di toccare la macro TERMINALE_A e soprattutto per
   evitare che centrali sul campo si blocchino e richiedano un
   aggiornamento anche del programma utente, gestisco la richiesta
   del comando 36 come 35 prioritario, e l'impostazione Texas viene
   creato internamente come comando 37 ma poi mandato sul campo
   come comando 36. Di fatto il comando 37 per il campo non
   dovrà mai essere usato, da intendersi riservato. */
/* 2015-06-01 Aggiunto il comando 38 per la programmazione delle
   periferiche analogiche SC8C. */
static const unsigned char master_cmd_dim[] = {
	 0, 0, 0,16, 1, 1, 1, 1, 4, 0,
	 0, 0, 0, 2, 3, 7, 4, 6, 2, 3,
	 2, 2, 9, 2, 0, 0, 1, 8, 1, 1,
	 4,17, 3, 0, 0, 15, 0, 0, 7, 2,
	 8};

/* Se si cambia qualcosa in questo array occorre modificare
   anche il firmware dell'attestaggio (mmaster_6). */
static const unsigned char master_cmd_recv_dim[] = {
	 0, 2,17, 3, 2, 2, 6, 4, 6, 7,
	 1, 2, 0, 2, 2, 2, 4, 1, 3,10,
	 5,10, 9,18,18, 2, 1, 5, 8, 10};

#define MM_QUEUE_LEN 0x1000
static struct {
  int fd;
  unsigned char send_queue[MM_QUEUE_LEN];
  unsigned int send_queue_i;
  unsigned int send_queue_o;
  int ritardo;
  int sincro;
  unsigned char cmd_recv[64];
  unsigned char cmd_recv_idx;
//} mmaster[MM_NUM*2];
} mmaster[MM_NUM];
/* Gestisco due code parallele, una parte per i comandi principali ed una parte
   per i comandi a bassa priorità, cioè i messaggi di aggiornamento per le tastiere.
   I messaggi a bassa priorità vengono inviati solo se non ci sono altri comandi
   principali da inviare. */

/* La coda a bassa priorità è unica per tutti i moduli. E' la coda su cui vanno
   a finire fondamentalmente solo gli stati per le tastiere e anche i messaggi
   di configurazione, che possono essere veramente molti e quindi la coda deve
   essere sufficientemente lunga. */
#define MM_QUEUE_LEN_LP 0x10000
static struct {
  int fd;
  unsigned char send_queue[MM_QUEUE_LEN_LP];
  unsigned int send_queue_i;
  unsigned int send_queue_o;
} mmasterLP;

static pthread_mutex_t master_send_queue_mutex = PTHREAD_MUTEX_INITIALIZER;
static sem_t master_send_queue_msg;
static sem_t master_loop_sem;
static timeout_t *master_timeout = NULL;
static timeout_t *master_notx_to = NULL;
static pthread_mutex_t master_reset_mutex = PTHREAD_MUTEX_INITIALIZER;

unsigned int master_SC8R2_uniqueid = 0;
void (*master_radio868_p)(int mm, unsigned char *cmd) = NULL;
void (*master_radio868_accept_fronts_p)(int conc) = NULL;

static void master_reset(int mm)
{
  char m[24];
  int i;
  
  if(mm >= MM_NUM)
  {
    sprintf(m, "ERR - reset mm %d", mm);
    support_log(m);
    support_logmm(m);
    return;
  }
  
  pthread_mutex_lock(&master_reset_mutex);
#if defined __CRIS__ || defined __arm__
  /* Reset MM */
  ioctl(mmaster[mm].fd, MM_RESET, 0);
#endif
  
  for(i=0; i<(MM_NUM*32); i++)
    if(TipoPeriferica[i] != 0xff) StatoPeriferica[i>>3] |= (1 << (i & 0x7));
  for(i=0; i<MM_NUM; i++)
  {
    mmaster[i].cmd_recv_idx = 0;
    /* Assicuro che il MUX completi il polling degli attestaggi
       prima di partire con l'invio dei comandi. Per qualche ragione
       i comandi vanno ad interferire e capita che risulti presente
       un attestaggio che invece non è connesso.
       Con questo ritardo al reset non è più capitato. */
    mmaster[i].ritardo = 400;
  }
  
//  master_send_queue_i = 0;
//  master_send_queue_o = 0;
  
  sprintf(m, "MM (%d) Reset", mm);
  support_log(m);
  support_logmm(m);
  pthread_mutex_unlock(&master_reset_mutex);
}

static void master_notx(int mm);

static void master_suspend_delete(void *null1, int null2)
{
  /* Scaduto il tempo di attesa per la riattivazione dei
     moduli master, sblocco tutti i moduli rimasti appesi.
     In questo modo eventuali comandi rimasti in coda possono
     essere smaltiti. */
  int i;
  for(i=0; i<MM_NUM; i++)
  {
    /* Assicuro che tutti i moduli master tornino in linea
       senza perderne nessuno per strada. */
    if(MMSuspend[i] && (MMPresente[i] != 0xff))
    {
      if(MMSuspend[i] < 0x30)
      {
        /* Provo a resettare il modulo mancante fino a 3 volte */
        MMSuspend[i] += 0x10;
        master_notx(i);
      }
      else
      {
        /* Se dopo 3 reset il modulo non si è ripresentato
           lo considero definititvamente perso. */
        MMSuspend[i] = 0;
        MMPresente[i] = 0xff;
      }
    }
    else
      MMSuspend[i] = 0;
  }
}

static void master_exit(void *null1, int null2)
{
  if(master_behaviour != MASTER_NORMAL)
  {
    exit(0);
  }
  else
  {
    /* A fronte di NoTxMM, anziché riavviare la centrale
       resetto solo i moduli master e ne recupero la funzionalità
       senza far cadere le comunicazioni in corso o perdere gli
       stati correnti. Gli stati in teoria dovrebbero comunque
       essere mantenuti anche attraverso il riavvio, ma in questo
       modo ho una gestione tutta più lineare. A meno degli eventi
       tipici della ripartenza dei moduli master. */
    /* Prima del reset sospendo tutti i moduli. */
    int i;
    for(i=0; i<MM_NUM; i++) MMSuspend[i] |= 1;
    master_reset(0);
    
    /* Attivo il timer per lo sblocco della sospensione in caso
       i moduli master non si ripresentino. */
    timeout_on(master_notx_to, master_suspend_delete, NULL, 0, 150);
  }
}

static void master_notx(int mm)
{
  char log[32];
  
  /* Sospendo subito il modulo master che non comunica.
     Gli altri potrebbero ancora essere attivi. */
  MMSuspend[mm] |= 1;
  
  if(MMPresente[mm] != 0xff)
  {
    log[0] = No_ModuloMasterTx;
    log[1] = mm;
    codec_queue_event(log);
  }
  
  sprintf(log, "No Tx Modulo Master %d", mm);
  support_log(log);
  support_logmm(log);
  
  if(!master_notx_to) master_notx_to = timeout_init();
  timeout_on(master_notx_to, master_exit, NULL, 0, 10);
}

static int inline master_inc(int *pointer)
{
  int last;
  
  last = (*pointer)++;
  *pointer &= MM_QUEUE_LEN-1;
  return last;
}

static int inline master_incLP(int *pointer)
{
  int last;
  
  last = (*pointer)++;
  *pointer &= MM_QUEUE_LEN_LP-1;
  return last;
}

int master_queue(int numMM, unsigned char *cmd)
{
  int i;
#if 0
#warning DEBUG
  int x, y;
  char log[64];
#endif

  if((numMM >= MM_NUM) || (MMPresente[numMM] == 0xff))
  {
    /* Il modulo master non è presente e nemmeno sospeso */
    char m[24];
    sprintf(m, "ERR - queue mm %d", numMM);
    support_logmm(m);
    master_reset(0);
    return -2;
  }

  pthread_mutex_lock(&master_send_queue_mutex);
  
  if(MMPresente[numMM] != MM_LARA)
  {
#if 0
#warning DEBUG
  sprintf(log, "MM (%2d) <Q ", numMM);
  y = master_cmd_dim[cmd[0]]+1;
  if(y > 20) y = 20;
  for(x=0; x<y; x++)
    sprintf(log+11+(x<<1), "%02x", cmd[x]);
  support_logmm(log);
  sprintf(log, "--- %d %d %d", master_send_queue_i, master_send_queue_o, master_cmd_dim[cmd[0]]+4);
  support_logmm(log);
#endif
    
    if(cmd[0] == 35)
    {
      /* I comandi veloci vengono gestiti a bassa priorità. */
      //numMM += MM_NUM;
      /* La coda a bassa priorità è unica per tutti i moduli, però è molto più
         lunga per poter gestire la configurazione dei terminali che richiede
         molti messaggi. */
      if(((mmasterLP.send_queue_i < mmasterLP.send_queue_o) &&
         ((mmasterLP.send_queue_i + master_cmd_dim[cmd[0]] + 4) > mmasterLP.send_queue_o)) ||
         ((mmasterLP.send_queue_i > mmasterLP.send_queue_o) &&
         ((mmasterLP.send_queue_i + master_cmd_dim[cmd[0]] + 4) > (mmasterLP.send_queue_o + MM_QUEUE_LEN_LP))))
      {
        pthread_mutex_unlock(&master_send_queue_mutex);
        support_logmm("MM (LP): Coda piena, comando perso.");
        return -1;
      }
      
      /* Poiché la coda è unica, devo inserire anche il modulo master di destinazione */
      mmasterLP.send_queue[master_incLP(&mmasterLP.send_queue_i)] = numMM;
      mmasterLP.send_queue[master_incLP(&mmasterLP.send_queue_i)] = master_cmd_dim[cmd[0]] + 1;
      for(i=0; i<=master_cmd_dim[cmd[0]]; i++)
        mmasterLP.send_queue[master_incLP(&mmasterLP.send_queue_i)] = cmd[i];
      
      sem_post(&master_send_queue_msg);
      pthread_mutex_unlock(&master_send_queue_mutex);
      return 0;
    }
    else if(cmd[0] == 36)
    {
      /* Il comando 36 è in realtà un comando 35 prioritario. */
      cmd[0] = 35;
    }
    else if(cmd[0] == 37)
    {
      /* Il comando 37 è in realtà il comando 36 di impostazione Texas,
         solo che per errore il 36 è stato riusato per il 35 prioritario. */
      cmd[0] = 36;
    }
    
    if(((mmaster[numMM].send_queue_i < mmaster[numMM].send_queue_o) &&
       ((mmaster[numMM].send_queue_i + master_cmd_dim[cmd[0]] + 4) > mmaster[numMM].send_queue_o)) ||
       ((mmaster[numMM].send_queue_i > mmaster[numMM].send_queue_o) &&
       ((mmaster[numMM].send_queue_i + master_cmd_dim[cmd[0]] + 4) > (mmaster[numMM].send_queue_o + MM_QUEUE_LEN))))
    {
      char m[40];
      
      pthread_mutex_unlock(&master_send_queue_mutex);
      sprintf(m, "MM (%2d): Coda piena, comando perso.", numMM);
      support_logmm(m);
#if 0
#warning DEBUG
  support_logmm("skip");
#endif
      return -1;
    }
      
    //mmaster[numMM].send_queue[master_inc(&mmaster[numMM].send_queue_i)] = numMM;
    mmaster[numMM].send_queue[master_inc(&mmaster[numMM].send_queue_i)] = master_cmd_dim[cmd[0]] + 1;
    for(i=0; i<=master_cmd_dim[cmd[0]]; i++)
      mmaster[numMM].send_queue[master_inc(&mmaster[numMM].send_queue_i)] = cmd[i];
  }
  else	/* LARA */
  {
#if 0
#warning DEBUG
  sprintf(log, "MM (%2d) <Q ", numMM);
  y = cmd[4] + 5;
  if(y > 20) y = 20;
  for(x=0; x<y; x++)
    sprintf(log+11+(x<<1), "%02x", cmd[x]);
  support_logmm(log);
  sprintf(log, "--- %d %d %d", master_send_queue_i, master_send_queue_o, cmd[4] + 8);
  support_logmm(log);
#endif
  
    if(((mmaster[numMM].send_queue_i < mmaster[numMM].send_queue_o) &&
       ((mmaster[numMM].send_queue_i + cmd[4] + 8) > mmaster[numMM].send_queue_o)) ||
       ((mmaster[numMM].send_queue_i > mmaster[numMM].send_queue_o) &&
       ((mmaster[numMM].send_queue_i + cmd[4] + 8) > (mmaster[numMM].send_queue_o + MM_QUEUE_LEN))))
    {
      char m[40];
      
      pthread_mutex_unlock(&master_send_queue_mutex);
      sprintf(m, "MM (%2d): Coda piena, comando perso.", numMM);
      support_logmm(m);
#if 0
#warning DEBUG
  support_logmm("skip");
#endif
      return -1;
    }
      
    //mmaster[numMM].send_queue[master_inc(&mmaster[numMM].send_queue_i)] = numMM;
    mmaster[numMM].send_queue[master_inc(&mmaster[numMM].send_queue_i)] = cmd[4] + 5;
    for(i=0; i<(cmd[4]+5); i++)
      mmaster[numMM].send_queue[master_inc(&mmaster[numMM].send_queue_i)] = cmd[i];
  }
  
  sem_post(&master_send_queue_msg);
  pthread_mutex_unlock(&master_send_queue_mutex);
#if 0
#warning DEBUG
  sprintf(log, "MM (%2d) <Q Ok", numMM);
  support_logmm(log);
#endif
  return 0;
}

#if 0
unsigned char master_slowdown()
{
  int dim;
  
  dim = master_send_queue_i - master_send_queue_o;
  if(dim<0) dim += sizeof(master_send_queue);
  
  if(dim > (sizeof(master_send_queue) - 200)) return 2;
  return 0;
}
#endif

static void* master_send(void *null)
{
  int mm, mmp, i, len, x, y;
  unsigned char cmd;
  unsigned char mmcmd[32];
  int mmprio[MM_NUM*2];
  char log[96];
  
//  pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
  
  debug_pid[DEBUG_PID_MSEND] = support_getpid();
  sprintf(log, "Master send() [%d]", debug_pid[DEBUG_PID_MSEND]);
  support_log(log);
  
  mmp = 0;
  /* Calcolo la tabella di priorità in base alla popolazione dei cestelli,
     che prevedono 4 moduli per cestello. Così oltre a fare rotazione sui
     singoli moduli, faccio rotazione anche sui cestelli. */
  /* La tabella è grande il doppio, così non mi devo preoccupare degli
     overflow di indicizzazione. */
  for(i=0; i<MM_NUM*2; i++)
    mmprio[i] = (((i*4)&0x0c) + ((i/4)&0x03)) & (MM_NUM-1);
  
  while(1)
  {
    if(MMPresente[0] == 0)
    {
      sem_getvalue(&master_send_queue_msg, &i);
      if(!i)
      {
        i = open("/proc/driver/mmaster/timeout/single", O_RDWR);
        write(i, "200", 4);
        close(i);
      }
    }
    
    /* Se ci sono messaggi in coda, verifico se sono
       comandi ritardati. Se ci sono comandi in attesa che
       possono essere spediti immediatamente, lo faccio ed
       esaurisco tutto quello che posso. Se tutte le code
       con comandi sono in attesa allora aspetto 10ms,
       decremento il contatore di attesa e poi verifico
       nuovamente. */
    mm = 0;
    for(i=0; i<MM_NUM; i++)
    {
      if(mmaster[i].ritardo)
        mm++;
      else if((mmaster[i].send_queue_o != mmaster[i].send_queue_i) ||
              //(mmaster[i+MM_NUM].send_queue_o != mmaster[i+MM_NUM].send_queue_i) ||
              ((mmasterLP.send_queue_o != mmasterLP.send_queue_i) && (mmasterLP.send_queue[mmasterLP.send_queue_o] == i)) ||
              mmaster[i].sincro)
      {
        /* Ho un comando che può essere spedito immediatamente. */
        mm = 0;
        break;
      }
    }
    
    /* Se ho solo code ritardate, gestisco i contatori */
    if(mm)
    {
      usleep(10000);
      for(i=0; i<MM_NUM; i++) if(mmaster[i].ritardo) mmaster[i].ritardo--;
      continue;
    }
    
    /* Non ci sono comandi in coda (quindi posso aspettare per sempre)
       oppure ci sono comandi che possono essere inviati immediatamente
       (quindi la wait non è bloccante). */
    sem_wait(&master_send_queue_msg);
    
    /* sincro */
    i = 0;
    for(mm=0; mm<MM_NUM; mm++)
      if(mmaster[mm].sincro)
      {
        cmd = 0x55;
        if(MMPresente[mm] != 0xff)
          if(write(mmaster[mm].fd, &cmd, 1) < 0)
            master_notx(mm);
         
        i++;
        mmaster[mm].sincro--;
        if(mmaster[mm].sincro) sem_post(&master_send_queue_msg);
      }
    if(i) continue; /* skip normal operations */
    
    /* normal */
    pthread_mutex_lock(&master_send_queue_mutex);
#if 0    
    mm = master_send_queue[master_inc(&master_send_queue_o)];
    len = master_send_queue[master_inc(&master_send_queue_o)];
    for(i=0; i<len; i++)
      mmcmd[i] = master_send_queue[master_inc(&master_send_queue_o)];
#else
    /* Qui sono sicuro di avere comandi che possono essere inviati immediatamente. */
    /* (03-08-2016) Non è vero, potrebbe essere stato appena accodato un comando che
       ha svegliato la sem_wait ma che è destinato ad un modulo ritardato.
       Non devo andare a toccare la coda LP! */
    for(i=0; i<MM_NUM; i++)
    {
      mm = mmprio[mmp+i];
      if((mmaster[mm].send_queue_o != mmaster[mm].send_queue_i) &&
         !mmaster[mm].ritardo) break;
    }
    if(i == MM_NUM)
    {
      /* Non ho nulla nella coda primaria, vado nella coda secondaria. */
#if 0
      for(i=0; i<MM_NUM; i++)
      {
        mm = mmprio[mmp+i] + MM_NUM;
        if((mmaster[mm].send_queue_o != mmaster[mm].send_queue_i) &&
           !mmaster[mm].ritardo) break;
      }
#else
      /* Ho già verificato che il modulo master in questione non sia ritardato */
      /* (03-08-2016) Non è necessariamente vero, potrebbe essere un comando
         appena accodato, mentre la verifica è precedente. */
      if((mmasterLP.send_queue_i != mmasterLP.send_queue_o) &&
         !mmaster[mmasterLP.send_queue[mmasterLP.send_queue_o]].ritardo)
      {
        mm = mmasterLP.send_queue[master_incLP(&mmasterLP.send_queue_o)];
        len = mmasterLP.send_queue[master_incLP(&mmasterLP.send_queue_o)];
        for(i=0; i<len; i++)
          mmcmd[i] = mmasterLP.send_queue[master_incLP(&mmasterLP.send_queue_o)];
      }
      else
        len = 0;
#endif
    }
    else
    {
      len = mmaster[mm].send_queue[master_inc(&mmaster[mm].send_queue_o)];
      for(i=0; i<len; i++)
        mmcmd[i] = mmaster[mm].send_queue[master_inc(&mmaster[mm].send_queue_o)];
    }
    
    mmp = (mmp + 1) % MM_NUM;
#endif
    pthread_mutex_unlock(&master_send_queue_mutex);
    
    if(!len)
    {
      /* Attenzione, ho bruciato il semaforo senza poi avere la possibilità
         di inviare qualcosa, occorre ripristinare il semaforo. */
      sem_post(&master_send_queue_msg);
      continue;
    }
    
#if 0
    if(mm >= MM_NUM)
    {
      /* Il comando arriva dalla coda secondaria. E' stato estratto completamente,
         posso quindi normalizzare il numero di modulo. */
      mm -= MM_NUM;
    }
#endif
    
    if((delphi_modo == DELPHITIPO) && (len>1) && (mmcmd[0]!=3) && (((mm<<5)+mmcmd[1]) > MASTER_LIMIT_TIPO)) continue;

    if(MMSuspend[mm])
    {
      /* Il modulo master si sta reinizializzando, devo lasciar passare solo
         i comandi di inizializzazione mentre tutti gli altri vengono riaccodati
         in modo da non andare persi. */
      /* Per tutti i moduli master tranne il Tebe lascio passare solo i comandi
         fino al 3, per il modulo master Tebe lascio passare solo la richiesta
         di lista nodi 7. */
      /* Se vengono inviati comandi nei momenti sbagliati è facile causare un
         NoTxMM, quindi innescando un loop. Il modulo Tebe non dovrebbe avere
         questo problema, però i comandi andrebbero probabilmente persi e quindi
         preferisco aspettare sempre che il modulo finisca di sistemarsi. */
         
      /* La sospensione della linea viene comunque sbloccata per timeout, quindi
         ad un certo punto eventuali comandi rimasti in coda senza più destinatario
         vengono alla fine smaltiti. */
      
      /* Per il modulo master 0 devo far passare anche il comando 6. */
      /* E anche il comando 36 di impostazione Texas. */
      if(((MMPresente[mm] != MM_LARA) && (mmcmd[0] > 3) && (mmcmd[0] != 6) && (mmcmd[0] != 36)) ||
         ((MMPresente[mm] == MM_LARA) && (mmcmd[0] != 7)))
      {
        master_queue(mm, mmcmd);
        continue;
      }
    }
      
    if(MMPresente[mm] != 0xff)
    {
      if((MMPresente[mm] == MM_LARA) && (mmcmd[0] == TEBE_SET_MASTER))
      {
        /* Il comando TEBE_SET_MASTER viene accodato con tutta la parte
           di indirizzamento e lunghezza messaggio solito, qui viene
           invece troncato al solo codice necessario. */
        len = 1;
      }
      else if((MMPresente[mm] != MM_LARA) && (mmcmd[0] == 3))
      {
        /* Riabilito l'invio dei comandi normali */
        //MMSuspend[mm] = 0;
        /* Prima di riabilitare attendo ancora un paio di secondi per
           dare tempo alle periferiche non presenti di segnalare la
           manomissione. */
        MMSuspendDelay[mm] = 20;
      }
      
      /* Attenzione: durante un periodo di sospensione, le periferiche
         risultano in linea anche se in realtà sono manomesse, poiché
         non è ancora ritornato l'evento di manomissione, ed il programma
         utente potrebbe accodare richieste verso queste.
         Occorre quindi verificare prima dell'invio che la periferica
         sia effettivamente esistente. */
      if(MMPresente[mm] != MM_LARA)
      {
        /* Tutti i comandi >3 hanno il secondo byte con l'indirizzo periferica,
           tranne il 55h gestito a parte ed alcuni messaggi non gestiti proprio.
           Il comando 6 va lasciato comunque passare. */
        /* C'è anche il comando 33 per la ridondanza! */
        /* E anche il comando 36 per l'impostazione Texas! */
        if((mmcmd[0] > 3) && (mmcmd[0] != 6) && (mmcmd[0] != 33) && (mmcmd[0] != 36) &&
          !master_periph_present((mm<<5)|mmcmd[1])) continue;
      }
         
      sprintf(log, "MM (%2d) <- ", mm);
      y = len;
      if(y > 40) y = 40;
      for(x=0; x<y; x++)
        sprintf(log+11+(x<<1), "%02x", mmcmd[x]);
      support_logmm(log);
//printf("MM (%2d) <- %s\n", mm, log);
      
      pthread_mutex_lock(&master_reset_mutex);
      if(write(mmaster[mm].fd, mmcmd, len) < 0)
        master_notx(mm);
      pthread_mutex_unlock(&master_reset_mutex);
      
#if 0
      if(((mmcmd[0] == 8)||(mmcmd[0] == 15)) && (MMPresente[mm] > 0) && (TipoPeriferica[(mm<<5)+mmcmd[1]] != 0xff))
        usleep(200000);
#else
      if(((mmcmd[0] == 8)||(mmcmd[0] == 15)) && (MMPresente[mm] == 2) && (TipoPeriferica[(mm<<5)+mmcmd[1]] != 0xff))
        mmaster[mm].ritardo = 20;
#endif
      
      else if(MMPresente[mm] == MM_LARA)
        //usleep(20000);
        mmaster[mm].ritardo = 2;
      else
        /* Distanzio comunque sempre un minimo TUTTI i comandi. */
        /* Mi serve anche per la gestione delle priorità e delle temporizzazioni. */
        mmaster[mm].ritardo = 1;
    }
  }
  
  return NULL;
}

static void master_set_status(int line, int status)
{
  unsigned char bit;
  
  bit = 1 << (line & 0x7);
  if(status)
    StatoPeriferica[line >> 3] |= bit;
  else
    StatoPeriferica[line >> 3] &= ~bit;
}

void master_queue_sensor_event(int ind, unsigned char event)
{
  unsigned char ev[8];
  
  ev[0] = Evento_Sensore;
  ev[1] = ind >> 8;
  ev[2] = ind & 0xff;
  ev[3] = event;
  ev[4] = MU_ini;
  codec_queue_event(ev);
}

void master_set_sabotage(int ind, unsigned char stat, unsigned char event)
{
  unsigned char ev[4];
  int status, bit = 0;
  
  if((MMPresente[ind>>8] != MM_LARA) && (SE[ind] & bitOOS)) return;
  
  stat &= 1;

#ifdef VARIANTE_SCORRANO
  if(stat) return;
#endif

  ev[0] = event;

  switch(event)
  {
    case Manomissione_Contenitore:
#ifdef GEST16MM
      ev[1] = ind >> 11;
      ev[2] = ind >> 3;
#else
      ev[1] = ind >> 3;
#endif
      bit = bitPackSabotage;
      break;
    case Manomissione_Dispositivo:
      ev[1] = ind >> 8;
      ev[2] = ind & 0xff;
      bit = bitInputSabotage;
      break;
  }
  
  status = stat * bit;

  if((SE[ind] & bit) ^ status)
  {
    SE[ind] &= ~(bitSabotage | bit);
    SE[ind] |= status;
    if(SE[ind] & bitInputSabotage) SE[ind] |= bitSabotage;
    if((((ind & 7) == 0) || ((ind & 7) == 1)) && (SE[ind] & bitPackSabotage))
      SE[ind] |= bitSabotage;
    
    if(status)
    {
      if(event == Manomissione_Contenitore)
      {
        M_CONTENITORE = (ind >> 3);
        SEmu[ind] |= bitSEmuPackSab;
      }
      else
        SEmu[ind] |= bitSEmuInSab;
      if(!(SE[ind] & bitGestSabotage)) SE[ind] |= (bit << 1);	// fronte di salita
      SE[ind] |= bitGestSabotage;
      SE_time_sabotage[ind] = time(NULL);
#if 0
      if(!(SE[ind] & bitMUNoSabotage)) codec_queue_event(ev);
#else
//      if(!(SE[ind] & bitMUNoSabotage) && !((event == Manomissione_Dispositivo) && (TipoPeriferica[ind>>3] == SAN) && (CONF_SA == 0))) codec_queue_event(ev);
      if(!(SE[ind] & bitMUNoSabotage) && !(SANanalogica[ind>>3])) codec_queue_event(ev);
#endif
      if(database_testandset_MU(ind, bitMUSabotage))
      {
//        if(!((event == Manomissione_Dispositivo) && (TipoPeriferica[ind>>3] == SAN) && (CONF_SA == 0)))
        if(!((event == Manomissione_Dispositivo) && SANanalogica[ind>>3]))
        {
          if(SE[ind] & bitMUNoSabotage) codec_queue_event(ev);
          master_queue_sensor_event(ind, MU_sabotage);
        }
      }
    }
    else
    {
      ev[0]++;
      if(!(SE[ind] & bitMUNoSabotage))
      {
        if(event == Manomissione_Contenitore)
          SEmu[ind] &= ~bitSEmuPackSab;
        else
          SEmu[ind] &= ~bitSEmuInSab;
        SE[ind] &= ~bitGestSabotage;
        SE[ind] |= (bit << 2);	// fronte di discesa
//        if(!((event == Manomissione_Dispositivo) && (TipoPeriferica[ind>>3] == SAN) && (CONF_SA == 0)))
        if(!((event == Manomissione_Dispositivo) && SANanalogica[ind>>3]))
          codec_queue_event(ev);
      }
      else if(!(SE[ind] & bitMUSabotage) && (SE[ind] & bitGestSabotage))
      {
        if(event == Manomissione_Contenitore)
          SEmu[ind] &= ~bitSEmuPackSab;
        else
          SEmu[ind] &= ~bitSEmuInSab;
        SE[ind] &= ~bitGestSabotage;
        SE[ind] |= (bit << 2);	// fronte di discesa
//        if(!((event == Manomissione_Dispositivo) && (TipoPeriferica[ind>>3] == SAN) && (CONF_SA == 0)))
        if(!((event == Manomissione_Dispositivo) && SANanalogica[ind>>3]))
        codec_queue_event(ev);
      }
    }
  }
}

void master_set_failure(int ind, unsigned char stat, unsigned char event)
{
  unsigned char ev[4];
  int status, bit = 0;
  
  stat &= 1;
  
  ev[0] = event;

  switch(event)
  {
    case Guasto_Periferica:
#ifdef GEST16MM
      ev[1] = ind >> 11;
      ev[2] = ind >> 3;
#else
      ev[1] = ind >> 3;
#endif
      ind += 2;
      if(SE[ind] & bitOOS) return;
      bit = bitGenFailure;
      break;
    case Guasto_Sensore:
      if(SE[ind] & bitOOS) return;
      ev[1] = ind >> 8;
      ev[2] = ind & 0xff;
      bit = bitInputFailure;
      break;
  }
  
  status = stat * bit;
  
  if((SE[ind] & bit) ^ status)
  {
    SE[ind] &= ~(bitFailure | bit);
    SE[ind] |= status;
    if(SE[ind] & bitInputFailure) SE[ind] |= bitFailure;
    if(((ind & 7) == 2) && (SE[ind] & bitGenFailure))
      SE[ind] |= bitFailure;
    
    if(status)
    {
      if(ev[0] == Guasto_Periferica)
      {
        GUASTO = (ind-2)>>3;
        SEmu[ind] |= bitSEmuGenFail;
      }
      else
        SEmu[ind] |= bitSEmuInFail;
      if(!(SE[ind] & bitGestFailure)) SE[ind] |= (bit << 1);	// fronte di salita
      SE[ind] |= bitGestFailure;
      SE_time_failure[ind] = time(NULL);
      
#if 0
      if(!(SE[ind] & bitMUNoFailure)) codec_queue_event(ev);
#else
//      if(!(SE[ind] & bitMUNoFailure) && !((event == Guasto_Sensore) && (TipoPeriferica[ind>>3] == SAN) && (CONF_SA == 0))) codec_queue_event(ev);
        if(!(SE[ind] & bitMUNoSabotage) && !(SANanalogica[ind>>3])) codec_queue_event(ev);
#endif      
      if(database_testandset_MU(ind, bitMUFailure))
      {
//        if(!((event == Guasto_Sensore) && (TipoPeriferica[ind>>3] == SAN) && (CONF_SA == 0)))
        if(!((event == Guasto_Sensore) && SANanalogica[ind>>3]))
        {
          if(SE[ind] & bitMUNoFailure) codec_queue_event(ev);
          master_queue_sensor_event(ind, MU_failure);
        }
      }
    }
    else
    {
      if(ev[0] == Guasto_Sensore)
        ev[0] = Fine_Guasto_Sensore;
      else
        ev[0]++;
      if(!(SE[ind] & bitMUNoFailure))
      {
        SE[ind] |= (bit << 2);	// fronte di discesa
        SE[ind] &= ~bitGestFailure;
//        if(!((event == Guasto_Sensore) && (TipoPeriferica[ind>>3] == SAN) && (CONF_SA == 0)))
        if(!((event == Guasto_Sensore) && SANanalogica[ind>>3]))
        codec_queue_event(ev);
      }
      else if(!(SE[ind] & bitMUFailure) && (SE[ind] & bitGestFailure))
      {
        SE[ind] |= (bit << 2);	// fronte di discesa
        SE[ind] &= ~bitGestFailure;
//        if(!((event == Guasto_Sensore) && (TipoPeriferica[ind>>3] == SAN) && (CONF_SA == 0)))
        if(!((event == Guasto_Sensore) && SANanalogica[ind>>3]))
        codec_queue_event(ev);
      }
    }
  }
}

void master_set_alarm(int ind, unsigned char status, char print)
{
  unsigned char ev[8];
  
  status &= 1;
  /* Applica la maschera di inversione dell'allarme. */
  status ^= ((SEinvAlm[ind>>3]>>(ind&7)) & 1);

  ev[1] = ind >> 8;
  ev[2] = ind & 0xff;
  
  if(SEmu[ind] & bitSEmuTest)
  {
    if((SE[ind] & bitTest) && status)
    {
      if(SE[ind] & bitFailure)
        master_set_failure(ind, 0, Guasto_Sensore);
      
      SE[ind] &= ~(bitTest | bitFailure);
      AT[ind] |= bitAbilReq;
    }
  }
//  else if (((SE[ind] & bitAlarm) ^ status) ||
//           (status && !(SE[ind] & bitGestAlarm)) ||
//           (!status && (SE[ind] & bitGestAlarm)))
  else if ((SE[ind] & bitAlarm) ^ status)
  {
    SE[ind] &= ~bitAlarm;
    SE[ind] |= status;
    
    if(!(SE[ind] & bitOOS))
    {
      ev[3] = Zona_SE[ind];
      if(status)
      {
        ev[0] = Allarme_Sensore;
        if(!(SE[ind] & bitMUNoAlarm)) SE[ind] |= bitLHF;
        SEmu[ind] |= bitSEmuAlarm;
        SE_time_alarm[ind] = time(NULL);
        
        /* 08/07/10 Anche se master_zones() è inserita nella protezione del lock
           in master_loop, questa impostazione serve a rigenerare il fronte su
           una zona già allarmata ogni volta che arriva un nuovo allarme. */
        if((Zona_SE[ind] < sizeof(ZONA)) && (ZONA[Zona_SE[ind]] & bitActive))
          ZONA[Zona_SE[ind]] |= bitLHF;
      }
      else
      {
        ev[0] = Ripristino_Sensore;
        if(!(SE[ind] & bitMUNoAlarm)) SE[ind] |= bitHLF;
        if((Zona_SE[ind] < sizeof(ZONA)) && (ZONA[Zona_SE[ind]] & bitActive))
          ZONA[Zona_SE[ind]] |= bitHLF;
      }
      
      if(print && !(SE[ind] & bitNoAlarm))
      {
        if(status)
        {
          if((Zona_SE[ind] == 0) || (ZONA[Zona_SE[ind]] & bitActive))
          {
            if(!(SE[ind] & bitGestAlarm)) SE[ind] |= bitLHF;
            SE[ind] |= bitGestAlarm;
            if(!(SE[ind] & bitMUNoAlarm)) codec_queue_event(ev);
            if(database_testandset_MU(ind, bitMUAlarm))
            {
              if(SE[ind] & bitMUNoAlarm) codec_queue_event(ev);
              master_queue_sensor_event(ind, MU_alarm);
            }
          }
        }
        else
        {
          if(!(SE[ind] & bitMUNoAlarm))
          {
            if((Zona_SE[ind] == 0) || (ZONA[Zona_SE[ind]] & bitActive))
            {
              SE[ind] |= bitHLF;
              SE[ind] &= ~bitGestAlarm;
              codec_queue_event(ev);
            }
          }
          else
          {
            if(!(SE[ind] & bitMUAlarm) && (SE[ind] & bitGestAlarm))
            {
              SE[ind] |= bitHLF;
              SE[ind] &= ~bitGestAlarm;
              codec_queue_event(ev);
            }
          }
        }
      }
    }
  }
}

static void master_send_analog_threshold(int linea)
{
  if(!master_send_threshold) return;
  
  unsigned char cmd[8];
  int cc;
  SogliaList *sl;
  
  if(master_periph_present(linea))
  {
    cmd[0] = 15;
    cmd[1] = linea & 0x1f;
    cmd[7] = 0;
    if(MMPresente[linea >> 5] != 4)
    {
      cmd[2] = 1;
      cmd[3] = DVSSAM[0];
      cmd[4] = 0;
      cmd[5] = 0;
      cmd[6] = DVSSAM[1];
      master_queue(linea >> 5, cmd);
      cmd[2] = 2;
      cmd[3] = DVSSAG[0];
      cmd[6] = DVSSAG[1];
      master_queue(linea >> 5, cmd);
    }
    if(DEFPT[linea][0] || DEFPT[linea][1])
    {
      cmd[2] = 0;
      for(cc=0;cc<4;cc++) cmd[cc+3] = DEFPT[linea][cc];
      master_queue(linea >> 5, cmd);
      cmd[2] = 3;
      if(CONF_SA)
        for(cc=0;cc<4;cc++) cmd[cc+3] = CNFG_SA[0][cc];
      else
        for(cc=0;cc<4;cc++) cmd[cc+3] = CNFG_SA[linea][cc];
      cmd[7] = 30;	// soglia bassa fissa
      master_queue(linea >> 5, cmd);
    }
    for(sl=SogliaSingola; sl; sl=sl->next)
    {
      if((sl->sens >> 3) == linea)
      {
        cmd[2] = (sl->sens & 0x07) + 8;
        memcpy(cmd+3, sl->so, 5);
        master_queue(linea >> 5, cmd);
      }
    }
  }
}

void master_send_sc8c_config(int linea)
{
  unsigned char cmd[8];
  int i, cc;
  SogliaList *sl;
  
  if(master_periph_present(linea) && (TipoPeriferica[linea] == SC8C))
  {
    cmd[0] = 38;
    cmd[1] = linea & 0x1f;
    for(sl=SogliaSingola; sl; sl=sl->next)
    {
      if((sl->sens >> 3) == linea)
      {
        cmd[2] = (sl->sens & 0x07);
        memcpy(cmd+3, sl->so, 5);	// tipo + 4 parametri
        master_queue(linea >> 5, cmd);
      }
    }
  }
}

void master_periph_restore(int basedev, unsigned char *ev)
{
  database_lock();
  
  basedev++;
  SE[basedev] &= ~(bitSabotage | bitLineSabotage);
  
  if(!(SE[basedev] & bitMUNoSabotage) ||
     ((SE[basedev] & bitMUNoSabotage) && !(SE[basedev] & bitMUSabotage) && (SE[basedev] & bitGestSabotage)))
  {
     SE[basedev] |= bitLineHLF;
     SE[basedev] &= ~bitGestSabotage;
     ev[0] = Periferica_Ripristino;
     codec_queue_event(ev);
  }
  
  database_unlock();
}

static const int analog_lut[8] = {1, 2, 0, -1, -1, 4, 3, 5};
static void master_gest_analog(int ind, int thr)
{
  int oldlvl, lvl;
  unsigned char ev[8];
  
  if((SANanalogica[ind >> 3]) && !(SE[ind] & bitOOS))
  {
    oldlvl = analog_lut[SEanalog[ind] & 0x7];
    lvl = analog_lut[thr];

    ev[0] = Evento_Sensore;
    ev[1] = ind >> 8;
    ev[2] = ind & 0xff;
    
    if(oldlvl < lvl)
    {
      ev[4] = 7;
      for(; oldlvl<lvl; oldlvl++)
      {
        ev[3] = oldlvl+4;
        codec_queue_event(ev);
      }
    }
    else if(oldlvl > lvl)
    {
      ev[4] = 6;
      for(; oldlvl>lvl; oldlvl--)
      {
        ev[3] = oldlvl+3;
        codec_queue_event(ev);
      }
    }
    
    SEanalog[ind] = thr;
  }
}

static void* master_parse(int numMM)
{
  int numMM32 = numMM << 5;
  int i, data, tmp, linea = 0, basedev = 0;
  unsigned char ev[20], cmd[20];
  FILE *fp;
  
  {
    data = mmaster[numMM].cmd_recv[0];

    if(data>2 && data!=10 && data!=17 && data!=20)
    {
      linea = numMM32 + mmaster[numMM].cmd_recv[1];
      if((delphi_modo == DELPHITIPO) && (linea > MASTER_LIMIT_TIPO)) return NULL;
      basedev = linea << 3;
#ifdef GEST16MM
      ev[1] = linea >> 8;
      ev[2] = linea;
#else
      ev[1] = linea;
#endif
    }

    switch(data)
    {
      case 1:
        if(!((MMPresente[numMM] == 0xff) || (MMPresente[numMM] == mmaster[numMM].cmd_recv[1]))) break;
        
        /* La lista periferiche può impiegare fino a 4 secondi (+4 per il ritardo aggiunto al reset). */
        if(((master_behaviour == MASTER_NORMAL)) || (master_behaviour == MASTER_ACTIVE))
          timeout_on(master_timeout, NULL, NULL, 0, 80);
        else
          timeout_on(master_timeout, NULL, NULL, 0, 400);
        
        MMPresente[numMM] = mmaster[numMM].cmd_recv[1];
        ev[0] = Tipo_ModuloMaster;
        ev[1] = numMM;
        ev[2] = MMPresente[numMM];
        codec_queue_event(ev);
        
        /* Per sicurezza sospendo il modulo, potrei dover gestire un reset
           del modulo master senza un precedente NoTx. */
        if(master_behaviour == MASTER_NORMAL)
          MMSuspend[numMM] |= 1;
        
        /* Appende il tipo di MM solo la prima volta. */
        if(!libuser_started)
        {
          fp = fopen(ADDROOTDIR(VersionFile), "a");
          fprintf(fp, "mm%d: tipo %d\n", numMM, MMPresente[numMM]);
          fclose(fp);
        }
        
        if(MMPresente[numMM] == MM_LARA)
        {
          MMPresente[numMM+1] = mmaster[numMM].cmd_recv[1];
          ev[1] = numMM+1;
          codec_queue_event(ev);
          MMPresente[numMM+2] = mmaster[numMM].cmd_recv[1];
          ev[1] = numMM+2;
          codec_queue_event(ev);
        }
        else if(MMPresente[numMM] == 0)
        {
          /* Se il modulo master 0 è multiplo, il timeout impostato
             non è influente perchè conta quello multiplo. */
          i = open("/proc/driver/mmaster/timeout/single", O_RDWR);
          write(i, "200", 4);
          close(i);
          
          if(MMTexas[numMM])
          {
            cmd[0] = 37;	// Attenzione: sul campo andrà come 36.
            master_queue(numMM, cmd);
          }
        }
        RILEVATO_MM = numMM;
        master_switch[numMM] = 0;
        if(MMPresente[numMM] == MM_LARA)
        {
          lara_init(numMM);
        }
        else if((master_behaviour == MASTER_NORMAL)||(master_behaviour == SLAVE_ACTIVE))
        {
          cmd[0] = 2;
          master_queue(numMM, cmd);
        }
        else if(master_behaviour == MASTER_ACTIVE)
        {
          cmd[0] = 33;
          master_queue(numMM, cmd);
        }
        else if(master_behaviour == MASTER_STANDBY)
        {
          /* wait for code 25 */
        }
        break;
      case 2:
        /* Eventuali manomissioni arrivano entro 2 secondi. */
        timeout_on(master_timeout, NULL, NULL, 0, 20);
        
        ev[0] = Periferiche_presenti;
#ifdef GEST16MM
        ev[1] = numMM32 >> 8;
        ev[2] = numMM32;
        for(i=3; i<11; i++) ev[i] = mmaster[numMM].cmd_recv[i-2];
#else
        ev[1] = numMM32;
        for(i=2; i<10; i++) ev[i] = mmaster[numMM].cmd_recv[i-1];
#endif
        codec_queue_event(ev);
#ifdef GEST16MM
        ev[2] = numMM32 | 0x10;
        for(i=3; i<11; i++) ev[i] = mmaster[numMM].cmd_recv[i+6];
#else
        ev[1] = numMM32 | 0x10;
        for(i=2; i<10; i++) ev[i] = mmaster[numMM].cmd_recv[i+7];
#endif
        codec_queue_event(ev);
        cmd[0] = 3;
        for(i=0; i<16; i++)
          cmd[i+1] = ((TipoPeriferica[numMM32 | (i << 1)]) & 0xf) |
                     (((TipoPeriferica[(numMM32 | (i << 1)) + 1]) & 0xf) << 4);
        master_queue(numMM, cmd);
        if(MMPresente[numMM] == 0)
        {
          cmd[0] = 6;
          for(i=16; i<32; i++)
            if((TipoPeriferica[numMM32 | i] == SEN) ||
               (TipoPeriferica[numMM32 | i] == TAS) ||
               (TipoPeriferica[numMM32 | i] == TS2)) break;
          if(i<32)
          {
            cmd[1] = i;
            master_queue(numMM, cmd);
          }
          for(i=0; i<16; i++)
            if((TipoPeriferica[numMM32 | i] == SEN) ||
               (TipoPeriferica[numMM32 | i] == TAS) ||
               (TipoPeriferica[numMM32 | i] == TS2)) break;
          if(i<16)
          {
            cmd[1] = i;
            master_queue(numMM, cmd);
          }
        }
        /* Se sono presenti SC8R2 devo inizializzarli. */
        for(i=0; i<32; i++) master_SC8R2_init(numMM32+i);
        break;
      case 3:
        /* Rinfresco l'attesa prima dell'avvio */
        timeout_on(master_timeout, NULL, NULL, 0, 30);
        
        ev[0] = Periferica_Incongruente;
#ifdef GEST16MM
        ev[3] = mmaster[numMM].cmd_recv[2];
#else
        ev[2] = mmaster[numMM].cmd_recv[2];
#endif
        codec_queue_event(ev);
        master_set_status(linea, 0);
        break;
      case 4:
        /* Rinfresco l'attesa prima dell'avvio */
        timeout_on(master_timeout, NULL, NULL, 0, 30);
        
        ev[0] = Periferica_Manomessa;
        M_LINEA = linea;
        master_set_status(linea, 0);
        database_lock();
        
        master_set_sabotage(basedev, 0, Manomissione_Contenitore);
        master_set_failure(basedev, 0, Guasto_Periferica);
        for(i=0; i<master_sensor_valid(basedev); i++)
        {
          master_set_alarm(basedev+i, 0, 1);
          master_set_sabotage(basedev+i, 0, Manomissione_Dispositivo);
          master_set_failure(basedev+i, 0, Guasto_Sensore);
        }
        
        basedev++;
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
        if(SE[basedev] & bitMUNoSabotage)
        {
          basedev--;
          for(i=0; i<master_sensor_valid(basedev); i++)
            master_set_failure(basedev+i, 1, Guasto_Sensore);
        }
        database_unlock();
        /* Comunico alla gestione radio la perdita della periferica 868 */
        if(master_radio868_p) master_radio868_p(numMM, mmaster[numMM].cmd_recv);
        break;
      case 5:
        if(!numMM && MMPresente[0] == 0)
        {
          i = open("/proc/driver/mmaster/timeout/single", O_RDWR);
          write(i, "50", 3);
          close(i);
        }
        master_set_status(linea, 1);
        for(i=0; i<8; i++) AT[basedev + i] |= bitAbilReq;
        master_periph_restore(basedev, ev);
        master_send_sc8c_config(linea);
        master_SC8R2_init(linea);
        RIPRISTINO_LINEA = linea;
        if(SE[basedev+1] & bitMUNoSabotage)
          for(i=0; i<master_sensor_valid(basedev); i++)
            master_set_failure(basedev+i, 0, Guasto_Sensore);
        break;
      case 6:
        master_set_status(linea, 1);
        master_send_analog_threshold(linea);
        AT[basedev] |= bitAbilReq;
        master_periph_restore(basedev, ev);
        RIPRISTINO_LINEA = linea;
        if(SE[basedev+1] & bitMUNoSabotage)
          for(i=0; i<master_sensor_valid(basedev); i++)
            master_set_failure(basedev+i, 0, Guasto_Sensore);
      case 7:
        data = mmaster[numMM].cmd_recv[2];
        database_lock();
        if(!(data & 0x80))
        {
          master_set_sabotage(basedev, data, Manomissione_Contenitore);
          if((TipoPeriferica[linea] == SS) || (TipoPeriferica[linea] == SC1))
            master_set_alarm(basedev, data >> 1, !(SE[basedev+1] & bitAlarm));
          else if(TipoPeriferica[linea] != SC8)
            master_set_failure(basedev, data >> 1, Guasto_Periferica);
        }
        tmp = mmaster[numMM].cmd_recv[3];
        if(!(data & 0x40) && (master_sensor_valid(basedev) == 8))
        {
          if(TipoPeriferica[linea] == SC8)
          {
            int tmp2;
            
            tmp2 = (tmp << 4) & 0xff;
            tmp = (tmp >> 4) | tmp2;
          }
          if(!((DEFPT[linea][2] == 1) && !DEFPT[linea][3] && !DEFPT[linea][4]))
          {
            for(i=0; i<8; i++)
            {
              master_set_alarm(basedev + i, tmp, 1);
              tmp >>= 1;
            }
          }
          else
          {
            master_set_alarm(basedev, tmp, !(SE[basedev+1] & bitAlarm));
          }
        }
        database_unlock();
        break;
      case 8:
        data = mmaster[numMM].cmd_recv[2];
        database_lock();
//        if(MMPresente[numMM] != 4)
        {
          master_set_sabotage(basedev, data, Manomissione_Contenitore);
          master_set_failure(basedev, data >> 1, Guasto_Periferica);
        }
        
        for(i=0; i<8; i++)
        {
          if(MMPresente[numMM] != 4)
          {
            tmp = mmaster[numMM].cmd_recv[3] & 0x01;
            tmp |= (mmaster[numMM].cmd_recv[4] & 0x01) << 1;
            if((TipoPeriferica[linea] == SAN) || (TipoPeriferica[linea] == SC8C))
              tmp |= (mmaster[numMM].cmd_recv[5] & 0x01) << 2;
          }
          else
          {
            tmp = mmaster[numMM].cmd_recv[3] & 0x01;
            if(mmaster[numMM].cmd_recv[4] & 0x01)
              tmp = 6 - tmp; 
            if(!tmp)
            {
              tmp = mmaster[numMM].cmd_recv[5] & 0x01;
              if(tmp) tmp = 7;
            }
          }
          
          switch(tmp)
          {
            case 0:	// riposo
              master_set_alarm(basedev + i, 0, 1);
              master_set_sabotage(basedev + i, 0, Manomissione_Dispositivo);
              master_set_failure(basedev + i, 0, Guasto_Sensore);
              break;
            case 1:	// allarme
              if((i > 0) || ((i == 0) && (!ronda_sos_p || !ronda_sos_p(tmp, linea))))
              {
                master_set_alarm(basedev + i, 1, 1);
                master_set_sabotage(basedev + i, 0, Manomissione_Dispositivo);
                master_set_failure(basedev + i, 0, Guasto_Sensore);
              }
              break;
            case 2:	// manomissione linea sensore
              if((TipoPeriferica[linea] == SAN) ||
                 (TipoPeriferica[linea] == SC8C))
              {
                master_set_failure(basedev + i, 1, Guasto_Sensore);
                master_set_alarm(basedev + i, 0, 1);
                master_set_sabotage(basedev + i, 0, Manomissione_Dispositivo);
              }
              else if(!(AlarmSabotage[numMM]^AlarmSabotageInv[linea]))
                master_set_sabotage(basedev + i, 1, Manomissione_Dispositivo);
              else
                master_set_alarm(basedev + i, 1, 1);
              break;
            case 5:	// allarme + tamper sensore
              master_set_alarm(basedev + i, 1, 1);
              if(!(AlarmSabotage[numMM]^AlarmSabotageInv[linea]))
                master_set_sabotage(basedev + i, 1, Manomissione_Dispositivo);
              else
                master_set_sabotage(basedev + i, 0, Manomissione_Dispositivo);
              master_set_failure(basedev + i, 0, Guasto_Sensore);
              break;
            case 6:	// tamper sensore
              if(!(AlarmSabotage[numMM]^AlarmSabotageInv[linea]))
              {
                master_set_alarm(basedev + i, 0, 1);
                master_set_sabotage(basedev + i, 1, Manomissione_Dispositivo);
              }
              else
              {
                master_set_alarm(basedev + i, 1, 1);
                master_set_sabotage(basedev + i, 0, Manomissione_Dispositivo);
              }
              master_set_failure(basedev + i, 0, Guasto_Sensore);
              break;
            case 7:	// guasto
              master_set_failure(basedev + i, 1, Guasto_Sensore);
              master_set_alarm(basedev + i, 0, 1);
              master_set_sabotage(basedev + i, 0, Manomissione_Dispositivo);
              break;
          }
          
          master_gest_analog(basedev + i, tmp);
          
          mmaster[numMM].cmd_recv[2] >>= 1;
          mmaster[numMM].cmd_recv[3] >>= 1;
          mmaster[numMM].cmd_recv[4] >>= 1;
          mmaster[numMM].cmd_recv[5] >>= 1;
        }
        database_unlock();
        break;
      case 9:
        database_lock();
        CHIAVE_TRASMESSA = linea;
        for(i=0; i<5; i++)
          CHIAVE[i] = mmaster[numMM].cmd_recv[2+i];
        database_unlock();
        break;
      case 10:
//        MMPresente[numMM] = 0xff;
        ev[0] = Sospesa_attivita_linea;
        ev[1] = numMM;
        codec_queue_event(ev);
        SOSPESA_LINEA = numMM;
        break;
      case 11:
        database_lock();
        if(!ronda_chiave_falsa_p || !ronda_chiave_falsa_p(linea))
        {
          ev[0] = Chiave_Falsa;
          codec_queue_event(ev);
          CHIAVE_FALSA = linea;
          if(database_testandset_MU(basedev + 3, bitMUFailure))
            master_queue_sensor_event(basedev + 3, MU_failure);
        }
        database_unlock();
        break;
      case 12:
        data = mmaster[numMM].cmd_recv[2];
        tmp = 0;
        for(i=0; i<data; i++)
        {
          tmp *= 10;
          tmp += mmaster[numMM].cmd_recv[3+i];
        }
        i = mmaster[numMM].cmd_recv[3+i];
        database_lock();
        if(data) SE[basedev + 4] = tmp;
        if(i == 0x0B)
        {
          if(data)
          {
            short *codes = (short*)RONDA;

            SE[basedev + 3] = 'T';
#ifndef RONDA_CHECK_CHIAVE
#warning Controllo della chiave utilizzata
            if(ronda_punzonatura_p) ronda_punzonatura_p(0, linea);
#endif
            if(data <= 4)
            {
              for(i=0; i<(sizeof(RONDA)/sizeof(short)); i++)
                if(codes[i] == SE[basedev + 4]) break;
              if(i == (sizeof(RONDA)/sizeof(short))) i = -1;
              if(i >= 0)
              {
                database_set_alarm2(&ME[i+768]);
#ifdef RONDA_CHECK_CHIAVE
                if(ronda_punzonatura_p) ronda_punzonatura_p(i, linea);
#endif
              }
            }
          }
          else
            SE[basedev + 1] |= 0x40;          
        }
        else
        {
          if(data)
            SE[basedev + 3] = i + 'A';
          else
            SE[basedev + 2] |= (1 << i);
        }
        database_unlock();
        break;
      case 13:
        ev[0] = Sentinella_on;
        codec_queue_event(ev);
        database_lock();
        SE[basedev + 1] &= ~0x20;
        master_set_alarm(basedev + 1, 1, 0);
        if(config.Variant == 1) master_set_alarm(basedev + 1, 0, 0);
        database_unlock();
        break;
      case 14:
        ev[0] = Sentinella_off;
        codec_queue_event(ev);
        database_lock();
        master_set_alarm(basedev + 1, 0, 0);
        database_unlock();
        break;
      case 15:
        ev[0] = Sentinella_off_timeout;
        codec_queue_event(ev);
        database_lock();
        SE[basedev + 1] |= 0x20;
        master_set_alarm(basedev + 1, 0, 0);
        database_unlock();
        break;
      case 17:
        ev[0] = Ricezione_codice_errato;
        ev[1] = numMM;
        codec_queue_event(ev);
        if(!mmaster[numMM].sincro)
        {
          mmaster[numMM].sincro = 4;
          sem_post(&master_send_queue_msg);
        }
        else
          mmaster[numMM].sincro += 4;
        break;
      case 18:
        database_lock();
        SE[basedev + 3] = mmaster[numMM].cmd_recv[2];
        database_unlock();
        break;
      case 19:
        database_lock();
        SE[basedev + 4] = 0;
        for(i=0; (i<8) && ((tmp = mmaster[numMM].cmd_recv[2+i]) != 4); i++)
        {
          SE[basedev + 4] *= 10;
          SE[basedev + 4] += (tmp & 0xf);
        }
#ifndef RONDA_CHECK_CHIAVE
#warning Controllo della chiave utilizzata
        if(ronda_punzonatura_p) ronda_punzonatura_p(0, linea);
#endif
        if(!i)
        {
          SE[basedev + 4] = bswap_32(uchartoint(mmaster[numMM].cmd_recv + 6));
          SE[basedev + 5] = bswap_32(uchartoint(mmaster[numMM].cmd_recv + 2)) & 0x00ffffff;
        }
        else if(i <= 4)
        {
          short *codes = (short*)RONDA;

          for(i=0; i<(sizeof(RONDA)/sizeof(short)); i++)
            if(codes[i] == SE[basedev + 4]) break;
          if(i == (sizeof(RONDA)/sizeof(short))) i = -1;
          if(i >= 0)
          {
            database_set_alarm2(&ME[i+768]);
#ifdef RONDA_CHECK_CHIAVE
            if(ronda_punzonatura_p) ronda_punzonatura_p(i, linea);
#endif
          }
        }
        SE[basedev + 1] |= 0x40;
        database_unlock();
        break;
      case 20:
        if((mmaster[numMM].cmd_recv[1] != 0x00) ||
           (mmaster[numMM].cmd_recv[2] != 0x55) ||
           (mmaster[numMM].cmd_recv[3] != 0xaa) ||
           (mmaster[numMM].cmd_recv[4] != 0xff))
        {
          ev[0] = ErrRx_ModuloMaster;
          ev[1] = numMM32;
          codec_queue_event(ev);
        }
        break;
      case 21:
        database_lock();
        V_ANALOG.ind = mmaster[numMM].cmd_recv[1];
        ev[0] = Valori_analogici;
        for(i=0; i<8; i++)
#ifdef GEST16MM
          V_ANALOG.vanl[i] = ev[3+i] = mmaster[numMM].cmd_recv[9-i];
#else
          V_ANALOG.vanl[i] = ev[2+i] = mmaster[numMM].cmd_recv[9-i];
#endif
        database_unlock();
        codec_queue_event(ev);
        break;
      case 22:
        database_lock();
        V_ORADATA.ind = mmaster[numMM].cmd_recv[1];
        V_ORADATA.sec = mmaster[numMM].cmd_recv[2];
        V_ORADATA.min = mmaster[numMM].cmd_recv[3];
        V_ORADATA.ore = mmaster[numMM].cmd_recv[4];
        V_ORADATA.gis = mmaster[numMM].cmd_recv[5];
        V_ORADATA.gio = mmaster[numMM].cmd_recv[6];
        V_ORADATA.mes = mmaster[numMM].cmd_recv[7];
        V_ORADATA.ann = mmaster[numMM].cmd_recv[8];
        database_unlock();
        break;
      case 23:
        ev[0] = StatoBadge;
#ifdef GEST16MM
        memcpy(ev+3, mmaster[numMM].cmd_recv+2, 16);
#else
        memcpy(ev+2, mmaster[numMM].cmd_recv+2, 16);
#endif
        codec_queue_event(ev);
        break;
      case 24:
        database_lock();
        memcpy(Barcode[linea], mmaster[numMM].cmd_recv+8, 10);
        master_set_alarm(basedev + 7, 1, 0);
        master_set_alarm(basedev + 7, 0, 0);
        database_unlock();
        break;
      case 25:
        timeout_on(master_timeout, NULL, NULL, 0, 80);
        master_behaviour = 2 - mmaster[numMM].cmd_recv[1];
        if(!master_switch[numMM])
        {
          master_switch[numMM] = 1;
          cmd[0] = 2;
          master_queue(numMM, cmd);
        }
        else
        {
          master_behaviour += 2;
          if(master_behaviour == MASTER_STANDBY)
            exit(0);
        }
        break;
      case 26:
/*
        if(MMPresente[numMM] != 0xff)
        {
          ev[0] = No_ModuloMasterTx;
          ev[1] = numMM;
//          MMPresente[numMM] = 0xff;
          codec_queue_event(ev);
        }
*/
        master_notx(numMM);
        break;
      case 27:
        if(mmaster[numMM].cmd_recv[1] == 1)
        {
          /* La versione 1.x corrisponde al modulo master doppio
             con backup. Quindi il primo comando 02h blocca il
             modulo master per circa 3 secondi, il secondo
             comando 02h che può arrivare subito per l'altro modulo
             master può andare in timeout.
             Il tempo di polling di linea nel caso peggiore
             corrisponde proprio al timeout.
             Idem per il comando 21h.
          */
          i = open("/proc/driver/mmaster/timeout/multiple", O_RDWR);
          write(i, "4000", 5);
          close(i);
        }
        
        /* Appende la versione del MM solo la prima volta. */
        if(!libuser_started)
        {
          fp = fopen(ADDROOTDIR(VersionFile), "a");
          fprintf(fp, "mm%d: firmware %d.%d.%d.%d\n", numMM, mmaster[numMM].cmd_recv[1], mmaster[numMM].cmd_recv[2], mmaster[numMM].cmd_recv[3], mmaster[numMM].cmd_recv[4]);
          fclose(fp);
          /* Gli attestaggi possono essere un pelo più lenti, quindi rinfresco il timeout anche qui. */
          timeout_on(master_timeout, NULL, NULL, 0, 80);
        }
        break;
      case 28:
      case 29:
        /* Gestione messaggi radio 868 */
        if(master_radio868_p) master_radio868_p(numMM, mmaster[numMM].cmd_recv);
        break;
    }
  }
  
  return NULL;
}

static void master_log(int mm, unsigned char *buf, int len)
{
  char m[96];
  int x;
  
  sprintf(m, "MM (%2d) -> ", mm);
  if(len>40) len = 40;
  for(x=0;x<len;x++)
    sprintf(m+11+(x<<1), "%02x", buf[x]);
  support_logmm(m);
}

static void* master_recv2(int mm_num)
{
  fd_set fdr;
  int i, res, len, maxfd;
  char log[32];
  struct timeval to;
  
//  pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
  
  debug_pid[DEBUG_PID_MRECV] = support_getpid();
  sprintf(log, "Master recv() [%d]", debug_pid[DEBUG_PID_MRECV]);
  support_log(log);
  
  while(1)
  {
    FD_ZERO(&fdr);
    maxfd = 0;
    for(i=0; i<mm_num; i++)
      if(mmaster[i].fd >= 0)
      {
        if(mmaster[i].fd > maxfd) maxfd = mmaster[i].fd;
        FD_SET(mmaster[i].fd, &fdr);
      }
    to.tv_sec = 0;
    to.tv_usec = 200000;
    pthread_testcancel();
    res = select(maxfd + 1, &fdr, NULL, NULL, &to);
    pthread_testcancel();
    if(res > 0)
    {
      for(i=0; i<mm_num; i++)
        if((mmaster[i].fd >= 0) && (FD_ISSET(mmaster[i].fd, &fdr)))
        {
          if(MMPresente[i] != MM_LARA)
          {
            if(!mmaster[i].cmd_recv_idx)
              len = 1;
            else
            {
              if(mmaster[i].cmd_recv[0] != 12)
                len = master_cmd_recv_dim[(int)mmaster[i].cmd_recv[0]] - mmaster[i].cmd_recv_idx;
              else
              {
                if(mmaster[i].cmd_recv_idx > 2)
                  len = mmaster[i].cmd_recv[2] + 4 - mmaster[i].cmd_recv_idx;
                else
                  len = 3 - mmaster[i].cmd_recv_idx;
              }
            }
          }
          else	/* LARA */
          {
            if(mmaster[i].cmd_recv_idx < 2)	// per ricevere i messaggi di backup
              len = 1; // 2 - mmaster[i].cmd_recv_idx;
            else if(mmaster[i].cmd_recv_idx < 5)	// per tutti gli altri messaggi
              len = 5 - mmaster[i].cmd_recv_idx;
            else
              len = mmaster[i].cmd_recv[4] + 5 - mmaster[i].cmd_recv_idx;
          }
          
          pthread_testcancel();
          res = read(mmaster[i].fd, &(mmaster[i].cmd_recv[(int)mmaster[i].cmd_recv_idx]), len);
          pthread_testcancel();
          
          if(res > 0)
            mmaster[i].cmd_recv_idx += res;
          
          if(MMPresente[i] != MM_LARA)
          {
            if(mmaster[i].cmd_recv[0] >= sizeof(master_cmd_recv_dim))
            {
              mmaster[i].cmd_recv_idx = 0;
              continue;
            }
            
            if((mmaster[i].cmd_recv[0] == 12) &&
               (mmaster[i].cmd_recv_idx > 2) && (mmaster[i].cmd_recv[2] == 0xff))
            {
              mmaster[i].cmd_recv_idx = 0;
              continue;
            }
            
            if(((mmaster[i].cmd_recv[0] != 12) &&
               (mmaster[i].cmd_recv_idx == master_cmd_recv_dim[(int)mmaster[i].cmd_recv[0]])) ||
              ((mmaster[i].cmd_recv[0] == 12) &&
               (mmaster[i].cmd_recv_idx > 2) && (mmaster[i].cmd_recv_idx == mmaster[i].cmd_recv[2] + 4)))
            {
              master_log(i, mmaster[i].cmd_recv, mmaster[i].cmd_recv_idx);
              mmaster[i].cmd_recv_idx = 0;
              master_parse(i);
            }
          }
          else
          {
            if(mmaster[i].cmd_recv_idx && (mmaster[i].cmd_recv[0] == 0x1a))
            {
              master_log(i, mmaster[i].cmd_recv, mmaster[i].cmd_recv_idx);
/*
              if(MMPresente[i] != 0xff)
              {
                unsigned char ev[2];
                ev[0] = No_ModuloMasterTx;
                ev[1] = i;
//                MMPresente[i] = 0xff;
                codec_queue_event(ev);
              }
*/
              master_notx(i);
              mmaster[i].cmd_recv_idx = 0;
            }
            else if((mmaster[i].cmd_recv_idx == 2) && (mmaster[i].cmd_recv[0] == CONFIG))
            {
              unsigned char ev[4];
              int n;
              
              /* Gestione ricezione "Rilevato MM" a sistema Lara già attivo. */
              master_log(i, mmaster[i].cmd_recv, mmaster[i].cmd_recv_idx);
              /* Per sicurezza sospendo il modulo, potrei dover gestire un reset
                 del modulo master senza un precedente NoTx. */
              if(mmaster[i].cmd_recv[1] == MM_LARA)
              {
                /* Deve ripresentarsi come Tebe, altrimenti scarto */
                ev[0] = Tipo_ModuloMaster;
                ev[2] = mmaster[i].cmd_recv[1];
                for(n=0; n<3; n++)
                {
                  MMSuspend[i+n] |= 1;
                  MMPresente[i+n] = mmaster[i].cmd_recv[1];
                  ev[1] = i+n;
                  codec_queue_event(ev);
                }
                
                lara_init(i);
              }
              
              mmaster[i].cmd_recv_idx = 0;
            }
            else if((mmaster[i].cmd_recv_idx == 5) && (mmaster[i].cmd_recv[0] == 0x1b))
            {
              /* Gestione ricezione versione a sistema Lara già attivo. */
              master_log(i, mmaster[i].cmd_recv, mmaster[i].cmd_recv_idx);
              mmaster[i].cmd_recv_idx = 0;
            }
            else if(((mmaster[i].cmd_recv_idx > 1) && (mmaster[i].cmd_recv[0] == TEBE_BACKUP)) ||
               ((mmaster[i].cmd_recv_idx > 4) && (mmaster[i].cmd_recv_idx == (mmaster[i].cmd_recv[4] + 5))))
            {
              master_log(i, mmaster[i].cmd_recv, mmaster[i].cmd_recv_idx);
              mmaster[i].cmd_recv_idx = 0;
              /* Proteggo la gestione del messaggio ricevuto rispetto al caricamento del lara.gz */
              pthread_mutex_lock(&laragz_mutex);
              lara_parse(i, mmaster[i].cmd_recv);
              pthread_mutex_unlock(&laragz_mutex);
            }
          }
        }
    }
  }
}

static void master_zones()
{
  int i, mask;
  unsigned char *zp, tmpzone[n_ZS+1];
  
/* 08/07/2010 */
//  database_lock();
  
  /* Update alarms in ZS */
  
  memset(tmpzone, 0, n_ZS+1);
  for(i=0; i<n_ZS+n_ZI+1; i++) ZONAEX[i] &= ~bitMUAlarmZone;
  
  for(i=0; i<n_SE; i++)
  {
    if(!(SE[i] & bitOOS) && Zona_SE[i] && (Zona_SE[i] <= (n_ZS+1)))
    {
#if 0
      if(!(i & 0x7)) /* first of 8 sensors */
        mask = bitFailure | bitSabotage | bitAlarm;
      else
        mask = bitSabotage | bitAlarm;
      
      tmpzone[Zona_SE[i]] |= (SE[i] & mask);
#else
      tmpzone[Zona_SE[i]] |= (SE[i] & (bitFailure | bitSabotage | bitAlarm));
      /* Considero la zona in allarme per "gest" in corso solo se l'impianto è in stile BdI
         e non sempre, altrimenti se il sensore è fisicamente a riposo viene generato un
         nuovo fronte di allarme che può innescare problemi se il programma utente la intercetta. */
      if((SE[i] & bitGestAlarm) && (SE[i] & bitMUNoAlarm)) tmpzone[Zona_SE[i]] |= bitAlarm;
      if((SE[i] & bitGestSabotage) && (SE[i] & bitMUNoSabotage)) tmpzone[Zona_SE[i]] |= bitSabotage;
      if((SE[i] & bitGestFailure) && (SE[i] & bitMUNoFailure)) tmpzone[Zona_SE[i]] |= bitFailure;
#endif
      if(SE[i] & bitMUAlarm) ZONAEX[Zona_SE[i]] |= bitMUAlarmZone;
    }
  }
  
  for(i=1; i<=n_ZS; i++)
  {
    if((ZS[i] & bitActive) && ((tmpzone[i] & bitAlarm) ^ (ZS[i] & bitAlarm)))
    {
      if(tmpzone[i] & bitAlarm)
        ZS[i] |= bitLHF;
      else
        ZS[i] |= bitHLF;
    }
    ZS[i] &= ~(bitStatusInactive | bitFailure | bitSabotage | bitAlarm);
    ZS[i] |= tmpzone[i];
    if(!(ZS[i] & bitActive))
    {
      ZS[i] &= ~bitAlarm;
      if(tmpzone[i] & bitAlarm) ZS[i] |= bitStatusInactive;
    }
  }
  
  /* Update alarms in ZI */

  for(i=0; i<n_ZI ; i++) ZI[i] &= ~(bitStatusInactive | bitFailure | bitSabotage);
  
  for(i=0; i<n_ZI; i++)
  {
    mask = 0;
    for(zp=TPDZI[i]; *zp!=0xff; zp++)
    {
      mask |= ((ZONA[*zp]) & (bitStatusInactive | bitFailure | bitSabotage | bitAlarm));
      ZONAEX[i+n_ZS+1] |= (ZONAEX[*zp] & bitMUAlarmZone);
    }
    if(ZI[i] & bitActive)
    {
      if((ZI[i] ^ mask) & bitAlarm)
      {
        if(mask & bitAlarm)
          ZI[i] |= bitLHF;
        else
          ZI[i] |= bitHLF;
      }
      ZI[i] &= ~bitAlarm;
      ZI[i] |= mask;
    }
    else
    {
      ZI[i] &= ~bitAlarm;
      ZI[i] |= (mask & ~bitAlarm);
      if(mask & bitAlarm) ZI[i] |= bitStatusInactive;
    }
  }
  
  /* Update alarms in ZT */
  
  ZT &= ~(bitStatusInactive | bitFailure | bitSabotage);
  
  mask = 0;
  for(zp=TPDZT; *zp!=0xff; zp++)
  {
    mask |= ((ZONA[*zp]) & (bitStatusInactive | bitFailure | bitSabotage | bitAlarm));
    ZONAEX[0] |= (ZONAEX[*zp] & bitMUAlarmZone);
  }
  if(ZT & bitActive)
  {
    if((ZT ^ mask) & bitAlarm)
    {
      if(mask & bitAlarm)
        ZT |= bitLHF;
      else
        ZT |= bitHLF;
    }
    ZT &= ~bitAlarm;
    ZT |= mask;
  }
  else
  {
    ZT &= ~bitAlarm;
    ZT |= (mask & ~bitAlarm);
    if(mask & bitAlarm) ZT |= bitStatusInactive;
  }
    
/* 08/07/2010 */
//  database_unlock();
}

static void master_actuators()
{
  unsigned char cmd[8], t0, t1, t2, lara_mask;
  int per, ind, i, req, t;

  cmd[0] = 8;

  for(per=0,ind=0; per<n_AT; ind++)
  {
    cmd[1] = ind & 0x1f;
    cmd[2] = 0;
    cmd[3] = 0;
    cmd[4] = 0;
    lara_mask = 0;
    
    if(AT[per] & bitNoise) cmd[2] |= 1;
    if(AT[per] & bitAbil) cmd[2] |= 4;
    if(SE[per] & bitTest) cmd[2] |= 2;
    
    t = per;
    for(i=0, req=0; i<8; i++,per++) req |= (AT[per] & bitAbilReq);
    
    if(req)
    {
      for(i=0, req=0; i<8; i++,t++)
      {
        if((AT[t] & bitON) && ((t == (n_AT-1)) || !(AT[t+1] & bitTemp))) cmd[3] |= (1<<i);      
        if(AT[t] & bitFlashing) cmd[4] |= (1<<i);
        if(AT[t] & bitAbilReq) lara_mask |= (1<<i);      
        
        AT[t] &= ~bitAbilReq;
      }
      
      if((MMPresente[ind>>5] == 0xff) || !master_periph_present(ind))
      {
        RICHIESTA_ERRATA_AT = ind;
      }
      else
      {
        if(MMPresente[ind>>5] == 0)	/* MM type 0 */
        {
          t0 = cmd[2];
          t1 = cmd[3];
          t2 = cmd[4];
          switch(master_periph_type(ind))
          {
            case SEN:
              // RIL_08
              cmd[2] = 0x80;
              for(i=1; i<9; i++)
              {
                cmd[3] = i;
                cmd[4] = (t1 & 0x01);
                if(t2 & 0x01) cmd[4] <<= 1;
                if(lara_mask & 0x01) master_queue(ind >> 5, cmd);
                t1 >>= 1;
                t2 >>= 1;
                lara_mask >>= 1;
              }
              break;
            case SC8:
            case SAN:
            case SC8C:
              if((DEFPT[ind][2] == 1) && !DEFPT[ind][3] && !DEFPT[ind][4])
              {
                if(t0 & 0x01) t1 |= 0x08;
                // RIL_0A
                if(TEST_ATTIVO & bitON)
                {
                  cmd[2] = ((t0 << 2) & 0x08) | 0x02;
                  master_queue(ind >> 5, cmd);
                }
                // RIL_0F
                cmd[2] = (((t0 << 2) | (t0 << 1)) & 0x08) | 0x04;
                if(!(t0 & 0x02))	// RIL_0C
                {
                  cmd[2] |= 0x80;
                  cmd[3] = (t1 >> 4) | (t1 << 4);
                  cmd[4] = 0;
                }
                master_queue(ind >> 5, cmd);
              }
              else
              {
                // RIL_0E
                cmd[2] = ((t0 << 3) & 0x08) | 0x81;
                // RIL_0C
                cmd[3] = (t1 >> 4) | (t1 << 4);
                cmd[4] = 0;
                master_queue(ind >> 5, cmd);
              }
              break;
            case ATt:
              // RIL_0E
              cmd[2] = ((t0 << 3) & 0x08) | 0x81;
              // RIL_0C
              cmd[3] = (t1 >> 4) | (t1 << 4);
              cmd[4] = 0;
              master_queue(ind >> 5, cmd);
              break;
            case SS:
              // RIL_0E
              cmd[2] = ((t0 << 3) & 0x08) | 0x01;
              cmd[3] = 0;
              cmd[4] = 0;
              master_queue(ind >> 5, cmd);
              // RIL_0A
              if(TEST_ATTIVO & bitON)
              {
                cmd[2] = ((t0 << 2) & 0x08) | 0x02;
                master_queue(ind >> 5, cmd);
              }
              // RIL_0F
              cmd[2] = (((t0 << 2) | (t0 << 1)) & 0x08) | 0x04;
              master_queue(ind >> 5, cmd);
              break;
            case SC1:
              // RIL_0E
              cmd[2] = ((t0 << 3) & 0x08) | 0x01;
              cmd[3] = 0;
              cmd[4] = 0;
              master_queue(ind >> 5, cmd);
              // RIL_0A
              if(TEST_ATTIVO & bitON)
              {
                cmd[2] = ((t0 << 2) & 0x08) | 0x02;
                master_queue(ind >> 5, cmd);
              }
              // RIL_0F
              cmd[2] = (((t0 << 2) | (t0 << 1)) & 0x08) | 0x04;
              if(!(t0 & 0x02))	// RIL_0C
              {
                cmd[2] |= 0x80;
                cmd[3] = (t1 >> 4) | (t1 << 4);
                cmd[4] = 0;
              }
              master_queue(ind >> 5, cmd);
              break;
            case TAS:
              // RIL_0E
              cmd[2] = ((t0 << 3) & 0x08) | 0x01;
              cmd[3] = 0;
              cmd[4] = 0;
              master_queue(ind >> 5, cmd);
              // RIL_0F
              cmd[2] = (((t0 << 2) | (t0 << 1)) & 0x08) | 0x04;
              master_queue(ind >> 5, cmd);
              break;
            case TS2:
              // RIL_08
              cmd[2] = 0x80;
              for(i=1; i<3; i++)
              {
                cmd[3] = i;
                cmd[4] = (t1 & 0x01);
                if(t2 & 0x01) cmd[4] <<= 1;
                if(lara_mask & 0x01) master_queue(ind >> 5, cmd);
                t1 >>= 1;
                t2 >>= 1;
                lara_mask >>= 1;
              }
              break;
            default:
              // RIL_0E
              cmd[2] = ((t0 << 3) & 0x08) | 0x01;
              cmd[3] = 0;
              cmd[4] = 0;
              master_queue(ind >> 5, cmd);
              // RIL_08
              cmd[2] = 0x80;
              for(i=1; i<9; i++)
              {
                cmd[3] = i;
                cmd[4] = (t1 & 0x01);
                if(t2 & 0x01) cmd[4] <<= 1;
                if(lara_mask & 0x01) master_queue(ind >> 5, cmd);
                t1 >>= 1;
                t2 >>= 1;
                lara_mask >>= 1;
              }
              break;
          }
        }
        else if(MMPresente[ind>>5] == MM_LARA)
        {
          lara_actuators(ind, cmd[3], lara_mask);
        }
        else if(MMPresente[ind>>5] != 0xff)
        {
          master_queue(ind >> 5, cmd);
        }
      }
    }
  }
}

static void* master_loop()
{
  FILE *fp;
  int i, j;
  void *user_handle;
  void (*user_init)(void) = NULL;
  void (*user_program)(void) = NULL;
  char log[32];
  int cpu_load, cpu_total;
  int cpu_load_old, cpu_total_old;
  int cpu_cont, cpu, nice, system, idle;
  
//  pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
  
  debug_pid[DEBUG_PID_MLOOP] = support_getpid();
  sprintf(log, "Master loop() [%d]", debug_pid[DEBUG_PID_MLOOP]);
  support_log(log);
#if 0
#warning !!!!!!!!!! DEBUG attivo !!!!!!!!!
fp = fopen("/tmp/master.log", "a");
fprintf(fp, "%s\n", log);
fclose(fp);
#endif
  
#if 0
  if(!unlink(Boot2bFileName))
  {
    /* reset user program but leave the file for exec permissions */
    fp = fopen(UserFileName, "w");
    fclose(fp);
    Boot2Error[0] = Evento_Esteso;
    Boot2Error[1] = Ex_Stringa;
    codec_queue_event(Boot2Error);
  }
  else if(!unlink(Boot2aFileName))
  {
    // give a second chance
    fp = fopen(Boot2bFileName, "w");
    fclose(fp);
  }
  else
  {
    fp = fopen(Boot2aFileName, "w");
    fclose(fp);
  }
#endif
  
  support_store_current_values();
  nvmem_load();

  user_handle = dlopen(ADDROOTDIR(UserFileName), RTLD_NOW);
  if(user_handle)
  {
    support_log("Caricato programma utente");
    user_init = dlsym(user_handle, "user_init");
    user_program = dlsym(user_handle, "user_program");
  }
  else
  {
    support_log(dlerror());
  }
  
  if(user_init && Restart)
  {
    database_lock();
    (*user_init)();
    database_unlock();
  }
  
  cmd_memory_control_init();
  /* Fondamentalmente devo aspettare di rilevare tutti i moduli master e tutte
     le periferiche manomesse. Se ho pochi moduli e tutte le periferiche presenti,
     questo tempo è fin eccessivo. Al contrario se ho tanti moduli e tante
     periferiche manomesse, questo tempo potrebbe essere insufficiente ed il
     programma utente con le sue attuazioni potrebbe partire troppo presto. */
  //sleep(9);
  /* Rendo questa attesa più dinamica. */
  while(master_timeout->active) usleep(100000);
  
  for(i=0; i<MM_NUM; i++)
    if(MMPresente[i] != 0xff)
    {
      if(MMPresente[i] == 0)
        for(j=i<<5; j<((i<<5)+32); j++) master_send_sc8c_config(j);
      else if(MMPresente[i] != MM_LARA)
        for(j=i<<5; j<((i<<5)+32); j++) master_send_analog_threshold(j);
    }
  cmd_actuator_refresh();
  master_actuators_refresh();
  
  support_log("Avvio centrale");
  
  cpu_load_old = 0;
  cpu_total_old = 0;
  cpu_cont = 50;
  
  while(1)
  {
    debug_mloop_phase = 1;
    sem_wait(&master_loop_sem);
    debug_mloop_phase = 2;
#if 0
    master_zones();
    debug_mloop_phase = 3;
    database_lock();
#else
    /* 08/07/10 Inserisco anche master_zones() all'interno del lock
       altrimenti può capitare che a fronte di un allarme si generino
       due fronti di salita sulla zona.
       
       Se l'allarme (protetto dal lock) arrivava dopo master_zones(),
       si otteneva per il programma utente il fronte di allarme e il
       solo fronte della zona, senza che la zona risultasse ancora in
       allarme. Vedi master_set_alarm.
       
       Al giro successivo master_zones() metteva in allarme la zona
       rigenerando il fronte di salita sulla zona ma senza il relativo
       fronte sul sensore. */
    
    database_lock();
    debug_mloop_phase = 3;
    master_zones();
#endif
    debug_mloop_phase = 4;
    if(user_program)
    {
      (*user_program)();
      gsm_user_program_end_loop();
    }
    debug_mloop_phase = 5;
    CHIAVE_TRASMESSA = 0;
    database_unlock();
    debug_mloop_phase = 6;
#if 0
#warning -------------------------------------------
#warning Verifica della reale variazione dello stato
#warning attuatori prima di generare il comando di
#warning attuazione. Confronto con i valori presenti
#warning in support.
#warning -------------------------------------------
#endif
    master_actuators();
    debug_mloop_phase = 7;
    
    if(!libuser_started)
    {
      libuser_started = 1;
      unlink(ADDROOTDIR(Boot2aFileName));
      unlink(ADDROOTDIR(Boot2bFileName));
    }
    debug_mloop_phase = 8;
    nvmem_save();
    debug_mloop_phase = 9;
    support_save_status();
    debug_mloop_phase = 10;
    if(database_changed) database_save();
    debug_mloop_phase = 11;
    
    if(cpu_cont)
    {
      cpu_cont--;
      if(cpu_cont == 10)
      {
        fp = fopen ("/proc/stat", "r");
        fscanf (fp, "%*s %d %d %d %d", &cpu, &nice, &system, &idle);
        fclose (fp);
        cpu_load_old = cpu + nice + system;
        cpu_total_old = cpu + nice + system + idle;
      }
      else if(!cpu_cont)
      {
        fp = fopen ("/proc/stat", "r");
        fscanf (fp, "%*s %d %d %d %d", &cpu, &nice, &system, &idle);
        fclose (fp);
        cpu_load = cpu + nice + system;
        cpu_total = cpu + nice + system + idle;
        i = (100 * (double) (cpu_load - cpu_load_old)) / (double) (cpu_total - cpu_total_old);
        log[0] = Evento_Esteso;
        log[1] = Ex_Stringa;
        log[2] = 0;
        sprintf(log+4, str_d_0, i);
        log[3] = strlen(log+4);
        codec_queue_event(log);
      }
    }
    
    debug_mloop_phase = 12;
  }
}

int master_init(void)
{
  int i;
#if defined __i386__ || defined __x86_64__
  char dev[256];
#else
  char dev[16];
#endif
  
  /* Init */
  memset(mmaster, 0, sizeof(mmaster));
  sem_init(&master_loop_sem, 0, 0);
  sem_init(&master_send_queue_msg, 0, 0);
  master_timeout = timeout_init();
  
  /* La funzione è definita in radio.so, se il plugin è presente
     vengono attivate le gestioni per la radio 868. */
  master_radio868_p = dlsym(NULL, "radio868_mm");
  master_radio868_accept_fronts_p = dlsym(NULL, "radio868_accept_fronts");
  
  /* Creazione UniqueID per radio 868 */
  if(!master_SC8R2_uniqueid)
  {
    struct ifreq ifr;
    
    i = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    
    memset(&ifr, 0, sizeof(ifr));
    strcpy(ifr.ifr_name, "eth0");
    ioctl(i, SIOCGIFHWADDR, &ifr);
    close(i);
    memcpy(&master_SC8R2_uniqueid, ifr.ifr_hwaddr.sa_data+2, 4);
    database_changed = 1;
  }
  
  /* Inizio con 2 secondi. */
  timeout_on(master_timeout, NULL, NULL, 0, 20);
  
  if(config.NodeID == 99)
  {
    mmaster[0].fd = open(ADDROOTDIR("/dev/pxl"), O_RDWR);
    i = 1;

    master_reset(0);
    
    pxl_start();
  }
  else
  {
    for(i=0; i<MM_NUM; i++)
    {
#if 0
/* Per forzare un modulo master in una posizione diversa da quella fisica */
if(i==1)
sprintf(dev, "/dev/mmaster%d", 7);
else if(i==7)
sprintf(dev, "/dev/mmaster%d", 1);
else
#endif
      sprintf(dev, ADDROOTDIR("/dev/mmaster%d"), i);
      mmaster[i].fd = open(dev, O_RDWR);
    }
    
    /* Attenzione, a volte il plugin RID viene caricato dopo questo punto,
       perdendo quindi il punto di sincronizzazione. Spostata la creazione
       del processo cluster dopo l'impostazione di queste variabili e
       l'incidenza è crollata (nel senso che non l'ho più visto accadere),
       ma per sicurezza aggiungo un piccolo ritardo. */
    usleep(100000);
    //if(master_behaviour != MASTER_NORMAL)
    if(master_ridon_sync)
      master_ridon_sync();
    
    master_reset(0);
  }
  
  support_load_status();
  
  pthread_create(&master_thd_s, NULL, (PthreadFunction)master_send, NULL);
  pthread_detach(master_thd_s);

  pthread_create(&master_thd, NULL, (PthreadFunction)master_recv2, (void*)i);
  pthread_detach(master_thd);
  
  pthread_create(&master_loop_thd, NULL, (PthreadFunction)master_loop, NULL);
  pthread_detach(master_loop_thd);
  
  return 0;
}

int master_stop(void)
{
  pxl_stop();
  
  pthread_cancel(master_thd_s);
  pthread_cancel(master_loop_thd);

// Si blocca perché quando viene chiamato master_stop
// ad uno switch (ridondanza), questo processo è già
// terminato e quindi non esiste più.
//  pthread_cancel(master_thd);

  sem_destroy(&master_send_queue_msg);
  sem_destroy(&master_loop_sem);
  
  return 0;
}

void master_do()
{
  static int master_loop_first_time = 0;
  int i;
  
  static int master_blocco = 0;
  char m[64];
  
  /* Finita l'inizializzazione dei moduli master, attendo un paio
     di secondi prima di riattivare l'invio dei comandi accodati,
     per dare tempo alle periferiche manomesse di andare fuori
     linea. */
  for(i=0; i<MM_NUM; i++)
  {
    if(MMSuspendDelay[i])
    {
      MMSuspendDelay[i]--;
      if(!MMSuspendDelay[i]) MMSuspend[i] = 0;
    }
  }
  
  if(!libuser_started && master_loop_first_time) return;
  master_loop_first_time = 1;
  sem_post(&master_loop_sem);
  
  /* Se il loop di gestione del programma utente si bloccasse
     per qualche motivo, il valore del semaforo crescerebbe e
     superata la soglia eseguo un reset della centrale. */
  sem_getvalue(&master_loop_sem, &i);
  if((i > 20) && !master_blocco)
  {
    master_blocco = 1;
    sprintf(m, "Master loop bloccato (%d)", debug_mloop_phase);
    support_log(m);
    support_log2(m);
    support_logmm(m);
//    exit(0);
  }
  else if(master_blocco && (i <= 1))
  {
    master_blocco = 0;
    //sprintf(m, "Master loop ripristino");
    support_log("Master loop ripristino");
    support_log2("Master loop ripristino");
    support_logmm("Master loop ripristino");
  }
}

int master_periph_present(int ind)
{
  if((MMPresente[ind>>5] != 0xff) && (TipoPeriferica[ind] < 0xf))
    return(StatoPeriferica[ind>>3] & (1<<(ind & 0x07)));
  else
    return 0;
}

int master_periph_type(int ind)
{
  return TipoPeriferica[ind];
}

int master_sensors_refresh()
{
  int i;
  unsigned char cmd[2];
  
  cmd[0] = 7;
  
  for(i=0; i<(MM_NUM*32); i++)
    if((MMPresente[i>>5] != MM_LARA) && master_periph_present(i) &&
       (/*(config.Variant != 1) ||*/ (TipoPeriferica[i] != SEN)))
    {
      cmd[1] = i & 0x1f;
      master_queue(i >> 5, cmd);
    }

  return 0;
}

int master_actuators_refresh()
{
  int i, j, at;
  
  for(at=0,i=0; i<(MM_NUM*32); i++)
    if(master_periph_present(i))
      for(j=0; j<8; j++)
        AT[at++] |= bitAbilReq;
    else
      at += 8;
  
  return 0;
}

int master_set_setting(int ind)
{
  int i;
  unsigned char d;

  d = ((DEFPT[ind][0] & 0x07) << 4) | (DEFPT[ind][1] & 0x03);
  if((TipoPeriferica[ind] == SC8) && (AT[ind<<3] & bitNoise)) d |= 0x08;
  AT[ind<<3] |= bitAbilReq;
  
  for(i=0; i<7; i++)
  {
    if(d & 0x01)
      AT[(ind<<3)+i] |= bitON;
    else
      AT[(ind<<3)+i] &= ~bitON;
    d >>= 1;
  }

  return 0;
}

int master_settings_refresh()
{
  int ind;
  
  for(ind=0; ind<n_DEFPT; ind++)
    if((DEFPT[ind][2] == 1) && !DEFPT[ind][3] && !DEFPT[ind][4])
      master_set_setting(ind);
      
  return 0;
}

int master_sensor_valid(int idx)
{
  if(MMPresente[idx>>8] == 0)
  {
    switch(TipoPeriferica[idx>>3])
    {
      case SS: case SC1: case TAS: case SEN:
        return 1;
        break;
      case SC8: case SAN: case TS2: case SC8C:
        return 8;
        break;
      default:
        return 0;
        break;
    }
  }
  else if(MMPresente[idx>>8] == MM_LARA)
  {
    if(TipoPeriferica[idx>>3] < 0x0f)
      return 8;
    else
      return 0;
  }
  else if(MMPresente[idx>>8] == 0xff)
  {
    return 0;
  }
  else if(MMPresente[idx>>8] == 4)
  {
    switch(TipoPeriferica[idx>>3])
    {
      case SS: case SC1: case SEN: case 3:
        return 1;
        break;
      case SC8:
        return 8;
        break;
      default:
        return 0;
        break;
    }
  }
  else
  {
    switch(TipoPeriferica[idx>>3])
    {
      case SSA: case SST: case TST: case TSK: case TLK: case TLB:
        return 1;
        break;
      case SA8: case SAN:
        return 8;
        break;
      default:
        return 0;
        break;
    }
  }
  
  return 0;
}

int master_actuator_valid(int idx)
{
  if(MMPresente[idx>>8] == 0)
  {
    switch(TipoPeriferica[idx>>3])
    {
      case SS: case SC1: case TS2:
        return 1;
        break;
      case SC8: case SAN: case ATt: case QR: case SEN: case SC8C:
        return 8;
        break;
//     case TAS:
     default:
        return 0;
        break;
    }
  }
  else if(MMPresente[idx>>8] == MM_LARA)
  {
    switch(TipoPeriferica[idx>>3])
    {
      case SC8:
        return 8;
        break;
      case TLB:
        return 3;
        break;
      default:
        return 0;
        break;
    }
  }
  else if(MMPresente[idx>>8] == 0xff)
  {
    return 0;
  }
  else if(MMPresente[idx>>8] == 4)
  {
    switch(TipoPeriferica[idx>>3])
    {
      case SS: case SC1: case 3:
        return 1;
        break;
      case SC8: case SEN:
        return 8;
        break;
      default:
        return 0;
        break;
    }
  }
  else
  {
    switch(TipoPeriferica[idx>>3])
    {
      case SSA: case SST:
        return 1;
        break;
      case SA8: case ATA: case TST: case TSK: case TLK: case TLB: case SAN:
        return 8;
        break;
      default:
        return 0;
        break;
    }
  }
  
  return 0;
}

void master_set_active()
{
  unsigned char cmd = 33;
  unsigned char cmd2[] = {TEBE_SET_MASTER, 0, 0, 0, 0};
  int mm;
  
  if(master_behaviour == SLAVE_STANDBY)
  {
    support_log("--- SWITCH ---");
    master_behaviour = SLAVE_ACTIVE;
    for(mm=0; mm<MM_NUM; mm++)
    {
      if(MMPresente[mm] != 0xff)
      {
        if(MMPresente[mm] != MM_LARA)
          master_queue(mm, &cmd);
        else
        {
          master_queue(mm, cmd2);
          mm += 2;
        }
      }
    }
    codec_consume_event_needed();
  }
}

void master_set_texas(int mm)
{
  MMTexas[mm] = 1;
  database_changed = 1;
}

void master_register_module(int module, int fd)
{
  int tfd;
  
  tfd = mmaster[module].fd;
  mmaster[module].fd = -1;
  close(tfd);
  if(module < MM_NUM) mmaster[module].fd = fd;
}

void master_SC8R2_init(int linea)
{
  unsigned char cmd[2];
  
  /* Simulo un ripristino periferica per provocare le azioni necessarie
     sul singolo concentratore 868. */
  cmd[0] = 5;
  cmd[1] = linea & 0x1f;
  if(master_radio868_p) master_radio868_p(linea>>5, cmd);
}

void master_radio868_accept_fronts(int conc)
{
  if(master_radio868_accept_fronts_p) master_radio868_accept_fronts_p(conc);
}

