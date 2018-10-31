#include "protocol.h"
#include "database.h"
#include "support.h"
#include "user.h"
#include "serial.h"
#include "timeout.h"
#include "gsm.h"
#include "master.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#ifdef __CRIS__
#include <asm/saetmm.h>
#endif
#include <fcntl.h>
#include "aes.h"
#include <net/if.h>

#define REM_CMD	0
#define REM_ACK	1
#define REM_CHK	2

#define REM_TIMEOUT	100

struct remote_periph {
  struct remote_periph *next;
  int periph;
  unsigned char sens;
};

struct remote_data {
  struct remote_periph *periph_list;
  int actuator;
  int connected;
  int phonenum;
  int modem;
  timeout_t *timeout;
  sem_t sem;
  struct sockaddr_in sa;
};

#define REMOTE ((struct remote_data*)((dev->prot_data)))

static pthread_mutex_t remote_mutex = PTHREAD_MUTEX_INITIALIZER;
static int remote_current_conn = -1;
static int remote_trying_conn = -1;
static char remote_consumer[MAX_NUM_CONSUMER] = {0, };

static const unsigned char REM_Check[] = {STX, REM_CHK, 0, 0, 0, -(STX+REM_CHK)};

static unsigned char remote_cks(unsigned char *msg, int len)
{
  int i;
  unsigned char cks = 0;
  
  for(i=0; i<len; i++) cks += msg[i];
  return -cks;
}

static void* remote_modem_disconnect(ProtDevice *dev)
{
  usleep(1100000);
  write(dev->fd, "+++", 3);
  usleep(1100000);
  write(dev->fd, "AT\r", 3);
  usleep(100000);
  write(dev->fd, "ATH0\r", 5);
  REMOTE->connected = 0;
  return NULL;
}

static int remote_timeout(ProtDevice *dev, int null)
{
  pthread_t pth;
  char m[64];
  
  sprintf(m, "(remote %d) Timeout", dev->consumer);
  support_log(m);
  
  database_lock();
  OFFA(REMOTE->actuator, REMOTE->actuator >= 0);
  database_unlock();
  
  pthread_mutex_lock(&remote_mutex);
  if(remote_current_conn == dev->consumer) remote_current_conn = -1;
  
  if(config.consumer[dev->consumer].configured == 5)
  {
    /* se la rete va in timeout prova subito a connettersi con il backup configurato */
    if(remote_trying_conn < 0)
      for(remote_trying_conn=dev->consumer-1;
          (remote_trying_conn>=0) && (!remote_consumer[remote_trying_conn]) &&
           (remote_trying_conn>remote_current_conn); remote_trying_conn--);
    pthread_mutex_unlock(&remote_mutex);
    shutdown(dev->fd, 2);
    close(dev->fd);
    REMOTE->connected = 0;
  }
  else if(REMOTE->modem)
  {
    pthread_mutex_unlock(&remote_mutex);
    REMOTE->connected = -1;    
    pthread_create(&pth, NULL, (PthreadFunction)remote_modem_disconnect, dev);
    pthread_detach(pth);
  }
  else
  {
    pthread_mutex_unlock(&remote_mutex);
    REMOTE->connected = 0;
  }
  return 0;
}

static void* remote_timeout_dialing_th(ProtDevice *dev)
{
  char m[64];
  
  sprintf(m, "(remote %d) Calling timeout", dev->consumer);
  support_log(m);
  
  gsm_poweron(dev->fd);
  REMOTE->connected = 0;
  
  pthread_mutex_lock(&remote_mutex);
  if(remote_trying_conn >= 0)
    for(remote_trying_conn--;
        (remote_trying_conn>=0) && (!remote_consumer[remote_trying_conn]) &&
         (remote_trying_conn>remote_current_conn); remote_trying_conn--);
  if(remote_current_conn == dev->consumer) remote_current_conn = -1;
  pthread_mutex_unlock(&remote_mutex);
  
  return NULL;
}

static int remote_timeout_dialing(ProtDevice *dev, int null)
{
  pthread_t pth;
  pthread_create(&pth, NULL, (PthreadFunction)remote_timeout_dialing_th, dev);
  pthread_detach(pth);
  return 0;
}

static void remote_send_status(ProtDevice *dev)
{
  struct remote_periph *rp;
  unsigned char sens;
  int i;
  char msg[6];
  
  rp = REMOTE->periph_list;
  
  msg[0] = STX;
  msg[1] = REM_CMD;
  
  while(rp)
  {
    sens = 0;
    for(i=(rp->periph<<3); i<(rp->periph<<3)+8;i++)
    {
      sens >>= 1;
      if(SE[i] & 0x01) sens |= 0x80;
    }
    
    if(!master_periph_present(rp->periph))
      sens = 0xff;
    
    *(short*)(msg+2) = rp->periph;
    msg[4] = sens;
    msg[5] = remote_cks(msg, 5);
    write(dev->fd, msg, 6);
    
    rp = rp->next;
  }
}

static const char *mdm_nocarrier = "NO CARRIER";
static const char *mdm_connect = "CONNECT";

