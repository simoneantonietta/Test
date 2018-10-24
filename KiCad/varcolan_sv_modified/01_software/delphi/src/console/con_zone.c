#include "console.h"
#include "con_zone.h"
#include "../database.h"
#include "../command.h"
#include "../strings.h"
#include <stdio.h>
#include <string.h>

/***********************************************************************
	ZONE menu functions
***********************************************************************/

static CALLBACK void* console_zone_deactivate2(ProtDevice *dev, int press, void *zone_as_voidp)
{
  char cmd[16];
  
//  cmd_zone_off((int)zone_as_voidp, 0);
  sprintf(cmd, "J%03d", (int)zone_as_voidp);
  codec_parse_cmd(cmd, 4, dev);
  
  console_send(dev, "ATS00\r");
  console_send(dev, str_17_0);
  console_send(dev, str_17_1);
  
  CONSOLE->last_menu_item = console_zone_deactivate;
  timeout_on(CONSOLE->timeout, (timeout_func)console_return_to_last_menu_item, dev, 0, TIMEOUT_MSG);

  return NULL;
}

static CALLBACK void* console_zone_deactivate1(ProtDevice *dev, int press, void *zone_as_voidp)
{
  int zone = (int)zone_as_voidp;
  char *zone_cod, *zone_desc, cmd[32];
  int pos;

  console_send(dev, "ATS03\r");
  
  string_zone_name(zone, &zone_cod, &zone_desc);
  sprintf(cmd, str_17_2, zone_cod);
  console_send(dev, cmd);
  if(CONSOLE->tipo == CONSOLE_STL01)
  {
    pos = (16 - strlen(zone_desc)) / 2;
    sprintf(cmd, "ATN%02d05%s\r", pos+1, zone_desc);
  }
  else
    sprintf(cmd, "ATN064051%s\r", zone_desc);
  console_send(dev, cmd);
  console_send(dev, str_17_3);
  
  console_register_callback(dev, KEY_ENTER, console_zone_deactivate2, zone_as_voidp);
  console_register_callback(dev, KEY_CANCEL, console_zone_deactivate, NULL);
  
  return NULL;
}

CALLBACK void* console_zone_deactivate(ProtDevice *dev, int press, void *null)
{
  char zone_to_show[1 + n_ZS + n_ZI];
  char *zone_cod, *zone_desc, str[18];
  int i;
  
  console_disable_menu(dev, str_17_4);

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
      sprintf(str, str_17_5, zone_cod);
      CONSOLE->support_list = console_list_add(CONSOLE->support_list, str, zone_desc, console_zone_deactivate1, i);
    }

  console_list_show(dev, CONSOLE->support_list, 0, 3, 0);

  return NULL;
}

static CALLBACK void* console_zone_activate2(ProtDevice *dev, int press, void *zone_as_voidp)
{
  char cmd[16];
  
//  cmd_zone_on((int)zone_as_voidp, 0);
  sprintf(cmd, "I%03d", (int)zone_as_voidp);
  codec_parse_cmd(cmd, 4, dev);
  
  console_send(dev, "ATS00\r");
  console_send(dev, str_17_6);
  console_send(dev, str_17_7);
  
  CONSOLE->last_menu_item = console_zone_activate;
  timeout_on(CONSOLE->timeout, (timeout_func)console_return_to_last_menu_item, dev, 0, TIMEOUT_MSG);

  return NULL;
}

static CALLBACK void* console_zone_activate1(ProtDevice *dev, int press, void *zone_as_voidp)
{
  int zone = (int)zone_as_voidp;
  char *zone_cod, *zone_desc, cmd[32];
  int pos;

  console_send(dev, "ATS03\r");
  
  string_zone_name(zone, &zone_cod, &zone_desc);
  sprintf(cmd, str_17_8, zone_cod);
  console_send(dev, cmd);
  if(CONSOLE->tipo == CONSOLE_STL01)
  {
    pos = (16 - strlen(zone_desc)) / 2;
    sprintf(cmd, "ATN%02d05%s\r", pos+1, zone_desc);
  }
  else
    sprintf(cmd, "ATN064051%s\r", zone_desc);
  console_send(dev, cmd);
  console_send(dev, str_17_9);
  
  console_register_callback(dev, KEY_ENTER, console_zone_activate2, zone_as_voidp);
  console_register_callback(dev, KEY_CANCEL, console_zone_activate, NULL);
  
  return NULL;
}

CALLBACK void* console_zone_activate(ProtDevice *dev, int press, void *null)
{
  char zone_to_show[1 + n_ZS + n_ZI];
  char *zone_cod, *zone_desc, str[18];
  int i;
  
  console_disable_menu(dev, str_17_10);
  
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
      sprintf(str, str_17_5, zone_cod);
      CONSOLE->support_list = console_list_add(CONSOLE->support_list, str, zone_desc, console_zone_activate1, i);
    }

  console_list_show(dev, CONSOLE->support_list, 0, 3, 0);

  return NULL;
}

