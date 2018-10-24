#ifndef _CONSOLE_IMPOSTAZIONI_H
#define _CONSOLE_IMPOSTAZIONI_H

CALLBACK void* console_impostazioni_timerange(ProtDevice *dev, int press, void *null);
CALLBACK void* console_impostazioni_holidays(ProtDevice *dev, int press, void *null);
CALLBACK void* console_impostazioni_datehour(ProtDevice *dev, int press, void *null);
CALLBACK void* console_impostazioni_zonesens(ProtDevice *dev, int press, void *null);
CALLBACK void* console_impostazioni_zoneassoc(ProtDevice *dev, int press, void *null);
CALLBACK void* console_impostazioni_zonerit(ProtDevice *dev, int press, void *null);
CALLBACK void* console_impostazioni_timeout(ProtDevice *dev, int press, void *null);
CALLBACK void* console_impostazioni_codes(ProtDevice *dev, int press, void *null);
CALLBACK void* console_impostazioni_network(ProtDevice *dev, int press, void *null);

void console_impostazioni_insert_hour(ProtDevice *dev, int x, int y, int secs, ConsoleCallback enter, ConsoleCallback cancel);
void console_impostazioni_insert_date(ProtDevice *dev, int x, int y, int year, ConsoleCallback enter,  ConsoleCallback cancel);

#endif
