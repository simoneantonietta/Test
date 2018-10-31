#include "database.h"
#include "strings.h"
#include "delphi.h"
#include "master.h"
#include "command.h"
#include "timeout.h"
#include "user.h"
#include "console/console.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <dlfcn.h>

//#define DEBUG

#define RADIO_TIPO_CONTATTO	1
#define RADIO_TIPO_IR		2
#define RADIO_TIPO_ATTUATORE	3
#define RADIO_TIPO_SIRENA_EXT	4
#define RADIO_TIPO_SIRENA_INT	5

#define N_LINEE 16
#define N_SENSORI 128
#define N_RADIO_ACT	16
#define N_RADIO_SIR	8
#define MAX_AUTO_SENS (N_SENSORI+N_RADIO_ACT+N_RADIO_SIR)

#define RADIO868_GUASTO 0x40
#define RADIO868_FLAGS_NO_GUASTO	0x01
typedef struct {
  struct {
    unsigned int id;		// id univoco della periferia
    unsigned char input;		// numero ingresso della periferica
    unsigned char sensore;	// numero sensore logico
    unsigned char flags;	// es: esclusione guasti
    unsigned char tipo; // tipo periferica
  } sens[N_LINEE*2][MAX_AUTO_SENS];
} RadioID;
RadioID radio868_id;

#define AUTOAPP_NOOP	0
#define AUTOAPP_ADD	1
#define AUTOAPP_DELETE	2
/* Predisposizione alla gestione in centrale del livello soglia
   per la batteria. Con 0xff seguo la segnalazione di periferica. */
/* L'indice corrisponde alla famiglia di ID univoco, non al tipo periferica.
   00 = contatto Saet 3V6
   01 = infrarosso 3V6
   02 = ...
   03 = sirena esterna 6V/14V (indistinguibile, soglia FF)
   04 = contatto SecurGross 9V
   05 = attuatore 3V6
   06 = sirena interna 9V
*/
const unsigned char mmaster_battery868_thr[] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
/* Con 2 interrogazioni al giorno, la segnalazione viene ritardata di 7gg.
   Il valore massimo è 15, quindi 7gg. */
#define BATTERY868_CNT 15
struct {
  unsigned char stato:2;
  unsigned char stbatt:2;
  unsigned char cntbatt:4;
} radio868_id_stato[N_LINEE*2][MAX_AUTO_SENS];

#define RADIO_MAP_FLAG_GEST	1
#warning *** Flag RADIO_MAP_FLAG_PROG non usato ***
#define RADIO_MAP_FLAG_PROG	2
#define RADIO_MAP_FLAG_CAND	4
#define RADIO_MAP_FLAG_FORCED	8
#define RADIO_MAP_FLAG_CLEAN_PROG	0x0f
#define RADIO_MAP_FLAG_BATTERY	0x10

typedef struct {
  unsigned int id;	// id univoco della periferia
  unsigned char tipo;	// tipo sensore
  unsigned char rssi;	// RSSI
  unsigned char batt;	// livello batteria
  unsigned char flag;	// programmato su concentratore, candidato alla programmazione
  
#define IDMASK ((1<<24)-1)
#define RADIO868_FLAGS_NO_GUASTO	0x01
#define RADIO868_FLAGS_MOD	0x10000
  struct {
    int sens[2];
    int flags;
    int managed;
    int mod;
  } prog;
} RadioMap;
RadioMap radio868_map[N_LINEE*2][MAX_AUTO_SENS];

#define AUTOAPP_DELAYED_SAVE 150
static timeout_t *radio868_id_delay;
int radio868_id_save = 0;

#define RADIO_AUTOAPPR_DYN		(1<<30)
#define RADIO_AUTOAPPR_SEND		(1<<31)
unsigned int radio868_autoappr_flags;

int AutoParam_in_corso = 0;
unsigned int radio868_param = 0;
unsigned int radio868_param_id = 0;
unsigned char radio868_param_list[8][2];
unsigned char radio868_param_conc;

/* Timer per verifica ripetuta batteria bassa prima della segnalazione all'utente */
/* Ogni 12 ore dalla prima segnalazione */
#define BATTERIA868_TIMER (12*60)
static int radio868_batteria_timer[N_LINEE*2] = {0, };

/* Attuatori e sirene radio (868) */
unsigned char AT868[N_LINEE*2][N_RADIO_ACT];
unsigned int AT868status[N_LINEE*2][N_RADIO_ACT];
unsigned char SIR868[N_LINEE*2][N_RADIO_SIR];
unsigned int SIR868status[N_LINEE*2][N_RADIO_SIR];

/* Mappa di programmazione dei sensori, usata come filtro di sensore esistente/configurato.
   Considero 128 sensori per linea (2 linee per modulo master). */
unsigned int SEprog[N_LINEE][128/32];

extern void (*master_radio868_p)(int mm, unsigned char *cmd);
void (*json_refresh_map_p)(void *null, int delay) = NULL;
void (*json_auto_notify_p)(int conc, int id, int tipo, int rssi, int batt, int link[], int managed) = NULL;
#warning JSON_ALERT
void (*json_alert_p)(char *error, char *class, int code) = NULL;
void (*json_notify_param_p)(int id, int tipo, unsigned char param[8][2]);

static timeout_t *radio_timeout;
static timeout_t *radio_command_timeout;
static unsigned int radio_last_serial;
static unsigned int radio_last_tasto;

static ProtDevice *radio_console = NULL;
static pthread_mutex_t radio_mutex = PTHREAD_MUTEX_INITIALIZER;
static int radio_carica_da = 0;

#define NUM_TLC 64

struct {
  unsigned int serialnum;
  unsigned char mask;
} radio_telecomando[NUM_TLC];

/*
Default funzione tasti telecomando:
A: toggle zona totale
B: toggle zona insieme 1
C: toggle zona insieme 2
D: on telecomando 500

Le zone sono definite globalmente per poter avere
gli attuatori dell'SC8R sempre aggiornate. Se le
zone fossero diverse per ogni telecomando non potrei
mantenere l'aggiornamento.
*/

#define FUNC_MASK	0xffff0000
#define FUNC_NONE	0x00000000
#define FUNC_ZTOGGLE	0x00010000
#define FUNC_ZATT	0x00020000
#define FUNC_ZDIS	0x00030000
#define FUNC_TLC	0x00040000

static int radio_zone[4] = {FUNC_ZTOGGLE|0, FUNC_ZTOGGLE|216, FUNC_ZTOGGLE|217, FUNC_TLC|500};
static int radio_timer_esito = 0;
static int radio_timer_led_tlc9T[N_LINEE*2] = {0, };

static char *str_21_0;
static char *str_21_1;
static char *str_21_2;
static char *str_21_3;
static char *str_21_4;
static char *str_21_5;
static char *str_21_6;
static char *str_21_7;
static char *str_21_8;
static char *str_21_9;
static char *str_21_10;
static char *str_21_11;
static char *str_21_12;
static char *str_21_13;
static char *str_21_14;
static char *str_21_15;
static char *str_21_16;
static char *str_21_17;
static char *str_21_18;
static char *str_21_19;
static char *str_21_20;
static char *str_21_21;
static char *str_21_22;
static char *str_21_23;
static char *str_21_24;
static char *str_21_25;
static char *str_21_26;
static char *str_21_27;
static char *str_21_28;
static char *str_21_29;
static char *str_21_30;
static char *str_21_31;
static char *str_21_32;
static char *str_21_33;
static char *str_21_34;
static char *str_21_35;
static char *str_21_36;
static char *str_21_37;
static char *str_21_38;
static char *str_21_39;
static char *str_21_40;
static char *str_21_41;
static char *str_21_42;
static char *str_21_43;
static char *str_21_44;
static char *str_21_45;
static char *str_21_46;
static char *str_21_47;
static char *str_21_48;
static char *str_21_49;
static char *str_21_50;
static char *str_21_51;
static char *str_21_52;
static char *str_21_53;
static char *str_21_54;
static char *str_21_55;
static char *str_21_56;
static char *str_21_57;
static char *str_21_58;
static char *str_21_59;

static char *str1_21_0="Inizio Manutenz.";
static char *str1_21_1="Fine Manutenz.";
static char *str1_21_2="CARICA          ";
static char *str1_21_3="FUNZIONI        ";
static char *str1_21_4="MODIFICA        ";
static char *str1_21_5="CANCELLA        ";
static char *str1_21_6="ATN0104Configurazione\r";
static char *str1_21_7="ATN0105attiva su altra\r";
static char *str1_21_8="ATN0406console.\r";
static char *str1_21_9="CARICA";
static char *str1_21_10="ATN0404Anagrafica\r";
static char *str1_21_11="ATN0505completa\r";
static char *str1_21_12="ATN0404Carica da:\r";
static char *str1_21_13="ATN0304Programmato\r";
static char *str1_21_14="ATN0305telecomando\r";
static char *str1_21_15="FUNZIONI";
static char *str1_21_16="ATN0503Modifica:\r";
static char *str1_21_17="INV Zona";
static char *str1_21_18="ATT Zona";
static char *str1_21_19="DIS Zona";
static char *str1_21_20="TLC";
static char *str1_21_21="ATN0103Funzione tasto:\r";
static char *str1_21_22="Inverti Zona";
static char *str1_21_23="Attiva Zona";
static char *str1_21_24="Disattiva Zona";
static char *str1_21_25="Invia Telecom.";
static char *str1_21_26="ATN0703Zona:\r";
static char *str1_21_27="ATN0204Invia Telecom.\r";
static char *str1_21_28="ATN0504Funzione\r";
static char *str1_21_29="ATN0505impostata\r";
static char *str1_21_30="Nessuna funzione";
static char *str1_21_31="Nessuna";
static char *str1_21_32="MODIFICA";
static char *str1_21_33="ATN0604Attesa\r";
static char *str1_21_34="ATN0305rilevamento\r";
static char *str1_21_35="ATN0306telecomando\r";
static char *str1_21_36="ATN0203Telecomando %02d\r";
static char *str1_21_37="Modifica abil.";
static char *str1_21_38="Cancella";
static char *str1_21_39="ATN0406Cancellare?\r";
static char *str1_21_40="ATN0406Cancellato\r";
static char *str1_21_41="S";
static char *str1_21_42="N";
static char *str1_21_43="CANCELLA";
static char *str1_21_44="ATN0504Azzerare\r";
static char *str1_21_45="ATN0305l'anagrafica?\r";
static char *str1_21_46="ATN0404Anagrafica\r";
static char *str1_21_47="ATN0505azzerata\r";
static char *str1_21_48="Descrizione";
static char *str1_21_49="ATN0108(max 16 caratt.)\r";
static char *str1_21_50="ATN0204Telecomando %02d\r";
static char *str1_21_51="Azzera tutti";
static char *str1_21_52="i telecomandi";
static char *str1_21_53="Telecomando %02d";
static char *str1_21_54="ATN0706%d\r";
static char *str1_21_55="ATN0106________________\r";
static char *str1_21_56="ATG0106\r";
static char *str1_21_57="ATN0104%s\r";
static char *str1_21_58="ATN0105%s\r";
static char *str1_21_59="ATG0706\r";

static char *str2_21_0="Inizio Manutenz.";
static char *str2_21_1="Fine Manutenz.";
static char *str2_21_2="CARICA";
static char *str2_21_3="FUNZIONI";
static char *str2_21_4="MODIFICA";
static char *str2_21_5="CANCELLA";
static char *str2_21_6="ATN064041Configurazione\r";
static char *str2_21_7="ATN064051attiva su altra\r";
static char *str2_21_8="ATN064061console.\r";
static char *str2_21_9="CARICA";
static char *str2_21_10="ATN064041Anagrafica\r";
static char *str2_21_11="ATN064051completa\r";
static char *str2_21_12="ATN064041Carica da:\r";
static char *str2_21_13="ATN064041Programmato\r";
static char *str2_21_14="ATN064051telecomando\r";
static char *str2_21_15="FUNZIONI";
static char *str2_21_16="ATN064031Modifica:\r";
static char *str2_21_17="INV Zona";
static char *str2_21_18="ATT Zona";
static char *str2_21_19="DIS Zona";
static char *str2_21_20="TLC";
static char *str2_21_21="ATN064031Funzione tasto:\r";
static char *str2_21_22="Inverti Zona";
static char *str2_21_23="Attiva Zona";
static char *str2_21_24="Disattiva Zona";
static char *str2_21_25="Invia Telecom.";
static char *str2_21_26="ATN064031Zona:\r";
static char *str2_21_27="ATN064041Invia Telecom.\r";
static char *str2_21_28="ATN064041Funzione\r";
static char *str2_21_29="ATN064051impostata\r";
static char *str2_21_30="Nessuna funzione";
static char *str2_21_31="Nessuna";
static char *str2_21_32="MODIFICA";
static char *str2_21_33="ATN066041Attesa\r";
static char *str2_21_34="ATN064051rilevamento\r";
static char *str2_21_35="ATN064061telecomando\r";
static char *str2_21_36="ATN064031Telecomando %02d\r";
static char *str2_21_37="Modifica abil.";
static char *str2_21_38="Cancella";
static char *str2_21_39="ATN064061Cancellare?\r";
static char *str2_21_40="ATN064061Cancellato\r";
static char *str2_21_41="S";
static char *str2_21_42="N";
static char *str2_21_43="CANCELLA";
static char *str2_21_44="ATN064041Azzerare\r";
static char *str2_21_45="ATN064051l'anagrafica?\r";
static char *str2_21_46="ATN064041Anagrafica\r";
static char *str2_21_47="ATN064051azzerata\r";
static char *str2_21_48="Descrizione";
static char *str2_21_49="ATN064081(max 16 caratt.)\r";
static char *str2_21_50="ATN064041Telecomando %02d\r";
static char *str2_21_51="Azzera tutti";
static char *str2_21_52="i telecomandi";
static char *str2_21_53="Telecomando %02d";
static char *str2_21_54="ATN064061%d\r";
static char *str2_21_55="ATN000060________________\r";
static char *str2_21_56="ATG00006\r";
static char *str2_21_57="ATN064041%s\r";
static char *str2_21_58="ATN064051%s\r";
static char *str2_21_59="ATG05206\r";

static char **radio_str[] = {
  &str_21_0, &str_21_1, &str_21_2, &str_21_3, &str_21_4, &str_21_5,
  &str_21_6, &str_21_7, &str_21_8, &str_21_9, &str_21_10, &str_21_11,
  &str_21_12, &str_21_13, &str_21_14, &str_21_15, &str_21_16, &str_21_17,
  &str_21_18, &str_21_19, &str_21_20, &str_21_21, &str_21_22, &str_21_23,
  &str_21_24, &str_21_25, &str_21_26, &str_21_27, &str_21_28, &str_21_29,
  &str_21_30, &str_21_31, &str_21_32, &str_21_33, &str_21_34, &str_21_35,
  &str_21_36, &str_21_37, &str_21_38, &str_21_39, &str_21_40, &str_21_41,
  &str_21_42, &str_21_43, &str_21_44, &str_21_45, &str_21_46, &str_21_47,
  &str_21_48, &str_21_49, &str_21_50, &str_21_51, &str_21_52, &str_21_53,
  &str_21_54, &str_21_55, &str_21_56, &str_21_57, &str_21_58, &str_21_59,
};

static char **radio_str1[] = {
  &str1_21_0, &str1_21_1, &str1_21_2, &str1_21_3, &str1_21_4, &str1_21_5,
  &str1_21_6, &str1_21_7, &str1_21_8, &str1_21_9, &str1_21_10, &str1_21_11,
  &str1_21_12, &str1_21_13, &str1_21_14, &str1_21_15, &str1_21_16, &str1_21_17,
  &str1_21_18, &str1_21_19, &str1_21_20, &str1_21_21, &str1_21_22, &str1_21_23,
  &str1_21_24, &str1_21_25, &str1_21_26, &str1_21_27, &str1_21_28, &str1_21_29,
  &str1_21_30, &str1_21_31, &str1_21_32, &str1_21_33, &str1_21_34, &str1_21_35,
  &str1_21_36, &str1_21_37, &str1_21_38, &str1_21_39, &str1_21_40, &str1_21_41,
  &str1_21_42, &str1_21_43, &str1_21_44, &str1_21_45, &str1_21_46, &str1_21_47,
  &str1_21_48, &str1_21_49, &str1_21_50, &str1_21_51, &str1_21_52, &str1_21_53,
  &str1_21_54, &str1_21_55, &str1_21_56, &str1_21_57, &str1_21_58, &str1_21_59,
};

static char **radio_str2[] = {
  &str2_21_0, &str2_21_1, &str2_21_2, &str2_21_3, &str2_21_4, &str2_21_5,
  &str2_21_6, &str2_21_7, &str2_21_8, &str2_21_9, &str2_21_10, &str2_21_11,
  &str2_21_12, &str2_21_13, &str2_21_14, &str2_21_15, &str2_21_16, &str2_21_17,
  &str2_21_18, &str2_21_19, &str2_21_20, &str2_21_21, &str2_21_22, &str2_21_23,
  &str2_21_24, &str2_21_25, &str2_21_26, &str2_21_27, &str2_21_28, &str2_21_29,
  &str2_21_30, &str2_21_31, &str2_21_32, &str2_21_33, &str2_21_34, &str2_21_35,
  &str2_21_36, &str2_21_37, &str2_21_38, &str2_21_39, &str2_21_40, &str2_21_41,
  &str2_21_42, &str2_21_43, &str2_21_44, &str2_21_45, &str2_21_46, &str2_21_47,
  &str2_21_48, &str2_21_49, &str2_21_50, &str2_21_51, &str2_21_52, &str2_21_53,
  &str2_21_54, &str2_21_55, &str2_21_56, &str2_21_57, &str2_21_58, &str2_21_59,
};

static void radio_save()
{
  FILE *fp;
  
  fp = fopen("/saet/data/radio.nv.tmp", "w");
  if(fp)
  {
    fwrite(radio_zone, 1, sizeof(radio_zone), fp);
    fwrite(radio_telecomando, 1, sizeof(radio_telecomando), fp);
    fwrite(&radio868_id, 1, sizeof(radio868_id), fp);
    fclose(fp);
  }
  rename("/saet/data/radio.nv.tmp", "/saet/data/radio.nv");
}

static void* radio_string_save_pth(void *null)
{
  string_save();
  return NULL;
}

static void radio_string_save()
{
  pthread_t pth;
  
  pthread_create(&pth, NULL, (PthreadFunction)radio_string_save_pth, NULL);
  pthread_detach(pth);
}

static void radio_console_exit_common(ProtDevice *dev, int man)
{
  timeout_off(CONSOLE->timeout);
  radio_console = NULL;
  radio_carica_da = 0;
  pthread_mutex_unlock(&radio_mutex);
  /* Disattiva la manutenzione */
  if(man)
  {
    database_lock();
    database_set_alarm2(&ME[1859]);
    database_unlock();
  }
  CONSOLE->callback[KEY_TIMEOUT] = NULL;
  CONSOLE->callback_arg[KEY_TIMEOUT] = NULL;
  console_send(dev, "ATF0\r");
  console_send(dev, "ATL000\r");
}

static CALLBACK void* radio_console_logout(ProtDevice *dev, int press, void *man)
{
  if(press) radio_console_exit_common(dev, (int)man);
  return console_logout(dev, press, NULL);
}

static void radio_return_to_menu(char *dev, int null2)
{
  console_show_menu((ProtDevice*)dev, 0, NULL);
}

static CALLBACK void* radio_console_exit(ProtDevice *dev, int press, void *man)
{
  radio_console_exit_common(dev, (int)man);
  return console_show_menu(dev, 0, NULL);
}

static int radio_check_configurazione(ProtDevice *dev, int man)
{
  if(pthread_mutex_trylock(&radio_mutex))
  {
    console_send(dev, str_21_6);
    console_send(dev, str_21_7);
    console_send(dev, str_21_8);
    timeout_on(CONSOLE->timeout, (timeout_func)radio_return_to_menu, dev, 0, TIMEOUT_MSG);
    return 0;
  }
  
  /* Il caricamento attiva d'ufficio la manutenzione */
  if(man)
  {
    database_lock();
    database_set_alarm2(&ME[1858]);
    database_unlock();
  }
  
  radio_carica_da = 0;
  radio_console = dev;
  
  return 1;
}

/****************/
/* menu' CARICA */
/****************/

static void radio_console_carica_common(ProtDevice *dev);

static CALLBACK void* radio_console_carica_da(ProtDevice *dev, int key_as_string, void *null)
{
  sscanf((char*)key_as_string, "%d", &radio_carica_da);
  radio_console_carica_common(dev);
  return NULL;
}

static CALLBACK void* radio_console_carica(ProtDevice *dev, int press, void *null)
{
  console_disable_menu(dev, str_21_9);
  
  if(!radio_check_configurazione(dev, 1)) return NULL;
  
  radio_carica_da = 0;
  radio_console_carica_common(dev);
  return NULL;
}

static void radio_console_carica_common(ProtDevice *dev)
{
  int i;
  char cmd[16];
  
  for(i=radio_carica_da; (i<NUM_TLC) && radio_telecomando[i].serialnum; i++);
  if(i >= NUM_TLC)
  {
    /* Anagrafica completa */
    console_send(dev, "ATS04\r");
    console_send(dev, "ATF0\r");
    console_send(dev, str_21_10);
    console_send(dev, str_21_11);
    radio_console = NULL;
    pthread_mutex_unlock(&radio_mutex);
    timeout_on(CONSOLE->timeout, (timeout_func)radio_return_to_menu, dev, 0, TIMEOUT_MSG);
    return;
  }
  
  radio_carica_da = i;
  
  console_send(dev, "ATS04\r");
  console_send(dev, "ATF0\r");
  console_send(dev, str_21_12);
  console_send(dev, str_21_59);
  console_send(dev, "ATF1\r");
  sprintf(cmd, "ATL111%d\r", radio_carica_da);
  console_send(dev, cmd);
  
  console_register_callback(dev, KEY_STRING, radio_console_carica_da, NULL);
  console_register_callback(dev, KEY_CANCEL, radio_console_exit, (void*)1);
  console_register_callback(dev, KEY_TIMEOUT, radio_console_exit, (void*)1);
  console_register_callback(dev, KEY_DIERESIS, radio_console_logout, (void*)1);
}

static void radio_console_carica_chiave_timeout(char *dev, int null2)
{
  radio_console_carica_da((ProtDevice*)dev, (int)"", NULL);
}

static void radio_console_carica_event(int ev)
{
  ProtDevice *dev;
  char cmd[16];
  
  if(!radio_console) return;
  dev = radio_console;
  
  console_send(dev, "ATF0\r");
  sprintf(cmd, "ATE%04d\r", console_login_timeout_time/10);
  console_send(dev, cmd);
  console_send(dev, "ATS04\r");
  
  console_send(dev, str_21_13);
  console_send(dev, str_21_14);
  
  if(ev == 1000)
  {
    /* Caricato telecomando */
    sprintf(cmd, str_21_54, radio_carica_da);
    console_send(dev, cmd);
    radio_carica_da++;
  }
  else
  {
    /* Telecomando presente */
    sprintf(cmd, str_21_54, ev);
    console_send(dev, cmd);
  }
  
  timeout_on(CONSOLE->timeout, (timeout_func)radio_console_carica_chiave_timeout, dev, 0, TIMEOUT_MSG);
}

