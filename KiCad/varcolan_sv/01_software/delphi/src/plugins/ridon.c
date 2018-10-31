#include "user.h"
#include "support.h"
#include "codec.h"
#include "master.h"
#include "timeout.h"
#include "ethernet.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip_icmp.h>
#include <arpa/inet.h>
#include <errno.h>

#ifdef __CRIS__
#include <asm/rtc.h>
#include "delphi.h"
#endif

//#define DEBUG

enum {RID_SYNCHRO=0, RID_DROPEVENT, RID_COMMAND, RID_CHECK,
  RID_ALIGN_TIME, RID_ALIGN_FILE, RID_ALIGN_START, RID_ALIGN_TIME2,
  RID_ALIGN_FILE2, RID_ACK_FILE2, RID_PING, RID_SET_TIME};

#define RID_TIMEOUT	6

#define RidonConf "/saet/data/ridon.conf"
#define LaraFile "/saet/data/lara.gz"
#define ClusterFile "/tmp/cluster"

static const unsigned char RID_Synchro[] = {STX, 1, 0, RID_SYNCHRO, -(2+1+RID_SYNCHRO)};
static const unsigned char RID_DropEvent[] = {STX, 1, 0, RID_DROPEVENT, -(2+1+RID_DROPEVENT)};
static const unsigned char RID_Check[] = {STX, 1, 0, RID_CHECK, -(2+1+RID_CHECK)};
static unsigned char RID_Align_Time[] = {STX, 5, 0, RID_ALIGN_TIME, 0, 0, 0, 0, -(2+5+RID_ALIGN_TIME)};
static unsigned char RID_Align_TimeLaraGz[] = {STX, 5, 0, RID_ALIGN_TIME2, 0, 0, 0, 0, -(2+5+RID_ALIGN_TIME2)};
static const unsigned char RID_Align_Start[] = {STX, 1, 0, RID_ALIGN_START, -(2+1+RID_ALIGN_START)};
static const unsigned char RID_Ack_FileLaraGz[] = {STX, 1, 0, RID_ACK_FILE2, -(2+1+RID_ACK_FILE2)};
static const unsigned char RID_Ping[] = {STX, 1, 0, RID_PING, -(2+1+RID_PING)};
static unsigned char RID_Set_Time[] = {STX, 5, 0, RID_SET_TIME, 0, 0, 0, 0, -(2+5+RID_ALIGN_TIME)};

static int ridon_fd = -1;
static int ridon_started = 0;
static int ridon_isalive = RID_TIMEOUT;
static int ridon_switched = 0;
static int ridon_preswitch = 0;
static int ridon_timealign = 0;

static sem_t ridon_sem;

static int ridon_ping_count = 0;
static int ridon_ping_fd;
static unsigned int ridon_ipserver[10] = {0, };

static const unsigned char ridon_icmp[64] = {0x08,0x00,0xf7,0xff,0x00,};

static void ridon_ping()
{
  struct sockaddr_in pingaddr;
  int i;
  
  pingaddr.sin_family = AF_INET;
  
  for(i=0; i<10; i++)
    if(ridon_ipserver[i])
    {
      pingaddr.sin_addr.s_addr = ridon_ipserver[i];
      sendto(ridon_ping_fd, ridon_icmp, sizeof(ridon_icmp), 0,
        (struct sockaddr *)&pingaddr, sizeof(struct sockaddr_in));
    }
}

static unsigned char ridon_cks(unsigned char *msg, int len)
{
  int i;
  unsigned char cks = 0;
  
  for(i=0; i<len; i++) cks += msg[i];
  return -cks;
}

