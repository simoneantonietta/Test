#ifndef _CONSOLE_LISTE_H
#define _CONSOLE_LISTE_H

CALLBACK void* console_liste_sens_alm(ProtDevice *dev, int press, void *null);
CALLBACK void* console_liste_sens_almMU(ProtDevice *dev, int press, void *null);
CALLBACK void* console_liste_sens_oos(ProtDevice *dev, int press, void *null);
CALLBACK void* console_liste_sens_failure(ProtDevice *dev, int press, void *null);
CALLBACK void* console_liste_sens_sabotage(ProtDevice *dev, int press, void *null);
CALLBACK void* console_liste_zone_active(ProtDevice *dev, int press, void *null);
CALLBACK void* console_liste_zone_not_active(ProtDevice *dev, int press, void *null);
CALLBACK void* console_liste_act_on(ProtDevice *dev, int press, void *null);
CALLBACK void* console_liste_act_oos(ProtDevice *dev, int press, void *null);
CALLBACK void* console_liste_holidays(ProtDevice *dev, int press, void *null);
CALLBACK void* console_liste_called_numbers(ProtDevice *dev, int press, void *null);

#endif
