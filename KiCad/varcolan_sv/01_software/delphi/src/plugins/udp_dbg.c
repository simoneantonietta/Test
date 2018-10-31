/*

Connessione in chiaro: una nuova connessione puo' avvenire solo dopo il timeout della precedente
Connessione crittata: una nuova connessione puo' avvenire anche da un nuovo IP (backup) se si riconosce il segreto impostato.

*/

#include "protocol.h"
#include "codec.h"
#include "support.h"
#include "timeout.h"
#include "master.h"
#include <unistd.h>
#include <pthread.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <byteswap.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>

#include "aes.c"

#define DEBUG

#define BASESECONDS	631148400

#define bitResend	0x01
//#define bitAuth		0x04
#define bitConnect	0x08
#define bitKeepAlive	0x20
#define bitAck		0x40
#define bitCommand	0x80

typedef struct {
  unsigned short msgid;
  unsigned char stato;
  unsigned short impianto;
  unsigned char nodo;
  unsigned int datetime;
  unsigned char len;
  unsigned char estensione;
  unsigned char evento[256];
} __attribute__ ((packed)) UDP_event;

typedef struct {
  UDP_event event;
  int len;
  int crypt;
} UDP_Msg;

static int udp_datetime(unsigned char *d)
{
  struct tm tm;
  time_t t;
  
  memset(&tm, 0, sizeof(struct tm));
  tm.tm_mday = d[0];
  tm.tm_mon = d[1] - 1;
  tm.tm_year = d[2] + 100;
  tm.tm_hour = d[3];
  tm.tm_min = d[4] & 0x3f;
  tm.tm_sec = d[5];
  t = mktime(&tm) - BASESECONDS;
  return t;
}

#ifdef GEST16MM
static char udp_event_translate[] = {
/*
  0, 0, 1, 1, 0, 0, 1, 1, 1, 1,	// 150
  1, 0, 0, 0, 0, 1, 1, 1, 1, 1,	// 160
  2, 1, 1, 1, 1, 1, 1, 1, 0, 1,	// 170
  1, 0, 0, 0, 1, 1, 0, 0, 1, 0,	// 180
  1, 1, 1, 1, 0, 0, 1, 0, 1, 0,	// 190
  0, 1, 0, 0, 1, 1, 0, 3, 3, 3,	// 200
  1, 1, 1, 0, 0, 0, 0, 0, 1, 0,	// 210
  0, 0, 0, 0, 0, 0, 0, 1	// 220
*/
  0, 0, 0, 0, 0, 0, 0, 0, 1, 1,	// 150
  1, 0, 0, 0, 0, 1, 1, 0, 0, 0,	// 160
  2, 0, 0, 0, 0, 1, 1, 1, 0, 0,	// 170
  1, 0, 0, 0, 1, 1, 0, 0, 0, 0,	// 180
  0, 1, 1, 1, 0, 0, 0, 0, 1, 0,	// 190
  0, 1, 0, 0, 1, 1, 0, 3, 3, 3,	// 200
  1, 1, 1, 0, 0, 0, 0, 0, 1, 0,	// 210
  0, 0, 0, 0, 0, 0, 0, 1, 4	// 220
};
#else
static char udp_event_translate[] = {
};
#endif

static void udp_event(unsigned char *uev, unsigned char *ev, int evlen, int *uevlen)
{
  int i;
  
  if(ev[0] == 247)
  {
    memcpy(uev, ev, evlen - 8);
    *uevlen = evlen + 2;
  }
  else if(ev[0] == 253)
  {
    *uevlen = -1;
  }
  else
  {
    uev[0] = 247;
    uev[1] = 1;
    uev[2] = ev[0] - 148;	// 150 - 2
    
    switch(udp_event_translate[ev[0]-150])
    {
      case 1:
        /* nessuna modifica */
        memcpy(uev + 3, ev+1, evlen - 9);
        *uevlen = evlen + 1+3;
        break;
        
      case 2:
        /* da b a ww */
        *(unsigned short*)(uev+3) = ev[1];
        memcpy(uev+5, ev+2, evlen - 9);
        *uevlen = evlen + 2+3;
        break;
        
      case 3:
        /* da b a ww secondo parametro */
        *(unsigned short*)(uev+3) = bswap_16(*(unsigned short*)(ev+1));
        *(unsigned short*)(uev+5) = ev[3];
        /* 25-11-2009 */
        //*uevlen = evlen + 2+3;
        /* In realtà viene protato dietro un errore concettuale di fondo:
           gli eventi 207-208-209 sono generati da INSTOR, quindi nel vecchio
           formato con numero di periferica su un solo byte. Ma il saet.new
           gestisce questo evento come se la periferica fosse indicata su
           due byte, ma il secondo non lo genera nessuno (vedi INSTOR).
           Al momento la cosa più corretta da fare è lasciare tutto com'è
           ed applicando la correzione nel plugin UDP, ma la cosa dovrà
           essere messa a posto se si vorranno gestire questi eventi su
           impianti con indirizzo di modulo master maggiore o uguale a 8! */
        *uevlen = evlen + 1+3;
        break;
        
      case 4:
        /* valori analogici 8 -> 16 bit */
        *(unsigned short*)(uev+3) = bswap_16(*(unsigned short*)(ev+1));
        memset(uev+5, 0, 16);
        for(i=0; i<8; i++)
          uev[5+i*2] = ev[3+i];
        *uevlen = evlen + 1+3+8;
        break;
        
      default:
        *(unsigned short*)(uev+3) = bswap_16(*(unsigned short*)(ev+1));
        memcpy(uev+5, ev+3, evlen - 9);
        *uevlen = evlen + 1+3;
        break;
    }
  }
}

