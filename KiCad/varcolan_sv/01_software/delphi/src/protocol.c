#include "protocol.h"
#include "serial.h"
#include "ethernet.h"
#include "codec.h"
#include "command.h"
#include "database.h"
#include "master.h"
#include "modem.h"
#include "gsm.h"
#include "delphi.h"
#include "support.h"
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>
#include <string.h>
#include <stdio.h>
#include <sys/socket.h>
#include <dlfcn.h>
#include <sys/types.h>
#include <dirent.h>
#include <signal.h>

#ifdef SAET_CONSOLE
#include "console/console.h"
#endif

#include <sys/time.h>
FILE *fpdebug = NULL;
time_t debugstart;
/* Registra i primi 5 minuti di colloquio */
#define DEBUG_RECTIME (5*60)

unsigned char LIN[MAX_NUM_CONSUMER];
unsigned short LIN_TO[MAX_NUM_CONSUMER];

static Event NakMsg = {NAK, 0, 0, -3};

static int prot_send1(Event *ev, ProtDevice *dev);

typedef struct _prot_registry {
  char *prot;
  int code;
  InitFunction timer;
  InitFunction command;
  PthreadFunction loop;
  struct _prot_registry *next;
} prot_registry;

static prot_registry *prot_registered = NULL;

const char PluginDir[] = "/saet/plugins/";

static pthread_mutex_t plugin_mutex = PTHREAD_MUTEX_INITIALIZER;
static int plugin_count;

/***************************************/
/* Protocol type translation           */
/***************************************/

int prot_plugin_register(char *prot, int code, InitFunction timer, InitFunction command, PthreadFunction loop)
{
  static int prot_code = 0x100;
  prot_registry *p;
  
  p = (prot_registry*)malloc(sizeof(prot_registry));
  p->prot = prot;
  if(code)
    p->code = code;
  else
    p->code = prot_code++;
  p->loop = loop;
  p->timer = timer;
  p->command = command;
  p->next = prot_registered;
  prot_registered = p;
  
  return p->code;
}

void prot_plugin_load()
{
  DIR *dir;
  struct dirent *plugin;
  char buf[64]; //, *err;
  
  prot_plugin_register("GEN", PROT_GENERIC, NULL, NULL, NULL);
  prot_plugin_register("S", PROT_SAETNET_OLD, NULL, NULL, (PthreadFunction)prot_loop);
  prot_plugin_register("SX", PROT_SAETNET_EX, NULL, NULL, (PthreadFunction)prot_loop);
  prot_plugin_register("C", PROT_CONSOLE_SYS, NULL, NULL, (PthreadFunction)console_loop);
  prot_plugin_register("L", PROT_CONSOLE_LARA, NULL, NULL, (PthreadFunction)console_loop);

  sprintf(buf, PluginDir);
  dir = opendir(ADDROOTDIR(buf));
  if(dir)
  {
    while((plugin = readdir(dir)) && (1))
    {
      sprintf(buf + sizeof(PluginDir) - 1, plugin->d_name);
      dlopen(ADDROOTDIR(buf), RTLD_NOW | RTLD_GLOBAL);
      //err = dlerror();
      //if(err) printf("%s\n", err);
    }
  }
}

static void prot_plugin_timer(void *p, int dev)
{
  ((prot_registry*)p)->timer((ProtDevice*)dev, 0);
  timeout_on(((ProtDevice*)dev)->plugin_timer, prot_plugin_timer, p, dev, 5);
}

typedef struct {
  int consumer;
  prot_registry *p;
} ser_data;

