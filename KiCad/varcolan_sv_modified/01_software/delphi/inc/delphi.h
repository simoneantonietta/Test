#ifndef _DELPHI_H
#define _DELPHI_H

#if defined __i386__ || defined __x86_64__
char* addrootdir(const char *file);
#define ADDROOTDIR(x) addrootdir(x)
#else
#define ADDROOTDIR(x) x
#endif

extern const char UserFileName[];
extern const char NewUserFileName[];
extern const char ConfFileName[];
extern const char NVFileName[];
extern const char NVTmpFileName[];
extern const char ConsumerFileName[];
extern const char StringsFileName[];
extern const char RestartFileName[];
extern const char StopClockFile[];
extern const char VersionFile[];

extern const char ref_ConfFileName[];
extern const char ref_NVFileName[];
extern const char ref_StringsFileName[];
extern const char ref_ConsumerFileName[];
extern const char ref_SprintFileName[];

extern const char Boot2aFileName[];
extern const char Boot2bFileName[];

extern unsigned char Boot1Error[];
extern unsigned char Boot2Error[];

extern int Restart;

void delphi_init();
void delphi_set_timer(int sec);
void delphi_save_conf();
void delphi_check_timerange();
void delphi_update_days();
void delphi_check_periods();

//extern void (*timer_hook)(void);

#ifdef __CRIS__
void delphi_restart();
void delphi_restart_network();

/* Dalla versione 1.x (kernel 2.4.14) alla versione 2.x (kernel 2.4.19)
   sono stati modificati i codici ioctl per l'RTC. Questi sono quelli
   della versione 1.x per compatibilita' */
/* Dalla kernel 2.4.26 sono tornati quelli di prima, quindi è il kernel
   2.4.19 ad dover essere gestito come caso speciale. */
//#define RTC_RD_TIME_V1 0x80247009
//#define RTC_SET_TIME_V1 0x4024700a
#define RTC_RD_TIME_V1 0x80245401
#define RTC_SET_TIME_V1 0x40245402
#endif

extern int delphi_modo;
#define DELPHICENTRALE	0
#define DELPHITIPO	1

#define DELPHI_FORCE_NODEID	37

void delphi_set_lang(char *app, int num, char **str[]);

#endif
