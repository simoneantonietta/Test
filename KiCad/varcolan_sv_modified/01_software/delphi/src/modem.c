#include "modem.h"
#include "serial.h"
#include "codec.h"
#include "master.h"
#include "support.h"
#include "timeout.h"
#include <sys/ioctl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

ProtDevice *modem_device = NULL;
char modem_serial[] = "/dev/ttySx";
static int modem_fd = -1;
static int modem_recovery_dim = DIM_EVENT_BUFFER + 1;
static int modem_consumer;
static int modem_rings = 0;
static int modem_user_close = 0;

#define TMD_SILAB	0
#define TMD_USROBOTICS	1
static int modem_type = TMD_SILAB;

#define MODEMBUFDIM	256

static char modem_buf[MODEMBUFDIM];
static pthread_t modem_thd;
//static sem_t modem_sem;
static pthread_mutex_t modem_mutex = PTHREAD_MUTEX_INITIALIZER;
static struct support_list *modem_head = NULL;
static struct support_list *modem_tail = NULL;
static timeout_t *modem_timeout;
static timeout_t *modem_timeout_retry;
static timeout_t *modem_timeout_rings = NULL;

volatile char TMD_Status[4] = {1, 0, 3, 0};

static void modem_reset(void *null1, int null2)
{
/* Il reset del modem non blocca le chiamate successive visto che il modem e' fisicamente
   sul sistema e non si puo' spegnere. Invece invio un AT ogni minuto per verificare se e
   quando il modem si riprende e alla ricezione dell'OK il sistema riparte con i comandi
   accodati. Il primo pero' viene perso perche' e' quello che aveva portato al blocco. */
   
  write(modem_fd, "AT\r", 3);
  timeout_on(modem_timeout, (timeout_func)modem_reset, NULL, 0, 60);
//  TMD_Status[0] = 0;	// modem is off.
}

static void modem_reset_rings(void *null1, int null2)
{
  if(TMD_Status[1] == 2) TMD_Status[1] = 0;
  modem_rings = 0;
}

static void modem_send(char *cmd, int timeout);

/* If command timeout try to resend. If timeout again ... */
static void modem_resend(char *cmd, int timeout)
{
  if(TMD_Status[1] == 0)
  {
    write(modem_fd, cmd, strlen(cmd));
    timeout_on(modem_timeout, modem_reset, NULL, 0, timeout);
  }
  else
  {
    // riaccodo il comando che aveva fallito per riprovare al termine della connessione attuale
    modem_send(cmd, timeout);
  }
}

static void modem_send(char *cmd, int timeout)
{
  struct support_list *tmp;

//  sem_wait(&modem_sem);
  pthread_mutex_lock(&modem_mutex);
  
  if(cmd[2] == 'D')	// don't duplicate ATD
  {
    tmp = modem_head;
    while(tmp)
    {
      if(!strcmp(cmd, tmp->cmd))
      {
        pthread_mutex_unlock(&modem_mutex);
//        sem_post(&modem_sem);
        return;
      }
      tmp = tmp->next;
    }
  }
  
  modem_tail = support_list_add_next(modem_tail, cmd, timeout, 0);
  
  if(!modem_head)
  {
    modem_head = modem_tail;
    if(TMD_Status[0] == 1 && TMD_Status[1] == 0)
    {
      if(cmd[2] == 'D') TMD_Status[3] = 2;
      write(modem_fd, cmd, strlen(cmd));
      timeout_on(modem_timeout, (timeout_func)modem_resend, modem_head->cmd, timeout, timeout);
    }
  }
    
//  sem_post(&modem_sem);
  pthread_mutex_unlock(&modem_mutex);
}

static void modem_send_immediate(char *cmd, int timeout)
{
  write(modem_fd, cmd, strlen(cmd));
  modem_head = support_list_add_prev(modem_head, cmd, timeout, 0);
  timeout_on(modem_timeout, (timeout_func)modem_resend, modem_head->cmd, timeout, timeout);
}