/******************/
/* menu' FUNZIONI */
/******************/

static CALLBACK void* radio_console_funzioni(ProtDevice *dev, int press, void *null);

static void radio_console_funzioni_modifica_timeout(char *dev, int null2)
{
  radio_console_funzioni((ProtDevice*)dev, 0, NULL);
}

static CALLBACK void* radio_console_funzioni_modifica_tlc(ProtDevice *dev, int tlc_as_string, void *null)
{
  int tlc;
  
  tlc = -1;
  sscanf((char*)tlc_as_string, "%d", &tlc);
  if((tlc < 0) || (tlc > n_ME))
  {
    radio_console_funzioni((ProtDevice*)dev, 0, NULL);
    return NULL;
  }
  
  radio_zone[CONSOLE->support_idx] = FUNC_TLC | tlc;
  console_send(dev, "ATF0\r");
  console_send(dev, "ATS03\r");
  console_send(dev, str_21_28);
  console_send(dev, str_21_29);
  timeout_on(CONSOLE->timeout, (timeout_func)radio_console_funzioni_modifica_timeout, dev, 0, TIMEOUT_MSG);
  radio_save();
  
  return NULL;
}

static CALLBACK void* radio_console_funzioni_modifica_func_tlc(ProtDevice *dev, int press, void *func)
{
  console_send(dev, "ATS03\r");
  
  console_send(dev, str_21_27);
  
  console_send(dev, str_21_59);
  console_send(dev, "ATF1\r");
  console_send(dev, "ATL111\r");
  
  console_register_callback(dev, KEY_STRING, radio_console_funzioni_modifica_tlc, NULL);
  console_register_callback(dev, KEY_CANCEL, radio_console_funzioni, NULL);
  console_register_callback(dev, KEY_TIMEOUT, radio_console_exit, NULL);
  console_register_callback(dev, KEY_DIERESIS, radio_console_logout, NULL);
  return NULL;
}

static CALLBACK void* radio_console_funzioni_modifica_zona(ProtDevice *dev, int press, void *zona)
{
  radio_zone[CONSOLE->support_idx] = (int)CONSOLE->temp_string | (int)zona;
  console_send(dev, "ATF0\r");
  console_send(dev, "ATS03\r");
  console_send(dev, str_21_28);
  console_send(dev, str_21_29);
  timeout_on(CONSOLE->timeout, (timeout_func)radio_console_funzioni_modifica_timeout, dev, 0, TIMEOUT_MSG);
  radio_save();
  
  return NULL;
}

static CALLBACK void* radio_console_funzioni_modifica_func_zona(ProtDevice *dev, int press, void *func)
{
  char descr[24], *des;
  int i;
  
  CONSOLE->temp_string = func;
  
  console_send(dev, "ATS03\r");
  
  console_send(dev, str_21_26);
  
  console_list_free(CONSOLE->support_list);
  CONSOLE->support_list = NULL;
  
  string_zone_name(0, &des, NULL);
  switch((int)func)
  {
    case FUNC_ZTOGGLE: sprintf(descr, "%s %s", str_21_17, des); break;
    case FUNC_ZATT: sprintf(descr, "%s %s", str_21_18, des); break;
    case FUNC_ZDIS: sprintf(descr, "%s %s", str_21_19, des); break;
  }
  CONSOLE->support_list = console_list_add(CONSOLE->support_list, descr, NULL, radio_console_funzioni_modifica_zona, 0);
  
  for(i=0; i<n_ZI; i++)
  {
    string_zone_name(1+n_ZS+i, &des, NULL);
    switch((int)func)
    {
      case FUNC_ZTOGGLE: sprintf(descr, "%s %s", str_21_17, des); break;
      case FUNC_ZATT: sprintf(descr, "%s %s", str_21_18, des); break;
      case FUNC_ZDIS: sprintf(descr, "%s %s", str_21_19, des); break;
    }
    CONSOLE->support_list = console_list_add(CONSOLE->support_list, descr, NULL, radio_console_funzioni_modifica_zona, 1+n_ZS+i);
  }
  
  CONSOLE->list_show_cancel = radio_console_funzioni;
  console_list_show(dev, CONSOLE->support_list, 0, 4, 0);
  return NULL;
}

static CALLBACK void* radio_console_funzioni_modifica_func_none(ProtDevice *dev, int press, void *null)
{
  radio_zone[CONSOLE->support_idx] = FUNC_NONE;
  console_send(dev, "ATF0\r");
  console_send(dev, "ATS03\r");
  console_send(dev, str_21_28);
  console_send(dev, str_21_29);
  timeout_on(CONSOLE->timeout, (timeout_func)radio_console_funzioni_modifica_timeout, dev, 0, TIMEOUT_MSG);
  radio_save();
  
  return NULL;
}

static CALLBACK void* radio_console_funzioni_modifica(ProtDevice *dev, int press, void *tasto)
{
  CONSOLE->support_idx = (int)tasto;
  
  console_send(dev, "ATS03\r");
  
  console_send(dev, str_21_21);
  
  console_list_free(CONSOLE->support_list);
  CONSOLE->support_list = NULL;
  
  CONSOLE->support_list = console_list_add(CONSOLE->support_list, str_21_22, NULL, radio_console_funzioni_modifica_func_zona, FUNC_ZTOGGLE);
  CONSOLE->support_list = console_list_add(CONSOLE->support_list, str_21_23, NULL, radio_console_funzioni_modifica_func_zona, FUNC_ZATT);
  CONSOLE->support_list = console_list_add(CONSOLE->support_list, str_21_24, NULL, radio_console_funzioni_modifica_func_zona, FUNC_ZDIS);
  CONSOLE->support_list = console_list_add(CONSOLE->support_list, str_21_25, NULL, radio_console_funzioni_modifica_func_tlc, FUNC_TLC);
  CONSOLE->support_list = console_list_add(CONSOLE->support_list, str_21_30, NULL, radio_console_funzioni_modifica_func_none, FUNC_NONE);
  
  CONSOLE->list_show_cancel = radio_console_funzioni;
  console_list_show(dev, CONSOLE->support_list, 0, 5, 0);
  return NULL;
}

static CALLBACK void* radio_console_funzioni(ProtDevice *dev, int press, void *null)
{
  char descr[24], *des;
  int i;
  
  console_disable_menu(dev, str_21_15);
  
  console_send(dev, str_21_16);
  
  console_list_free(CONSOLE->support_list);
  CONSOLE->support_list = NULL;
  
  for(i=0; i<4; i++)
  {
    switch(radio_zone[i]&FUNC_MASK)
    {
      case FUNC_NONE:
        sprintf(descr, "%c> %s", i+'A', str_21_31);
        break;
      case FUNC_ZTOGGLE:
        string_zone_name(radio_zone[i]&~FUNC_MASK, &des, NULL);
        sprintf(descr, "%c> %s %s", i+'A', str_21_17, des);
        break;
      case FUNC_ZATT:
        string_zone_name(radio_zone[i]&~FUNC_MASK, &des, NULL);
        sprintf(descr, "%c> %s %s", i+'A', str_21_18, des);
        break;
      case FUNC_ZDIS:
        string_zone_name(radio_zone[i]&~FUNC_MASK, &des, NULL);
        sprintf(descr, "%c> %s %s", i+'A', str_21_19, des);
        break;
      case FUNC_TLC:
        sprintf(descr, "%c> %s %04d", i+'A', str_21_20, radio_zone[i]&~FUNC_MASK);
        break;
    }
    CONSOLE->support_list = console_list_add(CONSOLE->support_list, descr, NULL, radio_console_funzioni_modifica, i);
  }
  
  CONSOLE->list_show_cancel = radio_console_exit;
  console_list_show(dev, CONSOLE->support_list, 0, 4, 0);
  return NULL;
}

/******************/
/* menu' MODIFICA */
/******************/

static CALLBACK void* radio_console_modifica_common(ProtDevice *dev, int press, void *null)
{
  console_send(dev, "ATS03\r");
  
  /* Attesa rilevamento telecomando */
  console_send(dev, str_21_33);
  console_send(dev, str_21_34);
  console_send(dev, str_21_35);
  
  console_register_callback(dev, KEY_CANCEL, radio_console_exit, NULL);
  console_register_callback(dev, KEY_TIMEOUT, radio_console_exit, NULL);
  console_register_callback(dev, KEY_DIERESIS, radio_console_logout, NULL);
  return NULL;
}

static void radio_console_modifica_timeout(char *dev, int null)
{
  radio_console_modifica_common((ProtDevice*)dev, 0, NULL);
}

static CALLBACK void* radio_console_modifica_descr2(ProtDevice *dev, int descr_as_int, void *tlc)
{
  char *descr = (char*)descr_as_int;
  
  free(RadioName[(int)tlc]);
  RadioName[(int)tlc] = (char*)strdup(descr);
  
  radio_string_save();
  radio_console_modifica_common((ProtDevice*)dev, 0, NULL);
  
  return NULL;
}

static CALLBACK void* radio_console_modifica_descr(ProtDevice *dev, int press, void *tlc)
{
  char *id_desc, cmd[32];

  console_send(dev, "ATS03\r");
  
  string_radio_name((int)tlc, NULL, &id_desc);
  
  sprintf(cmd, str_21_50, (int)tlc);
  console_send(dev, cmd);
  console_send(dev, str_21_55);
  console_send(dev, str_21_49);
  console_send(dev, str_21_56);
  if(!strcmp(id_desc, StringDescNull)) id_desc = "";
  sprintf(cmd, "ATL211%s\r", id_desc);
  console_send(dev, cmd);

  console_register_callback(dev, KEY_STRING, radio_console_modifica_descr2, tlc);
  console_register_callback(dev, KEY_CANCEL, radio_console_modifica_common, NULL);
  console_register_callback(dev, KEY_TIMEOUT, radio_console_exit, NULL);
  console_register_callback(dev, KEY_DIERESIS, radio_console_logout, NULL);
  return NULL;
}

static CALLBACK void* radio_console_modifica_mask(ProtDevice *dev, int press, void *tlc);

static CALLBACK void* radio_console_modifica_mask2(ProtDevice *dev, int press, void *tasto)
{
  radio_telecomando[CONSOLE->support_idx].mask ^= (1<<(int)tasto);
  radio_save();
  return radio_console_modifica_mask(dev, press, (void*)CONSOLE->support_idx);
}

static CALLBACK void* radio_console_modifica_mask(ProtDevice *dev, int press, void *tlc)
{
  int i;
  char descr[24], *des, *act;
  
  CONSOLE->support_idx = (int)tlc;
  
  console_send(dev, "ATS05\r");
  
  console_list_free(CONSOLE->support_list);
  CONSOLE->support_list = NULL;
  
  for(i=0; i<4; i++)
  {
    act = radio_telecomando[(int)tlc].mask&(1<<i)?str_21_41:str_21_42;
    
    switch(radio_zone[i]&FUNC_MASK)
    {
      case FUNC_NONE:
        sprintf(descr, "%s> %s", act, str_21_31);
        break;
      case FUNC_ZTOGGLE:
        string_zone_name(radio_zone[i]&~FUNC_MASK, &des, NULL);
        sprintf(descr, "%s> %s %s", act, str_21_17, des);
        break;
      case FUNC_ZATT:
        string_zone_name(radio_zone[i]&~FUNC_MASK, &des, NULL);
        sprintf(descr, "%s> %s %s", act, str_21_18, des);
        break;
      case FUNC_ZDIS:
        string_zone_name(radio_zone[i]&~FUNC_MASK, &des, NULL);
        sprintf(descr, "%s> %s %s", act, str_21_19, des);
        break;
      case FUNC_TLC:
        sprintf(descr, "%s> %s %04d", act, str_21_20, radio_zone[i]&~FUNC_MASK);
        break;
    }
    CONSOLE->support_list = console_list_add(CONSOLE->support_list, descr, NULL, radio_console_modifica_mask2, i);
  }
  
  CONSOLE->list_show_cancel = radio_console_modifica_common;
  console_list_show(dev, CONSOLE->support_list, 0, 4, 0);
  return NULL;
}

static CALLBACK void* radio_console_modifica_cancella2(ProtDevice *dev, int press, void *tlc)
{
  radio_telecomando[(int)tlc].serialnum = 0;
  radio_telecomando[(int)tlc].mask = 0;
  free(RadioName[(int)tlc]);
  RadioName[(int)tlc] = NULL;
  
  console_send(dev, "ATS06\r");
  console_send(dev, str_21_40);
  
  timeout_on(CONSOLE->timeout, (timeout_func)radio_console_modifica_timeout, dev, 0, TIMEOUT_MSG);
  radio_save();
  radio_string_save();
  
  return NULL;
}

static CALLBACK void* radio_console_modifica_cancella(ProtDevice *dev, int press, void *tlc)
{
  CONSOLE->support_idx = (int)tlc;
  
  console_send(dev, "ATS06\r");
  console_send(dev, str_21_39);
  
  console_register_callback(dev, KEY_ENTER, radio_console_modifica_cancella2, tlc);
  console_register_callback(dev, KEY_CANCEL, radio_console_modifica_common, NULL);
  console_register_callback(dev, KEY_TIMEOUT, radio_console_exit, NULL);
  console_register_callback(dev, KEY_DIERESIS, radio_console_logout, NULL);
  return NULL;
}

static CALLBACK void* radio_console_modifica(ProtDevice *dev, int press, void *null)
{
  console_disable_menu(dev, str_21_32);
  
  if(!radio_check_configurazione(dev, 0)) return NULL;
  
  return radio_console_modifica_common(dev, press, NULL);
}

static void radio_console_modifica_event(int idx)
{
  ProtDevice *dev;
  char cmd[32], *descr;
  
  if(idx >= NUM_TLC) return;
  
  dev = radio_console;
  
  console_send(dev, "ATF0\r");
  sprintf(cmd, "ATE%04d\r", console_login_timeout_time/10);
  console_send(dev, cmd);
  console_send(dev, "ATS03\r");
  
  sprintf(cmd, str_21_36, idx);
  console_send(dev, cmd);
  
  string_radio_name(idx, NULL, &descr);
  if(descr != StringDescNull)
  {
    sprintf(cmd, str_21_57, descr);
    console_send(dev, cmd);
  }
  
  console_list_free(CONSOLE->support_list);
  CONSOLE->support_list = NULL;
  
  CONSOLE->support_list = console_list_add(CONSOLE->support_list, str_21_48, NULL, radio_console_modifica_descr, idx);
  CONSOLE->support_list = console_list_add(CONSOLE->support_list, str_21_37, NULL, radio_console_modifica_mask, idx);
  CONSOLE->support_list = console_list_add(CONSOLE->support_list, str_21_38, NULL, radio_console_modifica_cancella, idx);
  
  CONSOLE->list_show_cancel = radio_console_modifica_common;
  console_list_show(dev, CONSOLE->support_list, 0, 3, 0);
}

/******************/
/* menu' CANCELLA */
/******************/

static CALLBACK void* radio_console_azzera1(ProtDevice *dev, int press, void *null)
{
  int i;
  
  for(i=0; i<NUM_TLC; i++)
  {
    radio_telecomando[i].serialnum = 0;
    radio_telecomando[i].mask = 0;
    free(RadioName[i]);
    RadioName[i] = NULL;
  }
  radio_save();
  radio_string_save();
  
  console_send(dev, "ATS03\r");
  console_send(dev, str_21_46);
  console_send(dev, str_21_47);
  timeout_on(CONSOLE->timeout, (timeout_func)radio_return_to_menu, dev, 0, TIMEOUT_MSG);
  return NULL;
}

static CALLBACK void* radio_console_azzera(ProtDevice *dev, int press, void *null)
{
  console_send(dev, "ATS03\r");
  
  console_send(dev, str_21_44);
  console_send(dev, str_21_45);
  
  console_register_callback(dev, KEY_ENTER, radio_console_azzera1, NULL);
  return NULL;
}

static CALLBACK void* radio_console_cancella(ProtDevice *dev, int press, void *null);

static void radio_console_cancella_timeout(char *dev, int null)
{
  radio_console_cancella((ProtDevice*)dev, 0, NULL);
}

static CALLBACK void* radio_console_cancella2(ProtDevice *dev, int press, void *idx_as_voidp)
{
  radio_telecomando[(int)idx_as_voidp].serialnum = 0;
  radio_telecomando[(int)idx_as_voidp].mask = 0;
  free(RadioName[(int)idx_as_voidp]);
  RadioName[(int)idx_as_voidp] = NULL;
  
  console_send(dev, "ATS06\r");
  console_send(dev, str_21_40);
  
  timeout_on(CONSOLE->timeout, (timeout_func)radio_console_cancella_timeout, dev, 0, TIMEOUT_MSG);
  
  radio_save();
  radio_string_save();
  
  return NULL;
}

static CALLBACK void* radio_console_cancella1(ProtDevice *dev, int press, void *idx_as_voidp)
{
  char cmd[32], *descr;
  
  console_send(dev, "ATS03\r");
  
  sprintf(cmd, str_21_50, (int)idx_as_voidp);
  console_send(dev, cmd);
  
  string_radio_name((int)idx_as_voidp, NULL, &descr);
  if(descr != StringDescNull)
  {
    sprintf(cmd, str_21_58, descr);
    console_send(dev, cmd);
  }
  
  console_send(dev, str_21_39);
  
  console_register_callback(dev, KEY_ENTER, radio_console_cancella2, idx_as_voidp);
  return NULL;
}

static CALLBACK void* radio_console_cancella(ProtDevice *dev, int press, void *null)
{
  int i;
  char tlc[20], *descr;
  
  console_disable_menu(dev, str_21_43);
  
  console_list_free(CONSOLE->support_list);
  CONSOLE->support_list = NULL;
  
  CONSOLE->support_list = console_list_add(CONSOLE->support_list, str_21_51, str_21_52, radio_console_azzera, -1);
  
  for(i=0; i<NUM_TLC; i++)
  {
    if(radio_telecomando[i].serialnum)
    {
      sprintf(tlc, str_21_53, i);
      string_radio_name(i, NULL, &descr);
      CONSOLE->support_list = console_list_add(CONSOLE->support_list, tlc, descr, radio_console_cancella1, i);
    }
  }
  
  console_list_show(dev, CONSOLE->support_list, 0, 3, 0);
  return NULL;
}

/******************/
/******************/

static MenuItem radio_console_menu[] = {
	{&str_21_2, L1 | L2 | L3 | L4 | L5 | L6 | L7 | L8, radio_console_carica},
	{&str_21_3, L1 | L2 | L3 | L4 | L5 | L6 | L7 | L8, radio_console_funzioni},
	{&str_21_4, L1 | L2 | L3 | L4 | L5 | L6 | L7 | L8, radio_console_modifica},
	{&str_21_5, L1 | L2 | L3 | L4 | L5 | L6 | L7 | L8, radio_console_cancella},
	{NULL, 0, NULL}
};

/******************/
/******************/

static void radio_set_lang(int tipo)
{
  int i;
  
  if(tipo == CONSOLE_STL01)
  {
    for(i=0; i<sizeof(radio_str)/sizeof(char*); i++) *radio_str[i] = *radio_str1[i];
    delphi_set_lang("RADIO", sizeof(radio_str)/sizeof(char*), radio_str);
  }
  else
  {
    for(i=0; i<sizeof(radio_str)/sizeof(char*); i++) *radio_str[i] = *radio_str2[i];
    delphi_set_lang("RADIO2", sizeof(radio_str)/sizeof(char*), radio_str);
  }
  
  free(CommandName[1858]);
  /*if(!CommandName[1858])*/ CommandName[1858] = (char*)strdup(str_21_0);
  free(CommandName[1859]);
  /*if(!CommandName[1859])*/ CommandName[1859] = (char*)strdup(str_21_1);
}

int mmaster_sensore_id(int conc, unsigned int id, int input, unsigned int *flags)
{
  int i;
  
  if(flags) *flags = 0;
  
  for(i=0; i<MAX_AUTO_SENS; i++)
  {
    if((radio868_id.sens[conc][i].id == id) &&
       ((input < 0) || (radio868_id.sens[conc][i].input == input)) &&
       (radio868_id_stato[conc][i].stato < AUTOAPP_DELETE))	// non è richiesta la cancellazione
    {
      if(flags) *flags = radio868_id.sens[conc][i].flags;
      return radio868_id.sens[conc][i].sensore;
    }
  }
  return -1;
}

void radio868_idprog_to(void *null1, int null2)
{
  int conc, i, list;
  unsigned char cmd[12];
  
  /* Invia la programmazione delle periferiche */
  
  /* Non sono più arrivate indicazioni di variazione, quindi posso
     salvare la configurazione corrente se variata. */
  if(radio868_id_save)
  {
    radio868_id_save = 0;
    radio_save();
#warning Parametri attuatori 868
    //mmaster_aggiorna_parametri_at868();
  }
  
  for(conc=0; conc<N_LINEE*2; conc++)
  {
    list = 0;
    for(i=0; i<MAX_AUTO_SENS; i++)
      if((radio868_id_stato[conc][i].stato == AUTOAPP_ADD) &&
         (TipoPeriferica[(conc>>2)*32+(conc&2)*8+2+(conc&1)*5] == SC8R2) &&
         (StatoPeriferica[(conc>>2)*4+(conc&2)] & (1<<(2+(conc&1)*5))))
      {
        cmd[0] = 40;
        cmd[1] = (conc&2)*8+2+(conc&1)*5;
        memcpy(cmd+2, &radio868_id.sens[conc][i].id, 4);	// id periferica
        cmd[6] = 1;
        cmd[7] = 0;
        cmd[8] = 0;
        master_queue((conc>>2), cmd);
        /* Elimino la prenotazione */
        radio868_id_stato[conc][i].stato = AUTOAPP_NOOP;
        list = 1;
      }
    /* Se ho programmato delle periferiche chiedo subito una lista così
       allineo tutte le informazioni (SE_prog, tipo periferica...) */
    if(list)
    {
      cmd[0] = 39;
      cmd[1] = (conc&2)*8+2+(conc&1)*5;
      cmd[2] = 0;
      master_queue((conc>>2), cmd);
    }
  }
}

