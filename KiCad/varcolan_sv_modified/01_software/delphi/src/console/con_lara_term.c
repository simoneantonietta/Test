#include "console.h"
#include "con_lara_term.h"
#include "../lara.h"
#include "../strings.h"
#include "../codec.h"
#include "../volto.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "con_lara_impostazioni.h"

static CALLBACK void* console_lara_term_show_counter(ProtDevice *dev, int press, void *null);
static CALLBACK void* console_lara_term_show_din(ProtDevice *dev, int press, void *null);
static CALLBACK void* console_lara_term_show_fosett(ProtDevice *dev, int press, void *null);
static CALLBACK void* console_lara_term_show_holiday(ProtDevice *dev, int press, void *null);

static MenuItem console_lara_term_menu_1[] = {
	{&str_7_0, L1 | L2 | L3 | L4 | L5 | L6 | L7 | L8, console_lara_term_show_counter},
	{&str_7_1, L1 | L2 | L3 | L4 | L5 | L6 | L7 | L8, console_lara_term_show_fosett},
	{&str_7_2, L1 | L2 | L3 | L4 | L5 | L6 | L7 | L8, console_lara_term_show_holiday},
	{NULL, 0, NULL}
};

static MenuItem console_lara_term_menu_2[] = {
	{&str_7_3, L1 | L2 | L3 | L4 | L5 | L6 | L7 | L8, console_lara_term_show_din},
	{&str_7_1, L1 | L2 | L3 | L4 | L5 | L6 | L7 | L8, console_lara_term_show_fosett},
	{&str_7_2, L1 | L2 | L3 | L4 | L5 | L6 | L7 | L8, console_lara_term_show_holiday},
	{NULL, 0, NULL}
};

static CALLBACK void* console_lara_term_modify_counter(ProtDevice *dev, int press, void *null);
static CALLBACK void* console_lara_term_modify_din(ProtDevice *dev, int press, void *null);
static CALLBACK void* console_lara_term_modify_fosett(ProtDevice *dev, int press, void *null);
static CALLBACK void* console_lara_term_modify_holiday(ProtDevice *dev, int press, void *null);
static CALLBACK void* console_lara_term_modify_area_in(ProtDevice *dev, int press, void *null);
static CALLBACK void* console_lara_term_modify_area_out(ProtDevice *dev, int press, void *null);
static CALLBACK void* console_lara_term_modify_timings(ProtDevice *dev, int press, void *null);
static CALLBACK void* console_lara_term_modify_options(ProtDevice *dev, int press, void *null);

static MenuItem console_lara_term_menu_3[] = {
	{&str_7_0, L1 | L2 | L3 | L4 | L5 | L6 | L7 | L8, console_lara_term_modify_counter},
	{&str_7_1, L1 | L2 | L3 | L4 | L5 | L6 | L7 | L8, console_lara_term_modify_fosett},
	{&str_7_2, L1 | L2 | L3 | L4 | L5 | L6 | L7 | L8, console_lara_term_modify_holiday},
	{&str_7_9, L1 | L2 | L3 | L4 | L5 | L6 | L7 | L8, console_lara_term_modify_area_in},
	{&str_7_10, L1 | L2 | L3 | L4 | L5 | L6 | L7 | L8, console_lara_term_modify_area_out},
	{&str_7_11, L1 | L2 | L3 | L4 | L5 | L6 | L7 | L8, console_lara_term_modify_timings},
	{&str_7_12, L1 | L2 | L3 | L4 | L5 | L6 | L7 | L8, console_lara_term_modify_options},
	{NULL, 0, NULL}
};

static MenuItem console_lara_term_menu_4[] = {
	{&str_7_3, L1 | L2 | L3 | L4 | L5 | L6 | L7 | L8, console_lara_term_modify_din},
	{&str_7_1, L1 | L2 | L3 | L4 | L5 | L6 | L7 | L8, console_lara_term_modify_fosett},
	{&str_7_2, L1 | L2 | L3 | L4 | L5 | L6 | L7 | L8, console_lara_term_modify_holiday},
	{&str_7_9, L1 | L2 | L3 | L4 | L5 | L6 | L7 | L8, console_lara_term_modify_area_in},
	{&str_7_10, L1 | L2 | L3 | L4 | L5 | L6 | L7 | L8, console_lara_term_modify_area_out},
	{&str_7_11, L1 | L2 | L3 | L4 | L5 | L6 | L7 | L8, console_lara_term_modify_timings},
	{&str_7_12, L1 | L2 | L3 | L4 | L5 | L6 | L7 | L8, console_lara_term_modify_options},
	{NULL, 0, NULL}
};

/***********************************************************************
	LARA TERMINALI menu functions
***********************************************************************/

/***********************************************************************
	VISUALIZZA
***********************************************************************/

static CALLBACK void* console_lara_term_show2(ProtDevice *dev, int press, void *term_as_voidp);

static CALLBACK void* console_lara_term_show_counter(ProtDevice *dev, int press, void *term_as_voidp)
{
  char cmd[32];
  int cnt;
  
  console_send(dev, "ATS05\r");
  console_send(dev, str_7_20);
  
  cnt = _lara->terminale[CONSOLE->support_idx].dincnt;
  if(cnt)
  {
    console_send(dev, str_7_21);
    sprintf(cmd, str_7_143, cnt);
    console_send(dev, cmd);
  }
  else
  {
    console_send(dev, str_7_22);
  }
  
  console_register_callback(dev, KEY_CANCEL, console_lara_term_show2, NULL);
  return NULL;
}

static CALLBACK void* console_lara_term_show_din(ProtDevice *dev, int press, void *term_as_voidp)
{
  int cnt;

  console_send(dev, "ATS05\r");
  console_send(dev, str_7_144);
  
  cnt = _lara->terminale[CONSOLE->support_idx].dincnt;
  if(cnt)
    console_send(dev, str_7_23);
  else
    console_send(dev, str_7_24);
  
  console_register_callback(dev, KEY_CANCEL, console_lara_term_show2, NULL);
  return NULL;
}

static CALLBACK void* console_lara_term_show_fosett1(ProtDevice *dev, int press, void *null)
{
  CONSOLE->list_show_cancel = console_lara_term_show_fosett;
  console_lara_impostazioni_fosett_show(dev, 0, (void*)((int)(_lara->terminale[CONSOLE->support_idx].fosett)));
  return NULL;
}

