#include "console.h"
#include "con_liste.h"
#include "../database.h"
#include "../command.h"
#include "../strings.h"
#include "../support.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/***********************************************************************
	LISTE menu functions
***********************************************************************/

static void console_liste_sens_list(ProtDevice *dev, int bitMask, ConsoleCallback func)
{
  char *sens_num, *sens_desc;
  int i;
  
  console_list_free(CONSOLE->support_list);
  CONSOLE->support_list = NULL;
  
  for(i=0; i<n_SE; i++)
    if(SE[i] & bitMask)
    {
      string_sensor_name(i, &sens_num, &sens_desc);
      if(CONSOLE->current_user->level > 2)
        CONSOLE->support_list = console_list_add(CONSOLE->support_list, sens_num, sens_desc, func, i);
      else
        CONSOLE->support_list = console_list_add(CONSOLE->support_list, sens_num, sens_desc, NULL, 0);
    }
    
  console_list_show(dev, CONSOLE->support_list, 0, 3, 0);
}

static CALLBACK void* console_liste_sens_accept_alarm(ProtDevice *dev, int press, void *sens_as_voidp)
{
  cmd_status_accept((int)sens_as_voidp, bitMUAlarm, MU_alarm);
  
  console_send(dev, "ATS00\r");
  console_send(dev, str_8_0);
  console_send(dev, str_8_1);

  timeout_on(CONSOLE->timeout, (timeout_func)console_return_to_last_menu_item, dev, 0, TIMEOUT_MSG);

  return NULL;
}

static CALLBACK void* console_liste_sens_accept_failure(ProtDevice *dev, int press, void *sens_as_voidp)
{
  cmd_status_accept((int)sens_as_voidp, bitMUFailure, MU_failure);
  
  console_send(dev, "ATS00\r");
  console_send(dev, str_8_2);
  console_send(dev, str_8_1);

  timeout_on(CONSOLE->timeout, (timeout_func)console_return_to_last_menu_item, dev, 0, TIMEOUT_MSG);

  return NULL;
}

static CALLBACK void* console_liste_sens_accept_sabotage(ProtDevice *dev, int press, void *sens_as_voidp)
{
  cmd_status_accept((int)sens_as_voidp, bitMUSabotage, MU_sabotage);
  
  console_send(dev, "ATS00\r");
  console_send(dev, str_8_4);
  console_send(dev, str_8_5);

  timeout_on(CONSOLE->timeout, (timeout_func)console_return_to_last_menu_item, dev, 0, TIMEOUT_MSG);

  return NULL;
}

static CALLBACK void* console_liste_sens_set_oos(ProtDevice *dev, int press, void *sens_as_voidp)
{
  cmd_sensor_off((int)sens_as_voidp);
  
  console_send(dev, "ATS00\r");
  console_send(dev, str_8_6);
  console_send(dev, str_8_7);

  timeout_on(CONSOLE->timeout, (timeout_func)console_return_to_last_menu_item, dev, 0, TIMEOUT_MSG);

  return NULL;
}

static CALLBACK void* console_liste_accept_oos_2(ProtDevice *dev, int press, void *sens_as_voidp);

static CALLBACK void* console_liste_accept_oos_1(ProtDevice *dev, int press, void *sens_as_voidp)
{
  console_send(dev, "ATR1\r");
  console_send(dev, str_8_8);
  console_send(dev, "ATR0\r");
  console_send(dev, str_8_9);
  
  console_register_callback(dev, KEY_ENTER, CONSOLE->liste_accept_func, sens_as_voidp);
  console_register_callback(dev, KEY_CANCEL, CONSOLE->last_menu_item, NULL);
  console_register_callback(dev, KEY_DOWN, console_liste_accept_oos_2, sens_as_voidp);
  
  return NULL;
}

static CALLBACK void* console_liste_accept_oos_2(ProtDevice *dev, int press, void *sens_as_voidp)
{
  console_send(dev, str_8_8);
  console_send(dev, "ATR1\r");
  console_send(dev, str_8_9);
  console_send(dev, "ATR0\r");

  console_register_callback(dev, KEY_ENTER, console_liste_sens_set_oos, sens_as_voidp);
  console_register_callback(dev, KEY_CANCEL, CONSOLE->last_menu_item, NULL);
  console_register_callback(dev, KEY_UP, console_liste_accept_oos_1, sens_as_voidp);

  return NULL;
}