void radio868_to_toolweb(int conc, int id, unsigned char *msg)
{
  static void (*toolweb_p)(int conc, int id, int tipo, int rssi, int batt, int in, int ind, int flags) = NULL;

  int ind, flags;
  
  if(!toolweb_p)
    toolweb_p = dlsym(NULL, "toolweb_868_perif");
  
  if(toolweb_p)
  {
    /* Rilevamento periferica */
    
    int i, ii, baseinput, nsens, natt;
    
    /* A tutte le periferiche associo almeno un sensore, la manomissione
       contenitore diventa manomissione di questo sensore.
       L'evento deve però essere "tamper sensore XX" e non "manomissione
       sensore XX", così che se un domani avessi periferiche radio
       (ma anche non) con ingressi a bilanciamento triplo, avrei
       due segnalazioni distinte specifiche ma con lo stesso numero
       identificativo. Non va bene nemmeno "manomissione periferica XX"
       perché in un campo misto potrei avere conflitti di numerazione. */
       
    baseinput = natt = 0;
    switch(msg[0])
    {
      case RADIO_TIPO_CONTATTO: nsens = 2; break;	// contatto
      case RADIO_TIPO_IR: nsens = 1; break;	// infrarosso
      case RADIO_TIPO_ATTUATORE: baseinput = 127; nsens = 1; natt = 1; break;	// attuatore (sensore speciale)
      case RADIO_TIPO_SIRENA_EXT: baseinput = 127; nsens = 1; break;	// sirena esterna (sensore speciale)
      case RADIO_TIPO_SIRENA_INT: baseinput = 127; nsens = 1; break;	// sirena interna (sensore speciale)
      default: nsens = 0; break;
    }
    for(ii=baseinput; ii<(baseinput+nsens); ii++)
    {
      /* Non mi serve trasmettere anche l'indicazione 'gestito' dell'SC8R, se lo è
         e c'è un sensore associato indico il sensore, se lo è ma non c'è un sensore
         cancello l'associazione sull'SC8R e indico il sensore 0xFFFF. */
      /* Occorre gestire il caso della predisposizione alla programmazione, per cui
         c'è già in centrale la programmazione del sensore ma l'SC8R non lo rileva.
         Dovrei inviare al tool come prima cosa tutta la lista dei programmati ma
         con RSSI non valido, e poi mandare quelli letti dall'SC8R che o vanno ad
         aggiungersi o vanno ad aggiornare l'RSSI per segnalare che effettivamente
         stanno funzionando. Per i sensori nuovi ma già registrati su un altro SC8R
         occorre segnalare il già programmato. */
      ind = mmaster_sensore_id(conc, id, ii, &flags);
      if(ii < 127)
        toolweb_p(conc, id, msg[0], msg[1], msg[2], ii, ind, flags);
      else
        toolweb_p(conc, id, msg[0], msg[1], msg[2], ii-127, ind, flags);
    }
    for(ii=128; ii<(128+natt); ii++)	// gli attuatori partono da 128, per distinguerli
    {
      ind = mmaster_sensore_id(conc, id, ii, &flags);
      toolweb_p(conc, id, msg[0], msg[1], msg[2], ii-127, ind, flags);
    }
  }
}

void radio868_auto_associa_sens(int conc, unsigned int id, int input, int sens, int flags);

int AutoAssociaPeriferica(int id, int nconc, int nlink, int *link)
{
  int n, i, managed, tipo;
  
  /* Caso 1: periferica già programmata */
  /* Caso 1a: periferica già programmata che varia le associazioni */
  /* Caso 1b: periferica già programmata che viene sprogrammata */
  
  /* Caso 2: periferica non programmata */
  /* Caso 2a: periferica non programmata che viene programmata */
  /* Caso 2b: periferica non programmata che cambia concentratore */
  /* Caso 2c: periferica non programmata che cambia concentratore ma aveva associazioni */
  
  if(!json_refresh_map_p)
    json_refresh_map_p = dlsym(NULL, "json_refresh_map");
  if(!json_auto_notify_p)
    json_auto_notify_p = dlsym(NULL, "json_auto_notify");
  
  /* Verifico se la periferica sia già programmata, e intanto recupero anche il tipo. */
  managed = 0;
  for(n=0; (n<N_LINEE*2)&&!managed; n++)
    for(i=0; (i<MAX_AUTO_SENS)&&!managed; i++)
    {
      if(radio868_map[n][i].id == id)
      {
        tipo = radio868_map[n][i].tipo;
        if(radio868_map[n][i].prog.managed)
        {
          managed = 1;
          break;
        }
      }
    }
  
  if(managed)
  {
    /* Per come funziona il for, 'n' è stato incrementato prima di uscire
       dal ciclo esterno, quindi va decrementato. */
    n--;
    
    /* La periferica è già programmata, non posso gestire comandi
       per un concentratore differente. */
    if(n == nconc)
    {
      /* Non importa se sia una variazione di associazione o una cancellazione,
         entrambe le operazioni fanno la stessa cosa. */
      for(n=0; n<nlink; n++)
      {
        /* Controllo che ci sia effettivamente una variazione */
        if(radio868_map[nconc][i].prog.sens[n] != link[n])
        {
          /* Rendo gli oggetti liberati di nuovo disponibili
             e mappo i nuovi programmati. */
          if((tipo == RADIO_TIPO_CONTATTO) || (tipo == RADIO_TIPO_IR))
          {
            if(radio868_map[nconc][i].prog.sens[n] >= 0)
              SEprog[nconc>>1][radio868_map[nconc][i].prog.sens[n]/32] &= ~(1<<(radio868_map[nconc][i].prog.sens[n]&31));
            if(link[n] >= 0)
              SEprog[nconc>>1][link[n]/32] |= (1<<(link[n]&31));
          }
          else if(((tipo == RADIO_TIPO_ATTUATORE) ||
                   (tipo == RADIO_TIPO_SIRENA_EXT) ||
                   (tipo == RADIO_TIPO_SIRENA_INT)) && (n == 0))
          {
            if(radio868_map[nconc][i].prog.sens[n] >= 0)
              SEprog[nconc>>1][radio868_map[nconc][i].prog.sens[n]/32] &= ~(1<<(radio868_map[nconc][i].prog.sens[n]&31));
            if(link[n] >= 0)
              SEprog[nconc>>1][link[n]/32] |= (1<<(link[n]&31));
          }
          
          radio868_map[nconc][i].prog.sens[n] = link[n];
          radio868_map[nconc][i].prog.mod = 1;
//printf("b. autosens[%d][%d] %d tipo %d link(%d,%d)\n", nconc, i, id, radio868_map[nconc][i].tipo, radio868_map[nconc][i].prog.sens[0], radio868_map[nconc][i].prog.sens[1]);
        }
      }
      
      if(json_refresh_map_p)
        json_refresh_map_p(NULL, 0);
      
      if(json_auto_notify_p)
        json_auto_notify_p(nconc, id, tipo, radio868_map[nconc][i].rssi, radio868_map[nconc][i].batt, radio868_map[nconc][i].prog.sens, 1);
    }
  }
  else
  {
    /* Periferica non programmata */
    /* Devo capire se ci fosse già un concentratore forzato, uso managed solo per
       fermare la ricerca al punto giusto. */
    managed = 0;
    for(n=0; (n<N_LINEE*2)&&!managed; n++)
    {
      if(n == nconc) continue;
      for(i=0; (i<MAX_AUTO_SENS)&&!managed; i++)
        if((radio868_map[n][i].id == id) && (radio868_map[n][i].prog.managed))
        {
          managed = 1;
          break;
        }
    }
    
    if(managed)
    {
      /* C'è una forzatura precedente (o comunque una pre associazione),
         libero quindi l'associazione */
      /* Per come funziona il for, 'n' è stato incrementato prima di uscire
         dal ciclo esterno, quindi va decrementato. */
      n--;
      
      /* Rendo gli oggetti liberati di nuovo disponibili */
      if((tipo == RADIO_TIPO_CONTATTO) || (tipo == RADIO_TIPO_IR))
      {
        if(radio868_map[n][i].prog.sens[0] >= 0)
          SEprog[n>>1][radio868_map[n][i].prog.sens[0]/32] &= ~(1<<(radio868_map[n][i].prog.sens[0]&31));
        if(radio868_map[n][i].prog.sens[1] >= 0)
          SEprog[n>>1][radio868_map[n][i].prog.sens[1]/32] &= ~(1<<(radio868_map[n][i].prog.sens[1]&31));
      }
      else if((tipo == RADIO_TIPO_ATTUATORE) ||
              (tipo == RADIO_TIPO_SIRENA_EXT) || (tipo == RADIO_TIPO_SIRENA_INT))
      {
        if(radio868_map[n][i].prog.sens[0] >= 0)
          SEprog[n>>1][radio868_map[n][i].prog.sens[0]/32] &= ~(1<<(radio868_map[n][i].prog.sens[0]&31));
      }
      radio868_map[n][i].prog.sens[0] = -1;
      radio868_map[n][i].prog.sens[1] = -1;
      radio868_map[n][i].prog.mod = 0;
//printf("c. autosens[%d][%d] %d tipo %d link(%d,%d)\n", n, i, id, radio868_map[n][i].tipo, radio868_map[n][i].prog.sens[0], radio868_map[n][i].prog.sens[1]);
    }
    
    /* Registro le nuove associazioni */
    /* Cerco la periferica sul concentratore desiderato, oppure creo una
       nuova periferica in anagrafica. */
    for(i=0; (i<MAX_AUTO_SENS)&&(radio868_map[nconc][i].id != id); i++);
    if(i == MAX_AUTO_SENS)
      for(i=0; (i<MAX_AUTO_SENS)&&radio868_map[nconc][i].id; i++);
    if(i < MAX_AUTO_SENS)
    {
      radio868_map[nconc][i].id = id;
      radio868_map[nconc][i].tipo = tipo;
      
      for(n=0; n<nlink; n++)
      {
        /* Controllo che ci sia effettivamente una variazione */
        if(radio868_map[nconc][i].prog.sens[n] != link[n])
        {
          /* Rendo gli oggetti liberati di nuovo disponibili
             e mappo i nuovi programmati. */
          if((tipo == RADIO_TIPO_CONTATTO) || (tipo == RADIO_TIPO_IR))
          {
            if(radio868_map[nconc][i].prog.sens[n] >= 0)
              SEprog[nconc>>1][radio868_map[nconc][i].prog.sens[n]/32] &= ~(1<<(radio868_map[nconc][i].prog.sens[n]&31));
            if(link[n] >= 0)
              SEprog[nconc>>1][link[n]/32] |= (1<<(link[n]&31));
          }
          else if(((tipo == RADIO_TIPO_ATTUATORE) ||
                   (tipo == RADIO_TIPO_SIRENA_EXT) ||
                   (tipo == RADIO_TIPO_SIRENA_INT)) && (n == 0))
          {
            if(radio868_map[nconc][i].prog.sens[n] >= 0)
              SEprog[nconc>>1][radio868_map[nconc][i].prog.sens[n]/32] &= ~(1<<(radio868_map[nconc][i].prog.sens[n]&31));
            if(link[n] >= 0)
              SEprog[nconc>>1][link[n]/32] |= (1<<(link[n]&31));
          }
          
          radio868_map[nconc][i].prog.sens[n] = link[n];
          radio868_map[nconc][i].prog.mod = 1;
        }
      }
//printf("d. autosens[%d][%d] %d tipo %d link(%d,%d) rssi %d\n", nconc, i, id, radio868_map[nconc][i].tipo, radio868_map[nconc][i].prog.sens[0], radio868_map[nconc][i].prog.sens[1], radio868_map[nconc][i].rssi);
      
      if(json_refresh_map_p)
        json_refresh_map_p(NULL, 0);
      
#if 0
      /* Segno in ogni caso la forzatura in centrale. Se è un cambio concentratore
         è ovvio che sia così, ma anche se sto semplicemente associando non voglio
         che la centrale mi cambi il concentratore sotto il naso. */
      buf[0] = 141;
      buf[1] = 0;
      buf[2] = 8;
      buf[3] = 0;
      buf[4] = nconc;
      memcpy(buf+5, &id, 4);
      buf[9] = 0xff;
      buf[10] = 0xff;
      buf[11] = 0;
      /* Se sono in autoapprendimento devo mandare il comando e basta,
         per provocare la forzatura, altrimenti devo aprire la connessione.
         La forzatura la devo fare anche se l'autoapprendimento è fermo
         perché potrei poi richiedere l'autoassociazione e la centrale
         deve sapere che c'è stata una forzatura, non posso tenere la
         forzatura solo in locale. */
      if(gfd > 0)
      {
        /* Connessione già attiva */
        WriteFile(gfd, buf, 12, &nn, NULL);
      }
      else
      {
        AttivaComunicazione();
        
        WriteFile(gfd, buf, 12, &nn, NULL);
        nn = ReadFileAsync(gfd, buf+16, 5, NULL, 2000);
        if((nn == 5) && (buf[16] == 141) && (buf[20] == 0))
        {
          nn = ReadFileAsync(gfd, buf+16, 16, NULL, 100);
          if(!memcmp(buf+5, &id, 4))
          {
            autosens[nconc][i].tipo = buf[16+9];
            if(buf[16+10] != 0xff)
              autosens[nconc][i].rssi = -buf[16+10];
            else
              autosens[nconc][i].rssi = 0xff;
            autosens[nconc][i].batteria = buf[16+11];
          }
          /* Eventuale ulteriori ingressi */
          do
            nn = ReadFileAsync(gfd, buf+16, 16, NULL, 100);
          while(nn > 0);
        }
        
        DisattivaComunicazione();
      }
      
#else
      if(!radio868_map[nconc][i].prog.mod)
        radio868_auto_associa_sens(nconc, id, -1, -1, 0);
      else
#endif
      
      if(json_auto_notify_p)
        json_auto_notify_p(nconc, id, tipo, radio868_map[nconc][i].rssi, radio868_map[nconc][i].batt, radio868_map[nconc][i].prog.sens, 0);
    }
  }
  
  return 0;
}

void AutoFlagsPeriferica(int id, int conc, int mflags, int flags)
{
  int n;
  
  for(n=0; n<MAX_AUTO_SENS; n++)
  if(radio868_map[conc][n].id == id)
  {
    if((radio868_map[conc][n].prog.flags & mflags) ^ (flags & mflags))
    {
      radio868_map[conc][n].prog.flags &= ~mflags;
      radio868_map[conc][n].prog.flags |= (flags & mflags) | RADIO868_FLAGS_MOD;
      radio868_map[conc][n].prog.mod = 1;
    }
  }
}

void radio868_auto_associa_sens(int conc, unsigned int id, int input, int sens, int flags)
{
  int i, f, c, t, e, l, s;
  
  /* Il concentratore deve essere indicato come 0-3 */
  
  /* Per prima cosa verifico che l'operazione sulla periferica sia congruente.
     La periferica deve o essere già programmata sullo stesso concentratore indicato,
     oppure non essere programmata affatto, altrimenti ignoro l'operazione. */
  /* Questo perché altrimenti partirebbero delle segnalazioni verso il toolweb che
     porterebbero ad indicazioni discordanti, farebbe sembrare la periferica come
     tornata libera anche se è già indicata associata ad un altro concentratore. */
  for(c=0; c<N_LINEE*2; c++)
  {
    if(c == conc) continue;
    for(i=0; i<MAX_AUTO_SENS; i++)
      if((radio868_id.sens[c][i].id == id) && (radio868_id_stato[c][i].stato != AUTOAPP_DELETE))
      {
        /* La periferica risulta già programmata su un altro concentratore, non devo modificare le impostazioni */
        /* Però se è richiesta la cancellazione su questo concentratore, devo cancellare.
           Ad esempio se il concentratore è stato spostato da una linea ad un'altra della stessa centrale. */
        if(input < 0)
        {
          unsigned char cmd[12];
          
          cmd[0] = 40;
          cmd[1] = (conc&2)*8+2+(conc&1)*5;
          memcpy(cmd+2, &id, 4);
          cmd[6] = 0;
          cmd[7] = 0;
          cmd[8] = 0;
          master_queue(conc>>2, cmd);
        }
        return;
      }
  }
  
  /* Ricerca del tipo periferica */
  /* Attenzione, se sto facendo una forzatura su un concentratore
     che non ha ancora mai rilevato la periferica, il tipo non viene
     trovato tra le periferiche del concentratore, devo fare una
     ricerca globale. */
  t = 0;
  for(c=0; !t && (c<2*N_LINEE); c++)
    for(i=0; !t && (i<MAX_AUTO_SENS); i++)
      if(radio868_map[c][i].id == id)
        t = radio868_map[c][i].tipo;
  
  f = -1;	// indice locazione libera
  c = e = 0;
  
  if(input < 0)
  {
    /* Rimuovo la periferica in toto */
    for(i=0; i<MAX_AUTO_SENS; i++)
    {
      if(radio868_id.sens[conc][i].id == id)
      {
        /* Azzero lo stato del sensore che viene eliminato non risultando poi
           più associato, altrimenti lo stato permane indefinitamente. */
        switch(t)
        {
          case RADIO_TIPO_CONTATTO:
          case RADIO_TIPO_IR:
            SE[(conc>>1)*128+radio868_id.sens[conc][i].sensore] = 0;
            SEprog[conc>>1][radio868_id.sens[conc][i].sensore/32] &=
              ~(1<<(radio868_id.sens[conc][i].sensore&31));
            break;
          case RADIO_TIPO_ATTUATORE:
            if(radio868_id.sens[conc][i].input == 0x7f)
            {
              AT868status[conc][radio868_id.sens[conc][i].sensore] = 0;
              SE[(conc>>1)*128+radio868_id.sens[conc][i].sensore] = 0;
              SEprog[conc>>1][radio868_id.sens[conc][i].sensore/32] &=
                ~(1<<(radio868_id.sens[conc][i].sensore&31));
            }
            break;
          case RADIO_TIPO_SIRENA_EXT:
          case RADIO_TIPO_SIRENA_INT:
            if(radio868_id.sens[conc][i].input == 0x7f)
            {
              SIR868status[conc][radio868_id.sens[conc][i].sensore] = 0;
              SE[(conc>>1)*128+radio868_id.sens[conc][i].sensore] = 0;
              SEprog[conc>>1][radio868_id.sens[conc][i].sensore/32] &=
                ~(1<<(radio868_id.sens[conc][i].sensore&31));
            }
            break;
        }
        
        radio868_id.sens[conc][i].id = 0;
        radio868_id.sens[conc][i].input = 0;
        radio868_id.sens[conc][i].sensore = 0;
        radio868_id.sens[conc][i].flags = 0;
        radio868_id_stato[conc][i].stato = AUTOAPP_NOOP;
        radio868_id_stato[conc][i].stbatt = 0;
        radio868_id_stato[conc][i].cntbatt = 0;
        f = i;
        e = 1;  // segno che ho cancellato una periferica esistente
      }
      else if(!radio868_id.sens[conc][i].id)
        f = i;
    }
    
    /* La rimozione periferica può avvenire anche se non risulta
       nell'elenco delle programmate, ad esempio se il concentratore
       segnala una periferica come gestita ma appunto per la centrale
       non lo è. Deve quindi istruire il concentratore che deve
       cancellarla. */
    if(f >= 0)
    {
      /* Reinserisco la registrazione solo come prenotazione. */
#if 0
      radio868_id.sens[conc][f].id = id;
      radio868_id_stato[conc][f].stato = AUTOAPP_DELETE;	// cancellazione
#else
      unsigned char cmd[12];
      
      cmd[0] = 40;
      cmd[1] = (conc&2)*8+2+(conc&1)*5;
      memcpy(cmd+2, &id, 4);
      cmd[6] = 0;
      cmd[7] = 0;
      cmd[8] = 0;
      master_queue(conc>>2, cmd);
#endif
    }
    
    if(!e)
    {
      if(t)
      {
printf("Forzatura id %08x conc %d\n", id, conc);
        /* Se ho richiesto la cancellazione di una periferica non esistente,
           o è una rimozione di una periferica dal concentratore per riallineamento,
           o è una forzatura di concentratore durante una acquisizione.
           Non è detto che l'acquisizione sia ancora in corso.
           Comunque in entrambi i casi segno la forzatura, ma solo se la periferica
           esiste già nella lista acquisita (radio868_map). */
        /* Prima rimuovo tutte le candidature per la periferica */
        /* Il tipo della periferica deve essere noto, quindi deve essere stata
           rilevata almeno una volta da un concentratore qualsiasi. */
        for(c=0; c<N_LINEE*2; c++)
          for(i=0; i<MAX_AUTO_SENS; i++)
            if(radio868_map[c][i].id == id)
            {
printf("Libero id %08x conc %d\n", id, c);
              radio868_map[c][i].flag &= ~(RADIO_MAP_FLAG_CAND|RADIO_MAP_FLAG_FORCED);
              e = 1;  // segno che comunque la periferica è tra le rilevate
            }
        /* Se la periferica è tra le rilevate allora imposto la forzatura */
        if(e)
        {
          /* Cerco la periferica nell'elenco del concentratore */
          for(i=0; (i<MAX_AUTO_SENS)&&(radio868_map[conc][i].id!=id); i++);
          if(i == MAX_AUTO_SENS)
          {
            /* La periferica non è (ancora) stata rilevata dal concentratore desiderato,
               devo aggiungere l'id all'elenco. */
            for(i=0; (i<MAX_AUTO_SENS)&&radio868_map[conc][i].id; i++);
            if(i < MAX_AUTO_SENS)
            {
              radio868_map[conc][i].id = id;
              radio868_map[conc][i].tipo = t;
              radio868_map[conc][i].rssi = 0xff;
              radio868_map[conc][i].batt = 0;
              radio868_map[conc][i].flag = 0;
            }
          }
          if(i < MAX_AUTO_SENS)
          {
            unsigned char msg[4];
            
printf("Forzato id %08x conc %d\n", id, conc);
            radio868_map[conc][i].flag |= (RADIO_MAP_FLAG_CAND|RADIO_MAP_FLAG_FORCED);
            msg[0] = t;
            msg[1] = radio868_map[conc][i].rssi;
            msg[2] = radio868_map[conc][i].batt;
            msg[3] = 0;
            radio868_to_toolweb(conc, id, msg);
          }
        }
      }
      else
      {
        /* Sto forzando una periferica mai vista */
        /* Non dovrebbe essere possibile... */
      }
    }
    else
    {
      /* Se ho cancellato la periferica devo anche segnare che non è più gestita */
      for(c=0; c<N_LINEE*2; c++)
        for(i=0; i<MAX_AUTO_SENS; i++)
          if(radio868_map[c][i].id == id)
            radio868_map[c][i].flag &= ~(RADIO_MAP_FLAG_GEST|RADIO_MAP_FLAG_PROG|RADIO_MAP_FLAG_FORCED);
    }
    return;
  }
  
  if(sens < 0)
  {
    /* Cancello l'eventuale registrazione presente */
    for(i=0; i<MAX_AUTO_SENS; i++)
    {
      if(radio868_id.sens[conc][i].id == id)
      {
        if(radio868_id.sens[conc][i].input == input)
        {
          /* Azzero lo stato del sensore che viene eliminato non risultando poi
             più associato, altrimenti lo stato permane indefinitamente. */
          switch(t)
          {
            case RADIO_TIPO_CONTATTO:
            case RADIO_TIPO_IR:
              SE[(conc>>1)*128+radio868_id.sens[conc][i].sensore] = 0;
              SEprog[conc>>1][radio868_id.sens[conc][i].sensore/32] &=
                ~(1<<(radio868_id.sens[conc][i].sensore&31));
              break;
            case RADIO_TIPO_ATTUATORE:
              if(radio868_id.sens[conc][i].input == 0x7f)
              {
                AT868status[conc][radio868_id.sens[conc][i].sensore] = 0;
                SE[(conc>>1)*128+radio868_id.sens[conc][i].sensore] = 0;
                SEprog[conc>>1][radio868_id.sens[conc][i].sensore/32] &=
                  ~(1<<(radio868_id.sens[conc][i].sensore&31));
              }
              break;
            case RADIO_TIPO_SIRENA_EXT:
            case RADIO_TIPO_SIRENA_INT:
              if(radio868_id.sens[conc][i].input == 0x7f)
              {
                SIR868status[conc][radio868_id.sens[conc][i].sensore] = 0;
                SE[(conc>>1)*128+radio868_id.sens[conc][i].sensore] = 0;
                SEprog[conc>>1][radio868_id.sens[conc][i].sensore/32] &=
                  ~(1<<(radio868_id.sens[conc][i].sensore&31));
              }
              break;
          }
          
          radio868_id.sens[conc][i].id = 0;
          radio868_id.sens[conc][i].input = 0;
          radio868_id.sens[conc][i].sensore = 0;
          radio868_id.sens[conc][i].flags = 0;
          //radio868_id.sens[conc][i].tipo = 0;
          radio868_id_stato[conc][i].stato = AUTOAPP_NOOP;
          radio868_id_stato[conc][i].stbatt = 0;
          radio868_id_stato[conc][i].cntbatt = 0;
          f = i;
        }
        else
          c++;	// ci sono altri ingressi registrati per la periferica
      }
    }
    if((f >= 0) && !c)
    {
      /* Era l'ultimo ingresso registrato, prenoto la deregistrazione
         della periferica sul modulo master. */
#if 0
      /* Reinserisco la registrazione solo come prenotazione. */
      radio868_id.sens[conc][f].id = id;
      radio868_id_stato[conc][f].stato = AUTOAPP_DELETE;	// cancellazione
#else
      unsigned char cmd[12];
      
      cmd[0] = 40;
      cmd[1] = (conc&2)*8+2+(conc&1)*5;
      memcpy(cmd+2, &id, 4);
      cmd[6] = 0;
      cmd[7] = 0;
      cmd[8] = 0;
      master_queue(conc>>2, cmd);
#endif
      
      /* Elimino il flag di gestito dalla mappa periferiche rilevate. */
      for(conc=0; conc<4; conc++)
      {
        for(i=0; i<MAX_AUTO_SENS; i++)
        {
          if(radio868_map[conc][i].id == id)
            radio868_map[conc][i].flag &= ~(RADIO_MAP_FLAG_GEST|RADIO_MAP_FLAG_PROG|RADIO_MAP_FLAG_FORCED);
        }
      }
    }
    return;
  }
  
  for(i=0; i<MAX_AUTO_SENS; i++)
  {
    if(radio868_id.sens[conc][i].id == id)
    {
      if(radio868_id.sens[conc][i].input == input)
      {
        if(radio868_id.sens[conc][i].sensore != sens)
        {
          /* Azzero lo stato del sensore che viene sostituito non risultando poi
             più associato, altrimenti lo stato permane indefinitamente. */
          switch(t)
          {
            case RADIO_TIPO_CONTATTO:
            case RADIO_TIPO_IR:
              SE[(conc>>1)*128+radio868_id.sens[conc][i].sensore] = 0;
              SEprog[conc>>1][radio868_id.sens[conc][i].sensore/32] &=
                ~(1<<(radio868_id.sens[conc][i].sensore&31));
              break;
            case RADIO_TIPO_ATTUATORE:
              if(radio868_id.sens[conc][i].input == 0x7f)
              {
                AT868status[conc][radio868_id.sens[conc][i].sensore] = 0;
                SE[(conc>>1)*128+radio868_id.sens[conc][i].sensore] = 0;
                SEprog[conc>>1][radio868_id.sens[conc][i].sensore/32] &=
                  ~(1<<(radio868_id.sens[conc][i].sensore&31));
              }
              break;
            case RADIO_TIPO_SIRENA_EXT:
            case RADIO_TIPO_SIRENA_INT:
              if(radio868_id.sens[conc][i].input == 0x7f)
              {
                SIR868status[conc][radio868_id.sens[conc][i].sensore] = 0;
                SE[(conc>>1)*128+radio868_id.sens[conc][i].sensore] = 0;
                SEprog[conc>>1][radio868_id.sens[conc][i].sensore/32] &=
                  ~(1<<(radio868_id.sens[conc][i].sensore&31));
              }
              break;
          }
        }
        
        /* Sostituzione semplice, non c'è altro da fare. */
        radio868_id.sens[conc][i].sensore = sens;
        /* Imposto i flags su tutti gli ingressi eventualmente registrati */
        for(i=0; i<MAX_AUTO_SENS; i++)
          if(radio868_id.sens[conc][i].id == id)
            radio868_id.sens[conc][i].flags = flags;
        return;
      }
      else
        c++;	// ci sono altri ingressi registrati per la periferica
    }
    else if(!radio868_id.sens[conc][i].id && (f < 0))
      f = i;
  }
  
  /* Se l'ingresso non è già in elenco lo inserisco nella
     prima posizione libera. Se non c'è nessuna posizione
     libera allora ritorno senza fare nulla. */
  if(f < 0) return;
  
  radio868_id.sens[conc][f].id = id;
  radio868_id.sens[conc][f].input = input;
  radio868_id.sens[conc][f].sensore = sens;
  /* Imposto i flags su tutti gli ingressi eventualmente registrati */
  for(i=0; i<MAX_AUTO_SENS; i++)
    if(radio868_id.sens[conc][i].id == id)
      radio868_id.sens[conc][i].flags = flags;
  
  if(input < 128)
    SEprog[conc>>1][sens/32] |= (1<<(sens&31));
  
  /* Se è il primo ingresso configurato, prenoto l'associazione periferica-concentratore */
  /* L'invio effettivo sul campo deve essere fatto alla fine
     dando priorità prima alle cancellazioni e poi agli inserimenti
     affinché possano essere gestite regolarmente tutte le posizioni. */
  if(!c)
  {
    radio868_id_stato[conc][f].stato = AUTOAPP_ADD;
    timeout_on(radio868_id_delay, radio868_idprog_to, NULL, 0, AUTOAPP_DELAYED_SAVE);
    
    /* La programmazione sul campo di una nuova periferica non provoca la restituzione
       automatica dello stato di periferica gestita, quindi la forzo direttamente
       in questo punto. */
    for(f=0; f<N_LINEE*2; f++)
    {
      for(i=0; i<MAX_AUTO_SENS; i++)
      {
        if(radio868_map[f][i].id == id)
        {
          if(f == conc)
          {
            radio868_map[f][i].flag &= ~RADIO_MAP_FLAG_CLEAN_PROG;
            radio868_map[f][i].flag |= RADIO_MAP_FLAG_GEST|RADIO_MAP_FLAG_CAND|RADIO_MAP_FLAG_FORCED;
          }
          else
            radio868_map[f][i].flag &= ~RADIO_MAP_FLAG_CLEAN_PROG;
        }
      }
    }
  }
}