static int ridon_sync(ProtDevice *dev, int command)
{
  char *cmd;
  
  if(master_behaviour == MASTER_ACTIVE)
  {
    if(!command)
      write(dev->fd, RID_DropEvent, sizeof(RID_DropEvent));
    else
    {
      cmd = (char*)malloc(dev->cmd.Len + 5);
      cmd[0] = 2;
      *(unsigned short*)(cmd+1) = dev->cmd.Len+1;
      cmd[3] = RID_COMMAND;
      memcpy(cmd+4, dev->cmd.Command, dev->cmd.Len);
      cmd[dev->cmd.Len + 4] = 0;
      cmd[dev->cmd.Len + 4] = ridon_cks(cmd, dev->cmd.Len + 4);
      write(dev->fd, cmd, dev->cmd.Len + 5);
      free(cmd);
    }
  }
  return 1;
}

static void ridon_send_synchro(void *null, int to)
{
  write(ridon_fd, RID_Synchro, sizeof(RID_Synchro));
}

static void ridon_start_master()
{
  if(master_behaviour == MASTER_ACTIVE)
  {
    ridon_send_synchro(NULL, 10);
    sem_wait(&ridon_sem);
    /* In modalità master attiva il cluster */
    cluster_activate();
    
    /* Faccio partire immediatamente la slave e invece
       ritardo di una secondo la master. Sono stati notati
       dei notx anomali alla partenza, più probabili quando
       la master partiva un attimo prima della slave. */
    sleep(1);
  }
  else
  {
    /* In modalità slave deve solo attendere la sincronizzazione */
    sem_wait(&ridon_sem);
  }
}

