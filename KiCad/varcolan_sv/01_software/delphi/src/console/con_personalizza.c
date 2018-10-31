#include "console.h"
#include "con_personalizza.h"
#include "../database.h"
#include "../strings.h"
#include "../master.h"
#include "../support.h"
#include "../delphi.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/***********************************************************************
	PERSONALIZZA menu functions
***********************************************************************/

static CALLBACK void* console_personalizza_message(ProtDevice *dev, int descr_as_int, void *idx_as_voidp)
{
  int idx = (int)idx_as_voidp;
  char *descr = (char*)descr_as_int;
  
  if(strlen(descr) > 16) descr[16] = 0;
  
  switch(CONSOLE->personalizza_string)
  {
    case PersZone:
      free(ZoneName[idx]);
      ZoneName[idx] = strdup(descr);
      string_save();
      break;
    case PersSensor:
      free(SensorName[idx]);
      SensorName[idx] = strdup(descr);
      string_save();
      break;
    case PersActuator:
      free(ActuatorName[idx]);
      ActuatorName[idx] = strdup(descr);
      string_save();
      break;
    case PersPhonebook:
      free(config.PhoneBook[idx].Name);
      config.PhoneBook[idx].Name = strdup(descr);
      delphi_save_conf();
      break;
    case PersCommand:
      free(CommandName[idx]);
      CommandName[idx] = strdup(descr);
      string_save();
      break;
    case PersCode:
      free(CodeName[idx]);
      CodeName[idx] = strdup(descr);
      string_save();
      break;
    case PersProfile:
      free(ProfileName[idx]);
      ProfileName[idx] = strdup(descr);
      string_save();
      break;
    case PersArea:
      free(AreaName[idx]);
      AreaName[idx] = strdup(descr);
      string_save();
      break;
    case PersHoliday:
      free(HolidayName[idx]);
      HolidayName[idx] = strdup(descr);
      string_save();
      break;
    case PersWeekTiming:
      free(WeekTimingName[idx]);
      WeekTimingName[idx] = strdup(descr);
      string_save();
      break;
    case PersTerminal:
      free(TerminalName[idx]);
      TerminalName[idx] = strdup(descr);
      string_save();
      break;
    default:
      break;
  }
  
  console_send(dev, "ATS00\r");
  console_send(dev, str_10_0);
  console_send(dev, str_10_1);
  
  timeout_on(CONSOLE->timeout, (timeout_func)console_return_to_last_menu_item, dev, 0, TIMEOUT_MSG);
  
  return NULL;
}

static void console_personalizza_common(ProtDevice *dev, int id, StringFunc string_name, ConsoleCallback cancel)
{
  char *id_cod, *id_desc, cmd[32];

  console_send(dev, "ATS03\r");
  
  string_name(id, &id_cod, &id_desc);
  
  sprintf(cmd, str_10_16, id_cod);
  console_send(dev, cmd);
  console_send(dev, str_10_17);
  console_send(dev, str_10_2);
  console_send(dev, str_10_18);
  if(!strcmp(id_desc, StringDescNull)) id_desc = "";
  sprintf(cmd, "ATL211%s\r", id_desc);
  console_send(dev, cmd);

  console_register_callback(dev, KEY_STRING, console_personalizza_message, (void*)id);
  console_register_callback(dev, KEY_CANCEL, cancel, NULL);
}

static CALLBACK void* console_personalizza_zone1(ProtDevice *dev, int press, void *zone_as_voidp)
{
  CONSOLE->personalizza_string = PersZone;
  console_personalizza_common(dev, (int)zone_as_voidp, string_zone_name, console_personalizza_zone);
  
  return NULL;
}

