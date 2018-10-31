#ifndef _SAET_LARA_H
#define _SAET_LARA_H

#include <time.h>

extern const char LaraFile[];
extern const char LaraFileTmp[];

#define MM_LARA 5

#define NOOP	0
#define TIMING	1
#define ON_T	2
#define	OFF_T	3

#ifdef GZIP
#include <zlib.h>
#else
#include <stdio.h>
#define gzopen fopen
#define gzclose fclose
#define gzread(a,b,c) fread(b,1,c,a)
#define gzwrite(a,b,c) fwrite(b,1,c,a)
typedef FILE* gzFile;
#endif

int lara_init(int mm);
void lara_reload(int mm);
void lara_time(int mm);
void lara_timer();
void lara_parse(int mm, unsigned char *cmd);
void lara_actuators(int ind, unsigned char att, unsigned char mask);

void lara_substitute_timings();
int lara_update_day(struct tm *day, unsigned char *t);

int lara_get_id_data(int id, int *secret, unsigned char *badge, int *group, int *stato);
int lara_get_id_free();
int lara_load_badge(int id, unsigned char* badge, int secret, int profilo, int stato);

void lara_set_abil(int id, int abil);
void lara_set_area(int id, int area);
void lara_set_secret(int id, int secret, int term);
void lara_set_counter(int id, int counter);
void lara_set_profile(int id, int profile);
void lara_add_profile_sec(int id, int profile);
void lara_del_profile_sec(int id, int profile);

void lara_set_area_counters(int mm, int area1, int area2);
void lara_set_area_group(int id, int area, int mm, time_t timeset);

extern unsigned char lara_terminal_prog[64];
void lara_set_term_din(int term, int din);
void lara_set_term_counter(int term, int counter);
void lara_set_term_times(int term, int timeopen, int timeopentimeout, int tapbk);

void lara_conf_cmd(int cmd, unsigned char *data);

void lara_save(int now);

void lara_input(int mm, int node, int ingressi, int lock);
//void lara_reset_periph();

/* codes */
#define GENERIC			0
#define CONFIG			1
#define LARA_TIME		2
#define LARA_PC_RQ		3
#define LARA_DISPLAY		4
#define LARA_ATTUA		5
#define LARA_IN_ATT		6
#define LARA_RQ			7
#define LARA_STAT_ATT		8
#define LARA_ID			9
#define LARA_CMD		10
#define LARA_NODES		11

#define TEBE_OUT		14
#define TEBE_IN			15

#define TEBE_BACKUP		18
#define TEBE_SET_MASTER		19

/* LARA_RQ requests */
#define STATUS_ATT		0
#define NODES			1

/* op-codes */
#define READ_BADGE		1
#define CHANGE_INPUT		2
#define DOOR_TIMEOUT		3
#define FORZATURA		4
#define TRANS_OK		5
#define TIMBR_OK		6
#define C_TRANS_OK		7
#define C_TIMBR_OK		8

#define ID_RQ			9
#define BADGE_RQ		10
#define PC_RQ			11
#define ACC_BADGE		12
#define ACC_ID			13
#define ACC_SEGR		14
#define ACC_SERV		15
#define V_AREA			16
#define V_ORA			17
#define FASCIA_RQ		18
#define FEST_RQ			19
#define PROG_FASCE		20
#define PROG_FEST		21
#define SET_SEGR_BADGE		22
#define SET_SEGR_ID		23
#define ERR_V_SEGR		24
#define SET_N_SEGR		25
#define C_BADGE_L		26
#define C_BADGE_F		27
#define JOIN_ID_GROUP		28
#define ID_FREE_RQ		29
#define GROUP_RQ		30
#define LETT_RQ			31
#define JOIN_GROUP_TEL		32
#define JOIN_LET_TEL		33
#define JOIN_LET_AREE		34
#define JOIN_GROUP_LET		35
#define JOIN_GROUP_FAS		36
#define V_DATA			37
#define RES_G_RESP		38
#define C_MASK			39
#define JOIN_ALL_TEL		40
#define INST_RQ			41
#define SET_INST_ADDR		42
#define V_AREA_L		43
#define JOIN_LET_FILTER		44

#define BADGE_DIRECT_CODE	0x80

#define TIMBRATO		100
#define TRANS			101
#define C_TIMBRATO		102
#define C_TRANS			103
#define SEGR_RQ			104
#define SET_G_RESP		105
#define DATI_ID			106
#define PC_RESP			107
#define DATI_FASCIA		108
#define DATI_FESTIVI		109
#define DATI_GROUP		110
#define DATI_LETT		111
#define DATI_INST		112

