#ifndef _SAET_USER_H
#define _SAET_USER_H

#include "database.h"
#include "event.h"

#undef ATT

struct storico{
  unsigned char code;
  unsigned char dati[63];
};

extern struct storico s;
extern unsigned char LIN[];

/* PROCEDURE PRIVATE */

int ProdStorico(struct storico *msg);
int fs(unsigned char *x);
int fd(unsigned char *x);
int fr(unsigned int *x, unsigned int bit);
void Doncon(int co, int val);
void Doffco(int co);
void Donanl(unsigned int *x);
void Donala(unsigned int *x);
void Doffa(unsigned int *x);
void Dontim(int ti, int val);
void Dontir(int ti, int val);
void Doffti(int ti);
//void Dontil(int ti, int val);
//void Dofftl(int ti);

int cmd_zone_on(int zone, int no_check);
int cmd_zone_off(int zone, int no_check);
int cmd_actuator_on(int idx);
int cmd_actuator_off(int idx);
int cmd_sensor_on(int idx);
int cmd_sensor_off(int idx);
int cmd_modem_send(int idx, char *val);
int cmd_accept_actuators(int grp);
int cmd_accept_act_timers();
int cmd_keyboard_set_state(int idx, int cond, int timeout);
int cmd_keyboard_set_mode(int idx, int cond);
int cmd_keyboard_set_echo(int idx, int cond);
int cmd_display_message(int idx, int pos, char *msg);
int cmd_display_set_mode(int idx, int mode);
int cmd_display_set_cursor_pos(int idx, int pos);

int nvmem_set(int idx, int val);
int nvmem_get(int idx);
int nvmem_set_dim(int dim);

/* PROCEDURE DI RIPRESA */

int REFRESH_SENSORI();
int REFRESH_ATTUATORI();
int REFRESH_TARATURE();

/* PROCEDURE DI ACCETTAZIONE */

#define ACC_ATTUATORI(x) cmd_accept_actuators(x)
#define ACC_ATTEMPO() cmd_accept_act_timers()
#define ACC_MEMORIE(x) cmd_accept_mem(x)
#define ACC_CONTATORI(x) cmd_accept_counters(x)
#define ACC_TIMERS(x) cmd_accept_timers(x)
#define ACC_FR() cmd_accept_fronts()

/* PROCEDURE DI COMANDO */

void SProva(int s);
#define InvioRumore(x) cmd_command_send_noise(x)
#define ResetRumore(x) cmd_command_reset_noise(x)
#define ONRUMORE_TUTTI() cmd_command_send_noise_all();
#define OFFRUMORE_TUTTI() cmd_command_reset_noise_all();
#define COMANDO_TEST(x) cmd_command_start_test(x)
#define FINE_TEST(x) cmd_command_end_test(x)

/* PROCEDURE VARIE */

int MemorizzaChiave(int idx);
int RicevutaChiave();
int RicevutoSegreto(unsigned short v);
int FuoriServizioAlr(int zone);
int InServizioAlr(int zone);
int Periferia_completa();
int ChgOra(char *num);
int ChgData(char *num);
int ChgFaceOra(char *num);

/* PROCEDURE DI GESTIONE MEMORIA DI ACCETTAZIONE */

int SetMU(int x, int odt);
int SetMUrange(int x, int c, int val);
int ClrMUcnt();

/* PROCEDURE E MACRO DI CONTROLLO TASTIERA */

#define Condizione_tastiera(i,c,t) cmd_keyboard_set_state(i,c,t)
#define Modo_tastiera(i,c) cmd_keyboard_set_mode(i,c)
#define Comand_eco(i,c) cmd_keyboard_set_echo(i,c)
#define MSG(i,cu,ms) cmd_display_message(i,cu,ms)
#define Modo_display(i,m) cmd_display_set_mode(i,m)
#define Posizione_cursore(i,cu) cmd_display_set_cursor_pos(i,cu)

/* PROCEDURE DI CONTROLLO PERIFERIA ANALOGICA (MM tipo 2) */

int RIAnalog(int i);

/* PROCEDURE DI CONTROLLO LETTORE DI BADGE (MM tipo 2) */

int ISTime(int i);
int RTime(int i);
int GBadge(int i, int bdgID, int x);

/* MACRO */

int ALCO(int x);
int SAZI(int x);
void ATTZI(int x, int y);
void GesZonaI(int zi);
int tasto_SEN(int i, char t);
int cod_SEN(int i);
int str_SEN(int i);
int rd_EVEN(int i);

void Invent(char gg, char mm, char aa);
void RitardoZona(int zona, int ritardo);

void SetVariante(int var);

#define AL(x)		(char)(x & bitAlarm)
#define ALSA(x)		(char)(x)
#define FSR(x)		(char)(x & bitLHF)
#define FDR(x)		(char)(x & bitHLF)
#define FS(x)		fs((char*)&x)
#define FD(x)		fd((char*)&x)
#define MA(x)		(char)(x & bitSabotage)
#define GU(x)		(char)(x & bitFailure)
#define LM(x)		(char)(x & bitFlashing)
#define ZD(x)		(char)(x & bitStatusInactive)
#define FZ(x)		(char)(x & bitOOS)
#define AZ(x)		(char)(x & bitActive)
#define ALMA(x)		(char)(x & (bitAlarm | bitSabotage))
#define FSAZ(x,y)	(FS(x) && AZ(ZONA[y]))
#define FRAZ(x,y)	((char)(x & bitLHF) && AZ(ZONA[y]))
#define ND_Z(x)		(char)(ZONA[x] & bitLockActive)
#define AT_Z(x)		(char)(ZONA[x] & bitActive)

