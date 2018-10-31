/*
   Il messaggio LARA_NODES invia sempre il nodo come presente, va gestito in base al campo.
   La timbratura di un badge deve produrre sempre il messaggio LARA_ID - ACC_BADGE.
   La perdita o il recupero di comunicazione di una serratura deve produrre un messaggio LARA_NODES.
   
   Funzionalmente, i terminali vanno programmati tutti con accesso solo badge.
   Lo sblocco della serratura va gestito come per il modulo varco, con comando esplicito
   di attuazione.
*/

#include "protocol.h"
#include "database.h"
#include "support.h"
#include "user.h"
#include "master.h"
#include "lara.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <errno.h>
#include <byteswap.h>
#include <termios.h>

//#define DEBUG

#define PACKED

#define LARA_DIMBUF 64

typedef struct {
	lara_tData	Code;
	lara_tAddress	Address;
	lara_tData	Length;
	lara_t_id	Id;
} lara_tMsg_ID PACKED;

#define SI_MSG_RESPONSE		0
#define SI_MSG_GET_STATUS	1
#define SI_MSG_ADD_TO_WHITELIST	3
#define SI_MSG_REMOVE_FROM_WHITELIST 4
#define SI_MSG_ACCESS_DENIED	8
#define SI_MSG_SHORT_ACTIVATION	9
#define SI_MSG_LONG_ACTIVATION	10
#define SI_MSG_LONG_RELEASE	11
#define SI_MSG_READER_EVENT	14
#define SI_MSG_READER_INFO_EVENT	15
#define SI_MSG_SYSTEM_INCONSISTENCY_EVENT	20

#define SI_FLAGS_DOWN 0x01
#define SI_FLAGS_PRE_DOWN 0x02
#define SI_FLAGS_PROGRAM 0x04
#define SI_FLAGS_STATUS 0x08
#define SI_FLAGS_RUN 0x10
#define SI_FLAGS_WL 0x20
#define SI_FLAGS_INIT 0x40

#define SMARTINTEGO_GW_ER 0
#define SMARTINTEGO_GW_CR 1

#define SI_REFID 0x0a

struct smartintego_term;

struct smartintego_gateway {
  int type;
  //int wn_addr;
  int dev_addr;
  char *connection;
  union {
    struct {
      int fd;
      struct sockaddr_in sa;
      int connected, wait;
    } er;
    struct {
      int bus;
      int addr;
      unsigned char *cmd;
      int retry;
    } cr;
  };
  int ridx;
  unsigned char rbuf[64];
  unsigned int outseq, inseq;
  int flags;
  int timeout;
  int watchdog;
  
  struct {
    int curid;
    unsigned char id[32]; // flags per 250 tessere di whitelist
    int badgelen;
    unsigned char badge[20];
    int nodo;
  } wl;
  
  struct smartintego_gateway *next;
};

struct smartintego_term {
  //int wn_addr;
  int dev_addr;
  int pending:16;
  int enter:16;
  int flags;
  lara_tMsg_ID reply;
  int ingressi;
  int doorsens;
  int doortimeout;
  struct smartintego_gateway *gw;
};

struct smartintego_global {
  int init;
  pthread_mutex_t mutex;
  pthread_mutex_t wlmutex;
  int mm, fdmm, mmi;
  unsigned char mmbuf[LARA_DIMBUF];
  struct smartintego_gateway *gwhead;
  /* Terminale 0 non gestito */
  struct smartintego_term term[LARA_N_TERMINALI+1];
} SmartIntego = {0, PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER, 0, };

#if 0
struct {
  int current_id;
  unsigned char term[LARA_N_TERMINALI+1];
  int badgelen;
  unsigned char badge[20];
} whitelist = {0, };
#endif

static unsigned short crc_tabccitt[256] = {
0x0000,0x1021,0x2042,0x3063,0x4084,0x50a5,0x60c6,0x70e7,
0x8108,0x9129,0xa14a,0xb16b,0xc18c,0xd1ad,0xe1ce,0xf1ef,
0x1231,0x0210,0x3273,0x2252,0x52b5,0x4294,0x72f7,0x62d6,
0x9339,0x8318,0xb37b,0xa35a,0xd3bd,0xc39c,0xf3ff,0xe3de,
0x2462,0x3443,0x0420,0x1401,0x64e6,0x74c7,0x44a4,0x5485,
0xa56a,0xb54b,0x8528,0x9509,0xe5ee,0xf5cf,0xc5ac,0xd58d,
0x3653,0x2672,0x1611,0x0630,0x76d7,0x66f6,0x5695,0x46b4,
0xb75b,0xa77a,0x9719,0x8738,0xf7df,0xe7fe,0xd79d,0xc7bc,
0x48c4,0x58e5,0x6886,0x78a7,0x0840,0x1861,0x2802,0x3823,
0xc9cc,0xd9ed,0xe98e,0xf9af,0x8948,0x9969,0xa90a,0xb92b,
0x5af5,0x4ad4,0x7ab7,0x6a96,0x1a71,0x0a50,0x3a33,0x2a12,
0xdbfd,0xcbdc,0xfbbf,0xeb9e,0x9b79,0x8b58,0xbb3b,0xab1a,
0x6ca6,0x7c87,0x4ce4,0x5cc5,0x2c22,0x3c03,0x0c60,0x1c41,
0xedae,0xfd8f,0xcdec,0xddcd,0xad2a,0xbd0b,0x8d68,0x9d49,
0x7e97,0x6eb6,0x5ed5,0x4ef4,0x3e13,0x2e32,0x1e51,0x0e70,
0xff9f,0xefbe,0xdfdd,0xcffc,0xbf1b,0xaf3a,0x9f59,0x8f78,
0x9188,0x81a9,0xb1ca,0xa1eb,0xd10c,0xc12d,0xf14e,0xe16f,
0x1080,0x00a1,0x30c2,0x20e3,0x5004,0x4025,0x7046,0x6067,
0x83b9,0x9398,0xa3fb,0xb3da,0xc33d,0xd31c,0xe37f,0xf35e,
0x02b1,0x1290,0x22f3,0x32d2,0x4235,0x5214,0x6277,0x7256,
0xb5ea,0xa5cb,0x95a8,0x8589,0xf56e,0xe54f,0xd52c,0xc50d,
0x34e2,0x24c3,0x14a0,0x0481,0x7466,0x6447,0x5424,0x4405,
0xa7db,0xb7fa,0x8799,0x97b8,0xe75f,0xf77e,0xc71d,0xd73c,
0x26d3,0x36f2,0x0691,0x16b0,0x6657,0x7676,0x4615,0x5634,
0xd94c,0xc96d,0xf90e,0xe92f,0x99c8,0x89e9,0xb98a,0xa9ab,
0x5844,0x4865,0x7806,0x6827,0x18c0,0x08e1,0x3882,0x28a3,
0xcb7d,0xdb5c,0xeb3f,0xfb1e,0x8bf9,0x9bd8,0xabbb,0xbb9a,
0x4a75,0x5a54,0x6a37,0x7a16,0x0af1,0x1ad0,0x2ab3,0x3a92,
0xfd2e,0xed0f,0xdd6c,0xcd4d,0xbdaa,0xad8b,0x9de8,0x8dc9,
0x7c26,0x6c07,0x5c64,0x4c45,0x3ca2,0x2c83,0x1ce0,0x0cc1,
0xef1f,0xff3e,0xcf5d,0xdf7c,0xaf9b,0xbfba,0x8fd9,0x9ff8,
0x6e17,0x7e36,0x4e55,0x5e74,0x2e93,0x3eb2,0x0ed1,0x1ef0
};

/* http://www.lammertbies.nl/comm/info/crc-calculation.html */
/*
  printf("%04x\n", crc_ccitt("123456789", 9));
  -> 31c3
*/

unsigned short crc_ccitt(unsigned char *msg, int len)
{
  unsigned short crc, tmp;
  
  crc = 0x0000;
  while(len--)
  {
    tmp = (crc >> 8) ^ *(msg++);
    crc = (crc << 8) ^ crc_tabccitt[tmp];
  }
  
  return crc;
}