static void modem_send_next()
{
  if(!modem_head || TMD_Status[0] != 1) return;

//  sem_wait(&modem_sem);
  pthread_mutex_lock(&modem_mutex);
  
  modem_head = support_list_del(modem_head);

  if(modem_head)
  {
    if(modem_head->cmd[2] == 'D') TMD_Status[3] = 2;
    write(modem_fd, modem_head->cmd, strlen(modem_head->cmd));
    timeout_on(modem_timeout, (timeout_func)modem_resend, modem_head->cmd, modem_head->timeout, modem_head->timeout);
  }
  else
    modem_tail = modem_head;
  
//  sem_post(&modem_sem);
  pthread_mutex_unlock(&modem_mutex);
}

static void* modem_recv(void *nothing)
{
  int n, i;
  char lastAT[32];
  int modem_retries;
  
  pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);

  support_log("TMD recv() start");
  
  i = 0;
  lastAT[0] = 0;
  modem_retries = 0;
  
  while(1)
  {
    n = read(modem_fd, modem_buf + i, 1);
    
    if(i == MODEMBUFDIM-1)
    {
      /* msg too long, something is going wrong */
      modem_buf[i] = '\n';
    }
    
    if(n > 0)
    {
      if(modem_buf[i] != '\n')
      {
        i++;
        continue;
      }
    }
    if(i > 0)
      modem_buf[i - 1] = 0;
    else
      modem_buf[0] = 0;
    i = 0;
    
    if(!modem_buf[0]) continue;	// skip empty lines
    
//#warning !!!!! Debug MODEM !!!!!
//printf("%s\n", modem_buf);
//support_log(modem_buf);

    if(!strncmp(modem_buf, "AT", 2))
    {
      strcpy(lastAT, modem_buf);
    }
    else if(!strncmp(modem_buf, "OK", 2))
    {
      timeout_off(modem_timeout);
      
      if(modem_device && modem_user_close)
      {
        modem_send_immediate("ATH\r", 5);
        free(modem_device);
        modem_device = NULL;
        modem_user_close = 0;
      }
      else if(!strncmp(lastAT, "ATH", 3))
      {
        TMD_Status[1] = 0;
      }
      
      modem_send_next();
    }
    else if(!strncmp(modem_buf, "CONNECT", 7))
    {
      timeout_off(modem_timeout);
      lastAT[0] = 0;
      if(TMD_Status[1] == 2)
        support_called_number_success();
      
      TMD_Status[1] = 5;
      
      modem_device = prot_init(protSER, modem_fd, modem_consumer);
      modem_device->modem = MODEM_CLASSIC;

      codec_drop_events(modem_device->consumer, modem_recovery_dim);

      support_log("TMD Start protocol");
      prot_loop(modem_device);
      support_log("TMD End protocol");
      // modem_user_close:
      // 1 -> connessione chiusa dal programma utente o timeout linea
      // 0 -> connessione chiusa per caduta linea
      if(modem_user_close)	// connection closed by user program
      {
        // ripristino il timeout in caso si sblocco forzato
        if(config.consumer[modem_consumer].data.serial.baud2)
          ser_setspeed(modem_fd, config.consumer[modem_consumer].data.serial.baud2 | CRTSCTS, 0);
        else
          ser_setspeed(modem_fd, config.consumer[modem_consumer].data.serial.baud1 | CRTSCTS, 0);
        
        support_log("TMD Terminating");
        sleep(1);
        modem_send_immediate("+++", 20);
      }
      else
      {
        if(modem_type == TMD_SILAB)
        {
          int ret;
          ret = TIOCM_RTS;
          ret = ioctl(modem_fd, TIOCMBIC, &ret);
        }

        prot_close(modem_device);
        // ripristino il timeout appena impostato da prot_close()
        if(config.consumer[modem_consumer].data.serial.baud2)
          ser_setspeed(modem_fd, config.consumer[modem_consumer].data.serial.baud2 | CRTSCTS, 0);
        else
          ser_setspeed(modem_fd, config.consumer[modem_consumer].data.serial.baud1 | CRTSCTS, 0);
        TMD_Status[1] = 0;    
        free(modem_device);
        modem_device = NULL;
        modem_send_next();
      }
    }
    else if(!strncmp(modem_buf, "RING", 4) && (TMD_Status[2] & 0x2))
    {
      support_log("TMD Ring");
      if(++modem_rings >= config.Rings)
      {
        TMD_Status[1] = 2;
        modem_send_immediate("ATA\r", 300);
      }
      timeout_on(modem_timeout_rings, modem_reset_rings, NULL, 0, 100);
    }
    else if((!strncmp(modem_buf, "NO CARRIER", 10)) ||
            (!strncmp(modem_buf, "BUSY", 4)) ||
            (!strncmp(modem_buf, "NO DIALTONE", 11)) ||
            (!strncmp(modem_buf, "NO DIAL TONE", 12)) ||
            (!strncmp(modem_buf, "NO ANSWER", 9)) ||
            (!strncmp(modem_buf, "UN-OBTAINABLE NUMBER", 20)))
    {
      timeout_off(modem_timeout);
      TMD_Status[1] = 0;
      lastAT[0] = 0;
      
      modem_retries++;
      
      if(modem_retries > config.Retries)
      {
//support_log("Error 1");
        TMD_Status[3] = 3;
        modem_retries = 0;
        modem_send_next();
      }
      else if(modem_head)
      {
//support_log("Error 2");
        TMD_Status[3] = 4;
        timeout_on(modem_timeout_retry, (timeout_func)modem_resend, modem_head->cmd, modem_head->timeout, config.Interval);
      }
      else
      {
//support_log("Error 3");
        modem_retries = 0;
        modem_send_next();
      }
    }
  }

  return NULL;
}

