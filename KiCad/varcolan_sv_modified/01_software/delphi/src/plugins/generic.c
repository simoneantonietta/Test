#include "protocol.h"
#include "support.h"
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>

static ProtDevice *generic_dev[MAX_NUM_CONSUMER] = {NULL, };

static void generic_loop(ProtDevice *dev)
{
  char buf[32];
  
  pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
  
  sprintf(buf, "Consumatore generico [%d]", getpid());
  support_log(buf);
  
  generic_dev[dev->consumer] = dev;
  
  /* Bloccato per sempre, mantiene la seriale aperta. */
  select(0, NULL, NULL, NULL, NULL);
}

void _init()
{
  printf("Generic plugin: " __DATE__ " " __TIME__ "\n");
  prot_plugin_register("GEN", 0, NULL, NULL, (PthreadFunction)generic_loop);
}

/* Attenzione: la seriale è configurata con controllo di flusso e timeout infinito! */

void COMScrivi(int com, unsigned char *buf, int len)
{
  int i;
  
  /* Trova il consumatore della seriale indicata. */
  for(i=0; i<MAX_NUM_CONSUMER; i++)
    if(generic_dev[i] && (config.consumer[generic_dev[i]->consumer].configured == com)) break;
  if(i == MAX_NUM_CONSUMER) return;
  
  write(generic_dev[i]->fd, buf, len);
}