static CALLBACK void* console_lara_term_show_fosett(ProtDevice *dev, int press, void *null)
{
  char *wtmg_cod, *wtmg_desc, cmd[32];

  console_send(dev, "ATS04\r");
  console_send(dev, str_7_25);
  
  if(_lara->terminale[CONSOLE->support_idx].fosett)
  {
    string_weektiming_name(_lara->terminale[CONSOLE->support_idx].fosett-1, &wtmg_cod, &wtmg_desc);
  
    sprintf(cmd, str_7_145, wtmg_cod);
    console_send(dev, cmd);
    sprintf(cmd, str_7_146, wtmg_desc);
    console_send(dev, cmd);
    console_register_callback(dev, KEY_ENTER, console_lara_term_show_fosett1, NULL);
  }
  else
    console_send(dev, str_7_26);
  
  console_register_callback(dev, KEY_CANCEL, console_lara_term_show2, NULL);
  return NULL;
}

static CALLBACK void* console_lara_term_show_holiday(ProtDevice *dev, int press, void *null)
{
  CONSOLE->list_show_cancel = console_lara_term_show2;
  console_lara_impostazioni_holiday_show(dev, 0, _lara->terminale[CONSOLE->support_idx].fest);

  return NULL;
}

static CALLBACK void* console_lara_term_show2(ProtDevice *dev, int press, void *null)
{
  char *term_cod, *term_desc, cmd[32];

  console_send(dev, "ATS03\r");
  
  string_terminal_name(CONSOLE->support_idx, &term_cod, &term_desc);
  
  sprintf(cmd, str_7_147, term_cod);
  console_send(dev, cmd);
  sprintf(cmd, str_7_148, term_desc);
  console_send(dev, cmd);
  
  CONSOLE->list_show_cancel = console_lara_term_show;
  if(_lara->param & LARA_PARAM_COUNTER)
    console_submenu(dev, console_lara_term_menu_1, CONSOLE->support_idx, 3);
  else
    console_submenu(dev, console_lara_term_menu_2, CONSOLE->support_idx, 3);
  
  return NULL;
}

static CALLBACK void* console_lara_term_show1(ProtDevice *dev, int press, void *term_as_voidp)
{
  CONSOLE->support_idx = (int)term_as_voidp - 1;
  console_lara_term_show2(dev, 0, NULL);
  return NULL;
}

CALLBACK void* console_lara_term_show(ProtDevice *dev, int press, void *null)
{
  char *term_cod, *term_desc;
  int i;

  console_disable_menu(dev, str_7_27);
  
  if(!console_check_lara(dev)) return NULL;

  console_list_free(CONSOLE->support_list);
  CONSOLE->support_list = NULL;

  for(i=0; i<LARA_N_TERMINALI; i++)
  {
    /* Il flag master non è mai stato utilizzato. La programmazione come master
       prevede che il terminale successivo sia lo slave, ma per far cosa non è
       mai stato definito.
       Il flag quindi viene riciclato per il blocco timbratura a tempo. */
    //if(!(i && (_lara->terminale[i-1].stato.s.master)) && lara_terminal_prog[i])
    if(lara_terminal_prog[i])
    {
      string_terminal_name(i, &term_cod, &term_desc);
      CONSOLE->support_list = console_list_add(CONSOLE->support_list, term_cod, term_desc, console_lara_term_show1, i+1);
    }
  }

  console_list_show(dev, CONSOLE->support_list, 0, 3, 0);
  console_register_callback(dev, KEY_CANCEL, console_show_menu, NULL);

  return NULL;
}

/***********************************************************************
	MODIFICA
***********************************************************************/

static CALLBACK void* console_lara_term_modify2(ProtDevice *dev, int press, void *null);

static CALLBACK void* console_lara_term_modify_counter1(ProtDevice *dev, int counter_as_int, void *null)
{
  int counter;
  
  console_send(dev, "ATS06\r");
  console_send(dev, "ATL000\r");
  console_send(dev, "ATF0\r");

  sscanf((char*)counter_as_int, "%d", &counter);

  if(counter > 999999)
  {
    console_send(dev, str_7_29);
  }
  else
  {
    console_send(dev, str_7_30);
    lara_set_term_counter(CONSOLE->support_idx, counter);
  }
  
  CONSOLE->last_menu_item = console_lara_term_modify2;
  timeout_on(CONSOLE->timeout, (timeout_func)console_return_to_last_menu_item, dev, 0, TIMEOUT_MSG);

  return NULL;
}

static CALLBACK void* console_lara_term_modify_counter(ProtDevice *dev, int press, void *null)
{
  char cmd[32];
  int cnt;
  
  console_send(dev, "ATS05\r");
  console_send(dev, str_7_31);
  cnt = _lara->terminale[CONSOLE->support_idx].dincnt;
  sprintf(cmd, str_7_149, cnt);
  console_send(dev, cmd);
    
  console_send(dev, str_7_150);
  console_send(dev, "ATL111\r");
  console_send(dev, str_7_151);
  console_send(dev, "ATF1\r");

  console_register_callback(dev, KEY_STRING, console_lara_term_modify_counter1, NULL);
  console_register_callback(dev, KEY_CANCEL, console_lara_term_show2, NULL);
  return NULL;
}

static CALLBACK void* console_lara_term_modify_din2(ProtDevice *dev, int din_as_int, void *null)
{
  int din;
  
  console_send(dev, "ATS06\r");
  console_send(dev, "ATL000\r");
  console_send(dev, "ATF0\r");
  
  din = 0;
  sscanf((char*)din_as_int, "%d", &din);

  if((din > 65529) || strcmp(CONSOLE->temp_string, (char*)din_as_int))
  {
    console_send(dev, str_7_33);
  }
  else
  {
    console_send(dev, str_7_34);
    lara_set_term_din(CONSOLE->support_idx, din);
  }
  
  CONSOLE->last_menu_item = console_lara_term_modify2;
  timeout_on(CONSOLE->timeout, (timeout_func)console_return_to_last_menu_item, dev, 0, TIMEOUT_MSG);

  return NULL;
}

static CALLBACK void* console_lara_term_modify_din1(ProtDevice *dev, int din_as_int, void *null)
{
  free(CONSOLE->temp_string);
  CONSOLE->temp_string = (char*)strdup((char*)din_as_int);
  
  console_send(dev, "ATF0\r");
  console_send(dev, "ATS08\r");
  console_send(dev, str_7_35);
  console_send(dev, str_7_151);
  console_send(dev, "ATF1\r");

  console_register_callback(dev, KEY_STRING, console_lara_term_modify_din2, NULL);
  console_register_callback(dev, KEY_CANCEL, console_lara_term_modify2, NULL);
  return NULL;
}