/* Viene chiamata a fronte dell'evento di presenza della periferica radio (Rx:1D...)
   e compila la lista interna delle periferiche radio. Verifica se si tratti di una
   periferica gestita e nel caso di discordanza tra quanto previsto e quanto ricevuto
   allinea il campo. */
int mmaster_auto_register(int conc, unsigned int id, unsigned char *data)
{
  int i, j, ind, c;
  
  /* L'ID 00000000 non è valido ma il concentratore ogni tanto me lo segnala.
     Sembra che a fronte di cancellazioni multiple in blocco, ogni tanto il
     concentratore sbagli e lasci in elenco delle cose spurie. */
  if((id == 0) && data[3])
  {
#if 0
    /* Segno anche una cancellazione, così che la registrazione sparisca
       anche dal concentratore. */
    for(i=0; (i<MAX_AUTO_SENS)&&radio868_map[conc][i].id; i++);
    if(i < MAX_AUTO_SENS)
    {
      radio868_id.sens[conc][i].id = 0;
      radio868_id.sens[conc][i].input = 0;
      radio868_id.sens[conc][i].sensore = 0;
      radio868_id.sens[conc][i].flags = 0;
      radio868_id_stato[conc][i].stato = AUTOAPP_DELETE;
      radio868_id_stato[conc][i].stbatt = 0;
      radio868_id_stato[conc][i].cntbatt = 0;
    }
#else
    unsigned char cmd[12];
    
    cmd[0] = 40;
    cmd[1] = (conc&2)*8+2+(conc&1)*5;
    memset(cmd+2, 0, 4+3);
    master_queue(conc>>2, cmd);
#endif
    return 0;
  }
  
  /* Verifica se sia già in elenco */
  for(i=0; (i<MAX_AUTO_SENS)&&(radio868_map[conc][i].id!=id); i++);
  /* Se non c'è allora riservo una posizione */
  if(i == MAX_AUTO_SENS)
    for(i=0; (i<MAX_AUTO_SENS)&&
      (radio868_map[conc][i].id||(radio868_id_stato[conc][i].stato==AUTOAPP_DELETE)); i++);
  if(i<MAX_AUTO_SENS)
  {
    radio868_map[conc][i].id = id;
    radio868_map[conc][i].tipo = data[0];
    /* Se la periferica è programmata non gestisco i dati di RSSI e batteria della segnalazione
       della lista acquisiti, potrebbe contenere valori vecchi non più coerenti. */
#if 0
    if(!(radio868_map[conc][i].flag & (RADIO_MAP_FLAG_GEST|RADIO_MAP_FLAG_PROG)) || data[3] ||
       (radio868_map[conc][i].rssi == 0))
#else
    if(!(radio868_map[conc][i].prog.managed) || data[3] ||
       (radio868_map[conc][i].rssi == 0))
#endif
    {
      radio868_map[conc][i].rssi = data[1];
      radio868_map[conc][i].batt = data[2];
    }
    
    /* Verifica se la periferica sia già associata a qualche sensore */
    ind = mmaster_sensore_id(conc, id, -1, NULL);
    if(data[3])
    {
      /* Periferica gestita dal concentratore */
      if(ind < 0)
      {
        /* La periferica non va più gestita, la rimuovo. */
        radio868_auto_associa_sens(conc, id, -1, -1, 0);
        data[3] = 0;
      }
      else
      {
        /* Tolgo l'eventuale prenotazione di programmazione,
           la periferica risulta già programmata. */
        /* Devo togliere tutte le eventuali prenotazioni. */
        for(ind=0; ind<MAX_AUTO_SENS; ind++)
        {
          if((radio868_id.sens[conc][ind].id == id) &&
             (radio868_id_stato[conc][ind].stato != AUTOAPP_DELETE))
            radio868_id_stato[conc][ind].stato = AUTOAPP_NOOP;
        }
        
        for(j=0; j<MAX_AUTO_SENS; j++)
        {
          if((radio868_id.sens[conc][j].id == id) &&
             (radio868_id_stato[conc][j].stato < AUTOAPP_DELETE))
          {
            /* Controllo batteria per le periferiche registrate */
            if((radio868_map[conc][i].batt != 0xff) &&
               (radio868_map[conc][i].flag & RADIO_MAP_FLAG_BATTERY))
            {
              /* Controllo batteria solo su segnalazione richiesta */
              radio868_map[conc][i].flag &= ~RADIO_MAP_FLAG_BATTERY;
              if(((id>>24) >= sizeof(mmaster_battery868_thr)) ||
                 (radio868_map[conc][i].batt < mmaster_battery868_thr[id>>24]))
              {
                if(radio868_id_stato[conc][j].cntbatt < BATTERY868_CNT)
                  radio868_id_stato[conc][j].cntbatt++;
                if(radio868_id_stato[conc][j].cntbatt == BATTERY868_CNT)
                {
                  //ind = radio868_id.sens[conc][j].sensore;
                  
                  if(!(radio868_id_stato[conc][j].stbatt & 1))
                  {
                    /* Fronte della batteria.
                       La gestione è demandata completamente al programma utente
                       che legge lo stato continuo di batteria bassa, quindi non
                       c'è nulla da dover fare qui. */
                  }
                  if(!(radio868_id_stato[conc][j].stbatt&2))
                  {
                    /* Invio la segnalazione (una volta al giorno)
                       La gestione è demandata completamente al programma utente
                       che legge lo stato continuo di batteria bassa, quindi non
                       c'è nulla da dover fare qui. */
                  }
                  radio868_id_stato[conc][j].stbatt = 3;	// in corso+inviata
                  /* Lo stato di batteria viene aggiornato anche nello stato sensori/attuatori/sirene */
                  switch(data[0])
                  {
                    case RADIO_TIPO_CONTATTO:
                    case RADIO_TIPO_IR:
#if 1
                      /* Sulla Delphi gli attuatori e le sirene vengono comunque mappati
                         sui sensori fisici della centrale così da uniformarne la gestione. */
                    case RADIO_TIPO_ATTUATORE:
                    case RADIO_TIPO_SIRENA_EXT:
                    case RADIO_TIPO_SIRENA_INT:
#endif
                      if(!(SE[(conc>>1)*128+radio868_id.sens[conc][i].sensore] & bitBattery))
                      {
                        SE[(conc>>1)*128+radio868_id.sens[conc][i].sensore] |= bitBattery;
                      }
                      break;
#if 0
                    case RADIO_TIPO_ATTUATORE:
                      if(!(AT868status[conc][radio868_id.sens[conc][j].sensore] & bitBattery))
                      {
                        AT868status[conc][radio868_id.sens[conc][j].sensore] |= bitBattery;
                      }
                      break;
                    case RADIO_TIPO_SIRENA_EXT:
                    case RADIO_TIPO_SIRENA_INT:
                      if(!(SIR868status[conc][radio868_id.sens[conc][j].sensore] & bitBattery))
                      {
                        SIR868status[conc][radio868_id.sens[conc][j].sensore] |= bitBattery;
                      }
                      break;
#endif
                    default:
                      break;
                  }
                }
              }
              else
              {
                /* Anche se la periferica segnala batteria bassa, la soglia
                   reale gestita dalla centrale è più bassa e non superata.
                   Riparto con il conteggio.
                   Non interrompo l'interrogazione, la periferica potrebbe
                   non più innescare se non inviasse ulteriori segnalazioni
                   Lo stato deve rimanere "in corso". */
                radio868_id_stato[conc][j].cntbatt = 1;
              }
            }
            break;
          }
        }
      }
    }
    else
    {
      if((id != 0xffffffff) && !(radio868_map[conc][i].flag & (RADIO_MAP_FLAG_GEST|RADIO_MAP_FLAG_PROG)))
      {
        /* La periferica non è gestita dal concentratore ma è
           programmata in centrale => refresh */
        for(ind=0; ind<MAX_AUTO_SENS; ind++)
        {
          if((radio868_id.sens[conc][ind].id == id) &&
             (radio868_id_stato[conc][ind].stato < AUTOAPP_DELETE))
          {
            radio868_id_stato[conc][ind].stato = AUTOAPP_ADD;
            timeout_on(radio868_id_delay, radio868_idprog_to, NULL, 0, AUTOAPP_DELAYED_SAVE);
            break;
          }
        }
      }
    }
    
    if(!(radio868_map[conc][i].flag & (RADIO_MAP_FLAG_GEST|RADIO_MAP_FLAG_PROG)) || data[3])
    {
      radio868_map[conc][i].flag &= ~(RADIO_MAP_FLAG_GEST|RADIO_MAP_FLAG_PROG);
      radio868_map[conc][i].flag |= data[3];
      radio868_map[conc][i].prog.managed = data[3];
      if(data[3] && !(radio868_map[conc][i].flag & RADIO_MAP_FLAG_CAND))
      {
        int cc, ii;
        /* Essendo la periferica programmata diventa automaticamente la candidata forzata.
           Devo andare ad eliminare altre eventuali candidature. */
        for(cc=0; cc<2*N_LINEE; cc++)
          for(ii=0; ii<MAX_AUTO_SENS; ii++)
            if((radio868_map[cc][ii].id == id) &&
               (radio868_map[cc][ii].flag & RADIO_MAP_FLAG_CAND))
            {
              /* Ho trovato il candidato corrente, lo rimuovo */
              radio868_map[cc][ii].flag &= ~(RADIO_MAP_FLAG_CAND|RADIO_MAP_FLAG_FORCED);
            }
        radio868_map[conc][i].flag |= RADIO_MAP_FLAG_CAND|RADIO_MAP_FLAG_FORCED;
      }
    }
    
//printf("a. autosens[%d][%d] %d tipo %d link(%d,%d) rssi %d\n", conc, i, id, radio868_map[conc][i].tipo, radio868_map[conc][i].prog.sens[0], radio868_map[conc][i].prog.sens[1], radio868_map[conc][i].rssi);

      
#warning Mappatura attuatori 868
#if 0
    if((radio868_map[conc][i].flag & (RADIO_MAP_FLAG_GEST|RADIO_MAP_FLAG_PROG)) &&
       (radio868_map[conc][i].tipo == RADIO_TIPO_ATTUATORE))
      mmaster_map_SC8R2_actuators();
#endif
  }
  
  /* Filtro l'acquisizione di periferiche al di fuori del periodo richiesto,
     in particolare quando fermo l'acquisizione e l'SC8R2 continua per 10-20
     secondi ad inviare segnalazioni. L'utente ha fatto stop, non voglio che
     gli si continuino ad aggiornare le informazioni. */
  if(!(radio868_autoappr_flags & RADIO_AUTOAPPR_SEND)) return 0;
  
  ind = i;
  /* Se la segnalazione mi arriva dal concentratore che gestisce la periferica, allora
     restituisco 1 per inoltrare la segnalazione all'utente. Una periferica gestita
     può segnalare sempre solo dallo stesso concentratore. */
  /* Però non è vero, potrei ricevere lo stesso ID prima dal concentratore sui cui la
     periferica è programmata, poi da un altro concentratore che ha visto in precedenza
     la periferica, ma non è programmata su questa. Non devo inoltrare la notifica
     dal concentratore non programmato. */
  /* Risolvo il problema inizializzando la mappa con la candidatura forzata dei sensori
     programmati. */
  if(radio868_map[conc][i].flag & (RADIO_MAP_FLAG_GEST|RADIO_MAP_FLAG_PROG)) return 1;
  /* Se ricevo da un altro concentratore, la periferica non può essere di tipo gestita per
     il motivo di cui sopra, quindi devo solo verificare la candidatura.
     Se ricevo dal concentratore candidato allora inoltro a prescindere. */
  if(radio868_map[conc][i].flag & RADIO_MAP_FLAG_CAND) return 1;
  /* Altrimenti vado a cercare chi è il candidato. Se lo trovo confronto l'RSSI
     e seleziono eventualmente il nuovo candidato. Se non lo trovo allora questo
     concentratore è il primo candidato. */
  for(c=0; c<N_LINEE*2; c++)
  {
    for(i=0; i<MAX_AUTO_SENS; i++)
    {
      if((radio868_map[c][i].id == radio868_map[conc][ind].id) &&
         (radio868_map[c][i].flag & RADIO_MAP_FLAG_CAND))
      {
        /* Ho trovato il candidato corrente */
        if(!(radio868_map[c][i].flag & RADIO_MAP_FLAG_FORCED) &&
           (radio868_map[conc][ind].rssi < radio868_map[c][i].rssi))
        {
          /* C'è un nuovo candidato (questo), quindi elimino il precedente */
          radio868_map[c][i].flag &= ~RADIO_MAP_FLAG_CAND;
          radio868_map[conc][ind].flag |= RADIO_MAP_FLAG_CAND;
          return 1;
        }
        else
        {
          /* Il candidato rimane lo stesso (l'altro), non inoltro questa segalazione */
          return 0;
        }
      }
    }
  }
  
  /* Se è il primo rilevamento o continua ad essere il candidato. */
  radio868_map[conc][ind].flag |= RADIO_MAP_FLAG_CAND;
  return 1;
}

static void radio_init()
{
  static int initdone = 0;
  FILE *fp;
  int i, conc;
  
  if(initdone) return;
  initdone = 1;
  
//  console_register_lang(radio_set_lang);
  
  radio_timeout = timeout_init();
  radio_command_timeout = timeout_init();
  
  memset(radio_telecomando, 0, sizeof(radio_telecomando));
  memset(&radio868_id, 0, sizeof(radio868_id));
  fp = fopen("/saet/data/radio.nv", "r");
  if(fp)
  {
    fread(radio_zone, 1, sizeof(radio_zone), fp);
    fread(radio_telecomando, 1, sizeof(radio_telecomando), fp);
    fread(&radio868_id, 1, sizeof(radio868_id), fp);
    fclose(fp);
  }
  
  radio868_id_delay = timeout_init();
  
  memset(radio868_map, 0, sizeof(radio868_map));
  
#warning Eliminare da anagrafica le periferiche di SC8R2 non più previsti
  
  for(i=0; console_menu_delphi[i].title && (console_menu_delphi[i].icon != RADIO_MENU); i++);
  if(console_menu_delphi[i].title)
  {
    console_menu_delphi[i].menu = radio_console_menu;
    console_menu_delphi[i].perm = L8;
  }
}

/*
Si può creare una macro per entrare in manutenzione seguendo
lo stato di allarme di un sensore, in modo che non sia necessario
avere il TerminalData. In questo caso però non è più possibile
entrare in manutenzione con il TerminalData perché il sensore
forzerebbe comunque lo stato. Questa funzione dovrebbe quindi
anche riazzerare CommandName[1858] e CommandName[1859] in modo
che non appaiano in elenco.

Oppure si creano due macro, di attivazione e disattivazione, e ci
pensa poi il programma utente a chiamarle come vuole. In questo
modo non si creano conflitti.
*/