static const struct {
  char *cmd;
  int filter;
} udp_translate[] = {
  // 0
  {NULL, 0}, {"A", 1}, {"B", 1}, {"C", 1}, {"D", 1},
  {"G", 0}, {"H", 0}, {"I", 2}, {"J", 2}, {"K", 1},
  // 10
  {"L", 0}, {"M", 1}, {"N", 0}, {"O", 0}, {"P", 9},
  {"Q", 9}, {"R", 3}, {"U0", 4}, {"V", 0}, {"W", 4},
  // 20
  {"Z", 5}, {">", 0}, {"/35", 0}, {"/36", 0}, {"/37", 0},
  {"/380", 0}, {"/381", 6}, {"/382", 6}, {"/383", 6}, {"/389", 0},
  // 30
  {"/52", 10}, {"/53", 10}, {"/54", 11}, {"/7", 8}, {"/aR", 12},
  {"/aW", 13}, {NULL, 0}, {NULL, 0}, {NULL, 0}, {NULL, 0},
  // 40
  {"a", 0}, {"b", 0}, {"c", 0}, {"d", 0}, {"e", 0},
  {"g", 0}, {"k", 0}, {"n", 0}, {"t", 0}, {"u", 0},
  // 50
  {"v", 0}, {"w", 0}, {"Y", 7}, {"/550", 14}, {"/551", 14},
  {"/552", 14}, {"/560", 15}, {"/561", 16}, {"/562", 17}, {"/563", 18},
  // 60
  {"/AGS", 14}, {"/AMS", 14}, {"/MR", 0}, {NULL, 0}, {NULL, 0},
  {NULL, 0}, {NULL, 0}, {NULL, 0}, {NULL, 0}, {"x", 0},
  // 70
  {"/ML", 19}, {NULL, 0}, {NULL, 0}, {NULL, 0}, {NULL, 0},
  {NULL, 0}, {NULL, 0}, {NULL, 0}, {NULL, 0}, {NULL, 0},
};

