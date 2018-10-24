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
#include <alsa/asoundlib.h>

unsigned int audio_version;

static int audio_msg = 0;
static int audio_len = 0;
static int audio_idx = 0;
static void *audio_buf = 0;

/* 60s di registrazione */
#define MAX_AUDIO_REC 960000

struct {
  int num;
  int len;
  unsigned char *data;
} audio_msg_priv = {0, };


#define PERIOD_SIZE 256
snd_pcm_t *audio_handle = NULL;

const unsigned int wave_fmt_header[7] =
  {0x20746d66, 16, 0x00010001, 8000, 16000, 0x00100002, 0x61746164};

int audio_init(int fd)
{
  /* Abilitazione ContactID */
  audio_version = 0x020000;
  
  return 1;
}

void audio_reset(int fd)
{
}

static int audio_queue()
{
  int err;
  
  err = snd_pcm_writei(audio_handle, audio_buf+audio_idx, PERIOD_SIZE);
  if(err >= 0)
  {
    audio_idx += err*sizeof(short);
    if(audio_idx > audio_len)
      audio_idx = 512;
  }
  return err;
}

static void audio_play_callback(snd_async_handler_t *ahandler)
{
  snd_pcm_t *handle = snd_async_handler_get_pcm(ahandler);
  snd_pcm_sframes_t avail;
  int err;
  
  avail = snd_pcm_avail_update(handle);
  while (avail >= PERIOD_SIZE)
  {
    err = audio_queue();
    if(err < 0) return;
    avail = snd_pcm_avail_update(handle);
  }
}

int audio_play(int fd, int num, int now)
{
  FILE *fp;
  char data[64];
  struct stat st;
  snd_pcm_hw_params_t *hw_params;
  snd_async_handler_t *ahandler;
  int i, err;
  
  sprintf(data, DATA_DIR "/%d.wav", num);
  fp = fopen(data, "r");
  if(!fp)
  {
    sprintf(data, DATA_DIR "/%d.pcm", num);
    fp = fopen(data, "r");
    if(!fp) return 0;
    
    support_log("Formato PCM non ancora supportato.");
    fclose(fp);
    return 0;
  }
  
  if(stat(data, &st)) return 0;
  audio_len = st.st_size;
  audio_idx = 512;
  audio_buf = malloc(audio_len + 512);
  if(!audio_buf)
  {
    fclose(fp);
    audio_len = 0;
    return 0;
  }
  
  fread(audio_buf, 1, audio_len, fp);
  fclose(fp);
  memset(audio_buf+audio_len, 0, 512);
  
  if((err = snd_pcm_open(&audio_handle, "default", SND_PCM_STREAM_PLAYBACK, 0)) < 0)
  {
    support_log("Dispositivo audio non trovato.");
    return 0;
  }
  
  i = 8000;
  if((snd_pcm_hw_params_malloc(&hw_params) < 0) ||
     (snd_pcm_hw_params_any(audio_handle, hw_params) < 0) ||
     (snd_pcm_hw_params_set_access(audio_handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED) < 0) ||
     (snd_pcm_hw_params_set_format(audio_handle, hw_params, SND_PCM_FORMAT_S16_LE) < 0) ||
     (snd_pcm_hw_params_set_rate_near(audio_handle, hw_params, &i, 0) < 0) ||
     (snd_pcm_hw_params_set_channels(audio_handle, hw_params, 1) < 0) ||
     (snd_pcm_hw_params(audio_handle, hw_params) < 0))
  {
    snd_pcm_close(audio_handle);
    audio_handle = NULL;
    return 0;
  }
  
  snd_pcm_hw_params_free(hw_params);
  
  /* Crea l'handler, poi accoda due periodi e avvia la riproduzione.
     L'handler viene chiamato ogni volta che termina un periodo e
     se ne accoda uno nuovo mentre l'altro è in riproduzione.
     Perciò occorre che l'handler verifichi se non sia stata fermata
     la riproduzione e in quel caso non accoda più ma chiude il
     dispositivo. Sempre se sia corretto chiudere dentro l'handler.
     In realtà c'è ancora un periodo in riproduzione, ma non è detto
     che possa esaurirsi se il gsm interrompe il flusso. Occorre
     proprio chiudere. Forse lo può fare lo stop(). */
  err = snd_async_add_pcm_handler(&ahandler, audio_handle, audio_play_callback, NULL);
  /* La gestione tramite writei() pare vada a blocchi di 1K-frames, quindi devo accodare
     almeno 8 blocchi di 256 frames per avere una gestione corretta del callback. */
  for(i=0; i<8; i++) audio_queue();
  
  if(snd_pcm_state(audio_handle) == SND_PCM_STATE_PREPARED)
    err = snd_pcm_start(audio_handle);
  
  return 1;
}

int audio_play_stop()
{
  if(audio_len)
  {
    snd_pcm_close(audio_handle);
    free(audio_buf);
    audio_buf = NULL;
    audio_len = 0;
  }
  
  return 1;
}

