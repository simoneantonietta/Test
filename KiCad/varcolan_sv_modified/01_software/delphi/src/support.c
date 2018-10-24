#include "support.h"
#include "database.h"
#include "modem.h"
#include "gsm.h"
#include "master.h"
#include "lara.h"
#include "delphi.h"
#include <stdlib.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <termios.h>
#include <syscall.h>

#ifdef GZIP
#include <zlib.h>
#else
#define gzopen fopen
#define gzclose fclose
#define gzread(a,b,c) fread(b,1,c,a)
#define gzwrite(a,b,c) fwrite(b,1,c,a)
#define gzputc(a,b) fputc(b,a)
#define gzgetc(a) fgetc(a)
typedef FILE* gzFile;
#endif

Config_t config;
pthread_mutex_t support_mutex = PTHREAD_MUTEX_INITIALIZER;

char const StatusFileName[] = DATA_DIR "/status.nv";
const static char StatusTmpFileName[] = DATA_DIR "/status.nv.tmp";
static const char LogFileName[] = "/tmp/saet.log.0";
static const char LogFileName_tmp[] = "/tmp/saet.log.1";
static const char Log2FileName[] = "/tmp/saet.log.2";
static const char Log2FileName_tmp[] = "/tmp/saet.log.3";
static const char LogMMFileName[] = "/tmp/saet.log.4";
static const char LogMMFileName_tmp[] = "/tmp/saet.log.5";

const char DevGpioA[] = "/dev/gpioa";

struct support_numbers_array support_called_numbers[CALLED_NUMBERS] = {{0, NULL, 0}, };
int support_called_number_idx = 0;

struct support_numbers_array support_received_numbers[CALLED_NUMBERS] = {{0, NULL, 0}, };
int support_received_number_idx = 0;

pthread_t debug_pid[DEBUG_PID_MAX];
int debug_mloop_phase = 0;

/***************************************/
/* Export:                             */
/* Registers a call                    */
/***************************************/

int support_called_number(char *number)
{
  if(!number) return -1;
  
  support_called_numbers[support_called_number_idx].time = time(NULL);
  if(support_called_numbers[support_called_number_idx].number)
    free(support_called_numbers[support_called_number_idx].number);
  support_called_numbers[support_called_number_idx].number = strdup(number);
  support_called_numbers[support_called_number_idx].success = 0;
  support_called_number_idx++;
  support_called_number_idx %= CALLED_NUMBERS;
  
  return 0;
}

void support_called_number_success()
{
  if(support_called_number_idx > 0)
    support_called_numbers[(support_called_number_idx-1)%CALLED_NUMBERS].success = 1;
}

int support_received_number(char *number)
{
  if(!number) return -1;
  
  support_received_numbers[support_received_number_idx].time = time(NULL);
  if(support_received_numbers[support_received_number_idx].number)
    free(support_received_numbers[support_received_number_idx].number);
  support_received_numbers[support_received_number_idx].number = strdup(number);
  support_received_numbers[support_received_number_idx].success = 0;
  support_received_number_idx++;
  support_received_number_idx %= CALLED_NUMBERS;
  
  return 0;
}

/***************************************/
/* Export:                             */
/* Stores phone numbers for modems     */
/***************************************/

int PhoneBook_ins(int pbook_number, char *pbook_name, char *n_to_store, char flags)
{
  if((pbook_number < 1) || (pbook_number > PBOOK_DIM) || (strlen(n_to_store) > 23)) return -1;
  
  strcpy(config.PhoneBook[pbook_number-1].Phone, n_to_store);
  free(config.PhoneBook[pbook_number-1].Name);
  config.PhoneBook[pbook_number-1].Name = strdup(pbook_name);
  config.PhoneBook[pbook_number-1].Abil = flags;
  
  return 0;
}

int PhoneBook_find(char *number)
{
  int i;
  char num1[32], num2[32];
  
  if(number[0] == '+')
    num1[0] = 0;
  else
    strcpy(num1, config.PhonePrefix);
    
  strcat(num1, number);
  
  for(i=0; i<PBOOK_DIM; i++)
  {
    if(config.PhoneBook[i].Phone[0] == '+')
      num2[0] = 0;
    else
      strcpy(num2, config.PhonePrefix);
    
    strcat(num2, config.PhoneBook[i].Phone);

    if(!strcmp(num1, num2)) return i;
  }

  support_log("Phonebook: Not found.");

  return -1;
}