static CALLBACK void* console_lara_term_modify_din(ProtDevice *dev, int press, void *null)
{
  console_send(dev, "ATS05\r");
  console_send(dev, str_7_36);
  console_send(dev, str_7_37);
  console_send(dev, "ATL121\r");
  console_send(dev, str_7_152);
  console_send(dev, "ATF1\r");

  console_register_callback(dev, KEY_STRING, console_lara_term_modify_din1, NULL);
  console_register_callback(dev, KEY_CANCEL, console_lara_term_modify2, NULL);
  return NULL;
}

static CALLBACK void* console_lara_term_modify_fosett_join1(ProtDevice *dev, int press, void *wtmg_as_voidp)
{
  unsigned char ev[8];
  
  ev[0] = 247;
  ev[1] = 0;
  ev[2] = 49;
  ev[3] = CONSOLE->support_idx+1;
  ev[4] = (int)wtmg_as_voidp;
  codec_queue_event(ev);
  
  _lara->terminale[CONSOLE->support_idx].fosett = (int)wtmg_as_voidp;
  _laraf->terminale[CONSOLE->support_idx] = 1;
  lara_save(0);
  
  console_send(dev, "ATS04\r");
  console_send(dev, str_7_38);
  console_send(dev, str_7_39);
            
  CONSOLE->last_menu_item = console_lara_term_modify_fosett;
  timeout_on(CONSOLE->timeout, (timeout_func)console_return_to_last_menu_item, dev, 0, TIMEOUT_MSG);
  
  return NULL;
}

static CALLBACK void* console_lara_term_modify_fosett_join(ProtDevice *dev, int press, void *null)
{
  char *wtmg_cod, *wtmg_desc;
  int i;

  console_send(dev, "ATS04\r");
  console_send(dev, str_7_40);

  console_list_free(CONSOLE->support_list);
  CONSOLE->support_list = NULL;

  for(i=1; i<=LARA_N_FASCE; i++)
  {
    if(_lara->terminale[CONSOLE->support_idx].fosett != i)
    {
      string_weektiming_name(i-1, &wtmg_cod, &wtmg_desc);
      CONSOLE->support_list = console_list_add(CONSOLE->support_list, wtmg_cod, wtmg_desc, console_lara_term_modify_fosett_join1, i);
    }
  }

  CONSOLE->list_show_cancel = console_lara_term_modify_fosett;
  console_list_show(dev, CONSOLE->support_list, 0, 2, 0);
  
  return NULL;
}


static CALLBACK void* console_lara_term_modify_fosett_delete1(ProtDevice *dev, int press, void *null)
{
  unsigned char ev[8];
  
  ev[0] = 247;
  ev[1] = 0;
  ev[2] = 49;
  ev[3] = CONSOLE->support_idx+1;
  ev[4] = 0;
  codec_queue_event(ev);
  
  _lara->terminale[CONSOLE->support_idx].fosett = 0;
  _laraf->terminale[CONSOLE->support_idx] = 1;
  lara_save(0);
  
  console_send(dev, "ATS04\r");
  console_send(dev, str_7_41);
  console_send(dev, str_7_42);
            
  CONSOLE->last_menu_item = console_lara_term_modify_fosett;
  timeout_on(CONSOLE->timeout, (timeout_func)console_return_to_last_menu_item, dev, 0, TIMEOUT_MSG);
  
  return NULL;
}

static CALLBACK void* console_lara_term_modify_fosett_delete(ProtDevice *dev, int press, void *null)
{
  char *wtmg_cod, *wtmg_desc, cmd[32];

  console_send(dev, "ATS04\r");
  console_send(dev, str_7_43);
  
  if(!_lara->terminale[CONSOLE->support_idx].fosett)
  {
    console_send(dev, str_7_44);
  }
  else
  {
    string_weektiming_name(_lara->terminale[CONSOLE->support_idx].fosett-1, &wtmg_cod, &wtmg_desc);
    sprintf(cmd, str_7_153, wtmg_cod);
    console_send(dev, cmd);
    sprintf(cmd, str_7_145, wtmg_desc);
    console_send(dev, cmd);
    console_register_callback(dev, KEY_ENTER, console_lara_term_modify_fosett_delete1, NULL);
  }
  
  console_register_callback(dev, KEY_CANCEL, console_lara_term_modify_fosett, NULL);
  return NULL;
}

static CALLBACK void* console_lara_term_modify_fosett_show(ProtDevice *dev, int press, void *null)
{
  CONSOLE->list_show_cancel = console_lara_term_modify_fosett;
  console_lara_impostazioni_fosett_show(dev, 0, (void*)((int)(_lara->terminale[CONSOLE->support_idx].fosett)));
    
  return NULL;
}

static MenuItem console_lara_term_modify_fosett_menu[] = {
	{&str_7_45, L1 | L2 | L3 | L4 | L5 | L6 | L7 | L8, console_lara_term_modify_fosett_join},
	{&str_7_46, L1 | L2 | L3 | L4 | L5 | L6 | L7 | L8, console_lara_term_modify_fosett_delete},
	{&str_7_47, L1 | L2 | L3 | L4 | L5 | L6 | L7 | L8, console_lara_term_modify_fosett_show},
	{NULL, 0, NULL}
};

static CALLBACK void* console_lara_term_modify_fosett(ProtDevice *dev, int press, void *null)
{
  console_send(dev, "ATS04\r");
  console_send(dev, str_7_48);

  CONSOLE->list_show_cancel = console_lara_term_modify2;
  console_submenu(dev, console_lara_term_modify_fosett_menu, 0, 3);
  
  return NULL;
}

static CALLBACK void* console_lara_term_modify_holiday_include1(ProtDevice *dev, int press, void *hday_as_voidp)
{
  int hday = (int)hday_as_voidp - 1;
  unsigned char ev[4];
  
  ev[0] = 247;
  ev[1] = 0;
  ev[2] = 50;
  ev[3] = CONSOLE->support_idx+1;
  codec_queue_event(ev);
  
  _lara->terminale[CONSOLE->support_idx].fest[hday>>3] |= (1<<(hday & 0x07));
  _laraf->terminale[CONSOLE->support_idx] = 1;
  lara_save(0);
  
  console_send(dev, "ATS04\r");
  console_send(dev, str_7_49);
  console_send(dev, str_7_50);
            
  CONSOLE->last_menu_item = console_lara_term_modify_holiday;
  timeout_on(CONSOLE->timeout, (timeout_func)console_return_to_last_menu_item, dev, 0, TIMEOUT_MSG);
  
  return NULL;
}

