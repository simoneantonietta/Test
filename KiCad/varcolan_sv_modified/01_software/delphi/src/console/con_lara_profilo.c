#include "console.h"
#include "con_lara_profilo.h"
#include "../lara.h"
#include "../strings.h"
#include "../codec.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "con_lara_impostazioni.h"

static CALLBACK void* console_lara_profile_show_term(ProtDevice *dev, int press, void *null);
static CALLBACK void* console_lara_profile_show_fosett(ProtDevice *dev, int press, void *null);
static CALLBACK void* console_lara_profile_show_holiday(ProtDevice *dev, int press, void *null);
static CALLBACK void* console_lara_profile_show_option(ProtDevice *dev, int press, void *null);

static MenuItem console_lara_profile_menu_1[] = {
	{&str_6_0, L1 | L2 | L3 | L4 | L5 | L6 | L7 | L8, console_lara_profile_show_term},
	{&str_6_1, L1 | L2 | L3 | L4 | L5 | L6 | L7 | L8, console_lara_profile_show_fosett},
	{&str_6_2, L1 | L2 | L3 | L4 | L5 | L6 | L7 | L8, console_lara_profile_show_holiday},
	{&str_6_3, L1 | L2 | L3 | L4 | L5 | L6 | L7 | L8, console_lara_profile_show_option},
	{NULL, 0, NULL}
};

static CALLBACK void* console_lara_profile_modify_term(ProtDevice *dev, int press, void *null);
static CALLBACK void* console_lara_profile_modify_fosett(ProtDevice *dev, int press, void *null);
static CALLBACK void* console_lara_profile_modify_holiday(ProtDevice *dev, int press, void *null);
static CALLBACK void* console_lara_profile_modify_option(ProtDevice *dev, int press, void *null);

static MenuItem console_lara_profile_menu_2[] = {
	{&str_6_0, L1 | L2 | L3 | L4 | L5 | L6 | L7 | L8, console_lara_profile_modify_term},
	{&str_6_1, L1 | L2 | L3 | L4 | L5 | L6 | L7 | L8, console_lara_profile_modify_fosett},
	{&str_6_2, L1 | L2 | L3 | L4 | L5 | L6 | L7 | L8, console_lara_profile_modify_holiday},
	{&str_6_3, L1 | L2 | L3 | L4 | L5 | L6 | L7 | L8, console_lara_profile_modify_option},
	{NULL, 0, NULL}
};

/***********************************************************************
	LARA TERMINALI menu functions
***********************************************************************/

/***********************************************************************
	VISUALIZZA
***********************************************************************/

static CALLBACK void* console_lara_profile_show2(ProtDevice *dev, int press, void *null);

static CALLBACK void* console_lara_profile_show_term(ProtDevice *dev, int press, void *null)
{
  char *term_cod, *term_desc;
  int i;

  console_list_free(CONSOLE->support_list);
  CONSOLE->support_list = NULL;

  console_send(dev, "ATS04\r");
  console_send(dev, str_6_8);

  for(i=0; i<LARA_N_TERMINALI; i++)
  {
    if(_lara->profilo[CONSOLE->support_idx].term[i>>3] & (1<<(i & 0x07)))
    {
      string_terminal_name(i, &term_cod, &term_desc);
      CONSOLE->support_list = console_list_add(CONSOLE->support_list, term_cod, term_desc, NULL, i+1);
    }
  }

  CONSOLE->list_show_cancel = console_lara_profile_show2;
  console_list_show(dev, CONSOLE->support_list, 0, 2, 0);
  
  return NULL;
}

static CALLBACK void* console_lara_profile_show_fosett1(ProtDevice *dev, int press, void *null)
{
  CONSOLE->list_show_cancel = console_lara_profile_show_fosett;
  console_lara_impostazioni_fosett_show(dev, 0, (void*)((int)(_lara->profilo[CONSOLE->support_idx].fosett)));
  return NULL;
}

