#ifndef _SAET_ETHERNET_H
#define _SAET_ETHERNET_H

#include "protocol.h"

int eth_open(int port, ProtDevice *dev);
int eth_close(ProtDevice *dev);

int cluster_init();
void cluster_ipaddr(char *addr);
void cluster_elimina_if();
int cluster_verifica_if();
void cluster_activate();

#endif