static int smartintego_send(struct smartintego_gateway *gw, unsigned int dest, unsigned char *msg, int len, int refid)
{
  unsigned char llmsg[256];
  unsigned short crc;
  char log[128];
  int i, l;
  
  /* Incapsula il messaggio applicativo nel messaggio link layer */
  /* LLD */
  llmsg[0] = len+18;
  llmsg[1] = 0xFE;
  llmsg[2] = 0xFD;
  /* MLD */
  llmsg[3] = len+13;
  llmsg[4] = 1;
  llmsg[5] = 0; // no encryption (limite SmartIntego)
  llmsg[6] = 0;
  llmsg[7] = refid; // RefID (0 = no ack)
  gw->outseq++;
  llmsg[8] = gw->outseq >> 24;
  llmsg[9] = gw->outseq >> 16;
  llmsg[10] = gw->outseq >> 8;
  llmsg[11] = gw->outseq;
  if(gw->outseq == 0xffffffff) gw->outseq = 0;
  /* ALD */
  llmsg[12] = dest >> 24;
  llmsg[13] = dest >> 16;
  llmsg[14] = dest >> 8;
  llmsg[15] = dest;
  memcpy(llmsg+16, msg, len);
  
  crc = crc_ccitt(llmsg, len+16);
  llmsg[len+16] = crc>>8;
  llmsg[len+17] = crc;
  
#ifdef DEBUG
{
  int x;
  printf("(%s) Tx: ", gw->connection);
  for(x=0; x<llmsg[0]; x++) printf("%02x", llmsg[x]);
  printf("\n");
}
#endif
  
  /* Watchdog 2 minuti */
  gw->watchdog = 120;
  
  if((gw->type == SMARTINTEGO_GW_ER) && gw->er.connected)
  {
    sprintf(log, "SI (%s) Tx: ", gw->connection);
    l = strlen(log);
    for(i=0; i<llmsg[0]; i++) sprintf(log+l+i*2, "%02x", llmsg[i]);
    support_logmm(log);
    
    write(gw->er.fd, llmsg, llmsg[0]);
    
//printf("%d keepalive\n", time(NULL));
    /* Esegue un GetStatus ogni 20 secondi per tenere attiva
       la connessione. Altrimenti dopo 30s di inattività la
       connessione si chiude. */
    gw->er.wait = 20;
  }
  else if(gw->type == SMARTINTEGO_GW_CR)
  {
    /* Prenoto la trasmissione via 485. */
    if(gw->cr.cmd)
    {
      sprintf(log, "SI (%s) ** comando sovrascritto **", gw->connection);
      support_logmm(log);
    }
    free(gw->cr.cmd);	// non si sa mai
    gw->cr.cmd = malloc(llmsg[0]);
    if(gw->cr.cmd) memcpy(gw->cr.cmd, llmsg, llmsg[0]);
    gw->cr.retry = 0;
  }
  
  if(gw->flags & SI_FLAGS_RUN) return 1;
  return 0;
}

static void smartintego_send_nodes()
{
  lara_tMsg reply;
  int i;
  
  reply.Code = LARA_NODES;
  reply.Address.domain = 0;
  reply.Address.subnet = 2;
  reply.Address.node = 126;
  reply.Length = 32;
  memset(reply.Msg, 0, 32);
  for(i=1; i<=LARA_N_TERMINALI; i++)
    if(SmartIntego.term[i].dev_addr)
    {
      reply.Msg[i/8] |= (1<<(i&7)); // programmata
      if(!(SmartIntego.term[i].flags & SI_FLAGS_DOWN))
        reply.Msg[16+(i/8)] |= (1<<(i&7));  // presente
    }
  pthread_mutex_lock(&SmartIntego.mutex);
  write(SmartIntego.fdmm, &reply, reply.Length+5);
  pthread_mutex_unlock(&SmartIntego.mutex);
}

static void smartintego_master_parse()
{
  lara_tMsg *msg = (void*)(SmartIntego.mmbuf);
  unsigned char cmd[32];
  int i;
  
  while((SmartIntego.mmi > 4) && (SmartIntego.mmi >= (msg->Length+5)))
  {
    switch(msg->Code)
    {
      case LARA_ATTUA:
        /* Comando uscite */
        // msg->Address.node è la serratura a cui inviare il comando
        /* Il comando di apertura porta è comunque sempre sempre il primo:
           msg->Msg[3] può valere NOOP, TIMING, ON_T e OFF_T. */
        /* Comando Short Term Activation (3.3.10) o Long Term Activation/Release */
        /* A fronte del comando di apertura devo inviare l'evento di transito. */
        /* I comandi effettivi iniziano con offset 3 */
        cmd[0] = 0;
        if((msg->Msg[3] == TIMING) || (msg->Msg[3] == ON_T))
        {
          if(SmartIntego.term[msg->Address.node].pending)
          {
            cmd[0] = 0x80;  // Card related message
            if(SmartIntego.term[msg->Address.node].doorsens < 0)
            {
              /* E' una conferma alla timbratura, oltre ad inviare il comando
                 di apertura alla serratura devo inviare anche il transito ok.
                 Non c'è controllo di porta aperta. */
              pthread_mutex_lock(&SmartIntego.mutex);
              SmartIntego.term[msg->Address.node].pending = 0;
              write(SmartIntego.fdmm, &SmartIntego.term[msg->Address.node].reply,
                SmartIntego.term[msg->Address.node].reply.Length+5);
              pthread_mutex_unlock(&SmartIntego.mutex);
            }
            else
            {
              /* E' attivo il controllo porta, rinfresco il timeout in attesa
                 del sensore. */
              pthread_mutex_lock(&SmartIntego.mutex);
              SmartIntego.term[msg->Address.node].pending = 0;
              if(msg->Msg[3] == TIMING)
              {
                if(_lara->terminale[msg->Address.node-1].timeopen)
                  SmartIntego.term[msg->Address.node].enter =
                    _lara->terminale[msg->Address.node-1].timeopen + 1;
                else
                  SmartIntego.term[msg->Address.node].enter = 30;
              }
              else
                SmartIntego.term[msg->Address.node].enter = -1;
              pthread_mutex_unlock(&SmartIntego.mutex);
            }
          }
          else
          {
            /* Apertura forzata dalla centrale, il transito viene gestito
               attraverso il controllo della porta, se c'è.
               Altrimenti per continuità con il Modulo Varco non viene segnalato
               alcun transito non potendo sapere se la porta rimane aperta o
               viene movimentata. */
          }
        }
        else if((msg->Msg[3] == OFF_T) && (SmartIntego.term[msg->Address.node].enter < 0))
        {
          pthread_mutex_lock(&SmartIntego.mutex);
          SmartIntego.term[msg->Address.node].enter = 0;
          pthread_mutex_unlock(&SmartIntego.mutex);
        }
        
        switch(msg->Msg[3])
        {
          case TIMING:
            cmd[0] |= SI_MSG_SHORT_ACTIVATION;
            if(_lara->terminale[msg->Address.node-1].timeopen <= 25)
              cmd[1] = _lara->terminale[msg->Address.node-1].timeopen * 10;
            else
              cmd[1] = 250; // massimo tempo consentito
            i = 2;
            break;
          case ON_T:
            cmd[0] |= SI_MSG_LONG_ACTIVATION;
            cmd[1] = cmd[2] = cmd[3] = cmd[4] = 0;
            i = 5;
            break;
          case OFF_T:
            cmd[0] |= SI_MSG_LONG_RELEASE;
            cmd[1] = cmd[2] = 0;
            i = 3;
            break;
          default:
            i = 0;
            break;
        }
        /* Uso l'attuatore +3 per forzare l'invio di un messaggio di Access Denied
           per non dover far aspettare il timeout alla serratura se la tessera è
           autorizzata ma il programma utente decide che non deve aprire lo stesso. */
        if(msg->Msg[6] == ON_T)
        {
          cmd[0] = 0x80|SI_MSG_ACCESS_DENIED;
          i = 1;
        }
        if(i) smartintego_send(SmartIntego.term[msg->Address.node].gw,
                SmartIntego.term[msg->Address.node].dev_addr, cmd, i, 0);
        
        /* Gestione programmazione tessere, imposto la serratura come punto di
           programmazione. Le letture eseguite su questo terminale non vengono
           inoltrate come accesso ma come programmazione */
        if(msg->Msg[9] == ON_T)
          SmartIntego.term[msg->Address.node].flags |= SI_FLAGS_PROGRAM;
        else if(msg->Msg[9] == OFF_T)
          SmartIntego.term[msg->Address.node].flags &= ~SI_FLAGS_PROGRAM;
        break;
      case LARA_RQ:
        /* Richiesta nodi presenti */
        if(msg->Msg[0] == NODES)
          smartintego_send_nodes();
        break;
      case LARA_ID:
        /* Conferma timbratura */
        /* Non è previsto il controllo di apertura porta sulle serrature,
           quindi replico direttamente alla conferma. */
        memcpy(&SmartIntego.term[msg->Address.node].reply, msg, (msg->Length+5));
        if(SmartIntego.term[msg->Address.node].reply.Id.op_code == TIMBRATO)
          SmartIntego.term[msg->Address.node].reply.Id.op_code = TIMBR_OK;
        else if(SmartIntego.term[msg->Address.node].reply.Id.op_code == TRANS)
          SmartIntego.term[msg->Address.node].reply.Id.op_code = TRANS_OK;
        /* E' necessario inviare SEMPRE una risposta alla serratura
           per interrompere la sua attesa di un qualche messaggio. Quindi
           o apertura o accesso negato. Poiché ci possono essere delle
           politiche di apertura decise dal programma utente, come fare
           quando la tessera è valida ma il programma utente non invia il
           comando di apertura? Si lascia che la serratura vada in timeout
           anche se questo vuol dire aumentare il consumo della batteria? */
        /* Si può definire un nuovo attuatore da comandare solo se si usano
           le serrature SmartIntego, utilizzato per comunicare l'accesso negato. */
        if((SmartIntego.term[msg->Address.node].reply.Id.op_code == TIMBR_OK) ||
           (SmartIntego.term[msg->Address.node].reply.Id.op_code == TRANS_OK))
        {
          /* Tessera riconosciuta, attesa comando di apertura */
          SmartIntego.term[msg->Address.node].pending = 3;
        }
        else
        {
          /* Accesso negato */
          if(SmartIntego.term[msg->Address.node].reply.Id.op_code == SET_G_RESP)
          {
            /* La tessera supervisione attiva la modalità di programmazione tessere.
               Però come programmo la tessera supervsione? */
          }
          
          /* Cerco il gateway di riferimento in base al nodo
             ed invio il comando "Access denied" (3.3.9) alla
             serratura. */
          cmd[0] = 0x80|SI_MSG_ACCESS_DENIED;
          smartintego_send(SmartIntego.term[msg->Address.node].gw,
            SmartIntego.term[msg->Address.node].dev_addr, cmd, 1, 0);
        }
        break;
      case LARA_NODES:
        /* Programmazione nodi */
        /* Con la programmazione nodi tornano anche tutti gli stati in linea,
           quindi vanno ripetuti gli eventi per le serrature non in linea. */
        /* Occorre escludere dalla lista delle serrature gestite i nodi non programmati. */
        for(i=1; i<=LARA_N_TERMINALI; i++)
        {
          if(!(msg->Msg[i/8] & (1<<(i&7))))
          {
            //SmartIntego.term[i].wn_addr = 0;
            SmartIntego.term[i].dev_addr = 0;
          }
          else if((msg->Msg[i/8] & (1<<(i&7))) && !SmartIntego.term[i].dev_addr)
          {
            /* La centrale ha più terminali programmati rispetto a quelli
               configurati in campo. Forzo un dev_addr fasullo. */
            SmartIntego.term[i].dev_addr = -1;
            SmartIntego.term[i].flags |= SI_FLAGS_DOWN;
          }
        }
        
        smartintego_send_nodes();
        break;
      default:
        break;
    }
    
    memmove(SmartIntego.mmbuf,
      SmartIntego.mmbuf+(SmartIntego.mmbuf[4]+5),
      SmartIntego.mmi-(SmartIntego.mmbuf[4]+5));
    SmartIntego.mmi -= (SmartIntego.mmbuf[4]+5);
  }
}

