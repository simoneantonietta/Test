#ifndef _CONSOLE_MODEM_H
#define _CONSOLE_MODEM_H

CALLBACK void* console_modem_store(ProtDevice *dev, int press, void *null);
CALLBACK void* console_modem_delete(ProtDevice *dev, int press, void *null);
CALLBACK void* console_modem_enable(ProtDevice *dev, int press, void *null);
CALLBACK void* console_modem_disable(ProtDevice *dev, int press, void *null);
CALLBACK void* console_modem_rec(ProtDevice *dev, int press, void *null);

#endif
