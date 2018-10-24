#ifndef _SAET_FINGER_H
#define _SAET_FINGER_H

typedef struct {
  unsigned char finger[10];
  unsigned char fdata[0];
} tebe_finger;

extern tebe_finger **finger;

void finger_init();
void finger_send(int mm, int id);
int finger_next_valid(int id);

int finger_store(unsigned char *pkt);

void finger_save();
void finger_delete(int id);

#define FINGER_BLK_DIM	26

#endif
