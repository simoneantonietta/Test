/*
   23-04-2007 - Aggiunto modem TDK.
*/

/*
hwtestserial -d /dev/ttyS2 -b 115200 -s 2 -tx AT\\B3 -txhex 0d -rx 1
hwtestserial -d /dev/ttyS2 -b 115200 -tx AT\\T9 -txhex 0d -rx 1
hwtestserial -d /dev/ttyS2 -b 19200 -tx ATE1 -txhex 0d -rx 1


hwtestserial -d /dev/ttyS2 -b 19200 -s 2 -tx AT\\B3 -txhex 0d -rx 1
hwtestserial -d /dev/ttyS2 -b 115200 -p odd -tx AT -txhex 0d -rx 1
*/

#include "protocol.h"
#include "support.h"
#include "contactid.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>

//#define DEBUG

#define CIDDIM 100
static unsigned char CIDev[CIDDIM][16];
static unsigned int CIDnum[CIDDIM];
static pthread_mutex_t CIDmux = PTHREAD_MUTEX_INITIALIZER;
static int CIDin = 0;
static int CIDout = 0;

static pthread_mutex_t CIDerrmux = PTHREAD_MUTEX_INITIALIZER;
static int CID_last_error = 0;

int ser_setspeed(int fdser, int baud, int timeout);
char* ser_send_recv(int fdser, char *cmd, char *buf);

static int sqrt_int(int val)
{
  int res, lastres, diff, n;
  
  val *= 4;
  lastres = val/200+2;
  n = 0;
  
  do
  {
    n++;
    res = (val/lastres + lastres) / 2;
    diff = res - lastres;
    lastres = res;
  }
  while((diff > 1)||(diff < -1));
  
  return (int)(res*SQRTMOD);
}

static int FFTint(int dir, int m, int *x, int *y)
{
   int n,i,i1,j,k,i2,l,l1,l2;
   int c1,c2,tx,ty,t1,t2,u1,u2,z;

   /* Calculate the number of points */
   n = 1 << m;

   /* Do the bit reversal */
   i2 = n >> 1;
   j = 0;
   for (i=0;i<n-1;i++) {
      if (i < j) {
         tx = x[i];
         ty = y[i];
         x[i] = x[j];
         y[i] = y[j];
         x[j] = tx;
         y[j] = ty;
      }
      k = i2;
      while (k <= j) {
         j -= k;
         k >>= 1;
      }
      j += k;
   }

   /* Compute the FFT */
   c1 = -LIMIT; 
   c2 = 0;
   l2 = 1;
   for (l=0;l<m;l++) {
      l1 = l2;
      l2 <<= 1;
      u1 = LIMIT; 
      u2 = 0;
      for (j=0;j<l1;j++) {
         for (i=j;i<n;i+=l2) {
            i1 = i + l1;
            t1 = S_MUL(u1, x[i1]) - S_MUL(u2, y[i1]);
            t2 = S_MUL(u1, y[i1]) + S_MUL(u2, x[i1]);
            x[i1] = x[i] - t1; 
            y[i1] = y[i] - t2;
            x[i] += t1;
            y[i] += t2;
         }
         z = S_MUL(u1, c1) - S_MUL(u2, c2);
         u2 = S_MUL(u1, c2) + S_MUL(u2, c1);
         u1 = z;
      }
      c2 = sqrt_int((LIMIT - c1) / 2);
      if (dir == 1) 
         c2 = -c2;
      c1 = sqrt_int((LIMIT + c1) / 2);
   }

   /* Scaling for forward transform */
   if (dir == 1) {
      for (i=0;i<n;i++) {
         x[i] /= n;
         y[i] /= n;
      }
   }
   
   return 1;
}

static char* cid_send_recv(int fd, char *cmd, char *buf)
{
  char tbuf[128];
  int i, n;
  
  if(!buf) buf = tbuf;
  
  write(fd, cmd, strlen(cmd));
  
  i = 0;
  while((n = read(fd, buf+i, 128-i)))
  {
    i += n;
    if(i == 128) return NULL;
  }
  
  buf[i] = 0;
  
  if(buf == tbuf) buf = NULL;
  return buf;
}

static const int cid_freq[] = {11,12,14,15, 19,21,24,26, 22,36,4,5, 6,7,8};
//static const int cid_freq[] = {11,12,14,15, 19,21,24,26, 22,36,4,5, 6,23,37};

