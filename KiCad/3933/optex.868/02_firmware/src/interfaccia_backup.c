/*****************************************************************************
contatto.c
******************************************************************************
 MICROCONTROLLORE TEXAS MSP430G2433
******************************************************************************

Uso solo il modulo TA0 del timer A come timer a 10 ms, ovviamente attivo solo durante il ciclo di funzionamento


+++++++++++++++++ STATO DI AVANZAMENTO DEI LAVORI +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
const char versione[2] = { 0x05, 0x0ff};        //versione(lsb),versione(.sb), versione(msb), tipo,
modificate   locazioni in dati trasmessi da periferica a concentratore

versione ff07				//tx power di 17 db
versione ff08				//tx power di 20 db
			#ifdef ALTRE_MISURE             (misura rumore  e max minimo rssi rx)
versione ff09				//per prove; non funziona write parametri, togliere rssi_rx_max e rssi_rx_mix
			#endif
					 //con wdog abilitato, rallentamento comandi a radio; (prove vedi dopo confronto con copia)
	#ifndef ALTRE_MISURE
   		versione ff0a				//tx power di 20 db
		#ifdef POTENZA_VARIABILE
	   		versione ff0c				//tx potenza variabile
	   	#endif
	#else
			versione ff0b				//per prove; non funziona write parametri, togliere rssi_rx_max e rssi_rx_mix
			#ifdef POTENZA_VARIABILE
				versione ff0d			//per prove; non funziona write parametri, togliere rssi_rx_max e rssi_rx_mix e potenza variabile
			#endif
	#endif

	versione ff12,13,14,15: tolto baco esecuzione interrupt radio con radio off ;
							allarme se ingresso tapparella open per t > Tintegrazione ingressi

	versione ef12,13,14,15: VERSIONE BATTERIA 9 VOLT

	versione ff16,17,18,19: versione che gestisce finestra proibita (no trasmissione) e 50 kbs

	versione ff20,21,22,23: radio 50 kbits   con parametri scelti a indovinare
	versione ff24,25,x4,x5: radio 50 kbits   con parametri concordati con Silicon Lab
    26/03/2014
	versione ff26,27,x4,x5: radio 50 kbits   con parametri concordati con Silicon Lab

	24/07/2014
	versione 0101    prima versione per test. spostamento allarme "input_stato.b.ampolla" da allarme1 a allarme2

	01/10/2014
	versione 0102    prima versione per test. spostamento allarme "input_stato.b.ampolla" da allarme2 a allarme1 (LUCA GARINO)

	04/11/2014
	versione 0103    gestione securgross

	24/11/2014
	versione 0104    correzione gestione LED e manutenzione_stato_b

	27/11/2014	versione 0105    correzione gestione LED.

	07/01/2015	versione 0106    correzione parametri default e soglia batteria
	08/01/2015	versione 0107    correzione misura tensione batteria (sommata caduta diodo) e soglia batteria
	12/01/2015	versione 0108    cambiata soglia batteria scarica
	03/02/2015	versione 0109    aumentata caduta sul diodo batteria (versione 9V) di 0.2V
	07/04/2015	versione 0110    aggiunto stato iniziale per lettura ingressi e livello batteria prima di entrare in gestione normale (per evitare tamper, allarmi e vbatteria errati dopo reset).
	09/06/2015	versione 0111    per prove
	09/06/2015	versione 0112    modifiche varie per tamponare problema mancate ricezione risposte da concentratore
	15/06/2015	versione 0113    contatore reset chip radio, contatore mancate fine trasmissione da radio, pilotaggio radio in polling e non in interrupt,
 								 potenza radio al minimo in caso di reset radio o mancata fine trasmissione.
								 Potenza massima trasmissione portata da 20 a 17 dB.                    (COMMENTO AGGIUNTO IL 20/02/2015 in base a confronti fatti su v1.10 archiviata il 14/04/2015 e la v1.14 archiviata il 17/06/2015)
								 trasmissione parametri spare :		cnt_err_superif_resetradio	-> spare.i16[0] (.i8[0]-.i8[1])
					   												cnt_err_superif_no_tx_radio -> spare.i16[1] (.i8[2]-.i8[3])
																	ultima_pw_tx 				-> spare.i8[4]
																	0xff		 				-> spare.i8[5]
	15/06/2015	versione 0114    il bit di guasto viene resettato solamente dopo una ricezione ok e non anche dopo 4 tentativi di comunicazione errati (come in versione 113)
	02/01/2016	versione 0115    misura tensione batteria durante ricezione radio (nella versione precedente avveniva subito dopo la fine della trasmissione).
  								 Inizia a convertire dopo fine tx (come versione precedente) ma butta le prime 20 letture e conserva la 21^ (copo circa 2.4 msec).
	07/01/2016	versione 0116    misura tensione batteria durante ricezione radio; Inizia a convertire dopo fine tx (come versione precedente) ma butta le prime 96 letture e conserva la media delle ultime 4, (attesa cikrca 10/12 ms);
								 se durante questo tempo arriva la ricezione di una synkWord, interrompe l'esecuzione dopo 6 letture (dalla ricezione della SW) e conserva la media delle ultime 4 (converte durante la ricezione dei dati).
	19/05/2016	versione 0117    riportata potenza massima a +20db.
	03/10/2016	versione 0118    ponticello per selezionare ingresso allarme accelerometro con tappa (solo securgross).
	07/10/2016	versione 0119    abbassata soglia batteria scarica, per versione a 9 volt, a 6,5;

	*/

//#define GENERAPORTANTE        //genera portante su comando pulsante tamper. (per prove accecamento)

//#define SECURGROSS            //definita dal compilatore
//#define BATTERIA_9V           //definita dal compilatore

//#define  KBS100               //   100kbs

//#define ALTRE_MISURE          // (misura rumore  e max minimo rssi rx)

#define NUMERO_PARAMETRI_PERIFERICA 4


#define POTENZA_VARIABILE

#define ANTICIPO_FPROIBITA  (300000/305)       					  //inizio finestra inibizione trasmissione
#define ANTICIPO     		(ANTICIPO_FPROIBITA + (230000/305))   //anticipo accensione radio per ricezione sincrona step da 30.5 usec [29 msec]

#define DELTA_FPROIBITA    70000/USEC15625						  // durata finestra proibita

#include "si4432_v2.h"
#include <msp430g2433.h>
//#include <stdlib.h>
//#include <signal.h>

#define PACKED __attribute__((__packed__))

//#define  POTENZA_MAX  0X16                // 17 dbm     (0x10x0/0x17 -> 1,2,5,8,11,14,17,20dbm )
#define  POTENZA_MAX  0X17                  // 20 dbm     (0x10x0/0x17 -> 1,2,5,8,11,14,17,20dbm )
//#define  POTENZA_MIN  0x17                // per prove ala massima potenza 0X10             // 1 dbm      (0x10x0/0x17 -> 1,2,5,8,11,14,17,20dbm )
#define  POTENZA_MIN  0X10                  // 1 dbm      (0x10x0/0x17 -> 1,2,5,8,11,14,17,20dbm )
#define  SOGLIA_RSSI_MIN 78                 // in dbm * -1
#define  SOGLIA_RSSI_MAX 65



#define PERIF_TIPO_CONTATTO     0x00
#define PERIF_TIPO_CONTATTO_9V  0x04

//#define _INFOMEM __attribute__ ((section(".infomem")))
#define CANALE_BASE     0
#define CANALE_TARATURA 4

#define    BANDA_RD             	0x73        // reg 0x75; banda.     (freq inizio banda  =   ((n & 0x1f) + 24 ) * {10.000 * [1 +if( n & 0x20)]}KHz ; cioe step da 10/20MHz + 240/480 MHz
					                            // frequenza inizio banda = 860.000.000;  (0x13 + 24) * 20MHz;
                                                // step ->  frequenza:  (freq = freq inizio banda + (n / 64000) * {10 * [1 +if( n & 0x20)]}KHz; cioe step da 156,25/312.5 Hz

                                                // freq = freq inizio banda + (n*312,5))

//#define    FREQ_CARR_RX 			28640 		//       reg 0x75 0x76; frequenza  868.950.000


#define    FREQ_CARR_CHB 			26240       //       reg 0x75 0x76; frequenza  868.200.000
#define    FREQ_CARR_CH1 			26880       //       reg 0x75 0x76; frequenza  868.400.000
#define    FREQ_CARR_CH2 			27520       //       reg 0x75 0x76; frequenza  868.600.000
#define    FREQ_CARR_CH3 			28160       //       reg 0x75 0x76; frequenza  868.800.000

#define    FREQ_CARR_TARATURA		26560       //       reg 0x75 0x76; frequenza  868.300.000


#define    FREQ_CARR_650 			27680       //       reg 0x75 0x76; frequenza  868.650.000     per prove ABB


#define NIBBLE_RX_PREAMB   5

#define LUNGHEZZA_DATI_PERIFERICHE     20       //400 //156


//define per tipo_messaggio
#define M_POLLING_P         		0x00
#define M_POLLING_C         		0x01
#define M_WR_TARATURA       		0x71
#define M_WR_NUMERO_SERIE   		0x72
#define M_WR_PARAMETRI      		0x73
#define M_RD_VERSIONE_FW    		0x74
#define M_CM_CLEAR_DIAGNOSI_RADIO   0x75
#define M_RD_PARAMETRI      		0x76
#define M_RD_ACQUISITA      		0x77
#define M_PORTANTE_ON       		0x78
#define M_IN_MANUTENZIONE   		0x79
//#define M_IN_INSTALLAZIONE  0x7a
#define M_P_ACQUISITA       		0x7b
#define M_P_ELIMINATA       		0x7c
#define M_IN_COLLAUDO       		0x7d
#define M_RESET_TEMPO_BATT     		0x7e


//define per modo periferiche
#define MP_FABBRICA         0x0f
#define MP_IN_SERVIZIO      0x00
#define MP_IN_MANUTENZIONE  0x01
//#define MP_IN_INSTALLAZIONE 0x02
#define MP_IN_COLLAUDO      0x03




#define MAX_RIPETIZIONI (5	- 1)				//numero massimo ripetizioni TX se non risposta da concentratore

#define USEC15625 15625

#ifdef BATTERIA_9V
	#define SOGLIA_BATT_L 165                   //178 //(175+14)  //(158 * 39,36 = 6,218 [7000- (19*39.36) = ) step da 39,36mV  (2500/256  * (1.000.000  + 330.000 ) / 330.000  (partitore 1.0M :330K) c'e' anche la caduta del diodo
	#define ISTERESI      2
	#define CADUTA_DIODO  14                    //19
#else
	#define SOGLIA_BATT_L 138                   //128 //STEP DA  19,52 (2500/256)
	#define ISTERESI      10
	#define CADUTA_DIODO  0
#endif


#define ATTESA_MBATT 100;                       //numero conversioni da fare per ritardo dopo fine tx (circa 100 usec /conversione); se riceve un SyncWord fa 6 conversioni dopo e ricava la media delle ultime 4;


//  *********************  DEFINIZIONE COSTANTI  ******************
//accelerometro
#define     TEMP_CFG_REG 		0x1f

#define     REFERENCE			0x26

#define     FIFO_CTRL_REG 		0x2E

#define     INT1_CFG	 		0x30
#define     INT1_SRC			0x31
#define     INT1_THS	 		0x32
#define     INT1_DURATION 		0x33
#define     CLICK_CFG			0X38
#define     CLICK_SRC			0x39

#define     CLICK_THS   		0X3A
#define     TIME_LIMIT          0X3B
#define     TIME_LATENCY	    0x3C
#define     TIME_WINDOW		    0x3D


//valori di set parametri
//tapparella

#define     TEMPO_TAPPA_DEF     10      //in secondi
#define     IMP_TAPPA_DEF       5       // VALORE DI DEFAULT per gli impulsi di allarme tapparella

//#define     CLICK_THS_S      120      // Soglie di sensibilità per riconoscimento colpi contro il vetro
//#define     MOV_THS_S        10       // Soglie di sensibilità per riconoscimento SPOSTAMENTO (apertura finestra)

#define     CLICK_THS_S      126        // Soglie di sensibilità per riconoscimento colpi contro il vetro
#define     MOV_THS_S        15         // Soglie di sensibilità per riconoscimento SPOSTAMENTO (apertura finestra)


//#define MASK_RADIO 	0x20

#define MASK_TAPP  	0x04                // ------------------------- sostituire con 0x02 (BIT1) sul port1 ---------------------------
//#define MASK_TAMPER 	0x02
//#define MASK_AMPOLLA 0x08


#define DA_RADIO   1
#define DISINT     2

//  ***********  DEFINIZIONE MNEMONICA DEI COMANDI DI SET/RESET USCITE e LETTURA INGRESSI ******************

#define     LED_ON        P3OUT |=  0x40                // ACCENDE IL LED A (P3.6) 1 = acceso
#define     LED_OFF       P3OUT &= ~0x40                // SPEGNE
#define     LED_T         P3OUT ^=  0x40                // TOGGLE IL LED A (P3.6)


// porta spi per comunicazione con SI4432
#define     SPI_CLOCK_1     P1OUT |=  0x01              // P1.0 Uscita clock della porta seriale sincrona (Io sono master)
#define     SPI_CLOCK_0     P1OUT &= ~0x01

#define     SPI_OUT_1       P1OUT |=  0x80              // P1.7 Uscita dati della porta seriale sincrona                    [nella versione precedente era 1.2]
#define     SPI_OUT_0       P1OUT &= ~0x80

#define     SPI_CS_RADIO_1        P3OUT |=  0x02        // P3.1 Uscita CHIP SELECT della porta seriale sincrona
#define     SPI_CS_RADIO_0        P3OUT &= ~0x02

#define     SDN_1           P3OUT |=  0x80              // P3.7 Uscita SHUT DOWN al SI4432
#define     SDN_0           P3OUT &= ~0x80

// SEGNALI in ingresso
#define     SPI_IN          (P1IN & 0x40)               // TEST DEL P1.6 Ingresso dati della porta seriale sincrona         [nella versione precedente era 1.1]

#define     IRQ_RADIO_IN    (P2IN & 0x20)               // TEST DEL P2.5 Ingresso interrupt da radio

#define     OUT_TEST_1     P3OUT |=  0x20               // P3.5 Uscita di test
#define     OUT_TEST_0     P3OUT &= ~0x20

#define     PULS_T          (P2IN & 0x02)               // TEST DEL P2.1 Ingresso Tamper
//#define     PULS_A          (!(P2IN & 0x02))            // TEST DEL P2.1 Ingresso Tamper
//#define     PULS_B          (!(P2IN & 0x04))            // TEST DEL P2.2 Ingresso tapparella
//#define     PULS_C          (!(P2IN & 0x08))            // TEST DEL P2.3 Ingresso ampolla

//  *********************  MACRO  ******************

//wdog ogni inpfreq [32kh] / 8kh;  ogni 1/4 secec
//#define     CLR_WDT    WDTCTL = WDTPW + WDTHOLD                           // RESET DEL WATCHDOG, E RIPARTENZA  **** STILE PIC! ****                       //per debugg wdog disabilitato
#define     CLR_WDT       WDTCTL = WDTPW + WDTCNTCL + WDTIS0 + WDTSSEL      // RESET DEL WATCHDOG, E RIPARTENZA  **** STILE PIC! ****  //WDOG ABILITATO
#define     STOP_WDT      WDTCTL = WDTPW + WDTHOLD  + WDTIS0 + WDTSSEL      // Stop watchdog timer
                                                                            // SE VOGLIO USARE ACLK DIVENTA:    WDTCTL = WDTPW + WDTCNTCL + WDTSSEL  //usa ACLK  (quarzo esterno)
				                                                            // altrimenti   WDTCTL = WDTPW + WDTCNTCL                                //usa SMCLK (rc osc interno [si spegne e cambia frequenza]

//costanti

#define ADDR_SEGMENTO_B  (unsigned int *)0x1080
#define ADDR_SEGMENTO_C  (unsigned int *)0x1040
#define ADDR_SEGMENTO_D  (unsigned int *)0x1000


//costanti in flash
#ifdef BATTERIA_9V

	#ifndef ALTRE_MISURE
		#ifndef POTENZA_VARIABILE
			const char versione[2] = { 0x1e, 0x0ef};            //versione(lsb), msb,
		#else
			#ifdef GENERAPORTANTE
				const char versione[2] = { 0x19, 0xf1};         //versione(lsb), msb, versione ff0c	 	//tx potenza variabile
			#else
				const char versione[2] = { 0x19, 0x01};         //versione(lsb), msb, versione ff0c	 	//tx potenza variabile NORMALE 9V
		   	#endif
		#endif
	#else
		#ifndef POTENZA_VARIABILE
			const char versione[2] = { 0x1f, 0x0ef};            //versione(lsb), msb, //per prove; non funziona write parametri, togliere rssi_rx_max e rssi_rx_mix
		#else
				const char versione[2] = { 0x29, 0x0ef};        //versione(lsb), msb, versione ff0c	 	//tx potenza variabile
		#endif
	#endif