static void ridon_loop(ProtDevice *dev)
{
  int i, ret, idx, sincro_ok;
  static struct stat nv;
  long size;
  char *inbuf, *outbuf, buf[32];
  FILE *fp;
  struct tm datehour;
  struct termios tio;
#ifdef __CRIS__
  int fd;
  struct rtc_time rt;
#endif
  
  /* Capita quando il plugin esce dal loop per errore sulla seriale,
     ma devo rientrare qui per dare modo di chiudere in modo pulito la
     seriale soprattutto se è USB, così da liberare la risorsa per
     quando viene riconnessa. */
  if(ridon_fd != -1)
  {
    /* Non posso uscire, perché se questa fosse la centrale ancora
       attiva dopo uno switch si bloccherebbe pure lei, invece deve
       continuare a lavorare. Però se si torna qui comunque lo switch
       è già stato gestito (o lo si sta per fare) quindi è sufficiente
       entrare in un loop tranquillo. */
    tcgetattr(dev->fd, &tio);
    tio.c_cc[VTIME] = 10;
    tio.c_cc[VMIN] = 0;
    tcsetattr(dev->fd, TCSANOW, &tio);  
    tcflush(dev->fd, TCIOFLUSH);
    
    while(1)
    {
      /* Quando si stacca l'USB l'errore EIO viene dato solo dalla scrittura
         e non dalla lettura. La read() la lascio solo per fare attesa di 1s
         e si sblocca appena stacco. */
      dev->buf[0] = '\n';
      write(dev->fd, dev->buf, 1);
      if(errno == EIO) return;
      ret = read(dev->fd, dev->buf, 1);
      if((ret < 0) || (errno == EIO)) return;
    }
  }
  
  sem_init(&ridon_sem, 0, 0);
  
  master_ridon_sync = ridon_start_master;
  master_behaviour = SLAVE_STANDBY;
  if((i = open("/saet/plugins/ridon.master", O_RDONLY)) >= 0)
  {
    master_behaviour = MASTER_ACTIVE;
    close(i);
    sprintf(buf, "RID master [%d]", getpid());
  }
  else
    sprintf(buf, "RID slave [%d]", getpid());
  
  support_log(buf);
  
  ridon_fd = dev->fd;
  codec_sync_register(dev, ridon_sync);
  
  tcgetattr(dev->fd, &tio);
  tio.c_cc[VTIME] = 10;
  tio.c_cc[VMIN] = 0;
  tcsetattr(dev->fd, TCSANOW, &tio);  
  tcflush(dev->fd, TCIOFLUSH);
  
  fp = fopen(RidonConf, "r");
  if(fp)
  {
    while(fgets(buf, 31, fp))
    {
      if(!strncmp(buf, "Cluster=", 8))
      {
        cluster_ipaddr(buf+8);
      }
      else if(!strncmp(buf, "Server", 6) && (buf[6] >= '0') && (buf[6] <= '9') && (buf[7] == '='))
      {
        /* Salvo direttamente l'indirizzo predigerito, così non lo devo
           elaborare ogni volta che mi serve. */
        ridon_ipserver[buf[6]-'0'] = inet_addr(buf+8);
      }
    }
    fclose(fp);
  }
  
  /* Creare un processo a parte con fork ed execve non serve a nulla,
     il processo creato è comunque figlio di questo thread e ne segue
     le sorti. Soprattutto, in caso di crash il cluster potrebbe comunque
     morire in modo incontrollato, senza quindi rilascare l'IP condiviso. */
#if 0
  unlink(ClusterFile);
  i = open(ClusterFile, O_RDWR|O_CREAT, S_IRWXU);
  fp = fdopen(i, "w");
  fwrite(cluster_bin, sizeof(cluster_bin), 1, fp);
  fclose(fp);
  
  if(ipcluster && !fork())
  {
    argv[0] = ClusterFile;
    argv[1] = ipcluster;
    argv[2] = NULL;
    envp[0] = NULL;
    execve(ClusterFile, argv, envp);
  }
#endif
  
  ridon_ping_fd = socket(AF_INET, SOCK_RAW, 1);	// 1 = ICMP
  
  i = 0;
  inbuf = NULL;
  outbuf = NULL;
  size = 0;
  idx = 0;
  sincro_ok = 0;
  
  while(1)
  {
    ret = read(dev->fd, dev->buf+i, 1);
    if((ret < 0) || (errno == EIO)) return;
    if(!ret)
    {
      if(!ridon_started)
      {
        if(master_behaviour == MASTER_ACTIVE)
          ridon_send_synchro(NULL, 10);
        else
          sincro_ok = 0;
      }
      continue;
    }
    if(!i && (dev->buf[0] != STX)) continue;
    i++;
    if((i < 3) || (i < ((*(unsigned short*)(dev->buf+1))+4))) continue;
    i = 0;
    if(ridon_cks(dev->buf, ((*(unsigned short*)(dev->buf+1))+4))) continue;
    
    switch(dev->buf[3])
    {
      case RID_SYNCHRO:
#ifdef DEBUG
support_log("RID - Syncro");
printf("RID - Syncro\n");
#endif
        /*
           Se si riceve SYNC ma si e' gia' partiti e' perche' l'altra Delphi e' stata
           fatta ripartire. Affinche' si riallinei tutto e' necessario far ripartire
           anche questa Delphi e nel frattempo l'altra rimane in attesa.
           Altrimenti sono in fase di partenza, sblocco questa Delphi ed invio SYNC
           all'altra per sbloccare anche lei.
        */
        if(!ridon_started && !sincro_ok)
        {
          sincro_ok = 1;
          
          if(master_behaviour != MASTER_ACTIVE)
            write(dev->fd, RID_Synchro, sizeof(RID_Synchro));
          
          nv.st_mtime = 0;
          if(!stat(StatusFileName, &nv))
          {
            *(int*)(RID_Align_Time+4) = nv.st_mtime;
            RID_Align_Time[8] = ridon_cks(RID_Align_Time, 8);
            write(dev->fd, RID_Align_Time, sizeof(RID_Align_Time));
          }
          else
            write(dev->fd, RID_Align_Time, sizeof(RID_Align_Time));
        }
        break;
      case RID_ALIGN_TIME:
#ifdef DEBUG
support_log("RID - Align time");
printf("RID - Align time\n");
#endif
        if(*(int*)(dev->buf+4) > nv.st_mtime)
        {
#ifdef DEBUG
support_log("Attesa allineamento status");
printf("Attesa allineamento status\n");
#endif
          unlink(StatusFileName);
        }
        else if(*(int*)(dev->buf+4) < nv.st_mtime)
        {
          size = nv.st_size;
          inbuf = malloc(size);
          if(!inbuf) return;
          fp = fopen(StatusFileName, "r");
          fread(inbuf, size, 1, fp);
          fclose(fp);
          outbuf = malloc(256);
          if(!outbuf)
          {
            free(inbuf);
            return;
          }
          outbuf[0] = STX;
          outbuf[3] = RID_ALIGN_FILE;
          i = 0;
          while(size)
          {
            ret = size>248?248:size;
            size -= ret;
            memcpy(outbuf+4, inbuf+i, ret);
            *(short*)(outbuf+1) = ret+1;
            outbuf[4+ret] = ridon_cks(outbuf, 4+ret);
            write(dev->fd, outbuf, 5+ret);
            i += ret;
          }
          i = 0;
          *(short*)(outbuf+1) = 1;
          outbuf[4] = ridon_cks(outbuf, 4);
          write(dev->fd, outbuf, 5);
#ifdef DEBUG
support_log("Invio allineamento status");
printf("Invio allineamento status\n");
#endif
          free(inbuf);
          free(outbuf);
          
          nv.st_mtime = 0;
          if(!stat(LaraFile, &nv))
          {
            *(int*)(RID_Align_TimeLaraGz+4) = nv.st_mtime;
            RID_Align_TimeLaraGz[8] = ridon_cks(RID_Align_TimeLaraGz, 8);
            write(dev->fd, RID_Align_TimeLaraGz, sizeof(RID_Align_TimeLaraGz));
          }
          else
            write(dev->fd, RID_Align_TimeLaraGz, sizeof(RID_Align_TimeLaraGz));
        }
        else
        {
          nv.st_mtime = 0;
          if(!stat(LaraFile, &nv))
          {
            *(int*)(RID_Align_TimeLaraGz+4) = nv.st_mtime;
            RID_Align_TimeLaraGz[8] = ridon_cks(RID_Align_TimeLaraGz, 8);
            write(dev->fd, RID_Align_TimeLaraGz, sizeof(RID_Align_TimeLaraGz));
          }
          else
            write(dev->fd, RID_Align_TimeLaraGz, sizeof(RID_Align_TimeLaraGz));
        }
        break;
      case RID_ALIGN_FILE:
#ifdef DEBUG
support_log("RID - Align file");
printf("RID - Align file\n");
#endif
        size = *(unsigned short*)(dev->buf+1) - 1;
        if(size)
        {
          fp = fopen(StatusFileName, "a");
          fwrite(dev->buf+4, size, 1, fp);
          fclose(fp);
        }
        else
        {
          nv.st_mtime = 0;
          if(!stat(LaraFile, &nv))
          {
            *(int*)(RID_Align_TimeLaraGz+4) = nv.st_mtime;
            RID_Align_TimeLaraGz[8] = ridon_cks(RID_Align_TimeLaraGz, 8);
            write(dev->fd, RID_Align_TimeLaraGz, sizeof(RID_Align_TimeLaraGz));
          }
          else
            write(dev->fd, RID_Align_TimeLaraGz, sizeof(RID_Align_TimeLaraGz));
        }
        break;
      case RID_ALIGN_TIME2:
#ifdef DEBUG
support_log("RID - Align time 2");
printf("RID - Align time 2\n");
#endif
        if(*(int*)(dev->buf+4) > nv.st_mtime)
        {
#ifdef DEBUG
support_log("Attesa allineamento lara.gz");
printf("Attesa allineamento lara.gz\n");
#endif
          unlink(LaraFile);
        }
        else if(*(int*)(dev->buf+4) < nv.st_mtime)
        {
          size = nv.st_size;
          inbuf = malloc(size);
          if(!inbuf) return;
          fp = fopen(LaraFile, "r");
          fread(inbuf, size, 1, fp);
          fclose(fp);
          outbuf = malloc(256);
          if(!outbuf)
          {
            free(inbuf);
            return;
          }
          outbuf[0] = STX;
          outbuf[3] = RID_ALIGN_FILE2;
          idx = 0;
          ret = size>248?248:size;
          size -= ret;
          memcpy(outbuf+4, inbuf+idx, ret);
          *(short*)(outbuf+1) = ret+1;
          outbuf[4+ret] = ridon_cks(outbuf, 4+ret);
          write(dev->fd, outbuf, 5+ret);
          idx += ret;
#ifdef DEBUG
support_log("Invio allineamento lara.gz");
printf("Invio allineamento lara.gz\n");
#endif
        }
        else
        {
#ifdef DEBUG
support_log("Ridon start (1)");
printf("Ridon start (1)\n");
#endif
          ridon_started = 1;
          sem_post(&ridon_sem);
        }
        break;
      case RID_ALIGN_FILE2:
#ifdef DEBUG
support_log("RID - Align file 2");
printf("RID - Align file 2\n");
#endif
        size = *(unsigned short*)(dev->buf+1) - 1;
        if(size)
        {
          fp = fopen(LaraFile, "a");
          fwrite(dev->buf+4, size, 1, fp);
          fclose(fp);
          write(dev->fd, RID_Ack_FileLaraGz, sizeof(RID_Ack_FileLaraGz));
        }
        else
        {
#ifdef DEBUG
support_log("Ridon start (2)");
printf("Ridon start (2)\n");
#endif
          ridon_started = 1;
          sem_post(&ridon_sem);
          write(dev->fd, RID_Align_Start, sizeof(RID_Align_Start));
        }
        break;
      case RID_ACK_FILE2:
#ifdef DEBUG
support_log("Ridon ACK lara.gz");
printf("Ridon ACK lara.gz\n");
#endif
        outbuf[0] = STX;
        outbuf[3] = RID_ALIGN_FILE2;
        ret = size>248?248:size;
        size -= ret;
        memcpy(outbuf+4, inbuf+idx, ret);
        *(short*)(outbuf+1) = ret+1;
        outbuf[4+ret] = ridon_cks(outbuf, 4+ret);
        write(dev->fd, outbuf, 5+ret);
        idx += ret;
        break;
      case RID_ALIGN_START:
#ifdef DEBUG
support_log("Ridon start (3)");
printf("Ridon start (3)\n");
#endif
        free(inbuf);
        inbuf = NULL;
        free(outbuf);
        outbuf = NULL;
        
        ridon_started = 1;
        sem_post(&ridon_sem);
        break;
      case RID_DROPEVENT:
#ifdef DEBUG
support_log("RID drop event");
printf("RID - drop event\n");
#endif
        if(ridon_started) codec_consume_event(config.consumer[dev->consumer].param[0]-'0');
        break;
      case RID_COMMAND:
#ifdef DEBUG
support_log("RID command");
printf("RID - command\n");
#endif
        if(ridon_started) codec_parse_cmd2(dev->buf+4,
          (config.consumer[config.consumer[dev->consumer].param[0]-'0'].dev));
        break;
      case RID_CHECK:
#ifdef DEBUG
support_log("RID check");
printf("RID - check\n");
#endif
        if(ridon_started && (master_behaviour == MASTER_ACTIVE)) write(dev->fd, RID_Check, sizeof(RID_Check));
        if(master_behaviour == MASTER_ACTIVE)
          ridon_isalive = RID_TIMEOUT * 1.5;
        else
          ridon_isalive = RID_TIMEOUT;
        break;
      case RID_PING:
#ifdef DEBUG
support_log("RID ping");
printf("RID - ping\n");
#endif
        if(master_behaviour == MASTER_ACTIVE)
        {
          /* La centrale slave è in grado di fare il ping, quindi ci sono
             i prerequisiti per uno switch. Verifico ancora una volta se
             non si sia ripristinata la connessione anche sulla master. */
          ridon_preswitch = 1;
        }
        else if(ridon_started)
          ridon_ping();
        break;
      case RID_SET_TIME:
#ifdef DEBUG
support_log("RID set time");
printf("RID - set time\n");
#endif
  
#ifdef __CRIS__
        gmtime_r((time_t*)(dev->buf+4), &datehour);
        fd = open("/dev/rtc", O_RDONLY);
        if(fd < 0) break;
        
        rt.tm_year = datehour.tm_year;
        rt.tm_mon = datehour.tm_mon;
        rt.tm_mday = datehour.tm_mday;
        rt.tm_hour = datehour.tm_hour;
        rt.tm_min = datehour.tm_min;
        rt.tm_sec = datehour.tm_sec;
        
        ret = ioctl(fd, RTC_SET_TIME, &rt);
        if(ret < 0) ioctl(fd, RTC_SET_TIME_V1, &rt);
        
        close(fd);
#endif
        
        stime((time_t*)(dev->buf+4));
        
        localtime_r((time_t*)(dev->buf+4), &datehour);
        MINUTI = datehour.tm_min;
        ORE = datehour.tm_hour;
        SECONDI = datehour.tm_sec;
        GIORNO_S = datehour.tm_wday + 1;
        GIORNO = datehour.tm_mday;
        MESE = datehour.tm_mon + 1;
        ANNO = datehour.tm_year % 100;
        break;
      default:
        break;
    }
  }
}