static CALLBACK void* console_lara_term_modify_holiday_include(ProtDevice *dev, int press, void *null)
{
  char *hday_cod, *hday_desc;
  int i;

  console_list_free(CONSOLE->support_list);
  CONSOLE->support_list = NULL;

  console_send(dev, "ATS04\r");
  console_send(dev, str_7_51);

  for(i=0; i<LARA_N_FESTIVI; i++)
  {
    if(!(_lara->terminale[CONSOLE->support_idx].fest[i>>3] & (1<<(i & 0x07))))
    {
      string_holiday_name(i, &hday_cod, &hday_desc);
      CONSOLE->support_list = console_list_add(CONSOLE->support_list, hday_cod, hday_desc, console_lara_term_modify_holiday_include1, i+1);
    }
  }

  CONSOLE->list_show_cancel = console_lara_term_modify_holiday;
  console_list_show(dev, CONSOLE->support_list, 0, 2, 0);
  
  return NULL;
}

static CALLBACK void* console_lara_term_modify_holiday_exclude1(ProtDevice *dev, int press, void *hday_as_voidp)
{
  int hday = (int)hday_as_voidp - 1;
  unsigned char ev[4];
  
  ev[0] = 247;
  ev[1] = 0;
  ev[2] = 50;
  ev[3] = CONSOLE->support_idx+1;
  codec_queue_event(ev);
  
  _lara->terminale[CONSOLE->support_idx].fest[hday>>3] &= ~(1<<(hday & 0x07));
  _laraf->terminale[CONSOLE->support_idx] = 1;
  lara_save(0);
  
  console_send(dev, "ATS04\r");
  console_send(dev, str_7_52);
  console_send(dev, str_7_53);
            
  CONSOLE->last_menu_item = console_lara_term_modify_holiday;
  timeout_on(CONSOLE->timeout, (timeout_func)console_return_to_last_menu_item, dev, 0, TIMEOUT_MSG);
  
  return NULL;
}

static CALLBACK void* console_lara_term_modify_holiday_exclude(ProtDevice *dev, int press, void *null)
{
  char *hday_cod, *hday_desc;
  int i;

  console_list_free(CONSOLE->support_list);
  CONSOLE->support_list = NULL;

  console_send(dev, "ATS04\r");
  console_send(dev, str_7_54);

  for(i=0; i<LARA_N_FESTIVI; i++)
  {
    if(_lara->terminale[CONSOLE->support_idx].fest[i>>3] & (1<<(i & 0x07)))
    {
      string_holiday_name(i, &hday_cod, &hday_desc);
      CONSOLE->support_list = console_list_add(CONSOLE->support_list, hday_cod, hday_desc, console_lara_term_modify_holiday_exclude1, i+1);
    }
  }

  CONSOLE->list_show_cancel = console_lara_term_modify_holiday;
  console_list_show(dev, CONSOLE->support_list, 0, 2, 0);
  
  return NULL;
}

static CALLBACK void* console_lara_term_modify_holiday_show(ProtDevice *dev, int press, void *null)
{
  CONSOLE->list_show_cancel = console_lara_term_modify_holiday;
  console_lara_impostazioni_holiday_show(dev, 0, _lara->terminale[CONSOLE->support_idx].fest);
  
  return NULL;
}

static MenuItem console_lara_term_modify_holiday_menu[] = {
	{&str_7_55, L1 | L2 | L3 | L4 | L5 | L6 | L7 | L8, console_lara_term_modify_holiday_include},
	{&str_7_56, L1 | L2 | L3 | L4 | L5 | L6 | L7 | L8, console_lara_term_modify_holiday_exclude},
	{&str_7_57, L1 | L2 | L3 | L4 | L5 | L6 | L7 | L8, console_lara_term_modify_holiday_show},
	{NULL, 0, NULL}
};

static CALLBACK void* console_lara_term_modify_holiday(ProtDevice *dev, int press, void *null)
{
  console_send(dev, "ATS04\r");
  console_send(dev, str_7_58);

  CONSOLE->list_show_cancel = console_lara_term_modify2;
  console_submenu(dev, console_lara_term_modify_holiday_menu, 0, 3);
  return NULL;
}

static CALLBACK void* console_lara_term_modify_area_in1(ProtDevice *dev, int press, void *area_as_voidp)
{  
  unsigned char ev[8];
  
  _lara->terminale[CONSOLE->support_idx].area[0] = (int)area_as_voidp;
  _laraf->terminale[CONSOLE->support_idx] = 1;
  lara_save(0);

  ev[0] = 247;
  ev[1] = 0;
  ev[2] = 48;
  ev[3] = CONSOLE->support_idx+1;
  ev[4] = _lara->terminale[CONSOLE->support_idx].area[0];
  ev[5] = _lara->terminale[CONSOLE->support_idx].area[1];
  codec_queue_event(ev);
  
  console_send(dev, "ATS04\r");
  console_send(dev, str_7_59);
  console_send(dev, str_7_60);
  
  CONSOLE->last_menu_item = console_lara_term_modify2;
  timeout_on(CONSOLE->timeout, (timeout_func)console_return_to_last_menu_item, dev, 0, TIMEOUT_MSG);

  return NULL;
}

static CALLBACK void* console_lara_term_modify_area_out1(ProtDevice *dev, int press, void *area_as_voidp)
{
  unsigned char ev[8];
  
  _lara->terminale[CONSOLE->support_idx].area[1] = (int)area_as_voidp;
  _laraf->terminale[CONSOLE->support_idx] = 1;
  lara_save(0);

  ev[0] = 247;
  ev[1] = 0;
  ev[2] = 48;
  ev[3] = CONSOLE->support_idx+1;
  ev[4] = _lara->terminale[CONSOLE->support_idx].area[0];
  ev[5] = _lara->terminale[CONSOLE->support_idx].area[1];
  codec_queue_event(ev);

  console_send(dev, "ATS04\r");
  console_send(dev, str_7_61);
  console_send(dev, str_7_62);
  
  CONSOLE->last_menu_item = console_lara_term_modify2;
  timeout_on(CONSOLE->timeout, (timeout_func)console_return_to_last_menu_item, dev, 0, TIMEOUT_MSG);

  return NULL;
}