static int remote_check_modem(ProtDevice *dev, char *buf, int idx)
{
  char m[64];
  
  idx++;
  if((idx < 7) && !strncmp(buf, mdm_connect, idx)) return idx;
  if((idx < 10) && !strncmp(buf, mdm_nocarrier, idx)) return idx;
  
  if((idx == 7) && !strncmp(buf, mdm_connect, idx))
  {
//printf("Connect\n");
    sprintf(m, "(remote %d) Modem Connect", dev->consumer);
    support_log(m);
    
    pthread_mutex_lock(&remote_mutex);
    if(remote_current_conn >= 0)
      ((struct remote_data*)(config.consumer[remote_current_conn].dev->prot_data))->connected = 0;
    remote_current_conn = dev->consumer;
    remote_trying_conn = -1;
    pthread_mutex_unlock(&remote_mutex);
    
    REMOTE->connected = 1;
    ONAT(REMOTE->actuator, REMOTE->actuator >= 0);
    remote_send_status(dev);
    timeout_on(REMOTE->timeout, (timeout_func)remote_timeout, dev, 0, REM_TIMEOUT);
    return 0;
  }
  else if((idx == 10) && !strncmp(buf, mdm_nocarrier, idx))
  {
//printf("No carrier\n");
    sprintf(m, "(remote %d) Modem No Carrie", dev->consumer);
    support_log(m);
    REMOTE->connected = 0;
    database_lock();
    OFFA(REMOTE->actuator, REMOTE->actuator >= 0);
    database_unlock();
    timeout_off(REMOTE->timeout);
    
    pthread_mutex_lock(&remote_mutex);
    if(remote_trying_conn >= 0)
      for(remote_trying_conn--;
          (remote_trying_conn>=0) && (!remote_consumer[remote_trying_conn]) &&
           (remote_trying_conn>remote_current_conn); remote_trying_conn--);
    if(remote_current_conn == dev->consumer) remote_current_conn = -1;
    pthread_mutex_unlock(&remote_mutex);
    
    return 0;
  }
  else
  {
    buf[0] = buf[idx-1];
    return 1;
  }
}

#include <errno.h>

static void remote_loop(ProtDevice *dev)
{
  int i, ret;
  char *tparam, *tps, oldch, sens;
  struct remote_periph *trp;
  char modem_buf[64], log[64];
  int modem_idx;
  char serial[] = "/dev/ttySx";
  static int one = 1;
  
  if(!config.consumer[dev->consumer].param) return;
  tparam = strdup(config.consumer[dev->consumer].param);
  if(!tparam) return;
  
  dev->prot_data = malloc(sizeof(struct remote_data));
  REMOTE->periph_list = NULL;
  REMOTE->actuator = -1;
  REMOTE->connected = 0;
  REMOTE->phonenum = -1;
  REMOTE->modem = 0;
  REMOTE->timeout = timeout_init();
  sem_init(&(REMOTE->sem), 0, 0);
  
  tps = tparam;
  
  /* lettura periferiche da remotizzare */
  do
  {
    for(i=0; tps[i] && (tps[i]!=',') && (tps[i]!=':'); i++);
    oldch = tps[i];
    tps[i] = '\0';
    if(sscanf(tps, "%d", &ret))
    {
      trp = malloc(sizeof(struct remote_periph));
      if(!trp)
      {
        free(tparam);
        REMOTE->periph_list = NULL;
        return;
      }
      trp->next = REMOTE->periph_list;
      REMOTE->periph_list = trp;
      trp->periph = ret;
      trp->sens = 0;
    }
    tps += i+1;
  } while (oldch == ',');
  
  /* lettura attuatore stato linea */
  for(i=0; tps[i] && (tps[i]!=':'); i++);
  oldch = tps[i];
  tps[i] = '\0';
  sscanf(tps, "%d", &REMOTE->actuator);
  tps += i+1;
  
     
  if(config.consumer[dev->consumer].configured == 5)
  {
    /* lettura indirizzo IP */
    if(oldch == ':')
    {
//printf("Conf: %d %s\n", config.consumer[dev->consumer].data.eth.port, tps);
      REMOTE->sa.sin_family = AF_INET;
      REMOTE->sa.sin_port = htons(config.consumer[dev->consumer].data.eth.port);
      REMOTE->sa.sin_addr.s_addr = inet_addr(tps);
      REMOTE->phonenum = 0;	// indico che e' master
    }
    else
    {
      dev->fd_eth = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
      if(dev->fd_eth < 0) return;
      setsockopt(dev->fd_eth, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(int));
      REMOTE->sa.sin_family = AF_INET;
      REMOTE->sa.sin_port = htons(config.consumer[dev->consumer].data.eth.port);
      REMOTE->sa.sin_addr.s_addr = INADDR_ANY;
      if(bind(dev->fd_eth, (struct sockaddr*)&REMOTE->sa, sizeof(struct sockaddr_in)))
      {
        close(dev->fd_eth);
        return;
      }
      listen(dev->fd_eth, 0);
    }
  }
  else
  {
    /* lettura numero di rubrica */
    if(oldch == ':')
    {
      /* Se non e' un gsm ma un modem normale, non capita nulla */
      gsm_poweron(dev->fd);
      
      sscanf(tps, "%d", &REMOTE->phonenum);
      serial[9] = '0' + config.consumer[dev->consumer].configured - 1;
      REMOTE->modem = prot_test_modem(serial);
      if((REMOTE->modem == MODEM_GSM) || (REMOTE->modem == MODEM_GPRS))
      {
        do
        {
          sleep(1);
          ser_send_recv(dev->fd, "AT\r", NULL);
          tps = ser_send_recv(dev->fd, "AT+CREG\r", modem_buf);
          sprintf(log, "(remote %d) CREG: %c", dev->consumer, tps[9]);
          support_log(log);
        }
        while(strncmp(tps, "+CREG: 0,1", 10));
      }
      usleep(50000);
      write(dev->fd, "ATX0S0=1\r", 9);
    }
    else
    {
      REMOTE->connected = 1;
      remote_send_status(dev);
      ONAT(REMOTE->actuator, REMOTE->actuator >= 0);
    }
  }
  
  free(tparam);
  remote_consumer[dev->consumer] = 1;
//printf("Consumatore %d registrato\n", dev->consumer);
  
  /* Attende la partenza di tutti i plugin di remotizzazione */
  sleep(1);
  
  while(1)
  {
    if(!REMOTE->connected)
    {
      database_lock();
      OFFA(REMOTE->actuator, REMOTE->actuator >= 0);
      database_unlock();
    }

    /* solo i master devono aspettare */
    if(config.consumer[dev->consumer].configured == 5)
    {
    if(REMOTE->phonenum >= 0)
    {
      sem_wait(&(REMOTE->sem));
    }
    /* se e' uno slave ethernet aspetto la connessione */
    else
    {
      timeout_off(REMOTE->timeout);
      if(dev->fd >= 0) close(dev->fd);
      dev->fd = accept(dev->fd_eth, NULL, NULL);
      
      sprintf(log, "(remote %d) Connected (accept)" , dev->consumer);
      support_log(log);
      
      REMOTE->connected = 1;
      remote_send_status(dev);
      ONAT(REMOTE->actuator, REMOTE->actuator >= 0);
      timeout_on(REMOTE->timeout, (timeout_func)remote_timeout, dev, 0, REM_TIMEOUT);
      
      pthread_mutex_lock(&remote_mutex);
      if(remote_current_conn >= 0)
        ((struct remote_data*)(config.consumer[remote_current_conn].dev->prot_data))->connected = 0;
      remote_current_conn = dev->consumer;
      remote_trying_conn = -1;
      pthread_mutex_unlock(&remote_mutex);
    }
    }
    
  i = 0;
  modem_idx = 0;
  while(1)
  {
    ret = read(dev->fd, dev->buf+i, 1);
    if(ret < 1)
    {
      if(config.consumer[dev->consumer].configured == 5)
      {
        pthread_mutex_lock(&remote_mutex);
        if(remote_current_conn == dev->consumer) remote_current_conn = -1;
        pthread_mutex_unlock(&remote_mutex);
        REMOTE->connected = 0;
        database_lock();
        OFFA(REMOTE->actuator, REMOTE->actuator >= 0);
        database_unlock();
        break;
      }
      else
        continue;
    }
//if(isalnum(dev->buf[i])) {printf("%c", dev->buf[i]); fflush(stdout);}
    
    modem_buf[modem_idx] = dev->buf[i];
    modem_idx = remote_check_modem(dev, modem_buf, modem_idx);
    
    if(!i && (dev->buf[0] != STX)) continue;
    i++;
    if(i < 6) continue;
    i = 0;
    if(remote_cks(dev->buf, 6)) continue;
    
    timeout_on(REMOTE->timeout, (timeout_func)remote_timeout, dev, 0, REM_TIMEOUT);
    
    switch(dev->buf[1])
    {
      case REM_CMD:
        /* esegue l'attuazione relativa */
        trp = REMOTE->periph_list;
        while(trp)
        {
          if(trp->periph == *(short*)(dev->buf+2))
          {
            sens = dev->buf[4];
//printf("Ricevuto: %02x\n", sens);
            database_lock();
            for(i=(trp->periph<<3); i<(trp->periph<<3)+8; i++)
            {
              EQAT(i, (~sens) & 0x01);
              sens >>= 1;
            }
            database_unlock();
            i = 0;
            trp = NULL;
          }
          else
            trp = trp->next;
        }
        
        dev->buf[1] = REM_ACK;
        dev->buf[5]--;
        write(dev->fd, dev->buf, 6);
        break;
      case REM_ACK:
        /* cerca periferica in elenco e aggiorna sens */
        trp = REMOTE->periph_list;
        while(trp)
        {
          if(trp->periph == *(short*)(dev->buf+2))
          {
            trp->sens = dev->buf[4];
            trp = NULL;
          }
          else
            trp = trp->next;
        }
        break;
      case REM_CHK:
        /* azzera timeout, se scade connesso=0 */
        /* verificare timeout con tempi gsm */
        break;
      default:
        break;
    }
  }
  }
}


