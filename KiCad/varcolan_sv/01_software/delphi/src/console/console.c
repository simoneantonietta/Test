#include "console.h"
#include "../command.h"
#include "../event.h"
#include "../serial.h"
#include "../support.h"
#include "../protocol.h"
#include "../database.h"
#include "../strings.h"
#include "../lara.h"
#include "../master.h"
#include "../delphi.h"
#include <unistd.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <semaphore.h>
#include <sys/ioctl.h>

#include "con_zone.h"
#include "con_liste.h"
#include "con_telecomandi.h"
#include "con_sensori.h"
#include "con_attuatori.h"
#include "con_sms.h"
#include "con_impostazioni.h"
#include "con_personalizza.h"
#include "con_badge.h"
#include "con_modem.h"
#include "con_sicurezza.h"
#include "con_inoltra.h"
#include "con_storico.h"

#include "con_lara_pass.h"
#include "con_lara_term.h"
#include "con_lara_profilo.h"
#include "con_lara_impostazioni.h"

#define EMULATE_MENU 0
#if EMULATE_MENU
#warning Navigazione menù emulata!
#endif

char *str_empty = "";
extern void *delphi_lang;

static const char ConsoleUserFileName[] = "/saet/console.pwd";

static User generic_user = {"", NULL, CONSOLE_TOP_LEVEL, CONSOLE_TOP_LEVEL};
static pthread_mutex_t console_mutex = PTHREAD_MUTEX_INITIALIZER;

int console_login_timeout_time = TIMEOUT_LOGIN;

User *console_user_list = NULL;
int console_localization = 0;

static CALLBACK void* console_main(ProtDevice *dev, int press, void *null);
CALLBACK void* console_logout(ProtDevice *dev, int press, void *null);
static CALLBACK void* console_logout_ack(ProtDevice *dev, int press, void *null);
#ifdef TEBESA
static CALLBACK void* console_stop(ProtDevice *dev, int press, void *null);
#endif

static MenuItem console_menu_dummy[] = {
	{NULL, 0, NULL}
};

static MenuItem console_menu_zone[] = {
	{&str_14_0, L1 | L2 | L3 | L4 | L5 | L6 | L7 | L8, console_zone_deactivate},
	{&str_14_1, L1 | L2 | L3 | L4 | L5 | L6 | L7 | L8, console_zone_activate},
	{&str_14_2, L1 | L2 | L3 | L4 | L5 | L6 | L7 | L8, console_zone_activate_w_alm},
	{&str_14_3, L1 | L2 | L3 | L4 | L5 | L6 | L7 | L8, console_zone_activate_w_oos},
	{NULL, 0, NULL}
};

static MenuItem console_menu_liste[] = {
	{&str_14_4, L1 | L2 | L3 | L4 | L5 | L6 | L7 | L8, console_liste_sens_alm},
	{&str_14_5, L1 | L2 | L3 | L4 | L5 | L6 | L7 | L8, console_liste_sens_almMU},
	{&str_14_6, L2 | L3 | L4 | L5 | L6 | L7 | L8, console_liste_sens_oos},
	{&str_14_7, L1 | L2 | L3 | L4 | L5 | L6 | L7 | L8, console_liste_sens_failure},
	{&str_14_8, L1 | L2 | L3 | L4 | L5 | L6 | L7 | L8, console_liste_sens_sabotage},
	{&str_14_9, L5 | L6 | L7 | L8, console_liste_zone_active},
	{&str_14_10, L5 | L6 | L7 | L8, console_liste_zone_not_active},
	{&str_14_11, L5 | L6 | L7 | L8, console_liste_act_on},
	{&str_14_12, L5 | L6 | L7 | L8, console_liste_act_oos},
	{&str_14_13, L5 | L6 | L7 | L8, console_liste_called_numbers},
	{&str_14_14, L5 | L6 | L7 | L8, console_liste_holidays},
	{NULL, 0, NULL}
};

static MenuItem console_menu_telecomandi[] = {
	{&str_14_15, L1 | L2 | L3 | L4 | L5 | L6 | L7 | L8, console_telecomandi_send},
	{&str_14_16, L8, console_telecomandi_peripherals},
	{&str_14_17, L8, console_telecomandi_sensorzonelist},
	{NULL, 0, NULL}
};

static MenuItem console_menu_sensori[] = {
	{&str_14_18, L1 | L2 | L3 | L4 | L5 | L6 | L7 | L8, console_sensori_out_of_service},
	{&str_14_19, L1 | L2 | L3 | L4 | L5 | L6 | L7 | L8, console_sensori_on_service},
	{&str_14_20, L1 | L2 | L3 | L4 | L5 | L6 | L7 | L8, console_sensori_accept},
	{&str_14_21, L1 | L2 | L3 | L4 | L5 | L6 | L7 | L8, console_sensori_accept_all},
	{NULL, 0, NULL}
};

static MenuItem console_menu_attuatori[] = {
	{&str_14_22, L1 | L2 | L3 | L4 | L5 | L6 | L7 | L8, console_attuatori_out_of_service},
	{&str_14_23, L1 | L2 | L3 | L4 | L5 | L6 | L7 | L8, console_attuatori_on_service},
	{&str_14_24, L1 | L2 | L3 | L4 | L5 | L6 | L7 | L8, console_attuatori_off},
	{&str_14_25, L1 | L2 | L3 | L4 | L5 | L6 | L7 | L8, console_attuatori_on},
	{NULL, 0, NULL}
};

static MenuItem console_menu_sms[] = {
	{&str_14_26, L1 | L2 | L3 | L4 | L5 | L6 | L7 | L8, console_sms_send},
	{&str_14_27, L1 | L2 | L3 | L4 | L5 | L6 | L7 | L8, console_sms_read},
	{&str_14_28, L1 | L2 | L3 | L4 | L5 | L6 | L7 | L8, console_sms_credit},
	{NULL, 0, NULL}
};

static MenuItem console_menu_impostazioni[] = {
	{&str_14_29, L1 | L2 | L3 | L4 | L5 | L6 | L7 | L8, console_impostazioni_timerange},
	{&str_14_30, L1 | L2 | L3 | L4 | L5 | L6 | L7 | L8, console_impostazioni_holidays},
	{&str_14_31, L1 | L2 | L3 | L4 | L5 | L6 | L7 | L8, console_impostazioni_datehour},
	{&str_14_32, L1 | L2 | L3 | L4 | L5 | L6 | L7 | L8, console_impostazioni_zonesens},
	{&str_14_33, L1 | L2 | L3 | L4 | L5 | L6 | L7 | L8, console_impostazioni_zoneassoc},
	{&str_14_34, L1 | L2 | L3 | L4 | L5 | L6 | L7 | L8, console_impostazioni_zonerit},
	{&str_14_35, L1 | L2 | L3 | L4 | L5 | L6 | L7 | L8, console_impostazioni_timeout},
	{&str_14_36, L1 | L2 | L3 | L4 | L5 | L6 | L7 | L8, console_impostazioni_codes},
	{&str_14_142, L8, console_impostazioni_network},
	{NULL, 0, NULL}
};

static MenuItem console_menu_personalizza[] = {
	{&str_14_37, L1 | L2 | L3 | L4 | L5 | L6 | L7 | L8, console_personalizza_zone},
	{&str_14_38, L1 | L2 | L3 | L4 | L5 | L6 | L7 | L8, console_personalizza_sensor},
	{&str_14_39, L1 | L2 | L3 | L4 | L5 | L6 | L7 | L8, console_personalizza_actuator},
	{&str_14_40, L1 | L2 | L3 | L4 | L5 | L6 | L7 | L8, console_personalizza_phonebook},
	{&str_14_41, L1 | L2 | L3 | L4 | L5 | L6 | L7 | L8, console_personalizza_command},
	{&str_14_42, L1 | L2 | L3 | L4 | L5 | L6 | L7 | L8, console_personalizza_code},
	{NULL, 0, NULL}
};

static MenuItem console_menu_badge[] = {
	{&str_14_43, L1 | L2 | L3 | L4 | L5 | L6 | L7 | L8, console_badge_enable},
	{&str_14_44, L1 | L2 | L3 | L4 | L5 | L6 | L7 | L8, console_badge_disable},
	{NULL, 0, NULL}
};

static MenuItem console_menu_modem[] = {
	{&str_14_45, L1 | L2 | L3 | L4 | L5 | L6 | L7 | L8, console_modem_store},
	{&str_14_46, L1 | L2 | L3 | L4 | L5 | L6 | L7 | L8, console_modem_delete},
	{&str_14_47, L1 | L2 | L3 | L4 | L5 | L6 | L7 | L8, console_modem_enable},
	{&str_14_48, L1 | L2 | L3 | L4 | L5 | L6 | L7 | L8, console_modem_disable},
	{&str_14_49, L1 | L2 | L3 | L4 | L5 | L6 | L7 | L8, console_modem_rec},
	{NULL, 0, NULL}
};

static MenuItem console_menu_sicurezza[] = {
	{&str_14_50, L1 | L2 | L3 | L4 | L5 | L6 | L7 | L8, console_sicurezza_pwd},
	{&str_14_51, L8, console_sicurezza_create},
	{&str_14_52, L8, console_sicurezza_delete},
	{NULL, 0, NULL}
};

static MenuItem console_menu_inoltra[] = {
	{&str_14_53, L1 | L2 | L3 | L4 | L5 | L6 | L7 | L8, console_inoltra_secret},
	{&str_14_54, L1 | L2 | L3 | L4 | L5 | L6 | L7 | L8, console_inoltra_event},
	{NULL, 0, NULL}
};

