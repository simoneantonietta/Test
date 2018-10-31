#include "strings.h"
#include "support.h"
#include "delphi.h"
#include "lara.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

char *ZoneName[1 + n_ZS + n_ZI];
char *SensorName[n_SE];
char *ActuatorName[n_AT];
char *CommandName[n_ME];
char *TimingName[n_FAG];
char *UserName[256];
char *CodeName[sizeof(RONDA)/sizeof(short)];
char *RadioName[64];

char *ProfileName[256];
char *AreaName[128];
char *HolidayName[32];
char *WeekTimingName[64];
char *TerminalName[64];

char *str_d_0 = "Carico CPU: %d%%";
char *str_d_1 = "Errore invio MMS";
char *str_d_2 = "Tot.";
char *str_d_3 = "(individuale)";
char *str_d_4 = "SENSORE %04d";
char *str_d_5 = "ATTUATORE %04d";
char *str_d_6 = "Cod. %04d";
char *str_d_7 = "PROFILO %03d";
char *str_d_8 = "AREA %03d";
char *str_d_9 = "FESTIVITA' N%02d";
char *str_d_10 = "FASCIA SETT. N%02d";
char *str_d_11 = "TERMINALE %02d";
char *str_d_12 = "Errore invio SMS";
char *str_d_13 = "Invio MMS";
char *str_d_14 = "Chiamata tipo %d al n.%d";
char *str_d_15 = "Messaggio audio num. %d al n.%d";
char *str_d_16 = "Invio SMS al n.%d";

char *StringDescNull = "-";

void string_init()
{
  memset(ZoneName, 0, sizeof(ZoneName));
  memset(SensorName, 0, sizeof(SensorName));
  memset(ActuatorName, 0, sizeof(ActuatorName));
  memset(CommandName, 0, sizeof(CommandName));
  memset(TimingName, 0, sizeof(TimingName));
  memset(UserName, 0, sizeof(UserName));
  memset(CodeName, 0, sizeof(CodeName));
  memset(RadioName, 0, sizeof(RadioName));

  memset(ProfileName, 0, sizeof(ProfileName));
  memset(AreaName, 0, sizeof(AreaName));
  memset(HolidayName, 0, sizeof(HolidayName));
  memset(WeekTimingName, 0, sizeof(WeekTimingName));
  memset(TerminalName, 0, sizeof(TerminalName));
}

static char* string_ascii(char *msg)
{
  char *out;
  
  out = malloc(strlen(msg)+1);
  utf8convert(msg, out);
  if(strlen(out) > 16) out[16] = '\0';
  return out;
}