static void* smartintego_mmaster(void *null)
{
  int ret, fd[2];
  fd_set fds;
  
  //sprintf(SmartIntego.mmbuf, "SmartIntego [%d]", pthread_self());
  sprintf(SmartIntego.mmbuf, "SmartIntego [%d]", getpid());
  support_log(SmartIntego.mmbuf);
  
  /* Occorre assicurare che il processo saet abbia già creato i file
     descriptor per i moduli reali, altrimenti si crea prima il virtuale
     che viene sovrascritto dal modulo reale. */
  sleep(1);
  
  socketpair(AF_LOCAL, SOCK_STREAM, 0, fd);
  master_register_module(SmartIntego.mm, fd[0]);
  SmartIntego.fdmm = fd[1];
  
  write(SmartIntego.fdmm, "\033\013\000\000\000\001\005", 7);	// versione 11.0.0.0 e modulo master tipo 5
  
  FD_ZERO(&fds);
  
  while(1)
  {
    ret = SmartIntego.fdmm;
    //if(dev->fd_eth > ret) ret = dev->fd_eth;
    FD_SET(SmartIntego.fdmm, &fds);
    //FD_SET(dev->fd_eth, &fds);
    select(ret+1, &fds, NULL, NULL, NULL);
    
    if(FD_ISSET(SmartIntego.fdmm, &fds))
    {
      /* Comando da centrale che deve essere gestito/inoltrato */
      ret = read(SmartIntego.fdmm, SmartIntego.mmbuf+SmartIntego.mmi, LARA_DIMBUF-SmartIntego.mmi);
      if(ret > 0)
      {
        SmartIntego.mmi += ret;
        smartintego_master_parse();
      }
    }
  }
  
  return NULL;
}

static void smartintego_config()
{
  FILE *fp;
  char line[256], type[32], connection[32];
  unsigned int dev_addr, wn_addr, chipid;
  int ret, term;
  struct smartintego_gateway *tgw;
  
  fp = fopen("/saet/data/smartintego.csv", "r");
  if(!fp) return;
  
  term = 1;
  
  while(fgets(line, sizeof(line)-1, fp))
  {
    /* Nell'esempio della documentazione i campi erano separati da tab,
       invece il fle di configurazione prodotto dal campo prova è separato
       da ';'. La cosa più seplice è sostituire i ';' con spazi, così il
       recupero dei campi funziona in entrambi i casi (es è anche più semplice
       nel caso dei ';'). */
    for(ret=0; line[ret]; ret++)
      if(line[ret] == ';') line[ret] = ' ';
    
    ret = sscanf(line, "%s %x %x %s %x", type, &dev_addr, &wn_addr, connection, &chipid);
    if(ret == 5)
    {
#ifdef DEBUG
      printf("%d: %s %x %x %s %x\n", ret, type, dev_addr, wn_addr, connection, chipid);
#endif
      if(!strcmp(type, "GN_ER"))
      {
        /* GatewayNode (TCP/IP) */
        tgw = calloc(1, sizeof(struct smartintego_gateway));
        tgw->type = SMARTINTEGO_GW_ER;
        tgw->dev_addr = dev_addr;
        //tgw->wn_addr = wn_addr;
        tgw->connection = (char*)strdup(connection);
        tgw->er.fd = -1;
        tgw->er.sa.sin_family = AF_INET;
        //tgw->er.sa.sin_port = htons(2101);
        tgw->er.sa.sin_addr.s_addr = inet_addr(connection);
        tgw->next = SmartIntego.gwhead;
        SmartIntego.gwhead = tgw;
      }
      else if(!strcmp(type, "GN_R"))
      {
        /* GatewayNode Repeater/Range extender */
        /* *** non si deve fare nulla *** */
      }
      else if(!strcmp(type, "GN_CR"))
      {
        /* GatewayNode (RS485) */
        tgw = calloc(1, sizeof(struct smartintego_gateway));
        tgw->type = SMARTINTEGO_GW_CR;
        tgw->dev_addr = dev_addr;
        //tgw->wn_addr = wn_addr;
        tgw->connection = (char*)strdup(connection);
        sscanf(connection, "%d.%d", &(tgw->cr.bus), &(tgw->cr.addr));
        tgw->next = SmartIntego.gwhead;
        SmartIntego.gwhead = tgw;
      }
      //else if(!strcmp(type, "LN_I_MP"))
      else if(!strncmp(type, "LN_", 3))
      {
        /* LockNode */
        /* Ogni serratura è associata ad un terminale Tebe. */
        if(term < (LARA_N_TERMINALI+1))
        {
          /* Cerca il gw associato */
          for(tgw=SmartIntego.gwhead; tgw&&strcmp(tgw->connection,connection); tgw=tgw->next);
          if(tgw)
          {
            SmartIntego.term[term].gw = tgw;
            SmartIntego.term[term].dev_addr = dev_addr;
            //SmartIntego.term[term].wn_addr = wn_addr;
            term++;
          }
        }
      }
    }
  }
  fclose(fp);
}

