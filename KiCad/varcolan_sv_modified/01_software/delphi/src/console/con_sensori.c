#include "console.h"
#include "con_sensori.h"
#include "../database.h"
#include "../strings.h"
#include "../command.h"
#include "../master.h"
#include <stdio.h>
#include <string.h>

/***********************************************************************
	SENSORI menu functions
***********************************************************************/

static void console_sensori_list(ProtDevice *dev, int mask, int val, ConsoleCallback func)
{
  char *sens_cod, *sens_desc;
  int i, j, num;

  console_list_free(CONSOLE->support_list);
  CONSOLE->support_list = NULL;

  for(i=0; i<n_SE;)
  {
    if(master_periph_present(i>>3))
    {
      num = master_sensor_valid(i);
      for(j=0; j<num; j++, i++)
      {
        if((SE[i] & mask) == val)
        {
          string_sensor_name(i, &sens_cod, &sens_desc);
          CONSOLE->support_list = console_list_add(CONSOLE->support_list, sens_cod, sens_desc, func, i);
        }
      }
      i += 8 - num;
    }
    else
      i += 8;
  }

  console_list_show(dev, CONSOLE->support_list, 0, 3, 0);
}

static CALLBACK void* console_sensori_on_service2(ProtDevice *dev, int press, void *sens_as_voidp)
{
  cmd_sensor_off((int)sens_as_voidp);
  
  console_send(dev, "ATS00\r");
  console_send(dev, str_11_0);
  console_send(dev, str_11_1);
  
  CONSOLE->last_menu_item = console_sensori_on_service;
  timeout_on(CONSOLE->timeout, (timeout_func)console_return_to_last_menu_item, dev, 0, TIMEOUT_MSG);

  return NULL;
}

static CALLBACK void* console_sensori_on_service1(ProtDevice *dev, int press, void *sens_as_voidp)
{  
  int sens = (int)sens_as_voidp;
  char *sens_cod, *sens_desc, cmd[32];
  int pos;

  console_send(dev, "ATS03\r");
  
  string_sensor_name(sens, &sens_cod, &sens_desc);
  sprintf(cmd, str_11_19, sens_cod);
  console_send(dev, cmd);
  pos = (16 - strlen(sens_desc)) / 2;
  sprintf(cmd, str_11_20, pos + 1, sens_desc);
  console_send(dev, cmd);
  console_send(dev, str_11_2);
  
  console_register_callback(dev, KEY_ENTER, console_sensori_on_service2, sens_as_voidp);
  console_register_callback(dev, KEY_CANCEL, console_sensori_on_service, NULL);
  
  return NULL;
}

CALLBACK void* console_sensori_on_service(ProtDevice *dev, int press, void *null)
{  
  console_disable_menu(dev, str_11_3);
  console_sensori_list(dev, bitOOS, 0, console_sensori_on_service1);
  return NULL;
}

static CALLBACK void* console_sensori_out_of_service2(ProtDevice *dev, int press, void *sens_as_voidp)
{
  database_lock();
  cmd_sensor_on((int)sens_as_voidp);
  database_unlock();
  
  console_send(dev, "ATS00\r");
  console_send(dev, str_11_0);
  console_send(dev, str_11_5);
  
  CONSOLE->last_menu_item = console_sensori_out_of_service;
  timeout_on(CONSOLE->timeout, (timeout_func)console_return_to_last_menu_item, dev, 0, TIMEOUT_MSG);

  return NULL;
}

static CALLBACK void* console_sensori_out_of_service1(ProtDevice *dev, int press, void *sens_as_voidp)
{  
  int sens = (int)sens_as_voidp;
  char *sens_cod, *sens_desc, cmd[32];
  int pos;

  console_send(dev, "ATS03\r");
  
  string_sensor_name(sens, &sens_cod, &sens_desc);
  sprintf(cmd, str_11_19, sens_cod);
  console_send(dev, cmd);
  pos = (16 - strlen(sens_desc)) / 2;
  sprintf(cmd, str_11_20, pos + 1, sens_desc);
  console_send(dev, cmd);
  console_send(dev, str_11_6);
  
  console_register_callback(dev, KEY_ENTER, console_sensori_out_of_service2, sens_as_voidp);
  console_register_callback(dev, KEY_CANCEL, console_sensori_out_of_service, NULL);
  
  return NULL;
}

