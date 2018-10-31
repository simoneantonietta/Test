#include "console.h"
#include "con_impostazioni.h"
#include "../database.h"
#include "../command.h"
#include "../master.h"
#include "../strings.h"
#include "../user.h"
#include "../support.h"
#include "../delphi.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <net/if.h>

/***********************************************************************
	IMPOSTAZIONI menu functions
***********************************************************************/

static CALLBACK void* console_impostazioni_insert_date_key(ProtDevice *dev, int press, void *digit_as_voidp);

static CALLBACK void* console_impostazioni_insert_date_keycanc(ProtDevice *dev, int press, void *null)
{
  char cmd[32];
  int i, j;
  
  if(press)
  {
    if(CONSOLE->tipo == CONSOLE_STL01)
    {
      if(CONSOLE->impostazioni_buf_digits == 4)
        sprintf(cmd, "ATN%02d%02d__/__\r", CONSOLE->impostazioni_buf_xpos, CONSOLE->impostazioni_buf_ypos);
      else
        sprintf(cmd, "ATN%02d%02d__/__/__\r", CONSOLE->impostazioni_buf_xpos, CONSOLE->impostazioni_buf_ypos);
    }
    else
    {
      if(CONSOLE->impostazioni_buf_digits == 4)
        sprintf(cmd, "ATN%03d%02d0__/__\r", CONSOLE->impostazioni_buf_xpos*8, CONSOLE->impostazioni_buf_ypos);
      else
        sprintf(cmd, "ATN%03d%02d0__/__/__\r", CONSOLE->impostazioni_buf_xpos*8, CONSOLE->impostazioni_buf_ypos);
    }
    console_send(dev, cmd);  
    CONSOLE->support_idx = 0;
    for(i=0; i<6; i++) CONSOLE->impostazioni_buf1[i] = 0;
  }
  else
  {
    if(!CONSOLE->support_idx)
    {
      CONSOLE->impostazioni_cancel(dev, 0, NULL);
      return NULL;
    }
    
    CONSOLE->support_idx--;
    CONSOLE->impostazioni_buf1[CONSOLE->support_idx] = 0;
    if(CONSOLE->tipo == CONSOLE_STL01)
    {
      sprintf(cmd, "ATN%02d%02d__/__/__\r", CONSOLE->impostazioni_buf_xpos, CONSOLE->impostazioni_buf_ypos);
      j = 0;
      for(i=0; CONSOLE->impostazioni_buf1[i]; i++)
      {
        cmd[7+j] = CONSOLE->impostazioni_buf1[i];
        j++;
        if((j == 2) || (j == 5)) j++;
      }
      if(CONSOLE->impostazioni_buf_digits == 4)
      {
        cmd[12] = '\r';
        cmd[13] = '\0';
      }
    }
    else
    {
      sprintf(cmd, "ATN%03d%02d0__/__/__\r", CONSOLE->impostazioni_buf_xpos*8, CONSOLE->impostazioni_buf_ypos);
      j = 0;
      for(i=0; CONSOLE->impostazioni_buf1[i]; i++)
      {
        cmd[9+j] = CONSOLE->impostazioni_buf1[i];
        j++;
        if((j == 2) || (j == 5)) j++;
      }
      if(CONSOLE->impostazioni_buf_digits == 4)
      {
        cmd[14] = '\r';
        cmd[15] = '\0';
      }
    }
    console_send(dev, cmd);  
  }
  
  for(i=0; i<10; i++)
    console_register_callback(dev, KEY_0 + i, console_impostazioni_insert_date_key, (void*)i);
  console_register_callback(dev, KEY_CANCEL, console_impostazioni_insert_date_keycanc, NULL);
  if(!CONSOLE->support_idx)
    console_register_callback(dev, KEY_ENTER, CONSOLE->impostazioni_enter, NULL);

  return NULL;
}

static CALLBACK void* console_impostazioni_insert_date_key(ProtDevice *dev, int press, void *digit_as_voidp)
{
  int digit = (int)digit_as_voidp;
  char cmd[32];
  int i, j, valid;
  
  valid = 0;
  
  switch(console_localization)
  {
    case 1:
      if((CONSOLE->support_idx < CONSOLE->impostazioni_buf_digits) &&
      ( ((CONSOLE->support_idx != 2) || ((CONSOLE->support_idx == 2) && (digit < 4))) &&
        ((CONSOLE->support_idx != 3) || ((CONSOLE->support_idx == 3) && 
         (CONSOLE->impostazioni_buf1[2] == '3') && (digit < 2)) || ((CONSOLE->support_idx == 3) &&
         (CONSOLE->impostazioni_buf1[2] != '3'))) &&
        ((CONSOLE->support_idx != 0) || ((CONSOLE->support_idx == 0) && (digit < 2))) &&
        ((CONSOLE->support_idx != 1) || ((CONSOLE->support_idx == 1) && 
         (CONSOLE->impostazioni_buf1[0] == '1') && (digit < 3)) || ((CONSOLE->support_idx == 1) &&
         (CONSOLE->impostazioni_buf1[0] != '1')))
      )) valid = 1;
      break;
    default:
      if((CONSOLE->support_idx < CONSOLE->impostazioni_buf_digits) &&
      ( ((CONSOLE->support_idx != 0) || ((CONSOLE->support_idx == 0) && (digit < 4))) &&
        ((CONSOLE->support_idx != 1) || ((CONSOLE->support_idx == 1) && 
         (CONSOLE->impostazioni_buf1[0] == '3') && (digit < 2)) || ((CONSOLE->support_idx == 1) &&
         (CONSOLE->impostazioni_buf1[0] != '3'))) &&
        ((CONSOLE->support_idx != 2) || ((CONSOLE->support_idx == 2) && (digit < 2))) &&
        ((CONSOLE->support_idx != 3) || ((CONSOLE->support_idx == 3) && 
         (CONSOLE->impostazioni_buf1[2] == '1') && (digit < 3)) || ((CONSOLE->support_idx == 3) &&
         (CONSOLE->impostazioni_buf1[2] != '1')))
      )) valid = 1;
      break;
  }
  
  if(valid)
  {
    CONSOLE->impostazioni_buf1[CONSOLE->support_idx++] = '0' + digit;
    if(CONSOLE->tipo == CONSOLE_STL01)
    {
      sprintf(cmd, "ATN%02d%02d__/__/__\r", CONSOLE->impostazioni_buf_xpos, CONSOLE->impostazioni_buf_ypos);
      j = 0;
      for(i=0; CONSOLE->impostazioni_buf1[i]; i++)
      {
        cmd[7+j] = CONSOLE->impostazioni_buf1[i];
        j++;
        if((j == 2) || (j == 5)) j++;
      }
      if(CONSOLE->impostazioni_buf_digits == 4)
      {
        cmd[12] = '\r';
        cmd[13] = '\0';
      }
    }
    else
    {
      sprintf(cmd, "ATN%03d%02d0__/__/__\r", CONSOLE->impostazioni_buf_xpos*8, CONSOLE->impostazioni_buf_ypos);
      j = 0;
      for(i=0; CONSOLE->impostazioni_buf1[i]; i++)
      {
        cmd[9+j] = CONSOLE->impostazioni_buf1[i];
        j++;
        if((j == 2) || (j == 5)) j++;
      }
      if(CONSOLE->impostazioni_buf_digits == 4)
      {
        cmd[14] = '\r';
        cmd[15] = '\0';
      }
    }
    console_send(dev, cmd);
  }
  
  for(i=0; i<10; i++)
    console_register_callback(dev, KEY_0 + i, console_impostazioni_insert_date_key, (void*)i);
  console_register_callback(dev, KEY_CANCEL, console_impostazioni_insert_date_keycanc, NULL);
  if(!CONSOLE->support_idx ||
    (CONSOLE->support_idx == CONSOLE->impostazioni_buf_digits))
      console_register_callback(dev, KEY_ENTER, CONSOLE->impostazioni_enter, NULL);

  return NULL;
}

