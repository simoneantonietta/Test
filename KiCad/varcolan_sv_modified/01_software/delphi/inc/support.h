#ifndef _SAET_SUPPORT_H
#define _SAET_SUPPORT_H

extern const char DevGpioA[];
extern const char StatusFileName[];

#include "protocol.h"

#define PBOOK_DIM	128
#define DATA_DIR	"/saet/data"

typedef struct {
  int		configured;
  int		type;
  union
  {
    struct
    {
      int	port;
    } eth;
    struct
    {
      int	baud1;
      int	baud2;
    } serial;
  } data;
  char		*param;
  int		progr;
  ProtDevice	*dev;
} Consumer_t;

typedef struct {
  char	Phone[24];
  char	*Name;
  char	Abil;
} PhoneBook_t;

#define PB_TX		0x01
#define PB_RX		0x02
#define PB_SMS_TX	0x04
#define PB_SMS_RX	0x08
#define PB_GSM_TX	0x10
#define PB_GSM_RX	0x20

typedef struct {
  unsigned char		NodeID;
  unsigned short	DeviceID;
  unsigned char		Rings;
  PhoneBook_t		PhoneBook[PBOOK_DIM];
  char			PhonePrefix[8];
  int			Retries;
  int			Interval;
  Consumer_t		consumer[MAX_NUM_CONSUMER];
  int			Log;
  int			LogLimit;
  int			debug;
  int			Variant;
  char*			VoltoServerName;
  int			VoltoMaxConn;
  int			VoltoMinConn;
  int			TebeBadgeDisableOnErr;
  int			VoltoServerUpdate;
} Config_t;

struct support_list
{
  char	*cmd;
  int	param;
  int	timeout;
  struct support_list *next;
};

#define CALLED_NUMBERS	20
struct support_numbers_array {
  time_t	time;
  char		*number;
  int		success;
};

extern struct support_numbers_array support_called_numbers[CALLED_NUMBERS];
extern int support_called_number_idx;

extern struct support_numbers_array support_received_numbers[CALLED_NUMBERS];
extern int support_received_number_idx;

extern Config_t config;

char* support_delim(char *str);
int support_find_serial_consumer(int type, int start);

struct support_list* support_list_add_prev(struct support_list *list, char *cmd, int timeout, int param);
struct support_list* support_list_add_next(struct support_list *list, char *cmd, int timeout, int param);
struct support_list* support_list_add(struct support_list *list, char *cmd, int timeout, int param);
struct support_list* support_list_del(struct support_list *list);

void support_log(char *log);
void support_log2(char *log);
void support_logmm(char *log);
void support_log_hook(void (*func)(char*));
void support_save_last_log();

int support_called_number(char *number);
int support_received_number(char *number);
void support_called_number_success();

void support_store_current_values();
void support_save_status();
int support_load_status();
int support_reset_status();

int support_sensor_valid(int idx);
int support_actuator_valid(int idx);

int PhoneBook_ins(int pbook_number, char *pbook_name, char *n_to_store, char flags);
int PhoneBook_find(char *number);

int support_baud(int baud);

#define BOARD_UNKNOWN	-1
#define BOARD_ANUBI	0
#define BOARD_ISIDE	1
#define BOARD_ATHENA	2

int support_get_board();

#define DEBUG_PID_MAIN	8
#define DEBUG_PID_MSEND	9
#define DEBUG_PID_MRECV	10
#define DEBUG_PID_MLOOP	11
#define DEBUG_PID_TEBEU	12
#define DEBUG_PID_VOLTO	13
//#define DEBUG_PID_MAIN	12
//#define DEBUG_PID_MAIN	13
//#define DEBUG_PID_MAIN	14
#define DEBUG_PID_MAX	14
extern pthread_t debug_pid[DEBUG_PID_MAX];
extern int debug_mloop_phase;
int support_getpid();

void utf8convert(char *in, char *out);

#define MKINT(a,b,c,d) (((a)<<24)|((b)<<16)|((c)<<8)|(d))
unsigned int uchartoint(unsigned char *data);
unsigned short uchartoshort(unsigned char *data);
void inttouchar(int num, unsigned char *data, int size);

#endif
