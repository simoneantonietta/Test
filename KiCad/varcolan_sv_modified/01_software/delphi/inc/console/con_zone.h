#ifndef _CONSOLE_ZONE_H
#define _CONSOLE_ZONE_H

CALLBACK void* console_zone_deactivate(ProtDevice *dev, int press, void *null);
CALLBACK void* console_zone_activate(ProtDevice *dev, int press, void *null);
CALLBACK void* console_zone_activate_w_alm(ProtDevice *dev, int press, void *null);
CALLBACK void* console_zone_activate_w_oos(ProtDevice *dev, int press, void *null);

#endif
