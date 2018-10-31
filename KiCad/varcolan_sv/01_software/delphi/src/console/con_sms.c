#include "console.h"
#include "con_sms.h"
#include "../gsm.h"
#include "../support.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/***********************************************************************
	GSM menu functions
***********************************************************************/

static char console_sms_icon[] = {
  'A', 'T', 'K', '0', '0', '0', '0', '0', '0', '0', '8', '0', '1',
  0x7e, 0x46, 0x4a, 0x52, 0x52, 0x4a, 0x46, 0x7e, '\r', '\0'};

static char console_gsm_signal_icon[6][23] = {
    {'A', 'T', 'K', '1', '2', '0', '0', '7', '0', '0', '8', '0', '1',
    0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, '\r', '\0'},
    {'A', 'T', 'K', '1', '2', '0', '0', '7', '0', '0', '8', '0', '1',
    0x80, 0x80, 0x80, 0x80, 0xf8, 0xf8, 0xf8, 0x80, '\r', '\0'},
    {'A', 'T', 'K', '1', '2', '0', '0', '7', '0', '0', '8', '0', '1',
    0xfc, 0xfc, 0xfc, 0x80, 0xf8, 0xf8, 0xf8, 0x80, '\r', '\0'},
    {'A', 'T', 'K', '1', '1', '2', '0', '7', '0', '0', '8', '0', '1',
    0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, '\r', '\0'},
    {'A', 'T', 'K', '1', '1', '2', '0', '7', '0', '0', '8', '0', '1',
    0x80, 0x80, 0x80, 0x80, 0xfe, 0xfe, 0xfe, 0x80, '\r', '\0'},
    {'A', 'T', 'K', '1', '1', '2', '0', '7', '0', '0', '8', '0', '1',
    0xff, 0xff, 0xff, 0x80, 0xfe, 0xfe, 0xfe, 0x80, '\r', '\0'},
  };

static int console_gsm_signal_strength[2][32] = {
  {0, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2},
  {3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5},
  };

static char *console_sms_msg = NULL;
static pthread_mutex_t console_sms_mutex = PTHREAD_MUTEX_INITIALIZER;

#define SMS_NUM	50

static struct _console_sms_messages{
  time_t time;
  int idx;
  char *number;
  char *date;
  char *message;
} console_sms_messages[SMS_NUM+1] = {[0 ... SMS_NUM] = {0, 0, NULL, NULL, NULL}};

static CALLBACK void* console_sms_send2(ProtDevice *dev, int number_as_int, void *null)
{
  GSM_SMS_send_direct((char*)number_as_int, console_sms_msg);
  
  free(console_sms_msg);
  console_sms_msg = NULL;

  console_send(dev, "ATF0\r");
  console_send(dev, "ATS00\r");
  console_send(dev, str_13_0);

  timeout_on(CONSOLE->timeout, (timeout_func)console_return_to_menu, dev, 0, TIMEOUT_MSG);
  
  return NULL;
}

static CALLBACK void* console_sms_send1(ProtDevice *dev, int msg_as_int, void *null)
{
  free(console_sms_msg);
  console_sms_msg = strdup((char*)msg_as_int);

  console_send(dev, "ATF0\r");
  console_send(dev, "ATS03\r");
  console_send(dev, str_13_1);
  console_send(dev, str_13_20);
  console_send(dev, "ATF1\r");
  console_send(dev, "ATL111\r");

  console_register_callback(dev, KEY_STRING, console_sms_send2, NULL);
  console_register_callback(dev, KEY_CANCEL, console_sms_send, NULL);

  return NULL;
}

CALLBACK void* console_sms_send(ProtDevice *dev, int press, void *null)
{
  console_disable_menu(dev, str_13_2);
  
  console_send(dev, str_13_3);
  console_send(dev, str_13_21);
  console_send(dev, "ATF1\r");
  console_send(dev, "ATL211\r");
  
  console_register_callback(dev, KEY_STRING, console_sms_send1, NULL);
  console_register_callback(dev, KEY_CANCEL, console_show_menu, NULL);

  return NULL;
}