static CALLBACK void* console_zone_activate_w_alm2(ProtDevice *dev, int press, void *zone_as_voidp)
{
  char cmd[16];
  
//  cmd_zone_on((int)zone_as_voidp, 1);
  sprintf(cmd, "Y1%03d", (int)zone_as_voidp);
  codec_parse_cmd(cmd, 5, dev);
  
  console_send(dev, "ATS00\r");
  console_send(dev, str_17_6);
  console_send(dev, str_17_7);
  
  CONSOLE->last_menu_item = console_zone_activate_w_alm;
  timeout_on(CONSOLE->timeout, (timeout_func)console_return_to_last_menu_item, dev, 0, TIMEOUT_MSG);

  return NULL;
}

static CALLBACK void* console_zone_activate_w_alm1(ProtDevice *dev, int press, void *zone_as_voidp)
{
  int zone = (int)zone_as_voidp;
  char *zone_cod, *zone_desc, cmd[32];
  int pos;

  console_send(dev, "ATS03\r");
  
  string_zone_name(zone, &zone_cod, &zone_desc);
  sprintf(cmd, str_17_8, zone_cod);
  console_send(dev, cmd);
  if(CONSOLE->tipo == CONSOLE_STL01)
  {
    pos = (16 - strlen(zone_desc)) / 2;
    sprintf(cmd, "ATN%02d05%s\r", pos+1, zone_desc);
  }
  else
    sprintf(cmd, "ATN064051%s\r", zone_desc);
  console_send(dev, cmd);
  console_send(dev, str_17_9);
  
  console_register_callback(dev, KEY_ENTER, console_zone_activate_w_alm2, zone_as_voidp);
  console_register_callback(dev, KEY_CANCEL, console_zone_activate_w_alm, NULL);
  
  return NULL;
}

CALLBACK void* console_zone_activate_w_alm(ProtDevice *dev, int press, void *null)
{
  char zone_to_show[1 + n_ZS + n_ZI];
  char *zone_cod, *zone_desc, str[18];
  int i;
  
  console_disable_menu(dev, str_17_10);

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
      sprintf(str, str_17_5, zone_cod);
      CONSOLE->support_list = console_list_add(CONSOLE->support_list, str, zone_desc, console_zone_activate_w_alm1, i);
    }

  console_list_show(dev, CONSOLE->support_list, 0, 3, 0);

  return NULL;
}

static CALLBACK void* console_zone_activate_w_oos2(ProtDevice *dev, int press, void *zone_as_voidp)
{
  char cmd[16];
  
//  cmd_zone_on((int)zone_as_voidp, 2);
  sprintf(cmd, "Y2%03d", (int)zone_as_voidp);
  codec_parse_cmd(cmd, 5, dev);
  
  console_send(dev, "ATS00\r");
  console_send(dev, str_17_6);
  console_send(dev, str_17_7);
  
  CONSOLE->last_menu_item = console_zone_activate_w_oos;
  timeout_on(CONSOLE->timeout, (timeout_func)console_return_to_last_menu_item, dev, 0, TIMEOUT_MSG);

  return NULL;
}

static CALLBACK void* console_zone_activate_w_oos1(ProtDevice *dev, int press, void *zone_as_voidp)
{
  int zone = (int)zone_as_voidp;
  char *zone_cod, *zone_desc, cmd[32];
  int pos;

  console_send(dev, "ATS03\r");
  
  string_zone_name(zone, &zone_cod, &zone_desc);
  sprintf(cmd, str_17_8, zone_cod);
  console_send(dev, cmd);
  if(CONSOLE->tipo == CONSOLE_STL01)
  {
    pos = (16 - strlen(zone_desc)) / 2;
    sprintf(cmd, "ATN%02d05%s\r", pos+1, zone_desc);
  }
  else
    sprintf(cmd, "ATN064051%s\r", zone_desc);
  console_send(dev, cmd);
  console_send(dev, str_17_9);
  
  console_register_callback(dev, KEY_ENTER, console_zone_activate_w_oos2, zone_as_voidp);
  console_register_callback(dev, KEY_CANCEL, console_zone_activate_w_oos, NULL);
  
  return NULL;
}

CALLBACK void* console_zone_activate_w_oos(ProtDevice *dev, int press, void *null)
{
  char zone_to_show[1 + n_ZS + n_ZI];
  char *zone_cod, *zone_desc, str[18];
  int i;
  
  console_disable_menu(dev, str_17_10);

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
      sprintf(str, str_17_5, zone_cod);
      CONSOLE->support_list = console_list_add(CONSOLE->support_list, str, zone_desc, console_zone_activate_w_oos1, i);
    }

  console_list_show(dev, CONSOLE->support_list, 0, 3, 0);

  return NULL;
}
