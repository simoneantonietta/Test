#ifndef __CONTACTID_H__
#define __CONTACTID_H__

#include "timeout.h"

#define CID_DELAY		200
extern timeout_t *CID_delay;

#define CID_TONE_SILENCE	16
#define CID_TONE_LEN		400
extern unsigned char *CID_dtmf_tone[17];

void CID_init(void);
int CID_decode_tone(int *x, int nfreq);
int CID_decode_tone_ulaw(unsigned char *data, int nfreq);
int CID_message(int cond, int num, int evento, int area, int pointid);
int CID_get_message(unsigned char *ev);
void CID_set_error(int error);
int CID_error(void);

void CID_transmit(int fd, int *msg, unsigned char *ev);

#endif /* __CONTACTID_H__ */