static CALLBACK void* console_lara_profile_show_fosett(ProtDevice *dev, int press, void *null)
{
  char *wtmg_cod, *wtmg_desc, cmd[32];

  console_send(dev, "ATS04\r");
  console_send(dev, str_6_9);
  
  if(_lara->profilo[CONSOLE->support_idx].fosett)
  {
    string_weektiming_name(_lara->profilo[CONSOLE->support_idx].fosett-1, &wtmg_cod, &wtmg_desc);
  
    sprintf(cmd, str_6_72, wtmg_cod);
    console_send(dev, cmd);
    sprintf(cmd, str_6_73, wtmg_desc);
    console_send(dev, cmd);
    console_register_callback(dev, KEY_ENTER, console_lara_profile_show_fosett1, NULL);
  }
  else
    console_send(dev, str_6_10);
  
  console_register_callback(dev, KEY_CANCEL, console_lara_profile_show2, NULL);
  return NULL;
}

static CALLBACK void* console_lara_profile_show_holiday(ProtDevice *dev, int press, void *null)
{
  CONSOLE->list_show_cancel = console_lara_profile_show2;
  console_lara_impostazioni_holiday_show(dev, 0, _lara->profilo[CONSOLE->support_idx].fest);

  return NULL;
}

static CALLBACK void* console_lara_profile_show_option_coerc(ProtDevice *dev, int press, void *null)
{
  console_send(dev, "ATS04\r");
  console_send(dev, str_6_11);
  if(_lara->profilo[CONSOLE->support_idx].stato.s.coerc)
    console_send(dev, str_6_12);
  else
    console_send(dev, str_6_13);
  
  console_register_callback(dev, KEY_CANCEL, console_lara_profile_show_option, NULL);
  return NULL;
}

static CALLBACK void* console_lara_profile_show_option_passback(ProtDevice *dev, int press, void *null)
{
  console_send(dev, "ATS04\r");
  console_send(dev, str_6_14);
  if(_lara->profilo[CONSOLE->support_idx].stato.s.apbk)
    console_send(dev, str_6_13);
  else
    console_send(dev, str_6_12);
  
  console_register_callback(dev, KEY_CANCEL, console_lara_profile_show_option, NULL);
  return NULL;
}

static CALLBACK void* console_lara_profile_show_option_type(ProtDevice *dev, int press, void *null)
{
  console_send(dev, "ATS04\r");
  console_send(dev, str_6_78);
  switch(_lara->profilo[CONSOLE->support_idx].stato.s.tipo)
  {
    case PROF_TIPO_BASE: console_send(dev, str_6_79); break;
    case PROF_TIPO_SCORTA: console_send(dev, str_6_80); break;
    case PROF_TIPO_VISIT_O: console_send(dev, str_6_81); break;
    case PROF_TIPO_VISIT_A: console_send(dev, str_6_82); break;
    default: console_send(dev, "ATS06\r"); break;
  }
  
  console_register_callback(dev, KEY_CANCEL, console_lara_profile_show_option, NULL);
  return NULL;
}

static MenuItem console_lara_profile_show_option_menu[] = {
	{&str_6_17, L1 | L2 | L3 | L4 | L5 | L6 | L7 | L8, console_lara_profile_show_option_coerc},
	{&str_6_18, L1 | L2 | L3 | L4 | L5 | L6 | L7 | L8, console_lara_profile_show_option_passback},
	{&str_6_77, L1 | L2 | L3 | L4 | L5 | L6 | L7 | L8, console_lara_profile_show_option_type},
	{NULL, 0, NULL}
};

static CALLBACK void* console_lara_profile_show_option(ProtDevice *dev, int press, void *null)
{
  console_send(dev, "ATS04\r");
  console_send(dev, str_6_19);

  CONSOLE->list_show_cancel = console_lara_profile_show2;
  console_submenu(dev, console_lara_profile_show_option_menu, 0, 3);
  
  return NULL;
}

static CALLBACK void* console_lara_profile_show2(ProtDevice *dev, int press, void *null)
{
  char *prof_cod, *prof_desc, cmd[32];

  console_send(dev, "ATS03\r");
  
  string_profile_name(CONSOLE->support_idx + 1, &prof_cod, &prof_desc);
  
  sprintf(cmd, str_6_69, prof_cod);
  console_send(dev, cmd);
  sprintf(cmd, str_6_70, prof_desc);
  console_send(dev, cmd);
  
  CONSOLE->list_show_cancel = console_lara_profile_show;
  console_submenu(dev, console_lara_profile_menu_1, CONSOLE->support_idx, 3);
  
  return NULL;
}