static MenuItem console_menu_storico[] = {
	{&str_14_55, L1 | L2 | L3 | L4 | L5 | L6 | L7 | L8, console_storico_realtime},
	{&str_14_56, L1 | L2 | L3 | L4 | L5 | L6 | L7 | L8, console_storico_consolidated},
	{&str_14_57, L5 | L6 | L7 | L8, console_storico_delete},
	{NULL, 0, NULL}
};

static MenuItem console_menu_logout[] = {
	{&str_empty, L1 | L2 | L3 | L4 | L5 | L6 | L7 | L8, console_logout_ack},
	{NULL, 0, NULL}
};

static MenuItem console_menu_lara_personalizza[] = {
    /* only for LARA */
	{&str_14_58, L1 | L2 | L3 | L4 | L5 | L6 | L7 | L8, console_personalizza_profile},
	{&str_14_59, L1 | L2 | L3 | L4 | L5 | L6 | L7 | L8, console_personalizza_area},
	{&str_14_60, L1 | L2 | L3 | L4 | L5 | L6 | L7 | L8, console_personalizza_holiday},
	{&str_14_61, L1 | L2 | L3 | L4 | L5 | L6 | L7 | L8, console_personalizza_weektiming},
	{&str_14_62, L1 | L2 | L3 | L4 | L5 | L6 | L7 | L8, console_personalizza_terminal},
	/* common for SYS and LARA */
	{&str_14_63, L1 | L2 | L3 | L4 | L5 | L6 | L7 | L8, console_personalizza_zone},
	{&str_14_64, L1 | L2 | L3 | L4 | L5 | L6 | L7 | L8, console_personalizza_sensor},
	{&str_14_65, L1 | L2 | L3 | L4 | L5 | L6 | L7 | L8, console_personalizza_actuator},
	{&str_14_66, L1 | L2 | L3 | L4 | L5 | L6 | L7 | L8, console_personalizza_phonebook},
	{&str_14_67, L1 | L2 | L3 | L4 | L5 | L6 | L7 | L8, console_personalizza_command},
	{&str_14_68, L1 | L2 | L3 | L4 | L5 | L6 | L7 | L8, console_personalizza_code},
	{NULL, 0, NULL}
};

static MenuItem console_menu_lara_pass[] = {
	{&str_14_69, L1 | L2 | L3 | L4 | L5 | L6 | L7 | L8, console_lara_pass_show},
	{&str_14_70, L7 | L8, console_lara_pass_modify},
	{NULL, 0, NULL}
};

static MenuItem console_menu_lara_terminali[] = {
	{&str_14_71, L1 | L2 | L3 | L4 | L5 | L6 | L7 | L8, console_lara_term_show},
	{&str_14_72, L7 | L8, console_lara_term_modify},
	{NULL, 0, NULL}
};

static MenuItem console_menu_lara_profili[] = {
	{&str_14_73, L1 | L2 | L3 | L4 | L5 | L6 | L7 | L8, console_lara_profile_show},
	{&str_14_74, L7 | L8, console_lara_profile_modify},
	{NULL, 0, NULL}
};

static MenuItem console_menu_lara_impostazioni[] = {
	{&str_14_75, L1 | L2 | L3 | L4 | L5 | L6 | L7 | L8, console_lara_impostazioni_weektiming},
	{&str_14_76, L1 | L2 | L3 | L4 | L5 | L6 | L7 | L8, console_lara_impostazioni_holidays},
	{&str_14_77, L1 | L2 | L3 | L4 | L5 | L6 | L7 | L8, console_impostazioni_datehour},
	{&str_14_78, L1 | L2 | L3 | L4 | L5 | L6 | L7 | L8, console_impostazioni_zonesens},
	{&str_14_79, L1 | L2 | L3 | L4 | L5 | L6 | L7 | L8, console_impostazioni_zoneassoc},
	{&str_14_80, L1 | L2 | L3 | L4 | L5 | L6 | L7 | L8, console_impostazioni_timeout},
	{&str_14_81, L1 | L2 | L3 | L4 | L5 | L6 | L7 | L8, console_impostazioni_codes},
	{&str_14_142, L8, console_impostazioni_network},
	{NULL, 0, NULL}
};

Menu console_menu_delphi[] = {
	{&str_14_82, 1, console_menu_zone, L5 | L6 | L7 | L8},
	{&str_14_83, 2, console_menu_liste, L1 | L2 | L3 | L4 | L5 | L6 | L7 | L8},
	{&str_14_84, 3, console_menu_telecomandi, L3 | L4 | L5 | L6 | L7 | L8},
	{&str_14_85, 4, console_menu_sensori, L5 | L6 | L7 | L8},
	{&str_14_86, 5, console_menu_attuatori, L5 | L6 | L7 | L8},
	{&str_14_87, 6, console_menu_impostazioni, L6 | L7 | L8},
	{&str_14_88, SMS_MENU, console_menu_sms, L2 | L3 | L4 | L5 | L6 | L7 | L8},
	{&str_14_89, 8, console_menu_personalizza, L6 | L7 | L8},
	{&str_14_90, 9, console_menu_badge, L7 | L8},
	{&str_14_91, CHIAVI_MENU, console_menu_dummy, 0},
	{&str_14_109, RADIO_MENU, console_menu_dummy, 0},
	{&str_14_92, 10, console_menu_modem, L7 | L8},
	{&str_14_93, 11, console_menu_sicurezza, L1 | L2 | L3 | L4 | L5 | L6 | L7 | L8},
	{&str_14_94, 12, console_menu_inoltra, L4 | L5 | L6 | L7 | L8},
	{&str_14_95, 13, console_menu_storico, L2 | L3 | L4 | L5 | L6 | L7 | L8},
	{&str_14_96, LOGOUT_MENU, console_menu_logout, L1 | L2 | L3 | L4 | L5 | L6 | L7 | L8},
	{NULL, 0, NULL, 0}
};

static Menu console_menu_lara[] = {
	{&str_14_97, 1, console_menu_zone, L5 | L6 | L7 | L8},
	{&str_14_98, 2, console_menu_liste, L1 | L2 | L3 | L4 | L5 | L6 | L7 | L8},
	{&str_14_99, 3, console_menu_telecomandi, L3 | L4 | L5 | L6 | L7 | L8},
	{&str_14_100, 4, console_menu_sensori, L5 | L6 | L7 | L8},
	{&str_14_101, 5, console_menu_attuatori, L5 | L6 | L7 | L8},
	{&str_14_102, 6, console_menu_lara_impostazioni, L6 | L7 | L8},
	{&str_14_103, SMS_MENU, console_menu_sms, L2 | L3 | L4 | L5 | L6 | L7 | L8},
	{&str_14_104, 8, console_menu_lara_personalizza, L6 | L7 | L8},
	{&str_14_105, 10, console_menu_modem, L7 | L8},
	{&str_14_106, 11, console_menu_sicurezza, L1 | L2 | L3 | L4 | L5 | L6 | L7 | L8},
	{&str_14_107, 12, console_menu_inoltra, L4 | L5 | L6 | L7 | L8},
	{&str_14_108, 13, console_menu_storico, L2 | L3 | L4 | L5 | L6 | L7 | L8},
	{&str_14_109, 15, console_menu_lara_pass, L2 | L3 | L4 | L5 | L6 | L7 | L8},
	{&str_14_110, 17, console_menu_lara_profili, L2 | L3 | L4 | L5 | L6 | L7 | L8},
	{&str_14_111, 16, console_menu_lara_terminali, L2 | L3 | L4 | L5 | L6 | L7 | L8},
	{&str_14_112, LOGOUT_MENU, console_menu_logout, L1 | L2 | L3 | L4 | L5 | L6 | L7 | L8},
	{NULL, 0, NULL, 0}
};

static Menu console_menu_lara_cameri[] = {
	{&str_14_102, 6, console_menu_lara_impostazioni, L6 | L7 | L8},
	{&str_14_106, 11, console_menu_sicurezza, L1 | L2 | L3 | L4 | L5 | L6 | L7 | L8},
	{&str_14_112, LOGOUT_MENU, console_menu_logout, L1 | L2 | L3 | L4 | L5 | L6 | L7 | L8},
	{NULL, 0, NULL, 0}
};

/***********************************************************************
	Generic support functions
***********************************************************************/

char **console_months[] = {&str_14_113, &str_14_114, &str_14_115, &str_14_116, &str_14_117, &str_14_118, &str_14_119, &str_14_120, &str_14_121, &str_14_122, &str_14_123, &str_14_124};

char* console_format_time(ProtDevice *dev, time_t t)
{
  static char buf[24];
  struct tm *dh;

  dh = localtime(&t);
  if(CONSOLE->tipo == CONSOLE_STL02)
  {
    switch(console_localization)
    {
      case 1:
        sprintf(buf, "%02d:%02d:%02d  %s %02d %02d", dh->tm_hour, dh->tm_min, dh->tm_sec, *console_months[dh->tm_mon], dh->tm_mday, dh->tm_year%100);
        break;
      default:
        sprintf(buf, "%02d:%02d:%02d  %02d %s %02d", dh->tm_hour, dh->tm_min, dh->tm_sec, dh->tm_mday, *console_months[dh->tm_mon], dh->tm_year%100);
        break;
    }
  }
  else
  {
    switch(console_localization)
    {
      case 1:
        sprintf(buf, "%02d:%02d:%02d  %s %02d", dh->tm_hour, dh->tm_min, dh->tm_sec, *console_months[dh->tm_mon], dh->tm_mday);
        break;
      default:
        sprintf(buf, "%02d:%02d:%02d  %02d %s", dh->tm_hour, dh->tm_min, dh->tm_sec, dh->tm_mday, *console_months[dh->tm_mon]);
        break;
    }
  }
  
  return strdup(buf);
}