static void console_lara_term_modify_area(ProtDevice *dev, int inout)
{
  char *area_cod, *area_desc, cmd[32];
  int i;
  
  string_area_name(_lara->terminale[CONSOLE->support_idx].area[inout], &area_cod, &area_desc);
  sprintf(cmd, str_7_154, area_cod);
  console_send(dev, cmd);
  sprintf(cmd, str_7_155, area_desc);
  console_send(dev, cmd);
  console_send(dev, str_7_63);
  
  console_list_free(CONSOLE->support_list);
  CONSOLE->support_list = NULL;

  for(i=0; i<LARA_N_AREE; i++)
  {
    if(i != _lara->terminale[CONSOLE->support_idx].area[inout])
    {
      string_area_name(i, &area_cod, &area_desc);
      if(inout)
        CONSOLE->support_list = console_list_add(CONSOLE->support_list, area_cod, area_desc, console_lara_term_modify_area_out1, i);
      else
        CONSOLE->support_list = console_list_add(CONSOLE->support_list, area_cod, area_desc, console_lara_term_modify_area_in1, i);
    }
  }

  /* Aggiunge l'AreaSelf */
  if(_lara->terminale[CONSOLE->support_idx].area[inout] != TEBE_AREASELF)
  {
    string_area_name(TEBE_AREASELF, &area_cod, &area_desc);
    if(inout)
      CONSOLE->support_list = console_list_add(CONSOLE->support_list, area_cod, area_desc, console_lara_term_modify_area_out1, TEBE_AREASELF);
    else
      CONSOLE->support_list = console_list_add(CONSOLE->support_list, area_cod, area_desc, console_lara_term_modify_area_in1, TEBE_AREASELF);
  }
  
  console_list_show(dev, CONSOLE->support_list, 0, 1, 0);
  console_register_callback(dev, KEY_CANCEL, console_lara_term_modify2, NULL);
}

static CALLBACK void* console_lara_term_modify_area_in(ProtDevice *dev, int press, void *null)
{  
  console_send(dev, "ATS04\r");
  console_send(dev, str_7_64);
  
  console_lara_term_modify_area(dev, 0);
  
  return NULL;
}

static CALLBACK void* console_lara_term_modify_area_out(ProtDevice *dev, int press, void *null)
{
  console_send(dev, "ATS04\r");
  console_send(dev, str_7_65);
  
  console_lara_term_modify_area(dev, 1);
  
  return NULL;
}

static CALLBACK void* console_lara_term_modify_timings_passback1(ProtDevice *dev, int time_as_int, void *null)
{
  int t;
  
  console_send(dev, "ATF0\r");
  console_send(dev, "ATS06\r");
  
  t = -1;
  sscanf((char*)time_as_int, "%d", &t);
  if((t < 0) || (t > 15))
    console_send(dev, str_7_66);
  else
  {
    lara_set_term_times(CONSOLE->support_idx, _lara->terminale[CONSOLE->support_idx].timeopen,
        _lara->terminale[CONSOLE->support_idx].timeopentimeout, t);
    console_send(dev, str_7_67);
  }
  
  CONSOLE->last_menu_item = console_lara_term_modify_timings;
  timeout_on(CONSOLE->timeout, (timeout_func)console_return_to_last_menu_item, dev, 0, TIMEOUT_MSG);

  return NULL;
}

static CALLBACK void* console_lara_term_modify_timings_passback(ProtDevice *dev, int press, void *null)
{
  char cmd[32];
  
  console_send(dev, "ATS04\r");
  console_send(dev, str_7_68);
  console_send(dev, str_7_69);
  sprintf(cmd, str_7_70, _lara->terminale[CONSOLE->support_idx].conf.s.tapbk);
  console_send(dev, cmd);
  console_send(dev, str_7_71);
  console_send(dev, "ATL111\r");
  console_send(dev, str_7_156);
  console_send(dev, "ATF1\r");
  
  console_register_callback(dev, KEY_STRING, console_lara_term_modify_timings_passback1, NULL);
  console_register_callback(dev, KEY_CANCEL, console_lara_term_modify_timings, NULL);
  return NULL;
}

static CALLBACK void* console_lara_term_modify_timings_doortimeout1(ProtDevice *dev, int time_as_int, void *null)
{
  int t;
  
  console_send(dev, "ATF0\r");
  console_send(dev, "ATS06\r");
  
  t = -1;
  sscanf((char*)time_as_int, "%d", &t);
  if((t < 0) || (t > 255))
    console_send(dev, str_7_72);
  else
  {
    lara_set_term_times(CONSOLE->support_idx, _lara->terminale[CONSOLE->support_idx].timeopen,
        t, _lara->terminale[CONSOLE->support_idx].conf.s.tapbk);
    console_send(dev, str_7_73);
  }
  
  CONSOLE->last_menu_item = console_lara_term_modify_timings;
  timeout_on(CONSOLE->timeout, (timeout_func)console_return_to_last_menu_item, dev, 0, TIMEOUT_MSG);

  return NULL;
}

static CALLBACK void* console_lara_term_modify_timings_doortimeout(ProtDevice *dev, int press, void *null)
{
  char cmd[32];
  
  console_send(dev, "ATS04\r");
  console_send(dev, str_7_74);
  console_send(dev, str_7_75);
  sprintf(cmd, str_7_76, _lara->terminale[CONSOLE->support_idx].timeopentimeout);
  console_send(dev, cmd);
  console_send(dev, str_7_77);
  console_send(dev, "ATL111\r");
  console_send(dev, str_7_156);
  console_send(dev, "ATF1\r");
  
  console_register_callback(dev, KEY_STRING, console_lara_term_modify_timings_doortimeout1, NULL);
  console_register_callback(dev, KEY_CANCEL, console_lara_term_modify_timings, NULL);
  return NULL;
}

static CALLBACK void* console_lara_term_modify_timings_onrele1(ProtDevice *dev, int time_as_int, void *null)
{
  int t;
  
  console_send(dev, "ATF0\r");
  console_send(dev, "ATS06\r");
  
  t = -1;
  sscanf((char*)time_as_int, "%d", &t);
  if((t < 0) || (t > 255))
    console_send(dev, str_7_81);
  else
  {
    lara_set_term_times(CONSOLE->support_idx, t,
        _lara->terminale[CONSOLE->support_idx].timeopentimeout,
        _lara->terminale[CONSOLE->support_idx].conf.s.tapbk);
    console_send(dev, str_7_82);
  }
  
  CONSOLE->last_menu_item = console_lara_term_modify_timings;
  timeout_on(CONSOLE->timeout, (timeout_func)console_return_to_last_menu_item, dev, 0, TIMEOUT_MSG);

  return NULL;
}

static CALLBACK void* console_lara_term_modify_timings_onrele(ProtDevice *dev, int press, void *null)
{
  char cmd[32];
  
  console_send(dev, "ATS04\r");
  console_send(dev, str_7_83);
  console_send(dev, str_7_84);
  sprintf(cmd, str_7_85, _lara->terminale[CONSOLE->support_idx].timeopen);
  console_send(dev, cmd);
  console_send(dev, str_7_86);
  console_send(dev, "ATL111\r");
  console_send(dev, str_7_156);
  console_send(dev, "ATF1\r");
  
  console_register_callback(dev, KEY_STRING, console_lara_term_modify_timings_onrele1, NULL);
  console_register_callback(dev, KEY_CANCEL, console_lara_term_modify_timings, NULL);
  return NULL;
}

