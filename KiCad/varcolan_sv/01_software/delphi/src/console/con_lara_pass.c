#include "console.h"
#include "con_lara_pass.h"
#include "../lara.h"
#include "../strings.h"
#include "../codec.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "con_lara_impostazioni.h"

static CALLBACK void* console_lara_pass_show_badgecnt(ProtDevice *dev, int press, void *id_as_voidp);
static CALLBACK void* console_lara_pass_show_badgepin(ProtDevice *dev, int press, void *id_as_voidp);
static CALLBACK void* console_lara_pass_show_profile(ProtDevice *dev, int press, void *id_as_voidp);
static CALLBACK void* console_lara_pass_show_area(ProtDevice *dev, int press, void *id_as_voidp);
static CALLBACK void* console_lara_pass_show_abil(ProtDevice *dev, int press, void *id_as_voidp);

static MenuItem console_lara_pass_menu_1[] = {
	{&str_5_0, L1 | L2 | L3 | L4 | L5 | L6 | L7 | L8, console_lara_pass_show_badgecnt},
	{&str_5_1, L1 | L2 | L3 | L4 | L5 | L6 | L7 | L8, console_lara_pass_show_profile},
	{&str_5_2, L1 | L2 | L3 | L4 | L5 | L6 | L7 | L8, console_lara_pass_show_area},
	{&str_5_3, L1 | L2 | L3 | L4 | L5 | L6 | L7 | L8, console_lara_pass_show_abil},
	{NULL, 0, NULL}
};

static MenuItem console_lara_pass_menu_2[] = {
	{&str_5_4, L1 | L2 | L3 | L4 | L5 | L6 | L7 | L8, console_lara_pass_show_badgepin},
	{&str_5_1, L1 | L2 | L3 | L4 | L5 | L6 | L7 | L8, console_lara_pass_show_profile},
	{&str_5_2, L1 | L2 | L3 | L4 | L5 | L6 | L7 | L8, console_lara_pass_show_area},
	{&str_5_3, L1 | L2 | L3 | L4 | L5 | L6 | L7 | L8, console_lara_pass_show_abil},
	{NULL, 0, NULL}
};

static CALLBACK void* console_lara_pass_modify_pin(ProtDevice *dev, int press, void *null);
static CALLBACK void* console_lara_pass_modify_count(ProtDevice *dev, int press, void *null);
static CALLBACK void* console_lara_pass_modify_profile(ProtDevice *dev, int press, void *null);
static CALLBACK void* console_lara_pass_modify_area(ProtDevice *dev, int press, void *null);
static CALLBACK void* console_lara_pass_modify_abil(ProtDevice *dev, int press, void *null);
static CALLBACK void* console_lara_pass_modify_supervisor(ProtDevice *dev, int press, void *null);

static MenuItem console_lara_pass_menu_3[] = {
	{&str_5_0, L1 | L2 | L3 | L4 | L5 | L6 | L7 | L8, console_lara_pass_modify_count},
	{&str_5_1, L1 | L2 | L3 | L4 | L5 | L6 | L7 | L8, console_lara_pass_modify_profile},
	{&str_5_2, L1 | L2 | L3 | L4 | L5 | L6 | L7 | L8, console_lara_pass_modify_area},
	{&str_5_3, L1 | L2 | L3 | L4 | L5 | L6 | L7 | L8, console_lara_pass_modify_abil},
	{&str_5_12, L1 | L2 | L3 | L4 | L5 | L6 | L7 | L8, console_lara_pass_modify_supervisor},
	{NULL, 0, NULL}
};

static MenuItem console_lara_pass_menu_4[] = {
	{&str_5_4, L1 | L2 | L3 | L4 | L5 | L6 | L7 | L8, console_lara_pass_modify_pin},
	{&str_5_1, L1 | L2 | L3 | L4 | L5 | L6 | L7 | L8, console_lara_pass_modify_profile},
	{&str_5_2, L1 | L2 | L3 | L4 | L5 | L6 | L7 | L8, console_lara_pass_modify_area},
	{&str_5_3, L1 | L2 | L3 | L4 | L5 | L6 | L7 | L8, console_lara_pass_modify_abil},
	{&str_5_12, L1 | L2 | L3 | L4 | L5 | L6 | L7 | L8, console_lara_pass_modify_supervisor},
	{NULL, 0, NULL}
};

static CALLBACK void* console_lara_pass_prof_show_term(ProtDevice *dev, int press, void *id_as_voidp);
static CALLBACK void* console_lara_pass_prof_show_fosett(ProtDevice *dev, int press, void *null);
static CALLBACK void* console_lara_pass_prof_show_holiday(ProtDevice *dev, int press, void *id_as_voidp);
static CALLBACK void* console_lara_pass_prof_show_option(ProtDevice *dev, int press, void *id_as_voidp);

static MenuItem console_lara_pass_prof_show_menu[] = {
	{&str_5_18, L1 | L2 | L3 | L4 | L5 | L6 | L7 | L8, console_lara_pass_prof_show_term},
	{&str_5_19, L1 | L2 | L3 | L4 | L5 | L6 | L7 | L8, console_lara_pass_prof_show_fosett},
	{&str_5_20, L1 | L2 | L3 | L4 | L5 | L6 | L7 | L8, console_lara_pass_prof_show_holiday},
	{&str_5_21, L1 | L2 | L3 | L4 | L5 | L6 | L7 | L8, console_lara_pass_prof_show_option},
	{NULL, 0, NULL}
};

static CALLBACK void* console_lara_pass_prof_modify_term(ProtDevice *dev, int press, void *id_as_voidp);
static CALLBACK void* console_lara_pass_prof_modify_fosett(ProtDevice *dev, int press, void *null);
static CALLBACK void* console_lara_pass_prof_modify_holiday(ProtDevice *dev, int press, void *null);
static CALLBACK void* console_lara_pass_prof_modify_option(ProtDevice *dev, int press, void *null);

static MenuItem console_lara_pass_prof_modify_menu[] = {
	{&str_5_18, L1 | L2 | L3 | L4 | L5 | L6 | L7 | L8, console_lara_pass_prof_modify_term},
	{&str_5_19, L1 | L2 | L3 | L4 | L5 | L6 | L7 | L8, console_lara_pass_prof_modify_fosett},
	{&str_5_20, L1 | L2 | L3 | L4 | L5 | L6 | L7 | L8, console_lara_pass_prof_modify_holiday},
	{&str_5_21, L1 | L2 | L3 | L4 | L5 | L6 | L7 | L8, console_lara_pass_prof_modify_option},
	{NULL, 0, NULL}
};

/***********************************************************************
	LARA PASS menu functions
***********************************************************************/

/***********************************************************************
	VISUALIZZA
***********************************************************************/

static CALLBACK void* console_lara_pass_show2(ProtDevice *dev, int press, void *id_as_voidp);

static CALLBACK void* console_lara_pass_show_badgecnt(ProtDevice *dev, int press, void *id_as_voidp)
{
  char cmd[24];
  
  console_send(dev, str_5_26);
  
  if(_lara->tessera[(int)id_as_voidp].stato.s.badge & BADGE_PROGR)
    console_send(dev, str_5_27);
  else
    console_send(dev, str_5_28);

  console_send(dev, str_5_29);  
  sprintf(cmd, str_5_149, _lara->tessera[(int)id_as_voidp].pincnt);
  console_send(dev, cmd);
  
  console_register_callback(dev, KEY_CANCEL, console_lara_pass_show2, id_as_voidp);

  return NULL;
}

