#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include "contactid.h"
#include "support.h"
#include "serial.h"
#include "audio.h"
#include "master.h"

//#define DEBUG_CID

#ifdef DEBUG_CID
FILE *cidfp;

void contactid_start_rec()
{
  cidfp = fopen("/tmp/saet.cid", "w");
}

void contactid_write_rec(unsigned char *buf)
{
  if(cidfp) fwrite(buf, 1, 128, cidfp);
}

void contactid_stop_rec()
{
  if(cidfp)
  {
    fclose(cidfp);
    rename("/tmp/saet.cid.3", "/tmp/saet.cid.4");
    rename("/tmp/saet.cid.2", "/tmp/saet.cid.3");
    rename("/tmp/saet.cid.1", "/tmp/saet.cid.2");
    rename("/tmp/saet.cid", "/tmp/saet.cid.1");
  }
  cidfp = NULL;
}
#endif

#define CIDDIM 100
static unsigned char CIDev[CIDDIM][16];
static unsigned int CIDnum[CIDDIM];
static pthread_mutex_t CIDmux = PTHREAD_MUTEX_INITIALIZER;
static int CIDin = 0;
static int CIDout = 0;

static pthread_mutex_t CIDerrmux = PTHREAD_MUTEX_INITIALIZER;
static int CID_last_error = 0;

timeout_t *CID_delay = NULL;

/***************************************************/

/*
    Normalmente l'area corrisponde alla zona ed il pointid al sensore.
    ContactID identifica tutto come "zona".
*/
int CID_message(int cond, int num, int evento, int area, int pointid)
{
  unsigned char buf[16];
  char m[64];
  int i;
  
  if(master_behaviour == SLAVE_STANDBY) return 0;
  if(!cond) return 0;
  
  if((evento > 9999)||(area > 99)||(pointid > 999)) return -1;
  if((evento < 0)||(area < 0)||(pointid < 0)) return -1;
  if((num < 1) || (num > PBOOK_DIM)) return -1;
  if(!(config.PhoneBook[num-1].Abil & PB_TX)) return -1;
  
  sprintf(buf, "%04d18%04d%02d%03d", config.DeviceID, evento, area, pointid);
  
  sprintf(m, "ContactID: invio %s ", buf);
  
//  buf[15] = 0;
  for(i=0; i<15; i++)
  {
    buf[i] -= '0';
    if(!buf[i])
      buf[15] += 10;
    else
      buf[15] += buf[i];
  }
  buf[15] %= 15;
  buf[15] = 15-buf[15];
  if(!buf[15]) buf[15] = 0x0F;
  
  if(buf[15] < 10)
    m[32] = buf[15]+'0';
  else if(buf[15] == 10)
    m[32] = '0';
  else
    m[32] = buf[15]+'A'-10;
  support_log(m);
  
  /* Verifico che l'audio sia abilitato al contactid */
  if(!audio_version || ((audio_version&0xff0000)<0x20000))
  {
    support_log("Modulo audio assente o non compatibile");
    CID_set_error(3);
    return -2;
  }
  
  pthread_mutex_lock(&CIDmux);
  if(((CIDin+1)%CIDDIM) != CIDout)
  {
    CIDnum[CIDin] = num;
    memcpy(&(CIDev[CIDin]), buf, 16);
    CIDin++;
    CIDin %= CIDDIM;
  }
  pthread_mutex_unlock(&CIDmux);
  
  return 0;
}

int CID_get_message(unsigned char *ev)
{
  int num;
  
  num = 0;
  pthread_mutex_lock(&CIDmux);
  if(CIDout != CIDin)
  {
    num = CIDnum[CIDout];
    memcpy(ev, CIDev[CIDout], 16);
    CIDout++;
    CIDout %= CIDDIM;
  }
  pthread_mutex_unlock(&CIDmux);
  return num;
}

void CID_set_error(int error)
{
  pthread_mutex_lock(&CIDerrmux);
  CID_last_error = error;
  pthread_mutex_unlock(&CIDerrmux);
  pthread_mutex_lock(&CIDmux);
  CIDout = CIDin;
  pthread_mutex_unlock(&CIDmux);
}

int CID_error()
{
  int v;
  
  pthread_mutex_lock(&CIDerrmux);
  v = CID_last_error;
  CID_last_error = 0;
  pthread_mutex_unlock(&CIDerrmux);
  return v;
}

/***************************************************/

#ifdef __CRIS__
#include "contactid_iside.c"
#elif defined __arm__
#include "contactid_athena.c"
#elif defined __i386__
#include "contactid_pc.c"
#elif defined __x86_64__
#include "contactid_pc.c"
#else
#error
#endif

