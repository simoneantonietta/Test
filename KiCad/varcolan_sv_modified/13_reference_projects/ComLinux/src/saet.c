#include "saet.h"
#include "aes.h"
#include <stdio.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <glib.h>
#include <linux/serial.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <string.h>
#include <stdlib.h>

#define DEBUG_SAETNET

#define BASESECONDS	631148400

#define bitResend	0x01
//#define bitAuth		0x04
#define bitConnect	0x08
#define bitKeepAlive	0x20
#define bitAck		0x40
#define bitCommand	0x80

#define SAETCOM_IMPIANTO 1
//#define SAETCOM_IMPIANTO 1593
/* Keepalive ogni 20 secondi */
#define KEEPALIVE_PERIOD 200

typedef struct
{
	unsigned short msgid;
	unsigned char stato;
	unsigned short impianto;
	unsigned char nodo;
	unsigned int datetime;
	unsigned char len;
	unsigned char estensione;
	unsigned char evento[256];
}__attribute__((packed)) UDP_event;

void set_bg_color(int state);
void print_event(char *event);

saetparam_t saetparam =
	{
	0,
	};
char *saet_test_path = NULL;

int saetfd;
pthread_t pth;
FILE *fpdebug = NULL;

GSList *cmdlist = NULL;
pthread_mutex_t cmdmutex = PTHREAD_MUTEX_INITIALIZER
;

void saet_load_param()
{
char buf[256];
FILE *fp;

sprintf(buf, "%s/.comlinux/connection", getenv("HOME"));
fp = fopen(buf, "r");
if(fp)
	{
	fread(&saetparam, sizeof(saetparam_t), 1, fp);
	fclose(fp);
	}

sprintf(buf, "%s/.comlinux/testpath", getenv("HOME"));
fp = fopen(buf, "r");
if(fp)
	{
	fscanf(fp, "%s", buf);
	saet_test_path = strdup(buf);
	fclose(fp);
	}
}

void saet_save_param()
{
char buf[256];
FILE *fp;

sprintf(buf, "%s/.comlinux/connection", getenv("HOME"));
fp = fopen(buf, "w");
if(!fp) return;

fwrite(&saetparam, sizeof(saetparam_t), 1, fp);

fclose(fp);
}

#define EVENT_STRING_BASE	150

static const char *EventString[] =
	{
#if 0
	    /* Lara events */
	    "",
	    "",
	    "",
	    "",
	    "",
	    "",
	    "",
	    "",
	    "",
	    "",
	    "",
	    "",
	    "",
	    "",
	    "",
	    "",
	    "",
	    "",
	    "",
	    "",
	    "",
	    "",
	    "",
	    "",
	    "",
	    "",
	    "",
	    "",
	    "",
	    "",
	    "",
	    "",
	    "",
	    "",
	    "",
	    "",
	    "",
	    "",
	    "",
	    "",
	    "",
	    "",
	    "",
	    "",
	    "",
	    "",
	    "",
	    "",
	    "",
	    "",
#endif
	    /* SYS events */
	    "Allarme sensore $0 Z=$2", "Ripristino sensore $0 Z=$2", "Guasto perif. $4", "Fine Guasto perif. $4", "Manom. sensore $0", "Fine Manom. sensore $0", "Manom. cont.e perif. $4", "Fine Manom.e cont.e perif. $4", "Attivata zona $2", "Disatt.  zona $2", "Attivaz. impedita zona $2", "In serv. sensore $0", "Fuori serv. sensore $0", "In serv. attuatore $0", "Fuori serv. attuatore $0", "MMaster $3: ric.cod errato", "Modulo Master $3: tipo $3", "Perif. incongr. ind.$4 tipo $3", "Perif. manomessa $4", "Ripristino perif. $4", "Sosp. att.ta' linea $3", "Chiave falsa perif. $4", "Sent. $4 attivata", "Sent. $4 disattivata", "Sent. $4 disatt. timeout", "NoTx m.master $3", "Errore Rx m.master $3", "Errore ric. mess. host", "Segnalazione evento n.$0", "Perif. presenti da $4 > $5$5$5$5$5$5$5$5", "SZ $8> $a $a $a $a $a $a $a $a", "SS $0> $a $a $a $a $a $a $a $a", "SA $0> $a $a $a $a $a $a $a $a", "Guasto sensore $0", "Variazione ora $9", "Cod. contr. $x-$x-$x", "Stato QRA I=$3 QRA II=$3. Att.ronda $3", "Accettato allarme sensore $0", "No punz. stazione $4", "On telecomando $0", "Param. tarat. perif.$4 Sens.$3 Dur.$3", "Stato Prova = $3", "Test attivo gruppo $8", "Risp.SCS", "Fine guasto sensore $0", "Valore $0 per segreto $3", "Perif. previste da $4 > $5$5$5$5$5$5$5$5", "Sensore in allarme $0 Z=$2", "Variata fascia oraria $8:$3 ore $9", "Invio diretto dati t$3 a host computer", "", "Fine invio dati", "Cnf.ronda $3- $3,$3,$3,$3,$3,$3,$3,$3,$3,$3,$3,$3,$3,$3,$3,$3", "Ore ronda $3- $9,$9,$9,$9,$9,$9", "Festivi $3- $3$3$3$3$3$3$3$3$3$3$3$3$3$3$3$3", "Fasce giornaliere $8- $9 $9  $9 $9", "ON Attuatore $0", "Transito id. $0 lettore $4", "Entrato id. $0 lettore $4", "Uscito  id. $0 lettore $4", "Codice valido $8", "Chiave valida $8", "Operatore $8", "Staz. $0 punzonata", "Spegnimento n. $0", "Reset fumo n. $0", "Livello $8 abilitato", "Variato segreto", "Connessione modem tipo $3", "Numero telef. primario - ", "Numero telef. second.o - ", "Esaminato Badge $4:$0", "Evento sensore $0 $e $E", "MU $0> $a $a $a $a $a $a $a $a", "Lista ZS $0> $8 $8 $8 $8 $8 $8 $8 $8", "Descr.Zona $2", "Descr.Dispositivo $0", "Fra $3 giorni variato a $3", "Valori analogici $4: $8 $8 $8 $8 $8 $8 $8 $8", "Letto badge $3-xxxxxxxxxx", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "Impianto non comunica $3", "", "", "", "", "", "Governo linee: in servizio linea $4", "Governo linee: fuori servizio linea $4",
	};