static CALLBACK void* console_lara_pass_show_badgepin(ProtDevice *dev, int press, void *id_as_voidp)
{
  console_send(dev, "ATS04\r");
  
  if(_lara->tessera[(int)id_as_voidp].stato.s.badge & BADGE_PROGR)
    console_send(dev, str_5_27);
  else
    console_send(dev, str_5_28);

  if(_lara->tessera[(int)id_as_voidp].stato.s.badge & SEGR_PROGR)
    console_send(dev, str_5_32);
  else
    console_send(dev, str_5_33);
  
  console_register_callback(dev, KEY_CANCEL, console_lara_pass_show2, id_as_voidp);

  return NULL;
}

static CALLBACK void* console_lara_pass_prof_show_term(ProtDevice *dev, int press, void *id_as_voidp)
{
  char *term_cod, *term_desc;
  int i;

  console_list_free(CONSOLE->support_list);
  CONSOLE->support_list = NULL;

  console_send(dev, "ATS04\r");
  console_send(dev, str_5_34);

  for(i=0; i<LARA_N_TERMINALI; i++)
  {
    if(_lara->tessera[(int)id_as_voidp].p.s.term[i>>3] & (1<<(i & 0x07)))
    {
      string_terminal_name(i, &term_cod, &term_desc);
      CONSOLE->support_list = console_list_add(CONSOLE->support_list, term_cod, term_desc, NULL, 0);
    }
  }

  CONSOLE->list_show_cancel = console_lara_pass_show_profile;
  console_list_show(dev, CONSOLE->support_list, 0, 2, 0);
  
  return NULL;
}

static CALLBACK void* console_lara_pass_prof_show_fosett(ProtDevice *dev, int press, void *null)
{    
  CONSOLE->list_show_cancel = console_lara_pass_show_profile;
  console_lara_impostazioni_fosett_show(dev, 0, (void*)((int)(_lara->tessera[CONSOLE->support_idx].p.s.fosett)));
    
  return NULL;
}

static CALLBACK void* console_lara_pass_prof_show_holiday(ProtDevice *dev, int press, void *null)
{
  CONSOLE->list_show_cancel = console_lara_pass_show_profile;
  console_lara_impostazioni_holiday_show(dev, 0, _lara->tessera[CONSOLE->support_idx].p.s.fest);
  
  return NULL;
}

static CALLBACK void* console_lara_pass_prof_show_option_coerc(ProtDevice *dev, int press, void *null)
{
  console_send(dev, "ATS05\r");
  console_send(dev, str_5_35);
  if(_lara->tessera[CONSOLE->support_idx].stato.s.coerc)
    console_send(dev, str_5_36);
  else
    console_send(dev, str_5_37);

  console_register_callback(dev, KEY_CANCEL, console_lara_pass_prof_show_option, NULL);
  return NULL;
}

static CALLBACK void* console_lara_pass_prof_show_option_passback(ProtDevice *dev, int press, void *null)
{
  console_send(dev, "ATS05\r");
  console_send(dev, str_5_38);
  if(_lara->tessera[CONSOLE->support_idx].stato.s.apbk)
    console_send(dev, str_5_37);
  else
    console_send(dev, str_5_36);

  console_register_callback(dev, KEY_CANCEL, console_lara_pass_prof_show_option, NULL);
  return NULL;
}

static CALLBACK void* console_lara_pass_prof_show_option(ProtDevice *dev, int press, void *null)
{
  console_send(dev, "ATS04\r");
  console_send(dev, str_5_41);
  
  console_list_free(CONSOLE->support_list);
  CONSOLE->support_list = NULL;

  CONSOLE->support_list = console_list_add(CONSOLE->support_list, str_5_42, NULL, console_lara_pass_prof_show_option_coerc, 0);
  CONSOLE->support_list = console_list_add(CONSOLE->support_list, str_5_43, NULL, console_lara_pass_prof_show_option_passback, 0);

  CONSOLE->list_show_cancel = console_lara_pass_show_profile;
  console_list_show(dev, CONSOLE->support_list, 0, 3, 0);

  return NULL;
}

static CALLBACK void* console_lara_pass_show_profile2(ProtDevice *dev, int press, void *null)
{
  char *prof_cod, *prof_desc;
  int i;
  
  console_list_free(CONSOLE->support_list);
  CONSOLE->support_list = NULL;

  console_send(dev, "ATS03\r");
  
  for(i=1; i<LARA_MULTIPROFILI; i++)
  {
    if(!_lara->tessera[CONSOLE->support_idx].p.profilo[i]) break;
    string_profile_name(_lara->tessera[CONSOLE->support_idx].p.profilo[i], &prof_cod, &prof_desc);
    CONSOLE->support_list = console_list_add(CONSOLE->support_list, prof_cod, prof_desc, NULL, 0);
  }
  CONSOLE->list_show_cancel = console_lara_pass_show_profile;
  console_list_show(dev, CONSOLE->support_list, 0, 3, 0);
  return NULL;
}

static CALLBACK void* console_lara_pass_show_profile(ProtDevice *dev, int press, void *null)
{
  char *prof_cod, *prof_desc, cmd[32];
  
  console_send(dev, "ATS03\r");

  if(_lara->tessera[CONSOLE->support_idx].p.s.profilo)
  {
    string_profile_name(_lara->tessera[CONSOLE->support_idx].p.s.profilo, &prof_cod, &prof_desc);
    sprintf(cmd, str_5_150, prof_cod);
    console_send(dev, cmd);
    sprintf(cmd, str_5_151, prof_desc);
    console_send(dev, cmd);
    
    console_register_callback(dev, KEY_ENTER, console_lara_pass_show_profile2, NULL);
    console_register_callback(dev, KEY_CANCEL, console_lara_pass_show2, (void*)(CONSOLE->support_idx));
  }
  else
  {
    CONSOLE->list_show_cancel = console_lara_pass_show2;
    console_submenu(dev, console_lara_pass_prof_show_menu, CONSOLE->support_idx, 4);
  }
  
  return NULL;
}

static CALLBACK void* console_lara_pass_show_area(ProtDevice *dev, int press, void *id_as_voidp)
{
  char *area_cod, *area_desc, cmd[32];
  
  console_send(dev, "ATS04\r");
  
  string_area_name(_lara->tessera[(int)id_as_voidp].area, &area_cod, &area_desc);
  sprintf(cmd, str_5_152, area_cod);
  console_send(dev, cmd);
  sprintf(cmd, str_5_153, area_desc);
  console_send(dev, cmd);
  
  console_register_callback(dev, KEY_CANCEL, console_lara_pass_show2, id_as_voidp);

  return NULL;
}

static CALLBACK void* console_lara_pass_show_abil(ProtDevice *dev, int press, void *id_as_voidp)
{
  console_send(dev, "ATS04\r");
  
  console_send(dev, str_5_44);
  if(_lara->tessera[(int)id_as_voidp].stato.s.abil == BADGE_ABIL)
    console_send(dev, str_5_45);
  else
    console_send(dev, str_5_46);
  
  console_register_callback(dev, KEY_CANCEL, console_lara_pass_show2, id_as_voidp);

  return NULL;
}

static CALLBACK void* console_lara_pass_show2(ProtDevice *dev, int press, void *null)
{
  char cmd[24];
  
  console_send(dev, "ATS03\r");

  if((CONSOLE->support_idx >= lara_NumBadge) ||
     (_lara->tessera[CONSOLE->support_idx].stato.s.abil == BADGE_VUOTO) ||
     (_lara->tessera[CONSOLE->support_idx].stato.s.abil == BADGE_CANC))
  {
    console_send(dev, str_5_47);
    CONSOLE->last_menu_item = console_lara_pass_show;
    timeout_on(CONSOLE->timeout, (timeout_func)console_return_to_last_menu_item, dev, 0, TIMEOUT_MSG);
    return NULL;
  }
  
  sprintf(cmd, str_5_154, CONSOLE->support_idx);
  console_send(dev, cmd);
  
  CONSOLE->list_show_cancel = console_lara_pass_show;
  if(_lara->param & LARA_PARAM_COUNTER)
    console_submenu(dev, console_lara_pass_menu_1, CONSOLE->support_idx, 4);
  else
    console_submenu(dev, console_lara_pass_menu_2, CONSOLE->support_idx, 4);
  
  return NULL;
}