static void smartintego_init(int mm)
{
  pthread_t pth;
  int i;
  
  pthread_mutex_lock(&SmartIntego.mutex);
  if(!SmartIntego.init)
  {
    SmartIntego.mm = mm;
    
    /* Lettura file di configurazione CSV SmartIntego */
    smartintego_config();
    
    for(i=0; i<=LARA_N_TERMINALI; i++)
    {
      /* Inizializzo senza controllo porta */
      SmartIntego.term[i].doorsens = -1;
      /* Inizializzo con porta aperta e batteria ok */
      SmartIntego.term[i].ingressi = 0x10;
      /* Richiesta iniziale di stato serratura */
      /* Non va bene, la richiesta di stato parte immediatamente dopo quella
         della richiesta stato al gateway e prima di ricevere la risposta che
         allinea i contatori. I contatori quindi non si allineano e le ricezioni
         vengono scartate per fuori sequenza. */
      /* Comunque lo stato delle serrature non viene ricevuto... */
      //SmartIntego.term[i].flags = SI_FLAGS_STATUS;
    }
    
    /* Possono esserci più consumatori di tipo SmartIntego per un unico impianto
       di controllo accesso, ognuno collegato ad un bus (es, impianti misti 485/LAN
       oppure due bus 485 distinti). Il modulo master virtuale è comunque unico
       e gestisce collettivamente tutte le informazioni. */
    pthread_create(&pth, NULL, (PthreadFunction)smartintego_mmaster, NULL);
    while(!SmartIntego.fdmm) sleep(1);
    
    SmartIntego.init = 1;
  }
  pthread_mutex_unlock(&SmartIntego.mutex);
}

void smartintego_send_inputs(int nodo, int opcode)
{
  lara_tMsg_ID msg;
  
  msg.Code = LARA_ID;
  msg.Address.domain = 0;
  msg.Address.subnet = 2;
  msg.Address.node = nodo;
  msg.Length = sizeof(lara_t_id);
  
  msg.Id.op_code = opcode;
  msg.Id.service_code = 0xffff;
  msg.Id.attrib.g_resp = 0;
  msg.Id.attrib.area = 0;
  msg.Id.attrib.abil = 0;
  msg.Id.attrib.gruppo = 0;
  msg.Id.attrib.fessura = 0;
  msg.Id.attrib.spare = 0;
  msg.Id.ingressi = SmartIntego.term[nodo].ingressi;
  msg.Id.id = 0xffff;
  msg.Id.segreto = 0;	//bswap_16(0);
  memset(msg.Id.badge, 0xff, BADGE_LENGTH);
  
  pthread_mutex_lock(&SmartIntego.mutex);
  write(SmartIntego.fdmm, &msg, sizeof(lara_tMsg_ID));
  pthread_mutex_unlock(&SmartIntego.mutex);
}

static void smartintego_check_whitelist(struct smartintego_gateway *gw);

void smartintego_send_whitelist(struct smartintego_gateway *gw, int resend)
{
  unsigned char cmd[24];
  
  /* L'invio precedente è stato confermato */
  gw->flags &= ~SI_FLAGS_WL;
  
  if(!resend) gw->wl.nodo++;
  for(; gw->wl.nodo<=LARA_N_TERMINALI; gw->wl.nodo++)
  {
    /* Terminale di questo gateway */
    if(SmartIntego.term[gw->wl.nodo].gw == gw) break;
  }
{
char log[64];
sprintf(log, "SI (%s): WL run %x nodo %d", gw->connection, gw->flags & SI_FLAGS_RUN, gw->wl.nodo);
support_logmm(log);
}
  if(gw->wl.nodo <= LARA_N_TERMINALI)
  {
    if(gw->flags & SI_FLAGS_RUN)
    {
      /* Verificare che il gw sia ancora in linea */
      if(!(gw->wl.curid & (1<<31)))
        cmd[0] = SI_MSG_ADD_TO_WHITELIST;
      else
        cmd[0] = SI_MSG_REMOVE_FROM_WHITELIST;
      cmd[1] = 0; // setup number
      cmd[2] = 0; // verification via ACP
      cmd[3] = gw->wl.badgelen;
      memcpy(cmd+4, gw->wl.badge, gw->wl.badgelen);
      if(smartintego_send(gw, SmartIntego.term[gw->wl.nodo].dev_addr, cmd, gw->wl.badgelen+4, SI_REFID))
        gw->flags |= SI_FLAGS_WL;
if(gw->flags & SI_FLAGS_WL)
{
char log[64];
sprintf(log, "SI (%s): WL inviato", gw->connection);
support_logmm(log);
}
      /* Timeout comando di 18s */
      gw->timeout = 18;
    }
  }
  else
  {
    if(gw->wl.curid)
    {
      gw->wl.curid = 0;
      pthread_mutex_unlock(&SmartIntego.wlmutex);
    }
    smartintego_check_whitelist(gw);
  }
}

static void smartintego_resend_whitelist(struct smartintego_gateway *gw)
{
  int i;
  
  if(_lara && _laraf && (lara_NumBadge >= 65250))
    memset(gw->wl.id, 0xff, sizeof(gw->wl.id));
}

