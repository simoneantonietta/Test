#ifndef _SAET_TEBE_H
#define _SAET_TEBE_H

#include "lara.h"

typedef struct {
  unsigned char code;
  unsigned char msg[32];
} tebe_msg;

extern int tebe_term_alive[LARA_N_TERMINALI];
#define KEEPALIVE_TIMEOUT 150

/* Codici per il campo 'code' per i messaggi BASE */
enum {CODE_SET_NODE, CODE_DATETIME, CODE_INPUT_REQ, CODE_DATETIME_ETH, CODE_VERSION = 27};

/* Codici per il campo 'code' per i messaggi LON o ETH */
enum {CODE_NULL = 0, CODE_BADGE_REQ, CODE_ID_REQ,
      CODE_BADGE_1, CODE_BADGE_2, CODE_BADGE_INV, CODE_ID_INV,
      CODE_TERM_REQ, CODE_TERM, CODE_PROF_REQ, CODE_PROF,
      CODE_FOS_REQ, CODE_FOS_1, CODE_FOS_2, CODE_FEST_REQ, CODE_FEST,
      CODE_RESULT, CODE_CREATE_BADGE_REQ, CODE_CHANGE_PIN_REQ,
      CODE_CHANGE_DINCONT_REQ, CODE_CHANGE_BLKBADGE_REQ,
      CODE_CHANGE_TAPBK_REQ, CODE_CHANGE_TAP_REQ, CODE_CHANGE_ONRELE_REQ, 
      CODE_PARAM_REQ, CODE_PARAM, CODE_CREATE_PIN_REQ,
      CODE_FINGER_REQ, CODE_FINGER, CODE_FINGER_ACK,
      CODE_INV_PASS = 80, CODE_INV_PASS_OK, CODE_INV_PROF, CODE_INV_PROF_OK,
      CODE_INV_TERM, CODE_INV_TERM_OK, CODE_INV_FOS, CODE_INV_FOS_OK,
      CODE_INV_FEST, CODE_INV_FEST_OK,
      CODE_EVENT = 100, CODE_EVENT_OK, CODE_EVENT_DELAY,
      CODE_STATUS, CODE_STATUS_OK,
      CODE_SENSORS, CODE_SENSORS_OK, CODE_ACTUATORS, CODE_ACTUATORS_OK,
      CODE_TERM_PROGRAM, CODE_TERM_PROGRAM_OK, CODE_KEEPALIVE, CODE_STOP,
      CODE_ESCORT, CODE_ESCORT_OK, CODE_BLKBADGE};

enum {RESULT_CREATE_BADGE, RESULT_CHANGE_PIN,
      RESULT_CHANGE_DINCONT, RESULT_CHANGE_BLKBADGE,
      RESULT_CHANGE_TAPBK, RESULT_CHANGE_TAP, RESULT_CHANGE_ONRELE,
      RESULT_EMERGENCY, RESULT_DATA_REQUEST, RESULT_CREATE_PIN};

void tebe_timer();
void tebe_parse(int mm, int node, tebe_msg *msg);

void tebe_init(int mm);
void tebe_stop();
void tebe_ripristino(int ind);
void tebe_manomissione(int ind);
void tebe_sensor_on(int idx);
void tebe_sensor_off(int idx);
int tebe_actuators(int mm, int nodo, unsigned char att);
void tebe_actuator_on(int idx);
void tebe_actuator_off(int idx);

void tebe_send_to(int mm, int node, unsigned char *msg);
void tebe_send_mm(int mm, unsigned char *msg);
void tebe_send_lan(unsigned char *msg, int len);
void tebe_send_finger(int id);

void tebe_result(unsigned char *res);
void tebe_emergency(int term, int emergenza);

void tebe_escort_link(int vid, int eid, int save);
#define TEBE_ESCORT_UNLINK  0xffff

#endif
