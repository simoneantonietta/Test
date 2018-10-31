#include "timeout.h"
#include <stdlib.h>
#include <pthread.h>

static timeout_t *timeout_head = NULL;
static pthread_mutex_t timeout_mutex = PTHREAD_MUTEX_INITIALIZER;

timeout_t* timeout_init()
{
  timeout_t *tmp;
  
  tmp = (timeout_t*)malloc(sizeof(timeout_t));
  if(!tmp) return NULL;
  tmp->active = 0;
  
  pthread_mutex_lock(&timeout_mutex);
  tmp->next = timeout_head;
  timeout_head = tmp;
  pthread_mutex_unlock(&timeout_mutex);

  return tmp;
}

void timeout_on(timeout_t *handle, timeout_func func, void *par1, int par2, int dsecs)
{
  if(handle)
  {
    handle->func = func;
    handle->par1 = par1;
    handle->par2 = par2;
    handle->dsecs = dsecs;
    handle->active = 1;
  }
}

void timeout_off(timeout_t *handle)
{
  handle->active = 0;
}

void timeout_delete(timeout_t *handle)
{
  handle->active = 0;
}

void timeout_check()
{
  timeout_t *tmp, *prev;

  prev = NULL;

  tmp = timeout_head;
  while(tmp)
  {
    if(tmp->active)
    {
      tmp->dsecs--;
      if(tmp->dsecs < 0)
      {
        tmp->active = 0;
        /* qui puo' essere chiamata una funzione di timeout => sem_wait */
        if(tmp->func) tmp->func(tmp->par1, tmp->par2);
      }
    }
    prev = tmp;
    tmp = tmp->next;
  }
}