static CALLBACK void* console_liste_accept_oos_3(ProtDevice *dev, int press, void *sens_as_voidp)
{
  console_send(dev, "ATR1\r");
  console_send(dev, str_8_9);
  console_send(dev, "ATR0\r");

  console_register_callback(dev, KEY_ENTER, console_liste_sens_set_oos, sens_as_voidp);
  console_register_callback(dev, KEY_CANCEL, CONSOLE->last_menu_item, NULL);

  return NULL;
}

static void console_liste_show_sensor(ProtDevice *dev, int idx, int bitmask)
{
  time_t t;
  char cmd[40], *sens_num, *sens_desc, *sens_date;
  
  console_send(dev, "ATS03\r");
  
  string_sensor_name(idx, &sens_num, &sens_desc);
  
  sprintf(cmd, str_8_50, sens_num);
  console_send(dev, cmd);
  sprintf(cmd, str_8_51, sens_desc);
  console_send(dev, cmd);
  
  t = 0;
  switch(bitmask)
  {
    case bitAlarm:
      t = SE_time_alarm[idx];
      break;
    case bitFailure:
      t = SE_time_failure[idx];
      break;
    case bitSabotage:
      t = SE_time_sabotage[idx];
      break;
  }
  
  sens_date = console_format_time(dev, t);
  sprintf(cmd, str_8_52, sens_date);
  console_send(dev, cmd);
  free(sens_date);
  
  if(SE[idx] & bitmask)
    console_liste_accept_oos_3(dev, 0, (void*)idx);
  else
    console_liste_accept_oos_1(dev, 0, (void*)idx);
}

static void console_liste_act_list(ProtDevice *dev, int bitMask, ConsoleCallback func)
{
  char *act_num, *act_desc;
  int i;
  
  console_list_free(CONSOLE->support_list);
  CONSOLE->support_list = NULL;
  
  for(i=0; i<n_AT; i++)
    if(AT[i] & bitMask)
    {
      string_actuator_name(i, &act_num, &act_desc);
      CONSOLE->support_list = console_list_add(CONSOLE->support_list, act_num, act_desc, func, i);
    }
    
  console_list_show(dev, CONSOLE->support_list, 0, 3, 0);
}

static CALLBACK void* console_liste_sens_alm1(ProtDevice *dev, int press, void *sens_as_voidp)
{
  console_liste_show_sensor(dev, (int)sens_as_voidp, bitAlarm);
  return NULL;
}

CALLBACK void* console_liste_sens_alm(ProtDevice *dev, int press, void *null)
{
  console_disable_menu(dev, str_8_14);
  CONSOLE->last_menu_item = console_liste_sens_alm;
  CONSOLE->liste_accept_func = console_liste_sens_accept_alarm;
  console_liste_sens_list(dev, bitAlarm, console_liste_sens_alm1);
  return NULL;
}

CALLBACK void* console_liste_sens_almMU(ProtDevice *dev, int press, void *null)
{
  console_disable_menu(dev, str_8_15);
  CONSOLE->last_menu_item = console_liste_sens_almMU;
  CONSOLE->liste_accept_func = console_liste_sens_accept_alarm;
  console_liste_sens_list(dev, bitMUAlarm, console_liste_sens_alm1);
  return NULL;
}

static CALLBACK void* console_liste_sens_set_oos_off(ProtDevice *dev, int press, void *sens_as_voidp)
{
  cmd_sensor_on((int)sens_as_voidp);
  
  console_send(dev, "ATS00\r");
  console_send(dev, str_8_6);
  console_send(dev, str_8_17);

  timeout_on(CONSOLE->timeout, (timeout_func)console_return_to_last_menu_item, dev, 0, TIMEOUT_MSG);
  return NULL;
}