CALLBACK void* console_personalizza_zone(ProtDevice *dev, int press, void *null)
{
  char zone_to_show[1 + n_ZS + n_ZI];
  char *zone_cod, *zone_desc, str[18];
  int i;
  
  console_disable_menu(dev, str_10_3);
  
  console_list_free(CONSOLE->support_list);
  CONSOLE->support_list = NULL;
  CONSOLE->last_menu_item = console_personalizza_zone;

  memset(zone_to_show, 0, 1 + n_ZS + n_ZI);
  for(i=0; i<n_SE; i++)
    if(Zona_SE[i] < (1 + n_ZS + n_ZI)) zone_to_show[Zona_SE[i]] = 1;
  if(TPDZT && (TPDZT[0]!=0xff)) zone_to_show[0] = 1;
  for(i=0; i<n_ZI; i++)
  {
    string_zone_name(n_ZS+1+i, &zone_cod, &zone_desc);
    if((TPDZI[i] && (TPDZI[i][0]!=0xff)) || (zone_desc != StringDescNull)) zone_to_show[n_ZS+1+i] = 1;
  }
  
  for(i=0; i<(1 + n_ZS + n_ZI); i++)
    if(zone_to_show[i])
    {
      string_zone_name(i, &zone_cod, &zone_desc);
      sprintf(str, str_10_4, zone_cod);
      CONSOLE->support_list = console_list_add(CONSOLE->support_list, str, zone_desc, console_personalizza_zone1, i);
    }

  console_list_show(dev, CONSOLE->support_list, 0, 3, 0);
  
  return NULL;
}

static CALLBACK void* console_personalizza_sensor1(ProtDevice *dev, int press, void *sens_as_voidp)
{
  CONSOLE->personalizza_string = PersSensor;
  console_personalizza_common(dev, (int)sens_as_voidp, string_sensor_name, console_personalizza_sensor);
  
  return NULL;
}

CALLBACK void* console_personalizza_sensor(ProtDevice *dev, int press, void *null)
{
  char *sens_cod, *sens_desc;
  int i, j, num;
  
  console_disable_menu(dev, str_10_5);
  
  console_list_free(CONSOLE->support_list);
  CONSOLE->support_list = NULL;
  CONSOLE->last_menu_item = console_personalizza_sensor;

  for(i=0; i<n_SE;)
  {
    if(master_periph_present(i>>3))
    {
      num = master_sensor_valid(i);
      for(j=0; j<num; j++)
      {
        string_sensor_name(i, &sens_cod, &sens_desc);
        CONSOLE->support_list = console_list_add(CONSOLE->support_list, sens_cod, sens_desc,
            console_personalizza_sensor1, i++);
      }
      i += 8 - num;
    }
    else
      i += 8;
  }

  console_list_show(dev, CONSOLE->support_list, 0, 3, 0);
  
  return NULL;
}

static CALLBACK void* console_personalizza_actuator1(ProtDevice *dev, int press, void *act_as_voidp)
{
  CONSOLE->personalizza_string = PersActuator;
  console_personalizza_common(dev, (int)act_as_voidp, string_actuator_name, console_personalizza_actuator);
  
  return NULL;
}

CALLBACK void* console_personalizza_actuator(ProtDevice *dev, int press, void *null)
{
  char *act_cod, *act_desc;
  int i, j, num;
  
  console_disable_menu(dev, str_10_6);
  
  console_list_free(CONSOLE->support_list);
  CONSOLE->support_list = NULL;
  CONSOLE->last_menu_item = console_personalizza_actuator;

  for(i=0; i<n_AT;)
  {
    if(master_periph_present(i>>3))
    {
      num = master_sensor_valid(i);
      for(j=0; j<num; j++)
      {
        string_actuator_name(i, &act_cod, &act_desc);
        CONSOLE->support_list = console_list_add(CONSOLE->support_list, act_cod, act_desc,
            console_personalizza_actuator1, i++);
      }
      i += 8 - num;
    }
    else
      i += 8;
  }

  console_list_show(dev, CONSOLE->support_list, 0, 3, 0);
  
  return NULL;
}

static CALLBACK void* console_personalizza_command1(ProtDevice *dev, int press, void *tcmd_as_voidp)
{
  CONSOLE->personalizza_string = PersCommand;
  console_personalizza_common(dev, (int)tcmd_as_voidp, string_command_name, console_personalizza_command);
  
  return NULL;
}

CALLBACK void* console_personalizza_command(ProtDevice *dev, int press, void *null)
{
  char *tcmd_cod, *tcmd_desc;
  int i;

  console_disable_menu(dev, str_10_7);
  
  console_list_free(CONSOLE->support_list);
  CONSOLE->support_list = NULL;
  CONSOLE->last_menu_item = console_personalizza_command;

  for(i=0; i<n_ME; i++)
  {
    string_command_name(i, &tcmd_cod, &tcmd_desc);
    CONSOLE->support_list = console_list_add(CONSOLE->support_list, tcmd_cod, tcmd_desc,
            console_personalizza_command1, i);
  }

  console_list_show(dev, CONSOLE->support_list, 0, 3, 0);
  
  return NULL;
}

