#ifndef _CONSOLE_SICUREZZA_H
#define _CONSOLE_SICUREZZA_H

CALLBACK void* console_sicurezza_pwd(ProtDevice *dev, int press, void *user);
CALLBACK void* console_sicurezza_create(ProtDevice *dev, int press, void *null);
CALLBACK void* console_sicurezza_delete(ProtDevice *dev, int press, void *null);

#endif