static const char *EventExString[] =
	{
	"GSM $3: $s", "TMD $3: $s", "$s"
	};

static const char *EventEx2String[][103] =
	{
		{
		/* Tebe */
		"Tebe ACK motivo:$3 tipo:$3 id:$1", "Lettura dati tessera $1", "Lettura dati Profilo $8", "Lettura dati terminale $8", "Lettura dati contatore $8", "Lettura dati fascia oraria $8", "Lettura dati festivo $8", "Ident. $1 transitato da Term. $6 in Area $8", "Ident. $1 timbrato in Term. $6 Fe $3", "Ident. $1 transitato da Term. $6 in Area $8 cod $1", "Ident. $1 timbrato in Term. $6 Fe $3 cod $1", "Ident. $1 Fuori Area in Term. $6", "Ident. $1 Non Abilitato nel Term. $6", "Ident. $1 No Transito x F.O. non abil. su Term. $6", "Ident. $1 No Transito x Festivita' su Term. $6", "Ident. $1 Profilo $8 Non Abilitato su Term. $6", "Ident. $1 Abilita Grado Resp. da Term. $6", "Ident. $1 Segnalata Coercizione", "Ident. Assente: No transito da Term. $6", "", "",  // 20
		"Forzatura porta da Term. $6", "Apertura prolungata porta da Term. $6", "Fine forzatura/apertura prolungata porta da Term. $6", "Allarme Ingresso Aux su Term. $6", "Fine allarme Ingresso Aux su Term. $6", "Bassa tensione su Term. $6", "Fine bassa tensione su Term. $6", "", "", "", "Ident. $1 variato PIN", "Ident. $1 disabilitato", "Ident. $1 abilitato", "Ident. $1 inserito da Term. $6", "Ident. $1 inserito su Centrale", "Ident. $1 forzato in Area $8", "Ident. $1 associato a Profilo $8", "Ident. $1 associato a F.O. Sett. $6", "Ident. $1 variata associazione a Terminali", "Ident. $1 variata associazione a Festivita'",	// 40
		"Ident. $1 variato Stato $S1", "Ident. $1 variato Contatore", "Profilo $8 associato a F.O. Sett. $6", "Profilo $8 variata associazione a Terminali", "Profilo $8 variata associazione a Festivita'", "Profilo $8 variato Stato $S2", "Term. $6 configurato: FI=$8 FO=$8", "Term. $6 configurato: Fe1>area $8 Fe2>area $8", "Term. $6 associato a F.O. Sett. $6", "Term. $6 variata associazione a Festivita'", "Term. $6 variato DIN/Cont", "Term. $6 variati Tempi: TAP=$8 TAZ=$8 TPBK=$6", "Term. $6 variato Stato $S3", "Variata F.O. Settimanale $6", "Variata Festivita' $6", "", "", "", "", "Reset Centrale $1",	// 60
		"Ident. $1 da Term. $6 richiesta cod. $1", "Presenze in Area $8: $1", "Ident. $1 associato Profilo sec. $8", "Ident. $1 cancellato Profilo sec. $8", "Ident. $1 Volto non riconosciuto", "Ident. $1 Volto non presente", "", "", "", "",
		},
		{
		/* Delphi */
		"Inizio Test Attivo gruppo $3", "Fine Test Attivo gruppo $3 (positivo)", "Allarme sensore $d Z=$2", "Fine allarme sensore $d Z=$2", "Guasto perif. $p", "Fine Guasto perif. $p", "Manom. sensore $d", "Fine Manom. sensore $d", "Manom. cont.e perif. $p", "Fine Manom.e cont.e perif. $p", "Attivata zona $2",  // 10
		"Disatt.  zona $2", "Attivaz. impedita zona $2", "In serv. sensore $d", "Fuori serv. sensore $d", "In serv. attuatore $d", "Fuori serv. attuatore $d", "MMaster $3: ric.cod errato", "Modulo Master $3: tipo $3", "Perif. incongr. ind.$p tipo $3", "Perif. manomessa $p",  // 20
		"Ripristino perif. $p", "Sosp. att.ta' linea $3", "Chiave falsa perif. $p", "Sent. $p attivata", "Sent. $p disattivata", "Sent. $p disatt. timeout", "NoTx m.master $3", "Errore Rx m.master $3", "Errore ric. mess. host", "Segnalazione evento n.$d",  // 30
		"Perif. presenti da $p > $5$5$5$5$5$5$5$5", "SZ $8> $a $a $a $a $a $a $a $a", "SS $d> $a $a $a $a $a $a $a $a", "SA $d> $a $a $a $a $a $a $a $a", "Guasto sensore $d", "Variazione ora $9", "", "", "Accettato allarme sensore $d", "",  // 40
		"On telecomando $d", "", "", "", "", "Fine guasto sensore $d", "Valore $d per segreto $3", "Perif. previste da $p > $5$5$5$5$5$5$5$5", "Sensore in allarme $d Z=$2", "",  // 50
		"", "", "Fine invio dati", "", "", "Festivi $3- $3$3$3$3$3$3$3$3$3$3$3$3$3$3$3$3", "", "ON Attuatore $d", "Transito id. $d lettore $p",  // 60
		"Entrato id. $d lettore $p", "Uscito  id. $d lettore $p", "Codice valido $8", "Chiave valida $8", "Operatore $o", "", "", "", "", "Variato segreto", "Connessione modem tipo $3",  // 70
		"", "", "", "Evento sensore $d $e $E", "MU $d> $a $a $a $a $a $a $a $a", "Lista ZS $d> $8 $8 $8 $8 $8 $8 $8 $8", "", "", "", "Valori analogici $d> $X$X $X$X $X$X $X$X $X$X $X$X $X$X $X$X",  // 80
		"", "Orologio disallineato", "Orologio allineato", "Manca rete 220V", "Ripristino rete 220V", "Guasto batteria", "Ripristino batteria", "Inizio Manutenzione", "Fine Manutenzione", "Fine Test Attivo gruppo $3 (negativo)", "Inizio Straordinario", "Fine Straordinario", "Inserimento anticipato", "Attivata zona rit. $2", "Stato centrale: $X", "On attuatore $d ($l)", "Off attuatore $d", "Salvataggio database", "Fuori serv. sensore $d (autoesclusione)", "In serv. sensore $d (autoesclusione)", "Allineamento: $t $d > $A $A $A $A $A $A $A $A", "Stato connessione: $3",
		},
	};

