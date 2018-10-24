#include "delphi.h"
#include "protocol.h"
#include "timeout.h"
#include "support.h"
#include "database.h"
#include "command.h"
#include "strings.h"
#include "console/console.h"
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>

//#define TIPO_MIRUS

static char *str_18_0;
static char *str_18_1;
static char *str_18_2;
static char *str_18_3;
static char *str_18_4;
static char *str_18_5;
static char *str_18_6;
static char *str_18_7;
static char *str_18_8;
static char *str_18_9;
static char *str_18_10;
static char *str_18_13;
static char *str_18_14;
static char *str_18_15;
static char *str_18_16;
static char *str_18_17;
static char *str_18_18;
static char *str_18_19;
static char *str_18_20;
static char *str_18_21;
static char *str_18_22;
static char *str_18_26;
static char *str_18_27;
static char *str_18_30;
static char *str_18_31;
static char *str_18_32;
static char *str_18_33;
static char *str_18_34;
static char *str_18_35;
static char *str_18_42;
static char *str_18_38;
static char *str_18_39;
static char *str_18_40;
static char *str_18_43;
static char *str_18_44;
static char *str_18_45;
static char *str_18_46;
static char *str_18_47;
static char *str_18_48;
static char *str_18_49;
static char *str_18_50;
static char *str_18_51;
static char *str_18_52;
static char *str_18_53;

static char *str1_18_0="ATN0104Configurazione\r";
static char *str1_18_1="ATN0105attiva su altra\r";
static char *str1_18_2="ATN0406console.\r";
static char *str1_18_3="ATN0504Inserire\r";
static char *str1_18_4="ATN0605chiave\r";
static char *str1_18_5="ATN0104Chiave presente\r";
static char *str1_18_6="ATN0404Anagrafica\r";
static char *str1_18_7="ATN0505completa\r";
static char *str1_18_8="ATN0104Caricata chiave\r";
static char *str1_18_9="ATN0405numero %d\r";
static char *str1_18_10="CARICA CHIAVE";
static char *str1_18_13="ATN0504Inserire\r";
static char *str1_18_14="ATN0605chiave\r";
static char *str1_18_15="ATN0306Modo zona 2:\r";
static char *str1_18_16="Impedita";
static char *str1_18_17="Allarme";
static char *str1_18_18="Fuori Servizio";
static char *str1_18_19="ATN0606Zona 2:\r";
static char *str1_18_20="Nessuna";
static char *str1_18_21="ZONA %s";
static char *str1_18_22="ATN0306Modo zona 1:\r";
static char *str1_18_26="ATN0204Inseritore n.%d\r";
static char *str1_18_27="ATN0606Zona 1:\r";
static char *str1_18_30="ATN0404Chiave non\r";
static char *str1_18_31="ATN0605valida\r";
static char *str1_18_32="ATN0303Chiave n.%02d\r";
static char *str1_18_33="ASSOCIA ZONE";
static char *str1_18_34="ATN0404Chiave da\r";
static char *str1_18_35="ATN0405associare\r";
static char *str1_18_42="ATN0405annullare\r";
static char *str1_18_38="ATN0404Chiave %d\r";
static char *str1_18_39="ATN0405annullata\r";
static char *str1_18_40="ANNULLA CHIAVE";
static char *str1_18_43="ATN0504CHIAVI\r";
static char *str1_18_44="ATN0405AZZERATE\r";
static char *str1_18_45="AZZERA CHIAVI";
static char *str1_18_46="ATN0405AZZERARE\r";
static char *str1_18_47="ATN0506CHIAVI?\r";
static char *str1_18_48="CARICA CHIAVE   ";
static char *str1_18_49="ASSOCIA ZONE    ";
static char *str1_18_50="ANNULLA CHIAVE  ";
static char *str1_18_51="AZZERA CHIAVI   ";
static char *str1_18_52="Inseritore n. %d";
static char *str1_18_53="ATG0707\r";

static char *str2_18_0="ATN064041Configurazione\r";
static char *str2_18_1="ATN064051attiva su altra\r";
static char *str2_18_2="ATN064061console.\r";
static char *str2_18_3="ATN064041Inserire\r";
static char *str2_18_4="ATN064051chiave\r";
static char *str2_18_5="ATN064041Chiave presente\r";
static char *str2_18_6="ATN064041Anagrafica\r";
static char *str2_18_7="ATN064051completa\r";
static char *str2_18_8="ATN064041Caricata chiave\r";
static char *str2_18_9="ATN064051numero %d\r";
static char *str2_18_10="CARICA CHIAVE";
static char *str2_18_13="ATN064041Inserire\r";
static char *str2_18_14="ATN064051chiave\r";
static char *str2_18_15="ATN064061Modo zona 2:\r";
static char *str2_18_16="Impedita";
static char *str2_18_17="Allarme";
static char *str2_18_18="Fuori Servizio";
static char *str2_18_19="ATN064061Zona 2:\r";
static char *str2_18_20="Nessuna";
static char *str2_18_21="ZONA %s";
static char *str2_18_22="ATN064061Modo zona 1:\r";
static char *str2_18_26="ATN064041Inseritore n.%d\r";
static char *str2_18_27="ATN064061Zona 1:\r";
static char *str2_18_30="ATN064041Chiave non\r";
static char *str2_18_31="ATN064051valida\r";
static char *str2_18_32="ATN064031Chiave n.%02d\r";
static char *str2_18_33="ASSOCIA ZONE";
static char *str2_18_34="ATN064041Chiave da\r";
static char *str2_18_35="ATN064051associare\r";
static char *str2_18_42="ATN064051annullare\r";
static char *str2_18_38="ATN064041Chiave %d\r";
static char *str2_18_39="ATN064051annullata\r";
static char *str2_18_40="ANNULLA CHIAVE";
static char *str2_18_43="ATN064041CHIAVI\r";
static char *str2_18_44="ATN064051AZZERATE\r";
static char *str2_18_45="AZZERA CHIAVI";
static char *str2_18_46="ATN064051AZZERARE\r";
static char *str2_18_47="ATN064061CHIAVI?\r";
static char *str2_18_48="CARICA CHIAVE";
static char *str2_18_49="ASSOCIA ZONE";
static char *str2_18_50="ANNULLA CHIAVE";
static char *str2_18_51="AZZERA CHIAVI";
static char *str2_18_52="Inseritore n. %d";
static char *str2_18_53="ATG05807\r";