void string_load()
{
  FILE *fp;
  char buf[64];
  int idx, i;
  
  fp = fopen(ADDROOTDIR(StringsFileName), "r");
  if(fp)
  {
    while(fgets(buf, 64, fp))
    {
      buf[63] = 0;
      /* remove ^M (<CR>) from DOS files */
      for(idx=strlen(buf)-1; (idx >= 0) && ((buf[idx] == '\r') || (buf[idx] == '\n') || (buf[idx] == '\t') || (buf[idx] == ' ')); idx--)
        buf[idx] = 0;
      idx = -1;
      
      switch(buf[0])
      {
        case '#':
        case '\r':
        case '\n':
        case 0:
          continue;
        case 'Z':
          sscanf(buf+1, "%d", &idx);
          if(idx < 0) continue;
          for(i=2; (buf[i]!='=') && (i<64); i++);
          if(i == 64) continue;
          ZoneName[idx] = string_ascii(buf + i + 1);
          break;
        case 'S':
          sscanf(buf+1, "%d", &idx);
          if(idx < 0) continue;
          for(i=2; (buf[i]!='=') && (i<64); i++);
          if(i == 64) continue;
          SensorName[idx] = string_ascii(buf + i + 1);
          break;
        case 'A':
          sscanf(buf+1, "%d", &idx);
          if(idx < 0) continue;
          for(i=2; (buf[i]!='=') && (i<64); i++);
          if(i == 64) continue;
          ActuatorName[idx] = string_ascii(buf + i + 1);
          break;
        case 'T':
          sscanf(buf+1, "%d", &idx);
          if(idx < 0) continue;
          for(i=2; (buf[i]!='=') && (i<64); i++);
          if(i == 64) continue;
          CommandName[idx] = string_ascii(buf + i + 1);
          break;
        case 'F':
          sscanf(buf+1, "%d", &idx);
          if(idx < 0) continue;
          for(i=2; (buf[i]!='=') && (i<64); i++);
          if(i == 64) continue;
          TimingName[idx] = string_ascii(buf + i + 1);
          break;
        case 'O':
          sscanf(buf+1, "%d", &idx);
          if(idx < 0) continue;
          for(i=2; (buf[i]!='=') && (i<64); i++);
          if(i == 64) continue;
          UserName[idx] = string_ascii(buf + i + 1);
          break;
        case 'C':
          sscanf(buf+1, "%d", &idx);
          if(idx < 0) continue;
          for(i=2; (buf[i]!='=') && (i<64); i++);
          if(i == 64) continue;
          CodeName[idx] = string_ascii(buf + i + 1);
          break;
        case 'R':
          sscanf(buf+1, "%d", &idx);
          if(idx < 0) continue;
          for(i=2; (buf[i]!='=') && (i<64); i++);
          if(i == 64) continue;
          RadioName[idx] = string_ascii(buf + i + 1);
          break;
        case 'L':
          switch(buf[1])
          {
            case 'P':
              sscanf(buf+2, "%d", &idx);
              if(idx < 0) continue;
              for(i=3; (buf[i]!='=') && (i<64); i++);
              if(i == 64) continue;
              ProfileName[idx] = string_ascii(buf + i + 1);
              break;
            case 'A':
              sscanf(buf+2, "%d", &idx);
              if(idx < 0) continue;
              for(i=3; (buf[i]!='=') && (i<64); i++);
              if(i == 64) continue;
              AreaName[idx] = string_ascii(buf + i + 1);
              break;
            case 'H':
              sscanf(buf+2, "%d", &idx);
              if(idx < 0) continue;
              for(i=3; (buf[i]!='=') && (i<64); i++);
              if(i == 64) continue;
              HolidayName[idx] = string_ascii(buf + i + 1);
              break;
            case 'W':
              sscanf(buf+2, "%d", &idx);
              if(idx < 0) continue;
              for(i=3; (buf[i]!='=') && (i<64); i++);
              if(i == 64) continue;
              WeekTimingName[idx] = string_ascii(buf + i + 1);
              break;
            case 'T':
              sscanf(buf+2, "%d", &idx);
              if(idx < 0) continue;
              for(i=3; (buf[i]!='=') && (i<64); i++);
              if(i == 64) continue;
              TerminalName[idx] = string_ascii(buf + i + 1);
              break;
            default:
              support_log("Errore di importazione stringhe");
          }
          break;
        default:
          support_log("Errore di importazione stringhe");
      }
    }
    fclose(fp);
  }

  /* Set default names */
  
  if(!ZoneName[0]) ZoneName[0] = strdup(str_d_2);
  for(i=1; i<=n_ZS; i++)
  {
    sprintf(buf, "S%03d", i);
    if(!ZoneName[i]) ZoneName[i] = strdup(buf);
  }
  for(i=0; i<n_ZI; i++)
  {
    sprintf(buf, "I %02d", i);
    if(!ZoneName[i + n_ZS + 1]) ZoneName[i + n_ZS + 1] = strdup(buf);
  }
  
  for(i=0; i<n_SE; i++)
  {
    sprintf(buf, "%04d", i);
    if(!SensorName[i]) SensorName[i] = strdup(buf);
  }
  
  for(i=0; i<n_AT; i++)
  {
    sprintf(buf, "%04d", i);
    if(!ActuatorName[i]) ActuatorName[i] = strdup(buf);
  }
  
  for(i=0; i<n_ME; i++)
  {
    sprintf(buf, "%04d", i);
    if(!CommandName[i]) CommandName[i] = strdup(buf);
  }
  
  for(i=0; i<n_FAG; i++)
  {
    sprintf(buf, "%03d", i);
    if(!TimingName[i]) TimingName[i] = strdup(buf);
  }
  
  for(i=0; i<256; i++)
  {
    sprintf(buf, "%03d", i);
    if(!UserName[i]) UserName[i] = strdup(buf);
  }
  
  if(!ProfileName[0]) ProfileName[0] = strdup(str_d_3);
  for(i=1; i<256; i++)
  {
    sprintf(buf, "%03d", i);
    if(!ProfileName[i]) ProfileName[i] = strdup(buf);
  }
  
  for(i=0; i<128; i++)
  {
    sprintf(buf, "%03d", i);
    if(!AreaName[i]) AreaName[i] = strdup(buf);
  }
  
  for(i=0; i<32; i++)
  {
    sprintf(buf, "%02d", i+1);
    if(!HolidayName[i]) HolidayName[i] = strdup(buf);
  }
  
  for(i=0; i<64; i++)
  {
    sprintf(buf, "%02d", i+1);
    if(!WeekTimingName[i]) WeekTimingName[i] = strdup(buf);
  }

  for(i=0; i<64; i++)
  {
    sprintf(buf, "%02d", i+1);
    if(!TerminalName[i]) TerminalName[i] = strdup(buf);
  }
}