static CALLBACK void* console_lara_pass_show1(ProtDevice *dev, int id_as_int, void *null)
{
  console_send(dev, "ATL000\r");
  console_send(dev, "ATF0\r");
    
  sscanf((char*)id_as_int, "%d", &(CONSOLE->support_idx));
  console_lara_pass_show2(dev, 0, NULL);
  
  return NULL;
}

CALLBACK void* console_lara_pass_show(ProtDevice *dev, int press, void *null)
{
  console_disable_menu(dev, str_5_48);
  
  if(!console_check_lara(dev)) return NULL;
  
  console_send(dev, str_5_49);
  console_send(dev, "ATL111\r");
  console_send(dev, str_5_155);
  console_send(dev, "ATF1\r");

  console_register_callback(dev, KEY_STRING, console_lara_pass_show1, NULL);
  console_register_callback(dev, KEY_CANCEL, console_show_menu, NULL);

  return NULL;
}

/***********************************************************************
	MODIFICA
***********************************************************************/

static CALLBACK void* console_lara_pass_modify2(ProtDevice *dev, int press, void *null);
static CALLBACK void* console_lara_pass_modify_profile1(ProtDevice *dev, int press, void *prof_as_voidp);
static CALLBACK void* console_lara_pass_modify_profile_pri(ProtDevice *dev, int press, void *null);
static CALLBACK void* console_lara_pass_modify_profile_sec_add(ProtDevice *dev, int press, void *null);
static CALLBACK void* console_lara_pass_modify_profile_sec_del(ProtDevice *dev, int press, void *null);

static CALLBACK void* console_lara_pass_modify_pin2(ProtDevice *dev, int pin_as_int, void *null)
{
  int pin;
  
  console_send(dev, "ATS06\r");
  console_send(dev, "ATL000\r");
  console_send(dev, "ATF0\r");

  sscanf((char*)pin_as_int, "%d", &pin);

  if((pin > 65529) || strcmp(CONSOLE->temp_string, (char*)pin_as_int))
  {
    console_send(dev, str_5_50);
  }
  else
  {
    console_send(dev, str_5_51);
    lara_set_secret(CONSOLE->support_idx, pin, 0);
  }
  
  CONSOLE->last_menu_item = console_lara_pass_modify2;
  timeout_on(CONSOLE->timeout, (timeout_func)console_return_to_last_menu_item, dev, 0, TIMEOUT_MSG);

  return NULL;
}

static CALLBACK void* console_lara_pass_modify_pin1(ProtDevice *dev, int pin_as_int, void *null)
{
  free(CONSOLE->temp_string);
  CONSOLE->temp_string = strdup((char*)pin_as_int);
  
  console_send(dev, "ATF0\r");
  console_send(dev, "ATS08\r");
  console_send(dev, str_5_52);
  console_send(dev, str_5_156);
  console_send(dev, "ATF1\r");

  console_register_callback(dev, KEY_STRING, console_lara_pass_modify_pin2, NULL);
  console_register_callback(dev, KEY_CANCEL, console_lara_pass_modify2, NULL);
  return NULL;
}

static CALLBACK void* console_lara_pass_modify_pin0(ProtDevice *dev, int pin_as_int, void *null)
{
  int pin;
  
  console_send(dev, "ATF0\r");
  console_send(dev, "ATS04\r");
  
  sscanf((char*)pin_as_int, "%d", &pin);
  if(_lara->tessera[CONSOLE->support_idx].pincnt != pin)
  {
    console_send(dev, str_5_53);
    
    CONSOLE->last_menu_item = console_lara_pass_modify2;
    timeout_on(CONSOLE->timeout, (timeout_func)console_return_to_last_menu_item, dev, 0, TIMEOUT_MSG);
    return NULL;
  }
  
  console_send(dev, str_5_54);
  console_send(dev, str_5_55);
  console_send(dev, "ATL121\r");
  console_send(dev, str_5_155);
  console_send(dev, "ATF1\r");

  console_register_callback(dev, KEY_STRING, console_lara_pass_modify_pin1, NULL);
  console_register_callback(dev, KEY_CANCEL, console_lara_pass_modify2, NULL);
  return NULL;
}

static CALLBACK void* console_lara_pass_modify_pin(ProtDevice *dev, int press, void *null)
{
  console_send(dev, "ATS04\r");

  console_send(dev, str_5_56);
  console_send(dev, "ATL121\r");
  console_send(dev, str_5_155);
  console_send(dev, "ATF1\r");

  console_register_callback(dev, KEY_STRING, console_lara_pass_modify_pin0, NULL);
  console_register_callback(dev, KEY_CANCEL, console_lara_pass_modify2, NULL);
  return NULL;
}

static CALLBACK void* console_lara_pass_modify_count1(ProtDevice *dev, int cnt_as_int, void *null)
{
  int cnt;
  
  console_send(dev, "ATS04\r");
  console_send(dev, "ATL000\r");
  console_send(dev, "ATF0\r");
  
  console_send(dev, str_5_57);
  console_send(dev, str_5_51);
  
  sscanf((char*)cnt_as_int, "%d", &cnt);
  lara_set_counter(CONSOLE->support_idx, cnt);
  
  CONSOLE->last_menu_item = console_lara_pass_modify2;
  timeout_on(CONSOLE->timeout, (timeout_func)console_return_to_last_menu_item, dev, 0, TIMEOUT_MSG);
  
  return NULL;
}

static CALLBACK void* console_lara_pass_modify_count(ProtDevice *dev, int press, void *null)
{
  char cmd[32];
  int cnt;
  
  console_send(dev, "ATS04\r");
  
  console_send(dev, str_5_59);
  cnt = _lara->tessera[CONSOLE->support_idx].pincnt;
  sprintf(cmd, str_5_157, cnt);
  console_send(dev, cmd);
  console_send(dev, str_5_60);
  console_send(dev, "ATL111\r");
  console_send(dev, str_5_156);
  console_send(dev, "ATF1\r");

  console_register_callback(dev, KEY_STRING, console_lara_pass_modify_count1, NULL);
  console_register_callback(dev, KEY_CANCEL, console_lara_pass_modify2, NULL);
  
  return NULL;
}

static CALLBACK void* console_lara_pass_prof_modify_term_include1(ProtDevice *dev, int press, void *term_as_voidp)
{
  int term = (int)term_as_voidp;
  unsigned char ev[8];
  
  ev[0] = 247;
  ev[1] = 0;
  ev[2] = 39;
  ev[3] = CONSOLE->support_idx & 0xff;
  ev[4] = CONSOLE->support_idx >> 8;
  codec_queue_event(ev);
  
  _lara->tessera[CONSOLE->support_idx].p.s.term[term>>3] |= (1<<(term & 0x07));
  _laraf->tessera[CONSOLE->support_idx].ag = 1;
  lara_save(0);
  
  console_send(dev, "ATS04\r");
  console_send(dev, str_5_61);
  console_send(dev, str_5_51);
            
  CONSOLE->last_menu_item = console_lara_pass_prof_modify_term;
  timeout_on(CONSOLE->timeout, (timeout_func)console_return_to_last_menu_item, dev, 0, TIMEOUT_MSG);
  
  return NULL;
}