#else
	#ifndef ALTRE_MISURE
		#ifndef POTENZA_VARIABILE
			const char versione[2] = { 0x1e, 0x0ff};            //versione(lsb), msb,
		#else
			#ifdef GENERAPORTANTE
				const char versione[2] = { 0x19, 0xf1};         //versione(lsb), msb, versione ff0c	 	//tx potenza variabile
			#else
				const char versione[2] = { 0x19, 0x01};         //versione(lsb), msb, versione ff0c	 	//tx potenza variabile NORMALE 3V
	   		#endif
	   	#endif
	#else
		#ifndef POTENZA_VARIABILE
			const char versione[2] = { 0x1f, 0x0ff};            //versione(lsb), msb, //per prove; non funziona write parametri, togliere rssi_rx_max e rssi_rx_mix
		#else
			const char versione[2] = { 0x29, 0x0ff};            //versione(lsb), msb, versione ff0c	 	//tx potenza variabile
		#endif
	#endif
#endif





const short step_fre_radio[5] = { FREQ_CARR_CHB , FREQ_CARR_CH1 ,  FREQ_CARR_CH2, FREQ_CARR_CH3, FREQ_CARR_TARATURA};


// *********************  PROTOTIPI DELLE FUNZIONI ******************

void riconoscimento_sensore_connesso(void);                                                                       // riconosce il sensore esterno e il tipo di connessione con cui si interfaccia
void init_sensors(void);                                                                                          // inizializza in base al tipo di sensore e di connessione il micro
void sensore_tx(unsigned char string[11],unsigned int size);

void wreg(unsigned int indirizzo,unsigned int dato,unsigned char mode);
void rx_byte(unsigned char *dati,unsigned char address,unsigned char numero,unsigned char mode);
void tx_byte(unsigned char *dati,unsigned char address,unsigned char numero,unsigned char mode);

void format_dati_polling(void);
void tx_polling(void);
void radio_on(void);
void init_radio(unsigned char canale);

void encrypta(unsigned int sequenza,unsigned char * dati,unsigned char *dati_cryp) ;
unsigned int decrypta(unsigned char* dati_c, unsigned char *dati_chiaro) ;
void crea_dati_enc(unsigned char n_byte_response, unsigned int*key, unsigned int sequen, unsigned char *response);  //NUMERO BYTES RISPOSTA, CHIAVE[8bit*8], SEQUENZA[16bit], PUNTATORE RISUSLTATO
void setta_parita(unsigned char *dato, unsigned char dispari);                                                      //parita
unsigned int controlla_parita(unsigned char dato, unsigned char dispari);                                           //parita

unsigned int  decodifica_dati_rx_comun_perif(void);


void leggi_flash(unsigned int * pnt_src, unsigned int * pnt_dest, unsigned int numero)  ;
void scrivi_flash(unsigned int * pnt_src, unsigned int * pnt_dest, unsigned int numero) ;
unsigned int calcola_crc_install(void);


void g_mancata_risposta(void)     ;
void ricarica_cnt_sono_vivo(void) ;
void start_adc(void);
void off_adc(void);
void carica_timer_rx_sincrono(void);

void gestione_radio_ex_interr(void);


//variabili

unsigned int cnt_ore;
unsigned int temp_main;
//unsigned int cnt_err_superif_crc_radio;
unsigned int timeout;
unsigned int cnt_err_superif_resetradio;
unsigned int cnt_err_superif_no_tx_radio;
unsigned char   cnt_ripetizione_tx;
unsigned char   stato;

struct
{
	 unsigned int timer_perif;                          //valore del timer sulla periferica al momento della ricezione della sync word;
	 unsigned int timer_conc;                           //valore mancante al trigger del secondo per il concentratore (al momento di caricare il messaggio nel fifo radio)

	 unsigned char dati[LUNGHEZZA_DATI_PERIFERICHE + 4];
	 unsigned char rssi;
	 unsigned char dati_rdy;
	 unsigned char pnt_byte;

			#ifdef ALTRE_MISURE
	 			unsigned char rumore_rx0;
	 			unsigned char rumore_rx1;
	 			unsigned char rumore_rx2;
	 			unsigned char rumore_rx3;
			#endif
	 struct
	 {
		unsigned char endtx               :1;
		unsigned char no_usa_calibrazione :1;
		unsigned char reset				  :1;
//		unsigned char l2 	 :1;
//		unsigned char puls_c :1;
//		unsigned char l3 	 :1;
//		unsigned char l4 	 :1;
	 }flag;
}buffer_radio;

typedef union
{
	struct
	{
 					unsigned char  allarme_1    :1;
 					unsigned char  allarme_2    :1;
 					unsigned char  tamper       :1;
 					unsigned char  batteria_l   :1;
 					unsigned char  guasto	    :1;
	} PACKED n;
	unsigned char i8;
} PACKED flag_sens_t;

typedef	union
{
		struct
		{
			   unsigned int  n_sequenza;			    // [15bit]

			   unsigned char tipo_messaggio;
			   flag_sens_t stato_allarmi; 		        // stato, batt,all, man,... )

			   unsigned int  id[2];

			   union
			   {
			   		unsigned char i8[8];
			   		unsigned int  i16[4];
			   }dati;
			   unsigned int    snum_conc;               // serial numero concentratore
			   unsigned char   rssi;
			   unsigned char   mode_canale;             //  servizo/manut/collaudo [4b(low)], canale[3b(high)],
			   									        //  da concentraatore a periferica non viene usato per il modo ma trasporta in numero concentratore in impianto e il modo_manutenzione_b
		} PACKED n;
		unsigned char i8[LUNGHEZZA_DATI_PERIFERICHE + 4];
}messaggio_radio_t;

messaggio_radio_t messaggio_rx, messaggio_tx;

//NON SPOSTARE ORDINE >>
//dati copiati in flash
//fabbrica                                              //seg B
struct {
		unsigned int   ser_num_perif[2];                //un bytes e' per il tipo
		unsigned int   taratura_radio;                  //calibrazione radio
	    union
		{
		   		unsigned char i8[8];
		   		unsigned int  i16[4];
		}tab_random;
		unsigned int   crc_taratura_radio_ser_num;
} segmB;

//installazione                                         //seg C
union {
	struct {
		unsigned int    snum_concentratore;
		unsigned char   canale_radio;
		unsigned char   crc_dati_install;
		struct{
			unsigned char   tempo_tappa  ;              //parametri
			unsigned char   impulsi_tappa;
			unsigned char   p3           ;
			unsigned char   p4		     ;
			unsigned char   p5			 ;
			unsigned char   p6			 ;
			unsigned char   libero       ;
			unsigned char   libero1      ;
		} parametri_periferica;
	};
	unsigned int p[0];
} segmC;

//NON SPOSTARE ORDINE <<

unsigned char pnt_tab_random;
union
{
	   struct
	   {
//		unsigned int numero_sequenza_ultimo_rx;         //usare solamente 15 bit
		unsigned int numero_sequenza_ultimo_tx;         //usare solamente 15 bit
		flag_sens_t stato_new;
		flag_sens_t stato_rx;
		unsigned char  parametri[8];
		unsigned int   tempo_batteria;                  //in flash segmento D
		unsigned char  rssi_rx;                         //rssi messaggio ricevuto da periferica
		unsigned char  mode_f;         

		unsigned int cnt_err_superif_comunicazione;
		unsigned int cnt_err_superif_secondi_tentativi;
//		unsigned int cnt_err_superif_decrypta;

//		unsigned char  canale_r;       

//		unsigned int numero_sequenza_tx;                //usare solamente 15 bit

		unsigned int  cnt_sono_vivo;
		unsigned char cnt_mess_persi;
		unsigned char cnt_tentativi;
		unsigned int  cnt_sec_coll_man_inst;
//		unsigned int  cnt_sec_inst;
			#ifdef ALTRE_MISURE
		unsigned char  rssi_rx_max;
		unsigned char  rssi_rx_min;
			#endif
//		unsigned char  rssi_rx;
//		unsigned char  rssi_rx;

	   		#ifdef POTENZA_VARIABILE
		unsigned char  ultima_pw_tx;
			#endif


	   }n;
	   unsigned char i8[20];
}dati_periferica;

struct
{
		unsigned char risponde_subito           :1;
		unsigned char comunicazione_in_corso    :1;
		unsigned char /*movimento*/l1			        :1;
		unsigned char adc_in_corso		 		:1;
		unsigned char stato_manutenzione_b	 	:1;
//		unsigned char puls_c :1;
//		unsigned char l3 	 :1;
		unsigned char decrypta_acq 	 :1;                //encryptazione per sensore acquisito
}flag;

unsigned char prossimo_polling;

unsigned char cnt_mask_tappa;
unsigned char cnt_imp_tappa;
unsigned char cnt_tempo_tappa;

unsigned char vbatt_media_corta[4];                     //per ogni trasmissione si fanno 4 letture e poi si media il valore
unsigned char vbatt[4];                                 //valori letti nelle ultime 4 trasmissioni
unsigned char lev_batteria;                             //si ottiene mediando i valori delle ultime 4 trasmissioni
unsigned char pnt_vbatt;
unsigned char cnt_conversioni_vbatt;


unsigned int  periodo_rx_sincrona[4];
unsigned char pnt_periodo_rx_sincrona_uso;
unsigned char n_concentratore_in_impianto;
unsigned char cnt_post_fpro;
unsigned char scrivi_tempo_batteria_pnd;


//------- Temporizzazione

unsigned char input_cnt;
unsigned char input_old;

typedef union
{
	struct
	{
		unsigned char l1             :1;
		unsigned char tamper 		 :1;
		unsigned char tappa 		 :1;
		unsigned char /*ampolla*/l2 		 :1;
		unsigned char l5             :1;
		unsigned char pontic_p1_4 	 :1;
		unsigned char l3 	         :1;
		unsigned char l4 	         :1;
	}b;
	unsigned char i8;
}input_t;

input_t input_stato;
//input_t input_fronte;

unsigned char tempo_led_on;

unsigned char stato_new_perledon;

enum state{Wakeup, Header, Length, Reading};
enum rx_function{detector_events,req_init, ack, nack, GDE, GPageM, GPropertyM, GDP, GMN, GCV, IDLE};

volatile unsigned int flag_tx_finished = 0 ;
volatile unsigned char rx_mex[11];
volatile unsigned int tipo_connessione_sensore;
unsigned char tx_string[11], rx_string[1+9];                    // stringhe di rx e tx per comunicazione UART
unsigned int tx_index=0, rx_index=0;                            // indici per comunicazione UART 
enum state actual_state = Wakeup;
volatile int flag_rx = 1;
volatile enum rx_function rx_cmd = IDLE, tx_cmd = IDLE;
unsigned char ack_str[6] = {0x00,0xaa,0x05,0x06,0x71,0x26};     // acknowledge
unsigned char init_str[6] = {0x00,0xaa,0x05,0x30,0x01,0xe0};    // control panel event con attvazione in modalità ARM
unsigned char GMN_str[5] = {0x00, 0xaa, 0x04, 0x39, 0xe7};      // get model number (1,2 = VXS - 3,4 = BXS) 
volatile unsigned int string_size;
volatile int  uart_pause_flag = 1;
int rx_valid_flag = 0;

