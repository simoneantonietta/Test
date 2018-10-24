#include "console.h"
#include "con_lara_impostazioni.h"
#include "con_impostazioni.h"
#include "../lara.h"
#include "../strings.h"
#include "../codec.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/***********************************************************************
	LARA IMPOSTAZIONI menu functions
***********************************************************************/

static CALLBACK void* console_lara_impostazioni_fosett_show1(ProtDevice *dev, int press, void *range_as_voidp)
{
  char cmd[32];
  int range = ((int)range_as_voidp >> 3) - 1;
  int day = (int)range_as_voidp & 0x07;
  
  console_send(dev, "ATS05\r");
  
  switch(day)
  {
    case 0: console_send(dev, str_4_0); break;
    case 1: console_send(dev, str_4_1); break;
    case 2: console_send(dev, str_4_2); break;
    case 3: console_send(dev, str_4_3); break;
    case 4: console_send(dev, str_4_4); break;
    case 5: console_send(dev, str_4_5); break;
    case 6: console_send(dev, str_4_6); break;
  }
  
  if(CONSOLE->tipo == CONSOLE_STL01)
  {
    sprintf(cmd, "ATN0107 I) ");
    if(_lara->fasciaoraria[range].fascia[day][0][0] != 0xff)
      sprintf(cmd+11, "%02d:%02d-", _lara->fasciaoraria[range].fascia[day][0][0],
        _lara->fasciaoraria[range].fascia[day][0][1]);
    else
      sprintf(cmd+11, " -:- -");
    if(_lara->fasciaoraria[range].fascia[day][0][2] != 0xff)
      sprintf(cmd+17, "%02d:%02d\r", _lara->fasciaoraria[range].fascia[day][0][2],
        _lara->fasciaoraria[range].fascia[day][0][3]);
    else
      sprintf(cmd+17, " -:- \r");
    console_send(dev, cmd);
    
    sprintf(cmd, "ATN0108II) ");
    if(_lara->fasciaoraria[range].fascia[day][1][0] != 0xff)
      sprintf(cmd+11, "%02d:%02d-", _lara->fasciaoraria[range].fascia[day][1][0],
        _lara->fasciaoraria[range].fascia[day][1][1]);
    else
      sprintf(cmd+11, " -:- -");
    if(_lara->fasciaoraria[range].fascia[day][1][2] != 0xff)
      sprintf(cmd+17, "%02d:%02d\r", _lara->fasciaoraria[range].fascia[day][1][2],
        _lara->fasciaoraria[range].fascia[day][1][3]);
    else
      sprintf(cmd+17, " -:- \r");
    console_send(dev, cmd);
  }
  else
  {
    console_send(dev, "ATN022070 I)\r");
    if(_lara->fasciaoraria[range].fascia[day][0][0] != 0xff)
      sprintf(cmd, "ATN053071%02d:%02d\r", _lara->fasciaoraria[range].fascia[day][0][0],
        _lara->fasciaoraria[range].fascia[day][0][1]);
    else
      sprintf(cmd, "ATN053071-:-\r");
    console_send(dev, cmd);
    console_send(dev, "ATN075070-\r");
    if(_lara->fasciaoraria[range].fascia[day][0][2] != 0xff)
      sprintf(cmd, "ATN101071%02d:%02d\r", _lara->fasciaoraria[range].fascia[day][0][2],
        _lara->fasciaoraria[range].fascia[day][0][3]);
    else
      sprintf(cmd, "ATN101071-:-\r");
    console_send(dev, cmd);
    
    console_send(dev, "ATN022080II)\r");
    if(_lara->fasciaoraria[range].fascia[day][1][0] != 0xff)
      sprintf(cmd, "ATN053081%02d:%02d\r", _lara->fasciaoraria[range].fascia[day][1][0],
        _lara->fasciaoraria[range].fascia[day][1][1]);
    else
      sprintf(cmd, "ATN053081-:-\r");
    console_send(dev, cmd);
    console_send(dev, "ATN075080-\r");
    if(_lara->fasciaoraria[range].fascia[day][1][2] != 0xff)
      sprintf(cmd, "ATN101081%02d:%02d\r", _lara->fasciaoraria[range].fascia[day][1][2],
        _lara->fasciaoraria[range].fascia[day][1][3]);
    else
      sprintf(cmd, "ATN101081-:-\r");
    console_send(dev, cmd);
  }
  console_register_callback(dev, KEY_CANCEL, console_lara_impostazioni_fosett_show, (void*)(range+1));
  
  return NULL;
}

