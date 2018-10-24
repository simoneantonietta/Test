#include "protocol.h"
#include "database.h"
#include "support.h"
#include "user.h"
#include "master.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#ifdef __CRIS__
#include <asm/saetmm.h>
#endif
#include <fcntl.h>

struct remote2_data {
  int mm, fdmm, i;
  unsigned char buf[32];
  struct sockaddr_in sa;
  struct {
    int num, fd;
    unsigned char buf[16];
    int i;
  } perif[32];
};

#define REMOTE2 ((struct remote2_data*)((dev->prot_data)))

static const unsigned char remote2_cmd_dim[] = {
	 1, 1, 1,17, 2, 2, 2, 2, 5, 1,
	 1, 1, 1, 3, 4, 8, 5, 7, 3, 4,
	 3, 3,10, 3, 1, 1, 2, 9, 2, 2,
	 5,18, 4, 4};

static void remote2_slave(struct remote2_data *data)
{
  /* Questa centrale si presenta come periferica 0 di tipo SC8.
     La master la vedrà come periferica pari all'id impianto. */
  int n, i, at, code;
  fd_set fds;
  
  while(1)
  {
    data->perif[0].fd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(connect(data->perif[0].fd, (struct sockaddr*)&(data->sa), sizeof(struct sockaddr)))
    {
      close(data->perif[0].fd);
      sleep(1);
      continue;
    }
    
    /* Connesso, invio il messaggio di presentazione */
    data->perif[0].buf[0] = 7;	// richiesta stato ingressi
    data->perif[0].buf[1] = config.DeviceID;
    write(data->perif[0].fd, data->perif[0].buf, 2);
    
    data->perif[0].i = 0;
    
    FD_ZERO(&fds);
    n = data->perif[0].fd;
    if(data->fdmm > n) n = data->fdmm;
    n++;
    
    while(1)
    {
      FD_SET(data->perif[0].fd, &fds);
      FD_SET(data->fdmm, &fds);
      select(n, &fds, NULL, NULL, NULL);
      if(FD_ISSET(data->perif[0].fd, &fds))
      {
        /* Dal master... */
        i = read(data->perif[0].fd, data->perif[0].buf+data->perif[0].i, 32-data->perif[0].i);
        if(i > 0)
        {
          data->perif[0].i += i;
          if(data->perif[0].i >= remote2_cmd_dim[data->perif[0].buf[0]])
          {
            /* L'indirizzo periferica deve coincidere con il numero impianto,
               ma non sto a controllare... */
            data->perif[0].buf[1] = 0;
            code = data->perif[0].buf[0];
            switch(code)
            {
              case 7:
                /* Richiesta stato sensori -> attuatori */
                at = 0;
                for(i=0; i<8; i++)
                  if(AT[(data->mm<<8)+i] & bitON) at |= (1<< i);
                /* Trasformo la richiesta in attuazione */
                data->perif[0].buf[0] = 8;
                data->perif[0].buf[2] = 0x80;
                data->perif[0].buf[3] = (at>>4)|(at<<4);
                data->perif[0].buf[4] = 0;
                write(data->perif[0].fd, data->perif[0].buf, 5);
                break;
              case 8:
                /* Comando attuatori -> ingesso sensori */
                if(data->perif[0].buf[2] & 0x80)
                {
                  /* Trasformo l'attuazione in ingresso sensori */
                  data->perif[0].buf[0] = 7;
                  /* Inverto i nibble di attuazione */
                  data->perif[0].buf[2] = 0x80;
                  data->perif[0].buf[3] = (data->perif[0].buf[3]>>4)|(data->perif[0].buf[3]<<4);
                  write(data->fdmm, data->perif[0].buf, 4);
                }
                break;
              default:
                break;
            }
            
            memmove(data->perif[0].buf, data->perif[0].buf+remote2_cmd_dim[code],
              data->perif[0].i-remote2_cmd_dim[code]);
            data->perif[0].i -= remote2_cmd_dim[code];
          }
        }
        else
        {
          /* Chiusura socket */
          shutdown(data->perif[0].fd, 2);
          close(data->perif[0].fd);
          break;
        }
      }
      if(FD_ISSET(data->fdmm, &fds))
      {
        /* Dalla centrale... */
        i = read(data->fdmm, data->buf+data->i, 32-data->i);
        if(i > 0)
        {
          data->i += i;
          if(data->i >= remote2_cmd_dim[data->buf[0]])
          {
            switch(data->buf[0])
            {
              case 2:
                /* Emulazione solo della periferica 0 */
                data->buf[1] = 0xf2;
                memset(data->buf+2, 0xff, 15);
                write(data->fdmm, data->buf, 17);
                break;
              case 3:
                break;
              case 7:
              case 8:
                /* Emulazione solo della periferica 0 */
                if(data->buf[1] == 0)
                {
                  /* Inoltro */
                  write(data->perif[0].fd, data->buf, remote2_cmd_dim[data->buf[0]]);
                }
                break;
              default:
                break;
            }
            
            memmove(data->buf, data->buf+remote2_cmd_dim[data->buf[0]], data->i-remote2_cmd_dim[data->buf[0]]);
            data->i -= remote2_cmd_dim[data->buf[0]];
          }
        }
      }
    }
  }
}

