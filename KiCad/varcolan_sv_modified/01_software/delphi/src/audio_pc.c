#include "audio.h"
#include "delphi.h"
#include "support.h"
#include "serial.h"
#include "timeout.h"
#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <pthread.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>

unsigned int audio_version;

int audio_init(int fd)
{
  audio_version = 0;
  return 0;
}

void audio_reset(int fd)
{
}

int audio_play(int fd, int num, int now)
{
  return 0;
}

int audio_play_base(int fd, int audiolen, unsigned char *audiobuf, int *cont, int *errori)
{
  return 0;
}

int audio_play_stop()
{
  return 1;
}

int audio_rec_prepare(int num)
{
  return 0;
}

int audio_rec_start()
{
  return 1;
}

static void* audio_rec_store(void *num)
{
  return NULL;
}

int audio_rec_stop(int store)
{
  return 0;
}

int audio_rec(int fd)
{
  return 0;
}

int audio_ready()
{
  return 0;
}