char* support_delim(char *str)
{
  static char *tstr = NULL;
  int i;
  char *ret;
  
  if(str) tstr = str;
  if(!tstr) return NULL;
  
  ret = tstr;
  
  for(i=0; tstr[i] && tstr[i]!=';'; i++);
  if(!tstr[i])
    tstr = NULL;
  else
  {
    tstr[i] = 0;
    tstr = tstr + i + 1;
  }
  
  return ret;
}

int support_find_serial_consumer(int type, int start)
{
  int i;
  
  type = PROT_CLASS(type);
  
  for(i=start; i<MAX_NUM_CONSUMER; i++)
    if(config.consumer[i].configured &&
      (config.consumer[i].configured != 5) &&
       PROT_CLASS(config.consumer[i].type) == type)
      return i;
      
  return -1;
}

struct support_list* support_list_add_prev(struct support_list *list, char *cmd, int timeout, int param)
{
  struct support_list *tmp;

  tmp = (struct support_list*)malloc(sizeof(struct support_list));
  tmp->cmd = strdup(cmd);
  tmp->timeout = timeout;
  tmp->param = param;
  tmp->next = list;
  
  return tmp;
}

struct support_list* support_list_add_next(struct support_list *list, char *cmd, int timeout, int param)
{
  struct support_list *tmp;

  tmp = (struct support_list*)malloc(sizeof(struct support_list));
  if(!strncmp(cmd, "AT+CMGS", 7))
    tmp->cmd = cmd;
  else
    tmp->cmd = strdup(cmd);
  tmp->timeout = timeout;
  tmp->param = param;
  
  if(list)
  {
    tmp->next = list->next;
    list->next = tmp;
  }
  else
    tmp->next = NULL;
  
  return tmp;
}

struct support_list* support_list_add(struct support_list *list, char *cmd, int timeout, int param)
{
  struct support_list *tmp;

  tmp = (struct support_list*)malloc(sizeof(struct support_list));
  tmp->cmd = cmd;
  tmp->timeout = timeout;
  tmp->param = param;
  tmp->next = list;
  
  return tmp;
}

struct support_list* support_list_del(struct support_list *list)
{
  struct support_list *tmp;

  if(list)
  {
    tmp = list->next;
    free(list->cmd);
    free(list);
    list = tmp;
  }
  
  return list;
}

static void (*support_log_hook_func)(char*) = NULL;

void support_log_hook(void (*func)(char*))
{
  support_log_hook_func = func;
}

void support_log(char *log)
{
  FILE *fp;
  struct stat st;
  
  if(!config.Log) return;
  
  pthread_mutex_lock(&support_mutex);
  
  if(!stat(ADDROOTDIR(LogFileName), &st) && (st.st_size > (config.LogLimit/2)))
    rename(ADDROOTDIR(LogFileName), ADDROOTDIR(LogFileName_tmp));
  
  fp = fopen(ADDROOTDIR(LogFileName), "a");
  fprintf(fp, "%02d/%02d/%02d %02d:%02d:%02d - %s\n", GIORNO, MESE, ANNO, ORE, MINUTI, SECONDI, log);
  fclose(fp);
  if(support_log_hook_func) support_log_hook_func(log);
  pthread_mutex_unlock(&support_mutex);
}

void support_log2(char *log)
{
  FILE *fp;
  struct stat st;
  
  if(!config.Log) return;
  
  pthread_mutex_lock(&support_mutex);
  
  if(!stat(ADDROOTDIR(Log2FileName), &st) && (st.st_size > (config.LogLimit/2)))
    rename(ADDROOTDIR(Log2FileName), ADDROOTDIR(Log2FileName_tmp));
  
  fp = fopen(ADDROOTDIR(Log2FileName), "a");
  fprintf(fp, "%02d/%02d/%02d %02d:%02d:%02d - %s\n", GIORNO, MESE, ANNO, ORE, MINUTI, SECONDI, log);
  fclose(fp);
  pthread_mutex_unlock(&support_mutex);
}