static CALLBACK void* console_liste_sens_oos_off(ProtDevice *dev, int press, void *sens_as_voidp)
{
  char cmd[32], *sens_num, *sens_desc;
  
  console_send(dev, "ATS03\r");
  string_sensor_name((int)sens_as_voidp, &sens_num, &sens_desc);
  console_send(dev, str_8_18);
  sprintf(cmd, str_8_53, sens_num);
  console_send(dev, cmd);
  sprintf(cmd, str_8_54, sens_desc);
  console_send(dev, cmd);
  
  console_register_callback(dev, KEY_ENTER, console_liste_sens_set_oos_off, sens_as_voidp);
  console_register_callback(dev, KEY_CANCEL, console_liste_sens_oos, NULL);
  
  return NULL;
}

CALLBACK void* console_liste_sens_oos(ProtDevice *dev, int press, void *null)
{
  console_disable_menu(dev, str_8_19);
  CONSOLE->last_menu_item = console_liste_sens_oos;
  console_liste_sens_list(dev, bitOOS, console_liste_sens_oos_off);
  return NULL;
}

static CALLBACK void* console_liste_sens_failure1(ProtDevice *dev, int press, void *sens_as_voidp)
{
  console_liste_show_sensor(dev, (int)sens_as_voidp, bitFailure);
  return NULL;
}

CALLBACK void* console_liste_sens_failure(ProtDevice *dev, int press, void *null)
{
  console_disable_menu(dev, str_8_20);
  CONSOLE->last_menu_item = console_liste_sens_failure;
  CONSOLE->liste_accept_func = console_liste_sens_accept_failure;
  console_liste_sens_list(dev, bitFailure | bitMUFailure, console_liste_sens_failure1);
  return NULL;
}

static CALLBACK void* console_liste_sens_sabotage1(ProtDevice *dev, int press, void *sens_as_voidp)
{
  console_liste_show_sensor(dev, (int)sens_as_voidp, bitSabotage);
  return NULL;
}

CALLBACK void* console_liste_sens_sabotage(ProtDevice *dev, int press, void *null)
{
  console_disable_menu(dev, str_8_21);
  CONSOLE->last_menu_item = console_liste_sens_sabotage;
  CONSOLE->liste_accept_func = console_liste_sens_accept_sabotage;
  console_liste_sens_list(dev, bitSabotage | bitMUSabotage, console_liste_sens_sabotage1);
  return NULL;
}

static CALLBACK void* console_liste_zone_active2(ProtDevice *dev, int press, void *zone_as_voidp)
{
  int zone = (int)zone_as_voidp;
  
  cmd_zone_off(zone, 0);
  
  console_send(dev, "ATS00\r");
  console_send(dev, str_8_22);
  console_send(dev, str_8_23);
  
  CONSOLE->last_menu_item = console_liste_zone_active;
  timeout_on(CONSOLE->timeout, (timeout_func)console_return_to_last_menu_item, dev, 0, TIMEOUT_MSG);

  return NULL;
}

static CALLBACK void* console_liste_zone_active1(ProtDevice *dev, int press, void *zone_as_voidp)
{
  int zone = (int)zone_as_voidp;
  char *zone_cod, *zone_desc, cmd[32];
  int pos;

  console_send(dev, "ATS03\r");
  
  string_zone_name(zone, &zone_cod, &zone_desc);
  sprintf(cmd, str_8_24, zone_cod);
  console_send(dev, cmd);
  pos = (16 - strlen(zone_desc)) / 2;
  sprintf(cmd, str_8_55, pos, zone_desc);
  console_send(dev, cmd);
  console_send(dev, str_8_25);
  
  console_register_callback(dev, KEY_ENTER, console_liste_zone_active2, zone_as_voidp);
  console_register_callback(dev, KEY_CANCEL, console_liste_zone_active, NULL);
  
  return NULL;
}

