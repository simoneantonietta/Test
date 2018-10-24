#include "console.h"
#include "con_modem.h"
#include "../support.h"
#include "../delphi.h"
#include "../audio.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

/***********************************************************************
	MODEM menu functions
***********************************************************************/

static pthread_mutex_t console_modem_mutex = PTHREAD_MUTEX_INITIALIZER;

static CALLBACK void* console_modem_store2(ProtDevice *dev, int num_as_int, void *null)
{
  pthread_mutex_lock(&console_modem_mutex);

  if(config.PhoneBook[CONSOLE->support_idx].Phone[0])
  {
    /* Somebody else has occupied the slot, so search for another */
    for(CONSOLE->support_idx=0; CONSOLE->support_idx<PBOOK_DIM; CONSOLE->support_idx++)
      if(!config.PhoneBook[CONSOLE->support_idx].Phone[0]) break;
  
    if(CONSOLE->support_idx == PBOOK_DIM)
    {
      pthread_mutex_unlock(&console_modem_mutex);

      console_send(dev, "ATS00\r");
      console_send(dev, str_9_0);
      console_send(dev, str_9_1);
  
      CONSOLE->last_menu_item = console_show_menu;
      timeout_on(CONSOLE->timeout, (timeout_func)console_return_to_last_menu_item, dev, 0, TIMEOUT_MSG);
      return NULL;
    }
  }

  free(config.PhoneBook[CONSOLE->support_idx].Name);
  config.PhoneBook[CONSOLE->support_idx].Name = CONSOLE->temp_string;
  memset(config.PhoneBook[CONSOLE->support_idx].Phone, 0, 24);
  if(num_as_int)  strncpy(config.PhoneBook[CONSOLE->support_idx].Phone, (char*)num_as_int, 23);
  config.PhoneBook[CONSOLE->support_idx].Abil = 0;

  delphi_save_conf();
  pthread_mutex_unlock(&console_modem_mutex);
  
  CONSOLE->temp_string = NULL;
  
  console_send(dev, "ATS00\r");
  console_send(dev, str_9_2);
  
  CONSOLE->last_menu_item = console_modem_store;
  timeout_on(CONSOLE->timeout, (timeout_func)console_return_to_last_menu_item, dev, 0, TIMEOUT_MSG);

  return NULL;
}

static CALLBACK void* console_modem_store1(ProtDevice *dev, int name_as_int, void *null)
{
  free(CONSOLE->temp_string);
  CONSOLE->temp_string = strdup((char*)name_as_int);
  
  console_send(dev, str_9_3);
  console_send(dev, str_9_48);
  console_send(dev, str_9_49);
  console_send(dev, "ATL111\r");

  console_register_callback(dev, KEY_CANCEL, console_modem_store, NULL);
  console_register_callback(dev, KEY_STRING, console_modem_store2, NULL);

  return NULL;
}

CALLBACK void* console_modem_store(ProtDevice *dev, int press, void *null)
{  
  for(CONSOLE->support_idx=0; CONSOLE->support_idx<PBOOK_DIM; CONSOLE->support_idx++)
    if(!config.PhoneBook[CONSOLE->support_idx].Phone[0]) break;
  
  if(CONSOLE->support_idx == PBOOK_DIM)
  {
    console_send(dev, "ATS00\r");
    console_send(dev, str_9_0);
    console_send(dev, str_9_1);
  
    CONSOLE->last_menu_item = console_show_menu;
    timeout_on(CONSOLE->timeout, (timeout_func)console_return_to_last_menu_item, dev, 0, TIMEOUT_MSG);
    return NULL;
  }

  console_disable_menu(dev, str_9_6);
  
  console_send(dev, str_9_7);
  console_send(dev, str_9_50);
  console_send(dev, str_9_51);
  console_send(dev, "ATL211\r");

  console_register_callback(dev, KEY_CANCEL, console_show_menu, NULL);
  console_register_callback(dev, KEY_STRING, console_modem_store1, NULL);
  
  return NULL;
}