void support_logmm(char *log)
{
  FILE *fp;
  struct stat st;
  
  if(!config.Log) return;
  
  pthread_mutex_lock(&support_mutex);
  
  if(!stat(ADDROOTDIR(LogMMFileName), &st) && (st.st_size > config.LogLimit))
    rename(ADDROOTDIR(LogMMFileName), ADDROOTDIR(LogMMFileName_tmp));
  
  fp = fopen(ADDROOTDIR(LogMMFileName), "a");
  fprintf(fp, "%02d/%02d/%02d %02d:%02d:%02d - %s\n", GIORNO, MESE, ANNO, ORE, MINUTI, SECONDI, log);
  fclose(fp);
  pthread_mutex_unlock(&support_mutex);
}

void support_save_last_log()
{
  system("rm -rf /tmp/saet.last");
  system("mkdir /tmp/saet.last");
  system("cp /tmp/saet.* /tmp/tebe.* /tmp/saet.last");
}

static char old_TMD_Status;
static char old_GSM_Status;
static unsigned char old_ZONA[1 + n_ZS + n_ZI];
static unsigned char old_ZONAEX[1 + n_ZS + n_ZI];
static unsigned int old_SE[n_SE];
static unsigned int old_AT[n_AT];

static int store_Tebe_OOS = 1;

#define SEmask	(bitOOS|bitNoAlarm|bitMUNoAlarm|bitMUNoSabotage|bitMUNoFailure)
#define SEmaskTebe	(bitNoAlarm|bitMUNoAlarm|bitMUNoSabotage|bitMUNoFailure)

void support_store_current_values()
{
  int i;
  
  old_TMD_Status = TMD_Status[2];
  old_GSM_Status = GSM_Status[2];

  database_lock();
  memcpy(old_ZONA, ZONA, 1 + n_ZS + n_ZI);
  memcpy(old_ZONAEX, ZONAEX, 1 + n_ZS + n_ZI);
  for(i=0; i<n_SE; i++)
    if(!store_Tebe_OOS && (MMPresente[i>>8] == MM_LARA))
      old_SE[i] = SE[i] & SEmaskTebe;
    else
      old_SE[i] = SE[i];
  memcpy(old_AT, (unsigned int*)AT, sizeof(old_AT));
  database_unlock();
}

static int support_check_status()
{
  int i;

  if((old_TMD_Status != TMD_Status[2]) || (old_GSM_Status != GSM_Status[2])) return 1;
  
  for(i=0; i<(1 + n_ZS + n_ZI); i++)
  {
    if((old_ZONA[i] & (bitActive | bitLockActive)) != (ZONA[i] & (bitActive | bitLockActive)))
    {
//     printf("Fallito 1.\n");
     return 1;
    }
    if((old_ZONAEX[i] & bitLockDisactive) != (ZONAEX[i] & bitLockDisactive))
    {
     return 1;
    }
  }
  for(i=0; i<n_SE; i++)
  {    
    if(!store_Tebe_OOS && (MMPresente[i>>8] == MM_LARA))
    {
      if((old_SE[i] & SEmaskTebe) != (SE[i] & SEmaskTebe))
      {
//       printf("Fallito 2 (%d: %02x %02x).\n", i, old_SE[i], SE[i]);
       return 1;
      }
    }
    else
    {
      /* Non salvo lo stato di fuori servizio sensore per i terminali Tebe */
      if((old_SE[i] & SEmask) != (SE[i] & SEmask))
      {
//       printf("Fallito 2 (%d: %02x %02x).\n", i, old_SE[i], SE[i]);
       return 1;
      }
    }
  }
  for(i=0; i<n_AT; i++)
    if((old_AT[i] & bitOOS) != (AT[i] & bitOOS))
    {
//     printf("Fallito 3. %04x %04x\n", old_AT[i], AT[i]);
     return 1;
    }
  
  return 0;
}

