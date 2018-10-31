#ifndef _CEI_H
#define _CEI_H

#define CEI_TIMEOUT	20

#define CEI_L2_FLAG_NAK	0x80
#define CEI_L2_FLAG_Q	0x40
#define CEI_L2_FLAG_DIR	0x20

#define CEI_L4_FLAG_S	0x80
#define CEI_L4_FLAG_LP	0x60
#define CEI_L4_FLAG_R	0x02
#define CEI_L4_FLAG_TA	0x01

#define CEI_CSC	0
#define CEI_CA	CEI_L2_FLAG_DIR

#define CEI_CRC2	0
#define CEI_CRC16	1

#define CEI_PROT_NONE	0
#define CEI_PROT_AUTH	1
#define CEI_PROT_FEAL	2
#define CEI_PROT_DES	3

#define CEI_SENDREPLY	1
#define CEI_WAITREPLY	2
#define CEI_SYNCHRO	4

typedef struct _cei_event_list {
  unsigned char *event;
  int len;
  struct _cei_event_list *next;
} cei_event_list;

#define   ENCIPHER_8  1
#define   DECIPHER_8  0

typedef struct feal_data {
  short r;                /* Numero iterazioni                 */
  unsigned char *kp;      /* Chiave di preambolo (8 byte)      */
  unsigned char ki[32];      /* Chiavi intermedie (2 * Num Iter.) */
  unsigned char *ke;      /* Chiave di epilogo (8 byte)        */
} feal_t;

typedef struct
{
  unsigned char S;
  unsigned char R;
  unsigned char outoforder;
  unsigned char flag;
  unsigned char Q;
  unsigned char Qout;
  unsigned char dir;
  unsigned char L2[65535];
  unsigned int L4_len;
  unsigned char *L4;
  unsigned char *L2out;
  unsigned int L2out_len;
  unsigned char L4_pktnum[4];
  unsigned char option;
  unsigned char lista;
  unsigned char *lastpkt;
  int lastlen;
  cei_event_list *evlist;
  unsigned char idR1[8];
  unsigned char idR2[8];
  unsigned char sk[8], skt[8];
  feal_t key[6];
  int skvalid;
  int identvalid;
} CEI_data;

#define BASESECONDS	631148400

#define FEAL_MK		0
#define FEAL_SK		1
#define FEAL_SKT	2
#define FEAL_MK_VAL	"01234567"
#define FEAL_ITER	8

#define ALM_SHIFT	2

#define ALM_INIZIO	1
#define ALM_FINE	0
#define ALM_PERSIST	2

#define ALM_PREALLARME		 1
#define ALM_ALLARME		 2
#define ALM_MANOMISSIONE	 3
#define ALM_GUASTO		 4
#define ALM_INTERR_LINEA	 8
#define ALM_STANDBY		 9
#define ALM_DISABILITATO	10
#define ALM_AUTOESCLUSO		11
#define ALM_DISINSERITO		12
#define ALM_TEST_MANUALE	13
#define ALM_INS_ANTICIPATO	14
#define ALM_STRAORDINARIO	15
#define ALM_TEST_NEGATIVO	16
#define ALM_MANCARETE		18
#define ALM_BATT_SCOLLEGATA	19
#define ALM_ALTERAZIONE		22
#define ALM_ATTIVAZIONE		23
#define ALM_GUASTO_IF		24
#define ALM_OROLOGIO_FUORI_SYN	29
#define ALM_ERRORE_PROGR	31
#define ALM_TEST_POSITIVO	32
#define ALM_STATO_RIVELATORE	34
#define ALM_MANUTENZIONE	36
#define ALM_ACCESSO_OPER	38
#define ALM_CONFIGURAZIONE	40
/* Estensioni non CEI */
#define ALM_CHIAVE		56
#define ALM_CODICE		59
#define ALM_TEST_ATTIVO		60
//#define ALM_ACC_ALLARME		61
//#define ALM_ACC_MANOMISSIONE	62
//#define ALM_ACC_GUASTO		63
#define ALM_ACC_ALLARME		2
#define ALM_ACC_MANOMISSIONE	3
#define ALM_ACC_GUASTO		4

/* Fuori mappatura CEI, viene trasformato nel codice corretto */
#define ALM_STATO		64

#define CLS_NONE		 0
#define CLS_UNITA_CENTRALE	 1
#define CLS_UNITA_SATELLITE	 2
#define CLS_ORGANO_COMANDO	 3
#define CLS_ATTUATORE		12
#define CLS_ZONA		 9
#define CLS_SENSORE		10
#define CLS_AGGREGAZIONE	11
#define CLS_RAGGRUPPAMENTO	13
#define CLS_OPERATORE		15

/* --- RONDA --- */

#define ALM_RONDA		 0	// codice dummy