static CALLBACK void* console_sms_read_exit(ProtDevice *dev, int press, void *null)
{
  console_show_menu(dev, 0, NULL);
  CONSOLE->state = 0;
  CONSOLE->list_show_cancel = NULL;
  pthread_mutex_unlock(&console_sms_mutex);
  
  CONSOLE->callback[KEY_TIMEOUT] = NULL;
  CONSOLE->callback_arg[KEY_TIMEOUT] = NULL;
  
  return NULL;
}

static CALLBACK void* console_sms_exit_logout(ProtDevice *dev, int press, void *null)
{
  if(press)
  {
    CONSOLE->state = 0;
    CONSOLE->list_show_cancel = NULL;
    pthread_mutex_unlock(&console_sms_mutex);
  }
  
  console_logout(dev, press, null);
  
  return NULL;
}

static void console_sms_exit_timeout(char *dev, int null)
{
  console_sms_read_exit((ProtDevice*)dev, 0, NULL);
}

static int console_sms_row = 0;
static CALLBACK void* console_sms_list(ProtDevice *dev, int press, void *null);
static CALLBACK void* console_sms_read1(ProtDevice *dev, int press, void *idx_as_voidp);
static CALLBACK void* console_sms_read2(ProtDevice *dev, int press, void *idx_as_voidp);

static CALLBACK void* console_sms_read_up(ProtDevice *dev, int press, void *idx_as_voidp)
{
  console_sms_row--;
  console_sms_read2(dev, 0, idx_as_voidp);
  return NULL;
}

static CALLBACK void* console_sms_read_down(ProtDevice *dev, int press, void *idx_as_voidp)
{
  console_sms_row++;
  console_sms_read2(dev, 0, idx_as_voidp);
  return NULL;
}

static CALLBACK void* console_sms_read_delete1(ProtDevice *dev, int press, void *idx_as_voidp)
{
  gsm_cancella_sms(console_sms_messages[(int)idx_as_voidp].idx);
  
  free(console_sms_messages[(int)idx_as_voidp].number);
  console_sms_messages[(int)idx_as_voidp].number = NULL;
  free(console_sms_messages[(int)idx_as_voidp].date);
  console_sms_messages[(int)idx_as_voidp].date = NULL;
  free(console_sms_messages[(int)idx_as_voidp].message);
  console_sms_messages[(int)idx_as_voidp].message = NULL;

  console_send(dev, "ATS03\r");  
  console_send(dev, str_13_4);  
  console_send(dev, str_13_5);  

  CONSOLE->last_menu_item = console_sms_list;
  timeout_on(CONSOLE->timeout, (timeout_func)console_return_to_last_menu_item, dev, 0, TIMEOUT_MSG);
  
  return NULL;
}

static CALLBACK void* console_sms_read_delete(ProtDevice *dev, int press, void *idx_as_voidp)
{
  console_send(dev, "ATS03\r");  
  console_send(dev, str_13_6);  
  console_send(dev, str_13_7);  

  console_register_callback(dev, KEY_CANCEL, console_sms_read1, idx_as_voidp);
  console_register_callback(dev, KEY_ENTER, console_sms_read_delete1, idx_as_voidp);
  console_register_callback(dev, KEY_TIMEOUT, console_sms_read_exit, NULL);
  console_register_callback(dev, KEY_DIERESIS, console_sms_exit_logout, NULL);

  return NULL;
}

