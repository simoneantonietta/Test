/*

Connessione in chiaro: una nuova connessione puo' avvenire solo dopo il timeout della precedente
Connessione crittata: una nuova connessione puo' avvenire anche da un nuovo IP (backup) se si riconosce il segreto impostato.

*/

#include "protocol.h"
#include "codec.h"
#include "support.h"
#include "timeout.h"
#include "master.h"
#include "delphi.h"
#include "tinyxml.h"
#include <unistd.h>
#include <pthread.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <netinet/ether.h>
#include <linux/if_packet.h>
#include <netdb.h>
#include <sys/time.h>
#include <byteswap.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <zlib.h>

#include "aes.c"

#define DEBUG

#define INTERFACE	"eth0"
#define BASESECONDS	631148400

#define bitResend	0x01
//#define bitAuth		0x04
#define bitConnect	0x08
#define bitKeepAlive	0x20
#define bitAck		0x40
#define bitCommand	0x80

#define UDP_TIMEOUT_LAN 1
#define UDP_TIMEOUT_BCK 5

enum UdpState {UDP_NULL = 0, UDP_REQUEST, UDP_CONFIRM, UDP_SECRET_SET, UDP_SECRET_CONF, UDP_CONNECTED,
    UDP_REQREG, UDP_CHALLENGE, UDP_REGISTER};

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

static const char cb64[]="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
static const char cd64[]="|$$$}rstuvwxyz{$$$$$$$>?@ABCDEFGHIJKLMNOPQRSTUVW$$$$$$XYZ[\\]^_`abcdefghijklmnopq";

static void encodeblock( unsigned char *in, unsigned char *out, int len )
{
    out[0] = (unsigned char) cb64[ (int)(in[0] >> 2) ];
    out[1] = (unsigned char) cb64[ (int)(((in[0] & 0x03) << 4) | ((in[1] & 0xf0) >> 4)) ];
    out[2] = (unsigned char) (len > 1 ? cb64[ (int)(((in[1] & 0x0f) << 2) | ((in[2] & 0xc0) >> 6)) ] : '=');
    out[3] = (unsigned char) (len > 2 ? cb64[ (int)(in[2] & 0x3f) ] : '=');
}

static char b64buf[64];

static char* b64enc(unsigned char *data, int len)
{
  int i;
  
  for(i=0; i<len; i+=3)
    encodeblock(data+i, b64buf+(i/3)*4, (i<(len-3))?3:len-i);
  b64buf[(i/3)*4] = 0;
  return b64buf;
}

static void decodeblock( unsigned char *in, unsigned char *out )
{   
    out[ 0 ] = (unsigned char ) (in[0] << 2 | in[1] >> 4);
    out[ 1 ] = (unsigned char ) (in[1] << 4 | in[2] >> 2);
    out[ 2 ] = (unsigned char ) (((in[2] << 6) & 0xc0) | in[3]);
}

static int b64dec(const char *in, unsigned char *out)
{
  int len, i, j, k;
  unsigned char tmp[4];
  
  len = strlen(in);
  j = 0;
  for(i=0; i<len; i++)
  {
    tmp[j++] = ((in[i]<43||in[i]>122)?0:(int)cd64[in[i]-43]-61-1);
    if(j == 4)
    {
      j = 0;
      decodeblock(tmp, out+(i/4)*3);
    }
  }
  if(j)
  {
    for(k=j; k<4; k++) tmp[k] = 0;
    decodeblock(tmp, out+(i/4)*3);
  }
  len = ((len / 4) * 3) + j;
  
  return len;
}

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