static const char *SensorEventType[] =
	{
	"allarme", "manomissione", "guasto", "soglia 1", "soglia 2", "soglia 3", "soglia 4", "soglia 5"
	};
static const char *SensorEventState[] =
	{
	"iniziato", "", "accettato", "non accettato", "accettato (3)", "discesa", "salita"
	};
static const char *ActuatorOnType[] =
	{
	"normale", "lampeggiante"
	};
static const char *SyncType[] =
	{
	"fine", "sensore", "zona", "attuatore"
	};

#define bitAlarm	(1<<0)
#define bitSabotage	(1<<3)
#define bitFailure	(1<<4)
#define bitOOS		(1<<7)

static char* event_format(const char *msg, unsigned char *ev, int len)
{
static char buf[256];
int i, j, k, v;

if(!msg) return NULL;

for(i = j = k = 0;msg[i];i++)
	{
	switch(msg[i])
		{
		case '$':
			i++;
			switch(msg[i])
				{
				case '0':
					v = ev[k++] * 256;
					v += ev[k++];
					sprintf(buf + j, "%04d", v);
					j += 4;
					break;
				case '1':
					v = ev[k++];
					v += ev[k++] * 256;
					sprintf(buf + j, "%05d", v);
					j += 5;
					break;
				case 'd':
					v = ev[k++];
					v += ev[k++] * 256;
					sprintf(buf + j, "%04d", v);
					j += 4;
					break;
				case '2':
					v = ev[k++];
					if(v == 0)
						{
						sprintf(buf + j, "Tot.");
						j += 4;
						}
					else if(v < 216)
						{
						sprintf(buf + j, "S%03d", v);
						j += 4;
						}
					else
						{
						sprintf(buf + j, "I%02d", v - 216);
						j += 3;
						}
					break;
				case '3':
					v = ev[k++];
					if(v != 255)
						{
						sprintf(buf + j, "%d", v);
						j += strlen(buf + j);
						}
					break;
				case '4':
					v = ev[k++];
					sprintf(buf + j, "%d-%02d", v >> 5, v & 0x1f);
					j += strlen(buf + j);
					break;
				case 'p':
					v = ev[k++];
					v += ev[k++] * 256;
					sprintf(buf + j, "%d-%02d", v >> 5, v & 0x1f);
					j += strlen(buf + j);
					break;
				case '5':
					v = ev[k++];
					if((v & 0xf) == 0xf)
						buf[j] = '-';
					else if((v & 0xf) == 0xe)
						buf[j] = '*';
					else if((v & 0xf) > 9)
						buf[j] = ((v & 0xf) - 10) + 'A';
					else
						buf[j] = (v & 0xf) + '0';
					j++;
					v >>= 4;
					if(v == 0xf)
						buf[j] = '-';
					else if(v == 0xe)
						buf[j] = '*';
					else if(v > 9)
						buf[j] = ((v & 0xf) - 10) + 'A';
					else
						buf[j] = v + '0';
					j++;
					break;
				case '6':
					v = ev[k++];
					sprintf(buf + j, "%02d", v);
					j += 2;
					break;
				case '7':
					break;
				case '8':
					v = ev[k++];
					sprintf(buf + j, "%03d", v);
					j += 3;
					break;
				case '9':
					v = ev[k++];
					if(v != 255)
						sprintf(buf + j, "%02d:%02d", v, ev[k]);
					else
						sprintf(buf + j, "--:--");
					k++;
					j += 5;
					break;
				case 'a':
					v = ev[k++];
					strcpy(buf + j, "----");
					if(v & bitOOS) buf[j] = 'S';
					if(v & bitFailure) buf[j + 1] = 'G';
					if(v & bitSabotage) buf[j + 2] = 'M';
					if(v & bitAlarm) buf[j + 3] = 'A';
					j += 4;
					break;
				case 'A':
					v = ev[k++];
					strcpy(buf + j, "-------");
					if(v & 0x80) buf[j] = 'S';
					if(v & 0x04) buf[j + 2] = 'G';
					if(v & 0x02) buf[j + 4] = 'M';
					if(v & 0x01) buf[j + 6] = 'A';
					if(v & 0x20) buf[j + 1] = 'g';
					if(v & 0x10) buf[j + 3] = 'm';
					if(v & 0x08) buf[j + 5] = 'a';
					j += 7;
					break;
				case 'e':
					v = ev[k++];
					sprintf(buf + j, "%s", SensorEventType[v - 1]);
					j += strlen(buf + j);
					break;
				case 'l':
					v = ev[k++];
					sprintf(buf + j, "%s", ActuatorOnType[v]);
					j += strlen(buf + j);
					break;
				case 'E':
					v = ev[k++];
					sprintf(buf + j, "%s", SensorEventState[v - 1]);
					j += strlen(buf + j);
					break;
				case 'o':
					/* Operatore esteso */
					v = ev[k++];
					sprintf(buf + j, "%03d", v);
					j += 3;
					if(len > 16)
						{
						v = ev[k++];
						switch(v)
							{
							case 0:
								sprintf(buf + j, " Terminale %03d", ev[k++]);
								j += 14;
								break;
							case 1:
								sprintf(buf + j, " Inseritore %03d", ev[k++]);
								j += 15;
								break;
							case 2:
								sprintf(buf + j, " Telecomando");
								j += 12;
								k++;
								break;
							default:
								sprintf(buf + j, " ??? %03d", ev[k++]);
								j += 8;
								break;
							}
						sprintf(buf + j, " - %s", ev + k);
						j += strlen(buf + j);
						}
					break;
				case 's':
					v = ev[k++];
					strncpy(buf + j, ev + k, v);
					k += v;
					j += v;
					break;
				case 'S':
					v = ev[k++];
					i++;
					switch(msg[i])
						{
						case '1':
							if(!(v & 0xe7))
								{
								sprintf(buf + j, "-");
								j++;
								}
							else
								{
								if(v & 0x02)
									{
									sprintf(buf + j, "Coer ");
									j += 5;
									}
								if(v & 0x04)
									{
									sprintf(buf + j, "PBK ");
									j += 4;
									}
								if(v & 0x01)
									{
									sprintf(buf + j, "SV ");
									j += 3;
									}
								if((v & 0xe0) == 0xe0)
									{
									sprintf(buf + j, "Canc");
									j += 4;
									}
								}
							break;
						case '2':
							sprintf(buf + j, "Coerc:%c  APBK:%c", (v & 0x01) ? 'A' : 'D',
							    (v & 0x02) ? 'D' : 'A');
							j += 15;
							break;
						case '3':
							sprintf(buf + j, "APBK:%c BlkTas:%c", (v & 0x01) ? 'D' : 'A',
							    (v & 0x10) ? 'A' : 'D');
							j += 15;
							break;
						default:
							break;
						}
					break;
				case 't':
					v = ev[k++];
					sprintf(buf + j, "%s", SyncType[v]);
					j += strlen(buf + j);
					break;
				case 'x':
					v = ev[k++] * 256;
					v += ev[k++];
					sprintf(buf + j, "%04x", v);
					j += 4;
					break;
				case 'X':
					sprintf(buf + j, "%02x", ev[k++]);
					j += 2;
					break;
				default:
					buf[j++] = msg[i];
					break;
				}
			break;
		default:
			buf[j++] = msg[i];
			break;
		}
	}

buf[j++] = '\n';
buf[j++] = '\0';
return buf;
}