void console_impostazioni_insert_date(ProtDevice *dev, int x, int y, int year, ConsoleCallback enter,  ConsoleCallback cancel)
{
  char cmd[32];
  int i;
  
  if(CONSOLE->impostazioni_buf1[0])
  {
    if(CONSOLE->tipo == CONSOLE_STL01)
      sprintf(cmd, "ATN%02d%02d%.2s/%.2s/%.2s\r", x, y,
        CONSOLE->impostazioni_buf1, CONSOLE->impostazioni_buf1 + 2, CONSOLE->impostazioni_buf1 + 4);
    else
      sprintf(cmd, "ATN%03d%02d0%.2s/%.2s/%.2s\r", x*8, y,
        CONSOLE->impostazioni_buf1, CONSOLE->impostazioni_buf1 + 2, CONSOLE->impostazioni_buf1 + 4);
  }
  else
  {
    memset(CONSOLE->impostazioni_buf1, 0, 8);
    if(CONSOLE->tipo == CONSOLE_STL01)
      sprintf(cmd, "ATN%02d%02d__/__/__\r", x, y);
    else
      sprintf(cmd, "ATN%03d%02d0__/__/__\r", x*8, y);
    CONSOLE->impostazioni_buf_xpos = x;
    CONSOLE->support_idx = 0;
  }
  
  if(year)
  {
    CONSOLE->impostazioni_buf_digits = 6;
    if(CONSOLE->impostazioni_buf1[0])
    {
      CONSOLE->support_idx = 6;
    }
  }
  else
  {
    if(CONSOLE->tipo == CONSOLE_STL01)
    {
      cmd[12] = '\r';
      cmd[13] = '\0';
    }
    else
    {
      cmd[14] = '\r';
      cmd[15] = '\0';
    }
    CONSOLE->impostazioni_buf_digits = 4;
    if(CONSOLE->impostazioni_buf1[0])
    {
      CONSOLE->support_idx = 4;
    }
  }

  CONSOLE->impostazioni_buf_ypos = y;
  CONSOLE->impostazioni_enter = enter;
  CONSOLE->impostazioni_cancel = cancel;
  
  console_send(dev, cmd);
  console_send(dev, "ATL000\r");
    
  for(i=0; i<10; i++)
    console_register_callback(dev, KEY_0 + i, console_impostazioni_insert_date_key, (void*)i);
  console_register_callback(dev, KEY_CANCEL, console_impostazioni_insert_date_keycanc, NULL);
  if(!CONSOLE->support_idx ||
    (CONSOLE->support_idx == CONSOLE->impostazioni_buf_digits))
      console_register_callback(dev, KEY_ENTER, CONSOLE->impostazioni_enter, NULL);
}

static CALLBACK void* console_impostazioni_insert_hour_key(ProtDevice *dev, int press, void *digit_as_voidp);

static CALLBACK void* console_impostazioni_insert_hour_keycanc(ProtDevice *dev, int press, void *null)
{
  char cmd[32];
  int i, j;
  
  if(press)
  {
    if(CONSOLE->tipo == CONSOLE_STL01)
    {
      if(CONSOLE->impostazioni_buf_digits == 6)
        sprintf(cmd, "ATN%02d%02d__:__:__\r", CONSOLE->impostazioni_buf_xpos, CONSOLE->impostazioni_buf_ypos);
      else
        sprintf(cmd, "ATN%02d%02d__:__\r", CONSOLE->impostazioni_buf_xpos, CONSOLE->impostazioni_buf_ypos);
    }
    else
    {
      if(CONSOLE->impostazioni_buf_digits == 6)
        sprintf(cmd, "ATN%03d%02d0__:__:__\r", CONSOLE->impostazioni_buf_xpos*8, CONSOLE->impostazioni_buf_ypos);
      else
        sprintf(cmd, "ATN%03d%02d0__:__\r", CONSOLE->impostazioni_buf_xpos*8, CONSOLE->impostazioni_buf_ypos);
    }
    console_send(dev, cmd);  
    CONSOLE->support_idx = 0;
    for(i=0; i<6; i++) CONSOLE->impostazioni_buf1[i] = 0;
  }
  else
  {
    if(!CONSOLE->support_idx)
    {
      CONSOLE->impostazioni_cancel(dev, 0, NULL);
      return NULL;
    }
    
    CONSOLE->support_idx--;
    CONSOLE->impostazioni_buf1[CONSOLE->support_idx] = 0;
    if(CONSOLE->tipo == CONSOLE_STL01)
    {
      if(CONSOLE->impostazioni_buf_digits == 6)
        sprintf(cmd, "ATN%02d%02d__:__:__\r", CONSOLE->impostazioni_buf_xpos, CONSOLE->impostazioni_buf_ypos);
      else
        sprintf(cmd, "ATN%02d%02d__:__\r", CONSOLE->impostazioni_buf_xpos, CONSOLE->impostazioni_buf_ypos);
      j = 0;
      for(i=0; CONSOLE->impostazioni_buf1[i]; i++)
      {
        cmd[7+j] = CONSOLE->impostazioni_buf1[i];
        j++;
        if((j == 2) || (j == 5)) j++;
      }
    }
    else
    {
      if(CONSOLE->impostazioni_buf_digits == 6)
        sprintf(cmd, "ATN%03d%02d0__:__:__\r", CONSOLE->impostazioni_buf_xpos*8, CONSOLE->impostazioni_buf_ypos);
      else
        sprintf(cmd, "ATN%03d%02d0__:__\r", CONSOLE->impostazioni_buf_xpos*8, CONSOLE->impostazioni_buf_ypos);
      j = 0;
      for(i=0; CONSOLE->impostazioni_buf1[i]; i++)
      {
        cmd[9+j] = CONSOLE->impostazioni_buf1[i];
        j++;
        if((j == 2) || (j == 5)) j++;
      }
    }
    console_send(dev, cmd);  
  }
  
  for(i=0; i<10; i++)
    console_register_callback(dev, KEY_0 + i, console_impostazioni_insert_hour_key, (void*)i);
  console_register_callback(dev, KEY_CANCEL, console_impostazioni_insert_hour_keycanc, NULL);
  if(!CONSOLE->support_idx)
    console_register_callback(dev, KEY_ENTER, CONSOLE->impostazioni_enter, NULL);

  return NULL;
}