static int cid_dtmf_decode(int *x2, int *y2, int nfreq)
{
  int i, j, v, f[sizeof(cid_freq)/sizeof(int)];
  
  j = -1;
  
  for(i=0; i<(sizeof(cid_freq)/sizeof(int)); i++)
  {
    if(x2[cid_freq[i]] < 0) x2[cid_freq[i]] = -x2[cid_freq[i]];
    if(y2[cid_freq[i]] < 0) y2[cid_freq[i]] = -y2[cid_freq[i]];
    f[i] = x2[cid_freq[i]]>y2[cid_freq[i]]?x2[cid_freq[i]]:y2[cid_freq[i]];
//    f[i] = x2[cid_freq[i]]*x2[cid_freq[i]] + y2[cid_freq[i]]*y2[cid_freq[i]];
  }
  
  j = 0;
  
  v = 0;
  for(i=0; i<nfreq; i++) if(f[i] > v) v = f[i];
  v /= 2;
  if(v < 200) v = 200;
//  if(v < 300) v = 300;
  for(i=0; i<(sizeof(cid_freq)/sizeof(int)); i++)
    if(f[i] > v) j |= (1<<i);
  
#ifdef DEBUG
{
static int oldj = 0;
if(!j)
{
  printf(".");
  fflush(stdout);
}
else
{
  if(!oldj) printf("\n");
  printf("%d - %3d %3d %3d %3d %3d %3d %3d %3d   (%3d %3d)   %3d %3d %3d %3d %3d\n", v, f[0], f[1], f[2], f[3], f[4], f[5], f[6], f[7], f[8], f[9], f[10], f[11], f[12], f[13], f[14]);
}
oldj = j;
}
#endif
  if((j & 0x0120) == 0x0120)
  {
    if(f[5] > f[8])
      j &= ~0x0100;
    else
      j &= ~0x0020;
  }
  if((j & 0x0280) == 0x0280)
  {
    if(f[7] > f[9])
      j &= ~0x0200;
    else
      j &= ~0x0080;
  }
  
  return j;
}

/****************************************
    Decodifica IMA ADPCM
****************************************/

static void ima_init(IMA *ima)
{
  ima->predictor = 0;
  ima->step_index = 0;
  ima->step = ima_step_table[0];
}

static int ima_decode(IMA *ima, int code)
{
  int diff;
  
  ima->step_index = ima->step_index + ima_index_table[code];
  if(ima->step_index < 0)
    ima->step_index = 0;
  else if(ima->step_index >= MAX_STEP)
    ima->step_index = MAX_STEP-1;
  
  if(code & 0x08) code = -(code&0x07)-1;
  diff = (ima->step * code / 4) + (ima->step / 8);
  ima->predictor = ima->predictor + diff;
  if(ima->predictor < -(PREDICT_LIMIT+1))
    ima->predictor = -(PREDICT_LIMIT+1);
  else if(ima->predictor > PREDICT_LIMIT)
    ima->predictor = PREDICT_LIMIT;
  
  ima->step = ima_step_table[ima->step_index];
  
  return SHIFT(ima->predictor);
}

/****************************************
    Gestione SiLab
****************************************/

static int cid_SiLab_read_tone(int fd, int mezzisecondi, void *null)
{
  unsigned char buf[128];
  int n, x[128], y[128], i, s, t, v;
  
  i = 0;
  s = 0;
  v = 0;
  t = mezzisecondi * 31;
//#warning
//printf("Timeout %.1f secondi (%d campioni)\n", mezzisecondi/2.0, t);
  
  while((s != 6) && t)
  {
    n = read(fd, buf+i, 128-i);
    
    if(!n) return 0;
    
    i += n;
    if(i == 128)
    {
//#warning
//printf(" %d %d   \r", s, t); fflush(stdout);
      for(i=0; i<128; i++)
      {
        x[i] = ulaw[buf[i]];
        y[i] = 0;
      }
      FFTint(1, 7, x, y);
      n = cid_dtmf_decode(x, y, 10);
//if(s!=1) printf("%d %04x %04x %d\n", s, n, v, t);
      switch(s)
      {
        case 0:
          if(!n) s++;
          break;
        case 1:
          if(n) s++;
          break;
        case 2:
          if(!v)
          {
            if(n && ((!(n & 0x300)) || (n == 0x100) || (n == 0x200)))
              v = n;
            else
              s--;
          }
          if(v && !n) s++;
          // fermo il timeout se sto ricevendo un tono
          t++;
          break;
        case 3:	// il periodo viene chiuso da più di 3 frame a zero
        case 4:
        case 5:
          if(n)
            s = 2;
          else
            s++;
          break;
        default:
          break;
      }
      
      i = 0;
      t--;
    }
  }
//#warning
//printf("\n%04x\n", v);  
  return v;
}

