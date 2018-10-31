#include "volto.h"
#include "support.h"
#include "aes.h"
#include "delphi.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <pthread.h>

//#define DEBUG

static int volto_localfd[2];
static unsigned char volto_key[24];
static struct aes_ctx volto_aes_ctx;
static const char *VOLTO_KEY_FILENAME = "/saet/volto.key";
/*
  L'inizializzazione viene fatta all'inizio e ogni volta che
  viene modificata l'anagrafica terminali.
  Questa variabile mantiene lo stato di attivazione del sistema,
  che se già attivo non viene reinizializzato.
*/
static int volto_active = 0;

static void volto_encrypt(volto_pkt *request)
{
  /* calcolare il crc e cifrare */
  request->crc = CRC16((unsigned char*)request, sizeof(volto_pkt)-2);
  
#if 0
#warning Dump messaggi
{
int xx;
printf("TX: ");
for(xx=0; xx<sizeof(volto_pkt); xx++) printf("%02x", ((unsigned char*)request)[xx]);
printf("\n");
}
#endif
  
#ifndef DEBUG
  /* Il pacchetto è esattamente 32 byte */
  aes_encrypt(&volto_aes_ctx, (u8*)request, (u8*)request);
  aes_encrypt(&volto_aes_ctx, ((u8*)request)+16, ((u8*)request)+16);
  
#if 0
#warning Dump messaggi
{
int xx;
printf("TC: ");
for(xx=0; xx<sizeof(volto_pkt); xx++) printf("%02x", ((unsigned char*)request)[xx]);
printf("\n");
}
#endif
#endif

}

static int volto_decrypt(volto_pkt *reply)
{
  /* decifrare e controllare il CRC */
#ifndef DEBUG
#if 0
#warning Dump messaggi
{
int xx;
printf("RC: ");
for(xx=0; xx<sizeof(volto_pkt); xx++) printf("%02x", ((unsigned char*)reply)[xx]);
printf("\n");
}
#endif
  
  /* Il pacchetto è esattamente 32 byte */
  aes_decrypt(&volto_aes_ctx, (u8*)reply, (u8*)reply);
  aes_decrypt(&volto_aes_ctx, ((u8*)reply)+16, ((u8*)reply)+16);
#endif

#if 0
#warning Dump messaggi
{
int xx;
printf("RX: ");
for(xx=0; xx<sizeof(volto_pkt); xx++) printf("%02x", ((unsigned char*)reply)[xx]);
printf("\n");
}
#endif
  
  return (reply->crc == CRC16((unsigned char*)reply, sizeof(volto_pkt)-2));
}

void volto_reply(volto_param *req, int reply)
{
  switch(req->tipo)
  {
    case VOLTO_IDFY_LARA:
      {
        lara_tMsg cmd;
        cmd.Code = LARA_ID;
        cmd.Address.node = req->lara.nodo;
        cmd.Length = sizeof(lara_t_id);
        req->lara.msg.attrib.spare = reply;
        memcpy(cmd.Msg, &(req->lara.msg), sizeof(lara_t_id));
        lara_parse(req->lara.mm, (unsigned char*)&cmd);
//printf("reply: %d\n", reply);
      }
      break;
    case VOLTO_IDFY_TEBE:
      break;
    case VOLTO_INSERT:
      break;
    case VOLTO_DELETE:
      break;
    case VOLTO_JOIN:
      break;
    case VOLTO_ENROLL:
      break;
    case VOLTO_ENR_CANC:
      break;
    default:
      break;
  }
}

void volto_req(volto_param *req)
{
  if(!volto_active) return;
  write(volto_localfd[0], req, sizeof(volto_param));
}

typedef struct {
  volto_param param;
  int fd;
  int time;
  unsigned char resend;
  unsigned char seq;
} Connection;

static unsigned char seq = 0;