static CALLBACK void* console_lara_profile_show1(ProtDevice *dev, int press, void *prof_as_voidp)
{
  CONSOLE->support_idx = (int)prof_as_voidp - 1;
  console_lara_profile_show2(dev, 0, NULL);
  return NULL;
}

CALLBACK void* console_lara_profile_show(ProtDevice *dev, int press, void *null)
{
  char *prof_cod, *prof_desc;
  int i;

  console_disable_menu(dev, str_6_20);
  
  if(!console_check_lara(dev)) return NULL;

  console_list_free(CONSOLE->support_list);
  CONSOLE->support_list = NULL;

  for(i=0; i<LARA_N_PROFILI; i++)
  {
    string_profile_name(i+1, &prof_cod, &prof_desc);
    CONSOLE->support_list = console_list_add(CONSOLE->support_list, prof_cod, prof_desc, console_lara_profile_show1, i+1);
  }

  console_list_show(dev, CONSOLE->support_list, 0, 3, 0);
  console_register_callback(dev, KEY_CANCEL, console_show_menu, NULL);

  return NULL;
}

/***********************************************************************
	MODIFICA
***********************************************************************/

static CALLBACK void* console_lara_profile_modify2(ProtDevice *dev, int press, void *null);

static CALLBACK void* console_lara_profile_modify_term_include1(ProtDevice *dev, int press, void *term_as_voidp)
{
  int term = (int)term_as_voidp - 1;
  unsigned char ev[4];
  
  ev[0] = 247;
  ev[1] = 0;
  ev[2] = 44;
  ev[3] = CONSOLE->support_idx+1;
  codec_queue_event(ev);
  
  _lara->profilo[CONSOLE->support_idx].term[term>>3] |= (1<<(term & 0x07));
  _laraf->profilo[CONSOLE->support_idx] = 1;
  lara_save(0);
  
  console_send(dev, "ATS04\r");
  console_send(dev, str_6_21);
  console_send(dev, str_6_22);
            
  CONSOLE->last_menu_item = console_lara_profile_modify_term;
  timeout_on(CONSOLE->timeout, (timeout_func)console_return_to_last_menu_item, dev, 0, TIMEOUT_MSG);
  
  return NULL;
}

static CALLBACK void* console_lara_profile_modify_term_include(ProtDevice *dev, int press, void *null)
{
  char *term_cod, *term_desc;
  int i;

  console_list_free(CONSOLE->support_list);
  CONSOLE->support_list = NULL;

  console_send(dev, "ATS04\r");
  console_send(dev, str_6_23);

  for(i=0; i<LARA_N_TERMINALI; i++)
  {
    if(!(_lara->profilo[CONSOLE->support_idx].term[i>>3] & (1<<(i & 0x07))) && lara_terminal_prog[i])
    {
      string_terminal_name(i, &term_cod, &term_desc);
      CONSOLE->support_list = console_list_add(CONSOLE->support_list, term_cod, term_desc, console_lara_profile_modify_term_include1, i+1);
    }
  }

  CONSOLE->list_show_cancel = console_lara_profile_modify_term;
  console_list_show(dev, CONSOLE->support_list, 0, 2, 0);
  
  return NULL;
}

static CALLBACK void* console_lara_profile_modify_term_exclude1(ProtDevice *dev, int press, void *term_as_voidp)
{
  int term = (int)term_as_voidp - 1;
  unsigned char ev[4];
  
  ev[0] = 247;
  ev[1] = 0;
  ev[2] = 44;
  ev[3] = CONSOLE->support_idx+1;
  codec_queue_event(ev);
  
  _lara->profilo[CONSOLE->support_idx].term[term>>3] &= ~(1<<(term & 0x07));
  _laraf->profilo[CONSOLE->support_idx] = 1;
  lara_save(0);
  
  console_send(dev, "ATS04\r");
  console_send(dev, str_6_21);
  console_send(dev, str_6_25);
            
  CONSOLE->last_menu_item = console_lara_profile_modify_term;
  timeout_on(CONSOLE->timeout, (timeout_func)console_return_to_last_menu_item, dev, 0, TIMEOUT_MSG);
  
  return NULL;
}