/***************************************************************************/
/******************** INIZIO DEL MAIN **************************************/
/***************************************************************************/
int main(void)
{
    
    P3DIR=0xE2;
    P3OUT=0x95;                                                 // PULL up p3.0,2,4; uscita 3.7 =1 (radioOFF)
    P3REN=0x15;                                                 // PULL-UP

// PAUSA INIZIALE CON BASSO CONSUMO, per immunita dai rimbalzi dell'inserimento pila
    DCOCTL  =   CALDCO_1MHZ;
    BCSCTL1 =   CALBC1_1MHZ | XT2OFF;
    BCSCTL3 =   XCAP_1;                                         //XIN/XOUT Cap :0, 6, 10,12.5 pF  (xcap0/xcap3)

	timeout = 100;
    while (--timeout)
    {
        CLR_WDT;
        temp_main = 400;
        while(temp_main--);
    }

//  CONFIGURAZIONE DELLE PORTE
// ____________________________________________________________
// |PIN | FUNZ.|DIR|PULL|  UTILIZZO            |  COMMENTI     |
// |----|------|---|----|----------------------|---------------|
// |    | P1.0 | O |    |SCLK al SI4432        |               |
// |    | P1.1 | I |    |UART RX e contatto    | interrupt     |
// |    | P1.2 | O |    |UART TX               |               |
// |    | P1.3 | I |    |input ADC batt 9VDC   |               |
// |    | P1.4 | O | PD |JTAG                  |modif per pont.|
// |    | P1.5 | I | PD |JTAG                  |modif per pontc|
// |    | P1.6 | I |    |inpSO da SI4432 e da..|               |
// |    | P1.7 | O |    |SI al SI4432          |               |
// |    -------------------------------------------------------|
// |    | P2.0 | I |    |LIBERO                |               |
// |    | P2.1 | I | UD |TAMPER                |               |
// |    | P2.2 | I |    |LIBERO                |               |
// |    | P2.3 | I |    |LIBERO                |               |
// |    | P2.4 | I |    |LIBERO                |               |
// |    | P2.5 | I |    |IRQ RADIO             | interrupt     |
// |    | P2.6 | I |    |XTAL IN               |               |
// |    | P2.7 | O |    |XTAL OUT              |               |
// |    -------------------------------------------------------|
// |    | P3.0 | I |    |LIBERO                |               |
// |    | P3.1 | O |    |CS al SI4432          |               |
// |    | P3.2 | I |    |LIBERO                |               |
// |    | P3.3 | I | UD |presenza batteria 9vdc| interrupt     |
// |    | P3.4 | I |    |LIBERO                |               |
// |    | P3.5 | O |    |TEST                  |               |
// |    | P3.6 | O |    |LED                   |               |
// |    | P3.7 | O |    |SDN RADIO             |               |
// |____|______|___|____|______________________|_______________|
// |____|______|___|____|______________________|_______________|

    #ifdef BATTERIA_9V
        P1DIR=0x95;     //  [nella versione precedente era 0x15]
        P1OUT=0x00;     // se 0x40 = pull up solo sul MISO per test
        P1REN=0x20;     // 0x60 se per prova metto pull up al MISO // JTAG pull-dwnati         //p1.4 uscita a massa per lettura ponticello    
	    P1SEL=0x08;     // P1.3 ADC                             ------------------------------- DA MODIFICARE PER USARE LA UART: P1SEL e P1SEL1 -------------------------------
    #else
        P1DIR=0x95;     //  [nella versione precedente era 0x15]
        P1OUT=0x00;     // se 0x40 = pull up solo sul MISO per test
        P1REN=0x28;     // 0x68 se per prova metto pull up al MISO // JTAG pull-dwnati         //p1.4 uscita per lettura ponticello       
	    P1SEL=0x00;     // P1.3 input con pull up
    #endif
    P1IE =0x00;     //disabilita gli interrrupts sugli input

    P2DIR=0x80;  
    P2OUT=0x1D;     // PULL up p2.0,2,3,4
    P2REN=0x0B;     // PULL-UP/dw
    P2SEL=0xC0;     // QUARZO
    P2IE = 0;       // disabilita gli interrrupts sugli input
    P2IFG = 0;      // cancella eventuali flags

    riconoscimento_sensore_connesso();                          // riconosce se la connessione al sensore esterno è seriale o contatto
   
   periodo_rx_sincrona[0] = 32767;
   periodo_rx_sincrona[1] = 32767;
   periodo_rx_sincrona[2] = 32767;
   periodo_rx_sincrona[3] = 32767;

    // CONFIGURAZIONE TIMER A
    // ACLK =  32.... khz  (quarzo esterno)
    TA0CTL = TASSEL_1 + TACLR + MC_0;    // ACLK/1  clear TAR
    TA0CCR0 = 512;                         //   ogni 15,625 msec
    TA0CCTL0= CCIE;                       // CCR1 interrupt enabled

	if ( ++pnt_periodo_rx_sincrona_uso  > 3)   pnt_periodo_rx_sincrona_uso = 0;
	TA0CCR1 = TA0R + (periodo_rx_sincrona[pnt_periodo_rx_sincrona_uso] + ((unsigned int)n_concentratore_in_impianto << 11)) ;     //   TIMER PER RICEZIONE SINCRONA

	leggi_flash(ADDR_SEGMENTO_D, &dati_periferica.n.tempo_batteria, 1);    //legge 1 intero

	leggi_flash(ADDR_SEGMENTO_B, segmB.ser_num_perif, (7+1));        //legge 7 interi(+segmB.crc_taratura_radio_ser_num): segmB.ser_num_periferica e segmB.taratura_radioe segmB.tab_random
    //vedere togliere

	if ((  segmB.ser_num_perif[0] + segmB.ser_num_perif[1]  + segmB.taratura_radio + segmB.crc_taratura_radio_ser_num
	      + segmB.tab_random.i16[0] + segmB.tab_random.i16[1]+ segmB.tab_random.i16[2]+ segmB.tab_random.i16[3] ) != 0xffff)
	{
		segmB.ser_num_perif[0] =  0x0000;
        #ifdef BATTERIA_9V
		    segmB.ser_num_perif[1] =  (PERIF_TIPO_CONTATTO_9V << 8);
        #else
		    segmB.ser_num_perif[1] =  (PERIF_TIPO_CONTATTO << 8);
        #endif
		segmB.tab_random.i16[0] = 0x0010;
		segmB.tab_random.i16[1] = 0x2030;
		segmB.tab_random.i16[2] = 0x4050;
		segmB.tab_random.i16[3] = 0x6070;

		segmB.taratura_radio = 0;
		dati_periferica.n.stato_new.n.guasto =1;
	}

    //vedere togliere
	if(    ((segmB.taratura_radio > 0xF000) && (segmB.taratura_radio < 0xFee0))   //-0x120              = -90Khz
	    || ((segmB.taratura_radio < 0x0FFF) && (segmB.taratura_radio > 0x0120))   //0x120 = 288 * 312.5 = 90Khz
	  )
	{
		segmB.taratura_radio = 0;
		dati_periferica.n.stato_new.n.guasto =1;
	}
	leggi_flash(ADDR_SEGMENTO_C, &segmC.snum_concentratore, 6);    //legge 6 interi: segmC.snum_concentratore, segmC.canale_radio, segmC.crc_dati_install, parametri sensore

	if (calcola_crc_install())
	{
		dati_periferica.n.stato_new.n.guasto =1;

		segmC.canale_radio                       = CANALE_BASE;
		segmC.parametri_periferica.tempo_tappa   = TEMPO_TAPPA_DEF;
		segmC.parametri_periferica.impulsi_tappa = IMP_TAPPA_DEF  ;
		segmC.parametri_periferica.p3	         = CLICK_THS_S;
		segmC.parametri_periferica.p4	         = MOV_THS_S;
		segmC.parametri_periferica.p5		     = 0xff;
		segmC.parametri_periferica.p6		     = 0xff;
		segmC.parametri_periferica.libero        = 0xff;
		segmC.parametri_periferica.libero1       = NUMERO_PARAMETRI_PERIFERICA; // correzione per esigenze capella // = 0xff;

		segmC.snum_concentratore                 = 0xffff;
	}

	TA0CTL |= MC_2;              // Start Timer_A in continuous mode
    _EINT(); 
    
    init_sensors();                                             // inizializzazione per sensore volumetrico o per contatto      
                    // Enable interrupts
    
//    *************** PAUSA INIZIALE ****************

	tempo_led_on = 1+ (250/250);
    LED_ON;           			// POI ACCENDE IL LED
    timeout =  500000/USEC15625;
	while ( timeout)
	{
	 	CLR_WDT;
	 	LPM3;
	}
    LED_OFF;
	tempo_led_on = 0;

// ***************** PRIMA di ENTRARE NEL MAIN LOOP VERIFICO SE DEVO CANCELLARE TUTTO!! *************
//         PRIMA VUOLE 3 s CON IL PULSANTE PREMUTO

    P2OUT |= 0x02;                                  // PULL up anche su p2.1  PULS_T
	timeout =  5000000/USEC15625;                   //  5 s
//	timer_attesa = 3000000 / USEC15625;             //  3 s
	while (timeout && !PULS_T)
	{
		CLR_WDT;
		LPM3;
	    P2OUT |= 0x02;                              // PULL up anche su p2.1  PULS_T //il timer interrupt lo toglie
	}
    if (timeout ==0)         	                    // OSSIA se il tempo e' scaduto con il pulsante schiacciato
    {
	    P2OUT |= 0x02;                              // PULL up anche su p2.1  PULS_T //il timer interrupt lo toglie
        timeout = 2000000/ USEC15625;      	        //  E LASCIA 1 S DI TEMPO PER mollare il pulsante
        while (timeout && !PULS_T)
		{
	        LED_ON;           			            // POI ACCENDE IL LED
	  		tempo_led_on = 1 + (250/250);
			CLR_WDT;
			LPM3;
		    P2OUT |= 0x02;                          // PULL up anche su p2.1  PULS_T //il timer interrupt lo toglie
		}
        LED_OFF;       			                    // POI ACCENDE IL LED
  		tempo_led_on = 0;
        if (timeout)         	                    // SOLO se il pulsante E' stato mollato prima che il cnt arrivasse a 0
        {
			segmC.canale_radio                       = CANALE_BASE;
			segmC.parametri_periferica.tempo_tappa   = TEMPO_TAPPA_DEF;
			segmC.parametri_periferica.impulsi_tappa = IMP_TAPPA_DEF  ;
			segmC.parametri_periferica.p3	         = CLICK_THS_S;
			segmC.parametri_periferica.p4	         = MOV_THS_S;
			segmC.parametri_periferica.p5		     = 0xff;
			segmC.parametri_periferica.p6		     = 0xff;
			segmC.parametri_periferica.libero        = 0xff;
			segmC.parametri_periferica.libero1       = NUMERO_PARAMETRI_PERIFERICA; // correzione per esigenze capella // = 0xff;

			segmC.snum_concentratore              = 0xffff;
		    segmC.crc_dati_install = 0;
			segmC.crc_dati_install = calcola_crc_install();
			scrivi_flash( &segmC.snum_concentratore, ADDR_SEGMENTO_C, 6);        //scrive 6 interi:
			CLR_WDT;
            for (temp_main=0; temp_main<10; temp_main++)
            {
		  		tempo_led_on = 0;                                                //led lampeggia
                LED_T;
				timeout = 250000/ USEC15625;
        		while (timeout) // && !PULS_T)
				{
					CLR_WDT;
					LPM3;
				}
			}
        }
        LED_OFF;
    }
    P2OUT=0x01;                                                                  // PULL up p2.0            ----------------- FORSE SI PUO' TOGLIERE ----------------  

	cnt_imp_tappa = segmC.parametri_periferica.impulsi_tappa;
	cnt_tempo_tappa = (segmC.parametri_periferica.tempo_tappa << 2);

	input_old  = 0xff;
	input_stato.i8 = 0;
	cnt_ripetizione_tx = MAX_RIPETIZIONI;

 	start_adc();              //misura vbatt
	temp_main = 4;            //per attendere lettura ingressi prima di loop normale
 	stato = 0xf0;             //per leggere ingressi logici e vbatt
    timeout =  500000/USEC15625;

//    if(tipo_connessione_sensore == 0)     // connessione tramite contatto
//    {
    _DINT();                  // DISABILITA GLI interrupts
	P2IE  = MASK_TAPP ;       // abilita gli interrupts su tapp
//    P1IE = MASK_TAPP;         // abilita gli interrupts su tapp
    _EINT();                  // Enable interrupts
//    }
	if ( segmC.snum_concentratore == 0XFFFF)
	{
			dati_periferica.n.numero_sequenza_ultimo_tx = (dati_periferica.n.numero_sequenza_ultimo_tx + 2 ) & 0x7ffe;              //incrementa numer sequenza dopo risposta ok (periferica solo numeri pari
		    TA0CCTL1 = 0;                                                                                                           // CCR1 interrupt DISABLE
	}
	else
	{
			dati_periferica.n.numero_sequenza_ultimo_tx = ((dati_periferica.n.numero_sequenza_ultimo_tx + 2) | 0x8000) & 0xfffe;    //incrementa numer sequenza dopo risposta ok
		    TA0CCTL1 = CCIE;                                                                                                        // CCR1 interrupt enabled
	}

    #ifdef ALTRE_MISURE
    	dati_periferica.n.rssi_rx_max = 0;
    	dati_periferica.n.rssi_rx_min = 0xff;
	#endif

	#ifdef POTENZA_VARIABILE
    	dati_periferica.n.ultima_pw_tx = POTENZA_MIN + ((POTENZA_MAX - POTENZA_MIN)/2);
	#endif


	if (IFG1 & PORIFG)
	{
		CLR_WDT;
			 cnt_ore = 0;
			 dati_periferica.n.tempo_batteria = 0;
			 scrivi_flash(&dati_periferica.n.tempo_batteria,ADDR_SEGMENTO_D, 1);
			 IFG1 &= ~PORIFG;
		CLR_WDT;
	}

	while(1)
    {

		CLR_WDT;

		switch ( stato)
		{
			case 0xf0:                                  //dopo reset per leggere ingressi e vbatt.

			    if ( flag.adc_in_corso)
			    {
	      			if ((ADC10CTL1 & ADC10BUSY) == 0)
      				{
							    if (++pnt_vbatt >= 4)
							    {
								    pnt_vbatt =0;
								    flag.adc_in_corso = 0;
							    }
							    vbatt[pnt_vbatt] =	ADC10MEM >> 2;
							    lev_batteria = CADUTA_DIODO +  ((unsigned int)(vbatt[0] + vbatt[1] + vbatt[2] + vbatt[3]) >> 2);

							    if (lev_batteria < SOGLIA_BATT_L)
								    dati_periferica.n.stato_new.n.batteria_l = 1;
							    else if (lev_batteria > SOGLIA_BATT_L + ISTERESI)
								    dati_periferica.n.stato_new.n.batteria_l = 0;

							    if ( flag.adc_in_corso)
								    start_adc();            //misura subito dopo tx
							    else
								    off_adc();
				    }
			    }

			    if ( ((input_cnt == 0) && (flag.adc_in_corso == 0))	 || (timeout == 0) )
			    {
					    if (--temp_main == 0)
					    {
							    stato = 0;
							    dati_periferica.n.cnt_sono_vivo = (2000000 / USEC15625) + segmB.tab_random.i8[0x7 & (pnt_tab_random++)];          //ogni USEC15625
					    }
			    }
			    else
					    temp_main = 2;
			    break;

			case 0:                                     //stato riposo

					if ( cnt_post_fpro)
					{
        				OUT_TEST_1;
						if ( dati_periferica.n.cnt_sono_vivo == 0)
	  						dati_periferica.n.cnt_sono_vivo = segmB.tab_random.i8[0x7 & (pnt_tab_random++)];
		        		OUT_TEST_0;
						break;
					}

                    #ifndef SECURGROSS

// ------------- su allarme1 mappare uno o più degli allarmi della uart -------------------
/*                        switch(tipo_connessione_sensore)    
                          {
                                case 0:             // connessione contatto*/
					    if ((cnt_imp_tappa == 0) || (input_stato.b.tappa == 0)/* || (input_stato.b.ampolla == 0)*/)
							    dati_periferica.n.stato_new.n.allarme_2 = 1;                                        // AUX
					    else
							    dati_periferica.n.stato_new.n.allarme_2 = 0;
					    /*if ((flag.movimento) || (input_stato.b.ampolla == 0))
							    dati_periferica.n.stato_new.n.allarme_1 = 1;                                        // CM
					    else
							    dati_periferica.n.stato_new.n.allarme_1 = 0;*/

/*                                      break;
                                case 1:             // connessione uart
                                        if(allarme_x sulla uart)
							                    dati_periferica.n.stato_new.n.allarme_1 = 1;                                        // CM
					                    else
							                    dati_periferica.n.stato_new.n.allarme_1 = 0;
                                        if(allarme_y sulla uart)
							                    dati_periferica.n.stato_new.n.allarme_2 = 1;                                        // CM
					                    else
							                    dati_periferica.n.stato_new.n.allarme_2 = 0;
                                        break;
                          }
*/
                                            
                    #else

                    	if ( input_stato.b.pontic_p1_4)
	                    {                                                                                           //come non securgross
/*                        switch(tipo_connessione_sensore)    
                          {

                                case 0:             // connessione contatto*/
					        if ((cnt_imp_tappa == 0) || (input_stato.b.tappa == 0)/* || (input_stato.b.ampolla == 0)*/)
						       	dati_periferica.n.stato_new.n.allarme_2 = 1;                                        // AUX
					        else
						       	dati_periferica.n.stato_new.n.allarme_2 = 0;

					      /*  if ((flag.movimento) || (input_stato.b.ampolla == 0))
							    dati_periferica.n.stato_new.n.allarme_1 = 1;                                        // CM
					        else
							    dati_periferica.n.stato_new.n.allarme_1 = 0;*/

/*                                      break;
                                case 1:             // connessione uart
                                        if(allarme_x sulla uart)
							                    dati_periferica.n.stato_new.n.allarme_1 = 1;                                        // CM
					                    else
							                    dati_periferica.n.stato_new.n.allarme_1 = 0;
                                        if(allarme_y sulla uart)
							                    dati_periferica.n.stato_new.n.allarme_2 = 1;                                        // CM
					                    else
							                    dati_periferica.n.stato_new.n.allarme_2 = 0;
                                        break;
                          }
*/

	                    }
	                    else
	                    {
/*                        switch(tipo_connessione_sensore)    
                          {
                                case 0:             // connessione contatto*/

					        if (/*(flag.movimento) || */(cnt_imp_tappa == 0) || (input_stato.b.tappa == 0)  /* || (input_stato.b.ampolla == 0)*/)
							    dati_periferica.n.stato_new.n.allarme_2 = 1;                                         // AUX
					        else
							    dati_periferica.n.stato_new.n.allarme_2 = 0;

					     /*   if ((input_stato.b.ampolla == 0))
							    dati_periferica.n.stato_new.n.allarme_1 = 1;                                         // CM
					        else
							    dati_periferica.n.stato_new.n.allarme_1 = 0;*/

/*                                      break;
                                case 1:             // connessione uart
                                        if(allarme_x sulla uart)
							                    dati_periferica.n.stato_new.n.allarme_1 = 1;                                        // CM
					                    else
							                    dati_periferica.n.stato_new.n.allarme_1 = 0;
                                        if(allarme_y sulla uart)
							                    dati_periferica.n.stato_new.n.allarme_2 = 1;                                        // CM
					                    else
							                    dati_periferica.n.stato_new.n.allarme_2 = 0;
                                        break;
                          }
*/
	                    }

                    #endif

					if (input_stato.b.tamper)
							dati_periferica.n.stato_new.n.tamper = 0;
					else
							dati_periferica.n.stato_new.n.tamper = 1;

	   				if ((segmC.snum_concentratore == 0XFFFF) /*|| (dati_periferica.n.mode_f == MP_IN_MANUTENZIONE)*/)
					{
							if (dati_periferica.n.stato_new.i8 != (stato_new_perledon))
							{
								stato_new_perledon =  dati_periferica.n.stato_new.i8;
								tempo_led_on = 1 + (250/250);
							}
					}

    				#ifdef GENERAPORTANTE
	    				if (input_stato.b.tamper == 1)
	    				{
									radio_on();

								    init_radio(segmC.canale_radio);               //tx continua  LAVORO
					 		        wreg(SI4432_MODULATION_MODE_CONTROL_2,0x00,DA_RADIO | DISINT);            // 0x71; FISSA!!!!
					 		        wreg(SI4432_OPERATING_AND_FUNCTION_CONTROL_1,SI4432_TXON,DA_RADIO | DISINT);    // manda in trasmissione

  									DCOCTL  =   CALDCO_1MHZ;
    								BCSCTL1 =   CALBC1_1MHZ | XT2OFF;

									stato = 10;
									timeout = messaggio_rx.n.dati.i8[0] * (100000 / USEC15625);
									tempo_led_on = 250;                    // on per 62 secondi, viene spento a fine portante
	    				}
	    			#endif
					if ((dati_periferica.n.stato_new.i8 != dati_periferica.n.stato_rx.i8) && (flag.comunicazione_in_corso == 0))
					{
						cnt_ripetizione_tx = MAX_RIPETIZIONI;
						tx_polling();
						stato = 2;
						timeout = 35000 / USEC15625;
					}

					else if ( dati_periferica.n.cnt_sono_vivo == 0)
					{
						tx_polling();
						stato = 2;
						timeout = 35000 / USEC15625;
					}
	        		break;

	        case 1:
	           		break;

			case 2:                                     //attesa fine tx

				if (buffer_radio.flag.endtx == 1)
				{
					start_adc();                        //misura subito dopo tx
					cnt_conversioni_vbatt = ATTESA_MBATT;

					stato = 3;
					timeout = 35000 / USEC15625;        //assicurati 15 msec
					if ( segmC.snum_concentratore == 0XFFFF)
						timeout += 40000 / USEC15625;             //se in manutenzione e vergine aspetta la risposta da piu concentratori (con ritardo)
				}
				else if ( timeout == 0)
				{                                       //radio non ok; per sicurezza di non lasciarlo attivo per  consumo
					SDN_1;       			  //radio off
					g_mancata_risposta();
			        #ifdef POTENZA_VARIABILE
				        dati_periferica.n.ultima_pw_tx = POTENZA_MIN;
			        #endif
                    dati_periferica.n.stato_new.n.guasto =1;
	                cnt_err_superif_no_tx_radio++;
					stato = 0;
				}
			    break;


			case 3:                                     //attesa fine rx
					if ( flag.adc_in_corso)
					{
						if ((ADC10CTL1 & ADC10BUSY) == 0)
						{
								vbatt_media_corta[3] =	vbatt_media_corta[2];
								vbatt_media_corta[2] =	vbatt_media_corta[1];
								vbatt_media_corta[1] =	vbatt_media_corta[0];
								vbatt_media_corta[0] =	ADC10MEM >> 2;
								if (--cnt_conversioni_vbatt)
								{
									ADC10CTL0 |= ENC + ADC10SC;             // Sampling and conversion start
								}
								else
								{
									if (++pnt_vbatt >= 4)
										pnt_vbatt =0;
									vbatt[pnt_vbatt] =	((unsigned int)(vbatt_media_corta[0] + vbatt_media_corta[1] + vbatt_media_corta[2] + vbatt_media_corta[3]) >> 2);

									lev_batteria = CADUTA_DIODO +  ((unsigned int)(vbatt[0] + vbatt[1] + vbatt[2] + vbatt[3]) >> 2);

									if (lev_batteria < SOGLIA_BATT_L)
										dati_periferica.n.stato_new.n.batteria_l = 1;
									else if (lev_batteria > SOGLIA_BATT_L + ISTERESI)
										dati_periferica.n.stato_new.n.batteria_l = 0;
									flag.adc_in_corso = 0;
									off_adc();
								}
						}
					}

				    if (buffer_radio.dati_rdy)
				    {
					    buffer_radio.dati_rdy = 0;

					    temp_main = decodifica_dati_rx_comun_perif();
					    if (temp_main)
					    {
	       					flag.comunicazione_in_corso = 0;
						    dati_periferica.n.stato_rx.i8 = messaggio_rx.n.stato_allarmi.i8;
						    cnt_ripetizione_tx = MAX_RIPETIZIONI;

						    if (flag.stato_manutenzione_b)
							    tempo_led_on =1 + (500/250);

						    if ( segmC.snum_concentratore == 0XFFFF)
							    dati_periferica.n.numero_sequenza_ultimo_tx = (dati_periferica.n.numero_sequenza_ultimo_tx + 2 ) & 0x7ffe;    //incrementa numer sequenza dopo risposta ok (periferica solo numeri pari
						    else
							    dati_periferica.n.numero_sequenza_ultimo_tx = ((dati_periferica.n.numero_sequenza_ultimo_tx + 2) | 0x8000) & 0xfffe;    //incrementa numer sequenza dopo risposta ok

						    dati_periferica.n.rssi_rx = buffer_radio.rssi;

                		    #ifdef ALTRE_MISURE
                			    if ( dati_periferica.n.rssi_rx > dati_periferica.n.rssi_rx_max)
            					    dati_periferica.n.rssi_rx_max = dati_periferica.n.rssi_rx;
                			    if ( dati_periferica.n.rssi_rx < dati_periferica.n.rssi_rx_min)
            					    dati_periferica.n.rssi_rx_min = dati_periferica.n.rssi_rx;
                		    #endif

						    ricarica_cnt_sono_vivo();
						    if ( dati_periferica.n.mode_f != MP_IN_COLLAUDO)
							    stato = 0;
						    else
						    {
								    if ( temp_main == 2)            //tx portante
								    {
									    radio_on();

									    init_radio(CANALE_TARATURA);         //tx continua  FREQ_CARR_TARATURA
					     		        wreg(SI4432_MODULATION_MODE_CONTROL_2,0x00,DA_RADIO | DISINT);            // 0x71; FISSA!!!!
					     		        wreg(SI4432_OPERATING_AND_FUNCTION_CONTROL_1,SI4432_TXON,DA_RADIO | DISINT);    // manda in trasmissione

      									DCOCTL  =   CALDCO_1MHZ;
        								BCSCTL1 =   CALBC1_1MHZ | XT2OFF;

									    stato = 10;
									    timeout = messaggio_rx.n.dati.i8[0] * (100000 / USEC15625);
									    tempo_led_on = 250;                    // on per 62 secondi, viene spento a fine portante
								    }
								    else
									    stato = 0;
						    }
						    off_adc();
					    }
				    }
				    else if ( timeout == 0)
				    {                             //radio non ok; per sicurezza di non lasciarlo attivo per  consumo
					    SDN_1;       			  //radio off

        			    #ifdef POTENZA_VARIABILE
    					     messaggio_rx.n.rssi = 0xff;                  //rssi = minimo perche non ha ricevuto risposta
        			    #endif

					    g_mancata_risposta();
					    stato = 0;
					    off_adc();
				    }
    			    break;

			case 10:
    				#ifdef GENERAPORTANTE
    					if (input_stato.b.tamper == 0)
    					{
    						SDN_1;       			  //radio off
    						stato = 0;
    						tempo_led_on = 1;           //spegne led
    					}
    				#else
    					if ( timeout == 0)// && (!input_stato.b.ampolla))
    					{
                            //non viene abilitato interr in tx continua			P2IE &= ~MASK_RADIO;      //disabilita gli interrrupts sulla radio
    						SDN_1;       			  //radio off
    						stato = 0;
    						tempo_led_on = 1;           //spegne led
    					}
    				#endif
        			break;

		}
		if (( stato == 2) || (stato== 3))
		{
			if (IRQ_RADIO_IN == 0)
					gestione_radio_ex_interr();
		}
		else
		{
			LPM3;
		}
	}   /********** FINE DEL LOOP PRINCIPALE **********/
}
/******************** FINE DEL MAIN **************************************/