static int ridon_timer(ProtDevice *dev, int param)
{
  unsigned char ev[128];
  struct timeval to;
  fd_set fds;
  int n, ret;
  
  static int cont1s = 0;
  
  /* Attenzione: il timer è chiamato 2 volte al secondo. */
  if(cont1s)
  {
    cont1s = 0;
    return 1;
  }
  
  cont1s = 1;
  
  if(ridon_started)
  {
    if(master_behaviour == MASTER_ACTIVE)
    {
      if(!ridon_timealign)
      {
        /* Allineo la data sullo slave */
        /* Ma solo se i secondi sono diversi da zero per evitare
           di perdere eventuali sincronizzazioni di centrale,
           lasciando anche un po' di margine. */
        n = time(NULL);
        if((n % 60) > 5)
        {
          *(int*)(RID_Set_Time+4) = n;
          RID_Set_Time[8] = ridon_cks(RID_Set_Time, 8);
          write(dev->fd, RID_Set_Time, sizeof(RID_Set_Time));
          /* Allineo ogni ora */
          ridon_timealign = 3600-1;
        }
      }
      else
        ridon_timealign--;
    }
    
    /* Verifico se siano arrivate risposte al ping, non serve farlo
       in modo puntuale, va bene una volta al secondo. Comunque prima di
       inviare un nuovo ping per essere sicuro di gestire correttamente
       la sequenza. */
    to.tv_sec = 0;
    to.tv_usec = 0;
    FD_ZERO(&fds);
    FD_SET(ridon_ping_fd, &fds);
    do
    {
      /* Estraggo tutte le risposte. */
      ret = select(ridon_ping_fd+1, &fds, NULL, NULL, &to);
      if(ret == 1)
      {
        //recvfrom(fd, packet, sizeof(packet), 0, (struct sockaddr *) &from, &fromlen);
        /* Non sono particolarmente interessato al campo from, basta ricevere un echo. */
        n = recvfrom(ridon_ping_fd, ev, sizeof(ev), 0, NULL, NULL);
        if(n > 0)
        {
          /* Verifico la raggiungibilità */
          n = (ev[0]&0xf)*4;
          /* type 0=reply, 3=unreachable */
          if(ev[n] == 0)
          {
            /* Ho la risposta da uno degli IP contattati, la rete è ok. */
            if(master_behaviour == MASTER_ACTIVE)
            {
              /* La master ricomincia il ciclo di verifica. */
              ridon_ping_count = 0;
            }
            else
            {
              /* La slave comunica il successo alla master. */
              write(dev->fd, RID_Ping, sizeof(RID_Ping));
            }
          }
        }
      }
    }
    while(ret == 1);
    
    ret = cluster_verifica_if();
    
    if(master_behaviour == SLAVE_STANDBY)
    {
      /* Solo la standby fa il check della master, dopo lo switch
         occorre resettare, quindi il check non serve piu'. */
      write(dev->fd, RID_Check, sizeof(RID_Check));
    }
    else if((master_behaviour == MASTER_ACTIVE) &&
            !ridon_switched &&
            (!LIN[config.consumer[dev->consumer].param[0]-'0'] || !ret))
    {
      /* Solo la master ha l'iniziativa di verificare la raggiungibilità
         del server, e lo fa solo se il server non è già connesso. */
      /* Cinque tentativi prima di chiedere alla slave ed innescare un
         eventuale switch. */
      if(ridon_ping_count < 5)
      {
        ridon_preswitch = 0;
        ridon_ping();
        ridon_ping_count++;
      }
      /* Chiedo 5 volte alla slave, se la slave rispone ok allora genero
         lo switch. Ma prima per sicurezza continuo con il ping anche
         sulla master. */
      else if(ridon_ping_count < 10)
      {
        ridon_ping();
        write(dev->fd, RID_Ping, sizeof(RID_Ping));
        ridon_ping_count++;
      }
      /* Se la master non riesce ma la slave sì, genero uno switch */
      else if(ridon_preswitch)
      {
        /* Se il server dovesse tornare raggiungibile proprio alla fine del
           ciclo, magari la slave comunica il successo ma la master ormai
           ha finito i cicli e non verifica più anche il proprio successo.
           per sicurezza quindi faccio ancora un paio di tentativi solo sulla
           master in modo da non essere influenzato ulteriormente dalla slave. */
        if(ridon_ping_count < 12)
        {
          ridon_ping();
          ridon_ping_count++;
        }
        else
        {
          support_log("Commutazione per perdita rete.");
          exit(0);
        }
      }
      /* Se invece nemmeno la slave risponde positivamente, ricomincio. */
      else
      {
        ridon_ping_count = 0;
      }
    }
    else
    {
      ridon_ping_count = 0;
    }
    
    if(ridon_isalive)
    {
      ridon_isalive--;
      if(!ridon_isalive)
      {
        ev[0] = Segnalazione_evento;
        
        if(master_behaviour == SLAVE_STANDBY)
        {
          master_set_active();
          cluster_activate();
          
          /* In questo caso è uno switch */
          ev[1] = 1000 >> 8;
          ev[2] = 1000 & 0xff;
        }
        else
        {
          support_log("RID: SLAVE disattiva");
          /* In questo caso è la slave che non risponde */
          ev[1] = 1001 >> 8;
          ev[2] = 1001 & 0xff;
        }
        codec_queue_event(ev);
        ridon_switched = 1;
        
#if 0
/* (10-11-2011) L'evento deve essere differenziato tra switch e slave non
   disponibile. Prima c'era l'IP a differenziare la sorgente, ora con la
   gestione ad IP comune devo sapere se si tratta di uno switch o di un
   problema sulla slave. */
        {
          char ev[4];
          
          /* Comunicare che la centrale slave e' caduta */
          ev[0] = Segnalazione_evento;
          ev[1] = 1000 >> 8;
          ev[2] = 1000 & 0xff;
          codec_queue_event(ev);
          
          ridon_switched = 1;
        }
#endif
      }
    }
  }
  
  return 1;
}

int Attivato_Backup()
{
  if(ridon_switched)
  {
    ridon_switched = 0;
    return 1;
  }
  return 0;
}

void _init()
{
  printf("Ridondanza plugin: " __DATE__ " " __TIME__ "\n");
  prot_plugin_register("RID", 0, ridon_timer, NULL, (PthreadFunction)ridon_loop);
}

