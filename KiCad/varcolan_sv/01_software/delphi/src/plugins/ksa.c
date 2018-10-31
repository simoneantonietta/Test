#include "../protocol.h"
#include "../codec.h"
#include "../serial.h"
#include "../support.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

#define KSA_ADDR	0x0a
#define KSA_NUMCMD	6

#define KSA	((unsigned char*)(dev->prot_data))

/*
  ev[0] = Evento_Esteso;
  ev[1] = Ex_Kilsen;
  ev[2] = (codice allarme);
  ev[3] = (lunghezza dati evento);
  ev[4] = (dati evento);
  codec_queue_event(ev);
*/

static short ksa_updcrc(short cc, short c)
{
  int i;
  
  c <<= 8;
  for(i=0; i<8; i++)
  {
    if((cc ^ c) & 0x8000)
      cc = (cc<<1)^0x1021;
    else
      cc <<= 1;
      
    c <<= 1;
  }
  
  return cc;
}

static unsigned short ksa_crc(unsigned char *d, int len)
{
  int n;
  unsigned short c = 0;
  
  for(n=0; n<len; n++)
    c = ksa_updcrc(c, d[n]);
  return c;
}

static int ksa_read_base(int fd, unsigned char *buf)
{
  int n, dim = 0;
  
  while(dim < 10)
  {
    n = read(fd, buf+dim, 10-dim);
    if(n)
      dim += n;
    else
      return 0;
  }
  return dim;
}

static int ksa_read_esteso(int fd, unsigned char *buf, int dimext)
{
  int n, dim = 0;
  
  while(dim < dimext)
  {
    n = read(fd, buf+dim, dimext-dim);
    dim += n;
  }
  return dim;
}

static void ksa_init(ProtDevice *dev)
{
  dev->prot_data = calloc(KSA_NUMCMD, 1);
}

static char *avaria1[] = {"Elemento non risponde", "Risposta doppia", "Tipo cambiato", "Taglio linea", "CRUCE EN ...", "Elemento sporco"};
static char *avaria2[] = {"Avaria del loop", "Avaria delle sirene", "Avaria di alimentazione", "Avaria messa a terra", "Avaria fuori servizio", "Avaria del sistema", "Avaria RS-232/485"};

