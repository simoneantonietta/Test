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

/*
play -t raw -r 8000 -b 8 -c 1 -e u-law 1.pcm
*/

//#define DEBUG_AUDIO
//#error
//#define support_log(x) support_log2(x)

/* 0: no audio  1: audio gsm con echo  2: audio gsm senza echo */
static int audio_type = 0;

static volatile int audio_recording = 0;
static int audio_msg = 0;
static int audio_stop = 0;
static pthread_mutex_t audio_rec_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t audio_stop_mutex = PTHREAD_MUTEX_INITIALIZER;
static sem_t audio_rec_sem;

unsigned int audio_version;

int audio_init(int fd)
{
  int i, n;
  unsigned char data[256];
  char byte;
  FILE *fp;
  
  audio_type = 0;
  
  ser_setspeed(fd, B115200 | CRTSCTS, 2);
  
  /* Il modulo audio viene resettato precedentemente a parte. */
#if 0
  /* Per essere sicuro che il sistema audio non sia bloccato in qualche stato
     non previsto (tipicamente in registrazione), invio un buffer completo di
     comandi UART_STOP. */
  memset(data, 3, 256);
  write(fd, data, 256);
  while(read(fd, data, 256));
#endif
  
  byte = 4;	// richiesta versione
  write(fd, &byte, 1);
  
  i = 0;
  while((n = read(fd, data+i, 256-i)) > 0) i += n;
  
  if((i == 5) && (data[0] == 4) && (data[1] == 0x0a))
  {
    n = 1;
    audio_type = 1;
    audio_version = MKINT(data[1], data[2], data[3], data[4]);
  }
  else if((i == 4) && (data[0] == 0x0a))
  {
    n = 0;
    audio_type = 2;
    audio_version = MKINT(data[0], data[1], data[2], data[3]);
  }
  else if((i == 1) && (data[0] == 4))
  {
    n = 0;
    audio_type = 1;
    *(int*)data = 0;
    audio_version = 0;
  }
  
  if(audio_type)
  {
    fp = fopen(VersionFile, "a");
    fprintf(fp, "audio: firmware %d.%d.%d.%d (%d)\n",
      (audio_version>>24)&0xff, (audio_version>>16)&0xff, (audio_version>>8)&0xff,
      audio_version&0xff, audio_type);
    fclose(fp);
    
    if((audio_version & 0xff0000) >= 0x020000)
    {
      /* Se ho già una calibrazione, la recupero. */
      fp = fopen("/saet/audiocal.bin", "r");
      if(fp)
      {
        if((n=fgetc(fp)) > 0)
        {
          data[0] = 0xaa;
          data[1] = n;
          write(fd, &data, 2);
          read(fd, data, 256);
        }
        fclose(fp);
      }
    }
  }
  
  ser_setspeed(fd, B19200 | CRTSCTS, 1);
  
  return audio_type;
}

void audio_reset(int fd)
{
  unsigned char data[256];
  
  ser_setspeed(fd, B115200 | CRTSCTS, 2);
  
  /* Per essere sicuro che il sistema audio non sia bloccato in qualche stato
     non previsto (tipicamente in registrazione), invio un buffer completo di
     comandi UART_STOP. */
  memset(data, 3, 256);
  write(fd, data, 256);
  while(read(fd, data, 256) > 0);
  
  ser_setspeed(fd, B19200 | CRTSCTS, 1);
}

#if 0
static void audio_play_timeout(void *null1, int null2)
{
  pthread_mutex_lock(&audio_stop_mutex);
  support_log("audio: timeout riproduzione");
  audio_stop = 1;
  pthread_mutex_unlock(&audio_stop_mutex);
}
#endif

//int xxxxx;

