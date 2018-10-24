#include "console.h"
#include "con_attuatori.h"
#include "../database.h"
#include "../strings.h"
#include "../command.h"
#include "../master.h"
#include <stdio.h>
#include <string.h>

/***********************************************************************
	ATTUATORI menu functions
***********************************************************************/

static void console_attuatori_list(ProtDevice *dev, int mask, int val, ConsoleCallback func)
{
  char *act_cod, *act_desc;
  int i, j, num;

  console_list_free(CONSOLE->support_list);
  CONSOLE->support_list = NULL;

  for(i=0; i<n_AT;)
  {
    if(master_periph_present(i>>3))
    {
      num = master_actuator_valid(i);
      for(j=0; j<num; j++, i++)
      {
        if((AT[i] & mask) == val)
        {
          string_actuator_name(i, &act_cod, &act_desc);
          CONSOLE->support_list = console_list_add(CONSOLE->support_list, act_cod, act_desc, func, i);
        }
      }
      i += 8 - num;
    }
    else
      i += 8;
  }

  console_list_show(dev, CONSOLE->support_list, 0, 3, 0);
}

static CALLBACK void* console_attuatori_on_service2(ProtDevice *dev, int press, void *act_as_voidp)
{
  cmd_actuator_off((int)act_as_voidp);
  
  console_send(dev, "ATS00\r");
  console_send(dev, str_0_0);
  console_send(dev, str_0_1);
  
  CONSOLE->last_menu_item = console_attuatori_on_service;
  timeout_on(CONSOLE->timeout, (timeout_func)console_return_to_last_menu_item, dev, 0, TIMEOUT_MSG);

  return NULL;
}

static CALLBACK void* console_attuatori_on_service1(ProtDevice *dev, int press, void *act_as_voidp)
{  
  int act = (int)act_as_voidp;
  char *act_cod, *act_desc, cmd[32];
  int pos;

  console_send(dev, "ATS03\r");
  
  string_actuator_name(act, &act_cod, &act_desc);
  sprintf(cmd, str_0_14, act_cod);
  console_send(dev, cmd);
  pos = (16 - strlen(act_desc)) / 2;
  sprintf(cmd, str_0_15, pos + 1, act_desc);
  console_send(dev, cmd);
  console_send(dev, str_0_2);
  
  console_register_callback(dev, KEY_ENTER, console_attuatori_on_service2, act_as_voidp);
  console_register_callback(dev, KEY_CANCEL, console_attuatori_on_service, NULL);

  return NULL;
}

CALLBACK void* console_attuatori_on_service(ProtDevice *dev, int press, void *null)
{  
  console_disable_menu(dev, str_0_3);
  console_attuatori_list(dev, bitOOS, 0, console_attuatori_on_service1);
  return NULL;
}

static CALLBACK void* console_attuatori_out_of_service2(ProtDevice *dev, int press, void *act_as_voidp)
{
  cmd_actuator_on((int)act_as_voidp);
  
  console_send(dev, "ATS00\r");
  console_send(dev, str_0_0);
  console_send(dev, str_0_5);
  
  CONSOLE->last_menu_item = console_attuatori_out_of_service;
  timeout_on(CONSOLE->timeout, (timeout_func)console_return_to_last_menu_item, dev, 0, TIMEOUT_MSG);

  return NULL;
}

static CALLBACK void* console_attuatori_out_of_service1(ProtDevice *dev, int press, void *act_as_voidp)
{  
  int act = (int)act_as_voidp;
  char *act_cod, *act_desc, cmd[32];
  int pos;

  console_send(dev, "ATS03\r");
  
  string_actuator_name(act, &act_cod, &act_desc);
  sprintf(cmd, "ATN0204%s\r", act_cod);
  console_send(dev, cmd);
  pos = (16 - strlen(act_desc)) / 2;
  sprintf(cmd, "ATN%02d05%s\r", pos + 1, act_desc);
  console_send(dev, cmd);
  console_send(dev, str_0_6);
  
  console_register_callback(dev, KEY_ENTER, console_attuatori_out_of_service2, act_as_voidp);
  console_register_callback(dev, KEY_CANCEL, console_attuatori_out_of_service, NULL);
  
  return NULL;
}

CALLBACK void* console_attuatori_out_of_service(ProtDevice *dev, int press, void *null)
{
  console_disable_menu(dev, str_0_7);
  console_attuatori_list(dev, bitOOS, bitOOS, console_attuatori_out_of_service1);
  return NULL;
}

static CALLBACK void* console_attuatori_on1(ProtDevice *dev, int press, void *act_as_voidp)
{
  database_lock();
  AT[(int)act_as_voidp] |= bitON | bitLHF | bitAbilReq;
  database_unlock();
  
  console_send(dev, "ATS00\r");
  console_send(dev, str_0_0);
  console_send(dev, str_0_9);
  
  CONSOLE->last_menu_item = console_attuatori_on;
  timeout_on(CONSOLE->timeout, (timeout_func)console_return_to_last_menu_item, dev, 0, TIMEOUT_MSG);

  return NULL;
}

CALLBACK void* console_attuatori_on(ProtDevice *dev, int press, void *null)
{
  console_disable_menu(dev, str_0_10);
  console_attuatori_list(dev, bitOOS | bitON, 0, console_attuatori_on1);
  return NULL;
}

static CALLBACK void* console_attuatori_off1(ProtDevice *dev, int press, void *act_as_voidp)
{
  database_lock();
  AT[(int)act_as_voidp] &= ~bitON;
  AT[(int)act_as_voidp] |= bitHLF | bitAbilReq;
  database_unlock();

  console_send(dev, "ATS00\r");
  console_send(dev, str_0_0);
  console_send(dev, str_0_12);
  
  CONSOLE->last_menu_item = console_attuatori_off;
  timeout_on(CONSOLE->timeout, (timeout_func)console_return_to_last_menu_item, dev, 0, TIMEOUT_MSG);

  return NULL;
}

CALLBACK void* console_attuatori_off(ProtDevice *dev, int press, void *null)
{
  console_disable_menu(dev, str_0_13);
  console_attuatori_list(dev, bitOOS | bitON, bitON, console_attuatori_off1);  
  return NULL;
}