CALLBACK void* console_lara_impostazioni_fosett_show(ProtDevice *dev, int press, void *fosett_as_voidp)
{
  char cmd[32];
  
  console_list_free(CONSOLE->support_list);
  CONSOLE->support_list = NULL;

  console_send(dev, "ATS04\r");
  
  if(fosett_as_voidp && _lara)
  {
    sprintf(cmd, str_4_7, (int)fosett_as_voidp);
    console_send(dev, cmd);

    CONSOLE->support_list = console_list_add(CONSOLE->support_list, str_4_8, NULL,
      console_lara_impostazioni_fosett_show1, ((int)fosett_as_voidp << 3) + 1);
    CONSOLE->support_list = console_list_add(CONSOLE->support_list, str_4_9, NULL,
      console_lara_impostazioni_fosett_show1, ((int)fosett_as_voidp << 3) + 2);
    CONSOLE->support_list = console_list_add(CONSOLE->support_list, str_4_10, NULL,
      console_lara_impostazioni_fosett_show1, ((int)fosett_as_voidp << 3) + 3);
    CONSOLE->support_list = console_list_add(CONSOLE->support_list, str_4_11, NULL,
      console_lara_impostazioni_fosett_show1, ((int)fosett_as_voidp << 3) + 4);
    CONSOLE->support_list = console_list_add(CONSOLE->support_list, str_4_12, NULL,
      console_lara_impostazioni_fosett_show1, ((int)fosett_as_voidp << 3) + 5);
    CONSOLE->support_list = console_list_add(CONSOLE->support_list, str_4_13, NULL,
      console_lara_impostazioni_fosett_show1, ((int)fosett_as_voidp << 3) + 6);
    CONSOLE->support_list = console_list_add(CONSOLE->support_list, str_4_14, NULL,
      console_lara_impostazioni_fosett_show1, ((int)fosett_as_voidp << 3) + 0);
    
    console_list_show(dev, CONSOLE->support_list, 0, 3, 0);
  }
  else
  {
    console_send(dev, str_4_15);
    console_register_callback(dev, KEY_CANCEL, CONSOLE->list_show_cancel, NULL);
  }
    
  return NULL;
}

static CALLBACK void* console_lara_impostazioni_holiday_show1(ProtDevice *dev, int press, void *fest_as_voidp)
{
  char *fest_cod, *fest_desc, cmd[32];
  int i0, i1, i2, f0, f1, f2, t;
  
  i0 = _lara->festivo[(int)fest_as_voidp].periodo.inizio[0];
  i1 = _lara->festivo[(int)fest_as_voidp].periodo.inizio[1];
  i2 = _lara->festivo[(int)fest_as_voidp].periodo.inizio[2];
  f0 = _lara->festivo[(int)fest_as_voidp].periodo.fine[0];
  f1 = _lara->festivo[(int)fest_as_voidp].periodo.fine[1];
  f2 = _lara->festivo[(int)fest_as_voidp].periodo.fine[2];
  if(console_localization == 1)
  {
    /* Inverto giorno e mese */
    t = i0;
    i0 = i1;
    i1 = t;
    t = f0;
    f0 = f1;
    f1 = t;
  }

  console_send(dev, "ATS04\r");
  
  fest_as_voidp--;
  
  string_holiday_name((int)fest_as_voidp, &fest_cod, &fest_desc);
  sprintf(cmd, str_4_58, fest_cod);
  console_send(dev, cmd);
  sprintf(cmd, str_4_59, fest_desc);
  console_send(dev, cmd);
  
  switch(_lara->festivo[(int)fest_as_voidp].tipofestivo)
  {
    case 0:
      console_send(dev, str_4_16);
      break;
    case 2:
      console_send(dev, str_4_17);
      sprintf(cmd, str_4_60, i0, i1, i2);
      console_send(dev, cmd);
      break;
    case 3:
      console_send(dev, str_4_18);
      sprintf(cmd, str_4_19, i0, i1, i2);
      console_send(dev, cmd);
      sprintf(cmd, str_4_20, f0, f1, f2);
      console_send(dev, cmd);
      break;
    case 6:
      console_send(dev, str_4_21);
      sprintf(cmd, str_4_61, i0, i1);
      console_send(dev, cmd);
      break;
    case 7:
      console_send(dev, str_4_22);
      sprintf(cmd, str_4_23, i0, i1);
      console_send(dev, cmd);
      sprintf(cmd, str_4_24, f0, f1);
      console_send(dev, cmd);
      break;
    default:
      break;
  }
  
  console_register_callback(dev, KEY_CANCEL, console_lara_impostazioni_holiday_show, (void*)(CONSOLE->impostazioni_range));
  return NULL;
}

