#ifndef _SAET_DATABASE_H
#define _SAET_DATABASE_H

#include "event.h"
#include "protocol.h"

#define MAX_NUM_MM	16

#define n_SE	(MAX_NUM_MM * 256)

extern unsigned int SE[n_SE];
extern unsigned char Zona_SE[n_SE];	// NV
extern unsigned char SEmu[n_SE];
extern unsigned char SEinvAlm[n_SE/8];	// NV
extern unsigned char SEanalog[n_SE];

extern time_t SE_time_alarm[n_SE];
extern time_t SE_time_failure[n_SE];
extern time_t SE_time_sabotage[n_SE];

extern char DVSSAM[2];			// NV
extern char DVSSAG[2];			// NV

#define n_DEFPT	(MAX_NUM_MM * 32)

extern char DEFPT[n_DEFPT][5];		// NV

#define n_TDTA	32

extern unsigned short *TDTA[n_TDTA];	// NV

#define n_SA	(MAX_NUM_MM * 32)

extern unsigned char SA[n_SA];
extern char CONF_SA;			// NV
extern char CNFG_SA[n_SA][4];		// NV

extern char SANanalogica[n_SA];		// NV

typedef struct _SogliaList {
  short	sens;
  unsigned char	so[6];
  struct _SogliaList *next;
} SogliaList;

extern SogliaList *SogliaSingola;

#define n_AT	n_SE

extern volatile unsigned int AT[n_AT];
extern unsigned short *TPDAT;		// NV

#define n_ME	2048

extern unsigned char ME[n_ME];
extern unsigned char ME2[n_ME];

#define n_MI	256

extern int MI[n_MI];

#define n_CO	256

extern unsigned char CO[n_CO];
extern unsigned short TCO[n_CO];

#define n_TM	256

extern unsigned char TM[n_TM];
extern unsigned int TTM[n_TM];

#define n_ZS	215
#define n_ZI	36

extern unsigned char ZONA[1 + n_ZS + n_ZI];
extern unsigned char ZONAEX[1 + n_ZS + n_ZI];
extern short *SE_ZONA[1 + n_ZS + n_ZI];

#define ZT	ZONA[0]
#define ZS	ZONA
#define ZI	(&(ZONA[n_ZS+1]))

extern unsigned char *TPDZT;		// NV
extern unsigned char *TPDZI[n_ZI];	// NV

extern unsigned char MIN_ZABIL;		// NV
extern unsigned char MAX_ZABIL;		// NV

extern unsigned short ZONA_R[1 + n_ZS];	// NV
extern unsigned short ZONA_R_timer[1 + n_ZS];
extern unsigned char ZONA_R_mode[1 + n_ZS];

#define n_FAS	7

//extern unsigned char FAS[n_FAS];
extern unsigned char *FASR[n_FAS];	// NV

#define n_FAG	255

extern unsigned char FAG[n_FAG];
extern unsigned char FAGR[n_FAG][4];	// NV

#define n_FESTIVI	32

#define DAY_WORKDAY	0
#define DAY_SEMIHOLIDAY	1
#define DAY_HOLIDAY	2

extern char SatHoliday;			// NV

extern unsigned char FESTIVI[n_FESTIVI];
extern unsigned char *TPDGF;		// NV
extern unsigned char *TPDGV;		// NV
extern unsigned char TIPO_GIORNO;
extern unsigned char SECONDI;
extern unsigned char MINUTI;
extern unsigned char ORE;
extern unsigned char GIORNO_S;
extern unsigned char GIORNO;
extern unsigned char MESE;
extern unsigned char ANNO;
extern unsigned char old_ORE;
extern unsigned char old_GIORNO;
extern unsigned char old_MESE;
extern unsigned char old_ANNO;
extern unsigned char N_MINUTO;
extern unsigned char N_ORA;
extern unsigned char N_GIORNO;
extern unsigned char N_MESE;
extern unsigned char N_ANNO;

extern short RICHIESTA_ERRATA_AT;
extern short CHIAVE_TRASMESSA;
extern short CHIAVE_FALSA;
extern unsigned char CHIAVE[5];
extern unsigned char M_CHIAVE[32][5];
struct v_analog_t {int ind; unsigned char vanl[8];};
extern struct v_analog_t V_ANALOG;
struct v_oradata_t {char ind, sec, min, ore, gis, gio, mes, ann;};
extern struct v_oradata_t V_ORADATA;
extern char LS_ABILITATA[MAX_NUM_CONSUMER];
extern unsigned char STATO_MODEM;
extern unsigned char NO_ATTI_ZONA;
extern unsigned char RILEVATO_MM;
extern unsigned short M_CONTENITORE;
extern unsigned short M_LINEA;
extern unsigned short GUASTO;
extern unsigned char SOSPESA_LINEA;
extern unsigned char RIPRISTINO_LINEA;

extern unsigned char RONDA[128][16];
//extern unsigned char ORA_RONDA[6][24];
//extern unsigned char RONDAext[176];
extern unsigned int QRA1;
extern unsigned int QRA2;
extern unsigned int TIPO_RONDA;