static CALLBACK void* console_impostazioni_insert_hour_key(ProtDevice *dev, int press, void *digit_as_voidp)
{
  int digit = (int)digit_as_voidp;
  char cmd[32];
  int i, j;
  
  if((CONSOLE->support_idx < CONSOLE->impostazioni_buf_digits) &&
  ( ((CONSOLE->support_idx != 0) || ((CONSOLE->support_idx == 0) && (digit < 3))) &&
    ((CONSOLE->support_idx != 1) || ((CONSOLE->support_idx == 1) && 
     (CONSOLE->impostazioni_buf1[0] == '2') && (digit < 4)) || ((CONSOLE->support_idx == 1) &&
     (CONSOLE->impostazioni_buf1[0] != '2'))) &&
    ((CONSOLE->support_idx != 2) || ((CONSOLE->support_idx == 2) && (digit < 6))) &&
    ((CONSOLE->support_idx != 4) || ((CONSOLE->support_idx == 4) && (digit < 6)))
  ))
  {
    CONSOLE->impostazioni_buf1[CONSOLE->support_idx++] = '0' + digit;
    if(CONSOLE->tipo == CONSOLE_STL01)
    {
      if(CONSOLE->impostazioni_buf_digits == 6)
        sprintf(cmd, "ATN%02d%02d__:__:__\r", CONSOLE->impostazioni_buf_xpos, CONSOLE->impostazioni_buf_ypos);
      else
        sprintf(cmd, "ATN%02d%02d__:__\r", CONSOLE->impostazioni_buf_xpos, CONSOLE->impostazioni_buf_ypos);
      j = 0;
      for(i=0; CONSOLE->impostazioni_buf1[i]; i++)
      {
        cmd[7+j] = CONSOLE->impostazioni_buf1[i];
        j++;
        if((j == 2) || (j == 5)) j++;
      }
    }
    else
    {
      if(CONSOLE->impostazioni_buf_digits == 6)
        sprintf(cmd, "ATN%03d%02d0__:__:__\r", CONSOLE->impostazioni_buf_xpos*8, CONSOLE->impostazioni_buf_ypos);
      else
        sprintf(cmd, "ATN%03d%02d0__:__\r", CONSOLE->impostazioni_buf_xpos*8, CONSOLE->impostazioni_buf_ypos);
      j = 0;
      for(i=0; CONSOLE->impostazioni_buf1[i]; i++)
      {
        cmd[9+j] = CONSOLE->impostazioni_buf1[i];
        j++;
        if((j == 2) || (j == 5)) j++;
      }
    }
    console_send(dev, cmd);
  }
  
  for(i=0; i<10; i++)
    console_register_callback(dev, KEY_0 + i, console_impostazioni_insert_hour_key, (void*)i);
  console_register_callback(dev, KEY_CANCEL, console_impostazioni_insert_hour_keycanc, NULL);
  if(!CONSOLE->support_idx ||
    (CONSOLE->support_idx == CONSOLE->impostazioni_buf_digits))
      console_register_callback(dev, KEY_ENTER, CONSOLE->impostazioni_enter, NULL);

  return NULL;
}

void console_impostazioni_insert_hour(ProtDevice *dev, int x, int y, int secs, ConsoleCallback enter, ConsoleCallback cancel)
{
  char cmd[32];
  int i;
  
  if(CONSOLE->impostazioni_buf1[0])
  {
    if(CONSOLE->tipo == CONSOLE_STL01)
      sprintf(cmd, "ATN%02d%02d%.2s:%.2s:%.2s\r", x, y,
        CONSOLE->impostazioni_buf1, CONSOLE->impostazioni_buf1 + 2, CONSOLE->impostazioni_buf1 + 4);
    else
      sprintf(cmd, "ATN%03d%02d0%.2s:%.2s:%.2s\r", x*8, y,
        CONSOLE->impostazioni_buf1, CONSOLE->impostazioni_buf1 + 2, CONSOLE->impostazioni_buf1 + 4);
  }
  else
  {
    memset(CONSOLE->impostazioni_buf1, 0, 8);
    if(CONSOLE->tipo == CONSOLE_STL01)
      sprintf(cmd, "ATN%02d%02d__:__:__\r", x, y);
    else
      sprintf(cmd, "ATN%03d%02d0__:__:__\r", x*8, y);
    CONSOLE->support_idx = 0;
  }
  
  if(secs)
  {
    CONSOLE->impostazioni_buf_digits = 6;
    if(CONSOLE->impostazioni_buf1[0])
    {
      CONSOLE->support_idx = 6;
    }
  }
  else
  {
    if(CONSOLE->tipo == CONSOLE_STL01)
    {
      cmd[12] = '\r';
      cmd[13] = '\0';
    }
    else
    {
      cmd[14] = '\r';
      cmd[15] = '\0';
    }
    CONSOLE->impostazioni_buf_digits = 4;
    if(CONSOLE->impostazioni_buf1[0])
    {
      CONSOLE->support_idx = 4;
    }
  }
  
  CONSOLE->impostazioni_buf_xpos = x;
  CONSOLE->impostazioni_buf_ypos = y;
  CONSOLE->impostazioni_enter = enter;
  CONSOLE->impostazioni_cancel = cancel;
  
  console_send(dev, cmd);
  console_send(dev, "ATL000\r");
    
  for(i=0; i<10; i++)
    console_register_callback(dev, KEY_0 + i, console_impostazioni_insert_hour_key, (void*)i);
  console_register_callback(dev, KEY_CANCEL, console_impostazioni_insert_hour_keycanc, NULL);
  if(!CONSOLE->support_idx ||
    (CONSOLE->support_idx == CONSOLE->impostazioni_buf_digits))
      console_register_callback(dev, KEY_ENTER, CONSOLE->impostazioni_enter, NULL);
}

static CALLBACK void* console_impostazioni_timerange3(ProtDevice *dev, int press, void *null)
{
  char cmd[8];
  
  if(CONSOLE->impostazioni_buf2[0])
    sprintf(cmd, "0%.4s", CONSOLE->impostazioni_buf2);
  else
    cmd[0] = '2';
  cmd_set_timerange(CONSOLE->impostazioni_range, cmd);
  
  if(CONSOLE->impostazioni_buf1[0])
    sprintf(cmd, "1%.4s", CONSOLE->impostazioni_buf1);
  else
    cmd[0] = '3';
  cmd_set_timerange(CONSOLE->impostazioni_range, cmd);
  
  console_send(dev, "ATS00\r");
  console_send(dev, str_2_0);
  console_send(dev, str_2_1);
  
  CONSOLE->last_menu_item = console_impostazioni_timerange;
  timeout_on(CONSOLE->timeout, (timeout_func)console_return_to_last_menu_item, dev, 0, TIMEOUT_MSG);

  return NULL;
}

static CALLBACK void* console_impostazioni_timerange2(ProtDevice *dev, int press, void *null)
{
  char cmd[32];
  
  memcpy(CONSOLE->impostazioni_buf2, CONSOLE->impostazioni_buf1, 6);

  console_send(dev, str_2_2);
  
  if(FAGR[CONSOLE->impostazioni_range][2] != 255)
  {
    sprintf(cmd, "%02d%02d", FAGR[CONSOLE->impostazioni_range][2], FAGR[CONSOLE->impostazioni_range][3]);
    memcpy(CONSOLE->impostazioni_buf1, cmd, 4);
  }
  else
    CONSOLE->impostazioni_buf1[0] = 0;
  console_impostazioni_insert_hour(dev, 6, 8, 0, console_impostazioni_timerange3, console_impostazioni_timerange);

  return NULL;
}

static CALLBACK void* console_impostazioni_timerange1(ProtDevice *dev, int press, void *range_as_voidp)
{
  int range = (int)range_as_voidp;
  char cmd[32];
  
  console_send(dev, "ATS03\r");
  sprintf(cmd, str_2_3, range);
  console_send(dev, cmd);
  console_send(dev, str_2_4);
  
  CONSOLE->impostazioni_range = range;
  if(FAGR[CONSOLE->impostazioni_range][0] != 255)
  {
    sprintf(cmd, "%02d%02d", FAGR[range][0], FAGR[range][1]);
    memcpy(CONSOLE->impostazioni_buf1, cmd, 4);
  }
  else
    CONSOLE->impostazioni_buf1[0] = 0;
  console_impostazioni_insert_hour(dev, 6, 6, 0, console_impostazioni_timerange2, console_impostazioni_timerange);

  return NULL;
}