void* prot_loop_serial(ser_data *data)
{
  ProtDevice *dev;
  char serial[16];
  int avvio = 1;
  
  while(1)
  {
      if(config.consumer[data->consumer].configured < 5)
      {
#ifdef __arm__
        /* Per la scheda Athena la COM4 non esiste. */
        if(config.consumer[data->consumer].configured == 4) return NULL;
#endif
        sprintf(serial, "/dev/ttyS%d", config.consumer[data->consumer].configured - 1);
      }
      else if(config.consumer[data->consumer].configured < 12)
        sprintf(serial, "/dev/ptyp%d", config.consumer[data->consumer].configured - 6);
      else
        sprintf(serial, "/dev/ttyUSB%d", config.consumer[data->consumer].configured - 12);
      
      if(PROT_CLASS(data->p->code) == PROT_SAETNET)
        dev = prot_open(protSER, data->consumer, serial,
          config.consumer[data->consumer].data.serial.baud1);
      else
        dev = prot_open(protGEN, data->consumer, serial,
          config.consumer[data->consumer].data.serial.baud1);
      
      if(dev)
      {
{
char m[32];
sprintf(m, "Plugin loop start (%d)", data->consumer);
support_log(m);
}
        dev->th_plugin = pthread_self();
        dev->command = data->p->command;
        if(data->p->timer)
        {
          if(!config.consumer[data->consumer].dev->plugin_timer)
            config.consumer[data->consumer].dev->plugin_timer = timeout_init();
          prot_plugin_timer(data->p, (int)config.consumer[data->consumer].dev);
        }
        
        if(avvio)
        {
          avvio = 0;
          pthread_mutex_lock(&plugin_mutex);
          plugin_count--;
/*
{
char m[32];
sprintf(m, "Plugin DEC (%d)", plugin_count);
support_log(m);
}
*/
          pthread_mutex_unlock(&plugin_mutex);
        }
        
        if(data->p->loop) data->p->loop(config.consumer[data->consumer].dev);
        if(data->p->timer) timeout_off(config.consumer[data->consumer].dev->plugin_timer);
        
{
char m[32];
sprintf(m, "Plugin loop end (%d)", data->consumer);
support_log(m);
}
        prot_close(dev);
        free(dev);
      }
      
      sleep(1);
  }
  
  return NULL;
}

void prot_plugin_serial_start(void *p, int consumer)
{
  pthread_t pth;
  ser_data *data;
  
  data = malloc(sizeof(ser_data));
  if(!data) return;
  
  data->consumer = consumer;
  data->p = p;
  
  pthread_mutex_lock(&plugin_mutex);
  plugin_count++;
/*
{
char m[32];
sprintf(m, "Plugin INC (%d)", plugin_count);
support_log(m);
}
*/
  pthread_mutex_unlock(&plugin_mutex);
  
  pthread_create(&pth, NULL, (PthreadFunction)prot_loop_serial, data);
  pthread_detach(pth);
}

void prot_plugin_start(int consumer)
{
  prot_registry *p = prot_registered;
  
  while(p)
  {
    if(config.consumer[consumer].type == p->code)
    {      
      if(config.consumer[consumer].configured == 5)
      {
        if(p->timer)
        {
          if(!config.consumer[consumer].dev->plugin_timer)
            config.consumer[consumer].dev->plugin_timer = timeout_init();
          prot_plugin_timer(p, (int)config.consumer[consumer].dev);
        }
        if(p->loop) p->loop(config.consumer[consumer].dev);
        if(p->timer) timeout_off(config.consumer[consumer].dev->plugin_timer);
      }
      else
        prot_plugin_serial_start(p, consumer);
      return;
    }
    p = p->next;
  }
}

int prot_plugin_command(int consumer, int cmd)
{
  if(config.consumer[consumer].dev->command)
    return config.consumer[consumer].dev->command(config.consumer[consumer].dev, cmd);
  else
    return -1;
}

int prot_type(char *prot)
{
  prot_registry *p = prot_registered;
  
  if(!prot) return PROT_NONE;
  
  while(p)
  {
    if(p->prot && (!strcmp(prot, p->prot))) return p->code;
    p = p->next;
  }
  if(!strcmp(prot, "M"))
    return PROT_MODEM;
  else if(!strcmp(prot, "G"))
    return PROT_GSM;
  return PROT_NONE;
}

/***************************************/
/* Checksum functions                  */
/***************************************/

unsigned char prot_calc_cks(Event *ev)
{
  int i;
  unsigned char cks;

  cks = 0;
  for(i=0; i<(ev->Len+4); i++) cks += ((unsigned char*)ev)[i];

  return cks;
}

static unsigned char prot_add_cks(Event *ev)
{
  unsigned char cks;

  ((unsigned char*)ev)[ev->Len+4] = 0;
  cks = prot_calc_cks(ev);
  ((unsigned char*)ev)[ev->Len+4] = cks;

  return cks;
}

/***************************************/
/* Thread function receiving commands  */
/***************************************/

static void protocol_timeout(ProtDevice *dev, int null)
{
  dev->i = 0;
  dev->exp = 2;
}

