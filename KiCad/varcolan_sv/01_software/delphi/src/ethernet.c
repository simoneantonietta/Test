#include "ethernet.h"
#include "support.h"
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <syscall.h>

static void* eth_listen(ProtDevice *dev)
{
  char log[64];
  struct sockaddr_in sa;
  int len;
  pid_t pid;
  
  pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
  
  debug_pid[dev->consumer] = support_getpid();
  
  sprintf(log, "ETH Init (server) [%d]", debug_pid[dev->consumer]);
  support_log(log);
  
  listen(dev->fd_eth, 0);
  
  while(1)
  {
    sprintf(log, "Attesa connessione [%d]", debug_pid[dev->consumer]);
    support_log(log);
    len = sizeof(sa);
    dev->fd = accept(dev->fd_eth, (struct sockaddr*)&sa, (void*)&len);
    if(dev->fd >= 0)
    {
      sprintf(log, "Connessione da %s [%d]", inet_ntoa(sa.sin_addr), debug_pid[dev->consumer]);
      support_log(log);
      dev->protocol_active = 1;
      prot_plugin_start(dev->consumer);
      if(dev->fd >= 0) close(dev->fd);
      dev->fd = -1;
    }
    else
    {
      support_log("ETH Close");
      return NULL;
    }
  }
}

static void* eth_plugin(ProtDevice *dev)
{
  char log[32];
  
  pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
  
  debug_pid[dev->consumer] = support_getpid();
  
  sprintf(log, "ETH Init (plugin) [%d]", debug_pid[dev->consumer]);
  support_log(log);
  
  while(1)
  {
    prot_plugin_start(dev->consumer);
    if(dev->fd >= 0) close(dev->fd);
    dev->fd = -1;
  }
}

int eth_open(int port, ProtDevice *dev)
{
  struct sockaddr_in sa;
  static int one = 1;
  
  if(!config.consumer[dev->consumer].param)
  {
    dev->fd_eth = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(dev->fd_eth >= 0)
    {
      setsockopt(dev->fd_eth, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(int));
      sa.sin_family = AF_INET;
      sa.sin_port = htons(port);
      sa.sin_addr.s_addr = INADDR_ANY;
      if(bind(dev->fd_eth, (struct sockaddr*)&sa, sizeof(struct sockaddr_in)))
      {
        close(dev->fd_eth);
        return 0;
      }
      pthread_create(&(dev->th_eth), NULL, (PthreadFunction)eth_listen, dev);
      pthread_detach(dev->th_eth);
      return 1;
    }
  }
  else
  {
    pthread_create(&(dev->th_eth), NULL, (PthreadFunction)eth_plugin, dev);
    pthread_detach(dev->th_eth);
    return 1;
  }
  
  return 0;
}

int eth_close(ProtDevice *dev)
{
  if(dev->fd >= 0)
  {
    shutdown(dev->fd, 2);
    close(dev->fd);
  }
  shutdown(dev->fd_eth, 2);
  close(dev->fd_eth);

  return 0;
}

/********************************/
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <netinet/ether.h>
#include <netpacket/packet.h>

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;

#include <linux/ethtool.h>
#include <linux/sockios.h>
#include "master.h"

static int cluster_fd = -1;
static struct sockaddr_in cluster_sockaddr;
static int cluster_running = 0;
static char *cluster_addr = NULL;

