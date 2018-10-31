#define LIMIT 8192

#define smul(a,b) ( (a)*(b) )
#define sround( x )  ( ( (x) + (LIMIT/2) ) / LIMIT )

#define S_MUL(a,b) sround( smul(a,b) )

short CID_ulaw[256] = {
0,-7903,-7647,-7391,-7135,-6879,-6623,-6367,-6111,-5855,
-5599,-5343,-5087,-4831,-4575,-4319,-4063,-3935,-3807,-3679,
-3551,-3423,-3295,-3167,-3039,-2911,-2783,-2655,-2527,-2399,
-2271,-2143,-2015,-1951,-1887,-1823,-1759,-1695,-1631,-1567,
-1503,-1439,-1375,-1311,-1247,-1183,-1119,-1055,-991,-959,
-927,-895,-863,-831,-799,-767,-735,-703,-671,-639,
-607,-575,-543,-511,-479,-463,-447,-431,-415,-399,
-383,-367,-351,-335,-319,-303,-287,-271,-255,-239,
-223,-215,-207,-199,-191,-183,-175,-167,-159,-151,
-143,-135,-127,-119,-111,-103,-95,-91,-87,-83,
-79,-75,-71,-67,-63,-59,-55,-51,-47,-43,
-39,-35,-31,-29,-27,-25,-23,-21,-19,-17,
-15,-13,-11,-9,-7,-5,-3,-1,8159,7903,
7647,7391,7135,6879,6623,6367,6111,5855,5599,5343,
5087,4831,4575,4319,4063,3935,3807,3679,3551,3423,
3295,3167,3039,2911,2783,2655,2527,2399,2271,2143,
2015,1951,1887,1823,1759,1695,1631,1567,1503,1439,
1375,1311,1247,1183,1119,1055,991,959,927,895,
863,831,799,767,735,703,671,639,607,575,
543,511,479,463,447,431,415,399,383,367,
351,335,319,303,287,271,255,239,223,215,
207,199,191,183,175,167,159,151,143,135,
127,119,111,103,95,91,87,83,79,75,
71,67,63,59,55,51,47,43,39,35,
31,29,27,25,23,21,19,17,15,13,
11,9,7,5,3,1,
};

// (sqrt(LIMIT)/2)
#define SQRTMOD 45

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

//static const int cid_freq[] = {11,12,14,15, 19,21,24,26, 22,36,4,5, 6,7,8};
//static const int cid_freq[] = {11,12,14,15, 19,21,24,26, 22,36,4,5, 6,7,37};
//static const int cid_freq[] = {11,12,14,15, 19,21,24,26, 22,36,4,5, 6,35,37};
//static const int cid_freq[] = {11,12,14,15, 19,21,24,26, 22,36,4,5, 6,23,37};
static const int cid_freq[] = {20,21,23,24, 34,35,37,38, 22,36};
static int __cid_seq = 0;

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
  if(v < 100) v = 100;
//  if(v < 200) v = 200;
//  if(v < 300) v = 300;
  for(i=0; i<(sizeof(cid_freq)/sizeof(int)); i++)
    if(f[i] > v) j |= (1<<i);
  
#if 0
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
#endif

#ifdef DEBUG_CID
{
  char log[128];
  for(i=0; (i<10)&&(f[i]<10); i++);
  if(i<10)
  {
    sprintf(log, "** %04d %d - %3d %3d %3d %3d %3d %3d %3d %3d   (%3d %3d)", __cid_seq, v, f[0], f[1], f[2], f[3], f[4], f[5], f[6], f[7], f[8], f[9]);
    support_log(log);
  }
}
#endif

#if 0
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
#endif
  
  return j;
}

int CID_decode_tone(int *x, int nfreq)
{
  int y[128];
  
  memset(y, 0, sizeof(y));
  FFTint(1, 7, x, y);
  return cid_dtmf_decode(x, y, nfreq);
}

int CID_decode_tone_ulaw(unsigned char *data, int nfreq)
{
  int x[128], i;
  
  for(i=0; i<128; i++)
    x[i] = CID_ulaw[data[i]];
  return CID_decode_tone(x, nfreq);
}

static int CID_calibr_modifica = 0;
static int CID_calibr_done = 0;

void CID_init()
{
  if(!CID_delay) CID_delay = timeout_init();
}

