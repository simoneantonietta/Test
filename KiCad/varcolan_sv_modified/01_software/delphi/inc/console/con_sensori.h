#ifndef _CONSOLE_SENSORI_H
#define _CONSOLE_SENSORI_H

CALLBACK void* console_sensori_on_service(ProtDevice *dev, int press, void *null);
CALLBACK void* console_sensori_out_of_service(ProtDevice *dev, int press, void *null);
CALLBACK void* console_sensori_accept(ProtDevice *dev, int press, void *null);
CALLBACK void* console_sensori_accept_all(ProtDevice *dev, int press, void *null);

#endif