static char **tipo_str[] = {
  &str_18_0, &str_18_1, &str_18_2, &str_18_3, &str_18_4,
  &str_18_5, &str_18_6, &str_18_7, &str_18_8, &str_18_9,
  &str_18_10, &str_18_13, &str_18_14, &str_18_15, &str_18_16,
  &str_18_17, &str_18_18, &str_18_19, &str_18_20, &str_18_21,
  &str_18_22, &str_18_26, &str_18_27, &str_18_30, &str_18_31,
  &str_18_32, &str_18_33, &str_18_34, &str_18_35, &str_18_42,
  &str_18_38, &str_18_39, &str_18_40, &str_18_43, &str_18_44,
  &str_18_45, &str_18_46, &str_18_47, &str_18_48, &str_18_49,
  &str_18_50, &str_18_51, &str_18_52, &str_18_53,
};

static char **tipo_str1[] = {
  &str1_18_0, &str1_18_1, &str1_18_2, &str1_18_3, &str1_18_4,
  &str1_18_5, &str1_18_6, &str1_18_7, &str1_18_8, &str1_18_9,
  &str1_18_10, &str1_18_13, &str1_18_14, &str1_18_15, &str1_18_16,
  &str1_18_17, &str1_18_18, &str1_18_19, &str1_18_20, &str1_18_21,
  &str1_18_22, &str1_18_26, &str1_18_27, &str1_18_30, &str1_18_31,
  &str1_18_32, &str1_18_33, &str1_18_34, &str1_18_35, &str1_18_42,
  &str1_18_38, &str1_18_39, &str1_18_40, &str1_18_43, &str1_18_44,
  &str1_18_45, &str1_18_46, &str1_18_47, &str1_18_48, &str1_18_49,
  &str1_18_50, &str1_18_51, &str1_18_52, &str1_18_53,
};

static char **tipo_str2[] = {
  &str2_18_0, &str2_18_1, &str2_18_2, &str2_18_3, &str2_18_4,
  &str2_18_5, &str2_18_6, &str2_18_7, &str2_18_8, &str2_18_9,
  &str2_18_10, &str2_18_13, &str2_18_14, &str2_18_15, &str2_18_16,
  &str2_18_17, &str2_18_18, &str2_18_19, &str2_18_20, &str2_18_21,
  &str2_18_22, &str2_18_26, &str2_18_27, &str2_18_30, &str2_18_31,
  &str2_18_32, &str2_18_33, &str2_18_34, &str2_18_35, &str2_18_42,
  &str2_18_38, &str2_18_39, &str2_18_40, &str2_18_43, &str2_18_44,
  &str2_18_45, &str2_18_46, &str2_18_47, &str2_18_48, &str2_18_49,
  &str2_18_50, &str2_18_51, &str2_18_52, &str2_18_53,
};

#define TIPO_NUM_CHIAVI		64
#define TIPO_NUM_INSERITORI	 8

#define TIPO_TIMEOUT_INSERITORE  70
#define TIPO_TIMEOUT_ATTIVAZIONE 40

typedef struct {
  unsigned char valida;
  unsigned char	code[5];
  unsigned char	zona[TIPO_NUM_INSERITORI][2];
  unsigned char	modo[TIPO_NUM_INSERITORI][2];
} tipo_chiave;

typedef struct {
  unsigned char valida;
  unsigned char	code[5];
  unsigned char	zona[TIPO_NUM_INSERITORI][2];
} tipo_chiave_old;

typedef struct {
  int chiave;
  int statozone;
  timeout_t *to;
  timeout_t *ab;
  timeout_t *toz;
} tipo_inseritore;

typedef struct {
  int modo;
  tipo_chiave chiave[TIPO_NUM_CHIAVI];
  tipo_inseritore inseritore[TIPO_NUM_INSERITORI];
} tipo_data;

static tipo_data *tipo_d;
//#define TIPO ((tipo_data*)((dev->prot_data)))
#define TIPO tipo_d

static const char *tipoFileName = DATA_DIR "/tipo.nv";
static unsigned char tipo_led[TIPO_NUM_INSERITORI] = {0x51, 0x52, 0x54, 0x57, 0x58, 0x5b, 0x5d, 0x5e};
//static unsigned char tipo_led[TIPO_NUM_INSERITORI] = {0x51, 0x53, 0x55, 0x57, 0x59, 0x5b, 0x5d, 0x5f};
static unsigned char tipo_addr[TIPO_NUM_INSERITORI] = {0x10, 0x01, 0x02, 0x13, 0x04, 0x15, 0x16, 0x7};
static int tipo_configurazione = 0;
static ProtDevice *tipo_console = NULL;
static pthread_mutex_t tipo_mutex = PTHREAD_MUTEX_INITIALIZER;
static ListItem *tipo_lista_inseritori = NULL;
static int tipo_save = 0;
static int tipo_saving = 0;
static int tipo_modo = 0;

static int tipo_cerca_chiave(unsigned char *key);
static void tipo_aggiorna_led();

/* La gestione del menu' sul TerminalData viene fatta tutta nel plugin. */
/* Il device e' la console */

static CALLBACK void* tipo_console_logout(ProtDevice *dev, int press, void *null)
{
  if(press)
  {
    timeout_off(CONSOLE->timeout);
    tipo_console = NULL;
    tipo_configurazione = 0;
    pthread_mutex_unlock(&tipo_mutex);
    CONSOLE->callback[KEY_TIMEOUT] = NULL;
    CONSOLE->callback_arg[KEY_TIMEOUT] = NULL;
    console_send(dev, "ATF0\r");
    console_send(dev, "ATL000\r");
  }
  
  return console_logout(dev, press, NULL);
}

static void tipo_return_to_menu(char *dev, int null2)
{
  console_show_menu((ProtDevice*)dev, 0, NULL);
}

static int tipo_check_configurazione(ProtDevice *dev)
{
  if(pthread_mutex_trylock(&tipo_mutex))
  {
    console_send(dev, str_18_0);
    console_send(dev, str_18_1);
    console_send(dev, str_18_2);
    timeout_on(CONSOLE->timeout, (timeout_func)tipo_return_to_menu, dev, 0, TIMEOUT_MSG);
    return 0;
  }
  
  tipo_console = dev;
  
  return 1;
}

static CALLBACK void* tipo_console_exit(ProtDevice *dev, int press, void *null)
{
  timeout_off(CONSOLE->timeout);
  tipo_console = NULL;
  tipo_configurazione = 0;
  pthread_mutex_unlock(&tipo_mutex);
  CONSOLE->callback[KEY_TIMEOUT] = NULL;
  CONSOLE->callback_arg[KEY_TIMEOUT] = NULL;
  return console_show_menu(dev, 0, NULL);
}

/****************/
/* menu' CARICA */
/****************/