/***************************************************************/
/****      FUNZIONI                                         ****/
/***************************************************************/

void riconoscimento_sensore_connesso(void)
{
/*    P1REN = BIT2;   // abilito pull-up sul pin da esaminare    
    P1OUT = BIT2;   // uscita alta sul pin da esaminare
    P1DIR &= ~BIT2; // pin da esaminare impostato in IN

    if((P1IN && BIT2) == 0)             // R5 = 0 Ohm, montaggio contatto
    {    
        tipo_connessione_sensore = 0; 
        P1DIR &= ~BIT1;                 // pin1 in IN per segnalare l'allarme
        P1IES &= ~BIT1;                 // transizione che genera l'interrupt è 0-1 
        P1IFG &= ~BIT1;                 // resetta il flag di interrupt
        P1IE |= BIT1;                   // abilita pin1 in interrupt 
    }    
    else                                // R5 non montata, montaggio seriale
    {*/
        tipo_connessione_sensore = 1;   
        
        TA1CTL |= TACLR;
        TA1CTL |= TASSEL_2 + MC_0 + ID_0; //TASSEL_2 + MC_0 + ID_0;      // con i 32kHz del quarzo esterno sarebbe TA1CTL |= TASSEL_1 + MC_0 + ID_0
        TA1CCR0 = 160;                    //4000;                        // con i 32kHz del quarzo esterno sarebbe TA1CCR0 = 160 per ottenere 5ms
        
        UCA0CTL1 |= UCSWRST;                    // reset UART interface
        
        UCA0CTL1 |= UCSSEL_2;                   // SMCLK: 1MHz
//        UCA0BR0 = 0xa0;                         // 8MHz 19200bps : UCABR0 = 416d = 1a0h
//        UCA0BR1 = 0x01;                         // 8MHz 19200bps              
//        UCA0MCTL = UCBRS2 + UCBRS0;             // UCBRSx = 6, UCBRFx = 0
        UCA0BR0 = 0x34;                         // 1MHz 19200bps : UCABR0 = 52d = 34h
        UCA0BR1 = 0x00;                         // 1MHz 19200bps
        UCA0MCTL = 0x00;                        // UCBRSx = 0, UCBRFx = 0
        
        P1DIR |= BIT2;
        //P1OUT |= BIT2;
        P1SEL = BIT1 + BIT2;                
        P1SEL2 = BIT1 + BIT2;

        UCA0CTL1 &= ~UCSWRST;                   // **Inizializza USCI state machine**
        UC0IE |= UCA0RXIE;                      // Abilita USCI_A0 RX interrupt
        sensore_tx(ack_str,6);              // ack
//    }                                                    
}

void init_sensors(void)
{
    int flag_test = 0;    
    switch(tipo_connessione_sensore)
    {
        case 0:                                             // contatto
                break;
        case 1:                                             // sensore volumetrico comunicante in UART
                /*int req_init_rx = 0;
                do
                {
                    while(flag_rx != 0);                    // aspetta di aver ricevuto un messaggio sulla UART                
                    flag_rx = 1;                            // resetta il flag che segnala la ricezione di un messaggio
                    sensore_tx(ack_str,6);                  // ack
                    if(rx_cmd == req_init)
                    {
                        while(flag_tx_finished == 0);       // aspetta che l'ack sia stato trasmesso
                        flag_tx_finished = 0;               // resetta flag che segnala la conclusione della trasmissione del messaggio su UART
                        __delay_cycles(2000);            
                        req_init_rx = 1;
                        sensore_tx(init_str,6);             // inizializzazione
                    }
                }while(req_init_rx == 0);
                __delay_cycles(20000);
                sensore_tx(GMN_str,5); 
                flag_rx = 1;                                // resetta il flag che segnala la ricezione di un messaggio            
                while(flag_rx != 0);                        // aspetta la ricezione di un messaggio sulla UART
                if(rx_cmd == GMN)
                {
                    switch(rx_mex[1])                       // rilevazione del modello di sensore OPTEX
                    {
                        case 1:                             // VXS-RAM
                                break;          
                        case 2:                             // VXS-RDAM
                                break;
                        case 3:                             // BXS-R
                                break;
                        case 4:                             // BXS-RAM
                                break;
                    }
                }
                break;*/

                while(flag_rx != 0);                    // aspetta di aver ricevuto un messaggio sulla UART  
                              
                flag_rx = 1;                            // resetta il flag che segnala la ricezione di un messaggio
                if(rx_valid_flag == 1)                  // il messaggio ricevuto non ha incontrato errori di trasmissione [controllo del checksum OK]
                {
                    switch(rx_cmd)
                    {
                        case ack:
                                                break;
                        case nack:                          // ritrasmetto l'ultimo messaggio
            //                                    sensore_tx();
                                                break;
                        case detector_events:
                                                if(tx_cmd != detector_events)
                                                    sensore_tx(ack_str,6);              // ack
                                                else                                    // evita che alla prossima ricezione possa catalogare un detector_events come risposta al mio vecchio comando                      
                                                    tx_cmd = IDLE;                                                                            
                                                break;
                        case req_init:          
                                                sensore_tx(ack_str,6);                  // ack
                                                while(flag_tx_finished == 0);           // aspetta che l'ack sia stato trasmesso
                                                flag_tx_finished = 0;                   // resetta il flag che segnala la conclusione della trasmissione del messaggio su UART
                                                __delay_cycles(2000);            
                                                sensore_tx(init_str,6);                 // inizializzazione
                                                tx_cmd = req_init;                      // DA SOSTITUIRE
                                                while(flag_tx_finished == 0);           // aspetta che l'inizializzazione sia stata trasmessa
                                                flag_tx_finished = 0;                   // resetta il flag che segnala la conclusione della trasmissione del messaggio su UART
                                                flag_test = 1;                                    
                                                break;
                        case GDE:
                                                break;
                        case GPageM:
                                                break;
                        case GPropertyM:
                                                break;
                        case GDP:
                                                break;
                        case GMN:
                                                switch(rx_mex[1])                       // rilevazione del modello di sensore OPTEX
                                                {
                                                    case 1:                             // VXS-RAM
                                                            break;          
                                                    case 2: P1OUT |= BIT6;              // VXS-RDAM
                                                            break;
                                                    case 3:                             // BXS-R
                                                            break;
                                                    case 4:                             // BXS-RAM
                                                            break;
                                                }
                                                break;
                        case GCV:
                                                break;     
                        default:                ;   
                    }
                }
                else                                    // checksum error
                {
        //            sensore_tx(nack);
        //            sensore_tx(last_cmd);
                    ;
                }
                if(flag_test == 1)
                {
                        while(rx_cmd != ack);
                        __delay_cycles(2000);
                        sensore_tx(GMN_str,5); 
                        flag_test = 0;
                }
                break;
    }
}

void sensore_tx(unsigned char string[11], unsigned int size)
{
        int i;
        switch(tipo_connessione_sensore)
        {
            case 0:         // connesione contatto
                    break;
            case 1:         // connessione seriale UART                   
                    for(i = 0; i < size; i++)       // copia la stringa da trasmettere nella stringa gestita dall'interrupt della UART
                        tx_string[i] = string[i]; 
                    string_size = size;      
                    UC0IE |= UCA0TXIE;              // abilita interrupt per la trasmissione dei caratteri su UART                                         
                    break;                   
        }        
}