static char* event_format_modbus(unsigned char *ev)
{
static char buf[256];
int i, j, k, v;

memcpy(buf, "ModBus: ", 8);
switch(ev[0])
	{
	case 0:
		sprintf(buf + 8, "Ind %03d - Stato comunicazione: %s\n", ev[1],
		    ev[2] ? "On" : "Off");
		break;
	case 1:
		switch(ev[5])
			{
			case 0:
			case 1:
				sprintf(buf + 8, "Ind %03d - Variazione: func %02xh addr %d val %d\n", ev[1], ev[2], ev[3] + ev[4] * 256, ev[6]);
				break;
			case 2:
				sprintf(buf + 8, "Ind %03d - Variazione: func %02xh addr %d val %d\n", ev[1], ev[2], ev[3] + ev[4] * 256, ev[6] + ev[7] * 256);
				break;
			default:
				sprintf(buf + 8, "Ind %03d - Variazione: func %02xh addr %d val %d\n", ev[1], ev[2], ev[3] + ev[4] * 256, ev[6] + (ev[7] << 8) + (ev[8] << 16) + (ev[9] << 24));
				break;
			}
		break;
	case 2:
		sprintf(buf + 8, "Ind %03d - Allineamento: func %02xh seq %d\n", ev[1], ev[2], ev[3] + ev[4] * 256);
		break;
	case 3:
		sprintf(buf + 8, "Ind %03d - Base tempi %d (%d)\n", ev[1], *(int*) (ev + 2), *(int*) (ev + 6));
		break;
	}
return buf;
}

static char* event_format_serrature(unsigned char *ev)
{
static char buf[256];
int i, j, k, v;

memcpy(buf, "Serrature: ", 11);
switch(ev[0])
	{
	case 0:
		sprintf(buf + 11, "Errore %03d\n", ev[1]);
		break;
	case 1:
		sprintf(buf + 11, "Variato stato utente %02d %d\n", ev[1], ev[2]);
		break;
	case 2:
		sprintf(buf + 11, "Utente %02d invalidato\n", ev[1]);
		break;
	case 3:
		sprintf(buf + 11, "Autorizzazione alla programmazione (%02d)\n", ev[1]);
		break;
	case 4:
		sprintf(buf + 11, "Autorizzazione all'apertura (%02d)\n", ev[1]);
		break;
	case 5:
		sprintf(buf + 11, "Serratura bloccata\n");
		break;
	case 6:
		sprintf(buf + 11, "Serratura sbloccata\n");
		break;
	case 7:
		if(ev[1] != 0xff)
			sprintf(buf + 11, "Fascia oraria, giorno %d fascia %d: %02d:%02d - %02d:%02d\n", ev[1], ev[2], ev[3], ev[4], ev[5], ev[6]);
		else
			sprintf(buf + 11, "Fascia oraria, fine\n");
		break;
	case 8:
		sprintf(buf + 11, "Giorni festivi fissi\n");
		break;
	case 9:
		sprintf(buf + 11, "Giorni festivi variabili\n");
		break;
	default:
		sprintf(buf + 11, "Sconosciuto (%d)\n", ev[0]);
		break;
	}
return buf;
}