#define ALM_R_GIRO		56
#define ALM_R_PARTENZA		57
#define ALM_R_CHIAVE_FALSA	58
#define ALM_R_FUORI_TEMPO	59
#define ALM_R_SEQUENZA		60
#define ALM_R_PUNZONATURA	61
#define ALM_R_MANCATA_PUNZ	62
#define ALM_R_RIP_MANC_PUNZ	63
#define ALM_R_SOS_RONDA		 2

/* ------------- */

typedef struct
{
  unsigned char L4header[8];
  unsigned char funzione;
  unsigned char dati[0];
} CEI_trasporto;

typedef struct
{
  unsigned char L4header[8];
  unsigned char versione:3;
  unsigned char formato:5;
  unsigned char processo:4;
  unsigned char ept:4;
  unsigned char tipoinfo;
  unsigned char numeromessaggio;
} CEI_header_ex;

typedef struct
{
  CEI_header_ex header;
  unsigned char dati[0];
} CEI_generico;

typedef struct
{
  CEI_header_ex header;
  unsigned int tempoaccadimento;
  unsigned char tipoevento:6;
  unsigned char transizione:2;
  unsigned char tiporivelatore:4;
  unsigned char areaapplicativa:4;
  unsigned char areaprovenienza:7;
  unsigned char categoria:1;
  unsigned char numeroelementomsb:4;
  unsigned char classeelemento:4;
  unsigned char numeroelementolsb;
} CEI_allarme;

typedef struct
{
  CEI_header_ex header;
  unsigned int tempoaccadimento;
} CEI_stato_header;

typedef struct
{
  unsigned char tipoevento:6;
  unsigned char transizione:2;
  unsigned char numeroelementomsb:4;
  unsigned char classeelemento:4;
  unsigned char numeroelementolsb;
} CEI_stato;

typedef struct
{
  CEI_header_ex header;
  unsigned char comando;
  unsigned char numeroelementomsb:4;
  unsigned char classeelemento:4;
  unsigned char numeroelementolsb;
} CEI_telecomando;

typedef struct
{
  CEI_header_ex header;
  unsigned char comando;
  unsigned char numeroelementomsb:4;
  unsigned char classeelemento:4;
  unsigned char numeroelementolsb;
  unsigned char valoremsb;
  unsigned char valorelsb;
  unsigned char secondovaloremsb;
  unsigned char secondovalorelsb;
} CEI_telecomando_valori;

typedef struct
{
  CEI_header_ex header;
  unsigned int tempoaccadimento;
  unsigned char esito;
  unsigned char numeroorigine;
} CEI_risposta_telecomando;

typedef struct
{
  CEI_header_ex header;
  unsigned int tempoaccadimento;
  unsigned char codicecomando;
  unsigned char esito;
  unsigned char numeroelementomsb:4;
  unsigned char classeelemento:4;
  unsigned char numeroelementolsb;
} CEI_segnalazione_comandi_locali;

typedef struct
{
  CEI_header_ex header;
  unsigned char comando;
  unsigned char numeroelementomsb:4;
  unsigned char classeelemento:4;
  unsigned char numeroelementolsb;
} CEI_richiesta_allineamento;

typedef struct
{
  CEI_header_ex header;
  unsigned int tempo;
} CEI_trasmissione_dataora;

typedef struct
{
  unsigned char mese:4;
  unsigned char tipo:4;
  unsigned char giorno;
} CEI_festivita;

typedef struct
{
  CEI_header_ex header;
  CEI_festivita fest[16];
} CEI_trasmissione_festivita;

typedef struct
{
  unsigned char mesei:4;
  unsigned char tipo:4;
  unsigned char giornoi;
  unsigned char mesef;
  unsigned char giornof;
} CEI_festivita_blocchi;

typedef struct
{
  CEI_header_ex header;
  CEI_festivita_blocchi fest[32];
} CEI_trasmissione_festivita_blocchi;

typedef struct
{
  unsigned char ora_inizio;
  unsigned char minuti_inizio;
  unsigned char ora_fine;
  unsigned char minuti_fine;
} CEI_fascia;

typedef struct
{
  CEI_header_ex header;
  CEI_fascia fascia[7][4];
} CEI_trasmissione_fasce;

typedef struct
{
  CEI_header_ex header;
  CEI_fascia fascia[2];
} CEI_trasmissione_fasce_sblocco;

typedef struct
{
  CEI_header_ex header;
  CEI_fascia fascia[32];
} CEI_trasmissione_fasce_periodo;

typedef struct
{
  CEI_header_ex header;
  unsigned char info_richiesta;
} CEI_richiesta_parametri;

typedef struct
{
  CEI_header_ex header;
  unsigned char tipodati;
  unsigned char indice;
  unsigned char dati[0];
} CEI_parametri_configurazione;

#endif