CALLBACK void* console_impostazioni_timerange(ProtDevice *dev, int press, void *null)
{
  int i;
  char range[16];
  
  console_disable_menu(dev, str_2_5);

  console_list_free(CONSOLE->support_list);
  CONSOLE->support_list = NULL;
  
  for(i=0; i<n_FAG; i++)
  {
    sprintf(range, str_2_6, i);
    CONSOLE->support_list = console_list_add(CONSOLE->support_list, range, NULL, console_impostazioni_timerange1, i);
  }
  
  console_list_show(dev, CONSOLE->support_list, 0, 5, 0);
  
  return NULL;
}

static char **console_daytype[3] = {&str_2_7, &str_2_8, &str_2_9};
static int console_selected_day = 0;

static CALLBACK void* console_impostazioni_holidays3(ProtDevice *dev, int press, void *daytype_as_voidp)
{
  cmd_set_day_type(console_selected_day, (int)daytype_as_voidp);
  
  console_send(dev, "ATS00\r");
  console_send(dev, str_2_10);
  console_send(dev, str_2_11);
  
  CONSOLE->last_menu_item = console_impostazioni_holidays;
  timeout_on(CONSOLE->timeout, (timeout_func)console_return_to_last_menu_item, dev, 0, TIMEOUT_MSG);

  return NULL;
}

static CALLBACK void* console_impostazioni_holidays2(ProtDevice *dev, int press, void *daytype_as_voidp)
{
  int i;
  char cmd[32];

  for(i=0; i<3; i++)
  {
    if((int)daytype_as_voidp == i) console_send(dev, "ATR1\r");
    if(CONSOLE->tipo == CONSOLE_STL01)
      sprintf(cmd, "ATN08%02d%s\r", i + 4, *console_daytype[i]);
    else
      sprintf(cmd, "ATN060%02d0  %s\t\r", i + 4, *console_daytype[i]);
    console_send(dev, cmd);
    if((int)daytype_as_voidp == i) console_send(dev, "ATR0\r");
  }

  if((int)daytype_as_voidp)
    console_register_callback(dev, KEY_UP, console_impostazioni_holidays2, daytype_as_voidp-1);
  if((int)daytype_as_voidp != 2)
    console_register_callback(dev, KEY_DOWN, console_impostazioni_holidays2, daytype_as_voidp+1);
  console_register_callback(dev, KEY_ENTER, console_impostazioni_holidays3, daytype_as_voidp);
  console_register_callback(dev, KEY_CANCEL, console_impostazioni_holidays, NULL);
  
  return NULL;
}

static CALLBACK void* console_impostazioni_holidays1(ProtDevice *dev, int press, void *day_as_voidp)
{
  char *date, cmd[32];
  time_t t;
  
  console_selected_day = (int)day_as_voidp;
  
  t = time(NULL) + ((int)(day_as_voidp+1) * 86400);
  date = console_format_date(dev, t);
  
  console_send(dev, "ATS03\r");
  if(CONSOLE->tipo == CONSOLE_STL01)
    sprintf(cmd, "ATN0104%s\r", date);
  else
    sprintf(cmd, "ATN008040%s\r", date);
  console_send(dev, cmd);
  
  free(date);
  
  console_impostazioni_holidays2(dev, 0, (void*)((int)FESTIVI[console_selected_day]));
  
  return NULL;
}

CALLBACK void* console_impostazioni_holidays(ProtDevice *dev, int press, void *null)
{
  int i;
  time_t t;
  char *date, daydescr[18];
  
  console_disable_menu(dev, str_2_12);

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
    CONSOLE->support_list = console_list_add(CONSOLE->support_list, daydescr, NULL, console_impostazioni_holidays1, i);
    free(date);
  }

  console_list_show(dev, CONSOLE->support_list, 0, 6, 0);
  
  return NULL;
}

static CALLBACK void* console_impostazioni_datehour_date2(ProtDevice *dev, int press, void *null)
{
  char buf[16];
  
  if(CONSOLE->impostazioni_buf1[0])
  {
    memcpy(buf+1, CONSOLE->impostazioni_buf1, 6);
    if(console_localization == 1)
    {
      /* Inverto giorno e mese */
      memcpy(buf+8, CONSOLE->impostazioni_buf1, 4);
      memcpy(buf+1, buf+18, 2);
      memcpy(buf+3, buf+16, 2);
    }
    cmd_set_date(buf);
    console_send(dev, "ATS00\r");
    console_send(dev, str_2_13);
    console_send(dev, str_2_14);

    CONSOLE->last_menu_item = console_impostazioni_datehour;
    timeout_on(CONSOLE->timeout, (timeout_func)console_return_to_last_menu_item, dev, 0, TIMEOUT_MSG);
  }
  else
    console_impostazioni_datehour(dev, 0, NULL);
  
  return NULL;
}

static CALLBACK void* console_impostazioni_datehour_date1(ProtDevice *dev, int press, void *null)
{
  console_send(dev, "ATS03\r");
  console_send(dev, str_2_15);
  
  CONSOLE->impostazioni_buf1[0] = 0;
  console_impostazioni_insert_date(dev, 5, 6, 1, console_impostazioni_datehour_date2, console_impostazioni_datehour);

  return NULL;
}

static CALLBACK void* console_impostazioni_datehour_hour2(ProtDevice *dev, int press, void *null)
{
  if(CONSOLE->impostazioni_buf1[0])
  {
    support_log("Variata ora da terminale");
    cmd_set_time(CONSOLE->impostazioni_buf1);
    console_send(dev, "ATS00\r");
    console_send(dev, str_2_16);
    console_send(dev, str_2_17);

    CONSOLE->last_menu_item = console_impostazioni_datehour;
    timeout_on(CONSOLE->timeout, (timeout_func)console_return_to_last_menu_item, dev, 0, TIMEOUT_MSG);
  }
  else
    console_impostazioni_datehour(dev, 0, NULL);
  
  return NULL;
}

static CALLBACK void* console_impostazioni_datehour_hour1(ProtDevice *dev, int press, void *null)
{
  console_send(dev, "ATS03\r");
  console_send(dev, str_2_18);
  
  CONSOLE->impostazioni_buf1[0] = 0;
  console_impostazioni_insert_hour(dev, 5, 6, 1, console_impostazioni_datehour_hour2, console_impostazioni_datehour);

  return NULL;
}

static CALLBACK void* console_impostazioni_datehour_hour(ProtDevice *dev, int press, void *null);

static CALLBACK void* console_impostazioni_datehour_date(ProtDevice *dev, int press, void *null)
{
  console_send(dev, "ATR1\r");
  console_send(dev, str_2_19);
  console_send(dev, "ATR0\r");
  console_send(dev, str_2_20);
  
  console_register_callback(dev, KEY_DOWN, console_impostazioni_datehour_hour, NULL);
  console_register_callback(dev, KEY_ENTER, console_impostazioni_datehour_date1, NULL);
  console_register_callback(dev, KEY_CANCEL, console_show_menu, NULL);
  
  return NULL;
}

static CALLBACK void* console_impostazioni_datehour_hour(ProtDevice *dev, int press, void *null)
{
  console_send(dev, str_2_19);
  console_send(dev, "ATR1\r");
  console_send(dev, str_2_20);
  console_send(dev, "ATR0\r");
  
  console_register_callback(dev, KEY_UP, console_impostazioni_datehour_date, NULL);
  console_register_callback(dev, KEY_ENTER, console_impostazioni_datehour_hour1, NULL);
  console_register_callback(dev, KEY_CANCEL, console_show_menu, NULL);
  
  return NULL;
}

CALLBACK void* console_impostazioni_datehour(ProtDevice *dev, int press, void *null)
{
  console_disable_menu(dev, str_2_23);
  
  console_send(dev, str_2_24);
  
  console_impostazioni_datehour_date(dev, 0, NULL);
  
  return NULL;
}

static CALLBACK void* console_impostazioni_zonesens1(ProtDevice *dev, int press, void *sens_as_voidp);