static int cid_SiLab_free_busy(int fd, void *null)
{
  unsigned char buf[128];
  int n, x[128], y[128], i, t;
  
  i = 0;
  t = 62;	// 1s
  
  while(t)
  {
    n = read(fd, buf+i, 128-i);
    i += n;
    if(i == 128)
    {
      for(i=0; i<128; i++)
      {
        x[i] = ulaw[buf[i]];
        y[i] = 0;
      }
      FFTint(1, 7, x, y);
      n = cid_dtmf_decode(x, y, 10);
      
      /* Se ricevo un tono entro 1s è il tono di occupato. */
      if(n)
      {
//printf("CID: linea occupata\n");
        return 1;
      }
      i = 0;
      t--;
    }
  }
  /* E' scaduto il timeout, quindi suona libero. */
//printf("CID: linea libera.\n");
  return 0;
}

static void cid_SiLab_init(int fd)
{
  char buf[64];
  
  /* Configurazione no echo, 115200 8X1 */
  write(fd, "ATX3E0\\B6\\T12\r", 14);
  while(read(fd, buf, 64));
  
  ser_setspeed(fd, B115200|CRTSCTS|CMSPAR|PARENB, 5);
  
  /* connessione host <-> DAA, ulaw, 8000Hz */
  cid_send_recv(fd, "AT:U71,0019\r", NULL);
  write(fd, "ATH\r", 4);
  while(read(fd, buf, 64));
  
  ser_setspeed(fd, B115200|CRTSCTS|CMSPAR|PARENB, 100);
}

static void cid_SiLab_call(int fd, char *number)
{
  char buf[32];
  
  sprintf(buf, "ATDT%s\r", number);
  write(fd, buf, strlen(buf));
}

static void cid_SiLab_send(int fd, unsigned char *code)
{
  int i;
  char buf[128];
  
  for(i=0; i<5; i++)	// 250ms
    write(fd, dtmf_silence, sizeof(dtmf_silence));
  support_log("ContactID: trasmissione messaggio.");
  for(i=0; i<16; i++)
  {
    write(fd, dtmf_tone[code[i]], sizeof(dtmf_silence));	// 50ms
    write(fd, dtmf_silence, sizeof(dtmf_silence));
  }
  for(i=0; i<(sizeof(dtmf_silence)*32);)
  {
    i += read(fd, buf, 128);
  }
  tcdrain(fd);
  tcflush(fd, TCIOFLUSH);
}

static void cid_SiLab_hangup(int fd)
{
  char buf[64];
  
  support_log("ContactID: hangup.");
  buf[0] = 0;
  ser_setspeed(fd, B115200|CRTSCTS|CMSPAR|PARENB|PARODD, 0);
  write(fd, buf, 1);
  ser_setspeed(fd, B115200|CRTSCTS|CMSPAR|PARENB, 5);
  write(fd, "AT\r", 3);
  read(fd, buf, 64);  
  write(fd, "ATH\r", 4);
  while(read(fd, buf, 64));  
  ser_setspeed(fd, B115200|CRTSCTS|CMSPAR|PARENB, 100);
  sleep(1);
}

/****************************************
    Gestione USRobotics
****************************************/

static void cid_USR_init(int fd)
{
  ser_setspeed(fd, B115200|CRTSCTS, 5);
  cid_send_recv(fd, "ATE0M0X3\r", NULL);
}