int prot_recv_dev2(ProtDevice *dev)
{
  int n, dimMsg;
  int numNak = 0;
  char m[32];

  if(dev->fd < 0) return 0;
  
  dev->i = 0;
  dev->exp = 2;
  
  while(1)
  {
    if(dev->i + dev->exp >= DIMBUF)
    {
      /* mesg len fault -> reset */
      dev->i = 0;
      dev->exp = 2;
    }
    
    if(dev->type == protSER)
    {
      n = read(dev->fd, dev->buf + dev->i, dev->exp);
      if(n<=0)	// closed file descriptor or closing connection
      {
        sprintf(m, "Protocol recv() (%d) stop", dev->consumer);
        support_log(m);
        
        if(fpdebug) fclose(fpdebug);
        fpdebug = NULL;
        
        dev->protocol_active = 0;
        return 0;
      }
    }
    else
    {
      n = recv(dev->fd, dev->buf + dev->i, dev->exp, 0);
      if(n<=0)	// close connection
      {
        sprintf(m, "Protocol recv() (%d) stop", dev->consumer);
        support_log(m);
        
        if(fpdebug) fclose(fpdebug);
        fpdebug = NULL;
        
        dev->protocol_active = 0;
        return 0;
      }
    }

    if(n>0)
      dev->i += n;
    else
      continue;
  
  /* Registro per i primi 10 minuti */
  if((dev->consumer == config.debug) && ((time(NULL)-debugstart) < DEBUG_RECTIME))
  {
    int x;
    struct timeval tv;
    
    gettimeofday(&tv, NULL);
    fprintf(fpdebug, "%ld.%06ld - RX: ", tv.tv_sec, tv.tv_usec);
    for(x=0; x<dev->i; x++) fprintf(fpdebug, "%02x", dev->buf[x]);
    fprintf(fpdebug, " - n:%d dev->i:%d dev->exp:%d\n", n, dev->i, dev->exp);
    fflush(fpdebug);
/*
    printf("%d.%06d - RX: ", tv.tv_sec, tv.tv_usec);
    for(x=0; x<dev->i; x++) printf("%02x", dev->buf[x]);
    printf("\n");
*/
  }

#warning Timeout 200ms
/* anche se impostato a 200ms, il timeout puo' scattare gia' dopo 100ms. */
    timeout_on(dev->pkt_timeout, (timeout_func)protocol_timeout, dev, 0, 2);
    
    if(n < dev->exp)
    {
      dev->exp -= n;
      continue;
    }

    timeout_off(dev->pkt_timeout);

    LIN[dev->consumer] = bitAlarm;
    LIN_TO[dev->consumer] = 100;	/* Timeout: 10s */

    switch(dev->buf[0])
    {
      case '+':
        dev->i = 1;
        dev->exp = 1;
        dev->buf[0] = dev->buf[1];
        continue;
      case STX:
        if(dev->i<4)
        {
          dev->exp = 4 - n;
          continue;
        }
        dimMsg = (4+1+dev->buf[3]);
        if(dev->i<dimMsg)
        {
          dev->exp = dimMsg - dev->i;
          continue;
        }
#warning Evento in coda
/* Nel caso in cui si chiuda la connessione mentre ho un evento in coda che
   aspetta l'ACK, alla riconnessione il Com90 riparte subito con un STX per
   recuperare versione e quant'altro e rimane perciò bloccato.
   Per il momento preferisco perdere l'evento in coda piuttosto che rimanere
   completamente bloccato. Occorrerà poi trovare un modo di salvare l'evento
   in coda. */
#if 0
        /* received STX when especting ACK/NAK */
        if(dev->lastevent)
        {
          dev->i = 0;
          dev->exp = 2;
          continue;
        }
#endif
        break;
      case ENQ:
        /* received ENQ when especting ACK/NAK */
        if(dev->lastevent)
        {
          prot_send1(dev->lastevent, dev);
          //dev->timeout_ack = PROT_TIMEOUT;
          dev->i = 0;
          dev->exp = 2;
          continue;
        }
        else
        {
          dimMsg = 2;
          break;
        }
#if 0
      case ETB:
        if(dev->timeout_ack) /* received ETB when especting ACK/NAK */
        {
          prot_send1(dev->lastevent, dev);
          dev->timeout_ack = PROT_TIMEOUT;
          dev->i = 0;
          dev->exp = 2;
          continue;
        }
        else
        {
          dimMsg = 2;
          break;
        }
#endif
      case ACK:
        dev->i = 0;
        dev->exp = 2;
        //dev->timeout_ack = 0;
        dev->lastevent = NULL;
        continue;
      case NAK:
        prot_send1(dev->lastevent, dev);
        //dev->timeout_ack = PROT_TIMEOUT;
        dev->i = 0;
        dev->exp = 2;
        continue;
      case 'N':
        /* skip the 'N' char avoiding sending NAK when receiving 'NO CARRIER' */
        dev->i = 1;
        dev->exp = 1;
        dev->buf[0] = dev->buf[1];
        continue;
      case 'O':
        if(dev->i<9)
        {
          dev->exp = 9 - n;
          continue;
        }
        if(!strncmp(dev->buf, "O CARRIER", 9))	// [N]O CARRIER
        {
          dev->protocol_active = 0;
          //dev->timeout_ack = 0;
//printf("-> no carrier\n");
          return 0;
        }
        dev->i = 0;
        dev->exp = 2;
        numNak = 0;
        continue;
      default:
#if 0
        /* Se questo va bene, si puo' togliere il case 'N'. */
        dev->i = 1;
        dev->exp = 1;
        dev->buf[0] = dev->buf[1];
        continue;
#else
        /*if(numNak<5)
        {
          prot_send_NAK(dev);
          numNak++;
        }
        else
        {
          numNak = 0;
        }*/
        if((dev->type == protSER) && (!dev->modem)) tcflush(dev->fd, TCIOFLUSH);
        if(dev->type == protETH)
        {
          dev->exp = 0;
          do
          {
            dev->i = recv(dev->fd, dev->buf, DIMBUF, MSG_DONTWAIT);
            dev->exp++;
          }
          while((dev->i > 0) && (dev->exp < 10));
          if(dev->exp >= 10)
          {
            dev->protocol_active = 0;
            //dev->timeout_ack = 0;
            return 0;
          }
        }
        dev->i = 0;
        dev->exp = 2;
        continue;
#endif
    }
    
    if((dev->consumer == config.debug) && ((time(NULL)-debugstart) < DEBUG_RECTIME))
    {
      struct timeval tv;
    
      gettimeofday(&tv, NULL);
      fprintf(fpdebug, "%ld.%06ld - RX: OK\n", tv.tv_sec, tv.tv_usec);
      fflush(fpdebug);
    }
    
    memcpy(&(dev->cmd), dev->buf, dimMsg);
    
    dev->i = 0;
    dev->exp = 2;
    return 1;
  }
}