static CALLBACK void* console_impostazioni_zonesens2(ProtDevice *dev, int zone_as_int, void *sens_as_voidp)
{
  int zone = -1;
  int oldzone, oldzi, newzi, n, i;
  short *p;
  unsigned char *list;
  
  console_send(dev, "ATS00\r");
  console_send(dev, str_2_25);

  sscanf((char*)zone_as_int, "%d", &zone);
  if((zone <= 0) || (zone > n_ZS))
  {
    console_send(dev, str_2_26);

    CONSOLE->last_menu_item = console_impostazioni_zonesens1;
    timeout_on(CONSOLE->timeout, (timeout_func)console_return_to_last_menu_item, dev, 0, TIMEOUT_MSG);
    
    return NULL;
  }
  
  database_lock();
  oldzone = Zona_SE[(int)sens_as_voidp];
  for(oldzi=0; oldzi<n_ZI; oldzi++)
  {
    list = TPDZI[oldzi];
    while(list && (*list != 0xff) && (*list != oldzone)) list++;
    if(list && (*list == oldzone)) break;
  }
  for(newzi=0; newzi<n_ZI; newzi++)
  {
    list = TPDZI[newzi];
    while(list && (*list != 0xff) && (*list != zone)) list++;
    if(list && (*list == zone)) break;
  }
  
  if(oldzone != 0xff)
  {
    for(n=0; SE_ZONA[oldzone][n]!=-1; n++);
    p = malloc(n*sizeof(short));
    p[n-1] = -1;
    i = 0;
    for(n=0; SE_ZONA[oldzone][n]!=-1; n++)
      if(SE_ZONA[oldzone][n] != (int)sens_as_voidp)
        p[i++] = SE_ZONA[oldzone][n];
    free(SE_ZONA[oldzone]);
    SE_ZONA[oldzone] = p;
  }
  if(zone != 0xff)
  {
    for(n=0; SE_ZONA[zone][n]!=-1; n++);
    p = malloc((n+2)*sizeof(short));
    memcpy(p, SE_ZONA[zone], n*sizeof(short));
    p[n] = (int)sens_as_voidp;
    p[n+1] = -1;
    free(SE_ZONA[zone]);
    SE_ZONA[zone] = p;
  }
  
  if(oldzi != newzi)
  {
    if(oldzi != n_ZI)
    {
      oldzi += 1+n_ZS;
      for(n=0; SE_ZONA[oldzi][n]!=-1; n++);
      p = malloc(n*sizeof(short));
      p[n-1] = -1;
      i = 0;
      for(n=0; SE_ZONA[oldzi][n]!=-1; n++)
        if(SE_ZONA[oldzi][n] != (int)sens_as_voidp)
          p[i++] = SE_ZONA[oldzi][n];
      free(SE_ZONA[oldzi]);
      SE_ZONA[oldzi] = p;
    }
    else
      oldzi = 0xff;
      
    if(newzi != n_ZI)
    {
      newzi += 1+n_ZS;
      for(n=0; SE_ZONA[newzi][n]!=-1; n++);
      p = malloc((n+2)*sizeof(short));
      memcpy(p, SE_ZONA[newzi], n*sizeof(short));
      p[n] = (int)sens_as_voidp;
      p[n+1] = -1;
      free(SE_ZONA[newzi]);
      SE_ZONA[newzi] = p;
    }
    else
      newzi = 0xff;
    
    i = 0;
    list = TPDZT;
    while(list && (*list != 0xff))
    {
      if((*list == Zona_SE[(int)sens_as_voidp]) || (*list == oldzi)) i |= 1;
      if((*list == zone) || (*list == newzi)) i |= 2;
      list++;
    }
    if(i == 1)
    {
      /* il sensore non è più presente nella zona totale */
      for(n=0; SE_ZONA[0][n]!=-1; n++);
      p = malloc(n*sizeof(short));
      p[n-1] = -1;
      i = 0;
      for(n=0; SE_ZONA[0][n]!=-1; n++)
        if(SE_ZONA[0][n] != (int)sens_as_voidp)
          p[i++] = SE_ZONA[0][n];
      free(SE_ZONA[0]);
      SE_ZONA[0] = p;
    }
    else if(i == 2)
    {
      /* il sensore ora è in zona totale */
      for(n=0; SE_ZONA[0][n]!=-1; n++);
      p = malloc((n+2)*sizeof(short));
      memcpy(p, SE_ZONA[0], n*sizeof(short));
      p[n] = (int)sens_as_voidp;
      p[n+1] = -1;
      free(SE_ZONA[0]);
      SE_ZONA[0] = p;
    }
  }
  
  Zona_SE[(int)sens_as_voidp] = zone;
  database_changed = 1;
  database_unlock();
  
  console_send(dev, str_2_33);

  timeout_on(CONSOLE->timeout, (timeout_func)console_return_to_menu, dev, 0, TIMEOUT_MSG);
  
  return NULL;
}

static CALLBACK void* console_impostazioni_zonesens1(ProtDevice *dev, int press, void *sens_as_voidp)
{
  char *sens_cod, *sens_desc, cmd[32];
  
  console_send(dev, "ATS03\r");
  
  string_sensor_name((int)sens_as_voidp, &sens_cod, &sens_desc);
  sprintf(cmd, str_2_57, sens_cod);
  console_send(dev, cmd);
  sprintf(cmd, str_2_58, sens_desc);
  console_send(dev, cmd);

  console_send(dev, str_2_59);
  console_send(dev, str_2_60);
  console_send(dev, "ATL111\r");

  console_register_callback(dev, KEY_STRING, console_impostazioni_zonesens2, sens_as_voidp);
  console_register_callback(dev, KEY_CANCEL, console_impostazioni_zonesens, NULL);
  
  return NULL;
}

CALLBACK void* console_impostazioni_zonesens(ProtDevice *dev, int press, void *null)
{
  char *sens_cod, *sens_desc;
  int i, j, num;
  
  console_disable_menu(dev, str_2_34);

  console_send(dev, str_2_35);

  console_list_free(CONSOLE->support_list);
  CONSOLE->support_list = NULL;

  for(i=0; i<n_SE;)
  {
//    if(master_periph_present(i>>3))
    {
      num = master_sensor_valid(i);
      for(j=0; j<num; j++)
      {
        string_sensor_name(i, &sens_cod, &sens_desc);
        CONSOLE->support_list = console_list_add(CONSOLE->support_list, sens_cod, sens_desc,
            console_impostazioni_zonesens1, i++);
      }
      i += 8 - num;
    }
//    else
//      i += 8;
  }

  console_list_show(dev, CONSOLE->support_list, 0, 2, 0);

  return NULL;
}

static CALLBACK void* console_impostazioni_zoneassoc1(ProtDevice *dev, int press, void *sens_as_voidp);