unsigned char checksum(unsigned char *cmd)
{
unsigned char cks = 0;
int i;

for(i = 0;i < (cmd[3] + 4);i++)
	cks += cmd[i];
return cks;
}

void saet_queue_cmd(unsigned char *cmd, int len)
{
unsigned char *tcmd;

tcmd = (unsigned char*) malloc(len + 5 + 2 + 6 + 2);
tcmd[0] = 2;
tcmd[1] = 100;
tcmd[2] = 128;
tcmd[4] = 0;  // impianto
tcmd[5] = 0;
#if 1
tcmd[3] = len + 2 + 6 + 2;
memcpy(tcmd + 6, "000000", 6);
memcpy(tcmd + 12, cmd, len);
memcpy(tcmd + 12 + len, "\r\n", 2);
tcmd[14 + len] = checksum(tcmd);
#else
tcmd[3] = len + 2+2;
memcpy(tcmd+6, cmd, len);
memcpy(tcmd+6+len, "\r\n", 2);
tcmd[8+len] = checksum(tcmd);
#endif

pthread_mutex_lock(&cmdmutex);
cmdlist = g_slist_append(cmdlist, tcmd);
pthread_mutex_unlock(&cmdmutex);
}

void lara_queue_cmd(unsigned char *cmd, int len)
{
unsigned char *tcmd;

tcmd = (unsigned char*) malloc(len + 5 + 2);
tcmd[0] = 2;
tcmd[1] = 100;
tcmd[2] = 128;
tcmd[3] = len + 2 + 2;
tcmd[4] = 0;  // impianto
tcmd[5] = 0;
memcpy(tcmd + 6, cmd, len);
memcpy(tcmd + 6 + len, "\r\n", 2);
tcmd[8 + len] = checksum(tcmd);

pthread_mutex_lock(&cmdmutex);
cmdlist = g_slist_append(cmdlist, tcmd);
pthread_mutex_unlock(&cmdmutex);
}

unsigned char* saet_get_cmd(int *len)
{
static unsigned char enq[2] =
	{
	5, 100
	};
unsigned char *cmd;
GSList *tmp;

pthread_mutex_lock(&cmdmutex);
if(cmdlist)
	{
	tmp = cmdlist;
	cmdlist = g_slist_next(cmdlist);
	cmd = tmp->data;
	g_slist_free_1(tmp);
	*len = cmd[3] + 5;
	}
else
	{
	cmd = enq;
	*len = 2;
	}
pthread_mutex_unlock(&cmdmutex);
return cmd;
}

void* saet_protocol_poll(void *null)
{
char ack[2] =
	{
	6, 100
	};
unsigned char stx[200], *cmd;
int res, i, j, expect, len;

char buf[80], *p;

//saet_queue_cmd("/593P\0", 6);

while(1)
	{
	cmd = saet_get_cmd(&len);
#ifdef DEBUG_SAETNET
	if(fpdebug)
		{
		int i;
		struct timeval tv;
		struct tm tm;
		gettimeofday(&tv, NULL);
		localtime_r(&(tv.tv_sec), &tm);
		fprintf(fpdebug, "%s %02d/%02d/%02d %02d:%02d:%02d.%03d TX: ", saetparam.data.lan.address, tm.tm_mday, tm.tm_mon + 1, tm.tm_year % 100, tm.tm_hour, tm.tm_min, tm.tm_sec, tv.tv_usec / 1000);
		for(i = 0;i < len;i++)
			fprintf(fpdebug, "%02x", cmd[i]);
		fprintf(fpdebug, "\n");
		}
#endif
	write(saetfd, cmd, len);
	if(cmd[0] == 2)
		{
		set_bg_color(3);
		free(cmd);
		}
	else
		set_bg_color(2);

#if 0
	j = 0;
	res = 0;
	expect = 2;
	do
		{
		i = read(saetfd, stx+res, expect);
		if(i <= 0)
			{
			set_bg_color(0);
			return NULL;
			}
		if(fpdebug)
			{
			int x;
			for(x=0; x<i; x++) fprintf(fpdebug, "%02x", stx[res+x]);
			fprintf(fpdebug, "-");
			}
		res += i;
		expect -= i;
#ifdef DEBUG
			{
			int x;
			printf("idx: %d\n", res);
			for(x=0;x<res;x++) printf("%02x", stx[x]);
			printf("\n");
			}
#endif
		if(expect) continue;

		if(res == 2)
			{
			if(stx[0] == 2) expect = 2;
			else j = 2;
			}
		if(res == 4)
			{
			expect = stx[3] + 1;
			j = expect + 4;
			}
#ifdef DEBUG
			{
			printf("Expect: %d\n", expect);
			}
#endif
		}while (!j || (res < j));
#else
	res = 0;
	while(1)
		{
		i = read(saetfd, stx + res, sizeof(stx) - res);
		if(i <= 0)
			{
			set_bg_color(0);
			return NULL;
			}
		if(fpdebug)
			{
			int x;
			for(x = 0;x < i;x++)
				fprintf(fpdebug, "%02x", stx[res + x]);
			fprintf(fpdebug, "-");
			}
		res += i;

		i = 0;
		while((i < res) && (stx[i] != 2))
			{
			i++;
			res--;
			}
		if(i && res) memmove(stx, stx + i, res);
		if((res >= 4) && (res >= (stx[3] + 5))) break;
		}
#endif

	if(res > 5)
		{
		int x;
		for(x = 0;x < res;x++)
			printf("%02x", stx[x]);
		printf("\n");
		}

#ifdef DEBUG_SAETNET
	if(fpdebug)
		{
		int i;
		struct timeval tv;
		struct tm tm;
		gettimeofday(&tv, NULL);
		localtime_r(&(tv.tv_sec), &tm);
		fprintf(fpdebug, "\n%s %02d/%02d/%02d %02d:%02d:%02d.%03d RX: ", saetparam.data.lan.address, tm.tm_mday, tm.tm_mon + 1, tm.tm_year % 100, tm.tm_hour, tm.tm_min, tm.tm_sec, tv.tv_usec / 1000);
		for(i = 0;i < res;i++)
			fprintf(fpdebug, "%02x", stx[i]);
		fprintf(fpdebug, "\n");
		}
#endif

	if(stx[3])
		{
		sprintf(buf, "%04d %04d %02d/%02d/%02d %02d:%02d:%02d ", stx[4] * 256 + stx[5], stx[6] * 256 + stx[7], stx[8] & 0x1f, stx[9] & 0x0f, stx[10] & 0x7f, stx[11] & 0x3f, stx[12] & 0x3f, stx[13]);
		print_event(buf);

		if(stx[14] == 253)
			p = event_format(EventExString[stx[15]], stx + 16, stx[3] + 5);
		else if(stx[14] == 247)
			{
			switch(stx[15])
				{
				case 0:  // Tebe
				case 1:  // Delphi
					p = event_format(EventEx2String[stx[15]][stx[16]], stx + 17, stx[3] + 5);
					break;
				case 2:  // EiB
					break;
				case 3:  // ModBus
					p = event_format_modbus(stx + 16);
					break;
				case 4:  // Scheduler
					p = "Scheduler\n";
					break;
				case 5:  // Serrature
					p = event_format_serrature(stx + 16);
					break;
				default:
					p = "Classe evento sconosciuta\n";
					break;
				}
			}
		else
			{
			res = stx[14] - EVENT_STRING_BASE;
			if(res >= 0)
				p = event_format(EventString[res], stx + 15, stx[3] + 5);
			else
				p = NULL;
			}
		if(p) print_event(p);
		}

	write(saetfd, ack, 2);
	usleep(saetparam.polling * 500);
	set_bg_color(1);
	usleep(saetparam.polling * 500);
	}
}

