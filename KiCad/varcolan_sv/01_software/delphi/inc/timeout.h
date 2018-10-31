#ifndef _SAET_TIMEOUT_H
#define _SAET_TIMEOUT_H

typedef struct _timeout_t {
  void (*func)(void*, int);
  void *par1;
  int par2;
  int dsecs;
  int active;
  struct _timeout_t *next;
} timeout_t;

typedef void (*timeout_func)(void*, int);

//void timeout_initialize();
timeout_t* timeout_init();
void timeout_on(timeout_t *handle, timeout_func func, void *par1, int par2, int dsecs);
void timeout_off(timeout_t *handle);
void timeout_delete(timeout_t *handle);
void timeout_check();

#endif
