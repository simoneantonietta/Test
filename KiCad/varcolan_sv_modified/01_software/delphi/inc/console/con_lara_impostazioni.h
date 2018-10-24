#ifndef _CONSOLE_LARA_IMPOSTAZIONI_H
#define _CONSOLE_LARA_IMPOSTAZIONI_H

CALLBACK void* console_lara_impostazioni_fosett_show(ProtDevice *dev, int press, void *fosett_as_voidp);
CALLBACK void* console_lara_impostazioni_holiday_show(ProtDevice *dev, int press, void *hdays_ad_voidp);

CALLBACK void* console_lara_impostazioni_weektiming(ProtDevice *dev, int press, void *null);
CALLBACK void* console_lara_impostazioni_holidays(ProtDevice *dev, int press, void *null);

#endif