void Radio(int ind)
{
  static unsigned char statozone[n_ZS+1];
  static int impiantoattivo = -1;
  static int manomissione = 0;
  
  int i, tasto, tmp;
  unsigned char cmd[8];
  
  radio_init();
  
  /* Verifica lo stato dell'impianto. */
  if(!radio_timeout->active)
  {
    tmp = 0;
    if(ZONA[0] & bitActive) tmp = 1;
    for(i=0; !tmp && (i<n_ZI); i++)
      if(ZI[i] & bitActive) tmp = 1;
    impiantoattivo &= ~0x02;
    if(impiantoattivo != tmp)
      impiantoattivo = tmp | 0x02;
  }
  
/*
   ATTENZIONE:
   Se la centrale gestisce solo le zone Totale e Insieme 0 e Insieme 1,
   lo stato di attivazione sull'attuatore +7 rappresenta semplicemente
   l'OR degli attuatori +0 +1 e +2, quindi potrebbe essere superfluo.
   Per ora lo lascio così, per maggiore flessibilità (l'impianto
   risulta attivo anche per una eventuale zona Insieme 2 o 3, anche
   senza evidenza sugli attuatori).
   Dovesse servire una uscita, questa diventa sacrificabile.
*/
  if(impiantoattivo & 0x02)
  {
    /* E' variato lo stato di impianto attivo. */
    
    /* Ad ogni variazione di stato, comunque tacito le sirene. */
//printf("Off sirene\n");
    OFFA((ind<<3)+4, 1);
    OFFA((ind<<3)+5, 1);
    
    /* Poi a seconda dello stato effettivo abilito o disabilito
       l'addormentamento dei sensori. In pratica comunico all'SC8R
       lo stato di attivazione. */
    if(impiantoattivo & 0x01)
    {
      ONAT((ind<<3)+7, 1);
    }
    else
    {
      OFFA((ind<<3)+7, 1);
    }
  }
  
  /* Stato zone -> LED */
  if(!(ME[1858] & bitAlarm))
  {
    if(radio_timer_led_tlc9T[(ind>>5)*4+((ind&0x10)>>3)+(((ind&0xf)==2)?0:1)])
    {
      /* Telecomandi 9T */
      for(i=0; i<3; i++)
      {
        if(ZONA[radio_zone[i] & ~FUNC_MASK] & bitActive)
          Donanl((unsigned int*)(&AT[(ind<<3)+i]));
        else if(ZONA[radio_zone[i] & ~FUNC_MASK] & bitStatusInactive)
          Donala((unsigned int*)(&AT[(ind<<3)+i]));
        else
          Doffa((unsigned int*)(&(AT[(ind<<3)+i])));
      }
      Donanl((unsigned int*)(&(AT[(ind<<3)+3])));
    
      radio_timer_led_tlc9T[(ind>>5)*4+((ind&0x10)>>3)+(((ind&0xf)==2)?0:1)]--;
    }
    else
    {
      /* Telecomandi 4T */
      /* Se l'impianto non è in manutenzione aggiorno
         lo stato dei led dei telecomandi radio. */
      //for(i=0; i<4; i++)
      /* Comando in ordine inverso così da inviare sempre
         il quarto led di abilitazione 9T prima degli altri stati */
      for(i=3; i>=0; i--)
      {
        switch(radio_zone[i] & FUNC_MASK)
        {
          case FUNC_ZTOGGLE:
          case FUNC_ZATT:
          case FUNC_ZDIS:
            if(ZONA[radio_zone[i] & ~FUNC_MASK] & bitActive)
              Donanl((unsigned int*)(&AT[(ind<<3)+i]));
            else if(ZONA[radio_zone[i] & ~FUNC_MASK] & bitStatusInactive)
              Donala((unsigned int*)(&AT[(ind<<3)+i]));
            else
              Doffa((unsigned int*)(&(AT[(ind<<3)+i])));
            break;
          default:
            Doffa((unsigned int*)(&(AT[(ind<<3)+i])));
            break;
        }
      }
    }

  /*
     La sirena deve suonare se:
     - allarme zona (la zona è quindi attiva e ci sono sensori in allarme)
     - manomissioni ad impianto sia attivo che disattivo (devo evitare che
       l'impianto venga manomesso di giorno), ma solo di tamper, non di
       batteria scarica. Come faccio a distinguere i tipi di periferica?
       Da una parte posso considerare tutte le manomissioni su indirizzo
       pari, ma per gli indirizzi dispari come faccio a sapere se sono
       relativi a batterie basse di periferiche radio o effettive manomissioni
       su periferiche SAN filate?
       
       Posso fregarmene, faccio suonare per qualunque manomissione.
       Una manomissione è una manomissione, non importa se è relativa alle
       periferiche radio o a quelle filate.
  */
  
  /* Posso suonare piano per manomissione, oppure forte per allarme. */
  
  tmp = 0;
  if(FS(ZONA[0])) tmp = 1;
  for(i=0; !tmp && (i<n_ZI); i++)
    if(FS(ZI[i])) tmp = 1;
  
//if (tmp) printf("On sirene\n");
//ONAT((ind<<3)+5, tmp);
  ONAT((ind<<3)+4, tmp);
  
  /* Se non sono già in allarme, verifico la manomissione.
     Occorre però generare una sorta di fronte, perché
     altrimenti attivare e disattivare l'impianto non fa
     smettere di sunare. */
  if(!(AT[(ind<<3)+4] & bitON))
//if(!(AT[(ind<<3)+5] & bitON))
  {
    if(!radio_timeout->active)
    {
      /* Calcola lo stato di manomissione ed i relativi fronti. */
      tmp = 0;
      if((ZONA[0] & bitSabotage) || M_LINEA || M_CONTENITORE) tmp = 1;
      for(i=0; !tmp && (i<n_ZI); i++)
        if(ZI[i] & bitSabotage) tmp = 1;
      
      manomissione &= 0x01;
      if(tmp ^ manomissione)
      {
        if(tmp)
          manomissione = 0x03;
        else
          manomissione = 0;
      }
    }
  
//if (manomissione & 0x02) printf("Lamp sirene\n");
//ONAT((ind<<3)+5, tmp);
    ONAL((ind<<3)+4, manomissione & 0x02);
  }
  
  /*
     Una volta attivata la sirena, la disattivazione deve per forza essere
     fatta a seguito di un disinserimento impianto. Se l'impianto era già
     disattivato (la sirena suona per manomissione), allora devo
     attivare->disattivare.
  */
  }
  else
  {
    /* In manutenzione azzero sempre l'attuatore +0, in modo
       da creare un impulso come conferma all'avvenuto caricamento
       del telecomando in anagrafica. */
    if(radio_timer_esito && !radio_timeout->active) radio_timer_esito--;
    if(!radio_timer_esito) Doffa((unsigned int*)(&AT[(ind<<3)+0]));
  }
  
  if(ME[1858] & bitLHF)
  {
    /* Disattiva tutte le zone */
    if(!radio_timeout->active)
    {
      for(i=1; i<=n_ZS; i++)
      {
        statozone[i] = ZONA[i];
        cmd_zone_off(i, 0);
      }
    }
    
    /* Entra in manutenzione */
    if(master_periph_present(ind))
    {
      cmd[0] = 8;
      cmd[1] = ind & 0x1f;
      cmd[2] = 0xc;
      cmd[3] = 0;
      cmd[4] = 0;
      master_queue(ind>>5, cmd);
    }
    
    /* Azzera gli attuatori LED telecomandi */
    for(i=0; i<4; i++) Doffa((unsigned int*)(&AT[(ind<<3)+i]));
  }
  
// 24-11-2008
  if(ME[1859] & bitLHF)
//  if(ME[1859] & bitAlarm)
  {
    database_reset_alarm2(&ME[1858]);
    database_reset_alarm2(&ME[1859]);
    
    /* Riattiva tutte le zone */
    if(!radio_timeout->active)
    {
      for(i=1; i<=n_ZS; i++)
      {
        /* Devo riattivare con allarme! */
        if(statozone[i] & bitActive) cmd_zone_on(i, 1);
      }
    }
    
    /* Esce dalla manutenzione */
    if(master_periph_present(ind))
    {
      cmd[0] = 8;
      cmd[1] = ind & 0x1f;
      cmd[2] = 0x4;
      cmd[3] = 0;
      cmd[4] = 0;
      master_queue(ind>>5, cmd);
    }
  }
  
  /* Ricezione codice */
  tasto = SE[(ind<<3)+3] - 'A';
  if((tasto >= 0) && (tasto <= 3))
  {
#ifdef DEBUG
printf("Tasto %d - %08d\n", tasto, SE[(ind<<3)+4]);
#endif
    for(i=0; i<NUM_TLC; i++)
      if(radio_telecomando[i].serialnum == SE[(ind<<3)+4]) break;
      
    if(ME[1858] & bitAlarm)
    {
      /* Gestione in manutenzione */
#ifdef DEBUG
printf("Gestione Anagrafica\n");
#endif
      
      if(i >= NUM_TLC)
      {
#ifdef DEBUG
printf("  non presente\n");
#endif
        /* Se il telecomando non è presente lo registra. */
        for(i=radio_carica_da; (i<NUM_TLC) && radio_telecomando[i].serialnum; i++);
        if(i >= NUM_TLC)
        {
#ifdef DEBUG
printf("    anagrafica completa\n");
#endif
          /* Anagrafica completa */
          Donala((unsigned int*)(&AT[(ind<<3)+0]));
          radio_timer_esito = 30;	// mantiene l'attuazione per 3 secondi
        }
        else
        {
#ifdef DEBUG
printf("    registrato (%d)\n", i);
#endif
          radio_telecomando[i].serialnum = SE[(ind<<3)+4];
          radio_telecomando[i].mask = 0xff;	// tutti i tasti abilitati
          
          /* Registrazione OK */
          Donanl((unsigned int*)(&AT[(ind<<3)+0]));
          radio_timer_esito = 30;	// mantiene l'attuazione per 3 secondi
          
          /* Segnala sul TerminalData il caricamento. */
          radio_console_carica_event(1000);
          
          radio_save();
        }
      }
      else
      {
        /* Se invece è già presente segnala comunque il caricamento. */
        /* Attenzione: questo può capitare solo se la precedente registrazione
           non è andata a buon fine sul telecomando o se il telecomando è stato
           riverginizzato successivamente. Solo in questi casi il telecomando
           può trasmettere sulla frequenza base ed il messaggio essere ricevuto
           in fase di manutenzione. */
        Donanl((unsigned int*)(&AT[(ind<<3)+0]));
        radio_timer_esito = 30;	// mantiene l'attuazione per 3 secondi
        
        /* Segnala sul TerminalData il numero di telecomando */
        radio_console_carica_event(i);
      }
    }
    else
    {
      /* Gestione normale */
#ifdef DEBUG
printf("Gestione normale\n");
#endif
      
      if(radio_console)
      {
        /* Il TerminalData è attivo e quindi devo intercettare il telecomando
           per le sue funzioni e non per l'operatività sull'impianto. */
        radio_console_modifica_event(i);
      }
      
      /* Se il telecomando è valido e la funzione del tasto non è mascherata */
      else if((i<NUM_TLC) && (radio_telecomando[i].mask & (1<<tasto)))
      {
        /* Esegue la funzione */
        i = radio_zone[tasto] & ~FUNC_MASK;
        switch(radio_zone[tasto] & FUNC_MASK)
        {
          case FUNC_ZTOGGLE:
            if(ZONA[i] & bitActive)
            {
              cmd_zone_off(i, 0);
              break;
            }
            /* Se la zona non è attiva mi comporto come FUNC_ZATT. */
          case FUNC_ZATT:
            /* La prima volta attiva normalmente, così se la zona
               non è pronta ne viene informato l'utente. Se viene
               comandata di nuovo entro un certo tempo allora
               attiva con fuori servizio. */
            if(radio_command_timeout->active &&
               (radio_last_serial == SE[(ind<<3)+4]) &&
               (radio_last_tasto == tasto))
            {
              cmd_zone_on(i, 2);
              timeout_off(radio_command_timeout);
            }
            else
            {
              cmd_zone_on(i, 0);
              if(!(ZONA[i] & bitActive))
              {
                radio_last_serial = SE[(ind<<3)+4];
                radio_last_tasto = tasto;
                timeout_on(radio_command_timeout, NULL, 0, 0, 50);
              }
            }
            break;
          case FUNC_ZDIS:
            cmd_zone_off(i, 0);
            break;
          case FUNC_TLC:
            database_set_alarm2(&ME[i]);
            break;
          default:
            break;
        }
      }
    }
  }
  else if((tasto >= 4) && (tasto <= 7))
  {
    /* Pressione prolungata tasto telecomando 4T */
  }
  else if(tasto == 12)
  {
    /* Pressione tasto telecomando 9T */
    /* Il tasto è dato dall'ultima cifra del codice */
    tasto = SE[(ind<<3)+4] % 10;
    SE[(ind<<3)+4] /= 10;
    for(i=0; i<NUM_TLC; i++)
      if(radio_telecomando[i].serialnum == SE[(ind<<3)+4]) break;
      
    if(ME[1858] & bitAlarm)
    {
      /* Gestione in manutenzione */
      if(i >= NUM_TLC)
      {
        /* Se il telecomando non è presente lo registra. */
        for(i=radio_carica_da; (i<NUM_TLC) && radio_telecomando[i].serialnum; i++);
        if(i >= NUM_TLC)
        {
          /* Anagrafica completa */
          Donala((unsigned int*)(&AT[(ind<<3)+0]));
          radio_timer_esito = 30;	// mantiene l'attuazione per 3 secondi
        }
        else
        {
          radio_telecomando[i].serialnum = SE[(ind<<3)+4];
          radio_telecomando[i].mask = 0xff;	// tutti i tasti abilitati
          
          /* Registrazione OK */
          cmd[0] = 39;
          cmd[1] = ind & 0x1f;
          cmd[2] = 2;
          master_queue(ind>>5, cmd);
          
          /* Segnala sul TerminalData il caricamento. */
          radio_console_carica_event(1000);
          
          radio_save();
        }
      }
      else
      {
        /* Se invece è già presente segnala comunque il caricamento. */
        /* Attenzione: questo può capitare solo se la precedente registrazione
           non è andata a buon fine sul telecomando o se il telecomando è stato
           riverginizzato successivamente. Solo in questi casi il telecomando
           può trasmettere sulla frequenza base ed il messaggio essere ricevuto
           in fase di manutenzione. */
        cmd[0] = 39;
        cmd[1] = ind & 0x1f;
        cmd[2] = 2;
        master_queue(ind>>5, cmd);
        
        /* Segnala sul TerminalData il numero di telecomando */
        radio_console_carica_event(i);
      }
    }
    else
    {
      /* Gestione 9T normale */
      if(radio_console)
      {
        /* Il TerminalData è attivo e quindi devo intercettare il telecomando
           per le sue funzioni e non per l'operatività sull'impianto. */
        radio_console_modifica_event(i);
      }
      
      /* Se il telecomando è valido e la funzione del tasto non è mascherata */
      else if(i<NUM_TLC)
      {
        switch(tasto)
        {
          case 1:
            /* Relè -> telecomando per programma utente */
            if((radio_telecomando[i].mask & (1<<3)) &&
               ((radio_zone[3] & FUNC_MASK) == FUNC_TLC))
            {
              i = radio_zone[3] & ~FUNC_MASK;
              database_set_alarm2(&ME[i]);
            }
            break;
          case 2:
            /* Panico */
            break;
          case 3:
            /* Interrogazione */
            break;
          case 4:
            /* Disattiva totale */
          case 5:
            /* Disattiva A */
          case 6:
            /* Disattiva B */
            if((radio_telecomando[i].mask & (1<<(tasto-4))) &&
               ((radio_zone[tasto-4] & FUNC_MASK) == FUNC_ZTOGGLE))
            {
              i = radio_zone[(tasto-4)] & ~FUNC_MASK;
              cmd_zone_off(i, 0);
            }
            break;
          case 7:
            /* Attiva totale */
          case 8:
            /* Attiva A */
          case 9:
            /* Attiva B */
            if((radio_telecomando[i].mask & (1<<(tasto-7))) &&
               ((radio_zone[tasto-7] & FUNC_MASK) == FUNC_ZTOGGLE))
            {
              i = radio_zone[(tasto-7)] & ~FUNC_MASK;
              /* Attiva con fuori servizio */
              cmd_zone_on(i, 2);
            }
            break;
          default:
            break;
        }
        
        if(tasto >= 3)
        {
          /* Aggiorna lo stato led */
          radio_timer_led_tlc9T[(ind>>5)*4+((ind&0x10)>>3)+(((ind&0xf)==2)?0:1)] = 30;
        }
        else
        {
          /* Spegne i led di zona */
          radio_timer_led_tlc9T[(ind>>5)*4+(ind&0x10)>>3+(((ind&0xf)==2)?0:1)] = 0;
        }
      }
    }
  }
  else if(tasto == 13)
  {
    /* Pressione prolungata tasto telecomando 9T */
  }
  /* Cancella il codice ricevuto */
  SE[(ind<<3)+3] = 0;
  
  /* FINE MACRO */
  if(!radio_timeout->active) timeout_on(radio_timeout, NULL, NULL, 0, 0);
}

void RadioSirena(int ind, int att)
{
  /*
     La sirena è attiva se è vera la condizione esterna ma anche
     se l'impianto non è in manutenzione. In manutenzione disabilito
     automaticamente le sirene.
  */
//  EQAT(ind, att && !AL(ME[1858]));
  
  /* Ci pensa già la sirena a non suonare se l'impianto è in manutezione */
  EQAT(ind, att);
}

int RadioBatteriaBassa(int ind)
{
  if(SE[ind] & bitBattery) return 1;
  return 0;
}

/***********************************************
   GESTIONE RADIO 868
***********************************************/

/* La funzione "tobase34" è replicata in radio.c e toolweb.c.
   Non voglio dipendere dal plugin toolweb per l'associazione
   dei sensori e relativa descrizione. */
static const char alphabet[] = "0123456789ABCDEFGHJKLMNPQRSTUVWXYZ";
void tobase34(int num, char *text, int len)
{
  text[len] = 0;
  while(len)
  {
    if((num&0x3f) < (sizeof(alphabet)-1))
    {
      text[len-1] = alphabet[num&0x3f];
      num >>= 6;
      len--;
    }
    else
    {
      text[0] = 0;
      return;
    }
  }
}