int audio_play(int fd, int num, int now)
{
  FILE *fp;
  char data[256];
  int n, cont, errori;
  struct timeval to;
  fd_set fdr;
  struct stat st;
  unsigned char *audiobuf;
  int audiolen;
//  struct sched_param prio;
  
  sprintf(data, DATA_DIR "/%d.pcm", num);
  
  fp = fopen(data, "r");
  if(!fp) return 0;
  
  if(stat(data, &st)) return 0;
  audiolen = st.st_size;
  audiobuf = malloc(audiolen + 256);
  if(!audiobuf) return 0;
  
  fread(audiobuf, 1, audiolen, fp);
  fclose(fp);
  memset(audiobuf + audiolen, 0x7f, 256);
  /* Elimina le sequenze "AT" e "at" dal flusso dati audio. */
  for(n=0; n<audiolen; n++)
  {
//    if(((audiobuf[n] == 'A') && (audiobuf[n+1] == 'T')) ||
//       ((audiobuf[n] == 'a') && (audiobuf[n+1] == 't')))

//    if(((audiobuf[n] == 'A') || (audiobuf[n] == 'a') || (audiobuf[n] == 0xe1)) &&
//       ((audiobuf[n+1] == 'T') || (audiobuf[n+1] == 't') || (audiobuf[n+1] == 0xf4)))
    
/* Nota: il b5 è presente spesso, ma da fastidio solo raramente. Forse dipende dalla sequenza.
   Nel caso in questione 10 b5 85, ho verificato che sia alterando il b5 sia il 10, si elimina
   comunque il blocco. */
    if((audiobuf[n] == 'T') || (audiobuf[n] == 't') ||
       (audiobuf[n] == 0xf4) || (audiobuf[n] == 0xd4) ||
       (audiobuf[n] == 0xd1) || (audiobuf[n] == 0xb5))
      audiobuf[n] ^= 0x01;
  }
  
//  prio.sched_priority = 99;
//  sched_setscheduler(getpid(), SCHED_FIFO, &prio);
  
  pthread_mutex_lock(&audio_stop_mutex);
  audio_stop = 0;
  pthread_mutex_unlock(&audio_stop_mutex);
  
  sprintf(data, "audio: riproduzione %d", num);
  support_log(data);
#ifdef DEBUG_AUDIO
printf("%s\n", data);
#endif
  
  if(now)
  {
    /* I nuovi GSM rispondono OK solo quando viene tirata su la
       cornetta dall'altra parte, oppure NO CARRIER se non risponde
       nessuno. */
    /* Quindi non ho bisogno di attendere nulla prima di riprodurre
       l'audio, quando arrivo qui è perché si è già pronti. */
  }
  else
  {
/*
    Attende che ci sia la risposta dall'altra parte.
    Verificare il riconoscimento del tono di squillo remoto.
*/
    FD_ZERO(&fdr);
    FD_SET(fd, &fdr);
//    to.tv_sec = 10;
    to.tv_sec = 3;
    to.tv_usec = 0;
    n = select(fd+1, &fdr, NULL, NULL, &to);
    /* Se ricevo qualcosa prima di iniziare la riproduzione e' perche' ho ricevuto NO CARRIER */
    if(n)
    {
      support_log("audio: riproduzione interrotta");
#ifdef DEBUG_AUDIO
printf("audio: riproduzione interrotta\n");
#endif
      free(audiobuf);
      return 1;
    }
  }
  
/*
   Anziché attivare il timeout, conto il numero di pacchetti audio.
   1 secondi = 31.25 pacchetti
   20 secondi = 625 pacchetti
   60 secondi = 1875 pacchetti
*/
  
  support_log("audio: inizio riproduzione");
#ifdef DEBUG_AUDIO
printf("audio: inizio riproduzione\n");
#endif
  
  /* La mancata riproduzione può essere dovuta al fatto che
     il GSM per qualche motivo tiene bloccata la seriale con
     il controllo di flusso? Ma perché dovrebbe, visto che
     dovrei avere comunque la possibilità di ricevere e
     inviare comandi sulla linea?
     Comunque, visto che tutto sommato non ha senso che la
     riproduzione sia legata al controllo di flusso del GSM,
     imposto la seriale senza controllo. */
  /* Per sicurezza l'impostazione diversa la faccio solo se
     ho il gsm di tipo nuovo. */
  if(audio_type == 2)
    ser_setspeed(fd, B115200, 1);
  else
    ser_setspeed(fd, B115200 | CRTSCTS, 1);
  
  /* Per essere sicuro che il sistema audio non sia bloccato in qualche stato
     non previsto (tipicamente in registrazione), invio un buffer completo di
     comandi UART_STOP. */
  memset(data, 3, 256);
  write(fd, data, 256);
  while(read(fd, data, 256) > 0);
  
  cont = 0;
  errori = 0;
  while(!audio_play_base(fd, audiolen, audiobuf, &cont, &errori));
  
  support_log("audio: fine riproduzione");
#ifdef DEBUG_AUDIO
printf("audio: fine riproduzione\n");
#endif
  
  /* Per essere sicuro che il sistema audio non sia bloccato in qualche stato
     non previsto (tipicamente in registrazione), invio un buffer completo di
     comandi UART_STOP. */
  memset(data, 3, 256);
  write(fd, data, 256);
  
  //tcdrain(fd); /* rimozione 14-06-2012 */
  /* dopotutto perché aspettare l'invio completo? Aspetta già abbastanza la
     while() che segue, e almeno non è bloccante a meno che non riceva
     continuamente qualcosa, ma non vedo il perché. */
  
  while(read(fd, data, 256) > 0);
  
//  ser_setspeed(fd, B2400 | CRTSCTS, 0);
//  ser_setspeed(fd, config.consumer[gsm_consumer].data.serial.baud1 | CRTSCTS, 0);
  ser_setspeed(fd, B19200 | CRTSCTS, 1);
  
  free(audiobuf);
  
  support_log("audio: fine"); /* 14-06-2012 */
  return 1;
}