static void cid_USR_call(int fd, char *number)
{
  int i;
  char buf[32];
  
  ser_setspeed(fd, B115200|CRTSCTS, 5);
  cid_send_recv(fd, "AT#CLS=8#VLS=0#VBT=1#VRN=0#VSM=130,8000\r", NULL);
  ser_setspeed(fd, B115200|CRTSCTS, 600);
  
  sprintf(buf, "ATDT%s\r", number);
  write(fd, buf, strlen(buf));
  
  buf[0] = 0;
  while(read(fd, buf, 1) && ((buf[0]=='\r')||(buf[0]=='\n')));
  
  if(buf[0] && !((buf[0]=='\r')||(buf[0]=='\n')))
  {
    for(i=1; i<31; i++)
    {
      if(!read(fd, buf+i, 1) || (buf[i]=='\n')) break;
    }
    if(strncmp(buf, "VCON", 4)) return;
  }
  
  write(fd, "AT#VRX\r", 7);
  
  buf[0] = 0;
  while(read(fd, buf, 1) && ((buf[0]=='\r')||(buf[0]=='\n')));
  
  if(buf[0] && !((buf[0]=='\r')||(buf[0]=='\n')))
  {
    for(i=1; i<31; i++)
      if(!read(fd, buf+i, 1) || (buf[i]=='\n')) break;
    if(strncmp(buf, "CONNECT", 7)) return;
  }
}

static void cid_USR_hangup(int fd)
{
  char buf[64];
  
#ifdef DEBUG
printf("hangup\n");
#endif
  support_log("ContactID: hangup.");
  buf[0] = 0x10;
  buf[1] = '!';
  ser_setspeed(fd, B115200|CRTSCTS, 5);
  write(fd, buf, 2);
  while(read(fd, buf, 64));  
  write(fd, "AT\r", 3);
  read(fd, buf, 64);  
  write(fd, "ATH\r", 4);
  while(read(fd, buf, 64));  
#ifdef DEBUG
printf("hangup ok\n");
#endif
}

static int cid_USR_read_tone(int fd, int mezzisecondi, void *data)
{
  unsigned char buf[128];
  int n, m, x[128], y[128], i, j, s, t, v, esc;
  
  i = 0;
  j = 0;
  s = 0;
  v = 0;
  t = mezzisecondi * 31;
  esc = 0;
  
  while((s != 6) && t)
  {
    m = read(fd, buf, 128);
    
    if(!m) return 0;
    for(i=0; i<m; i++)
    {
      if(esc || (buf[i] == 0x10))
      {
        if(!esc)
        {
          i++;
          if(i >= m)
          {
            esc = 1;
            break;
          }
        }
        esc = 0;
        if(buf[i] != 0x10) continue;
      }
      
      x[j] = ima_decode((IMA*)data, buf[i] >> 4);
      y[j] = 0;
      j++;
      x[j] = ima_decode((IMA*)data, buf[i] & 0xf);
      y[j] = 0;
      j++;
      
      if(j == 128)
      {
        FFTint(1, 7, x, y);
        n = cid_dtmf_decode(x, y, 15);
        switch(s)
        {
          case 0: if(!n) s++; break;
          case 1: if(n) s++; break;
          case 2:
            if(!v)
            {
              if(n && ((!(n & 0x300)) || (n == 0x100) || (n == 0x200)))
                v = n;
              else
                s--;
            }
            if(v && !n) s++;
            // fermo il timeout se sto ricevendo un tono
            t++;
            break;
          case 3: case 4: case 5:
            if(n) s = 2; else s++; break;
          default: break;
        }
        
        j = 0;
        t--;
      }
    }
  }
  
  return v;
}

static int cid_USR_free_busy(int fd, void *data)
{
  unsigned char buf[128];
  int n, x[128], y[128], i, j, t, esc;
  
  i = 0;
  j = 0;
  t = 62;
  esc = 0;
  
  while(t)
  {
    n = read(fd, buf, 128);
    
    if(!n) return 0;
    for(i=0; i<n; i++)
    {
      if(esc || (buf[i] == 0x10))
      {
        if(!esc)
        {
          i++;
          if(i >= n)
          {
            esc = 1;
            break;
          }
        }
        esc = 0;
        if(buf[i] != 0x10) continue;
      }
      
      x[j] = ima_decode((IMA*)data, buf[i] >> 4);
      y[j] = 0;
      j++;
      x[j] = ima_decode((IMA*)data, buf[i] & 0xf);
      y[j] = 0;
      j++;
      
      if(j == 128)
      {
        FFTint(1, 7, x, y);
        n = cid_dtmf_decode(x, y, 15);
        
        /* Se ricevo un tono entro 1s è il tono di occupato. */
        if(n)
        {
          support_log("CID: linea occupata.");
          return 1;
        }
        
        j = 0;
        t--;
      }
    }
  }
  
  /* E' scaduto il timeout, quindi suona libero. */
  support_log("CID: linea libera.");
  return 0;
}

