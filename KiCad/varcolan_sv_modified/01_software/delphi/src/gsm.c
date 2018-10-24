#include "gsm.h"
#include "serial.h"
#include "codec.h"
#include "support.h"
#include "audio.h"
#include "master.h"
#include "strings.h"
#include "contactid.h"
#include "console/console.h"
#include "delphi.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include <ctype.h>
#include <fcntl.h>

//#define GSM_DEBUG_PRINT

#define GSM_IOCTYPE 45
#define IO_GSM_ONOFF      _IO(GSM_IOCTYPE, 0x10) /*  */
#define IO_GSM_SHUTDOWN   _IO(GSM_IOCTYPE, 0x11) /*  */
#define IO_GSM_POWERMON   _IO(GSM_IOCTYPE, 0x12) /*  */

ProtDevice *gsm_device = NULL;
char gsm_serial[] = "/dev/ttySx";
static int gsm_fd = -1;
static int gsm_recovery_dim = DIM_EVENT_BUFFER+1;
static int gsm_consumer;

#define GSMBUFDIM	256
static unsigned char gsm_buf[GSMBUFDIM];
static int gsm_buf_idx = 0;
static pthread_t gsm_thd;
static pthread_mutex_t gsm_mutex = PTHREAD_MUTEX_INITIALIZER;
static timeout_t *gsm_timeout = NULL;
static timeout_t *gsm_mms_delay = NULL;
static timeout_t *gsm_audio_timeout = NULL;
static timeout_t *gsm_credito_timeout = NULL;

int gsm_is_gprs = 0;
int gsm_operator = -1;
int gsm_connected = 0;
int gsm_dialmode = 0;
static int gsm_gprs_init = GPRS_INIT_NONE;
int gsm_sms_event = 0;
int gsm_user_close = 0;
static int gsm_selint = 0;
static int gsm_flags = 0;

static char gsm_sms_pending[8];
static int gsm_sms_pending_in;
static int gsm_sms_pending_out;

struct data {
  union {
    /* SMS, GSM audio/data call */
    struct {
      char num[16];
      char *text;
      int msg;
      void (*cbk)(int esito, void *data);
      void *cbk_data;
    };
    /* MMS, GPRS data */
    struct {
      int type;
      int (*func)(int fd, void *data);
      char *ipaddr;
      int port;
      void *data;
      int len;
    };
  };
  int retry;
  int timeout;
  struct data *next;
};

struct data *gsm_sms_head = NULL, *gsm_sms_tail = NULL, *gsm_sms_cur = NULL;
struct data *gsm_gprs_head = NULL, *gsm_gprs_tail = NULL, *gsm_gprs_cur = NULL;
struct data *gsm_call_head = NULL, *gsm_call_tail = NULL, *gsm_call_cur = NULL;
struct data *gsm_delete_sms = NULL, *gsm_read_sms = NULL;

/* Statistics */
static int gsm_stat_line_val = 0;
static int gsm_stat_line_cnt = 0;
static int gsm_stat_line_min = 99;
static int gsm_stat_line_max = 0;
static int gsm_stat_line_act = 0;
static time_t gsm_stat_line_time_min = 0;
static time_t gsm_stat_line_time_max = 0;
static time_t gsm_stat_line_time_avg = 0;
static char gsm_moni1[36];
static char gsm_moni2[36];
static char gsm_csca[32];

static float gsm_credito_residuo = 0.0;
static int gsm_credito_valido = 0;
static int gsm_ring_from = 0;
static int gsm_ring_from_sconosciuto = 0;
static int gsm_audio_type;

static int gsm_cid_num = 0;
static int gsm_cid_num_base = 0;
static unsigned char gsm_cid_ev[16];

/* Il CEI Proteus richiede di gestire lo squillo dati come squillo normale. */
int gsm_ring_as_voice = 0;

char volatile GSM_Status[7] = {99, 0, 3, -1, 99, 0, 0};

enum {GSM_OPERATOR_TIM, GSM_OPERATOR_VODAFONE, GSM_OPERATOR_WIND, GSM_OPERATOR_H3G};

/* Questa struttura non si può più toccare, viene usata anche nella saetlib in mms.c */
struct gsm_op_data_t gsm_operator_data[] = {
  {"22201", "AT+CMGS=\"40916\"\r\0PRE CRE SIN\x1a", "mms.tim.it", "mms.tim.it/servlets/mms", "213.230.130.89", 0},
  {"22210", "ATD404;\r", "mms.vodafone.it", "mms.vodafone.it/servlets/mms", "10.128.224.10", 0},
  {"22288", "ATD*123#\r", "mms.wind", "mms.wind.it/", "212.245.244.11", 1},
  {"22299", "AT\r", "tre.it", "10.216.59.240:10021/mmsc", "62.13.171.3:8799", 0},
  {NULL, NULL, NULL, NULL, NULL, 0}
};

struct gsm_op_dataex_t {
  //char *code;
  char *apn;	/* data */
} gsm_operator_dataex[] = {
  {/*"22201",*/ "ibox.tim.it"},
  {/*"22210",*/ "web.omnitel.it"},
  {/*"22288",*/ "internet.wind"},
  {/*"22299",*/ "tre.it"},
  {/*NULL,*/ NULL}
};

enum {ST_START, ST_ACCENSIONE_1, ST_ACCENSIONE_2, ST_INIT_0, ST_INIT_1, ST_INIT_1A, ST_INIT_1B,
    ST_INIT_DETECT_1, ST_INIT_DETECT_1_ERR, ST_INIT_DETECT_2,
    ST_INIT_DETECT_3, ST_INIT_DETECT_3_ERR, ST_INIT_DETECT_STOP, ST_INIT_DETECT_4,
    ST_INIT_VERSION, ST_INIT_VERSION_READ, ST_INIT_VERSION_OK,
    ST_INIT_2, ST_INIT_3, ST_INIT_4, ST_INIT_5, ST_INIT_5A, ST_INIT_5B, ST_INIT_5C, ST_INIT_5D,
    ST_INIT_6, ST_INIT_7A, ST_INIT_7B, ST_INIT_7C, ST_INIT_7D, ST_INIT_7E,
    ST_INIT_8, ST_INIT_9, ST_INIT_10, ST_INIT_11, ST_IMPOSTA_SELINT, ST_IMPOSTA_SELINT_2,
    ST_INIT_12, ST_INIT_13, ST_INIT_14, ST_INIT_15, ST_INIT_LAST,
    ST_MAIN, ST_CHIUDI_CHIAMATA, ST_CHIAMATA_CHIUSA,
    ST_SMS_LEGGI, ST_SMS_LEGGI_TESTO, ST_SMS_ARRIVATO,
    ST_SMS_ATTENDI_OK, ST_SMS_ATTENDI_FINE, ST_SMS_INOLTRA,
    ST_SMS_ATTENDI_OK_PER_CANCELLARE, ST_SMS_CANCELLA, ST_SMS_CANCELLATO,
    ST_SMS_INVIA, ST_SMS_ATTESA_PROMPT, ST_SMS_INVIA_TESTO, ST_SMS_ATTESA_INVIO, ST_SMS_INVIATO,
    ST_GPRS_INVIA, ST_GPRS_INIT, ST_GPRS_INIT_1A, ST_GPRS_INIT_1B, ST_GPRS_INIT_1C, ST_GPRS_INIT_1D,
    ST_GPRS_INIT_2, ST_GPRS_INIT_3, ST_GPRS_INIT_4, ST_GPRS_INIT_5, ST_GPRS_INIT_6,
    ST_GPRS_CONNECT, ST_GPRS_CONNECT_ERROR, ST_GPRS_TRASMETTI,
    ST_GPRS_CONNECT_CLOSE, ST_GPRS_CONNECT_CLOSE_2, ST_GPRS_CONNECT_CLOSE_3, ST_GPRS_CONNECT_CLOSE_4,
    ST_AUDIO_INVIA, ST_AUDIO_ATTESA_RISPOSTA, ST_AUDIO_RIPRODUCI,
    ST_AUDIO_TERMINA, ST_AUDIO_TERMINA_2, ST_AUDIO_FINE,
    ST_AUDIO_REC, ST_AUDIO_REC_INIZIO, ST_AUDIO_REC_TERMINA, ST_AUDIO_REC_FINE,
    ST_SMS_LISTA_UNREAD, ST_SMS_LISTA_UNREAD_OK,
    ST_SMS_LISTA, ST_SMS_LISTA_2, ST_SMS_LISTA_FINE,
    ST_RICHIESTA_CREDITO, ST_RICHIESTA_CREDITO_INVIATA,
    ST_CREDITO_RESIDUO, ST_VERIFICA_CREDITO, ST_VERIFICA_CREDITO_OK,
    ST_VERIFICA_PORTANTE, ST_VERIFICA_PORTANTE_OK,
    ST_VERIFICA_CONFIGURAZIONE, ST_VERIFICA_CONFIGURAZIONE_OK,
    ST_VERIFICA_CONNESSIONE, ST_VERIFICA_CONNESSIONE_OK,
    ST_SPEGNIMENTO, ST_SPEGNIMENTO_1, ST_SPEGNIMENTO_2, ST_SPENTO,
    ST_REINIT_SMS, ST_REINIT_SMS_1, ST_REINIT_SMS_2, ST_REINIT_SMS_3,
    ST_SAETNET, ST_SAETNET_ERROR, ST_SAETNET_AVVIA, ST_SAETNET_SBLOCCA,
    ST_SAETNET_CHIUDI, ST_SAETNET_FINE, ST_SAETNET_CHIAMA, ST_SAETNET_ERROR_2,
    ST_CID_CHIAMA, ST_CID_CHIAMA_2, ST_CID_INVIA, ST_CID_FINE, ST_CID_FINE_2, ST_CID_FINE_3
};

#ifdef GSM_DEBUG_PRINT
char *statod[] = {"ST_START", "ST_ACCENSIONE_1", "ST_ACCENSIONE_2", "ST_INIT_0", "ST_INIT_1", "ST_INIT_1A", "ST_INIT_1B",
    "ST_INIT_DETECT_1", "ST_INIT_DETECT_1_ERR", "ST_INIT_DETECT_2",
    "ST_INIT_DETECT_3", "ST_INIT_DETECT_3_ERR", "ST_INIT_DETECT_STOP", "ST_INIT_DETECT_4",
    "ST_INIT_VERSION", "ST_INIT_VERSION_READ", "ST_INIT_VERSION_OK",
    "ST_INIT_2", "ST_INIT_3", "ST_INIT_4", "ST_INIT_5", "ST_INIT_5A", "ST_INIT_5B", "ST_INIT_5C", "ST_INIT_5D",
    "ST_INIT_6", "ST_INIT_7A", "ST_INIT_7B", "ST_INIT_7C", "ST_INIT_7D", "ST_INIT_7E",
    "ST_INIT_8", "ST_INIT_9", "ST_INIT_10", "ST_INIT_11", "ST_IMPOSTA_SELINT", "ST_IMPOSTA_SELINT_2",
    "ST_INIT_12", "ST_INIT_13", "ST_INIT_14", "ST_INIT_15", "ST_INIT_LAST",
    "ST_MAIN", "ST_CHIUDI_CHIAMATA", "ST_CHIAMATA_CHIUSA",
    "ST_SMS_LEGGI", "ST_SMS_LEGGI_TESTO", "ST_SMS_ARRIVATO",
    "ST_SMS_ATTENDI_OK", "ST_SMS_ATTENDI_FINE", "ST_SMS_INOLTRA",
    "ST_SMS_ATTENDI_OK_PER_CANCELLARE", "ST_SMS_CANCELLA", "ST_SMS_CANCELLATO",
    "ST_SMS_INVIA", "ST_SMS_ATTESA_PROMPT", "ST_SMS_INVIA_TESTO", "ST_SMS_ATTESA_INVIO", "ST_SMS_INVIATO",
    "ST_GPRS_INVIA", "ST_GPRS_INIT", "ST_GPRS_INIT_1A", "ST_GPRS_INIT_1B", "ST_GPRS_INIT_1C", "ST_GPRS_INIT_1D",
    "ST_GPRS_INIT_2", "ST_GPRS_INIT_3", "ST_GPRS_INIT_4", "ST_GPRS_INIT_5", "ST_GPRS_INIT_6",
    "ST_GPRS_CONNECT", "ST_GPRS_CONNECT_ERROR", "ST_GPRS_TRASMETTI",
    "ST_GPRS_CONNECT_CLOSE", "ST_GPRS_CONNECT_CLOSE_2", "ST_GPRS_CONNECT_CLOSE_3", "ST_GPRS_CONNECT_CLOSE_4",
    "ST_AUDIO_INVIA", "ST_AUDIO_ATTESA_RISPOSTA", "ST_AUDIO_RIPRODUCI",
    "ST_AUDIO_TERMINA", "ST_AUDIO_TERMINA_2", "ST_AUDIO_FINE",
    "ST_AUDIO_REC", "ST_AUDIO_REC_INIZIO", "ST_AUDIO_REC_TERMINA", "ST_AUDIO_REC_FINE",
    "ST_SMS_LISTA_UNREAD", "ST_SMS_LISTA_UNREAD_OK",
    "ST_SMS_LISTA", "ST_SMS_LISTA_2", "ST_SMS_LISTA_FINE",
    "ST_RICHIESTA_CREDITO", "ST_RICHIESTA_CREDITO_INVIATA",
    "ST_CREDITO_RESIDUO", "ST_VERIFICA_CREDITO", "ST_VERIFICA_CREDITO_OK",
    "ST_VERIFICA_PORTANTE", "ST_VERIFICA_PORTANTE_OK",
    "ST_VERIFICA_CONFIGURAZIONE", "ST_VERIFICA_CONFIGURAZIONE_OK",
    "ST_VERIFICA_CONNESSIONE", "ST_VERIFICA_CONNESSIONE_OK",
    "ST_SPEGNIMENTO", "ST_SPEGNIMENTO_1", "ST_SPEGNIMENTO_2", "ST_SPENTO",
    "ST_REINIT_SMS", "ST_REINIT_SMS_1", "ST_REINIT_SMS_2", "ST_REINIT_SMS_3",
    "ST_SAETNET", "ST_SAETNET_ERROR", "ST_SAETNET_AVVIA", "ST_SAETNET_SBLOCCA",
    "ST_SAETNET_CHIUDI", "ST_SAETNET_FINE", "ST_SAETNET_CHIAMA", "ST_SAETNET_ERROR_2",
    "ST_CID_CHIAMA", "ST_CID_CHIAMA_2", "ST_CID_INVIA", "ST_CID_FINE", "ST_CID_FINE_2", "ST_CID_FINE_3"};
#endif

#define GSM_FLAG_SMS_PENDING	0x0001
#define GSM_FLAG_VOICE_CALL	0x0002
#define GSM_FLAG_DATA_CALL	0x0004
#define GSM_FLAG_CALL_CLOSE	0x0008
#define GSM_FLAG_CREDITO	0x0010
#define GSM_FLAG_SPEGNIMENTO	0x0020
#define GSM_FLAG_GSM_SPENTO	0x0040
#define GSM_FLAG_ACCENSIONE	0x0080
#define GSM_FLAG_PERIODIC	0x0100
#define GSM_FLAG_LISTA_SMS	0x0200
#define GSM_FLAG_SMS_CANCELLA	0x0400
#define GSM_FLAG_SMS_VERIFY	0x0800
#define GSM_FLAG_RESET		0x1000
#define GSM_FLAG_REINIT		0x2000

static int gsm_waitresp;
static int MMSfieldtype[] = {0,1,1,1,0,2,0,0,3,0,0,1,0,0,2,0,0,0,0,1,0,0,1,1,1};