extern unsigned char PROVA;
extern unsigned char TEST_ATTIVO;
extern unsigned short GUASTO_NOTEST;

extern unsigned char EndListEnable;

#define n_SEGRETO	10

extern unsigned short SEGRETO[n_SEGRETO];

#define n_EVENTO	10

extern short EVENTO[n_EVENTO];

extern unsigned char PrintEvent[MAX_NUM_CONSUMER][MAX_NUM_EVENT];
extern unsigned char PrintEventEx[MAX_NUM_CONSUMER][MAX_NUM_EVENT_EX];
extern unsigned char PrintEventEx2[2][MAX_NUM_CONSUMER][MAX_NUM_EVENT_EX2];

extern int EventNum;
extern int EventNumEx;
extern int EventNumEx2;

extern unsigned char AlarmSabotage[MAX_NUM_MM * 32];
extern unsigned char AlarmSabotageInv[MAX_NUM_MM * 32];

extern unsigned char Barcode[MAX_NUM_MM * 32][10];

typedef struct
{
  unsigned char mesei:4;
  unsigned char tipo:4;
  unsigned char giornoi;
  unsigned char mesef;
  unsigned char giornof;
} Periodo;
extern Periodo periodo[32];

/**********************************/

/**** SE ****/
#define bitAlarm	(1<<0)
#define bitLHF		(1<<1)
#define bitHLF		(1<<2)
#define bitSabotage	(1<<3)
#define bitFailure	(1<<4)
#define bitTest		(1<<5)
#define bitOOSAZ	(1<<6)
#define bitOOS		(1<<7)

#define bitTimeout	(1<<5)

#define bitLineSabotage	(1<<8)
#define bitLineLHF	(1<<9)
#define bitLineHLF	(1<<10)

#define bitPackSabotage	(1<<8)
#define bitPackLHF	(1<<9)
#define bitPackHLF	(1<<10)

#define bitGenFailure	(1<<8)
#define bitGenLHF	(1<<9)
#define bitGenHLF	(1<<10)

#define bitInputSabotage	(1<<11)
#define bitInputSabotageLHF	(1<<12)
#define bitInputSabotageHLF	(1<<13)

#define bitTestFailure		(1<<16)
#define bitTestFailureLHF	(1<<17)
#define bitTestFailureHLF	(1<<18)

#define bitInputFailure		(1<<19)
#define bitInputFailureLHF	(1<<20)
#define bitInputFailureHLF	(1<<21)

#define bitNoAlarm	(1<<31)

/*** ZONA ***/
#define bitStatusInactive	(1<<5)
#define bitLockActive		(1<<6)
#define bitActive		(1<<7)

/*** ZONAEX ***/
#define bitMUAlarmZone	(1<<0)
#define bitLockDisactive	(1<<1)

/**** AT ****/
#define bitON		(1<<0)
#define bitNoise	(1<<3)
#define bitAbil		(1<<4)
#define bitAbilReq	(1<<5)
#define bitFlashing	(1<<6)
#define bitTemp		(1<<8)

/*** CO, TM, TL ***/
#define bitTiming	(1<<5)

/**** MU ****/
#define bitMUAlarm	(1L<<24)
#define bitMUSabotage	(1L<<14)
#define bitMUFailure	(1L<<22)
#define bitMUNoAlarm	(1L<<25)
#define bitMUNoSabotage	(1L<<15)
#define bitMUNoFailure	(1L<<23)
#define bitMUCounter	(1L<<26)

#define bitGestSabotage	(1L<<28)
#define bitGestFailure	(1L<<29)
#define bitGestAlarm	(1L<<30)

#define bitBattery	(1L<<27)

#define MU_alarm	1
#define MU_sabotage	2
#define MU_failure	3

#define MU_ini	1
#define MU_acc	3
#define MU_nac	4
#define MU_3ac	5

/*** SEmu ***/
#define bitSEmuAlarm	(1<<0)
#define bitSEmuPackSab	(1<<1)
#define bitSEmuLineSab	(1<<2)
#define bitSEmuInSab	(1<<3)
#define bitSEmuGenFail	(1<<4)
#define bitSEmuInFail	(1<<5)
#define bitSEmuTest	(1<<7)

/*** SEanalog ***/
#define bitAnalog	(1<<7)

/*** Sync ***/
#define bitSyncAlarm		(1<<0)
#define bitSyncSabotage		(1<<1)
#define bitSyncFailure		(1<<2)
#define bitSyncMuAlarm		(1<<3)
#define bitSyncMuSabotage	(1<<4)
#define bitSyncMuFailure	(1<<5)
#define bitSyncOOS		(1<<7)

/**********************************/

extern int database_changed;

void database_save();
int database_load();
void database_calc_zones();

void inline database_lock();
void inline database_unlock();
void database_set_alarm(unsigned int *sens);
void database_set_alarm2(unsigned char *sens);
void database_reset_alarm(unsigned int *sens);
void database_reset_alarm2(unsigned char *sens);
int database_testandset_MU(int index, unsigned int bit);

#endif