static void tipo_console_carica_chiave_timeout(char *dev, int null2)
{
  console_send((ProtDevice*)dev, "ATS04\r");
  console_send((ProtDevice*)dev, str_18_3);
  console_send((ProtDevice*)dev, str_18_4);
}

static void tipo_console_carica_chiave(unsigned char *chiave)
{
  ProtDevice *dev = tipo_console;
  char cmd[32];
  int n;
  
  console_send(dev, "ATS04\r");
  sprintf(cmd, "ATE%04d\r", console_login_timeout_time/10);
  console_send(dev, cmd);
  
  n = tipo_cerca_chiave(chiave);
  if(n >= 0)
  {
    console_send(dev, str_18_5);
    timeout_on(CONSOLE->timeout, (timeout_func)tipo_console_carica_chiave_timeout, dev, 0, TIMEOUT_MSG);
    return;
  }
  
  for(n=0; (n<TIPO_NUM_CHIAVI) && TIPO->chiave[n].valida; n++);
  if(n == TIPO_NUM_CHIAVI)
  {
    console_send(dev, str_18_6);
    console_send(dev, str_18_7);
    timeout_on(CONSOLE->timeout, (timeout_func)tipo_console_carica_chiave_timeout, dev, 0, TIMEOUT_MSG);
    return;
  }
  
  TIPO->chiave[n].valida = 1;
  memcpy(TIPO->chiave[n].code, chiave, 5);
  memset(TIPO->chiave[n].zona, 0xff, TIPO_NUM_INSERITORI*2);
  memset(TIPO->chiave[n].modo, 1, TIPO_NUM_INSERITORI*2);
  tipo_save = 1;
  
  console_send(dev, str_18_8);
  sprintf(cmd, str_18_9, n+1);
  console_send(dev, cmd);
  timeout_on(CONSOLE->timeout, (timeout_func)tipo_console_carica_chiave_timeout, dev, 0, TIMEOUT_MSG);
}

static CALLBACK void* tipo_console_carica(ProtDevice *dev, int press, void *null)
{
  int n;
  
  console_disable_menu(dev, str_18_10);
  
  if(!tipo_check_configurazione(dev)) return NULL;
  
  for(n=0; (n<TIPO_NUM_CHIAVI) && TIPO->chiave[n].valida; n++);
  if(n == TIPO_NUM_CHIAVI)
  {
    console_send(dev, str_18_6);
    console_send(dev, str_18_7);
    tipo_console = NULL;
    tipo_configurazione = 0;
    pthread_mutex_unlock(&tipo_mutex);
    timeout_on(CONSOLE->timeout, (timeout_func)tipo_return_to_menu, dev, 0, TIMEOUT_MSG);
    return NULL;
  }
  
  tipo_configurazione = 1;
  
  console_send(dev, str_18_13);
  console_send(dev, str_18_14);
  
  console_register_callback(dev, KEY_CANCEL, tipo_console_exit, NULL);
  console_register_callback(dev, KEY_TIMEOUT, tipo_console_exit, NULL);
  console_register_callback(dev, KEY_DIERESIS, tipo_console_logout, NULL);
  return NULL;
}

/*****************/
/* menu' ASSOCIA */
/*****************/

static CALLBACK void* tipo_console_associa(ProtDevice *dev, int press, void *null);

static CALLBACK void* tipo_console_associa_exit(ProtDevice *dev, int press, void *null)
{
  pthread_mutex_unlock(&tipo_mutex);
  return tipo_console_associa(dev, press, NULL);
}

static CALLBACK void* tipo_console_associa_z2m(ProtDevice *dev, int press, void *modo)
{
  CONSOLE->temp_string[(TIPO_NUM_INSERITORI*2)+1] = (int)modo;
  tipo_aggiorna_led();
  tipo_save = 1;
  return tipo_console_associa_exit(dev, press, NULL);
}

static CALLBACK void* tipo_console_associa_z2(ProtDevice *dev, int press, void *zona)
{
  CONSOLE->temp_string[1] = (int)zona;
  tipo_aggiorna_led();
  tipo_save = 1;
  
  if((int)zona == 0xff) return tipo_console_associa_z2m(dev, 0, (void*)1);
  
  console_send(dev, "ATS06\r");
  console_send(dev, str_18_15);
  
  console_list_free(CONSOLE->support_list);
  CONSOLE->support_list = NULL;
  
  CONSOLE->support_list = console_list_add(CONSOLE->support_list, str_18_16, NULL, tipo_console_associa_z2m, 0);
  CONSOLE->support_list = console_list_add(CONSOLE->support_list, str_18_17, NULL, tipo_console_associa_z2m, 1);
  CONSOLE->support_list = console_list_add(CONSOLE->support_list, str_18_18, NULL, tipo_console_associa_z2m, 2);
  
  CONSOLE->list_show_cancel = tipo_console_associa_exit;
  console_list_show(dev, CONSOLE->support_list, ((unsigned char*)CONSOLE->temp_string)[(TIPO_NUM_INSERITORI*2)+1], 2, 0);
  
  return NULL;
}

static CALLBACK void* tipo_console_associa_z1m(ProtDevice *dev, int press, void *modo)
{
  int i, idx, pos;
  char zone_to_show[1 + n_ZS + n_ZI];
  char cmd[32];
  char *zone_cod, *zone_desc;
  
  CONSOLE->temp_string[TIPO_NUM_INSERITORI*2] = (int)modo;
  tipo_save = 1;
  
  console_send(dev, "ATS06\r");
  console_send(dev, str_18_19);
  
  console_list_free(CONSOLE->support_list);
  CONSOLE->support_list = NULL;
  
  memset(zone_to_show, 0, 1 + n_ZS + n_ZI);
  for(i=0; i<n_SE; i++)
    if(Zona_SE[i] < (1 + n_ZS + n_ZI)) zone_to_show[Zona_SE[i]] = 1;
  if(TPDZT && (TPDZT[0]!=0xff)) zone_to_show[0] = 1;
  for(i=0; i<n_ZI; i++)
    if(TPDZI[i] && (TPDZI[i][0]!=0xff)) zone_to_show[n_ZS+1+i] = 1;
  
  pos = 0;
  idx = 1;
  
  CONSOLE->support_list = console_list_add(CONSOLE->support_list, str_18_20, "", tipo_console_associa_z2, 0xff);
  
//  if((int)zona != 0xff)
    for(i=0; i<(1 + n_ZS + n_ZI); i++)
      if(zone_to_show[i])
      {
        string_zone_name(i, &zone_cod, &zone_desc);
        sprintf(cmd, str_18_21, zone_cod);
        CONSOLE->support_list = console_list_add(CONSOLE->support_list, cmd, zone_desc, tipo_console_associa_z2, i);
        if(((unsigned char*)CONSOLE->temp_string)[1] == i) pos = idx;
        idx++;
      }
  
  CONSOLE->list_show_cancel = tipo_console_associa_exit;
  console_list_show(dev, CONSOLE->support_list, pos, 1, 0);
  
  return NULL;
}