static void* remote_eth_connect(ProtDevice *dev)
{
  int res;
  char m[64];
  
  sprintf(m, "(remote %d) Connecting" , dev->consumer);
  support_log(m);
  dev->fd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
  res = connect(dev->fd, (struct sockaddr*)&REMOTE->sa, sizeof(struct sockaddr));
//printf("Connect: %d %d\n", res, errno);
  if(res < 0)
  {
    sprintf(m, "(remote %d) Connect failure" , dev->consumer);
    support_log(m);
    REMOTE->connected = 0;
    
    pthread_mutex_lock(&remote_mutex);
    for(remote_trying_conn--;
          (remote_trying_conn>=0) && (!remote_consumer[remote_trying_conn]) &&
           (remote_trying_conn>remote_current_conn); remote_trying_conn--);
    if(remote_trying_conn == remote_current_conn) remote_trying_conn = -1;
    pthread_mutex_unlock(&remote_mutex);
    
    return 0;
  }
  
  sprintf(m, "(remote %d) Connected" , dev->consumer);
  support_log(m);
  pthread_mutex_lock(&remote_mutex);
  if(remote_current_conn >= 0)
    ((struct remote_data*)(config.consumer[remote_current_conn].dev->prot_data))->connected = 0;
  remote_current_conn = dev->consumer;
  remote_trying_conn = -1;
  pthread_mutex_unlock(&remote_mutex);
  
  sem_post(&(REMOTE->sem));
  REMOTE->connected = 1;
  ONAT(REMOTE->actuator, REMOTE->actuator >= 0);
  remote_send_status(dev);
  timeout_on(REMOTE->timeout, (timeout_func)remote_timeout, dev, 0, REM_TIMEOUT);
  
  return NULL;
}