unsigned int  decodifica_dati_rx_comun_perif(void)
{
		   unsigned int i;

		   i  = buffer_radio.dati[0] ;	//lunghezza dati
		   if ( i == (LUNGHEZZA_DATI_PERIFERICHE + 2))
		   {
				buffer_radio.timer_conc = (buffer_radio.dati[LUNGHEZZA_DATI_PERIFERICHE+2] << 8) + buffer_radio.dati[LUNGHEZZA_DATI_PERIFERICHE+1];  //prelieva il valore del timer del concentratore per sincronizzare sirena
				if (decrypta(&buffer_radio.dati[1], messaggio_rx.i8))
				{
			   		if (      (segmB.ser_num_perif[0] == messaggio_rx.n.id[0])
					       && (segmB.ser_num_perif[1] == messaggio_rx.n.id[1])
						   && (messaggio_rx.n.n_sequenza == dati_periferica.n.numero_sequenza_ultimo_tx +1)
					 	   && (    (segmC.snum_concentratore == 0xffff)                                  // e' vergine o riceve dal suo concentratore
							    || (segmC.snum_concentratore == messaggio_rx.n.snum_conc)
							  )
					   )
				    {
						switch ( messaggio_rx.n.tipo_messaggio)
			   			{
		   		   			case M_IN_COLLAUDO:
									if ( (messaggio_rx.n.dati.i16[2] > 20))
									{
										dati_periferica.n.mode_f = MP_IN_COLLAUDO;
										dati_periferica.n.cnt_sec_coll_man_inst = messaggio_rx.n.dati.i16[2];
									}
									else
									{
										if ( segmC.snum_concentratore == 0xffff)
											dati_periferica.n.mode_f = MP_FABBRICA;
										else
											dati_periferica.n.mode_f = MP_IN_SERVIZIO;
										dati_periferica.n.cnt_sec_coll_man_inst = 0;
									}
							return 1;

				   		   	case M_PORTANTE_ON:
							   if ( dati_periferica.n.mode_f == MP_IN_COLLAUDO)
							   {
									buffer_radio.flag.no_usa_calibrazione = messaggio_rx.n.dati.i8[1];
									dati_periferica.n.cnt_sec_coll_man_inst = messaggio_rx.n.dati.i16[2];
									return 2;
							   }
							   else
							   	   return 0;

				   		   	case M_WR_TARATURA:
							   if ( dati_periferica.n.mode_f == MP_IN_COLLAUDO)
							   {
									 segmB.taratura_radio = messaggio_rx.n.dati.i16[0];
									 dati_periferica.n.cnt_sec_coll_man_inst = messaggio_rx.n.dati.i16[2];
									 segmB.crc_taratura_radio_ser_num = 0xffff - (  segmB.ser_num_perif[0] + segmB.ser_num_perif[1] + segmB.taratura_radio
									 										+ segmB.tab_random.i16[0] + segmB.tab_random.i16[1]+ segmB.tab_random.i16[2]+ segmB.tab_random.i16[3]);
									 scrivi_flash( segmB.ser_num_perif, ADDR_SEGMENTO_B, (7+1));        //scrive 7 interi: segmB.ser_num_perif e segmB.taratura_radio e segmB.tab_random
									 prossimo_polling = M_RD_ACQUISITA;
									 flag.risponde_subito = 1;                        //risponde subito
									 return 1;
							   }
							   else
							   	   return 0;


							case M_WR_NUMERO_SERIE:
							   if ( dati_periferica.n.mode_f == MP_IN_COLLAUDO)
							   {
									 segmB.ser_num_perif[0] = messaggio_rx.n.dati.i16[0];
							         #ifdef BATTERIA_9V
									         segmB.ser_num_perif[1] = (PERIF_TIPO_CONTATTO_9V << 8) | messaggio_rx.n.dati.i8[2];
							         #else
									         segmB.ser_num_perif[1] = (PERIF_TIPO_CONTATTO << 8) | messaggio_rx.n.dati.i8[2];
							         #endif
									 segmB.tab_random.i16[0] = messaggio_rx.n.dati.i16[2];
									 segmB.tab_random.i16[1] = messaggio_rx.n.dati.i16[3];
									 segmB.tab_random.i16[2] = messaggio_rx.n.snum_conc;
									 segmB.tab_random.i8[6]  = messaggio_rx.n.rssi;
		                             #ifdef POTENZA_VARIABILE
								                             messaggio_rx.n.rssi = 0xff;                  //fa trasmettere a potenza max
		                             #endif
									 segmB.tab_random.i8[7]  = messaggio_rx.n.mode_canale;

									 segmB.crc_taratura_radio_ser_num = 0xffff - (  segmB.ser_num_perif[0] + segmB.ser_num_perif[1] + segmB.taratura_radio
									 										+ segmB.tab_random.i16[0] + segmB.tab_random.i16[1]+ segmB.tab_random.i16[2]+ segmB.tab_random.i16[3]);
									 scrivi_flash( segmB.ser_num_perif, ADDR_SEGMENTO_B, (7+1));        //scrive 7 interi: segmB.ser_num_perif e segmB.taratura_radio e segmB.tab_random
									 flag.risponde_subito = 1;                        //risponde subito
									 return 1;
							   }
							   else
							   	   return 0;

							case M_WR_PARAMETRI:
									 segmC.parametri_periferica.tempo_tappa    = messaggio_rx.n.dati.i8[0];
									 segmC.parametri_periferica.impulsi_tappa  = messaggio_rx.n.dati.i8[1];
									 segmC.parametri_periferica.p3		 = messaggio_rx.n.dati.i8[2];
									 segmC.parametri_periferica.p4	     = messaggio_rx.n.dati.i8[3];
									 segmC.parametri_periferica.p5			 = messaggio_rx.n.dati.i8[4];
									 segmC.parametri_periferica.p6			 = messaggio_rx.n.dati.i8[5];
									 segmC.parametri_periferica.libero         = messaggio_rx.n.dati.i8[6];
									 segmC.parametri_periferica.libero1        = NUMERO_PARAMETRI_PERIFERICA; // correzione per esigenze capella // = messaggio_rx.n.dati.i8[7];

									 segmC.crc_dati_install = 0;
									 segmC.crc_dati_install = calcola_crc_install();
									 scrivi_flash( &segmC.snum_concentratore, ADDR_SEGMENTO_C, 6);        //scrive 6 interi
									 prossimo_polling = M_RD_PARAMETRI;
									 flag.risponde_subito = 1;                        //risponde subito

    						    return 1;

							case M_RESET_TEMPO_BATT:
									 cnt_ore = 0;
									 dati_periferica.n.tempo_batteria = 0;
	   								 scrivi_flash(&dati_periferica.n.tempo_batteria,ADDR_SEGMENTO_D, 1);
									 prossimo_polling = M_RESET_TEMPO_BATT;
									 flag.risponde_subito = 1;                        //risponde subito
	    					    return 1;

							case M_P_ACQUISITA:
								 if(
									     (messaggio_rx.n.dati.i16[0] == messaggio_rx.n.snum_conc)  					//controlla campioni
									 &&  (messaggio_rx.n.dati.i16[2] == ~messaggio_rx.n.snum_conc)  				//controlla campioni
									 &&  (messaggio_rx.n.dati.i16[1] == ~messaggio_rx.n.dati.i16[3] )               //canale radio
									 &&  (messaggio_rx.n.snum_conc  != 0XFFFF)
								   )
								 {
										 segmC.snum_concentratore = messaggio_rx.n.dati.i16[0];
										 segmC.canale_radio       = messaggio_rx.n.dati.i16[1];
										 segmC.crc_dati_install = 0;
									 	 segmC.crc_dati_install = calcola_crc_install();
									 	 scrivi_flash( &segmC.snum_concentratore, ADDR_SEGMENTO_C, 6);        //scrive 6 interi
									 	 prossimo_polling = M_RD_PARAMETRI ; //M_RD_ACQUISITA;
									 	 flag.risponde_subito = 1;                        //risponde subito
										 TA0CCTL1 = CCIE;                       // CCR1 interrupt enabled
						    		 	 return 1;
								 }
								 else
										return 0;

							case M_P_ELIMINATA:
									 if(     (segmC.snum_concentratore != 0xffff)                             //non e' vergine o riceve dal suo concentratore
										 &&  (messaggio_rx.n.dati.i16[0] == 0xffff)   //controlla campioni
										 &&  (messaggio_rx.n.dati.i16[2] == ~0xffff)  //controlla campioni
										 &&  (messaggio_rx.n.dati.i16[1] == ~messaggio_rx.n.dati.i16[3] )              //canale radio
										 &&  (messaggio_rx.n.dati.i16[1] == CANALE_BASE)             				   //canale radio
									   )
									 {
										 segmC.snum_concentratore = 0XFFFF;
										 segmC.canale_radio       = CANALE_BASE;
									 	 segmC.crc_dati_install = 0;
									 	 segmC.crc_dati_install = calcola_crc_install();
									 	 scrivi_flash( &segmC.snum_concentratore, ADDR_SEGMENTO_C, 6);        //scrive 6 interi
//									 	 prossimo_polling = M_RD_ACQUISITA;
									 	 flag.risponde_subito = 1;                        //cosi se il concentratore e' in manutenzione viene subito registrata in rpl
										 TA0CCTL1 = 0;                       // CCR1 interrupt DISenabled
									    return 1;
									 }
								 	 else
										return 0;

							case M_IN_MANUTENZIONE:
									if ( messaggio_rx.n.dati.i16[2] > 20)
									{
										dati_periferica.n.mode_f = MP_IN_MANUTENZIONE;
										dati_periferica.n.cnt_sec_coll_man_inst = messaggio_rx.n.dati.i16[2];
									}
									else
									{
										if ( dati_periferica.n.mode_f == MP_IN_MANUTENZIONE)
												flag.risponde_subito = 1;                        //risponde subito
										if ( segmC.snum_concentratore == 0xffff)
											dati_periferica.n.mode_f = MP_FABBRICA;
										else
											dati_periferica.n.mode_f = MP_IN_SERVIZIO;
										dati_periferica.n.cnt_sec_coll_man_inst = 0;
									}
	    						return 1;

							case M_POLLING_C:
								if (segmC.snum_concentratore == messaggio_rx.n.snum_conc)
								{
									n_concentratore_in_impianto = (messaggio_rx.n.mode_canale & 0x3);       //da concentratore a perif  viene usato per altri  dati
									flag.stato_manutenzione_b   = ((messaggio_rx.n.mode_canale & 0x8) >> 3);   		//da concentratore a perif  viene usato per altri  dati

									carica_timer_rx_sincrono();
									dati_periferica.n.mode_f = MP_IN_SERVIZIO;
									dati_periferica.n.cnt_sec_coll_man_inst = 0;
								}
	    						return 1;

				   		   	case M_RD_PARAMETRI:
									 prossimo_polling = M_RD_PARAMETRI;
									 flag.risponde_subito = 1;                        //risponde subito
	    					    return 1;

				   		   	case M_RD_ACQUISITA:
									 prossimo_polling = M_RD_ACQUISITA;
									 flag.risponde_subito = 1;                        //risponde subito
	    					    return 1;

				   		   	case M_CM_CLEAR_DIAGNOSI_RADIO:
									 dati_periferica.n.cnt_err_superif_comunicazione = 0;
									 dati_periferica.n.cnt_err_superif_secondi_tentativi = 0;
									 dati_periferica.n.stato_new.n.guasto = 0;

									 cnt_err_superif_resetradio = 0;
									 cnt_err_superif_no_tx_radio = 0;
			                        #ifdef ALTRE_MISURE
	                                    dati_periferica.n.rssi_rx_max = 0;
            	                        dati_periferica.n.rssi_rx_min = 0xff;
			                        #endif
									 prossimo_polling = M_CM_CLEAR_DIAGNOSI_RADIO;
									 flag.risponde_subito = 1;                        //risponde subito
    						    return 1;


						  default:

							if ( segmC.snum_concentratore == 0xffff)
									dati_periferica.n.mode_f = MP_FABBRICA;
							else
									dati_periferica.n.mode_f = MP_IN_SERVIZIO;
							dati_periferica.n.cnt_sec_coll_man_inst = 0;

    						return 1;
						}
					}
				}
		   }
		   return 0;
}

void carica_timer_rx_sincrono(void)
{
	unsigned int tempn,temp;
		temp = ((TA0CCR1 - buffer_radio.timer_perif) + ANTICIPO);        		//tempo da sincronispo "su periferica"
		//sincronizza periferica
		tempn = (buffer_radio.timer_perif + buffer_radio.timer_conc - ANTICIPO);
		if (( tempn - TA0R) < 0x8000)
			TA0CCR1 = tempn;					                        //   TIMER PER RICEZIONE SINCRONA
		//eventuale correzione errore periodo
		if (temp < buffer_radio.timer_conc)
		{
			  if ( (buffer_radio.timer_conc - temp)  > 6)               // unita da 30,52 usec
			  {
					if      (periodo_rx_sincrona[0] < periodo_rx_sincrona[1])
						periodo_rx_sincrona[0]++;
					else if (periodo_rx_sincrona[1] < periodo_rx_sincrona[2])
						periodo_rx_sincrona[1]++;
					else if (periodo_rx_sincrona[2] < periodo_rx_sincrona[3])
						periodo_rx_sincrona[2]++;
					else //if (periodo_rx_sincrona[3] < periodo_rx_sincrona[4])
						periodo_rx_sincrona[3]++;
			  }
		}
		else if ( (temp - buffer_radio.timer_conc)  > 6)
		{
					if      (periodo_rx_sincrona[0] > periodo_rx_sincrona[1])
						periodo_rx_sincrona[0]--;
					else if (periodo_rx_sincrona[1] > periodo_rx_sincrona[2])
						periodo_rx_sincrona[1]--;
					else if (periodo_rx_sincrona[2] > periodo_rx_sincrona[3])
						periodo_rx_sincrona[2]--;
					else //if (periodo_rx_sincrona[3] > periodo_rx_sincrona[4])
						periodo_rx_sincrona[3]--;
		}
}


unsigned int calcola_crc_install(void)
{
#if 0
	unsigned int temp;
	temp     =  0x5a69
			    ^  segmC.snum_concentratore
				^ *((unsigned int*)&segmC.canale_radio)
				^ *((unsigned int*)&segmC.parametri_periferica.tempo_tappa)
				^ *((unsigned int*)&segmC.parametri_periferica.click_ths)
				^ *((unsigned int*)&segmC.parametri_periferica.p5)
				^ *((unsigned int*)&segmC.parametri_periferica.libero);
#else
	unsigned int temp, i;
	temp     =  0x5a69;
	for(i=0; i<(sizeof(segmC)/2); i++) temp ^= segmC.p[i];
#endif

   return ((temp >> 8) ^ (temp & 0xff));
}


void ricarica_cnt_sono_vivo(void)
{
	 if ( flag.risponde_subito)
	 {
	 	dati_periferica.n.cnt_sono_vivo = 1 +(500000/USEC15625);
	 	flag.risponde_subito = 0;
	 }
	 else if (   (dati_periferica.n.mode_f == MP_IN_COLLAUDO)
	         )
	 	dati_periferica.n.cnt_sono_vivo = 1 + (1250000 / USEC15625);

	 else if (//   (dati_periferica.n.mode_f  == MP_IN_INSTALLAZIONE) ||
	           (dati_periferica.n.mode_f  == MP_IN_MANUTENZIONE)
	         )
	 	dati_periferica.n.cnt_sono_vivo = 1 + (2750000 / USEC15625) + segmB.tab_random.i8[0x7 & (pnt_tab_random++)];

	 else
	 	dati_periferica.n.cnt_sono_vivo = 1+(120000000 / USEC15625) + segmB.tab_random.i8[0x7 & (pnt_tab_random++)];

//togliere
					 dati_periferica.n.stato_new.n.guasto = 0;

}



void g_mancata_risposta(void)
{
	if(   ( dati_periferica.n.mode_f == MP_IN_MANUTENZIONE)
	   ||( dati_periferica.n.mode_f == MP_IN_COLLAUDO)
	  )
	{                                                        //in manutenzione e collaudo non fa ripetizioni
		dati_periferica.n.stato_rx.i8 = dati_periferica.n.stato_new.i8;       //annulla differenze per non ripartire con tx
		cnt_ripetizione_tx = MAX_RIPETIZIONI;
	   	flag.comunicazione_in_corso = 0;
		ricarica_cnt_sono_vivo();
	}
	else
	{
			if ( dati_periferica.n.cnt_err_superif_secondi_tentativi < 0xffff)
				dati_periferica.n.cnt_err_superif_secondi_tentativi++;
			if ( cnt_ripetizione_tx)
			{
		 		dati_periferica.n.cnt_sono_vivo = 1+(1250000 / USEC15625) + segmB.tab_random.i8[0x7 & (pnt_tab_random++)];
				if (--cnt_ripetizione_tx == (MAX_RIPETIZIONI - 2))
				{
						if ( segmC.snum_concentratore == 0XFFFF)
							dati_periferica.n.numero_sequenza_ultimo_tx = (dati_periferica.n.numero_sequenza_ultimo_tx + 2 ) & 0x7ffe;    //incrementa numer sequenza dopo risposta ok (periferica solo numeri pari
						else
							dati_periferica.n.numero_sequenza_ultimo_tx = ((dati_periferica.n.numero_sequenza_ultimo_tx + 2) | 0x8000) & 0xfffe;    //incrementa numer sequenza dopo risposta ok
				}
			}
			else
			{
				if ( dati_periferica.n.cnt_err_superif_comunicazione < 0xffff)
					dati_periferica.n.cnt_err_superif_comunicazione++;
				dati_periferica.n.stato_rx.i8 = dati_periferica.n.stato_new.i8;       //annulla differenze per non ripartire con tx
				cnt_ripetizione_tx = MAX_RIPETIZIONI;
			   	flag.comunicazione_in_corso = 0;
				ricarica_cnt_sono_vivo();
			}

			#ifdef POTENZA_VARIABILE
			if (buffer_radio.flag.reset)
	   		{
				dati_periferica.n.ultima_pw_tx = POTENZA_MIN;
			}
			else if (dati_periferica.n.ultima_pw_tx < POTENZA_MAX)
			{
					dati_periferica.n.ultima_pw_tx++;
			}

			#endif
    }
}


//-----------------------------------
// LEGGE DALLA FLASH I DATI DI CONFIGURAZIONE
void leggi_flash(unsigned int * pnt_src, unsigned int * pnt_dest, unsigned int numero)
{
    unsigned int iii;
    for (iii =0; iii<numero; iii++)
    {
        *pnt_dest = *pnt_src;
        pnt_src++;
        pnt_dest++;
    }
}


//-----------------------------------
// SCRIVE NELLA FLASH I DATI DI CONFIGURAZIONE
void scrivi_flash(unsigned int * pnt_src, unsigned int * pnt_dest, unsigned int numero)
{
    unsigned int iii;

    _DINT();                     // DISABILITA GLI interrupts
    CLR_WDT;
    FCTL2 = FWKEY + FSSEL_1 + 2 ;      // IMPOSTA LA FREQUENZA DEL FLASH TIMING GENERATOR f(FTG) = MCLK/3  = 333kHz

//    CANCELLA IL SEGMENTO..
//    FCTL2 = 0x0A540;     // MCLK/1

	FCTL3 = FWKEY + 00;     		/* Lock = 0 */
    FCTL1 = FWKEY + ERASE;     		/* ERASE = 1 */
    *pnt_dest = 0;       			/* erase Flash segment */
    FCTL1 = FWKEY + 00;     		/* ERASE = 0 */
    FCTL3 = FWKEY + LOCK;     		/* Lock = 1 */
    CLR_WDT;
    for (iii =0; iii<numero; iii++)
    {
        FCTL3 = FWKEY + 00;         /* Lock = 0 */
        FCTL1 = FWKEY + WRT;        /* WRT = 1 */
        *pnt_dest = *pnt_src;    	/* program Flash word */
        FCTL1 = FWKEY + 00;         /* WRT = 0 */
        FCTL3 = FWKEY + LOCK;       /* Lock = 1 */
        pnt_src++;
        pnt_dest++;
  	 CLR_WDT;
	}
    TA0CCR0 = TAR + 512;          // RICARICO PER EVITARE CHE MI SCAPPI!
    _EINT();                     // Enable interrupts
}