static CALLBACK void* console_lara_pass_prof_modify_term_include(ProtDevice *dev, int press, void *null)
{
  char *term_cod, *term_desc;
  int i;

  console_list_free(CONSOLE->support_list);
  CONSOLE->support_list = NULL;

  console_send(dev, "ATS04\r");
  console_send(dev, str_5_63);

  for(i=0; i<LARA_N_TERMINALI; i++)
  {
    if(!(_lara->tessera[CONSOLE->support_idx].p.s.term[i>>3] & (1<<(i & 0x07))) && lara_terminal_prog[i])
    {
      string_terminal_name(i, &term_cod, &term_desc);
      CONSOLE->support_list = console_list_add(CONSOLE->support_list, term_cod, term_desc, console_lara_pass_prof_modify_term_include1, i);
    }
  }

  CONSOLE->list_show_cancel = console_lara_pass_prof_modify_term;
  console_list_show(dev, CONSOLE->support_list, 0, 2, 0);
  
  return NULL;
}

static CALLBACK void* console_lara_pass_prof_modify_term_exclude1(ProtDevice *dev, int press, void *term_as_voidp)
{
  int term = (int)term_as_voidp;
  unsigned char ev[8];
  
  ev[0] = 247;
  ev[1] = 0;
  ev[2] = 39;
  ev[3] = CONSOLE->support_idx & 0xff;
  ev[4] = CONSOLE->support_idx >> 8;
  codec_queue_event(ev);
  
  _lara->tessera[CONSOLE->support_idx].p.s.term[term>>3] &= ~(1<<(term & 0x07));
  _laraf->tessera[CONSOLE->support_idx].ag = 1;
  lara_save(0);
  
  console_send(dev, "ATS04\r");
  console_send(dev, str_5_61);
  console_send(dev, str_5_65);
            
  CONSOLE->last_menu_item = console_lara_pass_prof_modify_term;
  timeout_on(CONSOLE->timeout, (timeout_func)console_return_to_last_menu_item, dev, 0, TIMEOUT_MSG);
  
  return NULL;
}

static CALLBACK void* console_lara_pass_prof_modify_term_exclude(ProtDevice *dev, int press, void *null)
{
  char *term_cod, *term_desc;
  int i;

  console_list_free(CONSOLE->support_list);
  CONSOLE->support_list = NULL;

  console_send(dev, "ATS04\r");
  console_send(dev, str_5_66);

  for(i=0; i<LARA_N_TERMINALI; i++)
  {
    if(_lara->tessera[CONSOLE->support_idx].p.s.term[i>>3] & (1<<(i & 0x07)))
    {
      string_terminal_name(i, &term_cod, &term_desc);
      CONSOLE->support_list = console_list_add(CONSOLE->support_list, term_cod, term_desc, console_lara_pass_prof_modify_term_exclude1, i);
    }
  }

  CONSOLE->list_show_cancel = console_lara_pass_prof_modify_term;
  console_list_show(dev, CONSOLE->support_list, 0, 2, 0);
  
  return NULL;
}

static MenuItem console_lara_pass_prof_modify_term_menu[] = {
	{&str_5_67, L1 | L2 | L3 | L4 | L5 | L6 | L7 | L8, console_lara_pass_prof_modify_term_include},
	{&str_5_68, L1 | L2 | L3 | L4 | L5 | L6 | L7 | L8, console_lara_pass_prof_modify_term_exclude},
	{NULL, 0, NULL}
};

static CALLBACK void* console_lara_pass_prof_modify_term(ProtDevice *dev, int press, void *null)
{
  console_send(dev, "ATS04\r");
  console_send(dev, str_5_69);

  CONSOLE->list_show_cancel = console_lara_pass_modify_profile1;
  console_submenu(dev, console_lara_pass_prof_modify_term_menu, 0, 3);
  
  return NULL;
}

static CALLBACK void* console_lara_pass_prof_modify_fosett_join1(ProtDevice *dev, int press, void *wtmg_as_voidp)
{
  unsigned char ev[8];
  
  ev[0] = 247;
  ev[1] = 0;
  ev[2] = 38;
  ev[3] = CONSOLE->support_idx & 0xff;
  ev[4] = CONSOLE->support_idx >> 8;
  ev[5] = (int)wtmg_as_voidp;
  codec_queue_event(ev);
  
  _lara->tessera[CONSOLE->support_idx].p.s.fosett = (int)wtmg_as_voidp;
  _laraf->tessera[CONSOLE->support_idx].ag = 1;
  lara_save(0);
  
  console_send(dev, "ATS04\r");
  console_send(dev, str_5_70);
  console_send(dev, str_5_71);
            
  CONSOLE->last_menu_item = console_lara_pass_prof_modify_fosett;
  timeout_on(CONSOLE->timeout, (timeout_func)console_return_to_last_menu_item, dev, 0, TIMEOUT_MSG);
  
  return NULL;
}

static CALLBACK void* console_lara_pass_prof_modify_fosett_join(ProtDevice *dev, int press, void *null)
{
  char *wtmg_cod, *wtmg_desc;
  int i;

  console_send(dev, "ATS04\r");
  console_send(dev, str_5_72);

  console_list_free(CONSOLE->support_list);
  CONSOLE->support_list = NULL;

  for(i=1; i<=LARA_N_FASCE; i++)
  {
    if(_lara->tessera[CONSOLE->support_idx].p.s.fosett != i)
    {
      string_weektiming_name(i-1, &wtmg_cod, &wtmg_desc);
      CONSOLE->support_list = console_list_add(CONSOLE->support_list, wtmg_cod, wtmg_desc, console_lara_pass_prof_modify_fosett_join1, i);
    }
  }

  CONSOLE->list_show_cancel = console_lara_pass_prof_modify_fosett;
  console_list_show(dev, CONSOLE->support_list, 0, 2, 0);
  
  return NULL;
}

static CALLBACK void* console_lara_pass_prof_modify_fosett_delete1(ProtDevice *dev, int press, void *null)
{
  unsigned char ev[8];
  
  ev[0] = 247;
  ev[1] = 0;
  ev[2] = 38;
  ev[3] = CONSOLE->support_idx & 0xff;
  ev[4] = CONSOLE->support_idx >> 8;
  ev[5] = 0;
  codec_queue_event(ev);
  
  _lara->tessera[CONSOLE->support_idx].p.s.fosett = 0;
  _laraf->tessera[CONSOLE->support_idx].ag = 1;
  lara_save(0);
  
  console_send(dev, "ATS04\r");
  console_send(dev, str_5_70);
  console_send(dev, str_5_74);
            
  CONSOLE->last_menu_item = console_lara_pass_prof_modify_fosett;
  timeout_on(CONSOLE->timeout, (timeout_func)console_return_to_last_menu_item, dev, 0, TIMEOUT_MSG);
  
  return NULL;
}

static CALLBACK void* console_lara_pass_prof_modify_fosett_delete(ProtDevice *dev, int press, void *null)
{
  char *wtmg_cod, *wtmg_desc, cmd[32];

  console_send(dev, "ATS04\r");
  console_send(dev, str_5_75);
  
  if(!_lara->tessera[CONSOLE->support_idx].p.s.fosett)
  {
    console_send(dev, str_5_76);
  }
  else
  {
    string_weektiming_name(_lara->tessera[CONSOLE->support_idx].p.s.fosett-1, &wtmg_cod, &wtmg_desc);
    sprintf(cmd, str_5_151, wtmg_cod);
    console_send(dev, cmd);
    sprintf(cmd, str_5_153, wtmg_desc);
    console_send(dev, cmd);
    console_register_callback(dev, KEY_ENTER, console_lara_pass_prof_modify_fosett_delete1, NULL);
  }
  
  console_register_callback(dev, KEY_CANCEL, console_lara_pass_prof_modify_fosett, NULL);
  return NULL;
}