static CALLBACK void* console_impostazioni_zoneassoc2(ProtDevice *dev, int zone_as_int, void *zone_as_voidp)
{
  int i, j, n, dim, zone = -1;
  unsigned char *list;
  short *p;
  
  console_send(dev, "ATS00\r");
  console_send(dev, str_2_36);

  sscanf((char*)zone_as_int, "%d", &zone);
  if((zone < 0) || (zone >= n_ZI))
  {
    console_send(dev, str_2_37);

    CONSOLE->last_menu_item = console_impostazioni_zoneassoc1;
    timeout_on(CONSOLE->timeout, (timeout_func)console_return_to_last_menu_item, dev, 0, TIMEOUT_MSG);
    
    return NULL;
  }
  
  database_lock();
  
  /* delete simple zone from the owning group zone */
  for(i=0; i<n_ZI; i++)
  {
    if(TPDZI[i])
    {
      for(dim=0; (TPDZI[i][dim] != 0xff) && (TPDZI[i][dim] != (int)zone_as_voidp); dim++);
      if(TPDZI[i][dim] == (int)zone_as_voidp)
      {
        for(; !dim || (TPDZI[i][dim-1] != 0xff); dim++) TPDZI[i][dim] = TPDZI[i][dim+1];
        
        /* ricalcola la lista sensori della vecchia zona insieme */
        n = 0;
        list = TPDZI[i];
        while(*list != 0xff)
        {
          for(j=0; SE_ZONA[*list][j]!=-1; j++) n++;
          list++;
        }
        p = malloc((n+1)*sizeof(short));
        p[n] = -1;
        n = 0;
        list = TPDZI[i];
        while(*list != 0xff)
        {
          for(j=0; SE_ZONA[*list][j]!=-1; j++)
            p[n++] = SE_ZONA[*list][j];
          list++;
        }
        free(SE_ZONA[i+n_ZS+1]);
        SE_ZONA[i+n_ZS+1] = p;
      }
    }
  }
  
  /* add simple zone to the new group zone */
  if(TPDZI[zone])
  {
    for(dim=0; TPDZI[zone][dim] != 0xff; dim++);
    list = (unsigned char*)malloc(dim+2);
    memcpy(list, TPDZI[zone], dim);
  }
  else
  {
    dim = 0;
    list = (unsigned char*)malloc(dim+2);
  }
  
  list[dim] = (int)zone_as_voidp;
  list[dim+1] = 0xff;
  
  free(TPDZI[zone]);
  TPDZI[zone] = list;
  
  /* ricalcola la lista sensori della nuova zona insieme */
  n = 0;
  while(*list != 0xff)
  {
    for(j=0; SE_ZONA[*list][j]!=-1; j++) n++;
    list++;
  }
  p = malloc((n+1)*sizeof(short));
  p[n] = -1;
  n = 0;
  list = TPDZI[zone];
  while(*list != 0xff)
  {
    for(j=0; SE_ZONA[*list][j]!=-1; j++)
      p[n++] = SE_ZONA[*list][j];
    list++;
  }
  free(SE_ZONA[zone]);
  SE_ZONA[zone] = p;
  
  /* ricalcola la lista sensori della zona totale */
  n = 0;
  list = TPDZT;
  while(*list != 0xff)
  {
    for(j=0; SE_ZONA[*list][j]!=-1; j++) n++;
    list++;
  }
  p = malloc((n+1)*sizeof(short));
  p[n] = -1;
  n = 0;
  list = TPDZT;
  while(*list != 0xff)
  {
    for(j=0; SE_ZONA[*list][j]!=-1; j++)
      p[n++] = SE_ZONA[*list][j];
    list++;
  }
  free(SE_ZONA[0]);
  SE_ZONA[0] = p;
  
  database_changed = 1;
  
  database_unlock();
  
  console_send(dev, str_2_38);

  timeout_on(CONSOLE->timeout, (timeout_func)console_return_to_menu, dev, 0, TIMEOUT_MSG);

  return NULL;
}

static CALLBACK void* console_impostazioni_zoneassoc1(ProtDevice *dev, int press, void *zone_as_voidp)
{
  char *zone_cod, *zone_desc, cmd[32];
  
  console_send(dev, "ATS03\r");
  
  string_zone_name((int)zone_as_voidp, &zone_cod, &zone_desc);
  sprintf(cmd, str_2_57, zone_cod);
  console_send(dev, cmd);
  sprintf(cmd, str_2_58, zone_desc);
  console_send(dev, cmd);

  console_send(dev, str_2_61);
  console_send(dev, str_2_60);
  console_send(dev, "ATL111\r");

  console_register_callback(dev, KEY_STRING, console_impostazioni_zoneassoc2, zone_as_voidp);
  console_register_callback(dev, KEY_CANCEL, console_impostazioni_zoneassoc, NULL);
  
  return NULL;
}

CALLBACK void* console_impostazioni_zoneassoc(ProtDevice *dev, int press, void *null)
{
  char *zone_cod, *zone_desc;
  char zs[n_ZS+1];
  int i;
  
  console_disable_menu(dev, "ZS/ZI");

  console_send(dev, str_2_39);
  
  memset(zs, 0, n_ZS+1);
  for(i=0; i<n_SE; i++)
    if(Zona_SE[i] <= n_ZS) zs[Zona_SE[i]] = 1;
    
  console_list_free(CONSOLE->support_list);
  CONSOLE->support_list = NULL;
  
  for(i=1; i<=n_ZS; i++)
  {
    if(zs[i])
    {
      string_zone_name(i, &zone_cod, &zone_desc);
      CONSOLE->support_list = console_list_add(CONSOLE->support_list, zone_cod, zone_desc,
          console_impostazioni_zoneassoc1, i);
    }
  }

  console_list_show(dev, CONSOLE->support_list, 0, 2, 0);

  return NULL;
}

static CALLBACK void* console_impostazioni_zonerit2(ProtDevice *dev, int rit_as_int, void *zone_as_voidp)
{
  int rit = -1;
  
  console_send(dev, "ATS00\r");
  console_send(dev, str_2_40);

  sscanf((char*)rit_as_int, "%d", &rit);
  if((rit < 0) || (rit > 60000))
  {
    console_send(dev, str_2_41);

    CONSOLE->last_menu_item = console_impostazioni_zonerit;
    timeout_on(CONSOLE->timeout, (timeout_func)console_return_to_last_menu_item, dev, 0, TIMEOUT_MSG);
    
    return NULL;
  }
  
  RitardoZona((int)zone_as_voidp, rit);
  
  console_send(dev, str_2_42);
  
  timeout_on(CONSOLE->timeout, (timeout_func)console_return_to_menu, dev, 0, TIMEOUT_MSG);
  
  return NULL;
}

static CALLBACK void* console_impostazioni_zonerit1(ProtDevice *dev, int press, void *zone_as_voidp)
{
  char *zone_cod, *zone_desc, cmd[32];
  
  console_send(dev, "ATS03\r");
  
  string_zone_name((int)zone_as_voidp, &zone_cod, &zone_desc);
  sprintf(cmd, str_2_57, zone_cod);
  console_send(dev, cmd);
  sprintf(cmd, str_2_58, zone_desc);
  console_send(dev, cmd);

  console_send(dev, str_2_43);
  console_send(dev, str_2_62);
  sprintf(cmd, "ATL111%d\r", ZONA_R[(int)zone_as_voidp]/10);
  console_send(dev, cmd);

  console_register_callback(dev, KEY_STRING, console_impostazioni_zonerit2, zone_as_voidp);
  console_register_callback(dev, KEY_CANCEL, console_impostazioni_zonerit, NULL);
  
  return NULL;
}

CALLBACK void* console_impostazioni_zonerit(ProtDevice *dev, int press, void *null)
{
  char *zone_cod, *zone_desc;
  char zs[n_ZS+1];
  int i;
  
  console_disable_menu(dev, str_2_44);

  console_send(dev, str_2_45);
  
  memset(zs, 0, n_ZS+1);
  for(i=0; i<n_SE; i++)
    if(Zona_SE[i] <= n_ZS) zs[Zona_SE[i]] = 1;
    
  console_list_free(CONSOLE->support_list);
  CONSOLE->support_list = NULL;
  
  for(i=1; i<=n_ZS; i++)
  {
    if(zs[i])
    {
      string_zone_name(i, &zone_cod, &zone_desc);
      CONSOLE->support_list = console_list_add(CONSOLE->support_list, zone_cod, zone_desc,
          console_impostazioni_zonerit1, i);
    }
  }

  console_list_show(dev, CONSOLE->support_list, 0, 2, 0);

  return NULL;
}

