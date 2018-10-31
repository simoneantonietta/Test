#ifndef _SAET_MODEM_H
#define _SAET_MODEM_H

#include "protocol.h"

extern ProtDevice *modem_device;
extern char modem_serial[];

void modem_init();
void modem_setspeed(int baud);

int TMD_Call(int pbook_number);
int TMD_Call_terminate();
int TMD_Status_event(int event_index);
int TMD_Set_recovery(int onoff, int dim);

extern volatile char TMD_Status[4];

#endif