static void console_modem_list(ProtDevice *dev, ConsoleCallback func)
{
  int i;
  
  console_list_free(CONSOLE->support_list);
  CONSOLE->support_list = NULL;
  
  pthread_mutex_lock(&console_modem_mutex);
  /* lock to avoid half stored slots */

  for(i=0; i<PBOOK_DIM; i++)
    if(config.PhoneBook[i].Phone[0] && config.PhoneBook[i].Name)
    {
      CONSOLE->support_list = console_list_add(CONSOLE->support_list,
      	config.PhoneBook[i].Name, config.PhoneBook[i].Phone, func, i);
    }

  pthread_mutex_unlock(&console_modem_mutex);

  console_list_show(dev, CONSOLE->support_list, 0, 3, 0);
}

static CALLBACK void* console_modem_delete1(ProtDevice *dev, int press, void *id_as_voidp)
{
  CONSOLE->support_idx = (int)id_as_voidp;
  
  pthread_mutex_lock(&console_modem_mutex);

  free(config.PhoneBook[CONSOLE->support_idx].Name);
  config.PhoneBook[CONSOLE->support_idx].Name = NULL;
  config.PhoneBook[CONSOLE->support_idx].Phone[0] = 0;
  config.PhoneBook[CONSOLE->support_idx].Abil = 0;
  delphi_save_conf();

  pthread_mutex_unlock(&console_modem_mutex);
  
  console_send(dev, "ATS00\r");
  console_send(dev, str_9_8);
  console_send(dev, str_9_9);
  
  CONSOLE->last_menu_item = console_modem_delete;
  timeout_on(CONSOLE->timeout, (timeout_func)console_return_to_last_menu_item, dev, 0, TIMEOUT_MSG);

  return NULL;
}

CALLBACK void* console_modem_delete(ProtDevice *dev, int press, void *null)
{
  console_disable_menu(dev, str_9_10);

  CONSOLE->last_menu_item = console_modem_delete;
  console_modem_list(dev, console_modem_delete1);
  
  return NULL;
}

static CALLBACK void* console_modem_enable2(ProtDevice *dev, int press, void *abil_as_voidp)
{
  int abil = (int)abil_as_voidp;
  
  pthread_mutex_lock(&console_modem_mutex);
  config.PhoneBook[CONSOLE->support_idx].Abil |= abil;
  delphi_save_conf();
  pthread_mutex_unlock(&console_modem_mutex);
  
  console_send(dev, "ATS00\r");
  console_send(dev, str_9_8);
  console_send(dev, str_9_12);
  
  CONSOLE->last_menu_item = console_modem_enable;
  timeout_on(CONSOLE->timeout, (timeout_func)console_return_to_last_menu_item, dev, 0, TIMEOUT_MSG);

  return NULL;
}

static CALLBACK void* console_modem_enable1(ProtDevice *dev, int press, void *id_as_voidp)
{
  char cmd[32];

  CONSOLE->support_idx = (int)id_as_voidp;
  
  console_send(dev, "ATS03\r");
  sprintf(cmd, str_9_52, config.PhoneBook[CONSOLE->support_idx].Name);
  console_send(dev, cmd);
  sprintf(cmd, str_9_53, config.PhoneBook[CONSOLE->support_idx].Phone);
  console_send(dev, cmd);

  console_list_free(CONSOLE->support_list);
  CONSOLE->support_list = NULL;
  
  if(!(config.PhoneBook[CONSOLE->support_idx].Abil & PB_TX))
    CONSOLE->support_list = console_list_add(CONSOLE->support_list, str_9_13, NULL, console_modem_enable2, PB_TX);
  if(!(config.PhoneBook[CONSOLE->support_idx].Abil & PB_RX))
    CONSOLE->support_list = console_list_add(CONSOLE->support_list, str_9_14, NULL, console_modem_enable2, PB_RX);
  if(!(config.PhoneBook[CONSOLE->support_idx].Abil & PB_SMS_TX))
    CONSOLE->support_list = console_list_add(CONSOLE->support_list, str_9_15, NULL, console_modem_enable2, PB_SMS_TX);
  if(!(config.PhoneBook[CONSOLE->support_idx].Abil & PB_SMS_RX))
    CONSOLE->support_list = console_list_add(CONSOLE->support_list, str_9_16, NULL, console_modem_enable2, PB_SMS_RX);
  if(!(config.PhoneBook[CONSOLE->support_idx].Abil & PB_GSM_TX))
    CONSOLE->support_list = console_list_add(CONSOLE->support_list, str_9_17, NULL, console_modem_enable2, PB_GSM_TX);
  if(!(config.PhoneBook[CONSOLE->support_idx].Abil & PB_GSM_RX))
    CONSOLE->support_list = console_list_add(CONSOLE->support_list, str_9_18, NULL, console_modem_enable2, PB_GSM_RX);
  console_list_show(dev, CONSOLE->support_list, 0, 3, 0);
  console_register_callback(dev, KEY_CANCEL, console_modem_enable, NULL);
  
  return NULL;
}