static void remote2_master_parse(struct remote2_data *data)
{
  int i;
  
  i = read(data->fdmm, data->buf+data->i, 32);
  if(i > 0)
  {
    data->i += i;
    if(data->i < remote2_cmd_dim[data->buf[0]]) return;
  }
  
  switch(data->buf[0])
  {
    case 2:
      /* Richiesta periferiche presenti */
      memset(data->buf+1, 0x22, 16);
      write(data->fdmm, data->buf, 17);
      break;
    case 3:
      /* Invio periferiche previste - ignoro */
      break;
    case 7:
      /* richiesta stato ingressi - da inoltrare a slave */
    case 8:
      /* attuazioni - da inoltrare a slave */
      /* Gestione comune */
      for(i=0; (i<32)&&(data->perif[i].num!=data->buf[1]); i++);
      if(i < 32)
      {
        /* Inoltro pari pari */
        write(data->perif[i].fd, data->buf, remote2_cmd_dim[data->buf[0]]);
      }
      break;
    default:
      break;
  }
  
  memmove(data->buf, data->buf+remote2_cmd_dim[data->buf[0]], data->i-remote2_cmd_dim[data->buf[0]]);
  data->i -= remote2_cmd_dim[data->buf[0]];
}

static void remote2_master_parse_perif(struct remote2_data *data, int idx)
{
  int i, at, code;
  
  /* Messaggi in arrivo dallo slave. */
  /* Assumo che tutti i comandi siano in formato modulo master e
     che quindi il secondo byte sia l'indrizzo periferica. */
  if(data->perif[idx].i > 1)
    data->perif[idx].num = data->perif[idx].buf[1];
  
  code = data->perif[idx].buf[0];
  if(data->perif[idx].i >= remote2_cmd_dim[code])
  {
    switch(code)
    {
      case 7:
        /* Richiesta stato sensori -> attuatori */
        at = 0;
        for(i=0; i<8; i++)
          if(AT[(data->mm<<8)+(data->perif[idx].num<<3)+i] & bitON) at |= (1<< i);
        /* Trasformo la richiesta in attuazione */
        data->perif[idx].buf[0] = 8;
        data->perif[idx].buf[2] = 0x80;
        data->perif[idx].buf[3] = (at>>4)|(at<<4);
        data->perif[idx].buf[4] = 0;
        write(data->perif[idx].fd, data->perif[idx].buf, 5);
        break;
      case 8:
        /* Comando attuatori -> ingesso sensori */
        if(data->perif[idx].buf[2] & 0x80)
        {
          /* Trasformo l'attuazione in ingresso sensori */
          data->perif[idx].buf[0] = 7;
          /* Inverto i nibble di attuazione */
          data->perif[idx].buf[2] = 0x80;
          data->perif[idx].buf[3] = (data->perif[idx].buf[3]>>4)|(data->perif[idx].buf[3]<<4);
          write(data->fdmm, data->perif[idx].buf, 4);
        }
        break;
      default:
        break;
    }
    
    memmove(data->perif[idx].buf, data->perif[idx].buf+remote2_cmd_dim[code],
      data->perif[idx].i-remote2_cmd_dim[code]);
    data->perif[idx].i -= remote2_cmd_dim[code];
  }
}