static CALLBACK void* console_personalizza_phonebook1(ProtDevice *dev, int press, void *phidx_as_voidp)
{
  int phidx = (int)phidx_as_voidp;
  char *phidx_desc, cmd[32];
  
  console_send(dev, "ATS03\r");
  sprintf(cmd, str_10_16, config.PhoneBook[phidx].Phone);
  console_send(dev, cmd);
  console_send(dev, str_10_17);
  console_send(dev, str_10_18);
  phidx_desc = config.PhoneBook[phidx].Name;
  if(phidx_desc == StringDescNull) phidx_desc = "";
  sprintf(cmd, "ATL211%s\r", phidx_desc);
  console_send(dev, cmd);
  
  CONSOLE->personalizza_string = PersPhonebook;
  console_register_callback(dev, KEY_STRING, console_personalizza_message, phidx_as_voidp);
  console_register_callback(dev, KEY_CANCEL, console_personalizza_phonebook, NULL);
  
  return NULL;
}

CALLBACK void* console_personalizza_phonebook(ProtDevice *dev, int press, void *null)
{
  char *phone_desc;
  int i;
  
  console_disable_menu(dev, str_10_9);
  
  console_list_free(CONSOLE->support_list);
  CONSOLE->support_list = NULL;
  CONSOLE->last_menu_item = console_personalizza_phonebook;

  for(i=0; i<PBOOK_DIM; i++)
  {
    if(config.PhoneBook[i].Phone[0])
    {
      phone_desc = config.PhoneBook[i].Name;
      if(!phone_desc) phone_desc = StringDescNull;
      CONSOLE->support_list = console_list_add(CONSOLE->support_list, config.PhoneBook[i].Phone, phone_desc,
            console_personalizza_phonebook1, i);
    }
  }

  console_list_show(dev, CONSOLE->support_list, 0, 3, 0);
  
  return NULL;
}

static CALLBACK void* console_personalizza_code1(ProtDevice *dev, int press, void *code_as_voidp)
{
  CONSOLE->personalizza_string = PersCode;
  console_personalizza_common(dev, (int)code_as_voidp, string_code_name, console_personalizza_code);
  
  return NULL;
}

CALLBACK void* console_personalizza_code(ProtDevice *dev, int press, void *null)
{
  char *code_cod, *code_desc;
  int i;

  console_disable_menu(dev, str_10_10);
  
  console_list_free(CONSOLE->support_list);
  CONSOLE->support_list = NULL;
  CONSOLE->last_menu_item = console_personalizza_code;

  for(i=0; i<(sizeof(RONDA)/sizeof(short)); i++)
  {
    string_code_name(i, &code_cod, &code_desc);
    CONSOLE->support_list = console_list_add(CONSOLE->support_list, code_cod, code_desc,
            console_personalizza_code1, i);
  }

  console_list_show(dev, CONSOLE->support_list, 0, 3, 0);
  
  return NULL;
}

static CALLBACK void* console_personalizza_profile1(ProtDevice *dev, int press, void *prof_as_voidp)
{
  CONSOLE->personalizza_string = PersProfile;
  console_personalizza_common(dev, (int)prof_as_voidp, string_profile_name, console_personalizza_profile);
  
  return NULL;
}

CALLBACK void* console_personalizza_profile(ProtDevice *dev, int press, void *null)
{
  char *prof_cod, *prof_desc;
  int i;

  console_disable_menu(dev, str_10_11);
  
  console_list_free(CONSOLE->support_list);
  CONSOLE->support_list = NULL;
  CONSOLE->last_menu_item = console_personalizza_profile;

  for(i=0; i<256; i++)
  {
    string_profile_name(i, &prof_cod, &prof_desc);
    CONSOLE->support_list = console_list_add(CONSOLE->support_list, prof_cod, prof_desc,
            console_personalizza_profile1, i);
  }

  console_list_show(dev, CONSOLE->support_list, 0, 3, 0);
  
  return NULL;
}