char* console_format_date(ProtDevice *dev, time_t t)
{
  static char buf[8];
  struct tm *dh;

  dh = localtime(&t);
  switch(console_localization)
  {
    case 1:
      sprintf(buf, "%s %02d", *console_months[dh->tm_mon], dh->tm_mday);
      break;
    default:
      sprintf(buf, "%02d %s", dh->tm_mday, *console_months[dh->tm_mon]);
      break;
  }
  
  return strdup(buf);
}

/***********************************************************************
	Callback support functions
***********************************************************************/

void console_register_callback(ProtDevice *dev, int idx, ConsoleCallback func, void *arg)
{
  CONSOLE->callback[idx] = func;
  CONSOLE->callback_arg[idx] = arg;
}

void console_reset_callback(ProtDevice *dev, int logout)
{
  int i;
  
  /* il callback di timeout non viene cancellato */
  for(i=0; i<(KEY_MAX-1); i++)
  {
    CONSOLE->callback[i] = NULL;
    CONSOLE->callback_arg[i] = NULL;
  }
  
  if(logout) console_register_callback(dev, KEY_DIERESIS, console_logout, NULL);
}

static void console_callback_store(ProtDevice *dev)
{
  int i;
  
  for(i=0; i<KEY_MAX; i++)
  {
    CONSOLE->current_callback[i] = CONSOLE->callback[i];
    CONSOLE->current_callback_arg[i] = CONSOLE->callback_arg[i];
  }
}

static void console_callback_reload(ProtDevice *dev)
{
  int i;
  
  for(i=0; i<KEY_MAX; i++)
  {
    CONSOLE->callback[i] = CONSOLE->current_callback[i];
    CONSOLE->callback_arg[i] = CONSOLE->current_callback_arg[i];
  }
}

static int console_get_key_index(char key)
{
  switch(key)
  {
    case '0': return KEY_0;
    case '1': return KEY_1;
    case '2': return KEY_2;
    case '3': return KEY_3;
    case '4': return KEY_4;
    case '5': return KEY_5;
    case '6': return KEY_6;
    case '7': return KEY_7;
    case '8': return KEY_8;
    case '9': return KEY_9;
    case '*': return KEY_ASTERISK;
    case '#': return KEY_DIERESIS;
    case 'U': return KEY_UP;
    case 'D': return KEY_DOWN;
    case 'L': return KEY_LEFT;
    case 'R': return KEY_RIGHT;
    case 'E': return KEY_ENTER;
    case 'C': return KEY_CANCEL;
    default: return -1;
  }
}

void console_return_to_last_menu_item(char *dev, int null2)
{
  console_reset_callback((ProtDevice*)dev, 1);
  CONSOLE->last_menu_item((ProtDevice*)dev, 0, NULL);
}

/***********************************************************************
	Send functions
***********************************************************************/

static int console_check(ProtDevice *dev);

static void console_resend(ProtDevice *dev, int timeout)
{
  CONSOLE->resend_counter++;
  if(CONSOLE->resend_counter == 3)
  {
#if TEBESA
    console_stop(dev, 0, NULL);
#else
    pthread_cancel(CONSOLE->pth);
    timeout_off(CONSOLE->hour_timeout); 
       
    pthread_mutex_lock(&console_mutex);
    while((CONSOLE->head = support_list_del(CONSOLE->head)));
    pthread_mutex_unlock(&console_mutex);
    
    CONSOLE->active = console_check(dev);
#endif
  }
  else
  {
    write(dev->fd, CONSOLE->cmd, strlen(CONSOLE->cmd));
    timeout_on(CONSOLE->ser_timeout, (timeout_func)console_resend, dev, timeout, timeout);
  }
}

void console_send(ProtDevice *dev, char *cmd)
{
  pthread_mutex_lock(&console_mutex);
  
  CONSOLE->tail = support_list_add_next(CONSOLE->tail, cmd, 20, 0);
  
  if(!CONSOLE->head)
  {
    CONSOLE->resend_counter = 0;
    
    CONSOLE->head = CONSOLE->tail;
    
    write(dev->fd, cmd, strlen(cmd));
    
    CONSOLE->cmd = CONSOLE->head->cmd;
    timeout_on(CONSOLE->ser_timeout, (timeout_func)console_resend, dev, 20, 20);
  }
  
  pthread_mutex_unlock(&console_mutex);
}

static void console_send_next(ProtDevice *dev)
{
  pthread_mutex_lock(&console_mutex);
  
  if(!CONSOLE->head)
  {
    pthread_mutex_unlock(&console_mutex);
    return;
  }
  
  CONSOLE->head = support_list_del(CONSOLE->head);

  if(CONSOLE->head)
  {
    CONSOLE->resend_counter = 0;

    write(dev->fd, CONSOLE->head->cmd, strlen(CONSOLE->head->cmd));
    
    CONSOLE->cmd = CONSOLE->head->cmd;
    timeout_on(CONSOLE->ser_timeout, (timeout_func)console_resend, dev, CONSOLE->head->timeout, CONSOLE->head->timeout);
  }
  else
    CONSOLE->tail = CONSOLE->head;
  
  pthread_mutex_unlock(&console_mutex);
}

/***********************************************************************
	List functions
***********************************************************************/

ListItem* console_list_add(ListItem *list, char *row1, char *row2, ConsoleCallback func, int param)
{
  ListItem *ltmp, *tail;
  
  ltmp = (ListItem*)malloc(sizeof(ListItem));
  strcpy(ltmp->row1, row1);
  if(row2)
    strcpy(ltmp->row2, row2);
  else
    ltmp->row2[0] = -1;
  ltmp->func = func;
  ltmp->param = param;
  ltmp->next = NULL;
  
  if(!list) return ltmp;
  
  tail = list;
  while(tail->next) tail = tail->next;
  tail->next = ltmp;
  
  return list;
}

void console_list_free(ListItem *list)
{
  ListItem *ltmp;
  
  while(list)
  {
    ltmp = list->next;
    free(list);
    list = ltmp;
  }
}

int console_list_count(ListItem *list)
{
  int count = 0;
  
  while(list)
  {
    count++;
    list = list->next;
  }
  
  return count;
}

static CALLBACK void* console_list_up(ProtDevice *dev, int press, void *list)
{
  if(press)
  {
    CONSOLE->list_current_idx = 0;
    CONSOLE->list_current_pos = 0;
  }
  else
  {
    if(CONSOLE->list_current_pos)
      CONSOLE->list_current_pos--;
    if(CONSOLE->list_current_idx)
      CONSOLE->list_current_idx--;
  }
  
  console_list_show(dev, (ListItem*)list, CONSOLE->list_current_idx, CONSOLE->list_current_num, CONSOLE->list_current_pos);
  return NULL;
}

static CALLBACK void* console_list_down(ProtDevice *dev, int press, void *list)
{
  int cnt;

  cnt = console_list_count((ListItem*)list);
  
  if(press)
  {    
    CONSOLE->list_current_idx = cnt - 1;
    if(cnt > CONSOLE->list_current_num) cnt = CONSOLE->list_current_num;
    CONSOLE->list_current_pos = cnt - 1;
  }
  else
  {
    if((CONSOLE->list_current_pos < (CONSOLE->list_current_num-1)) && (CONSOLE->list_current_pos < (cnt-1)))
      CONSOLE->list_current_pos++;
    if(CONSOLE->list_current_idx < (cnt-1))
      CONSOLE->list_current_idx++;
  }
  
  console_list_show(dev, (ListItem*)list, CONSOLE->list_current_idx, CONSOLE->list_current_num, CONSOLE->list_current_pos);
  return NULL;
}

static void console_list_set_pos_digit(ProtDevice *dev, int digit, ListItem *list)
{
  ListItem *l = list;
  int num, idx;
  
  CONSOLE->list_digits[0] = CONSOLE->list_digits[1];
  CONSOLE->list_digits[1] = CONSOLE->list_digits[2];
  CONSOLE->list_digits[2] = CONSOLE->list_digits[3];
  CONSOLE->list_digits[3] = '0' + digit;

  sscanf(CONSOLE->list_digits, "%d", &num);
  for(idx = 0; l && (l->param < num); idx++, l = l->next);
  if(!l)
  {
    idx = 0;
    memcpy(CONSOLE->list_digits, "0000", 4);
  }
  
  CONSOLE->list_current_pos = 0;
  CONSOLE->list_current_idx = idx;
  
  console_list_show(dev, (ListItem*)list, CONSOLE->list_current_idx, CONSOLE->list_current_num, CONSOLE->list_current_pos);
}

static CALLBACK void* console_list_set_pos_0(ProtDevice *dev, int press, void *list)
{
  console_list_set_pos_digit(dev, 0, list);
  return NULL;
}

static CALLBACK void* console_list_set_pos_1(ProtDevice *dev, int press, void *list)
{
  console_list_set_pos_digit(dev, 1, list);
  return NULL;
}

static CALLBACK void* console_list_set_pos_2(ProtDevice *dev, int press, void *list)
{
  console_list_set_pos_digit(dev, 2, list);
  return NULL;
}

static CALLBACK void* console_list_set_pos_3(ProtDevice *dev, int press, void *list)
{
  console_list_set_pos_digit(dev, 3, list);
  return NULL;
}