int audio_play_base(int fd, int audiolen, unsigned char *audiobuf, int *cont, int *errori)
{
  int i, n, ptr;
  unsigned char data[258];
  unsigned char byte;
  
  static int errori_old;
  
  if(!*errori) errori_old = 0;
  
  pthread_mutex_lock(&audio_stop_mutex);
  ptr = 0;
  while(!audio_stop && (ptr < audiolen))
  {
//support_log("Invio blocco audio");
    pthread_mutex_unlock(&audio_stop_mutex);
    byte = 1;
    write(fd, &byte, 1);
    write(fd, audiobuf + ptr, 256);
    (*cont)++;
    if(*cont >= 1875)
    {
      audio_play_stop();
      support_log("audio: timeout riproduzione");
    }
//support_log("  lettura eco audio");
    
    if(audio_type == 1)
    {
#if 0
      /* Gestione echo del GSM */
      i = 258;
      while(i)
      {
        n = read(fd, data+258-i, i);
        if(!n)
        {
          support_log("audio: errore lettura");
#ifdef DEBUG_AUDIO
printf("audio: errore lettura\n");
#endif
          break;
        }
        i -= n;
      }
      for(i=1; i<257; i++)
      {
        if(audiobuf[ptr+i-1] != data[i])
        {
          (*errori)++;
          if(*errori > 16) break;
        }
      }
      
      if((i != 257) || (data[257] != ACK))
        audio_play_stop();
#else
      /* Gestione echo del GSM */
      i = 258;
      while(i)
      {
        n = read(fd, data+258-i, i);
        if(!n) break;
        i -= n;
      }
      
      /* Tre casi:
         1. ricevo il pacchetto completo come atteso + ACK
         2. ricevo il pacchetto completo come atteso senza ACK
         3. ricevo un pacchetto totalmente corrotto
      */
      
      if(!n)
      {
        *cont += 3;
      }
      
      if(i <= 1)	// casi 1 e 2?
      {
        for(n=1; n<257; n++)
        {
          if(audiobuf[ptr+n-1] != data[n])
          {
            /* Il pacchetto è corrotto, anche se la dimensione è buona. */
            /* Quindi caso 3. */
            *errori = 1;
            audio_play_stop();
            break;
          }
          if(n == 257)
          {
            /* Pacchetto buono, verifico ACK */
            if(i || (data[257] != ACK))
            {
              /* Non ho ricevuto l'ACK, per sicurezza invio
                 un pacchetto di stop per riallineare. */
              support_log("audio: timeout ACK, continua");
              tcflush(fd, TCOFLUSH);
              memset(data, 3, 256);
              write(fd, data, 256);
            }
          }
        }
      }
      else
      {
        /* Caso 3 */
        *errori = 1;
        audio_play_stop();
      }
#endif
    }
    else if(audio_type == 2)
    {
      /* Il GSM non fa echo */
      n = read(fd, data, 258);
      if((n != 1) || (data[0] != ACK))
      {
          /* Se sono arrivato qui perché non è stato ricevuto nulla, sono stati
             impiegati 100ms pari a 3 pacchetti, che devo contare altrimenti il
             timeout di fine chiamata si allunga significativamente, soprattutto
             se poi si continua a non ricevere nulla. */
          if(!n)
          {
#if 1
            /*if(!*errori)*/ support_log("audio: timeout ACK, continua");
            *cont += 3;
            /* Ma soprattutto se è andato in timeout, è comunque probabile che
               l'AVR abbia perso qualche byte in ricezione e si trovi quindi con
               il buffer non completo. Se non lo riallineo, il pacchetto successivo
               viene in parte consumato per completare il pacchetto precedente,
               dopodiché i byte del pacchetto vengono scartati o peggio interpretati
               come comandi. Ma anche non fossero interpretati come comandi, finirebbe
               che i buffer rimarrebbero disallienati.
               Per questo motivo, prima di proseguire con il pacchetto successivo,
               invio un pacchetto di STOP per essere sicuro di riportare il sistema
               a posto. */
            /* E' anche possibile che il gsm si sia spento per qualche ragione e
               quindi il CTS blocchi la trasmissione. Non voglio saturare il buffer
               in uscita (che arrivato a 8K blocca in scrittura e non se ne esce più).
               Quindi a scanso di equivoci faccio anche un flush della seriale. */
            tcflush(fd, TCOFLUSH);
            memset(data, 3, 256);
            write(fd, data, 256);
#else
#warning DEBUG
            *cont += 3;
#endif
          }
          else
          {
            /* Se invece ci sono state ricezioni anomale, probabilmente è il
               messaggio del GSM che comporta la chiusura della chiamata. */
               
            /* Assicuro di pulire il buffer in ricezione, eventuali errori
               precedenti possono essere dovuti a qualche segnalazione del gsm,
               prima che arrivi l'ACK, e quindi lo aspetto. Aspetto per 32ms
               che è il tempo complessivo di trasmissione di un buffer intero. */
            fd_set fds;
            struct timeval to;
            
//            if(!*errori) support_log("audio: errore lettura, continua");
            FD_ZERO(&fds);
            FD_SET(fd, &fds);
            to.tv_sec = 0;
            to.tv_usec = 33000;	// 33ms per marginarmi
            while(select(fd+1, &fds, NULL, NULL, &to)) read(fd, data, 258);
            (*cont)++;
            support_log("audio: errore lettura");
            audio_play_stop();
            *errori = 1;
          }
      }
    }
    else
    {
      *errori = 1;
      support_log("audio: non rilevato");
      audio_play_stop();
    }
    
    ptr += 256;
    
//support_log("  lettura eco audio ok");
    
    if(*errori != errori_old)
    {
      errori_old = *errori;
      sprintf(data, "audio: rilevati %d errori", *errori);
      support_log(data);
#ifdef DEBUG_AUDIO
printf("%s\n", data);
#endif
    }
    
    pthread_mutex_lock(&audio_stop_mutex);
  }
  pthread_mutex_unlock(&audio_stop_mutex);
  
  return audio_stop;
}

