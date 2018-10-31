#ifndef _SAET_PROTOCOL_H
#define _SAET_PROTOCOL_H

#include <pthread.h>
#include <semaphore.h>
#include "timeout.h"

#define MAX_NUM_CONSUMER	8
#define DIMBUF	264

typedef struct {
  unsigned char	MsgType;
  unsigned char	NodeID;
  unsigned char	Status;
  unsigned char	Len;
  unsigned char	DeviceID[2];
  unsigned char	Command[DIMBUF];
} Command;

typedef struct {
  unsigned char	MsgType;
  unsigned char	NodeID;
  unsigned char	Status;
  char		Len;
  unsigned char	DeviceID[2];
  unsigned char	Event[80];
} Event;

#define IS_COMMAND(x)	(x->Status || 0x80)
#define IS_EVENT(x)	!(x->Status || 0x80)

typedef struct _ProtDevice ProtDevice;

typedef int (*InitFunction)(ProtDevice *dev, int param);
typedef void* (*PthreadFunction)(void*);

struct _ProtDevice {

  int		type;	/* serial, eth, etc... */
  int		fd;	/* file descriptor */
  int		fd_eth;	/* file descriptor for ethernet */
  short		mask;	/* enable mask */
  pthread_t	th_plugin;	/* thread for plugin */
  pthread_t	th_eth;	/* thread for socket listening */

  Command	cmd;		/* last command received */
  sem_t		sem_cmd;	/* command valid semaphore - NON UTILIZZATO */
  sem_t		sem_ack;	/* command acknoledge semaphore - NON UTILIZZATO */
  int		timeout;	/* timeout counter for ack */

  Event		*lastevent;		/* DO NOT USE THIS! */
  unsigned char	lastCommand[DIMBUF];	/* DO NOT USE THIS! */

  int	i;		/* receiver buffer */
  unsigned char	buf[DIMBUF];

  int	consumer;	/* consumer id */
  
  int	protocol_active;	/* protocol active */
  int	pid;		/* PID del thread */
  int	modem;		/* modem type : 0 - none, 1 - classic, 2 - gsm, 3 - gprs */
  
  timeout_t	*pkt_timeout;
  timeout_t	*send_timeout;
  timeout_t	*plugin_timer;

  InitFunction	command;
  void	*prot_data;
  
  int	exp;		/* receiver buffer */
  Event		event;		/* DO NOT USE THIS! */
};

extern unsigned char LIN[MAX_NUM_CONSUMER];
extern unsigned short LIN_TO[MAX_NUM_CONSUMER];

#define PROT_TIMEOUT	50

#define protSER	0
#define protETH	1
#define protGEN	2

#define STX	2
#define ETX	3
#define ENQ	5
#define ACK	6
#define DLE	16
#define NAK	21
#define ETB	23

#define MODEM_NO	0
#define MODEM_CLASSIC	1
#define MODEM_GSM	2
#define MODEM_GPRS	3

#define PROT_CLASS(p)	((p) & 0x0000ffff)
/* Note: support_find_serial_consumer looks for classes */

#define PROT_NONE		0x00000000
#define PROT_GENERIC		0x00000001
#define PROT_SAETNET		0x00000002
#define PROT_SAETNET_OLD	0x00010002
#define PROT_SAETNET_EX		0x00020002
#define PROT_MODEM		0x00000003
#define PROT_GSM		0x00000004
#define PROT_CONSOLE		0x00000005
#define PROT_CONSOLE_SYS	0x00010005
#define PROT_CONSOLE_LARA	0x00020005
/*
#define PROT_RID_CONS_0		0x00000006
#define PROT_RID_CONS_1		0x00010006
#define PROT_RID_CONS_2		0x00020006
#define PROT_RID_CONS_3		0x00030006
#define PROT_RID_CONS_4		0x00040006
#define PROT_RID_CONS_5		0x00050006
#define PROT_RID_CONS_6		0x00060006
#define PROT_RID_CONS_7		0x00070006
*/
int prot_type(char *prot);
unsigned char prot_calc_cks(Event *ev);

ProtDevice* prot_open(int type, int consumer, char *cdata, int idata);
ProtDevice* prot_init(int type, int fd, int consumer);
int prot_start_recv(ProtDevice *dev);
int prot_close(ProtDevice *dev);
int prot_recv(Command *cmd, ProtDevice *dev);
int prot_send(Event *ev, ProtDevice *dev);
int prot_send_NAK(ProtDevice *dev);

void prot_loop(ProtDevice *dev);
void prot_start();

int prot_plugin_register(char *prot, int code, InitFunction timer, InitFunction command, PthreadFunction loop);
void prot_plugin_load();
void prot_plugin_start(int consumer);
int prot_plugin_command(int consumer, int cmd);

int prot_test_modem(const char *dev);

#endif