static int gsm_send_mms(int fd, void *null)
{
  int n, contentlength = 0;
  fd_set fds;
  struct timeval to;
  
  int mmserr = 0;
  
  /* Se la connessione non va a buon fine non fa nulla. */
  if(fd < 0) return 0;
  
  write(fd, gsm_gprs_cur->data, gsm_gprs_cur->len);
  
  FD_ZERO(&fds);
  
  while(1)
  {
    to.tv_sec = 120;
    to.tv_usec = 0;
    FD_SET(fd, &fds);
    n = select(fd+1, &fds, NULL, NULL, &to);
    
    if(n)
    {
//---------
    
    n = read(fd, gsm_buf + gsm_buf_idx, 1);
    
    if(gsm_buf_idx == GSMBUFDIM-2)
    {
      /* msg too long, something is going wrong */
      gsm_buf[gsm_buf_idx] = '\n';
    }
    
    if(n > 0)
    {
      if(gsm_buf[gsm_buf_idx] != '\n')
      {
        gsm_buf_idx++;
        continue;
      }
    }
    
//---------
    }
    else if(!n)
    {
      support_log("MMS: Timeout");
      /* Reinvio l'mms */
      return 0;
    }
    
    gsm_buf[gsm_buf_idx] = 0;
    gsm_buf_idx = 0;
    
    if(!strncasecmp(gsm_buf, "Content-Length:", 15))
    {
      support_log(gsm_buf);
      contentlength = atoi(gsm_buf+15);
    }
    else if(!strncasecmp(gsm_buf, "NO CARRIER", 10))
    {
      support_log("MMS: NO CARRIER");
      /* Reinvio l'mms */
      return 0;
    }
    else if(gsm_buf[0] == '\r')
    {
      if(!contentlength)
      {
        support_log("MMS Error - null content");
        /* Reinvio l'mms */
        return 0;
      }
      
      to.tv_sec = 2;
      to.tv_usec = 0;
      FD_SET(fd, &fds);
      n = select(fd+1, &fds, NULL, NULL, &to);
      if(n)
      {
#if 0
        for(n=0; n<contentlength; n++)
        {
          read(fd, gsm_buf+n, 1);
          if(n == 10)
          {
            if(gsm_buf[n] == 0x80)
              support_log("MMS Ok");
            else
            {
              support_log("MMS Error");
              mmserr = 1;
              /* Reinvio l'mms */
              //return 0;
           }
          }
        }
#else
        /* E' necessario parsificare tutta la risposta MMS
           alla ricerca del campo Report Status. */
        do
        {
          n = read(fd, gsm_buf+gsm_buf_idx, contentlength-gsm_buf_idx);
          if(n <= 0)
          {
            support_log("MMS Timeout");
            gsm_buf_idx = 0;
            return 0;
          }
          gsm_buf_idx += n;
        }
        while(gsm_buf_idx < contentlength);
        gsm_buf_idx = 0;
        
        mmserr = 1;
        for(n=0; n<contentlength; )
        {
          if(gsm_buf[n] == 0x92)
          {
            /* Report Status */
            if(gsm_buf[n+1] == 0x80)
            {
              support_log("MMS Ok");
              mmserr = 0;
            }
            else
              support_log("MMS Error");
            break;
          }
          else
          {
            /* Salta il campo */
            switch(MMSfieldtype[gsm_buf[n]&0x7f])
            {
              case 0:	// short int
                n += 2;
                break;
              case 1:	// text string
                while(gsm_buf[n++]);
                break;
              default:	// 2=long int, 3=value length
                /* Non dovrebbero arrivare nell'esito */
                n = contentlength;
                break;
            }
          }
        }
#endif

#if 1
#warning Debug MMS
if(mmserr)
{
  FILE *fp;
  
  fp = fopen(ADDROOTDIR("/tmp/saet.mms.err"), "w");
  if(fp)
  {
    fwrite(gsm_buf, 1, contentlength, fp);
    fclose(fp);
  }
}
#endif
      }
      else
      {
        support_log("MMS Timeout");
      }
      support_log("MMS Return");
      if(mmserr) return 0;
      return 1;
    }
    else if(gsm_buf[0])
    {
      support_log(gsm_buf);
    }
  }
}

void gsm_queue_mms(char *mms, int len)
{
  unsigned char m[64];
  struct data *data;
  int n;
  
  if(!gsm_is_gprs) return;
  
  data = (struct data*)malloc(sizeof(struct data));
  
  data->type = GPRS_INIT_MMS;
  data->func = gsm_send_mms;
  data->ipaddr = strdup(gsm_operator_data[gsm_operator].gateway);
  for(n=0; gsm_operator_data[gsm_operator].gateway[n]&&(gsm_operator_data[gsm_operator].gateway[n]!=':'); n++);
  if(!gsm_operator_data[gsm_operator].gateway[n])
    data->port = 80;
  else
  {
    data->port = atoi(gsm_operator_data[gsm_operator].gateway+n+1);
    data->ipaddr[n] = 0;
  }
  data->data = mms;
  data->len = len;
  data->retry = 1;
  data->timeout = 18000;	// 30 minuti (per evitare che la coda cresca all'infinito in caso di problemi
  data->next = NULL;
  
  pthread_mutex_lock(&gsm_mutex);
  if(!gsm_gprs_head)
    gsm_gprs_head = data;
  else
    gsm_gprs_tail->next = data;
  gsm_gprs_tail = data;
  pthread_mutex_unlock(&gsm_mutex);
  
  m[0] = Evento_Esteso;
  m[1] = Ex_Stringa_GSM;
  m[2] = 103;
  sprintf(m+4, str_d_13);
  m[3] = 9;
  codec_queue_event(m);
}

void gsm_gprs_connect_ex(int (*func)(int,void*), char *ipaddr, int port, int type, void *param)
{
  struct data *data;
  
  if(!gsm_is_gprs)
  {
    /* Se il GPRS non è disponibile devo sbloccare l'invio in backup. */
    if(func) func(-1, NULL);
    return;
  }
  
  data = (struct data*)malloc(sizeof(struct data));
  
  data->type = type;
  data->func = func;
  data->ipaddr = strdup(ipaddr);
  data->port = port;
  data->data = param;
  data->retry = 2;
  data->timeout = 3000;	// 5 minuti
  data->next = NULL;
  
  pthread_mutex_lock(&gsm_mutex);
  if(!gsm_gprs_head)
    gsm_gprs_head = data;
  else
    gsm_gprs_tail->next = data;
  gsm_gprs_tail = data;
  pthread_mutex_unlock(&gsm_mutex);
}

void gsm_gprs_connect(int (*func)(int,void*), char *ipaddr, int port, void *param)
{
  gsm_gprs_connect_ex(func, ipaddr, port, GPRS_INIT_TCP, param);
}

void gsm_send(char *cmd, int timeout)
{
  if(!gsm_waitresp)
  {
    write(gsm_fd, cmd, strlen(cmd));
    gsm_waitresp = 1;
    timeout_on(gsm_timeout, NULL, NULL, 0, timeout);
  }
}