static MenuItem console_lara_term_modify_timings_menu[] = {
	{&str_7_87, L1 | L2 | L3 | L4 | L5 | L6 | L7 | L8, console_lara_term_modify_timings_passback},
	{&str_7_88, L1 | L2 | L3 | L4 | L5 | L6 | L7 | L8, console_lara_term_modify_timings_doortimeout},
	{&str_7_89, L1 | L2 | L3 | L4 | L5 | L6 | L7 | L8, console_lara_term_modify_timings_onrele},
	{NULL, 0, NULL}
};

static CALLBACK void* console_lara_term_modify_timings(ProtDevice *dev, int press, void *null)
{
  console_send(dev, "ATF0\r");
  console_send(dev, "ATS04\r");
  console_send(dev, str_7_90);
  
  CONSOLE->list_show_cancel = console_lara_term_modify2;
  console_submenu(dev, console_lara_term_modify_timings_menu, 0, 3);
  return NULL;
}

static CALLBACK void* console_lara_term_modify_options_passback1(ProtDevice *dev, int press, void *active_as_voidp)
{
  unsigned char ev[8];
  
  console_send(dev, "ATS05\r");
  
  _lara->terminale[CONSOLE->support_idx].stato.s.apbk = (int)active_as_voidp;
  _laraf->terminale[CONSOLE->support_idx] = 1;
  lara_save(0);
  
  ev[0] = 247;
  ev[1] = 0;
  ev[2] = 53;
  ev[3] = CONSOLE->support_idx+1;
  ev[4] = _lara->terminale[CONSOLE->support_idx].stato.b;
  codec_queue_event(ev);
  
  if(active_as_voidp)
    console_send(dev, str_7_91);
  else
    console_send(dev, str_7_92);
  
  CONSOLE->last_menu_item = console_lara_term_modify_options;
  timeout_on(CONSOLE->timeout, (timeout_func)console_return_to_last_menu_item, dev, 0, TIMEOUT_MSG);
  return NULL;
}

static CALLBACK void* console_lara_term_modify_options_passback(ProtDevice *dev, int press, void *null)
{
  console_send(dev, "ATS04\r");
  console_send(dev, str_7_93);
  
  if(!_lara->terminale[CONSOLE->support_idx].stato.s.apbk)
  {
    console_send(dev, str_7_94);
    console_send(dev, str_7_95);
    console_register_callback(dev, KEY_ENTER, console_lara_term_modify_options_passback1, (void*)1);
  }
  else
  {
    console_send(dev, str_7_96);
    console_send(dev, str_7_97);
    console_register_callback(dev, KEY_ENTER, console_lara_term_modify_options_passback1, (void*)0);
  }
  
  console_register_callback(dev, KEY_CANCEL, console_lara_term_modify_options, NULL);
  return NULL;
}

static CALLBACK void* console_lara_term_modify_options_blkbadge2(ProtDevice *dev, int press, void *blkbadge_as_voidp)
{
  unsigned char ev[8];
  
  console_send(dev, "ATS05\r");
  //console_send(dev, str_7_98);
  
  _lara->terminale[CONSOLE->support_idx].stato.s.blkbadge = (int)blkbadge_as_voidp;
  _laraf->terminale[CONSOLE->support_idx] = 1;
  lara_save(0);

  ev[0] = 247;
  ev[1] = 0;
  ev[2] = 53;
  ev[3] = CONSOLE->support_idx+1;
  ev[4] = _lara->terminale[CONSOLE->support_idx].stato.b;
  codec_queue_event(ev);
  
  if(blkbadge_as_voidp)
    console_send(dev, str_7_94);
  else
    console_send(dev, str_7_96);
  
  CONSOLE->last_menu_item = console_lara_term_modify_options;
  timeout_on(CONSOLE->timeout, (timeout_func)console_return_to_last_menu_item, dev, 0, TIMEOUT_MSG);
  return NULL;
}

#if 0
static CALLBACK void* console_lara_term_modify_options_blkbadge1(ProtDevice *dev, int press, void *blkbadge_as_voidp)
{
  char *term_cod, cmd[32];
  
  console_send(dev, "ATS04\r");
  console_send(dev, str_7_101);
  string_terminal_name(CONSOLE->support_idx+1, &term_cod, NULL);
  sprintf(cmd, "ATN0305%s\r", term_cod);
  console_send(dev, cmd);
  console_send(dev, str_7_102);
  console_send(dev, str_7_103);
  
  console_register_callback(dev, KEY_ENTER, console_lara_term_modify_options_blkbadge2, (void*)1);
  console_register_callback(dev, KEY_CANCEL, console_lara_term_modify_options, NULL);
  return NULL;
}
#endif

static CALLBACK void* console_lara_term_modify_options_blkbadge(ProtDevice *dev, int press, void *null)
{
  console_send(dev, "ATS04\r");
  console_send(dev, str_7_104);
  
  if(_lara->terminale[CONSOLE->support_idx].stato.s.blkbadge)
  {
    console_send(dev, str_7_94);
    console_send(dev, str_7_95);
    console_register_callback(dev, KEY_ENTER, console_lara_term_modify_options_blkbadge2, (void*)0);
  }
  else
  {
    console_send(dev, str_7_96);
    console_send(dev, str_7_97);
    //console_register_callback(dev, KEY_ENTER, console_lara_term_modify_options_blkbadge1, NULL);
    console_register_callback(dev, KEY_ENTER, console_lara_term_modify_options_blkbadge2, (void*)1);
  }
  
  console_register_callback(dev, KEY_CANCEL, console_lara_term_modify_options, NULL);
  return NULL;
}

static CALLBACK void* console_lara_term_modify_options_keyblock1(ProtDevice *dev, int press, void *block_as_voidp)
{
  unsigned char ev[8];
  
  console_send(dev, "ATS05\r");
  
  _lara->terminale[CONSOLE->support_idx].stato.s.blktas = (int)block_as_voidp;
  _laraf->terminale[CONSOLE->support_idx] = 1;
  lara_save(0);

  ev[0] = 247;
  ev[1] = 0;
  ev[2] = 53;
  ev[3] = CONSOLE->support_idx+1;
  ev[4] = _lara->terminale[CONSOLE->support_idx].stato.b;
  codec_queue_event(ev);
  
  if(block_as_voidp)
    console_send(dev, str_7_109);
  else
    console_send(dev, str_7_110);
  
  CONSOLE->last_menu_item = console_lara_term_modify_options;
  timeout_on(CONSOLE->timeout, (timeout_func)console_return_to_last_menu_item, dev, 0, TIMEOUT_MSG);
  return NULL;
}

