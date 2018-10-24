#ifndef _SAET_SERIAL_H
#define _SAET_SERIAL_H

#include <termios.h>

int ser_open(const char *dev, int baud, int timeout);
int ser_close(int fdser);
int ser_setspeed(int fdser, int baud, int timeout);
char* ser_send_recv(int fdser, char *cmd, char *buf);

#endif
