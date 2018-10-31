#ifndef _CONSOLE_STORICO_H
#define _CONSOLE_STORICO_H

#include "../protocol.h"

void console_storico_event_loop(ProtDevice *dev);

CALLBACK void* console_storico_realtime(ProtDevice *dev, int press, void *null);
CALLBACK void* console_storico_consolidated(ProtDevice *dev, int press, void *null);
CALLBACK void* console_storico_delete(ProtDevice *dev, int press, void *null);

#endif