CALLBACK void* console_lara_impostazioni_holiday_show(ProtDevice *dev, int press, void *hdays_ad_voidp)
{
  char *fest_cod, *fest_desc;
  int i;
  
  CONSOLE->impostazioni_range = (int)hdays_ad_voidp;
  
  console_list_free(CONSOLE->support_list);
  CONSOLE->support_list = NULL;

  console_send(dev, "ATS04\r");
  console_send(dev, str_4_25);

  for(i=0; i<LARA_N_FESTIVI; i++)
  {
    if(((char*)hdays_ad_voidp)[i>>3] & (1<<(i & 0x07)))
    {
      string_holiday_name(i, &fest_cod, &fest_desc);
      CONSOLE->support_list = console_list_add(CONSOLE->support_list, fest_cod, fest_desc,
        console_lara_impostazioni_holiday_show1, i+1);
    }
  }

  console_list_show(dev, CONSOLE->support_list, 0, 2, 0);
  
  return NULL;
}

/***********************************************************************
	FASCE ORARIE SETTIMANALI
***********************************************************************/

static CALLBACK void* console_lara_impostazioni_weektiming1(ProtDevice *dev, int press, void *fosett_as_voidp);
static CALLBACK void* console_lara_impostazioni_weektiming2(ProtDevice *dev, int press, void *fosett_as_voidp);

static CALLBACK void* console_lara_impostazioni_weektiming4(ProtDevice *dev, int press, void *null)
{
  if(CONSOLE->pos)
  {
    unsigned char ev[4];
    
    ev[0] = 247;
    ev[1] = 0;
    ev[2] = 54;
    ev[3] = (CONSOLE->impostazioni_range >> 3);
    codec_queue_event(ev);
  }
  console_lara_impostazioni_weektiming2(dev, 0, (void*)(CONSOLE->impostazioni_range));
  return NULL;
}