void support_save_status()
{
  gzFile fp;
  
  if(!support_check_status()) return;

  fp = gzopen(ADDROOTDIR(StatusTmpFileName), "w");
  if(!fp) return;

#if 0
  gzputc(fp, TMD_Status[2]);
  gzputc(fp, GSM_Status[2]);
  gzwrite(fp, ZONA, 1 + n_ZS + n_ZI);
  gzwrite(fp, SE, sizeof(SE));
  gzwrite(fp, AT, sizeof(AT));
  gzwrite(fp, ZONAEX, 1 + n_ZS + n_ZI);
  
  gzclose(fp);
  
  rename(StatusTmpFileName, StatusFileName);
  
  support_store_current_values();
#else
  support_store_current_values();
  
  gzputc(fp, old_TMD_Status);
  gzputc(fp, old_GSM_Status);
  gzwrite(fp, old_ZONA, 1 + n_ZS + n_ZI);
  gzwrite(fp, old_SE, sizeof(SE));
  gzwrite(fp, old_AT, sizeof(AT));
  gzwrite(fp, old_ZONAEX, 1 + n_ZS + n_ZI);
  
  gzclose(fp);
  
  rename(StatusTmpFileName, StatusFileName);
#endif
}

int support_load_status()
{
  gzFile fp;
  int i, x;
  
  fp = gzopen(ADDROOTDIR(StatusFileName), "r");
  if(!fp) return -1;
  
  TMD_Status[2] = gzgetc(fp);
  GSM_Status[2] = gzgetc(fp);
  
  gzread(fp, ZONA, 1 + n_ZS + n_ZI);
  for(i=0; i<(1 + n_ZS + n_ZI); i++)
  {
    x = ZONA[i] & (bitActive | bitLockActive);
    old_ZONA[i] = x;
    ZONA[i] = x;
  }
  
  gzread(fp, SE, sizeof(SE));
  for(i=0; i<n_SE; i++)
  {
    x = SE[i] & SEmask;
    old_SE[i] = x;
    SE[i] = x;
  }
  
  gzread(fp, AT, sizeof(AT));
  for(i=0; i<n_AT; i++)
  {
    x = AT[i] & bitOOS;
    old_AT[i] = x;
    AT[i]=x;
  }
  
  gzread(fp, ZONAEX, 1 + n_ZS + n_ZI);
  for(i=0; i<(1 + n_ZS + n_ZI); i++)
  {
    x = ZONAEX[i] & bitLockDisactive;
    old_ZONAEX[i] = x;
    ZONAEX[i] = x;
  }
  
  gzclose(fp);
  return 0;
}

int support_reset_status()
{
  unlink(ADDROOTDIR(StatusTmpFileName));
  return unlink(ADDROOTDIR(StatusFileName));
}

int support_baud(int baud)
{
  switch(baud)
  {
    case 600: return B600;
    case 1200: return B1200;
    case 2400: return B2400;
    case 4800: return B4800;
    case 9600: return B9600;
    case 19200: return B19200;
    case 38400: return B38400;
    case 57600: return B57600;
    case 115200: return B115200;
    default: return 0;
  }
}

int support_get_board()
{
  FILE *fp;
  char buf[64];
  
  fp = fopen(ADDROOTDIR("/etc/board"), "r");
  if(!fp) return BOARD_ANUBI;
  fgets(buf, 64, fp);
  fclose(fp);
  
  if(!strncmp(buf, "Anubi", 5))
    return BOARD_ANUBI;
  else if(!strncmp(buf, "Iside", 5))
    return BOARD_ISIDE;
  else if(!strncmp(buf, "Athena", 6))
    return BOARD_ATHENA;
  else 
    return BOARD_UNKNOWN;
}

void utf8convert(char *in, char *out)
{
  while(*in)
  {
    if(((*in & 0xfc) == 0xc0) && ((*(in+1) & 0xc0) == 0x80))
    {
      *out++ = (*in << 6) + (*(in+1) & 0x3f);
      in += 2;
    }
    else
    {
      *out++ = *in++;
    }
  }
  
  *out++ = 0;
}

void Tebe_OOS_volatile()
{
  store_Tebe_OOS = 0;
}

unsigned int uchartoint(unsigned char *data)
{
  return data[0]|(data[1]<<8)|(data[2]<<16)|(data[3]<<24);
}

unsigned short uchartoshort(unsigned char *data)
{
  return data[0]|(data[1]<<8);
}

void inttouchar(int num, unsigned char *data, int size)
{
  memcpy(data, &num, size);
}

int support_getpid()
{
  /* A differenza delle schede Iside, sulla scheda Athena i thread vengono
     raggruppati sotto un unico PID, quello del genitore. Occorre quindi
     recuperare non il PID ma il TID (che su Iside si equivalgono), ma
     occorre farlo tramite system call. */
  return syscall(SYS_gettid);
}

