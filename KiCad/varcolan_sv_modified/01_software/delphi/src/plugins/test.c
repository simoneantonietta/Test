#include "user.h"
#include "gsm.h"
#include <stdio.h>
#include <unistd.h>

extern int gsm_connected;
extern int gsm_user_close;
extern int gsm_overflow;
extern int console_sms_waitOK;

extern void* Ronda_stazione_p;

static void test_loop(ProtDevice *dev)
{
#if 0
  char buf[64];
  int me;
  
  write(dev->fd, "----\n", 5);
  for(me=0; me<7; me++)
  {
    sprintf(buf, "GSM_Status[%d] = %d\n", me, GSM_Status[me]);
    write(dev->fd, buf, strlen(buf));
  }
  sprintf(buf, "gsm_connected = %d\n", gsm_connected);
  write(dev->fd, buf, strlen(buf));
  sprintf(buf, "gsm_user_close = %d\n", gsm_user_close);
  write(dev->fd, buf, strlen(buf));
  sprintf(buf, "gsm_overflow = %d\n", gsm_overflow);
  write(dev->fd, buf, strlen(buf));
  sprintf(buf, "console_sms_waitOK = %d\n", console_sms_waitOK);
  write(dev->fd, buf, strlen(buf));
  write(dev->fd, "----\n", 5);
  return;
  
  while(1)
  {
    me = read(dev->fd, buf, 10);
    buf[me] = 0;
    if((sscanf(buf, "%d", &me) == 1) && (me < 1024))
    {
      printf("Telecomando %04d\n", me);
      ONME(me,1);
    }
  }
#else
  unsigned short *s;
  int i;
  
/*
  printf("%p\n", Ronda_stazione_p);
  
  s = Ronda_stazione_p;
  for(i=0; i<10; i++) printf("%d\n", *s++);
*/
  
  printf("TEST loop()\n");
  while(1) sleep(1);
#endif
}

#if 1
void _init()
#else
void registra(void) __attribute__((constructor));
void registra()
#endif
{
  printf("Test plugin: " __DATE__ " " __TIME__ "\n");
  prot_plugin_register("TEST", 0, NULL, NULL, (PthreadFunction)test_loop);
}