static void remote2_loop(ProtDevice *dev)
{
  int i, n1, n2, n3, n4, ret;
  char tparam[32];
  static int one = 1;
  fd_set fds;
  
  if(!config.consumer[dev->consumer].param)
  {
    sleep(1);
    return;
  }
  
  dev->prot_data = malloc(sizeof(struct remote2_data));
  if(!dev->prot_data) return;
  
  REMOTE2->i = 0;
  REMOTE2->mm = 16;
  
  /* lettura parametri - master:n.modulo slave:ip master */
  /* Tiro via tutti i caratteri che non servono */
  sscanf(config.consumer[dev->consumer].param, "%s", tparam);
  ret = sscanf(tparam, "%d:%d.%d.%d.%d", &(REMOTE2->mm), &n1, &n2, &n3, &n4);
  if(REMOTE2->mm > 15)
  {
    sleep(1);
    return;
  }
  
  if(config.consumer[dev->consumer].configured == 5)
  {
    /* lettura indirizzo IP */
    if(ret == 5)
    {
      /* SLAVE */
      if(config.DeviceID > 31) return;
      
      /* Occorre assicurare che il processo saet abbia già creato i file
         descriptor per i moduli reali, altrimenti si crea prima il virtuale
         che viene sovrascritto dal modulo reale. */
      sleep(1);
      
      /* Sovrappongo il modulo emulato a quello reale */
      i = open("/dev/pxl", O_RDWR);
      master_register_module(REMOTE2->mm, i);
#ifdef __CRIS__  
      /* Reset MM */
      ioctl(i, MM_RESET, 0);
#endif
      
      REMOTE2->fdmm = open("/dev/pxlmm", O_RDWR);
      write(REMOTE2->fdmm, "\033\013\000\000\000\001", 7);	// versione 11.0.0.0 e modulo master tipo 0
      
      for(i=0; tparam[i]!=':'; i++);
      REMOTE2->sa.sin_family = AF_INET;
      REMOTE2->sa.sin_port = htons(config.consumer[dev->consumer].data.eth.port);
      REMOTE2->sa.sin_addr.s_addr = inet_addr(tparam+i+1);
      
      remote2_slave(REMOTE2);
    }
    else
    {
      /* MASTER */
      /* Occorre assicurare che il processo saet abbia già creato i file
         descriptor per i moduli reali, altrimenti si crea prima il virtuale
         che viene sovrascritto dal modulo reale. */
      sleep(1);
      
      /* Sovrappongo il modulo emulato a quello reale */
      i = open("/dev/pxl", O_RDWR);
      master_register_module(REMOTE2->mm, i);
#ifdef __CRIS__  
      /* Reset MM */
      ioctl(i, MM_RESET, 0);
#endif
      
      dev->fd_eth = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
      if(dev->fd_eth < 0) return;
      setsockopt(dev->fd_eth, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(int));
      REMOTE2->sa.sin_family = AF_INET;
      REMOTE2->sa.sin_port = htons(config.consumer[dev->consumer].data.eth.port);
      REMOTE2->sa.sin_addr.s_addr = INADDR_ANY;
      if(bind(dev->fd_eth, (struct sockaddr*)&REMOTE2->sa, sizeof(struct sockaddr_in)))
      {
        close(dev->fd_eth);
        return;
      }
      listen(dev->fd_eth, 0);
      
      for(i=0; i<32; i++)
      {
        REMOTE2->perif[i].fd = -1;
        REMOTE2->perif[i].num = -1;
        REMOTE2->perif[i].i = 0;
      }
      
      REMOTE2->fdmm = open("/dev/pxlmm", O_RDWR);
      write(REMOTE2->fdmm, "\033\013\000\000\000\001", 7);	// versione 11.0.0.0 e modulo master tipo 0
      
      n3 = REMOTE2->fdmm;
      if(dev->fd_eth > n3) n3 = dev->fd_eth;
      FD_ZERO(&fds);
      
      while(1)
      {
        FD_SET(REMOTE2->fdmm, &fds);
        FD_SET(dev->fd_eth, &fds);
        n4 = n3;
        for(i=0; i<32; i++)
        {
          if(REMOTE2->perif[i].fd >= 0)
          {
            FD_SET(REMOTE2->perif[i].fd, &fds);
            if(REMOTE2->perif[i].fd > n4) n4 = REMOTE2->perif[i].fd;
          }
        }
        select(n4+1, &fds, NULL, NULL, NULL);
        
        if(FD_ISSET(dev->fd_eth, &fds))
        {
          /* Cerco uno slot libero */
          for(i=0; i<32; i++)
            if(REMOTE2->perif[i].fd < 0)
            {
              REMOTE2->perif[i].fd = accept(dev->fd_eth, NULL, NULL);
              break;
            }
          if(i == 32)
          {
            /* Rifiuto la connessione */
            i = accept(dev->fd_eth, NULL, NULL);
            close(i);
          }
        }
        if(FD_ISSET(REMOTE2->fdmm, &fds))
        {
          /* Comando da centrale che deve essere gestito/inoltrato */
          remote2_master_parse(REMOTE2);
        }
        for(i=0; i<32; i++)
        {
          if((REMOTE2->perif[i].fd >= 0) && (FD_ISSET(REMOTE2->perif[i].fd, &fds)))
          {
            /* Comando da slave che deve essere gestito */
            n4 = read(REMOTE2->perif[i].fd, REMOTE2->perif[i].buf+REMOTE2->perif[i].i, 16-REMOTE2->perif[i].i);
            if(n4 <= 0)
            {
              shutdown(REMOTE2->perif[i].fd, 2);
              close(REMOTE2->perif[i].fd);
              REMOTE2->perif[i].fd = -1;
              REMOTE2->perif[i].num = -1;
            }
            else
            {
              REMOTE2->perif[i].i += n4;
              remote2_master_parse_perif(REMOTE2, i);
            }
          }
        }
      }
    }
  }
  else
  {
    /* Solo remotizzazione LAN */
    sleep(1);
    return;
  }
}

void _init()
{
  printf("Remotizzazione (v.2) (plugin): " __DATE__ " " __TIME__ "\n");
  prot_plugin_register("REM2", 0, NULL, NULL, (PthreadFunction)remote2_loop);
}