static CALLBACK void* console_lara_profile_modify_term_exclude(ProtDevice *dev, int press, void *null)
{
  char *term_cod, *term_desc;
  int i;

  console_list_free(CONSOLE->support_list);
  CONSOLE->support_list = NULL;

  console_send(dev, "ATS04\r");
  console_send(dev, str_6_26);

  for(i=0; i<LARA_N_TERMINALI; i++)
  {
    if(_lara->profilo[CONSOLE->support_idx].term[i>>3] & (1<<(i & 0x07)))
    {
      string_terminal_name(i, &term_cod, &term_desc);
      CONSOLE->support_list = console_list_add(CONSOLE->support_list, term_cod, term_desc, console_lara_profile_modify_term_exclude1, i+1);
    }
  }

  CONSOLE->list_show_cancel = console_lara_profile_modify_term;
  console_list_show(dev, CONSOLE->support_list, 0, 2, 0);
  
  return NULL;
}

static MenuItem console_lara_profile_modify_term_menu[] = {
	{&str_6_27, L1 | L2 | L3 | L4 | L5 | L6 | L7 | L8, console_lara_profile_modify_term_include},
	{&str_6_28, L1 | L2 | L3 | L4 | L5 | L6 | L7 | L8, console_lara_profile_modify_term_exclude},
	{NULL, 0, NULL}
};

static CALLBACK void* console_lara_profile_modify_term(ProtDevice *dev, int press, void *null)
{
  console_send(dev, "ATS04\r");
  console_send(dev, str_6_29);

  CONSOLE->list_show_cancel = console_lara_profile_modify2;
  console_submenu(dev, console_lara_profile_modify_term_menu, 0, 3);

  return NULL;
}

static CALLBACK void* console_lara_profile_modify_fosett_join1(ProtDevice *dev, int press, void *wtmg_as_voidp)
{
  unsigned char ev[8];
  
  ev[0] = 247;
  ev[1] = 0;
  ev[2] = 43;
  ev[3] = CONSOLE->support_idx+1;
  ev[4] = (int)wtmg_as_voidp;
  codec_queue_event(ev);
  
  _lara->profilo[CONSOLE->support_idx].fosett = (int)wtmg_as_voidp;
  _laraf->profilo[CONSOLE->support_idx] = 1;
  lara_save(0);
  
  console_send(dev, "ATS04\r");
  console_send(dev, str_6_30);
  console_send(dev, str_6_31);
            
  CONSOLE->last_menu_item = console_lara_profile_modify_fosett;
  timeout_on(CONSOLE->timeout, (timeout_func)console_return_to_last_menu_item, dev, 0, TIMEOUT_MSG);
  
  return NULL;
}

static CALLBACK void* console_lara_profile_modify_fosett_join(ProtDevice *dev, int press, void *null)
{
  char *wtmg_cod, *wtmg_desc;
  int i;

  console_send(dev, "ATS04\r");
  console_send(dev, str_6_32);

  console_list_free(CONSOLE->support_list);
  CONSOLE->support_list = NULL;

  for(i=1; i<=LARA_N_FASCE; i++)
  {
    if(_lara->profilo[CONSOLE->support_idx].fosett != i)
    {
      string_weektiming_name(i-1, &wtmg_cod, &wtmg_desc);
      CONSOLE->support_list = console_list_add(CONSOLE->support_list, wtmg_cod, wtmg_desc, console_lara_profile_modify_fosett_join1, i);
    }
  }

  CONSOLE->list_show_cancel = console_lara_profile_modify_fosett;
  console_list_show(dev, CONSOLE->support_list, 0, 2, 0);
  
  return NULL;
}