static CALLBACK void* console_list_set_pos_4(ProtDevice *dev, int press, void *list)
{
  console_list_set_pos_digit(dev, 4, list);
  return NULL;
}

static CALLBACK void* console_list_set_pos_5(ProtDevice *dev, int press, void *list)
{
  console_list_set_pos_digit(dev, 5, list);
  return NULL;
}

static CALLBACK void* console_list_set_pos_6(ProtDevice *dev, int press, void *list)
{
  console_list_set_pos_digit(dev, 6, list);
  return NULL;
}

static CALLBACK void* console_list_set_pos_7(ProtDevice *dev, int press, void *list)
{
  console_list_set_pos_digit(dev, 7, list);
  return NULL;
}

static CALLBACK void* console_list_set_pos_8(ProtDevice *dev, int press, void *list)
{
  console_list_set_pos_digit(dev, 8, list);
  return NULL;
}

static CALLBACK void* console_list_set_pos_9(ProtDevice *dev, int press, void *list)
{
  console_list_set_pos_digit(dev, 9, list);
  return NULL;
}

static ConsoleCallback console_list_set_pos[10] = {console_list_set_pos_0, console_list_set_pos_1, console_list_set_pos_2, console_list_set_pos_3, console_list_set_pos_4, console_list_set_pos_5, console_list_set_pos_6, console_list_set_pos_7, console_list_set_pos_8, console_list_set_pos_9};

void console_list_show(ProtDevice *dev, ListItem *list, int idx, int num, int pos)
{
  ListItem *ltmp;
  int i, row, size;
  char cmd[40];
  
  if(!list)
  {
    sprintf(cmd, "ATS%02d\r", 9 - (num * 2));
    console_send(dev, cmd);
    sprintf(cmd, str_14_125, 8 - num);
    console_send(dev, cmd);
    if(CONSOLE->list_show_cancel)
      console_register_callback(dev, KEY_CANCEL, CONSOLE->list_show_cancel, NULL);
    else
      console_register_callback(dev, KEY_CANCEL, console_show_menu, NULL);
    return;
  }

  size = 1;
  if(list->row2[0] != -1) size++;
  
  row = 9 - num * size;

  CONSOLE->list_current_idx = idx;
  CONSOLE->list_current_num = num;
  CONSOLE->list_current_pos = pos;
  
  ltmp = list;
  
  for(i=0; i<(idx-pos); i++) ltmp = ltmp->next;
  
  for(i=0; ltmp && (i<num); i++)
  {
    if(i == pos) console_send(dev, "ATR1\r");
    
    sprintf(cmd, str_14_143, row++, ltmp->row1);
    console_send(dev, cmd);
    if(ltmp->row2[0] != -1)
    {
      sprintf(cmd, str_14_143, row++, ltmp->row2);
      console_send(dev, cmd);
    }
    
    if(i == pos)
    {
      console_send(dev, "ATR0\r");
      console_register_callback(dev, KEY_ENTER, ltmp->func, (void*)ltmp->param);
    }
    
    ltmp = ltmp->next;
  }
  
  if(row < 9)
  {
    sprintf(cmd, "ATS%02d\r", row);
    console_send(dev, cmd);
  }
  
  console_register_callback(dev, KEY_UP, console_list_up, list);
  console_register_callback(dev, KEY_DOWN, console_list_down, list);
  for(i=0; i<10; i++) console_register_callback(dev, KEY_0 + i, console_list_set_pos[i], (void*)list);
  if(CONSOLE->list_show_cancel)
    console_register_callback(dev, KEY_CANCEL, CONSOLE->list_show_cancel, NULL);
  else
    console_register_callback(dev, KEY_CANCEL, console_show_menu, NULL);
}

/***********************************************************************
	Menu functions
***********************************************************************/

static void console_list_menu_show_items(ProtDevice *dev);

static CALLBACK void* console_list_menu_up(ProtDevice *dev, int press, void *null)
{
  int perm, idx;
  
  perm = 1L << (CONSOLE->current_user->level - 1);
  
  if(press)
  {
    idx = 0;

    if(!(CONSOLE->menu[CONSOLE->menu_current_menu].menu[idx].perm & perm))
      while(CONSOLE->menu[CONSOLE->menu_current_menu].menu[idx].title &&
          !(CONSOLE->menu[CONSOLE->menu_current_menu].menu[idx].perm & perm))
        idx++;

    if(CONSOLE->menu[CONSOLE->menu_current_menu].menu[idx].title)
    {
      CONSOLE->menu_current_idx = idx;
      CONSOLE->menu_current_pos = 0;
    }
  }
  else
  {
    idx = CONSOLE->menu_current_idx - 1;
  
    if((idx >= 0) && !(CONSOLE->menu[CONSOLE->menu_current_menu].menu[idx].perm & perm))
      while((idx >= 0) && !(CONSOLE->menu[CONSOLE->menu_current_menu].menu[idx].perm & perm))
        idx--;

    if(idx >= 0)
    {
      CONSOLE->menu_current_idx = idx;
      if(CONSOLE->menu_current_pos > 0) CONSOLE->menu_current_pos--;
    }
  }
  
  console_list_menu_show_items(dev);
  return NULL;
}

static CALLBACK void* console_list_menu_down(ProtDevice *dev, int press, void *null)
{
  int perm, idx;
  
  perm = 1L << (CONSOLE->current_user->level - 1);
  
  if(press)
  {
    for(idx = CONSOLE->menu_current_idx + 1; CONSOLE->menu[CONSOLE->menu_current_menu].menu[idx].title; idx++);
    while((idx >= 0) && !(CONSOLE->menu[CONSOLE->menu_current_menu].menu[idx].perm & perm)) idx--;
    
    if(idx >= 0)
    {
      CONSOLE->menu_current_idx = idx;
      if(idx < 3)
        CONSOLE->menu_current_pos = idx;
      else
        CONSOLE->menu_current_pos = 3;
    }
  }
  else
  {
    idx = CONSOLE->menu_current_idx + 1;
  
    if(!(CONSOLE->menu[CONSOLE->menu_current_menu].menu[idx].perm & perm))
      while(CONSOLE->menu[CONSOLE->menu_current_menu].menu[idx].title &&
          !(CONSOLE->menu[CONSOLE->menu_current_menu].menu[idx].perm & perm))
        idx++;
        
    if(CONSOLE->menu[CONSOLE->menu_current_menu].menu[idx].title)
    {
      CONSOLE->menu_current_idx = idx;
      if(CONSOLE->menu_current_pos < 3) CONSOLE->menu_current_pos++;
    }
  }
  
  console_list_menu_show_items(dev);
  return NULL;
}

#if EMULATE_MENU
static CALLBACK void* console_list_menu_right(ProtDevice *dev, int press, void *null)
{
  int perm;
  
  perm = 1L << (CONSOLE->current_user->level - 1);

  for(CONSOLE->menu_current_menu++; CONSOLE->menu[CONSOLE->menu_current_menu].title &&
        !(CONSOLE->menu[CONSOLE->menu_current_menu].perm & perm); CONSOLE->menu_current_menu++);
  if(!CONSOLE->menu[CONSOLE->menu_current_menu].title)
    for(CONSOLE->menu_current_menu=0; CONSOLE->menu[CONSOLE->menu_current_menu].title &&
          !(CONSOLE->menu[CONSOLE->menu_current_menu].perm & perm); CONSOLE->menu_current_menu++);
  
  console_show_menu(dev, 0, NULL);
  return NULL;
}

static CALLBACK void* console_list_menu_left(ProtDevice *dev, int press, void *null)
{
  int perm;
  
  perm = 1L << (CONSOLE->current_user->level - 1);
  
  for(CONSOLE->menu_current_menu--; (CONSOLE->menu_current_menu >= 0) &&
        !(CONSOLE->menu[CONSOLE->menu_current_menu].perm & perm); CONSOLE->menu_current_menu--);
  if(CONSOLE->menu_current_menu < 0)
  {
    for(CONSOLE->menu_current_menu=0; CONSOLE->menu[CONSOLE->menu_current_menu].title; CONSOLE->menu_current_menu++);
    for(CONSOLE->menu_current_menu--; CONSOLE->menu[CONSOLE->menu_current_menu].title &&
          !(CONSOLE->menu[CONSOLE->menu_current_menu].perm & perm); CONSOLE->menu_current_menu--);
  }
  
  console_show_menu(dev, 0, NULL);
  return NULL;
}
#endif

static CALLBACK void* console_list_menu(ProtDevice *dev, int menu, void *null)
{
  int i;
  
  for(i=0; CONSOLE->menu[i].title && (CONSOLE->menu[i].icon != menu); i++);

#if EMULATE_MENU
  {
    char cmd[32];
    sprintf(cmd, str_14_144, CONSOLE->menu[i].title);
    console_send(dev, cmd);
    sprintf(cmd, "ATT05201%02d\r", CONSOLE->menu[i].icon);
    console_send(dev, cmd);
  }
#endif
  
  if(CONSOLE->menu[i].title && delphi_lang)
  {
    char cmd[32];
    strcpy(cmd,"ATN0101                \r");
    memcpy(cmd+7+(16-strlen(*(CONSOLE->menu[i].title)))/2,
      *(CONSOLE->menu[i].title), strlen(*(CONSOLE->menu[i].title)));
    console_send(dev, cmd);
  }
  
  if(CONSOLE->menu[i].title && CONSOLE->menu[i].menu)
  {
    CONSOLE->menu_current_menu = i;
    CONSOLE->menu_current_idx = 0;
    CONSOLE->menu_current_pos = 0;
    
    console_list_menu_show_items(dev);
  }
  else
    console_send(dev, "ATS05\r");

  console_register_callback(dev, KEY_MENU, console_list_menu, NULL);
  console_register_callback(dev, KEY_ENTER, CONSOLE->menu[CONSOLE->menu_current_menu].menu[CONSOLE->menu_current_idx].func, NULL);
  
  return NULL;
}