static int remote_timer(ProtDevice *dev, int param)
{
#warning Solo controllo stato.
/* Qui faccio il controllo solo sullo stato. Per la periferica di campo remota
   si puo' prima fare il controllo dei fronti e dopo quello degli stati.
   Se becco il fronte bene, altrimenti ho comunque lo stato.
   Non dovrebbero interferire */
  
  struct remote_periph *rp;
  unsigned char sens;
  int i, inviato = 0;
  char msg[6];
  char modem_buf[32], m[64];
  pthread_t pth;
  
  /* Attivita' secondo priorita'. La priorita' e': LAN, seriale, GSM. */
/*
printf("(%d)", dev->consumer);
if(REMOTE) printf(" connected=%d remote_trying_conn=%d master=%d", REMOTE->connected, remote_trying_conn, REMOTE->phonenum);
printf("\n");
*/
  
  /* Connessione in corso */
  if(!REMOTE || (REMOTE->connected < 0)) return 0;
  
  if(!REMOTE->connected)
  {
    if(REMOTE->phonenum >= 0)
    {
#warning LAN -> GSM -> seriale
    pthread_mutex_lock(&remote_mutex);
    
    /* trova il prossimo consumatore che si deve connettere */
    if(remote_trying_conn < 0)
      for(remote_trying_conn=MAX_NUM_CONSUMER-1;
          (remote_trying_conn>=0) && (!remote_consumer[remote_trying_conn]) &&
           (remote_trying_conn>remote_current_conn); remote_trying_conn--);
    
    if(remote_trying_conn == remote_current_conn)
      remote_trying_conn = -1;
    else if(remote_trying_conn == dev->consumer)
    {
      /* Prova a connettersi */
      if(config.consumer[dev->consumer].configured == 5)
      {
//printf("LAN Trying...\n");
        REMOTE->connected = -1;
        remote_trying_conn = dev->consumer;
        pthread_create(&pth, NULL, (PthreadFunction)remote_eth_connect, dev);
        pthread_detach(pth);
      }
      else
      {
//printf("GSM Trying...\n");
        REMOTE->connected = -1;
        remote_trying_conn = dev->consumer;
        sprintf(m, "(remote %d) Calling" , dev->consumer);
        support_log(m);
        timeout_on(REMOTE->timeout, (timeout_func)remote_timeout_dialing, dev, 0, 600);
        sprintf(modem_buf, "ATD%s\r", config.PhoneBook[REMOTE->phonenum].Phone);
        write(dev->fd, modem_buf, strlen(modem_buf));
      }
    }
    
    pthread_mutex_unlock(&remote_mutex);
    }
    return 0;
  }
  
  rp = REMOTE->periph_list;
  
  msg[0] = STX;
  msg[1] = REM_CMD;
  
  while(rp)
  {
    sens = 0;
    for(i=(rp->periph<<3); i<(rp->periph<<3)+8;i++)
    {
      sens >>= 1;
      if(SE[i] & 0x01) sens |= 0x80;
    }
    
    if(!master_periph_present(rp->periph))
      sens = 0xff;
    
    if(sens != rp->sens)
    {
//printf("Inviato: %02x\n", sens);
      inviato = 1;
      *(short*)(msg+2) = rp->periph;
      msg[4] = sens;
      msg[5] = remote_cks(msg, 5);
      write(dev->fd, msg, 6);
    }
    
    rp = rp->next;
  }
  
  if(!inviato) write(dev->fd, REM_Check, 6);
  
  return 1;
}

struct remote2_data {
  int mm, fdmm, i, init;
  unsigned char buf[64];
  struct sockaddr_in sa;
  struct {
    int num, fd, i, timeout;
    unsigned char buf[64];
  } perif[32];
  struct aes_ctx aes;
};

#define MASTER_PERIF_TIMEOUT	20

#define REMOTE2 ((struct remote2_data*)((dev->prot_data)))

static const unsigned char remote2_cmd_dim[] = {
	 1, 1, 1,17, 2, 2, 2, 2, 5, 1,
	 1, 1, 1, 3, 4, 8, 5, 7, 3, 4,
	 3, 3,10, 3, 1, 1, 2, 9, 2, 2,
	 5,18, 4, 4};

static void remote2_slave_parse(struct remote2_data *data)
{
  int i, crc;
  unsigned char buf[16], bufout[16];
  
  while((data->i > 0) && (data->i >= remote2_cmd_dim[data->buf[0]]))
  {
    switch(data->buf[0])
    {
      case 2:
        /* Emulazione solo della periferica 0 */
        data->buf[1] = 0xf2;
        memset(data->buf+2, 0xff, 15);
        write(data->fdmm, data->buf, 17);
        break;
      case 3:
        buf[0] = 4;
        for(i=1; i<32; i++)
        {
          if(i&1)
          {
            if((data->buf[(i/2)+1] & 0xf0) != 0xf0)
            {
              buf[1] = i;
              write(data->fdmm, buf, 2);
            }
          }
          else
          {
            if((data->buf[(i/2)+1] & 0x0f) != 0x0f)
            {
              buf[1] = i;
              write(data->fdmm, buf, 2);
            }
          }
        }
        data->init = 1;
        break;
      case 7:
      case 8:
        /* Emulazione solo della periferica 0 */
        if(data->buf[1] == 0)
        {
          /* Inoltro */
          data->buf[1] = config.DeviceID;
          if(data->perif[0].fd >= 0)
          {
            buf[0] = remote2_cmd_dim[data->buf[0]];
            memcpy(buf+1, data->buf, buf[0]);
            buf[13] = time(NULL);
            crc = CRC16(buf, 14);
            memcpy(buf+14, &crc, 2);
            aes_encrypt(&(data->aes), bufout, buf);
            write(data->perif[0].fd, bufout, 16);
          }
        }
        break;
      default:
        break;
    }
    
    memmove(data->buf, data->buf+remote2_cmd_dim[data->buf[0]], data->i-remote2_cmd_dim[data->buf[0]]);
    data->i -= remote2_cmd_dim[data->buf[0]];
  }
}