static CALLBACK void* console_lara_impostazioni_weektiming3(ProtDevice *dev, int press, void *null)
{
  int range = (CONSOLE->impostazioni_range >> 3) - 1;
  int day = CONSOLE->impostazioni_range & 0x07;
  int pos = CONSOLE->pos - 1;
  
  if(CONSOLE->pos)
  {
    if(CONSOLE->impostazioni_buf1[0])
    {
      _lara->fasciaoraria[range].fascia[day][pos>>1][(pos&0x01)<<1] = GVAL2(CONSOLE->impostazioni_buf1);
      _lara->fasciaoraria[range].fascia[day][pos>>1][((pos&0x01)<<1)+1] = GVAL2(CONSOLE->impostazioni_buf1+2);
    }
    else
    {
      _lara->fasciaoraria[range].fascia[day][pos>>1][(pos&0x01)<<1] = 0xff;
      _lara->fasciaoraria[range].fascia[day][pos>>1][((pos&0x01)<<1)+1] = 0xff;
      switch(CONSOLE->pos)
      {
        case 1: console_send(dev, str_4_62); break;
        case 2: console_send(dev, str_4_63); break;
        case 3: console_send(dev, str_4_64); break;
        case 4: console_send(dev, str_4_65); break;
      }
    }
    _laraf->fasciaoraria[range] = 1;
//    lara_substitute_timings();
    lara_save(0);    
        
    if(CONSOLE->pos == 4)
    {
      unsigned char ev[4];
      
      ev[0] = 247;
      ev[1] = 0;
      ev[2] = 54;
      ev[3] = range+1;
      codec_queue_event(ev);
      console_lara_impostazioni_weektiming1(dev, 0, (void*)(range+1));
    }
  }
  
  CONSOLE->pos++;
  CONSOLE->impostazioni_buf1[0] = 0;
  
  switch(CONSOLE->pos)
  {
    case 1:
      console_impostazioni_insert_hour(dev, 5, 7, 0,
        console_lara_impostazioni_weektiming3, console_lara_impostazioni_weektiming4);
      break;
    case 2:
      console_impostazioni_insert_hour(dev, 11, 7, 0,
        console_lara_impostazioni_weektiming3, console_lara_impostazioni_weektiming4);
      break;
    case 3:
      console_impostazioni_insert_hour(dev, 5, 8, 0,
        console_lara_impostazioni_weektiming3, console_lara_impostazioni_weektiming4);
      break;
    case 4:
      console_impostazioni_insert_hour(dev, 11, 8, 0,
        console_lara_impostazioni_weektiming3, console_lara_impostazioni_weektiming4);
      break;
  }
  
  return NULL;
}

static CALLBACK void* console_lara_impostazioni_weektiming2(ProtDevice *dev, int press, void *range_as_voidp)
{
  char cmd[32];
  int range = ((int)range_as_voidp >> 3) - 1;
  int day = (int)range_as_voidp & 0x07;
  
  console_send(dev, "ATS05\r");
  
  switch(day)
  {
    case 0: console_send(dev, str_4_0); break;
    case 1: console_send(dev, str_4_1); break;
    case 2: console_send(dev, str_4_2); break;
    case 3: console_send(dev, str_4_3); break;
    case 4: console_send(dev, str_4_4); break;
    case 5: console_send(dev, str_4_5); break;
    case 6: console_send(dev, str_4_6); break;
  }
  
  if(CONSOLE->tipo == CONSOLE_STL01)
  {
    sprintf(cmd, "ATN0107 I) ");
    if(_lara->fasciaoraria[range].fascia[day][0][0] != 0xff)
      sprintf(cmd+11, "%02d:%02d-", _lara->fasciaoraria[range].fascia[day][0][0],
        _lara->fasciaoraria[range].fascia[day][0][1]);
    else
      sprintf(cmd+11, " -:- -");
    if(_lara->fasciaoraria[range].fascia[day][0][2] != 0xff)
      sprintf(cmd+17, "%02d:%02d\r", _lara->fasciaoraria[range].fascia[day][0][2],
        _lara->fasciaoraria[range].fascia[day][0][3]);
    else
      sprintf(cmd+17, " -:- \r");
    console_send(dev, cmd);
    
    sprintf(cmd, "ATN0108II) ");
    if(_lara->fasciaoraria[range].fascia[day][1][0] != 0xff)
      sprintf(cmd+11, "%02d:%02d-", _lara->fasciaoraria[range].fascia[day][1][0],
        _lara->fasciaoraria[range].fascia[day][1][1]);
    else
      sprintf(cmd+11, " -:- -");
    if(_lara->fasciaoraria[range].fascia[day][1][2] != 0xff)
      sprintf(cmd+17, "%02d:%02d\r", _lara->fasciaoraria[range].fascia[day][1][2],
        _lara->fasciaoraria[range].fascia[day][1][3]);
    else
      sprintf(cmd+17, " -:- \r");
    console_send(dev, cmd);
  }
  else
  {
    console_send(dev, "ATN022070 I)\r");
    if(_lara->fasciaoraria[range].fascia[day][0][0] != 0xff)
      sprintf(cmd, "ATN053071%02d:%02d\r", _lara->fasciaoraria[range].fascia[day][0][0],
        _lara->fasciaoraria[range].fascia[day][0][1]);
    else
      sprintf(cmd, "ATN053071-:-\r");
    console_send(dev, cmd);
    console_send(dev, "ATN075070-\r");
    if(_lara->fasciaoraria[range].fascia[day][0][2] != 0xff)
      sprintf(cmd, "ATN101071%02d:%02d\r", _lara->fasciaoraria[range].fascia[day][0][2],
        _lara->fasciaoraria[range].fascia[day][0][3]);
    else
      sprintf(cmd, "ATN101071-:-\r");
    console_send(dev, cmd);
    
    console_send(dev, "ATN022080II)\r");
    if(_lara->fasciaoraria[range].fascia[day][1][0] != 0xff)
      sprintf(cmd, "ATN053081%02d:%02d\r", _lara->fasciaoraria[range].fascia[day][1][0],
        _lara->fasciaoraria[range].fascia[day][1][1]);
    else
      sprintf(cmd, "ATN053081-:-\r");
    console_send(dev, cmd);
    console_send(dev, "ATN075080-\r");
    if(_lara->fasciaoraria[range].fascia[day][1][2] != 0xff)
      sprintf(cmd, "ATN101081%02d:%02d\r", _lara->fasciaoraria[range].fascia[day][1][2],
        _lara->fasciaoraria[range].fascia[day][1][3]);
    else
      sprintf(cmd, "ATN101081-:-\r");
    console_send(dev, cmd);
  }
  
  CONSOLE->pos = 0;
  CONSOLE->impostazioni_range = (int)range_as_voidp;
  
  console_register_callback(dev, KEY_ENTER, console_lara_impostazioni_weektiming3, NULL);
  console_register_callback(dev, KEY_CANCEL, console_lara_impostazioni_weektiming1, (void*)(range+1));
  
  return NULL;
}