static CALLBACK void* tipo_console_associa_z1(ProtDevice *dev, int press, void *zona)
{
  CONSOLE->temp_string[0] = (int)zona;
  tipo_aggiorna_led();
  tipo_save = 1;
  
  if((int)zona == 0xff) return tipo_console_associa_z1m(dev, 0, (void*)1);
  
  console_send(dev, "ATS06\r");
  console_send(dev, str_18_22);
  
  console_list_free(CONSOLE->support_list);
  CONSOLE->support_list = NULL;
  
  CONSOLE->support_list = console_list_add(CONSOLE->support_list, str_18_16, NULL, tipo_console_associa_z1m, 0);
  CONSOLE->support_list = console_list_add(CONSOLE->support_list, str_18_17, NULL, tipo_console_associa_z1m, 1);
  CONSOLE->support_list = console_list_add(CONSOLE->support_list, str_18_18, NULL, tipo_console_associa_z1m, 2);
  
  CONSOLE->list_show_cancel = tipo_console_associa_exit;
  console_list_show(dev, CONSOLE->support_list, ((unsigned char*)CONSOLE->temp_string)[TIPO_NUM_INSERITORI*2], 2, 0);
  
  return NULL;
}

static CALLBACK void* tipo_console_associa2(ProtDevice *dev, int press, void *inseritore)
{
  int i, idx, pos;
  char zone_to_show[1 + n_ZS + n_ZI];
  char cmd[32];
  char *zone_cod, *zone_desc;
  
  console_send(dev, "ATS04\r");
  sprintf(cmd, str_18_26, (int)inseritore);
  console_send(dev, cmd);
  console_send(dev, str_18_27);
  
  console_list_free(CONSOLE->support_list);
  CONSOLE->support_list = NULL;
  
  memset(zone_to_show, 0, 1 + n_ZS + n_ZI);
  for(i=0; i<n_SE; i++)
    if(Zona_SE[i] < (1 + n_ZS + n_ZI)) zone_to_show[Zona_SE[i]] = 1;
  if(TPDZT && (TPDZT[0]!=0xff)) zone_to_show[0] = 1;
  for(i=0; i<n_ZI; i++)
    if(TPDZI[i] && (TPDZI[i][0]!=0xff)) zone_to_show[n_ZS+1+i] = 1;
  
  pos = 0;
  idx = 1;
  
  CONSOLE->temp_string = TIPO->chiave[CONSOLE->support_idx].zona[(int)inseritore];
  CONSOLE->support_list = console_list_add(CONSOLE->support_list, str_18_20, "", tipo_console_associa_z1, 0xff);
  
  for(i=0; i<(1 + n_ZS + n_ZI); i++)
    if(zone_to_show[i])
    {
      string_zone_name(i, &zone_cod, &zone_desc);
      sprintf(cmd, str_18_21, zone_cod);
      CONSOLE->support_list = console_list_add(CONSOLE->support_list, cmd, zone_desc, tipo_console_associa_z1, i);
      if(((unsigned char*)CONSOLE->temp_string)[0] == i) pos = idx;
      idx++;
    }
  
  CONSOLE->list_show_cancel = tipo_console_associa_exit;
  console_list_show(dev, CONSOLE->support_list, pos, 1, 0);
  
  return NULL;
}

static CALLBACK void* tipo_console_associa1(ProtDevice *dev, int key_as_string, void *null)
{
  int key;
  char cmd[32];
  
  console_send(dev, "ATF0\r");
  console_send(dev, "ATL000\r");
  console_send(dev, "ATS04\r");
  
  key = -1;
  sscanf((char*)key_as_string, "%d", &key);
  key--;
  if((key < 0) || (key >= TIPO_NUM_CHIAVI) || !TIPO->chiave[key].valida)
  {
    console_send(dev, str_18_30);
    console_send(dev, str_18_31);
    tipo_console = NULL;
    tipo_configurazione = 0;
    pthread_mutex_unlock(&tipo_mutex);
    CONSOLE->last_menu_item = tipo_console_associa;
    timeout_on(CONSOLE->timeout, (timeout_func)console_return_to_last_menu_item, dev, 0, TIMEOUT_MSG);
    return NULL;
  }
  
  CONSOLE->support_idx = key;
  sprintf(cmd, str_18_32, key+1);
  console_send(dev, cmd);
  
  if(tipo_modo & 0x01)
  {
    CONSOLE->list_show_cancel = tipo_console_associa_exit;
    console_list_show(dev, tipo_lista_inseritori, 0, 4, 0);
  }
  else
    tipo_console_associa2(dev, 0, (void*)0);
  
  console_register_callback(dev, KEY_DIERESIS, tipo_console_logout, NULL);
  
  return NULL;
}

static CALLBACK void* tipo_console_associa(ProtDevice *dev, int press, void *null)
{
  console_disable_menu(dev, str_18_33);
  
  if(!tipo_check_configurazione(dev)) return NULL;
  
  tipo_configurazione = 2;
  
  console_send(dev, str_18_34);
  console_send(dev, str_18_35);
  console_send(dev, str_18_53);
  console_send(dev, "ATF1\r");
  console_send(dev, "ATL111\r");
  
  console_register_callback(dev, KEY_STRING, tipo_console_associa1, NULL);
  console_register_callback(dev, KEY_CANCEL, tipo_console_exit, NULL);
  console_register_callback(dev, KEY_TIMEOUT, tipo_console_exit, NULL);
  console_register_callback(dev, KEY_DIERESIS, tipo_console_logout, NULL);
  return NULL;
}

/*****************/
/* menu' ANNULLA */
/*****************/