//#define UDP_KEY "                        "
//#define UDP_KEY "111111111111111111111111"
/* Default GEMSS */
#define UDP_KEY "_.-.__.--.___.---.____.-"

void* saet_protocol_dgram(void *null)
{
struct sockaddr sa;
struct sockaddr_in sa2;
UDP_event ev, evack;
int fd, fdo, res, len, l, msgid;
unsigned char buf[80], *p;
struct tm *tm;
time_t t;
struct timeval to;
fd_set fds;
enum
{
	UDP_NULL = 0,
	UDP_REQUEST,
	UDP_CONFIRM,
	UDP_SECRET_SET,
	UDP_SECRET_CONF,
	UDP_CONNECTED,
	UDP_REQREG,
	UDP_CHALLENGE,
	UDP_REGISTER
};
struct aes_ctx ctx;
int aes_key_set;
int keepalive, connected;

aes_gen_tabs();
#if 1
aes_key_set = 0;
#else
aes_set_key(&ctx, UDP_KEY, 24);
aes_key_set = 1;
#endif

//fd = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);

fdo = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
//fdo = saetfd;
memset(&sa2, 0, sizeof(struct sockaddr_in));
if(saetparam.data.lan.address[0])
	{
	sa2.sin_family = AF_INET;
	sa2.sin_port = htons(saetparam.data.lan.port);
	sa2.sin_addr.s_addr = inet_addr(saetparam.data.lan.address);
	}
connected = 0;

msgid = 0;

keepalive = KEEPALIVE_PERIOD;

FD_ZERO(&fds);
while(1)
	{
	if(sa2.sin_addr.s_addr && !connected)
		{
		//sa2.sin_port = htons(saetparam.data.lan.port);
		//sa2.sin_addr.s_addr = inet_addr(saetparam.data.lan.address);
		res = connect(fdo, (struct sockaddr*) &sa2, sizeof(struct sockaddr));

		memset(&ev, 0, sizeof(UDP_event));
		ev.stato = bitConnect | bitCommand;
		ev.impianto = SAETCOM_IMPIANTO;
		ev.estensione = 1;
		ev.evento[0] = UDP_REQUEST;
		*(int*) (ev.evento + 1) = 0x12345678;
		len = ev.len = sizeof(UDP_event) - 251;
		if(aes_key_set)
			{
			int i;
			for(i = 0;i < len;i += 16)
				aes_decrypt(&ctx, ((char*) &ev) + i, ((char*) &ev) + i);
			len = i;
			}
//      sendto(fdo, &ev, len, 0, (struct sockaddr*)&sa2, sizeof(struct sockaddr_in));
		send(fdo, &ev, len, 0);
			{
			int i;
			printf("UDP TX: ");
			for(i = 0;i < ev.len;i++)
				printf("%02x", ((unsigned char*) &ev)[i]);
			printf("\n");
			}
		connected = 1;
		}

	FD_SET(saetfd, &fds);
	to.tv_sec = 0;
	to.tv_usec = 100000;
	res = select(saetfd + 1, &fds, NULL, NULL, &to);
	if(res)
		{
		res = sizeof(struct sockaddr);
		len = recvfrom(saetfd, &ev, sizeof(UDP_event), 0, &sa, &res);
			{
			int i;
			printf("UDP RX: ");
			for(i = 0;i < len;i++)
				printf("%02x", ((unsigned char*) &ev)[i]);
			printf("\n");
			}
		if(aes_key_set)
			{
			int i;
			for(i = 0;i < len;i += 16)
				aes_decrypt(&ctx, ((char*) &ev) + i, ((char*) &ev) + i);
				{
				int i;
				printf("  ->  : ");
				for(i = 0;i < len;i++)
					printf("%02x", ((unsigned char*) &ev)[i]);
				printf("\n");
				}
			}
		}
	else
		{
		//printf("timeout\n");
		p = saet_get_cmd(&len);
		if(p[0] == 2)
			{
				{
				int i;
				printf("cmd: ");
				for(i = 0;i < len;i++)
					printf("%02x", p[i]);
				printf("\n");
				}
#warning ---- Comando UDP ----
			evack.msgid = msgid++;
			evack.stato = bitCommand;
			evack.impianto = SAETCOM_IMPIANTO;
			evack.nodo = 0;
			evack.datetime = time(NULL) - BASESECONDS;
			evack.len = sizeof(UDP_event) - 256 + 2;
			evack.estensione = 1;
			evack.evento[0] = 1;	// Delphi
			evack.evento[1] = 0;
#if 0
			if(p[12] == 0x64)
			evack.evento[1] = 43;  // Stato sensori
			else if(p[12] == 0x2f)
			evack.evento[1] = 25;// Memoria utente
			else if(p[12] == 0x78)
			evack.evento[1] = 69;// Allineamento
			if(evack.evento[1]) send(fdo, &evack, evack.len, 0);
#else
			memcpy(evack.evento + 1, p + 12, p[3] - 10);
			evack.len = sizeof(UDP_event) - 256 + p[3] - 10+1;
				{
				int i;
				printf("UDP TX: ");
				for(i = 0;i < evack.len;i++)
					printf("%02x", ((unsigned char*) &evack)[i]);
				printf("\n");
				}
			if(aes_key_set)
				{
				int i;
				l = evack.len;
				for(i = 0;i < l;i += 16)
					aes_encrypt(&ctx, ((char*) &evack) + i, ((char*) &evack) + i);
				l = i;
					{
					int i;
					printf("  <-  : ");
					for(i = 0;i < l;i++)
						printf("%02x", ((unsigned char*) &evack)[i]);
					printf("\n");
					}
				}
			else
				l = evack.len;

			send(fdo, &evack, l, 0);
#endif
			free(p);
			}
		}
	if(!res)
		{
		if(keepalive)
			keepalive--;
		else
			{
			/* Ogni 5 secondi */
			keepalive = KEEPALIVE_PERIOD;

			if(connected == 2)
				{
				evack.msgid = msgid++;
				evack.stato = bitKeepAlive;
				evack.impianto = SAETCOM_IMPIANTO;
				evack.nodo = 0;
				evack.datetime = time(NULL) - BASESECONDS;
				evack.len = sizeof(UDP_event) - 256;
				evack.estensione = 1;
					{
					int i;
					printf("UDP TX: ");
					for(i = 0;i < evack.len;i++)
						printf("%02x", ((unsigned char*) &evack)[i]);
					printf("\n");
					}
				if(aes_key_set)
					{
					int i;
					l = evack.len;
					for(i = 0;i < l;i += 16)
						aes_encrypt(&ctx, ((char*) &evack) + i, ((char*) &evack) + i);
					l = i;
						{
						int i;
						printf("  <-  : ");
						for(i = 0;i < l;i++)
							printf("%02x", ((unsigned char*) &evack)[i]);
						printf("\n");
						}
					}
				else
					l = evack.len;

				send(fdo, &evack, l, 0);
				}
			}

		continue;
		}

	if(!(ev.stato & bitAck))
		{
		int l;

		// E' un evento a cui inviare Ack
		memcpy(&evack, &ev, len);  //sizeof(UDP_event)-256);
		evack.stato |= bitAck;
		evack.stato &= ~bitResend;
		evack.datetime = time(NULL) - BASESECONDS;
		if(!(ev.stato & bitConnect)) evack.len = sizeof(UDP_event) - 256;
			{
			int i;
			printf("UDP TX: ");
			for(i = 0;i < evack.len;i++)
				printf("%02x", ((unsigned char*) &evack)[i]);
			printf("\n");
			}
		if(aes_key_set)
			{
			int i;
			l = evack.len;
			for(i = 0;i < l;i += 16)
				aes_encrypt(&ctx, ((char*) &evack) + i, ((char*) &evack) + i);
			l = i;
				{
				int i;
				printf("  <-  : ");
				for(i = 0;i < l;i++)
					printf("%02x", ((unsigned char*) &evack)[i]);
				printf("\n");
				}
			}
		else
			l = evack.len;
		send(fdo, &evack, l, 0);
		}
	else
		continue;

	if(ev.stato & bitConnect)
		{
		if(!(ev.stato & bitCommand))
			{
#if 1
			if(ev.evento[0] == UDP_CONFIRM)
				{
				printf("invio chiave\n");
				/* Imposto la chiave */
				memset(&ev, 0, sizeof(UDP_event));
				ev.stato = bitConnect | bitCommand;
				ev.impianto = SAETCOM_IMPIANTO;
				ev.estensione = 1;
				ev.evento[0] = UDP_SECRET_SET;
				memcpy(ev.evento + 1, UDP_KEY, 24);
				ev.len = sizeof(UDP_event) - 231;
				send(fdo, &ev, ev.len, 0);
				}
			else if(ev.evento[0] == UDP_SECRET_CONF)
				{
				printf("chiave impostata\n");
				aes_set_key(&ctx, UDP_KEY, 24);
				aes_key_set = 1;
				}
			else if(ev.evento[0] == UDP_REQREG)
				{
				unsigned char C1[16];
				//memset(&ev, 0, sizeof(UDP_event));
				ev.stato = bitConnect | bitCommand;
				ev.impianto = SAETCOM_IMPIANTO;
				ev.estensione = 1;
				ev.evento[0] = UDP_CHALLENGE;
				memcpy(C1, ev.evento + 2, 16);
				memset(ev.evento + 1, 0x12, 16);
				memcpy(ev.evento + 17, C1, 16);
				ev.len = sizeof(UDP_event) - 256 + 33;
					{
					int i;
					printf("UDP TX: ");
					for(i = 0;i < ev.len;i++)
						printf("%02x", ((unsigned char*) &ev)[i]);
					printf("\n");
					}
				sendto(fdo, &ev, ev.len, 0, &sa, sizeof(struct sockaddr_in));
				}
			else if(ev.evento[0] == UDP_REGISTER)
				{
				memcpy(&sa2, &sa, sizeof(struct sockaddr_in));
				printf("centrale registrata\n");
				}
#endif
			}
		continue;
		}

	if(len > 9)
		{
		p = (void*) &ev;
		p[ev.len] = 0;

		t = ev.datetime + BASESECONDS;
		tm = localtime(&t);
		sprintf(buf, "%04d %04d %02d/%02d/%02d %02d:%02d:%02d ", ev.impianto, 0, tm->tm_mday, tm->tm_mon + 1, tm->tm_year % 100, tm->tm_hour, tm->tm_min, tm->tm_sec);
		print_event(buf);

		if(ev.evento[0] == 253)
			p = event_format(EventExString[ev.evento[1]], ev.evento + 2, ev.len);
		else if(ev.evento[0] == 247)
			{
			switch(ev.evento[1])
				{
				case 0:  // Tebe
				case 1:  // Delphi
					p = event_format(EventEx2String[ev.evento[1]][ev.evento[2]], ev.evento + 3, ev.len);
					break;
				case 2:  // EiB
					break;
				case 3:  // ModBus
					p = event_format_modbus(ev.evento + 2);
					break;
				case 4:  // Scheduler
					p = "Scheduler\n";
					break;
				case 5:  // Serrature
					p = event_format_serrature(ev.evento + 2);
					break;
				default:
					p = "Classe evento sconosciuta\n";
					break;
				}
			}
		else
			{
			res = ev.evento[0] - EVENT_STRING_BASE;
			if(res >= 0)
				p = event_format(EventString[res], ev.evento + 1, ev.len);
			else
				p = NULL;
			}
		if(p) print_event(p);
		}
	}
}