void CID_calibra(int fd, int offs)
{
  unsigned char buf[128];
  int ret, pre, post;
  
  pre = 0;
  
  /* Ferma la registrazione */
  buf[0] = 3;
  write(fd, buf, 1);
  /* Svuota la ricezione */
  while(read(fd, buf, 128));
  
  /* Richiede la calibrazione corrente */
  buf[0] = 0xAA;
  buf[1] = 0x00;
  write(fd, buf, 2);
  /* La calibrazione è 1 byte */
  ret = read(fd, &pre, 128);
  if(ret == 1)
  {
    post = pre+offs;
    
    if(offs)
    {
      buf[1] = CID_calibr_modifica = post;
      buf[0] = 0xAA;
      write(fd, buf, 2);
      read(fd, buf, 128);
    }
#ifdef DEBUG_CID
sprintf(buf, "**** Calibrazione %02x -> %02x", pre, post);
support_log(buf);
#endif
  }
  
  /* Riattiva la cattura audio */
  buf[0] = 2;
  write(fd, buf, 1);
}

int CID_read_tone(int fd, int timeout, int *durata)
{
  unsigned char buf[128];
  int n, ret, tone, count, d;
  
__cid_seq=0;
  
  n = 0;
  tone = 0;
  count = 0;
  d = 0;
  while(timeout && (count < 3))
  {
    ret = read(fd, buf+n, 128-n);
    if(!ret)
    {
      /* Non arrivano dati, fermo tutto */
#ifdef DEBUG_CID
support_log("**** No dati");
#endif
      timeout = 0;
      
      /* Probabilmente la calibrazione è andata, elimino il file
         almeno da poter recuperare la centrale con un reset. */
      unlink("/saet/audiocal.bin");
    }
    n += ret;
    if(n == 128)
    {
__cid_seq++;
#ifdef DEBUG_CID
contactid_write_rec(buf);
#endif
      n = 0;
      ret = CID_decode_tone_ulaw(buf, 10);
//printf("**** %04x (%d %d)\n", ret, count, timeout);
      if(ret)
      {
        if(ret != tone)
        {
          if(d < 2)
          {
            tone = ret;
            d = 1;
          }
          else
          {
            count++;
          }
        }
        else
          d++;
        count = 0;
      }
      else if(tone)
        count++;
      timeout--;
    }
  }
  
  if(d < 2) tone = 0;
  
#ifdef DEBUG_CID
sprintf(buf, "** Decodifica: %04x %d", tone, d);
support_log(buf);
#endif
  
#if 1
  if(!CID_calibr_done)
  {
  if(tone & 0x33)
  {
    if(tone & 0x100)
      CID_calibra(fd, -2);
    else
      CID_calibra(fd, -4);
  }
  else if(tone & 0xcc)
  {
    if(tone & 0x200)
      CID_calibra(fd, 2);
    else
      CID_calibra(fd, 4);
  }
  }
#endif
    
  if(tone & 0x0f)
    tone = 0x100;
  else if(tone & 0xf0)
    tone = 0x200;
  
  *durata = d;
  return tone;
}

int CID_read_tone_ACK(int fd, int timeout)
{
  unsigned char buf[128];
  int n, ret, tone, count, ack;
  
#ifdef DEBUG_CID
support_log("** Attesa ACK");
#endif
  
__cid_seq=0;

  n = 0;
  ack = 0;
  tone = 0;
  count = 0;
  while(timeout && (ack < 64))
  {
    ret = read(fd, buf+n, 128-n);
    if(!ret)
    {
      /* Non arrivano dati, fermo tutto */
#ifdef DEBUG_CID
printf("**** No dati\n");
#endif
      timeout = 0;
    }
    n += ret;
    if(n == 128)
    {
__cid_seq++;
#ifdef DEBUG_CID
contactid_write_rec(buf);
#endif
      n = 0;
      
#ifdef DEBUG_CID
    /* Nella versione definitiva la decodifica può essere
       fatta anche solo parziale, mentre si attende la
       partenza del contatore ack. */
      ret = CID_decode_tone_ulaw(buf, 10);
#endif
      if(!ack)
      {
#ifndef DEBUG_CID
        ret = CID_decode_tone_ulaw(buf, 10);
#endif
        if(ret == 0x100)
        {
          /* Mi aspetto almeno tre campioni consecutivi. */
          if(count < 3)
            count++;
          else
          {
            ack = 1;
            tone = 0x100;
          }
        }
        else
          count = 0;
      }
      else
        ack++;
      
      timeout--;
    }
  }
  
  return tone;
}