static CALLBACK void* tipo_console_annulla1(ProtDevice *dev, int key_as_string, void *null)
{
  int key;
  char cmd[32];
  
  console_send(dev, "ATF0\r");
  console_send(dev, "ATL000\r");
  console_send(dev, "ATS04\r");
  sprintf(cmd, "ATE%04d\r", console_login_timeout_time/10);
  console_send(dev, cmd);
  
  key = -1;
  sscanf((char*)key_as_string, "%d", &key);
  key--;
  if((key < 0) || (key >= TIPO_NUM_CHIAVI) || !TIPO->chiave[key].valida)
  {
    console_send(dev, str_18_30);
    console_send(dev, str_18_31);
    tipo_console = NULL;
    tipo_configurazione = 0;
    pthread_mutex_unlock(&tipo_mutex);
    timeout_on(CONSOLE->timeout, (timeout_func)tipo_return_to_menu, dev, 0, TIMEOUT_MSG);
    return NULL;
  }
  
  TIPO->chiave[key].valida = 0;
  tipo_save = 1;
  
  sprintf(cmd, str_18_38, key+1);
  console_send(dev, cmd);
  console_send(dev, str_18_39);
  tipo_console = NULL;
  tipo_configurazione = 0;
  pthread_mutex_unlock(&tipo_mutex);
//  CONSOLE->last_menu_item = tipo_console_annulla;
//  timeout_on(CONSOLE->timeout, (timeout_func)console_return_to_last_menu_item, dev, 0, TIMEOUT_MSG);
  timeout_on(CONSOLE->timeout, (timeout_func)tipo_return_to_menu, dev, 0, TIMEOUT_MSG);
  
  return NULL;
}

static CALLBACK void* tipo_console_annulla(ProtDevice *dev, int press, void *null)
{
  console_disable_menu(dev, str_18_40);
  
  if(!tipo_check_configurazione(dev)) return NULL;
  
  tipo_configurazione = 3;
  
  console_send(dev, str_18_34);
  console_send(dev, str_18_42);
  console_send(dev, str_18_53);
  console_send(dev, "ATF1\r");
  console_send(dev, "ATL111\r");
  console_register_callback(dev, KEY_STRING, tipo_console_annulla1, NULL);
  console_register_callback(dev, KEY_CANCEL, tipo_console_exit, NULL);
  console_register_callback(dev, KEY_TIMEOUT, tipo_console_exit, NULL);
  console_register_callback(dev, KEY_DIERESIS, tipo_console_logout, NULL);
  return NULL;
}

/****************/
/* menu' AZZERA */
/****************/

static CALLBACK void* tipo_console_azzera1(ProtDevice *dev, int press, void *null)
{
  int i;
  
  console_send(dev, "ATS00\r");
  console_send(dev, str_18_43);
  console_send(dev, str_18_44);
  for(i=0; i<TIPO_NUM_CHIAVI; i++) TIPO->chiave[i].valida = 0;
  unlink(tipoFileName);
  tipo_console = NULL;
  pthread_mutex_unlock(&tipo_mutex);
  timeout_on(CONSOLE->timeout, (timeout_func)tipo_return_to_menu, dev, 0, TIMEOUT_MSG);
  return NULL;
}

static CALLBACK void* tipo_console_azzera(ProtDevice *dev, int press, void *null)
{
  console_disable_menu(dev, str_18_45);
  
  if(!tipo_check_configurazione(dev)) return NULL;
  
  console_send(dev, str_18_46);
  console_send(dev, str_18_47);
  console_register_callback(dev, KEY_ENTER, tipo_console_azzera1, NULL);
  console_register_callback(dev, KEY_CANCEL, tipo_console_exit, NULL);
  console_register_callback(dev, KEY_TIMEOUT, tipo_console_exit, NULL);
  console_register_callback(dev, KEY_DIERESIS, tipo_console_logout, NULL);
  return NULL;
}

static MenuItem tipo_console_menu[] = {
	{&str_18_48, L1 | L2 | L3 | L4 | L5 | L6 | L7 | L8, tipo_console_carica},
	{&str_18_49, L1 | L2 | L3 | L4 | L5 | L6 | L7 | L8, tipo_console_associa},
	{&str_18_50, L1 | L2 | L3 | L4 | L5 | L6 | L7 | L8, tipo_console_annulla},
	{&str_18_51, L1 | L2 | L3 | L4 | L5 | L6 | L7 | L8, tipo_console_azzera},
	{NULL, 0, NULL}
};

/* Da qui il device e' il consumatore TIPO */

static unsigned char tipo_crc(unsigned char *d, int len)
{
  unsigned char crc;
  int i;
  
  crc = 0;
  for(i=0; i<len; i++) crc ^= d[i];
  return crc;
}

static int tipo_cerca_chiave(unsigned char *key)
{
  int i;
  
  for(i=0; i<TIPO_NUM_CHIAVI; i++)
    if(TIPO->chiave[i].valida && !memcmp(key, TIPO->chiave[i].code, 5))
      return i;
  
  return -1;
}

static int tipo_accendi_led(int inseritore, int chiave)
{
  int n, z1, z2, i;
  unsigned char ev[4];
  
  ev[0] = Punzonatura;
  n = inseritore*1000+chiave+1;
  ev[1] = n >> 8;
  ev[2] = n & 0xff;
  codec_queue_event(ev);
  
  if(tipo_modo > 1)
  {
    n = 0;
    z1 = -1;
    z2 = -1;
    TIPO->inseritore[inseritore].statozone &= ~0x03;
    for(i=0; !n && (i<TIPO_NUM_CHIAVI); i++)
      if(TIPO->chiave[i].valida)
      {
        if(z1 < 0)
        {
          if(TIPO->chiave[i].zona[inseritore][0] != 0xff)
            z1 = TIPO->chiave[i].zona[inseritore][0];
        }
        else if((TIPO->chiave[i].zona[inseritore][0] != 0xff) && (z1 != TIPO->chiave[i].zona[inseritore][0]))
          n = 1;
        
        if(z2 < 0)
        {
          if(TIPO->chiave[i].zona[inseritore][1] != 0xff)
            z2 = TIPO->chiave[i].zona[inseritore][1];
        }
        else if((TIPO->chiave[i].zona[inseritore][1] != 0xff) && (z2 != TIPO->chiave[i].zona[inseritore][1]))
          n = 1;
      }
    
    if(!n)
    {
      if((z1 != -1) && (ZONA[z1] & bitActive))
        TIPO->inseritore[inseritore].statozone |= 0x01;
      if((z2 != -1) && (ZONA[z2] & bitActive))
        TIPO->inseritore[inseritore].statozone |= 0x02;
      return 1;
    }
  }
  
  return 0;
}