static const char cid_idx_to_tone[16] = "01234567890*#ABC";

static void cid_USR_send(int fd, unsigned char *code)
{
/*
at#vbt=1 (toni da 1/10s)
at#vts=1,2,3,4,5,6... (sequenza di cifre)
*/
  char buf[64];
  int i;
  
  buf[0] = 0x10;
  buf[1] = '!';
  write(fd, buf, 2);
  ser_setspeed(fd, B115200|CRTSCTS, 5);
  while(read(fd, buf, 64));
  
  sprintf(buf, "at#vts=");
  for(i=0; i<16; i++)
  {
    buf[i*2+7] = cid_idx_to_tone[code[i]];
    buf[i*2+8] = ',';
  }
  buf[38] = '\r';
  write(fd, buf, 39);
  ser_setspeed(fd, B115200|CRTSCTS, 50);
  while(read(fd, buf, 64));
  
  write(fd, "AT#VRX\r", 7);
  
  buf[0] = 0;
  while(read(fd, buf, 1) && ((buf[0]=='\r')||(buf[0]=='\n')));
  
  if(buf[0] && !((buf[0]=='\r')||(buf[0]=='\n')))
  {
    for(i=1; i<31; i++)
      if(!read(fd, buf+i, 1) || (buf[i]=='\n')) break;
    if(strncmp(buf, "CONNECT", 7)) return;
  }
}

/****************************************
    Gestione TDK
****************************************/

static int cid_TDK_send_recv(int fd, char *cmd, int len, char *buf)
{
  int i, n;
  
  write(fd, cmd, len);
  i = 0;
  do
  {
    n = read(fd, buf+i, 128-i);
    if(!n) break;
    i += n;
  }
  while((i < 6) || strncmp(buf+i-6, "\r\nOK\r\n", 6));
  
  if(!n) return 0;
  return i;
}

static int cid_TDK_read_tone(int fd, int mezzisecondi, void *null)
{
  unsigned char buf[128];
  int n, s, v, val;
  
  mezzisecondi *= 15;
  
  s = v = val = 0;
  
  for(; mezzisecondi; mezzisecondi--)
  {
    n = cid_TDK_send_recv(fd, "A/", 2, buf);
    if(n)
    {
      sscanf(buf+4, "%d", &v);
//      printf("%d %02x\n", mezzisecondi, v);
      
      if(!v && s)
        break;
      else if(v && !s)
      {
        if((v == 0x02) || (v == 0x08) || (v & 0x80))
        {
          s++;
          val = v;
        }
      }
    }
  }
  
  if(val & 0x80)
    v = 0x400;
  else if(val & 0x08)
    v = 0x200;
  else if(val & 0x02)
    v = 0x100;
  else
    v = 0;
  
//#warning
//printf("\n%04x\n", v);  
  return v;
}

static int cid_TDK_free_busy(int fd, void *null)
{
  int n;
  
  n = cid_TDK_read_tone(fd, 2, NULL);
  if(n)
  {
    support_log("CID: linea occupata.");
    return 1;
  }
  
  support_log("CID: linea libera.");
  return 0;
}

static void cid_TDK_init(int fd)
{
  char buf[64];
  
  cid_TDK_send_recv(fd, "ATS99=39S20=34S73=0X3\r", 22, buf);
  usleep(200000);
  cid_TDK_send_recv(fd, "ATS6=2\r", 7, buf);
}

static void cid_TDK_call(int fd, char *number)
{
  char buf[64];
  
  ser_setspeed(fd, B9600, 80);
  
  sprintf(buf, "ATDT%s;\r", number);
  cid_TDK_send_recv(fd, buf, strlen(buf), buf);
  
  ser_setspeed(fd, B9600, 5);
  
  /* Configurazione tono 1400Hz (freq 2) */
  usleep(50000);
  cid_TDK_send_recv(fd, "AT@A2.$EA=$2B.$C7\r", 18, buf);	// 1400
//  cid_TDK_send_recv(fd, "AT@A2.$EA=$3F.$20\r", 18, buf);	// 1209
//  cid_TDK_send_recv(fd, "AT@A2.$EA=$69.$0B\r", 18, buf);	// 697
//  cid_TDK_send_recv(fd, "AT@A2.$EA=$74.$EF\r", 18, buf);
  
  /* Configurazione tono 2300Hz (freq 4) */
  usleep(50000);
  cid_TDK_send_recv(fd, "AT@A2.$F2=$C8.$E8\r", 18, buf);	// 2300
//  cid_TDK_send_recv(fd, "AT@A2.$F2=$32.$6C\r", 18, buf);	// 1336
  
  /* Impostando a 1 il valore di attesa per il blind dial,
     a connessione avvenuta il comando di dial viene
     eseguito immediatamente. */
  cid_TDK_send_recv(fd, "ATS6=1\r", 7, buf);
  cid_TDK_send_recv(fd, "ATS63?\r", 7, buf);
}