static CALLBACK void* console_menu_select(ProtDevice *dev, int press, void *null)
{
  if(CONSOLE->menu[CONSOLE->menu_current_menu].menu[CONSOLE->menu_current_idx].func)
    CONSOLE->menu[CONSOLE->menu_current_menu].menu[CONSOLE->menu_current_idx].func(dev, 0, NULL);
  else
  {
    console_register_callback(dev, KEY_MENU, console_list_menu, NULL);
    console_register_callback(dev, KEY_UP, console_list_menu_up, NULL);
    console_register_callback(dev, KEY_DOWN, console_list_menu_down, NULL);
#if EMULATE_MENU
    console_register_callback(dev, KEY_LEFT, console_list_menu_left, NULL);
    console_register_callback(dev, KEY_RIGHT, console_list_menu_right, NULL);
#endif
    console_register_callback(dev, KEY_ENTER, console_menu_select, NULL);
  }
    
  return NULL;
}

static void console_list_menu_show_items(ProtDevice *dev)
{
  int i, r[4], rr, perm;
  char cmd[32];
  
  /* find the first item allowed */
  perm = 1L << (CONSOLE->current_user->level - 1);
  if(!(CONSOLE->menu[CONSOLE->menu_current_menu].menu[CONSOLE->menu_current_idx].perm & perm))
    while(!(CONSOLE->menu[CONSOLE->menu_current_menu].menu[CONSOLE->menu_current_idx].perm & perm))
      CONSOLE->menu_current_idx++;
  
  i = 0;
  for(rr=0; rr<4; rr++) r[rr] = -1;
  
  for(rr=0; (rr<4) && CONSOLE->menu[CONSOLE->menu_current_menu].menu[i].title; rr++)
  {
    while(CONSOLE->menu[CONSOLE->menu_current_menu].menu[i].title &&
          !(CONSOLE->menu[CONSOLE->menu_current_menu].menu[i].perm & perm)) i++;
    if(CONSOLE->menu[CONSOLE->menu_current_menu].menu[i].title) r[rr] = i++;
  }
  
  while(CONSOLE->menu_current_idx != r[CONSOLE->menu_current_pos])
  {
    r[0] = r[1];
    r[1] = r[2];
    r[2] = r[3];
    i = r[3] + 1;
    while(CONSOLE->menu[CONSOLE->menu_current_menu].menu[i].title &&
          !(CONSOLE->menu[CONSOLE->menu_current_menu].menu[i].perm & perm)) i++;
    if(CONSOLE->menu[CONSOLE->menu_current_menu].menu[i].title) r[3] = i;
  }
  
  
  for(i=0; (i<4) && (r[i]>= 0); i++)
  {
    if(CONSOLE->menu_current_idx == r[i]) console_send(dev, "ATR1\r");
    sprintf(cmd, str_14_145, i + 5, *CONSOLE->menu[CONSOLE->menu_current_menu].menu[r[i]].title);
    console_send(dev, cmd);
    if(CONSOLE->menu_current_idx == r[i]) console_send(dev, "ATR0\r");
  }
  
  if(CONSOLE->menu[CONSOLE->menu_current_menu].icon == LOGOUT_MENU) i = 0;
  
  if(i<4)
  {
    sprintf(cmd, "ATS%02d\r", i + 5);
    console_send(dev, cmd);
  }
  
  console_register_callback(dev, KEY_MENU, console_list_menu, NULL);
  console_register_callback(dev, KEY_UP, console_list_menu_up, NULL);
  console_register_callback(dev, KEY_DOWN, console_list_menu_down, NULL);
#if EMULATE_MENU
    console_register_callback(dev, KEY_LEFT, console_list_menu_left, NULL);
    console_register_callback(dev, KEY_RIGHT, console_list_menu_right, NULL);
#endif
  console_register_callback(dev, KEY_ENTER, console_menu_select, NULL);
}

CALLBACK void* console_show_menu(ProtDevice *dev, int press, void *null)
{
  int i, j, perm;
  char cmd[64];
  
  console_register_callback(dev, KEY_MENU, console_list_menu, NULL);
  
  console_send(dev, "ATS00\r");
  console_send(dev, "ATL000\r");
  console_send(dev, "ATF0\r");
  sprintf(cmd, "ATM00");
  j = 5;
  perm = 1L << (CONSOLE->current_user->level - 1);
  
  /* start menu list from current position */
  if(CONSOLE->menu_current_menu >= 0)
  {
    for(i=0; CONSOLE->menu[i].title && (i != CONSOLE->menu_current_menu); i++);
    for(; CONSOLE->menu[i].title; i++)
      if(CONSOLE->menu[i].perm & perm)
      {
        sprintf(cmd + j, "%02d", CONSOLE->menu[i].icon);
        j += 2;
      }
  }
  
  /* fill menu list with remaining (or all) icons */
  for(i=0; CONSOLE->menu[i].title && (i != CONSOLE->menu_current_menu); i++)
    if(CONSOLE->menu[i].perm & perm)
    {
      sprintf(cmd + j, "%02d", CONSOLE->menu[i].icon);
      j += 2;
    }
    
  sprintf(cmd + j, "\r");
  console_send(dev, cmd);
  
  return NULL;
}

void console_return_to_menu(char *dev, int null)
{
  console_show_menu((ProtDevice*)dev, 0, NULL);
}

void console_disable_menu(ProtDevice *dev, char *title)
{
  char cmd[32];

  console_send(dev, "ATM\r");
  console_send(dev, "ATS00\r");
  console_send(dev, "ATF0\r");
  console_send(dev, "ATL000\r");
  sprintf(cmd, str_14_146, ((16-strlen(title))/2)+1, title);
  console_send(dev, cmd);
  console_send(dev, str_14_147);
}

void console_submenu(ProtDevice *dev, MenuItem *menu, int param, int num)
{
  int i, perm;
  
  if(!menu) return;
  
  perm = 1L << (CONSOLE->current_user->level - 1);
  
  console_list_free(CONSOLE->support_list);
  CONSOLE->support_list = NULL;
  
  for(i=0; menu[i].title; i++)
  {
    if(menu[i].perm & perm)
      CONSOLE->support_list = console_list_add(CONSOLE->support_list, *menu[i].title, NULL, menu[i].func, param);
  }
  console_list_show(dev, CONSOLE->support_list, 0, num, 0);
}

/***********************************************************************
	LOGOUT menu functions
***********************************************************************/

static CALLBACK void* console_logout_save(ProtDevice *dev, int press, void *null)
{
  lara_save(2);	// Salva eventuali variazioni pendenti
  
#ifdef TEBESA
  console_stop(dev, press, null);
#else
  console_main(dev, press, null);
#endif
}

static CALLBACK void* console_logout_ack(ProtDevice *dev, int press, void *null)
{
  console_reset_callback(dev, 0);
  
  console_send(dev, "ATL000\r");
  console_send(dev, "ATF0\r");
  console_send(dev, "ATM\r");
  console_send(dev, "ATS00\r");
  console_send(dev, "ATU0360206403\r");
  console_send(dev, str_14_148);
  
#if 0
#ifdef TEBESA
  console_register_callback(dev, KEY_ENTER, console_stop, NULL);
#else
  console_register_callback(dev, KEY_ENTER, console_main, NULL);
#endif
#else
  console_register_callback(dev, KEY_ENTER, console_logout_save, NULL);
#endif
  console_register_callback(dev, KEY_CANCEL, console_show_menu, NULL);
  
  return NULL;
}

CALLBACK void* console_logout(ProtDevice *dev, int press, void *null)
{
  if(press)
    console_logout_ack(dev, 0, NULL);
  else
    console_callback_reload(dev);
  
  return NULL;
}

/***********************************************************************
	Direct codes functions
***********************************************************************/

static CALLBACK void* console_direct(ProtDevice *dev, int press, void *null);

static char console_alarm_icon[] = {
  'A', 'T', 'K', '0', '2', '4', '0', '0', '0', '0', '8', '0', '1',
  0x01, 0x9d, 0xe3, 0x82, 0x42, 0x4c, 0xb0, 0x20, '\r', '\0'};
static char console_notready_icon[] = {
  'A', 'T', 'K', '0', '0', '8', '0', '0', '0', '0', '8', '0', '1',
  0x60, 0x78, 0x7e, 0x53, 0x7e, 0x78, 0x60, 0x40, '\r', '\0'};

static void console_return_main(char *dev, int null2)
{
//  console_main((ProtDevice*)dev, 0, NULL);
  console_direct((ProtDevice*)dev, 0, NULL);
}

static CALLBACK void* console_zone_allarmate_down(ProtDevice *dev, int press, void *zona_as_voidp);