static CALLBACK void* console_lara_impostazioni_weektiming1(ProtDevice *dev, int press, void *fosett_as_voidp)
{
  char cmd[32];
  
  console_list_free(CONSOLE->support_list);
  CONSOLE->support_list = NULL;

  console_send(dev, "ATS03\r");
  
  sprintf(cmd, str_4_33, (int)fosett_as_voidp);
  console_send(dev, cmd);

  CONSOLE->support_list = console_list_add(CONSOLE->support_list, str_4_8, NULL,
    console_lara_impostazioni_weektiming2, ((int)fosett_as_voidp << 3) + 1);
  CONSOLE->support_list = console_list_add(CONSOLE->support_list, str_4_9, NULL,
    console_lara_impostazioni_weektiming2, ((int)fosett_as_voidp << 3) + 2);
  CONSOLE->support_list = console_list_add(CONSOLE->support_list, str_4_10, NULL,
    console_lara_impostazioni_weektiming2, ((int)fosett_as_voidp << 3) + 3);
  CONSOLE->support_list = console_list_add(CONSOLE->support_list, str_4_11, NULL,
    console_lara_impostazioni_weektiming2, ((int)fosett_as_voidp << 3) + 4);
  CONSOLE->support_list = console_list_add(CONSOLE->support_list, str_4_12, NULL,
    console_lara_impostazioni_weektiming2, ((int)fosett_as_voidp << 3) + 5);
  CONSOLE->support_list = console_list_add(CONSOLE->support_list, str_4_13, NULL,
    console_lara_impostazioni_weektiming2, ((int)fosett_as_voidp << 3) + 6);
  CONSOLE->support_list = console_list_add(CONSOLE->support_list, str_4_14, NULL,
    console_lara_impostazioni_weektiming2, ((int)fosett_as_voidp << 3) + 0);
    
  CONSOLE->list_show_cancel = console_lara_impostazioni_weektiming;
  console_list_show(dev, CONSOLE->support_list, 0, 4, 0);

  return NULL;
}

