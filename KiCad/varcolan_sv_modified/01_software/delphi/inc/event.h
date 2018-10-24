#ifndef _SAET_EVENT_H
#define _SAET_EVENT_H

#define EV_OFFSET	150

#define Allarme_Sensore			EV_OFFSET +  0
#define Ripristino_Sensore		EV_OFFSET +  1
#define Guasto_Periferica		EV_OFFSET +  2
#define Fine_Guasto_Periferica  	EV_OFFSET +  3
#define Manomissione_Dispositivo	EV_OFFSET +  4
#define Fine_Manomis_Dispositivo	EV_OFFSET +  5
#define Manomissione_Contenitore	EV_OFFSET +  6
#define Fine_Manomis_Contenitore	EV_OFFSET +  7
#define Attivata_zona			EV_OFFSET +  8
#define Disattivata_zona		EV_OFFSET +  9
#define Attivazione_impedita		EV_OFFSET + 10
#define Sensore_In_Servizio		EV_OFFSET + 11
#define Sensore_Fuori_Servizio		EV_OFFSET + 12
#define Attuatore_In_Servizio		EV_OFFSET + 13
#define Attuatore_Fuori_Servizio	EV_OFFSET + 14
#define Ricezione_codice_errato 	EV_OFFSET + 15
#define Tipo_ModuloMaster		EV_OFFSET + 16
#define Periferica_Incongruente		EV_OFFSET + 17
#define Periferica_Manomessa		EV_OFFSET + 18
#define Periferica_Ripristino		EV_OFFSET + 19
#define Sospesa_attivita_linea		EV_OFFSET + 20
#define Chiave_Falsa			EV_OFFSET + 21
#define Sentinella_on			EV_OFFSET + 22
#define Sentinella_off			EV_OFFSET + 23
#define Sentinella_off_timeout		EV_OFFSET + 24
#define No_ModuloMasterTx		EV_OFFSET + 25
#define ErrRx_ModuloMaster		EV_OFFSET + 26
#define Errore_messaggio_host		EV_OFFSET + 27
#define Segnalazione_evento		EV_OFFSET + 28
#define Periferiche_presenti		EV_OFFSET + 29
#define Lista_zone			EV_OFFSET + 30
#define Lista_sensori			EV_OFFSET + 31
#define Lista_attuatori			EV_OFFSET + 32
#define Guasto_Sensore			EV_OFFSET + 33
#define Variazione_ora			EV_OFFSET + 34
#define Codici_controllo		EV_OFFSET + 35
#define Stato_QRA_Ronda			EV_OFFSET + 36
#define Accettato_allarme_Sensore	EV_OFFSET + 37
#define Mancata_punzonatura		EV_OFFSET + 38
#define ON_telecomando			EV_OFFSET + 39
#define Parametri_taratura		EV_OFFSET + 40
#define Stato_prova			EV_OFFSET + 41
#define Test_attivo_in_corso		EV_OFFSET + 42
#define Risposta_SCS			EV_OFFSET + 43
#define Fine_Guasto_Sensore		EV_OFFSET + 44
#define Invio_codice_segreto		EV_OFFSET + 45
#define Periferiche_previste		EV_OFFSET + 46
#define Sensore_in_Allarme		EV_OFFSET + 47
#define Variata_fascia_oraria		EV_OFFSET + 48
#define Invio_host_no_storico		EV_OFFSET + 49
#define InvioMemoria			EV_OFFSET + 50
#define FineInvioMemoria		EV_OFFSET + 51
#define Liste_ronda			EV_OFFSET + 52
#define Orari_ronda			EV_OFFSET + 53
#define Giorni_festivi			EV_OFFSET + 54
#define Fasce_orarie			EV_OFFSET + 55
#define ON_attuatore			EV_OFFSET + 56
#define Transitato_Ident		EV_OFFSET + 57
#define Entrato_Ident			EV_OFFSET + 58
#define Uscito_Ident			EV_OFFSET + 59
#define Codice_valido			EV_OFFSET + 60
#define Chiave_valida			EV_OFFSET + 61
#define Operatore			EV_OFFSET + 62
#define Punzonatura			EV_OFFSET + 63
#define Spegnimento			EV_OFFSET + 64
#define Reset_fumo			EV_OFFSET + 65
#define Livello_abilitato		EV_OFFSET + 66
#define Variato_segreto			EV_OFFSET + 67
#define Conferma_ricezione_modem	EV_OFFSET + 68
#define N_telefonico_1    		EV_OFFSET + 69
#define N_telefonico_2     		EV_OFFSET + 70
#define StatoBadge			EV_OFFSET + 71
#define Evento_Sensore			EV_OFFSET + 72
#define Lista_MU			EV_OFFSET + 73
#define Lista_ZS			EV_OFFSET + 74
#define Str_DescrDisp			EV_OFFSET + 75
#define Str_DescrZona			EV_OFFSET + 76
#define Variato_stato_Festivo		EV_OFFSET + 77
#define Valori_analogici		EV_OFFSET + 78
#define Badge_letto			EV_OFFSET + 79
#define Governo_linee_in_servizio	EV_OFFSET + 104
#define Governo_linee_fuori_servizio	EV_OFFSET + 105

#define MAX_NUM_EVENT	106

#define Evento_Esteso		EV_OFFSET + 103
#define Ex_Stringa_GSM		0
#define Ex_Stringa_Modem	1
#define Ex_Stringa		2

#define MAX_NUM_EVENT_EX	4

#define Evento_Esteso2		EV_OFFSET + 97
#define Ex2_Lara		0
#define Ex2_Delphi		1
//#define Ex2_EIB_KNX		2
//#define Ex2_ModBus		3
//#define Ex2_Scheduler		4

#define MAX_NUM_EVENT_EX2	103

#define Ex2_Inizio_test		0
#define Ex2_Fine_test		1

#define Ex2_Orologio_FS		82
#define Ex2_Orologio_OK		83
#define Ex2_MancaRete		84
#define Ex2_RipristinoRete	85
#define Ex2_GuastoBatteria	86
#define Ex2_RipristinoBatteria	87
#define Ex2_InizioManutanzione	88
#define Ex2_FineManutenzione	89
#define Ex2_Fine_test_negativo	90
#define Ex2_InizioStraordinario	91
#define Ex2_FineStraordinario	92
#define Ex2_Ins_Anticipato	93
#define Ex2_Attivata_zona_rit	94
#define Ex2_Stato_centrale	95
#define Ex2_On_attuatore	96
#define Ex2_Off_attuatore	97
#define Ex2_SalvataggioDB	98
#define Ex2_Fuori_servizio_auto	99
#define Ex2_In_servizio_auto	100
#define Ex2_Allineamento	101
// Usato solo dall'ISI
#define Ex2_Stato_connessione	102

#endif