void CID_read_silence(int fd, int dur)
{
  unsigned char buf[128];
  int n, ret, tone, count, timeout;
  
#ifdef DEBUG_CID
support_log("** Pausa");
#endif
  
__cid_seq=0;

  n = 0;
  tone = 0;
  count = 0;
  timeout = 128;
  while(timeout && (count < dur))
  {
    ret = read(fd, buf+n, 128-n);
    if(!ret)
    {
      /* Non arrivano dati, fermo tutto */
#ifdef DEBUG_CID
support_log("**** No dati");
#endif
      timeout = 0;
    }
    n += ret;
    if(n == 128)
    {
__cid_seq++;
#ifdef DEBUG_CID
contactid_write_rec(buf);
#endif
      n = 0;
      ret = CID_decode_tone_ulaw(buf, 10);
      if(ret)
        count = 0;
      else
        count++;
      timeout--;
    }
  }
}

#if 0
void CID_delay(int fd, int dur)
{
  unsigned char buf[128];
  int n, ret, tone, count, timeout;
  
#ifdef DEBUG_CID
srintf(buf, "** Ritardo (%d)", dur);
support_log(buf);
#endif
  
__cid_seq=0;

  n = 0;
  tone = 0;
  count = 0;
  timeout = 128;
  while(timeout && (count < dur))
  {
    ret = read(fd, buf+n, 128-n);
    if(!ret)
    {
      /* Non arrivano dati, fermo tutto */
#ifdef DEBUG_CID
support_log("**** No dati");
#endif
      timeout = 0;
    }
    n += ret;
    if(n == 128)
    {
__cid_seq++;
#ifdef DEBUG_CID
contactid_write_rec(buf);
#endif
      n = 0;
      if(count < (dur-3))
        count++;
      else
      {
        ret = CID_decode_tone_ulaw(buf, 10);
        if(!ret) count++;
      }
      timeout--;
    }
  }
}
#endif

static int CID_tentativi = 0;