static void tipo_aggiorna_led()
{
  int n, z1, z2, i, j;
  
  if(tipo_modo == 2)
  {
    if(!TIPO || !(TIPO->inseritore[0].to) || (TIPO->inseritore[0].to->active)) return;
    
    n = 0;
    z1 = -1;
    z2 = -1;
    TIPO->inseritore[0].statozone &= ~0x03;
    for(i=0; !n && (i<TIPO_NUM_CHIAVI); i++)
      if(TIPO->chiave[i].valida)
      {
        if(z1 < 0)
        {
          if(TIPO->chiave[i].zona[0][0] != 0xff)
            z1 = TIPO->chiave[i].zona[0][0];
        }
        else if((TIPO->chiave[i].zona[0][0] != 0xff) && (z1 != TIPO->chiave[i].zona[0][0]))
          n = 1;
        
        if(z2 < 0)
        {
          if(TIPO->chiave[i].zona[0][1] != 0xff)
            z2 = TIPO->chiave[i].zona[0][1];
        }
        else if((TIPO->chiave[i].zona[0][1] != 0xff) && (z2 != TIPO->chiave[i].zona[0][1]))
          n = 1;
      }
    
    if(!n)
    {
      if((z1 != -1) && (ZONA[z1] & bitActive))
        TIPO->inseritore[0].statozone |= 0x01;
      if((z2 != -1) && (ZONA[z2] & bitActive))
        TIPO->inseritore[0].statozone |= 0x02;
    }
  }
  else if(tipo_modo == 3)
  {
    for(j=0; j<TIPO_NUM_INSERITORI; j++)
    {
      if(!TIPO || !(TIPO->inseritore[j].to) || (TIPO->inseritore[j].to->active)) continue;
      
      n = 0;
      z1 = -1;
      z2 = -1;
      TIPO->inseritore[j].statozone &= ~0x03;
      for(i=0; !n && (i<TIPO_NUM_CHIAVI); i++)
        if(TIPO->chiave[i].valida)
        {
          if(z1 < 0)
          {
            if(TIPO->chiave[i].zona[j][0] != 0xff)
              z1 = TIPO->chiave[i].zona[j][0];
          }
          else if((TIPO->chiave[i].zona[j][0] != 0xff) && (z1 != TIPO->chiave[i].zona[j][0]))
            n = 1;
        
          if(z2 < 0)
          {
            if(TIPO->chiave[i].zona[j][1] != 0xff)
              z2 = TIPO->chiave[i].zona[j][1];
          }
          else if((TIPO->chiave[i].zona[j][1] != 0xff) && (z2 != TIPO->chiave[i].zona[j][1]))
            n = 1;
        }
      
      if(!n)
      {
        if((z1 != -1) && (ZONA[z1] & bitActive))
          TIPO->inseritore[j].statozone |= 0x01;
        if((z2 != -1) && (ZONA[z2] & bitActive))
          TIPO->inseritore[j].statozone |= 0x02;
      }
    }
  }
}

static void tipo_inseritore_timeout_zone(void *null, int inseritore)
{
  int n, z1, z2;
  
  n = TIPO->inseritore[inseritore].chiave;
  
  z1 = TIPO->chiave[n].zona[inseritore][0];
  z2 = TIPO->chiave[n].zona[inseritore][1];
  
  if(TIPO->inseritore[inseritore].statozone & 0x01)
    cmd_zone_on(z1, TIPO->chiave[n].modo[inseritore][0]);
  else
    cmd_zone_off(z1, 1);

  if(TIPO->inseritore[inseritore].statozone & 0x02)
    cmd_zone_on(z2, TIPO->chiave[n].modo[inseritore][1]);
  else
    cmd_zone_off(z2, 1);
  
  TIPO->inseritore[inseritore].statozone &= ~0x03;
  if(ZONA[z1] & bitActive) TIPO->inseritore[inseritore].statozone |= 0x01;
  if(ZONA[z2] & bitActive) TIPO->inseritore[inseritore].statozone |= 0x02;
}

static void tipo_inseritore_timeout(void *null, int inseritore)
{
  int n, i, z1, z2;
  
/*
  n = TIPO->inseritore[inseritore].chiave;
  
  if((TIPO->chiave[n].zona[inseritore][0] != 0xff) && (TIPO->chiave[n].zona[inseritore][1] != 0xff))
  {
    if(TIPO->inseritore[inseritore].statozone & 0x01)
      cmd_zone_on(TIPO->chiave[n].zona[inseritore][0], TIPO->chiave[n].modo[inseritore][0]);
    else
      cmd_zone_off(TIPO->chiave[n].zona[inseritore][0], 1);
  
    if(TIPO->inseritore[inseritore].statozone & 0x02)
      cmd_zone_on(TIPO->chiave[n].zona[inseritore][1], TIPO->chiave[n].modo[inseritore][1]);
    else
      cmd_zone_off(TIPO->chiave[n].zona[inseritore][1], 1);
  }
*/
  
  if(tipo_modo < 2)
    TIPO->inseritore[inseritore].statozone = 0;
  else
  {
    n = 0;
    z1 = -1;
    z2 = -1;
    for(i=0; !n && (i<TIPO_NUM_CHIAVI); i++)
      if(TIPO->chiave[i].valida)
      {
        if(z1 < 0)
        {
          if(TIPO->chiave[i].zona[inseritore][0] != 0xff)
            z1 = TIPO->chiave[i].zona[inseritore][0];
        }
        else if((TIPO->chiave[i].zona[inseritore][0] != 0xff) && (z1 != TIPO->chiave[i].zona[inseritore][0]))
          n = 1;
        
        if(z2 < 0)
        {
          if(TIPO->chiave[i].zona[inseritore][1] != 0xff)
            z2 = TIPO->chiave[i].zona[inseritore][1];
        }
        else if((TIPO->chiave[i].zona[inseritore][1] != 0xff) && (z2 != TIPO->chiave[i].zona[inseritore][1]))
          n = 1;
      }
    
    if(!n)
      TIPO->inseritore[inseritore].statozone &= ~0x04;
    else
      TIPO->inseritore[inseritore].statozone = 0;
  }
}

