#ifndef __VOLTO_H__
#define __VOLTO_H__

#include "lara.h"

#define VOLTO_IDFY_LARA	1
#define VOLTO_IDFY_TEBE	2
#define VOLTO_INSERT	3
#define VOLTO_DELETE	4
#define VOLTO_JOIN	5
#define VOLTO_ENROLL	6
#define VOLTO_ENR_CANC	7
//#define VOLTO_KEY_SET	8

typedef struct {
  int tipo;
  union {
    struct {
      int mm;
      int nodo;
      lara_t_id msg;
    } lara;
    struct {
    } tebe;
    struct {
      int id;
      int id2;
      int imp;
      int term;
    } anagrafica;
  };
} volto_param;

typedef struct {
  unsigned char seq;
  unsigned int time;
  unsigned char code;
  unsigned char data[24];
  unsigned short crc;
} __attribute__((packed)) volto_pkt;

void volto_init();
void volto_req(volto_param *req);

#endif /* __VOLTO_H__ */