static CALLBACK void* console_impostazioni_timeout1(ProtDevice *dev, int time_as_int, void *null)
{
  char cmd[16];
  int time;
  
  time = -1;
  sscanf((char*)time_as_int, "%d", &time);

  console_send(dev, "ATF0\r");
  console_send(dev, "ATS00\r");

  if((time == -1) || (time > 9999))
  {
    console_send(dev, str_2_46);
    console_send(dev, str_2_47);

    CONSOLE->last_menu_item = console_impostazioni_timeout;
    timeout_on(CONSOLE->timeout, (timeout_func)console_return_to_last_menu_item, dev, 0, TIMEOUT_MSG);
  }
  else
  {
    console_send(dev, str_2_48);
    console_send(dev, str_2_49);

    console_login_timeout_time = time * 10;
    sprintf(cmd, "ATE%04d\r", time);
    console_send(dev, cmd);
    
    timeout_on(CONSOLE->timeout, (timeout_func)console_return_to_menu, dev, 0, TIMEOUT_MSG);
  }
  
  return NULL;
}

CALLBACK void* console_impostazioni_timeout(ProtDevice *dev, int press, void *null)
{
  char cmd[16];

  console_disable_menu(dev, str_2_50);

  console_send(dev, str_2_51);
  console_send(dev, str_2_63);
  console_send(dev, "ATF1\r");
  sprintf(cmd, "ATL111%d\r", console_login_timeout_time/10);
  console_send(dev, cmd);
  
  console_register_callback(dev, KEY_STRING, console_impostazioni_timeout1, NULL);
  console_register_callback(dev, KEY_CANCEL, console_show_menu, NULL);
  
  return NULL;
}

CALLBACK void* console_impostazioni_codes(ProtDevice *dev, int press, void *null);

static CALLBACK void* console_impostazioni_codes2(ProtDevice *dev, int code_as_int, void *code_as_voip)
{
  int code_idx = (int)code_as_voip;
  int code;
  
  code = -1;
  
  sscanf((char*)code_as_int, "%d", &code);
  
  if(code >= 0)
  {
    ((short*)RONDA)[code_idx] = code;
    database_changed = 1;
    console_impostazioni_codes(dev, 0, NULL);
  }
  else
  {
    console_send(dev, "ATS07\r");
    console_send(dev, str_2_52);
    CONSOLE->last_menu_item = console_impostazioni_codes;
    timeout_on(CONSOLE->timeout, (timeout_func)console_return_to_last_menu_item, dev, 0, TIMEOUT_MSG);
  }
  
  return NULL;
}

static CALLBACK void* console_impostazioni_codes1(ProtDevice *dev, int press, void *code_as_voip)
{
  int code = (int)code_as_voip;
  char *cod, *cod_desc, cmd[32];
  
  console_send(dev, "ATS03\r");
  
  console_send(dev, "ATL111\r");
  string_code_name(code, &cod, &cod_desc);
  sprintf(cmd, str_2_57, cod);
  console_send(dev, cmd);
  sprintf(cmd, str_2_58, cod_desc);
  console_send(dev, cmd);
  
  console_send(dev, str_2_64);
  console_send(dev, str_2_65);
  
  console_register_callback(dev, KEY_STRING, console_impostazioni_codes2, code_as_voip);
  console_register_callback(dev, KEY_CANCEL, console_impostazioni_codes, NULL);
  
  return NULL;
}

CALLBACK void* console_impostazioni_codes(ProtDevice *dev, int press, void *null)
{
  char *cod, *cod_desc;
  int i;
  
  console_disable_menu(dev, str_2_53);

  console_send(dev, "ATL000\r");
  
  console_list_free(CONSOLE->support_list);
  CONSOLE->support_list = NULL;
  
  for(i=0; i<(sizeof(RONDA)/sizeof(short)); i++)
  {
    string_code_name(i, &cod, &cod_desc);
    CONSOLE->support_list = console_list_add(CONSOLE->support_list, cod, cod_desc,
        console_impostazioni_codes1, i);
  }

  console_list_show(dev, CONSOLE->support_list, 0, 3, 0);
  
  return NULL;
}