CALLBACK void* console_modem_enable(ProtDevice *dev, int press, void *null)
{
  console_disable_menu(dev, str_9_19);

  CONSOLE->last_menu_item = console_modem_enable;
  console_modem_list(dev, console_modem_enable1);
  
  return NULL;
}

static CALLBACK void* console_modem_disable2(ProtDevice *dev, int press, void *abil_as_voidp)
{
  int abil = (int)abil_as_voidp;
  
  pthread_mutex_lock(&console_modem_mutex);
  config.PhoneBook[CONSOLE->support_idx].Abil &= ~abil;
  delphi_save_conf();
  pthread_mutex_unlock(&console_modem_mutex);
  
  console_send(dev, "ATS00\r");
  console_send(dev, str_9_8);
  console_send(dev, str_9_21);
  
  CONSOLE->last_menu_item = console_modem_disable;
  timeout_on(CONSOLE->timeout, (timeout_func)console_return_to_last_menu_item, dev, 0, TIMEOUT_MSG);

  return NULL;
}

static CALLBACK void* console_modem_disable1(ProtDevice *dev, int press, void *id_as_voidp)
{
  char cmd[32];

  CONSOLE->support_idx = (int)id_as_voidp;
  
  console_send(dev, "ATS03\r");
  sprintf(cmd, str_9_52, config.PhoneBook[CONSOLE->support_idx].Name);
  console_send(dev, cmd);
  sprintf(cmd, str_9_53, config.PhoneBook[CONSOLE->support_idx].Phone);
  console_send(dev, cmd);

  console_list_free(CONSOLE->support_list);
  CONSOLE->support_list = NULL;
  
  if(config.PhoneBook[CONSOLE->support_idx].Abil & PB_TX)
    CONSOLE->support_list = console_list_add(CONSOLE->support_list, str_9_13, NULL, console_modem_disable2, PB_TX);
  if(config.PhoneBook[CONSOLE->support_idx].Abil & PB_RX)
    CONSOLE->support_list = console_list_add(CONSOLE->support_list, str_9_14, NULL, console_modem_disable2, PB_RX);
  if(config.PhoneBook[CONSOLE->support_idx].Abil & PB_SMS_TX)
    CONSOLE->support_list = console_list_add(CONSOLE->support_list, str_9_15, NULL, console_modem_disable2, PB_SMS_TX);
  if(config.PhoneBook[CONSOLE->support_idx].Abil & PB_SMS_RX)
    CONSOLE->support_list = console_list_add(CONSOLE->support_list, str_9_16, NULL, console_modem_disable2, PB_SMS_RX);
  if(config.PhoneBook[CONSOLE->support_idx].Abil & PB_GSM_TX)
    CONSOLE->support_list = console_list_add(CONSOLE->support_list, str_9_17, NULL, console_modem_disable2, PB_GSM_TX);
  if(config.PhoneBook[CONSOLE->support_idx].Abil & PB_GSM_RX)
    CONSOLE->support_list = console_list_add(CONSOLE->support_list, str_9_18, NULL, console_modem_disable2, PB_GSM_RX);
  console_list_show(dev, CONSOLE->support_list, 0, 3, 0);
  console_register_callback(dev, KEY_CANCEL, console_modem_disable, NULL);
  
  return NULL;
}

CALLBACK void* console_modem_disable(ProtDevice *dev, int press, void *null)
{
  console_disable_menu(dev, str_9_28);

  CONSOLE->last_menu_item = console_modem_disable;
  console_modem_list(dev, console_modem_disable1);
  
  return NULL;
}