static CALLBACK void* console_sms_read2(ProtDevice *dev, int press, void *idx_as_voidp)
{
  int i, len, w, p, pp, idx = (int)idx_as_voidp;
  char cmd[40];
  
  if(CONSOLE->tipo == CONSOLE_STL02)
  {
    p = pp = 0;
    /* Calcola e salta le prime righe */
    for(i=0; i<console_sms_row; i++)
    {
      w = 22;
      
      p += w;
      while((p>pp) && (console_sms_messages[idx].message[p]!=' ')) p--;
      if(p == pp)
      {
        /* La riga è indivisibile */
        pp += w;
        p = pp;
      }
      else
      {
        p++;
        pp = p;
      }
    }
    /* Ora p e pp puntano all'inizio della prima riga da visualizzare */
    for(i=0; (i<5)&&(pp<strlen(console_sms_messages[idx].message)); i++)
    {
      w = strlen(console_sms_messages[idx].message+pp);
      if(w > 22) w = 22;
      
      sprintf(cmd, str_13_22, i+3);
      len = strlen(cmd);
      memcpy(cmd+len, console_sms_messages[idx].message+pp, w);
      
      p += w;
      if(strlen(console_sms_messages[idx].message+pp) > 22)
        while((p>pp) && (console_sms_messages[idx].message[p]!=' ')) p--;
      if(p == pp)
      {
        /* La riga è indivisibile */
        memcpy(cmd+len+w, "\t\r", 3);
        pp += w;
        p = pp;
      }
      else
      {
        memcpy(cmd+len+p-pp, "\t\r", 3);
        p++;
        pp = p;
      }
      
      console_send(dev, cmd);
    }
    
    if(pp >= strlen(console_sms_messages[idx].message))
    {
      console_send(dev, "ATR1\r");
      console_send(dev, str_13_9);
      console_send(dev, "ATR0\r");
      console_register_callback(dev, KEY_ENTER, console_sms_read_delete, idx_as_voidp);
    }
    else
    {
      console_send(dev, "ATS08\r");
      console_register_callback(dev, KEY_DOWN, console_sms_read_down, idx_as_voidp);
    }
  }
  else
  {
    w = 16;
  
    if(console_sms_messages[idx].message && (strlen(console_sms_messages[idx].message) > 0))
    {
      for(i=console_sms_row; 
      ((i-console_sms_row)<=(((strlen(console_sms_messages[idx].message+(console_sms_row*w))-1)/w)))
       && ((i-console_sms_row)<5); i++)
      {
        sprintf(cmd, str_13_22, (i-console_sms_row)+3, console_sms_messages[idx].message+(i*w));
        console_send(dev, cmd);
      }
    }
    
    if((strlen(console_sms_messages[idx].message+(console_sms_row*w))) <= (w*5))
    {
      console_send(dev, "ATR1\r");
      console_send(dev, str_13_9);
      console_send(dev, "ATR0\r");
      console_register_callback(dev, KEY_ENTER, console_sms_read_delete, idx_as_voidp);
    }
    else
    {
      console_send(dev, "ATS08\r");
      console_register_callback(dev, KEY_DOWN, console_sms_read_down, idx_as_voidp);
    }
  }
  if(console_sms_row > 0) console_register_callback(dev, KEY_UP, console_sms_read_up, idx_as_voidp);
  console_register_callback(dev, KEY_CANCEL, console_sms_list, NULL);
  console_register_callback(dev, KEY_TIMEOUT, console_sms_read_exit, NULL);
  console_register_callback(dev, KEY_DIERESIS, console_sms_exit_logout, NULL);

  return NULL;
}

static CALLBACK void* console_sms_read1(ProtDevice *dev, int press, void *idx_as_voidp)
{
  console_sms_row = 0;
  console_send(dev, "ATS03\r");  
  
  console_sms_read2(dev, 0, idx_as_voidp);
  
  return NULL;
}

CALLBACK void* console_sms_read(ProtDevice *dev, int press, void *null)
{
  int i;
  
  if(pthread_mutex_trylock(&console_sms_mutex))
  {
    console_send(dev, "ATS00\r");
    console_send(dev, str_13_10);
    console_send(dev, str_13_11);
    timeout_on(CONSOLE->timeout, (timeout_func)console_return_to_menu, dev, 0, TIMEOUT_MSG);
    return NULL;
  }
  
  console_disable_menu(dev, str_13_12);

  console_send(dev, str_13_13);
  console_send(dev, str_13_14);
  console_send(dev, str_13_20);
  
  for(i=0; i<=SMS_NUM; i++)
  {
    console_sms_messages[i].time = 0;
    console_sms_messages[i].idx = 0;
    free(console_sms_messages[i].number);
    console_sms_messages[i].number = NULL;
    free(console_sms_messages[i].date);
    console_sms_messages[i].date = NULL;
    free(console_sms_messages[i].message);
    console_sms_messages[i].message = NULL;
  }
  CONSOLE->state = 1;
  CONSOLE->pos = 0;

  console_register_callback(dev, KEY_CANCEL, console_sms_read_exit, NULL);
  console_register_callback(dev, KEY_TIMEOUT, console_sms_read_exit, NULL);
  console_register_callback(dev, KEY_DIERESIS, console_sms_exit_logout, NULL);
  
  gsm_lista_sms();
  
  return NULL;
}

