#ifndef _CONSOLE_H
#define _CONSOLE_H

#define CONSOLE_STRINGS	1206

#include "console_strings.h"
#include "console_strings1.h"
#include "console_strings2.h"

extern char *str_empty;

#include "../timeout.h"
#include "../protocol.h"

typedef void* (*ConsoleCallback)(ProtDevice*, int, void*);

#ifndef _TEBE_SA_CONSOLE_INCLUDE

#define CALLBACK

#ifndef GVAL2
#define GVAL2(cmd) (((cmd)[0]-'0') * 10 + ((cmd)[1]-'0'))
#endif

typedef struct {
  char				**title;
  int				perm;
  ConsoleCallback	func;
} MenuItem;

typedef struct {
  char		**title;
  int		icon;
  MenuItem	*menu;
  int		perm;
} Menu;

typedef struct _User {
  char	*name;
  char	*pass;
  int	level;
  int	proglevel;
  struct _User *next;
} User;

typedef struct _ListItem {
  char				row1[24];
  char				row2[24];
  ConsoleCallback	func;
  int				param;
  struct _ListItem	*next;
} ListItem;

typedef enum
{
  PersZone,
  PersSensor,
  PersActuator,
  PersPhonebook,
  PersCommand,
  PersCode,
  PersProfile,
  PersArea,
  PersHoliday,
  PersWeekTiming,
  PersTerminal
} PersType;

#endif

enum {KEY_0, KEY_1, KEY_2, KEY_3, KEY_4, KEY_5, KEY_6, KEY_7, KEY_8, KEY_9, KEY_ASTERISK, KEY_DIERESIS, KEY_UP, KEY_DOWN, KEY_LEFT, KEY_RIGHT, KEY_ENTER, KEY_CANCEL, KEY_STRING, KEY_MENU, KEY_TIMEOUT, KEY_MAX};

#define L1	1L<<0
#define L2	1L<<1
#define L3	1L<<2
#define L4	1L<<3
#define L5	1L<<4
#define L6	1L<<5
#define L7	1L<<6
#define L8	1L<<7

#define CONSOLE_TOP_LEVEL  8

#define TIMEOUT_MSG		15
#define TIMEOUT_LOGIN	400
#define COMMAND_DIM		16

#define DIM_REALTIME		15
#define DIM_CONSOLIDATED	150

#define CONSOLE_STL01	0
#define CONSOLE_STL02	1

typedef struct {
  char *cmd;
  int active;
  timeout_t *timeout;
  timeout_t *ser_timeout;
  timeout_t *hour_timeout;
  pthread_t pth;
  struct support_list *head;
  struct support_list *tail;
  int resend_counter;
  ConsoleCallback callback[KEY_MAX];
  void *callback_arg[KEY_MAX];
  ConsoleCallback current_callback[KEY_MAX];
  void *current_callback_arg[KEY_MAX];
#ifndef _TEBE_SA_CONSOLE_INCLUDE
  Menu *menu;
#else
  void *menu;
#endif
  ConsoleCallback last_menu_item;
  int menu_current_menu;
  int menu_current_idx;
  int menu_current_pos;
#ifndef _TEBE_SA_CONSOLE_INCLUDE
  User *current_user;
#else
  void *current_user;
#endif
  char *temp_string;
#ifndef _TEBE_SA_CONSOLE_INCLUDE
  ListItem *support_list;
#else
  void *support_list;
#endif
  int list_current_idx;
  int list_current_num;
  int list_current_pos;
  char list_digits[5];
  ConsoleCallback list_show_cancel;
  int support_idx;
  int impostazioni_range;
  int storico_realtime_active;
  Event storico_events_copy[DIM_CONSOLIDATED];
#ifndef _TEBE_SA_CONSOLE_INCLUDE
  User user_tmp;
#else
  char user_tmp[20];
#endif
  ConsoleCallback liste_accept_func;
  char impostazioni_buf1[8];
  char impostazioni_buf2[8];
  int impostazioni_buf_digits;
  int impostazioni_buf_xpos;
  int impostazioni_buf_ypos;
  ConsoleCallback impostazioni_enter;
  ConsoleCallback impostazioni_cancel;
  int state;
  int pos;
#ifndef _TEBE_SA_CONSOLE_INCLUDE
  PersType personalizza_string;
#else
  int personalizza_string;
#endif
  int tipo;
} ConsoleData;

void console_loop(ProtDevice *dev);

#ifdef _TEBE_SA_CONSOLE_INCLUDE

void console_start(ProtDevice *dev);

#else

#define CONSOLE ((ConsoleData*)(((ProtDevice*)dev)->prot_data))

void console_sms_message(char *msg, int selint);

void console_register_callback(ProtDevice *dev, int idx, ConsoleCallback func, void *arg);
void console_reset_callback(ProtDevice *dev, int logout);
void console_return_to_last_menu_item(char *null1, int null2);
void console_send(ProtDevice *dev, char *cmd);

ListItem* console_list_add(ListItem *list, char *row1, char *row2, ConsoleCallback func, int param);
void console_list_free(ListItem *list);
int console_list_count(ListItem *list);
void console_list_show(ProtDevice *dev, ListItem *list, int idx, int num, int pos);
void console_submenu(ProtDevice *dev, MenuItem *menu, int param, int num);
void console_return_to_menu(char *dev, int null2);
void console_disable_menu(ProtDevice *dev, char *title);
char* console_format_time(ProtDevice *dev, time_t t);
char* console_format_date(ProtDevice *dev, time_t t);
void console_save_users();
CALLBACK void* console_show_menu(ProtDevice *dev, int press, void *null);
CALLBACK void* console_logout(ProtDevice *dev, int press, void *null);

int console_check_lara(ProtDevice *dev);

extern char **console_months[];
extern int console_login_timeout_time;
extern User *console_user_list;

#define SMS_MENU	7
#define LOGOUT_MENU	14
#define CHIAVI_MENU	17
#define RADIO_MENU	15

extern Menu console_menu_delphi[];

#endif

typedef void (*ConsoleSetLangFunc)(int);
void console_register_lang(ConsoleSetLangFunc func);
extern int console_localization;

#endif
