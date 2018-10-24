#include "protocol.h"
#include "codec.h"
#include "event.h"
#include "database.h"
#include "support.h"
#include <unistd.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>

static void equitalia_loop(ProtDevice *dev)
{
  static int stato = 0;
  char buf[256];
  
  pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
  
  support_log("Equitalia (debug) Init");
  
  while(1)
  {
    if((!stato && (ORE == 6) && ((MINUTI == 40) || (MINUTI == 41))) ||
//       (!stato && (ORE == 15) && ((MINUTI == 40) || (MINUTI == 41))) ||
       (!stato && (ORE == 21) && ((MINUTI == 40) || (MINUTI == 41))))
    {
      stato = 1;
      sprintf(buf, "tar -czf /tmp/09%02d%02d_%02d%02d.tar.gz /tmp/saet.log.*", MESE, GIORNO, ORE, MINUTI);
      system(buf);
    }
    else if(MINUTI > 41)
      stato = 0;
    sleep(60);
  }
}

void _init()
{
  pthread_t pth;
  
  printf("Equitalia (debug) plugin: " __DATE__ " " __TIME__ "\n");
  
  pthread_create(&pth,  NULL, (PthreadFunction)equitalia_loop, NULL);
  pthread_detach(pth);
}