static CALLBACK void* console_personalizza_area1(ProtDevice *dev, int press, void *area_as_voidp)
{
  CONSOLE->personalizza_string = PersArea;
  console_personalizza_common(dev, (int)area_as_voidp, string_area_name, console_personalizza_area);
  
  return NULL;
}

CALLBACK void* console_personalizza_area(ProtDevice *dev, int press, void *null)
{
  char *area_cod, *area_desc;
  int i;

  console_disable_menu(dev, str_10_12);
  
  console_list_free(CONSOLE->support_list);
  CONSOLE->support_list = NULL;
  CONSOLE->last_menu_item = console_personalizza_area;

  for(i=0; i<128; i++)
  {
    string_area_name(i, &area_cod, &area_desc);
    CONSOLE->support_list = console_list_add(CONSOLE->support_list, area_cod, area_desc,
            console_personalizza_area1, i);
  }

  console_list_show(dev, CONSOLE->support_list, 0, 3, 0);
  
  return NULL;
}

static CALLBACK void* console_personalizza_holiday1(ProtDevice *dev, int press, void *hday_as_voidp)
{
  CONSOLE->personalizza_string = PersHoliday;
  console_personalizza_common(dev, (int)hday_as_voidp-1, string_holiday_name, console_personalizza_holiday);
  
  return NULL;
}

CALLBACK void* console_personalizza_holiday(ProtDevice *dev, int press, void *null)
{
  char *hday_cod, *hday_desc;
  int i;

  console_disable_menu(dev, str_10_13);
  
  console_list_free(CONSOLE->support_list);
  CONSOLE->support_list = NULL;
  CONSOLE->last_menu_item = console_personalizza_holiday;

  for(i=0; i<32; i++)
  {
    string_holiday_name(i, &hday_cod, &hday_desc);
    CONSOLE->support_list = console_list_add(CONSOLE->support_list, hday_cod, hday_desc,
            console_personalizza_holiday1, i+1);
  }

  console_list_show(dev, CONSOLE->support_list, 0, 3, 0);
  
  return NULL;
}

CALLBACK void* console_personalizza_weektiming1(ProtDevice *dev, int press, void *wtmg_as_voidp)
{
  CONSOLE->personalizza_string = PersWeekTiming;
  console_personalizza_common(dev, (int)wtmg_as_voidp-1, string_weektiming_name, console_personalizza_weektiming);
  
  return NULL;
}

CALLBACK void* console_personalizza_weektiming(ProtDevice *dev, int press, void *null)
{
  char *wtmg_cod, *wtmg_desc;
  int i;

  console_disable_menu(dev, str_10_14);
  
  console_list_free(CONSOLE->support_list);
  CONSOLE->support_list = NULL;
  CONSOLE->last_menu_item = console_personalizza_weektiming;

  for(i=0; i<64; i++)
  {
    string_weektiming_name(i, &wtmg_cod, &wtmg_desc);
    CONSOLE->support_list = console_list_add(CONSOLE->support_list, wtmg_cod, wtmg_desc,
            console_personalizza_weektiming1, i+1);
  }

  console_list_show(dev, CONSOLE->support_list, 0, 3, 0);
  
  return NULL;
}

CALLBACK void* console_personalizza_terminal1(ProtDevice *dev, int press, void *term_as_voidp)
{
  CONSOLE->personalizza_string = PersTerminal;
  console_personalizza_common(dev, (int)term_as_voidp, string_terminal_name, console_personalizza_terminal);
  
  return NULL;
}

CALLBACK void* console_personalizza_terminal(ProtDevice *dev, int press, void *null)
{
  char *term_cod, *term_desc;
  int i;

  console_disable_menu(dev, str_10_15);
  
  console_list_free(CONSOLE->support_list);
  CONSOLE->support_list = NULL;
  CONSOLE->last_menu_item = console_personalizza_terminal;

  for(i=0; i<64; i++)
  {
    string_terminal_name(i, &term_cod, &term_desc);
    CONSOLE->support_list = console_list_add(CONSOLE->support_list, term_cod, term_desc,
            console_personalizza_terminal1, i);
  }

  console_list_show(dev, CONSOLE->support_list, 0, 3, 0);
  
  return NULL;
}