static void ksa_parse(ProtDevice *dev, unsigned char *buf)
{
  unsigned char ev[80];
  int i;
  
  switch(buf[6])
  {
    case 3:
      KSA[0] = 1;
      break;
    case 16:
      ev[0] = Evento_Esteso;
      ev[1] = Ex_Stringa;
      sprintf(ev+4, "KSA [AVR]- Centrale: %d Loop: %d Zona: %d Ind: %d", buf[10], buf[11], buf[12], buf[13]);
      ev[3] = strlen(ev+4);
      codec_queue_event(ev);
      if(buf[21] < 100)
      {
        sprintf(ev+15, "%s", avaria1[buf[21]]);
      }
      else
      {
        sprintf(ev+15, "%s", avaria2[buf[21]-100]);
      }
      ev[3] = strlen(ev+4);
      codec_queue_event(ev);
      ev[3] = 51;
      memcpy(ev+15, buf+24, 40);
      codec_queue_event(ev);
      break;
    case 17:
      ev[0] = Evento_Esteso;
      ev[1] = Ex_Stringa;
      sprintf(ev+4, "KSA [DIS]- Centrale: %d Loop: %d Zona: %d Ind: %d", buf[10], buf[11], buf[12], buf[13]);
      ev[3] = strlen(ev+4);
      codec_queue_event(ev);
      if(buf[11])
      {
        sprintf(ev+15, "Disconnessione elemento. Tipo: %d", buf[14]);
        ev[3] = strlen(ev+4);
      }
      else
      {
        if(buf[12])
        {
          sprintf(ev+15, "Disconnessione zona.");
          ev[3] = 31;
        }
        else
        {
          sprintf(ev+15, "Disconnessione generale.");
          ev[3] = 35;
        }
      }
      codec_queue_event(ev);
      ev[3] = 51;
      memcpy(ev+15, buf+24, 40);
      codec_queue_event(ev);
      break;
    case 18:
      ev[0] = Evento_Esteso;
      ev[1] = Ex_Stringa;
      sprintf(ev+4, "KSA [PRE]- Centrale: %d Loop: %d Zona: %d Ind: %d", buf[10], buf[11], buf[12], buf[13]);
      ev[3] = strlen(ev+4);
      codec_queue_event(ev);
      sprintf(ev+15, "Tipo: %d Valore: %d", buf[14], buf[21]);
      ev[3] = strlen(ev+4);
      codec_queue_event(ev);
      ev[3] = 51;
      memcpy(ev+15, buf+24, 40);
      codec_queue_event(ev);
      break;
    case 19:
      ev[0] = Evento_Esteso;
      ev[1] = Ex_Stringa;
      sprintf(ev+4, "KSA [ALM]- Centrale: %d Loop: %d Zona: %d Ind: %d", buf[10], buf[11], buf[12], buf[13]);
      ev[3] = strlen(ev+4);
      codec_queue_event(ev);
      if(buf[11])
      {
        sprintf(ev+15, "Tipo: %d Valore: %d", buf[14], buf[21]);
        ev[3] = strlen(ev+4);
      }
      else
      {
        sprintf(ev+15, "Attivazione manuale sirene");
        ev[3] = 41;
      }
      codec_queue_event(ev);
      ev[3] = 51;
      memcpy(ev+15, buf+24, 40);
      codec_queue_event(ev);
      break;
    case 20:
      ev[0] = Evento_Esteso;
      ev[1] = Ex_Stringa;
      sprintf(ev+4, "KSA [PLS]- Centrale: %d Loop: %d Zona: %d Ind: %d", buf[10], buf[11], buf[12], buf[13]);
      ev[3] = strlen(ev+4);
      codec_queue_event(ev);
      ev[3] = 51;
      memcpy(ev+15, buf+24, 40);
      codec_queue_event(ev);
      
#ifdef DEBUG
      printf("-------- ALLARME --(%02d)--\n", buf[6]);
      printf("Centrale : %d\n", buf[10]);
      printf("Loop     : %d\n", buf[11]);
      printf("Zona     : %d\n", buf[12]);
      printf("Direzione: %d\n", buf[13]);
      printf("Tipo     : %d\n", buf[14]);
      printf("Ora      : %02d:%02d\n", buf[15], buf[16]);
      printf("Data     : %02d/%02d/%02d%02d\n", buf[17], buf[18], buf[19], buf[20]);
      printf("Dato/Cod.: %d\n", buf[21]);
#endif
/*
      ev[0] = Evento_Esteso;
      ev[1] = Ex_Kilsen;
      ev[2] = buf[6];
      ev[3] = 54;
      memcpy(ev+4, buf+10, 54);
      codec_queue_event(ev);
*/
      break;
    default:
      break;
  }

  if(KSA[0])
  {
    KSA[0] = 0;
    buf[6] = 0xe2;	// prog. grafico
  }
  else
  {
    buf[6] = 0x00;
    for(i=1; i<KSA_NUMCMD; i++)
      if(KSA[i])
      {
        buf[6] = i;
        KSA[i] = 0x00;
        break;
      }
  }
  
  buf[2] = buf[3];
  buf[3] = KSA_ADDR;
  buf[4] = 0;
  buf[5] = 0x1c;
  *(short*)(buf+8) = ksa_crc(buf, 8);
  write(dev->fd, buf, 10);
}

static char *ksa_generico[] = {"", "Riarmo sistema", "Attivazione sirene", "Silenziare sirene", "Tacitazione sirene"};

static void ksa_loop(ProtDevice *dev)
{
  int n;
  unsigned char buf[256];
  unsigned char ev[64];
  
  ksa_init(dev);
/*  
  buf[6] = 16;
  buf[10] = 1;
  buf[11] = 1;
  buf[12] = 1;
  buf[13] = 1;
  buf[14] = 1;
  memset(buf+24, 'A', 40);
  ksa_parse(dev, buf);
*/  
  while(1)
  {
    n = ksa_read_base(dev->fd, buf);
    if(n && (buf[0] == 0xa5) && (buf[1] == 0x53) &&
       (*(unsigned short*)(buf+8) == ksa_crc(buf, 8)))
    {
      ksa_read_esteso(dev->fd, buf+10, buf[4]);
      
      if((buf[2] == KSA_ADDR) && (buf[5] == 0x1a))
        ksa_parse(dev, buf);
      else if((buf[2] == 0xff) && (buf[5] == 0x1b))
      {
          ev[0] = Evento_Esteso;
          ev[1] = Ex_Stringa;
          sprintf(ev+4, "KSA - Centrale: -> %s", ksa_generico[buf[6]]);
          ev[3] = strlen(ev+4);
          codec_queue_event(ev);
      }
    }
  }
}

/*
Comandi KSA700:
cmd	significato
---	------------------
 1	Resincronizzazione
 2	Riarmo
 3	Attivazione sirena
 4	Silenziare sirena
 5	Tacitazione sirena
*/

static int ksa_command(ProtDevice *dev, int cmd)
{
  if(dev->prot_data && (cmd > 0) && (cmd < KSA_NUMCMD))
  {
    KSA[cmd] = 1;
    return 1;
  }
  else
    return 0;
}

void _init()
{
  printf("KSA plugin: " __DATE__ " " __TIME__ "\n");
  prot_plugin_register("KSA", 0, NULL, ksa_command, (PthreadFunction)ksa_loop);
}