void modem_init()
{
  char buf[64], *rec;
  
  TMD_Status[0] = 1;
  
  modem_consumer = support_find_serial_consumer(PROT_MODEM, 0);
  /* if we get here, we have a modem consumer configured, but check anyway */
  if(modem_consumer < 0) return;

  modem_serial[9] = '0' + config.consumer[modem_consumer].configured - 1;
  modem_fd = ser_open(modem_serial, config.consumer[modem_consumer].data.serial.baud1 | CRTSCTS, 0);
  
  rec = ser_send_recv(modem_fd, "ATI\r", buf);
  if(strncmp(rec, "5601", 4))
    modem_type = TMD_SILAB;
  else
    modem_type = TMD_USROBOTICS;
  
  if(config.consumer[modem_consumer].data.serial.baud2)
    modem_setspeed(config.consumer[modem_consumer].data.serial.baud2);

  modem_timeout = timeout_init();
  modem_timeout_retry = timeout_init();
  modem_timeout_rings = timeout_init();
  
//  sem_init(&modem_sem, 0, 1);

  pthread_create(&modem_thd, NULL, modem_recv, NULL);
  pthread_detach(modem_thd);

  support_log("TMD Init");

  modem_send("AT\r", 5); // check if modem is on.
}

void modem_setspeed(int baud)
{
  if(modem_type == TMD_SILAB)
  {
    switch(baud)
    {
      case B1200: write(modem_fd, "AT\\T2\r", 6); break;
      case B2400: write(modem_fd, "AT\\T3\r", 6); break;
      case B4800: write(modem_fd, "AT\\T4\r", 6); break;
      case B9600: write(modem_fd, "AT\\T6\r", 6); break;
      case B19200: write(modem_fd, "AT\\T9\r", 6); break;
      case B38400: write(modem_fd, "AT\\T10\r", 7); break;
      case B57600: write(modem_fd, "AT\\T11\r", 7); break;
      case B115200: write(modem_fd, "AT\\T12\r", 7); break;
      default: break;
    }
    tcdrain(modem_fd);
    ser_setspeed(modem_fd, baud | CRTSCTS, 0);
  }
}

