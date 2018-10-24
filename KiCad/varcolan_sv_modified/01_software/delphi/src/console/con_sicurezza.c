#include "console.h"
#include "con_sicurezza.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

//#define PASSWD_IMQ

/***********************************************************************
	SICUREZZA menu functions
***********************************************************************/

static pthread_mutex_t console_sicurezza_mutex = PTHREAD_MUTEX_INITIALIZER;

static CALLBACK void* console_sicurezza_pwd3(ProtDevice *dev, int pass_as_int, void *user_as_voidp)
{
  char *pass = (char*)pass_as_int;
  User *user = (User*)user_as_voidp;

  if(strcmp(CONSOLE->temp_string, pass))
  {
    console_send(dev, "ATS00\r");
    console_send(dev, str_12_0);
    console_send(dev, str_12_1);
  }
  else
  {
    pthread_mutex_lock(&console_sicurezza_mutex);
    
    if(!user)
    {
      user = CONSOLE->current_user;
    }
    else
    {
      User *tmp;
      
      tmp = (User*)malloc(sizeof(User));
      memcpy(tmp, user, sizeof(User));
      user = tmp;
      
      tmp = console_user_list;
      while(tmp && tmp->next)
        tmp = tmp->next;
      
      if(!tmp)
        console_user_list = user;
      else
        tmp->next = user;
      
      user->next = NULL;
      CONSOLE->user_tmp.name = NULL;
      CONSOLE->user_tmp.pass = NULL;
    }
    free(user->pass);
    user->pass = CONSOLE->temp_string;
    CONSOLE->temp_string = NULL;
    console_save_users();
    
    pthread_mutex_unlock(&console_sicurezza_mutex);

    console_send(dev, "ATS00\r");
    
    if(user == CONSOLE->current_user)
    {
      console_send(dev, str_12_0);
      console_send(dev, str_12_3);
    }
    else
    {
      console_send(dev, str_12_4);
      console_send(dev, str_12_5);
    }
  }

  timeout_on(CONSOLE->timeout, (timeout_func)console_return_to_menu, dev, 0, TIMEOUT_MSG);

  return NULL;
}

static CALLBACK void* console_sicurezza_pwd2(ProtDevice *dev, int pass_as_int, void *user)
{
#ifndef PASSWD_IMQ
  if(strlen((char*)pass_as_int) > 6)
#else
  if(strlen((char*)pass_as_int) != 6)
#endif
  {
    console_send(dev, "ATS04\r");
#ifndef PASSWD_IMQ
    console_send(dev, str_12_6);
#else
    console_send(dev, "ATN0105INSERIRE 6 CIFRE\r");
#endif
    if(!user)
      CONSOLE->last_menu_item = console_sicurezza_pwd;
    else
      CONSOLE->last_menu_item = console_sicurezza_create;
    timeout_on(CONSOLE->timeout, (timeout_func)console_return_to_last_menu_item, dev, 0, TIMEOUT_MSG);
    return NULL;
  }
  
  free(CONSOLE->temp_string);
  CONSOLE->temp_string = strdup((char*)pass_as_int);

  console_send(dev, str_12_7);
  console_send(dev, str_12_36);
  console_send(dev, str_12_37);

  console_register_callback(dev, KEY_STRING, console_sicurezza_pwd3, user);
  console_register_callback(dev, KEY_CANCEL, console_show_menu, NULL);
  
  return NULL;
}

CALLBACK void* console_sicurezza_pwd(ProtDevice *dev, int press, void *user)
{
  if(!user)
  {
    console_disable_menu(dev, str_12_8);
    console_send(dev, str_12_9);
    if(!CONSOLE->current_user->pass)
    {
      console_send(dev, str_12_10);
      console_send(dev, str_12_11);
    
      timeout_on(CONSOLE->timeout, (timeout_func)console_return_to_menu, dev, 0, TIMEOUT_MSG);
      return NULL;
    }
  }
  else
  {
    console_disable_menu(dev, str_12_12);
  }
  console_send(dev, "ATL121\r");
  console_send(dev, str_12_13);
  console_send(dev, str_12_38);
  console_send(dev, str_12_39);

  console_register_callback(dev, KEY_STRING, console_sicurezza_pwd2, user);
  console_register_callback(dev, KEY_CANCEL, console_show_menu, NULL);
  
  return NULL;
}

static CALLBACK void* console_sicurezza_create3(ProtDevice *dev, int profilo_as_int, void *null)
{
  User *user;
  char cmd[32];
  
  CONSOLE->user_tmp.proglevel = atoi((char*)profilo_as_int);
  
  if((CONSOLE->user_tmp.proglevel < 1) || (CONSOLE->user_tmp.proglevel > CONSOLE_TOP_LEVEL))
  {
    console_reset_callback(dev, 0);
    console_send(dev, "ATS00\r");
    console_send(dev, str_12_14);
    console_send(dev, str_12_15);
    
    timeout_on(CONSOLE->timeout, (timeout_func)console_return_to_menu, dev, 0, TIMEOUT_MSG);
    return NULL;
  }
  
  if(CONSOLE->user_tmp.proglevel != CONSOLE_TOP_LEVEL)
  {
    user = console_user_list;
    while(user && (user->proglevel != CONSOLE_TOP_LEVEL)) user = user->next;
    
    if(!user)
    {
      console_reset_callback(dev, 0);
      console_send(dev, "ATS00\r");
      console_send(dev, str_12_16);
      console_send(dev, str_12_17);
      sprintf(cmd, str_12_18, CONSOLE_TOP_LEVEL);
      console_send(dev, cmd);

      timeout_on(CONSOLE->timeout, (timeout_func)console_return_to_menu, dev, 0, TIMEOUT_MSG*2);
      return NULL;
    }
  }
  
  console_sicurezza_pwd(dev, 0, &CONSOLE->user_tmp);

  return NULL;
}