static CALLBACK void* console_lara_term_modify_options_keyblock(ProtDevice *dev, int press, void *null)
{
  console_send(dev, "ATS04\r");
  console_send(dev, str_7_111);
  
  if(_lara->terminale[CONSOLE->support_idx].stato.s.blktas)
  {
    console_send(dev, str_7_112);
    console_send(dev, str_7_113);
    console_register_callback(dev, KEY_ENTER, console_lara_term_modify_options_keyblock1, (void*)0);
  }
  else
  {
    console_send(dev, str_7_114);
    console_send(dev, str_7_115);
    console_register_callback(dev, KEY_ENTER, console_lara_term_modify_options_keyblock1, (void*)1);
  }
  
  console_register_callback(dev, KEY_CANCEL, console_lara_term_modify_options, NULL);
  return NULL;
}

static CALLBACK void* console_lara_term_modify_options_filter_in1(ProtDevice *dev, int press, void *filter_as_voidp)
{
  unsigned char ev[8];
  
  _lara->terminale[CONSOLE->support_idx].conf.s.filtro &= 0x03;
  _lara->terminale[CONSOLE->support_idx].conf.s.filtro |= ((int)filter_as_voidp << 2);
  _laraf->terminale[CONSOLE->support_idx] = 1;
  lara_save(0);
  
  ev[0] = 247;
  ev[1] = 0;
  ev[2] = 47;
  ev[3] = CONSOLE->support_idx+1;
  ev[4] = _lara->terminale[CONSOLE->support_idx].conf.s.filtro;
  codec_queue_event(ev);
  
  console_send(dev, "ATS05\r");
  console_send(dev, str_7_116);
  
  CONSOLE->last_menu_item = console_lara_term_modify_options;
  timeout_on(CONSOLE->timeout, (timeout_func)console_return_to_last_menu_item, dev, 0, TIMEOUT_MSG);
  return NULL;
}

static CALLBACK void* console_lara_term_modify_options_filter_out1(ProtDevice *dev, int press, void *filter_as_voidp)
{
  unsigned char ev[8];
  
  _lara->terminale[CONSOLE->support_idx].conf.s.filtro &= 0x0c;
  _lara->terminale[CONSOLE->support_idx].conf.s.filtro |= (int)filter_as_voidp;
  _laraf->terminale[CONSOLE->support_idx] = 1;
  lara_save(0);
  
  ev[0] = 247;
  ev[1] = 0;
  ev[2] = 47;
  ev[3] = CONSOLE->support_idx+1;
  ev[4] = _lara->terminale[CONSOLE->support_idx].conf.s.filtro;
  codec_queue_event(ev);
  
  console_send(dev, "ATS05\r");
  console_send(dev, str_7_117);

  CONSOLE->last_menu_item = console_lara_term_modify_options;
  timeout_on(CONSOLE->timeout, (timeout_func)console_return_to_last_menu_item, dev, 0, TIMEOUT_MSG);
  return NULL;
}

static void console_lara_term_modify_options_filter(ProtDevice *dev, int inout)
{
  int filtro;
  
  filtro = _lara->terminale[CONSOLE->support_idx].conf.s.filtro;
  if(!inout) filtro >>= 2;
  filtro &= 0x03;
  
  switch(filtro)
  {
    case 0:
      console_send(dev, str_7_118);
      break;
    case 1:
      console_send(dev, str_7_119);
      break;
    case 2:
      console_send(dev, str_7_120);
      break;
  }
  
  console_list_free(CONSOLE->support_list);
  CONSOLE->support_list = NULL;

  if(filtro != 0)
  {
    if(inout)
      CONSOLE->support_list = console_list_add(CONSOLE->support_list, str_7_121, NULL, console_lara_term_modify_options_filter_out1, 0);
    else
      CONSOLE->support_list = console_list_add(CONSOLE->support_list, str_7_121, NULL, console_lara_term_modify_options_filter_in1, 0);
  }
  if(filtro != 1)
  {
    if(inout)
      CONSOLE->support_list = console_list_add(CONSOLE->support_list, str_7_123, NULL, console_lara_term_modify_options_filter_out1, 1);
    else
      CONSOLE->support_list = console_list_add(CONSOLE->support_list, str_7_123, NULL, console_lara_term_modify_options_filter_in1, 1);
  }
  if(filtro != 2)
  {
    if(inout)
      CONSOLE->support_list = console_list_add(CONSOLE->support_list, str_7_125, NULL, console_lara_term_modify_options_filter_out1, 2);
    else
      CONSOLE->support_list = console_list_add(CONSOLE->support_list, str_7_125, NULL, console_lara_term_modify_options_filter_in1, 2);
  }
  
  CONSOLE->list_show_cancel = console_lara_term_modify_options;
  console_list_show(dev, CONSOLE->support_list, 0, 2, 0);
}

static CALLBACK void* console_lara_term_modify_options_filter_in(ProtDevice *dev, int press, void *null)
{
  console_send(dev, "ATS04\r");
  console_send(dev, str_7_127);
  
  console_lara_term_modify_options_filter(dev, 0);
//  console_register_callback(dev, KEY_CANCEL, console_lara_term_modify_options, NULL);
  return NULL;
}

static CALLBACK void* console_lara_term_modify_options_filter_out(ProtDevice *dev, int press, void *null)
{
  console_send(dev, "ATS04\r");
  console_send(dev, str_7_128);
  
  console_lara_term_modify_options_filter(dev, 1);
//  console_register_callback(dev, KEY_CANCEL, console_lara_term_modify_options, NULL);
  return NULL;
}

static CALLBACK void* console_lara_term_modify_options_biometric1(ProtDevice *dev, int press, void *null)
{
  unsigned char ev[8];
  
  _lara->terminale[CONSOLE->support_idx].stato.s.volto ^= 1;
  _laraf->terminale[CONSOLE->support_idx] = 1;
  lara_save(0);
  volto_init();
  
  ev[0] = 247;
  ev[1] = 0;
  ev[2] = 53;
  ev[3] = CONSOLE->support_idx+1;
  ev[4] = _lara->terminale[CONSOLE->support_idx].stato.b;
  codec_queue_event(ev);
  
  console_send(dev, "ATS05\r");
  
  if(_lara->terminale[CONSOLE->support_idx].stato.s.volto)
    console_send(dev, str_7_139);
  else
    console_send(dev, str_7_141);
  
  CONSOLE->last_menu_item = console_lara_term_modify_options;
  timeout_on(CONSOLE->timeout, (timeout_func)console_return_to_last_menu_item, dev, 0, TIMEOUT_MSG);
  return NULL;
}