int smartintego_parse(struct smartintego_gateway *gw)
{
  unsigned short crc;
  unsigned int inseq, nodo, len, ret;
  lara_tMsg_ID msg;
  char log[128];
  int i, l;
  
  /* Nel caso di connessione 485 sono sicuro di parsificare
     un solo messaggio alla volta. */
  ret = 1;
  
  while(gw->ridx >= gw->rbuf[0])
  {
    crc = crc_ccitt(gw->rbuf, gw->rbuf[0]-2);

#ifdef DEBUG
{
  int x;
  printf("(%s) Rx: ", gw->connection);
  for(x=0; x<gw->rbuf[0]; x++) printf("%02x", gw->rbuf[x]);
  printf("\n");
#if 0
  printf("CRC: ricevuto %04x calcolato %04x\n",
      (gw->rbuf[gw->ridx-2]<<8)+gw->rbuf[gw->ridx-1], crc);
#endif
}
#endif
    if((gw->rbuf[1] == 0xFE) && (gw->rbuf[2] == 0xFD) &&
       (crc == ((gw->rbuf[gw->rbuf[0]-2]<<8)+gw->rbuf[gw->rbuf[0]-1])))
    {
      sprintf(log, "SI (%s) Rx: ", gw->connection);
      l = strlen(log);
      for(i=0; i<gw->rbuf[0]; i++) sprintf(log+l+i*2, "%02x", gw->rbuf[i]);
      support_logmm(log);
      
      inseq = (gw->rbuf[8]<<24)|(gw->rbuf[9]<<16)|(gw->rbuf[10]<<8)|gw->rbuf[11];

      if((gw->outseq == 0) && (gw->rbuf[7] & 0x80) && (gw->rbuf[16] == SI_MSG_GET_STATUS))
      {
        /* Ho appena attivato una connessione e ho eseguito un GetStatus.
           Assumo la risposta sempre corretta e quindi inizializzo il contatore
           atteso per il messaggio in ingresso. */
        gw->inseq = inseq;
sprintf(log, "SI (%s): Impostato 'inseq'", gw->connection);
support_logmm(log);
        if(!(gw->flags & SI_FLAGS_RUN))
        {
          gw->flags |= SI_FLAGS_RUN;
          smartintego_send_nodes();
//printf("Invio whitelist\n");                    
sprintf(log, "SI (%s): Invio whitelist", gw->connection);
support_logmm(log);
          smartintego_resend_whitelist(gw);
        }
      }
      if(inseq >= gw->inseq)
      {
sprintf(log, "SI (%s): Verificata sequenza", gw->connection);
support_logmm(log);
        gw->inseq = inseq;
        if(gw->inseq == 0xffffffff) gw->inseq = 0;
        
        if((gw->rbuf[16] == SI_MSG_RESPONSE) && (gw->rbuf[7] == (0x80|SI_REFID)))
        {
sprintf(log, "SI (%s): Conferma whitelist", gw->connection);
support_logmm(log);
          /* E' la conferma alla programmazione della whitelist, invio subito il comando successivo. */
          if((gw->rbuf[17] != 1) && (gw->rbuf[17] != 0xa))
            /* Conferma */
            smartintego_send_whitelist(gw, 0);
          else
            /* Errore - renvio */
            smartintego_send_whitelist(gw, 1);
        }
        else if((gw->rbuf[16] == SI_MSG_READER_EVENT) ||
               /* I nuovi fw delle serrature se non configurati correttamente
                  producono questo evento che pare avere la stessa struttura
                  del reader event. Si tratta di un tampone. */
                (gw->rbuf[16] == SI_MSG_SYSTEM_INCONSISTENCY_EVENT))
        {
if(gw->rbuf[16] == SI_MSG_SYSTEM_INCONSISTENCY_EVENT)
{
sprintf(log, "SI (%s): *** SYSTEM_INCONSISTENCY_EVENT ***", gw->connection);
support_logmm(log);
}
          /* Timbratura */
          /* Notifico la timbratura al sistema Tebe per verifica */
          msg.Code = LARA_ID;
          msg.Address.domain = 0;
          msg.Address.subnet = 2;
          /* Recupero il dev_addr mittente e cerco il nodo corrispondente */
          inseq = (gw->rbuf[14]<<8) + gw->rbuf[15];
          for(nodo=1; nodo<=LARA_N_TERMINALI; nodo++)
            if(SmartIntego.term[nodo].dev_addr == inseq) break;
          /* Se trovo il nodo corrispondente, trasmetto l'evento al sistema Tebe */
          if(nodo <= LARA_N_TERMINALI)
          {
//printf("Timbratura (prog %d)\n", SmartIntego.term[nodo].flags & SI_FLAGS_PROGRAM);
            /* 0x00 porta aperta, 0x04 porta chiusa
               0x00 batteria bassa , 0x10 batteria ok */
            /* basic lock state (bit 3: warn batt, bit 4: alarm batt) */
            SmartIntego.term[nodo].ingressi &= ~0x10;
            SmartIntego.term[nodo].ingressi |= (gw->rbuf[23]&(1<<3))?0:0x10;
          
            msg.Address.node = nodo;
            msg.Length = sizeof(lara_t_id);
            
#if 0
            if(SmartIntego.term[nodo].flags & SI_FLAGS_PROGRAM)
              msg.Id.op_code = C_BADGE_L | BADGE_DIRECT_CODE;
            else
              msg.Id.op_code = ACC_BADGE | BADGE_DIRECT_CODE;
#else
            /* Attualmente l'applicazione per FastWeb prevede di leggere
               un codice già in formato ASCII, quindi lo uso direttamente.
               Se in futuro si vorrà utilizzare per altre applicazioni lo
               UniqueID allora occorre prevedere o di tradurre lo UniqueID
               in un formato ASCII (preferibile) oppure passare il codice
               binario come BADGE_DIRECT_CODE. In tutti i casi si dovrà
               aggiungere un parametro di configurazione per la modalità. */
            if(SmartIntego.term[nodo].flags & SI_FLAGS_PROGRAM)
              msg.Id.op_code = C_BADGE_L;
            else
              msg.Id.op_code = ACC_BADGE;
#endif
            msg.Id.service_code = 0xffff;
            msg.Id.attrib.g_resp = 0;
            msg.Id.attrib.area = 0;
            msg.Id.attrib.abil = 0;
            msg.Id.attrib.gruppo = 0;
            msg.Id.attrib.fessura = 0;
            msg.Id.attrib.spare = 0;
            msg.Id.ingressi = SmartIntego.term[nodo].ingressi;
            msg.Id.id = 0xffff;
            msg.Id.segreto = 0;	//bswap_16(0);
            len = gw->rbuf[24];
            if(len > BADGE_LENGTH) len = BADGE_LENGTH;
            memcpy(msg.Id.badge, gw->rbuf+25, len);
            memset(msg.Id.badge+len, 0xff, BADGE_LENGTH-len);
            
            pthread_mutex_lock(&SmartIntego.mutex);
            write(SmartIntego.fdmm, &msg, sizeof(lara_tMsg_ID));
            pthread_mutex_unlock(&SmartIntego.mutex);
          }
        }
        else if(gw->rbuf[16] == SI_MSG_READER_INFO_EVENT)
        {
          /* Lo gestisco solo per il controllo batteria */
          /* Recupero il dev_addr mittente e cerco il nodo corrispondente */
          inseq = (gw->rbuf[14]<<8) + gw->rbuf[15];
          for(nodo=1; nodo<=LARA_N_TERMINALI; nodo++)
            if(SmartIntego.term[nodo].dev_addr == inseq) break;
          /* Se trovo il nodo corrispondente, trasmetto l'evento al sistema Tebe */
          if(nodo <= LARA_N_TERMINALI)
          {
            /* 0x00 porta aperta, 0x04 porta chiusa
               0x00 batteria bassa , 0x10 batteria ok */
            /* basic lock state (bit 3: warn batt, bit 4: alarm batt) */
            SmartIntego.term[nodo].ingressi &= ~0x10;
            SmartIntego.term[nodo].ingressi |= (gw->rbuf[23]&(1<<3))?0:0x10;
            
            smartintego_send_inputs(nodo, CHANGE_INPUT);
          }
        }
      }
      else
      {
        /* Errore sequenza */
        /* Le specifiche indicano di rispondere con un esito Sequence counter error (07) */
        unsigned char cmd[8];
        //gw->inseq++;
        cmd[0] = SI_MSG_RESPONSE;
        cmd[1] = 0x07; //
        cmd[2] = gw->inseq >> 24; //
        cmd[3] = gw->inseq >> 16; //
        cmd[4] = gw->inseq >> 8; //
        cmd[5] = gw->inseq; //
        smartintego_send(gw, gw->dev_addr, cmd, 6, 0);
      }
    }
    else
      ret = 0;
    
    /* Consuma il messaggio ricevuto */
    i = gw->rbuf[0];
    memmove(gw->rbuf, gw->rbuf+i, gw->ridx-i);
    gw->ridx -= i;
  }
  return ret;
}

static int smartintego_badge(unsigned char *si_badge, unsigned char *badge)
{
  int i, j;
  
  j = 0;
  for(i=0; i<10; i++)
  {
    if((badge[i]&0xf0) != 0xf0)
      si_badge[j++] = 0x30|(badge[i]>>4);
    else
      break;
    if((badge[i]&0x0f) != 0x0f)
      si_badge[j++] = 0x30|(badge[i]&0xf);
    else
      break;
  }
  return j;
}

static void smartintego_check_whitelist(struct smartintego_gateway *gw)
{
  int i;
  
  if(_lara && _laraf && (lara_NumBadge >= 65250))
  {
    if(!(gw->flags & SI_FLAGS_INIT))
    {
      sleep(1);
      gw->flags |= SI_FLAGS_INIT;
      smartintego_resend_whitelist(gw);
    }
    
    if(gw->wl.curid && !gw->timeout)
    {
      /* Se ricevo un evento dal gateway mentre sto aspettando l'esito della
         programmazione della whitelist, il timeout si azzera subito. Se poi
         l'esito della programmazione non arriva per qualche ragione, il
         meccanismo di invio rimane bloccato, il mutex non si libera più.
         Se mi accorgo quindi di avere una tessera della whitelist in attesa
         ma il timeout nel frattempo è scaduto, reinnesco il timeout.
         L'esito potrebbe ancora essere lì per arrivare, ma se non arriva
         scatta il reinvio allos cadere naturale del timeout. */
      gw->timeout = 18;
    }
    
    if(!gw->wl.curid && (gw->flags & SI_FLAGS_RUN) && !pthread_mutex_trylock(&SmartIntego.wlmutex))
    {
      /* Cerco una tessera da inviare nella whitelist */
      for(i=65000; i<65250; i++)
        if(gw->wl.id[(i-65000)/8] & (1<<((i-65000)&7)))
        {
          /* Invia la tessera a tutti i terminali (serrature).
             Posso mandare un comando a tutti i gw per ottimizzare,
             ma ogni serratura di un gw va programmata in sequenza. */
          gw->wl.id[(i-65000)/8] &= ~(1<<((i-65000)&7));
          if((_lara->tessera[i].stato.s.abil != BADGE_VUOTO) &&
             (_lara->tessera[i].stato.s.badge & BADGE_PROGR))
          {
            gw->wl.curid = i;
            if(_lara->tessera[i].stato.s.abil != BADGE_ABIL)
              gw->wl.curid |= (1<<31);  // cancellazione
            gw->wl.badgelen = smartintego_badge(gw->wl.badge, _lara->tessera[i].badge);
            gw->wl.nodo = 0;
            smartintego_send_whitelist(gw, 0);
            break;
          }
        }
      if(!gw->wl.curid)
        pthread_mutex_unlock(&SmartIntego.wlmutex);
    }
    
    /* Verifica la prenotazione di aggiornamento tessera a livello di centrale. */
    pthread_mutex_lock(&SmartIntego.mutex);
    for(i=65000; i<65250; i++)
      if(_laraf->tessera[i].ag)
      {
        _laraf->tessera[i].ag = 0;
        for(gw=SmartIntego.gwhead; gw; gw=gw->next)
          gw->wl.id[(i-65000)/8] |= (1<<((i-65000)&7));
      }
    pthread_mutex_unlock(&SmartIntego.mutex);
  }
}