CALLBACK void* console_lara_impostazioni_weektiming(ProtDevice *dev, int press, void *null)
{
  char *wtmg_cod, *wtmg_desc;
  int i;

  console_disable_menu(dev, str_4_41);

  console_list_free(CONSOLE->support_list);
  CONSOLE->support_list = NULL;
  
  if(_lara)
  {
    for(i=1; i<=LARA_N_FASCE; i++)
    {
      string_weektiming_name(i-1, &wtmg_cod, &wtmg_desc);
      CONSOLE->support_list = console_list_add(CONSOLE->support_list, wtmg_cod, wtmg_desc,
        console_lara_impostazioni_weektiming1, i);
    }
  }
  
  CONSOLE->list_show_cancel = console_show_menu;
  console_list_show(dev, CONSOLE->support_list, 0, 3, 0);

  return NULL;
}

/***********************************************************************
	FESTIVITA'
***********************************************************************/

static CALLBACK void* console_lara_impostazioni_holidays1bis(ProtDevice *dev, int press, void *fest_as_voidp);
static CALLBACK void* console_lara_impostazioni_holidays2(ProtDevice *dev, int press, void *null);

static CALLBACK void* console_lara_impostazioni_holidays9(ProtDevice *dev, int press, void *set)
{
  int i;
  
  if(set)
    for(i=0; i<LARA_N_TERMINALI; i++)
      *(int*)(_lara->terminale[i].fest) |= (1L << (CONSOLE->impostazioni_range-1));
  
  console_send(dev, "ATS03\r");
  console_lara_impostazioni_holiday_show1(dev, 0, ((void*)CONSOLE->impostazioni_range));
  console_register_callback(dev, KEY_ENTER, console_lara_impostazioni_holidays2, NULL);
  console_register_callback(dev, KEY_CANCEL, console_lara_impostazioni_holidays, NULL);
  
  return NULL;
}

static CALLBACK void* console_lara_impostazioni_holidays8(ProtDevice *dev, int press, void *set)
{
  int i;
  
  if(set)
    for(i=0; i<LARA_N_PROFILI; i++)
      *(int*)(_lara->profilo[i].fest) |= (1L << (CONSOLE->impostazioni_range-1));
  
  console_send(dev, str_4_42);
  console_register_callback(dev, KEY_ENTER, console_lara_impostazioni_holidays9, (void*)1);
  console_register_callback(dev, KEY_CANCEL, console_lara_impostazioni_holidays9, NULL);
  return NULL;
}

static CALLBACK void* console_lara_impostazioni_holidays7(ProtDevice *dev, int press, void *set)
{
  int i;
  
  if(set)
    for(i=1; i<BADGE_NUM; i++)
      *(int*)(_lara->tessera[i].p.s.fest) |= (1L << (CONSOLE->impostazioni_range-1));
  
  console_send(dev, str_4_43);
  console_send(dev, str_4_44);
  console_register_callback(dev, KEY_ENTER, console_lara_impostazioni_holidays8, (void*)1);
  console_register_callback(dev, KEY_CANCEL, console_lara_impostazioni_holidays8, NULL);
  return NULL;
}

static CALLBACK void* console_lara_impostazioni_holidays6(ProtDevice *dev, int press, void *null)
{
  console_send(dev, "ATS03\r");
  console_send(dev, str_4_45);
  console_send(dev, str_4_46);
  console_send(dev, str_4_47);
  
  console_send(dev, "ATR1\r");
  console_send(dev, str_4_48);
  console_send(dev, "ATR0\r");
  console_send(dev, str_4_49);
  console_send(dev, "ATR1\r");
  console_send(dev, str_4_50);
  console_send(dev, "ATR0\r");
  console_send(dev, str_4_51);
  
  console_register_callback(dev, KEY_ENTER, console_lara_impostazioni_holidays7, (void*)1);
  console_register_callback(dev, KEY_CANCEL, console_lara_impostazioni_holidays7, NULL);
  return NULL;
}