static CALLBACK void* console_lara_profile_modify_fosett_delete1(ProtDevice *dev, int press, void *null)
{
  unsigned char ev[8];
  
  ev[0] = 247;
  ev[1] = 0;
  ev[2] = 43;
  ev[3] = CONSOLE->support_idx+1;
  ev[4] = 0;
  codec_queue_event(ev);
  
  _lara->profilo[CONSOLE->support_idx].fosett = 0;
  _laraf->profilo[CONSOLE->support_idx] = 1;
  lara_save(0);
  
  console_send(dev, "ATS04\r");
  console_send(dev, str_6_33);
  console_send(dev, str_6_34);
            
  CONSOLE->last_menu_item = console_lara_profile_modify_fosett;
  timeout_on(CONSOLE->timeout, (timeout_func)console_return_to_last_menu_item, dev, 0, TIMEOUT_MSG);
  
  return NULL;
}

static CALLBACK void* console_lara_profile_modify_fosett_delete(ProtDevice *dev, int press, void *null)
{
  char *wtmg_cod, *wtmg_desc, cmd[32];

  console_send(dev, "ATS04\r");
  console_send(dev, str_6_35);
  
  if(!_lara->profilo[CONSOLE->support_idx].fosett)
  {
    console_send(dev, str_6_36);
  }
  else
  {
    string_weektiming_name(_lara->profilo[CONSOLE->support_idx].fosett-1, &wtmg_cod, &wtmg_desc);
    sprintf(cmd, str_6_76, wtmg_cod);
    console_send(dev, cmd);
    sprintf(cmd, str_6_72, wtmg_desc);
    console_send(dev, cmd);
    console_register_callback(dev, KEY_ENTER, console_lara_profile_modify_fosett_delete1, NULL);
  }
  
  console_register_callback(dev, KEY_CANCEL, console_lara_profile_modify_fosett, NULL);
  return NULL;
}

static CALLBACK void* console_lara_profile_modify_fosett_show(ProtDevice *dev, int press, void *null)
{
  CONSOLE->list_show_cancel = console_lara_profile_modify_fosett;
//  console_lara_profile_fosett_show(dev, 0, NULL);
  console_lara_impostazioni_fosett_show(dev, 0, (void*)((int)(_lara->profilo[CONSOLE->support_idx].fosett)));
    
  return NULL;
}

static MenuItem console_lara_profile_modify_fosett_menu[] = {
	{&str_6_37, L1 | L2 | L3 | L4 | L5 | L6 | L7 | L8, console_lara_profile_modify_fosett_join},
	{&str_6_38, L1 | L2 | L3 | L4 | L5 | L6 | L7 | L8, console_lara_profile_modify_fosett_delete},
	{&str_6_39, L1 | L2 | L3 | L4 | L5 | L6 | L7 | L8, console_lara_profile_modify_fosett_show},
	{NULL, 0, NULL}
};

static CALLBACK void* console_lara_profile_modify_fosett(ProtDevice *dev, int press, void *null)
{
  console_send(dev, "ATS04\r");
  console_send(dev, str_6_40);

  CONSOLE->list_show_cancel = console_lara_profile_modify2;
  console_submenu(dev, console_lara_profile_modify_fosett_menu, 0, 3);
  
  return NULL;
}

static CALLBACK void* console_lara_profile_modify_holiday_include1(ProtDevice *dev, int press, void *hday_as_voidp)
{
  int hday = (int)hday_as_voidp - 1;
  unsigned char ev[4];
  
  ev[0] = 247;
  ev[1] = 0;
  ev[2] = 45;
  ev[3] = CONSOLE->support_idx+1;
  codec_queue_event(ev);
  
  _lara->profilo[CONSOLE->support_idx].fest[hday>>3] |= (1<<(hday & 0x07));
  _laraf->profilo[CONSOLE->support_idx] = 1;
  lara_save(0);
  
  console_send(dev, str_6_41);
  console_send(dev, str_6_42);
  console_send(dev, str_6_31);
            
  CONSOLE->last_menu_item = console_lara_profile_modify_holiday;
  timeout_on(CONSOLE->timeout, (timeout_func)console_return_to_last_menu_item, dev, 0, TIMEOUT_MSG);
  
  return NULL;
}

