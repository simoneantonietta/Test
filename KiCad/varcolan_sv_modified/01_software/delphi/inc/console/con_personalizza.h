#ifndef _CONSOLE_PERSONALIZZA_H
#define _CONSOLE_PERSONALIZZA_H

CALLBACK void* console_personalizza_zone(ProtDevice *dev, int press, void *null);
CALLBACK void* console_personalizza_sensor(ProtDevice *dev, int press, void *null);
CALLBACK void* console_personalizza_actuator(ProtDevice *dev, int press, void *null);
CALLBACK void* console_personalizza_phonebook(ProtDevice *dev, int press, void *null);
CALLBACK void* console_personalizza_command(ProtDevice *dev, int press, void *null);
CALLBACK void* console_personalizza_code(ProtDevice *dev, int press, void *null);

CALLBACK void* console_personalizza_profile(ProtDevice *dev, int press, void *null);
CALLBACK void* console_personalizza_area(ProtDevice *dev, int press, void *null);
CALLBACK void* console_personalizza_holiday(ProtDevice *dev, int press, void *null);
CALLBACK void* console_personalizza_weektiming(ProtDevice *dev, int press, void *null);
CALLBACK void* console_personalizza_terminal(ProtDevice *dev, int press, void *null);

#endif