int TMD_Call(int pbook_number)
{
  char buf[32];
  
  if(master_behaviour == SLAVE_STANDBY) return 0;
  
  if((pbook_number < 1) || (pbook_number > PBOOK_DIM)) return -1;
  if(TMD_Status[0] != 1) return -2;
  if(!config.PhoneBook[pbook_number-1].Phone[0]) return -3;
  if(!(config.PhoneBook[pbook_number-1].Abil & PB_TX)) return -4;
  
  support_log("TMD Call");
  if(config.PhoneBook[pbook_number-1].Name)
    support_called_number(config.PhoneBook[pbook_number-1].Name);
  else
    support_called_number(config.PhoneBook[pbook_number-1].Phone);
  
  sprintf(buf, "ATDT%s\r", config.PhoneBook[pbook_number-1].Phone);
  modem_send(buf, 650);
  TMD_Status[1] = 1;
  TMD_Status[3] = 1;
  
  return 0;
}

int TMD_Call_terminate()
{
  if(master_behaviour == SLAVE_STANDBY) return 0;
  
  if(!TMD_Status[1]) return -1;
  if(TMD_Status[1] < 3)
  {
    support_log("TMD Call terminate (1)");
    modem_send_immediate("\r", 300);
    TMD_Status[1] = 0;
    return 0;
  }
  
  support_log("TMD Call terminate (2)");
  modem_user_close = 1;
  prot_close(modem_device);
  TMD_Status[3] = 0;
  
  return 0;
}

int TMD_Status_event(int event_index)
{
  int i, j;
  unsigned char ev[32];
    
  ev[0] = Evento_Esteso;
  ev[1] = Ex_Stringa_Modem;
  ev[2] = event_index;
  
  switch(event_index)
  {
    case 4:
      for(i=0; i<PBOOK_DIM; i++)
        if(config.PhoneBook[i].Phone[0])
        {
          ev[3] = strlen(config.PhoneBook[i].Phone) + 5;
          sprintf(ev+4, "%2d - %s", i+1, config.PhoneBook[i].Phone);
          codec_queue_event(ev);
        }
      break;
    case 5:
      i = support_called_number_idx;
      if(!support_called_numbers[i].time) i = 0;
        
      for(j=0; j<CALLED_NUMBERS; j++,i++)
        if(support_called_numbers[i%CALLED_NUMBERS].time)
        {
          ev[3] = strlen(support_called_numbers[i%CALLED_NUMBERS].number) + 5;
          sprintf(ev+4, "%2d - %s", j+1, support_called_numbers[i%CALLED_NUMBERS].number);
          codec_queue_event(ev);
          sprintf(ev+4, "%2d - ", j+1);
          strftime(ev+9, 22, "%x %X", localtime(&support_called_numbers[i%CALLED_NUMBERS].time));
          ev[3] = strlen(ev+4);
          codec_queue_event(ev);
        }
      break;
    case 6:
      i = support_called_number_idx;
      if(!support_called_numbers[i].time) i = 0;
        
      for(j=0; j<CALLED_NUMBERS; j++,i++)
        if(support_called_numbers[i%CALLED_NUMBERS].time && !support_called_numbers[i%CALLED_NUMBERS].success)
        {
          ev[3] = strlen(support_called_numbers[i%CALLED_NUMBERS].number) + 5;
          sprintf(ev+4, "%2d - %s", j+1, support_called_numbers[i%CALLED_NUMBERS].number);
          codec_queue_event(ev);
          sprintf(ev+4, "%2d - ", j+1);
          strftime(ev+9, 22, "%x %X", localtime(&support_called_numbers[i%CALLED_NUMBERS].time));
          ev[3] = strlen(ev+4);
          codec_queue_event(ev);
        }
      break;
    case 7:
      break;
  }
  return 0;
}

int TMD_Set_recovery(int onoff, int dim)
{
  if(onoff)
    modem_recovery_dim = dim;
  else
    modem_recovery_dim = DIM_EVENT_BUFFER;
  
  return 0;
}