static CALLBACK void* console_lara_profile_modify_holiday_include(ProtDevice *dev, int press, void *null)
{
  char *hday_cod, *hday_desc;
  int i;

  console_list_free(CONSOLE->support_list);
  CONSOLE->support_list = NULL;

  console_send(dev, "ATS04\r");
  console_send(dev, str_6_32);

  for(i=0; i<LARA_N_FESTIVI; i++)
  {
    if(!(_lara->profilo[CONSOLE->support_idx].fest[i>>3] & (1<<(i & 0x07))))
    {
      string_holiday_name(i, &hday_cod, &hday_desc);
      CONSOLE->support_list = console_list_add(CONSOLE->support_list, hday_cod, hday_desc, console_lara_profile_modify_holiday_include1, i+1);
    }
  }

  CONSOLE->list_show_cancel = console_lara_profile_modify_holiday;
  console_list_show(dev, CONSOLE->support_list, 0, 2, 0);
  
  return NULL;
}

static CALLBACK void* console_lara_profile_modify_holiday_exclude1(ProtDevice *dev, int press, void *hday_as_voidp)
{
  int hday = (int)hday_as_voidp - 1;
  unsigned char ev[4];
  
  ev[0] = 247;
  ev[1] = 0;
  ev[2] = 45;
  ev[3] = CONSOLE->support_idx+1;
  codec_queue_event(ev);
  
  _lara->profilo[CONSOLE->support_idx].fest[hday>>3] &= ~(1<<(hday & 0x07));
  _laraf->profilo[CONSOLE->support_idx] = 1;
  lara_save(0);
  
  console_send(dev, "ATS04\r");
  console_send(dev, str_6_42);
  console_send(dev, str_6_34);
            
  CONSOLE->last_menu_item = console_lara_profile_modify_holiday;
  timeout_on(CONSOLE->timeout, (timeout_func)console_return_to_last_menu_item, dev, 0, TIMEOUT_MSG);
  
  return NULL;
}

static CALLBACK void* console_lara_profile_modify_holiday_exclude(ProtDevice *dev, int press, void *null)
{
  char *hday_cod, *hday_desc;
  int i;

  console_list_free(CONSOLE->support_list);
  CONSOLE->support_list = NULL;

  console_send(dev, "ATS04\r");
  console_send(dev, str_6_35);

  for(i=0; i<LARA_N_FESTIVI; i++)
  {
    if(_lara->profilo[CONSOLE->support_idx].fest[i>>3] & (1<<(i & 0x07)))
    {
      string_holiday_name(i, &hday_cod, &hday_desc);
      CONSOLE->support_list = console_list_add(CONSOLE->support_list, hday_cod, hday_desc, console_lara_profile_modify_holiday_exclude1, i+1);
    }
  }

  CONSOLE->list_show_cancel = console_lara_profile_modify_holiday;
  console_list_show(dev, CONSOLE->support_list, 0, 2, 0);
  
  return NULL;
}

static CALLBACK void* console_lara_profile_modify_holiday_show(ProtDevice *dev, int press, void *null)
{
  CONSOLE->list_show_cancel = console_lara_profile_modify_holiday;
  console_lara_impostazioni_holiday_show(dev, 0, _lara->profilo[CONSOLE->support_idx].fest);

  return NULL;
}

static MenuItem console_lara_profile_modify_holiday_menu[] = {
	{&str_6_37, L1 | L2 | L3 | L4 | L5 | L6 | L7 | L8, console_lara_profile_modify_holiday_include},
	{&str_6_38, L1 | L2 | L3 | L4 | L5 | L6 | L7 | L8, console_lara_profile_modify_holiday_exclude},
	{&str_6_39, L1 | L2 | L3 | L4 | L5 | L6 | L7 | L8, console_lara_profile_modify_holiday_show},
	{NULL, 0, NULL}
};

static CALLBACK void* console_lara_profile_modify_holiday(ProtDevice *dev, int press, void *null)
{
  console_send(dev, "ATS04\r");
  console_send(dev, str_6_51);

  CONSOLE->list_show_cancel = console_lara_profile_modify2;
  console_submenu(dev, console_lara_profile_modify_holiday_menu, 0, 3);
  return NULL;
}