CALLBACK void* console_liste_zone_active(ProtDevice *dev, int press, void *null)
{
  char zone_to_show[1 + n_ZS + n_ZI];
  char *zone_cod, *zone_desc, str[18];
  int i;
  
  console_disable_menu(dev, str_8_26);

  console_list_free(CONSOLE->support_list);
  CONSOLE->support_list = NULL;
  
  memset(zone_to_show, 0, 1 + n_ZS + n_ZI);
  for(i=0; i<n_SE; i++)
    if((Zona_SE[i] < (1 + n_ZS + n_ZI)) && (ZONA[Zona_SE[i]] & bitActive)) zone_to_show[Zona_SE[i]] = 1;
  if(TPDZT && (TPDZT[0]!=0xff) && (ZONA[0] & bitActive)) zone_to_show[0] = 1;
  for(i=0; i<n_ZI; i++)
//    if(TPDZI[i] && (TPDZI[i][0]!=0xff) && (ZI[i] & bitActive)) zone_to_show[n_ZS+1+i] = 1;
  {
    string_zone_name(n_ZS+1+i, &zone_cod, &zone_desc);
    if(((TPDZI[i] && (TPDZI[i][0]!=0xff)) || (zone_desc != StringDescNull)) && (ZI[i] & bitActive)) zone_to_show[n_ZS+1+i] = 1;
  }
  
  for(i=0; i<(1 + n_ZS + n_ZI); i++)
    if(zone_to_show[i])
    {
      string_zone_name(i, &zone_cod, &zone_desc);
      sprintf(str, str_8_27, zone_cod);
      CONSOLE->support_list = console_list_add(CONSOLE->support_list, str, zone_desc, console_liste_zone_active1, i);
    }

  console_list_show(dev, CONSOLE->support_list, 0, 3, 0);

  return NULL;
}

static CALLBACK void* console_liste_zone_not_active2(ProtDevice *dev, int press, void *zone_as_voidp)
{
  int zone = (int)zone_as_voidp;
  
  cmd_zone_on(zone, 0);
  cmd_zone_refresh(1);
  cmd_actuator_refresh();
  
  console_send(dev, "ATS00\r");
  console_send(dev, str_8_22);
  console_send(dev, str_8_29);
  
  CONSOLE->last_menu_item = console_liste_zone_not_active;
  timeout_on(CONSOLE->timeout, (timeout_func)console_return_to_last_menu_item, dev, 0, TIMEOUT_MSG);

  return NULL;
}

static CALLBACK void* console_liste_zone_not_active1(ProtDevice *dev, int press, void *zone_as_voidp)
{
  int zone = (int)zone_as_voidp;
  char *zone_cod, *zone_desc, cmd[32];
  int pos;

  console_send(dev, "ATS03\r");
  
  string_zone_name(zone, &zone_cod, &zone_desc);
  sprintf(cmd, str_8_30, zone_cod);
  console_send(dev, cmd);
  pos = (16 - strlen(zone_desc)) / 2;
  sprintf(cmd, str_8_55, pos, zone_desc);
  console_send(dev, cmd);
  console_send(dev, str_8_31);
  
  console_register_callback(dev, KEY_ENTER, console_liste_zone_not_active2, zone_as_voidp);
  console_register_callback(dev, KEY_CANCEL, console_liste_zone_not_active, NULL);
  
  return NULL;
}

CALLBACK void* console_liste_zone_not_active(ProtDevice *dev, int press, void *null)
{
  char zone_to_show[1 + n_ZS + n_ZI];
  char *zone_cod, *zone_desc, str[18];
  int i;
  
  console_disable_menu(dev, str_8_32);

  console_list_free(CONSOLE->support_list);
  CONSOLE->support_list = NULL;
  
  memset(zone_to_show, 0, 1 + n_ZS + n_ZI);
  for(i=0; i<n_SE; i++)
    if((Zona_SE[i] < (1 + n_ZS + n_ZI)) && !(ZONA[Zona_SE[i]] & bitActive)) zone_to_show[Zona_SE[i]] = 1;
  if(TPDZT && (TPDZT[0]!=0xff) && !(ZONA[0] & bitActive)) zone_to_show[0] = 1;
  for(i=0; i<n_ZI; i++)
//    if(TPDZI[i] && (TPDZI[i][0]!=0xff) && !(ZI[i] & bitActive)) zone_to_show[n_ZS+1+i] = 1;
  {
    string_zone_name(n_ZS+1+i, &zone_cod, &zone_desc);
    if(((TPDZI[i] && (TPDZI[i][0]!=0xff)) || (zone_desc != StringDescNull)) && !(ZI[i] & bitActive)) zone_to_show[n_ZS+1+i] = 1;
  }
  
  for(i=0; i<(1 + n_ZS + n_ZI); i++)
    if(zone_to_show[i])
    {
      string_zone_name(i, &zone_cod, &zone_desc);
      sprintf(str, str_8_33, zone_cod);
      CONSOLE->support_list = console_list_add(CONSOLE->support_list, str, zone_desc, console_liste_zone_not_active1, i);
    }

  console_list_show(dev, CONSOLE->support_list, 0, 3, 0);

  return NULL;
}