static CALLBACK void* console_lara_term_modify_options_biometric(ProtDevice *dev, int press, void *null)
{
  console_send(dev, "ATS04\r");
  console_send(dev, str_7_137);
  console_send(dev, str_7_138);
  
  if(_lara->terminale[CONSOLE->support_idx].stato.s.volto)
  {
    console_send(dev, str_7_139);
    console_send(dev, str_7_140);
  }
  else
  {
    console_send(dev, str_7_141);
    console_send(dev, str_7_142);
  }
  
  console_register_callback(dev, KEY_ENTER, console_lara_term_modify_options_biometric1, NULL);
  console_register_callback(dev, KEY_CANCEL, console_lara_term_modify_options, NULL);
  return NULL;
}

static CALLBACK void* console_lara_term_modify_options_restricted1(ProtDevice *dev, int press, void *null)
{
  unsigned char ev[8];
  
  _lara->terminale[CONSOLE->support_idx].stato.s.acc_ris ^= 1;
  _laraf->terminale[CONSOLE->support_idx] = 1;
  lara_save(0);
  
  ev[0] = 247;
  ev[1] = 0;
  ev[2] = 53;
  ev[3] = CONSOLE->support_idx+1;
  ev[4] = _lara->terminale[CONSOLE->support_idx].stato.b;
  codec_queue_event(ev);
  
  console_send(dev, "ATS05\r");
  
  if(_lara->terminale[CONSOLE->support_idx].stato.s.acc_ris)
    console_send(dev, str_7_139);
  else
    console_send(dev, str_7_141);
  
  CONSOLE->last_menu_item = console_lara_term_modify_options;
  timeout_on(CONSOLE->timeout, (timeout_func)console_return_to_last_menu_item, dev, 0, TIMEOUT_MSG);
  return NULL;
}

static CALLBACK void* console_lara_term_modify_options_restricted(ProtDevice *dev, int press, void *null)
{
  console_send(dev, "ATS04\r");
  console_send(dev, str_7_158);
  console_send(dev, str_7_159);
  
  if(_lara->terminale[CONSOLE->support_idx].stato.s.acc_ris)
  {
    console_send(dev, str_7_139);
    console_send(dev, str_7_140);
  }
  else
  {
    console_send(dev, str_7_141);
    console_send(dev, str_7_142);
  }
  
  console_register_callback(dev, KEY_ENTER, console_lara_term_modify_options_restricted1, NULL);
  console_register_callback(dev, KEY_CANCEL, console_lara_term_modify_options, NULL);
  return NULL;
}

static MenuItem console_lara_term_modify_options_menu[] = {
	{&str_7_129, L1 | L2 | L3 | L4 | L5 | L6 | L7 | L8, console_lara_term_modify_options_passback},
	{&str_7_130, L1 | L2 | L3 | L4 | L5 | L6 | L7 | L8, console_lara_term_modify_options_blkbadge},
	{&str_7_131, L1 | L2 | L3 | L4 | L5 | L6 | L7 | L8, console_lara_term_modify_options_keyblock},
	{&str_7_135, L1 | L2 | L3 | L4 | L5 | L6 | L7 | L8, console_lara_term_modify_options_biometric},
	{&str_7_132, L1 | L2 | L3 | L4 | L5 | L6 | L7 | L8, console_lara_term_modify_options_filter_in},
	{&str_7_133, L1 | L2 | L3 | L4 | L5 | L6 | L7 | L8, console_lara_term_modify_options_filter_out},
	{&str_7_157, L1 | L2 | L3 | L4 | L5 | L6 | L7 | L8, console_lara_term_modify_options_restricted},
	{NULL, 0, NULL}
};

static CALLBACK void* console_lara_term_modify_options(ProtDevice *dev, int press, void *null)
{
  console_send(dev, "ATS03\r");
  console_send(dev, str_7_134);
    
  CONSOLE->list_show_cancel = console_lara_term_modify2;
  console_submenu(dev, console_lara_term_modify_options_menu, 0, 3);
  return NULL;
}

static CALLBACK void* console_lara_term_modify2(ProtDevice *dev, int press, void *null)
{
  char *term_cod, *term_desc, cmd[32];

  console_send(dev, "ATF0\r");
  console_send(dev, "ATS03\r");
  
  string_terminal_name(CONSOLE->support_idx, &term_cod, &term_desc);
  
  sprintf(cmd, str_7_147, term_cod);
  console_send(dev, cmd);
  sprintf(cmd, str_7_148, term_desc);
  console_send(dev, cmd);
  
  CONSOLE->list_show_cancel = console_lara_term_modify;
  if(_lara->param & LARA_PARAM_COUNTER)
    console_submenu(dev, console_lara_term_menu_3, CONSOLE->support_idx, 3);
  else
    console_submenu(dev, console_lara_term_menu_4, CONSOLE->support_idx, 3);
  
  return NULL;
}

static CALLBACK void* console_lara_term_modify1(ProtDevice *dev, int press, void *term_as_voidp)
{
  CONSOLE->support_idx = (int)term_as_voidp - 1;
  console_lara_term_modify2(dev, 0, NULL);
  return NULL;
}

CALLBACK void* console_lara_term_modify(ProtDevice *dev, int press, void *null)
{
  char *term_cod, *term_desc;
  int i;
  
  console_disable_menu(dev, str_7_27);

  if(!console_check_lara(dev)) return NULL;

  console_list_free(CONSOLE->support_list);
  CONSOLE->support_list = NULL;

  for(i=0; i<LARA_N_TERMINALI; i++)
  {
    /* Il flag master non è mai stato utilizzato. La programmazione come master
       prevede che il terminale successivo sia lo slave, ma per far cosa non è
       mai stato definito.
       Il flag quindi viene riciclato per il blocco timbratura a tempo. */
    //if(!(i && (_lara->terminale[i-1].stato.s.master)) && lara_terminal_prog[i])
    if(lara_terminal_prog[i])
    {
      string_terminal_name(i, &term_cod, &term_desc);
      CONSOLE->support_list = console_list_add(CONSOLE->support_list, term_cod, term_desc, console_lara_term_modify1, i+1);
    }
  }

  console_list_show(dev, CONSOLE->support_list, 0, 3, 0);
  console_register_callback(dev, KEY_CANCEL, console_show_menu, NULL);
  return NULL;
}