static void tipo_punzonatura(unsigned char *key, int inseritore)
{
  int n;
  
  n = tipo_cerca_chiave(key);
  if(n < 0) return;
  
  database_lock();
  database_set_alarm2(&ME[0]);
  database_set_alarm2(&ME[1794+n]);
  ME2[1794+n] = inseritore;
  database_unlock();
  
  if(TIPO->inseritore[inseritore].to->active || tipo_accendi_led(inseritore, n))
  {
    if(TIPO->inseritore[inseritore].to->active && (TIPO->inseritore[inseritore].chiave != n)) return;
    
    if((TIPO->chiave[n].zona[inseritore][0] == 0xff))
    {
      if(TIPO->chiave[n].zona[inseritore][1] != 0xff)
      {
        /* solo seconda zona definita */
        TIPO->inseritore[inseritore].statozone ^= 0x02;
        /* attivazione immediata */
        if(TIPO->inseritore[inseritore].statozone & 0x02)
          cmd_zone_on(TIPO->chiave[n].zona[inseritore][1], TIPO->chiave[n].modo[inseritore][1]);
        else
          cmd_zone_off(TIPO->chiave[n].zona[inseritore][1], 1);
      }
//      else
        /* nessuna zona definita */
//        TIPO->inseritore[inseritore].statozone = 0;
      
      /* Assicura che lo stato sia congruente con l'effettiva attivazione. */
      TIPO->inseritore[inseritore].statozone &= ~0x02;
      if(ZONA[TIPO->chiave[n].zona[inseritore][1]] & bitActive)
        TIPO->inseritore[inseritore].statozone |= 0x02;
    }
    else if(TIPO->chiave[n].zona[inseritore][1] == 0xff)
    {
      /* solo prima zona definita */
      TIPO->inseritore[inseritore].statozone ^= 0x01;
      /* attivazione immediata */
      if(TIPO->inseritore[inseritore].statozone & 0x01)
        cmd_zone_on(TIPO->chiave[n].zona[inseritore][0], TIPO->chiave[n].modo[inseritore][0]);
      else
        cmd_zone_off(TIPO->chiave[n].zona[inseritore][0], 1);
      
      /* Assicura che lo stato sia congruente con l'effettiva attivazione. */
      TIPO->inseritore[inseritore].statozone &= ~0x01;
      if(ZONA[TIPO->chiave[n].zona[inseritore][0]] & bitActive)
        TIPO->inseritore[inseritore].statozone |= 0x01;
    }
    else
    {
      /* entrambe le zone definite */
      TIPO->inseritore[inseritore].statozone++;
      TIPO->inseritore[inseritore].statozone &= 0x03;
      timeout_on(TIPO->inseritore[inseritore].toz, tipo_inseritore_timeout_zone, NULL, inseritore, TIPO_TIMEOUT_ATTIVAZIONE);
    }
  }
  else
  {
    if((TIPO->chiave[n].zona[inseritore][0] != 0xff) && (ZONA[TIPO->chiave[n].zona[inseritore][0]] & bitActive))
      TIPO->inseritore[inseritore].statozone |= 0x01;
    if((TIPO->chiave[n].zona[inseritore][1] != 0xff) && (ZONA[TIPO->chiave[n].zona[inseritore][1]] & bitActive))
      TIPO->inseritore[inseritore].statozone |= 0x02;
  }
  
  TIPO->inseritore[inseritore].statozone |= 0x04;
  TIPO->inseritore[inseritore].chiave = n;
  
  timeout_on(TIPO->inseritore[inseritore].to, tipo_inseritore_timeout, NULL, inseritore, TIPO_TIMEOUT_INSERITORE);
}

static void tipo_set_lang(int tipo)
{
  int i;
  char buf[24];
  
  if(tipo == CONSOLE_STL01)
  {
    for(i=0; i<sizeof(tipo_str)/sizeof(char*); i++) *tipo_str[i] = *tipo_str1[i];
    delphi_set_lang("TIPO", sizeof(tipo_str)/sizeof(char*), tipo_str);
  }
  else
  {
    for(i=0; i<sizeof(tipo_str)/sizeof(char*); i++) *tipo_str[i] = *tipo_str2[i];
    delphi_set_lang("TIPO2", sizeof(tipo_str)/sizeof(char*), tipo_str);
  }
  
  console_list_free(tipo_lista_inseritori);
  tipo_lista_inseritori = NULL;
  for(i=0; i<TIPO_NUM_INSERITORI; i++)
  {
    sprintf(buf, str_18_52, i);
    tipo_lista_inseritori = console_list_add(tipo_lista_inseritori, buf, NULL, tipo_console_associa2, i);
  }
}

static void tipo_null(void *null1, int null2)
{
}