static CALLBACK void* console_zone_allarmate_up(ProtDevice *dev, int press, void *zona_as_voidp)
{
  int zona = (int)zona_as_voidp;
  int i, z, s;
  char cmd[32], *num, *desc;
  
  console_send(dev, "ATS00\r");
  console_send(dev, "ATL000\r");
  
  if(zona > (n_ZS+1))
    return console_direct(dev, 0, NULL);
  
  z = 1;
  
//  for(i=0; (i<8) && (z<(n_ZS+1)); z++)
  for(i=0; (i<8) && (z<90); z++)
  {
    for(s=0; s<n_SE; s++)
    {
      if(Zona_SE[s] == z)
      {
        if((z >= zona) && ((ZS[z] & (bitStatusInactive|bitActive)) == bitStatusInactive))
        {
          zona = z;
          sprintf(cmd, str_14_126, zona);
          console_send(dev, cmd);
          console_send(dev, str_14_147);
          s = 0;
          for(i=0; i<3; i++)
          {
            for(; s<n_SE; s++)
              if((Zona_SE[s] == z) && (SE[s] & bitAlarm))
              {
                string_sensor_name(s, &num, &desc);
                sprintf(cmd, str_14_149, 3+i*2, num);
                console_send(dev, cmd);
                sprintf(cmd, str_14_149, 4+i*2, desc);
                console_send(dev, cmd);
                s++;
                break;
              }
          }
          i = 8;
        }
        i++;
        break;
      }
    }
  }
  if(i <= 8)
    return console_direct(dev, 0, NULL);
  
  console_register_callback(dev, KEY_CANCEL, console_main, NULL);
  console_register_callback(dev, KEY_UP, console_zone_allarmate_up, (void*)(zona+1));
  console_register_callback(dev, KEY_DOWN, console_zone_allarmate_down, (void*)(zona-1));
  
  return NULL;
}

static CALLBACK void* console_zone_allarmate_down(ProtDevice *dev, int press, void *zona_as_voidp)
{
  int zona = (int)zona_as_voidp;
  int i, z, s, zt;
  char cmd[32], *num, *desc;
  
  console_send(dev, "ATS00\r");
  console_send(dev, "ATL000\r");
  
  if(zona < 1)
    return console_direct(dev, 0, NULL);
  
  zt = 1;
  z = 0;
//  for(i=0; (i<8) && (zt<(n_ZS+1)); zt++)
  for(i=0; (i<8) && (zt<90); zt++)
  {
    for(s=0; s<n_SE; s++)
    {
      if(Zona_SE[s] == zt)
      {
        z = zt;
        i++;
        break;
      }
    }
  }
  
  for(i=0; (i<8) && (z>0); z--)
  {
    for(s=0; s<n_SE; s++)
    {
      if(Zona_SE[s] == z)
      {
        if((z <= zona) && ((ZS[z] & (bitStatusInactive|bitActive)) == bitStatusInactive))
        {
          zona = z;
          sprintf(cmd, str_14_127, zona);
          console_send(dev, cmd);
          console_send(dev, str_14_147);
          s = 0;
          for(i=0; i<3; i++)
          {
            for(; s<n_SE; s++)
              if((Zona_SE[s] == z) && (SE[s] & bitAlarm))
              {
                string_sensor_name(s, &num, &desc);
                sprintf(cmd, str_14_149, 3+i*2, num);
                console_send(dev, cmd);
                sprintf(cmd, str_14_149, 4+i*2, desc);
                console_send(dev, cmd);
                s++;
                break;
              }
          }
          i = 8;
        }
        i++;
        break;
      }
    }
  }
  if(i <= 8)
    return console_direct(dev, 0, NULL);
  
  console_register_callback(dev, KEY_CANCEL, console_main, NULL);
  console_register_callback(dev, KEY_UP, console_zone_allarmate_up, (void*)(zona+1));
  console_register_callback(dev, KEY_DOWN, console_zone_allarmate_down, (void*)(zona-1));
  
  return NULL;
}

static CALLBACK void* console_code(ProtDevice *dev, int code_as_int, void *null)
{
  int code, code_idx;
  short *codes;
  char *code_cod, *code_desc, cmd[32];
  
  code = -1;
  code_idx = -1;
  
  sscanf((char*)code_as_int, "%d", &code);

  if(code >= 0)
  {
    codes = (short*)RONDA;
    for(code_idx=0; code_idx<(sizeof(RONDA)/sizeof(short)); code_idx++)
      if(codes[code_idx] == code) break;
    if(code_idx == (sizeof(RONDA)/sizeof(short))) code_idx = -1;
  }
  
  console_reset_callback(dev, 0);
  console_send(dev, "ATS00\r");
  
  if((code < 0) || (code_idx < 0))
  {
    console_send(dev, str_14_128);
    console_send(dev, str_14_129);
  }
  else
  {
    string_code_name(code_idx, &code_cod, &code_desc);
    sprintf(cmd, str_14_150, code_cod);
    console_send(dev, cmd);
    sprintf(cmd, str_14_151, code_desc);
    console_send(dev, cmd);
    database_lock();
    database_set_alarm2(&ME[code_idx+768]);
    database_unlock();
  }
  
  console_send(dev, "ATP30\r");
  timeout_on(CONSOLE->timeout, (timeout_func)console_return_main, dev, 0, TIMEOUT_MSG);

  return NULL;
}

static CALLBACK void* console_hour(ProtDevice *dev, int press, void *null);
static CALLBACK void* console_login(ProtDevice *dev, int press, void *null);

static CALLBACK void* console_direct(ProtDevice *dev, int press, void *null)
{
  char cmd[16];
  int i, z, s; //, m;
  
  console_reset_callback(dev, 0);
  
  if(!libuser_started)
  {
    console_register_callback(dev, KEY_DIERESIS, console_login, NULL);
    console_register_callback(dev, KEY_ASTERISK, console_direct, NULL);
    console_register_callback(dev, KEY_ENTER, console_hour, NULL);
    return NULL;
  }
  
  CONSOLE->active = 2;
  
  console_send(dev, "ATA\r");
  console_send(dev, "ATQ1\r");

  console_send(dev, "ATF0\r");
  
  sprintf(cmd, "ATE%04d\r", console_login_timeout_time/10);
  console_send(dev, cmd);
  
  console_send(dev, "ATL111\r");
#if 0
  console_send(dev, str_14_130);
  console_send(dev, str_14_131);
  console_send(dev, "ATN0706____\r");
  console_send(dev, "ATG0706\r");
#else
  
  z = 1;
  i = 0;
  
//  for(; (i<8) && (z<(n_ZS+1)); z++)
  for(; (i<8) && (z<90); z++)
  {
    for(s=0; s<n_SE; s++)
    if(Zona_SE[s] == z)
    {
      if(ZS[z] & bitActive) console_send(dev, "ATR1\r");
      sprintf(cmd, str_14_152, i+1, i+'A');
      console_send(dev, cmd);
      if(ZS[z] & bitActive) console_send(dev, "ATR0\r");
//      m = 0;
//      for(s=0; s<n_SE; s++)
//        if((Zona_SE[s] == z) && (SE[s] & bitMUAlarm)) m = 1;
//      if(m)
      if(ZONAEX[z] & bitMUAlarmZone)
      {
        console_alarm_icon[7] = i+'0';
        console_send(dev, console_alarm_icon);
      }
      if((ZS[z] & (bitStatusInactive|bitActive)) == bitStatusInactive)
      {
        console_notready_icon[7] = i+'0';
        console_send(dev, console_notready_icon);
      }
      
      i++;
      break;
    }
  }
  for(i=0; i<8; i++)
  {
    sprintf(cmd, str_14_153, i+1);
    console_send(dev, cmd);
  }
  console_send(dev, str_14_132);
  console_send(dev, str_14_154);
  console_send(dev, str_14_155);
  
  console_register_callback(dev, KEY_UP, console_zone_allarmate_up, (void*)0);
  console_register_callback(dev, KEY_DOWN, console_zone_allarmate_down, (void*)255);
#endif
  
  console_register_callback(dev, KEY_STRING, console_code, NULL);
  console_register_callback(dev, KEY_CANCEL, console_main, NULL);
  
  return NULL;
}

/***********************************************************************
	Hour functions
***********************************************************************/

static CALLBACK void* console_hour_exit(ProtDevice *dev, int press, void *null)
{
  timeout_off(CONSOLE->hour_timeout);
  console_main(dev, 0, NULL);
  return NULL;
}

static void console_show_hour(char *dev, int null2)
{
  char cmd[32];
  time_t t;
  struct tm *dh;
  
  t = time(NULL);
  dh = localtime(&t);
  
  if(console_localization == 1)
    sprintf(cmd, str_14_156, dh->tm_mon+1, dh->tm_mday, dh->tm_year-100);
  else
    sprintf(cmd, str_14_156, dh->tm_mday, dh->tm_mon+1, dh->tm_year-100);
  console_send((ProtDevice*)dev, cmd);
  sprintf(cmd, str_14_157, dh->tm_hour, dh->tm_min, dh->tm_sec);
  console_send((ProtDevice*)dev, cmd);
  timeout_on(CONSOLE->hour_timeout, (timeout_func)console_show_hour, dev, 0, 10);
  console_register_callback((ProtDevice*)dev, KEY_CANCEL, console_hour_exit, NULL);
}

static CALLBACK void* console_hour(ProtDevice *dev, int press, void *null)
{
  console_reset_callback(dev, 0);
  
  CONSOLE->active = 2;

  console_send(dev, "ATQ1\r");
  console_send(dev, "ATA\r");
  console_send(dev, "ATF0\r");
  console_send(dev, "ATE0005\r");
  
  console_show_hour((char*)dev, 0);
  
  return NULL;
}

/***********************************************************************
	Login functions
***********************************************************************/

static void console_failed(char *dev, int null2)
{
#ifdef TEBESA
  console_stop((ProtDevice*)dev, 0, NULL);
#else
  console_main((ProtDevice*)dev, 0, NULL);
#endif
}

static unsigned char console_login_event[40];

