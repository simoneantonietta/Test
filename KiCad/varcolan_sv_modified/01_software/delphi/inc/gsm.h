#ifndef _SAET_GSM_H
#define _SAET_GSM_H

#include "protocol.h"

extern ProtDevice *gsm_device;
extern char gsm_serial[];

extern int gsm_is_gprs;
extern int gsm_operator;
extern int gsm_ring_as_voice;

struct gsm_op_data_t {
  char *code;
  char *credit;
  char *apn;
  char *mmsc;
  char *gateway;
  int retval;
};
extern struct gsm_op_data_t gsm_operator_data[];

void gsm_init(int type);
void gsm_line_status();
void gsm_queue_mms(char *mms, int len);
void gsm_gprs_connect(int (*func)(int,void*), char *ipaddr, int port, void *param);
void gsm_gprs_connect_ex(int (*func)(int,void*), char *ipaddr, int port, int type, void *param);

#define GPRS_INIT_NONE	0
#define GPRS_INIT_MMS	1
#define GPRS_INIT_TCP	2
#define GPRS_INIT_UDP	3

int GSM_OnOff(int control);
int GSM_Call(int call_type, int pbook_number);
int GSM_Message(int msg, int pbook_number);
int GSM_Call_terminate(int call_type);
int GSM_SMS_send_direct(char *number, char *message);
int GSM_SMS_send(int pbook_number, char *message);
int GSM_SMS_send_callback(int pbook_number, char *message, void (*func)(int,void*), void *data);
int GSM_SMS_s_c_n(char *n_to_store);
int GSM_Status_event(int event_index);
int GSM_Set_recovery(int onoff, int dim);
int GSM_SIM_Credit();
int GSM_Squillo();
int GSM_Squillo_sconosciuto();

void gsm_user_program_end_loop();

void gsm_cancella_sms(int idx);
void gsm_lista_sms(void);
void gsm_lista_leggi_sms(int idx);

extern volatile char GSM_Status[7];

/* Per compatibilità verso plugin che gestiscono il gsm in modo autonomo. */
void gsm_poweron(int fd);

#endif