static void remote2_slave(struct remote2_data *data)
{
  /* Questa centrale si presenta come periferica 0 di tipo SC8.
     La master la vedrà come periferica pari all'id impianto. */
  int n, i, at, code, crc, connesso;
  fd_set fds;
  struct timeval to;
  unsigned char buf[16], bufout[16];
  
  connesso = 1;
  data->init = 0;
  
  while(1)
  {
    data->perif[0].fd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
//support_logmm("connessione");
    if(connect(data->perif[0].fd, (struct sockaddr*)&(data->sa), sizeof(struct sockaddr)))
    {
//support_logmm("errore");
      close(data->perif[0].fd);
      data->perif[0].fd = -1;
      sleep(1);
      
      FD_ZERO(&fds);
      FD_SET(data->fdmm, &fds);
      to.tv_sec = 0;
      to.tv_usec = 0;
      select(data->fdmm+1, &fds, NULL, NULL, &to);
      if(FD_ISSET(data->fdmm, &fds))
      {
        i = read(data->fdmm, data->buf+data->i, 64-data->i);
        if(i > 0)
        {
          data->i += i;
          remote2_slave_parse(data);
        }
      }
      
      if(connesso)
      {
//support_logmm("manomissione periferica");
        connesso = 0;
        /* La mancata comunicazione con la master viene segnalata tramite manomissione periferica */
        buf[0] = 4;
        buf[1] = 0;
        write(data->fdmm, buf, 2);
      }
      
      continue;
    }
    
//support_logmm("connesso");
    if(!connesso)
    {
//support_logmm("ripsristino periferica");
      connesso = 1;
      /* Ripristino periferica */
      buf[0] = 5;
      buf[1] = 0;
      write(data->fdmm, buf, 2);
    }
    
    /* Connesso, invio il messaggio di presentazione */
    buf[0] = 2;
    buf[1] = 7;	// richiesta stato ingressi
    buf[2] = config.DeviceID;
    buf[13] = time(NULL);
    crc = CRC16(buf, 14);
    memcpy(buf+14, &crc, 2);
    aes_encrypt(&(data->aes), bufout, buf);
    write(data->perif[0].fd, bufout, 16);
    
    data->perif[0].i = 0;
    data->perif[0].timeout = 0;
    
    n = data->perif[0].fd;
    if(data->fdmm > n) n = data->fdmm;
    n++;
    
    to.tv_sec = 5;
    to.tv_usec = 0;
    while(1)
    {
      FD_ZERO(&fds);
      if(data->init) FD_SET(data->perif[0].fd, &fds);
      FD_SET(data->fdmm, &fds);
      i = select(n, &fds, NULL, NULL, &to);
      if(i == 0)
      {
//support_logmm("timeout");
        to.tv_sec = 5;
        to.tv_usec = 0;
        /* Timeout - invio stato in vita */
        if(data->perif[0].timeout)
        {
//support_logmm("-> chiusura");
          /* Non ho ricevuto la conferma all'invio precedente, chudo */
          shutdown(data->perif[0].fd, 2);
          close(data->perif[0].fd);
          break;
        }
        else
        {
//support_logmm("-> polling");
          buf[0] = 1;
          buf[1] = 0;
          buf[13] = time(NULL);
          crc = CRC16(buf, 14);
          memcpy(buf+14, &crc, 2);
          aes_encrypt(&(data->aes), bufout, buf);
          write(data->perif[0].fd, bufout, 16);
          data->perif[0].timeout = 1;
        }
      }
      if(FD_ISSET(data->perif[0].fd, &fds))
      {
        /* Dal master... */
        i = read(data->perif[0].fd, data->perif[0].buf+data->perif[0].i, 64-data->perif[0].i);
        if(i > 0)
        {
          data->perif[0].i += i;
          //while((data->perif[0].i > 0) && (data->perif[0].i >= remote2_cmd_dim[data->perif[0].buf[0]]))
          while(data->perif[0].i >= 16)
          {
            aes_decrypt(&(data->aes), bufout, data->perif[0].buf);
            crc = CRC16(bufout, 14);
            if(!memcmp(&crc, bufout+14, 2))
            {
            memcpy(data->perif[0].buf, bufout+1, bufout[0]);
            
            /* L'indirizzo periferica deve coincidere con il numero impianto,
               ma non sto a controllare... */
            code = data->perif[0].buf[0];
            switch(code)
            {
              case 0:
                data->perif[0].timeout = 0;
                break;
              case 7:
                /* Richiesta stato sensori -> attuatori */
                at = 0;
                for(i=0; i<8; i++)
                  if(AT[(data->mm<<8)+i] & bitON) at |= (1<< i);
                /* Trasformo la richiesta in attuazione */
                buf[0] = 5;
                buf[1] = 8;
                buf[2] = config.DeviceID;
                buf[3] = 0x80;
                buf[4] = (at>>4)|(at<<4);
                buf[5] = 0;
                buf[13] = time(NULL);
                crc = CRC16(buf, 14);
                memcpy(buf+14, &crc, 2);
                aes_encrypt(&(data->aes), bufout, buf);
                write(data->perif[0].fd, bufout, 16);
                break;
              case 8:
                /* Comando attuatori -> ingesso sensori */
                if(data->perif[0].buf[2] & 0x80)
                {
                  /* Trasformo l'attuazione in ingresso sensori */
                  buf[0] = 7;
                  buf[1] = 0;
                  buf[2] = 0x80;
                  /* Inverto i nibble di attuazione - NO */
                  //buf[3] = (data->perif[0].buf[3]>>4)|(data->perif[0].buf[3]<<4);
                  buf[3] = data->perif[0].buf[3];
                  write(data->fdmm, buf, 4);
                }
                break;
              default:
                break;
            }
            }
            
#if 0
            memmove(data->perif[0].buf, data->perif[0].buf+remote2_cmd_dim[code],
              data->perif[0].i-remote2_cmd_dim[code]);
            data->perif[0].i -= remote2_cmd_dim[code];
#else
            memmove(data->perif[0].buf, data->perif[0].buf+16, data->perif[0].i-16);
            data->perif[0].i -= 16;
#endif
          }
        }
        else
        {
          /* Chiusura socket */
          shutdown(data->perif[0].fd, 2);
          close(data->perif[0].fd);
          break;
        }
      }
      if(FD_ISSET(data->fdmm, &fds))
      {
        /* Dalla centrale... */
        i = read(data->fdmm, data->buf+data->i, 64-data->i);
        if(i > 0)
        {
          data->i += i;
          remote2_slave_parse(data);
        }
      }
    }
  }
}