static int console_sms_order(const void *v1, const void *v2)
{
  if(((struct _console_sms_messages*)v1)->time < ((struct _console_sms_messages*)v2)->time) return 1;
  return -1;
}

static CALLBACK void* console_sms_list(ProtDevice *dev, int press, void *null)
{
  int i;
  char *date;
  struct tm dh;
  
  console_list_free(CONSOLE->support_list);
  CONSOLE->support_list = NULL;
  
  for(i=1; (i<=SMS_NUM); i++)
    if(console_sms_messages[i].number)
    {
//      console_sms_messages[i].idx = i;
      sscanf(console_sms_messages[i].date, "%d/%d/%d,%d:%d:%d",
          &dh.tm_year, &dh.tm_mon, &dh.tm_mday, &dh.tm_hour, &dh.tm_min, &dh.tm_sec);
      dh.tm_mon--;
      dh.tm_year += 100;
      dh.tm_isdst = -1;
      console_sms_messages[i].time = mktime(&dh);
    }
    else
      console_sms_messages[i].time = 0;
  
  qsort(console_sms_messages+1, SMS_NUM, sizeof(struct _console_sms_messages), console_sms_order);
  
  for(i=1; (i<=SMS_NUM); i++)
    if(console_sms_messages[i].number)
    {
      date = console_format_time(dev, console_sms_messages[i].time);
      CONSOLE->support_list = console_list_add(CONSOLE->support_list, date,
          console_sms_messages[i].number, console_sms_read1, i);
      free(date);
    }
    
  console_send(dev, "ATS03\r");  
  CONSOLE->list_show_cancel = console_sms_read_exit;

  console_list_show(dev, CONSOLE->support_list, 0, 3, 0);
  console_register_callback(dev, KEY_CANCEL, console_sms_read_exit, NULL);
  console_register_callback(dev, KEY_TIMEOUT, console_sms_read_exit, NULL);
  console_register_callback(dev, KEY_DIERESIS, console_sms_exit_logout, NULL);
  
  return NULL;
}

CALLBACK void* console_sms_credit(ProtDevice *dev, int press, void *null)
{
  console_disable_menu(dev, str_13_15);
  
  console_send(dev, str_13_16);
  console_send(dev, str_13_17);
  
  if(GSM_SIM_Credit())
    console_register_callback(dev, KEY_CANCEL, console_show_menu, NULL);
  else
    timeout_on(CONSOLE->timeout, (timeout_func)console_return_to_menu, dev, 0, TIMEOUT_MSG*2);
  
  return NULL;
}

/* Dopo la lista vengono richiesti tutti i messaggi. Questa operazione
   potrebbe essere interrotta da una chiamata nel mezzo. */