static const char cid_to_key[] = {'0','1','2','3','4','5','6','7','8','9','0','*','#','A','B','C'};

static void cid_TDK_send(int fd, unsigned char *code)
{
  int i;
  char buf[128];
  
  support_log("ContactID: trasmissione messaggio.");
  sprintf(buf, "ATDT0000000000000000;\r");
  for(i=0; i<16; i++) buf[4+i] = cid_to_key[code[i]];
  
  ser_setspeed(fd, B9600, 50);
  cid_TDK_send_recv(fd, buf, 22, buf);
  ser_setspeed(fd, B9600, 5);
  
  /* Configurazione tono 1400Hz (freq 2) */
  usleep(50000);
  cid_TDK_send_recv(fd, "AT@A2.$EA=$2B.$C7\r", 18, buf);	// 1400
//  cid_TDK_send_recv(fd, "AT@A2.$EA=$3F.$20\r", 18, buf);	// 1209
  
  cid_TDK_send_recv(fd, "ATS63?\r", 7, buf);
}

static void cid_TDK_hangup(int fd)
{
  char buf[64];
  
  support_log("ContactID: hangup.");
  cid_TDK_send_recv(fd, "ATH0\r", 5, buf);
  usleep(200000);
  cid_TDK_send_recv(fd, "ATS6=2\r", 7, buf);
  sleep(1);
}

/****************************************
    Verifica modem
****************************************/

static const Modem Modem_SiLab = {
	.init = cid_SiLab_init,
	.call = cid_SiLab_call,
	.hangup = cid_SiLab_hangup,
	.read_tone = cid_SiLab_read_tone,
	.free_busy = cid_SiLab_free_busy,
	.send = cid_SiLab_send,
};

static const Modem Modem_USRobotics = {
	.init = cid_USR_init,
	.call = cid_USR_call,
	.hangup = cid_USR_hangup,
	.read_tone = cid_USR_read_tone,
	.free_busy = cid_USR_free_busy,
	.send = cid_USR_send,
};

static const Modem Modem_TDK = {
	.init = cid_TDK_init,
	.call = cid_TDK_call,
	.hangup = cid_TDK_hangup,
	.read_tone = cid_TDK_read_tone,
	.free_busy = cid_TDK_free_busy,
	.send = cid_TDK_send,
};

static const Modem* cid_probe_modem(int fd)
{
  char buf[128], *p;
  
  p = cid_send_recv(fd, "ATI\r", buf);
  if(!p[0])
  {
    ser_setspeed(fd, B115200|CRTSCTS|PARENB|PARODD, 5);
    write(fd, "\r", 1);
    ser_setspeed(fd, B115200|CRTSCTS, 5);
    p = cid_send_recv(fd, "ATI\r", buf);
  }
  
  /* Verifica modem US Robotics, voice modem */
  if(p[0] && strstr(p, "5601"))
  {
    p = cid_send_recv(fd, "AT+FCLASS=?\r", buf);
    if(p[0] && strstr(p, "8"))
    {
      support_log("ContactID: modem USRobotics.");
      return &Modem_USRobotics;
    }
    else
      return NULL;
  }
  
  if(!p[0])
  {
    ser_setspeed(fd, B115200|CRTSCTS|PARENB|PARODD, 5);
    write(fd, "\r", 1);
    p = cid_send_recv(fd, "ATI\r", buf);
  }
  
  /* Verifica modem SiLab, revision diversa da H */
  if(p[0] && !strstr(p, "H"))
  {
    p = cid_send_recv(fd, "ATI6\r", buf);
    if(p[0] && (strstr(p, "2456") || strstr(p, "2433") || strstr(p, "2414")))
    {
      support_log("ContactID: modem SiLab.");
      return &Modem_SiLab;
    }
  }
  
  ser_setspeed(fd, B9600, 5);
  cid_send_recv(fd, "AT\r", buf);
  p = cid_send_recv(fd, "ATI4\r", buf);
  if(p[0] && strstr(p, "TDK 73M2901"))
  {
    support_log("ContactID: modem TDK.");
    return &Modem_TDK;
  }
  
  return NULL;
}