void string_save()
{
  FILE *fp;
  int i;
  char buf[8], *cod;
  
  fp = fopen(ADDROOTDIR(StringsFileName), "w");

  for(i=0; i<(1+n_ZS+n_ZI); i++)
  {
    string_zone_name(i, &cod, NULL);
    if(strcmp(cod, ZoneName[i]))
      fprintf(fp, "Z%d=%s\n", i, ZoneName[i]);
  }
  for(i=0; i<n_SE; i++)
  {
    sprintf(buf, "%04d", i);
    if(strcmp(buf, SensorName[i]))
      fprintf(fp, "S%d=%s\n", i, SensorName[i]);
  }
  for(i=0; i<n_AT; i++)
  {
    sprintf(buf, "%04d", i);
    if(strcmp(buf, ActuatorName[i]))
      fprintf(fp, "A%d=%s\n", i, ActuatorName[i]);
  }
  for(i=0; i<n_ME; i++)
  {
    sprintf(buf, "%04d", i);
    if(strcmp(buf, CommandName[i]))
      fprintf(fp, "T%d=%s\n", i, CommandName[i]);
  }
  for(i=0; i<n_FAG; i++)
  {
    sprintf(buf, "%03d", i);
    if(strcmp(buf, TimingName[i]))
      fprintf(fp, "F%d=%s\n", i, TimingName[i]);
  }
  for(i=0; i<256; i++)
  {
    sprintf(buf, "%03d", i);
    if(strcmp(buf, UserName[i]))
      fprintf(fp, "U%d=%s\n", i, UserName[i]);
  }
  for(i=0; i<(sizeof(RONDA)/sizeof(short)); i++)
  {
    if(CodeName[i])
      fprintf(fp, "C%d=%s\n", i, CodeName[i]);
  }
  for(i=0; i<64; i++)
  {
    if(RadioName[i])
      fprintf(fp, "R%d=%s\n", i, RadioName[i]);
  }
  for(i=0; i<256; i++)
  {
    sprintf(buf, "%03d", i);
    if(strcmp(buf, ProfileName[i]))
      fprintf(fp, "LP%d=%s\n", i, ProfileName[i]);
  }
  for(i=0; i<128; i++)
  {
    sprintf(buf, "%03d", i);
    if(strcmp(buf, AreaName[i]))
      fprintf(fp, "LA%d=%s\n", i, AreaName[i]);
  }
  for(i=0; i<32; i++)
  {
    sprintf(buf, "%02d", i+1);
    if(strcmp(buf, HolidayName[i]))
      fprintf(fp, "LH%d=%s\n", i, HolidayName[i]);
  }
  for(i=0; i<64; i++)
  {
    sprintf(buf, "%02d", i+1);
    if(strcmp(buf, WeekTimingName[i]))
      fprintf(fp, "LW%d=%s\n", i, WeekTimingName[i]);
  }
  for(i=0; i<64; i++)
  {
    sprintf(buf, "%02d", i+1);
    if(strcmp(buf, TerminalName[i]))
      fprintf(fp, "LT%d=%s\n", i, TerminalName[i]);
  }
  
  fclose(fp);
  sync();
}