static CALLBACK void* console_sicurezza_create2(ProtDevice *dev, int idutente_as_int, void *null)
{
  CONSOLE->user_tmp.name = strdup((char*)idutente_as_int);

  console_send(dev, "ATL111\r");
  console_send(dev, str_12_19);
  console_send(dev, str_12_40);
  console_send(dev, str_12_41);

  console_register_callback(dev, KEY_STRING, console_sicurezza_create3, NULL);
  console_register_callback(dev, KEY_CANCEL, console_show_menu, NULL);
  
  return NULL;
}

CALLBACK void* console_sicurezza_create(ProtDevice *dev, int press, void *null)
{
  free(CONSOLE->user_tmp.name);
  free(CONSOLE->user_tmp.pass);
  CONSOLE->user_tmp.name = NULL;
  CONSOLE->user_tmp.pass = NULL;
  CONSOLE->user_tmp.proglevel = 0;
  
  console_disable_menu(dev, str_12_12);
  console_send(dev, "ATL211\r");
  console_send(dev, str_12_21);
  console_send(dev, str_12_38);
  console_send(dev, str_12_39);

  console_register_callback(dev, KEY_STRING, console_sicurezza_create2, NULL);
  console_register_callback(dev, KEY_CANCEL, console_show_menu, NULL);
  
  return NULL;
}

static CALLBACK void* console_sicurezza_delete3(ProtDevice *dev, int idutente_as_int, void *idutente_as_voidp)
{
  User *user, *tmp;
  char *idutente = (char*)idutente_as_voidp;

  pthread_mutex_lock(&console_sicurezza_mutex);

  if(!console_user_list)
  {
    pthread_mutex_unlock(&console_sicurezza_mutex);
    
    console_send(dev, "ATS04\r");
    console_send(dev, str_12_30);
    console_send(dev, str_12_23);
    
    timeout_on(CONSOLE->timeout, (timeout_func)console_return_to_menu, dev, 0, TIMEOUT_MSG);
    return NULL;
  }
  else
  {
    if(!strcmp(idutente, console_user_list->name))
    {
      if(console_user_list == CONSOLE->current_user)
      {
        pthread_mutex_unlock(&console_sicurezza_mutex);
        
        console_send(dev, "ATS04\r");
        console_send(dev, str_12_30);
        console_send(dev, str_12_25);
        
        timeout_on(CONSOLE->timeout, (timeout_func)console_return_to_menu, dev, 0, TIMEOUT_MSG);
        return NULL;
      }
      else
      {
        user = console_user_list->next;
        free(console_user_list->name);
        free(console_user_list->pass);
        free(console_user_list);
        console_user_list = user;
      }
    }
    else
    {
      user = console_user_list;
      while(user->next && (strcmp(idutente, user->next->name))) user = user->next;
      if(!user->next)
      {
        pthread_mutex_unlock(&console_sicurezza_mutex);
        
        console_send(dev, "ATS04\r");
        console_send(dev, str_12_30);
        console_send(dev, str_12_23);
        
        timeout_on(CONSOLE->timeout, (timeout_func)console_return_to_menu, dev, 0, TIMEOUT_MSG);
        return NULL;
      }
      else if(user->next == CONSOLE->current_user)
      {
        pthread_mutex_unlock(&console_sicurezza_mutex);
        
        console_send(dev, "ATS04\r");
        console_send(dev, str_12_28);
        console_send(dev, str_12_25);
        
        timeout_on(CONSOLE->timeout, (timeout_func)console_return_to_menu, dev, 0, TIMEOUT_MSG);
        return NULL;
      }
      else
      {
        tmp = user->next;
        user->next = user->next->next;
        free(tmp->name);
        free(tmp->pass);
        free(tmp);
      }
    }
  }
          
  console_save_users();
  
  pthread_mutex_unlock(&console_sicurezza_mutex);
  
  console_send(dev, "ATS04\r");
  console_send(dev, str_12_30);
  console_send(dev, str_12_31);

  timeout_on(CONSOLE->timeout, (timeout_func)console_return_to_menu, dev, 0, TIMEOUT_MSG);
  
  return NULL;
}

static CALLBACK void* console_sicurezza_delete2(ProtDevice *dev, int press, void *iduser)
{
  free(CONSOLE->temp_string);
  CONSOLE->temp_string = strdup(iduser);

  console_send(dev, "ATS00\r");
  console_send(dev, str_12_32);
  console_send(dev, str_12_33);
  console_send(dev, "ATL000\r");

  console_register_callback(dev, KEY_ENTER, console_sicurezza_delete3, CONSOLE->temp_string);
  console_register_callback(dev, KEY_CANCEL, console_show_menu, NULL);
  
  return NULL;
}

CALLBACK void* console_sicurezza_delete(ProtDevice *dev, int press, void *null)
{
  User *user;
  static char console_sicurezza_level[16];
  
  console_disable_menu(dev, str_12_35);

  console_list_free(CONSOLE->support_list);
  CONSOLE->support_list = NULL;
  
  pthread_mutex_lock(&console_sicurezza_mutex);

  user = console_user_list;
  while(user)
  {
    sprintf(console_sicurezza_level, str_12_34, user->proglevel);
    CONSOLE->support_list = console_list_add(CONSOLE->support_list, user->name, console_sicurezza_level, console_sicurezza_delete2, (int)user->name);
    user = user->next;
  }

  pthread_mutex_unlock(&console_sicurezza_mutex);
  
  console_list_show(dev, CONSOLE->support_list, 0, 3, 0);

  console_register_callback(dev, KEY_CANCEL, console_show_menu, NULL);
  
  return NULL;
}