int ser_open(const char *dev, int baud, int timeout)
{
int fd;
struct serial_struct info;

fd = open(dev, O_RDWR | O_NOCTTY);
if(fd < 0) return fd;

ser_setspeed(fd, baud, timeout);

ioctl(fd, TIOCGSERIAL, &info);
info.closing_wait = ASYNC_CLOSING_WAIT_NONE;
ioctl(fd, TIOCSSERIAL, &info);
return fd;
}

int ser_setspeed(int fdser, int baud, int timeout)
{
struct termios tio;

memset(&tio, 0, sizeof(tio));

tio.c_cflag = baud | CS8 | CLOCAL | CREAD;
tio.c_iflag = IGNPAR;
tio.c_oflag = 0;
tio.c_lflag = 0;

tio.c_cc[VINTR] = 0;
tio.c_cc[VQUIT] = 0;
tio.c_cc[VERASE] = 0;
tio.c_cc[VKILL] = 0;
tio.c_cc[VEOF] = 4;
tio.c_cc[VTIME] = timeout;
tio.c_cc[VMIN] = timeout ? 0 : 1;
tio.c_cc[VSWTC] = 0;
tio.c_cc[VSTART] = 0;
tio.c_cc[VSTOP] = 0;
tio.c_cc[VSUSP] = 0;
tio.c_cc[VEOL] = 0;
tio.c_cc[VREPRINT] = 0;
tio.c_cc[VDISCARD] = 0;
tio.c_cc[VWERASE] = 0;
tio.c_cc[VLNEXT] = 0;
tio.c_cc[VEOL2] = 0;

tcflush(fdser, TCIOFLUSH);
return tcsetattr(fdser, TCSANOW, &tio);
}

