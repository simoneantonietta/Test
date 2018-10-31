#include "console.h"
#include "con_inoltra.h"
#include "../database.h"
#include "../delphi.h"
#include <stdio.h>

/***********************************************************************
	INOLTRA menu functions
***********************************************************************/

static CALLBACK void* console_inoltra_secret2(ProtDevice *dev, int secret_as_int, void *null)
{
  int secret = -1;
  
  console_send(dev, "ATS00\r");

  sscanf((char*)secret_as_int, "%d", &secret);
  if((secret < 0) || (secret > 65535))
  {
    console_send(dev, str_3_0);
    console_send(dev, str_3_1);

    CONSOLE->last_menu_item = console_inoltra_secret;
    timeout_on(CONSOLE->timeout, (timeout_func)console_return_to_last_menu_item, dev, 0, TIMEOUT_MSG);
    
    return NULL;
  }
  
  if(CONSOLE->support_idx == 9999)
  {
    FILE *fp;
    
    fp = fopen(StopClockFile, "w");
    if(fp)
    {
      fprintf(fp, "%d", secret);
      fclose(fp);
    }
    else
    {
      console_send(dev, str_3_2);
      console_send(dev, str_3_3);

      timeout_on(CONSOLE->timeout, (timeout_func)console_return_to_menu, dev, 0, TIMEOUT_MSG);
      return NULL;
    }
  }
  else
  {
    SEGRETO[CONSOLE->support_idx] = secret;
    database_changed = 1;
  }
  
  console_send(dev, str_3_0);
  console_send(dev, str_3_5);

  timeout_on(CONSOLE->timeout, (timeout_func)console_return_to_menu, dev, 0, TIMEOUT_MSG);

  return NULL;
}

static CALLBACK void* console_inoltra_secret1(ProtDevice *dev, int type_as_int, void *null)
{
  CONSOLE->support_idx = -1;
  sscanf((char*)type_as_int, "%d", &CONSOLE->support_idx);
  if((CONSOLE->support_idx != 9999) && ((CONSOLE->support_idx < 0) || (CONSOLE->support_idx > (n_SEGRETO-1))))
  {
    console_send(dev, "ATS00\r");
    console_send(dev, str_3_6);
    console_send(dev, str_3_1);

    CONSOLE->last_menu_item = console_inoltra_secret;
    timeout_on(CONSOLE->timeout, (timeout_func)console_return_to_last_menu_item, dev, 0, TIMEOUT_MSG);
    
    return NULL;
  }
  
  console_send(dev, str_3_8);
  console_send(dev, str_3_20);
  console_send(dev, str_3_21);
  console_send(dev, "ATL111\r");

  console_register_callback(dev, KEY_STRING, console_inoltra_secret2, NULL);
  console_register_callback(dev, KEY_CANCEL, console_inoltra_secret, NULL);
  
  return NULL;
}

CALLBACK void* console_inoltra_secret(ProtDevice *dev, int press, void *null)
{
  console_disable_menu(dev, str_3_9);
  
  console_send(dev, str_3_10);
  console_send(dev, str_3_22);
  console_send(dev, str_3_23);
  console_send(dev, "ATL111\r");

  console_register_callback(dev, KEY_STRING, console_inoltra_secret1, NULL);
  console_register_callback(dev, KEY_CANCEL, console_show_menu, NULL);
  
  return NULL;
}

static CALLBACK void* console_inoltra_event2(ProtDevice *dev, int event_as_int, void *null)
{
  int event = -1;

  console_send(dev, "ATS00\r");
  
  sscanf((char*)event_as_int, "%d", &event);
  if((event < 0) || (event > 65535))
  {
    console_send(dev, str_3_11);
    console_send(dev, str_3_1);

    CONSOLE->last_menu_item = console_inoltra_event;
    timeout_on(CONSOLE->timeout, (timeout_func)console_return_to_last_menu_item, dev, 0, TIMEOUT_MSG);
    
    return NULL;
  }
  
  EVENTO[CONSOLE->support_idx] = event;
  
  console_send(dev, str_3_11);
  console_send(dev, str_3_5);

  timeout_on(CONSOLE->timeout, (timeout_func)console_return_to_menu, dev, 0, TIMEOUT_MSG);

  return NULL;
}

static CALLBACK void* console_inoltra_event1(ProtDevice *dev, int type_as_int, void *null)
{
  CONSOLE->support_idx = -1;
  sscanf((char*)type_as_int, "%d", &CONSOLE->support_idx);
  if((CONSOLE->support_idx < 0) || (CONSOLE->support_idx > (n_EVENTO-1)))
  {
    console_send(dev, "ATS00\r");
    console_send(dev, str_3_15);
    console_send(dev, str_3_1);

    CONSOLE->last_menu_item = console_inoltra_event;
    timeout_on(CONSOLE->timeout, (timeout_func)console_return_to_last_menu_item, dev, 0, TIMEOUT_MSG);
    
    return NULL;
  }
  
  console_send(dev, str_3_17);
  console_send(dev, str_3_20);
  console_send(dev, str_3_21);
  console_send(dev, "ATL111\r");

  console_register_callback(dev, KEY_STRING, console_inoltra_event2, NULL);
  console_register_callback(dev, KEY_CANCEL, console_inoltra_event, NULL);
  
  return NULL;
}

CALLBACK void* console_inoltra_event(ProtDevice *dev, int press, void *null)
{
  console_disable_menu(dev, str_3_18);
  
  console_send(dev, str_3_19);
  console_send(dev, str_3_22);
  console_send(dev, str_3_23);
  console_send(dev, "ATL111\r");

  console_register_callback(dev, KEY_STRING, console_inoltra_event1, NULL);
  console_register_callback(dev, KEY_CANCEL, console_show_menu, NULL);
  
  return NULL;
}