static CALLBACK void* console_lara_profile_modify_option_coerc1(ProtDevice *dev, int press, void *action_as_voidp)
{
  unsigned char ev[8];
  
  console_send(dev, "ATS06\r");
  
  _lara->profilo[CONSOLE->support_idx].stato.s.coerc = (int)action_as_voidp;
  _laraf->profilo[CONSOLE->support_idx] = 1;
  lara_save(0);
  
  ev[0] = 247;
  ev[1] = 0;
  ev[2] = 46;
  ev[3] = CONSOLE->support_idx+1;
  ev[4] = _lara->profilo[CONSOLE->support_idx].stato.b;
  codec_queue_event(ev);
  
  if(action_as_voidp)
    console_send(dev, str_6_12);
  else
    console_send(dev, str_6_13);
  
  CONSOLE->last_menu_item = console_lara_profile_modify_option;
  timeout_on(CONSOLE->timeout, (timeout_func)console_return_to_last_menu_item, dev, 0, TIMEOUT_MSG);
  
  return NULL;
}

static CALLBACK void* console_lara_profile_modify_option_coerc(ProtDevice *dev, int press, void *null)
{
  console_send(dev, "ATS04\r");
  console_send(dev, str_6_54);
  
  if(_lara->profilo[CONSOLE->support_idx].stato.s.coerc)
  {
    console_send(dev, str_6_12);
    console_send(dev, str_6_56);
    console_register_callback(dev, KEY_ENTER, console_lara_profile_modify_option_coerc1, (void*)0);
  }
  else
  {
    console_send(dev, str_6_13);
    console_send(dev, str_6_58);
    console_register_callback(dev, KEY_ENTER, console_lara_profile_modify_option_coerc1, (void*)1);
  }

  console_register_callback(dev, KEY_CANCEL, console_lara_profile_modify_option, NULL);
  return NULL;
}

static CALLBACK void* console_lara_profile_modify_option_passback1(ProtDevice *dev, int press, void *action_as_voidp)
{
  unsigned char ev[8];
  
  console_send(dev, "ATS06\r");
  
  _lara->profilo[CONSOLE->support_idx].stato.s.apbk = (int)action_as_voidp;
  _laraf->profilo[CONSOLE->support_idx] = 1;
  lara_save(0);
  
  ev[0] = 247;
  ev[1] = 0;
  ev[2] = 46;
  ev[3] = CONSOLE->support_idx+1;
  ev[4] = _lara->profilo[CONSOLE->support_idx].stato.b;
  codec_queue_event(ev);
  
  if(action_as_voidp)
    console_send(dev, str_6_13);
  else
    console_send(dev, str_6_12);
  
  CONSOLE->last_menu_item = console_lara_profile_modify_option;
  timeout_on(CONSOLE->timeout, (timeout_func)console_return_to_last_menu_item, dev, 0, TIMEOUT_MSG);
  
  return NULL;
}

static CALLBACK void* console_lara_profile_modify_option_passback(ProtDevice *dev, int press, void *null)
{
  console_send(dev, "ATS04\r");
  console_send(dev, str_6_61);
  
  if(!_lara->profilo[CONSOLE->support_idx].stato.s.apbk)
  {
    console_send(dev, str_6_12);
    console_send(dev, str_6_56);
    console_register_callback(dev, KEY_ENTER, console_lara_profile_modify_option_passback1, (void*)1);
  }
  else
  {
    console_send(dev, str_6_13);
    console_send(dev, str_6_58);
    console_register_callback(dev, KEY_ENTER, console_lara_profile_modify_option_passback1, (void*)0);
  }

  console_register_callback(dev, KEY_CANCEL, console_lara_profile_modify_option, NULL);
  return NULL;
}

static CALLBACK void* console_lara_profile_modify_option_type2(ProtDevice *dev, int press, void *type_as_voidp)
{
  unsigned char ev[8];
  
  _lara->profilo[CONSOLE->support_idx].stato.s.tipo = (int)type_as_voidp;
  _laraf->profilo[CONSOLE->support_idx] = 1;
  lara_save(0);
  
  ev[0] = 247;
  ev[1] = 0;
  ev[2] = 46;
  ev[3] = CONSOLE->support_idx+1;
  ev[4] = _lara->profilo[CONSOLE->support_idx].stato.b;
  codec_queue_event(ev);
  
  console_lara_profile_modify_option((void*)dev, 0, NULL);
  
  return NULL;
}