static CALLBACK void* console_lara_pass_prof_modify_fosett_show(ProtDevice *dev, int press, void *null)
{
  CONSOLE->list_show_cancel = console_lara_pass_prof_modify_fosett;
  console_lara_impostazioni_fosett_show(dev, 0, (void*)((int)(_lara->tessera[CONSOLE->support_idx].p.s.fosett)));
    
  return NULL;
}

static MenuItem console_lara_pass_prof_modify_fosett_menu[] = {
	{&str_5_77, L1 | L2 | L3 | L4 | L5 | L6 | L7 | L8, console_lara_pass_prof_modify_fosett_join},
	{&str_5_78, L1 | L2 | L3 | L4 | L5 | L6 | L7 | L8, console_lara_pass_prof_modify_fosett_delete},
	{&str_5_79, L1 | L2 | L3 | L4 | L5 | L6 | L7 | L8, console_lara_pass_prof_modify_fosett_show},
	{NULL, 0, NULL}
};

static CALLBACK void* console_lara_pass_prof_modify_fosett(ProtDevice *dev, int press, void *null)
{
  console_send(dev, "ATS04\r");
  console_send(dev, str_5_80);

  CONSOLE->list_show_cancel = console_lara_pass_modify_profile1;
  console_submenu(dev, console_lara_pass_prof_modify_fosett_menu, 0, 3);
  
  return NULL;
}

static CALLBACK void* console_lara_pass_prof_modify_holiday_include1(ProtDevice *dev, int press, void *hday_as_voidp)
{
  int hday = (int)hday_as_voidp - 1;
  unsigned char ev[8];
  
  ev[0] = 247;
  ev[1] = 0;
  ev[2] = 40;
  ev[3] = CONSOLE->support_idx & 0xff;
  ev[4] = CONSOLE->support_idx >> 8;
  codec_queue_event(ev);
  
  _lara->tessera[CONSOLE->support_idx].p.s.fest[hday>>3] |= (1<<(hday & 0x07));
  _laraf->tessera[CONSOLE->support_idx].ag = 1;
  lara_save(0);
  
  console_send(dev, "ATS04\r");
  console_send(dev, str_5_81);
  console_send(dev, str_5_71);
            
  CONSOLE->last_menu_item = console_lara_pass_prof_modify_holiday;
  timeout_on(CONSOLE->timeout, (timeout_func)console_return_to_last_menu_item, dev, 0, TIMEOUT_MSG);
  
  return NULL;
}

static CALLBACK void* console_lara_pass_prof_modify_holiday_include(ProtDevice *dev, int press, void *null)
{
  char *hday_cod, *hday_desc;
  int i;

  console_list_free(CONSOLE->support_list);
  CONSOLE->support_list = NULL;

  console_send(dev, "ATS04\r");
  console_send(dev, str_5_72);

  for(i=0; i<LARA_N_FESTIVI; i++)
  {
    if(!(_lara->tessera[CONSOLE->support_idx].p.s.fest[i>>3] & (1<<(i & 0x07))))
    {
      string_holiday_name(i, &hday_cod, &hday_desc);
      CONSOLE->support_list = console_list_add(CONSOLE->support_list, hday_cod, hday_desc, console_lara_pass_prof_modify_holiday_include1, i+1);
    }
  }

  CONSOLE->list_show_cancel = console_lara_pass_prof_modify_holiday;
  console_list_show(dev, CONSOLE->support_list, 0, 2, 0);
  
  return NULL;
}

static CALLBACK void* console_lara_pass_prof_modify_holiday_exclude1(ProtDevice *dev, int press, void *hday_as_voidp)
{
  int hday = (int)hday_as_voidp - 1;
  unsigned char ev[8];
  
  ev[0] = 247;
  ev[1] = 0;
  ev[2] = 40;
  ev[3] = CONSOLE->support_idx & 0xff;
  ev[4] = CONSOLE->support_idx >> 8;
  codec_queue_event(ev);
  
  _lara->tessera[CONSOLE->support_idx].p.s.fest[hday>>3] &= ~(1<<(hday & 0x07));
  _laraf->tessera[CONSOLE->support_idx].ag = 1;
  lara_save(0);
  
  console_send(dev, "ATS04\r");
  console_send(dev, str_5_81);
  console_send(dev, str_5_74);
            
  CONSOLE->last_menu_item = console_lara_pass_prof_modify_holiday;
  timeout_on(CONSOLE->timeout, (timeout_func)console_return_to_last_menu_item, dev, 0, TIMEOUT_MSG);
  
  return NULL;
}

static CALLBACK void* console_lara_pass_prof_modify_holiday_exclude(ProtDevice *dev, int press, void *null)
{
  char *hday_cod, *hday_desc;
  int i;

  console_list_free(CONSOLE->support_list);
  CONSOLE->support_list = NULL;

  console_send(dev, "ATS04\r");
  console_send(dev, str_5_75);

  for(i=0; i<LARA_N_FESTIVI; i++)
  {
    if(_lara->tessera[CONSOLE->support_idx].p.s.fest[i>>3] & (1<<(i & 0x07)))
    {
      string_holiday_name(i, &hday_cod, &hday_desc);
      CONSOLE->support_list = console_list_add(CONSOLE->support_list, hday_cod, hday_desc, console_lara_pass_prof_modify_holiday_exclude1, i+1);
    }
  }

  CONSOLE->list_show_cancel = console_lara_pass_prof_modify_holiday;
  console_list_show(dev, CONSOLE->support_list, 0, 2, 0);
  
  return NULL;
}

static CALLBACK void* console_lara_pass_prof_modify_holiday_show(ProtDevice *dev, int press, void *null)
{
  CONSOLE->list_show_cancel = console_lara_pass_prof_modify_holiday;
  console_lara_impostazioni_holiday_show(dev, 0, _lara->tessera[CONSOLE->support_idx].p.s.fest);
  
  return NULL;
}

static MenuItem console_lara_pass_prof_modify_holiday_menu[] = {
	{&str_5_87, L1 | L2 | L3 | L4 | L5 | L6 | L7 | L8, console_lara_pass_prof_modify_holiday_include},
	{&str_5_88, L1 | L2 | L3 | L4 | L5 | L6 | L7 | L8, console_lara_pass_prof_modify_holiday_exclude},
	{&str_5_89, L1 | L2 | L3 | L4 | L5 | L6 | L7 | L8, console_lara_pass_prof_modify_holiday_show},
	{NULL, 0, NULL}
};

static CALLBACK void* console_lara_pass_prof_modify_holiday(ProtDevice *dev, int press, void *null)
{
  console_send(dev, "ATS04\r");
  console_send(dev, str_5_90);

  CONSOLE->list_show_cancel = console_lara_pass_modify_profile1;
  console_submenu(dev, console_lara_pass_prof_modify_holiday_menu, 0, 3);
  return NULL;
}

static CALLBACK void* console_lara_pass_prof_modify_option_coerc1(ProtDevice *dev, int press, void *action_as_voidp)
{
  unsigned char ev[8];
  
  console_send(dev, "ATS06\r");
  
  _lara->tessera[CONSOLE->support_idx].stato.s.coerc = (int)action_as_voidp;
  _laraf->tessera[CONSOLE->support_idx].ag = 1;
  lara_save(0);
  
  ev[0] = 247;
  ev[1] = 0;
  ev[2] = 41;
  ev[3] = CONSOLE->support_idx & 0xff;
  ev[4] = CONSOLE->support_idx >> 8;
  ev[5] = _lara->tessera[CONSOLE->support_idx].stato.b;
  codec_queue_event(ev);
  
  if(action_as_voidp)
    console_send(dev, str_5_91);
  else
    console_send(dev, str_5_92);
  
  CONSOLE->last_menu_item = console_lara_pass_prof_modify_option;
  timeout_on(CONSOLE->timeout, (timeout_func)console_return_to_last_menu_item, dev, 0, TIMEOUT_MSG);
  
  return NULL;
}

