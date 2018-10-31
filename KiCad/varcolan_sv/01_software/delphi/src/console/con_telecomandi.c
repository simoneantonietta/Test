#include "console.h"
#include "con_telecomandi.h"
#include "../database.h"
#include "../strings.h"
#include "../list.h"
#include <stdio.h>
#include <string.h>

/***********************************************************************
	TELECOMANDI menu functions
***********************************************************************/

static CALLBACK void* console_telecomandi_send2(ProtDevice *dev, int press, void *tcmd_as_voidp)
{
  database_lock();
  database_set_alarm2(&ME[(int)tcmd_as_voidp]);
  database_unlock();
  
  console_send(dev, "ATS00\r");
  console_send(dev, str_16_0);
  console_send(dev, str_16_1);
  
  CONSOLE->last_menu_item = console_telecomandi_send;
  timeout_on(CONSOLE->timeout, (timeout_func)console_return_to_last_menu_item, dev, 0, TIMEOUT_MSG);

  return NULL;
}

static CALLBACK void* console_telecomandi_send1(ProtDevice *dev, int press, void *tcmd_as_voidp)
{
  int tcmd = (int)tcmd_as_voidp;
  char *tcmd_cod, *tcmd_desc, cmd[32];
  int pos;

  console_send(dev, "ATS03\r");
  
  string_command_name(tcmd, &tcmd_cod, &tcmd_desc);
  sprintf(cmd, str_16_12, tcmd_cod);
  console_send(dev, cmd);
  pos = (16 - strlen(tcmd_desc)) / 2;
  sprintf(cmd, str_16_13, pos+1, tcmd_desc);
  console_send(dev, cmd);
  console_send(dev, str_16_2);
  
  console_register_callback(dev, KEY_ENTER, console_telecomandi_send2, tcmd_as_voidp);
  console_register_callback(dev, KEY_CANCEL, console_telecomandi_send, NULL);
  
  return NULL;
}

CALLBACK void* console_telecomandi_send(ProtDevice *dev, int press, void *null)
{
  char *tcmd_cod, *tcmd_desc;
  int i;

  console_disable_menu(dev, str_16_3);

  console_list_free(CONSOLE->support_list);
  CONSOLE->support_list = NULL;

  for(i=0; i<n_ME; i++)
  {
    string_command_name(i, &tcmd_cod, &tcmd_desc);
    if(tcmd_desc != StringDescNull)
      CONSOLE->support_list = console_list_add(CONSOLE->support_list, tcmd_cod, tcmd_desc, console_telecomandi_send1, i);
  }

  console_list_show(dev, CONSOLE->support_list, 0, 3, 0);
  return NULL;
}

CALLBACK void* console_telecomandi_peripherals(ProtDevice *dev, int press, void *null)
{
  console_disable_menu(dev, str_16_4);
  
  console_send(dev, str_16_5);
  console_send(dev, str_16_6);
  console_send(dev, str_16_7);
  
  list_periph();
  
  timeout_on(CONSOLE->timeout, (timeout_func)console_return_to_menu, dev, 0, TIMEOUT_MSG*2);
  
  return NULL;
}

CALLBACK void* console_telecomandi_sensorzonelist(ProtDevice *dev, int press, void *null)
{
  console_disable_menu(dev, str_16_8);
  
  console_send(dev, str_16_9);
  console_send(dev, str_16_10);
  console_send(dev, str_16_11);
  
  list_sensors_zone();
  
  timeout_on(CONSOLE->timeout, (timeout_func)console_return_to_menu, dev, 0, TIMEOUT_MSG*2);
  
  return NULL;
}