static CALLBACK void* console_liste_act_set_on_off(ProtDevice *dev, int press, void *act_as_voidp)
{
  AT[(int)act_as_voidp] &= ~bitON;
  AT[(int)act_as_voidp] |= bitHLF | bitAbilReq;
  
  console_send(dev, "ATS00\r");
  console_send(dev, str_8_34);
  console_send(dev, str_8_35);

  CONSOLE->last_menu_item = console_liste_act_on;
  timeout_on(CONSOLE->timeout, (timeout_func)console_return_to_last_menu_item, dev, 0, TIMEOUT_MSG);
  return NULL;
}

static CALLBACK void* console_liste_act_on_off(ProtDevice *dev, int press, void *act_as_voidp)
{
  char cmd[32], *act_num, *act_desc;
  
  console_send(dev, "ATS03\r");
  
  string_actuator_name((int)act_as_voidp, &act_num, &act_desc);
  console_send(dev, str_8_36);
  sprintf(cmd, str_8_56, act_num);
  console_send(dev, cmd);
  sprintf(cmd, str_8_54, act_desc);
  console_send(dev, cmd);
  
  console_register_callback(dev, KEY_ENTER, console_liste_act_set_on_off, act_as_voidp);
  console_register_callback(dev, KEY_CANCEL, console_liste_act_on, NULL);
  
  return NULL;
}

CALLBACK void* console_liste_act_on(ProtDevice *dev, int press, void *null)
{
  console_disable_menu(dev, str_8_38);
  console_liste_act_list(dev, bitON, console_liste_act_on_off);
  return NULL;
}

static CALLBACK void* console_liste_act_set_oos_off(ProtDevice *dev, int press, void *act_as_voidp)
{
  cmd_actuator_on((int)act_as_voidp);
  
  console_send(dev, "ATS00\r");
  console_send(dev, str_8_34);
  console_send(dev, str_8_40);

  CONSOLE->last_menu_item = console_liste_act_oos;
  timeout_on(CONSOLE->timeout, (timeout_func)console_return_to_last_menu_item, dev, 0, TIMEOUT_MSG);
  return NULL;
}

static CALLBACK void* console_liste_act_oos_off(ProtDevice *dev, int press, void *act_as_voidp)
{
  char cmd[32], *act_num, *act_desc;
  
  console_send(dev, "ATS03\r");
  
  string_actuator_name((int)act_as_voidp, &act_num, &act_desc);
  console_send(dev, str_8_41);
  sprintf(cmd, str_8_56, act_num);
  console_send(dev, cmd);
  sprintf(cmd, str_8_54, act_desc);
  console_send(dev, cmd);
  
  console_register_callback(dev, KEY_ENTER, console_liste_act_set_oos_off, act_as_voidp);
  console_register_callback(dev, KEY_CANCEL, console_liste_act_oos, NULL);
  
  return NULL;
}

CALLBACK void* console_liste_act_oos(ProtDevice *dev, int press, void *null)
{
  console_disable_menu(dev, str_8_42);
  console_liste_act_list(dev, bitOOS, console_liste_act_oos_off);
  return NULL;
}

static char **console_daytype[3] = {&str_8_43, &str_8_44, &str_8_45};
static int console_selected_day = 0;