static CALLBACK void* console_modem_rec4(ProtDevice *dev, int press, void *num_as_voidp)
{
  char cmd[32];
  
  audio_rec_stop(0);
  sprintf(cmd, "ATE%04d\r", console_login_timeout_time/10);
  console_send(dev, cmd);
  return console_modem_rec(dev, press, num_as_voidp);
}

static CALLBACK void* console_modem_rec3(ProtDevice *dev, int press, void *num_as_voidp)
{
  char cmd[32];
  int num = (int)num_as_voidp;
  int dur;
  
  dur = audio_rec_stop(1);
  console_send(dev, "ATS03\r");
  sprintf(cmd, str_9_29, num);
  console_send(dev, cmd);
  console_send(dev, str_9_30);
  sprintf(cmd, str_9_31, dur);
  console_send(dev, cmd);
  sprintf(cmd, "ATE%04d\r", console_login_timeout_time/10);
  console_send(dev, cmd);
  
  CONSOLE->last_menu_item = console_modem_rec;
  timeout_on(CONSOLE->timeout, (timeout_func)console_return_to_last_menu_item, dev, 0, 2*TIMEOUT_MSG);
  return NULL;
}

static CALLBACK void* console_modem_rec2(ProtDevice *dev, int press, void *num_as_voidp)
{
  audio_rec_start();
  
  console_send(dev, "ATS03\r");
  console_send(dev, str_9_32);
  if(str_9_33[0])
  {
    console_send(dev, "ATR1\r");
    console_send(dev, str_9_33);
    console_send(dev, "ATR0\r");
  }
  console_send(dev, str_9_34);
  console_send(dev, str_9_35);
  
  console_register_callback(dev, KEY_ENTER, console_modem_rec3, num_as_voidp);
  console_register_callback(dev, KEY_CANCEL, console_modem_rec4, num_as_voidp);
  return NULL;
}

static CALLBACK void* console_modem_rec1(ProtDevice *dev, int press, void *num_as_voidp)
{
  int num = (int)num_as_voidp;
  
  console_send(dev, "ATS03\r");
  
  if(!audio_rec_prepare(num))
  {
    console_send(dev, str_9_36);
    console_send(dev, str_9_37);
    console_send(dev, str_9_38);
    CONSOLE->last_menu_item = console_modem_rec;
    timeout_on(CONSOLE->timeout, (timeout_func)console_return_to_last_menu_item, dev, 0, TIMEOUT_MSG);
    return NULL;
  }
  
  console_send(dev, str_9_39);
  console_send(dev, str_9_40);
  
  console_send(dev, str_9_41);
  if(str_9_33[0])
  {
    console_send(dev, "ATR1\r");
    console_send(dev, str_9_33);
    console_send(dev, "ATR0\r");
  }
  console_send(dev, str_9_43);
  console_send(dev, str_9_44);
  console_send(dev, "ATE0000\r");
  
  console_register_callback(dev, KEY_ENTER, console_modem_rec2, num_as_voidp);
  console_register_callback(dev, KEY_CANCEL, console_modem_rec4, num_as_voidp);
  return NULL;
}

CALLBACK void* console_modem_rec(ProtDevice *dev, int press, void *null)
{
  int i;
  char bufpcm[64], bufwav[64];
  struct stat st;
  
  console_disable_menu(dev, str_9_45);
  
  console_list_free(CONSOLE->support_list);
  CONSOLE->support_list = NULL;
  
  for(i=1; ; i++)
  {
    sprintf(bufpcm, DATA_DIR "/%d.pcm", i);
    sprintf(bufwav, DATA_DIR "/%d.wav", i);
    if(!stat(bufpcm, &st) || !stat(bufwav, &st))
    {
      sprintf(bufpcm, str_9_46, i);
      CONSOLE->support_list = console_list_add(CONSOLE->support_list, bufpcm, NULL, console_modem_rec1, i);
    }
    else
      break;
  }
  CONSOLE->support_list = console_list_add(CONSOLE->support_list, str_9_47, NULL, console_modem_rec1, i);
  console_list_show(dev, CONSOLE->support_list, 0, 4, 0);
  
  return NULL;
}



