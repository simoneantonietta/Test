typedef struct {
  int conn;
  union {
    struct {
      char address[16];
      int port;
    } lan;
    struct {
      char device[16];
      int baud;
    } ser;
  } data;
  int polling;
  int debug;
} saetparam_t;

extern saetparam_t saetparam;
extern char *saet_test_path;

void saet_load_param();
void saet_save_param();

int saet_connect();
void saet_disconnect();

void saet_queue_cmd(unsigned char *cmd, int len);
void lara_queue_cmd(unsigned char *cmd, int len);