void console_sms_message(char *msg, int selint)
{
  int i, j, num, cons;
  char cmd[32];
  ProtDevice *dev;
  /* Solo una tastiera alla volta può leggere la lista degli sms, quindi
     non c'è sovrapposizione. */
  static int current = 0;
  /* Per lo stesso motivo posso usare una variabile statica anche per la
     dimensione attesa del testo dell'sms. */
  static int smslen = 0;
  
  cons = -1;
  while((cons = support_find_serial_consumer(PROT_CONSOLE, ++cons)) >= 0)
  {
    dev = config.consumer[cons].dev;
    
    if(!CONSOLE) continue;
    
    if(CONSOLE->active && !strncmp(msg, "+CMTI:", 6))
    {
      console_send(dev, "ATP04\r");
      console_send(dev, console_sms_icon);
    }
    
    if((CONSOLE->active == 1) && !strncmp(msg, "+CSQ:", 5))
    {
      sscanf(msg + 5, "%d", &i);
      //if(i != 99)
      if(i <= 31)
      {
        console_send(dev, console_gsm_signal_icon[console_gsm_signal_strength[0][i]]);
        console_send(dev, console_gsm_signal_icon[console_gsm_signal_strength[1][i]]);
      }
      else
      {
        console_send(dev, console_gsm_signal_icon[0]);
        console_send(dev, console_gsm_signal_icon[3]);
      }
    }
    
  switch(CONSOLE->state)
  {
    case 0:
      if(!strncmp(msg, "+CUSD:", 6))
      {
        if(CONSOLE->active == 2)
        {
          timeout_on(CONSOLE->timeout, (timeout_func)console_return_to_menu, dev, 0, 40);
          console_send(dev, "ATS00\r");
        }
// Questa forma viene filtrata all'origine e non arriva fino a qui:
// +CUSD: 0,"Costo chiamata: 0.22 Euro. Il credito e':16.76 Euro. WIND INFO:Con MegaOre navighi in Internet (cellulare o PC Card) 50 ore a soli 9euro mese.Info su Wind.it",0            

// Devo invece gestire le due forme:
// +CUSD: 0,"Il credito e': 0.0000 Euro; Credito Dati:0.0000 Euro.",0
// +CUSD: 0,"Il credito e': 6.70 Euro.",15
// +CUSD: 2,"Il Credito è 10.00 Euro. Scopra i nuovi tagli di ricarica a partire da 10 euro da Bancomat e dal suo Home Banking! Info su Wind.it",15
        //msg[24] = 0;
        for(i=21; msg[i]!=' '; i++);
        msg[i++] = 0;
        sprintf(cmd, str_13_23, msg+10);
        if(CONSOLE->active == 2) console_send(dev, cmd);
        for(j=i; msg[j]!='o'; j++);
        j++;
        num = msg[j];
        msg[j] = 0;
        sprintf(cmd, str_13_24, msg+25);
        if(CONSOLE->active == 2) console_send(dev, cmd);
        if(num == ';')
        {
          /* Credito dati */
          num = j+1;
          for(i=num; msg[i]!=':'; i++);
          msg[i] = 0;
          sprintf(cmd, str_13_25, msg+num);
          if(CONSOLE->active == 2) console_send(dev, cmd);
          num = i+1;
          for(i=num; msg[i]!='o'; i++);
          i++;
          msg[i] = 0;
          sprintf(cmd, str_13_26, msg+num);
          if(CONSOLE->active == 2) console_send(dev, cmd);
        }
      }
      break;
    case 1:
      if(!strncmp(msg, "+CMGL:", 6))
      {
        if(!CONSOLE->pos)
        {
          console_send(dev, "ATO.\r");
          sprintf(cmd, "ATE%04d\r", console_login_timeout_time/10);
          console_send(dev, cmd);
        }
        
        CONSOLE->pos++;
        if(CONSOLE->pos > 3) CONSOLE->pos = 0;
        current = 1;
        
        sscanf(msg+6, "%d", &num);
        for(i=6; msg[i]!=','; i++);
        for(i++; msg[i]!=','; i++);
        if(msg[i+1] == '"')
        {
          if(num <= SMS_NUM)
          {
            console_sms_messages[num].idx = num;
            console_sms_messages[num].number = strdup(msg + i + 2);
            for(i=0; console_sms_messages[num].number[i]&&(console_sms_messages[num].number[i]!='"'); i++);
            console_sms_messages[num].number[i] = 0;
            
            /* Se il modulo lo permette, recupero subito la data del messaggio. */
            if(selint > 0)
            {
              for(i+=2; console_sms_messages[num].number[i]!=','; i++);
              console_sms_messages[num].date = strdup(console_sms_messages[num].number + i + 2);
              for(i=0; console_sms_messages[num].date[i]&&(console_sms_messages[num].date[i]!='"'); i++);
              console_sms_messages[num].date[i] = 0;
              
              /* Recupero anche la lunghezza del testo per eventualmente fare la conversione UCS2 */
              for(i=strlen(msg); msg[i]!=','; i--);
              sscanf(msg+i+1, "%d", &smslen);
              
              CONSOLE->state = 3;
              current = num;
            }
            else
            /* Altrimenti accodo una lettura singola. */
              gsm_lista_leggi_sms(num);
          }
        }
        else
        {
          /* si tratta di un messaggio di stato */
#if 1
          gsm_cancella_sms(num);
#else
          for(i=strlen(msg)-1; msg[i]!=','; i--);
          sprintf(cmd, "Stato SMS %s", msg+i+1);
          console_sms_messages[num].number = strdup(cmd);
#endif
        }
      }
      else if(!strncmp(msg, "OK", 2))
      {
        for(i=0; (i<=SMS_NUM) && (!console_sms_messages[i].number || console_sms_messages[i].date); i++);
        if(i <= SMS_NUM)
        {
          CONSOLE->state = 2;
        }
        else
        {
          CONSOLE->state = 0;
          console_sms_list(dev, 0, NULL);
        }
      }
      else if(!strncmp(msg, "+CMS ERROR", 10) || !strncmp(msg, "ERROR", 5))
      {
        sprintf(cmd, "ATE%04d\r", console_login_timeout_time / 10);
        console_send(dev, cmd);
        console_send(dev, "ATS06\r");
        console_send(dev, str_13_19);
        CONSOLE->state = 0;
        timeout_on(CONSOLE->timeout, (timeout_func)console_sms_exit_timeout, dev, 0, TIMEOUT_MSG);
      }
      break;
    case 2:
      if(!strncmp(msg, "AT+CMGR=", 8))
      {
        sscanf(msg+8, "%d", &current);
      }
      else if(!strncmp(msg, "+CMGR:", 6))
      {
        if(!CONSOLE->pos) console_send(dev, "ATO.\r");
        CONSOLE->pos++;
        if(CONSOLE->pos > 3) CONSOLE->pos = 0;
        
        for(i=6; msg[i]!=','; i++);
        for(i++; msg[i]!=','; i++);
        for(i++; msg[i]!=','; i++);
        console_sms_messages[current].date = strdup(msg + i + 2);
        
        /* Recupero anche la lunghezza del testo per eventualmente fare la conversione UCS2 */
        for(i=strlen(msg); msg[i]!=','; i--);
        sscanf(msg+i+1, "%d", &smslen);
        
        CONSOLE->state = 3;

        sprintf(cmd, "ATE%04d\r", console_login_timeout_time / 10);
        console_send(dev, cmd);
      }
      else if(!strncmp(msg, "+CMS ERROR", 10) || !strncmp(msg, "ERROR", 5))
      {
        for(i=0; (i<=SMS_NUM) && !console_sms_messages[i].number; i++);
        if(i <= SMS_NUM)
        {
          CONSOLE->state = 2;
        }
        else
        {
          CONSOLE->state = 0;
          console_sms_list(dev, 0, NULL);
        }
      }
      break;
    case 3:
      console_sms_messages[current].message = strdup(msg);
      if(strlen(console_sms_messages[current].message) == (smslen*4))
      {
        char hex[8];
        
        /* Si tratta di un messaggio UCS2 */
        hex[4] = 0;
        for(i=0; i<strlen(console_sms_messages[current].message); i+=4)
        {
          memcpy(hex, console_sms_messages[current].message+i, 4);
          sscanf(hex, "%x", &num);
          if(num > 255) num = ' ';
          console_sms_messages[current].message[i/4] = num;
        }
        console_sms_messages[current].message[i/4] = 0;
      }
      CONSOLE->state = 1;
      break;
    default:
      break;
  }
  
  }
}

/*

at+cmgl=ALL                                                                     
+CMGL:  1,"REC READ","2101"                                                     
Benvenuto in Vodafone Omnitel! Con Fai da Te Aziende accedi a un mondo di servie
+CMGL:  2,"REC READ","+393408709107"                                            
Prova 0                                                                         
+CMGL:  14,"REC READ","Vodafone"                                                
Vodafone Omnitel ti regala 100 SMS gratuiti al giorno dall'Italia per 1 mese a .
OK                                                                              

+CMGR: "REC READ","Vodafone",,"02/11/18,15:13:42+40"                            
Vodafone Omnitel ti regala 100 SMS gratuiti al giorno dall'Italia per 1 mese a .
OK                                                                              

*/