static int udp_parse_cmd(unsigned char *cmd, int len, ProtDevice *dev)
{
  unsigned char command[256];
  int newlen = 0;
  
  switch(cmd[0])
  {
    case 0:	// Lara/Tebe
      command[0] = '/';
      command[1] = 'L';
      command[2] = 'a' - 1 + cmd[1];
      memcpy(command+3, cmd+2, len-2);
      newlen = len+1;
      break;
    case 1:	// Delphi
      if(cmd[1] < (sizeof(udp_translate)/(sizeof(udp_translate[0]))))
      {
        if(udp_translate[cmd[1]].cmd)
        {
          sprintf(command, "%s", udp_translate[cmd[1]].cmd);
          newlen = strlen(command);
          switch(udp_translate[cmd[1]].filter)
          {
            case 1:
              if(len != 4) return 0;
              sprintf(command + newlen, "%04d", *(unsigned short*)(cmd+2));
              newlen += 5;
              break;
            case 2:
              if(len != 3) return 0;
              sprintf(command + newlen, "%03d", cmd[2]);
              newlen += 4;
              break;
            case 3:
              if(len != 4) return 0;
              command[newlen] = cmd[2] + '0';
              sprintf(command + newlen+1, "%02d", cmd[3]);
              newlen += 4;
              break;
            case 4:
              if(len != 5) return 0;
              sprintf(command + newlen, "%02d%02d%02d", cmd[2], cmd[3], cmd[4]);
              newlen += 7;
              break;
            case 5:
              if(len != 12) return 0;
              memcpy(command + newlen, cmd+2, 10);
              newlen += 10;
              break;
            case 6:
              if(len != 4) return 0;
              memcpy(command + newlen, cmd+2, 2);
              newlen += 2;
              break;
            case 7:
              if(len != 4) return 0;
              sprintf(command + newlen, "%d%03d", cmd[2], cmd[3]);
              newlen += 5;
              break;
            case 8:
              if(len != 6) return 0;
              sprintf(command + newlen, "%02d%d%02d%02d", cmd[2], cmd[3], cmd[4], cmd[5]);
              newlen += 8;
              break;
            case 9:
              if(len != 5) return 0;
              sprintf(command + newlen, "%04d", *(unsigned short*)(cmd+2));
              command[newlen+2] = cmd[4];
              newlen += 5;
              break;
            case 10:
              if(len != 5) return 0;
              command[newlen++] = cmd[2] + '0';
/*
              sprintf(command + newlen+1, "%04d", *(unsigned short*)(cmd+3));
              newlen += 6;
*/
              command[newlen++] = cmd[3];
              command[newlen++] = cmd[4];
              break;
            case 11:
              if(len != 34) return 0;
              command[newlen] = '0';
              memcpy(command+newlen+1, cmd+2, 16);
              newlen += 17;
              codec_parse_cmd(command, newlen, dev);
              command[newlen] = '1';
              memcpy(command+newlen+1, cmd+18, 16);
              break;
            case 12:
              if(len != 4) return 0;
              sprintf(command + newlen, "%03d", *(unsigned short*)(cmd+2));
              newlen += 4;
              break;
            case 13:
              if(len != 10) return 0;
              sprintf(command + newlen, "%03d", *(unsigned short*)(cmd+2));
              memcpy(command+newlen+3, cmd+4, 6);
              newlen += 9;
              break;
            case 14:
              if(len != 3) return 0;
              command[newlen] = cmd[2] + '0';
              newlen += 1;
              break;
            case 15:
              if(len != 7) return 0;
              command[newlen] = cmd[2] + '0';
              memcpy(command+newlen+1, cmd+3, 4);
              newlen += 5;
              break;
            case 16:
              if(len != 16) return 0;
              command[newlen] = cmd[2] + '0';
              memcpy(command+newlen+1, cmd+3, 13);
              newlen += 14;
              break;
            case 17:
              if(len != 8) return 0;
              command[newlen] = cmd[2] + '0';
              memcpy(command+newlen+1, cmd+3, 5);
              newlen += 6;
              break;
            case 18:
              if(len != 7) return 0;
              memcpy(command+newlen, cmd+2, 5);
              newlen += 5;
              break;
            case 19:
              if(len > 198) return 0;
              memcpy(command+newlen, cmd+2, len-2);
              newlen += len-2;
              break;
            default:
              break;
          }
        }
        else
          newlen = 0;
      }
      break;
    default:
      break;
  }
  
  if(newlen)
    return codec_parse_cmd(command, newlen, dev);
  else
    return 0;
}

static void udp_timeout(void *connto, int null)
{
  *(int*)connto = 1;
}

static int udp_decrypt(struct aes_ctx *ctx, UDP_Msg *msg)
{
  int i;
  unsigned char *data;
  
  data = (unsigned char*)&msg->event;
  msg->crypt = 0;
  
  if(msg->len & 0x0f) return 0;	// deve essere un multiplo di 16
  for(i=0; i<msg->len; i+=16) aes_decrypt(ctx, data+i, data+i);
  return 1;
}

static void udp_encrypt(struct aes_ctx *ctx, UDP_Msg *msg)
{
  int i;
  unsigned char *data;
  
  data = (unsigned char*)&msg->event;
  msg->crypt = 1;
  
  i = msg->len;
  msg->len += 15;
  msg->len = (msg->len & ~0x0f);
  for(; i<msg->len; i++) data[i] = 0;
  
  for(i=0; i<msg->len; i+=16) aes_encrypt(ctx, data+i, data+i);
}

