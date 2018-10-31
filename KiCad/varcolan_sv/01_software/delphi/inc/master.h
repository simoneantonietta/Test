#ifndef _SAET_MASTER_H
#define _SAET_MASTER_H

#include "database.h"

#define MM_NUM	MAX_NUM_MM

#define GEST16MM

#define  SS 0
#define SC1 1
#define SC8 2
#define ATt 4
#define  QR 5
#define TAS 6
#define SEN 7
#define TS2 10
#define SC8R2 11
#define SC8C 12

#define SSA 0
#define SST 1
#define SA8 2
#define ATA 3
#define TST 4
#define TSK 5
#define TLK 6
#define TLB 7
#define SAN 8

#define MASTER_NORMAL	0
#define MASTER_ACTIVE	1
#define SLAVE_STANDBY	2
#define SLAVE_ACTIVE	3
#define MASTER_STANDBY	4

#define SC8R_1_IND	2
#define SC8R_2_IND	7

extern unsigned char MMPresente[MM_NUM];
extern unsigned char MMSuspend[MM_NUM];
extern unsigned char MMSuspendDelay[MM_NUM];
extern unsigned char TipoPeriferica[MM_NUM*32];
extern unsigned char StatoPeriferica[MM_NUM*32/8];
extern unsigned char MMTexas[MM_NUM];
extern int master_switch[MM_NUM];

extern char SA8_all_alarm;

int master_init();
int master_stop();

void master_do();

extern int libuser_started;
extern int master_behaviour;
extern void (*master_ridon_sync)();

int master_queue(int numMM, unsigned char *cmd);
unsigned char master_slowdown();

void master_queue_sensor_event(int ind, unsigned char event);
void master_periph_restore(int basedev, unsigned char *ev);

int master_periph_present(int ind);
int master_periph_type(int ind);
int master_sensors_refresh();
int master_actuators_refresh();
int master_set_setting(int ind);
int master_settings_refresh();

int master_sensor_valid(int idx);
int master_actuator_valid(int idx);

void master_set_sabotage(int ind, unsigned char stat, unsigned char event);
void master_set_failure(int ind, unsigned char stat, unsigned char event);
void master_set_alarm(int ind, unsigned char status, char print);

void master_set_active();

void master_register_module(int module, int fd);

extern unsigned int master_SC8R2_uniqueid;
void master_SC8R2_init(int linea);
void master_radio868_accept_fronts(int conc);

#endif
