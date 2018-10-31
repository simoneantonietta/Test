#ifndef _SAET_BADGE_H
#define _SAET_BADGE_H

int badge_request(int periph);
int badge_manage(int periph, unsigned int badge, unsigned char cmd);
int badge_acquire(int periph, unsigned int badge);
int badge_load(unsigned char *data);

#endif
