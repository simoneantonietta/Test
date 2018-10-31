#include "console.h"
#include "con_badge.h"
#include "../badge.h"
#include <stdio.h>

/***********************************************************************
	BADGE menu functions
***********************************************************************/

static CALLBACK void* console_badge_common2(ProtDevice *dev, int reader_as_int, void *enable)
{
  int console_reader = -1;
  sscanf((char*)reader_as_int, "%d", &console_reader);
  if((console_reader == -1) || (console_reader > 255))
  {
    console_send(dev, "ATS00\r");
    console_send(dev, str_1_0);
    console_send(dev, str_1_1);
    
    if(enable)
      CONSOLE->last_menu_item = console_badge_enable;
    else
      CONSOLE->last_menu_item = console_badge_disable;
    timeout_on(CONSOLE->timeout, (timeout_func)console_return_to_last_menu_item, dev, 0, TIMEOUT_MSG);
  }
  else
  {
    if(enable)
    {
      badge_manage(console_reader, CONSOLE->support_idx, '1');
      CONSOLE->last_menu_item = console_badge_enable;
    }
    else
    {
      badge_manage(console_reader, CONSOLE->support_idx, '2');
      CONSOLE->last_menu_item = console_badge_disable;
    }

    console_send(dev, "ATS00\r");
    console_send(dev, str_1_2);
    if(enable)
      console_send(dev, str_1_3);
    else
      console_send(dev, str_1_4);
  
    timeout_on(CONSOLE->timeout, (timeout_func)console_return_to_last_menu_item, dev, 0, TIMEOUT_MSG);
  }
  return NULL;
}

static CALLBACK void* console_badge_common1(ProtDevice *dev, int badge_as_int, void *enable)
{
  CONSOLE->support_idx = -1;
  sscanf((char*)badge_as_int, "%d", &(CONSOLE->support_idx));
  if((CONSOLE->support_idx == -1) || (CONSOLE->support_idx > 1999))
  {
    console_send(dev, "ATS00\r");
    console_send(dev, str_1_5);
    console_send(dev, str_1_6);
    
    if(enable)
      CONSOLE->last_menu_item = console_badge_enable;
    else
      CONSOLE->last_menu_item = console_badge_disable;
    timeout_on(CONSOLE->timeout, (timeout_func)console_return_to_last_menu_item, dev, 0, TIMEOUT_MSG);
  }
  else
  {
    console_send(dev, str_1_7);
    console_send(dev, str_1_11);
    console_send(dev, "ATL111\r");
    console_send(dev, str_1_12);
  
    console_register_callback(dev, KEY_STRING, console_badge_common2, enable);
    console_register_callback(dev, KEY_CANCEL, console_badge_enable, NULL);
  }
  return NULL;
}

static void console_badge_common(ProtDevice *dev, int enable)
{
  console_send(dev, str_1_8);
  console_send(dev, str_1_13);
  console_send(dev, "ATL111\r");
  console_send(dev, str_1_14);
  
  console_register_callback(dev, KEY_STRING, console_badge_common1, (void*)enable);
  console_register_callback(dev, KEY_CANCEL, console_show_menu, NULL);
}

CALLBACK void* console_badge_enable(ProtDevice *dev, int press, void *null)
{
  console_disable_menu(dev, str_1_9);
  console_badge_common(dev, 1);
  return NULL;
}

CALLBACK void* console_badge_disable(ProtDevice *dev, int press, void *null)
{
  console_disable_menu(dev, str_1_10);
  console_badge_common(dev, 0);
  return NULL;
}