static CALLBACK void* console_lara_profile_modify_option_type1(ProtDevice *dev, int press, void *type_as_voidp)
{
  int type = (int)type_as_voidp;
  
  console_send(dev, "ATS04\r");
  console_send(dev, str_6_78);
  
  switch(type)
  {
    case PROF_TIPO_BASE: console_send(dev, str_6_79); break;
    case PROF_TIPO_SCORTA: console_send(dev, str_6_80); break;
    case PROF_TIPO_VISIT_O: console_send(dev, str_6_81); break;
    case PROF_TIPO_VISIT_A: console_send(dev, str_6_82); break;
    default: console_send(dev, "ATS06\r"); break;
  }
  
  if(type > 0)
    console_register_callback(dev, KEY_UP, console_lara_profile_modify_option_type1, (void*)(type-1));
  if(type < PROF_TIPO_LAST)
    console_register_callback(dev, KEY_DOWN, console_lara_profile_modify_option_type1, (void*)(type+1));
  console_register_callback(dev, KEY_ENTER, console_lara_profile_modify_option_type2, (void*)type);
  console_register_callback(dev, KEY_CANCEL, console_lara_profile_modify_option, NULL);
  return NULL;
}

static CALLBACK void* console_lara_profile_modify_option_type(ProtDevice *dev, int press, void *null)
{
  return console_lara_profile_modify_option_type1(dev, press, (void*)((int)_lara->profilo[CONSOLE->support_idx].stato.s.tipo));
}

static MenuItem console_lara_profile_modify_option_menu[] = {
	{&str_6_17, L1 | L2 | L3 | L4 | L5 | L6 | L7 | L8, console_lara_profile_modify_option_coerc},
	{&str_6_18, L1 | L2 | L3 | L4 | L5 | L6 | L7 | L8, console_lara_profile_modify_option_passback},
	{&str_6_77, L1 | L2 | L3 | L4 | L5 | L6 | L7 | L8, console_lara_profile_modify_option_type},
	{NULL, 0, NULL}
};

static CALLBACK void* console_lara_profile_modify_option(ProtDevice *dev, int press, void *null)
{
  console_send(dev, "ATS04\r");
  console_send(dev, str_6_19);

  CONSOLE->list_show_cancel = console_lara_profile_modify2;
  console_submenu(dev, console_lara_profile_modify_option_menu, 0, 3);
  return NULL;
}

static CALLBACK void* console_lara_profile_modify2(ProtDevice *dev, int press, void *null)
{
  char *prof_cod, *prof_desc, cmd[32];

  console_send(dev, "ATS03\r");
  
  string_profile_name(CONSOLE->support_idx + 1, &prof_cod, &prof_desc);
  
  sprintf(cmd, str_6_69, prof_cod);
  console_send(dev, cmd);
  sprintf(cmd, str_6_70, prof_desc);
  console_send(dev, cmd);
  
  CONSOLE->list_show_cancel = console_lara_profile_modify;
  console_submenu(dev, console_lara_profile_menu_2, CONSOLE->support_idx, 3);
  
  return NULL;
}

static CALLBACK void* console_lara_profile_modify1(ProtDevice *dev, int press, void *prof_as_voidp)
{
  CONSOLE->support_idx = (int)prof_as_voidp - 1;
  console_lara_profile_modify2(dev, 0, NULL);
  return NULL;
}

CALLBACK void* console_lara_profile_modify(ProtDevice *dev, int press, void *null)
{
  char *prof_cod, *prof_desc;
  int i;

  console_disable_menu(dev, str_6_20);
  
  if(!console_check_lara(dev)) return NULL;

  console_list_free(CONSOLE->support_list);
  CONSOLE->support_list = NULL;

  for(i=0; i<LARA_N_PROFILI; i++)
  {
    string_profile_name(i+1, &prof_cod, &prof_desc);
    CONSOLE->support_list = console_list_add(CONSOLE->support_list, prof_cod, prof_desc, console_lara_profile_modify1, i+1);
  }

  console_list_show(dev, CONSOLE->support_list, 0, 3, 0);
  console_register_callback(dev, KEY_CANCEL, console_show_menu, NULL);

  return NULL;
}
