#include "finger.h"
#include "lara.h"
#include "tebe.h"
#include "delphi.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

static const char FingerFileName[] = "/saet/data/finger.db";
static const char FingerFileNameTmp[] = "/saet/data/finger.db.tmp";

tebe_finger **finger = NULL;

pthread_mutex_t finger_mutex = PTHREAD_MUTEX_INITIALIZER;
int finger_save_pending = 0;

void finger_init()
{
  FILE *fp;
  unsigned short id, len;
  
  if(finger) return;
  
//printf("Finger: Open DB\n");
  fp = fopen(ADDROOTDIR(FingerFileName), "r");
  if(!fp) return;
  
//printf("Finger: Init (%d)\n", BADGE_NUM);
  finger = calloc(BADGE_NUM, sizeof(tebe_finger));
  if(!finger) return;
  
  while(fread(&id, 1, 2, fp))
  {
    fread(&len, 1, 2, fp);
//printf("Finger: ID %d (len %d)\n", (int)id, (int)len);
    if(id < BADGE_NUM)
    {
      finger[id] = malloc(len+FINGER_BLK_DIM);
      fread(finger[id], 1, len, fp);
    }
    else
    {
      fclose(fp);
      return;
    }
  }
  
  fclose(fp);
}

void* finger_save_th(void *null)
{
  FILE *fp;
  int i, j, len;
  
  if(!finger)
  {
    pthread_mutex_lock(&finger_mutex);
    finger_save_pending = 0;
    pthread_mutex_unlock(&finger_mutex);
    return NULL;
  }
  
  while(1)
  {
    pthread_mutex_lock(&finger_mutex);
    finger_save_pending++;
    pthread_mutex_unlock(&finger_mutex);
    
    fp = fopen(ADDROOTDIR(FingerFileNameTmp), "w");
    if(!fp)
    {
      pthread_mutex_lock(&finger_mutex);
      finger_save_pending = 0;
      pthread_mutex_unlock(&finger_mutex);
      return NULL;
    }
    
    for(i=0; i<BADGE_NUM; i++)
    {
      if(finger[i])
      {
        fwrite(&i, 1, 2, fp);
        len = 10;
        for(j=0; j<10; j++)
          if(finger[i]->finger[j]) len += finger[i]->finger[j]*32;
        len += 10;
        fwrite(&len, 1, 2, fp);
        fwrite(finger[i], 1, len, fp);
      }
    }
    
    fclose(fp);
    
    rename(FingerFileNameTmp, FingerFileName);
    sync();
    
    pthread_mutex_lock(&finger_mutex);
    if(finger_save_pending > 1)
    {
      finger_save_pending = 0;
      pthread_mutex_unlock(&finger_mutex);
      return NULL;
    }
    pthread_mutex_unlock(&finger_mutex);
  }
}

void finger_save()
{
  pthread_t pth;
  
  pthread_mutex_lock(&finger_mutex);
  if(finger_save_pending > 0)
    finger_save_pending = 1;
  else
  {
    finger_save_pending = 1;
    pthread_create(&pth, NULL, finger_save_th, NULL);
    pthread_detach(pth);
  }
  pthread_mutex_unlock(&finger_mutex);
}

void finger_send(int mm, int id)
{
  unsigned char buf[40];
  int i, n;
  
  if(!finger) return;
  
  if(id < 0) return;
// Se id=0 invia il primo ID valido. L'ID 0 non può mai avere impronta,
// non può mai nemmeno timbrare in condizioni normali.
  if(!id) for(; (id<BADGE_NUM)&&(!finger[id]); id++);
  if(id >= BADGE_NUM) return;
  
//printf("Finger: Send ID %d (mm %d)\n", id, mm);
  if(!finger[id])
  {
    /* Cancellazione */
    buf[0] = 5;
    buf[1] = CODE_FINGER;
    buf[2] = id;
    buf[3] = id >> 8;
    buf[4] = 0;
    buf[5] = 0;
    tebe_send_mm(mm, buf);
    return;
  }
  
  n = 0;
  for(i=0; i<10; i++) n += finger[id]->finger[i] * 32;
  n += 10+FINGER_BLK_DIM-1;
  n /= FINGER_BLK_DIM;
  
  for(i=0; i<n; i++)
  {
    buf[0] = 5 + FINGER_BLK_DIM;
    buf[1] = CODE_FINGER;
    buf[2] = id;
    buf[3] = id >> 8;
    buf[4] = n;
    buf[5] = i;
    memcpy(buf+6, ((unsigned char*)(finger[id]))+(i*FINGER_BLK_DIM), FINGER_BLK_DIM);
    tebe_send_mm(mm, buf);
  }
}

int finger_next_valid(int id)
{
  if(finger)
    for(id++; id<BADGE_NUM; id++)
      if(finger[id]) return id;
  return -1;
}

int finger_store(unsigned char *pkt)
{
  static unsigned char *f = NULL;
  static unsigned short id = 0xffff;
  int i;
  
  if(!pkt[3])
  {
    /* inizio di una nuova impronta */
    free(f);
    f = NULL;
//    id = *(unsigned short*)(pkt);
    id = uchartoshort(pkt);
    
    if(!pkt[2])
    {
      /* in realtà si tratta di una cancellazione */
      if(!finger || (id >= BADGE_NUM) || !finger[id])
        return 0;  // non esistono impronte, non c'è nulla da cancellare
      
//printf("Cancella impronta id %d\n", id);
      free(finger[id]);
      finger[id] = NULL;
      
      for(i=1; (i<BADGE_NUM)&&!finger[i]; i++);
      if(i == BADGE_NUM)
      {
        /* L'anagrafica impronte è completamente vuota, creo l'impronta dummy. */
        finger[1] = calloc(1, 10+32+FINGER_BLK_DIM);
        finger[1]->finger[0] = 1;
      }
      finger_save();
      return 1; // invia la cancellazione a tutti i terminali
    }
    else
      f = malloc(pkt[2] * FINGER_BLK_DIM);
  }
  
//  if(!f || (*(unsigned short*)(pkt) >= BADGE_NUM) || (*(unsigned short*)(pkt) != id)) return 0;
  if(!f || (uchartoshort(pkt) >= BADGE_NUM) || (uchartoshort(pkt) != id)) return 0;
  
  memcpy(f+(pkt[3]*FINGER_BLK_DIM), pkt+4, FINGER_BLK_DIM);
  
  if(pkt[3] == (pkt[2] - 1))
  {
    if(!finger)
    {
      finger = calloc(BADGE_NUM, sizeof(tebe_finger));
      if(!finger) return 0;
    }
    
    free(finger[id]);
    finger[id] = (tebe_finger*)f;
    f = NULL;
    id = 0xffff;
    finger_save();
    return 1;
  }
  
  return 0;
}

void finger_delete(int id)
{
  unsigned char f[4];
  
//  *(unsigned short*)f = id;
  inttouchar(id, f, 2);
  f[2] = 0;
  f[3] = 0;
  finger_store(f);
  
  tebe_send_finger(id);
}