static CALLBACK void* console_lara_pass_prof_modify_option_coerc(ProtDevice *dev, int press, void *null)
{
  console_send(dev, "ATS04\r");
  console_send(dev, str_5_93);
  
  if(_lara->tessera[CONSOLE->support_idx].stato.s.coerc)
  {
    console_send(dev, str_5_91);
    console_send(dev, str_5_95);
    console_register_callback(dev, KEY_ENTER, console_lara_pass_prof_modify_option_coerc1, (void*)0);
  }
  else
  {
    console_send(dev, str_5_96);
    console_send(dev, str_5_97);
    console_register_callback(dev, KEY_ENTER, console_lara_pass_prof_modify_option_coerc1, (void*)1);
  }

  console_register_callback(dev, KEY_CANCEL, console_lara_pass_prof_modify_option, NULL);
  return NULL;
}

static CALLBACK void* console_lara_pass_prof_modify_option_passback1(ProtDevice *dev, int press, void *action_as_voidp)
{
  unsigned char ev[8];
  
  console_send(dev, "ATS06\r");
  
  _lara->tessera[CONSOLE->support_idx].stato.s.apbk = (int)action_as_voidp;
  _laraf->tessera[CONSOLE->support_idx].ag = 1;
  lara_save(0);
  
  ev[0] = 247;
  ev[1] = 0;
  ev[2] = 41;
  ev[3] = CONSOLE->support_idx & 0xff;
  ev[4] = CONSOLE->support_idx >> 8;
  ev[5] = _lara->tessera[CONSOLE->support_idx].stato.b;
  codec_queue_event(ev);
  
  if(action_as_voidp)
    console_send(dev, str_5_92);
  else
    console_send(dev, str_5_91);
  
  CONSOLE->last_menu_item = console_lara_pass_prof_modify_option;
  timeout_on(CONSOLE->timeout, (timeout_func)console_return_to_last_menu_item, dev, 0, TIMEOUT_MSG);
  
  return NULL;
}

static CALLBACK void* console_lara_pass_prof_modify_option_passback(ProtDevice *dev, int press, void *null)
{
  console_send(dev, "ATS04\r");
  console_send(dev, str_5_100);
  
  if(!_lara->tessera[CONSOLE->support_idx].stato.s.apbk)
  {
    console_send(dev, str_5_91);
    console_send(dev, str_5_95);
    console_register_callback(dev, KEY_ENTER, console_lara_pass_prof_modify_option_passback1, (void*)1);
  }
  else
  {
    console_send(dev, str_5_92);
    console_send(dev, str_5_97);
    console_register_callback(dev, KEY_ENTER, console_lara_pass_prof_modify_option_passback1, (void*)0);
  }

  console_register_callback(dev, KEY_CANCEL, console_lara_pass_prof_modify_option, NULL);
  return NULL;
}

#if 0
static CALLBACK void* console_lara_pass_prof_modify_option_supervisor1(ProtDevice *dev, int press, void *action_as_voidp)
{
  unsigned char ev[8];
  
  console_send(dev, "ATS06\r");
  
  _lara->tessera[CONSOLE->support_idx].stato.s.supervisor = (int)action_as_voidp;
  _laraf->tessera[CONSOLE->support_idx].ag = 1;
  lara_save(0);
  
  ev[0] = 247;
  ev[1] = 0;
  ev[2] = 41;
  ev[3] = CONSOLE->support_idx & 0xff;
  ev[4] = CONSOLE->support_idx >> 8;
  ev[5] = _lara->tessera[CONSOLE->support_idx].stato.b;
  codec_queue_event(ev);
  
  if(action_as_voidp)
    console_send(dev, str_5_105);
  else
    console_send(dev, str_5_106);
  
  CONSOLE->last_menu_item = console_lara_pass_prof_modify_option;
  timeout_on(CONSOLE->timeout, (timeout_func)console_return_to_last_menu_item, dev, 0, TIMEOUT_MSG);
  
  return NULL;
}

static CALLBACK void* console_lara_pass_prof_modify_option_supervisor(ProtDevice *dev, int press, void *null)
{
  console_send(dev, "ATS04\r");
  console_send(dev, str_5_107);
  
  if(_lara->tessera[CONSOLE->support_idx].stato.s.supervisor)
  {
    console_send(dev, str_5_108);
    console_send(dev, str_5_109);
    console_register_callback(dev, KEY_ENTER, console_lara_pass_prof_modify_option_supervisor1, (void*)0);
  }
  else
  {
    console_send(dev, str_5_110);
    console_send(dev, str_5_111);
    console_register_callback(dev, KEY_ENTER, console_lara_pass_prof_modify_option_supervisor1, (void*)1);
  }

  console_register_callback(dev, KEY_CANCEL, console_lara_pass_prof_modify_option, NULL);
  return NULL;
}
#endif

static MenuItem console_lara_pass_prof_modify_option_menu[] = {
	{&str_5_42, L1 | L2 | L3 | L4 | L5 | L6 | L7 | L8, console_lara_pass_prof_modify_option_coerc},
	{&str_5_43, L1 | L2 | L3 | L4 | L5 | L6 | L7 | L8, console_lara_pass_prof_modify_option_passback},
//	{&str_5_12, L1 | L2 | L3 | L4 | L5 | L6 | L7 | L8, console_lara_pass_prof_modify_option_supervisor},
	{NULL, 0, NULL}
};

static CALLBACK void* console_lara_pass_prof_modify_option(ProtDevice *dev, int press, void *null)
{
  console_send(dev, "ATS04\r");
  console_send(dev, str_5_115);

  CONSOLE->list_show_cancel = console_lara_pass_modify_profile1;
  console_submenu(dev, console_lara_pass_prof_modify_option_menu, 0, 3);
  return NULL;
}

static CALLBACK void* console_lara_pass_modify_profile_sec_del1(ProtDevice *dev, int press, void *prof_as_voidp)
{
  console_send(dev, "ATS03\r");
  
  lara_del_profile_sec(CONSOLE->support_idx, (int)prof_as_voidp);
  
  console_send(dev, str_5_145);
  console_send(dev, str_5_148);
          
  CONSOLE->last_menu_item = console_lara_pass_modify_profile_sec_del;
  timeout_on(CONSOLE->timeout, (timeout_func)console_return_to_last_menu_item, dev, 0, TIMEOUT_MSG);
  
  return NULL;
}

static CALLBACK void* console_lara_pass_modify_profile_sec_del(ProtDevice *dev, int press, void *null)
{
  char *prof_cod, *prof_desc;
  int i;
  
  console_list_free(CONSOLE->support_list);
  CONSOLE->support_list = NULL;
  
  for(i=1; (i<LARA_MULTIPROFILI) && _lara->tessera[CONSOLE->support_idx].p.profilo[i]; i++)
  {
    string_profile_name(_lara->tessera[CONSOLE->support_idx].p.profilo[i], &prof_cod, &prof_desc);
    CONSOLE->support_list = console_list_add(CONSOLE->support_list, prof_cod, prof_desc, console_lara_pass_modify_profile_sec_del1, _lara->tessera[CONSOLE->support_idx].p.profilo[i]);
  }
  
  CONSOLE->list_show_cancel = console_lara_pass_modify_profile;
  console_list_show(dev, CONSOLE->support_list, 0, 3, 0);
  
  return NULL;
}