static CALLBACK void* console_liste_holidays3(ProtDevice *dev, int press, void *daytype_as_voidp)
{
  cmd_set_day_type(console_selected_day, (int)daytype_as_voidp);
  
  console_send(dev, "ATS00\r");
  console_send(dev, str_8_46);
  console_send(dev, str_8_47);
  
  CONSOLE->last_menu_item = console_liste_holidays;
  timeout_on(CONSOLE->timeout, (timeout_func)console_return_to_last_menu_item, dev, 0, TIMEOUT_MSG);

  return NULL;
}

static CALLBACK void* console_liste_holidays2(ProtDevice *dev, int press, void *daytype_as_voidp)
{
  int i;
  char cmd[32];

  for(i=0; i<3; i++)
  {
    if((int)daytype_as_voidp == i) console_send(dev, "ATR1\r");
    sprintf(cmd, str_8_57, i + 4, *console_daytype[i]);
    console_send(dev, cmd);
    if((int)daytype_as_voidp == i) console_send(dev, "ATR0\r");
  }

  if((int)daytype_as_voidp)
    console_register_callback(dev, KEY_UP, console_liste_holidays2, daytype_as_voidp-1);
  if((int)daytype_as_voidp != 2)
    console_register_callback(dev, KEY_DOWN, console_liste_holidays2, daytype_as_voidp+1);
  console_register_callback(dev, KEY_ENTER, console_liste_holidays3, daytype_as_voidp);
  console_register_callback(dev, KEY_CANCEL, console_liste_holidays, NULL);
  
  return NULL;
}

static CALLBACK void* console_liste_holidays1(ProtDevice *dev, int press, void *day_as_voidp)
{
  char *date, cmd[32];
  time_t t;
  
  console_selected_day = (int)day_as_voidp;
  
  t = time(NULL) + (((int)day_as_voidp + 1) * 86400);
  date = console_format_date(dev, t);
  
  console_send(dev, "ATS03\r");
  sprintf(cmd, str_8_51, date);
  console_send(dev, cmd);
  free(date);
  
  console_liste_holidays2(dev, 0, (void*)((int)FESTIVI[console_selected_day]));
  
  return NULL;
}

CALLBACK void* console_liste_holidays(ProtDevice *dev, int press, void *null)
{
  int i;
  time_t t;
  char *date, daydescr[32];
  
  console_disable_menu(dev, str_8_48);

  console_list_free(CONSOLE->support_list);
  CONSOLE->support_list = NULL;
  
  t = time(NULL);

  date = console_format_date(dev, t);
  sprintf(daydescr, "%s %s", date, *console_daytype[TIPO_GIORNO]);
  CONSOLE->support_list = console_list_add(CONSOLE->support_list, daydescr, NULL, NULL, 0);
  free(date);
  
  for(i=0; i<n_FESTIVI;i++)
  {
    t += 86400;
    date = console_format_date(dev, t);
    sprintf(daydescr, "%s %s", date, *console_daytype[FESTIVI[i]]);
//    if(CONSOLE->current_user->level >= 5)
      CONSOLE->support_list = console_list_add(CONSOLE->support_list, daydescr, NULL, console_liste_holidays1, i);
//    else
//      CONSOLE->support_list = console_list_add(CONSOLE->support_list, daydescr, NULL, NULL, 0);
    free(date);
  }

  console_list_show(dev, CONSOLE->support_list, 0, 6, 0);
  
  return NULL;
}

CALLBACK void* console_liste_called_numbers(ProtDevice *dev, int press, void *null)
{
  int i, j;
  char *date;
  
  console_disable_menu(dev, str_8_49);

  console_list_free(CONSOLE->support_list);
  CONSOLE->support_list = NULL;
  
  j = support_called_number_idx - 1;
  if(j < 0) j = CALLED_NUMBERS;
  
  for(i=0; (i<CALLED_NUMBERS) && (support_called_numbers[j].number); i++)
  {
    date = console_format_date(dev, support_called_numbers[j].time);
    CONSOLE->support_list = console_list_add(CONSOLE->support_list, date, support_called_numbers[j].number, NULL, 0);
    free(date);
    j--;
    if(j < 0) j = CALLED_NUMBERS;
  }
  console_list_show(dev, CONSOLE->support_list, 0, 3, 0);
  
  return NULL;
}