static void volto_send(Connection *conn)
{
  volto_pkt request;
  
  /* Invia la richiesta al server */
  memset(&request, 0, sizeof(request));
  switch(conn->param.tipo)
  {
          case VOLTO_IDFY_LARA:
            request.code = 1;	// riconoscimento id
            request.data[0] = conn->param.lara.msg.id >> 8;
            request.data[1] = conn->param.lara.msg.id & 0xff;
            request.data[2] = config.DeviceID & 0xff;
            request.data[3] = config.DeviceID >> 8;
            request.data[4] = conn->param.lara.nodo;
            break;
          case VOLTO_IDFY_TEBE:
            break;
          case VOLTO_INSERT:
            if(config.VoltoServerUpdate)
            {
              request.code = 2;	// inserimento nuovo id
              request.data[0] = conn->param.anagrafica.id & 0xff;
              request.data[1] = conn->param.anagrafica.id >> 8;
            }
            break;
          case VOLTO_DELETE:
            if(config.VoltoServerUpdate)
            {
              request.code = 3;	// cancellazione id
              request.data[0] = conn->param.anagrafica.id & 0xff;
              request.data[1] = conn->param.anagrafica.id >> 8;
            }
            break;
          case VOLTO_JOIN:
            if(config.VoltoServerUpdate)
            {
              request.code = 4;	// associazione id-id
              request.data[0] = conn->param.anagrafica.id & 0xff;
              request.data[1] = conn->param.anagrafica.id >> 8;
              request.data[2] = conn->param.anagrafica.id2 & 0xff;
              request.data[3] = conn->param.anagrafica.id2 >> 8;
            }
            break;
          case VOLTO_ENROLL:
            request.code = 5;	// predisposizione enroll
            request.data[0] = conn->param.anagrafica.id & 0xff;
            request.data[1] = conn->param.anagrafica.id >> 8;
            request.data[2] = conn->param.anagrafica.imp & 0xff;
            request.data[3] = conn->param.anagrafica.imp >> 8;
            request.data[4] = conn->param.anagrafica.term;
            break;
          case VOLTO_ENR_CANC:
            request.code = 6;	// annulla enroll
            request.data[0] = conn->param.anagrafica.id & 0xff;
            request.data[1] = conn->param.anagrafica.id >> 8;
            request.data[2] = conn->param.anagrafica.imp & 0xff;
            request.data[3] = conn->param.anagrafica.imp >> 8;
            request.data[4] = conn->param.anagrafica.term;
            break;
          default:
            break;
  }
  
  if(!request.code) return;
  
  conn->time = request.time = time(NULL);
  conn->seq = request.seq = seq++;
  volto_encrypt(&request);
//printf("send\n");
  write(conn->fd, &request, sizeof(request));
}

void LogStato(int n, Connection *conn)
{
#if 0
  printf("connessione %d\n", n);
  printf("  fd = %d\n", conn[n].fd);
  printf("  time = %d\n", conn[n].time);
  printf("  param = %d\n", conn[n].param.tipo);
#endif
}