static CALLBACK void* console_lara_impostazioni_holidays5(ProtDevice *dev, int press, void *null)
{
  unsigned char ev[4];
  
  ev[0] = 247;
  ev[1] = 0;
  ev[2] = 55;
  ev[3] = CONSOLE->impostazioni_range;
  codec_queue_event(ev);
  
  if(console_localization == 1)
  {
    /* Inverto giorno e mese */
    _lara->festivo[CONSOLE->impostazioni_range-1].periodo.fine[1] = GVAL2(CONSOLE->impostazioni_buf1);
    _lara->festivo[CONSOLE->impostazioni_range-1].periodo.fine[0] = GVAL2(CONSOLE->impostazioni_buf1+2);
  }
  else
  {
    _lara->festivo[CONSOLE->impostazioni_range-1].periodo.fine[0] = GVAL2(CONSOLE->impostazioni_buf1);
    _lara->festivo[CONSOLE->impostazioni_range-1].periodo.fine[1] = GVAL2(CONSOLE->impostazioni_buf1+2);
  }
  _lara->festivo[CONSOLE->impostazioni_range-1].periodo.fine[2] = GVAL2(CONSOLE->impostazioni_buf1+4);
  _laraf->festivo[CONSOLE->impostazioni_range-1] = 1;
  lara_save(0);
  
  console_lara_impostazioni_holidays6(dev, 0, NULL);
  return NULL;
}

static CALLBACK void* console_lara_impostazioni_holidays4(ProtDevice *dev, int press, void *null)
{
  unsigned char ev[4];
  
  ev[0] = 247;
  ev[1] = 0;
  ev[2] = 55;
  ev[3] = CONSOLE->impostazioni_range;
  codec_queue_event(ev);
  
  if(console_localization == 1)
  {
    /* Inverto giorno e mese */
    _lara->festivo[CONSOLE->impostazioni_range-1].periodo.inizio[1] = GVAL2(CONSOLE->impostazioni_buf1);
    _lara->festivo[CONSOLE->impostazioni_range-1].periodo.inizio[0] = GVAL2(CONSOLE->impostazioni_buf1+2);
  }
  else
  {
    _lara->festivo[CONSOLE->impostazioni_range-1].periodo.inizio[0] = GVAL2(CONSOLE->impostazioni_buf1);
    _lara->festivo[CONSOLE->impostazioni_range-1].periodo.inizio[1] = GVAL2(CONSOLE->impostazioni_buf1+2);
  }
  _lara->festivo[CONSOLE->impostazioni_range-1].periodo.inizio[2] = GVAL2(CONSOLE->impostazioni_buf1+4);
  _laraf->festivo[CONSOLE->impostazioni_range-1] = 1;
  lara_save(0);
  
  CONSOLE->impostazioni_buf1[0] = 0;
  
  if(_lara->festivo[CONSOLE->impostazioni_range-1].tipofestivo & 0x01)
  {
    if(_lara->festivo[CONSOLE->impostazioni_range-1].tipofestivo & 0x04)
      console_impostazioni_insert_date(dev, 6, 8, 0,
        console_lara_impostazioni_holidays5, console_lara_impostazioni_holidays2);
    else
      console_impostazioni_insert_date(dev, 5, 8, 1,
        console_lara_impostazioni_holidays5, console_lara_impostazioni_holidays2);
  }
  else
    console_lara_impostazioni_holidays6(dev, 0, NULL);
    
  return NULL;
}