static CALLBACK void* console_lara_pass_modify_profile_sec_add1(ProtDevice *dev, int press, void *prof_as_voidp)
{
  console_send(dev, "ATS03\r");
  
  lara_add_profile_sec(CONSOLE->support_idx, (int)prof_as_voidp);
  
  console_send(dev, str_5_145);
  console_send(dev, str_5_147);
          
  CONSOLE->last_menu_item = console_lara_pass_modify_profile_sec_add;
  timeout_on(CONSOLE->timeout, (timeout_func)console_return_to_last_menu_item, dev, 0, TIMEOUT_MSG);
  
  return NULL;
}

static CALLBACK void* console_lara_pass_modify_profile_sec_add(ProtDevice *dev, int press, void *null)
{
  char *prof_cod, *prof_desc;
  int i, j;
  
  for(i=1; (i<LARA_MULTIPROFILI) && _lara->tessera[CONSOLE->support_idx].p.profilo[i]; i++);
  if(i == LARA_MULTIPROFILI)
  {
    /* Profilo completo */
    console_send(dev, "ATS03\r");
    console_send(dev, str_5_145);
    console_send(dev, str_5_146);
            
    CONSOLE->last_menu_item = console_lara_pass_modify_profile;
    timeout_on(CONSOLE->timeout, (timeout_func)console_return_to_last_menu_item, dev, 0, TIMEOUT_MSG);
  }
  else
  {
    console_list_free(CONSOLE->support_list);
    CONSOLE->support_list = NULL;
    
    for(i=1; i<256; i++)
    {
      for(j=0; (j<LARA_MULTIPROFILI) &&
          _lara->tessera[CONSOLE->support_idx].p.profilo[j] != i; j++);
      
      if(j == LARA_MULTIPROFILI)
      {
        string_profile_name(i, &prof_cod, &prof_desc);
        CONSOLE->support_list = console_list_add(CONSOLE->support_list, prof_cod, prof_desc,
          console_lara_pass_modify_profile_sec_add1, i);
      }
    }
    
    CONSOLE->list_show_cancel = console_lara_pass_modify_profile;
    console_list_show(dev, CONSOLE->support_list, 0, 3, 0);
  }
  
  return NULL;
}

static CALLBACK void* console_lara_pass_modify_profile1(ProtDevice *dev, int press, void *prof_as_voidp)
{
  char *prof_cod, *prof_desc, cmd[32];

  console_send(dev, "ATS03\r");
  
  lara_set_profile(CONSOLE->support_idx, (int)prof_as_voidp);
  
  if(prof_as_voidp)
  {
    console_send(dev, str_5_116);
    console_send(dev, str_5_117);
            
    CONSOLE->last_menu_item = console_lara_pass_modify_profile;
    timeout_on(CONSOLE->timeout, (timeout_func)console_return_to_last_menu_item, dev, 0, TIMEOUT_MSG);
  }
  else
  {
    string_profile_name(_lara->tessera[CONSOLE->support_idx].p.s.profilo, &prof_cod, &prof_desc);
    sprintf(cmd, str_5_158, prof_cod);
    console_send(dev, cmd);
    sprintf(cmd, str_5_159, prof_desc);
    console_send(dev, cmd);
  
    CONSOLE->list_show_cancel = console_lara_pass_modify_profile_pri;
    console_submenu(dev, console_lara_pass_prof_modify_menu, CONSOLE->support_idx, 4);
  }
  
  return NULL;
}

static CALLBACK void* console_lara_pass_modify_profile_pri(ProtDevice *dev, int press, void *null)
{
  char *prof_cod, *prof_desc, cmd[32];
  int i;
  
  console_send(dev, "ATS03\r");
  
  string_profile_name(_lara->tessera[CONSOLE->support_idx].p.s.profilo, &prof_cod, &prof_desc);
  sprintf(cmd, str_5_158, prof_cod);
  console_send(dev, cmd);
  sprintf(cmd, str_5_159, prof_desc);
  console_send(dev, cmd);
  
  console_list_free(CONSOLE->support_list);
  CONSOLE->support_list = NULL;
  
  for(i=0; i<256; i++)
  {
    if(!i || (i != _lara->tessera[CONSOLE->support_idx].p.s.profilo))
    {
      string_profile_name(i, &prof_cod, &prof_desc);
      CONSOLE->support_list = console_list_add(CONSOLE->support_list, prof_cod, prof_desc, console_lara_pass_modify_profile1, i);
    }
  }
  
  if(!_lara->tessera[CONSOLE->support_idx].p.s.profilo)
    CONSOLE->list_show_cancel = console_lara_pass_modify2;
  else
    CONSOLE->list_show_cancel = console_lara_pass_modify_profile;
  console_list_show(dev, CONSOLE->support_list, 0, 1, 0);

  return NULL;
}

static MenuItem console_lara_pass_modify_profile_menu[] = {
	{&str_5_142, L1 | L2 | L3 | L4 | L5 | L6 | L7 | L8, console_lara_pass_modify_profile_pri},
	{&str_5_143, L1 | L2 | L3 | L4 | L5 | L6 | L7 | L8, console_lara_pass_modify_profile_sec_add},
	{&str_5_144, L1 | L2 | L3 | L4 | L5 | L6 | L7 | L8, console_lara_pass_modify_profile_sec_del},
	{NULL, 0, NULL}
};

static CALLBACK void* console_lara_pass_modify_profile(ProtDevice *dev, int press, void *null)
{
  /*
      Se il profilo è individuale, permetto la modifica del solo profilo primario.
      Se il profilo è multiplo, attivo un menù per la scelta del profilo primario o dei secondari.
      Per il primario la gestione è quella attuale, per i secondari occorre una gestione
      assegna/inibisci.
  */
  if(!_lara->tessera[CONSOLE->support_idx].p.s.profilo)
    return console_lara_pass_modify_profile_pri(dev, press, null);
  else
  {
    console_send(dev, "ATS03\r");
    CONSOLE->list_show_cancel = console_lara_pass_modify2;
    console_submenu(dev, console_lara_pass_modify_profile_menu, CONSOLE->support_idx, 4);
    return NULL;
  }
}

static CALLBACK void* console_lara_pass_modify_area1(ProtDevice *dev, int press, void *area_as_voidp)
{
  console_send(dev, "ATS04\r");
  console_send(dev, str_5_118);
  console_send(dev, str_5_119);

  lara_set_area(CONSOLE->support_idx, (int)area_as_voidp);
  
  CONSOLE->last_menu_item = console_lara_pass_modify2;
  timeout_on(CONSOLE->timeout, (timeout_func)console_return_to_last_menu_item, dev, 0, TIMEOUT_MSG);
  
  return NULL;
}

static CALLBACK void* console_lara_pass_modify_area(ProtDevice *dev, int press, void *null)
{
  char *area_cod, *area_desc, cmd[32];
  int i;
  
  console_send(dev, "ATS04\r");
  
  string_area_name(_lara->tessera[CONSOLE->support_idx].area, &area_cod, &area_desc);
  sprintf(cmd, str_5_160, area_cod);
  console_send(dev, cmd);
  sprintf(cmd, str_5_159, area_desc);
  console_send(dev, cmd);
  
  console_list_free(CONSOLE->support_list);
  CONSOLE->support_list = NULL;
  
  for(i=0; i<LARA_N_AREE; i++)
  {
    if(i != _lara->tessera[CONSOLE->support_idx].area)
    {
      string_area_name(i, &area_cod, &area_desc);
      CONSOLE->support_list = console_list_add(CONSOLE->support_list, area_cod, area_desc, console_lara_pass_modify_area1, i);
    }
  }
  
  /* Aggiunge l'AreaSelf */
  if(_lara->tessera[CONSOLE->support_idx].area != TEBE_AREASELF)
  {
    string_area_name(TEBE_AREASELF, &area_cod, &area_desc);
    CONSOLE->support_list = console_list_add(CONSOLE->support_list, area_cod, area_desc, console_lara_pass_modify_area1, TEBE_AREASELF);
  }
  
  CONSOLE->list_show_cancel = console_lara_pass_modify2;
  console_list_show(dev, CONSOLE->support_list, 0, 1, 0);

  return NULL;
}

