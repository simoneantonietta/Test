#ifndef _SAET_COMMAND_H
#define _SAET_COMMAND_H

#include "codec.h"

int cmd_get_day_type(int day);
int cmd_set_day_type(int day, unsigned char type);

int cmd_set_date(unsigned char *val);
int cmd_get_time();
int cmd_set_time(unsigned char *val);

int cmd_set_timerange(int idx, unsigned char *val);
int cmd_set_holydays(unsigned char *val);

//int cmd_send_mem(Event *ev, ProtDevice *dev);
int cmd_send_version(ProtDevice *dev);
int cmd_send_internal_info(ProtDevice *dev);

int cmd_zone_on(int zone, int no_check);
int cmd_zone_off(int zone, int no_check);
int cmd_zone_refresh(int onoff);

int cmd_periph_set(int idx, int status);
int cmd_update_tunings(unsigned char idx, unsigned char time, unsigned char sens);

int cmd_actuator_on(int idx);
int cmd_actuator_off(int idx);
int cmd_actuator_refresh();

int cmd_sensor_on(int idx);
int cmd_sensor_off(int idx);

//void cmd_modem_number_event(unsigned char event, char *val);
//int cmd_modem_number(unsigned char *val);
//int cmd_modem_send(int idx, char *val);
int cmd_modem_status(unsigned char *val, int retstatus);

int cmd_accept_actuators(int grp);
int cmd_accept_act_timers();
int cmd_accept_mem(int grp);
int cmd_accept_counters(int grp);
int cmd_accept_timers(int grp);
int cmd_accept_fronts();

int cmd_command_check_test(int s);
int cmd_command_send_noise(int idx);
int cmd_command_reset_noise(int idx);
int cmd_command_send_noise_all();
int cmd_command_reset_noise_all();
int cmd_command_start_test(int grp);
int cmd_command_end_test(int grp);

int cmd_keyboard_send_secret(int idx, unsigned char *secret);
int cmd_keyboard_send_key(int idx, unsigned char *key);
int cmd_keyboard_set_state(int idx, int cond, int timeout);
int cmd_keyboard_set_mode(int idx, int cond);
int cmd_keyboard_set_echo(int idx, int cond);
int cmd_display_message(int idx, int pos, char *msg);
int cmd_display_set_mode(int idx, int mode);
int cmd_display_set_cursor_pos(int idx, int pos);

int cmd_memory_control_init();
int cmd_memory_control();

int cmd_status_accept(short idx, int bit, char atype);

int cmd_request_analog(int ind);
int cmd_send_threshold(int ind, unsigned char ch, unsigned char *threshold);

int cmd_TPDGF_reset();
int cmd_TPDGF_add(unsigned char *day);
int cmd_TPDGV_reset();
int cmd_TPDGV_add(unsigned char *day);
int cmd_set_saturday_type(unsigned char type);

#endif