static CALLBACK void* console_lara_impostazioni_holidays3(ProtDevice *dev, int press, void *tipo_as_voidp)
{
  int i;
  unsigned char ev[4];
  
  console_send(dev, "ATS06\r");
  
  ev[0] = 247;
  ev[1] = 0;
  ev[2] = 55;
  ev[3] = CONSOLE->impostazioni_range;
  codec_queue_event(ev);
  
  _lara->festivo[CONSOLE->impostazioni_range-1].tipofestivo = (int)tipo_as_voidp;
  _laraf->festivo[CONSOLE->impostazioni_range-1] = 1;
  lara_save(0);
  
  CONSOLE->impostazioni_buf1[0] = 0;
  
  if(!(int)tipo_as_voidp)
  {
    sprintf(ev, "%02d", CONSOLE->impostazioni_range);
    free(HolidayName[CONSOLE->impostazioni_range-1]);
    HolidayName[CONSOLE->impostazioni_range-1] = strdup(ev);
    string_save();
    
    for(i=0; i<LARA_N_PROFILI; i++)
      *(int*)(_lara->profilo[i].fest) &= ~(1L << (CONSOLE->impostazioni_range-1));
    for(i=0; i<BADGE_NUM; i++)
      *(int*)(_lara->tessera[i].p.s.fest) &= ~(1L << (CONSOLE->impostazioni_range-1));
    for(i=0; i<LARA_N_TERMINALI; i++)
      *(int*)(_lara->terminale[i].fest) &= ~(1L << (CONSOLE->impostazioni_range-1));
    
    console_lara_impostazioni_holidays1bis(dev, 0, NULL);
  }
  else if((int)tipo_as_voidp & 0x04)
    console_impostazioni_insert_date(dev, 6, 7, 0,
      console_lara_impostazioni_holidays4, console_lara_impostazioni_holidays2);
  else
    console_impostazioni_insert_date(dev, 5, 7, 1,
      console_lara_impostazioni_holidays4, console_lara_impostazioni_holidays2);
  
  return NULL;
}

static CALLBACK void* console_lara_impostazioni_holidays2(ProtDevice *dev, int press, void *null)
{
  console_send(dev, "ATS06\r");
  
  console_list_free(CONSOLE->support_list);
  CONSOLE->support_list = NULL;
  
  CONSOLE->support_list = console_list_add(CONSOLE->support_list, str_4_52, NULL,
    console_lara_impostazioni_holidays3, 0);
  CONSOLE->support_list = console_list_add(CONSOLE->support_list, str_4_53, NULL,
    console_lara_impostazioni_holidays3, 2);
  CONSOLE->support_list = console_list_add(CONSOLE->support_list, str_4_54, NULL,
    console_lara_impostazioni_holidays3, 3);
  CONSOLE->support_list = console_list_add(CONSOLE->support_list, str_4_55, NULL,
    console_lara_impostazioni_holidays3, 6);
  CONSOLE->support_list = console_list_add(CONSOLE->support_list, str_4_56, NULL,
    console_lara_impostazioni_holidays3, 7);
  
  CONSOLE->list_show_cancel = console_lara_impostazioni_holidays1bis;
  console_list_show(dev, CONSOLE->support_list, 0, 3, 0);
  
  return NULL;
}

static CALLBACK void* console_lara_impostazioni_holidays1bis(ProtDevice *dev, int press, void *null)
{
  console_send(dev, "ATS03\r");
  console_lara_impostazioni_holiday_show1(dev, 0, ((void*)CONSOLE->impostazioni_range));
  console_register_callback(dev, KEY_ENTER, console_lara_impostazioni_holidays2, NULL);
  console_register_callback(dev, KEY_CANCEL, console_lara_impostazioni_holidays, NULL);
  
  return NULL;
}

static CALLBACK void* console_lara_impostazioni_holidays1(ProtDevice *dev, int press, void *fest_as_voidp)
{
  CONSOLE->impostazioni_range = (int)fest_as_voidp;
  console_lara_impostazioni_holidays1bis(dev, 0, NULL);
  
  return NULL;
}

CALLBACK void* console_lara_impostazioni_holidays(ProtDevice *dev, int press, void *null)
{
  char *hday_cod, *hday_desc;
  int i;
  
  console_disable_menu(dev, str_4_57);

  console_list_free(CONSOLE->support_list);
  CONSOLE->support_list = NULL;
  
  if(_lara)
  {
    for(i=0; i<LARA_N_FESTIVI; i++)
    {
      string_holiday_name(i, &hday_cod, &hday_desc);
      CONSOLE->support_list = console_list_add(CONSOLE->support_list, hday_cod, hday_desc,
        console_lara_impostazioni_holidays1, i+1);
    }
  }
  
  CONSOLE->list_show_cancel = console_show_menu;
  console_list_show(dev, CONSOLE->support_list, 0, 3, 0);

  return NULL;
}