static void* gsm_manage(void *null)
{
  int ret, n;
  unsigned char gsm_event[256];
  int gsm_stato, gsm_creg_counter, gsm_cmeerror_counter;
  int gsm_delay_next, gsm_retry;
  int gsm_mittente, gsm_test_len, gsm_sms_idx, gsm_sms_max, gsm_sms_num;
  int gsm_incoming_data;
#ifdef __arm__
  int gpio_fd;
#endif

  pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
  
  debug_pid[gsm_consumer] = support_getpid();
  
  sprintf(gsm_event, "GSM start [%d]", debug_pid[gsm_consumer]);
  support_log(gsm_event);
  
  gsm_event[0] = Evento_Esteso;
  gsm_event[1] = Ex_Stringa_GSM;
  
  gsm_creg_counter = gsm_cmeerror_counter = gsm_delay_next = gsm_waitresp = gsm_retry = 0;
  gsm_sms_pending_in = gsm_sms_pending_out = 0;
  gsm_mittente = gsm_test_len = gsm_sms_idx = gsm_sms_max = gsm_sms_num = 0;
  
  gsm_incoming_data = -1;
  
  CID_init();
  
#ifdef __arm__
  /* GPIO per accensione e spegnimento modulo */
  gpio_fd = open("/dev/gpio", O_RDWR);
#endif
  
  /* Alla partenza verifico che non ci siano sms non letti... */
  gsm_flags = GSM_FLAG_SMS_PENDING;
  gsm_stato = ST_START;
  
  while(1)
  {
    if(gsm_fd < 0)
    {
      usleep(100000);
      n = 0;
    }
    else
      n = read(gsm_fd, gsm_buf + gsm_buf_idx, 1);
    if(n)
    {
      if(gsm_buf_idx && (gsm_buf[0] == '>') && (gsm_stato == ST_SMS_ATTESA_PROMPT))
      {
        support_log("Invio SMS");
        gsm_buf[0] = 0;
        gsm_waitresp = 0;
        gsm_stato++;
      }
      else if(gsm_buf[gsm_buf_idx] == 0x1a)
      {
        gsm_buf[gsm_buf_idx] = 0;
      }
      else if((gsm_buf[gsm_buf_idx] != '\r') && (gsm_buf[gsm_buf_idx] != '\n'))
      {
        gsm_buf_idx++;
        if(gsm_buf_idx == GSMBUFDIM)
        {
          /* msg too long, something is going wrong */
          while(read(gsm_fd, gsm_buf, GSMBUFDIM));
          gsm_buf_idx = 0;
        }
        continue;
      }
      else
      {
        if(gsm_buf[gsm_buf_idx-1] == '\r')
          gsm_buf[gsm_buf_idx-1] = 0;
        else
          gsm_buf[gsm_buf_idx] = 0;
      }
    }
    else
    {
      struct data *conn;
      
      /* Scarta quanto eventualmente ricevuto */
      gsm_buf_idx = 0;
      
      /* Gestione dei timeout della coda SMS */
      /* ATTENZIONE: la gestione del timeout viene interrotta
         durante la chiamata alla callback. La funzione di
         callback è bloccante per la macchina a stati del gsm. */
      for(conn=gsm_sms_head; conn; conn=conn->next)
      {
        if(conn->timeout > 0)
        {
          conn->timeout--;
          if(!conn->timeout && conn->cbk) conn->cbk(-3, conn->cbk_data);
        }
      }
      
      /* Gestione dei timeout della coda GPRS */
      /* ATTENZIONE: la gestione del timeout viene interrotta
         durante la chiamata alla callback. La funzione di
         callback è bloccante per la macchina a stati del gsm. */
      for(conn=gsm_gprs_head; conn; conn=conn->next)
      {
        if(conn->timeout > 0)
        {
          conn->timeout--;
          if(!conn->timeout && conn->func) conn->func(-1, conn->data);
        }
      }
    }
    
    /* Scarta tutti gli sms in coda scaduti */
    while(gsm_sms_head && !(gsm_sms_head->timeout))
    {
      struct data *conn;
      
      conn = gsm_sms_head->next;
      free(conn->ipaddr);
      free(conn);
      gsm_sms_head = conn;
    }
    
    /* Scarta tutte le connessioni gprs in coda scadute */
    while(gsm_gprs_head && !(gsm_gprs_head->timeout))
    {
      struct data *conn;
      
      conn = gsm_gprs_head->next;
      free(conn->ipaddr);
      free(conn);
      gsm_gprs_head = conn;
    }
    
    /* Potrebbe arrivare un sms vuoto, quindi in questo caso la riga vuota
       non deve essere ingorata ma essere gestita come il testo dell'sms. */
    if(gsm_buf_idx || (gsm_stato == ST_SMS_LEGGI_TESTO))
    {
      //gsm_retry = 0;
      gsm_buf_idx = 0;
      
      if(gsm_buf[0] || (gsm_stato == ST_SMS_LEGGI_TESTO))
      {
      
      support_log2(gsm_buf);
    
#ifdef GSM_DEBUG_PRINT
#warning !!! GSM printf !!!
printf("** (%s) %s\n", statod[gsm_stato], gsm_buf);
#endif
#ifdef SAET_CONSOLE
      if((gsm_stato == ST_SMS_LISTA) || (gsm_stato == ST_SMS_LISTA_2))
        console_sms_message(gsm_buf, gsm_selint);
#endif
      
      /* Riconoscimento modulo GSM, non usa più /saet/modem.conf */
      if(gsm_stato == ST_INIT_DETECT_1)
      {
        if(!strcmp(gsm_buf, "332"))
          gsm_stato = ST_INIT_DETECT_2;
      }
      else if(gsm_stato == ST_INIT_DETECT_3)
      {
#if 0
        gsm_stato = ST_INIT_DETECT_4;
        if(!strcmp(gsm_buf, "GM862") || !strcmp(gsm_buf, "GM862-GSM"))
          gsm_is_gprs = 0;
        else if(!strcmp(gsm_buf, "GM862-GPRS") || !strcmp(gsm_buf, "GM862-PCS") ||
                !strcmp(gsm_buf, "GM862 QUAD") || !strcmp(gsm_buf, "GM862-QUAD"))
          gsm_is_gprs = 1;
        else
          gsm_stato = ST_INIT_DETECT_3;
#else
        if(!strncmp(gsm_buf, "GM862", 5))
        {
          gsm_is_gprs = 0;
          if(!strcmp(gsm_buf+6, "GPRS") || !strcmp(gsm_buf+6, "PCS") ||
             !strcmp(gsm_buf+6, "QUAD")) gsm_is_gprs = 1;
          gsm_stato = ST_INIT_DETECT_4;
        }
        else if(!strncmp(gsm_buf, "GC864", 5) || !strncmp(gsm_buf, "GL865", 5))
        {
          gsm_is_gprs = 1;
          gsm_stato = ST_INIT_DETECT_4;
        }
#endif
      }
      else if(gsm_stato == ST_INIT_VERSION)
      {
        gsm_stato = ST_INIT_VERSION_READ;
      }
      else if(gsm_stato == ST_INIT_VERSION_READ)
      {
        char m[64];
        if(!strncmp(gsm_buf, "+CGMR:", 6))
          sprintf(m, "GSM version: %s (type %d)", gsm_buf+7, MODEM_GSM+gsm_is_gprs);
        else
          sprintf(m, "GSM version: %s (type %d)", gsm_buf, MODEM_GSM+gsm_is_gprs);
        support_log(m);
        gsm_stato = ST_INIT_VERSION_OK;
      }
      
      /* Gestione delle ricezioni */
      if(gsm_stato == ST_SMS_LEGGI_TESTO)
      {
        /* Quando faccio una lettura con +CMGR o +CMGL ... */
        gsm_stato = ST_SMS_ARRIVATO;
      }
      else if(gsm_stato == ST_SMS_INOLTRA)
      {
/*
        strcpy(gsm_buffer_inoltra, gsm_buffer);
        gsm_invia_sms(GSM_SMS_PRIORITARIO|GSM_SMS_DEST_PRIMO, GSM_SMS_TESTO_INOLTRA);
*/
        gsm_buf[160] = 0;
        GSM_SMS_send(1, gsm_buf);
        gsm_stato = ST_SMS_ATTENDI_OK_PER_CANCELLARE;
      }
      else if(gsm_stato == ST_CREDITO_RESIDUO)
      {
        gsm_stato = ST_VERIFICA_CREDITO;
      }
#if 0
/* (06/05/2016) E' capitato di nuovo, quindi il taccone non funziona sempre.
   Lo sostituisco con l'azzeramento dei retry a fronte dell'OK e simili,
   così risolvo anche un altro caso simile. */
      else if(!strcmp(gsm_buf, "0") && (gsm_stato == ST_AUDIO_TERMINA))
      {
        /* E' capitato su un impianto con GSM vecchio (1.05.xx) che a seguito
           di messaggio vocale, l'esito tornato al comando AT fosse numerico.
           Si è alterata l'impostazione del formato esito. Non so cosa si possa
           essere alterato di altro, quindi per sicurezza spengo e riaccendo. */
        gsm_delay_next = 0;
        pthread_mutex_lock(&gsm_mutex);
        gsm_flags |= GSM_FLAG_ACCENSIONE;
        pthread_mutex_unlock(&gsm_mutex);
        gsm_stato = ST_SPEGNIMENTO;
      }
#endif
      else if(!strcmp(gsm_buf, "OK"))
      {
        timeout_off(gsm_timeout);
        gsm_cmeerror_counter = 0;
        gsm_retry = 0;
        
        if(gsm_creg_counter > 150)
        {
          gsm_delay_next = 0;
          pthread_mutex_lock(&gsm_mutex);
          gsm_flags |= GSM_FLAG_ACCENSIONE;
          pthread_mutex_unlock(&gsm_mutex);
          gsm_stato = ST_SPEGNIMENTO;
        }
        else if(gsm_delay_next)
        {
          /* Aggiunge una pausa di 1s e poi fa scattare il timeout */
          timeout_on(gsm_timeout, NULL, NULL, 0, gsm_delay_next*10);
          gsm_delay_next = 0;
        }
        else
        {
          /* Aggiunge una pausa di 100ms e poi fa scattare il timeout */
          timeout_on(gsm_timeout, NULL, NULL, 0, 1);
          /* Passa subito allo stato successivo, ma se c'è una send aspetta lo scadere del timeout. */
          gsm_stato++;
        }
      }
      else if((!strcmp(gsm_buf, "NO CARRIER")) ||
              (!strcmp(gsm_buf, "BUSY")) ||
              (!strcmp(gsm_buf, "NO DIALTONE")) ||
              (!strcmp(gsm_buf, "NO DIAL TONE")) ||
              (!strcmp(gsm_buf, "NO ANSWER")))
      {
        /* Aggiunge una pausa di 100ms e poi fa scattare il timeout */
        timeout_on(gsm_timeout, NULL, NULL, 0, 1);
        gsm_retry = 0;
        
        if(gsm_stato == ST_AUDIO_ATTESA_RISPOSTA)
        {
          if(gsm_call_cur->retry)
          {
            /* Riaccodo per un altro tentativo */
            gsm_call_cur->retry--;
            pthread_mutex_lock(&gsm_mutex);
            if(!gsm_call_head)
              gsm_call_head = gsm_call_cur;
            else
              gsm_call_tail->next = gsm_call_cur;
            gsm_call_tail = gsm_call_cur;
            pthread_mutex_unlock(&gsm_mutex);
            gsm_call_cur = NULL;
          }
          gsm_stato = ST_AUDIO_TERMINA;
        }
        else if(gsm_stato == ST_CID_CHIAMA_2)
        {
          /* Alterno il numero base con quello di backup.
             Assumo che il numero base sia sempre valido (è verificato
             alla generazione dell'evento), devo verificare quello
             di backup. */
          if(gsm_cid_num == gsm_cid_num_base)
          {
            if((config.PhoneBook[gsm_cid_num_base/*-1+1*/].Abil & PB_TX) &&
               config.PhoneBook[gsm_cid_num_base/*-1+1*/].Phone[0])
              gsm_cid_num += 1;
          }
          else
            gsm_cid_num = gsm_cid_num_base;
          timeout_on(CID_delay, NULL, NULL, 0, CID_DELAY);
          gsm_stato = ST_MAIN;
        }
        else
        {
          /* Passa subito allo stato successivo, ma se c'è una send aspetta lo scadere del timeout. */
          gsm_stato++;
        }
      }
      else if(!strncmp(gsm_buf, "+CMS ERROR:", 11))
      {
        /* Aggiunge una pausa di 300ms e poi fa scattare il timeout */
        timeout_on(gsm_timeout, NULL, NULL, 0, 3);
        gsm_retry = 0;
        
        /* Passa subito allo stato successivo, ma se c'è una send aspetta lo scadere del timeout. */
        /* Se però sono ancora in inizializzazione, allora ripeto il comando che DEVE dare OK,
           pena il malfunzionamento della centrale. */
        if(gsm_stato >= ST_MAIN)
        {
          if(!strcmp(gsm_buf, "+CMS ERROR: 302") ||
             !strcmp(gsm_buf, "+CMS ERROR: 303") ||
             !strcmp(gsm_buf, "+CMS ERROR: 304"))
          {
            /* Per qualche motivo il modulo GSM non risulta configurato correttamente,
               quindi lo reinizializzo. Ho poi ancora un tentativo per l'invio dell'sms
               corrente che rimane in 'gsm_sms_cur'. */
            gsm_stato = ST_INIT_2;
          }
          else if(!strcmp(gsm_buf, "+CMS ERROR: 321") && (gsm_stato == ST_SMS_LEGGI))
          {
            gsm_stato = ST_MAIN;
          }
          else if(!strcmp(gsm_buf, "+CMS ERROR: 322"))
          {
            /* E' un errore che non dovrebbe mai capitare, invece è capitato 2 volte
               in un anno, è quindi comunque molto raro. In attesa che Telit ci spieghi
               perché capita, visto che con un riavvio si risolve faccio che spegnere
               e riaccendere il modulo. */
/*
10/05/11 15:55:50 - AT+CMGS="+393493853720"
10/05/11 15:55:50 - ALL.S.28 ALL. GENERICO UPS ANAGRAFE
10/05/11 15:55:50 - +CMS ERROR: 322
*/
            /* Tra l'altro, quando è capitato, il gsm era stato appena reinizializzato
               per via di un aggiornamento di programma utente che aveva fatto ripartire
               la centrale, quindi non credo che basti reinizializzare, è molto probabile
               che si debba proprio spegnere e riaccendere. */
/*
10/05/11 15:46:11 - GSM version: 07.02.603 (type 3)
10/05/11 15:46:11 - GSM Init
...
10/05/11 15:46:23 - AT+CPMS?
10/05/11 15:46:23 - +CPMS: "SM",1,50,"SM",1,50,"SM",1,50
10/05/11 15:46:23 - OK
...
e poi nulla di particolare fino all'invio sms che ha restituito errore.
*/
            pthread_mutex_lock(&gsm_mutex);
            gsm_flags |= GSM_FLAG_ACCENSIONE;
            pthread_mutex_unlock(&gsm_mutex);
            gsm_stato = ST_SPEGNIMENTO;
          }
          else
            gsm_stato++;
        }
      }
      else if(!strncmp(gsm_buf, "+CME ERROR:", 11))
      {
        gsm_retry = 0;
        
        if(!strcmp(gsm_buf+11, " 10")) GSM_Status[3] = 2;
        else if(!strcmp(gsm_buf+11, " 13")) GSM_Status[3] = 3;
        else if(!strcmp(gsm_buf+11, " 15")) GSM_Status[3] = 5;
        GSM_Status[1] = 0;
        
        /* E' capitato che il GSM iniziasse a dare +CME ERROR: 13 e non
           ne uscisse più. Occorre spegnere e riaccendere e poi si riprende. */
        gsm_cmeerror_counter++;
        if((strlen(gsm_buf) > 14) && (gsm_buf[12] == '5'))
        {
          /* Errore GPRS (5XX), ripeto il ciclo. */
          /* Aggiunge una pausa di 100ms e poi fa scattare il timeout */
          timeout_on(gsm_timeout, NULL, NULL, 0, 1);
          /* Passa subito allo stato successivo, ma se c'è una send aspetta lo scadere del timeout. */
          gsm_stato++;
        }
        else if(gsm_cmeerror_counter < 10)
        {
          if(!strncmp(gsm_buf+12, "100", 3))
          {
            /* Capita con credito Wind di ricevere CME ERROR 100 (unknown) in modo
               sistematico, per ora quindi salto semplicemente la richiesta credito. */
            gsm_stato++;
          }
          else
          /* Aggiunge una pausa di 1.5s e poi fa scattare il timeout */
          if(GSM_Status[3])
            timeout_on(gsm_timeout, NULL, NULL, 0, 50);
          else
            timeout_on(gsm_timeout, NULL, NULL, 0, 15);
        }
        else
        {
          pthread_mutex_lock(&gsm_mutex);
          gsm_flags |= GSM_FLAG_ACCENSIONE;
          pthread_mutex_unlock(&gsm_mutex);
          gsm_stato = ST_SPEGNIMENTO;
        }
      }
      else if(!strncmp(gsm_buf, "ERROR", 5))
      {
        gsm_retry = 0;
        
        if(gsm_stato == ST_INIT_7E) gsm_dialmode = 0;
        else if(gsm_stato == ST_INIT_4) gsm_stato = ST_INIT_1; // diventa 2 subito qui sotto
        
        /* Aggiunge una pausa di 100ms e poi fa scattare il timeout */
        timeout_on(gsm_timeout, NULL, NULL, 0, 1);
        /* Passa subito allo stato successivo, ma se c'è una send aspetta lo scadere del timeout. */
        gsm_stato++;
      }
      else if(!strncmp(gsm_buf, "CONNECT", 7))
      {
        timeout_off(gsm_timeout);
        gsm_waitresp = 0;
        gsm_buf_idx = 0;
        gsm_retry = 0;
        
        if((gsm_stato == ST_SAETNET) || (gsm_stato == ST_SAETNET_CHIAMA))
          gsm_stato = ST_SAETNET_AVVIA;
        else if(gsm_stato == ST_GPRS_CONNECT)
          gsm_stato = ST_GPRS_TRASMETTI;
        else if(gsm_stato == ST_AUDIO_REC)
          gsm_stato++;
        else
          gsm_stato = ST_SAETNET_SBLOCCA;
      }
      else if(!strncmp(gsm_buf, "+CMTI:", 6))
      {
        sscanf(gsm_buf+12, "%d", &n);
        /* Aggiungo in elenco l'indice sms da leggere, poi lo leggo quando posso. */
        gsm_sms_pending[gsm_sms_pending_in++] = n;
        gsm_sms_pending_in &= 7;
        /* Se ho molti sms da leggere, faccio che leggerli via CMGL. */
        if(gsm_sms_pending_in == gsm_sms_pending_out)
        {
          pthread_mutex_lock(&gsm_mutex);
          gsm_flags |= GSM_FLAG_SMS_PENDING;
          pthread_mutex_unlock(&gsm_mutex);
        }
#ifdef SAET_CONSOLE
        console_sms_message(gsm_buf, gsm_selint);
#endif
      }
      else if(!strncmp(gsm_buf, "+CSQ:", 5))
      {
        if(sscanf(gsm_buf+6, "%d", &n) > 0)
        {
          GSM_Status[4] = n;
          //if(n != 99)
          if(n <= 31)
          {
            gsm_stat_line_val += n;
            gsm_stat_line_cnt++;
            gsm_stat_line_act = n;
            if(gsm_stat_line_min > n)
            {
              gsm_stat_line_min = n;
              gsm_stat_line_time_min = time(NULL);
            }
            if(gsm_stat_line_max < n)
            {
              gsm_stat_line_max = n;
              gsm_stat_line_time_max = time(NULL);
            }
          }
#ifdef SAET_CONSOLE
          console_sms_message(gsm_buf, gsm_selint);
#endif
        }
      }
      else if(!strncmp(gsm_buf, "+CMGR: \"REC ", 12))
      {
/*
+CMGR: "REC UNREAD","+404",,"08/01/25,11:32:55+04"
Vodafone 25-01-2008,  11:32: Traff. Dispon. E. 43.69, Ult. Addebito E. 0.19
OK
+CMGR: "REC READ","+393408709107","Giancarlo","08/11/25,16:11:24+04"
Test
OK
*/
        if(!strncmp(gsm_buf, "+CMGR: \"REC UNREAD\"", 19))
        {
          char num[32];
          
          memcpy(num, gsm_buf+21, 32);
          for(n=0; num[n]!='\"'; n++);
          num[n] = 0;
          n = PhoneBook_find(num);
          if((n>=0) && (config.PhoneBook[n].Abil & PB_SMS_RX))
          {
            support_log("SMS ricevuto");
            
            gsm_mittente = n + 1;
            
            if(config.PhoneBook[n].Name)
              support_received_number(config.PhoneBook[n].Name);
            else
              support_received_number(config.PhoneBook[n].Phone);
            
            for(n=strlen(gsm_buf); gsm_buf[n]!=','; n--);
            sscanf(gsm_buf+n+1, "%d", &gsm_test_len);
            
            /* Ho ancora in coda lo "\n" che precede il testo dell'sms,
               lo devo scartare altrimenti prendo lui come testo del messaggio
               e non ciò che segue. Nel caso l'sms fosse poi vuoto andrò a
               gestire i nuovi a capo come corpo del messaggio. */
            read(gsm_fd, gsm_buf, 1);
            
            gsm_stato = ST_SMS_LEGGI_TESTO;
          }
          else if((gsm_operator == 0) && !strcmp(num, "40916"))
          {
            /* Verifica se è un sms di credito residuo TIM */
            if(gsm_credito_timeout->active)
            {
              gsm_stato = ST_CREDITO_RESIDUO;
              timeout_off(gsm_credito_timeout);
            }
            else if(gsm_sms_pending_out != gsm_sms_pending_in)
              gsm_stato = ST_SMS_ATTENDI_OK;
            else
              gsm_stato = ST_SMS_ATTENDI_OK_PER_CANCELLARE;
          }
          else if((gsm_operator == 1) && (!strcmp(num, "404") || !strcmp(num, "+404")))
          {
            /* Verifica se è un sms di credito residuo Vodafone */
            if(gsm_credito_timeout->active)
            {
              gsm_stato = ST_CREDITO_RESIDUO;
              timeout_off(gsm_credito_timeout);
            }
            else if(gsm_sms_pending_out != gsm_sms_pending_in)
              gsm_stato = ST_SMS_ATTENDI_OK;
            else
              gsm_stato = ST_SMS_ATTENDI_OK_PER_CANCELLARE;
          }
          else if(!strcmp(num, "Vodafone"))
          {
            /* Notifiche di Vodafone da inoltrare */
            gsm_stato = ST_SMS_INOLTRA;
          }
          else
          {
            support_log("SMS non autorizzato");
            if(gsm_sms_pending_out != gsm_sms_pending_in)
              gsm_stato = ST_SMS_ATTENDI_OK;
            else
              gsm_stato = ST_SMS_ATTENDI_OK_PER_CANCELLARE;
          }
        }
        else if(gsm_stato != ST_SMS_LISTA_2)
        {
          gsm_stato = ST_SMS_ATTENDI_OK_PER_CANCELLARE;
        }
        
        /* Calcola l'indice dell'sms da cancellare */
        gsm_sms_idx++;
        if(gsm_sms_idx > gsm_sms_max) gsm_sms_idx = 1;
      }
      else if(!strncmp(gsm_buf, "+CMGR: 0,", 9) ||
              !strncmp(gsm_buf, "+CMGR: 1,", 9))
      {
        /* E' arrivato un sms ma per qualche motivo viene letto in formato PDU.
           Reinizializzo il gsm e rileggo l'sms. */
        /* Attenzione: se reinizializzo il gsm e l'sms ricevuto era l'ultimo
           slot disponibile, l'indicazione gsm_sms_idx viene persa al +CPMS,
           occorre quindi prima cancellare, poi reinizializzare e leggere
           nuovamente l'sms ricevuto. Ma pure la cancellazione altera gsm_sms_idx!
           Invece di reinizializzare genericamente il modulo, creo un nuovo
           stato per il solo invio delle impostazioni e forzo la rilettura
           dell'sms tramite flag. In questo modo non altero l'indice dell'sms
           da leggere. Attenzione all'ordine in cui si fanno le cose in ST_MAIN. */
        gsm_stato = ST_REINIT_SMS;
      }
      else if(!strncmp(gsm_buf, "+CMGS:", 6))
      {
        if(gsm_sms_cur->cbk)
          gsm_sms_cur->cbk(0, gsm_sms_cur->cbk_data);
        
        free(gsm_sms_cur->text);
        free(gsm_sms_cur);
        gsm_sms_cur = NULL;
        
        support_log("SMS Inviato");
        support_called_number_success();
      }
      else if(!strncmp(gsm_buf, "+CMGF:", 6))
      {
        sscanf(gsm_buf+6, "%d", &n);
        if(n != 1)
        {
          /* Il GSM non è più configurato correttamente,
             occorre ripetere l'inizializzazione. */
          gsm_stato = ST_REINIT_SMS;
        }
      }
/*
+CRING: VOICE
+CLIP: "+393408709107",145,"",128,"",0
+CRING: VOICE
+CLIP: "+393408709107",145,"",128,"",0
*/
      else if(!strncmp(gsm_buf, "+CRING: VOICE", 13))
      {
        gsm_incoming_data = 0;
        support_log("GSM Ring (Voice)");
      }
      else if(!strncmp(gsm_buf, "+CRING: DATA", 12))	// SELINT 0
      {
        gsm_incoming_data = 1;
        support_log("GSM Ring (Data)");
      }
      else if(!strncmp(gsm_buf, "+CRING: ", 8) && strstr(gsm_buf+8, "SYNC"))	// SELINT 2
      {
        gsm_incoming_data = 1;
        support_log("GSM Ring (Data)");
      }
      else if(!strncmp(gsm_buf, "+CLIP:", 6))
      {
        char m[64];
        
        if(gsm_incoming_data < 0) continue;
        
        sprintf(m, "GSM Num:%s", gsm_buf + 6);
        support_log(m);
        
        for(n=8; gsm_buf[n] != '"'; n++);
        gsm_buf[n] = 0;
      
        n = PhoneBook_find(gsm_buf+8);
        if(n < 0)
        {
          gsm_incoming_data = -1;
          gsm_ring_from_sconosciuto = 1;
          continue;
        }
        
        if(config.PhoneBook[n].Name)
          support_received_number(config.PhoneBook[n].Name);
        else
          support_received_number(config.PhoneBook[n].Phone);
        
        if(gsm_incoming_data && !gsm_ring_as_voice)
        {
          if((config.PhoneBook[n].Abil & PB_RX) && (GSM_Status[2] & 0x2))
          {
            GSM_Status[1] = 2;
            pthread_mutex_lock(&gsm_mutex);
            gsm_flags |= GSM_FLAG_DATA_CALL;
            pthread_mutex_unlock(&gsm_mutex);
          }
          else
          {
            /* Anche se il numero è in rubrica, non è abilitato e lo
               gestisco come se fosse sconosciuto. */
            gsm_incoming_data = -1;
            gsm_ring_from_sconosciuto = 1;
          }
        }
        else
        {
          if((config.PhoneBook[n].Abil & PB_GSM_RX) && (GSM_Status[2] & 0x1))
          {
            if(audio_ready())
            {
              /* Registrazione messaggio audio */
              GSM_Status[1] = 3;
              pthread_mutex_lock(&gsm_mutex);
              gsm_flags |= GSM_FLAG_VOICE_CALL | GSM_FLAG_CALL_CLOSE;
              pthread_mutex_unlock(&gsm_mutex);
            }
            else
            {
              /* Gestione squillo. */
              gsm_ring_from = n+1;
              pthread_mutex_lock(&gsm_mutex);
              gsm_flags |= GSM_FLAG_CALL_CLOSE;
              pthread_mutex_unlock(&gsm_mutex);
            }
          }
          else
          {
            /* Anche se il numero è in rubrica, non è abilitato e lo
               gestisco come se fosse sconosciuto. */
            gsm_incoming_data = -1;
            gsm_ring_from_sconosciuto = 1;
          }
        }
      }
      else if(!strncmp(gsm_buf, "+CMGL:", 6))
      {
        /* Reimposto il timeout per non farlo scadere a metà elenco. */
        timeout_on(gsm_timeout, NULL, NULL, 0, 300);
        
        for(n=6; gsm_buf[n] != ','; n++);
        if(!strncmp(gsm_buf+n+1, "\"REC UNREAD\"", 12))
        {
          char num[32];
          int i;
          
          memcpy(num, gsm_buf+n+15, 32);
          for(i=0; num[i]!='\"'; i++);
          num[i] = 0;
          n = PhoneBook_find(num);
          if((n>=0) && (config.PhoneBook[n].Abil & PB_SMS_RX))
          {
            support_log("SMS ricevuto");
            gsm_mittente = n + 1;
            if(config.PhoneBook[n].Name)
              support_received_number(config.PhoneBook[n].Name);
            else
              support_received_number(config.PhoneBook[n].Phone);
          
            for(n=strlen(gsm_buf); gsm_buf[n]!=','; n--);
            sscanf(gsm_buf+n+1, "%d", &gsm_test_len);
            
            /* Ho ancora in coda lo "\n" che precede il testo dell'sms,
               lo devo scartare altrimenti prendo lui come testo del messaggio
               e non ciò che segue. Nel caso l'sms fosse poi vuoto andrò a
               gestire i nuovi a capo come corpo del messaggio. */
            read(gsm_fd, gsm_buf, 1);
            
            gsm_stato = ST_SMS_LEGGI_TESTO;
          }
          else
            support_log("SMS non autorizzato");
          
          i = 0;
        }
      }
      else if(!strncmp(gsm_buf, "+CREG:", 6))
      {
        if(gsm_stato < ST_MAIN)
        {
          /* Se non è ancora registrato, all'OK reinvio la richiesta. */
          if(strncmp(gsm_buf+6, " 0,1", 4) && strncmp(gsm_buf+6, " 0,5", 4))
            gsm_delay_next = 2;
          else
            gsm_connected = 1;
          gsm_creg_counter++;
        }
        else if(!strncmp(gsm_buf+6, " 0,3", 4))
        {
          /* Il GSM si è disconnesso, se lo rimane per troppo tempo spengo e riaccendo. */
          gsm_stat_line_time_avg = time(NULL);
          gsm_connected = 0;
          gsm_creg_counter++;
          gsm_delay_next = 2;
        }
        else if(strncmp(gsm_buf+6, " 0,1", 4) && strncmp(gsm_buf+6, " 0,5", 4))
        {
          /* Se il GSM è disconnesso, ma vedo che sta provando a riconnettersi, allora
             aspetto a spegnere e riaccendere. In pratica spengo solo se rimane nello stato 3
             continuamente per molto tempo. */
          gsm_stat_line_time_avg = time(NULL);
          gsm_connected = 0;
          gsm_creg_counter = 0;
        }
        else
        {
          gsm_connected = 1;
          gsm_creg_counter = 0;
        }
      }
      else if(!strncmp(gsm_buf, "+CPMS:", 6))
      {
        /* Misura la dimensione della SIM per la gestione degli sms. */
        sscanf(gsm_buf+12, "%d,%d", &gsm_sms_idx, &gsm_sms_max);
        if(gsm_sms_max == 0)
        {
          /* C'è qualcosa che non va, ultimamente (15/03/2012) è capitato
             un certo numero di volte di ricevere per qualche motivo:
             +CPMS: "SM",0,0,"SM",0,0,"SM",0,0
             e questa condizione non va più via. Quindi quando ci casco
             dentro, spengo e riaccendo. */
          support_log("Errore SIM (SMS max=0)");
          pthread_mutex_lock(&gsm_mutex);
          gsm_flags |= GSM_FLAG_RESET;
          pthread_mutex_unlock(&gsm_mutex);
        }
        else if(gsm_sms_idx == gsm_sms_max)
        {
          /* Se la SIM è piena cancello un sms. */
          /* Però prima devo vedere se non ci sia un sms non letto...
             Se sfiga vuole che l'sms non letto sia proprio l'1, pazienza, lo perdo.
             Successivamente mi faccio una scansione dei non letti con tranquillità.
             Fare prima la scansione ricordandomi che poi devo cancellare è più
             complicato e tutto sommato senza un grande valore aggiunto. */
          pthread_mutex_lock(&gsm_mutex);
          gsm_flags |= GSM_FLAG_SMS_PENDING|GSM_FLAG_SMS_CANCELLA;
          pthread_mutex_unlock(&gsm_mutex);
        }
        else if((gsm_flags & GSM_FLAG_SMS_VERIFY) && (gsm_sms_idx != gsm_sms_num))
        {
          /* E' variato il numero di sms nella sim, quindi è plausibile
             che ne siano arrivati di nuovi non gestiti, tipicamente
             durante una riproduzione audio. */
          pthread_mutex_lock(&gsm_mutex);
          gsm_flags |= GSM_FLAG_SMS_PENDING;
          pthread_mutex_unlock(&gsm_mutex);
        }
        else
        {
          pthread_mutex_lock(&gsm_mutex);
          gsm_flags &= ~GSM_FLAG_SMS_VERIFY;
          pthread_mutex_unlock(&gsm_mutex);
        }
        gsm_sms_num = gsm_sms_idx;
      }
      else if(!strncmp(gsm_buf, "+COPS: 0,2", 10))
      {
        for(n=0; gsm_operator_data[n].code; n++)
          if(!strncmp(gsm_buf+12, gsm_operator_data[n].code, 5))
          {
            char m[64];
            gsm_operator = n;
            sprintf(m, "GSM - Operatore: %.5s", gsm_operator_data[n].code);
            support_log(m);
            break;
          }
      }
      else if((gsm_operator == GSM_OPERATOR_WIND) && !strncmp(gsm_buf, "+CUSD:", 6))
      {
        /* Verifica se è un sms di credito residuo Wind */
        if(gsm_credito_timeout->active)
        {
          gsm_stato = ST_VERIFICA_CREDITO;
          timeout_off(gsm_credito_timeout);
        }
      }
      else if(!strncmp(gsm_buf, "#SELINT:", 8))
      {
        sscanf(gsm_buf+8, "%d", &gsm_selint);
        if(gsm_selint != 2) gsm_stato = ST_IMPOSTA_SELINT;
      }
      else if(!strncmp(gsm_buf, "#SCFG: 1,", 9))
      {
        if(strncmp(gsm_buf+9, "1,16,60,", 8))
          gsm_stato = ST_GPRS_INIT_1B;
        /* Quando arriva l'OK passa poi subito a 1B ed invia la nuova configurazione. */
      }
      else if(!strncmp(gsm_buf, "+CSCA:", 6))
      {
        strcpy(gsm_csca, gsm_buf + 7);
      }
      else if(!strncmp(gsm_buf, "#MONI:", 6))
      {
        if(!strncmp(gsm_buf + 7, "Cc:000 ", 7))
          gsm_delay_next = 2;
        else
        {
          int i;
          for(i=15; i<80; i++)
          {
            if(gsm_buf[i] == ' ')
            {
              if((gsm_buf[i+1] == 'L') || (gsm_buf[i+1] == 'P'))
              {
                gsm_buf[i] = 0;
              }
            }
          }
          i = 0;
          strncpy(gsm_moni1, gsm_buf + 7, 36);
          gsm_moni1[35] = 0;
          strncpy(gsm_moni2, gsm_buf + 8 + strlen(gsm_buf + 7), 36);
          gsm_moni2[35] = 0;
        }
      }
      else if(!strncmp(gsm_buf, "+CPIN:", 6))
      {
        if(!strcmp(gsm_buf+6, " READY"))
          GSM_Status[3] = 0;
        else if(!strcmp(gsm_buf+6, " SIM PIN"))
        {
          GSM_Status[3] = 1;
          gsm_delay_next = 5;
        }
      }
      
      }	// if(gsm_buf[0])
    }
    
    if(!gsm_timeout->active && gsm_waitresp)
    {
      /* Timeout ultimo comando */
      gsm_waitresp = 0;
      gsm_buf_idx = 0;
      
      gsm_retry++;
      if(gsm_retry > 3)
      {
        /* Se non risponde per 3 volte, provo a spegnere e riaccendere. */
        gsm_retry = 0;
        pthread_mutex_lock(&gsm_mutex);
        gsm_flags |= GSM_FLAG_ACCENSIONE;
        pthread_mutex_unlock(&gsm_mutex);
        gsm_stato = ST_SPEGNIMENTO;
      }
    }
    
#ifdef GSM_DEBUG_PRINT
{
static int oldst = -1;
static int oldr = -1;
if((gsm_stato != oldst) || (gsm_retry != oldr))
{
printf("stato: %s (retry: %d)\n", statod[gsm_stato], gsm_retry);
oldst = gsm_stato;
oldr = gsm_retry;
}
}
#endif
    switch(gsm_stato)
    {
      case ST_START:
        if(gsm_fd < 0)
        {
          if(config.consumer[gsm_consumer].data.serial.baud2)
            gsm_fd = ser_open(gsm_serial, config.consumer[gsm_consumer].data.serial.baud2 | CRTSCTS, 1);
          else
            gsm_fd = ser_open(gsm_serial, config.consumer[gsm_consumer].data.serial.baud1 | CRTSCTS, 1);
          if(gsm_fd < 0)
          {
            sleep(1);
            continue;
          }
        }
        
        tcflush(gsm_fd, TCIOFLUSH);
        
#ifdef __arm__
        ret = ioctl(gpio_fd, IO_GSM_POWERMON, 0);
        if(ret)
        {
          if(!gsm_timeout->active)
          {
            /* Accensione GSM */
            support_log("GSM power on");
            support_log2("GSM power on");
            ioctl(gpio_fd, IO_GSM_SHUTDOWN, 0);
            ioctl(gpio_fd, IO_GSM_ONOFF, 0);
            
            timeout_on(gsm_timeout, NULL, NULL, 0, 200);
            gsm_stato = ST_ACCENSIONE_1;
          }
        }
#else
        ioctl(gsm_fd, TIOCMGET, &ret);
        if(!(ret & TIOCM_CTS))
        {
          if(!gsm_timeout->active)
          {
            /* Accensione GSM */
            support_log("GSM power on");
            support_log2("GSM power on");
            ret = TIOCM_RTS;
            ioctl(gsm_fd, TIOCMBIC, &ret);
            usleep(100000);
            ret = TIOCM_RTS;
            ioctl(gsm_fd, TIOCMBIS, &ret);
            
            timeout_on(gsm_timeout, NULL, NULL, 0, 200);
            gsm_stato = ST_ACCENSIONE_1;
          }
        }
#endif
        else
        {
          support_log("GSM acceso (1)");
          gsm_stato = ST_INIT_0;
        }
        gsm_creg_counter = 0;
        gsm_gprs_init = GPRS_INIT_NONE;
        break;
      case ST_ACCENSIONE_1:
#ifdef __arm__
        ret = ioctl(gpio_fd, IO_GSM_POWERMON, 0);
        ret = !ret;
#else
        ioctl(gsm_fd, TIOCMGET, &ret);
        ret &= TIOCM_CTS;
#endif
        if(!gsm_timeout->active)
        {
          //gsm_stato = ST_INIT_0;
          /* Il modulo non risulta acceso nei tempi previsti,
             procedo ad un nuovo spegnimento e riaccensione. */
          gsm_stato = ST_START;
        }
        else if(ret)
        {
          support_log("GSM acceso (2)");
          timeout_on(gsm_timeout, NULL, NULL, 0, 10);
          gsm_stato = ST_ACCENSIONE_2;
        }
        break;
      case ST_ACCENSIONE_2:
        if(!gsm_timeout->active)
        {
          //gsm_stato = ST_INIT_1;
          gsm_stato = ST_INIT_0;
        }
        break;
      case ST_INIT_0:
        support_log("GSM audio reset");
        audio_reset(gsm_fd);
        gsm_stato = ST_INIT_1;
        break;
      case ST_INIT_1:
        GSM_Status[0] = 1;
        gsm_flags &= ~GSM_FLAG_REINIT;
        gsm_send("AT\r", 2);
        break;
      case ST_INIT_1A:
        /* Alla prima accensione viene inviato l'impulso di spegnimento
           anche se il gsm è acceso. Quindi dopo il primo AT aspetto 4s
           e verifico se il modulo sia ancora acceso per evitare di rimanere
           bloccato su timeout troppo lunghi (es: +COPS). */
#ifdef __CRIS__
        timeout_on(gsm_timeout, NULL, NULL, 0, 40);
        gsm_stato = ST_INIT_1B;
#else
        gsm_stato = ST_INIT_DETECT_1;
#endif
        break;
      case ST_INIT_1B:
#ifdef __CRIS__
        /* Allo scadere dei 4 secondi... */
        if(!gsm_timeout->active)
        {
          ioctl(gsm_fd, TIOCMGET, &ret);
          if(ret & TIOCM_CTS)
            gsm_stato = ST_INIT_DETECT_1;
          else
            gsm_stato = ST_START;
        }
#endif
        break;
      case ST_INIT_DETECT_1:
        gsm_send("ATI\r", 2);
        break;
      case ST_INIT_DETECT_1_ERR:
        gsm_stato = ST_INIT_DETECT_3_ERR;
        break;
      case ST_INIT_DETECT_2:
        /* Attende l'OK. */
        break;
      case ST_INIT_DETECT_3:
        gsm_send("ATI4\r", 2);
        break;
      case ST_INIT_DETECT_3_ERR:
        support_log("GSM non rilevato.");
        support_log2("GSM non rilevato.");
        gsm_stato = ST_INIT_DETECT_STOP;
        break;
      case ST_INIT_DETECT_STOP:
        /* Si ferma qui */
        break;
      case ST_INIT_DETECT_4:
        /* Attende l'OK. */
        break;
      case ST_INIT_VERSION:
        gsm_send("AT+CGMR\r", 2);
        break;
      case ST_INIT_VERSION_READ:
        break;
      case ST_INIT_VERSION_OK:
        break;
      case ST_INIT_2:
        pthread_mutex_lock(&gsm_mutex);
        gsm_flags &= ~GSM_FLAG_GSM_SPENTO;
        pthread_mutex_unlock(&gsm_mutex);
#ifdef __arm__
        gsm_send("AT#SLED=2;#DVI=1;#JDR=2\r", 5);
#else
        gsm_send("AT+IPR=19200\r", 5);
#endif
        break;
      case ST_INIT_3:
        gsm_send("AT+CMEE=1;+CRC=1\r", 50);
        break;
      case ST_INIT_4:
        /* Se il gsm si accende senza SIM, la procedura si blocca in questo stato.
           Occorre quindi verificare se si è ancora in manutenzione per permettere
           lo spegnimento affinché si possa inserire la SIM.
           Ma se il GSM si riaccende perché è scaduta la manutenzione? Come faccio
           a spegnere di nuovo? */
        if((!gsm_waitresp) && (gsm_flags & GSM_FLAG_SPEGNIMENTO))
        {
          /* Se la richiesta credito arriva da un utente, non
             devo ripristinare il limite sms. */
          pthread_mutex_lock(&gsm_mutex);
          gsm_flags &= ~GSM_FLAG_SPEGNIMENTO;
          gsm_stato = ST_SPEGNIMENTO;
          pthread_mutex_unlock(&gsm_mutex);
        }
        else
          gsm_send("AT+CPIN?\r", 2);
        break;
      case ST_INIT_5:
        gsm_send("AT+CMGF=1;+CNMI=2,1,0,0,0;+CSMP=17,167,0,241;+CLIP=1\r", 150);
        break;
      case ST_INIT_5A:
        gsm_send("AT+CSCS=\"8859-1\"\r", 150);
        break;
      case ST_INIT_5B:
        gsm_send("AT+CSDH=1\r", 10);
        break;
      case ST_INIT_5C:
        gsm_send("AT+ICF=3\r", 10);
        break;
      case ST_INIT_5D:
        gsm_send("AT#SKIPESC=1\r", 10);
        break;
      case ST_INIT_6:
        /* Visto che anche questa è una fase lunga, verifico se eventualmente
           è stato richiesto uno spegnimento prima di agganciarmi al ponte. */
        if((!gsm_waitresp) && (gsm_flags & GSM_FLAG_SPEGNIMENTO))
        {
          /* Se la richiesta credito arriva da un utente, non
             devo ripristinare il limite sms. */
          pthread_mutex_lock(&gsm_mutex);
          gsm_flags &= ~GSM_FLAG_SPEGNIMENTO;
          pthread_mutex_unlock(&gsm_mutex);
          gsm_stato = ST_SPEGNIMENTO;
        }
        else
          gsm_send("AT+CREG?\r", 50);
        break;
      case ST_INIT_7A:
        /* Viene poi eventualmente azzerato dall'ERROR dell'impostazione successiva. */
        gsm_dialmode = 1;
        
        if(!gsm_audio_type)
          gsm_stato++;
        else
          gsm_stato = ST_INIT_7E;
        break;
      case ST_INIT_7B:
#ifdef __CRIS__
        gsm_send("AT+IPR=0\r", 5);
#else
        gsm_send("AT\r", 5);
#endif
        break;
      case ST_INIT_7C:
        for(n=0; !gsm_audio_type && (n < 3); n++)
          gsm_audio_type = audio_init(gsm_fd);
        gsm_stato++;
        break;
      case ST_INIT_7D:
#ifdef __CRIS__
        gsm_send("AT+IPR=19200\r", 5);
#else
        gsm_send("AT\r", 5);
#endif
        break;
      case ST_INIT_7E:
        gsm_send("AT#DIALMODE=1\r", 50);
        break;
      case ST_INIT_8:
        gsm_send("AT+COPS=3,2\r", 1800);
        break;
      case ST_INIT_9:
        gsm_send("AT+COPS?\r", 50);
        break;
      case ST_INIT_10:
        //if(gsm_operator == GSM_OPERATOR_WIND)
          gsm_send("AT+CUSD=1\r", 50);
        //else
        //  gsm_stato++;
        break;
      case ST_INIT_11:
        gsm_stato = ST_INIT_12;
        break;
      case ST_IMPOSTA_SELINT:
        break;
      case ST_IMPOSTA_SELINT_2:
        gsm_send("AT#SELINT=2\r", 50);
        break;
      case ST_INIT_12:
        gsm_send("AT#SELINT?\r", 50);
        break;
      case ST_INIT_13:
        gsm_send("AT#MONI\r", 100);
        break;
      case ST_INIT_14:
        gsm_send("AT+CSCA?\r", 200);
        break;
      case ST_INIT_15:
        gsm_send("AT+CSQ\r", 50);
        break;
      case ST_INIT_LAST:
        /* Deve essere l'ultimo stato di inizializzazione, perché
           deve poter cancellare un sms se la SIM è piena. */
        gsm_send("AT+CPMS?\r", 50);
        break;
/***********************************************/
      case ST_MAIN:
        /* Questo è lo stato normale finita l'inizializzazione del GSM,
           in attesa delle operazioni di invio sms o gestione degli
           sms ricevuti. */
        
        if(gsm_flags & GSM_FLAG_REINIT)
          gsm_stato = ST_INIT_1;
        else if(gsm_flags & GSM_FLAG_VOICE_CALL)
          gsm_stato = ST_AUDIO_REC;
        else if(gsm_flags & GSM_FLAG_DATA_CALL)
          gsm_stato = ST_SAETNET;
        else if(gsm_flags & GSM_FLAG_CALL_CLOSE)
          gsm_stato = ST_CHIUDI_CHIAMATA;
        else if(gsm_delete_sms)
        {
          struct data *p;
          pthread_mutex_lock(&gsm_mutex);
          p = gsm_delete_sms;
          gsm_sms_idx = gsm_delete_sms->msg;
          gsm_delete_sms = gsm_delete_sms->next;
          free(p);
          pthread_mutex_unlock(&gsm_mutex);
          gsm_stato = ST_SMS_CANCELLA;
        }
        else if(gsm_sms_pending_out != gsm_sms_pending_in)
        {
          gsm_sms_idx = gsm_sms_pending[gsm_sms_pending_out++];
          gsm_sms_pending_out &= 7;
          gsm_stato = ST_SMS_LEGGI;
        }
        else if(gsm_flags & GSM_FLAG_SMS_VERIFY)
        {
          pthread_mutex_lock(&gsm_mutex);
          if(gsm_flags & GSM_FLAG_SMS_PENDING)
          {
            gsm_stato = ST_SMS_LISTA_UNREAD;
            gsm_flags &= ~GSM_FLAG_SMS_PENDING;
          }
          gsm_flags &= ~GSM_FLAG_SMS_VERIFY;
          pthread_mutex_unlock(&gsm_mutex);
        }
        else if(gsm_flags & GSM_FLAG_SMS_CANCELLA)
        {
          gsm_sms_idx = 1;
          gsm_stato = ST_SMS_CANCELLA;
          pthread_mutex_lock(&gsm_mutex);
          gsm_flags &= ~GSM_FLAG_SMS_CANCELLA;
          pthread_mutex_unlock(&gsm_mutex);
        }
        else if(gsm_sms_cur)
        {
          /* Se il precedente invio è fallito, ci riprovo. */
          if(gsm_sms_cur->retry)
          {
            gsm_sms_cur->retry--;
            gsm_stato = ST_SMS_INVIA;
          }
          else
          {
            /* L'invio è fallito definitivamente */
            if(gsm_sms_cur->cbk)
              gsm_sms_cur->cbk(-1, gsm_sms_cur->cbk_data);
            
            free(gsm_sms_cur->text);
            free(gsm_sms_cur);
            gsm_sms_cur = NULL;
          }
        }
        else if(!gsm_mms_delay->active && gsm_gprs_cur)
        {
          /* Se il precedente invio è fallito, ci riprovo. */
          if(gsm_gprs_cur->retry)
          {
            gsm_gprs_cur->retry--;
            gsm_stato = ST_GPRS_INVIA;
          }
          else
          {
            gsm_event[2] = 105;
            sprintf(gsm_event+4, str_d_1);
            gsm_event[3] = strlen(gsm_event+4);
            codec_queue_event(gsm_event);
            
            if(gsm_gprs_cur->func) gsm_gprs_cur->func(-1, NULL);
            free(gsm_gprs_cur->text);
            free(gsm_gprs_cur->ipaddr);
            free(gsm_gprs_cur);
            gsm_gprs_cur = NULL;
          }
        }
        else if(gsm_sms_head)
        {
          pthread_mutex_lock(&gsm_mutex);
          /* Verifico anche dentro il lock, potrebbe non essere
             più valida la condizione (cancellazione lista) */
          if(gsm_sms_head)
          {
            gsm_sms_cur = gsm_sms_head;
            gsm_sms_head = gsm_sms_head->next;
            if(!gsm_sms_head) gsm_sms_tail = NULL;
            gsm_stato = ST_SMS_INVIA;
          }
          pthread_mutex_unlock(&gsm_mutex);
        }
        else if(!gsm_mms_delay->active && gsm_gprs_head && !gsm_gprs_cur)
        {
          pthread_mutex_lock(&gsm_mutex);
          /* Verifico anche dentro il lock, potrebbe non essere
             più valida la condizione (cancellazione lista) */
          if(gsm_gprs_head)
          {
            gsm_gprs_cur = gsm_gprs_head;
            gsm_gprs_head = gsm_gprs_head->next;
            if(!gsm_gprs_head) gsm_gprs_tail = NULL;
            gsm_stato = ST_GPRS_INVIA;
          }
          pthread_mutex_unlock(&gsm_mutex);
        }
        else if(gsm_call_head)
        {
          pthread_mutex_lock(&gsm_mutex);
          /* Verifico anche dentro il lock, potrebbe non essere
             più valida la condizione (cancellazione lista) */
          if(gsm_call_head)
          {
            gsm_call_cur = gsm_call_head;
            gsm_call_head = gsm_call_head->next;
            if(!gsm_call_head) gsm_call_tail = NULL;
            gsm_call_cur->next = NULL;
            if(gsm_call_cur->msg == -1)
            {
              support_log("GSM chiamata dati");
              gsm_stato = ST_SAETNET_CHIAMA;	// dati
            }
            else
            {
              support_log("GSM chiamata voce");
              gsm_stato = ST_AUDIO_INVIA;	// voce
            }
          }
          pthread_mutex_unlock(&gsm_mutex);
        }
        else if(gsm_flags & GSM_FLAG_SMS_PENDING)
        {
          gsm_stato = ST_SMS_LISTA_UNREAD;
          pthread_mutex_lock(&gsm_mutex);
          gsm_flags &= ~GSM_FLAG_SMS_PENDING;
          pthread_mutex_unlock(&gsm_mutex);
        }
        else if(gsm_flags & GSM_FLAG_LISTA_SMS)
        {
          gsm_stato = ST_SMS_LISTA;
          pthread_mutex_lock(&gsm_mutex);
          gsm_flags &= ~GSM_FLAG_LISTA_SMS;
          pthread_mutex_unlock(&gsm_mutex);
        }
        else if(gsm_flags & GSM_FLAG_PERIODIC)
        {
          pthread_mutex_lock(&gsm_mutex);
          gsm_flags &= ~GSM_FLAG_PERIODIC;
          pthread_mutex_unlock(&gsm_mutex);
          gsm_stato = ST_VERIFICA_PORTANTE;
        }
        else if(gsm_flags & GSM_FLAG_CREDITO)
        {
          support_log("Richiesta credito");
          pthread_mutex_lock(&gsm_mutex);
          gsm_flags &= ~GSM_FLAG_CREDITO;
          pthread_mutex_unlock(&gsm_mutex);
          gsm_stato = ST_RICHIESTA_CREDITO;
        }
        else if(gsm_flags & (GSM_FLAG_SPEGNIMENTO|GSM_FLAG_RESET))
        {
          pthread_mutex_lock(&gsm_mutex);
          gsm_flags &= ~GSM_FLAG_SPEGNIMENTO;
          pthread_mutex_unlock(&gsm_mutex);
          gsm_stato = ST_SPEGNIMENTO;
        }
        else if(gsm_flags & GSM_FLAG_ACCENSIONE)
        {
          pthread_mutex_lock(&gsm_mutex);
          gsm_flags &= ~GSM_FLAG_ACCENSIONE;
          pthread_mutex_unlock(&gsm_mutex);
        }
        else if((gsm_cid_num || (gsm_cid_num = CID_get_message(gsm_cid_ev))) && !CID_delay->active)
        {
          if(!gsm_cid_num_base)
          {
            /* Nuovo evento da trasmettere. */
            gsm_cid_num_base = gsm_cid_num;
            if(!(config.PhoneBook[gsm_cid_num-1].Abil & PB_TX))
            {
              /* Se il numero non è abilitato, viene scartato l'evento. */
              gsm_cid_num_base = gsm_cid_num = 0;
            }
            else if(!config.PhoneBook[gsm_cid_num-1].Phone[0])
            {
              CID_set_error(1);
              gsm_cid_num_base = gsm_cid_num = 0;
            }
            else
              gsm_stato = ST_CID_CHIAMA;
          }
          else
          {
            /* Evento in coda, chiamo il numero corrente. */
            gsm_stato = ST_CID_CHIAMA;
          }
        }
        else if(GSM_Status[3] < 0)
        {
          /* La precedente procedura di shutdown è stata interrotta da qualche
             evento (es: ricezione contemporanea di un sms) e quindi occorre
             ripristinare gli stati di accensione. */
          gsm_stato = ST_START;
        }
        break;
      case ST_CHIUDI_CHIAMATA:
        gsm_send("ATH\r", 50);
        break;
      case ST_CHIAMATA_CHIUSA:
        gsm_incoming_data = -1;
        pthread_mutex_lock(&gsm_mutex);
        gsm_flags &= ~(GSM_FLAG_VOICE_CALL|GSM_FLAG_CALL_CLOSE);
        if(gsm_call_cur)
        {
          free(gsm_call_cur);
          gsm_call_cur = NULL;
        }
        pthread_mutex_unlock(&gsm_mutex);
        GSM_Status[1] = 0;
        timeout_on(gsm_mms_delay, NULL, NULL, 0, 50);
        if(gsm_flags & GSM_FLAG_SMS_VERIFY)
          gsm_stato = ST_INIT_LAST;
        else
          gsm_stato = ST_MAIN;
        break;
      case ST_SMS_LEGGI:
        sprintf(gsm_buf, "AT+CMGR=%d\r", gsm_sms_idx);
        gsm_send(gsm_buf, 50);
        break;
      case ST_SMS_LEGGI_TESTO:
        break;
      case ST_SMS_ARRIVATO:
        /* Gestisce l'sms arrivato, dopodiché cancella il successivo per fare posto. */
        /* Posso evitare di usare un secondo buffer per ricopiare il testo dell'sms
           dal momento che tra l'arrivo del testo e l'arrivo dell'OK faccio comunque
           girare la funzione di logica. */
        if(strlen(gsm_buf) == (gsm_test_len*4))
        {
          char hex[8];
          int i;
          
          /* Si tratta di un messaggio UCS2 */
          hex[4] = 0;
          for(i=0; i<strlen(gsm_buf); i+=4)
          {
            memcpy(hex, gsm_buf+i, 4);
            sscanf(hex, "%x", &n);
            if(n > 255) n = ' ';
            gsm_buf[i/4] = n;
          }
          gsm_buf[i/4] = 0;
        }
        
        if(!strncasecmp(gsm_buf, "TC ", 3))
        {
          n = -1;
          sscanf(gsm_buf+3, "%d", &n);
          if(n >= 0)
          {
            int i;
            short *codes = (short*)RONDA;
            for(i=0; i<(sizeof(RONDA)/sizeof(short)); i++)
              if(codes[i] == n) break;
            if(i == (sizeof(RONDA)/sizeof(short))) i = -1;
            if(i >= 0)
            {
              database_lock();
              ME2[i+768] = gsm_mittente;
              database_set_alarm2(&ME[i+768]);
              database_unlock();
            }
            i = 0;
          }
        }
        
        if(gsm_sms_event)
        {
          n = gsm_mittente - 1;
          gsm_event[2] = 106;
          if(gsm_mittente < 0)
            sprintf(gsm_event+4, "%s", gsm_buf);
          else if(config.PhoneBook[n].Name)
            sprintf(gsm_event+4, "%s: %s", config.PhoneBook[n].Name, gsm_buf);
          else
            sprintf(gsm_event+4, "%s: %s", config.PhoneBook[n].Phone, gsm_buf);
          /* Elimino i ritorno a capo finali */
          for(n=strlen(gsm_event+4)+3; gsm_event[n] < ' '; n--) gsm_event[n] = 0;
          gsm_event[3] = strlen(gsm_event+4);
          if(gsm_event[3] > 60) gsm_event[3] = 60;
          codec_queue_event(gsm_event);
        }
        
        /* Attenzione, devo cancellare solo se non ho più altri sms pendenti,
           altrimenti ne arrivassero due di fila e consecutivi rischierei di
           cancellare il secondo prima di averlo letto. */
        if(gsm_sms_pending_out != gsm_sms_pending_in)
          gsm_stato = ST_SMS_ATTENDI_OK;
        else
          gsm_stato = ST_SMS_ATTENDI_OK_PER_CANCELLARE;
        break;
      case ST_SMS_ATTENDI_OK:
        /* Attende l'arrivo dell'OK e poi torna al main. */
        break;
      case ST_SMS_ATTENDI_FINE:
        gsm_stato = ST_MAIN;
        break;
      case ST_SMS_INOLTRA:
#warning Inoltro messaggio
        /* Qui ho il testo del messaggio da inoltrare, devo copiarmelo da
           qualche parte e poi inviarlo dopo aver finito la gestione della
           lettura. */
        break;
      case ST_SMS_ATTENDI_OK_PER_CANCELLARE:
        /* Attende l'arrivo dell'OK e poi passa a cancellare. */
        if(!gsm_waitresp)
        {
          /* Non è arrivato l'OK nel timeout, probabile sms vuoto e quindi
             l'OK è stato preso come testo. Proseguo con la cancellazione. */
          gsm_stato = ST_SMS_CANCELLA;
        }
        break;
      case ST_SMS_CANCELLA:
        sprintf(gsm_buf, "AT+CMGD=%d\r", gsm_sms_idx);
        gsm_send(gsm_buf, 50);
        break;
      case ST_SMS_CANCELLATO:
        //gsm_stato = ST_MAIN;
        /* Riallineo il contatore degli sms noti. */
        gsm_stato = ST_INIT_LAST;
        break;
      case ST_SMS_INVIA:
        if(!gsm_waitresp)
        {
          sprintf(gsm_buf, "AT+CMGS=\"%s\"\r", gsm_sms_cur->num);
          gsm_send(gsm_buf, 50);
          gsm_stato = ST_SMS_ATTESA_PROMPT;
        }
        break;
      case ST_SMS_ATTESA_PROMPT:
        /* Nel caso non arrivasse il prompt per qualche motivo. */
        if(!gsm_waitresp) gsm_stato = ST_SMS_INVIA_TESTO;
        break;
      case ST_SMS_INVIA_TESTO:
        gsm_send(gsm_sms_cur->text, 1800);
        gsm_stato = ST_SMS_ATTESA_INVIO;
        break;
      case ST_SMS_ATTESA_INVIO:
        /* E' scaduto il timeout di invio, procedo oltre. */
        if(!gsm_waitresp) gsm_stato = ST_MAIN;
        break;
      case ST_SMS_INVIATO:
        gsm_stato = ST_MAIN;
        break;
      case ST_GPRS_INVIA:
        if(gsm_gprs_cur->type == GPRS_INIT_NONE)
        {
          /* Si tratta di una richiesta cancellata. */
          free(gsm_gprs_cur->text);
          free(gsm_gprs_cur->ipaddr);
          free(gsm_gprs_cur);
          gsm_gprs_cur = NULL;
          gsm_stato = ST_MAIN;
        }
        else if(gsm_gprs_init != gsm_gprs_cur->type)
          gsm_stato = ST_GPRS_INIT;
        else
          gsm_stato = ST_GPRS_INIT_4;
        break;
      case ST_GPRS_INIT:
        if(gsm_gprs_cur->type == GPRS_INIT_MMS)
          sprintf(gsm_buf, "AT+CGDCONT=1,\"IP\",\"%s\",\"0.0.0.0\",0,0\r", gsm_operator_data[gsm_operator].apn);
        else
          sprintf(gsm_buf, "AT+CGDCONT=1,\"IP\",\"%s\",\"0.0.0.0\",0,0\r", gsm_operator_dataex[gsm_operator].apn);
        gsm_send(gsm_buf, 200);
        break;
      case ST_GPRS_INIT_1A:
        gsm_stato = ST_GPRS_INIT_1D;
        break;
      case ST_GPRS_INIT_1B:
        break;
      case ST_GPRS_INIT_1C:
        gsm_send("AT#SCFG=1,1,16,60,600,50\r", 50);
        break;
      case ST_GPRS_INIT_1D:
        if(gsm_gprs_cur->type != GPRS_INIT_UDP)
          gsm_stato = ST_GPRS_INIT_2;
        else
          gsm_send("AT#SCFG?\r", 50);
        break;
      case ST_GPRS_INIT_2:
        gsm_send("AT#DSTO=5\r", 50);
        break;
      case ST_GPRS_INIT_3:
        gsm_send("AT#SKTCT=200\r", 50);
        break;
      case ST_GPRS_INIT_4:
        if(gsm_gprs_cur->type != GPRS_INIT_UDP)
          sprintf(gsm_buf, "AT#SKTSET=0,%d,\"%s\"\r", gsm_gprs_cur->port, gsm_gprs_cur->ipaddr);
        else
          sprintf(gsm_buf, "AT#SKTSET=1,%d,\"%s\",0,%d\r", gsm_gprs_cur->port, gsm_gprs_cur->ipaddr, gsm_gprs_cur->port);
        gsm_send(gsm_buf, 50);
        break;
      case ST_GPRS_INIT_5:
        /* Dopo aver impostato i parametri GPRS occorre attendere 5 secondi
           prima di attivare la connessione, altrimenti restituisce CME ERROR 555 */
        timeout_on(gsm_timeout, NULL, NULL, 0, 50);
        gsm_stato = ST_GPRS_INIT_6;
        break;
      case ST_GPRS_INIT_6:
        if(!gsm_timeout->active)
          gsm_stato = ST_GPRS_CONNECT;
        break;
      case ST_GPRS_CONNECT:
        gsm_gprs_init = gsm_gprs_cur->type;
        gsm_send("AT#SKTOP\r", 1200);
        break;
      case ST_GPRS_CONNECT_ERROR:
        timeout_on(gsm_mms_delay, NULL, NULL, 0, 20);
        gsm_stato = ST_MAIN;
        break;
      case ST_GPRS_TRASMETTI:
        if(gsm_gprs_cur->func &&
          gsm_gprs_cur->func(gsm_fd, gsm_gprs_cur->data))
        {
          free(gsm_gprs_cur->text);
          free(gsm_gprs_cur->ipaddr);
          free(gsm_gprs_cur);
          gsm_gprs_cur = NULL;
        }
        timeout_on(gsm_mms_delay, NULL, NULL, 0, 20);
        gsm_stato++;
        break;
      case ST_GPRS_CONNECT_CLOSE:
        if(!gsm_mms_delay->active)
        {
          gsm_send("+++", 100);
          gsm_stato++;
        }
        break;
      case ST_GPRS_CONNECT_CLOSE_2:
        /* Se arriva l'OK avanzo automaticamente, altrimenti
           avanzo allo scadere del timeout.
           Non sempre la sequenza +++ produce l'OK, ad esempio
           se il socket si è già chiuso. */
        /* In realtà più che l'OK deve arrivare il NO CARRIER. */
        if(!gsm_waitresp) gsm_stato++;
        break;
      case ST_GPRS_CONNECT_CLOSE_3:
        /* Questo mi serve per sicurezza, casomai non si fosse chiusa
           la connessione, qui innesco uno spegnimento su ritrasmissione. */
        gsm_send("AT\r", 5);
        break;
      case ST_GPRS_CONNECT_CLOSE_4:
        /* Distanzia di 10 secondi gli invii MMS. */
        timeout_on(gsm_mms_delay, NULL, NULL, 0, 100);
        gsm_stato = ST_MAIN;
        break;
      case ST_AUDIO_INVIA:
#ifdef __CRIS__
#ifdef SAET_CONSOLE
        // azzera l'icona di segnale sul terminal data.
        console_sms_message("+CSQ:99", gsm_selint);
#endif
        gsm_send("AT+IPR=0\r", 5);
#else
        gsm_stato++;
#endif
        /* Anziché attendere il timeout naturale, se non c'è risposta
           entro 30 secondi interrompo e passo alla chiamata successiva. */
        timeout_on(gsm_audio_timeout, NULL, NULL, 0, 300);
        break;
      case ST_AUDIO_ATTESA_RISPOSTA:
#if 1
        sprintf(gsm_buf, "ATD%s;\r", gsm_call_cur->num);
        gsm_send(gsm_buf, 700);
        
        if(!gsm_audio_timeout->active)
        {
          /* Dopo 30s senza risposta sblocco la chiamata. */
          timeout_off(gsm_timeout);
          gsm_waitresp = 0;
          gsm_buf_idx = 0;
          support_log("GSM nessuna risposta");
          
          if(gsm_call_cur->retry)
          {
            /* Riaccodo per un altro tentativo */
            support_log("GSM altro tentativo");
            gsm_call_cur->retry--;
            pthread_mutex_lock(&gsm_mutex);
            if(!gsm_call_head)
              gsm_call_head = gsm_call_cur;
            else
              gsm_call_tail->next = gsm_call_cur;
            gsm_call_tail = gsm_call_cur;
            pthread_mutex_unlock(&gsm_mutex);
            gsm_call_cur = NULL;
          }
          gsm_stato = ST_AUDIO_TERMINA;
        }
#else
#warning DEBUGGGGGGGG!!!!
        gsm_send("AT\r", 10);
#endif
        GSM_Status[1] = 3;
        break;
      case ST_AUDIO_RIPRODUCI:
        /* Ho ancora in coda lo "\n" che conclude l'OK, lo devo consumare
           per evitare che venga preso in considerazione dalla riproduzione
           che lo considererebbe una ricezione anomala. */
        read(gsm_fd, gsm_buf, 1);
        
        audio_play(gsm_fd, gsm_call_cur->msg, gsm_dialmode);
#ifdef __CRIS__
#if 0
        /* Al termine delle chiamate audio verifico che non siano
           arrivati nuovi sms da gestire. */
        pthread_mutex_lock(&gsm_mutex);
        gsm_flags |= GSM_FLAG_SMS_PENDING;
        pthread_mutex_unlock(&gsm_mutex);
#else
        /* Al termine di ogni chiamata audio verifico con un +CMGS
           se siano arrivati nuovi sms. Il +CMGS è molto più veloce
           che non andare a leggere gli unread con +CMGL. */
        pthread_mutex_lock(&gsm_mutex);
        gsm_flags |= GSM_FLAG_SMS_VERIFY;
        pthread_mutex_unlock(&gsm_mutex);
#endif
#else
        /* Tempo massimo di riproduzione 60s */
        timeout_on(gsm_audio_timeout, NULL, NULL, 0, 600);
#endif
        gsm_stato++;
        break;
      case ST_AUDIO_TERMINA:
#ifdef __CRIS__
        gsm_send("AT\r", 5);
#else
        /* Per la gestione Athena devo attendere la chiusura della chiamata
           oppure il timeout di riproduzione. L'audio è asincrono. */
        if(!gsm_audio_timeout->active)
          gsm_stato++;
#endif
        break;
      case ST_AUDIO_TERMINA_2:
#ifdef __CRIS__
        gsm_send("AT+IPR=19200\r", 5);
#else
        audio_play_stop();
        gsm_stato++;
#endif
        break;
      case ST_AUDIO_FINE:
        support_log("GSM fine chiamata");
        gsm_stato = ST_CHIUDI_CHIAMATA;
        break;
      
      case ST_AUDIO_REC:
        gsm_send("ATA\r", 50);
        break;
      case ST_AUDIO_REC_INIZIO:
        audio_rec(gsm_fd);
#ifdef __CRIS__
#if 0
        /* Al termine delle chiamate audio verifico che non siano
           arrivati nuovi sms da gestire. */
        pthread_mutex_lock(&gsm_mutex);
        gsm_flags |= GSM_FLAG_SMS_PENDING;
        pthread_mutex_unlock(&gsm_mutex);
#else
        /* Al termine di ogni chiamata audio verifico con un +CMGS
           se siano arrivati nuovi sms. Il +CMGS è molto più veloce
           che non andare a leggere gli unread con +CMGL. */
        pthread_mutex_lock(&gsm_mutex);
        gsm_flags |= GSM_FLAG_SMS_VERIFY;
        pthread_mutex_unlock(&gsm_mutex);
#endif
#endif
        gsm_stato++;
        break;
      case ST_AUDIO_REC_TERMINA:
#ifdef __CRIS__
        gsm_send("AT\r", 5);
#endif
        break;
      case ST_AUDIO_REC_FINE:
        support_log("GSM fine chiamata");
        gsm_stato = ST_CHIUDI_CHIAMATA;
        break;
      
      case ST_SMS_LISTA_UNREAD:
        gsm_send("AT+CMGL=\"REC UNREAD\"\r", 300);
        break;
      case ST_SMS_LISTA_UNREAD_OK:
        gsm_stato = ST_MAIN;
        break;
      case ST_SMS_LISTA:
        gsm_send("AT+CMGL=\"ALL\"\r", 300);
        break;
      case ST_SMS_LISTA_2:
        if(!gsm_read_sms)
          gsm_stato = ST_MAIN;
        else
        {
          sprintf(gsm_buf, "AT+CMGR=%d\r", gsm_read_sms->msg);
          gsm_send(gsm_buf, 50);
        }
        break;
      case ST_SMS_LISTA_FINE:
        {
          struct data *p;
          p = gsm_read_sms;
          gsm_read_sms = gsm_read_sms->next;
          free(p);
        }
        gsm_stato = ST_SMS_LISTA_2;
        break;
      
      case ST_RICHIESTA_CREDITO:
        if(gsm_operator_data[gsm_operator].credit[2] == '+')
        {
          /* Prepara l'sms. Se sono in questo stato non ho altri sms pendenti */
          /* Mi adeguo inoltre al formato originale della struttura. */
          gsm_sms_cur = malloc(sizeof(struct data));
          memcpy(gsm_sms_cur->num, gsm_operator_data[gsm_operator].credit+9, sizeof(gsm_sms_cur->num));
          for(n=0; gsm_sms_cur->num[n]!='"'; n++);
          gsm_sms_cur->num[n] = 0;
          gsm_sms_cur->text = strdup(gsm_operator_data[gsm_operator].credit +
                   strlen(gsm_operator_data[gsm_operator].credit)+1);
          gsm_sms_cur->cbk = NULL;
          gsm_sms_cur->cbk_data = NULL;
          gsm_sms_cur->retry = 1;
          gsm_stato = ST_SMS_INVIA;
        }
        else
        {
          gsm_send(gsm_operator_data[gsm_operator].credit, 600);
        }
        /* La risposta deve arrivare entro due minuti. */
        timeout_on(gsm_credito_timeout, NULL, NULL, 0, 1200);
        break;
      case ST_RICHIESTA_CREDITO_INVIATA:
        gsm_stato = ST_MAIN;
        break;
      case ST_CREDITO_RESIDUO:
        break;
      case ST_VERIFICA_CREDITO:
#if 0
        switch(gsm_operator)
        {
          case GSM_OPERATOR_TIM:
            {
            char *p;
            int eur,cent;
            eur = cent = 0;
#if 0
            if(sscanf(gsm_buf, "TIM: traffico residuo di %d,%d", &eur, &cent) ||
               sscanf(gsm_buf, "TIM: traffico residuo %d,%d", &eur, &cent))
            {
              gsm_credito_residuo = eur+(cent/100.0);
              gsm_credito_valido = 1;
            }
#else
            p = gsm_buf;
            /* Cerco la virgola del credito */
            while(*p && (*p != ',')) p++;
            if(*p)
            {
              /* Torno all'inizio del valore del credito */
              for(p--; (p>gsm_buf)&&(*p>='0')&&(*p<='9'); p--);
              eur = cent = 0;
              if(sscanf(p, "%d,%d", &eur, &cent) == 2)
              {
                gsm_credito_residuo = eur+(cent/100.0);
                gsm_credito_valido = 1;
              }
            }
#endif
            }
            gsm_stato = ST_SMS_ATTENDI_OK_PER_CANCELLARE;
            break;
          case GSM_OPERATOR_VODAFONE:
            {
            char *p;
            int eur,cent;
            eur = cent = 0;
/*
Il tuo credito disponibile al 23-01-2012 alle 16:08 e' 10.00 euro.Ultimo
addebito 0.00 euro.Per tenere sotto controllo il traffico registrati su
vodafone.it
*/
            p = gsm_buf;
            /* Cerco la virgola del credito */
            while(*p && (*p != '.')) p++;
            if(*p)
            {
              /* Torno all'inizio del valore del credito */
              for(p--; (p>gsm_buf)&&(*p>='0')&&(*p<='9'); p--);
              eur = cent = 0;
              if(sscanf(p, "%d.%d", &eur, &cent) == 2)
              {
                gsm_credito_residuo = eur+(cent/100.0);
                gsm_credito_valido = 1;
              }
            }
            }
            gsm_stato = ST_SMS_ATTENDI_OK_PER_CANCELLARE;
            break;
          case GSM_OPERATOR_WIND:
            /* Il credito arriva attraverso SMS flash */
//+CUSD: 0,"Il credito e': 0.0000 Euro; Credito Dati:0.0000 Euro.",0
//+CUSD: 0,"Il credito e': 25.53 Euro.",15
//+CUSD: 0,"Il credito e': 30.67 Euro. Se hai poco credito puoi andare su Wind.it e ricaricare on line comodamente con carta di credito.",15
//+CUSD: 0,"Wind Servizio di richiamata. Il numero prenotato e' tornato libero. Informazione gratuita da Wind.",0
//+CUSD: 2,"Il Credito è 10.00 Euro. Scopra i nuovi tagli di ricarica a partire da 10 euro da Bancomat e dal suo Home Banking! Info su Wind.it",15
//+CUSD: 4
#if 0
            if(sscanf(gsm_buf+10, "Il credito e': %f", &gsm_credito_residuo))
            {
              gsm_credito_valido = 1;
#ifdef SAET_CONSOLE
              console_sms_message(gsm_buf, gsm_selint);
#endif
            }
#else
            {
            char *p;
            int eur,cent;
            p = gsm_buf+10;
            /* Cerco la virgola del credito */
            while(*p && (*p != '.')) p++;
            if(*p)
            {
              /* Torno all'inizio del valore del credito */
              for(p--; (p>gsm_buf)&&(*p>='0')&&(*p<='9'); p--);
              eur = cent = 0;
              if(sscanf(p, "%d.%d", &eur, &cent) == 2)
              {
                gsm_credito_residuo = eur+(cent/100.0);
                gsm_credito_valido = 1;
#ifdef SAET_CONSOLE
                console_sms_message(gsm_buf, gsm_selint);
#endif
              }
            }
            }
#endif
            gsm_stato = ST_MAIN;
          default:
            break;
        }
#endif
        
        /* Gestione generica, cerco una dicitura del tipo "00.00" o "00,00". */
        {
          unsigned char *p;
          int eur,cent;
          eur = cent = 0;
          
          if(gsm_operator == GSM_OPERATOR_WIND)
          {
            //p = gsm_buf+10;
            p = (unsigned char*)strcasestr(gsm_buf, "credito");
            if(!p)
            {
              gsm_stato = ST_VERIFICA_CREDITO_OK;
              break;
            }
          }
          else
            p = gsm_buf;
          
          /* Provo prima con il punto. */
          while(*p && (*p != '.')) p++;
          if(*p && isdigit(*(p-1)) && isdigit(*(p+1)))
          {
            /* Torno all'inizio del valore del credito */
            for(p--; (p>gsm_buf)&&isdigit(*p); p--);
            if(sscanf(p, "%d.%d", &eur, &cent) == 2)
            {
              gsm_credito_residuo = eur+(cent/100.0);
              gsm_credito_valido = 1;
#ifdef SAET_CONSOLE
              if(gsm_operator == GSM_OPERATOR_WIND)
                console_sms_message(gsm_buf, gsm_selint);
#endif
            }
          }
          
          if(!gsm_credito_valido)
          {
            if(gsm_operator == GSM_OPERATOR_WIND)
              p = gsm_buf+10;
            else
              p = gsm_buf;
          
            /* Non ho trovato il credito con il punto, provo con la virgola. */
            while(*p && (*p != ',')) p++;
            if(*p && isdigit(*(p-1)) && isdigit(*(p+1)))
            {
              /* Torno all'inizio del valore del credito */
              for(p--; (p>gsm_buf)&&isdigit(*p); p--);
              if(sscanf(p, "%d,%d", &eur, &cent) == 2)
              {
                gsm_credito_residuo = eur+(cent/100.0);
                gsm_credito_valido = 1;
#ifdef SAET_CONSOLE
                if(gsm_operator == GSM_OPERATOR_WIND)
                  console_sms_message(gsm_buf, gsm_selint);
#endif
              }
            }
          }
        }
        
        if(gsm_credito_valido)
        {
          char m[32];
          sprintf(m, "Credito residuo %.02f", gsm_credito_residuo);
          support_log(m);
        }
        gsm_stato = ST_VERIFICA_CREDITO_OK;
        break;
      case ST_VERIFICA_CREDITO_OK:
        /* Attende solo l'OK evitando di scrivere più volte nel log il credito residuo. */
        break;
      
      case ST_VERIFICA_PORTANTE:
        gsm_send("AT+CSQ\r", 50);
        break;
      case ST_VERIFICA_PORTANTE_OK:
        //gsm_stato = ST_MAIN;
        /* Anziché tornare allo stato ST_MAIN, verifico che la SIM non sia piena.
           Non dovrebbe mai accadere, ma controllando una volta al minuto scongiuro
           che un errore possa provocare il blocco della ricezione degli sms. */
        gsm_stato = ST_VERIFICA_CONFIGURAZIONE;
        break;
      case ST_VERIFICA_CONFIGURAZIONE:
        /* Verifica che il modulo GSM sia ancora configurato correttamente,
           che non si sia resettato per conto proprio perdendo quindi le
           impostazioni iniziali. Nel caso le ripeto. */
        gsm_send("AT+CMGF?\r", 50);
        break;
      case ST_VERIFICA_CONFIGURAZIONE_OK:
        gsm_stato = ST_VERIFICA_CONNESSIONE;
        break;
      case ST_VERIFICA_CONNESSIONE:
        gsm_send("AT+CREG?\r", 50);
        break;
      case ST_VERIFICA_CONNESSIONE_OK:
        //gsm_stato = ST_MAIN;
        /* Anziché tornare allo stato ST_MAIN, verifico che la SIM non sia piena.
           Non dovrebbe mai accadere, ma controllando una volta al minuto scongiuro
           che un errore possa provocare il blocco della ricezione degli sms. */
        gsm_stato = ST_INIT_LAST;
        break;
      
      case ST_SPEGNIMENTO:
        GSM_Status[1] = 0;
        GSM_Status[3] = -1;
        GSM_Status[4] = 99;
        gsm_moni1[0] = 0;
        gsm_moni2[0] = 0;
        gsm_csca[0] = 0;
        gsm_stat_line_val = 0;
        gsm_stat_line_cnt = 0;
        gsm_stat_line_min = 99;
        gsm_stat_line_max = 0;
        gsm_stat_line_time_min = 0;
        gsm_stat_line_time_max = 0;
        
        console_sms_message("+CSQ: 99", gsm_selint);
        
#ifdef __arm__
        ret = ioctl(gpio_fd, IO_GSM_POWERMON, 0);
        if(!ret)
        {
          support_log("GSM shutdown");
          support_log2("GSM shutdown");
          
          ioctl(gpio_fd, IO_GSM_ONOFF, 1);
          
          gsm_stato = ST_SPEGNIMENTO_2;
        }
#else
        ioctl(gsm_fd, TIOCMGET, &ret);
        if(ret & TIOCM_CTS)
        {
          support_log("GSM shutdown");
          support_log2("GSM shutdown");
          
          ret = TIOCM_RTS;
          ioctl(gsm_fd, TIOCMBIC, &ret);
          usleep(100000);
          ret = TIOCM_RTS;
          ioctl(gsm_fd, TIOCMBIS, &ret);
          
          gsm_stato = ST_SPEGNIMENTO_2;
        }
#endif
        else
        {
          /* E' stato richiesto lo spegnimento del modulo
             ma risulta già spento. Probabilmente è stato
             spento manualmente, e probabilmente per poter
             cambiare la SIM. Devo quindi dare tempo per
             l'operazione prima di riaccendere. */
          support_log("GSM power off manuale");
          support_log2("GSM power off manuale");
          ser_close(gsm_fd);
          gsm_fd = -1;
          gsm_stato = ST_SPEGNIMENTO_1;
        }
        timeout_on(gsm_timeout, NULL, NULL, 0, 300);
        break;
      case ST_SPEGNIMENTO_1:
        /* Do il tempo di cambiare la SIM */
        if(!gsm_timeout->active) gsm_stato = ST_SPENTO;
        break;
      case ST_SPEGNIMENTO_2:
#ifdef __arm__
        ret = ioctl(gpio_fd, IO_GSM_POWERMON, 0);
        ret = !ret;
#else
        ioctl(gsm_fd, TIOCMGET, &ret);
        ret &= TIOCM_CTS;
#endif
        if(!ret || !gsm_timeout->active)
        {
          support_log("GSM spento");
          timeout_off(gsm_timeout);
          ser_close(gsm_fd);
          gsm_fd = -1;
          gsm_stato = ST_SPENTO;
        }
        break;
      case ST_SPENTO:
        GSM_Status[0] = 0;
        pthread_mutex_lock(&gsm_mutex);
        gsm_flags |= GSM_FLAG_GSM_SPENTO;
        if(gsm_flags & GSM_FLAG_SPEGNIMENTO)
        {
          /* Se per qualche motivo dovesse arrivarmi una nuova
             richiesta di spegnimento, la elimino per evitare
             che una volta riacceso il gsm si spenga di nuovo. */
          /* E lo devo controllare prima della riaccensione, altrimenti
             se accodo un doppio flag da spento finisce che accendo e
             poi spengo, che non va bene. */
          gsm_flags &= ~GSM_FLAG_SPEGNIMENTO;
        }
        else if((gsm_flags & (GSM_FLAG_ACCENSIONE|GSM_FLAG_RESET)) && !gsm_timeout->active)
        {
          /* Garantisco 2 secondi tra spegnimento e riaccensione. */
          timeout_on(gsm_timeout, NULL, NULL, 0, 20);
          gsm_flags &= ~(GSM_FLAG_ACCENSIONE|GSM_FLAG_RESET);
          gsm_stato = ST_START;
        }
        pthread_mutex_unlock(&gsm_mutex);
        break;
      
      case ST_REINIT_SMS:
        /* Attende l'OK, poi reinizializza il GSM per gli SMS e rilegge
           l'ultimo SMS ricevuto, il cui indice è in gsm_sms_idx. */
        break;
      case ST_REINIT_SMS_1:
        gsm_send("AT+CMGF=1;+CNMI=2,1,0,0,0;+CSMP=17,167,0,241\r", 150);
        break;
      case ST_REINIT_SMS_2:
        gsm_send("AT+CSCS=\"8859-1\"\r", 150);
        break;
      case ST_REINIT_SMS_3:
        gsm_stato = ST_SMS_LEGGI;
        gsm_flags |= GSM_FLAG_REINIT;
        break;
      
      case ST_SAETNET:
        gsm_send("ATA\r", 600);
        break;
      case ST_SAETNET_ERROR:
        gsm_stato = ST_SAETNET_CHIUDI;
        break;
      case ST_SAETNET_AVVIA:
        /* data connection */
        GSM_Status[1] = 5;
        ser_setspeed(gsm_fd, B19200 | CRTSCTS, 0);
        gsm_device = prot_init(protSER, gsm_fd, gsm_consumer);
        gsm_device->modem = MODEM_GSM;
        codec_drop_events(gsm_device->consumer, gsm_recovery_dim);
        support_log("GSM Start protocol");
        prot_loop(gsm_device);
        support_log("GSM End protocol");
        // gsm_user_close:
        // 1 -> connessione chiusa dal programma utente o per timeout linea
        // 0 -> connessione chiusa per caduta linea
        if(gsm_user_close)	// connection closed by user program
        {
          support_log("GSM Terminating");
          gsm_stato = ST_SAETNET_SBLOCCA;
        }
        else
        {
          prot_close(gsm_device);
          gsm_stato = ST_SAETNET_FINE;
        }
        // ripristino il timeout
        ser_setspeed(gsm_fd, B19200 | CRTSCTS, 1);
        break;
      case ST_SAETNET_SBLOCCA:
        gsm_send("+++", 20);
        break;
      case ST_SAETNET_CHIUDI:
        gsm_send("ATH\r", 20);
        break;
      case ST_SAETNET_FINE:
        GSM_Status[1] = 0;
        pthread_mutex_lock(&gsm_mutex);
        if(gsm_call_cur)
        {
          free(gsm_call_cur);
          gsm_call_cur = NULL;
        }
        gsm_flags &= ~GSM_FLAG_DATA_CALL;
        pthread_mutex_unlock(&gsm_mutex);
        free(gsm_device);
        gsm_device = NULL;
        gsm_user_close = 0;
        gsm_stato = ST_MAIN;
        break;
      case ST_SAETNET_CHIAMA:
        sprintf(gsm_buf, "ATD%s\r", gsm_call_cur->num);
        gsm_send(gsm_buf, 600);
        break;
      case ST_SAETNET_ERROR_2:
        gsm_stato = ST_SAETNET_CHIUDI;
        break;
      
      case ST_CID_CHIAMA:
        gsm_send("AT+IPR=0\r", 5);
        break;
      case ST_CID_CHIAMA_2:
        sprintf(gsm_buf, "ATD%s;\r", config.PhoneBook[gsm_cid_num-1].Phone);
        gsm_send(gsm_buf, 600);
        break;
      case ST_CID_INVIA:
        CID_transmit(gsm_fd, &gsm_cid_num, gsm_cid_ev);
        if(!gsm_cid_num)
        {
          /* Evento trasmesso o esauriti i tentativi. */
          gsm_cid_num_base = 0;
        }
        else
        {
          /* Errore trasmissione, provo eventualmente il numero di backup
             (o ritorno al numero base, alterno) */
          if(gsm_cid_num == gsm_cid_num_base)
          {
            if((config.PhoneBook[gsm_cid_num_base/*-1+1*/].Abil & PB_TX) &&
               config.PhoneBook[gsm_cid_num_base/*-1+1*/].Phone[0])
              gsm_cid_num += 1;
          }
          else
            gsm_cid_num = gsm_cid_num_base;
        }
        gsm_stato = ST_CID_FINE;
        break;
      case ST_CID_FINE:
        gsm_send("AT\r", 5);
        break;
      case ST_CID_FINE_2:
        gsm_send("AT+IPR=19200\r", 5);
        break;
      case ST_CID_FINE_3:
        /* Chiude la chiamata e verifica se siano arrivati SMS */
        pthread_mutex_lock(&gsm_mutex);
        gsm_flags |= GSM_FLAG_SMS_VERIFY;
        pthread_mutex_unlock(&gsm_mutex);
        gsm_stato = ST_CHIUDI_CHIAMATA;
        break;
    }
  }
  
  return NULL;
}