int prot_recv(Command *c, ProtDevice *dev)
{
  return prot_recv_dev2(dev);
}

/***************************************/
/* Export:                             */
/* Initialization of ProtDevice struct */
/* - struct alloc                      */
/* - sem/mutex init                    */
/* - thread create                     */
/***************************************/

ProtDevice* prot_init(int type, int fd, int consumer)
{
  ProtDevice *dev;

  if(consumer == MAX_NUM_CONSUMER) return NULL;
  
  dev = (ProtDevice*)malloc(sizeof(ProtDevice));
  if(dev)
  {
    dev->type = type;
    dev->fd = fd;
    
    dev->fd_eth = -1;
    dev->th_eth = 0;
    dev->th_plugin = 0;
    
    LS_ABILITATA[consumer] = 0x03;
    dev->consumer = consumer;
    
    LIN[dev->consumer] = 0;
    
    bzero(&(dev->cmd), sizeof(Command));

    //dev->timeout_ack = 0;
    
    dev->i = 0;
    
    dev->protocol_active = 1;
    dev->pid = support_getpid();
    dev->modem = 0;
    
    dev->lastevent = NULL;
    
    config.consumer[consumer].dev = dev;
    
    dev->pkt_timeout = timeout_init();
    dev->send_timeout = timeout_init();
    dev->plugin_timer = NULL;
    
    dev->command = NULL;
    dev->prot_data = NULL;
  }

  return dev;
}

/***************************************/
/* Export:                             */
/* ProtDevice creation                 */
/* - type: protSER, protETH            */
/* - data: e.g. "/dev/ttyS0"           */
/***************************************/

