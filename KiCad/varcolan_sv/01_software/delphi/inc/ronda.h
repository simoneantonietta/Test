#ifndef _SAET_RONDA_H
#define _SAET_RONDA_H

#define RONDA_NUM	10
#define RONDA_STAZIONI	32
#define RONDA_ORARI	32

#define RONDA_CHIUSA	0
#define RONDA_CHIAMATA	1
#define RONDA_IN_CORSO	2
#define RONDA_CONCLUSA	3

#define RONDA_TEMPO_MIN	600

#define RONDA_LED_CHIAMATA	0
#define RONDA_LED_TEMPO_OK	1

#define RONDA_EVENTO_NONE		 -1
#define RONDA_EVENTO_FINE_GIRO		  0
#define RONDA_EVENTO_INIZIO_GIRO	  1
#define RONDA_EVENTO_MANCATA_PARTENZA	  2
#define RONDA_EVENTO_RICHIESTA_GIRO	100
#define RONDA_EVENTO_PUNZONATURA	200
#define RONDA_EVENTO_MANCATA_PUNZON	300
#define RONDA_EVENTO_RIP_MANC_PUNZON	400
#define RONDA_EVENTO_FUORI_TEMPO	500
#define RONDA_EVENTO_FUORI_SEQUENZA	600
#define RONDA_EVENTO_CHIAVE_FALSA	700
#define RONDA_EVENTO_SOS		800

//#define RONDA_CHECK_CHIAVE

typedef struct {
  unsigned char zona;
  unsigned short tempo;
} Ronda_disattivazione_t;

typedef struct {
  unsigned char zona;
} Ronda_attivazione_t;

typedef struct {
  unsigned char stazione;
  unsigned short tempo;
  Ronda_disattivazione_t disattivazione[4];
  Ronda_attivazione_t attivazione[4];
} Ronda_percorso_t;

typedef struct {
  unsigned char ora;
  unsigned char minuti;
  unsigned char percorso;
} Ronda_orario_t;

extern void (*ronda_punzonatura_p)(int codice, int periferica);
extern int (*ronda_sos_p)(int allarme, int periferica);
extern int (*ronda_chiave_falsa_p)(int periferica);
extern int (*ronda_partenza_p)(int percorso);
extern int (*ronda_partenza_manuale_p)(int percorso);
extern int (*ronda_chiudi_p)(int percorso);
extern void (*ronda_save_p)();

extern Ronda_percorso_t *Ronda_percorso_p;
extern Ronda_orario_t *Ronda_orario_p;
extern unsigned short *Ronda_stazione_p;
extern int *Ronda_zonafiltro_p;

#endif