int audio_rec_prepare(int num)
{
  char log[40];
  
  sprintf(log, "Preparazione messaggio %d", num);
  support_log(log);
  
  if(num && audio_msg_priv.num) return 0;
  audio_msg_priv.data = malloc(MAX_AUDIO_REC);
  if(!audio_msg_priv.data) return 0;
  audio_msg_priv.num = num;
  audio_msg_priv.len = 0;
  audio_handle = NULL;
  return 1;
}

static void audio_rec_callback(snd_async_handler_t *ahandler)
{
  /* Su iMX51 il callback funziona solo se si chiedono 2048 frames.
     Non ne arrivano di più, e di meno manda in overrun. */
  /* Anche 1024 va bene, la callback viene chiamata più frequente.
     Con 512 invece si va in overrun. */
  snd_pcm_t *handle = snd_async_handler_get_pcm(ahandler);
  int err;
  
  if(audio_msg_priv.len < (MAX_AUDIO_REC-1024*sizeof(short)))
  {
    err = snd_pcm_readi (handle, audio_msg_priv.data+audio_msg_priv.len, 1024);
    if(err < 0)
    {
      /* Overrun - faccio ripartire */
      snd_pcm_start(handle);
    }
    else
      audio_msg_priv.len += err*sizeof(short);
  }
}

int audio_rec_start()
{
  int err;
  char buf[32];
  
  if(snd_pcm_state(audio_handle) == SND_PCM_STATE_PREPARED)
    err = snd_pcm_start(audio_handle);
    
  sprintf(buf, "Avvio registrazione %d", audio_msg_priv.num);
  support_log(buf);
  
  return 1;
}

static void* audio_rec_store(void *null)
{
  int ret;
  FILE *fp2;
  char buf[512];
  
  sprintf(buf, DATA_DIR "/%d.wav", audio_msg_priv.num);
  fp2 = fopen(buf, "w");
  if(fp2)
  {
    /* RIFF WAV format */
    ret = 0x46464952;
    fwrite(&ret, 1, 4, fp2);
    ret = 36 + audio_msg_priv.len + 12 + 456;
    fwrite(&ret, 1, 4, fp2);
    ret = 0x45564157;
    fwrite(&ret, 1, 4, fp2);
    
    ret = 0x65646f63;	// code
    fwrite(&ret, 1, 4, fp2);
    ret = 4+456;
    fwrite(&ret, 1, 4, fp2);
    ret = audio_msg_priv.num;
    fwrite(&ret, 1, 4, fp2);
    /* Uso questo chunk per allineare i dati audio
       affinché partano esattamente dall'inizio del settore. */
    memset(buf, 0, 456);
    fwrite(buf, 1, 456, fp2);
    
    fwrite(wave_fmt_header, 1, sizeof(wave_fmt_header), fp2);
    fwrite(&audio_msg_priv.len, 1, 4, fp2);
    fwrite(audio_msg_priv.data, 1, audio_msg_priv.len, fp2);
    fclose(fp2);
    sync();
  }
  audio_msg_priv.num = 0;
  free(audio_msg_priv.data);
  audio_msg_priv.data = NULL;
  
  support_log("Audio store");
  return NULL;
}

int audio_rec_stop(int store)
{
  struct stat st;
  pthread_t pth;
  
  if(audio_handle)
  {
    snd_pcm_close(audio_handle);
    support_log("Fine registrazione");
  }
  
  if(audio_msg_priv.num && store)
  {
    pthread_create(&pth, NULL, audio_rec_store, NULL);
    pthread_detach(pth);
  }
  else if(audio_msg_priv.data)
  {
    audio_msg_priv.num = 0;
    free(audio_msg_priv.data);
    audio_msg_priv.data = NULL;
  }
  
  return (audio_msg_priv.len+8000)/16000;
}

int audio_rec(int fd)
{
  snd_pcm_hw_params_t *hw_params;
  snd_async_handler_t *ahandler;
  int err;
  
  if((err = snd_pcm_open(&audio_handle, "default", SND_PCM_STREAM_CAPTURE, 0)) < 0)
  {
    support_log("Dispositivo audio non trovato.");
    return 0;
  }
  
  err = 8000;
  if((snd_pcm_hw_params_malloc(&hw_params) < 0) ||
     (snd_pcm_hw_params_any(audio_handle, hw_params) < 0) ||
     (snd_pcm_hw_params_set_access(audio_handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED) < 0) ||
     (snd_pcm_hw_params_set_format(audio_handle, hw_params, SND_PCM_FORMAT_S16_LE) < 0) ||
     (snd_pcm_hw_params_set_rate_near(audio_handle, hw_params, &err, 0) < 0) ||
     (snd_pcm_hw_params_set_channels(audio_handle, hw_params, 1) < 0) ||
     (snd_pcm_hw_params(audio_handle, hw_params) < 0))
  {
    snd_pcm_close(audio_handle);
    audio_handle = NULL;
    return 0;
  }
  
  snd_pcm_hw_params_free(hw_params);
  
  err = snd_async_add_pcm_handler(&ahandler, audio_handle, audio_rec_callback, NULL);
  
  support_log("Attesa avvio registrazione");
  
  return 1;
}

int audio_ready()
{
  if(!audio_msg_priv.num) return 0;
  return 1;
}