void gsm_init(int type)
{
/*
  int retry;
  char logbuf[64];
*/
  
  gsm_consumer = support_find_serial_consumer(PROT_GSM, 0);

  /* if we get here, we have a gsm consumer configured, but check anyway */
  if(gsm_consumer < 0) return;

  gsm_serial[9] = '0' + config.consumer[gsm_consumer].configured - 1;
  
/*
  retry = 0;
  do
  {
    if(config.consumer[gsm_consumer].data.serial.baud2)
      gsm_fd = ser_open(gsm_serial, config.consumer[gsm_consumer].data.serial.baud2 | CRTSCTS, 10);
    else
      gsm_fd = ser_open(gsm_serial, config.consumer[gsm_consumer].data.serial.baud1 | CRTSCTS, 10);
    if(gsm_fd < 0)
    {
      retry++;
      support_log("GSM: Errore apertura seriale");
      sleep(1);
    }
  }
  while((gsm_fd < 0) && (retry < 5));
  
  if(gsm_fd < 0)
  {
    support_log("GSM Init error (2)");
    sprintf(logbuf, "GSM: Errore apertura '%s'", gsm_serial);
    support_log2(logbuf);
    return;
  }
*/
  
  //GSM_Status[0] = 1;
  GSM_Status[1] = 0;
  
  /* Accorcia il timeout sulla ricezione seriale */
/*
  if(config.consumer[gsm_consumer].data.serial.baud2)
    ser_setspeed(gsm_fd, config.consumer[gsm_consumer].data.serial.baud2 | CRTSCTS, 1);
  else
    ser_setspeed(gsm_fd, config.consumer[gsm_consumer].data.serial.baud1 | CRTSCTS, 1);
*/
  
  support_log("GSM Init");
  support_log2("GSM Init");

  if(!gsm_timeout) gsm_timeout = timeout_init();
  if(!gsm_mms_delay) gsm_mms_delay = timeout_init();
  if(!gsm_audio_timeout) gsm_audio_timeout = timeout_init();
  if(!gsm_credito_timeout) gsm_credito_timeout = timeout_init();
  
  gsm_stat_line_time_avg = time(NULL);
  gsm_moni1[0] = 0;
  gsm_moni2[0] = 0;
  gsm_csca[0] = 0;
  gsm_buf_idx = 0;
  
  pthread_create(&gsm_thd, NULL, gsm_manage, NULL);
  pthread_detach(gsm_thd);
}