//------------------------------------------------------------------------------
/* taratura    ***** ESEGUE UN CICLO DI TARATURA DELLA FREQUENZA BASE  *****
------------------------------------------------------------------------------*/
/*
void taratura (void)
{
    unsigned char oldinput,newinput;
    unsigned int cnt_input;

    _DINT();                // DISABILITA GLI interrupts per evitare che l'interrupt di TA0 mi riattivi il WDT
    STOP_WDT;
    LED_A_ON;
    SPI_CS_0;               // Mette il Chip Select (attivo basso)
    while(SPI_IN);          // aspetta che il chipcon si svegli

    tx_byte(CHANNR);        // COMANDO DI SCRITTURA CANALE
    tx_byte(0X00);          // AZZERA IL CANALE ATTUALE

    oldinput = P1IN & 0x60; //MASCHERA su p1.5 e p1.6
    cnt_input = 6200;

    while(P1IN & 0X10)      // RIMANE IN TRASMISSIONE FINO AL RILASCIO
    {
        tx_byte(SCAL);          // comando di  CALIBRAZIONE MANUALE NOTA BENE: lui continua indisturbato fino alla fine calibrazione e poi passa in tx
        tx_byte(SFTX);          //  PULISCE IL buffer di uscita
        tx_byte(STX);           //  MANDA IN TX
        while((cnt_input)&&(P1IN & 0X10))           // RIMANE  in trasmissione fissa in attesa dei comandi up-dw
        {
            newinput = P1IN & 0x60;                 //MASCHERA su p1.5 e p1.6
            if(newinput != oldinput) cnt_input--;
            else cnt_input = 6200;
        }
        cnt_input = 6200;
        oldinput = newinput;
        if (newinput & 0x20) FRQOFFSET++;           // PIN P1.5
        if (newinput & 0x40) FRQOFFSET--;           // PIN P1.6
        // Aggiorno l'offset di frequenza
        tx_byte(SIDLE);         //  MANDA IN IDLE
        tx_byte(FSCTRL0);       // COMANDO DI SCRITTURA OFFSET DI FREQUENZA
        tx_byte(FRQOFFSET);     //  carica il valore appena aggiornato
    }
    tx_byte(SIDLE);             //  MANDA IN IDLE
    tx_byte(CHANNR);            // COMANDO DI SCRITTURA CANALE
    tx_byte(CANALE_L);          // ri - CARICA IL CANALE di lavoro MEM. IN FLASH
    tx_byte(SPWD);              //  SPEGNE
    SPI_CS_1;                   // TOGLIE il Chip Select
    scrivi_flash_conf();        // AGGIORNA LA FLASH
    LED_A_OFF;
}

*/

void format_dati_polling()
{
		messaggio_tx.n.n_sequenza = dati_periferica.n.numero_sequenza_ultimo_tx;
		messaggio_tx.n.stato_allarmi.i8 = dati_periferica.n.stato_new.i8;
		messaggio_tx.n.id[0] = segmB.ser_num_perif[0];
		messaggio_tx.n.id[1] = segmB.ser_num_perif[1];
		messaggio_tx.n.snum_conc = segmC.snum_concentratore; //dati_periferica.n.tempo_batteria;
		messaggio_tx.n.rssi = dati_periferica.n.rssi_rx;
		messaggio_tx.n.mode_canale = dati_periferica.n.mode_f | (segmC.canale_radio <<4 );

		messaggio_tx.n.tipo_messaggio = prossimo_polling;

		switch ( prossimo_polling)
		{
				case M_RD_PARAMETRI:

			    #ifdef ALTRE_MISURE
					       messaggio_tx.n.dati.i8[0] = 0xff; //cnt_err_superif_crc_radio    ;
					       messaggio_tx.n.dati.i8[1] = dati_periferica.n.ultima_pw_tx;
					       messaggio_tx.n.dati.i8[2] = buffer_radio.rumore_rx0;     //segmC.parametri_periferica.click_ths_lo   ;
					       messaggio_tx.n.dati.i8[3] = buffer_radio.rumore_rx1;     //segmC.parametri_periferica.click_ths_hi   ;
					       messaggio_tx.n.dati.i8[4] = buffer_radio.rumore_rx2;     //segmC.parametri_periferica.mov_ths_lo     ;
					       messaggio_tx.n.dati.i8[5] = buffer_radio.rumore_rx3;     //segmC.parametri_periferica.mov_ths_hi     ;
					       messaggio_tx.n.dati.i8[6] = dati_periferica.n.rssi_rx_max; //segmC.parametri_periferica.libero         ;
					       messaggio_tx.n.dati.i8[7] = dati_periferica.n.rssi_rx_min; //segmC.parametri_periferica.libero1        ;
			    #else
					       messaggio_tx.n.dati.i8[0] = segmC.parametri_periferica.tempo_tappa    ;
					       messaggio_tx.n.dati.i8[1] = segmC.parametri_periferica.impulsi_tappa  ;
					       messaggio_tx.n.dati.i8[2] = segmC.parametri_periferica.p3	   ;
					       messaggio_tx.n.dati.i8[3] = segmC.parametri_periferica.p4		   ;
					       messaggio_tx.n.dati.i8[4] = segmC.parametri_periferica.p5			   ;
					       messaggio_tx.n.dati.i8[5] = segmC.parametri_periferica.p6			   ;
					       messaggio_tx.n.dati.i8[6] = segmC.parametri_periferica.libero         ;
					       messaggio_tx.n.dati.i8[7] = NUMERO_PARAMETRI_PERIFERICA; // correzione per esigenze capella // = segmC.parametri_periferica.libero1        ;
			    #endif
				break;

				case M_RD_ACQUISITA:
					   messaggio_tx.n.dati.i16[0] = segmB.taratura_radio;
					   messaggio_tx.n.dati.i16[1] = 0xffff;
					   messaggio_tx.n.dati.i16[2] = 0xffff;
					   messaggio_tx.n.dati.i8[6]  = 0xff;
					   messaggio_tx.n.dati.i8[7]  = 0xff;      //su spare.i8[5]
				break;

				case M_RD_VERSIONE_FW:
					   messaggio_tx.n.dati.i8[0] = versione[0];
					   messaggio_tx.n.dati.i8[1] = versione[1];
					   messaggio_tx.n.dati.i16[1] = dati_periferica.n.cnt_err_superif_comunicazione;
					   messaggio_tx.n.dati.i16[2] = dati_periferica.n.cnt_err_superif_secondi_tentativi;
					   messaggio_tx.n.dati.i16[3] = 0xffff;;
				break;


				case M_CM_CLEAR_DIAGNOSI_RADIO:                //non trasmessa ciclicamente
					   messaggio_tx.n.dati.i16[0] = dati_periferica.n.cnt_err_superif_comunicazione;
					   messaggio_tx.n.dati.i16[1] = dati_periferica.n.cnt_err_superif_secondi_tentativi;
					   messaggio_tx.n.dati.i16[2] = 0xffff; //dati_periferica.n.cnt_err_superif_decrypta;
					   messaggio_tx.n.dati.i16[3] = 0xffff;
				break;

				case M_RESET_TEMPO_BATT:                       //non trasmessa ciclicamente
					   messaggio_tx.n.dati.i16[0] = dati_periferica.n.tempo_batteria;
					   messaggio_tx.n.dati.i16[1] = 0xffff;
					   messaggio_tx.n.dati.i16[2] = 0xffff;
					   messaggio_tx.n.dati.i16[3] = 0xffff;
				break;

				default:
					   messaggio_tx.n.tipo_messaggio = M_POLLING_P;

					   messaggio_tx.n.dati.i16[0] = dati_periferica.n.tempo_batteria;
					   messaggio_tx.n.dati.i8[2] = lev_batteria;

					   messaggio_tx.n.dati.i8[3]   = dati_periferica.n.ultima_pw_tx; //(cnt_err_superif_resetradio & 0xff);           //su spare.i8[4]
					   messaggio_tx.n.dati.i16[2]  = cnt_err_superif_resetradio;  //input_stato.i8;        //su spare.i16[0]
					   messaggio_tx.n.dati.i16[3]  = cnt_err_superif_no_tx_radio; //rx_acc_click;       //su spare.i16[1]
				break;

		}
		switch (dati_periferica.n.numero_sequenza_ultimo_tx & 0x6)
		{
			   	case 0:
				   		   prossimo_polling = M_POLLING_P;
				break;

			   	case 2:
						   prossimo_polling = M_RD_ACQUISITA;
				break;

			   	case 4:
						   prossimo_polling = M_RD_VERSIONE_FW;
				break;

				case 6:
						   prossimo_polling = M_RD_PARAMETRI;
				break;
		}
}




// PER RADIO
void tx_polling(void)
{
	   unsigned char buffer_c[20];

	   //messi qui per non farli ogni giro di main >>

	   if ( dati_periferica.n.mode_f == MP_IN_COLLAUDO)
	   			tempo_led_on = 1 + (4000/250);

	   if ( scrivi_tempo_batteria_pnd)
	   {
	   		scrivi_tempo_batteria_pnd = 0;
	   		scrivi_flash(&dati_periferica.n.tempo_batteria,ADDR_SEGMENTO_D, 1);
	   }
	   //messi qui per non farli ogni giro di main <<

	   radio_on();

	   flag.comunicazione_in_corso = 1;
	   format_dati_polling();
	   encrypta(messaggio_tx.n.n_sequenza,&messaggio_tx.i8[2], buffer_c);   //  (unsigned short sequenza,unsigned char * dati,unsigned char *dati_cryp)

	   buffer_radio.flag.endtx = 0;

	   init_radio(segmC.canale_radio);

	   wreg(SI4432_TRANSMIT_PACKET_LENGTH,20,DA_RADIO | DISINT); 	   	 	 	// reg 0x3E; LUNGHEZZA DEL PACCHETTO TRASMESSO
	   tx_byte(buffer_c,SI4432_FIFO_ACCESS,20,DA_RADIO | DISINT);		        // carica la fifo

	   wreg(SI4432_OPERATING_AND_FUNCTION_CONTROL_1,SI4432_TXON | SI4432_XTON | SI4432_PLLON,DA_RADIO | DISINT);      //REG 0X07

	   buffer_radio.dati_rdy = 0;
	   buffer_radio.flag.reset = 0;

	DCOCTL  =   CALDCO_1MHZ;
    BCSCTL1 =   CALBC1_1MHZ | XT2OFF;

}


const unsigned char config_05_0c[] =  {
	0X00,                                                             //reg 0x05, SI4432_INTERRUPT_ENABLE_1; 				  clear interrupt enable 1
	0X00,                                                             //reg 0x06, SI4432_INTERRUPT_ENABLE_2; 				  clear interrupt enable 2
	SI4432_XTON,                                                      //reg 0x07; SI4432_OPERATING_AND_FUNCTION_CONTROL_1	  ready mode
	SI4432_RXMULTIPK | SI4432_RXFIFORESET | SI4432_TXFIFORESET,       //reg 0x08; SI4432_OPERATING_AND_FUNCTION_CONTROL_2     COMANDO DI CANCELLAZIONE DELLa rx FIFO, BISOGNA METTERE A 1.....
	0xb9,                                                             //reg 0x09; (SI4432_CRYSTAL_OSCILLATOR_LOAD_CAPACITANCE Adjust Crystal for zero freq error
	0x07,                                                             //reg 0x0a; SI4432_MICROCONTROLLER_OUTPUT_CLOCK	      (facoltativo)
	0x12,                                                             //reg 0x0b; SI4432_GPIO0_CONFIGURATION                  gpio0 tx stato
	0x15,                                                             //reg 0x0c; SI4432_GPIO1_CONFIGURATION				  gpio1 rx STATE
//	0x1F,                                                             //reg 0x0d; SI4432_GPIO2_CONFIGURATION                  gpio2 gnd
};

const unsigned char config_1C_25[] =  {
    //set Modem parameters
	#ifdef KBS100   //   100kbs
	// *********** VERSIONE UNGHERESE!!!!! PROVATA OK DA 90 A 110 Kb !!!!!!!!!!  *****************
		0x81,       // reg 0x1C, SI4432_IF_FILTER_BANDWIDTH
		0x40,       // reg 0x1D, SI4432_AFC_LOOP_GEARSHIFT_OVERRIDE
		0x0A,       // reg 0x1E, SI4432_AFC_TIMING_CONTROL
		0x00,       // reg 0x1F, SI4432_CLOCK_RECOVERY_GEARSHIFT_OVERRIDE
		0x78,       // reg 0x20, SI4432_CLOCK_RECOVERY_OVERSAMPLING_RATIO
		0x01,       // reg 0x21, SI4432_CLOCK_RECOVERY_OFFSET_2
		0x11,       // reg 0x22, SI4432_CLOCK_RECOVERY_OFFSET_1
		0x11,       // reg 0x23, SI4432_CLOCK_RECOVERY_OFFSET_0
		0x14,       // reg 0x24, SI4432_CLOCK_RECOVERY_TIMING_LOOP_GAIN_1
		0x46,       // reg 0x25, SI4432_CLOCK_RECOVERY_TIMING_LOOP_GAIN_0
	#else      //50kbs
	  	0x05, //  0x07, //  00xae,       // reg 0x1C, SI4432_IF_FILTER_BANDWIDTH
  	  	0x44, //  0x40, //  00x44,       // reg 0x1D, SI4432_AFC_LOOP_GEARSHIFT_OVERRIDE
  		0x0A, //  0x0A, //  00x0a,       // reg 0x1E, SI4432_AFC_TIMING_CONTROL
  		0x03, //  0x00, //  00x00,       // reg 0x1F, SI4432_CLOCK_RECOVERY_GEARSHIFT_OVERRIDE
  		0x50, //  0x50, //  00x3c,       // reg 0x20, SI4432_CLOCK_RECOVERY_OVERSAMPLING_RATIO
  		0x01, //  0x01, //  00x02,       // reg 0x21, SI4432_CLOCK_RECOVERY_OFFSET_2
  		0x99, //  0x99, //  00x22,       // reg 0x22, SI4432_CLOCK_RECOVERY_OFFSET_1
  		0x9a, //  0x9a, //  00x22,       // reg 0x23, SI4432_CLOCK_RECOVERY_OFFSET_0
  		0x06, //  0x02, //  00x14,       // reg 0x24, SI4432_CLOCK_RECOVERY_TIMING_LOOP_GAIN_1
  		0x68, //  0x24, //  00x46,       // reg 0x25, SI4432_CLOCK_RECOVERY_TIMING_LOOP_GAIN_0
	#endif
};



const unsigned char config_33_3E[] =  {
  //set sync
   SI4432_SYNCLEN_2BYTE,    	  //REG 0X33, SI4432_HEADER_CONTROL_2			 lunghezza pacchetto variabile SI4432_SYNCLEN_MASK((xx -1)<<1) (=0x0a)
  //set preambolo
   0X0C,                          //reg 0x34, SI4432_PREAMBLE_LENGTH			 LUNGHEZZA DEL PREAMBOLO TRASMESSO, in nibble
   (NIBBLE_RX_PREAMB << 3) + 2,	  //REG 0x35, SI4432_PREAMBLE_DETECTION_CONTROL	 LUNGHEZZA DEL PREAMBOLO ATTESO--- 20 BIT + OFFSET RSSI

   0x54,        				  //reg 0x36, SI4432_SYNC_WORD_3				 SYNC WORD 3
   0x3D,        				  //reg 0x37, SI4432_SYNC_WORD_2				 SYNC WORD 2
};

void radio_on()
{
		    temp_main = TA0R;
			while ((TA0R - temp_main)  < 20000/30)
			{
				SDN_0;                  //  TOGLIE LO SHUT DOWN al SI4432  e attende 20 msec
			    CLR_WDT;
			}

	        DCOCTL =   CALDCO_8MHZ;     //CALIBRAZIONE DEL DCO A 8 MHz (deve andare A PILA!)
            BCSCTL1=   CALBC1_8MHZ  | XT2OFF;

			wreg(SI4432_CRYSTAL_OSCILLATOR_LOAD_CAPACITANCE,0xb9,DA_RADIO | DISINT);           //reg 0x09; //Adjust Crystal for zero freq error
			wreg(SI4432_OPERATING_AND_FUNCTION_CONTROL_1, SI4432_XTON, DA_RADIO | DISINT);     //reg 0x07; //ready mode
}