static CALLBACK void* console_verify(ProtDevice *dev, int pass_as_int, void *null)
{
  char *pass = (char*)pass_as_int;
  int i;
  User *ut;
  
  if(CONSOLE->current_user && !strcmp(pass, CONSOLE->current_user->pass))
  {
    if(LS_ABILITATA[dev->consumer] & 0x02)
      CONSOLE->current_user->level = CONSOLE->current_user->proglevel;
    else
      CONSOLE->current_user->level = 1;
    
    i = 1;
    ut = console_user_list;
    while(ut)
    {
      if(ut == CONSOLE->current_user) break;
      ut = ut->next;
      i++;
    }
    ME2[1792] = i;
    database_set_alarm2(&ME[1792]);
    
    CONSOLE->menu_current_menu = -1;
    console_show_menu(dev, 0, NULL);
    
    sprintf(console_login_event, str_14_133, dev->consumer, CONSOLE->current_user->name);
    console_login_event[0] = Evento_Esteso;
    console_login_event[1] = Ex_Stringa;
    console_login_event[2] = 0;
    console_login_event[3] = strlen(console_login_event + 4);
    
    codec_queue_event(console_login_event);
  }
  else
  {
    CONSOLE->current_user = NULL;
    console_reset_callback(dev, 0);
  
    console_send(dev, "ATS00\r");
    console_send(dev, str_14_158);
    console_send(dev, str_14_134);
  
    timeout_on(CONSOLE->timeout, (timeout_func)console_failed, dev, 0, TIMEOUT_MSG);
    console_send(dev, "ATP30\r");
  }
  
  return NULL;
}

static CALLBACK void* console_passwd(ProtDevice *dev, int user_as_int, void *null)
{
  char *user = (char*)user_as_int;
  User *ut;
  
  console_reset_callback(dev, 0);
  
  ut = console_user_list;
  while(ut)
  {
    if(!strcasecmp(user, ut->name)) break;
    ut = ut->next;
  }
  CONSOLE->current_user = ut;
  if(CONSOLE->current_user && CONSOLE->current_user->pass[0] == 0)
  {
    CONSOLE->menu_current_menu = -1;
    console_show_menu(dev, 0, NULL);
    return NULL;
  }
  
  console_send(dev, "ATL121\r");
  console_send(dev, "ATS03\r");
  console_send(dev, str_14_135);
  console_send(dev, str_14_136);
  console_send(dev, str_14_159);
  console_send(dev, str_14_160);
//  console_send(dev, "ATL121\r");
  
  console_register_callback(dev, KEY_STRING, console_verify, NULL);
#ifdef TEBESA
  console_register_callback(dev, KEY_CANCEL, console_stop, NULL);
#else
  console_register_callback(dev, KEY_CANCEL, console_main, NULL);
#endif
  
  return NULL;
}

static CALLBACK void* console_login(ProtDevice *dev, int press, void *null)
{
  char cmd[16];
  
  console_reset_callback(dev, 0);
  
  if(!libuser_started)
  {
#ifdef TEBESA
    CONSOLE->active = -1;
#else
    console_register_callback(dev, KEY_DIERESIS, console_login, NULL);
    console_register_callback(dev, KEY_ASTERISK, console_direct, NULL);
    console_register_callback(dev, KEY_ENTER, console_hour, NULL);
#endif
    return NULL;
  }
  
  CONSOLE->active = 2;
  
  console_send(dev, "ATA\r");
  console_send(dev, "ATQ1\r");

  console_send(dev, "ATF0\r");
  
  sprintf(cmd, "ATE%04d\r", console_login_timeout_time/10);
  console_send(dev, cmd);
  
  if(!console_user_list)
  {
    ME2[1792] = 1;
    database_set_alarm2(&ME[1792]);
    CONSOLE->current_user = &generic_user;
    if(LS_ABILITATA[dev->consumer] & 0x02)
      CONSOLE->current_user->level = CONSOLE->current_user->proglevel;
    else
      CONSOLE->current_user->level = 1;
    CONSOLE->menu_current_menu = -1;
    console_show_menu(dev, 0, NULL);

    return NULL;
  }
  
  console_send(dev, "ATL211\r");
  console_send(dev, str_14_137);
  console_send(dev, str_14_138);
  console_send(dev, str_14_161);
  console_send(dev, str_14_162);
//  console_send(dev, "ATL211\r");
  
  console_register_callback(dev, KEY_STRING, console_passwd, NULL);
#ifdef TEBESA
  console_register_callback(dev, KEY_CANCEL, console_stop, NULL);
#else
  console_register_callback(dev, KEY_CANCEL, console_main, NULL);
#endif
  
  return NULL;
}

/***********************************************************************
	Main entry function
***********************************************************************/

static CALLBACK void* console_main(ProtDevice *dev, int press, void *null)
{
  CONSOLE->menu_current_menu = CONSOLE->menu[0].icon;
  
  CONSOLE->active = 1;
  
  if(CONSOLE->current_user)
  {
    CONSOLE->current_user = NULL;
    database_set_alarm2(&ME[1793]);
  }
  
  console_reset_callback(dev, 0);
  console_register_callback(dev, KEY_DIERESIS, console_login, NULL);
  console_register_callback(dev, KEY_ASTERISK, console_direct, NULL);
  console_register_callback(dev, KEY_ENTER, console_hour, NULL);
  console_register_callback(dev, KEY_TIMEOUT, NULL, NULL);

  console_send(dev, "ATQ0\r");
  console_send(dev, "ATS00\r");
  console_send(dev, "ATM\r");
  console_send(dev, "ATB\r");
  console_send(dev, "ATE0000\r");
  console_send(dev, "ATL000\r");
  
  return NULL;
}

/***********************************************************************
	Users loader
***********************************************************************/

static void console_load_users()
{
  FILE *fp;
  User *user, *ut;
  char buf[80];
  
  if(console_user_list) return;
  fp = fopen(ConsoleUserFileName, "r");
  if(!fp) return;
  
  ut = NULL;
  while(fgets(buf, 80, fp))
  {
    user = (User*)malloc(sizeof(User));
    if(!ut)
      console_user_list = user;
    else
      ut->next = user;
    ut = user;
    
    user->next = NULL;
    user->name = strdup(support_delim(buf));
    user->pass = strdup(support_delim(NULL));
    user->proglevel = atoi(support_delim(NULL));
  }
  fclose(fp);
}

void console_save_users()
{
  FILE *fp;
  User *user;

  if(!console_user_list) return;
  fp = fopen(ConsoleUserFileName, "w");
  if(!fp) return;
  user = console_user_list;
  while(user)
  {
    fprintf(fp, "%s;%s;%d\n", user->name, user->pass, user->proglevel);
    user = user->next;
  }
  fclose(fp);
}

/***********************************************************************
	Serial receiver
***********************************************************************/

static void console_recv(ProtDevice *dev)
{
  int n, i;
  char buf[256];
  ConsoleCallback console_current_callback;
  void* console_current_arg;
  
  pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
  
  i = 0;

  while(1)
  {
    pthread_testcancel();
    n = read(dev->fd, buf + i, 1);
    pthread_testcancel();
    
    if(i == sizeof(buf)-1)
    {
      /* msg too long, something is going wrong */
      buf[i] = '\n';
    }
    
    if(n > 0)
    {
      if(buf[i] != '\n')
      {
        i++;
        continue;
      }
    }
    
    if(i > 0)
      buf[i-1] = 0;
    else
      buf[0] = 0;
    i = 0;
    
    if(!buf[0]) continue;	// skip empty lines
//#warning -------------
//printf("(TDat) %s\n", buf);
//support_log(buf);
    
    if(!strncmp(buf, "OK", 2))
    {
      timeout_off(CONSOLE->ser_timeout);
      console_send_next(dev);
    }
    else if(!strncmp(buf, "KK", 2))
    {
      n = console_get_key_index(buf[3]);
#ifdef TEBESA
      if(!((n >= 0) && (CONSOLE->callback[n] == console_stop)))
#endif
      console_send(dev, "ATP05\r");
      if((n >= 0) && (CONSOLE->callback[n]))
      {
        /* KK0# management due to implicit dieresis key callbak for logout */
        /* Recall old callback if not long pressed                         */
        
        if(n == KEY_DIERESIS) console_callback_store(dev);
        
        /* reset current list digits if ENTER or DELETE are pressed */
        
        if((n == KEY_ENTER)||(n == KEY_CANCEL)||(n == KEY_UP)||(n == KEY_DOWN))
          memcpy(CONSOLE->list_digits, "0000", 4);
        
        /* set realtime screen not active */
        
        if((n != KEY_DIERESIS) || (buf[2] == '1'))
          CONSOLE->storico_realtime_active = 0;
        
        /* Normal key management */
        
        console_current_callback = CONSOLE->callback[n];
        console_current_arg = CONSOLE->callback_arg[n];
        
        /* La centrale slave blocca la gestione del proprio terminal data
           per evitare possibili disallienamenti */
        if(master_behaviour != SLAVE_STANDBY)
        {
          console_reset_callback(dev, 1);
          console_current_callback(dev, buf[2] - '0', console_current_arg);
        }
      }
    }
    else if(!strncmp(buf, "KS", 2))
    {
      if(CONSOLE->callback[KEY_STRING])
      {
        console_current_callback = CONSOLE->callback[KEY_STRING];
        console_current_arg = CONSOLE->callback_arg[KEY_STRING];
        console_reset_callback(dev, 1);
        console_current_callback(dev, (int)(buf+2), console_current_arg);
      }
    }
    else if(!strncmp(buf, "MS", 2))
    {
      console_send(dev, "ATP05\r");
      CONSOLE->list_show_cancel = NULL;
      if(CONSOLE->callback[KEY_MENU])
      {
        console_current_callback = CONSOLE->callback[KEY_MENU];
        console_current_arg = CONSOLE->callback_arg[KEY_MENU];
        console_reset_callback(dev, 1);
        console_current_callback(dev, atoi(buf+2), console_current_arg);
      }
    }
    else if(!strncmp(buf, "TF", 2))
    {
    }
    else if(!strncmp(buf, "TT", 2))
    {
      /* set realtime screen not active */
      CONSOLE->storico_realtime_active = 0;

      timeout_off(CONSOLE->hour_timeout);

      if(CONSOLE->callback[KEY_TIMEOUT])
        CONSOLE->callback[KEY_TIMEOUT](dev, 0, NULL);
      
#ifdef TEBESA
      console_stop(dev, 0, NULL);
#else
      console_main(dev, 0, NULL);
#endif
    }
  }
}