void gsm_line_status()
{
  static int delay = 0;
  
  delay++;
  if(delay < 30) return;
  delay = 0;
  
  pthread_mutex_lock(&gsm_mutex);
  gsm_flags |= GSM_FLAG_PERIODIC;
  pthread_mutex_unlock(&gsm_mutex);
}

void gsm_user_program_end_loop()
{
}

void gsm_cancella_sms(int idx)
{
  struct data *msg;
  
  pthread_mutex_lock(&gsm_mutex);
  msg = malloc(sizeof(struct data));
  if(msg)
  {
    msg->msg = idx;
    /* Non è importante l'ordine. */
    msg->next = gsm_delete_sms;
    gsm_delete_sms = msg;
  }
  pthread_mutex_unlock(&gsm_mutex);
}

void gsm_lista_sms(void)
{
  pthread_mutex_lock(&gsm_mutex);
  gsm_flags |= GSM_FLAG_LISTA_SMS;
  pthread_mutex_unlock(&gsm_mutex);
}

void gsm_lista_leggi_sms(int idx)
{
  struct data *msg;
  
  msg = malloc(sizeof(struct data));
  msg->msg = idx;
  msg->next = gsm_read_sms;
  gsm_read_sms = msg;
}

int GSM_OnOff(int control)
{
  pthread_mutex_lock(&gsm_mutex);
  if(control < 0)
    gsm_flags |= GSM_FLAG_RESET;
  else if(control)
    gsm_flags |= GSM_FLAG_ACCENSIONE;
  else
    gsm_flags |= GSM_FLAG_SPEGNIMENTO;
  pthread_mutex_unlock(&gsm_mutex);
  return 0;
}