static CALLBACK void* console_lara_pass_modify_abil_action(ProtDevice *dev, int press, void *action)
{
  lara_set_abil(CONSOLE->support_idx, (int)action);
  if((int)action == BADGE_CANC)
    console_lara_pass_modify(dev, 0, NULL);
  else
    console_lara_pass_modify2(dev, 0, NULL);
  return NULL;
}

static CALLBACK void* console_lara_pass_modify_abil_abil(ProtDevice *dev, int press, void *action)
{
  console_send(dev, "ATS05\r");
  if((int)action == BADGE_ABIL)
    console_send(dev, str_5_120);
  else
    console_send(dev, str_5_121);
  console_send(dev, str_5_122);
  
  console_register_callback(dev, KEY_CANCEL, console_lara_pass_modify_abil, NULL);
  console_register_callback(dev, KEY_ENTER, console_lara_pass_modify_abil_action, action);
  return NULL;
}

static CALLBACK void* console_lara_pass_modify_abil_canc(ProtDevice *dev, int press, void *null)
{
  console_send(dev, "ATS05\r");
  console_send(dev, str_5_123);
  console_send(dev, str_5_122);
  
  console_register_callback(dev, KEY_CANCEL, console_lara_pass_modify_abil, NULL);
  console_register_callback(dev, KEY_ENTER, console_lara_pass_modify_abil_action, (void*)BADGE_CANC);
  return NULL;
}

static MenuItem console_lara_pass_prof_modify_abil_menu_1[] = {
	{&str_5_125, L1 | L2 | L3 | L4 | L5 | L6 | L7 | L8, console_lara_pass_modify_abil_abil},
	{&str_5_126, L1 | L2 | L3 | L4 | L5 | L6 | L7 | L8, console_lara_pass_modify_abil_canc},
	{NULL, 0, NULL}
};

static MenuItem console_lara_pass_prof_modify_abil_menu_2[] = {
	{&str_5_127, L1 | L2 | L3 | L4 | L5 | L6 | L7 | L8, console_lara_pass_modify_abil_abil},
	{&str_5_128, L1 | L2 | L3 | L4 | L5 | L6 | L7 | L8, console_lara_pass_modify_abil_canc},
	{NULL, 0, NULL}
};

static CALLBACK void* console_lara_pass_modify_abil(ProtDevice *dev, int press, void *null)
{
  console_send(dev, "ATS04\r");
  
  CONSOLE->list_show_cancel = console_lara_pass_modify2;

  console_send(dev, str_5_129);
  if(_lara->tessera[CONSOLE->support_idx].stato.s.abil == BADGE_ABIL)
  {
    console_send(dev, str_5_120);
    console_submenu(dev, console_lara_pass_prof_modify_abil_menu_1, BADGE_DISABIL, 2);
  }
  else
  {
    console_send(dev, str_5_121);
    console_submenu(dev, console_lara_pass_prof_modify_abil_menu_2, BADGE_ABIL, 2);
  }
  
  return NULL;
}

static CALLBACK void* console_lara_pass_modify_supervisor1(ProtDevice *dev, int press, void *action_as_voidp)
{
  unsigned char ev[8];
  
  console_send(dev, "ATS06\r");
  
  _lara->tessera[CONSOLE->support_idx].stato.s.supervisor = (int)action_as_voidp;
  _laraf->tessera[CONSOLE->support_idx].ag = 1;
  lara_save(0);
  
  ev[0] = 247;
  ev[1] = 0;
  ev[2] = 41;
  ev[3] = CONSOLE->support_idx & 0xff;
  ev[4] = CONSOLE->support_idx >> 8;
  ev[5] = _lara->tessera[CONSOLE->support_idx].stato.b;
  codec_queue_event(ev);
  
  if(!action_as_voidp)
    console_send(dev, str_5_92);
  else
    console_send(dev, str_5_91);
  
  CONSOLE->last_menu_item = console_lara_pass_modify2;
  timeout_on(CONSOLE->timeout, (timeout_func)console_return_to_last_menu_item, dev, 0, TIMEOUT_MSG);
  
  return NULL;
}

static CALLBACK void* console_lara_pass_modify_supervisor(ProtDevice *dev, int press, void *null)
{
  console_send(dev, "ATS04\r");
  console_send(dev, str_5_134);
  
  if(_lara->tessera[CONSOLE->support_idx].stato.s.supervisor)
  {
    console_send(dev, str_5_91);
    console_send(dev, str_5_95);
    console_register_callback(dev, KEY_ENTER, console_lara_pass_modify_supervisor1, (void*)0);
  }
  else
  {
    console_send(dev, str_5_92);
    console_send(dev, str_5_97);
    console_register_callback(dev, KEY_ENTER, console_lara_pass_modify_supervisor1, (void*)1);
  }

  console_register_callback(dev, KEY_CANCEL, console_lara_pass_modify2, NULL);
  return NULL;
}

static CALLBACK void* console_lara_pass_modify2(ProtDevice *dev, int press, void *null)
{
  char cmd[24];
  
  console_send(dev, "ATF0\r");
  console_send(dev, "ATL000\r");
  console_send(dev, "ATS03\r");

  if((CONSOLE->support_idx >= lara_NumBadge) ||
     (_lara->tessera[CONSOLE->support_idx].stato.s.abil == BADGE_VUOTO) ||
     (_lara->tessera[CONSOLE->support_idx].stato.s.abil == BADGE_CANC))
  {
    console_send(dev, str_5_139);
    CONSOLE->last_menu_item = console_lara_pass_modify;
    timeout_on(CONSOLE->timeout, (timeout_func)console_return_to_last_menu_item, dev, 0, TIMEOUT_MSG);
    return NULL;
  }
  
  sprintf(cmd, str_5_154, CONSOLE->support_idx);
  console_send(dev, cmd);
  
  CONSOLE->list_show_cancel = console_lara_pass_modify;
  if(_lara->param & LARA_PARAM_COUNTER)
    console_submenu(dev, console_lara_pass_menu_3, CONSOLE->support_idx, 4);
  else
    console_submenu(dev, console_lara_pass_menu_4, CONSOLE->support_idx, 4);
  
  return NULL;
}

static CALLBACK void* console_lara_pass_modify1(ProtDevice *dev, int id_as_int, void *null)
{
  console_send(dev, "ATL000\r");
  console_send(dev, "ATF0\r");
    
  sscanf((char*)id_as_int, "%d", &(CONSOLE->support_idx));
  console_lara_pass_modify2(dev, 0, NULL);
  
  return NULL;
}

CALLBACK void* console_lara_pass_modify(ProtDevice *dev, int press, void *null)
{
  console_disable_menu(dev, str_5_140);
  
  if(!console_check_lara(dev)) return NULL;
  
  console_send(dev, str_5_141);
  console_send(dev, "ATL111\r");
  console_send(dev, str_5_155);
  console_send(dev, "ATF1\r");

  console_register_callback(dev, KEY_STRING, console_lara_pass_modify1, NULL);
  console_register_callback(dev, KEY_CANCEL, console_show_menu, NULL);

  return NULL;
}