static int udp_check_msg(UDP_event *msg, int *res, unsigned short *msgid)
{
  unsigned char stato;
  
  /* Verifica la dimensione minima del pacchetto. */
  if(*res < sizeof(UDP_event)-256) return 0;
  
  if(msg->len > *res) return 0;
  *res = msg->len;
  
  /* Il campo nodo è attualmente inutilizzato, per default vale 0
     per le centrali ISI, vale 1 per le centrali Delphi. */
  if(msg->nodo > 1) return 0;
  
  /* Il campo estensione è 0 se protocollo
     vecchio, 1 se protocollo nuovo. */
  stato = msg->stato;
  if(!(stato & bitKeepAlive) && (msg->estensione != 1)) return 0;
  
  if(stato & bitConnect)
  {
    /* Poichè questo è il codice per la centrale, posso ricevere
       solo i due comandi per le fasi 1 e 3, con bitCommand
       necessariamente impostato.
       Non devono essere presenti altri bit, a meno dell'Ack.
       In caso di Ack, possono essere solo relativi alle fasi 2 e 4,
       senza bitCommand impostato. */
    /* Nei messaggi di connessione non eseguo il controllo sul msgid
       per permettere la connessione con i contatori sicuramente
       disallienati. */
    if(!(stato & bitAck))
    {
      if(!(stato & bitCommand) || (stato & ~(bitConnect|bitCommand|bitResend)) || !msg->len)
        return 0;
      if((msg->evento[0] != 1 /*UDP_REQUEST*/) && (msg->evento[0] != 3 /*UDP_SECRET_SET*/))
        return 0;
      if((msg->evento[0] == 1 /*UDP_REQUEST*/)  && (msg->len != 5+sizeof(UDP_event)-256))
        return 0;
      if((msg->evento[0] == 3 /*UDP_SECRET_SET*/)  && (msg->len != 25+sizeof(UDP_event)-256))
        return 0;
    }
    else
    {
      if((stato & bitCommand) || (stato & ~(bitConnect|bitAck|bitResend)) || !msg->len)
        return 0;
      if((msg->evento[0] != 2 /*UDP_CONFIRM*/) && (msg->evento[0] != 4 /*UDP_SECRET_CONF*/))
        return 0;
      if((msg->evento[0] == 2 /*UDP_CONFIRM*/)  && (msg->len != 5+sizeof(UDP_event)-256))
        return 0;
      if((msg->evento[0] == 4 /*UDP_SECRET_CONF*/)  && (msg->len != 5+sizeof(UDP_event)-256))
        return 0;
    }
  }
  else
  {
    /* Per i messaggi di Ack, il controllo sul msgid avviene nel loop principale
       ed è riferito al contatore 'out', non al contatore 'in'. */
    if(!(stato & bitAck))
    {
      if(msg->msgid != *msgid)
      {
        if(stato & bitKeepAlive)
        {
          /* Il keepalive riceve l'ack anche se è fuori sequenza,
             ma non modifica il contatore 'in' atteso.
             Imposta però il bit Resend per segnalare la necessità
             di un riallineamento attraverso una procedura di
             connessione da parte del supervisore. */
          msg->stato |= bitResend;
          return 1;
        }
        else if((stato & bitResend) && (msg->msgid == (*msgid-1)))
        {
          /* Se ricevo un messaggio con il bit Resend e contatore
             pari all'ultimo messaggio ricevuto, significa che il
             messaggio di Ack è andato perso e lo devo reinviare,
             ma il messaggio era già stato trattato. */
          return 1;
        }
        else
          return 0;
      }
// GC 20080509
      else if(stato & bitKeepAlive)
      {
        /* L'Ack non deve avere il bit di resend impostato se non per forzare il riallineamento. */
        msg->stato &= ~bitResend;
      }
      (*msgid)++;
    }
  }
  return 1;
}