int GSM_Call(int call_type, int pbook_number)
{
  char buf[32];
  unsigned char ev[48];
  struct data *call, *tcall;
  
  if(master_behaviour == SLAVE_STANDBY) return 0;
  
  ev[0] = Evento_Esteso;
  ev[1] = Ex_Stringa_GSM;
  ev[2] = 102;
  sprintf(ev+4, str_d_15, call_type, pbook_number);
  ev[3] = strlen(ev+4);
  
  if(config.Log == 2) codec_queue_event(ev);
  
  if((pbook_number < 1) || (pbook_number > PBOOK_DIM)) return -1;
  if(GSM_Status[3] != 0) return -2;
  if(!config.PhoneBook[pbook_number-1].Phone[0]) return -3;
  if(call_type && !(config.PhoneBook[pbook_number-1].Abil & PB_TX)) return -4;
  //if(!call_type && !(config.PhoneBook[pbook_number-1].Abil & PB_GSM_TX)) return -4;
  /* Non gestisco le chiamate audio in questo modo, non ha senso. */
  if(!call_type) return -5;
  
  if(config.Log != 2) codec_queue_event(ev);
  support_log("GSM Call");
  
  if(config.PhoneBook[pbook_number-1].Name)
    support_called_number(config.PhoneBook[pbook_number-1].Name);
  else
    support_called_number(config.PhoneBook[pbook_number-1].Phone);
  
  call = (struct data*)malloc(sizeof(struct data));
  if(config.PhoneBook[pbook_number-1].Phone[0] == '+')
    sprintf(call->num, "%s", config.PhoneBook[pbook_number-1].Phone);
  else
    sprintf(call->num, "%s%s", config.PhonePrefix, config.PhoneBook[pbook_number-1].Phone);
  call->msg = -1;	// chiamata dati
  call->retry = 0;	// fa solo un tentativo
  call->text = NULL;
  call->next = NULL;
  
  pthread_mutex_lock(&gsm_mutex);
  /* Verfica eventuali duplicazioni */
  tcall = gsm_call_head;
  while(tcall)
  {
    if(!strcmp(tcall->num, call->num) && (tcall->msg == -1)) break;
    tcall = tcall->next;
  }
  if(tcall ||
     (gsm_call_cur && !strcmp(gsm_call_cur->num, call->num) && (gsm_call_cur->msg == -1)))
  {
    sprintf(buf, "Chiamata dati duplicata (%d)", pbook_number);
    support_log(buf);
    free(call);
  }
  else
  {
    if(!gsm_call_head)
      gsm_call_head = call;
    else
      gsm_call_tail->next = call;
    gsm_call_tail = call;
  }
  pthread_mutex_unlock(&gsm_mutex);
  
  return 0;
}