/***********************************************************************
	Init and main (loop) functions
***********************************************************************/

struct LangPluginList {
  ConsoleSetLangFunc setlang;
  struct LangPluginList *next;
} *console_lang_list = NULL;

void console_register_lang(ConsoleSetLangFunc func)
{
  struct LangPluginList *list;
  
  pthread_mutex_lock(&console_mutex);
  
  list = malloc(sizeof(struct LangPluginList));
  list->setlang = func;
  list->next = console_lang_list;
  console_lang_list = list;
  
  pthread_mutex_unlock(&console_mutex);
}

static void console_check_timeout(char *dev, int null2)
{
  CONSOLE->active = console_check((ProtDevice*)dev);
}

static int console_check(ProtDevice *dev)
{
  char *res, buf[128];
  int cts, i;
  struct LangPluginList *list;
  
  ioctl(dev->fd, TIOCMGET, &cts);
  if(!(cts & TIOCM_CTS))
  {
    timeout_on(CONSOLE->timeout, (timeout_func)console_check_timeout, dev, 10, 10);
    return 0;
  }

  ser_setspeed(dev->fd, config.consumer[dev->consumer].data.serial.baud1 | CRTSCTS, 10);
  res = ser_send_recv(dev->fd, "ATI\r", buf);
  
  if(!strncmp(res, "STL01", 5))
  {
    CONSOLE->tipo = CONSOLE_STL01;
    for(i=0; i<CONSOLE_STRINGS; i++) *console_str[i] = *console_str1[i];
    
    delphi_set_lang("TERMDATA", sizeof(console_str)/sizeof(char*), console_str);
  }
  else if(!strncmp(res, "STL02", 5))
  {
    CONSOLE->tipo = CONSOLE_STL02;
    for(i=0; i<CONSOLE_STRINGS; i++) *console_str[i] = *console_str2[i];
    
    delphi_set_lang("TERMDATA2", sizeof(console_str)/sizeof(char*), console_str);
  }
  else if(res[0] == 0)
  {
    /* Trovato qualcosa sulla seriale ma poi non comunica, per sicurezza
       riavvio la seriale. */
    ser_close(dev->fd);
    sprintf(buf, "/dev/ttyS%d", config.consumer[dev->consumer].configured - 1);
    dev->fd = ser_open(buf, config.consumer[dev->consumer].data.serial.baud1 | CRTSCTS, 0);
    timeout_on(CONSOLE->timeout, (timeout_func)console_check_timeout, dev, 10, 10);
    return 0;
  }
  else
  {
    timeout_on(CONSOLE->timeout, (timeout_func)console_check_timeout, dev, 10, 10);
    return 0;
  }
  
  list = console_lang_list;
  while(list)
  {
    list->setlang(CONSOLE->tipo);
    list = list->next;
  }
  
  if(config.consumer[dev->consumer].data.serial.baud2)
  {
    support_log(str_14_139);
    switch(config.consumer[dev->consumer].data.serial.baud2)
    {
      case B1200: write(dev->fd, "ATD1\r", 5); break;
      case B2400: write(dev->fd, "ATD2\r", 5); break;
      case B4800: write(dev->fd, "ATD3\r", 5); break;
      case B9600: write(dev->fd, "ATD4\r", 5); break;
      case B19200: write(dev->fd, "ATD5\r", 5); break;
      case B38400: write(dev->fd, "ATD6\r", 5); break;
      case B57600: write(dev->fd, "ATD7\r", 5); break;
      default: return 0;
    }
    tcdrain(dev->fd);
    ser_setspeed(dev->fd, config.consumer[dev->consumer].data.serial.baud2 | CRTSCTS, 0);  
  }
  else
    ser_setspeed(dev->fd, config.consumer[dev->consumer].data.serial.baud1 | CRTSCTS, 0);
  
  pthread_create(&(CONSOLE->pth), NULL, (PthreadFunction)console_recv, dev);
  pthread_detach(CONSOLE->pth);
  console_main(dev, 0, NULL);
  
  return 1;
}

static int console_init(ProtDevice *dev)
{
  int i;
  
  if(dev->consumer < 0) return 0;
  
  if(!dev->prot_data)
  {
    dev->prot_data = calloc(1, sizeof(ConsoleData));
    if(!dev->prot_data) return 0;
  
    CONSOLE->timeout = timeout_init();
    CONSOLE->ser_timeout = timeout_init();
    CONSOLE->hour_timeout = timeout_init();
  }
  
  if(config.consumer[dev->consumer].type == PROT_CONSOLE_SYS)
    CONSOLE->menu = console_menu_delphi;
  else if(config.Variant == 2)
    CONSOLE->menu = console_menu_lara_cameri;
  else
    CONSOLE->menu = console_menu_lara;
  
  CONSOLE->menu_current_menu = -1;
  CONSOLE->menu_current_idx = 0;
  CONSOLE->menu_current_pos = 0;
  CONSOLE->current_user = NULL;
  CONSOLE->temp_string = NULL;
  CONSOLE->support_list = NULL;
  CONSOLE->list_current_idx = 0;
  CONSOLE->list_current_num = 0;
  CONSOLE->list_current_pos = 0;
  CONSOLE->list_show_cancel = NULL;
  CONSOLE->support_idx = -1;
  CONSOLE->storico_realtime_active = 0;
  CONSOLE->user_tmp.name = NULL;
  CONSOLE->user_tmp.pass = NULL;
  CONSOLE->user_tmp.level = 0;
  CONSOLE->state = 0;
  CONSOLE->pos = 0;
  CONSOLE->head = NULL;
  CONSOLE->tail = NULL;
  
  /* 24/05/2011 - L'anagrafica si azzera solo via DelphiTool
     Anzi no, altrimenti se la centrale è in rete diventa troppo facile
     cancellare l'anagrafica ed accedere al TerminalData per fare quello
     che si vuole. Occorre trovare un modo più sicuro. */
  //if(Restart) unlink(ConsoleUserFileName);
  
  console_load_users();
  
  if(support_find_serial_consumer(PROT_GSM, 0) < 0)
  {
    for(i=0; CONSOLE->menu[i].title && (CONSOLE->menu[i].icon != SMS_MENU); i++);
    if(CONSOLE->menu[i].title) CONSOLE->menu[i].perm = 0;
    
    /* Menu' Messaggi audio solo se e' presente il GSM */
    console_menu_modem[4].perm = 0;
  }
  
#ifdef TEBESA
  CONSOLE->active = 0;
#else
  CONSOLE->active = console_check(dev);
#endif
  
  return 1;
}

void console_loop(ProtDevice *dev)
{
  char buf[24];
  
  if(!console_init(dev)) return;
  
  debug_pid[dev->consumer] = support_getpid();
  sprintf(buf, "CON Init [%d]", debug_pid[dev->consumer]);
  support_log(buf);
  
  pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
  
  console_storico_event_loop(dev);
}

#ifdef TEBESA
void console_start(ProtDevice *dev)
{
  int i;
  
  if(!libuser_started) return;
  
  pthread_create(&(CONSOLE->pth), NULL, (PthreadFunction)console_recv, dev);
  CONSOLE->active = 1;
  CONSOLE->tipo = CONSOLE_STL01;
  for(i=0; i<CONSOLE_STRINGS; i++) *console_str[i] = *console_str1[i];
  delphi_set_lang("TERMDATA", sizeof(console_str)/sizeof(char*), console_str);
  console_login(dev, 0, NULL);
  pthread_join(CONSOLE->pth, NULL);
}

static CALLBACK void* console_stop(ProtDevice *dev, int press, void *null)
{
  timeout_off(CONSOLE->timeout); 
  timeout_off(CONSOLE->ser_timeout); 
  timeout_off(CONSOLE->hour_timeout); 
  
  pthread_mutex_lock(&console_mutex);
  while((CONSOLE->head = support_list_del(CONSOLE->head)));
  CONSOLE->tail = NULL;
  pthread_mutex_unlock(&console_mutex);
  CONSOLE->active = 0;
  
  pthread_cancel(CONSOLE->pth);
  
  return NULL;
}
#endif

/***********************************************************************
	Check if LARA is present
***********************************************************************/

int console_check_lara(ProtDevice *dev)
{
  if(_lara) return 1;
  
  console_send(dev, "ATS00\r");
  console_send(dev, str_14_140);
  console_send(dev, str_14_141);
  
  timeout_on(CONSOLE->timeout, (timeout_func)console_return_to_menu, dev, 0, 2 * TIMEOUT_MSG);
  
  return 0;
}
