#ifndef _SAET_STRINGS_H
#define _SAET_STRINGS_H

#include "database.h"

/* SYS/Delphi */
extern char *ZoneName[1 + n_ZS + n_ZI];
extern char *SensorName[n_SE];
extern char *ActuatorName[n_AT];
extern char *CommandName[n_ME];
extern char *TimingName[n_FAG];
extern char *UserName[256];
extern char *CodeName[sizeof(RONDA)/sizeof(short)];
extern char *RadioName[64];

/* LARA/Delphi */
extern char *ProfileName[256];
extern char *AreaName[128];
extern char *HolidayName[32];
extern char *WeekTimingName[64];
extern char *TerminalName[64];

extern char *StringDescNull;

void string_init();
void string_load();
void string_save();
void string_zone_name(int zone, char **cod, char **desc);
void string_sensor_name(int sens, char **cod, char **desc);
void string_actuator_name(int act, char **cod, char **desc);
void string_command_name(int cmd, char **cod, char **desc);
void string_code_name(int code, char **cod, char **desc);
void string_radio_name(int tlc, char **cod, char **desc);

void string_profile_name(int prof, char **cod, char **desc);
void string_area_name(int area, char **cod, char **desc);
void string_holiday_name(int hday, char **cod, char **desc);
void string_weektiming_name(int tmng, char **cod, char **desc);
void string_terminal_name(int term, char **cod, char **desc);

typedef void (*StringFunc)(int id, char **cod, char **desc);

extern char *str_d_0;
extern char *str_d_1;
extern char *str_d_2;
extern char *str_d_3;
extern char *str_d_4;
extern char *str_d_5;
extern char *str_d_6;
extern char *str_d_7;
extern char *str_d_8;
extern char *str_d_9;
extern char *str_d_10;
extern char *str_d_11;
extern char *str_d_12;
extern char *str_d_13;
extern char *str_d_14;
extern char *str_d_15;
extern char *str_d_16;

#endif