int GSM_Message(int msg, int pbook_number)
{
  char buf[32];
  unsigned char ev[48];
  struct data *call, *tcall;
  
  if(master_behaviour == SLAVE_STANDBY) return 0;
  
  ev[0] = Evento_Esteso;
  ev[1] = Ex_Stringa_GSM;
  ev[2] = 101;
  sprintf(ev+4, str_d_15, msg, pbook_number);
  ev[3] = strlen(ev+4);
  
  if(config.Log == 2) codec_queue_event(ev);
  
  if((pbook_number < 1) || (pbook_number > PBOOK_DIM)) return -1;
  if(GSM_Status[3] != 0) return -2;
  if(!config.PhoneBook[pbook_number-1].Phone[0]) return -3;
  if(!(config.PhoneBook[pbook_number-1].Abil & PB_GSM_TX) || (msg < 1)) return -4;

  if(config.Log != 2) codec_queue_event(ev);
  sprintf(buf, "GSM Call (message %d)", msg);
  support_log(buf);
  
  if(config.PhoneBook[pbook_number-1].Name)
    support_called_number(config.PhoneBook[pbook_number-1].Name);
  else
    support_called_number(config.PhoneBook[pbook_number-1].Phone);
  
  call = (struct data*)malloc(sizeof(struct data));
  if((config.PhoneBook[pbook_number-1].Phone[0] == '+') ||
     (!strcmp(config.PhonePrefix, "+39") && (config.PhoneBook[pbook_number-1].Phone[0] == '1')))
    sprintf(call->num, "%s", config.PhoneBook[pbook_number-1].Phone);
  else
    sprintf(call->num, "%s%s", config.PhonePrefix, config.PhoneBook[pbook_number-1].Phone);
  call->msg = msg;
  call->retry = 1;	// In totale fa 2 tentativi
  call->text = NULL;
  call->next = NULL;
  
  pthread_mutex_lock(&gsm_mutex);
  /* Verfica eventuali duplicazioni */
  tcall = gsm_call_head;
  while(tcall)
  {
    if(!strcmp(tcall->num, call->num) && (tcall->msg == msg)) break;
    tcall = tcall->next;
  }
  if(tcall ||
     (gsm_call_cur && !strcmp(gsm_call_cur->num, call->num) && (gsm_call_cur->msg == msg)))
  {
    sprintf(buf, "Messaggio duplicato (%d)", msg);
    support_log(buf);
    free(call);
  }
  else
  {
    if(!gsm_call_head)
      gsm_call_head = call;
    else
      gsm_call_tail->next = call;
    gsm_call_tail = call;
  }
  pthread_mutex_unlock(&gsm_mutex);
  
  return 0;
}