static void smartintego_loop(ProtDevice *dev)
{
  int i, ret, fd[2], bus;
  unsigned char cmd[32];
  struct smartintego_gateway *gw;
  static int one = 1;
  char log[128];
  
  fd_set fdr, fdw;
  int fdmax, sret;
  struct timeval to;
  
  if(!config.consumer[dev->consumer].param)
  {
    sleep(1);
    return;
  }
  
  sprintf((char*)cmd, "SmartIntego loop [%d]", getpid());
  support_log((char*)cmd);
  
//  dev->prot_data = calloc(1, sizeof(struct smartintego_data));
//  if(!dev->prot_data) return;
  
  /* lettura parametri - n.modulo e bus485 */
  ret = sscanf(config.consumer[dev->consumer].param, "%d:%d", &i, &bus);
  if(!ret || (i > 15) || (i < 0))
  {
    free(dev->prot_data);
    sleep(1);
    return;
  }
    
  smartintego_init(i);
  
  /* Occorre configurare un consumatore per ogni bus 485 ed
     un solo consumatore ethernet per tutti i GN via TCP.
     Se questo è il consumatore ethernet apre tutte le
     connessioni necessarie. */
  
  if(config.consumer[dev->consumer].configured == 5)
  {
    fdmax = 0;
    to.tv_sec = 1;
    to.tv_usec = 0;
    
    while(1)
    {
      FD_ZERO(&fdr);
      FD_ZERO(&fdw);
      
      /* Apre le connessioni */
      for(gw=SmartIntego.gwhead; gw; gw=gw->next)
      {
        if(gw->type == SMARTINTEGO_GW_ER)
        {
          if((gw->er.fd < 0) && !gw->er.wait)
          {
            gw->er.fd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
            if(gw->er.fd < 0) continue;
            /* L'apertura della connessione non deve essere bloccante,
               deve poter essere ripetuta se dovesse cadere, e deve
               essere con keepalive. */
            /* I default per il keepalive lato linux sono:
               7200s di idle prima di iniziare con i probe (2h)
               75s tra i probe
               9 probe falliti per chiudere la connessione. (11min)*/
            /* Si possono modificare in "/proc/sys/net/ipv4/tcp_keepalive_*" */
            //setsockopt(gw->er.fd, SOL_SOCKET, SO_KEEPALIVE, &one, sizeof(int));
            /* Il keepalive non serve a nulla, il gateway chiude il socket dopo
               30 secondi di inattività, anche se imposto il keepalive a 20s.
               L'unico modo per non far chiudere la connessione è continuare a
               far transitare qualcosa, tipicamente un GetStatus ogni 25s. */
            fcntl(gw->er.fd, F_GETFL, &i);
            i |= O_NONBLOCK;
            fcntl(gw->er.fd, F_SETFL, i);
            gw->er.sa.sin_port = htons(config.consumer[dev->consumer].data.eth.port);
            ret = connect(gw->er.fd, (struct sockaddr*)&(gw->er.sa), sizeof(struct sockaddr));
{
  char log[64];
  sprintf(log, "SI: %s - connessione %08x %d (%d)", gw->connection, gw->er.sa.sin_addr.s_addr, config.consumer[dev->consumer].data.eth.port, gw->er.fd);
  support_log(log);
}
            if(ret < 0)
            {
              if(errno == EINPROGRESS)
              {
{
  char log[64];
  sprintf(log, "SI: %s - connessione differita", gw->connection);
  support_log(log);
}
                /* Attende l'esito differito */
                FD_SET(gw->er.fd, &fdw);
                if(gw->er.fd > fdmax) fdmax = gw->er.fd;
                /* Tempo massimo per la connessione. */
                gw->er.wait = 10;
              }
              else
              {
{
  char log[64];
  sprintf(log, "SI: %s - connessione rifiutata (%d)", gw->connection, errno);
  support_log(log);
}
                close(gw->er.fd);
                gw->er.fd = -1;
                gw->watchdog = 0;
                /* Attende qualche secondo prima di ritentare. */
                gw->er.wait = 40;
              }
            }
            else
            {
{
  char log[64];
  sprintf(log, "SI: %s - connesso", gw->connection);
  support_log(log);
}
              /* La connessione si è già aperta al volo */
              gw->er.connected = 1;
              gw->ridx = 0;
              gw->outseq = 0xfffffffe;
              //gw->er.wait = 0;
              //gw->timeout = 0;
              FD_SET(gw->er.fd, &fdr);
              if(gw->er.fd > fdmax) fdmax = gw->er.fd;
              
              /* Invio immediatamente una richiesta di stato per allineare i contatori */
              cmd[0] = SI_MSG_GET_STATUS;
              smartintego_send(gw, gw->dev_addr, cmd, 1, 0);
              /* Timeout comando di 15s */
              gw->timeout = 15;
            }
          }
          else if(gw->er.connected)
          {
            FD_SET(gw->er.fd, &fdr);
          }
        }
      }
      
      sret = select(fdmax+1, &fdr, &fdw, NULL, &to);
      if(sret <= 0)
      {
        to.tv_sec = 1;
        to.tv_usec = 0;
        //continue;
      }
      
      if(!sret)
      {
        pthread_mutex_lock(&SmartIntego.mutex);
        for(i=1; i<=LARA_N_TERMINALI; i++)
        {
          if(SmartIntego.term[i].pending)
            SmartIntego.term[i].pending--;
          if(SmartIntego.term[i].enter > 0)
            SmartIntego.term[i].enter--;
          if(SmartIntego.term[i].doortimeout)
          {
            SmartIntego.term[i].doortimeout--;
            if(!SmartIntego.term[i].doortimeout)
            {
//printf("Porta aperta\n");
              pthread_mutex_unlock(&SmartIntego.mutex);
              smartintego_send_inputs(i, DOOR_TIMEOUT);
              pthread_mutex_lock(&SmartIntego.mutex);
            }
          }
        }
        pthread_mutex_unlock(&SmartIntego.mutex);
        
        for(gw=SmartIntego.gwhead; gw; gw=gw->next)
        {
          if(gw->type == SMARTINTEGO_GW_ER)
          {
            if(gw->watchdog)
            {
              gw->watchdog--;
              if((gw->watchdog == 10) || !gw->watchdog)
              {
                struct smartintego_gateway *gw2;
                
                /* A 10 secondi dalla fine e poi alla fine
                   stampa tutti i dati dello stato dei gateway */
                char buf[64];
                FILE *fp;
                
                sprintf(buf, "/tmp/GW_%d.log", gw->connection, getpid());
                fp = fopen(buf, "a");
                
                fprintf(fp, "----------------------\n");
                fprintf(fp, "time=%ld\n", time(NULL));
                for(gw2=SmartIntego.gwhead; gw2; gw2=gw2->next)
                {
                  if(gw2->type == SMARTINTEGO_GW_ER)
                  {
                    fprintf(fp, "gateway %s type ER\n", gw2->connection);
                    fprintf(fp, "  fd=%d\n  connected=%d\n  wait=%d\n", gw2->er.fd, gw2->er.connected, gw2->er.wait);
                    fprintf(fp, "  ridx=%d\n  outseq=%u\n  inseq=%u\n", gw2->ridx, gw2->outseq, gw2->inseq);
                    fprintf(fp, "  flags=0x%x\n  timeout=%d\n\n", gw2->flags, gw2->timeout);
                  }
                }
                fclose(fp);
                /* Esegue un riavvio della centrale */
                if(!gw->watchdog) exit(0);
              }
            }
          }
        }
      }
      
      for(gw=SmartIntego.gwhead; gw; gw=gw->next)
      {
        if(gw->type == SMARTINTEGO_GW_ER)
        {
          smartintego_check_whitelist(gw);
          
          if(gw->er.fd >= 0)
          {
            if(!gw->er.connected)
            {
              if(FD_ISSET(gw->er.fd, &fdw))
              {
                i = sizeof(int);
                getsockopt(gw->er.fd, SOL_SOCKET, SO_ERROR, &ret, &i);
                if(ret)
                {
{
  char log[64];
  sprintf(log, "SI: %s - connessione rifiutata (d)", gw->connection);
  support_log(log);
}
                  /* Errore connessione */
                  close(gw->er.fd);
                  gw->er.fd = -1;
                  /* Attende qualche secondo prima di ritentare. */
                  gw->er.wait = 40;
                  gw->watchdog = 0;
                  /* Segnalazione caduta nodi associati al gateway */
                  if(!(gw->flags & SI_FLAGS_DOWN))
                  {
                    gw->flags |= SI_FLAGS_DOWN;
                    if((gw->flags & SI_FLAGS_RUN) && (gw->flags & SI_FLAGS_WL))
                      smartintego_send_whitelist(gw, 0);
                    gw->flags &= ~SI_FLAGS_RUN;
                    for(i=1; i<=LARA_N_TERMINALI; i++)
                      if(SmartIntego.term[i].gw == gw)
                        SmartIntego.term[i].flags |= SI_FLAGS_DOWN;
                    smartintego_send_nodes();
                  }
                }
                else
                {
{
  char log[64];
  sprintf(log, "SI: %s - connesso (d)", gw->connection);
  support_log(log);
}
                  gw->er.connected = 1;
                  gw->ridx = 0;
                  gw->outseq = 0xfffffffe;
                  //gw->er.wait = 0;
                  //gw->timeout = 0;
                  
                  /* Segnalazione ripristino nodi associati al gateway */
                  if(gw->flags & SI_FLAGS_DOWN)
                  {
                    gw->flags &= ~(SI_FLAGS_PRE_DOWN|SI_FLAGS_DOWN|SI_FLAGS_RUN);
                    for(i=1; i<=LARA_N_TERMINALI; i++)
                      if(SmartIntego.term[i].gw == gw)
                        SmartIntego.term[i].flags &= ~SI_FLAGS_DOWN;
                  }
                  
                  /* Invio immediatamente una richiesta di stato per allineare i contatori */
                  cmd[0] = SI_MSG_GET_STATUS;
                  smartintego_send(gw, gw->dev_addr, cmd, 1, 0);
                  /* Timeout comando di 15s */
                  gw->timeout = 15;
                }
              }
            }
            else if(gw->er.connected && FD_ISSET(gw->er.fd, &fdr))
            {
              ret = read(gw->er.fd, gw->rbuf+gw->ridx, sizeof(gw->rbuf)-gw->ridx);
              if(ret <= 0)
              {
{
  char log[64];
  sprintf(log, "SI: %s - disconnesso", gw->connection);
  support_log(log);
}
                /* La connessione è stata chiusa */
                close(gw->er.fd);
                gw->er.fd = -1;
                gw->er.connected = 0;
                gw->er.wait = 2;
                gw->watchdog = 0;
                /* Segnalazione caduta nodi associati al gateway */
                if(!(gw->flags & SI_FLAGS_DOWN))
                {
                  gw->flags |= SI_FLAGS_DOWN;
                  if((gw->flags & SI_FLAGS_RUN) && (gw->flags & SI_FLAGS_WL))
                    smartintego_send_whitelist(gw, 0);
                  gw->flags &= ~SI_FLAGS_RUN;
                  for(i=1; i<=LARA_N_TERMINALI; i++)
                    if(SmartIntego.term[i].gw == gw)
                      SmartIntego.term[i].flags |= SI_FLAGS_DOWN;
                  smartintego_send_nodes();
                }
              }
              else
              {
                gw->ridx += ret;
                smartintego_parse(gw);
                /* Se ho ricevuto, il gateway è necessariamente connesso */
                gw->flags &= ~SI_FLAGS_PRE_DOWN;
                /* Anche se azzero il timeout senza verificare che la ricezione
                   sia la conferma al comando inviato, nel caso della whitelist
                   mi accorgo della condizione e reinnesco l'invio. */
                gw->timeout = 0;
              }
            }
            else if(!gw->er.wait)
            {
              cmd[0] = SI_MSG_GET_STATUS;
              smartintego_send(gw, gw->dev_addr, cmd, 1, 0);
              /* Timeout comando di 15s */
              gw->timeout = 15;
            }
          }
          
          if(gw->er.wait && !sret)
          {
            gw->er.wait--;
            if(!gw->er.wait && !gw->er.connected && (gw->er.fd >= 0))
            {
{
  char log[64];
  sprintf(log, "SI: %s - timeout connessione", gw->connection);
  support_log(log);
}
              /* Non si è attivata la connessione nel tempo massimo */
              close(gw->er.fd);
              gw->er.fd = -1;
              gw->watchdog = 0;
              /* Segnalazione caduta nodi associati al gateway */
              if(!(gw->flags & SI_FLAGS_DOWN))
              {
                gw->flags |= SI_FLAGS_DOWN;
                if((gw->flags & SI_FLAGS_RUN) && (gw->flags & SI_FLAGS_WL))
                  smartintego_send_whitelist(gw, 0);
                gw->flags &= ~SI_FLAGS_RUN;
                for(i=1; i<=LARA_N_TERMINALI; i++)
                  if(SmartIntego.term[i].gw == gw)
                    SmartIntego.term[i].flags |= SI_FLAGS_DOWN;
                smartintego_send_nodes();
              }
            }
          }
          
          if(gw->timeout && !sret)
          {
            gw->timeout--;
            if(!gw->timeout)
            {
              if(!(gw->flags & SI_FLAGS_PRE_DOWN))
              {
                gw->flags |= SI_FLAGS_PRE_DOWN;
                if((gw->flags & SI_FLAGS_RUN) && (gw->flags & SI_FLAGS_WL))
                  smartintego_send_whitelist(gw, 1);
              }
              else if(!(gw->flags & SI_FLAGS_DOWN))
              {
{
  char log[64];
  sprintf(log, "SI: %s - timeout", gw->connection);
  support_log(log);
}
                /* Il gateway non risponde, chiudo la connessione */
                close(gw->er.fd);
                gw->er.fd = -1;
                gw->er.connected = 0;
                gw->er.wait = 0;
                gw->watchdog = 0;
                /* Segnalo la caduta dei nodi */
                if(!(gw->flags & SI_FLAGS_DOWN))
                {
                  gw->flags |= SI_FLAGS_DOWN;
                  if((gw->flags & SI_FLAGS_RUN) && (gw->flags & SI_FLAGS_WL))
                    smartintego_send_whitelist(gw, 0);
                  gw->flags &= ~SI_FLAGS_RUN;
                  for(i=1; i<=LARA_N_TERMINALI; i++)
                    if(SmartIntego.term[i].gw == gw)
                      SmartIntego.term[i].flags |= SI_FLAGS_DOWN;
                  smartintego_send_nodes();
                }
              }
            }
          }
        }
      }
    }
  }
  else
  {
    /* La seriale viene aperta con controllo di flusso, non dovrebbe
       essere un problema per il convertitore 485.
       Imposto solo il timeout per sbloccare l'attesa della risposta. */
    struct termios tio;
    tcgetattr(dev->fd, &tio);
    tio.c_cc[VTIME] = 10;
    tio.c_cc[VMIN] = 0;
    tcsetattr(dev->fd, TCSANOW, &tio);
    
    for(gw=SmartIntego.gwhead; gw; gw=gw->next)
    {
      if((gw->type == SMARTINTEGO_GW_CR) && (gw->cr.bus == bus))
      {
        gw->ridx = 0;
        gw->outseq = 0xfffffffe;
        
        /* Allineo subito i contatori */
        cmd[0] = SI_MSG_GET_STATUS;
        smartintego_send(gw, gw->dev_addr, cmd, 1, 0);
      }
    }
    
    while(1)
    {
      static unsigned char syncro[] = {0xff, 0xfe, 0xfe, 0xfe, 0xfe, 0x55, 0x00, 0x00, 0x00, 0x00};
      usleep(2000);
      write(dev->fd, syncro, sizeof(syncro));
      
      for(gw=SmartIntego.gwhead; gw; gw=gw->next)
      {
        if((gw->type == SMARTINTEGO_GW_CR) && (gw->cr.bus == bus))
        {
          smartintego_check_whitelist(gw);
          
          usleep(3000);
          if(!gw->cr.cmd)
          {
            /* Richiesta stato serratura */
            for(i=1; i<=LARA_N_TERMINALI; i++)
            {
              if((SmartIntego.term[i].gw == gw) &&
                 (SmartIntego.term[i].flags & SI_FLAGS_STATUS))
              {
                cmd[0] = SI_MSG_GET_STATUS;
                smartintego_send(gw, SmartIntego.term[i].dev_addr, cmd, 1, 0);
                /* Timeout comando di 15s */
                gw->timeout = 15;
                SmartIntego.term[i].flags &= ~SI_FLAGS_STATUS;
                break;
              }
            }
          }
          if(gw->cr.cmd)
          {
            /* Comando pendente */
            sprintf(log, "SI (%s) Tx: ", gw->connection);
            ret = strlen(log);
            for(i=0; i<gw->cr.cmd[0]; i++) sprintf(log+ret+i*2, "%02x", gw->cr.cmd[i]);
            support_logmm(log);
            
            cmd[0] = 0xfc;
            cmd[1] = gw->cr.addr;
            write(dev->fd, cmd, 2);
            write(dev->fd, gw->cr.cmd, gw->cr.cmd[0]);
            gw->timeout = 15;
            
            /* Attesa ACK/NACK */
            gw->ridx = 0;
            while(1)
            {
              ret = read(dev->fd, gw->rbuf+gw->ridx, sizeof(gw->rbuf)-gw->ridx);
              if(ret > 0)
              {
                gw->ridx += ret;
                if(gw->ridx >= 2)
                {
                  if((gw->rbuf[0] == 0xff) && (gw->rbuf[1] == 0x01))
                  {
                    /* Ricevuto ACK, scarto il comando */
                    free(gw->cr.cmd);
                    gw->cr.cmd = NULL;
                  }
                  else
                  {
                    /* Ricevuto NACK o altro non previsto */
                    gw->cr.retry++;
                    if(gw->cr.retry >= 3)
                    {
                      /* Il messaggio non è stato spedito, lo scarto dopo 3 tentativi */
                      free(gw->cr.cmd);
                      gw->cr.cmd = NULL;
                    }
                  }
                  break;
                }
              }
              else
              {
                /* Timeout RX */
                /* Il timeout per un "ACP initiated command" è 15 secondi */
                if(gw->timeout && !gw->ridx)
                  gw->timeout--;
                else
                {
                  gw->cr.retry++;
                  if(gw->cr.retry >= 3)
                  {
                    /* Il messaggio non è stato spedito, lo scarto dopo 3 tentativi */
                    free(gw->cr.cmd);
                    gw->cr.cmd = NULL;
                  }
                  break;
                }
              }
            }
          }
          else
          {
            /* Poll */
            write(dev->fd, &gw->cr.addr, 1);
            
            /* Attesa messaggio o ack veloce */
            gw->ridx = 0;
            while(1)
            {
              ret = read(dev->fd, gw->rbuf+gw->ridx, sizeof(gw->rbuf)-gw->ridx);
              if(ret > 0)
              {
                gw->ridx += ret;
                if(gw->rbuf[0] == gw->cr.addr)
                {
                  /* Segnalazione ripristino nodi associati al gateway */
                  if(gw->flags & SI_FLAGS_DOWN)
                  {
                    gw->ridx = 0;
                    gw->outseq = 0xfffffffe;
                    gw->timeout = 0;
                    
                    gw->flags &= ~(SI_FLAGS_PRE_DOWN|SI_FLAGS_DOWN|SI_FLAGS_RUN);
                    for(i=1; i<=LARA_N_TERMINALI; i++)
                      if(SmartIntego.term[i].gw == gw)
                        SmartIntego.term[i].flags &= ~SI_FLAGS_DOWN;
                    /* Allineo subito i contatori */
                    cmd[0] = SI_MSG_GET_STATUS;
                    smartintego_send(gw, gw->dev_addr, cmd, 1, 0);
                  }
                  /* Nulla da comunicare, passo al gateway successivo. */
                  break;
                }
                else if(gw->rbuf[0] == 0xfc)
                {
                  /* Segnalazione ripristino nodi associati al gateway */
                  if(gw->flags & SI_FLAGS_DOWN)
                  {
                    gw->ridx = 0;
                    gw->outseq = 0xfffffffe;
                    gw->timeout = 0;
                    
                    gw->flags &= ~(SI_FLAGS_PRE_DOWN|SI_FLAGS_DOWN|SI_FLAGS_RUN);
                    for(i=1; i<=LARA_N_TERMINALI; i++)
                      if(SmartIntego.term[i].gw == gw)
                        SmartIntego.term[i].flags &= ~SI_FLAGS_DOWN;
                    /* Allineo subito i contatori */
                    cmd[0] = SI_MSG_GET_STATUS;
                    smartintego_send(gw, gw->dev_addr, cmd, 1, 0);
                  }
                  /* E' in arrivo un messaggio, lo leggo tutto. */
                  if((gw->ridx >= 3) && (gw->ridx >= (gw->rbuf[2])+2))
                  {
                    memmove(gw->rbuf, gw->rbuf+2, gw->ridx-2);
                    gw->ridx -= 2;
                    
                    if(smartintego_parse(gw))
                    {
                      cmd[1] = gw->cr.addr;	// ACK
                    }
                    else
                      cmd[1] = 0xfb;	// NACK
                      
                    /* Invio ACK/NACK */
                    usleep(2500);
                    cmd[0] = 0xff;
                    write(dev->fd, cmd, 2);
                    break;
                  }
                }
              }
              else
              {
                /* Timeout */
                /* Segnalazione caduta nodi associati al gateway */
                if(!(gw->flags & SI_FLAGS_PRE_DOWN))
                  gw->flags |= SI_FLAGS_PRE_DOWN;
                else if(!(gw->flags & SI_FLAGS_DOWN))
                {
                  gw->flags |= SI_FLAGS_DOWN;
                  if((gw->flags & SI_FLAGS_RUN) && (gw->flags & SI_FLAGS_WL))
                    smartintego_send_whitelist(gw, 0);
                  gw->flags &= ~SI_FLAGS_RUN;
                  for(i=1; i<=LARA_N_TERMINALI; i++)
                    if(SmartIntego.term[i].gw == gw)
                      SmartIntego.term[i].flags |= SI_FLAGS_DOWN;
                  smartintego_send_nodes();
                }
                break;
              }
            }
          }
        }
      }
    }
  }
}