static void udp_loop(ProtDevice *dev)
{
  Event ev;
  
// GC 20080305
  UDP_Msg umi, umo;
  
  int numResend = 0;
  int res, fdi, fdo, salen, tempo_disconn, port_s;
  int peer = 0, secret = 0, forcealign = 1;
  int connto = 0;
  struct sockaddr_in sa, safrom;
  struct in_addr fpeer;
  char ipconf[20];

  unsigned short msgid_in = 0;
  unsigned short msgid_out = 0;
  fd_set fds;
  struct timeval tv;
  timeout_t *udpto;
  char key[32], keylen = 0;
  struct aes_ctx aesctx;
  int fdkey;
  char keyfile[32];
  
  enum {UDP_NULL = 0, UDP_REQUEST, UDP_CONFIRM, UDP_SECRET_SET, UDP_SECRET_CONF, UDP_CONNECTED} udpstate = UDP_NULL;

  if(!dev) return;
  if(!config.consumer[dev->consumer].param ||
     (config.consumer[dev->consumer].configured != 5)) return;
  
  pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
  
  support_log("UDP Init");
// GC 20080508
  while(!libuser_started) sleep(1);
  
// GC 20080305
  umo.len = -1;
  
  fdi = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if(!fdi) return;
  fdo = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if(!fdo) return;
  
//  tempo_disconn = atoi(config.consumer[dev->consumer].param) * 10;
  tempo_disconn = 0;
  port_s = 0;
  ipconf[0] = 0;
  fpeer.s_addr = 0;
  sscanf(config.consumer[dev->consumer].param, "%d:%d:%s", &tempo_disconn, &port_s, ipconf);
  tempo_disconn *= 10;
  if(ipconf[0])
  {
    if(!inet_aton(ipconf, &fpeer))
      fpeer.s_addr = 0;
  }
  
  udpto = timeout_init();
  
  sa.sin_family = AF_INET;
  sa.sin_port = htons(config.consumer[dev->consumer].data.eth.port);
  sa.sin_addr.s_addr = INADDR_ANY;
  res = bind(fdi, (struct sockaddr*)&sa, sizeof(struct sockaddr_in));
  
  /* Imposto la porta del supervisore, può essere diversa da quella locale */
  if(port_s) sa.sin_port = htons(port_s);
  
  sprintf(keyfile, "/saet/udp%d.key", dev->consumer);
  fdkey = open(keyfile, O_RDONLY);
  if(fdkey >= 0)
  {
    read(fdkey, &peer, 4);
    keylen = read(fdkey, key, 32);
    close(fdkey);
    aes_set_key(&aesctx, key, keylen);
    udpstate = UDP_CONNECTED;
    sa.sin_addr.s_addr = peer;
    connect(fdo, (struct sockaddr*)&sa, sizeof(struct sockaddr));
    if(tempo_disconn) timeout_on(udpto, udp_timeout, &connto, 0, tempo_disconn);
  }
  
  tv.tv_sec = 0;
  tv.tv_usec = 100000;
  
  while(1)
  {
    FD_ZERO(&fds);
    FD_SET(fdi, &fds);
    res = select(fdi+1, &fds, NULL, NULL, &tv);
    
    if(connto)
    {
      connto = 0;
      peer = 0;
      
// GC 20071219
      /* Se avevo un messaggio in canna ma mi scade la connessione, il messaggio
         contenuto in umo.event che era criptato, deve essere decifrato per la corretta
         gestione dei reinvii. */
      if(keylen && (umo.len > 0))
      {
        udp_decrypt(&aesctx, &umo);
        umo.len = umo.event.len;
      }
      
      keylen = 0;
      udpstate = UDP_NULL;
    }
    
    if(res)
    {
      salen = sizeof(struct sockaddr_in);
// GC 20080305
      umi.len = recvfrom(fdi, &umi.event, sizeof(UDP_event), 0, (struct sockaddr*)&safrom, &salen);
      umi.crypt = 0;
#ifdef DEBUG
printf("\nrecv=%d peer=%08x auth=%d\n",umi.len,peer,keylen);
#endif
#ifdef DEBUG
{
char cmd[256];
int x,len;
struct timeval tv;
gettimeofday(&tv, NULL);
sprintf(cmd, "%d.%03d RX:", tv.tv_sec, tv.tv_usec/1000);
len = strlen(cmd);
for(x=0; x<umi.len; x++) sprintf(cmd+len+(x*2), "%02x", ((unsigned char*)&umi.event)[x]);
//support_log(cmd);
printf("%s\n", cmd);
}
#endif
      /* controlla indirizzo peer */
//      if(peer && (udpstate != UDP_CONNECTED) && (safrom.sin_addr.s_addr != peer)) continue;
      if(keylen && !udp_decrypt(&aesctx, &umi)) continue;
#ifdef DEBUG
{
char cmd[256];
int x,len;
struct timeval tv;
gettimeofday(&tv, NULL);
sprintf(cmd, "%d.%03d RX:", tv.tv_sec, tv.tv_usec/1000);
len = strlen(cmd);
for(x=0; x<umi.len; x++) sprintf(cmd+len+(x*2), "%02x", ((unsigned char*)&umi.event)[x]);
//support_log(cmd);
printf("%s\n", cmd);
}
#endif
      if(umi.event.impianto != config.DeviceID) continue;
//      if(res > umi.event.len) res = umi.event.len;
#ifdef DEBUG
printf("Atteso %d\n", msgid_in);
#endif
// GC 20080305
      if(!udp_check_msg(&umi.event, &umi.len, &msgid_in)) continue;
#ifdef DEBUG
printf("Valido.\n");
printf("  msgid=%d(i:%d,o:%d) stato: %s %s %s %s %s", umi.event.msgid, msgid_in, msgid_out,
          umi.event.stato&bitConnect?"Connect ":"", umi.event.stato&bitKeepAlive?"KeepAlive ":"",
          umi.event.stato&bitCommand?"Command ":"", umi.event.stato&bitAck?"Ack ":"", umi.event.stato&bitResend?"Resend ":"");      
#endif
      if(tempo_disconn) timeout_on(udpto, udp_timeout, &connto, 0, tempo_disconn);
      
#ifdef DEBUG
printf("STATO %d\n", udpstate);
#endif
      switch(udpstate)
      {
        case UDP_NULL:
          /* Gestione LIN come per protocollo SaetNet. */
          LIN[dev->consumer] = 0;
          
// GC 20071219
          /* Se avevo un messaggio in canna ma mi scade la connessione, il messaggio
             contenuto in umo.event che era criptato, deve essere decifrato per la corretta
             gestione dei reinvii. */
          if(keylen && umo.crypt && (umo.len > 0))
          {
            udp_decrypt(&aesctx, &umo);
            umo.len = umo.event.len;
          }
          
          keylen = 0;
// GC 20080305
          if((umi.event.stato & bitConnect) && (umi.event.evento[0] == UDP_REQUEST))
          {
            safrom.sin_port = sa.sin_port;
            if(fpeer.s_addr) safrom.sin_addr = fpeer;
            umi.event.stato |= bitAck;
            umi.event.datetime = time(NULL) - BASESECONDS;
            umi.event.estensione = 1;
            umi.event.len = sizeof(UDP_event)-251;
#ifdef DEBUG
{
int x;
printf("TX:");
  for(x=0; x<umi.event.len; x++) printf(" %02x", ((unsigned char*)&umi.event)[x]);
printf("\n");
}
#endif
            sendto(fdo, &umi.event, umi.event.len, 0, (struct sockaddr*)&safrom, sizeof(struct sockaddr_in));
            umi.event.msgid = msgid_out;
            umi.event.stato = /*bitCommand |*/ bitConnect;
            umi.event.evento[0] = UDP_CONFIRM;
            srand(time(NULL));
            do
              secret = rand();
            while(!secret);
            *(int*)(umi.event.evento+1) = secret;
            umi.event.len = sizeof(UDP_event)-251;
#ifdef DEBUG
{
int x;
printf("TX:");
  for(x=0; x<umi.event.len; x++) printf(" %02x", ((unsigned char*)&umi.event)[x]);
printf("\n");
}
#endif
            sendto(fdo, &umi.event, umi.event.len, 0, (struct sockaddr*)&safrom, sizeof(struct sockaddr_in));
            udpstate = UDP_CONFIRM;
          }
          else if(umi.event.stato & bitKeepAlive)
          {
            break;
          }
          continue;
        case UDP_REQUEST:
          break;
        case UDP_CONFIRM:
// GC 20080305
          if((umi.event.stato & bitAck) && (umi.event.msgid == msgid_out) &&
             (umi.event.evento[0] == UDP_CONFIRM) && secret && (*(int*)(umi.event.evento+1) == secret))
          {
            /* A connessione avvenuta, azzero i contatori
               per allineamento verso il supervisore. */
            msgid_in = 0;
            msgid_out = 0;
            forcealign = 0;
            udpstate = UDP_CONNECTED;
//            peer = safrom.sin_addr.s_addr;
            if(fpeer.s_addr)
              peer = fpeer.s_addr;
            else
              peer = safrom.sin_addr.s_addr;
            sa.sin_addr.s_addr = peer;
            connect(fdo, (struct sockaddr*)&sa, sizeof(struct sockaddr));
            fdkey = open(keyfile, O_RDWR|O_CREAT);
            if((read(fdkey, &res, 4) < 4) || (res != peer))
            {
              lseek(fdkey, 0, SEEK_SET);
              write(fdkey, &peer, 4);
              if(keylen) write(fdkey, key, keylen);
            }
            close(fdkey);
          }
          else
          {
            if(peer)
              udpstate = UDP_CONNECTED;
            else
              udpstate = UDP_NULL;
          }
          secret = 0;
          continue;
        case UDP_SECRET_SET:
          break;
        case UDP_SECRET_CONF:
          if(peer && (safrom.sin_addr.s_addr != peer)) continue;
          
// GC 20080305
          if((umi.event.stato & bitAck) && (umi.event.msgid == msgid_out) &&
             (umi.event.evento[0] == UDP_SECRET_CONF) && secret && (*(int*)(umi.event.evento+1) == secret))
          {
            msgid_out++;
            udpstate = UDP_CONNECTED;
            fdkey = open(keyfile, O_RDWR|O_CREAT);
            write(fdkey, &peer, 4);
            write(fdkey, key, 24);
            close(fdkey);
            
// GC 20080305
            /* Se ho un evento già in canna e cambio la chiave, devo prima decrittare
               con la vecchia chiave, altrimenti l'evento diventa irrecuperabile. */
            if(keylen && umo.crypt && (umo.len > 0))
            {
              udp_decrypt(&aesctx, &umo);
              umo.len = umo.event.len;
            }
            
            aes_set_key(&aesctx, key, 24);
            keylen = 24;
          }
          else
          {
/*
// GC 20070213
            if(keylen)
*/
// GC 20071213
            if(peer)
              udpstate = UDP_CONNECTED;
            else
              udpstate = UDP_NULL;
          }
          secret = 0;
          continue;
        case UDP_CONNECTED:
          /* Gestione LIN come per protocollo SaetNet. */
          LIN[dev->consumer] = bitAlarm;
          
#if 0
          /* Controlla indirizzo peer. Se il supervisore era connesso con crittografia
             permetto che rimanga connesso anche se cambia IP (ho riconosciuto la chiave)
             per gestire il backup immediato. */
             
          if(peer && (safrom.sin_addr.s_addr != peer))
          {
            if(keylen)
            {
              peer = safrom.sin_addr.s_addr;
              sa.sin_addr.s_addr = peer;
              connect(fdo, (struct sockaddr*)&sa, sizeof(struct sockaddr));
            }
            else
              continue;
          }
#endif
          /* I messaggi da un IP differente vengono accettati solo se si
             tratta di KeepAlive o richieste di connessione.
             In quest'ultimo caso parte la
             procedura normale di registrazione che in seguito assegna
             il valore corretto alla variabile 'peer'. */
// GC 20080305
          if(umi.event.stato & bitKeepAlive) break;
          
          if(peer && (safrom.sin_addr.s_addr != peer) &&
             (!(umi.event.stato & bitConnect) || (umi.event.evento[0] != UDP_REQUEST)))
            continue;
          
          if(umi.event.stato & bitConnect)
          {
            if(umi.event.evento[0] == UDP_SECRET_SET)
            {
              memcpy(key, umi.event.evento+1, 24);
              if(fpeer.s_addr) safrom.sin_addr = fpeer;
              safrom.sin_port = sa.sin_port;
              umi.event.stato |= bitAck;
              umi.event.datetime = time(NULL) - BASESECONDS;
              umi.event.estensione = 1;
              umi.len = umi.event.len = sizeof(UDP_event)-256;
#ifdef DEBUG
{
int x;
printf("TX:");
  for(x=0; x<umi.event.len; x++) printf(" %02x", ((unsigned char*)&umi.event)[x]);
printf("\n");
}
#endif
              if(keylen) udp_encrypt(&aesctx, &umi);
              sendto(fdo, &umi.event, umi.len, 0, (struct sockaddr*)&safrom, sizeof(struct sockaddr_in));
              umi.event.msgid = msgid_out;
              umi.event.stato = /*bitCommand |*/ bitConnect;
              umi.event.impianto = config.DeviceID;
              umi.event.nodo = 0;
              umi.event.estensione = 1;
              umi.event.evento[0] = UDP_SECRET_CONF;
              /* Associa alla conferma un valore casuale che deve essere confermato
                 dall'Ack. Si assicura che questo valore casuale sia diverso da 0. */
              do
                secret = rand();
              while(!secret);
              *(int*)(umi.event.evento+1) = secret;
              umi.len = umi.event.len = sizeof(UDP_event)-251;
#ifdef DEBUG
{
int x;
printf("TX:");
  for(x=0; x<umi.event.len; x++) printf(" %02x", ((unsigned char*)&umi.event)[x]);
printf("\n");
}
#endif
              if(keylen) udp_encrypt(&aesctx, &umi);
              sendto(fdo, &umi.event, umi.len, 0, (struct sockaddr*)&safrom, sizeof(struct sockaddr_in));
              udpstate = UDP_SECRET_CONF;
            }
            else if(umi.event.evento[0] == UDP_REQUEST)
            {
              if(fpeer.s_addr) safrom.sin_addr = fpeer;
              safrom.sin_port = sa.sin_port;
              umi.event.stato |= bitAck;
              umi.event.datetime = time(NULL) - BASESECONDS;
              umi.event.estensione = 1;
              umi.len = umi.event.len = sizeof(UDP_event)-251;
#ifdef DEBUG
{
int x;
printf("TX:");
  for(x=0; x<umi.event.len; x++) printf(" %02x", ((unsigned char*)&umi.event)[x]);
printf("\n");
}
#endif
              if(keylen) udp_encrypt(&aesctx, &umi);
              sendto(fdo, &umi.event, umi.len, 0, (struct sockaddr*)&safrom, sizeof(struct sockaddr_in));
              umi.event.msgid = msgid_out;
              umi.event.impianto = config.DeviceID;
              umi.event.nodo = 0;
              umi.event.estensione = 1;
              umi.event.stato = /*bitCommand |*/ bitConnect;
              umi.event.evento[0] = UDP_CONFIRM;
              /* Associa alla conferma un valore casuale che deve essere confermato
                 dall'Ack. Si assicura che questo valore casuale sia diverso da 0. */
              srand(time(NULL));
              do
                secret = rand();
              while(!secret);
              *(int*)(umi.event.evento+1) = secret;
              umi.len = umi.event.len = sizeof(UDP_event)-251;
#ifdef DEBUG
{
int x;
printf("TX:");
  for(x=0; x<umi.event.len; x++) printf(" %02x", ((unsigned char*)&umi.event)[x]);
printf("\n");
}
#endif
              if(keylen) udp_encrypt(&aesctx, &umi);
              sendto(fdo, &umi.event, umi.len, 0, (struct sockaddr*)&safrom, sizeof(struct sockaddr_in));
              udpstate = UDP_CONFIRM;
            }
            continue;
          }
          break;
      }
      
#ifdef DEBUG
printf("Gestione generale\n");
if(umi.event.stato & bitAck) printf("ACK (len=%d, %02x=%02x)\n", umo.len, msgid_out, umi.event.msgid);
#endif
      if((umi.event.stato & bitAck) && (umo.len > 0) && (umi.event.msgid == msgid_out))
      {
        msgid_out++;
        umo.len = -1;
        numResend = 0;
      }
      else if((umi.len > (sizeof(UDP_event)-256)) && (umi.event.stato & bitCommand))
      {
        /* Gestisco il comando solo se non si tratta di un resend già gestito.
           A questo punto il contatore msgid_in è già stato incrementato, quindi
           devo fare il controllo sul valore precedente. */
        if(!(umi.event.stato & bitResend) || (umi.event.msgid == (msgid_in-1)))
          udp_parse_cmd(umi.event.evento, umi.len - (sizeof(UDP_event)-256), dev);
        /* L'Ack invece lo rimando in ogni caso. */
        umi.event.stato |= bitAck;
        umi.event.datetime = time(NULL) - BASESECONDS;
        umi.event.estensione = 1;
//        umi.event.impianto = config.DeviceID;
        umi.event.len = umi.len = sizeof(UDP_event)-256;
        if(keylen) udp_encrypt(&aesctx, &umi);
        write(fdo, &umi.event, umi.len);
      }
      else if(umi.event.stato & bitKeepAlive)
      {
        umi.event.stato |= bitAck;
// GC 20080318
/* L'Ack non deve avere il bit di resend impostato se non per forzare il riallineamento. */
// GC 20080509
//        umi.event.stato &= ~bitResend;
        
        if(forcealign) umi.event.stato |= bitResend;
        umi.event.datetime = time(NULL) - BASESECONDS;
        umi.event.estensione = 1;
//        umi.event.impianto = config.DeviceID;
        umi.event.len = umi.len = sizeof(UDP_event)-256;
#ifdef DEBUG
{
int x;
printf("TX:");
  for(x=0; x<umi.len; x++) printf(" %02x", ((unsigned char*)&umi.event)[x]);
printf("\n");
}
#endif
        if(keylen) udp_encrypt(&aesctx, &umi);
//        write(fdo, &umi.event, res);
        if(fpeer.s_addr) safrom.sin_addr = fpeer;
        safrom.sin_port = sa.sin_port;
        sendto(fdo, &umi.event, umi.len, 0, (struct sockaddr*)&safrom, sizeof(struct sockaddr_in));
      }
    }
    else if(umo.len >= 0)
    {
      if(numResend < 3)
        numResend++;
      else
        forcealign = 1;
      
      if(peer)
      {
        /* reinvia */
        if(keylen && umo.crypt) udp_decrypt(&aesctx, &umo);
        umo.event.stato |= bitResend;
        /* Riallinea il contatore se nel frattempo sono
           stati gestiti altri messaggi. */
        umo.event.msgid = msgid_out;
        umo.event.impianto = config.DeviceID;
#ifdef DEBUG
{
int x;
printf("TX:");
for(x=0; x<umo.event.len; x++) printf(" %02x", ((unsigned char*)&umo.event)[x]);
printf("\n");
}
#endif
        if(keylen) udp_encrypt(&aesctx, &umo);
        write(fdo, &umo.event, umo.len);
        tv.tv_sec = 1;
        tv.tv_usec = 0;
        continue;
      }
// GC 20080325
      else
      {
        tv.tv_sec = 1;
        tv.tv_usec = 0;
      }
    }
    
    if((umo.len < 0) && peer)
    {
      while(!(res = codec_get_event(&ev, dev)));
      if(res > 0)
      {
        umo.crypt = 0;
        umo.event.msgid = msgid_out;
        umo.event.stato = 0;
        umo.event.impianto = config.DeviceID;
        umo.event.nodo = ev.NodeID;  // potrebbe cambiare per la presenza del plugin Tipo
        umo.event.datetime = udp_datetime(ev.Event + 2);
        umo.event.estensione = 1;
        udp_event(umo.event.evento, ev.Event + 8, ev.Len, &umo.len);
        if(umo.len > 0)
        {
          umo.event.len = umo.len;
#ifdef DEBUG
{
int x;
printf("TX:");
  for(x=0; x<umo.event.len; x++) printf(" %02x", ((unsigned char*)&umo.event)[x]);
printf("\n");
}
#endif
          if(keylen) udp_encrypt(&aesctx, &umo);
          write(fdo, &umo.event, umo.len);
          tv.tv_sec = 1;
          tv.tv_usec = 0;
        }
      }
      else
      {
        tv.tv_sec = 0;
        tv.tv_usec = 100000;
      }
    }
    else if(umo.len < 0)
    {
      tv.tv_sec = 1;
      tv.tv_usec = 0;
    }
  }
}

void _init()
{
  printf("UDP plugin: " __DATE__ " " __TIME__ " (v2.0.5)\n");
  prot_plugin_register("UDP", 0, NULL, NULL, (PthreadFunction)udp_loop);
  aes_gen_tabs();
}

