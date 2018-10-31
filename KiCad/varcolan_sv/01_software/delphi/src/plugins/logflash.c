#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <unistd.h>
#include <pthread.h>

#include "support.h"
#include "database.h"

static const char LogFileName[] = "/tmp/storage/saet.log.0";
static const char LogFileName_tmp[] = "/tmp/storage/saet.log.1";

static int logfd[2];

static void* logflash_loop(void *null)
{
  FILE *fp;
  struct stat st;
  char log[128];
  int n;
  
  write(logfd[0], "FlashLog Avvio", 14);
  
  mkdir("/tmp/storage", 0777);
  
  while(1)
  {
    n = read(logfd[1], log, 127);
    if(n > 0)
    {
      log[n] = 0;
      
      if(!stat(LogFileName, &st) && (st.st_size > config.LogLimit))
        rename(LogFileName, LogFileName_tmp);
      
      if((fp = fopen(LogFileName, "a")))
      {
        fprintf(fp, "%02d/%02d/%02d %02d:%02d:%02d - %s\n", GIORNO, MESE, ANNO, ORE, MINUTI, SECONDI, log);
        fclose(fp);
      }
    }
    else
      return NULL;
  }
  
  return NULL;
}

static void logflash(char *log)
{
  write(logfd[0], log, strlen(log));
}

void _init()
{
  pthread_t pth;
  
  printf("LogFlash (plugin): " __DATE__ " " __TIME__ "\n");
  
  if(socketpair(AF_UNIX, SOCK_DGRAM, 0, logfd)) return;
  pthread_create(&pth, NULL, logflash_loop, NULL);
  pthread_detach(pth);
  
  support_log_hook(logflash);
}