static int remote2_master_parse(struct remote2_data *data)
{
  int i, crc;
  unsigned char buf[16], bufout[16];
  
  if((data->i < 1) || (data->i < remote2_cmd_dim[data->buf[0]])) return 0;
  
  switch(data->buf[0])
  {
    case 2:
      /* Richiesta periferiche presenti */
      memset(data->buf+1, 0x22, 16);
      write(data->fdmm, data->buf, 17);
      break;
    case 3:
      /* Invio periferiche previste - imposto dei timeout per le periferiche previste */
      data->init = 1;
      /* La periferica 0 non deve essere usata */
      if((data->buf[1] & 0x0f) != 0x0f)
      {
        buf[0] = 4;
        buf[1] = 0;
        write(data->fdmm, buf, 2);
      }
      /* Per le altre imposto il timeout per dare tempo alla connessione di attivarsi
         senza segnalare subito la manomissione periferica */
      for(i=1; i<32; i++)
      {
        if(i&1)
        {
          if((data->buf[(i/2)+1] & 0xf0) != 0xf0)
          {
            data->perif[i].num = i;
            data->perif[i].timeout = MASTER_PERIF_TIMEOUT;
          }
        }
        else
        {
          if((data->buf[(i/2)+1] & 0x0f) != 0x0f)
          {
            data->perif[i].num = i;
            data->perif[i].timeout = MASTER_PERIF_TIMEOUT;
          }
        }
      }
      break;
    case 7:
      /* richiesta stato ingressi - da inoltrare a slave */
    case 8:
      /* attuazioni - da inoltrare a slave */
      /* Gestione comune */
      for(i=0; (i<32)&&(data->perif[i].num!=data->buf[1]); i++);
//printf("send %d->#%d\n", data->buf[1], i);
      if((i < 32) && (data->perif[i].fd >= 0))
      {
        /* Inoltro pari pari */
        buf[0] = remote2_cmd_dim[data->buf[0]];
        memcpy(buf+1, data->buf, buf[0]);
        buf[13] = time(NULL);
        crc = CRC16(buf, 14);
        memcpy(buf+14, &crc, 2);
        aes_encrypt(&(data->aes), bufout, buf);
        write(data->perif[i].fd, bufout, 16);
      }
      break;
    default:
      break;
  }
  
  memmove(data->buf, data->buf+remote2_cmd_dim[data->buf[0]], data->i-remote2_cmd_dim[data->buf[0]]);
  data->i -= remote2_cmd_dim[data->buf[0]];
  
  return 1;
}

static int remote2_master_parse_perif(struct remote2_data *data, int idx)
{
  int i, at, code, len, crc;
  unsigned char buf[16], bufout[16];
  
  /* Messaggi in arrivo dallo slave. */
  if(data->perif[idx].i >= 16)
  {
    aes_decrypt(&(data->aes), bufout, data->perif[idx].buf);
    len = bufout[0];
    memcpy(data->perif[idx].buf, bufout+1, len);
    
    crc = CRC16(bufout, 14);
    if(!memcmp(&crc, bufout+14, 2))
      crc = 1;
    else
      crc = 0;
  }
  else
    return 0;
  
//if(!crc) support_logmm("CRC error");
#if 0
{
char log[64];
strcpy(log, "dbg:");
for(i=0; i<len; i++) sprintf(log+4+i*2, "%02x", data->perif[idx].buf[i]);
support_logmm(log);
}
#endif
  
  if(crc)
  {
  
  /* Assumo che tutti i comandi siano in formato modulo master e
     che quindi il secondo byte sia l'indrizzo periferica. */
  if((/*data->perif[idx].i*/len > 1) && (data->perif[idx].num < 0))
  {
    /* Rifiuto la connessione dalla slave 0 */
    if(data->perif[idx].buf[1] == 0)
    {
      shutdown(data->perif[idx].fd, 2);
      close(data->perif[idx].fd);
      data->perif[idx].fd = -1;
      data->perif[idx].num = -1;
      data->perif[idx].i = 0;
      data->perif[idx].timeout = 0;
      return 0;
    }
    
    data->perif[idx].num = data->perif[idx].buf[1];
//support_logmm("connessa periferica");
    
    /* E' una nuova connessione, verifico che non ce ne sia
       già un'altra con lo stesso numero e nel caso chiudo
       la vecchia (potrebbe essere rimasta appesa). */
    for(i=0; i<32; i++)
    {
      if((i != idx) && (data->perif[i].num == data->perif[idx].num))
      {
        if(data->perif[i].fd >= 0)
        {
          shutdown(data->perif[i].fd, 2);
          close(data->perif[i].fd);
        }
        data->perif[i].fd = -1;
        data->perif[i].num = -1;
        data->perif[i].i = 0;
        data->perif[i].timeout = 0;
      }
    }
    
    /* Segnalo il ripristino periferica se è il caso */
    if(!(master_periph_present((data->mm<<5)+data->perif[idx].num)))
    {
      buf[0] = 5;
      buf[1] = data->perif[idx].num;
      write(data->fdmm, buf, 2);
    }
  }
  
#if 0
printf("parse #%d %d\n", idx, data->perif[idx].num);
printf("buf (%d): ", data->perif[idx].i);
for(i=0; i<len; i++) printf("%02x", data->perif[idx].buf[i]);
printf("\n");
#endif

  code = data->perif[idx].buf[0];
//  if(data->perif[idx].i >= remote2_cmd_dim[code])
//  {
    switch(code)
    {
      case 0:
        /* Polling per stato in vita */
        data->perif[idx].timeout = MASTER_PERIF_TIMEOUT;
        buf[0] = 1;
        buf[1] = 0;
        buf[13] = time(NULL);
        crc = CRC16(buf, 14);
        memcpy(buf+14, &crc, 2);
        aes_encrypt(&(data->aes), bufout, buf);
        write(data->perif[idx].fd, bufout, 16);
        break;
      case 7:
//support_logmm("richiesta stato");
        /* Richiesta stato sensori -> attuatori */
        at = 0;
        for(i=0; i<8; i++)
          if(AT[(data->mm<<8)+(data->perif[idx].num<<3)+i] & bitON) at |= (1<< i);
        /* Trasformo la richiesta in attuazione */
        buf[0] = 5;
        buf[1] = 8;
        buf[2] = data->perif[idx].num;
        buf[3] = 0x80;
        buf[4] = (at>>4)|(at<<4);
        buf[5] = 0;
        buf[13] = time(NULL);
        crc = CRC16(buf, 14);
        memcpy(buf+14, &crc, 2);
        aes_encrypt(&(data->aes), bufout, buf);
        write(data->perif[idx].fd, bufout, 16);
        
        /* Forzo anche un allineamento slave->master */
        buf[0] = 2;
        buf[1] = 7;
        crc = CRC16(buf, 14);
        memcpy(buf+14, &crc, 2);
        aes_encrypt(&(data->aes), bufout, buf);
        write(data->perif[idx].fd, bufout, 16);
        break;
      case 8:
        /* Comando attuatori -> ingesso sensori */
        if(data->perif[idx].buf[2] & 0x80)
        {
          /* Trasformo l'attuazione in ingresso sensori */
          buf[0] = 7;
          buf[1] = data->perif[idx].num;
          buf[2] = 0x80;
          /* Inverto i nibble di attuazione */
          //buf[3] = (data->perif[idx].buf[3]>>4)|(data->perif[idx].buf[3]<<4);
          buf[3] = data->perif[idx].buf[3];
          write(data->fdmm, buf, 4);
        }
        break;
      default:
        break;
    }
    
  }	// if crc
#if 0
    memmove(data->perif[idx].buf, data->perif[idx].buf+remote2_cmd_dim[code],
      data->perif[idx].i-remote2_cmd_dim[code]);
    data->perif[idx].i -= remote2_cmd_dim[code];
#else
    memmove(data->perif[idx].buf, data->perif[idx].buf+16, data->perif[idx].i-16);
    data->perif[idx].i -= 16;
#endif
    
    return 1;
//  }
  
//  return 0;
}