static void* volto_loop(void *null)
{
  fd_set fdr, fdw;
  int maxfd, n, f, l;
  struct timeval to;
  char serveraddr[16];
  int serverport;
  volto_pkt reply;
  volto_param param;
  struct sockaddr_in sa, sap;
  Connection *conn;
  int conn_attive, conn_libere;
  char logstr[64];
  
  if(!config.VoltoMaxConn) config.VoltoMaxConn = 5;
  if(!config.VoltoMinConn) config.VoltoMinConn = 2;
  
  conn = calloc(config.VoltoMaxConn, sizeof(Connection));
  if(!conn) return NULL;
  
  conn_attive = conn_libere = 0;
  seq = 0;
  
  if(!config.VoltoServerName)
  {
    support_log("Volto: server non configurato.");
    volto_active = 0;
    return NULL;
  }
  
  for(n=0; config.VoltoServerName[n] && (config.VoltoServerName[n]!=':'); n++);
  if(!config.VoltoServerName[n] || (n > 15))
  {
    support_log("Volto: errore configurazione.");
    volto_active = 0;
    return NULL;
  }
  memcpy(serveraddr, config.VoltoServerName, n);
  serveraddr[n] = 0;
  serverport = atoi(config.VoltoServerName+n+1);
  sa.sin_family = AF_INET;
  sa.sin_port = htons(serverport);
  sa.sin_addr.s_addr = inet_addr(serveraddr);
  
  support_log("Volto loop");
  debug_pid[DEBUG_PID_VOLTO] = support_getpid();
  
  while(1)
  {
#warning Verifiche tampone
    if(!config.VoltoMaxConn) config.VoltoMaxConn = 5;
    if(!config.VoltoMinConn) config.VoltoMinConn = 2;
    
    
    if(conn_attive < config.VoltoMinConn)
    {
      f = 0;
      for(n=0; n<config.VoltoMaxConn; n++)
        if(conn[n].time) f++;	// connesso o in connessione
      
      // controllo che non ci siano già connessioni in corso
      for(n=0; (n<config.VoltoMaxConn)&&(f<config.VoltoMinConn); n++)
        if(!conn[n].time)
        {
          conn[n].time = -1;
          f++;
        }
    }
    
    /* Cerca di attivare una connessione */
    for(n=0; n<config.VoltoMaxConn; n++)
    {
      if(conn[n].time == -1)
      {
        conn[n].fd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
        if(conn[n].fd >= 0)
        {
sprintf(logstr, "connessione %d", n);
support_log2(logstr);
          fcntl(conn[n].fd, F_GETFL, &f);
          f |= O_NONBLOCK;
          fcntl(conn[n].fd, F_SETFL, f);
          f = connect(conn[n].fd, (struct sockaddr*)&sa, sizeof(struct sockaddr));
          if(f < 0)
          {
            if(errno == EINPROGRESS)
              conn[n].time = -2;
            else
{
support_log2("Errore apertura socket");
              close(conn[n].fd);
//101008
              conn[n].fd = -1;
              conn[n].time = 0;
              conn[n].param.tipo = 0;
LogStato(n, conn);
}
          }
          else
          {
            conn[n].time = time(NULL);
            conn_attive++;
            conn_libere++;
            if(conn_attive == 1) support_log("Volto: connesso.");
l = sizeof(struct sockaddr);
getsockname(conn[n].fd, (struct sockaddr*)&sap, &l);
sprintf(logstr, "    %d -> %d", n, ntohl(sap.sin_port)>>16);
support_log2(logstr);
sprintf(logstr, "  attive:%d libere:%d", conn_attive, conn_libere);
support_log2(logstr);
          }
        }
// E' giusto che la connessione non attiva sia a -1.
//        else
//          conn[n].fd = 0;
      }
    }
    
    FD_ZERO(&fdr);
    FD_ZERO(&fdw);
    FD_SET(volto_localfd[1], &fdr);
    maxfd = volto_localfd[1];
    
    for(n=0; n<config.VoltoMaxConn; n++)
    {
      if(conn[n].time == -2)
      {
        FD_SET(conn[n].fd, &fdw);
        if(conn[n].fd > maxfd) maxfd = conn[n].fd;
      }
      else if(conn[n].time > 0)
      {
        FD_SET(conn[n].fd, &fdr);
        if(conn[n].fd > maxfd) maxfd = conn[n].fd;
      }
    }
    
    to.tv_sec = 1;
    to.tv_usec = 0;
    if(!select(maxfd+1, &fdr, &fdw, NULL, &to))
    {
      for(n=0; n<config.VoltoMaxConn; n++)
      {
        if(conn[n].time == -3)
          conn[n].time = -1;
        else if(conn[n].time > 0)
        {
          if((conn_libere > config.VoltoMinConn) && ((time(NULL) - conn[n].time) > 60))
          {
sprintf(logstr, "  chiusura per inattività (%d)", n);
support_log2(logstr);
            // le connessioni vengono chiuse dopo 60s di inutilizzo
            close(conn[n].fd);
            conn[n].fd = -1;
            conn[n].time = 0;
            conn[n].param.tipo = 0;
            conn_attive--;
            conn_libere--;
sprintf(logstr, "  attive:%d libere:%d", conn_attive, conn_libere);
support_log2(logstr);
LogStato(n, conn);
          }
// 101008
          else if(conn[n].param.tipo && ((time(NULL) - conn[n].time) > 20))
          {
sprintf(logstr, "Timeout (%d)", n);
support_log2(logstr);
#if 0
            if(!conn[n].resend)
            {
              conn[n].resend++;
              volto_send(&conn[n]);
            }
            else
#endif
            {
//support_log("Volto: Timeout (chiusura)");
sprintf(logstr, "  chiusura %d", n);
support_log2(logstr);
              volto_reply(&conn[n].param, 2);	// errore
              /* visto che non ho ricevuto risposta, per sicurezza
                 chiudo proprio la connessione. */
              shutdown(conn[n].fd, 2);
              close(conn[n].fd);
              conn[n].fd = -1;
              conn[n].time = 0;
              conn[n].param.tipo = 0;
              conn_attive--;
sprintf(logstr, "  attive:%d libere:%d", conn_attive, conn_libere);
support_log2(logstr);
LogStato(n, conn);
            }
          }
        }
      }
      continue;
    }
    
    if(FD_ISSET(volto_localfd[1], &fdr))
    {
      // Richiesta riconoscimento
      read(volto_localfd[1], &param, sizeof(volto_param));
      
      // Verifica che lo stesso terminale non sia già impegnato
      for(f=0; (f<config.VoltoMaxConn) &&
               ((conn[f].param.tipo != VOLTO_IDFY_LARA) || (conn[f].param.lara.nodo != param.lara.nodo)) ; f++);
      if(f < config.VoltoMaxConn)
      {
        volto_reply(&param, 2);	// errore
sprintf(logstr, "richiesta duplicata su terminale %d", param.lara.nodo);
support_log2(logstr);
        continue;
      }
      
      // Cerca una connessione disponibile
#if 0
for(f=0; f<config.VoltoMaxConn; f++)
{
  printf("# %d %d %d\n", f, conn[f].param.tipo, conn[f].time);
}
#endif
      for(f=0; (f<config.VoltoMaxConn) && ((conn[f].time <= 0) || conn[f].param.tipo); f++);
      
      if(f < config.VoltoMaxConn)
      {
sprintf(logstr, "richiesta %d", f);
support_log2(logstr);
        memcpy(&(conn[f].param), &param, sizeof(volto_param));
        conn[f].resend = 0;
        conn_libere--;
sprintf(logstr, "  attive:%d libere:%d", conn_attive, conn_libere);
support_log2(logstr);
support_log("Volto: Invio richiesta");
        volto_send(&conn[f]);
        // attiva una nuova connessione se serve
        for(f++; (f<config.VoltoMaxConn) && ((conn[f].time <= 0) || conn[f].param.tipo); f++);
        if(f == config.VoltoMaxConn)
        {
          // non c'è una connessione libera
          for(f=0; (f<config.VoltoMaxConn) && conn[f].time; f++);
          if(f < config.VoltoMaxConn)
            conn[f].time = -1;
        }
      }
      else
      {
support_log2("Limite connessioni");
sprintf(logstr, "  MinConn %d  MaxConn %d  time %ld", config.VoltoMinConn, config.VoltoMaxConn, time(NULL));
support_log2(logstr);
for(f=0; f<config.VoltoMaxConn; f++)
{
sprintf(logstr, "  %d %d tipo %d time %d", f, conn[f].fd, conn[f].param.tipo, conn[f].time);
support_log2(logstr);
}
        /*
           Il meccanismo di apertura e chiusura delle connessioni garantisce
           che ci siano sempre connessioni libere a meno che non siano tutte
           occupate. Quindi se arrivo qui è perché sono tutte occupate e posso
           solo dare l'errore, non ha senso tentare di aprire nuove connessioni.
        */
        volto_reply(&param, 2);	// errore
      }
    }
    
//printf("*7\n");
    for(f=0; f<config.VoltoMaxConn; f++)
    {
      if(conn[f].time == -2)
      {
        // Verifica la connessione
        if(FD_ISSET(conn[f].fd, &fdw))
        {
          l = sizeof(int);
          getsockopt(conn[f].fd, SOL_SOCKET, SO_ERROR, &n, &l);
          if(n)
          {
            // Errore connessione
support_log2("Errore connessione");
            conn[f].time = -3;
            close(conn[f].fd);
// 101008
            conn[f].fd = -1;
            conn[f].param.tipo = 0;
LogStato(f, conn);
          }
          else
          {
//printf("*5\n");
            conn[f].time = time(NULL);
            conn_attive++;
            conn_libere++;
            if(conn_attive == 1) support_log("Volto: connesso.");
l = sizeof(struct sockaddr);
getsockname(conn[f].fd, (struct sockaddr*)&sap, &l);
sprintf(logstr, "    %d -> %d", f, ntohl(sap.sin_port)>>16);
support_log2(logstr);
sprintf(logstr, "  attive:%d libere:%d", conn_attive, conn_libere);
support_log2(logstr);
          }
        }
      }
      else if(conn[f].time > 0)
      {
        // Connessione attiva
        if(FD_ISSET(conn[f].fd, &fdr))
        {
          n = read(conn[f].fd, &reply, sizeof(volto_pkt));
          if(n <= 0)
          {
sprintf(logstr, "  chisura da remoto (%d)", f);
support_log2(logstr);
            // connessione interrotta
            shutdown(conn[f].fd, 2);
            close(conn[f].fd);
            conn[f].fd = -1;
            conn[f].time = 0;
            if(conn[f].param.tipo)
            {
              // la connessione non era già libera per via della richiesta in corso
              volto_reply(&conn[f].param, 2);	// errore
              conn[f].param.tipo = 0;
            }
            else
              conn_libere--;
            conn_attive--;
sprintf(logstr, "  attive:%d libere:%d", conn_attive, conn_libere);
support_log2(logstr);
            if(!conn_attive)
            {
              support_log("Volto: disconnesso.");
support_log2("disconnesso");
            }
LogStato(f, conn);
          }
          else
          {
sprintf(logstr, "ricevuto su %d", f);
support_log2(logstr);
            // risposta dal server
            if(volto_decrypt(&reply) && (reply.seq == conn[f].seq))
            {
sprintf(logstr, "esito (%d): %d %d", f, reply.code, reply.data[0]);
support_log2(logstr);
              if(reply.code == 127)
              {
                // imposta chiave
                FILE *fp;
                memcpy(volto_key, reply.data, 24);
#if 0
#warning Dump messaggi
{
int xx;
for(xx=0; xx<24; xx++) printf("%02x", volto_key[xx]);
printf("\n");
}
#endif
                if((fp = fopen(ADDROOTDIR(VOLTO_KEY_FILENAME), "w")))
                {
                  fwrite(volto_key, 1, 24, fp);
                  fclose(fp);
                  sync();
                }
                aes_set_key(&volto_aes_ctx, volto_key, 24);
                volto_send(&conn[f]);
              }
              else
              {
                // gestione risposta
                switch(reply.data[0])
                {
                  case 1: case 2:
                    volto_reply(&conn[f].param, 1);	// ok
                    break;
                  case 124: case 125: case 127:
                    volto_reply(&conn[f].param, 2);	// errore (non riconosciuto)
                    break;
                  case 126:
                    volto_reply(&conn[f].param, 3);	// errore (non presente in anagrafica)
                    break;
                  default:
                    break;
                }
                conn[f].param.tipo = 0;
                conn[f].time = time(NULL);
                conn_libere++;
sprintf(logstr, "  attive:%d libere:%d", conn_attive, conn_libere);
support_log2(logstr);
              }
            }
          }
        }
      }
    }
  }
  
  return NULL;
}

void volto_init()
{
  int i;
  FILE *fp;
  pthread_t pth;
  
  if(!_lara || volto_active) return;
  
  for(i=0; i<=LARA_N_TERMINALI; i++)
    if(_lara->terminale[i].stato.s.volto) break;
  
  /* Se non ci sono terminali che richiedono il riconoscimento
     del volto, allora non esegue l'inizializzazione. */
  if(i > LARA_N_TERMINALI) return;
  
  if(socketpair(AF_UNIX, SOCK_DGRAM, 0, volto_localfd)) return;

  volto_active = 1;
  memset(volto_key, '1', 24);
  if((fp = fopen(ADDROOTDIR(VOLTO_KEY_FILENAME), "r")))
  {
    fread(volto_key, 1, 24, fp);
    fclose(fp);
  }
  aes_set_key(&volto_aes_ctx, volto_key, 24);
  
  pthread_create(&pth, NULL, volto_loop, NULL);
  pthread_detach(pth);
}