// Badge non presente
#define NO_BADGE		150
// Badge già caricato
#define BADGE_EXIST		151
// ID fuori zona
#define NOT_IN_ZONE		152
// Lettore non permesso
#define ID_NO_JOIN_LETT		153
// ID fuori orario
#define ID_DISAB		154
// ID disabilitato
#define ID_ZONA_9		155
// ID occupato
#define ID_BUSY			156
// ID non valido
#define NO_ID			157
// Errore codice segreto
#define ERR_SEGR		158
// Segreto non programmato
#define NO_SEGR			159
// Memoria piena
#define MEM_FULL		160
// Errore interno
#define GENERIC_ERROR		161
// Transito con badge
#define BADGE_REQUIRED		162
// Solo telecomandi
#define SOLO_ATTUAZ		163

typedef unsigned char lara_tData;

typedef struct {
	lara_tData domain;
	lara_tData subnet;
	lara_tData node;
} __attribute__ ((packed)) lara_tAddress;

typedef struct {
	lara_tData		Code;
	lara_tAddress	Address;
	lara_tData		Length;
	lara_tData		Msg[255];
} __attribute__ ((packed)) lara_tMsg;

typedef struct {
	lara_tData		sec;
	lara_tData		minute;
	lara_tData		hour;
	lara_tData		day;
	lara_tData		month;
	lara_tData		year;
	lara_tData		dow;
} __attribute__ ((packed)) lara_tDate;

typedef struct {
	lara_tData:3;
	lara_tData	row:1;
	lara_tData	col:4;
	lara_tData	string[17];
} __attribute__ ((packed)) lara_tDisplay;

typedef struct {
	lara_tData	address;
	lara_tData	ingressi;
	lara_tData	stati;
	lara_tData	comandi[8];
    unsigned short	timers[8];	
} __attribute__ ((packed)) lara_tActuator;

#define LARA_MULTIPROFILI	14

typedef struct {
  unsigned char	badge[10];
  union {
    struct {
      unsigned char	supervisor:1;
      unsigned char	coerc:1;
      unsigned char	apbk:1;
      unsigned char	badge:2;
      unsigned char	abil:3;
    } s;
    unsigned char b;
  } stato;
  union {
    struct {
      unsigned char	profilo;
      unsigned char	term[8];
      unsigned char	fosett;
      unsigned char	fest[4];
    } s;
    unsigned char	profilo[LARA_MULTIPROFILI];
  } p;
  unsigned char	area;
  unsigned int	pincnt:24;
} __attribute__ ((packed)) lara_Tessere;

typedef struct {
  unsigned short	apbktime:10;
  unsigned short	apbkdir:1;
  unsigned short	ag:2;
  unsigned short	ce:2;
} r_pass_flag;

#define BADGE_VUOTO		0
#define BADGE_DISABIL		4
#define BADGE_ABIL		6
#define BADGE_CANC		7

#define BADGE_PROGR		1
#define SEGR_PROGR		2

#define NUM_BADGE_COLLETTIVI	4
#define SOGLIA_MIN_SOLO_ATT		80
#define SOGLIA_MAX_SOLO_ATT		91

#define TEBE_AREASELF		128

#define PROF_TIPO_BASE		0
#define PROF_TIPO_SCORTA	1
#define PROF_TIPO_VISIT_O	2
#define PROF_TIPO_VISIT_A	3
#define PROF_TIPO_LAST		PROF_TIPO_VISIT_A

typedef struct {
  unsigned char	term[8];
  unsigned char	fosett;
  unsigned char	fest[4];
  union {
    struct {
      unsigned char	coerc:1;
      unsigned char	apbk:1;
      unsigned char	tipo:4;
    } s;
    unsigned char b;
  } stato;
} lara_Profili;

typedef struct {
  union {
    struct {
      unsigned char	apbk:1;
      unsigned char	blkbadge:1;
      unsigned char	tipo:2;
      unsigned char	blktas:1;
      unsigned char	volto:1;
      unsigned char	ritforz:1;
      unsigned char	acc_ris:1;
    } s;
    unsigned char b;
  } stato;
  unsigned char	fosett;
  unsigned char	fest[4];
  unsigned char	timeopen;
  unsigned char	timeopentimeout;
  union {
    struct {
      unsigned char	filtro:4;
      unsigned char	tapbk:4;
    } s;
    unsigned char b;
  } conf;
  unsigned char	area[2];
  unsigned int	dincnt:24;
} __attribute__ ((packed)) lara_Terminali;

typedef struct {
  unsigned short	contatore;
} lara_Presenze;

