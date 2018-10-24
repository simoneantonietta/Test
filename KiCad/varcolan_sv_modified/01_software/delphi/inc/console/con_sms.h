#ifndef _CONSOLE_SMS_H
#define _CONSOLE_SMS_H

CALLBACK void* console_sms_send(ProtDevice *dev, int press, void *null);
CALLBACK void* console_sms_read(ProtDevice *dev, int press, void *null);
CALLBACK void* console_sms_credit(ProtDevice *dev, int press, void *null);

#endif