int audio_play_stop()
{
  pthread_mutex_lock(&audio_stop_mutex);
#ifdef DEBUG_AUDIO
printf("Audio stop\n");
#endif
  audio_stop = 1;
  pthread_mutex_unlock(&audio_stop_mutex);
  return 1;
}

int audio_rec_prepare(int num)
{
  char log[40];
  
  sprintf(log, "Preparazione messaggio %d", num);
  support_log(log);
  
  if(num && audio_msg) return 0;
  audio_msg = num;
  sem_init(&audio_rec_sem, 0, 0);
  return 1;
}

int audio_rec_start()
{
  sem_post(&audio_rec_sem);
  return 1;
}

static void* audio_rec_store(void *num)
{
  int len;
  FILE *fp1, *fp2;
  char buf[256];
  
  fp1 = fopen("/tmp/rec.pcm", "r");
  if(fp1)
  {
    sprintf(buf, DATA_DIR "/%d.pcm", (int)num);
    fp2 = fopen(buf, "w");
    if(fp2)
    {
      while((len = fread(buf, 1, 256, fp1)) > 0)
        fwrite(buf, 1, len, fp2);
      fclose(fp2);
    }
    fclose(fp1);
    unlink("/tmp/rec.pcm");
  }
  support_log("Audio store");
  return NULL;
}