#define zS(x) x
#define zI(x) (x+128)
#define zT 0

#define ONME(x,c) if (c) database_set_alarm2(&ME[x])
#define OFFM(x,c) if (c) database_reset_alarm2(&ME[x])
#define EQME(x,c) if (c) database_set_alarm2(&ME[x]); else database_reset_alarm2(&ME[x])
#define ONCO(x,n,t,c) if (c) Doncon(x, ((n)<<12)|((t)&0xFFF))
#define OFFC(x,c) if (c) Doffco(x)
#define IncrCO(x) TCO[x]+=0x1000
#define ValCO(x) (TCO[x]>>12)
#define DecrCO(x) if (TCO[x] & 0xf000) TCO[x]-=0x1000
#define OffCO(x) TCO[x]=0
#define ShowCO(x) s.code=Segnalazione_evento;s.dati[0]=TCO[x]>>8;s.dati[1]=TCO[x]&0xff;ProdStorico(&s)
#define ONTM(x,t,c) if (c) Dontim(x,t)
#define ONTR(x,t,c) if (c) Dontir(x,t)
#define OFFT(x,c) if (c) Doffti(x)
//#define ONTL(x,t,c) if (c) Dontil(x,t)
//#define OFFL(x,c) if (c) Dofftl(x)
#define ONAT(x,c) if (c) Donanl(&AT[x])
#define ONAL(x,c) if (c) Donala(&AT[x])
#define OFFA(x,c) if (c) Doffa(&AT[x])
#define EQAT(x,c) if (c) Donanl(&AT[x]); else Doffa(&AT[x])
#define EQAL(x,c) if (c) Donala(&AT[x]); else Doffa(&AT[x])
#define ONATM(x,c) if (c) database_set_alarm(&AT[x])
#define OFFAM(x,c) if (c) database_reset_alarm(&AT[x])
#define EQATM(x,c) if (c) database_set_alarm(&AT[x]); else database_reset_alarm(&AT[x])
#define ONSEM(x,c) if (c) database_set_alarm(&SE[x])
#define OFFSM(x,c) if (c) database_reset_alarm(&SE[x])
#define EQSEM(x,c) if (c) database_set_alarm(&SE[x]); else database_reset_alarm(&SE[x])
#define DIS(x,c) if (c) cmd_zone_off(x,0)
#define ATT(x,c) if (c) cmd_zone_on(x,0)
#define ATTCA(x,c,cc) if (c) cmd_zone_on(x,cc)
#define ATTDIS(x,c) if (c) cmd_zone_on(x,0); else cmd_zone_off(x,0)
#define ATTDCA(x,c,cc) if (c) cmd_zone_on(x,cc); else cmd_zone_off(x,0)
#define INVZ(x,c) if (c) {if(ZONA[x] & bitActive) cmd_zone_off(x,0); else cmd_zone_on(x,0);}
#define BDI(x,c) if (c) ZONA[x] |= bitLockActive
#define SDI(x,c) if (c) ZONA[x] &= ~bitLockActive
#define BAT(x,c) if (c) ZONAEX[x] |= bitLockDisactive
#define SAT(x,c) if (c) ZONAEX[x] &= ~bitLockDisactive
#define ISS(x,c) if (c) cmd_sensor_on(x)
#define FSS(x,c) if (c) cmd_sensor_off(x)
#define ISA(x,c) if (c) cmd_actuator_on(x)
#define FSA(x,c) if (c) cmd_actuator_off(x)
#define num_SEN(x) SE[((x) << 3) + 4]
#define evn_SEN(x) SE[((x) << 3) + 4]
#define key_SEN(x) (CHIAVE_TRASMESSA==x)
#define INSTOR4(c,cd,d1,d3,d4) if (c) {s.code=cd;s.dati[0]=(d1)>>8;s.dati[1]=(d1)&0xff;s.dati[2]=d3;s.dati[3]=d4;ProdStorico(&s);}
#define INSTOR(c,cd,d1,d3) if (c) {s.code=cd;s.dati[0]=(d1)>>8;s.dati[1]=(d1)&0xff;s.dati[2]=d3;ProdStorico(&s);}
#define INSTORW(c,cd,d1) if (c) {s.code=cd;s.dati[0]=(d1)>>8;s.dati[1]=(d1)&0xff;ProdStorico(&s);}
#define INSTORB(c,cd,d1) if (c) {s.code=cd;s.dati[0]=d1;ProdStorico(&s);}
#define INSTORC(c,cd) if (c) {s.code=cd; ProdStorico(&s);}
//#define DYALL(i,ns,ms,c) if (c) D_allarme(i,ns,ms)
#define RUMORE(n,c) if (c) AT[(n)<<3] |= bitAbilReq | bitNoise; else {AT[(n)<<3] &= ~bitNoise; AT[(n)<<3] |= bitAbilReq}
//#define TELEFONA1(num) cmd_modem_number_event(N_telefonico_1,num)
//#define TELEFONA2(num) cmd_modem_number_event(N_telefonico_2,num)
//#define SModem(li,s) cmd_modem_send(li,s)

#define MEMSET(idx,val) nvmem_set(idx,val);
#define MEMGET(idx) nvmem_get(idx);
#define MEMSETDIM(dim) nvmem_set_dim(dim);

#define Inizializzazione(x) user_init(x)
#define Utente(x) user_program(x)

#define si	1
#define no	0

#endif