CALLBACK void* console_impostazioni_network1(ProtDevice *dev, int num_as_int, void *step)
{
  int num;
  FILE *fp1, *fp2;
  char cmd[256];
  
  if((int)step)
  {
    num = -1;
    sscanf((char*)num_as_int, "%d", &num);
  }
  
  switch((int)step)
  {
    case 0:
      console_send(dev, "ATS03\r");
      console_send(dev, str_2_66);
      console_send(dev, "ATL111\r");
      console_send(dev, str_2_67);
      console_send(dev, str_2_68);
      break;
    case 1:
      CONSOLE->impostazioni_buf1[0] = num;
      sprintf(cmd, str_2_69, num&0xff);
      console_send(dev, cmd);
      console_send(dev, str_2_70);
      console_send(dev, str_2_71);
      break;
    case 2:
      CONSOLE->impostazioni_buf1[1] = num;
      sprintf(cmd, str_2_72, num&0xff);
      console_send(dev, cmd);
      console_send(dev, str_2_73);
      console_send(dev, str_2_74);
      break;
    case 3:
      CONSOLE->impostazioni_buf1[2] = num;
      sprintf(cmd, str_2_75, num&0xff);
      console_send(dev, cmd);
      console_send(dev, str_2_76);
      console_send(dev, str_2_77);
      break;
    case 4:
      CONSOLE->impostazioni_buf1[3] = num;
      sprintf(cmd, str_2_78, num&0xff);
      console_send(dev, cmd);
      console_send(dev, str_2_79);
      console_send(dev, str_2_80);
      console_send(dev, str_2_81);
      break;
    case 5:
      CONSOLE->impostazioni_buf1[4] = num;
      sprintf(cmd, str_2_82, num&0xff);
      console_send(dev, cmd);
      console_send(dev, str_2_83);
      console_send(dev, str_2_84);
      break;
    case 6:
      CONSOLE->impostazioni_buf1[5] = num;
      sprintf(cmd, str_2_85, num&0xff);
      console_send(dev, cmd);
      console_send(dev, str_2_86);
      console_send(dev, str_2_87);
      break;
    case 7:
      CONSOLE->impostazioni_buf2[0] = num;
      sprintf(cmd, str_2_88, num&0xff);
      console_send(dev, cmd);
      console_send(dev, str_2_89);
      console_send(dev, str_2_90);
      break;
    case 8:
      CONSOLE->impostazioni_buf2[1] = num;
      sprintf(cmd, str_2_91, num&0xff);
      console_send(dev, cmd);
      console_send(dev, str_2_92);
      console_send(dev, str_2_93);
      console_send(dev, str_2_94);
      break;
    case 9:
      CONSOLE->impostazioni_buf2[2] = num;
      sprintf(cmd, str_2_95, num&0xff);
      console_send(dev, cmd);
      console_send(dev, str_2_96);
      console_send(dev, str_2_97);
      break;
    case 10:
      CONSOLE->impostazioni_buf2[3] = num;
      sprintf(cmd, str_2_98, num&0xff);
      console_send(dev, cmd);
      console_send(dev, str_2_99);
      console_send(dev, str_2_100);
      break;
    case 11:
      CONSOLE->impostazioni_buf2[4] = num;
      sprintf(cmd, str_2_101, num&0xff);
      console_send(dev, cmd);
      console_send(dev, str_2_102);
      console_send(dev, str_2_103);
      break;
    case 12:
      CONSOLE->impostazioni_buf2[5] = num;
      sprintf(cmd, str_2_104, num&0xff);
      console_send(dev, cmd);
      
      fp1 = fopen("/tmp/network.conf", "w");
      fp2 = fopen("/etc/network/network.conf", "r");
      
      while(fgets(cmd, 256, fp2))
      {
        if(!strncmp(cmd, "BOOTPROTO=", 10))
        {
          fprintf(fp1, "BOOTPROTO=\"none\"\n");
        }
        else if(!strncmp(cmd, "IP=", 3))
        {
          fprintf(fp1, "IP=\"%d.%d.%d.%d\"\n",
            (unsigned char)CONSOLE->impostazioni_buf1[0],
            (unsigned char)CONSOLE->impostazioni_buf1[1],
            (unsigned char)CONSOLE->impostazioni_buf1[2],
            (unsigned char)CONSOLE->impostazioni_buf1[3]);
        }
        else if(!strncmp(cmd, "NETMASK=", 8))
        {
          fprintf(fp1, "NETMASK=\"%d.%d.%d.%d\"\n",
            (unsigned char)CONSOLE->impostazioni_buf1[4],
            (unsigned char)CONSOLE->impostazioni_buf1[5],
            (unsigned char)CONSOLE->impostazioni_buf2[0],
            (unsigned char)CONSOLE->impostazioni_buf2[1]);
        }
        else if(!strncmp(cmd, "GATEWAY=", 8))
        {
          fprintf(fp1, "GATEWAY=\"%d.%d.%d.%d\"\n",
            (unsigned char)CONSOLE->impostazioni_buf2[2],
            (unsigned char)CONSOLE->impostazioni_buf2[3],
            (unsigned char)CONSOLE->impostazioni_buf2[4],
            (unsigned char)CONSOLE->impostazioni_buf2[5]);
        }
        else if(!strncmp(cmd, "NETWORK=", 8))
        {
          fprintf(fp1, "NETWORK=\"%d.%d.%d.%d\"\n",
            (unsigned char)(CONSOLE->impostazioni_buf1[0]&CONSOLE->impostazioni_buf1[4]),
            (unsigned char)(CONSOLE->impostazioni_buf1[1]&CONSOLE->impostazioni_buf1[5]),
            (unsigned char)(CONSOLE->impostazioni_buf1[2]&CONSOLE->impostazioni_buf2[0]),
            (unsigned char)(CONSOLE->impostazioni_buf1[3]&CONSOLE->impostazioni_buf2[1]));
        }
        else if(!strncmp(cmd, "BROADCAST=", 10))
        {
          fprintf(fp1, "BROADCAST=\"%d.%d.%d.%d\"\n",
            (unsigned char)(CONSOLE->impostazioni_buf1[0]|~CONSOLE->impostazioni_buf1[4]),
            (unsigned char)(CONSOLE->impostazioni_buf1[1]|~CONSOLE->impostazioni_buf1[5]),
            (unsigned char)(CONSOLE->impostazioni_buf1[2]|~CONSOLE->impostazioni_buf2[0]),
            (unsigned char)(CONSOLE->impostazioni_buf1[3]|~CONSOLE->impostazioni_buf2[1]));
        }
        else
        {
          fprintf(fp1, cmd);
        }
      }
      fclose(fp1);
      fclose(fp2);
      
      console_send(dev, "ATL000\r");
      console_send(dev, "ATS03\r");
      console_send(dev, str_2_55);
      console_send(dev, str_2_56);
      timeout_on(CONSOLE->timeout, (timeout_func)console_return_to_menu, dev, 0, TIMEOUT_MSG);
      delphi_restart_network();
      
      return NULL;
      break;
    default:
      break;
  }
  
  console_register_callback(dev, KEY_STRING, console_impostazioni_network1, (void*)(step+1));
  console_register_callback(dev, KEY_CANCEL, console_show_menu, NULL);
  return NULL;
}

CALLBACK void* console_impostazioni_network(ProtDevice *dev, int press, void *null)
{
  FILE *fp;
  char buf[256], ip[16], nm[16], gw[16], bootp[16];
  int i;
  
    int s;
    struct ifreq ifr;
    unsigned int addr;
  
  console_disable_menu(dev, str_2_54);
  
  fp = fopen("/etc/network/network.conf", "r");
  while(fgets(buf, 256, fp))
  {
    if(!strncmp(buf, "BOOTPROTO=", 10))
    {
      memcpy(bootp, buf+11, 16);
      for(i=0; i<16; i++)
        if(bootp[i] == '"')
        {
          bootp[i] = 0;
          break;
        }
    }
#if 0
    else if(!strncmp(buf, "IP=", 3))
    {
      memcpy(ip, buf+4, 16);
      for(i=0; i<16; i++)
        if(ip[i] == '"')
        {
          ip[i] = 0;
          break;
        }
    }
    else if(!strncmp(buf, "NETMASK=", 8))
    {
      memcpy(nm, buf+9, 16);
      for(i=0; i<16; i++)
        if(nm[i] == '"')
        {
          nm[i] = 0;
          break;
        }
    }
    else if(!strncmp(buf, "GATEWAY=", 8))
    {
      memcpy(gw, buf+9, 16);
      for(i=0; i<16; i++)
        if(gw[i] == '"')
        {
          gw[i] = 0;
          break;
        }
    }
#endif
  }
  fclose(fp);
  
  strcpy(gw, "0.0.0.0");
  
  fp = fopen("/proc/net/route", "r");
  while(fgets(buf, 256, fp))
  {
    /* Uso temporaneamente questi buffer */
    if(!strncmp(buf, "eth0", 4))
    {
      sscanf(buf+4, "%s %s", ip, nm);
      if(!strcmp(ip, "00000000"))
      {
        sscanf(nm+6, "%02x", &i);
        sprintf(gw, "%d.", i);
        sscanf(nm+4, "%02x", &i);
        sprintf(gw+strlen(gw), "%d.", i);
        sscanf(nm+2, "%02x", &i);
        sprintf(gw+strlen(gw), "%d.", i);
        sscanf(nm, "%02x", &i);
        sprintf(gw+strlen(gw), "%d", i);
        break;
      }
    }
  }
  fclose(fp);
  
    s = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
    strcpy(ifr.ifr_name, "eth0");
    
    ioctl(s, SIOCGIFADDR, &ifr);
    addr = ((struct sockaddr_in*)(&(ifr.ifr_addr)))->sin_addr.s_addr;
    sprintf(ip, "%d.%d.%d.%d", addr&0xff, (addr>>8)&0xff, (addr>>16)&0xff, (addr>>24)&0xff);
    ioctl(s, SIOCGIFNETMASK, &ifr);
    addr = ((struct sockaddr_in*)(&(ifr.ifr_addr)))->sin_addr.s_addr;
    sprintf(nm, "%d.%d.%d.%d", addr&0xff, (addr>>8)&0xff, (addr>>16)&0xff, (addr>>24)&0xff);
    
    close(s);
  
  if(!strcmp(bootp, "dhcp"))
  {
    
    console_send(dev, str_2_105);
    sprintf(buf, str_2_106, ip);
    console_send(dev, buf);
    console_send(dev, str_2_111);
  }
  else
  {
    console_send(dev, str_2_105);
    sprintf(buf, str_2_106, ip);
    console_send(dev, buf);
    
    console_send(dev, str_2_107);
    sprintf(buf, str_2_108, nm);
    console_send(dev, buf);
    
    console_send(dev, str_2_109);
    sprintf(buf, str_2_110, gw);
    console_send(dev, buf);
  }
  
  console_register_callback(dev, KEY_ENTER, console_impostazioni_network1, NULL);
  console_register_callback(dev, KEY_CANCEL, console_show_menu, NULL);
  
  return NULL;
}