typedef struct {
  unsigned char	fascia[7][2][4];
} lara_FOSett;

typedef struct {
  unsigned char	tipofestivo;
  struct {
    unsigned char	inizio[3];
    unsigned char	fine[3];
  } periodo;
} lara_Festivi;

#define LARA_N_FASCE		64
#define LARA_N_FESTIVI		32
#define LARA_N_TERMINALI	63
#define LARA_N_AREE		127
#define LARA_N_PROFILI		255

typedef struct {
  unsigned int		param;
  lara_Profili		profilo[LARA_N_PROFILI];
  lara_Terminali	terminale[LARA_N_TERMINALI+1];
  lara_Presenze		presenza[LARA_N_AREE];
  lara_FOSett		fasciaoraria[LARA_N_FASCE];
  lara_Festivi		festivo[LARA_N_FESTIVI];
  lara_Tessere		*tessera;
} lara_Config;

typedef struct {
  int	group;
  int	reserved[3];
} lara_TessereEx;

extern pthread_mutex_t laragz_mutex;
extern lara_Config *_lara;
extern lara_TessereEx *_laraex;
extern unsigned char _lara_ingressi[64];

extern int lara_NumBadge;

#define BADGE_LENGTH 19
//#define BADGE_NUM 32768
//#define BADGE_NUM 20000
//#define BADGE_NUM 8000
#define BADGE_NUM lara_NumBadge

typedef struct {
  unsigned char	profilo[LARA_N_PROFILI];
  unsigned char	terminale[LARA_N_TERMINALI+1];
  unsigned char	fasciaoraria[LARA_N_FASCE];
  unsigned char	festivo[LARA_N_FESTIVI];
  r_pass_flag *tessera;	// [BADGE_NUM]
  unsigned int *tessera_area_last_update; // [BADGE_NUM]
} lara_Flags;

extern lara_Flags *_laraf;

#define LARA_PARAM_COUNTER	(1L<<0)
#define LARA_PARAM_MULTIPROFILE	(1L<<1)
#define LARA_PARAM_BADGEDISABLE	(1L<<2)
#define LARA_PARAM_EXTRA_ID	(1L<<3)

extern unsigned short lara_giustificativo[LARA_N_TERMINALI+1];

typedef struct {
    unsigned char op_code;       // codice operativo: può essere un codice d'errore
    short service_code;
    struct {
        unsigned char area    : 4;
        unsigned char g_resp  : 4;
        unsigned char gruppo  : 7;
        unsigned char abil    : 1;
        unsigned char spare   : 4;
        unsigned char fessura : 4;
    } attrib;
    unsigned char ingressi;
    short id;
    short segreto;
    char badge[BADGE_LENGTH];
} __attribute__ ((packed)) lara_t_id;

typedef struct {
  unsigned char gruppo;
  unsigned char stato;
  unsigned char fasce;
  unsigned char lettori[12];
  unsigned char telecomandi[12];
} __attribute__ ((packed)) lara_t_gruppo;

typedef struct {
  unsigned char address;
  unsigned char stato;
  unsigned char alarm_addr;
  unsigned char area[2];
  unsigned char ingress;
  unsigned char telecomandi[12];
} __attribute__ ((packed)) lara_t_lettore;

typedef struct {
  unsigned char fissi;
  unsigned char mobili;
} __attribute__ ((packed)) lara_t_festivi;

typedef struct {
  unsigned char fascia;
  unsigned char ora_inizio;
  unsigned char min_inizio;
  unsigned char ora_fine;
  unsigned char min_fine;
} __attribute__ ((packed)) lara_t_fascia;

typedef struct {
  unsigned char sec;
  unsigned char min;
  unsigned char hour;
  unsigned char day;
  unsigned char mon;
  unsigned char year;
  unsigned char dow;
} __attribute__ ((packed)) lara_t_date;

typedef struct {
  unsigned char fig;
  unsigned char read_addr;
  unsigned char act_addr;
  unsigned char interface;
} __attribute__ ((packed)) lara_t_install;

typedef struct {
    unsigned char op_code;
    union {
      lara_t_gruppo group;
      lara_t_lettore lett;
      lara_t_festivi fest;
      lara_t_fascia fasce;
      lara_t_date data;
      lara_t_install inst;
    } tipo;
} __attribute__ ((packed)) lara_t_cmd;

typedef struct {
  unsigned short id;
  unsigned short profilo;
  time_t time;
} __attribute__ ((packed)) Lara_timbrato;

extern Lara_timbrato lara_timbrato[(LARA_N_TERMINALI+1)*2];

#endif
