#ifndef _CONSOLE_ATTUATORI_H
#define _CONSOLE_ATTUATORI_H

CALLBACK void* console_attuatori_on_service(ProtDevice *dev, int press, void *null);
CALLBACK void* console_attuatori_out_of_service(ProtDevice *dev, int press, void *null);
CALLBACK void* console_attuatori_on(ProtDevice *dev, int press, void *null);
CALLBACK void* console_attuatori_off(ProtDevice *dev, int press, void *null);

#endif