static void tipo_loop(ProtDevice *dev)
{
  int i, n, addr;
  char cmd[24];
  struct termios tio;
  FILE *fp;
  
#ifdef TIPO_MIRUS
  support_log("Attivata limitazione periferiche");
  delphi_modo = DELPHITIPO;
  config.NodeID = DELPHI_FORCE_NODEID;
#endif
  
  sprintf(cmd, "TIPO Init (%d)", getpid());
  support_log(cmd);
  
//  console_register_lang(tipo_set_lang);
  
  pthread_mutex_lock(&tipo_mutex);
  if(!TIPO)
  {
    TIPO = malloc(sizeof(tipo_data));
    if(!TIPO) return;
    memset(TIPO, 0, sizeof(tipo_data));
    fp = fopen(tipoFileName, "r");
    if(fp)
    {
      i = fgetc(fp);
      if(i != EOF)
      {
        if(i & 0x80)
          fread(TIPO->chiave, 1, sizeof(TIPO->chiave), fp);
        else
        {
          tipo_chiave_old *old;
          
          rewind(fp);
          old = malloc(TIPO_NUM_CHIAVI*sizeof(tipo_chiave_old));
          fread(old, 1, TIPO_NUM_CHIAVI*sizeof(tipo_chiave_old), fp);
          for(i=0; i<TIPO_NUM_CHIAVI; i++)
          {
            TIPO->chiave[i].valida = old[i].valida;
            memcpy(TIPO->chiave[i].code, old[i].code, 5);
            memcpy(TIPO->chiave[i].zona, old[i].zona, TIPO_NUM_INSERITORI*2);
          }
          tipo_save = 1;
// 20080515
          free(old);
        }
      }
      fclose(fp);
    }
    for(i=0; i<TIPO_NUM_INSERITORI; i++)
    {
      TIPO->inseritore[i].to = timeout_init();
      TIPO->inseritore[i].ab = timeout_init();
      TIPO->inseritore[i].toz = timeout_init();
    }      
  }
  pthread_mutex_unlock(&tipo_mutex);
  
  tcgetattr(dev->fd, &tio);
  tio.c_cflag = config.consumer[dev->consumer].data.serial.baud1 | CS8 | CLOCAL | CREAD | PARENB;
  tio.c_iflag = IGNPAR | IGNBRK;
  tio.c_cc[VTIME] = 2;
  tio.c_cc[VMIN] = 0;
  tcsetattr(dev->fd, TCSANOW, &tio);
  
  for(i=0; console_menu_delphi[i].title && (console_menu_delphi[i].icon != CHIAVI_MENU); i++);
  if(console_menu_delphi[i].title)
  {
    console_menu_delphi[i].menu = tipo_console_menu;
    console_menu_delphi[i].perm = L8;
  }
  
  tipo_aggiorna_led();
  
  i = 0;
  addr = 0;
  
  while(1)
  {
    n = read(dev->fd, dev->buf+i, 1);
    if(!n)
    {
      if(tipo_modo & 0x01)
      {
#if 0
        for(n=0; n<TIPO_NUM_INSERITORI; n++)
        {
//          cmd[0] = tipo_led[0x07 ^ TIPO->inseritore[n].statozone];
          cmd[0] = tipo_led[TIPO->inseritore[n].statozone];
          cmd[1] = tipo_addr[n];
          write(dev->fd, cmd, 2);
        }
#else
        cmd[0] = tipo_led[TIPO->inseritore[addr].statozone];
        cmd[1] = tipo_addr[addr];
printf("TX: %02x%02x\n", cmd[0], cmd[1]);
        write(dev->fd, cmd, 2);
        addr++;
        addr %= TIPO_NUM_INSERITORI;
#endif
      }
      else
      {
//        cmd[0] = tipo_led[0x07 ^ TIPO->inseritore[0].statozone];
        cmd[0] = tipo_led[TIPO->inseritore[0].statozone];
printf("TX: %02x\n", cmd[0]);
        write(dev->fd, cmd, 1);
      }
      continue;
    }
    
//printf("%02x", dev->buf[i]);
//fflush(stdout);
    
    if(tipo_modo & 0x01)
    {
      if((dev->buf[0] == 0x5a) && (i < 8)) i++;
      if(i == 8)
      {
printf("RX: ");
for(i=0; i<8; i++) printf("%02x", dev->buf[i]);
printf("\n");
        /* inserita chiave */
        if(!tipo_crc(dev->buf+1, 7))
        {
          if(!TIPO->inseritore[dev->buf[1]].ab->active)
          {
            if(dev->buf[1])
            {
              /* tutti gli inseritori con indirizzo diverso da zero si
                 comportano sempre in modo normale */
              tipo_punzonatura(dev->buf+2, dev->buf[1]);
            }
            else
            {
              switch(tipo_configurazione)
              {
                case 0:
                  tipo_punzonatura(dev->buf+2, 0);
                  break;
                case 1:
                  console_send(tipo_console, "ATP05\r");
                  tipo_console_carica_chiave(dev->buf+2);
                  break;
                case 2:
                  console_send(tipo_console, "ATP05\r");
                  n = tipo_cerca_chiave(dev->buf+2);
                  sprintf(cmd, "%d", n+1);
                  tipo_console_associa1(tipo_console, (int)cmd, NULL);
                  break;
                case 3:
                  console_send(tipo_console, "ATP05\r");
                  n = tipo_cerca_chiave(dev->buf+2);
                  sprintf(cmd, "%d", n+1);
                  tipo_console_annulla1(tipo_console, (int)cmd, NULL);
                  break;
              }
            }
          }
          timeout_on(TIPO->inseritore[dev->buf[1]].ab, tipo_null, NULL, 0, 10);
          
//          cmd[0] = tipo_led[0x07 ^ TIPO->inseritore[dev->buf[1]].statozone];
          cmd[0] = tipo_led[TIPO->inseritore[dev->buf[1]].statozone];
          cmd[1] = tipo_addr[dev->buf[1]];
          write(dev->fd, cmd, 2);
        }
        i = 0;
      }
    }
    else
    {
      if((dev->buf[0] == 0x5a) && (i < 7)) i++;
      if(i == 7)
      {
//printf("---- Badge:\n");
//for(n=0; n<7; n++) printf("%02x", dev->buf[n]);
//printf("\n----\n");
        /* inserita chiave */
        if(!tipo_crc(dev->buf+1, 6))
        {
          if(!TIPO->inseritore[0].ab->active)
          {
            switch(tipo_configurazione)
            {
              case 0:
                tipo_punzonatura(dev->buf+1, 0);
                break;
              case 1:
                console_send(tipo_console, "ATP05\r");
                tipo_console_carica_chiave(dev->buf+1);
                break;
              case 2:
                console_send(tipo_console, "ATP05\r");
                n = tipo_cerca_chiave(dev->buf+1);
                sprintf(cmd, "%d", n+1);
                tipo_console_associa1(tipo_console, (int)cmd, NULL);
                break;
              case 3:
                console_send(tipo_console, "ATP05\r");
                n = tipo_cerca_chiave(dev->buf+1);
                sprintf(cmd, "%d", n+1);
                tipo_console_annulla1(tipo_console, (int)cmd, NULL);
                break;
            }
          }
          timeout_on(TIPO->inseritore[0].ab, tipo_null, NULL, 0, 10);
          
//          cmd[0] = tipo_led[0x07 ^ TIPO->inseritore[0].statozone];
          cmd[0] = tipo_led[TIPO->inseritore[0].statozone];
          write(dev->fd, cmd, 1);
        }
        i = 0;
      }
    }
  }
}

static void tipo_loop2(ProtDevice *dev)
{
  tipo_modo = 1;
  tipo_loop(dev);
}

static void tipo_loop3(ProtDevice *dev)
{
  tipo_modo = 2;
  tipo_loop(dev);
}

static void tipo_loop4(ProtDevice *dev)
{
  tipo_modo = 3;
  tipo_loop(dev);
}

static void tipo_save_th(void *null)
{
  FILE *fp;
  
  fp = fopen(tipoFileName, "w");
  fputc(0x80, fp);
  fwrite(TIPO->chiave, 1, sizeof(TIPO->chiave), fp);
  fclose(fp);
  tipo_saving = 0;
}

static int tipo_timer(ProtDevice *dev, int param)
{
  pthread_t pth;
  
  tipo_aggiorna_led();
  
  if((SECONDI == 59) && tipo_save && !tipo_saving)
  {
    tipo_saving = 1;
    if(!pthread_create(&pth,  NULL, (PthreadFunction)tipo_save_th, NULL))
    {
      pthread_detach(pth);
      tipo_save = 0;
    }
    else
      tipo_saving = 0;
  }
  return 1;
}

void _init()
{
#ifndef TIPO_MIRUS
  printf("DelphiTipo (plugin): " __DATE__ " " __TIME__ "\n");
#else
  printf("DelphiTipo[Mirus] (plugin): " __DATE__ " " __TIME__ "\n");
#endif
  
  console_register_lang(tipo_set_lang);
  
  prot_plugin_register("TIPO", 0, tipo_timer, NULL, (PthreadFunction)tipo_loop);
  prot_plugin_register("TIP2", 0, tipo_timer, NULL, (PthreadFunction)tipo_loop2);
  prot_plugin_register("TIP3", 0, tipo_timer, NULL, (PthreadFunction)tipo_loop3);
  prot_plugin_register("TIP4", 0, tipo_timer, NULL, (PthreadFunction)tipo_loop4);
}