int GSM_Call_terminate(int call_type)
{
  if(master_behaviour == SLAVE_STANDBY) return 0;
  
  if(!GSM_Status[1]) return -1;
  
  /* data call */
  if((call_type == 1) && (GSM_Status[1] > 3))
  {
    prot_close(gsm_device);
    gsm_user_close = 1;
  }
  
  /* voice call */
  if((call_type == 0) && (GSM_Status[1] == 3))
  {
    support_log("gsm audio stop");
    audio_rec_stop(0);
    audio_play_stop();
  }
  return 0;
}

int GSM_Call_refuse()
{
  pthread_mutex_lock(&gsm_mutex);
  gsm_flags |= GSM_FLAG_CALL_CLOSE;
  pthread_mutex_unlock(&gsm_mutex);
  return 0;
}

int GSM_SMS_send_direct(char *number, char *message)
{
  struct data *sms;
  int i;
  
  if(master_behaviour == SLAVE_STANDBY) return 0;
  
  if(GSM_Status[3] != 0) return -2;
  if(!number || !number[0]) return -3;
  
  support_called_number(number);
  
  sms = (struct data*)malloc(sizeof(struct data));
  if(number[0] == '+')
    sprintf(sms->num, "%s", number);
  else
    sprintf(sms->num, "%s%s", config.PhonePrefix, number);
  
  /* Per la conversione delle lettere accentate */
  sms->text = (char*)malloc(strlen(message) + 2);
  utf8convert(message, sms->text);
//  gsmconvert(sms->text);
  // add Ctrl-Z to send message
  i = strlen(sms->text);
  sms->text[i] = 0x1a;
  sms->text[i+1] = 0;
  sms->retry = 1;
  sms->cbk = NULL;
  sms->cbk_data = NULL;
  sms->timeout = -1;	// non scade mai
  sms->next = NULL;
  
  pthread_mutex_lock(&gsm_mutex);
  if(!gsm_sms_head)
    gsm_sms_head = sms;
  else
    gsm_sms_tail->next = sms;
  gsm_sms_tail = sms;
  pthread_mutex_unlock(&gsm_mutex);
  
  return 0;
}

int GSM_SMS_send(int pbook_number, char *message)
{
  return GSM_SMS_send_callback(pbook_number, message, NULL, NULL);
}

int GSM_SMS_send_callback(int pbook_number, char *message, void (*func)(int,void*), void *data)
{
  unsigned char ev[48];
  struct data *sms;
  int i;
  
  if(master_behaviour == SLAVE_STANDBY) return 0;
  
  ev[0] = Evento_Esteso;
  ev[1] = Ex_Stringa_GSM;
  ev[2] = 100;
  sprintf(ev+4, str_d_16, pbook_number);
  ev[3] = strlen(ev+4);
  
  if(config.Log == 2) codec_queue_event(ev);
  
  if((pbook_number < 1) || (pbook_number > PBOOK_DIM)) return -1;
  if(GSM_Status[3] != 0) return -2;
  if(!config.PhoneBook[pbook_number-1].Phone[0]) return -3;
  if(!(config.PhoneBook[pbook_number-1].Abil & PB_SMS_TX)) return -4;

  if(config.Log != 2) codec_queue_event(ev);
  support_log("SMS Send");
  
  if(config.PhoneBook[pbook_number-1].Name)
    support_called_number(config.PhoneBook[pbook_number-1].Name);
  else
    support_called_number(config.PhoneBook[pbook_number-1].Phone);
  
  sms = (struct data*)malloc(sizeof(struct data));
  if(config.PhoneBook[pbook_number-1].Phone[0] == '+')
    sprintf(sms->num, "%s", config.PhoneBook[pbook_number-1].Phone);
  else
    sprintf(sms->num, "%s%s", config.PhonePrefix, config.PhoneBook[pbook_number-1].Phone);
  
  /* Per la conversione delle lettere accentate */
  sms->text = (char*)malloc(strlen(message) + 2);
  utf8convert(message, sms->text);
//  gsmconvert(sms->text);
  // add Ctrl-Z to send message
  i = strlen(sms->text);
  sms->text[i] = 0x1a;
  sms->text[i+1] = 0;
  sms->retry = 1;
  sms->cbk = func;
  sms->cbk_data = data;
  if(func)
    sms->timeout = 3000;	// 5 minuti
  else
    sms->timeout = -1;	// non scade mai
  sms->next = NULL;
  
  pthread_mutex_lock(&gsm_mutex);
  if(!gsm_sms_head)
    gsm_sms_head = sms;
  else
    gsm_sms_tail->next = sms;
  gsm_sms_tail = sms;
  pthread_mutex_unlock(&gsm_mutex);
  
  return 0;
}

int GSM_SMS_s_c_n(char *n_to_store)
{
  /* Mai usato e tutto sommato non ha senso usarlo. */
  return 0;
}

int GSM_Status_event(int event_index)
{
  int i, j;
  unsigned char ev[48];
  struct tm *t;
  time_t tt;
    
  ev[0] = Evento_Esteso;
  ev[1] = Ex_Stringa_GSM;
  ev[2] = event_index;

  switch(event_index)
  {
    case 0:
      if(!gsm_connected) return -1;
      ev[3] = strlen(gsm_moni1) + 5;
      sprintf(ev+4, "1/2: %s", gsm_moni1);
      codec_queue_event(ev);
      ev[3] = strlen(gsm_moni2) + 5;
      sprintf(ev+4, "2/2: %s", gsm_moni2);
      codec_queue_event(ev);
      break;
    case 1:
      if(!gsm_connected) return -1;
      ev[3] = strlen(gsm_csca);
      strcpy(ev+4, gsm_csca);
      codec_queue_event(ev);
      break;
    case 2:
      for(i=0; i<PBOOK_DIM; i++)
        if(config.PhoneBook[i].Phone[0])
        {
          ev[3] = strlen(config.PhoneBook[i].Phone) + 5;
          sprintf(ev+4, "%2d - %s", i+1, config.PhoneBook[i].Phone);
          codec_queue_event(ev);
        }
      break;
    case 3:
      if(!gsm_connected) return -1;
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
    case 4:
      if(!gsm_connected) return -1;
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
    case 5:
      if(!gsm_connected) return -1;
      i = support_received_number_idx;
      if(!support_received_numbers[i].time) i = 0;
        
      for(j=0; j<CALLED_NUMBERS; j++,i++)
        if(support_received_numbers[i%CALLED_NUMBERS].time)
        {
          ev[3] = strlen(support_received_numbers[i%CALLED_NUMBERS].number) + 5;
          sprintf(ev+4, "%2d - %s", j+1, support_received_numbers[i%CALLED_NUMBERS].number);
          codec_queue_event(ev);
          sprintf(ev+4, "%2d - ", j+1);
          strftime(ev+9, 22, "%x %X", localtime(&support_received_numbers[i%CALLED_NUMBERS].time));
          ev[3] = strlen(ev+4);
          codec_queue_event(ev);
        }
      break;
    case 6:
      ev[3] = 1;
      ev[4] = GSM_Status[3] + '0';
      codec_queue_event(ev);
      break;
    case 7:
      if(!gsm_connected) return -1;
      t = localtime(&gsm_stat_line_time_min);
      sprintf(ev + 4, "min: %2d ", gsm_stat_line_min);
      strftime(ev + 12, 28, "(%d/%m/%Y %T)", t);
      ev[3] = strlen(ev + 4);
      codec_queue_event(ev);
      t = localtime(&gsm_stat_line_time_max);
      sprintf(ev + 4, "max: %2d ", gsm_stat_line_max);
      strftime(ev + 12, 28, "(%d/%m/%Y %T)", t);
      ev[3] = strlen(ev + 4);
      codec_queue_event(ev);
    case 8:
      if(!gsm_connected) return -1;
      sprintf(ev + 4, "act: %2d", gsm_stat_line_act);
      ev[3] = strlen(ev + 4);
      codec_queue_event(ev);
      tt = time(NULL) - gsm_stat_line_time_avg;
      t = gmtime(&tt);
      if(gsm_stat_line_cnt)
        sprintf(ev + 4, "avg: %2d (%ld ", gsm_stat_line_val/gsm_stat_line_cnt, tt/86400);
      else
        sprintf(ev + 4, "avg: NA (%ld ", tt/86400);
      strftime(ev + 4 + strlen(ev + 4), 10, "%T)", t);
      ev[3] = strlen(ev + 4);
      codec_queue_event(ev);
      break;
    case 9:
      gsm_stat_line_val = 0;
      gsm_stat_line_cnt = 0;
      gsm_stat_line_min = 99;
      gsm_stat_line_max = 0;
      gsm_stat_line_time_min = 0;
      gsm_stat_line_time_max = 0;
      gsm_stat_line_time_avg = time(NULL);
      ev[3] = 2;
      ev[4] = 'O';
      ev[5] = 'K';
      codec_queue_event(ev);
      break;
  }
  return 0;
}

int GSM_Set_recovery(int onoff, int dim)
{
  if(onoff)
    gsm_recovery_dim = dim;
  else
    gsm_recovery_dim = DIM_EVENT_BUFFER+1;
  
  return 0;
}

int GSM_SIM_Credit()
{
  if(gsm_operator < 0) return 0;
  
/*
  if(gsm_operator_data[gsm_operator].credit[2] == 'D')
  {
    gsm_send(gsm_operator_data[gsm_operator].credit, 600, 0);
  }
*/
  pthread_mutex_lock(&gsm_mutex);
  gsm_flags |= GSM_FLAG_CREDITO;
  pthread_mutex_unlock(&gsm_mutex);
  
  /* Wind restituisce un sms flash, non un sms normale, devo quindi
     segnalare questo fatto alla console affinché rimanga in attesa. */
  return gsm_operator_data[gsm_operator].retval;
}

int GSM_Clear_Queue()
{
  struct data *sms;
  struct data *call;
  
  if(master_behaviour == SLAVE_STANDBY) return 0;
  
  /* Elimina dalla coda tutte le chiamate e gli sms/mms in partenza */
  support_log("Cancellazione coda gsm");
  
  pthread_mutex_lock(&gsm_mutex);
  //pthread_mutex_lock(&gsm_mms_mutex);
  
  while(gsm_sms_head)
  {
    sms = gsm_sms_head;
    gsm_sms_head = gsm_sms_head->next;
    if(sms->cbk) sms->cbk(-2, sms->cbk_data);
    free(sms->text);
    free(sms);
  }
  gsm_sms_tail = NULL;
  
  while(gsm_gprs_head)
  {
    sms = gsm_gprs_head;
    gsm_gprs_head = gsm_gprs_head->next;
    if(sms->func) sms->func(-1, sms->data);
    free(sms->ipaddr);
    free(sms);
  }
  gsm_gprs_tail = NULL;
  
  while(gsm_call_head)
  {
    call = gsm_call_head;
    gsm_call_head = gsm_call_head->next;
    free(call);
  }
  gsm_call_tail = NULL;
  
  //pthread_mutex_unlock(&gsm_mms_mutex);
  pthread_mutex_unlock(&gsm_mutex);
  
  return 0;
}

int GSM_Soglia_Credito(float soglia)
{
  /* Segnalazione impulsiva di credito basso, ogni volta che
     viene fatta una richiesta di credito. */
  if(gsm_credito_valido)
  {
    gsm_credito_valido = 0;
    if(gsm_credito_residuo < soglia) return 1;
  }
  return 0;
}

int GSM_Soglia_Credito_cont(float soglia)
{
  /* Segnalazione continua del livello di credito, in assenza
     di credito valido viene segnalato credito basso. */
  if(gsm_credito_residuo < soglia) return 1;
  return 0;
}

int GSM_Squillo()
{
  int ret;
  
  ret = gsm_ring_from;
  if(gsm_ring_from != 0) gsm_ring_from = 0;
  return ret;
}

int GSM_Squillo_sconosciuto()
{
  int ret;
  
  ret = gsm_ring_from_sconosciuto;
  if(gsm_ring_from_sconosciuto != 0) gsm_ring_from_sconosciuto = 0;
  return ret;
}

/* Per compatibilità verso plugin che gestiscono il gsm in modo autonomo. */
void gsm_poweron(int fd)
{
  int ret, i;
  
#ifdef __CRIS__
  i = ioctl(fd, TIOCMGET, &ret);
    
  if(!(ret & TIOCM_CTS))
  {
    support_log("GSM power on");
    support_log2("GSM power on");
    ret = TIOCM_RTS;
    ret = ioctl(fd, TIOCMBIC, &ret);
    usleep(100000);
    ret = TIOCM_RTS;
    ret = ioctl(fd, TIOCMBIS, &ret);
    for(i=0; i<20; i++)
    {
      ioctl(fd, TIOCMGET, &ret);
      if(!(ret & TIOCM_CTS))
        sleep(1);
      else
        i = 20;
    }
  }
  sleep(2);
#endif

#ifdef __arm__
  fd = open("/dev/gpio", O_RDWR);
  ret = ioctl(fd, IO_GSM_POWERMON, 0);
  if(ret)
  {
    support_log("GSM power on");
    support_log2("GSM power on");
    ret = ioctl(fd, IO_GSM_SHUTDOWN, 0);
    ret = ioctl(fd, IO_GSM_ONOFF, 0);
    for(i=0; i<20; i++)
    {
      ret = ioctl(fd, IO_GSM_POWERMON, 0);
      if(!ret) break;
    }
  }
  close(fd);
  sleep(2);
#endif
}