void CID_transmit(int fd, int *msg, unsigned char *ev)
{
  unsigned char buf[128];//, *abuf;
  int /*n, ret,*/ stato, tone, tentativi, durata;
  FILE *fp;
  
  /* Verifico che l'audio sia abilitato al contactid */
  if(!audio_version || ((audio_version&0xff0000)<0x20000))
  {
    *msg = 0;
    CID_set_error(3);
    return;
  }
  
  /* Attiva la cattura audio */
  //ser_setspeed(fd, B115200 | CRTSCTS, 5);
  ser_setspeed(fd, B115200 | CRTSCTS, 1);
  buf[0] = 2;
  write(fd, buf, 1);
  
#ifdef DEBUG_CID
  contactid_start_rec();
support_log("** Identificazione");
#endif
  /* Attendo l'identificazione ContactID */
  tentativi = 0;
  stato = tone = 0;
  do
  {
    switch(stato)
    {
      case 0:
        tone = CID_read_tone(fd, 500, &durata);
#ifdef DEBUG_CID
sprintf(buf, "** Tono (1): %04x %d", tone, durata);
support_log(buf);
#endif
        if(CID_calibr_modifica && !CID_calibr_done)
          stato = 3;
        else if((tone == 0x100) && (durata < 11))
          stato = 1;
        else
          tentativi++;
        break;
      case 1:
        tone = CID_read_tone(fd, 500, &durata);
#ifdef DEBUG_CID
sprintf(buf, "** Tono (2): %04x %d", tone, durata);
support_log(buf);
#endif
        if((tone == 0x200) && (durata < 11))
          stato = 2;
        else if((tone != 0x100) || (durata >= 11))
        {
          stato = 0;
          tentativi++;
        }
        break;
      case 3:
        /* E' necessaria la calibrazione, non trasmetto subito gli eventi
           ma uso questa come sessione unicamente di calibrazione. */
        /* La sessione di calibrazione è unica, voglio evitare che ad ogni
           identificazioni mi trovi in una condizione in cui vado avanti
           ed indietro. */
        CID_calibr_done = 1;
        tone = CID_calibr_modifica;
        CID_read_tone(fd, 500, &durata);
        if(tone == CID_calibr_modifica)
        {
          tentativi++;
          if(tentativi > 2)
          {
            /* Fine calibrazione, ma ormai mi sono perso la sincronizzazione
               con il ContactID, quindi chiudo la chiamata e la ripeto. */
            timeout_on(CID_delay, NULL, NULL, 0, CID_DELAY);
            
            /* Ferma la registrazione */
            buf[0] = 3;
            write(fd, buf, 1);
            /* Svuota la ricezione */
            while(read(fd, buf, 128));
#ifdef DEBUG_CID
            contactid_stop_rec();
#endif
            
            ser_setspeed(fd, B19200 | CRTSCTS, 1);
            
            fp = fopen("/saet/audiocal.bin", "w");
            if(fp)
            {
              fputc(CID_calibr_modifica, fp);
              fclose(fp);
            }
            CID_calibr_modifica = 0;
            return;
          }
        }
        break;
      default:
        break;
    }
  }
  while(((tone != 0) || (durata != 0)) && (stato != 2) && (tentativi < 10));
    
  if(stato != 2)
  {
#ifdef DEBUG_CID
support_log("** Errore identificazione");
    contactid_stop_rec();
#endif
    /* Nessuna risposta dal centro */
    CID_tentativi++;
    if(CID_tentativi > 8)
    {
      CID_tentativi = 0;
      CID_set_error(2);
      *msg = 0;
    }
    else
      timeout_on(CID_delay, NULL, NULL, 0, CID_DELAY);
    
    /* Ferma la registrazione */
    buf[0] = 3;
    write(fd, buf, 1);
    /* Svuota la ricezione */
    while(read(fd, buf, 128));
    
    ser_setspeed(fd, B19200 | CRTSCTS, 1);
    return;
  }
  
  CID_calibr_done = 1;
  
  /* Invio il messaggio */
  tentativi = 0;
  do
  {
    /* Non fermo la registrazione, la riproduzione tramite comando specifico
       può avvenire in parallelo. */
    /* Il ritardo di 250ms viene introdotto dalla gestione del comando stesso. */
    /* 21-02-2012: dove sono messi questi 250ms? Non mi risulta. Inserisco la pausa
       in modo esplicito. */
    //usleep(210000);
    /* Mi attendo silenzio per almeno 200ms (13*128=1664 campioni) */
    //CID_read_silence(fd, 13);
    /* Mi attendo silenzio per almeno 150ms (9*128=1152 campioni) */
    //CID_read_silence(fd, 9);
    /* Mi attendo silenzio per almeno 250ms (16*128=2048 campioni) */
    CID_read_silence(fd, 16);
    
#ifdef DEBUG_CID
support_log("** Invio");
#endif
    support_log("ContactID: trasmissione messaggio.");
    buf[0] = 5;
    memcpy(buf+1, ev, 16);
    write(fd, buf, 17);
    
    /* Attendo l'ACK, potrei ricevere anche l'eco dei toni trasmessi */
    tone = CID_read_tone_ACK(fd, 100+150);
    //tone = CID_read_tone(fd, 100+150, &durata);
#ifdef DEBUG_CID
sprintf(buf, "** Tono: %04x", tone);
support_log(buf);
#endif
    if((tone == 0x100) /*&& (durata > 50)*/)
    {
      support_log("ContactID: invio confermato.");
      /* Verifico se ho altri eventi da spedire */
      *msg = CID_get_message(ev);
      CID_tentativi = 0;
      tentativi = 0;
    }
    else if(tentativi < 8)
    {
      /* Reinvio il messaggio corrente */
      tentativi++;
    }
    else if(CID_tentativi < 8)
    {
      /* Reinvio il messaggio con una nuova chiamata */
      CID_tentativi++;
      timeout_on(CID_delay, NULL, NULL, 0, CID_DELAY);
      break;
    }
    else
    {
      /* Il messaggio proprio non parte... */
      *msg = 0;
      CID_set_error(2);
    }
  }
  while(*msg);
  
  /* Ferma la registrazione */
  buf[0] = 3;
  write(fd, buf, 1);
  /* Svuota la ricezione */
  while(read(fd, buf, 128));
  
#ifdef DEBUG_CID
  contactid_stop_rec();
#endif
  
  ser_setspeed(fd, B19200 | CRTSCTS, 1);
  
  if(!*msg) CID_tentativi = 0;
}