ProtDevice* prot_open(int type, int consumer, char *cdata, int idata)
{
  int fd;
  ProtDevice *dev;

  switch(type)
  {
    case protSER:
      if(idata)
        fd = ser_open(cdata, idata | CRTSCTS, 0);
      else
        fd = ser_open(cdata, B19200 | CRTSCTS, 0);
      if(fd >= 0)
      {
        dev = prot_init(type, fd, consumer);
        return dev;
      }
      break;
    case protETH:
      dev = prot_init(type, -1, consumer);
      if(!eth_open(idata, dev))
      {
        free(dev);
        return NULL;
      }
      return dev;
      break;
    case protGEN:
      if(idata)
        fd = ser_open(cdata, idata | CRTSCTS, 0);
      else
        fd = ser_open(cdata, B19200 | CRTSCTS, 0);
      if(fd >= 0)
      {
        dev = prot_init(type, -1, consumer);
        dev->fd = fd;
        return dev;
      }
      break;
    default:
      break;
  }
  return NULL;
}

/***************************************/
/* Export:                             */
/* ProtDevice resources                */
/* - dev: ProtDevice struct ptr        */
/*                                     */
/*  Note: dev MUST be freed after      */
/*  prot_close call                    */
/***************************************/

int prot_close(ProtDevice *dev)
{
  if(!dev) return -1;

  dev->protocol_active = 0;
  dev->lastevent = NULL;

  LS_ABILITATA[dev->consumer] = 0;
  LIN[dev->consumer] = 0;
  LIN_TO[dev->consumer] = 0;

  config.consumer[dev->consumer].dev = NULL;

  switch(dev->type)
  {
    case protSER:
      if(!dev->modem)
        return ser_close(dev->fd);
      else
      {
        /* Sblocca la read() in prot_loop() */ 
        if(config.consumer[dev->consumer].data.serial.baud2)
          ser_setspeed(dev->fd, config.consumer[dev->consumer].data.serial.baud2 | CRTSCTS, 1);
        else
          ser_setspeed(dev->fd, config.consumer[dev->consumer].data.serial.baud1 | CRTSCTS, 1);
        kill(dev->pid, SIGUSR2);
      }
      break;
    case protETH:
      return eth_close(dev);
      break;
    case protGEN:
      return ser_close(dev->fd);
      break;
    default:
      break;
  }

  return 0;
}

/***************************************/
/* Export:                             */
/* Send command on ProtDevice          */
/* - cmd: Command struct ptr           */
/* - dev: ProtDevice struct ptr        */
/***************************************/

/***************************************/
/* Export:                             */
/* Receive event on ProtDevice         */
/* - ev: Event struct ptr              */
/* - dev: ProtDevice struct ptr        */
/***************************************/

static int prot_send1(Event *ev, ProtDevice *dev)
{
  if(!ev || (dev->fd < 0)) return -1;

  if((dev->consumer == config.debug) && ((time(NULL)-debugstart) < DEBUG_RECTIME))
  {
    int x;
    struct timeval tv;
    
    gettimeofday(&tv, NULL);
    fprintf(fpdebug, "%ld.%06ld - TX: ", tv.tv_sec, tv.tv_usec);
    for(x=0; x<(ev->Len+5); x++) fprintf(fpdebug, "%02x", (int)(((unsigned char*)(ev))[x]));
    fprintf(fpdebug, "\n");
    fflush(fpdebug);
/*
    printf("%d.%06d - TX: ", tv.tv_sec, tv.tv_usec);
    for(x=0; x<(ev->Len+5); x++) printf("%02x", (int)(((unsigned char*)(ev))[x]));
    printf("\n");
*/
  }
  
  switch(dev->type)
  {
    case protSER:
      write(dev->fd, ev, ev->Len + 5);		// 4B hdr + 1B cks
      break;
    case protETH:
      send(dev->fd, ev, ev->Len + 5, 0);
      break;
    default:
      break;
  }
  
  return 0;
}

int prot_send(Event *ev, ProtDevice *dev)
{
  ev->Status = 0; // master_slowdown();
  dev->lastevent = ev;
  prot_add_cks(ev);
  prot_send1(ev, dev);
  //dev->timeout_ack = PROT_TIMEOUT;
  return 0;
}