void radio868_mm(int mm, unsigned char *msg)
{
  unsigned char cmd[12];
  int i, conc, id, ind, flags;
  
  /* Carica le anagrafiche */
  radio_init();
  
  /* Attenzione: per errore è stato dimensionato l'SC8R2 per
     16 linee e non per 16 moduli master (32 linee).
     L'utilizzo dell'SC8R2 rimane quindi limitato ai primi 8 moduli. */
  if(mm < 8)
  {
    switch(msg[0])
    {
      case 4:
        if(TipoPeriferica[mm*32+msg[1]] == SC8R2)
        {
          /* Azzero i rilevamenti e soprattutto le candidature
             delle periferiche per il concentratore manomesso.
             Le periferiche candidate non verrebbero più segnalate
             dagli altri concentratori a meno che il segnale non
             risulti poi migliore. Ma fossero state forzate allora
             nemmeno in quel caso. */
          conc = (mm<<2)+((msg[1]&0x10)>>3)+((msg[1]&0x0f)==2?0:1);
          memset(radio868_id_stato[conc], 0, sizeof(radio868_id_stato[conc]));
          memset(radio868_map[conc], 0, sizeof(radio868_map[conc]));
        }
        break;
      case 5:
        if(TipoPeriferica[mm*32+msg[1]] == SC8R2)
        {
          /* Invio lo UniqueID (MAC address) */
          cmd[0] = 40;
          cmd[1] = msg[1];
          /* Differenzio lo uniqueid per ogni modulo master.
             Per ogni uniqueid si possono gestire al massimo
             4 concentratori, quindi un modulo master. */
          i = master_SC8R2_uniqueid + (mm<<8);
          memcpy(cmd+2, &i, 4);
          //cmd[6] = 0x10|(mm*2)|(msg[1]/16);
          cmd[6] = 0x10|((msg[1]/16)&1);
          cmd[7] = 0;
          cmd[8] = 0;
          master_queue(mm, cmd);
          /* Richiedo la lista periferiche */
          cmd[0] = 39;
          cmd[1] = msg[1];
          cmd[2] = 0;
          master_queue(mm, cmd);
          radio868_autoappr_flags &= ~(RADIO_AUTOAPPR_DYN|RADIO_AUTOAPPR_SEND);
          /* Prenoto la programmazione di tutte le periferiche radio */
          conc = (mm<<2)+((msg[1]&0x10)>>3)+((msg[1]&0x0f)==2?0:1);
          cmd[0] = cmd[1] = cmd[2] = 0;
          for(i=0; i<MAX_AUTO_SENS; i++)
          {
            if(radio868_id.sens[conc][i].id)
            {
              cmd[3] = 1;	// gest - può essere sovrascritto
              mmaster_auto_register(conc, radio868_id.sens[conc][i].id, cmd);
              
              radio868_id_stato[conc][i].stato = AUTOAPP_ADD;
              timeout_on(radio868_id_delay, radio868_idprog_to, NULL, 0, AUTOAPP_DELAYED_SAVE);
            }
          }
          /* Richiedo l'allineamento stati periferiche */
          cmd[0] = 7;
          cmd[1] = msg[1];
          master_queue(mm, cmd);
        }
        break;
      case 28:
        /* Segnalazione anomalia da periferica radio */
        /* 1C0Xiiiiiiii0tss
           Vale solo l'ID univoco, l'indirizzo del concentratore è superfluo.
           Il tipo periferica è utile per una immediata decodifica (non c'è bisogno
           di memorizzarlo o ricavarlo dall'ID), lo stato è complessivo. */
        conc = (mm<<2)+((msg[1]&0x10)>>3)+((msg[1]&0x0f)==2?0:1);
        id = (msg[5]<<24)+(msg[4]<<16)+(msg[3]<<8)+msg[2];
        
        /* Batteria bassa */
        /* stbatt: bit0,segnalazione storico; bit1,segnalazione inviata
           cntbatt: segnalazione in corso e conteggio */
        for(i=0; i<MAX_AUTO_SENS; i++)
        {
          if((radio868_id.sens[conc][i].id == id) &&
             (radio868_id_stato[conc][i].stato < AUTOAPP_DELETE))	// non è richiesta la cancellazione
          {
            radio868_id.sens[conc][i].tipo = msg[6];
            
            ind = radio868_id.sens[conc][i].sensore;
            if(msg[7]&8)
            {
              /* Batteria bassa */
              /* Innesco verifica batteria */
              if(!radio868_id_stato[conc][i].cntbatt)
                radio868_id_stato[conc][i].cntbatt = 1;	// in corso
              if(!radio868_batteria_timer[conc])
                radio868_batteria_timer[conc] = BATTERIA868_TIMER;

#warning Gestione della segnalazione corrente del batteria bassa
#if 1
              switch(msg[6])
              {
                case RADIO_TIPO_CONTATTO:
                case RADIO_TIPO_IR:
                  /* Sulla Delphi gli attuatori e le sirene vengono comunque mappati
                     sui sensori fisici della centrale così da uniformarne la gestione. */
                case RADIO_TIPO_ATTUATORE:
                case RADIO_TIPO_SIRENA_EXT:
                case RADIO_TIPO_SIRENA_INT:
                  if(!(SE[(conc>>1)*128+radio868_id.sens[conc][i].sensore] & bitBattery))
                  {
                    SE[(conc>>1)*128+radio868_id.sens[conc][i].sensore] |= bitBattery;
                  }
                  break;
                default:
                  break;
              }
#endif
            }
            else
            {
              radio868_id_stato[conc][i].stbatt &= ~1;	// ripristino batteria (mantengo inviata)
              radio868_id_stato[conc][i].cntbatt = 0;
              /* Lo stato di batteria viene aggiornato anche nello stato sensori/attuatori/sirene */
              switch(msg[6])
              {
                case RADIO_TIPO_CONTATTO:
                case RADIO_TIPO_IR:
#if 1
                  /* Sulla Delphi gli attuatori e le sirene vengono comunque mappati
                     sui sensori fisici della centrale così da uniformarne la gestione. */
                case RADIO_TIPO_ATTUATORE:
                case RADIO_TIPO_SIRENA_EXT:
                case RADIO_TIPO_SIRENA_INT:
#endif
                  if(SE[(conc>>1)*128+radio868_id.sens[conc][i].sensore] & bitBattery)
                  {
                    SE[(conc>>1)*128+radio868_id.sens[conc][i].sensore] &= ~bitBattery;
                  }
                  break;
                default:
                  break;
              }
            }
            break;
          }
        }
        
        switch(msg[6])
        {
          case RADIO_TIPO_CONTATTO:	// contatto radio
          case RADIO_TIPO_IR:	// infrarosso (secondo allarme fisso a 0, mai associato)
            /* Sensore mappato per il primo ingresso */
            ind = mmaster_sensore_id(conc, id, 0, &flags);
            if(ind >= 0)
            {
#if 0
              if(((config.flags & CONFIG_FLAG_RADIO_GUASTI) && (line[linea].msg_data[10]&0x20)) ||
                 (!(config.flags & CONFIG_FLAG_RADIO_GUASTI) && (line[linea].msg_data[10]&0x40)))
#else
              /* Gestione guasto 1h */
              if(msg[7] & RADIO868_GUASTO)
#endif
              {
                /* Con il filtro guasto attivo non solo non segnalo il
                   guasto ma mantengo inalterate anche le altre eventuali
                   anomalie in corso per evitare che vadano e vengano a
                   causa della mancata comunicazione. */
                if(!(flags&RADIO868_FLAGS_NO_GUASTO))
                {
                  /* Guasto periferica */
                  
                  /* La segnalazione di guasto dal concentratore mantiene lo stato
                     di batteria bassa. Se gestisco il guasto, la batteria bassa
                     deve andare via. */
                  SE[(conc>>1)*128+ind] &= ~bitBattery;
                  
                  for(i=0; i<MAX_AUTO_SENS; i++)
                  {
                    if((radio868_id.sens[conc][i].id == (unsigned int)id) &&
                       (radio868_id_stato[conc][i].stato < AUTOAPP_DELETE))	// non è richiesta la cancellazione
                    {
                      /* Non segnalo il ripristino della batteria in caso di guasto periferica,
                         aspetto la segnalazione esplicita. Azzero solo il contatore. */
                      radio868_id_stato[conc][i].cntbatt = 0;
                    }
                  }
                  
                  master_set_sabotage((conc>>1)*128+ind, 0, Manomissione_Dispositivo);
                  master_set_alarm((conc>>1)*128+ind, 0, 1);
                  master_set_failure((conc>>1)*128+ind, 1, Guasto_Sensore);
                }
              }
              else
              {
                /* Fine guasto */
                master_set_failure((conc>>1)*128+ind, 0, Guasto_Sensore);
                /* Manomissione contenitore */
                master_set_sabotage((conc>>1)*128+ind, (msg[7]&4)?1:0, Manomissione_Dispositivo);
                /* Elimino la segnalazione di manomissione se è già gestita, per non gestirla
                   due volte. Se non è configurato il primo ingresso, la manomissione viene
                   gestita dal secondo ingresso, altrimenti solo dal primo. */
                msg[7] &= ~4;
                /* Allarme ingresso 1 */
                master_set_alarm((conc>>1)*128+ind, (msg[7]&1)?1:0, 1);
              }
            }
            /* Sensore mappato per il secondo ingresso */
            ind = mmaster_sensore_id(conc, id, 1, &flags);
            if(ind >= 0)
            {
#if 0
              if(((config.flags & CONFIG_FLAG_RADIO_GUASTI) && (line[linea].msg_data[10]&0x20)) ||
                 (!(config.flags & CONFIG_FLAG_RADIO_GUASTI) && (line[linea].msg_data[10]&0x40)))
#else
              if(msg[7] & RADIO868_GUASTO)
#endif
              {
                /* Con il filtro guasto attivo non solo non segnalo il
                   guasto ma mantengo inalterate anche le altre eventuali
                   anomalie in corso per evitare che vadano e vengano a
                   causa della mancata comunicazione. */
                if(!(flags&RADIO868_FLAGS_NO_GUASTO))
                {
                  /* Guasto periferica */
                  
                  /* La segnalazione di guasto dal concentratore mantiene lo stato
                     di batteria bassa. Se gestisco il guasto, la batteria bassa
                     deve andare via. */
                  SE[(conc>>1)*128+ind] &= ~bitBattery;
                  
                  for(i=0; i<MAX_AUTO_SENS; i++)
                  {
                    if((radio868_id.sens[conc][i].id == (unsigned int)id) &&
                       (radio868_id_stato[conc][i].stato < AUTOAPP_DELETE))	// non è richiesta la cancellazione
                    {
                      /* Non segnalo il ripristino della batteria in caso di guasto periferica,
                         aspetto la segnalazione esplicita. Azzero solo il contatore. */
                      radio868_id_stato[conc][i].cntbatt = 0;
                    }
                  }
                  
                  master_set_sabotage((conc>>1)*128+ind, 0, Manomissione_Dispositivo);
                  master_set_alarm((conc>>1)*128+ind, 0, 1);
                  master_set_failure((conc>>1)*128+ind, 1, Guasto_Sensore);
                }
              }
              else
              {
                /* Fine guasto */
                master_set_failure((conc>>1)*128+ind, 0, Guasto_Sensore);
                /* Manomissione contenitore (se non gestito dal primo ingresso) */
                master_set_sabotage((conc>>1)*128+ind, (msg[7]&4)?1:0, Manomissione_Dispositivo);
                /* Allarme ingresso 2 */
                master_set_alarm((conc>>1)*128+ind, (msg[7]&2)?1:0, 1);
              }
            }
            break;
          case RADIO_TIPO_ATTUATORE:
          case RADIO_TIPO_SIRENA_EXT:
          case RADIO_TIPO_SIRENA_INT:
            /* Manomissione contenitore / manomissione periferica */
            ind = mmaster_sensore_id(conc, id, 127, &flags);
            if(ind >= 0)
            {
#if 0
              if(((config.flags & CONFIG_FLAG_RADIO_GUASTI) && (line[linea].msg_data[10]&0x20)) ||
                 (!(config.flags & CONFIG_FLAG_RADIO_GUASTI) && (line[linea].msg_data[10]&0x40)))
#else
              /* Gestione guasto 1h */
              if(msg[7] & RADIO868_GUASTO)
#endif
              {
                /* Con il filtro guasto attivo non solo non segnalo il
                   guasto ma mantengo inalterate anche le altre eventuali
                   anomalie in corso per evitare che vadano e vengano a
                   causa della mancata comunicazione. */
                if(!(flags&RADIO868_FLAGS_NO_GUASTO))
                {
                  /* Guasto periferica */
                  
                  /* La segnalazione di guasto dal concentratore mantiene lo stato
                     di batteria bassa. Se gestisco il guasto, la batteria bassa
                     deve andare via. */
                  SE[(conc>>1)*128+ind] &= ~bitBattery;
                  
                  for(i=0; i<MAX_AUTO_SENS; i++)
                  {
                    if((radio868_id.sens[conc][i].id == (unsigned int)id) &&
                       (radio868_id_stato[conc][i].stato < AUTOAPP_DELETE))	// non è richiesta la cancellazione
                    {
                      /* Non segnalo il ripristino della batteria in caso di guasto periferica,
                         aspetto la segnalazione esplicita. Azzero solo il contatore. */
                      radio868_id_stato[conc][i].cntbatt = 0;
                    }
                  }
                  
                  master_set_sabotage((conc>>1)*128+ind, 0, Manomissione_Dispositivo);
                  master_set_failure((conc>>1)*128+ind, 1, Guasto_Sensore);
                }
              }
              else
              {
                /* Fine guasto */
                master_set_failure((conc>>1)*128+ind, 0, Guasto_Sensore);
                /* Manomissione contenitore */
                master_set_sabotage((conc>>1)*128+ind, (msg[7]&4)?1:0, Manomissione_Dispositivo);
              }
            }
            break;
          default:
            break;
        }
        break;
      case 29:
        /* Rilevata periferica radio */
        /* 1D0Xiiiiiiii0trrbb0g
           Inoltro dei dati al configuratore, che dovrà associare l'ID univoco
           al concentratore di riferimento (basandosi su RSSI), ed ogni ingresso
           ad un sensore virtuale. */
        conc = (mm<<2)+((msg[1]&0x10)>>3)+((msg[1]&0x0f)==2?0:1);
        id = (msg[5]<<24)+(msg[4]<<16)+(msg[3]<<8)+msg[2];
        
        if((msg[9] < 2))
        {
          /* Se sono in corso di aggiornamento periferiche, attendo che la lista sia conclusa
             prima di iniziare a programmare le periferiche mancanti. */
          if(radio868_id_delay->active)
            timeout_on(radio868_id_delay, radio868_idprog_to, NULL, 0, AUTOAPP_DELAYED_SAVE);
          
          /* Segna la periferica rilevata per gestione futura */
          if(mmaster_auto_register(conc, id, msg+6))
            radio868_to_toolweb(conc, id, msg+6);
        }
        else if(msg[9] == 2)
        {
          /* Parametri periferica */
/*
...
R0:1D02870000000100FF02
R0:1D02870000000101FF02
R0:1D02870000000102FF02
R0:1D02870000000103FF02
R0:1D028700000001FFFF02

*/
          if(!memcmp(msg+2, &radio868_param_id, 4))
          {
            if(msg[7] != 0xff)
            {
              static const char TipoParam[8][8] = {
                {-1,-1,-1,-1,-1,-1,-1,-1},	// nessuna
                {1,0,5,6,-1,-1,-1,-1},	// contatto
                //{11,7,8,-1,-1,-1,-1,-1},	// infrarosso
                {11,-1,-1,-1,-1,-1,-1,-1},	// infrarosso (solo sensibilità)
                /* L'attuatore ha 5 parametri, ma l'utente configura solo il primo,
                   gli altri sono gestiti automaticamente (CmdA e CmdB per attuazioni
                   temporizzate). */
                {9,-1,-1,-1,-1,-1,-1,-1},	// attuatore
                {10,-1,-1,-1,-1,-1,-1,-1},	// sirena esterna
                {10,-1,-1,-1,-1,-1,-1,-1},	// sirena interna
                {-1,-1,-1,-1,-1,-1,-1,-1},	// tipo 6
                {-1,-1,-1,-1,-1,-1,-1,-1},	// tipo 7
                };
              radio868_param_list[msg[7]][0] = TipoParam[msg[6]][msg[7]]; // tipo
              if(radio868_param_list[msg[7]][0] != 0xff)
                radio868_param_list[msg[7]][1] = msg[8]; // val
            }
            else
            {
              /* Fine lista parametri, inoltro al destinatario */
              if(AutoParam_in_corso == 1)
              {
                if(!json_notify_param_p) json_notify_param_p = dlsym(NULL, "json_notify_param");
                if(json_notify_param_p) json_notify_param_p(radio868_param_id, msg[6], radio868_param_list);
              }
              AutoParam_in_corso = 0;
            }
          }
        }
        else if(msg[9] & 0x80)
        {
          /* Parametri diagnostici */
/*
...
R0:1D020200000503000086
R0:1D020200000503006287
R0:1D020200000503000088
R0:1D020200000503000089
R0:1D0202000005030D588A
R0:1D020200000503FFFF8F
*/
#warning Inoltrare al toolweb
#if 0
          if(debug_enable == DEBUG_RADIO)
          {
            debug_buf[debug_in++] = 143;
            debug_buf[debug_in++] = 0;
            debug_buf[debug_in++] = 8;	// len
            debug_buf[debug_in++] = 0;
            debug_buf[debug_in++] = conc;
            for(i=2; i<6; i++) debug_buf[debug_in++] = msg[i];
            debug_buf[debug_in++] = msg[9] & 0x0f;
            debug_buf[debug_in++] = msg[8];
            debug_buf[debug_in++] = msg[7];
          }
#endif
        }
        break;
      default:
        break;
    }
  }
}

void radio868_list_auto_single(int refresh, int mm, int nconc)
{
  int conc, c1, c2, i, j, s;
  unsigned char msg[4];
  
  if(refresh < 2)
  {
    /* Occorre inviare tutte le periferiche programmate in anagrafica
       e poi eseguire il comando sul bus. */
    if(nconc >= 0)
    {
      /* Solo concentratore indicato */
      c1 = mm*4 + nconc;
      c2 = c1 + 1;
    }
    else
    {
      /* Tutti e 4 i concentratori di un modulo master */
      c1 = mm*4;
      c2 = mm*4 + 4;
    }
    
    for(conc=c1; conc<c2; conc++)
      for(i=0; i<MAX_AUTO_SENS; i++)
      {
        radio868_map[conc][i].prog.sens[0] = radio868_map[conc][i].prog.sens[1] = -1;
        radio868_map[conc][i].prog.mod = 0;
      }
    /* Segna i sensori bloccati dalle periferiche filari */
    /* Per comodità di Sprint viene definito un nuovo tipo di periferica virtuale
       con codice 0xF, per cui il campo la considera come non esistente ma oggettivamente
       esiste e su questa periferica vengono fatti affluire i sensori 868 programmabili.
       In questo modo si scongiura anche la possibilità di programmare sensori radio 868
       su sensori che in un secondo momento vengono occupati da una periferica filare
       aggiuntiva. */
    memset(SEprog, 0, sizeof(SEprog));
    for(i=mm*256; i<(mm+1)*256; i++)
    {
      if(TipoPeriferica[i/8] != 0xf)
        SEprog[i/128][(i&0x7f)/32] |= (1<<(i&31));
    }
    
    for(conc=c1; conc<c2; conc++)
    {
      for(i=0; i<MAX_AUTO_SENS; i++)
        if(radio868_id.sens[conc][i].id && (radio868_id_stato[conc][i].stato != AUTOAPP_DELETE))
        {
          /* Cerco il tipo in radio_map */
          msg[0] = 0;
          for(j=0; (j<MAX_AUTO_SENS)&&(radio868_map[conc][j].id!=radio868_id.sens[conc][i].id); j++);
          if(j < MAX_AUTO_SENS)
          {
            msg[0] = radio868_map[conc][j].tipo;
            
            /* Popolo le associazioni */
            switch(msg[0])
            {
              case RADIO_TIPO_CONTATTO:
              case RADIO_TIPO_IR:
                radio868_map[conc][j].prog.sens[radio868_id.sens[conc][i].input] =
                  radio868_id.sens[conc][i].sensore;
                SEprog[conc>>1][radio868_id.sens[conc][i].sensore/32] |=
                  (1<<(radio868_id.sens[conc][i].sensore&31));
                break;
              case RADIO_TIPO_ATTUATORE:
              case RADIO_TIPO_SIRENA_INT:
              case RADIO_TIPO_SIRENA_EXT:
                if(radio868_id.sens[conc][i].input == 127)
                {
                  radio868_map[conc][j].prog.sens[0] =
                    radio868_id.sens[conc][i].sensore;
                  SEprog[conc>>1][radio868_id.sens[conc][i].sensore/32] |=
                    (1<<(radio868_id.sens[conc][i].sensore&31));
                }
                break;
              default:
                break;
            }
          }
          msg[1] = 0;
          msg[2] = 0;
          msg[3] = 1;
          radio868_to_toolweb(conc, radio868_id.sens[conc][i].id, msg);
        }
    }
  }
  
  if(refresh == 1)
  {
#warning Bloccare apprendimento se impianto attivo
  }
  
  /* Invio il comando sul bus */
  for(conc=c1; conc<c2; conc++)
  {
    if((TipoPeriferica[(conc>>2)*32+(conc&2)*8+2+(conc&1)*5] == SC8R2) &&
       (StatoPeriferica[(conc>>2)*4+(conc&2)] & (1<<(2+(conc&1)*5))))
    {
      radio868_autoappr_flags &= ~(RADIO_AUTOAPPR_DYN|RADIO_AUTOAPPR_SEND);
      if(refresh < 2)
      {
        radio868_autoappr_flags |= RADIO_AUTOAPPR_SEND;
        if(refresh)
          radio868_autoappr_flags |= RADIO_AUTOAPPR_DYN;
      }
      
      /* Il refresh 2 serve a chiudere l'apprendimento. */
      if(refresh == 2) refresh = 0;
      
      msg[0] = 39;
      msg[1] = (conc&2)*8+2+(conc&1)*5;
      msg[2] = refresh;
      master_queue((conc>>2), msg);
    }
  }
  
  if(!json_refresh_map_p)
    json_refresh_map_p = dlsym(NULL, "json_refresh_map");
  if(json_refresh_map_p)
    json_refresh_map_p(NULL, 5);
}

void radio868_list_auto(int refresh)
{
  int conc, i, j, s;
  unsigned char msg[4];
  
  if(refresh < 2)
  {
    /* Occorre inviare tutte le periferiche programmate in anagrafica
       e poi eseguire il comando sul bus. */
    
    for(conc=0; conc<N_LINEE*2; conc++)
      for(i=0; i<MAX_AUTO_SENS; i++)
      {
        radio868_map[conc][i].prog.sens[0] = radio868_map[conc][i].prog.sens[1] = -1;
        radio868_map[conc][i].prog.mod = 0;
      }
    /* Segna i sensori bloccati dalle periferiche filari */
    /* Per comodità di Sprint viene definito un nuovo tipo di periferica virtuale
       con codice 0xF, per cui il campo la considera come non esistente ma oggettivamente
       esiste e su questa periferica vengono fatti affluire i sensori 868 programmabili.
       In questo modo si scongiura anche la possibilità di programmare sensori radio 868
       su sensori che in un secondo momento vengono occupati da una periferica filare
       aggiuntiva. */
    memset(SEprog, 0, sizeof(SEprog));
    for(i=0; i<N_LINEE*128; i++)
    {
      //if(TipoPeriferica[i/8] < 0xf)
      if(TipoPeriferica[i/8] != 0xf)
        SEprog[i/128][(i&0x7f)/32] |= (1<<(i&31));
    }
    
    for(conc=0; conc<N_LINEE*2; conc++)
    {
      for(i=0; i<MAX_AUTO_SENS; i++)
        if(radio868_id.sens[conc][i].id && (radio868_id_stato[conc][i].stato != AUTOAPP_DELETE))
        {
          /* Cerco il tipo in radio_map */
          msg[0] = 0;
          for(j=0; (j<MAX_AUTO_SENS)&&(radio868_map[conc][j].id!=radio868_id.sens[conc][i].id); j++);
          if(j < MAX_AUTO_SENS)
          {
            msg[0] = radio868_map[conc][j].tipo;
            
            /* Popolo le associazioni */
            switch(msg[0])
            {
              case RADIO_TIPO_CONTATTO:
              case RADIO_TIPO_IR:
                radio868_map[conc][j].prog.sens[radio868_id.sens[conc][i].input] =
                  radio868_id.sens[conc][i].sensore;
                SEprog[conc>>1][radio868_id.sens[conc][i].sensore/32] |=
                  (1<<(radio868_id.sens[conc][i].sensore&31));
                break;
              case RADIO_TIPO_ATTUATORE:
              case RADIO_TIPO_SIRENA_INT:
              case RADIO_TIPO_SIRENA_EXT:
                if(radio868_id.sens[conc][i].input == 127)
                {
                  radio868_map[conc][j].prog.sens[0] =
                    radio868_id.sens[conc][i].sensore;
                  SEprog[conc>>1][radio868_id.sens[conc][i].sensore/32] |=
                    (1<<(radio868_id.sens[conc][i].sensore&31));
                }
                break;
              default:
                break;
            }
          }
          msg[1] = 0;
          msg[2] = 0;
          msg[3] = 1;
          radio868_to_toolweb(conc, radio868_id.sens[conc][i].id, msg);
        }
    }
  }
  
  if(refresh == 1)
  {
#warning Bloccare apprendimento se impianto attivo
  }
  
  /* Invio il comando sul bus */
  for(conc=0; conc<N_LINEE*2; conc++)
  {
    if((TipoPeriferica[(conc>>2)*32+(conc&2)*8+2+(conc&1)*5] == SC8R2) &&
       (StatoPeriferica[(conc>>2)*4+(conc&2)] & (1<<(2+(conc&1)*5))))
    {
      radio868_autoappr_flags &= ~(RADIO_AUTOAPPR_DYN|RADIO_AUTOAPPR_SEND);
      if(refresh < 2)
      {
        radio868_autoappr_flags |= RADIO_AUTOAPPR_SEND;
        if(refresh)
          radio868_autoappr_flags |= RADIO_AUTOAPPR_DYN;
      }
      
      /* Il refresh 2 serve a chiudere l'apprendimento. */
      if(refresh == 2) refresh = 0;
      
      msg[0] = 39;
      msg[1] = (conc&2)*8+2+(conc&1)*5;
      msg[2] = refresh;
      master_queue((conc>>2), msg);
    }
  }
  
  if(!json_refresh_map_p)
    json_refresh_map_p = dlsym(NULL, "json_refresh_map");
  if(json_refresh_map_p)
    json_refresh_map_p(NULL, 5);
}

void radio868_list_send()
{
  int conc, i;
  unsigned char data[4];
  
  for(conc=0; conc<N_LINEE*2; conc++)
    for(i=0; i<MAX_AUTO_SENS; i++)
      if(radio868_map[conc][i].prog.mod)
      {
        radio868_map[conc][i].prog.mod = 0;
        if((radio868_map[conc][i].prog.sens[0] < 0) && (radio868_map[conc][i].prog.sens[1] < 0))
        {
          radio868_auto_associa_sens(conc, radio868_map[conc][i].id, -1, -1, 0);
          
          if(radio868_map[conc][i].prog.managed)
          {
            radio868_map[conc][i].prog.managed = 0;
            data[0] = radio868_map[conc][i].tipo;
            data[1] = radio868_map[conc][i].rssi;
            data[2] = radio868_map[conc][i].batt;
            data[3] = 0;
            radio868_to_toolweb(conc, radio868_map[conc][i].id, data);
          }
        }
        else
        {
          switch(radio868_map[conc][i].tipo)
          {
            case RADIO_TIPO_CONTATTO:
              radio868_auto_associa_sens(conc, radio868_map[conc][i].id, 0, radio868_map[conc][i].prog.sens[0], radio868_map[conc][i].prog.flags);
              radio868_auto_associa_sens(conc, radio868_map[conc][i].id, 1, radio868_map[conc][i].prog.sens[1], radio868_map[conc][i].prog.flags);
              break;
            case RADIO_TIPO_IR:
              radio868_auto_associa_sens(conc, radio868_map[conc][i].id, 0, radio868_map[conc][i].prog.sens[0], radio868_map[conc][i].prog.flags);
              break;
            default:
              break;
          }
          
          if(!radio868_map[conc][i].prog.managed)
          {
            radio868_map[conc][i].prog.managed = 1;
            data[0] = radio868_map[conc][i].tipo;
            data[1] = radio868_map[conc][i].rssi;
            data[2] = radio868_map[conc][i].batt;
            data[3] = 1;
            radio868_to_toolweb(conc, radio868_map[conc][i].id, data);
          }
        }
      }
  
  radio_save();
}

int radio868_auto_seleziona_sensore(int input, int sel, int zona, char *descr)
{
  int i, n;
  short *p;
  char buf[8];
  
  for(i=0; (i<128)&&(SEprog[sel/MAX_AUTO_SENS/2][i/32]&(1<<(i&31))); i++);
  if(i<128)
  {
    SEprog[sel/MAX_AUTO_SENS/2][i/32] |= (1<<(i&31));
    
    radio868_auto_associa_sens(sel/MAX_AUTO_SENS,
      radio868_map[sel/MAX_AUTO_SENS][sel%MAX_AUTO_SENS].id, input, i, 0);
    
#if 0
    if(zona && (zona <= n_ZS))
    {
      Zona_SE[(sel/MAX_AUTO_SENS/2)*128+i] = zona;
      
      for(n=0; SE_ZONA[zona][n]!=-1; n++);
      p = malloc((n+2)*sizeof(short));
      memcpy(p, SE_ZONA[zona], n*sizeof(short));
      p[n] = i;
      p[n+1] = -1;
      free(SE_ZONA[zona]);
      SE_ZONA[zona] = p;
    }
#endif
    
    sprintf(buf, "%04d", (sel/MAX_AUTO_SENS/2)*128+i);
    if(!strcmp(SensorName[(sel/MAX_AUTO_SENS/2)*128+i], buf))
    {
      free(SensorName[(sel/MAX_AUTO_SENS/2)*128+i]);
      n = strlen(descr);
      SensorName[(sel/MAX_AUTO_SENS/2)*128+i] = malloc(n+4+1);
      strcpy(SensorName[(sel/MAX_AUTO_SENS/2)*128+i], descr);
      tobase34(radio868_map[sel/MAX_AUTO_SENS][sel%MAX_AUTO_SENS].id&0xffffff,
        SensorName[(sel/MAX_AUTO_SENS/2)*128+i]+n, 4);
    }
    return 1;
  }
  return 0;
}