int ser_baud(int baud)
{
switch(baud)
	{
	case 600:
		return B600;
	case 1200:
		return B1200;
	case 2400:
		return B2400;
	case 4800:
		return B4800;
	case 9600:
		return B9600;
	case 19200:
		return B19200;
	case 38400:
		return B38400;
	case 57600:
		return B57600;
	case 115200:
		return B115200;
	default:
		return B9600;
	}
}

int saet_connect()
{
struct sockaddr_in sa;
int res;

switch(saetparam.conn)
	{
	case 1:
		saetfd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
		if(saetfd < 0) return 0;
		sa.sin_family = AF_INET;
		sa.sin_port = htons(saetparam.data.lan.port);
		sa.sin_addr.s_addr = inet_addr(saetparam.data.lan.address);
		res = connect(saetfd, (struct sockaddr*) &sa, sizeof(struct sockaddr));
		if(res < 0)
			{
			close(saetfd);
			return 0;
			}
		pthread_create(&pth, NULL, saet_protocol_poll, NULL);
		break;
	case 2:
		saetfd = ser_open(saetparam.data.ser.device, ser_baud(saetparam.data.ser.baud), 0);
		if(saetfd < 0) return 0;
		pthread_create(&pth, NULL, saet_protocol_poll, NULL);
		break;
	case 3:
		saetfd = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
		if(saetfd < 0) return 0;
		sa.sin_family = AF_INET;
		//sa.sin_port = htons(saetparam.data.lan.port);
		sa.sin_port = htons(4006);
		sa.sin_addr.s_addr = INADDR_ANY;
		res = bind(saetfd, (struct sockaddr*) &sa, sizeof(struct sockaddr));
		if(res < 0)
			{
			close(saetfd);
			return 0;
			}
		pthread_create(&pth, NULL, saet_protocol_dgram, NULL);
		break;
	default:
		return 0;
	}

if(saetparam.debug) fpdebug = fopen("/tmp/comlinux.dbg", "a");

return 1;
}

void saet_disconnect()
{
close(saetfd);
if(saetparam.debug && fpdebug) fclose(fpdebug);
fpdebug = NULL;
}