int prot_send_NAK(ProtDevice *dev)
{
  if((dev->consumer == config.debug) && ((time(NULL)-debugstart) < DEBUG_RECTIME))
  {
    struct timeval tv;
    
    gettimeofday(&tv, NULL);
    fprintf(fpdebug, "%ld.%06ld - -> NAK\n", tv.tv_sec, tv.tv_usec);
    fflush(fpdebug);
  }
  
  return prot_send1(&NakMsg, dev);
}

void prot_loop(ProtDevice *dev)
{
  char m[32];
  
  debug_pid[dev->consumer] = support_getpid();
  sprintf(m, "Protocol start - %d [%d]", dev->consumer, debug_pid[dev->consumer]);
  support_log(m);
  
  if(config.debug == dev->consumer)
  {
    debugstart = time(NULL);
    fpdebug = fopen(ADDROOTDIR("/tmp/saet.debug"), "w");
  }
  
  /* we have to sleep to not receive the first 2 bytes of protocol corrupted when using gsm */
  if(dev->modem == MODEM_GSM) usleep(100000);
  LIN_TO[dev->consumer] = 600;	/* Timeout: 60s (Com90 is slow to send the first ENQ if we call */
  
  while(prot_recv_dev2(dev))
    codec_parse_command(&(dev->cmd), dev);
}

void prot_start()
{
  int i;
  ProtDevice *dev;
  
  plugin_count = 0;
  
  for(i=0; i<MAX_NUM_CONSUMER; i++)
  {
    if(config.consumer[i].configured)
    {
      if(config.consumer[i].configured != 5)
      {
        prot_plugin_start(i);
      }
      else if(config.consumer[i].configured == 5)
      {
        dev = prot_open(protETH, i, NULL, config.consumer[i].data.eth.port);
      }
    }
  }
  
  /* Attende che tutti i plugin seriali siano stati attivati.
     Non ha senso attendere anche i plugin ethernet poiché
     comunque fino alla connessione il plugin non viene
     effettivamente attivato. Attualmente quindi le inizializzazioni
     dei plugin che modificano il comportamento della centrale
     possono essere fatte solo dai plugin seriali (vedi ridondanza). */
/*
{
time_t tt;
tt = 0;
*/

  do
  {
    pthread_mutex_lock(&plugin_mutex);
    i = plugin_count;
    pthread_mutex_unlock(&plugin_mutex);
/*
if(time(NULL) != tt)
{
char m[32];
tt = time(NULL);
sprintf(m, "Plugin CHK (%d)", i);
support_log(m);
}
*/
    sleep(0);
  }
  while(i > 0);

/*
}
*/

}

/***************************************/
/* Export:                             */
/* ProtDevice resources                */
/* - dev: device to probe              */
/*    ("/dev/ttySx")                   */
/***************************************/

#if 0
struct _modem
{
  char *ati;
  char *cmd;
  char *res;
  char *init;
  char type;
  struct _modem *next;
};