void radio868_auto_associa_periferiche()
{
  int idx, i, s;
  unsigned int ATmappato[N_LINEE*2][(N_RADIO_ACT+31)/32];
  unsigned int SIRmappato[N_LINEE*2][(N_RADIO_SIR+31)/32];
  //char rssi_cur, rssi_ref;

  /* Inizializza la maschera attuatori/sensori mappati */
  memset(ATmappato, 0, sizeof(ATmappato));
  memset(SIRmappato, 0, sizeof(SIRmappato));
  for(i=0; i<N_LINEE*2; i++)
  {
    /* Per gli attuatori radio ho già una mappatura base
       in mmaster_SC8R_att_lut. */
    for(idx=0; idx<N_RADIO_ACT; idx++)
    {
//      if(mmaster_SC8R_att_lut[i][idx].id)
//        ATmappato[i][idx/32] |= (1<<(idx&31));
    }
    /* Per le sirene invece devo scandire tutta la lista
       delle periferiche radio registrate. */
    for(idx=0; idx<MAX_AUTO_SENS; idx++)
    {
      if((radio868_map[i][idx].tipo == RADIO_TIPO_SIRENA_EXT) ||
         (radio868_map[i][idx].tipo == RADIO_TIPO_SIRENA_INT))
      {
        s = mmaster_sensore_id(i, radio868_map[i][idx].id, 127, NULL);
        if(s >= 0) SIRmappato[i][s/32] |= (1<<(s&31));
      }
    }
  }
  
  for(idx=0; idx<(N_LINEE*2*MAX_AUTO_SENS); idx++)
  {
    if(!radio868_map[idx/MAX_AUTO_SENS][idx%MAX_AUTO_SENS].id) continue;
    
    /* La nuova procedura di acquisizione seleziona fin dall'inizio il candidato
       e lo segna con un apposito flag. */
    if(!(radio868_map[idx/MAX_AUTO_SENS][idx%MAX_AUTO_SENS].flag & RADIO_MAP_FLAG_CAND) ||
       (radio868_map[idx/MAX_AUTO_SENS][idx%MAX_AUTO_SENS].flag & (RADIO_MAP_FLAG_GEST|RADIO_MAP_FLAG_PROG))) continue;
    
    switch(radio868_map[idx/MAX_AUTO_SENS][idx%MAX_AUTO_SENS].tipo)
    {
      case 1:	// contatto
        s = radio868_auto_seleziona_sensore(0, idx, 2, "CM ");
        s |= radio868_auto_seleziona_sensore(1, idx, 3, "AUX ");
        if(s)
        {
          radio868_map[idx/MAX_AUTO_SENS][idx%MAX_AUTO_SENS].flag = RADIO_MAP_FLAG_GEST|RADIO_MAP_FLAG_CAND|RADIO_MAP_FLAG_FORCED;
          radio868_map[idx/MAX_AUTO_SENS][idx%MAX_AUTO_SENS].prog.managed = 1;
        }
        break;
      case 2:	// infrarosso
        if(radio868_auto_seleziona_sensore(0, idx, 1, "IR "))
        {
          radio868_map[idx/MAX_AUTO_SENS][idx%MAX_AUTO_SENS].flag = RADIO_MAP_FLAG_GEST|RADIO_MAP_FLAG_CAND|RADIO_MAP_FLAG_FORCED;
          radio868_map[idx/MAX_AUTO_SENS][idx%MAX_AUTO_SENS].prog.managed = 1;
        }
        break;
#if 0
      case 3:	// attuatore
        for(i=0; (i<N_RADIO_ACT)&&(ATmappato[idx/MAX_AUTO_SENS][i/32]&(1<<(i&31))); i++);
        if(i<N_RADIO_ACT)
        {
          ATmappato[idx/MAX_AUTO_SENS][i/32] |= (1<<(i&31));
          radio868_auto_associa_sens(idx/MAX_AUTO_SENS,
            radio868_map[idx/MAX_AUTO_SENS][idx%MAX_AUTO_SENS].id, 127, i, 0);
          /* Il relè lo deve ovviamente associare esplicitamente
             l'installatore.
          radio868_auto_associa_sens(idx/MAX_AUTO_SENS,
            radio868_map[idx/MAX_AUTO_SENS][idx%MAX_AUTO_SENS].id, 128, ???, 0);
          */
          radio868_map[idx/MAX_AUTO_SENS][idx%MAX_AUTO_SENS].flag = RADIO_MAP_FLAG_GEST|RADIO_MAP_FLAG_CAND|RADIO_MAP_FLAG_FORCED;
        }
        break;
      case 4:	// sirena esterna
      case 5:	// sirena interna
        for(i=0; (i<N_RADIO_SIR)&&(SIRmappato[idx/MAX_AUTO_SENS][i/32]&(1<<(i&31))); i++);
        //if(i<N_RADIO_SIR)
        if(i<4)
        {
          SIRmappato[idx/MAX_AUTO_SENS][i/32] |= (1<<(i&31));
          radio868_auto_associa_sens(idx/MAX_AUTO_SENS,
            radio868_map[idx/MAX_AUTO_SENS][idx%MAX_AUTO_SENS].id, 127, i, 0);
          radio868_map[idx/MAX_AUTO_SENS][idx%MAX_AUTO_SENS].flag = RADIO_MAP_FLAG_GEST|RADIO_MAP_FLAG_CAND|RADIO_MAP_FLAG_FORCED;
        }
        break;
#endif
      default:
        /* Sconosciuta, non assegno nulla */
        break;
    }
  }
  
  radio_save();
  radio_string_save();
}

int radio868_param_req(int id)
{
  int i, conc;
  unsigned char cmd[12];
  
  if(AutoParam_in_corso)
  {
    if(!json_alert_p) json_alert_p = dlsym(NULL, "json_alert");
    if(json_alert_p) json_alert_p("Richiesta precedente ancora in corso.", "error", 63);
    AutoParam_in_corso = 2;	// non inoltro più i parametri della richiesta precedente
    return -1;
  }
  
  for(conc=0; conc<N_LINEE*2; conc++)
  {
    for(i=0; (i<MAX_AUTO_SENS)&&((radio868_map[conc][i].id!=id)||(!radio868_map[conc][i].prog.managed)); i++);
    if(i<MAX_AUTO_SENS) break;
  }
  
  if(conc == N_LINEE*2)
  {
    /* La periferica non è in elenco. */
    return -1;
  }
  
  AutoParam_in_corso = 1;
  radio868_param_conc = conc;
  radio868_param_id = id;
  memset(radio868_param_list, 0xff, sizeof(radio868_param_list));
  
  cmd[0] = 40;
  cmd[1] = (conc&2)*8+2+(conc&1)*5;
  memcpy(cmd+2, &id, 4);	// id periferica
  cmd[6] = 0x30;
  cmd[7] = 0;
  cmd[8] = 0;
  master_queue((conc>>2), cmd);
  
  return 0;
}

void radio868_param_set(int id, int idx, int val)
{
  if((id == radio868_param_id) && (idx < 8))
    radio868_param_list[idx][1] = val;
}

int radio868_param_send(int all)
{
  int i;
  unsigned char cmd[12];
  
  for(i=0; (i<8)&&(radio868_param_list[i][0]!=0xff); i++)
  {
    cmd[0] = 40;
    cmd[1] = (radio868_param_conc&2)*8+2+(radio868_param_conc&1)*5;
    memcpy(cmd+2, &radio868_param_id, 4);	// id periferica
    if((i == 7) || (radio868_param_list[i+1][0] == 0xff))
      cmd[6] = 0x32;	// ultimo parametro
    else
      cmd[6] = 0x31;
    cmd[7] = i;
    cmd[8] = radio868_param_list[i][1];
    master_queue((radio868_param_conc>>2), cmd);
  }
  return 0;
}

#warning Check batteria
/* Per il momento la gestione della segalazione di batteria bassa è
   demandata completamente al programma utente, quindi è sufficiente
   restituire lo stato corrente da bitBattery.
   Nel momento in cui si volesse una gestione nativamente ritardata
   basta attivare il meccanismo analogo alla centrale Facile ed
   impostare bitBattery secondo il ritardo, così da non alterare la
   macro del programma utente ma solo il plugin radio. */
#if 0
void mmaster_batteria868_check()
{
  int i, z, a;
  
  for(i=0; i<N_LINEE*2; i++)
  {
    if(radio868_batteria_timer[i])
    {
      radio868_batteria_timer[i]--;
      if(!radio868_batteria_timer[i])
      {
        /* Verifico se ci siano ancora periferiche con batteria bassa segnalata.
           Se ci sono allora attivo una nuova richiesta di lista stato periferiche
           radio per avere i valori aggiornati di batteria. */
        /* Eseguo la verifica solo se non è attiva la diagnostica, altrimenti
           viene disattivata. Se c'è la diagnostica rimando appena possibile. */
        if(!(radio868_autoappr_flags & RADIO_AUTOAPPR_DYN))
        {
          a = 0;
          for(z=0; z<MAX_AUTO_SENS; z++)
          {
            if(radio868_id_stato[i][z].cntbatt)
            {
              a = 1;
              radio868_map[i][z].flag |= RADIO_MAP_FLAG_BATTERY;
            }
          }
          if(a)
          {
            radio868_autoappr_flags |= (1<<(i&2))<<(i&1);
            radio868_autoappr_flags &= ~RADIO_AUTOAPPR_SEND;
            radio868_batteria_timer[i] = BATTERIA868_TIMER;
          }
        }
        else
          radio868_batteria_timer[i] = 5;
      }
    }
  }
}

void mmaster_batteria868_reset()
{
  int conc, i;
  
  for(conc=0; conc<N_LINEE*2; conc++)
    for(i=0; i<MAX_AUTO_SENS; i++)
      radio868_id_stato[conc][i].stbatt &= ~2;
}
#endif

void radio868_refresh(int idx)
{
  int conc, i;
  unsigned char cmd[4];
  
  /* Cerco solo tra i concentratori della linea specifica, il sensore
     può appartenere a uno qualunque dei due. */
  for(conc=(idx/128)*2; conc<(((idx/128)*2)+2); conc++)
  {
    for(i=0; (i<MAX_AUTO_SENS)&&radio868_id.sens[conc][i].id&&
             (radio868_id.sens[conc][i].sensore!=(idx&0x7f)); i++);
    if(i<MAX_AUTO_SENS)
    {
      /* Trovato il sensore e quindi identificato il concentratore. */
      cmd[0] = 7;
      cmd[1] = (conc&2)*8+2+(conc&1)*5;
      master_queue(conc >> 2, cmd);
      return;
    }
  }
}

void radio868_accept_fronts(int conc)
{
  int i;
  
  for(i=0; i<MAX_AUTO_SENS; i++)
  {
    if(radio868_id.sens[conc][i].id)
    {
      switch(radio868_id.sens[conc][i].tipo)
      {
        case RADIO_TIPO_CONTATTO:
        case RADIO_TIPO_IR:
          SE[radio868_id.sens[conc][i].sensore] &=
            ~(bitLHF | bitHLF | bitPackLHF | bitPackHLF | bitInputSabotageLHF |
              bitInputSabotageHLF | bitTestFailureLHF | bitTestFailureHLF |
              bitInputFailureLHF | bitInputFailureHLF);
          break;
#warning Accettazione fronti attuatori e sirene
#if 0
        case RADIO_TIPO_ATTUATORE:
          //AT[k] &= ~(bitLHF | bitHLF);
          break;
          
        case RADIO_TIPO_SIRENA_EXT:
        case RADIO_TIPO_SIRENA_INT:
#endif
        default:
          break;
      }
    }
  }
}

/***********************************************
   GESTIONE VISUALKEY
***********************************************/
#include "usb.h"
#include "vkeyrx/version.h"
#include "vkeyrx/3774_v0.0.11.bin.c"

#include "support.h"
#include "codec.h"
#include "gsm.h"

//#define DEBUG
#ifdef DEBUG
#define DPRINTF printf
#else
void DPRINTF(char *x, ...) {}
#endif

#define BULK_IN_EP	0x82
#define BULK_OUT_EP	0x05

#define CONFIG_USER_MAX	8

typedef struct {
  struct {
  unsigned char valid;
  unsigned char mask;
  unsigned char pin[2];
  char descr[16];
  } Utente[CONFIG_USER_MAX];
  unsigned int flags;
  char status_descr[6][16];
} VKConfig;

VKConfig vkconfig;

static int status_descr_mod = 0;

static usb_dev_handle *radio_hdev;

enum { LOG_RESET = 1, LOG_RTC_SET, LOG_ALLARME, LOG_MANOMISSIONE, LOG_GUASTO,
  /* saltare la numerazione qui in mezzo */
  LOG_SIRENA = 10, LOG_TELECOMANDO, LOG_GSM_SMS, LOG_GSM_CALL, LOG_MANUTENZIONE, LOG_USB_CONNECT,
  LOG_ZONA, LOG_SENS, LOG_FW_UPDATE, LOG_PERIF_PROG, LOG_UTENTE, LOG_EMERGENZA,
};

void save_vk_config()
{
  FILE *fp;
  
  fp = fopen(DATA_DIR "/visualkey.nv.tmp", "w");
  if(fp)
  {
    fwrite(&vkconfig, 1, sizeof(VKConfig), fp);
    fclose(fp);
    rename(DATA_DIR "/visualkey.nv.tmp", DATA_DIR "/visualkey.nv");
  }
}

static unsigned char radio_status[2] = {0, 0};
static pthread_mutex_t radio_vk_mutex = PTHREAD_MUTEX_INITIALIZER;

void radio_visualkey_led(int idx, char *descr, int onoff)
{
  radio_init();
  
  /* Solo se i parametri sono validi e la descrizione è differente
     (per evitare continui invii e salvataggi inutili) */
  if((idx >= 0) && (idx < 6) && descr && strncmp(descr, vkconfig.status_descr[idx], 16))
  {
    strncpy(vkconfig.status_descr[idx], descr, 16);
    status_descr_mod = 1;
  }
  
  pthread_mutex_lock(&radio_vk_mutex);
  
  switch(idx)
  {
    case 0:	// batteria periferiche
      if(onoff)
        radio_status[0] |= (1<<7);
      else
        radio_status[0] &= ~(1<<7);
      break;
    case 1:	// batteria centrale
      if(onoff)
        radio_status[0] |= (1<<6);
      else
        radio_status[0] &= ~(1<<6);
      break;
    case 2:	// manca rete
      if(onoff)
        radio_status[0] |= (1<<5);
      else
        radio_status[0] &= ~(1<<5);
      break;
    case 5:	// credito GSM ok
      if(onoff)
        radio_status[1] |= (1<<6);
      else
        radio_status[1] &= ~(1<<6);
      break;
    default:
      break;
  }
  
  pthread_mutex_unlock(&radio_vk_mutex);
  
  if(ME[1858] & bitLHF)
  {
  }
  
  if(ME[1859] & bitLHF)
  {
    database_reset_alarm2(&ME[1858]);
    database_reset_alarm2(&ME[1859]);
  }
}

static void status_descr_invia()
{
  unsigned char buf[64];
  int i;
  
  memset(buf, 0, 64);
  for(i=0; i<2; i++)
  {
    buf[0] = 0x93;
    buf[1] = 7;
    buf[2] = 0+(i*3);
    memcpy(buf+6, vkconfig.status_descr[0+(i*3)], 16);
    buf[22] = 1+(i*3);
    memcpy(buf+26, vkconfig.status_descr[1+(i*3)], 16);
    buf[42] = 2+(i*3);
    memcpy(buf+46, vkconfig.status_descr[2+(i*3)], 16);
    usb_bulk_write(radio_hdev, BULK_OUT_EP, buf, 62, 10);
  }
}

static void vk_update_firmware()
{
  unsigned char buf[36];
  int n, ret;
  time_t t;
  
  for(n=0; n<sizeof(fw3774); n+=32)
  {
    buf[0] = 0xf0;
    buf[1] = 0;
    buf[2] = n>>5;	// # blocco
    buf[3] = n>>13;
    if((n+32) <= sizeof(fw3774))
    {
      memcpy(buf+4, ((unsigned char*)(fw3774))+n, 32);
    }
    else
    {
      memset(buf+4, 0xff, 32);
      memcpy(buf+4, ((unsigned char*)(fw3774))+n, sizeof(fw3774)-n);
    }
    usb_bulk_write(radio_hdev, BULK_OUT_EP, buf, 36, 10);
    
    t = time(NULL);
    do
      ret = usb_bulk_read(radio_hdev, BULK_IN_EP, buf, 36, 1000);
    while(!ret && (time(NULL) < (t+5)));	// timeout 5 secondi
    if(!ret) return;
    if((buf[0] == 0xf0) && (buf[1] == 0x01)) return;
  }
  
  buf[0] = 0xf0;
  buf[1] = 1;
  usb_bulk_write(radio_hdev, BULK_OUT_EP, buf, 2, 10);
  t = time(NULL);
  do
    ret = usb_bulk_read(radio_hdev, BULK_IN_EP, buf, 36, 1000);
  while(!ret && (time(NULL) < (t+5)));	// timeout 5 secondi
  
  sleep(1);
}