/****************************************
    Main
****************************************/

static void cid_loop(ProtDevice *dev)
{
  unsigned char buf[128];
  struct termios tio;
  int tono, seqnum, stato, tentativo, ciclo, squilli;
  const Modem *modem;
  IMA ima;
  void *data;
  
  tcgetattr(dev->fd, &tio);
  tio.c_cc[VTIME] = 5;
  tio.c_cc[VMIN] = 0;
  tcsetattr(dev->fd, TCSANOW, &tio);
  
  cid_send_recv(dev->fd, "AT\r", buf);
  
  modem = cid_probe_modem(dev->fd);
  if(!modem)
  {
    support_log("ContactID: modem non valido.");
    CID_last_error = 3;
    return;
  }
  
  modem->init(dev->fd);
  /* si può migliorare... */
  data = &ima;
  
  support_log("ContactID: avvio.");
  
  stato = 0;
  tentativo = 0;
  seqnum = 0;
  ciclo = 0;
  squilli = 0;
  
  while(1)
  {
#ifdef DEBUG
if(stato) printf("stato %d\n", stato);
#endif
    switch(stato)
    {
      case 0:	// riposo
        pthread_mutex_lock(&CIDmux);
        if(CIDout != CIDin)
        {
          pthread_mutex_unlock(&CIDmux);
          tentativo = 1;
          ciclo = 1;
          stato = 1;
          seqnum = 0;
        }
        else
        {
          pthread_mutex_unlock(&CIDmux);
          usleep(100000);
        }
        break;
      case 1:	// composizione
#ifdef DEBUG
printf("CID: stato 1\n");
#endif
        if(!config.PhoneBook[CIDnum[CIDout]-1+seqnum].Phone[0])
        {
          switch(seqnum)
          {
            case 0:
              /* Questo deve essere sempre definito. */
              pthread_mutex_lock(&CIDerrmux);
              CID_last_error = 1;
              pthread_mutex_unlock(&CIDerrmux);
              pthread_mutex_lock(&CIDmux);
              CIDout = CIDin;
              pthread_mutex_unlock(&CIDmux);
              stato = 0;
              continue;
              break;
            case 1:
              /* E' definito solo il numero primario.
                 Continuo ad usare quello. */
              seqnum = 0;
              break;
            case 2:
              /* Questo deve essere sempre definito se si vuole
                 utilizzare un secondo centro. */
              seqnum = 0;
              break;
            case 3:
              /* E' definito solo il numero primario del secondo centro.
                 Continuo ad usare quello. */
              seqnum = 2;
              break;
          }
        }
        sprintf(buf, "ContactID: chiamata al numero %d", CIDnum[CIDout]+seqnum);
        support_log(buf);
        modem->call(dev->fd, config.PhoneBook[CIDnum[CIDout]-1+seqnum].Phone);
        
        /* l'inizializzazione IMA serve solo per l'USR,
           ma non è un problema farla sempre. */
        ima_init(&ima);
        
        squilli = 0;
        stato = 2;
        break;
      case 2:	// sincronismo 1400Hz
#if 0
tono = cid_read_tone(dev->fd, 30);
printf("tono = %04x\n", tono);
tentativo++;
if(tentativo > 7)
{
          tentativo = 1;
          pthread_mutex_lock(&CIDmux);
          CIDout = CIDin;
          pthread_mutex_unlock(&CIDmux);
          
            cid_hangup(dev->fd);
            stato = 0;
}
break;
#endif

#ifdef DEBUG
printf("CID: stato 2\n");
#endif
        if((tono = modem->read_tone(dev->fd, 30, data)) == 0x100)
          stato = 3;
        else
        {
#ifdef DEBUG
printf("tono = %04x\n", tono);
#endif
          if(tono == 0x200)
          {
            /* Ho ricevuto prima il tono a 2300HZ, quindi riprovo ad aspettare il 1400Hz */
            tentativo++;
            if(tentativo < 3) continue;
            /* Non è arrivato il tono a 1400Hz, continuo con il numero successivo */
            seqnum ^= 0x01;
            tentativo = 1;
            stato = 1;
            ciclo++;
          }
          else if(tono >= 0x400)
          {
            /* toni libero/occupato */
            tentativo = 1;
            squilli++;
            if(squilli < 4) continue;
            
            /* Ho ricevuto libero/occupato */
            switch(modem->free_busy(dev->fd, data))
            {
              case 0:	// libero
                support_log("ContactID: linea libera.");
                seqnum ^= 0x02;
                seqnum &= 0x02;
                tentativo = 1;
                stato = 1;
                ciclo++;
                break;
              case 1:	// occupato
                support_log("ContactID: linea occupata.");
                seqnum ^= 0x01;
                tentativo = 1;
                stato = 1;
                ciclo++;
                break;
            }
          }
          else if(tono)
          {
            /* Toni diversi */
            tentativo++;
            if(tentativo < 3) continue;
            stato = 1;
            ciclo++;
          }
          else
          {
            /* Timeout */
            stato = 1;
            ciclo++;
          }
          
          modem->hangup(dev->fd);
          if(ciclo > 8)
          {
            pthread_mutex_lock(&CIDerrmux);
            CID_last_error = 2;
            pthread_mutex_unlock(&CIDerrmux);
            pthread_mutex_lock(&CIDmux);
            CIDout = CIDin;
            pthread_mutex_unlock(&CIDmux);
            stato = 0;
          }
        }
        break;
      case 3:	// sincronismo 2300Hz
#ifdef DEBUG
printf("CID: stato 3\n");
#endif
        if((tono = modem->read_tone(dev->fd, 1, data)) == 0x200)
        {
          tentativo = 1;
          stato = 4;
        }
        else
        {
#ifdef DEBUG
printf("tono = %04x\n", tono);
#endif
          modem->hangup(dev->fd);
          stato = 0;
        }
        break;
      case 4:	// invio segnalazione
#ifdef DEBUG
printf("CID: stato 4\n");
#endif
        modem->send(dev->fd, CIDev[CIDout]);
        ima_init(&ima);
        stato = 5;
        break;
      case 5:	// ricezione ok (1400Hz)
#ifdef DEBUG
printf("CID: stato 5\n");
#endif
        if((tono = modem->read_tone(dev->fd, 6, data)) == 0x100)
        {
#ifdef DEBUG
printf("CID: invio OK\n");
#endif
          support_log("ContactID: invio messaggio OK.");
          tentativo = 1;
          pthread_mutex_lock(&CIDmux);
          CIDout++;
          CIDout %= CIDDIM;
          if(CIDout != CIDin) stato = 4;
          pthread_mutex_unlock(&CIDmux);
          
          if(stato != 4)
          {
            modem->hangup(dev->fd);
            stato = 0;
          }
        }
        else
        {
#ifdef DEBUG
printf("CID: invio ERR (tono = %04x)\n", tono);
#endif
          support_log("ContactID: errore trasmissione.");
          tentativo++;
          if(tentativo > 3)
          {
            modem->hangup(dev->fd);
            stato = 0;
          }
          else
            stato = 4;
        }
        break;
    }
  }
}

/*
    Normalmente l'area corrisponde alla zona ed il pointid al sensore.
    ContactID identifica tutto come "zona".
*/
int CID_message(int cond, int num, int evento, int area, int pointid)
{
  unsigned char buf[16];
  char m[64];
  int i;
  
  if(!cond) return 0;
  
  if((evento > 9999)||(area > 99)||(pointid > 999)) return -1;
  if((evento < 0)||(area < 0)||(pointid < 0)) return -1;
  if((num < 1) || (num > PBOOK_DIM)) return -1;
  
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

int CID_error()
{
  int v;
  
  pthread_mutex_lock(&CIDerrmux);
  v = CID_last_error;
  CID_last_error = 0;
  pthread_mutex_unlock(&CIDerrmux);
  return v;
}

void _init()
{
  printf("ContactID (plugin): " __DATE__ " " __TIME__ "\n");
  prot_plugin_register("CID", 0, NULL, NULL, (PthreadFunction)cid_loop);
}