int prot_test_modem(const char *dev)
{
  int fd, ret = MODEM_NO;
  char *ati, *buf, line[256];
  FILE *fp;
  struct _modem *pmodem;
  static struct _modem *list = NULL;
  
  if(!list)
  {
    fp = fopen("/saet/modem.conf", "r");
    while(fgets(line, 255, fp))
    {
      if((line[0] != '\n') || (line[0] != '#'))
      {
        pmodem = (struct _modem*)malloc(sizeof(struct _modem));
        pmodem->next = list;
        list = pmodem;
        buf = support_delim(line);
        pmodem->ati = strdup(buf);
        buf = support_delim(NULL);
        pmodem->cmd = (char*)malloc(strlen(buf)+2);
        sprintf(pmodem->cmd, "%s\r", buf);
        buf = support_delim(NULL);
        pmodem->res = strdup(buf);
        buf = support_delim(NULL);
        pmodem->init = (char*)malloc(strlen(buf)+2);
        sprintf(pmodem->init, "%s\r", buf);
        buf = support_delim(NULL);
        pmodem->type = buf[0];
      }
    }
    fclose(fp);
  }

  fd = ser_open(dev, B19200 | CRTSCTS, 20);
  if(fd < 0) return MODEM_NO;
  ser_send_recv(fd, "ATV1\r", line);
  usleep(50000);
  ser_send_recv(fd, "ATE1\r", line);
  usleep(50000);
  buf = ser_send_recv(fd, "AT\r", line);
  if(strcmp(buf, "OK\r\n"))
  {
    tcflush(fd, TCIOFLUSH);
    close(fd);
    return MODEM_NO;
  }
  
  usleep(50000);
  ati = strdup(ser_send_recv(fd, "ATI\r", line));
  for(pmodem = list; pmodem; pmodem = pmodem->next)
  {
    if(!strncmp(pmodem->ati, ati, strlen(pmodem->ati)))
    {
      if(pmodem->cmd[0] != '\r')
      {
        buf = ser_send_recv(fd, pmodem->cmd, line);
      }
      if((pmodem->cmd[0] == '\r') || (!strncmp(pmodem->res, buf, strlen(pmodem->res))))
      {
        switch(pmodem->type)
        {
          case '1':
            ret = MODEM_CLASSIC;
            break;
          case '2':
            ret = MODEM_GSM;
            break;
          case '3':
            ret = MODEM_GPRS;
            break;
          default:
            ret = MODEM_NO;
            break;
        }
        if(pmodem->init[0] != '\r') ser_send_recv(fd, pmodem->init, NULL);
        break;
      }
    }
  }

  free(ati);
  tcflush(fd, TCIOFLUSH);
  close(fd);

  return ret;
}
#else
struct _modem
{
  char *ati;
  char *cmd;
  char *res;
  char *init;
  int type;
} modem_list[] = {
{"332","ATI4\r","GM862", "\r", 2},
{"332","ATI4\r","GM862-GSM", "\r", 2},
{"332","ATI4\r","GM862-GPRS", "\r", 3},
{"332","ATI4\r","GM862-PCS", "\r", 3},
{"332","ATI4\r","GM862-QUAD", "\r", 3},
{"5601","\r","", "ATM0", 1},
{"D","ATI6\r","2456", "ATX3\\V2&H5&G7\r", 1},
{"D","ATI6\r","2433", "ATX3\\V2&H5&G7\r", 1},
{"D","ATI6\r","2414", "ATX3\\V2&H5&G7\r", 1},
{"F","ATI6\r","2456", "ATX3\\V2&H5&G7\r", 1},
{"F","ATI6\r","2433", "ATX3\\V2&H5&G7\r", 1},
{"F","ATI6\r","2414", "ATX3\\V2&H5&G7\r", 1},
{"H","ATI6\r","2456", "ATX3\\V2&H5&G7\r", 1},
{"H","ATI6\r","2433", "ATX3\\V2&H5&G7\r", 1},
{"H","ATI6\r","2414", "ATX3\\V2&H5&G7\r", 1},
{NULL, NULL, NULL, NULL, 0},
};

int prot_test_modem(const char *dev)
{
  int fd, ret = MODEM_NO;
  char *ati, *buf, line[256];
  struct _modem *pmodem;
  
  fd = ser_open(dev, B19200 | CRTSCTS, 20);
  if(fd < 0) return MODEM_NO;
  ser_send_recv(fd, "ATV1\r", line);
  usleep(50000);
  ser_send_recv(fd, "ATE1\r", line);
  usleep(50000);
  buf = ser_send_recv(fd, "AT\r", line);
  if(strcmp(buf, "OK\r\n"))
  {
    tcflush(fd, TCIOFLUSH);
    close(fd);
    return MODEM_NO;
  }
  
  usleep(50000);
  ati = strdup(ser_send_recv(fd, "ATI\r", line));
  for(pmodem = modem_list; pmodem->ati; pmodem++)
  {
    if(!strncmp(pmodem->ati, ati, strlen(pmodem->ati)))
    {
      if(pmodem->cmd[0] != '\r')
      {
        buf = ser_send_recv(fd, pmodem->cmd, line);
      }
      if((pmodem->cmd[0] == '\r') || (!strncmp(pmodem->res, buf, strlen(pmodem->res))))
      {
        switch(pmodem->type)
        {
          case 1:
            ret = MODEM_CLASSIC;
            break;
          case 2:
            ret = MODEM_GSM;
            break;
          case 3:
            ret = MODEM_GPRS;
            break;
          default:
            ret = MODEM_NO;
            break;
        }
        if(pmodem->init[0] != '\r') ser_send_recv(fd, pmodem->init, NULL);
        break;
      }
    }
  }

  free(ati);
  tcflush(fd, TCIOFLUSH);
  close(fd);

  return ret;
}
#endif


