#ifndef _CONSOLE_TELECOMANDI_H
#define _CONSOLE_TELECOMANDI_H

CALLBACK void* console_telecomandi_send(ProtDevice *dev, int press, void *null);
CALLBACK void* console_telecomandi_peripherals(ProtDevice *dev, int press, void *null);
CALLBACK void* console_telecomandi_sensorzonelist(ProtDevice *dev, int press, void *null);

#endif