void init_radio(unsigned char canale)
{

		unsigned char temp[11];
		unsigned char j;

			// prepara dati su temp e li usa dopo dopo
			// Potenza di uscita
			#ifdef POTENZA_VARIABILE
				if ( segmC.snum_concentratore == 0XFFFF)
					 dati_periferica.n.ultima_pw_tx = POTENZA_MAX			  ; 			// reg 0x6D; SI4432_TX_POWER					  1C = 11  1D = 14; 1f = 20db
				else
				{
					if (messaggio_rx.n.rssi > SOGLIA_RSSI_MIN)
					{
						if (dati_periferica.n.ultima_pw_tx < POTENZA_MAX)
						{
							dati_periferica.n.ultima_pw_tx++;
						}
					}
					else if (messaggio_rx.n.rssi < SOGLIA_RSSI_MAX)
					{
						if (dati_periferica.n.ultima_pw_tx > POTENZA_MIN)
						{
							dati_periferica.n.ultima_pw_tx--;
						}
					}
				}
				temp[0] = dati_periferica.n.ultima_pw_tx;

			#else
				temp[0] = POTENZA_MAX  				  ; 			// reg 0x6D; SI4432_TX_POWER					  1C = 11  1D = 14; 1f = 20db
			#endif
	        #ifdef KBS100
			        temp[1] = 0x19  	  			  		  ; // reg 0x6E; SI4432_TX_DATA_RATE_1				  DATA RATE 100k
			        temp[2] = 0x9a  	  			  		  ; // reg 0x6f; SI4432_TX_DATA_RATE_0				  DATA RATE 100k
			        temp[3] = 0x0C | SI4432_ENWHITE  		  ; // reg 0x70; SI4432_MODULATION_MODE_CONTROL_1 	  DATA RATE 100k
			        temp[4] = 0x23       			  		  ; // reg 0x71; SI4432_MODULATION_MODE_CONTROL_2     segnali non invertiti
			        temp[5] = 0x50  	  			  		  ; // reg 0x72; SI4432_FREQUENCY_DEVIATION			  PROF. MODULAZIONE 50kHz
	        #else
			        temp[1] = 0x0c   	  			  		  ; // reg 0x6E; SI4432_TX_DATA_RATE_1				  DATA RATE 150
			        temp[2] = 0xcd   	  			  		  ; // reg 0x6f; SI4432_TX_DATA_RATE_0				  DATA RATE 50k
			        temp[3] = 0x04 | SI4432_ENWHITE  		  ; // reg 0x70; SI4432_MODULATION_MODE_CONTROL_1 	  DATA RATE 50k
			        temp[4] = 0x23        			  		  ; // reg 0x71; SI4432_MODULATION_MODE_CONTROL_2     segnali non invertiti
			        temp[5] = 0x28; //0x50    	  			  ; // reg 0x72; SI4432_FREQUENCY_DEVIATION			  PROF. MODULAZIONE 50kHz
	        #endif
			// FREQUENZA
 			if (( dati_periferica.n.mode_f == MP_IN_COLLAUDO) && (buffer_radio.flag.no_usa_calibrazione))
 			{
 				buffer_radio.flag.no_usa_calibrazione = 0;
 				temp[6] = 0; 			// reg 0x73; SI4432_FREQUENCY_OFFSET_1			  frequenza offset
 				temp[7] = 0; 			// reg 0x74; SI4432_FREQUENCY_OFFSET_2			  frequenza offset
 			}
 			else
			{
				temp[6] = segmB.taratura_radio       	      ; 			// reg 0x73; SI4432_FREQUENCY_OFFSET_1			  frequenza offset
				temp[7] = (segmB.taratura_radio >> 8) & 0X03; 			    // reg 0x74; SI4432_FREQUENCY_OFFSET_2			  frequenza offset
			}

			temp[8] = BANDA_RD			    		  ;                     // reg 0x75; SI4432_FREQUENCY_BAND_SELECT		  banda

			temp[9] = step_fre_radio[canale] >>8 	 ;    		            // reg 0x76; SI4432_NOMINAL_CARRIER_FREQUENCY_1   frequenza
			temp[10]= step_fre_radio[canale] & 0XFF ;     	                // reg 0x77; SI4432_NOMINAL_CARRIER_FREQUENCY_0   frequenza

//attesa radio ready
			j =0x0;
			while ((j & SI4432_PWST_MASK ) != (1 << 5))                     //test se ready
			{
			   rx_byte(&j,SI4432_CRYSTAL_OSCILLATOR_CONTROL_TEST,1,DA_RADIO | DISINT);
			}

			tx_byte((unsigned char*)config_05_0c,SI4432_INTERRUPT_ENABLE_1,8,DA_RADIO | DISINT);   //da reg 0x05 a reg 0x0c

	        #ifdef KBS100
			        wreg(SI4432_CHARGEPUMP_CURRENT_TRIMMING_OVERRIDE,0xC0,DA_RADIO | DISINT);       // reg 0x58; DATA RATE 100k
	        #else
			        wreg(SI4432_CHARGEPUMP_CURRENT_TRIMMING_OVERRIDE,0x80,DA_RADIO | DISINT);       // reg 0x58; DATA RATE 50k
	        #endif

			tx_byte(temp,SI4432_TX_POWER,11,DA_RADIO | DISINT);   		//da reg 0x6D; SI4432_TX_POWER a reg 0x77; SI4432_NOMINAL_CARRIER_FREQUENCY_0

			tx_byte((unsigned char*)config_1C_25,SI4432_IF_FILTER_BANDWIDTH,10,DA_RADIO | DISINT);   //da reg 0x1C a reg 0x25

	        #ifdef KBS100
			        wreg(SI4431_AFC_LIMIT						 	,0x50,DA_RADIO | DISINT);       // reg 0x2A;
			        wreg(SI4432_AGC_OVERRIDE_1					 	,0x60,DA_RADIO | DISINT);       // reg 0x69;
	        #else
			        wreg(SI4431_AFC_LIMIT						 	,0x28,DA_RADIO | DISINT);       // reg 0x2A;   0x00      //50kbs
			        wreg(SI4432_AGC_OVERRIDE_1					 	,0x60,DA_RADIO | DISINT);       // reg 0x69;   0x60      //50kbs
	        #endif
		            wreg(SI4432_DATA_ACCESS_CONTROL, SI4432_ENPACRX | SI4432_ENPACTX | SI4432_ENCRC,DA_RADIO | DISINT);//0XA8);    // reg 0x30;

//PROVARE A TOGLIERLO
//			wreg(SI4432_HEADER_CONTROL_1				,0X8C);    // reg 0x32;  non ce ne frega niente!

			tx_byte((unsigned char*)config_33_3E,SI4432_HEADER_CONTROL_2,5,DA_RADIO | DISINT);   //da reg 0x33 a reg 0x3E

//set soglie fifo
			temp[0] = 62; 					 // reg 0x7c; SI4432_TX_FIFO_CONTROL_2; per evitare interrupt soglia tx alf empty
			temp[1] = 62;  	  			  	 // reg 0x7D; SI4432_TX_FIFO_CONTROL_1; soglia tx alf full non arriva mai
			temp[2] = 62;  	  			  	 // reg 0x7e; SI4432_RX_FIFO_CONTROL  ; soglia rx fifo full
			tx_byte(temp,SI4432_TX_FIFO_CONTROL_2,3,DA_RADIO | DISINT);   		//da reg 0x7c; SI4432_TX_FIFO_CONTROL_2 a reg 0x7e; SI4432_RX_FIFO_CONTROL

			wreg(SI4432_OPERATING_AND_FUNCTION_CONTROL_2,SI4432_RXMULTIPK | SI4432_RXFIFORESET | SI4432_TXFIFORESET ,DA_RADIO | DISINT);    //reg 0x08; finiamo con un bel COMANDO DI CANCELLAZIONE DELLa rx FIFO, BISOGNA METTERE A 1.....
			wreg(SI4432_OPERATING_AND_FUNCTION_CONTROL_2,SI4432_RXMULTIPK ,DA_RADIO | DISINT);        									    //reg 0x08;  ........E POI RIMETTERE A ZERO

			rx_byte(temp,SI4432_INTERRUPT_STATUS_1,2,DA_RADIO | DISINT);        //REG 0X03 clear interrup pending

			temp[0] = SI4432_ENPKVALID | SI4432_ENPKSENT;// | SI4432_ENTXFFAEM;	// reg 0x05; SI4432_INTERRUPT_ENABLE_1; abilita interrupt
			temp[1] = SI4432_ENPREAVAL | SI4432_ENSWDET | SI4432_ENPOR; 										   	 	// reg 0x06; SI4432_INTERRUPT_ENABLE_2; abilita interrupt
			tx_byte(temp,SI4432_INTERRUPT_ENABLE_1,2,DA_RADIO | DISINT);   		//da reg 0x7C; SI4432_TX_POWER a reg 0x77; SI4432_NOMINAL_CARRIER_FREQUENCY_0

        	rx_byte(temp,SI4432_INTERRUPT_STATUS_1,2,DA_RADIO | DISINT);     //REG 0X03 clear interrup pending
}

void wreg(unsigned int address,unsigned int dati,unsigned char mode)
{
  unsigned int i,bit_p;

	if (mode & DISINT)
		_DINT();                     // DISABILITA GLI interrupts
	if (mode & DA_RADIO)
	{
		SPI_CS_RADIO_0;
		SPI_CS_RADIO_0;
		SPI_CS_RADIO_0;
		SPI_CS_RADIO_0;
		address |= 0x80;
	}
	
	bit_p = 0x80;
	for ( i=0;i<8;i++)
	{
			if ( address  & bit_p ) //& (0x80 >> i))
				SPI_OUT_1;
			else
				SPI_OUT_0;
			SPI_CLOCK_1;
			bit_p >>= 1;
			SPI_CLOCK_0;
	}

	bit_p = 0x80;
	for ( i=0;i<8;i++)
	{
			if ( dati & bit_p ) //(0x80 >> i))
				SPI_OUT_1;
			else
				SPI_OUT_0;
			SPI_CLOCK_1;
			bit_p >>= 1;
			SPI_CLOCK_0;
	}
	if (mode & DA_RADIO)
	{
		SPI_CS_RADIO_1;
	}
	SPI_OUT_0;
	if ( mode & DISINT)
	    _EINT();                    // Enable interrupts
}

void tx_byte(unsigned char *dati,unsigned char address,unsigned char numero,unsigned char mode)
{
  unsigned int i,j,bit_p;

	if (mode & DISINT)
		_DINT();                     // DISABILITA GLI interrupts
	if (mode & DA_RADIO)
	{
		SPI_CS_RADIO_0;
		SPI_CS_RADIO_0;
		SPI_CS_RADIO_0;
		SPI_CS_RADIO_0;
		address |= 0x80;
	}
	else
	{
		address |= 0x40;               //incrementa indirizzo ad ogni byte scritto
	}
	bit_p = 0x80;
	for ( i=0;i<8;i++)
	{
			if ( address  & bit_p ) //(0x80 >> i))
				SPI_OUT_1;
			else
				SPI_OUT_0;
			SPI_CLOCK_1;
			bit_p >>= 1;
			SPI_CLOCK_0;
	}
	for ( j=0;j<numero;j++)
	{
		bit_p = 0x80;
		for ( i=0;i<8;i++)
		{
			if ( (*(dati+j))  & bit_p ) //(0x80 >> i))
				SPI_OUT_1;
			else
				SPI_OUT_0;
			SPI_CLOCK_1;
			bit_p >>= 1;
			SPI_CLOCK_0;
		}
	}
	if (mode & DA_RADIO)
	{
		SPI_CS_RADIO_1;
	}
	SPI_OUT_0;
	if ( mode & DISINT)
	    _EINT();                    // Enable interrupts
}

void rx_byte(unsigned char *dati,unsigned char address,unsigned char numero,unsigned char mode)
{
  unsigned int i,j,bit_p;

	if (mode & DISINT)
		_DINT();                     // DISABILITA GLI interrupts
	if (mode & DA_RADIO)
	{
		SPI_CS_RADIO_0;
		SPI_CS_RADIO_0;
		SPI_CS_RADIO_0;
		SPI_CS_RADIO_0;
	}
	else
	{
		address |= 0xC0;               //read e incrementa indirizzo ad ogni byte letto
	}
	bit_p = 0x80;
	for ( i=0;i<8;i++)
	{
			if ( address  & bit_p)
				SPI_OUT_1;
			else
				SPI_OUT_0;
			SPI_CLOCK_1;
			bit_p >>= 1;
			SPI_CLOCK_0;
	}
	for ( j=0;j<numero;j++)
	{
		bit_p = 0x80;
		for ( i=0;i<8;i++)
		{
			if (SPI_IN)
				*(dati+j) |= bit_p;
			else
				*(dati+j) &= ~(bit_p);
			SPI_CLOCK_1;
			bit_p >>=1;
			SPI_CLOCK_0;
		}
	}
	if (mode & DA_RADIO)
	{
		SPI_CS_RADIO_1;
	}
	if ( mode & DISINT)
	    _EINT();                    // Enable interrupts
}


//PER CRYPTAZIONE

#ifdef SECURGROSS
	const unsigned int key_crypto[8] = {0xf3f9, 0x5ab5, 0x4689, 0xc357,  0x5926, 0x5a9c,  0xaf28, 0x3632};
#else
	const unsigned int key_crypto[8] = {0xf3f8, 0x5aa5, 0x4699, 0xc357,  0x5926, 0x5a9c,  0xaf28, 0x3632};
#endif



void setta_parita(unsigned char *dato, unsigned char dispari)           //parita
{
	unsigned int i,j;
	j = dispari;

	for ( i=0;i<7;i++)
	{
		 if ( (*dato) & (1<<i))
			j++;
	}
	if ( j & 1)
		(*dato) |= 0x80;
	else
		(*dato) &= ~0x80;
}

unsigned int controlla_parita(unsigned char dato, unsigned char dispari)           //parita
{
	unsigned int i,j;
	j = dispari;

	for ( i=0;i<8;i++)
	{
		 if ( dato & (1<<i))
			j++;
	}
	if ( j & 1)
		return 0;
	else
		return 1;
}

unsigned int decrypta(unsigned char* dati_c, unsigned char *dati_chiaro)
{
		unsigned int i;
		unsigned char buffer[20];
		unsigned int sequenza;

		sequenza  = (dati_c[0] ^ dati_c[10]) + ((dati_c[1] ^ dati_c[12]) << 8);

		crea_dati_enc(18, (unsigned int*)key_crypto, sequenza, buffer);     //NUMERO BYTES RISPOSTA, CHIAVE[8b], SEQUENZA[2b], PUNTATORE RISUSLTATO

		i =0;
		if (controlla_parita(dati_c[0+2] ^ buffer[0], 0))           //parita su tipo_messaggio:pari
		{
			if (controlla_parita(dati_c[1+2] ^ buffer[1], 1))           //parita su stato_allarmi :dispari
			{
					i++;
			}
		}
		if ( i == 0)
		{
			return 0;
		}
		else
		{
			dati_chiaro[0] = sequenza;
			dati_chiaro[1] = sequenza>>8;

			for ( i=0;i<18;i++)
			{
				dati_chiaro[i+2] = dati_c[i+2] ^ buffer[i];
			}
			dati_chiaro[0+2]  &= 0x07f;
			dati_chiaro[1+2]  &= 0x07f;
		    return 1;
		}
}

void encrypta(unsigned int sequenza,unsigned char * dati,unsigned char *dati_cryp)
{
		unsigned int i;
		unsigned char buffer[18];

		crea_dati_enc(18, (unsigned int*)key_crypto, sequenza, buffer);     //NUMERO BYTES RISPOSTA, CHIAVE[8b], SEQUENZA[2b], PUNTATORE RISUSLTATO

		setta_parita(&dati[0], 0);           //parita su tipo_messaggio:pari
		setta_parita(&dati[1], 1);           //parita su stato_allarmi :dispari

		for ( i=0;i<18;i++)
		{
            dati_cryp[i+2] =  dati[i] ^ buffer[i];
		}

		dati_cryp[0] = sequenza ^ dati_cryp[10];
		dati_cryp[1] = (sequenza>>8) ^ dati_cryp[12];
}


#define i4(x,a,b,c,d)	((((((unsigned int)(x)) >> (a)) & 1) << 0) + (((((unsigned int)(x)) >> (b)) & 1) << 1) + (((((unsigned int)(x)) >> (c)) & 1) << 2) +  (((((unsigned int)(x)) >> (d)) & 1) << 3))

#define ht2_f4a  0x5496L
#define ht2_f4b  0xc135L


unsigned int f20 (unsigned int x)
{
	unsigned int  temp;

	temp = ((ht2_f4a >> i4(x, 1, 2, 4, 5)) & 5)
	     + (((ht2_f4b >> i4(x, 0, 8, 9,10)) & 5) << 1);
	return (temp);
}