int audio_rec_stop(int store)
{
  struct stat st;
  pthread_t pth;
  
  audio_recording = 0;
  /* se il thread di registrazione era in attesa dell'inizio lo fermo */
  sem_post(&audio_rec_sem);
  
  pthread_mutex_lock(&audio_rec_mutex);
  st.st_size = 0;
  if(audio_msg && store)
  {
    stat("/tmp/rec.pcm", &st);
    pthread_create(&pth, NULL, audio_rec_store, (void*)audio_msg);
    pthread_detach(pth);
  }
  
  audio_msg = 0;
  pthread_mutex_unlock(&audio_rec_mutex);
  
  return (st.st_size+4000)/8000;
}

int audio_rec(int fd)
{
  FILE *fp;
  int len, total;
  char data[256];
  char byte;
  
  pthread_mutex_lock(&audio_rec_mutex);
  if(!audio_msg) return 0;
  fp = fopen("/tmp/rec.pcm", "w");
  if(!fp)
  {
    pthread_mutex_unlock(&audio_rec_mutex);
    return 1;
  }
  
#if 0
printf("Segnale acustico\n");
  sleep(1);
  audio_play_base(fd, 100);	// beep
#endif
  
  ser_setspeed(fd, B115200 | CRTSCTS, 5);
  audio_recording = 1;
  
  support_log("Attesa avvio registrazione");
  sem_wait(&audio_rec_sem);
  if(!audio_recording) return 1;
  
  sprintf(data, "Avvio registrazione %d", audio_msg);
  support_log(data);
  
  byte = 2;
  write(fd, &byte, 1);
  
  total = 0;
  while(audio_recording) // || (total & 0xff))	// leggo tutto l'ultimo blocco
  {
    len = read(fd, data, 256);
    fwrite(data, len, 1, fp);
    total += len;
    /* Max 40s */
    if(total > 320000) audio_recording = 0;
  }
  support_log("Fine registrazione");
  
  byte = 3;
  write(fd, &byte, 1);
  
  tcdrain(fd);
  ser_setspeed(fd, B19200 | CRTSCTS, 1);
  
  fclose(fp);
  pthread_mutex_unlock(&audio_rec_mutex);
  support_log("Uscita");
  
  return 1;
}

int audio_ready()
{
  if(!audio_msg) return 0;
  return 1;
}