static void cluster_gratuitousARP(struct sockaddr_in *addr)
{
  int fd;
  unsigned int alen;
  struct ifreq ifr;
  struct sockaddr_ll me;
  unsigned char buf[256];
  struct arphdr *ah = (struct arphdr *) buf;
  unsigned char *p = (unsigned char *) (ah + 1);
  
  fd = socket(PF_PACKET, SOCK_DGRAM, 0);
  
  memset(&ifr, 0, sizeof(ifr));
  strcpy(ifr.ifr_name, "eth0:1");
  ioctl(fd, SIOCGIFINDEX, &ifr);
  
  me.sll_family = AF_PACKET;
  me.sll_ifindex = ifr.ifr_ifindex;
  me.sll_protocol = htons(ETH_P_ARP);
  bind(fd, (struct sockaddr *) &me, sizeof(me));
  alen = sizeof(me);
  getsockname(fd, (struct sockaddr *) &me, &alen);
  
  //ah->ar_hrd = htons(me.sll_hatype);
  ah->ar_hrd = htons(ARPHRD_ETHER);
  ah->ar_pro = htons(ETH_P_IP);
  ah->ar_hln = me.sll_halen;
  ah->ar_pln = 4;
  ah->ar_op = htons(ARPOP_REQUEST);
  
  /* Sender */
  memcpy(p, &me.sll_addr, ah->ar_hln);
  p += me.sll_halen;
  
  memcpy(p, &(addr->sin_addr.s_addr), 4);
  p += 4;
  
  /* Target */
  memset(p, -1, ah->ar_hln);
  p += ah->ar_hln;
  
  memcpy(p, &(addr->sin_addr.s_addr), 4);
  p += 4;
  
  /* Gratuitous ARP (broadcast) */
  memset(me.sll_addr, -1, me.sll_halen);
  sendto(fd, buf, p - buf, 0, (struct sockaddr *) &me, sizeof(me));
  
  close(fd);
}

int cluster_init()
{
  cluster_fd = socket(PF_INET, SOCK_STREAM, 0);
  /* Disattiva una eventuale configurazione residua */
  cluster_elimina_if();
  
  return cluster_fd;
}

void cluster_elimina_if()
{
  struct ifreq ifr;
  
  strcpy(ifr.ifr_name, "eth0:1");
  ioctl(cluster_fd, SIOCGIFFLAGS, &ifr);
  ifr.ifr_flags &= ~IFF_UP;
  ioctl(cluster_fd, SIOCSIFFLAGS, &ifr);
}

void cluster_ipaddr(char *addr)
{
  free(cluster_addr);
  cluster_addr = strdup(addr);
}

int cluster_verifica_if()
{
  struct ethtool_cmd cmd;
  struct ifreq ifr;
  
  cmd.cmd = ETHTOOL_GSET;
  memset(&ifr, 0, sizeof(ifr));
  strcpy(ifr.ifr_name, "eth0");
  ifr.ifr_data = (caddr_t)&cmd;
  ioctl(cluster_fd, SIOCETHTOOL, &ifr);
  
  /* Verificare lo stato della rete, se si riconnette il cavo occorre inviare un GratuitousARP.
  Es: la ridondanza attiva la slave, ma la slave era senza cavo di rete, il router non sa che
  l'IP del cluster ora è associato ad un MAC differente. */
  if(cluster_addr)
  {
    if(!cmd.speed)
      cluster_running = 0;
    else if(!cluster_running && ((master_behaviour == MASTER_ACTIVE) || (master_behaviour == SLAVE_ACTIVE)))
    {
      cluster_running = 1;
      cluster_gratuitousARP(&cluster_sockaddr);
    }
  }
  
  return cmd.speed?1:0;
}

void cluster_activate()
{
  struct ifreq ifr;
  
  if(cluster_addr)
  {
    /* Termino ipsetd per evitare che un ping di verifica attivazione IP
       non vada ad impostare lo stesso indirizzo anche per eth0. */
    system("killall ipsetd");
    
    strcpy(ifr.ifr_name, "eth0:1");
    cluster_sockaddr.sin_family = AF_INET;
    cluster_sockaddr.sin_addr.s_addr = inet_addr(cluster_addr);
    memcpy(&ifr.ifr_addr, &cluster_sockaddr, sizeof(struct sockaddr));
    ioctl(cluster_fd, SIOCSIFADDR, &ifr);
    
    cluster_gratuitousARP(&cluster_sockaddr);
    
    cluster_running = 1;
    support_log("Cluster attivo");
  }
}