CALLBACK void* console_sensori_out_of_service(ProtDevice *dev, int press, void *null)
{
  console_disable_menu(dev, str_11_7);
  console_sensori_list(dev, bitOOS, bitOOS, console_sensori_out_of_service1);
  return NULL;
}

static CALLBACK void* console_sensori_accept1(ProtDevice *dev, int press, void *sens_as_voidp)
{  
  int sens = (int)sens_as_voidp;
  
  if((SE[sens] & bitMUAlarm) && !(SE[sens] & bitAlarm))
    cmd_status_accept(sens, bitMUAlarm, MU_alarm);
  else if((SE[sens] & bitMUSabotage) && !(SE[sens] & bitSabotage))
    cmd_status_accept(sens, bitMUSabotage, MU_sabotage);
  else if((SE[sens] & bitMUFailure) && !(SE[sens] & bitFailure))
    cmd_status_accept(sens, bitMUFailure, MU_failure);
    
  console_send(dev, "ATS00\r");
  console_send(dev, str_11_8);
  
  CONSOLE->last_menu_item = console_sensori_accept;
  timeout_on(CONSOLE->timeout, (timeout_func)console_return_to_last_menu_item, dev, 0, TIMEOUT_MSG);

  return NULL;
}

CALLBACK void* console_sensori_accept(ProtDevice *dev, int press, void *null)
{
  char *sens_cod, *sens_desc;
  int i, j;

  console_list_free(CONSOLE->support_list);
  CONSOLE->support_list = NULL;

  for(i=0; i<n_SE;)
  {
    if(master_periph_present(i>>3))
    {
      for(j=0; j<8; j++)
      {
        if((SE[i] & bitMUAlarm) && !(SE[i] & bitAlarm))
        {
          string_sensor_name(i, &sens_cod, &sens_desc);
          strcat(sens_cod, str_11_9);
          CONSOLE->support_list = console_list_add(CONSOLE->support_list, sens_cod, sens_desc, console_sensori_accept1, i++);
        }
        else if((SE[i] & bitMUFailure) && !(SE[i] & bitFailure))
        {
          string_sensor_name(i, &sens_cod, &sens_desc);
          strcat(sens_cod, str_11_10);
          CONSOLE->support_list = console_list_add(CONSOLE->support_list, sens_cod, sens_desc, console_sensori_accept1, i++);
        }
        else if((SE[i] & bitMUSabotage) && !(SE[i] & bitSabotage))
        {
          string_sensor_name(i, &sens_cod, &sens_desc);
          strcat(sens_cod, str_11_11);
          CONSOLE->support_list = console_list_add(CONSOLE->support_list, sens_cod, sens_desc, console_sensori_accept1, i++);
        }
        else
          i++;
      }
    }
    else
      i += 8;
  }

  console_disable_menu(dev, str_11_12);
  console_list_show(dev, CONSOLE->support_list, 0, 3, 0);

  return NULL;
}

static CALLBACK void* console_sensori_accept_all1(ProtDevice *dev, int press, void *null)
{
  int i;
  
  for(i=0; i<n_SE; i++)
  {
    if((SE[i] & bitMUAlarm) && !(SE[i] & bitAlarm))
      cmd_status_accept(i, bitMUAlarm, MU_alarm);
    if((SE[i] & bitMUSabotage) && !(SE[i] & bitSabotage))
      cmd_status_accept(i, bitMUSabotage, MU_sabotage);
    if((SE[i] & bitMUFailure) && !(SE[i] & bitFailure))
      cmd_status_accept(i, bitMUFailure, MU_failure);
  }
  
  console_send(dev, "ATS00\r");
  console_send(dev, str_11_13);
  console_send(dev, str_11_14);

  timeout_on(CONSOLE->timeout, (timeout_func)console_return_to_menu, dev, 0, TIMEOUT_MSG);
  return NULL;
}

CALLBACK void* console_sensori_accept_all(ProtDevice *dev, int press, void *null)
{
  console_disable_menu(dev, str_11_15);
  console_send(dev, str_11_16);
  console_send(dev, str_11_17);
  console_send(dev, str_11_18);
  
  console_register_callback(dev, KEY_ENTER, console_sensori_accept_all1, NULL);
  console_register_callback(dev, KEY_CANCEL, console_show_menu, NULL);

  return NULL;
}