static void radio_loop()
{
  int n, nn, i;
  unsigned char buf[64];
  char *cod, *desc;
  int manutenzione;
  ProtDevice dev;
  
  /* Assicuro che il main thread abbia tempo di caricare
     la configurazione dei consumatori. */
  sleep(1);
  
  for(dev.consumer=0; (dev.consumer<MAX_NUM_CONSUMER)&&(config.consumer[dev.consumer].configured); dev.consumer++);
  if(dev.consumer<MAX_NUM_CONSUMER)
  {
    config.consumer[dev.consumer].type = PROT_SAETNET;
    LS_ABILITATA[dev.consumer] = 1;
  }
  DPRINTF("Consumatore: %d\n", dev.consumer);
  
  manutenzione = 0;
  
  while(1)
  {
    n = usb_bulk_read(radio_hdev, BULK_IN_EP, buf, 64, 1000);
    if(n > 0)
    {
#ifdef DEBUG
struct timeval tv;
gettimeofday(&tv, NULL);
printf("%d.%06d (%d) ", tv.tv_sec, tv.tv_usec, n);
for(i=0; i<n; i++) printf("%02x", buf[i]);
printf("\n");
#endif
      switch(buf[0])
      {
        case 0x90:
          buf[0] = 0x91;
          
          /*if(ZONA_R_timer[0])
            buf[1] = 3;
          else*/ if(ZT & bitActive)
            buf[1] = 2;
          else if(ZT & bitStatusInactive)
            buf[1] = 1;
          else
            buf[1] = 0;
          
          buf[2] = 0;
          
          /*if(ZONA_R_timer[216])
            buf[2] = 0x03;
          else*/ if(ZI[0] & bitActive)
            buf[2] = 0x02;
          else if(ZI[0] & bitStatusInactive)
            buf[2] = 0x01;
          
          /*if(ZONA_R_timer[217])
            buf[2] |= 0x0c;
          else*/ if(ZI[1] & bitActive)
            buf[2] |= 0x08;
          else if(ZI[1] & bitStatusInactive)
            buf[2] |= 0x04;
          
          /* GSM e altro */
          //buf[3] = 63;	// Assente
          buf[3] = GSM_Status[4] & 0x3f;
          
          for(i=0; i<n_SE; i++)
          {
            if(SE[i] & (bitMUAlarm|bitMUSabotage)) buf[2] |= (1<<4);
            if(SE[i] & bitSabotage) buf[3] |= (1<<7);
          }
          
          pthread_mutex_lock(&radio_vk_mutex);
          buf[2] |= radio_status[0];
          buf[3] |= radio_status[1];
          pthread_mutex_unlock(&radio_vk_mutex);
          
          usb_bulk_write(radio_hdev, BULK_OUT_EP, buf, 4, 10);
          break;
        case 0x92:
          buf[0] = 0x93;
          n = (short)(buf[3] * 256 + buf[2]);
          switch(buf[1] & 0x3f)
          {
            case 0:	// sensori
              for(i=0; i<3; i++)
              {
                if(n < 0)
                  nn = -1;
                else if(buf[1] & 0x40)
                  for(nn=n; (nn < n_SE) && ((nn & 0x7) >= master_sensor_valid(nn)); nn++);
                else if(buf[1] & 0x80)
                  for(nn=n; (nn >= 0) && ((nn & 0x7) >= master_sensor_valid(nn)); nn--);
                else if(n < n_SE)
                  nn = n;
                else
                  nn = -1;
                  
                if((nn >= 0) && (nn < n_SE))
                {
                  /* numero richiesto */
                  buf[2+(i*20)] = n;
                  buf[3+(i*20)] = n >> 8;
                  /* numero effettivo */
                  buf[4+(i*20)] = nn;
                  buf[5+(i*20)] = nn >> 8;
                  /* descrizione */
                  string_sensor_name(nn, &cod, &desc);
                  
                  sprintf(buf+6+(i*20), "%.16s", desc);
                  
                  if(buf[1] & 0x40)
                    n = nn+1;
                  else if(buf[1] & 0x80)
                    n = nn-1;
                  else
                    n++;
                }
                else
                {
                  /* numero richiesto */
                  buf[2+(i*20)] = n;
                  buf[3+(i*20)] = n >> 8;
                  /* numero effettivo */
                  buf[4+(i*20)] = 0xff;
                  buf[5+(i*20)] = 0xff;
                  /* descrizione */
                  buf[6+(i*20)] = 0;
                }
              }
              
              buf[1] &= 0x3f;
              usb_bulk_write(radio_hdev, BULK_OUT_EP, buf, 62, 10);
              break;
            case 1:	// zone
              for(i=0; i<3; i++)
              {
                if(n < 0)
                  nn = -1;
                else if(n < 3)
                {
                  /* numero richiesto */
                  buf[2+(i*20)] = n;
                  buf[3+(i*20)] = n >> 8;
                  /* numero effettivo */
                  buf[4+(i*20)] = n;
                  buf[5+(i*20)] = n >> 8;
                  /* descrizione */
                  if(n == 0)
                    string_zone_name(0, &cod, &desc);
                  else if(n == 1)
                    string_zone_name(216, &cod, &desc);
                  else if(n == 2)
                    string_zone_name(217, &cod, &desc);
                  sprintf(buf+6+(i*20), "%.16s", desc);
                  
                  n++;
                }
                else
                {
                  /* numero richiesto */
                  buf[2+(i*20)] = n;
                  buf[3+(i*20)] = n >> 8;
                  /* numero effettivo */
                  buf[4+(i*20)] = 0xff;
                  buf[5+(i*20)] = 0xff;
                  /* descrizione */
                  buf[6+(i*20)] = 0;
                }
              }
              
              buf[1] &= 0x3f;
              usb_bulk_write(radio_hdev, BULK_OUT_EP, buf, 62, 10);
              break;
            case 2:	// telecomandi
              for(i=0; i<3; i++)
              {
                if(n < 0)
                  nn = -1;
                else if(buf[1] & 0x40)
                {
                  for(nn=n; nn<n_ME; nn++)
                  {
                    string_command_name(nn, &cod, &desc);
                    if(desc != StringDescNull) break;
                  }
                }
                else if(buf[1] & 0x80)
                {
                  for(nn=n; nn>=0; nn--)
                  {
                    string_command_name(nn, &cod, &desc);
                    if(desc != StringDescNull) break;
                  }
                }
                else if(n < n_ME)
                {
                  nn = n;
                  string_command_name(nn, &cod, &desc);
                  if(desc == StringDescNull) nn = -1;
                }
                else
                  nn = -1;
                  
                if((nn >= 0) && (nn < n_ME))
                {
                  /* numero richiesto */
                  buf[2+(i*20)] = n;
                  buf[3+(i*20)] = n >> 8;
                  /* numero effettivo */
                  buf[4+(i*20)] = nn;
                  buf[5+(i*20)] = nn >> 8;
                  /* descrizione */
                  sprintf(buf+6+(i*20), "%.16s", desc);
                  
                  if(buf[1] & 0x40)
                    n = nn+1;
                  else if(buf[1] & 0x80)
                    n = nn-1;
                  else
                    n++;
                }
                else
                {
                  /* numero richiesto */
                  buf[2+(i*20)] = n;
                  buf[3+(i*20)] = n >> 8;
                  /* numero effettivo */
                  buf[4+(i*20)] = 0xff;
                  buf[5+(i*20)] = 0xff;
                  /* descrizione */
                  buf[6+(i*20)] = 0;
                }
              }
              
              buf[1] &= 0x3f;
              usb_bulk_write(radio_hdev, BULK_OUT_EP, buf, 62, 10);
              break;
            case 3:	// utenti
              for(i=0; i<3; i++)
              {
                if(n < 0)
                  nn = -1;
                else if(buf[1] & 0x40)
                  for(nn=n; (nn < CONFIG_USER_MAX) && !vkconfig.Utente[nn].valid; nn++);
                else if(buf[1] & 0x80)
                  for(nn=n; (nn >= 0) && !vkconfig.Utente[nn].valid; nn--);
                else if((n < CONFIG_USER_MAX) && (vkconfig.Utente[n].valid))
                  nn = n;
                else
                  nn = -1;
                  
                if((nn >= 0) && (nn < CONFIG_USER_MAX))
                {
                  /* numero richiesto */
                  buf[2+(i*20)] = n;
                  buf[3+(i*20)] = n>>8;
                  /* numero effettivo */
                  buf[4+(i*20)] = nn;
                  buf[5+(i*20)] = 0;
                  /* descrizione */
                  memcpy(buf+6+(i*20), vkconfig.Utente[nn].descr, 16);
                  
                  if(buf[1] & 0x40)
                    n = nn+1;
                  else if(buf[1] & 0x80)
                    n = nn-1;
                  else
                    n++;
                }
                else if(nn == CONFIG_USER_MAX)
                {
                  for(nn=0; (nn < CONFIG_USER_MAX) && vkconfig.Utente[nn].valid; nn++);
                  if(nn < CONFIG_USER_MAX)
                  {
                    /* numero richiesto */
                    buf[2+(i*20)] = n;
                    buf[3+(i*20)] = n>>8;
                    /* numero effettivo */
                    buf[4+(i*20)] = nn | 0x80;
                    buf[5+(i*20)] = 0;
                    /* descrizione */
                    memcpy(buf+6+(i*20), "Nuovo utente", 13);
                    n = CONFIG_USER_MAX+1;
                  }
                  else
                  {
                    /* numero richiesto */
                    buf[2+(i*20)] = n;
                    buf[3+(i*20)] = n>>8;
                    /* numero effettivo */
                    buf[4+(i*20)] = 0xff;
                    buf[5+(i*20)] = 0xff;
                    /* descrizione */
                    buf[6+(i*20)] = 0;
                  }
                }
                else
                {
                  /* numero richiesto */
                  buf[2+(i*20)] = n;
                  buf[3+(i*20)] = n>>8;
                  /* numero effettivo */
                  buf[4+(i*20)] = 0xff;
                  buf[5+(i*20)] = 0xff;
                  /* descrizione */
                  buf[6+(i*20)] = 0;
                }
              }
              
              buf[1] &= 0x3f;
              usb_bulk_write(radio_hdev, BULK_OUT_EP, buf, 62, 10);
              break;
            case 4:	// rubrica - numero
              for(i=0; i<3; i++)
              {
                if(n < 0)
                  nn = -1;
                else if(buf[1] & 0x40)
                  for(nn=n; (nn < PBOOK_DIM) && !config.PhoneBook[nn].Phone[0]; nn++);
                else if(buf[1] & 0x80)
                  for(nn=n; (nn >= 0) && !config.PhoneBook[nn].Phone[0]; nn--);
                else if((n < PBOOK_DIM) && (config.PhoneBook[n].Phone[0]))
                  nn = n;
                else
                  nn = -1;
                  
                if((nn >= 0) && (nn < PBOOK_DIM))
                {
                  /* numero richiesto */
                  buf[2+(i*20)] = n;
                  buf[3+(i*20)] = 0;
                  /* numero effettivo */
                  buf[4+(i*20)] = nn;
                  /* flags */
                  /* Inserisco i flags nel byte alto della numerazione */
                  buf[63] = 0;
                  if(config.PhoneBook[nn].Abil & (PB_SMS_TX|PB_SMS_RX)) buf[63] = 1;
                  if(config.PhoneBook[nn].Abil & (PB_GSM_TX|PB_GSM_RX)) buf[63] |= 2;
                  buf[5+(i*20)] = buf[63];
                  /* descrizione */
                  sprintf(buf+6+(i*20), "%.16s", config.PhoneBook[nn].Phone);
                  
                  if(buf[1] & 0x40)
                    n = nn+1;
                  else if(buf[1] & 0x80)
                    n = nn-1;
                  else
                    n++;
                }
                else
                {
                  /* numero richiesto */
                  buf[2+(i*20)] = n;
                  buf[3+(i*20)] = n >> 8;
                  /* numero effettivo */
                  buf[4+(i*20)] = 0xff;
                  buf[5+(i*20)] = 0xff;
                  /* descrizione */
                  buf[6+(i*20)] = 0;
                }
              }
              
              buf[1] &= 0x3f;
              usb_bulk_write(radio_hdev, BULK_OUT_EP, buf, 62, 10);
              break;
            case 5:	// rubrica - descrizione
              for(i=0; i<3; i++)
              {
                if(n < 0)
                  nn = -1;
                else if(buf[1] & 0x40)
                  for(nn=n; (nn < PBOOK_DIM) && !config.PhoneBook[nn].Phone[0]; nn++);
                else if(buf[1] & 0x80)
                  for(nn=n; (nn >= 0) && !config.PhoneBook[nn].Phone[0]; nn--);
                else if((n < PBOOK_DIM) && (config.PhoneBook[n].Phone[0]))
                  nn = n;
                else
                  nn = -1;
                  
                if((nn >= 0) && (nn < PBOOK_DIM))
                {
                  /* numero richiesto */
                  buf[2+(i*20)] = n;
                  buf[3+(i*20)] = 0;
                  /* numero effettivo */
                  buf[4+(i*20)] = nn;
                  /* flags */
                  /* Inserisco i flags nel byte alto della numerazione */
                  buf[63] = 0;
                  if(config.PhoneBook[nn].Abil & (PB_SMS_TX|PB_SMS_RX)) buf[63] = 1;
                  if(config.PhoneBook[nn].Abil & (PB_GSM_TX|PB_GSM_RX)) buf[63] |= 2;
                  buf[5+(i*20)] = buf[63];
                  /* descrizione */
                  if(config.PhoneBook[nn].Name[0])
                    sprintf(buf+6+(i*20), "%.16s", config.PhoneBook[nn].Name);
                  else
                    sprintf(buf+6+(i*20), "Rubrica %d", nn+1);
                    
                  if(buf[1] & 0x40)
                    n = nn+1;
                  else if(buf[1] & 0x80)
                    n = nn-1;
                  else
                    n++;
                }
                else if(nn == PBOOK_DIM)
                {
                  for(nn=0; (nn < PBOOK_DIM) && config.PhoneBook[nn].Phone[0]; nn++);
                  if(nn < PBOOK_DIM)
                  {
                    /* numero richiesto */
                    buf[2+(i*20)] = n;
                    buf[3+(i*20)] = 0;
                    /* numero effettivo */
                    buf[4+(i*20)] = nn | 0x80;
                    buf[5+(i*20)] = 0;
                    /* descrizione */
                    memcpy(buf+6+(i*20), "Nuovo contatto", 15);
                    n = PBOOK_DIM+1;
                  }
                  else
                  {
                    /* numero richiesto */
                    buf[2+(i*20)] = n;
                    buf[3+(i*20)] = 0;
                    /* numero effettivo */
                    buf[4+(i*20)] = 0xff;
                    buf[5+(i*20)] = 0xff;
                    /* descrizione */
                    buf[6+(i*20)] = 0;
                  }
                }
                else
                {
                  /* numero richiesto */
                  buf[2+(i*20)] = n;
                  buf[3+(i*20)] = n >> 8;
                  /* numero effettivo */
                  buf[4+(i*20)] = 0xff;
                  buf[5+(i*20)] = 0xff;
                  /* descrizione */
                  buf[6+(i*20)] = 0;
                }
              }
              
              buf[1] &= 0x3f;
              usb_bulk_write(radio_hdev, BULK_OUT_EP, buf, 62, 10);
              break;
            case 6:
              memset(buf+4, 0, 58);
              *(int*)(buf+10) = vkconfig.flags;
              buf[13] = 1;	// Delphi
              *(unsigned int*)(buf+18) = SECONDI|(MINUTI<<6)|(ORE<<12)|(GIORNO<<17)|(MESE<<22)|(ANNO<<26);
              usb_bulk_write(radio_hdev, BULK_OUT_EP, buf, 62, 10);
              break;
            default:
              break;
          }
          break;
        case 0x93:
          /* Invio descrizioni */
          n = (short)(buf[3] * 256 + buf[2]);
          buf[20] = 0;
          switch(buf[1])
          {
            case 0:	// sensori
              if((n >= 0) && (n < n_SE))
              {
                free(SensorName[n]);
                SensorName[n] = (char*)strdup(buf+4);
                string_save();
              }
              break;
            case 1:	// zone
              if(n == 1)
                n = 216;
              else if(n == 2)
                n = 217;
              else if(n)
                n = -1;
              if(n >= 0)
              {
                free(ZoneName[n]);
                ZoneName[n] = (char*)strdup(buf+4);
                string_save();
              }
              break;
            case 2:	// telecomandi
              if((n >= 0) && (n < n_ME))
              {
                free(CommandName[n]);
                CommandName[n] = (char*)strdup(buf+4);
                string_save();
              }
              break;
            case 3:	// utenti
              if((n >= 0) && (n < CONFIG_USER_MAX))
              {
                memcpy(vkconfig.Utente[n].descr, buf+4, 16);
                save_vk_config();
              }
              break;
              break;
            case 4:	// rubrica - numero
              if(buf[2] < PBOOK_DIM)
              {
                buf[20] = 0;
                memcpy(config.PhoneBook[buf[2]].Phone, buf+4, 17);
                config.PhoneBook[buf[2]].Abil = 0;
                if(buf[3] & 0x01) config.PhoneBook[buf[2]].Abil = PB_SMS_TX|PB_SMS_RX;
                if(buf[3] & 0x02) config.PhoneBook[buf[2]].Abil |= PB_GSM_TX|PB_GSM_RX;
                delphi_save_conf();
              }
              break;
            case 5:	// rubrica - descrizione
              if(buf[2] < PBOOK_DIM)
              {
                free(config.PhoneBook[buf[2]].Name);
                buf[20] = 0;
                config.PhoneBook[buf[2]].Name = (char*)strdup(buf+4);
                config.PhoneBook[buf[2]].Abil = 0;
                if(buf[3] & 0x01) config.PhoneBook[buf[2]].Abil = PB_SMS_TX|PB_SMS_RX;
                if(buf[3] & 0x02) config.PhoneBook[buf[2]].Abil |= PB_GSM_TX|PB_GSM_RX;
                delphi_save_conf();
              }
              break;
            case 6:	// configurazione
              vkconfig.flags = *(int*)(buf+8);
              save_vk_config();
              break;
          }
          break;
        case 0x94:
          /* Comando */
          if(buf[1] == 0)
          {
            database_set_alarm2(&(ME[0]));
            
            /* Attivazione/disattivazione zone */
            if(buf[2] & 0x80)
            {
              // disattiva
              switch(buf[2] & 0x7f)
              {
                case 0:
                  cmd_zone_off(0, 0);
                  break;
                case 1:
                  cmd_zone_off(216, 0);
                  break;
                case 2:
                  cmd_zone_off(217, 0);
                  break;
                default:
                  break;
              }
            }
            else
            {
              // attiva con fuori servizio
              switch(buf[2] & 0x7f)
              {
                case 0:
                  cmd_zone_on(0, 2);
                  break;
                case 1:
                  cmd_zone_on(216, 2);
                  break;
                case 2:
                  cmd_zone_on(217, 2);
                  break;
                default:
                  break;
              }
            }
          }
          else if(buf[1] == 1)
          {
            /* Telecomando */
            n = buf[3] * 256 + buf[2];
            if(n < n_ME) database_set_alarm2(&(ME[n]));
          }
          else if(buf[1] == 2)
          {
            /* Emergenza */
            database_set_alarm2(&(ME[511]));
          }
          break;
        case 0x95:
          /* Lista anomalie */
          buf[0] = 0x96;
          memset(buf+4, 0xff, 16);
          switch(buf[3])
          {
            case 0:	// allarmi
              nn = bitAlarm;
              break;
            case 1:	// guasti
              nn = bitFailure;
              break;
            case 2:	// manomissioni
              nn = bitSabotage;
              break;
            default:
              nn = 0;
              break;
          }
          n = 0;
          for(i=0; i<8; i++)
          {
            for(; (n<n_SE)&&(!(SE[n]&nn)||(SE[n]&bitOOS)); n++);
            if(n<n_SE)
            {
              buf[i*2+4] = n;
              buf[i*2+5] = n>>8;
              n++;
            }
          }
          usb_bulk_write(radio_hdev, BULK_OUT_EP, buf, 20, 10);
          break;
        case 0x98:
          /* Creazione utente/cambio PIN */
          if(!vkconfig.Utente[buf[1]].valid && buf[2])
            sprintf(vkconfig.Utente[buf[1]].descr, "Utente %d", buf[1]);
          else if(!buf[2])
            vkconfig.Utente[buf[1]].descr[0] = 0;
          vkconfig.Utente[buf[1]].valid = buf[2];
          vkconfig.Utente[buf[1]].mask = buf[3];
          vkconfig.Utente[buf[1]].pin[0] = buf[4];
          vkconfig.Utente[buf[1]].pin[1] = buf[5];
          save_vk_config();
          break;
        default:
          break;
      }
    }
    else
    {
#if 0
struct timeval tv;
gettimeofday(&tv, NULL);
printf("%d.%06d %d (%s)\n", tv.tv_sec, tv.tv_usec, n, usb_strerror());
#endif
      /* Timeout ricezione dati o errore. */
      if(n == -19)
      {
        /* Si è persa la connessione USB. */
        return;
      }
    }
    
    /* Gestione descrizioni personalizzate */
    if(status_descr_mod)
    {
      status_descr_mod = 0;
      status_descr_invia();
      save_vk_config();
    }
    
    /* Gestione storico */
    if(dev.consumer < MAX_NUM_CONSUMER)
    {
      Event ev;
      
      do
      {
        while(!(n = codec_get_event(&ev, &dev)));
        if(n > 0)
        {
          nn = 0;
          switch(ev.Event[8])
          {
            /* Per allarmi e manomissioni mi aggancio all'evento inizio
               così da non ripetere più volte le stesse segnalazioni. */
            case Evento_Sensore:
              if((ev.Event[12] == 1) && ((ev.Event[11] == 1) || (ev.Event[11] == 2)))
              {
                if(ev.Event[11] == 1)
                  buf[5] = LOG_ALLARME;
                else
                  buf[5] = LOG_MANOMISSIONE;
                buf[6] = ev.Event[10];
                buf[7] = ev.Event[9];
                buf[8] = 0;
                nn = 9;
              }
              break;
            case Attivata_zona:
              if(!ev.Event[9] || (ev.Event[9] == 216) || (ev.Event[9] == 217))
              {
                buf[5] = LOG_ZONA;
                buf[6] = ev.Event[9];
                if(buf[6]) buf[6] -= 215;
                buf[7] = 1;
                buf[8] = 0;
                nn = 9;
              }
              break;
            case Disattivata_zona:
              if(!ev.Event[9] || (ev.Event[9] == 216) || (ev.Event[9] == 217))
              {
                buf[5] = LOG_ZONA;
                buf[6] = ev.Event[9];
                if(buf[6]) buf[6] -= 215;
                buf[7] = 0;
                buf[8] = 0;
                nn = 9;
              }
              break;
            default:
              break;
          }
          
          if(nn)
          {
            buf[0] = 0x97;
            *(unsigned int*)(buf+1) = ev.Event[7]|((ev.Event[6]&0x3f)<<6)|(ev.Event[5]<<12)|
              (ev.Event[2]<<17)|(ev.Event[3]<<22)|(ev.Event[4]<<26);
            usb_bulk_write(radio_hdev, BULK_OUT_EP, buf, 9, 10);
          }
        }
      }
      while(n > 0);
    }
    
    /* Gestione manutenzione */
    if(ME[1858] && !manutenzione)
    {
      manutenzione = 1;
      buf[0] = 0x99;
      buf[1] = 1;	// inizio manutenzione
      usb_bulk_write(radio_hdev, BULK_OUT_EP, buf, 2, 10);
    }
    else if(manutenzione && !ME[1858])
    {
      manutenzione = 0;
      buf[0] = 0x99;
      buf[1] = 0;	// fine manutenzione
      usb_bulk_write(radio_hdev, BULK_OUT_EP, buf, 2, 10);
    }
  }
}

static void* visualkey_loop(void *null)
{
  struct usb_bus *bus;
  struct usb_device *dev;
  int ret;
  FILE *fp;
  unsigned char buf[8];
  
  pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
  
  usb_init();
  usb_find_busses();
  
  while(1)
  {
  
  dev = NULL;
  /* Occorre liberare tutti i device ad ogni giro, altrimenti il find
     alloca continuamente memoria per gli stessi device trovati ogni volta. */
  for(bus = usb_busses; bus; bus = bus->next)
  {
    while(bus->devices)
    {
      dev = bus->devices->next;
      free(bus->devices);
      bus->devices = dev;
    }
  }
  
  usb_find_devices();
  
  for (bus = usb_busses; bus; bus = bus->next)
  {
    for(dev=bus->devices; dev; dev=dev->next)
      if((dev->descriptor.idVendor == 0xffff) && (dev->descriptor.idProduct == 0x3774))
      {
        bus = NULL;
        break;
      }
    if(!bus) break;
  }
  
  if(!dev)
  {
    DPRINTF("Errore rilevazione ricevitore.\n");
    sleep(2);
    continue;
  }
  
  DPRINTF("Trovato ricevitore.\n");
  
  radio_hdev = usb_open(dev);
  DPRINTF("handle: %p\n", radio_hdev);
  
  ret = usb_claim_interface(radio_hdev, 1);
  DPRINTF("claim interf: %d (%s)\n", ret, usb_strerror());
  
  ret = usb_clear_halt(radio_hdev, BULK_OUT_EP);
  DPRINTF("clear: %d (%s)\n", ret, usb_strerror());
  if(ret < 0)
  {
    usb_release_interface(radio_hdev, 1);
    usb_close(radio_hdev);
    continue;
  }
  ret = usb_clear_halt(radio_hdev, BULK_IN_EP);
  DPRINTF("clear: %d (%s)\n", ret, usb_strerror());
  if(ret < 0)
  {
    usb_release_interface(radio_hdev, 1);
    usb_close(radio_hdev);
    continue;
  }
  
  /* Svuota eventuali messaggi accodati. */
  while(usb_bulk_read(radio_hdev, BULK_IN_EP, buf, 64, 100) > 0);
  
  /* Imposta l'interfaccia visualkey */
  buf[0] = 0x80;
  buf[1] = 0;
  usb_bulk_write(radio_hdev, BULK_OUT_EP, buf, 2, 10);
  
  buf[0] = 0xf1;
  usb_bulk_write(radio_hdev, BULK_OUT_EP, buf, 1, 10);
  if((usb_bulk_read(radio_hdev, BULK_IN_EP, buf, 64, 100) > 0) && (buf[0] == 0xf1))
  {
    int forceupdate = 0;
    
    if(!unlink("/tmp/visualkey_fw")) forceupdate = 1;
    
    DPRINTF("Versione %d.%d.%d\n", buf[1], buf[2], buf[3]);
    DPRINTF("Impianto:%02x%02x ch:%02x cal:%02x\n", buf[4], buf[5], buf[6], buf[7]);
    if((buf[1] != VERSION_MAJOR) || (buf[2] != VERSION_MINOR) || (buf[3] != VERSION_BUILD) || forceupdate)
    {
      DPRINTF("Aggiornamento firmware.\n");
      support_log("Aggiornamento firmware radio.");
      vk_update_firmware();
      usb_release_interface(radio_hdev, 1);
      usb_close(radio_hdev);
      exit(0);
    }
  }
  else
  {
    usb_release_interface(radio_hdev, 1);
    usb_close(radio_hdev);
    continue;
  }
  
  memset(&vkconfig, 0, sizeof(VKConfig));
  fp = fopen(DATA_DIR "/visualkey.nv", "r");
  if(fp)
  {
    fread(&vkconfig, 1, sizeof(VKConfig), fp);
    fclose(fp);
  }
  if(!vkconfig.Utente[0].valid)
  {
    vkconfig.Utente[0].valid = 1;
    memcpy(vkconfig.Utente[0].descr, "Amministratore", 15);
  }
  for(ret=0; ret<CONFIG_USER_MAX; ret++)
  {
    buf[0] = 0x98;
    buf[1] = ret;
    buf[2] = vkconfig.Utente[ret].valid;
    buf[3] = vkconfig.Utente[ret].mask;
    buf[4] = vkconfig.Utente[ret].pin[0];
    buf[5] = vkconfig.Utente[ret].pin[1];
    usb_bulk_write(radio_hdev, BULK_OUT_EP, buf, 6, 10);
  }
  
  status_descr_invia();
  
  radio_loop();
  
    usb_release_interface(radio_hdev, 1);
    usb_close(radio_hdev);
  }
  
  return NULL;
}

void _init()
{
  pthread_t pth;
  
  printf("Radio plugin: " __DATE__ " " __TIME__ "\n");
  
  console_register_lang(radio_set_lang);
  
  pthread_create(&pth,  NULL, (PthreadFunction)visualkey_loop, NULL);
  pthread_detach(pth);
}