static void remote2_loop(ProtDevice *dev)
{
  int i, n1, n2, n3, n4, ret;
  char tparam[32];
  static int one = 1;
  fd_set fds;
  struct timeval to;
  struct ifreq ifr;
  
  if(!config.consumer[dev->consumer].param)
  {
    sleep(1);
    return;
  }
  
  dev->prot_data = malloc(sizeof(struct remote2_data));
  if(!dev->prot_data) return;
  
  REMOTE2->i = 0;
  REMOTE2->mm = 16;
  
  /* lettura parametri - master:n.modulo slave:ip master */
  /* Tiro via tutti i caratteri che non servono */
  sscanf(config.consumer[dev->consumer].param, "%s", tparam);
  ret = sscanf(tparam, "%d:%d.%d.%d.%d", &(REMOTE2->mm), &n1, &n2, &n3, &n4);
  if(REMOTE2->mm > 15)
  {
    sleep(1);
    return;
  }
  
  if(config.consumer[dev->consumer].configured == 5)
  {
    /* lettura indirizzo IP */
    if(ret == 5)
    {
      /* SLAVE */
      if((config.DeviceID == 0) || (config.DeviceID > 31)) return;
      
      /* Occorre assicurare che il processo saet abbia già creato i file
         descriptor per i moduli reali, altrimenti si crea prima il virtuale
         che viene sovrascritto dal modulo reale. */
      sleep(1);
      
      /* Sovrappongo il modulo emulato a quello reale */
      i = open("/dev/pxl", O_RDWR);
      master_register_module(REMOTE2->mm, i);
#ifdef __CRIS__  
      /* Reset MM */
      ioctl(i, MM_RESET, 0);
#endif
      
      REMOTE2->fdmm = open("/dev/pxlmm", O_RDWR);
      write(REMOTE2->fdmm, "\033\013\000\000\000\001", 7);	// versione 11.0.0.0 e modulo master tipo 0
      
      for(i=0; tparam[i]!=':'; i++);
      REMOTE2->sa.sin_family = AF_INET;
      REMOTE2->sa.sin_port = htons(config.consumer[dev->consumer].data.eth.port);
      REMOTE2->sa.sin_addr.s_addr = inet_addr(tparam+i+1);
      
      for(i=0; i<24; i+=4)
      {
        tparam[0+i] = n1;
        tparam[1+i] = n2;
        tparam[2+i] = n3;
        tparam[3+i] = n4;
      }
      aes_set_key(&REMOTE2->aes, tparam, 24);
      
      remote2_slave(REMOTE2);
    }
    else
    {
      /* MASTER */
      /* Occorre assicurare che il processo saet abbia già creato i file
         descriptor per i moduli reali, altrimenti si crea prima il virtuale
         che viene sovrascritto dal modulo reale. */
      sleep(1);
      
      /* Sovrappongo il modulo emulato a quello reale */
      i = open("/dev/pxl", O_RDWR);
      master_register_module(REMOTE2->mm, i);
#ifdef __CRIS__  
      /* Reset MM */
      ioctl(i, MM_RESET, 0);
#endif
      
      dev->fd_eth = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
      if(dev->fd_eth < 0) return;
      setsockopt(dev->fd_eth, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(int));
      REMOTE2->sa.sin_family = AF_INET;
      REMOTE2->sa.sin_port = htons(config.consumer[dev->consumer].data.eth.port);
      REMOTE2->sa.sin_addr.s_addr = INADDR_ANY;
      if(bind(dev->fd_eth, (struct sockaddr*)&REMOTE2->sa, sizeof(struct sockaddr_in)))
      {
        close(dev->fd_eth);
        return;
      }
      listen(dev->fd_eth, 0);
      
      strcpy(ifr.ifr_name, "eth0");
      ioctl(dev->fd_eth, SIOCGIFADDR, &ifr);
/*
      {
        struct sockaddr_in *sa2;
        sa2 = (struct sockaddr_in*)&(ifr.ifr_addr);
        printf("%08x\n", sa2->sin_addr.s_addr);
      }
*/
      for(i=0; i<24; i+=4)
        memcpy(tparam+i, &(((struct sockaddr_in*)&(ifr.ifr_addr))->sin_addr.s_addr), 4);
      aes_set_key(&REMOTE2->aes, tparam, 24);
      
      for(i=0; i<32; i++)
      {
        REMOTE2->perif[i].fd = -1;
        REMOTE2->perif[i].num = -1;
        REMOTE2->perif[i].i = 0;
        REMOTE2->perif[i].timeout = 0;
      }
      
      REMOTE2->fdmm = open("/dev/pxlmm", O_RDWR);
      write(REMOTE2->fdmm, "\033\013\000\000\000\001", 7);	// versione 11.0.0.0 e modulo master tipo 0
      
      REMOTE2->init = 0;
      n3 = REMOTE2->fdmm;
      if(dev->fd_eth > n3) n3 = dev->fd_eth;
      
      to.tv_sec = 1;
      to.tv_usec = 0;
      while(1)
      {
        FD_ZERO(&fds);
//printf("set fdmm %d\n", REMOTE2->fdmm);
        /* Non accetto connessioni da slave finché non è inizializzato
           il modulo master (richiesta periferiche) */
        FD_SET(REMOTE2->fdmm, &fds);
//printf("set fdeth %d\n", dev->fd_eth);
        if(REMOTE2->init) FD_SET(dev->fd_eth, &fds);
        n4 = n3;
        for(i=0; i<32; i++)
        {
          if(REMOTE2->perif[i].fd >= 0)
          {
//printf("set #%d %d\n", i, REMOTE2->perif[i].fd);
            FD_SET(REMOTE2->perif[i].fd, &fds);
            if(REMOTE2->perif[i].fd > n4) n4 = REMOTE2->perif[i].fd;
          }
        }
        if(!select(n4+1, &fds, NULL, NULL, &to))
        {
          to.tv_sec = 1;
          to.tv_usec = 0;
          for(i=0; i<32; i++)
          {
            if(REMOTE2->perif[i].timeout)
            {
              REMOTE2->perif[i].timeout--;
              if(!REMOTE2->perif[i].timeout)
              {
                REMOTE2->perif[i].buf[0] = 4;
                REMOTE2->perif[i].buf[1] = REMOTE2->perif[i].num;
                write(REMOTE2->fdmm, REMOTE2->perif[i].buf, 2);
                if(REMOTE2->perif[i].fd >= 0)
                {
                  shutdown(REMOTE2->perif[i].fd, 2);
                  close(REMOTE2->perif[i].fd);
                  REMOTE2->perif[i].fd = -1;
                  REMOTE2->perif[i].num = -1;
                  REMOTE2->perif[i].i = 0;
                }
              }
            }
          }
          continue;
        }
        
        if(FD_ISSET(REMOTE2->fdmm, &fds))
        {
//printf("comando master\n");
          /* Comando da centrale che deve essere gestito/inoltrato */
          i = read(REMOTE2->fdmm, REMOTE2->buf+REMOTE2->i, 64-REMOTE2->i);
          if(i > 0)
          {
            REMOTE2->i += i;
            while(remote2_master_parse(REMOTE2));
          }
//printf("comando master fine\n");
        }
        for(i=0; i<32; i++)
        {
          if((REMOTE2->perif[i].fd >= 0) && (FD_ISSET(REMOTE2->perif[i].fd, &fds)))
          {
//printf("select #%d %d\n", i, REMOTE2->perif[i].fd);
            /* Comando da slave che deve essere gestito */
            n4 = read(REMOTE2->perif[i].fd, REMOTE2->perif[i].buf+REMOTE2->perif[i].i, 64-REMOTE2->perif[i].i);
            if(n4 <= 0)
            {
              if(REMOTE2->perif[i].num >= 0)
              {
                REMOTE2->perif[i].buf[0] = 4;
                REMOTE2->perif[i].buf[1] = REMOTE2->perif[i].num;
                write(REMOTE2->fdmm, REMOTE2->perif[i].buf, 2);
              }
              
              shutdown(REMOTE2->perif[i].fd, 2);
              close(REMOTE2->perif[i].fd);
              REMOTE2->perif[i].fd = -1;
              REMOTE2->perif[i].num = -1;
              REMOTE2->perif[i].i = 0;
              REMOTE2->perif[i].timeout = 0;
            }
            else
            {
              REMOTE2->perif[i].i += n4;
              while(remote2_master_parse_perif(REMOTE2, i));
//printf("(fine)\n");
            }
          }
        }
        if(FD_ISSET(dev->fd_eth, &fds))
        {
          /* Cerco uno slot libero */
          for(i=0; i<32; i++)
            if((REMOTE2->perif[i].fd < 0) && (REMOTE2->perif[i].num < 0))
            {
              REMOTE2->perif[i].fd = accept(dev->fd_eth, NULL, NULL);
//printf("connect #%d %d\n", i, REMOTE2->perif[i].fd);
              break;
            }
          if(i == 32)
          {
            /* Rifiuto la connessione */
            i = accept(dev->fd_eth, NULL, NULL);
            close(i);
          }
//printf("connect fine\n");
        }
      }
    }
  }
  else
  {
    /* Solo remotizzazione LAN */
    sleep(1);
    return;
  }
}

void _init()
{
  printf("Remotizzazione (v.2) (plugin): " __DATE__ " " __TIME__ "\n");
  prot_plugin_register("REM", 0, remote_timer, NULL, (PthreadFunction)remote_loop);
  prot_plugin_register("REM2", 0, NULL, NULL, (PthreadFunction)remote2_loop);
}