void crea_dati_enc(unsigned char n_byte_response, unsigned int *key, unsigned int sequen, unsigned char *response)
{
	unsigned int	i,j;
	unsigned int	k,random;

	random = sequen;

	if ( sequen & 0x8000)
	{

		flag.decrypta_acq = 1;
		for (i = 0; i < 4; i++)
		{
			k =  key[i] ^ segmC.snum_concentratore;
    	    j =  f20(random ^ k);
			random =  (random << 4) + j;
		}
	}
	else
	{
		flag.decrypta_acq = 0;
		for (i = 0; i < 4; i++)
		{
			k =  key[i];
    	    j =  f20(random ^ k);
			random =  (random << 4) + j;
		}
	}
	for (i = 0; i < n_byte_response; i++)
	{
		*(response + i) = 0;
		for (j = 0; j < 2; j++)
		{
			k = f20(random + key[(i & 0x07)]);
			*(response + i) += ( k << (j* 4));
			random = (random << 1) ^ (k & 3);
		}
	}
}

void gestione_radio_ex_interr(void)
{
	unsigned char temp[2];
	unsigned char temp_b;

	DCOCTL =   CALDCO_8MHZ;                                             //CALIBRAZIONE DEL DCO A 8 MHz (deve andare A PILA!)
    BCSCTL1=   CALBC1_8MHZ  | XT2OFF;

		rx_byte(temp,SI4432_INTERRUPT_STATUS_1,2,DA_RADIO);           	//legge status interrupt e clear interrup pending

		if ( temp[1] & SI4432_IPREAVAL)
		{
			wreg(SI4432_OPERATING_AND_FUNCTION_CONTROL_2,SI4432_RXMULTIPK | SI4432_RXFIFORESET,DA_RADIO);     //reg 0x08; finiamo con un bel COMANDO DI CANCELLAZIONE DELLE FIFO, BISOGNA METTERE A 1.....
   			wreg(SI4432_OPERATING_AND_FUNCTION_CONTROL_2,SI4432_RXMULTIPK ,DA_RADIO);       				  //reg 0x08;  ........E POI RIMETTERE A ZERO
		}
		if ( temp[1] & SI4432_ISWDET)
		{
			buffer_radio.timer_perif = TA0R;
			rx_byte(&temp_b,SI4432_RECEIVED_SIGNAL_STRENGTH_INDICATOR,1,DA_RADIO);          //legge rssi

			temp_b >>= 1;
			if ( temp_b < 122)
				buffer_radio.rssi = 122 - temp_b;
			else
				buffer_radio.rssi = 0;

			cnt_conversioni_vbatt = 6;		//misura tensione batteria subito dopo (media di 4 letture fra 100/200 e entro 500/600 usec)
		}
		if ( temp[0] & SI4432_ENPKVALID)
		{
			rx_byte(&temp_b,SI4432_RECEIVED_PACKET_LENGTH,1,DA_RADIO);                               //legge lunghezza
			if( ( temp_b == LUNGHEZZA_DATI_PERIFERICHE + 2) && ( buffer_radio.flag.endtx == 1))      //+2 per differenziare parola periferica e concentratore
			{
				rx_byte(&buffer_radio.dati[0],SI4432_FIFO_ACCESS,temp_b+1,DA_RADIO);
				buffer_radio.pnt_byte = temp_b+1;
				buffer_radio.dati_rdy = buffer_radio.pnt_byte;

				temp[0] = 0;				//per non piu if interr da radio(verra spenta)
				temp[1] = 0;				//per non piu if interr da radio(verra spenta)
				SDN_1;       				//radio off
			}
			else
			{
				wreg(SI4432_OPERATING_AND_FUNCTION_CONTROL_2,SI4432_RXMULTIPK | SI4432_RXFIFORESET | SI4432_TXFIFORESET,DA_RADIO);        //reg 0x08; finiamo con un bel COMANDO DI CANCELLAZIONE DELLE FIFO, BISOGNA METTERE A 1.....
	   			wreg(SI4432_OPERATING_AND_FUNCTION_CONTROL_2,SI4432_RXMULTIPK ,DA_RADIO);                                                 //reg 0x08;  ........E POI RIMETTERE A ZERO
			}
		}

	//interrupt a fine trasmissione
		if ( temp[0] & SI4432_ENPKSENT)
		{
			buffer_radio.flag.endtx = 1;
			wreg(SI4432_OPERATING_AND_FUNCTION_CONTROL_1,SI4432_RXON,DA_RADIO);      //reg 0x07;   //radio in rx
		}

		if ( temp[1] & SI4432_IPOR)
		{
			cnt_err_superif_resetradio++;
			buffer_radio.flag.reset = 1;
  			SDN_1;       				//radio off
		}

		DCOCTL  =   CALDCO_1MHZ;
		BCSCTL1 =   CALBC1_1MHZ | XT2OFF;
}



/***************************************************************/
/****      FUNZIONI DI GESTIONE DEGLI INTERRUPTS            ****/
/***************************************************************/

/*---------------------------------------------------------------------------------
// GESTISCE L'INTERRUPT della PORTA2
*/
#pragma vector=PORT2_VECTOR
__interrupt void interr_port2(void)
{
    if (( P2IFG & MASK_TAPP) && (P2IE & MASK_TAPP))
	{
        P2IE  &= ~MASK_TAPP;            // DISabilita ULTERIORI interrupts per rimbalzi
		cnt_mask_tappa = 3;             //pausa minima fra impulsi = 2/3 * 15,625 msec
		if (cnt_imp_tappa)
			cnt_imp_tappa--;

		cnt_tempo_tappa = (segmC.parametri_periferica.tempo_tappa <<  2);      //si ricarica ad ogni impulso
	}
}

/*---------------------------------------------------------------------------------
// GESTISCE L'INTERRUPT della PORTA1
*/
/*#pragma vector=PORT1_VECTOR
__interrupt void interr_port1(void)
{
    if (( P1IFG & MASK_TAPP) && (P1IE & MASK_TAPP))         // controlla che l'interrupt provenga dalla tapparella
	{
        P1IE  &= ~MASK_TAPP;                                // disabilita ulteriori interrupts per rimbalzi
		cnt_mask_tappa = 3;                                 // pausa minima fra impulsi = 2/3 * 15,625 msec
		if (cnt_imp_tappa)
			cnt_imp_tappa--;

		cnt_tempo_tappa = (segmC.parametri_periferica.tempo_tappa <<  2);      //si ricarica ad ogni impulso
	}
}*/

// Funzioni per la gestione delle isr della UART in tx e rx
#pragma vector=USCIAB0TX_VECTOR
__interrupt void USCI0TX_ISR(void)
{
    flag_tx_finished = 0;
    UCA0TXBUF = tx_string[tx_index++]; // TX next character 
    if(tx_index == 1 && uart_pause_flag == 1)
    {
       UC0IE &= ~UCA0TXIE;              // Disabilita USCI_A0 TX interrupt       
       TA1CTL |= MC_1;                  // abilita il contatore in UP mode
       TA1CCTL0 |= CCIE;                // compare interrupt abilitato
    }
    else if (tx_index == string_size)   // TX over? 
    {        
       UC0IE &= ~UCA0TXIE;              // Disable USCI_A0 TX interrupt              
       tx_index = 0; 
       uart_pause_flag = 1;    
       flag_tx_finished = 1;
    }    
    
} 
  
#pragma vector=USCIAB0RX_VECTOR 
__interrupt void USCI0RX_ISR(void) 
{ 
    static int length, pos, j, sum;
    switch(actual_state)
    {
                    case Wakeup:if(UCA0RXBUF == 0x00)
                                {
                                    pos = 0;
                                    actual_state = Header;                                    
                                }
                                else                                           
                                    actual_state = Wakeup;
                                break;
                    case Header:if(UCA0RXBUF == 0xaa)
                                    actual_state = Length;                                     
                                else
                                        actual_state = Wakeup;                                         
                                break;
                    case Length:length = UCA0RXBUF;                                
                                actual_state = Reading;
                                break;
                    case Reading:if(pos+2 < length - 1)            // pos è l'indicde del contenuto del pacchetto, disallineato di 3 Bytes rispetto all'intero pacchetto rx (0xAA,length)
                                 {                                           
                                     rx_mex[pos] = UCA0RXBUF;                                             
                                     pos++;                            
                                     actual_state = Reading;
                                 }
                                 else
                                 {
                                     rx_mex[pos] = UCA0RXBUF; 
                                     for(j=0,sum=length+0xaa;j<pos;j++)                      // calcola la somma dei Byte ricevuti eccetto l'ultimo 
                                            sum += rx_mex[j];                                                 
                                     if((sum & 0xff) != rx_mex[pos])                         // checksum error
                                     {
                                            P1OUT &= ~ BIT0;
                                            rx_valid_flag = 0;
                                     }
                                     else
                                     {
                                            P1OUT |= BIT0;                                         
                                            rx_valid_flag = 1;
                                     }                                            
                                     switch(rx_mex[0])
                                     {
                                            case 0x06:  rx_cmd = ack;               // acknowledge ricevuto
                                                        break;
                                            case 0x15:  rx_cmd = nack;              // not acknowledge ricevuto
                                                        break;
                                            case 0x70:  rx_cmd = detector_events;   // risposta a detector events ricevuta
                                                        break;
                                            case 0x71:  if(rx_mex[1] == 0x1f)
                                                            rx_cmd = req_init;      // richiesta di inizializzazione ricevuta
                                                        break;
                                            case 0x90:  rx_cmd = GDE;               // risposta a Get Detector Events ricevuta
                                                        break;
                                            case 0x91:  rx_cmd = GPageM;            // risposta a Get Page Mask ricevuta
                                                        break;
                                            case 0x92:  rx_cmd = GPropertyM;        // risposta a Get Property Mask ricevuta
                                                        break;
                                            case 0x93:  rx_cmd = GDP;               // risposta a Get Detector Property ricevuta
                                                        break;
                                            case 0x94:  rx_cmd = GMN;               // risposta a Get Model Number ricevuta                                                          
                                                        break;
                                            case 0x95:  rx_cmd = GCV;               // risposta a Get Communication Version ricevuta
                                                        break;
                                     }
                                     flag_rx = 0; 
                                     actual_state = Wakeup;                                     
                                 }                    
                                 break;
    }         
}

//---------------------------------------------------------------------------------
// Timer A0 interrupt service routine GESTISCE L'INTERRUPT TA0
// TA0 è usato come  TIMER a  USEC15625
#pragma vector=TIMER0_A0_VECTOR
__interrupt void Timer_A0(void)
{
	unsigned char temp;
	static unsigned char cnt_sec, cnt_250msec;
//	OUT_TEST_1;

	TA0CCR0 += 512;       // Add Offset to CCR,  clock del timer = 32 KHz

// --------- Temporizzazioni
    	P2OUT |= 0x0A;     // PULL up anche su p2.1,3
//    	P1DIR  = 0x25;     // p1.5 = uscita per lettura ponticello
    	P1OUT |= 0x20;     // p1.4 con pullup lettura ponticello
//    	P1REN |= 0x20;     // p1.5 = input con pull up per lettura ponticello

		if ( timeout)
			timeout--;

		if ( cnt_post_fpro)
			   cnt_post_fpro--;

		temp  = P2IN & 0x0e;
		temp |= P1IN & 0x20;   //lettura p1.4 ponticello

		P2OUT=0x01;     // PULL up solo su p2.0

//		P1DIR  =  0x05;     //
	    P1OUT &= ~0x20;     //  pull dw su P15

		if ( (temp & 0x2e) != input_old)
		{
			input_old = (temp & 0x2e);
			input_cnt = 65000/USEC15625;
//			stai_sveglio = 100000/USEC15625;
		}
		else if ( input_cnt )
				input_cnt--;
		else
		{
//			input_fronte.i8 = ((~input_old) & 0x0e) & (~input_stato.i8);
			input_stato.i8    = (~input_old) & 0x2e;
		}

//    if(tipo_connessione_sensore == 0)     // connessione tramite contatto
//    {

		if (cnt_mask_tappa)
		{
			if ( --cnt_mask_tappa == 0)
			{
//              P1IFG &= ~MASK_TAPP;
//              P1IE |= MASK_TAPP;
				P2IFG &= ~MASK_TAPP;
				P2IE  |= MASK_TAPP;            // Riabilita interrupts tapparella
			}
		}
//    }

		if ( dati_periferica.n.cnt_sono_vivo)
				dati_periferica.n.cnt_sono_vivo-- ;


		if ( ++cnt_250msec >= 250000/USEC15625)
		{
			cnt_250msec = 0;

			if (tempo_led_on)
			{
			   if ( --tempo_led_on == 0)
        			LED_OFF;
			   else
			        LED_ON;
			}

			if (cnt_tempo_tappa)
			{
				if (--cnt_tempo_tappa == 0)
				{
					cnt_imp_tappa   = segmC.parametri_periferica.impulsi_tappa;
//					cnt_tempo_tappa = segmC.parametri_periferica.tempo_tappa;
				}
			}
			if ( ++cnt_sec >= 1000/250)
			{                                    //ogni sec
				cnt_sec = 0;

				if ( ++cnt_ore > 3600)
				{
					cnt_ore = 0;
					dati_periferica.n.tempo_batteria++;
					scrivi_tempo_batteria_pnd = 1;
				}
//
//
				if (dati_periferica.n.cnt_sec_coll_man_inst)
				{
					if (--dati_periferica.n.cnt_sec_coll_man_inst == 0)
					{
					 	if ( segmC.snum_concentratore == 0xffff)
					 		dati_periferica.n.mode_f = MP_FABBRICA;
					 	else
			 				dati_periferica.n.mode_f = MP_IN_SERVIZIO;
			 			dati_periferica.n.cnt_sec_coll_man_inst = 0;
					}
				}
			}
		}
		LPM3_EXIT;
//	}
}

#pragma vector=TIMER0_A1_VECTOR     //interrupt finestra messaggi sincroni
__interrupt void Timer0_C1(void)
{
		OUT_TEST_1;
		cnt_post_fpro = TA0IV;            //PER CLEAR INT PND
		if ( ++pnt_periodo_rx_sincrona_uso  > 3)   pnt_periodo_rx_sincrona_uso = 0;
		TA0CCR1 += periodo_rx_sincrona[pnt_periodo_rx_sincrona_uso] + ((unsigned int)n_concentratore_in_impianto << 11);

		cnt_post_fpro = DELTA_FPROIBITA;

		OUT_TEST_0;
}


#pragma vector=TIMER1_A0_VECTOR      //interrupt per pausa tra wakeup e preambolo in comunicazione UART
__interrupt void Timer1A0(void)
{
    TA1CTL &= ~MC_1;                 // disabilita contatore
    TA1CCTL0 &= ~CCIE;               // disabilita compare interrupt e azzera il flag di interrupt
    uart_pause_flag = 0;
    UC0IE |= UCA0TXIE;               // Enable USCI_A0 TX interrupt              
 }

void start_adc(void)
{

#ifdef BATTERIA_9V
	ADC10CTL1 = INCH_3;            // select ch 3

	ADC10CTL0 =   SREF_1          	// Vref+/Vss;
				+ ADC10SHT_2        // sample time = 16*adc10cls;
				+ ADC10SR        	// ref buffer 50ksps (low power);
				+ REFBURST        	// ref buffer on solamente durante sample e conversione
				+ REF2_5V 			// ref a 2.5 volt
				+ REFON 			// ref on
				+ ADC10ON        	// ADC10 on
//				+ ENC + ADC10SC             // Sampling and conversion start
				;
#else
	ADC10CTL1 = INCH_11;            // select AVcc/2

	ADC10CTL0 =   SREF_1          	// Vref+/Vss;
				+ ADC10SHT_2        // sample time = 16*adc10cls;
				+ ADC10SR        	// ref buffer 50ksps (low power);
				+ REFBURST        	// ref buffer on solamente durante sample e conversione
				+ REF2_5V 			// ref a 2.5 volt
				+ REFON 			// ref on
				+ ADC10ON        	// ADC10 on
//				+ ENC + ADC10SC             // Sampling and conversion start
				;
#endif
	ADC10CTL0 |= ENC + ADC10SC;             // Sampling and conversion start
    flag.adc_in_corso = 1;
}

void off_adc(void)
{
//	ADC10CTL1 = INCH_11;            // select AVcc/2
   	ADC10CTL0 &= ~ENC;             // Sampling and conversion start

	ADC10CTL0 =   SREF_1          	// Vref+/Vss;
				+ ADC10SHT_2        // sample time = 16*adc10cls;
				+ ADC10SR        	// ref buffer 50ksps (low power);
				+ REFBURST        	// ref buffer on solamente durante sample e conversione
				+ REF2_5V 			// ref a 2.5 volt
  //				+ REFON 			// ref on
  //				+ ADC10ON        	// ADC10 on
 //				+ ENC + ADC10SC             // Sampling and conversion start
				;
//				flag.adc_in_corso = 0;
}

/* Cattura tutti gli interrupt non gestiti diversamente */
__interrupt void _unexpected_(void)
{
  //reset per wdog
  while (1);
}