static void udp_keepalive(void *keepalive, int null)
{
  *(int*)keepalive = 1;
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
      if((msg->evento[0] != 1 /*UDP_REQUEST*/) && (msg->evento[0] != 3 /*UDP_SECRET_SET*/) &&
         (msg->evento[0] != 7 /*UDP_CHALLENGE*/))
        return 0;
      if((msg->evento[0] == 1 /*UDP_REQUEST*/)  && (msg->len != 5+sizeof(UDP_event)-256))
        return 0;
      if((msg->evento[0] == 3 /*UDP_SECRET_SET*/)  && (msg->len != 25+sizeof(UDP_event)-256))
        return 0;
      if((msg->evento[0] == 7 /*UDP_CHALLENGE*/)  && (msg->len != 17+16+sizeof(UDP_event)-256))
        return 0;
    }
    else
    {
      if((stato & bitCommand) || (stato & ~(bitConnect|bitAck|bitResend)) || !msg->len)
        return 0;
      if((msg->evento[0] != 2 /*UDP_CONFIRM*/) && (msg->evento[0] != 4 /*UDP_SECRET_CONF*/) &&
         (msg->evento[0] != 6 /*UDP_REQREG*/) && (msg->evento[0] != 8 /*UDP_REGISTER*/))
        return 0;
      if((msg->evento[0] == 2 /*UDP_CONFIRM*/)  && (msg->len != 5+sizeof(UDP_event)-256))
        return 0;
      if((msg->evento[0] == 4 /*UDP_SECRET_CONF*/)  && (msg->len != 5+sizeof(UDP_event)-256))
        return 0;
      if((msg->evento[0] == 6 /*UDP_REQREG*/)  && (msg->len != 2+16+sizeof(UDP_event)-256))
        return 0;
      if((msg->evento[0] == 8 /*UDP_REGISTER*/)  && (msg->len != 17+sizeof(UDP_event)-256))
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

static unsigned short csum(unsigned short *buf, int nwords)
{
    unsigned long sum;
    for(sum=0; nwords>0; nwords--)
        sum += *buf++;
    sum = (sum >> 16) + (sum &0xffff);
    sum += (sum >> 16);
    return (unsigned short)(~sum);
}

static const unsigned char ZeroBuf[16] = {0, };
#define BUF_SIZ sizeof(struct ether_header)+sizeof(struct iphdr)+sizeof(struct udphdr)+30+6
/* Indici degli IP supervisore da utilizzare (0=corrente, 1=LAN, 2=backup PPP) */
#define SV_CUR	0
#define SV_LAN	1
#define SV_BCK	2
struct UdpConf {
  //int idx, Id, Port, Retries, Debug;
  int Port;
  int fdraw, eth0running;
  unsigned int IPhost, IPgw, IPsv[3];
  int lport;
  unsigned char MAChost[6], MACgw[6];
  unsigned char chall[16];
};

static void udp_eth0_gateway(struct UdpConf *udpconfig)
{
  struct ifreq ifr;
  //struct arpreq arpreq;
  //struct sockaddr_in *sin;
  unsigned int nm;
  
  if(udpconfig->eth0running)
  {
    /* Recupera l'IP del gateway */
    memset(&ifr, 0, sizeof(struct ifreq));
    strcpy(ifr.ifr_name, INTERFACE);
    ioctl(udpconfig->fdraw, SIOCGIFADDR, &ifr);
    udpconfig->IPhost = ((struct sockaddr_in*)(&ifr.ifr_addr))->sin_addr.s_addr;
    ioctl(udpconfig->fdraw, SIOCGIFNETMASK, &ifr);
    nm = ((struct sockaddr_in*)(&ifr.ifr_addr))->sin_addr.s_addr;
    
    if((udpconfig->IPhost & nm) == (udpconfig->IPsv[SV_LAN] & nm))
    {
      /* ISI e GEMSS sono sulla stessa sottorete. */
      /* Se sono sulla stessa sottorete, le informazioni sul GW
         coincidono con quelle del SV, visto che servono solo per
         il recupero del MAC. */
      if((udpconfig->IPsv[SV_LAN] & nm) == (udpconfig->IPsv[SV_BCK] & nm))
      {
        static int once = 0;
        if(!once)
        {
          once = 1;
          support_log("UDP: *****************************************");
          support_log("UDP: * Delphi e GEMSS sulla stessa sottorete *");
          support_log("UDP: * sia LAN che backup.                   *");
          support_log("UDP: *****************************************");
        }
      }
      
      udpconfig->IPgw = udpconfig->IPsv[SV_LAN];
    }
    else
    {
      FILE *fp;
      char route[256];
      udpconfig->IPgw = 0;
      
      fp = fopen("/tmp/defaultroute", "r");
      if(fp && fgets(route, 255, fp))
      {
        /* Il backup PPP è attivo e in questo file (non vuoto) ho salvato il default route per eth0 */
        udpconfig->IPgw = inet_addr(route);
        fclose(fp);
      }
      else
      {
        /* Recupero il default route per eth0 dalla routing table */
        fp = fopen("/proc/net/route", "r");
        while(fgets(route, 255, fp))
        {
          if(!strncmp(route, INTERFACE, 4))
          {
            sscanf(route, "%*s %*s %x", &udpconfig->IPgw);
            if(udpconfig->IPgw) break;
          }
        }
        fclose(fp);
      }
    }
    
    /* Recupera il MAC del gateway */
    if(udpconfig->IPgw)
    {
      FILE *fp;
      char arp[256];
      int flags, hex;
      
      memset(udpconfig->MACgw, 0, 6);
#if 0
/* La chiamata ioctl non funziona! */
printf("** Recupero MAC IP:%08x (%d)\n", udpconfig->IPgw, udpconfig->fdraw);
      memset(&arpreq, 0, sizeof(arpreq));
      sin = (struct sockaddr_in *) &arpreq.arp_pa;
      sin->sin_family = AF_INET;
      sin->sin_addr.s_addr = udpconfig->IPgw;
      
      ioctl(udpconfig->fdraw, SIOCGARP, &arpreq);
printf("ARP presente %x %x\n", arpreq.arp_flags, ATF_COM);
      if(arpreq.arp_flags & ATF_COM)
        memcpy(udpconfig->MACgw, arpreq.arp_ha.sa_data, 6);
#else
      fp = fopen("/proc/net/arp", "r");
      while(fgets(arp, 255, fp))
      {
        if(inet_addr(arp) == udpconfig->IPgw)
        {
          sscanf(arp, "%*s %*s %x %s", &flags, arp);
          if(flags & ATF_COM)
          {
            sscanf(arp, "%x", &hex);
            udpconfig->MACgw[0] = hex;
            sscanf(arp+3, "%x", &hex);
            udpconfig->MACgw[1] = hex;
            sscanf(arp+6, "%x", &hex);
            udpconfig->MACgw[2] = hex;
            sscanf(arp+9, "%x", &hex);
            udpconfig->MACgw[3] = hex;
            sscanf(arp+12, "%x", &hex);
            udpconfig->MACgw[4] = hex;
            sscanf(arp+15, "%x", &hex);
            udpconfig->MACgw[5] = hex;
          }
          break;
        }
      }
      fclose(fp);
#endif
    }
  }
}

static const unsigned char icmp[64] = {0x08,0x00,0xf7,0xff,0x00,};

int REQREG_prepare(unsigned char *buf, struct UdpConf *udpconfig)
{
  struct ether_header *eh = (struct ether_header *) buf;
  //struct arphdr *arph = (struct arphdr *) (buf + sizeof(struct ether_header));
  struct iphdr *iph = (struct iphdr *) (buf + sizeof(struct ether_header));
  struct udphdr *udph = (struct udphdr *) (buf + sizeof(struct iphdr) + sizeof(struct ether_header));
  int len, v;
  
  /* Controllo ogni volta quale sia il gateway corrente, e verifico il
     relativo MAC ogni volta. */
  udp_eth0_gateway(udpconfig);
  if(!udpconfig->IPgw) return 0;
  
  if(!memcmp(udpconfig->MACgw, ZeroBuf, 6))
  {
    /* Il MAC non è ancora noto, genero una richiesta di ARP */
    
    /* ATTENZIONE: se costruisco un pacchetto di ARP direttamente,
       scavalco lo stack IP e la risposta dell'ARP non va a finire
       nella cache ARP. Dovrei rimanere in ascolto ed aggiornare
       la tabella esplicitamente.
       
       Tanto vale quindi generare un ping, non mi interessa la
       risposta dovendo indirizzare un REQREG al server GEMSS, mi
       serve solo che si aggiorni la tabella di ARP. */
    
    struct sockaddr_in pingaddr;
    int fd;
    
    fd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    pingaddr.sin_family = AF_INET;
    pingaddr.sin_addr.s_addr = udpconfig->IPgw;
    sendto(fd, icmp, sizeof(icmp), 0,
        (struct sockaddr *)&pingaddr, sizeof(struct sockaddr_in));
    close(fd);
    
    return 0;
  }
  else
  {
    /* Genero un messaggio REQREG.
       Lo ricostruisco ogni volta perché potrebbe cambiare l'IP, il gateway... */
    
    /* Construct the Ethernet header */
    memset(buf, 0, BUF_SIZ);
    /* Ethernet header */
    memcpy(eh->ether_shost, udpconfig->MAChost, 6);
    memcpy(eh->ether_dhost, udpconfig->MACgw, 6);
    /* Ethertype field */
    eh->ether_type = htons(ETH_P_IP);

    /* IP Header */
    iph->ihl = 5;
    iph->version = 4;
    iph->tos = 16; // Low delay
    iph->id = htons(54321);
    iph->ttl = 255; // hops
    iph->protocol = 17; // UDP
    /* Source IP address, can be spoofed */
    iph->saddr = udpconfig->IPhost;
    /* Destination IP address */
    iph->daddr = udpconfig->IPsv[SV_LAN];

    /* UDP Header */
    udph->source = htons(udpconfig->lport);
    udph->dest = htons(udpconfig->Port);
    udph->check = 0; // skip

    /* Visto che il messaggio lo genero una volta sola e poi lo ripeto solo,
       diventa inutile popolare il campo datetime. */
    len = sizeof(struct ether_header)+sizeof(struct iphdr)+sizeof(struct udphdr);
    buf[len++] = 0;	// msgid
    buf[len++] = 0;	// msgid
    buf[len++] = bitConnect;	// stato
    buf[len++] = config.DeviceID&0xff;	// impianto
    buf[len++] = config.DeviceID>>8;	// impianto
    buf[len++] = 0;	// nodo
    buf[len++] = 0;	// datetime
    buf[len++] = 0;	// datetime
    buf[len++] = 0;	// datetime
    buf[len++] = 0;	// datetime
    buf[len++] = 14+16;	// len
    buf[len++] = 1;	// estensione
    buf[len++] = UDP_REQREG;	// evento[0]
    buf[len++] = 0;	// evento[1]
    
    /* Genero il challenge iniziale diverso da zero. */
    do
    {
      v = rand();
      memcpy(udpconfig->chall, &v, 4);
      v = rand();
      memcpy(udpconfig->chall+4, &v, 4);
      v = rand();
      memcpy(udpconfig->chall+8, &v, 4);
      v = rand();
      memcpy(udpconfig->chall+12, &v, 4);
    }
    while(!memcmp(udpconfig->chall, ZeroBuf, 16));
    
    memcpy(buf+len, udpconfig->chall, 16);
    len += 16;
    
    /* Aggiungo il MAC address per identificare la centrale. */
    memcpy(buf+len, udpconfig->MAChost, 6);
    len += 6;

    /* Length of UDP payload and header */
    udph->len = htons(BUF_SIZ - sizeof(struct ether_header) - sizeof(struct iphdr));
    /* Length of IP payload and header */
    iph->tot_len = htons(BUF_SIZ - sizeof(struct ether_header));
    /* Calculate IP checksum on completed header */
    iph->check = csum((unsigned short *)(buf+sizeof(struct ether_header)), sizeof(struct iphdr)/2);
    
    return BUF_SIZ;
  }
}

struct credenziali {
  int valid, ctx;
  unsigned char MAChost[6];
  unsigned int peer;
  char key[32], keylen;
  int port_s;
  char *url_s;
  unsigned int addr_s;
  char *regserv;
};

static int load_credenziali_cb(int event, const char *txt, int len, void *user)
{
  struct credenziali *credenziali = user;
  struct in_addr addr;
  
    if(event == EVENT_START)
    {
      if(!strcmp(txt, "credentials"))
      {
        memset(credenziali, 0, sizeof(struct credenziali));
        credenziali->valid = 1;
      }
      else if(!strcmp(txt, "peer"))
        credenziali->ctx = 1;
      else if(!strcmp(txt, "key"))
        credenziali->ctx = 2;
      else if(!strcmp(txt, "portspv"))	// tinyxml toglie gli '_'
        credenziali->ctx = 3;
      else if(!strcmp(txt, "addrspv"))
        credenziali->ctx = 4;
      else if(!strcmp(txt, "regsrv"))
        credenziali->ctx = 5;
    }
    else if(event == EVENT_TEXT)
    {
      switch(credenziali->ctx)
      {
        case 1:
          addr.s_addr = inet_addr(txt);
          credenziali->peer = addr.s_addr;
          break;
        case 2:
          credenziali->keylen = b64dec(txt, credenziali->key);
          break;
        case 3:
          sscanf(txt, "%d", &credenziali->port_s);
          break;
        case 4:
          credenziali->url_s = strdup(txt);
          break;
        case 5:
          credenziali->regserv = strdup(txt);
          break;
        default:
          break;
      }
    }
#if 0
    else if(event == EVENT_NAME)
    {
    }
    else if(event == EVENT_VAL)
    {
    }
    else if(event == EVENT_END)
    {
    }
#endif
  
  return 1;
}

void udp_save_credenziali(char *file, struct credenziali *credenziali)
{
  gzFile *zfp;
  struct in_addr addr;
  
  addr.s_addr = credenziali->peer;
  zfp = gzopen(file, "w");
  if(zfp)
  {
    gzprintf(zfp, "<credentials>\n<peer>%s</peer>\n", inet_ntoa(addr));
    gzprintf(zfp, "<key>%s</key>\n", b64enc(credenziali->key, credenziali->keylen));
    if(credenziali->port_s)
    {
      gzprintf(zfp, "<port_spv>%d</port_spv>\n", credenziali->port_s);
      gzprintf(zfp, "<addr_spv>%s</addr_spv>\n", credenziali->url_s);
    }
    if(credenziali->regserv)
      gzprintf(zfp, "<reg_srv>%s</reg_srv>\n", credenziali->regserv);
    gzprintf(zfp, "</credentials>\n");
  }
  gzclose(zfp);
  sync();
}

void udp_get_credenziali(struct credenziali *credenziali)
{
  /* E' richiesto il recupero delle credenziali */
  /* Il server delle credenziali è fisso e si deve poter accedere
     all'indirizzo pubblico. In caso di rete chiusa la procedura di
     registrazione viene fatta offline e il file delle credenziali
     ottenuto viene caricato a mano sulla centrale. */
/*
GET /api/plant/register HTTP/1.1
Host: 192.168.30.11:8081
plant_id: 123
mac: 00:50:c2:17:00:01
Cache-Control: no-cache

HTTP/1.1 200 OK
X-Powered-By: Express
Date: Tue, 03 May 2016 09:16:41 GMT
Connection: keep-alive
Content-Length: 65

{"register":"ok","endpoint":{"host":"192.168.30.11","port":4006}}


curl -X GET -H "plant_id: 123" -H "mac: 00:50:c2:17:00:01" http://192.168.30.11:8081/api/plant/register
{"register":"retry"}
{"register":"ok","endpoint":{"host":"192.168.30.11","port":4006}}

*/
  int fd, i, n;
  struct sockaddr_in sa;
  struct hostent *host;
  char *p, *par, *serv, *port;
  
  if(!credenziali->regserv)
    while(1) sleep(1);	// si ferma qui
  
  serv = strdup(credenziali->regserv);
  port = strstr(serv, ":");
  if(!port)
    while(1) sleep(1);	// si ferma qui
  *port = 0;
  port++;
  
  do
  {
    host = gethostbyname(serv);
    if(host)
    {
      sa.sin_family = AF_INET;
      sa.sin_port = htons(atoi(port));
      memcpy(&sa.sin_addr.s_addr, host->h_addr, 4);
      
      fd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
      if(!connect(fd, (struct sockaddr*)&sa, sizeof(struct sockaddr_in)))
      {
        p = malloc(1024);
        do
        {
          sprintf(p, "GET /api/plant/register HTTP/1.1\r\nHost: %s:%s\r\nplant_id: %d\r\n"
            "mac: %02x:%02x:%02x:%02x:%02x:%02x\r\nCache-Control: no-cache\r\n\r\n",
            serv, port, config.DeviceID,
            credenziali->MAChost[0], credenziali->MAChost[1], credenziali->MAChost[2],
            credenziali->MAChost[3], credenziali->MAChost[4], credenziali->MAChost[5]);
printf("\n\nTX %s", p);
          send(fd, p, strlen(p), 0);
          
          p[0] = 0;
          i = 0;
          do
          {
#warning TIMEOUT!!!!
/* Usare select con timeout 10s */
            n = recv(fd, p+i, 1024-i, 0);
            if(n > 0)
            {
              i += n;
              p[i] = 0;
printf("RX %s", p);
            }
          }
          while((n > 0) && !strstr(p, "}"));
          if(n > 0)
          {
            /* Verifico se ho le credenziali o se devo ripetere la richiesta.
               Se devo ripetere sfrutto la connessione keepalive. */
            if(strstr(p, "\"register\"") && strstr(p, "\"retry\""))
            {
              /* Aspetto un po' e ripeto la richiesta */
              sleep(1);
              n = 0;
            }
            else if(strstr(p, "\"register\"") && strstr(p, "\"endpoint\""))
            {
              /* Credenziali */
              par = strstr(p, "\"host\"");
              par += 6;
              while(*par != '"') par++;
              credenziali->url_s = strdup(par+1);
              for(par=credenziali->url_s; *par!='"'; par++);
              *par = 0;
              par = strstr(p, "\"port\"");
              par += 6;
              while(*par != ':') par++;
              credenziali->port_s = atoi(par+1);
              
              host = gethostbyname(credenziali->url_s);
              if(host)
                memcpy(&credenziali->addr_s, host->h_addr, 4);
            }
          }
          else
            sleep(2);
        }
        while(n <= 0);
        
        free(p);
      }
      else
      {
        host = NULL;
        sleep(2);
      }
      close(fd);
    }
    else
      sleep(2);
  }
  while(!host);
printf("\n\nCredenziali: %s %08x %d\n", credenziali->url_s, credenziali->addr_s, credenziali->port_s);
}

static void udp_loop(ProtDevice *dev)
{
  Event ev;
  
  struct UdpConf udpconfig;
  
// GC 20080305
  UDP_Msg umi, umo;
  
  int numResend = 0;
  int res, fdi, fdo, salen, tempo_disconn, port_s;
  int peer = 0, secret = 0, forcealign = 1;
  int connto = 0;
  int keepalive = 0;
  struct sockaddr_in sa, safrom, sato, sareg;
  struct in_addr fpeer;
  char forcedip[20], ipconf[2][20], *p;

  unsigned short msgid_in = 0;
  unsigned short msgid_out = 0;
  fd_set fds;
  struct timeval tv;
  timeout_t *udpto, *kato;
  int tempo_keepalive;
  struct aes_ctx aesctx;
  int fdkey;
  char keyfilebuf[32], *keyfile;
  tinyxml_t *xml;
  gzFile *zfp;
  struct credenziali credenziali = {0, };
  
  struct ifreq ifr;
  unsigned char REQREGbuf[BUF_SIZ];
  int REQREGlen;
  struct sockaddr_ll REQREGsa;
  
/* Richieste di registrazione ogni 20s */
#define TIMEOUT_REQREG  19
  int timeout_REQREG = 0;
  UDP_event umr;
  int forcebackup = 0;
  int reqreg_retry = 0;
  
  int gprs_check = 0;
  
  enum UdpState udpstate = UDP_NULL;
  
  if(!dev) return;
  if(!config.consumer[dev->consumer].param ||
     (config.consumer[dev->consumer].configured != 5)) return;
  
  pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
  
  support_log("UDP Init");
// GC 20080508
  while(!libuser_started) sleep(1);
  
// GC 20080305
  umo.len = -1;
  
  udpconfig.fdraw = -1;
  udpconfig.Port = config.consumer[dev->consumer].data.eth.port;
  
//  tempo_disconn = atoi(config.consumer[dev->consumer].param) * 10;
  tempo_disconn = 0;
  port_s = 0;
  forcedip[0] = 0;
  ipconf[0][0] = 0;
  ipconf[1][0] = 0;
  fpeer.s_addr = 0;
  sscanf(config.consumer[dev->consumer].param, "%d:%d", &tempo_disconn, &port_s);
  tempo_disconn *= 10;
  
  for(p = config.consumer[dev->consumer].param; *p && (*p != ':'); p++);
  if(*p) for(p++; *p && (*p != ':'); p++);
  if(*p)
  {
    p++;
    strncpy(forcedip, p, 20);
    for(; *p && (*p != ':'); p++);
  }
  if(*p)
  {
    p++;
    strncpy(ipconf[0], p, 20);
    for(; *p && (*p != ':'); p++);
  }
  if(*p)
  {
    p++;
    strncpy(ipconf[1], p, 20);
    for(; *p && (*p != ':'); p++);
  }
  forcedip[19] = 0;
  for(p = forcedip; *p && (*p != ':'); p++);
  *p = 0;
  ipconf[0][19] = 0;
  for(p = ipconf[0]; *p && (*p != ':'); p++);
  *p = 0;
  ipconf[1][19] = 0;
  for(p = ipconf[1]; *p && (*p != ':'); p++);
  *p = 0;
  
  if(forcedip[0])
  {
    if(!inet_aton(forcedip, &fpeer))
      fpeer.s_addr = 0;
  }
  
  credenziali.valid = 0;
  sprintf(keyfilebuf, "/saet/udp%d.key", dev->consumer);
  keyfile = ADDROOTDIR(keyfilebuf);
  zfp = gzopen(keyfile, "r");
  if(zfp)
  {
    int n;
    p = malloc(1024);
    n = gzread(zfp, p, 1024);
p[n] = 0;
printf("%s\n", p);
    xml = tinyxml_new(1024, load_credenziali_cb, &credenziali);
    tinyxml_feed(xml, p, n);
    tinyxml_free(xml);
    
    if(!credenziali.valid)
    {
      memcpy(&credenziali.peer, p, 4);
      credenziali.keylen = 0;
      if(n > 4)
      {
        credenziali.keylen = n - 4;
        memcpy(&credenziali.key, p+4, credenziali.keylen);
      }
      udp_save_credenziali(keyfile, &credenziali);
    }
    
    free(p);
    gzclose(zfp);
  }
  
  if(ipconf[0][0])
  {
    udpconfig.IPsv[SV_CUR] = udpconfig.IPsv[SV_LAN] = udpconfig.IPsv[SV_BCK] = inet_addr(ipconf[0]);
    if(ipconf[1][0])
    {
      udpconfig.IPsv[SV_BCK] = inet_addr(ipconf[1]);
    }
  }
  
  if(ipconf[0][0] || credenziali.regserv)
  {
    /* Recupero il MAC della scheda, è l'unica cosa che non cambia
       quindi lo faccio una volta per tutte. */
    udpconfig.fdraw = socket(AF_PACKET, SOCK_RAW, IPPROTO_RAW);
    
    memset(&ifr, 0, sizeof(struct ifreq));
    strcpy(ifr.ifr_name, INTERFACE);
    ioctl(udpconfig.fdraw, SIOCGIFINDEX, &ifr);
    REQREGsa.sll_ifindex = ifr.ifr_ifindex;
    REQREGsa.sll_halen = ETH_ALEN;
    ioctl(udpconfig.fdraw, SIOCGIFHWADDR, &ifr);
    memcpy(udpconfig.MAChost, ifr.ifr_hwaddr.sa_data, 6);
    memcpy(credenziali.MAChost, ifr.ifr_hwaddr.sa_data, 6);
  }
  
  udpto = timeout_init();
  kato = timeout_init();
  
  fdi = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if(!fdi) return;
  fdo = fdi;
  
  sa.sin_family = AF_INET;
  sa.sin_port = htons(config.consumer[dev->consumer].data.eth.port);
  sa.sin_addr.s_addr = INADDR_ANY;
  res = bind(fdi, (struct sockaddr*)&sa, sizeof(struct sockaddr_in));
  
  if(credenziali.valid && credenziali.peer)
  {
    aes_set_key(&aesctx, credenziali.key, credenziali.keylen);
    udpstate = UDP_CONNECTED;
    sa.sin_addr.s_addr = credenziali.peer;
    memcpy(&sato, &sa, sizeof(struct sockaddr));
    //connect(fdo, (struct sockaddr*)&sa, sizeof(struct sockaddr));
    if(tempo_disconn) timeout_on(udpto, udp_timeout, &connto, 0, tempo_disconn);
    
    /* Se sono in modalità di registrazione inversa, al riavvio
       non posso garantire che i parametri di rete siano gli stessi
       di quelli salvati, mantengo la chiave ma annullo la
       registrazione */
    if(udpconfig.IPsv[SV_CUR])
    {
      credenziali.peer = 0;
      timeout_off(kato);
    }
    
    peer = credenziali.peer;
    if(credenziali.port_s) port_s = credenziali.port_s;
  }
  
  if(port_s < 0)
  {
    udp_get_credenziali(&credenziali);
    sa.sin_port = htons(credenziali.port_s);
    udpconfig.Port = credenziali.port_s;
    udpconfig.IPsv[SV_CUR] = udpconfig.IPsv[SV_LAN] = credenziali.addr_s;
  }
  else if(port_s > 0)
  {
    /* Imposto la porta del supervisore, può essere diversa da quella locale */
    sa.sin_port = htons(port_s);
    udpconfig.Port = port_s;
  }
  
  tempo_keepalive = 600;
  
  tv.tv_sec = 0;
  tv.tv_usec = 100000;
  
  while(1)
  {
    FD_ZERO(&fds);
    FD_SET(fdi, &fds);
    res = select(fdi+1, &fds, NULL, NULL, &tv);
    
    /* Verifica connessione al server */
    if(udpconfig.IPsv[SV_LAN])
    {
      ioctl(udpconfig.fdraw, SIOCGIFFLAGS, &ifr);
      if(!(ifr.ifr_flags & IFF_RUNNING))
      {
        if(udpconfig.eth0running)
        {
          /* Cavo scollegato, interrompo eventuali richieste in corso
             sulla LAN (**** e attivo il backup GPRS ****)NO. */
          udpconfig.eth0running = 0;
        }
        memset(udpconfig.MACgw, 0, 6);
        udpconfig.IPgw = 0;
      }
      else if(!udpconfig.eth0running)
      {
        /* Cavo ricollegato, innesco le richieste di registrazione */
        udpconfig.eth0running = 1;
      }
    }
    
    if(connto)
    {
#ifdef DEBUG
printf("*******\nAzzeramento peer (1)\n*******\n");
#endif
      connto = 0;
      peer = 0;
      timeout_off(kato);
      
// GC 20071219
      /* Se avevo un messaggio in canna ma mi scade la connessione, il messaggio
         contenuto in umo.event che era criptato, deve essere decifrato per la corretta
         gestione dei reinvii. */
      if(credenziali.keylen && (umo.len > 0))
      {
        udp_decrypt(&aesctx, &umo);
        umo.len = umo.event.len;
      }
      
      credenziali.keylen = 0;
      udpstate = UDP_NULL;
    }
    
    if(peer && keepalive)
    {
      /* Non ho più attività dal server, prima di attivare il backup
         provo a riregistrare la centrale. */
#ifdef DEBUG
printf("*******\nAzzeramento peer (2)\n*******\n");
#endif
      peer = 0;
      timeout_REQREG = 0;
      reqreg_retry = 0;
      keepalive = 0;
      
      //support_log("Connessione scaduta");
    }
    
    if(res)
    {
      salen = sizeof(struct sockaddr_in);
// GC 20080305
      umi.len = recvfrom(fdi, &umi.event, sizeof(UDP_event), 0, (struct sockaddr*)&safrom, &salen);
      umi.crypt = 0;
#ifdef DEBUG
printf("\nrecv=%d peer=%08x auth=%d\n",umi.len,peer,credenziali.keylen);
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
      /* I messaggi di registrazione inversa viaggiano sempre in chiaro,
         anche se c'è una chiave impostata. */
      if((umi.event.impianto == config.DeviceID) && (umi.event.nodo == 0) && (umi.event.stato & bitConnect) &&
         (umi.event.evento[0] >= UDP_REQREG) && (umi.event.evento[0] <= UDP_REGISTER))
      {
        /* Sembra proprio un messaggio della procedura di registrazione inversa.
           E' già in chiaro, lo devo lasciar passare. */
      }
      else
        if(credenziali.keylen && !udp_decrypt(&aesctx, &umi)) continue;
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
// GC 20140205
      if((umi.event.stato & bitConnect) && (umi.event.evento[0] == UDP_CHALLENGE))
      {
#if 0
        if(!memcmp(umi.event.evento+17, udpconfig.chall, 16) && !backupchallclear)
        {
          /* Il challenge è tornto in chiaro. Accetto alternanza uno chiaro e uno cifrato
             come buona per la gestione del ripristino connessione primaria perché con il
             GEMSS non si sa a priori se la chiave sia impostata correttamente e risponde
             appunto una volta cifrato e una volta in chiaro. */
          /* Non accetto invece sequenza di risposte in chiaro */
          /* Il GEMSS risponde sempre in chiaro in questi casi, non so perché. */
          
          backupchallclear = 1;
        }
#endif
        
        /* Verifico se il challenge di ritorno corrisponde a quello della richiesta. */
        if(credenziali.keylen) aes_decrypt(&aesctx, umi.event.evento+17, umi.event.evento+17);
        
#ifdef DEBUG
{
int x;
printf("\nchallenge: ");
for(x=0; x<16; x++) printf("%02x", udpconfig.chall[x]);
printf("=");
for(x=0; x<16; x++) printf("%02x", umi.event.evento[17+x]);
printf("\n");
}
#endif
        if(memcmp(umi.event.evento+17, udpconfig.chall, 16) ||
           !memcmp(umi.event.evento+17, ZeroBuf, 16))
        {
          /* Il challenge non corrisponde, ignoro il messaggio. */
          //support_log("Challenge errato");
        }
        else
        {
          memset(udpconfig.chall, 0, 16);
          
          safrom.sin_port = sa.sin_port;
          umi.event.stato |= bitAck;
          umi.event.datetime = time(NULL) - BASESECONDS;
          umi.event.estensione = 1;
          umi.event.len = sizeof(UDP_event)-(256-17);
          //debug(udpconfig.Debug & 1, idx, "TX", (unsigned char*)&umi.event, umi.event.len);
          //support_log("Invio ACK");
printf("Invio ACK\n");
          sendto(fdo, &umi.event, umi.event.len, 0, (struct sockaddr*)&safrom, sizeof(struct sockaddr_in));
          umi.event.msgid = msgid_out;
          umi.event.stato = bitConnect;
          umi.event.evento[0] = UDP_REGISTER;
          if(credenziali.keylen) aes_encrypt(&aesctx, umi.event.evento+1, umi.event.evento+1);
          umi.event.len = sizeof(UDP_event)-(256-17);
          //debug(udpconfig.Debug & 1, idx, "TX", (unsigned char*)&umi.event, umi.event.len);
          //support_log("Invio Fase 8");
#ifdef DEBUG
{
char cmd[256];
int x,len;
sprintf(cmd, "\nTX (%08x:%d):", safrom.sin_addr, safrom.sin_port);
len = strlen(cmd);
for(x=0; x<umi.len; x++) sprintf(cmd+len+(x*2), "%02x", ((unsigned char*)&umi.event)[x]);
//support_log(cmd);
printf("%s\n", cmd);
}
#endif
          sendto(fdo, &umi.event, umi.event.len, 0, (struct sockaddr*)&safrom, sizeof(struct sockaddr_in));
        }
      }
      else
      {
        if(tempo_disconn) timeout_on(udpto, udp_timeout, &connto, 0, tempo_disconn);
        if(peer) timeout_on(kato, udp_keepalive, &keepalive, 0, tempo_keepalive);
      }
      
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
          if(credenziali.keylen && umo.crypt && (umo.len > 0))
          {
            udp_decrypt(&aesctx, &umo);
            umo.len = umo.event.len;
          }
          
          credenziali.keylen = 0;
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
printf("1.TX:");
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
printf("2.TX:");
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
            numResend = 0;
            udpstate = UDP_CONNECTED;
//            peer = safrom.sin_addr.s_addr;
            if(fpeer.s_addr)
              peer = fpeer.s_addr;
            else
              peer = safrom.sin_addr.s_addr;
            sa.sin_addr.s_addr = peer;
            memcpy(&sato, &sa, sizeof(struct sockaddr));
            //connect(fdo, (struct sockaddr*)&sa, sizeof(struct sockaddr));
#ifdef DEBUG
printf("*** Connesso! (%08x %08x %08x)\n", peer, fpeer.s_addr, credenziali.peer);
#endif
            if(credenziali.peer != peer)
            {
              credenziali.peer = peer;
              udp_save_credenziali(keyfile, &credenziali);
            }
            
            timeout_on(kato, udp_keepalive, &keepalive, 0, tempo_keepalive);
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
            credenziali.peer = peer;
            
            msgid_out++;
            udpstate = UDP_CONNECTED;
            
// GC 20080305
            /* Se ho un evento già in canna e cambio la chiave, devo prima decrittare
               con la vecchia chiave, altrimenti l'evento diventa irrecuperabile. */
            if(credenziali.keylen && umo.crypt && (umo.len > 0))
            {
              udp_decrypt(&aesctx, &umo);
              umo.len = umo.event.len;
            }
            
            aes_set_key(&aesctx, credenziali.key, 24);
            credenziali.keylen = 24;
            
            udp_save_credenziali(keyfile, &credenziali);
          }
          else
          {
/*
// GC 20070213
            if(credenziali.keylen)
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
            if(credenziali.keylen)
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
              memcpy(credenziali.key, umi.event.evento+1, 24);
              if(fpeer.s_addr) safrom.sin_addr = fpeer;
              safrom.sin_port = sa.sin_port;
              umi.event.stato |= bitAck;
              umi.event.datetime = time(NULL) - BASESECONDS;
              umi.event.estensione = 1;
              umi.len = umi.event.len = sizeof(UDP_event)-256;
#ifdef DEBUG
{
int x;
printf("3.TX:");
  for(x=0; x<umi.event.len; x++) printf(" %02x", ((unsigned char*)&umi.event)[x]);
printf("\n");
}
#endif
              if(credenziali.keylen) udp_encrypt(&aesctx, &umi);
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
printf("4.TX:");
  for(x=0; x<umi.event.len; x++) printf(" %02x", ((unsigned char*)&umi.event)[x]);
printf("\n");
}
#endif
              if(credenziali.keylen) udp_encrypt(&aesctx, &umi);
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
printf("5.TX:");
  for(x=0; x<umi.event.len; x++) printf(" %02x", ((unsigned char*)&umi.event)[x]);
printf("\n");
}
#endif
              if(credenziali.keylen) udp_encrypt(&aesctx, &umi);
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
printf("6.TX:");
  for(x=0; x<umi.event.len; x++) printf(" %02x", ((unsigned char*)&umi.event)[x]);
printf("\n");
}
#endif
              if(credenziali.keylen) udp_encrypt(&aesctx, &umi);
              sendto(fdo, &umi.event, umi.len, 0, (struct sockaddr*)&safrom, sizeof(struct sockaddr_in));
              udpstate = UDP_CONFIRM;
            }
            continue;
          }
          break;
        case UDP_REQREG:
          break;
        case UDP_CHALLENGE:
          break;
        case UDP_REGISTER:
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
        umo.crypt = 0;
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
        if(credenziali.keylen) udp_encrypt(&aesctx, &umi);
        //write(fdo, &umi.event, umi.len);
        sendto(fdo, &umi.event, umi.len, 0, (struct sockaddr*)&sato, sizeof(struct sockaddr_in));
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
printf("7.TX:");
  for(x=0; x<umi.len; x++) printf(" %02x", ((unsigned char*)&umi.event)[x]);
printf("\n");
}
#endif
        if(credenziali.keylen) udp_encrypt(&aesctx, &umi);
//        write(fdo, &umi.event, res);
        if(fpeer.s_addr) safrom.sin_addr = fpeer;
        safrom.sin_port = sa.sin_port;
        sendto(fdo, &umi.event, umi.len, 0, (struct sockaddr*)&safrom, sizeof(struct sockaddr_in));
      }
    }
    else if(!peer && udpconfig.IPsv[SV_CUR])
    {
      /* Invio il messaggio REQREG per iniziare la connessione */
      if(timeout_REQREG)
        timeout_REQREG--;
      else
      {
        {
          int v;
          
          reqreg_retry++;
          
          timeout_REQREG = TIMEOUT_REQREG;	// ripetizioni ogni 20s per dare tempo anche alle connessioni lente di chiudere il giro
          
          sareg.sin_family = AF_INET;
          sareg.sin_port = htons(udpconfig.Port);
          sareg.sin_addr.s_addr = udpconfig.IPsv[SV_CUR];
          umr.msgid = 0;
          umr.stato = bitConnect;
          umr.impianto = config.DeviceID;
          umr.nodo = 0;
          umr.datetime = time(NULL) - BASESECONDS;
          umr.len = 2+16+6+sizeof(UDP_event)-256;
          umr.estensione = 1;
          umr.evento[0] = UDP_REQREG;
          umr.evento[1] = 0;
          
          /* Calcolo un challenge diverso da zero. */
          do
          {
            v = rand();
            memcpy(udpconfig.chall, &v, 4);
            v = rand();
            memcpy(udpconfig.chall+4, &v, 4);
            v = rand();
            memcpy(udpconfig.chall+8, &v, 4);
            v = rand();
            memcpy(udpconfig.chall+12, &v, 4);
          }
          while(!memcmp(udpconfig.chall, ZeroBuf, 16));
          
          memcpy(umr.evento+2, udpconfig.chall, 16);
          
          /* Aggiungo il MAC address per identificare la centrale. */
          memcpy(umr.evento+2+16, udpconfig.MAChost, 6);
          
          //debug(udpconfig.Debug & 1, idx, "TX", (unsigned char*)&umr, umr.len);
          //support_log("Invio ReqReg");
#ifdef DEBUG
printf("**** Invio REQREG (%08x:%d)\n", udpconfig.IPsv[SV_CUR], udpconfig.Port);
#endif
#ifdef DEBUG
{
int x;
printf("TX:");
for(x=0; x<umr.len; x++) printf("%02x", ((unsigned char*)&umr)[x]);
printf("\n");
}
#endif
          sendto(fdo, &umr, umr.len, 0, (struct sockaddr*)&sareg, sizeof(struct sockaddr_in));
        }
      }
    }
    else if(umo.len >= 0)
    {
      if(numResend < 3)
        numResend++;
      else
      {
        forcealign = 1;
        
        if(udpconfig.IPsv[SV_CUR])
        {
          /* Se ho un evento che non riesco ad inoltrare, annullo
             la connessione in corso e riparto con la registrazione.
             La chiave deve rimanere valida. */
          //support_log("Annulla peer");
#ifdef DEBUG
printf("*******\nAzzeramento peer (3)\n*******\n");
#endif
          peer = 0;
          timeout_REQREG = 0;
          timeout_off(kato);
        }
      }
      
      if(peer)
      {
        /* reinvia */
        if(credenziali.keylen && umo.crypt) udp_decrypt(&aesctx, &umo);
        umo.event.stato |= bitResend;
        /* Riallinea il contatore se nel frattempo sono
           stati gestiti altri messaggi. */
        umo.event.msgid = msgid_out;
        umo.event.impianto = config.DeviceID;
#ifdef DEBUG
{
int x;
printf("8a.TX:");
for(x=0; x<umo.len; x++) printf(" %02x", ((unsigned char*)&umo.event)[x]);
printf("\n");
}
#endif
        if(credenziali.keylen) udp_encrypt(&aesctx, &umo);
#ifdef DEBUG
{
int x;
printf("8b.TX:");
for(x=0; x<umo.len; x++) printf(" %02x", ((unsigned char*)&umo.event)[x]);
printf("\n");
}
#endif
        //write(fdo, &umo.event, umo.len);
        sendto(fdo, &umo.event, umo.len, 0, (struct sockaddr*)&sato, sizeof(struct sockaddr_in));
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
printf("9a.TX:");
  for(x=0; x<umo.len; x++) printf(" %02x", ((unsigned char*)&umo.event)[x]);
printf("\n");
}
#endif
        if(credenziali.keylen) udp_encrypt(&aesctx, &umo);
#ifdef DEBUG
{
int x;
printf("9b.TX:");
  for(x=0; x<umo.len; x++) printf(" %02x", ((unsigned char*)&umo.event)[x]);
printf("\n");
}
#endif
        //write(fdo, &umo.event, umo.len);
        sendto(fdo, &umo.event, umo.len, 0, (struct sockaddr*)&sato, sizeof(struct sockaddr_in));
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
    else if((umo.len > 0) && !peer)
    {
      tv.tv_sec = 1;
      tv.tv_usec = 0;
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
  printf("UDP plugin: " __DATE__ " " __TIME__ " (v2.0.7)\n");
  prot_plugin_register("UDP", 0, NULL, NULL, (PthreadFunction)udp_loop);
  aes_gen_tabs();
}