void SmartIntego_door_check(int term, int se)
{
  if(term > LARA_N_TERMINALI) return;
  
  /* Al primo giro di programma utente fatto per verifica,
     il modulo master virtuale non è ancora inizializzato.
     Non devo quindi eseguire alcuna verifica altrimenti
     non avrei un destinatario per lo stato porta. */
  /* Potrebbe anche succedere che il programma utente
     preveda la chiamata a questa funzione, ma poi il plugin
     non sia configurato tra i consumatori. Quindi il modulo
     master non verrà mai attivato e la funzione non deve
     perciò fare nulla. */
  if(SmartIntego.fdmm == 0) return;
  
  /* I sensori utilizzati per il controllo porta non sono associati ad una zona,
     quindi non sono disponibili i fronti di salita e discesa per l'allarme. */
  SmartIntego.term[term].doorsens = se;
  if((se >= 0) && (se < n_SE))
  {
//printf("SE[%d]=%02x ingressi=%02x\n", se, SE[se] & bitAlarm, SmartIntego.term[term].ingressi);
    if((SE[se] & bitAlarm) && (SmartIntego.term[term].ingressi & 0x04))
    {
      /* Apertura porta */
      /* 0x00 porta aperta, 0x04 porta chiusa */
      SmartIntego.term[term].ingressi &= ~0x04;
      if(SmartIntego.term[term].enter)
      {
//printf("Transito porta\n");
        /* Transito */
        pthread_mutex_lock(&SmartIntego.mutex);
        SmartIntego.term[term].reply.Id.ingressi = SmartIntego.term[term].ingressi;
        if(SmartIntego.term[term].enter > 0)
        {
          SmartIntego.term[term].enter = 0;
          SmartIntego.term[term].doortimeout = _lara->terminale[term-1].timeopentimeout;
          write(SmartIntego.fdmm, &SmartIntego.term[term].reply, SmartIntego.term[term].reply.Length+5);
          pthread_mutex_unlock(&SmartIntego.mutex);
        }
        else
        {
          pthread_mutex_unlock(&SmartIntego.mutex);
          smartintego_send_inputs(term, CHANGE_INPUT);
        }
      }
      else
      {
//printf("Forzatura\n");
        /* Cambio stato (forzatura) */
        smartintego_send_inputs(term, FORZATURA);
      }
    }
    else if(!(SE[se] & bitAlarm) && !(SmartIntego.term[term].ingressi & 0x04))
    {
//printf("Chiusura porta\n");
      /* Chiusura porta */
      SmartIntego.term[term].ingressi |= 0x04;
      smartintego_send_inputs(term, CHANGE_INPUT);
      pthread_mutex_lock(&SmartIntego.mutex);
      SmartIntego.term[term].doortimeout = 0;
      pthread_mutex_unlock(&SmartIntego.mutex);
    }
  }
}

void _init()
{
  printf("SmartIntego (plugin): " __DATE__ " " __TIME__ "\n");
  prot_plugin_register("SVSI", 0, NULL, NULL, (PthreadFunction)smartintego_loop);
}