void string_zone_name(int zone, char **cod, char **desc)
{
  static char buf[18];
  char *tmp;
  
  if(!cod) cod = &tmp;
  
  //if(cod)
  {
    if(!zone) *cod = str_d_2;
    if((zone >= 1) && (zone <= n_ZS))
    {
      sprintf(buf, "S%03d", zone);
      *cod = buf;
    }
    if(zone > n_ZS)
    {
      sprintf(buf, "I %02d", zone - n_ZS - 1);
      *cod = buf;
    }
  }
  
  if(desc)
  {
    if(strcmp(*cod, ZoneName[zone]))
      *desc = ZoneName[zone];
    else
      *desc = StringDescNull;
  }
}

void string_sensor_name(int sens, char **cod, char **desc)
{
  static char buf[18];
  
  if(desc)
  {
    sprintf(buf, "%04d", sens);
    if(strcmp(buf, SensorName[sens]))
      *desc = SensorName[sens];
    else
      *desc = StringDescNull;
  }

  sprintf(buf, str_d_4, sens);
  if(cod) *cod = buf;  
}

void string_actuator_name(int act, char **cod, char **desc)
{
  static char buf[18];
  
  if(desc)
  {
    sprintf(buf, "%04d", act);
    if(strcmp(buf, ActuatorName[act]))
      *desc = ActuatorName[act];
    else
      *desc = StringDescNull;
  }
  
  sprintf(buf, str_d_5, act);
  if(cod) *cod = buf;  
}

void string_command_name(int cmd, char **cod, char **desc)
{
  static char buf[18];
  
  if(desc)
  {
    sprintf(buf, "%04d", cmd);
    if(strcmp(buf, CommandName[cmd]))
      *desc = CommandName[cmd];
    else
      *desc = StringDescNull;
  }
  
  sprintf(buf, "TC %04d", cmd);
  if(cod) *cod = buf;  
}

void string_code_name(int code, char **cod, char **desc)
{
  static char buf[18];
  
  if(desc)
  {
    if(CodeName[code])
      *desc = CodeName[code];
    else
      *desc = StringDescNull;
  }
  
  sprintf(buf, str_d_6, code);
  if(cod) *cod = buf;  
}

void string_radio_name(int tlc, char **cod, char **desc)
{
  if(desc)
  {
    if(RadioName[tlc])
      *desc = RadioName[tlc];
    else
      *desc = StringDescNull;
  }
  
  if(cod) *cod = StringDescNull;  
}

void string_profile_name(int prof, char **cod, char **desc)
{
  static char buf[18];
  
  if(desc)
  {
    sprintf(buf, "%03d", prof);
    if(strcmp(buf, ProfileName[prof]))
      *desc = ProfileName[prof];
    else
      *desc = StringDescNull;
  }
  
  sprintf(buf, str_d_7, prof);
  if(cod) *cod = buf;  
}

void string_area_name(int area, char **cod, char **desc)
{
  static char buf[18];
  
  if(desc)
  {
    sprintf(buf, "%03d", area);
    if((area < LARA_N_AREE) && strcmp(buf, AreaName[area]))
      *desc = AreaName[area];
    else
      *desc = StringDescNull;
  }
  
  sprintf(buf, str_d_8, area);
  if(cod) *cod = buf;  
}

void string_holiday_name(int hday, char **cod, char **desc)
{
  static char buf[18];
  
  if(desc)
  {
    sprintf(buf, "%02d", hday+1);
    if(strcmp(buf, HolidayName[hday]))
      *desc = HolidayName[hday];
    else
      *desc = StringDescNull;
  }
  
  sprintf(buf, str_d_9, hday+1);
  if(cod) *cod = buf;  
}

void string_weektiming_name(int tmng, char **cod, char **desc)
{
  static char buf[18];
  
  if(desc)
  {
    sprintf(buf, "%02d", tmng+1);
    if(strcmp(buf, WeekTimingName[tmng]))
      *desc = WeekTimingName[tmng];
    else
      *desc = StringDescNull;
  }
  
  sprintf(buf, str_d_10, tmng+1);
  if(cod) *cod = buf;  
}

void string_terminal_name(int term, char **cod, char **desc)
{
  static char buf[18];
/*  
  if(term > 63);
  {
    *cod = "";
    *desc = "";
    return;
  }
*/  
  if(desc)
  {
    sprintf(buf, "%02d", term+1);
    if(strcmp(buf, TerminalName[term]))
      *desc = TerminalName[term];
    else
      *desc = StringDescNull;
  }
  
  sprintf(buf, str_d_11, term+1);
  if(cod) *cod = buf;  
}

