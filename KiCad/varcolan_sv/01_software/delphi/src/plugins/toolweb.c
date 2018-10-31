#include "database.h"
#include "support.h"
#include "master.h"
#include "delphi.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <dlfcn.h>

static int gfd = -1;
static int userid;

static int toolweb_868_perif_lastid = -1;
static int toolweb_868_perif_managed = 0;
static int toolweb_868_perif_preload = 1;

#define KEEPALIVE_TIMEOUT 30
static timeout_t *toolweb_868_keepalive_timeout = NULL;
static timeout_t *toolweb_868_perif_timeout = NULL;
static timeout_t *toolweb_868_perif_notify_timeout = NULL;

unsigned int (*SEprog_p)[128/32] = NULL;

static const int NumSens[8] = {0, 2, 1, 2, 1, 1, 0, 0};

void toolweb_868_keepalive(void *null1, int null2)
{
  send(gfd, " ", 1, 0);
  timeout_on(toolweb_868_keepalive_timeout, toolweb_868_keepalive, NULL, 0, KEEPALIVE_TIMEOUT);
}

void SendReply(char *reply)
{
#if 0
  struct timeval tv;
  gettimeofday(&tv, NULL);
  printf("%3d.%02d T: %s\n", tv.tv_sec%1000, tv.tv_usec/10000, reply);
#endif
  
  send(gfd, reply, strlen(reply), 0);
  timeout_on(toolweb_868_keepalive_timeout, toolweb_868_keepalive, NULL, 0, KEEPALIVE_TIMEOUT);
}

void skipspace(char **json)
{
  while(**json && (**json == ' ')) (*json)++;
}

#define CheckChar(json,c) \
	skipspace(&json); \
	if(*json != c) return -1;

static const char alphabet[] = "0123456789ABCDEFGHJKLMNPQRSTUVWXYZ";

void tobase34(int num, char *text, int len)
{
  text[len] = 0;
  while(len)
  {
    if((num&0x3f) < (sizeof(alphabet)-1))
    {
      text[len-1] = alphabet[num&0x3f];
      num >>= 6;
      len--;
    }
    else
    {
      text[0] = 0;
      return;
    }
  }
}

int frombase34(char *text)
{
  int i, num;
  
  num = 0;
  while(*text)
  {
    num <<= 6;
    for(i=0; i<(sizeof(alphabet)-1)&&(alphabet[i]!=*text); i++);
    if(i<(sizeof(alphabet)-1))
    {
      num |= i;
      text++;
    }
    else
      return -1;
  }
  return num;
}

/* Le catene da Browser a DLL e da DLL a Browser sono diverse.
   Il Browser è UCS-16, dialoga con lo strato C che trasforma in UTF-8,
   e quindi alla DLL arriva il testo in UTF-8. Verso il Browser la
   comunicazione è diretta, quindi il messaggio json deve produrre l'unicode. */

/* Converte da UTF8 a ISO */
char* jsontoiso(char **buf)
{
  static char tbuf[16];
  char h[8];
  int i, p, n;
  
  h[4] = 0;
  p = 0;
  while(**buf != '"')
  {
    if(**buf == '&')
    {
      (*buf)++;
      if(!strncmp(*buf, "amp;", 4))
      {
        tbuf[p++] = '&';
        *buf += 3;
      }
      else if(!strncmp(*buf, "lt;", 3))
      {
        tbuf[p++] = '<';
        *buf += 2;
      }
      else if(!strncmp(*buf, "gt;", 3))
      {
        tbuf[p++] = '>';
        *buf += 2;
      }
    }
    else if(**buf == '\\')
    {
      (*buf)++;
      switch(**buf)
      {
        case '\\':
        case '/':
        case '"':
          if(p < 16) tbuf[p++] = **buf;
          break;
        case 'u':
          memcpy(h, *buf+1, 4);
          sscanf(h, "%x", &i);
          if(i < 0x100)
            tbuf[p++] = i;
          else if(i == 0x20ac)	// euro
            tbuf[p++] = 0xa4;
          *buf += 4;
          break;
        default:
          break;
      }
    }
    else if(**buf & 0x80)
    {
      if((**buf & 0xe0) == 0xc0)
      {
        n = 1;
        i = **buf & 0x1f;
      }
      else if((**buf & 0xf0) == 0xe0)
      {
        n = 2;
        i = **buf & 0x0f;
      }
      else if((**buf & 0xf8) == 0xf0)
      {
        n = 3;
        i = **buf & 0x07;
      }
      else if((**buf & 0xfc) == 0xf8)
      {
        n = 4;
        i = **buf & 0x03;
      }
      else if((**buf & 0xfe) == 0xfc)
      {
        n = 5;
        i = **buf & 0x01;
      }
      
      while(n)
      {
        (*buf)++;
        i <<= 6;
        i |= (**buf & 0x3f);
        n--;
      }
      
      if(i < 0x100)
        tbuf[p++] = i;
      else if(i == 0x20ac)	// euro
        tbuf[p++] = 0xa4;
    }
    else if(p < 16)
      tbuf[p++] = **buf;
    
    (*buf)++;
  }
  (*buf)++;
  if(p < 16) tbuf[p] = 0;
  
  return tbuf;
}

void isotojson(char *dest, unsigned char *buf, int len)
{
  /* Ogni carattere speciale può diventare carattere unicode su 6 caratteri */
  int i;
  
  for(i=0; i<len; i++)
  {
    if(!*buf) break;
    if(*buf < 0x80)
    {
      switch(*buf)
      {
        case '&':
          /* Non è un problema json, è un problema xmpp/xml */
          memcpy(dest, "&amp;", 5);
          dest += 5;
          break;
        case '<':
          /* Non è un problema json, è un problema xmpp/xml */
          memcpy(dest, "&lt;", 4);
          dest += 4;
          break;
        case '>':
          /* Non è un problema json, è un problema xmpp/xml */
          memcpy(dest, "&gt;", 4);
          dest += 4;
          break;
        case '\\':
        case '/':
        case '"':
          dest[0] = '\\';
          dest[1] = *buf;
          dest += 2;
          break;
        case '\n':
          dest[0] = '\\';
          dest[1] = 'n';
          dest += 2;
          break;
        case '\r':
          /* skip */
          break;
        default:
          if(*buf >= 32)
          {
            *dest = *buf;
            dest++;
          }
          else
          {
            /* Non dovrebbero mai esserci caratteri non stampabili!
               C'è qualcosa che non va, quindi ritorno immediatamente. */
            *dest = 0;
            return;
          }
          break;
      }
    }
    else
    {
      switch(*buf)
      {
        case 0xa4:
          // Euro
          memcpy(dest, "\\u20ac", 6);
          dest += 6;
          break;
        default:
          sprintf(dest, "\\u00%02x", *buf);
          dest += 6;
          break;
      }
    }
    buf++;
  }
  *dest = 0;
}

typedef struct {
  int dim;
  int pos;
  char *buffer;
} JSONbuffer;

void json_init_buffer(JSONbuffer *json)
{
  if(!json) return;
  json->dim = 0;
  json->pos = 0;
  json->buffer = NULL;
}

void json_reset_buffer(JSONbuffer *json)
{
  if(!json) return;
  if(json->buffer) json->buffer[0] = 0;
  json->pos = 0;
}

void json_free_buffer(JSONbuffer *json)
{
  if(!json) return;
  free(json->buffer);
  json->dim = 0;
  json->pos = 0;
  json->buffer = NULL;
}

void json_add_string(JSONbuffer *json, char *buffer)
{
  int l1, l2;
  char *p;
  
  if(!json->dim)
  {
    json->buffer = malloc(0x10000);
    if(json->buffer)
    {
      json->buffer[0] = 0;
      json->dim = 0x10000;
    }
    else
      return;
  }
  
  l1 = strlen(buffer);
  //l2 = strlen(json->buffer);
  l2 = json->pos;
  
  if((l1+l2) > json->dim)
  {
    p = realloc(json->buffer, json->dim*2);
    if(p)
    {
      json->buffer = p;
      json->dim *= 2;
    }
  }
  
  if(json->buffer)
  {
    //strcat(json->buffer, buffer);
    strcpy(json->buffer+json->pos, buffer);
    json->pos += l1;
  }
}

void json_add_integer(JSONbuffer *json, int i)
{
  char buffer[16];
  
  sprintf(buffer, "%d", i);
  json_add_string(json, buffer);
}

void configurazionetoolweb()
{
  JSONbuffer json;
  int i, firstc;
  char buf[256], *descr;
  
  /* Nella configurazione inserisco tutti i sensori delle periferiche 868 virtuali,
     solo per indicare le descrizioni nelle associazioni sensori. */
  json_init_buffer(&json);
  json_add_string(&json, "/json {\"title\":\"");
  gethostname(buf, 32);
  buf[31] = 0;
  json_add_string(&json, buf);
  json_add_string(&json, "\", \"config\":[");
#if 0
  firstc = 1;
  for(i=0; i</*N_LINEE*2*/16*128; i++)
  {
    if(TipoPeriferica[i/8] == 0x0f)
    {
      if(!firstc) json_add_string(&json, ",");
      firstc = 0;
      
      string_sensor_name(i, NULL, &descr);
      if(!strcmp(descr, "-")) descr = "";
      
      sprintf(buf, "{\"sensor\":{\"idx\":%d,\"descr\":\"%s\",\"type\":0,\"zones\":[],\"in\":0,\"out\":0,\"count\":0,\"interval\":0}}", i, descr);
      json_add_string(&json, buf);
    }
  }
#endif
  json_add_string(&json, "], \"user\":");
  json_add_integer(&json, userid);
  json_add_string(&json, "}");
  SendReply(json.buffer);
  json_free_buffer(&json);
}

#define RADIO_TIPO_CONTATTO	1
#define RADIO_TIPO_IR		2
#define RADIO_TIPO_ATTUATORE	3
#define RADIO_TIPO_SIRENA_EXT	4
#define RADIO_TIPO_SIRENA_INT	5

char json_descr_buffer[128];

char* json_descr_sens(int conc, int sens)
{
  char *descr, isodescr[80];
  
  string_sensor_name((conc>>1)*128+sens, NULL, &descr);
  if(!strcmp(descr, "-")) descr = "";
  isotojson(isodescr, descr, strlen(descr));
  //sprintf(json_descr_buffer, "\"%d-%03d - %s\"", conc>>1, sens, isodescr);
  sprintf(json_descr_buffer, "\"%04d - %s\"", (conc>>1)*128+sens, isodescr);
}

char* json_descr(int tipo, int conc, int s, int link[])
{
  char *descr, isodescr[80];
  
  if(link[s] < 0) return "\"\"";
  
  switch(tipo)
  {
    case RADIO_TIPO_CONTATTO:
    case RADIO_TIPO_IR:
/*
      string_sensor_name((conc>>1)*128+link[s], NULL, &descr);
      if(!strcmp(descr, "-")) descr = "";
      isotojson(isodescr, descr, strlen(descr));
      sprintf(json_descr_buffer, "\"%d-%03d - %s\"", conc>>1, link[s], isodescr);
*/
      json_descr_sens(conc, link[s]);
      break;
    case RADIO_TIPO_ATTUATORE:
      if(s == 0)
      {
        sprintf(json_descr_buffer, "\"Attuatore radio #%d-%02d\"", conc, link[s]);
      }
      else
      {
        string_actuator_name((conc>>1)*128+link[s], NULL, &descr);
        if(!strcmp(descr, "-")) descr = "";
        isotojson(isodescr, descr, strlen(descr));
        sprintf(json_descr_buffer, "\"%d-%03d - %s\"", conc>>1, link[s], isodescr);
      }
      break;
    case RADIO_TIPO_SIRENA_EXT:
    case RADIO_TIPO_SIRENA_INT:
      sprintf(json_descr_buffer, "\"Sirena radio #%d-%02d\"", conc, link[s]);
      break;
    default:
      return "\"\"";
  }
  return json_descr_buffer;
}

static timeout_t *refresh_map_delay = NULL;
void json_refresh_map(void *null, int delay);

void* json_refresh_map_th(void *null)
{
  pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
  json_refresh_map(NULL, 0);
}

void json_refresh_map_to(void *null, int delay)
{
  pthread_t pth;
  pthread_create(&pth,  NULL, (PthreadFunction)json_refresh_map_th, NULL);
  pthread_detach(pth);
}

void json_refresh_map(void *null, int delay)
{
  if(delay)
  {
    if(!refresh_map_delay)  refresh_map_delay = timeout_init();
    /* Per la gestione ritardata creo un thread appposito in modo da non
       bloccare il main loop, che deve poter girare velocemente.
       Prima dell'ottimizzazione json_buffer, la creazione della risposta
       richiedeva molti secondi e cresceva esponenzialmente con il numero
       di concentratori. Con l'ottimizzazione il tempo è più lineare ma
       comunque nell'ordine di mezzo secondo per 8 concentratori e potrebbe
       arrivare anche a 1s o più. Se il main loop fosse bloccato non girerebbe
       in modo regolare il programma utente, i vari timer, l'orologio ecc. */
    timeout_on(refresh_map_delay, json_refresh_map_to, NULL, 0, delay);
  }
  else
  {
    JSONbuffer reply;
    char buffer[64];
    int conc, i, firstc, first;
    
    if(refresh_map_delay) timeout_off(refresh_map_delay);
    
    json_init_buffer(&reply);
    
    json_add_string(&reply, "/json {\"auto\":{\"linklist\":[");
    firstc = 1;
    for(conc=0; conc</*N_LINEE*2*/16*2; conc++)
    {
      if((TipoPeriferica[(conc>>2)*32+(conc&2)*8+2+(conc&1)*5] != SC8R2) ||
         !(StatoPeriferica[(conc>>2)*4+(conc&2)] & (1<<(2+(conc&1)*5))))
        continue;
      
      if(!firstc) json_add_string(&reply, ",");
      firstc = 0;
      //sprintf(buffer, "{\"line\":%d,\"conc\":%d,\"sensors\":[", conc/2, 2+(conc&1)*5);
      sprintf(buffer, "{\"line\":%d,\"conc\":%d,\"sensors\":[", conc/4, (conc&2)*8+2+(conc&1)*5);
      json_add_string(&reply, buffer);
      first = 1;
      for(i=0; i<128; i++)
      {
        if(SEprog_p && !(SEprog_p[conc>>1][i/32] & (1<<(i&31))))
        {
          if(!first) json_add_string(&reply, ",");
          json_add_integer(&reply, ((conc>>1)*128)+i);
          first = 0;
        }
      }
      json_add_string(&reply, "],\"sensdescr\":[");
      first = 1;
      for(i=0; i<128; i++)
      {
        if(SEprog_p && !(SEprog_p[conc>>1][i/32] & (1<<(i&31))))
        {
          if(!first) json_add_string(&reply, ",");
          json_descr_sens(conc, i);
          json_add_string(&reply, json_descr_buffer);
          first = 0;
        }
      }
      json_add_string(&reply, "],\"radioactuators\":[");
      first = 1;
      /* Verifica quanti attuatori siano già programmati sul concentratore.
         Se inferiori a N_RADIO_ACT allora metto la lista dei sensori
         disponibili da associare. */
      for(i=0; i<128; i++)
      {
        if(SEprog_p && !(SEprog_p[conc>>1][i/32] & (1<<(i&31))))
        {
          if(!first) json_add_string(&reply, ",");
          json_add_integer(&reply, ((conc>>1)*128)+i);
          first = 0;
        }
      }
      json_add_string(&reply, "],\"actuators\":[");
      /* L'array "actuators" DEVE essere sempre vuoto affinché non appaia
         la tendina per la selezione del relè associato. L'attuatore associato
         è quello corrispondente al sensore e viene comandato dal programma
         utente. */
      json_add_string(&reply, "],\"radiosirens\":[");
      first = 1;
      /* Verifica quante sirene siano già programmate sul concentratore.
         Se inferiori a N_RADIO_SIR allora metto la lista dei sensori
         disponibili da associare. */
      for(i=0; i<128; i++)
      {
        if(SEprog_p && !(SEprog_p[conc>>1][i/32] & (1<<(i&31))))
        {
          if(!first) json_add_string(&reply, ",");
          json_add_integer(&reply, ((conc>>1)*128)+i);
          first = 0;
        }
      }
      json_add_string(&reply, "]}");
    }
    json_add_string(&reply, "],\"areas\":[{\"area\":-1,\"descr\":\"\"}");
    for(i=0; i<8; i++)
    {
      if(MMPresente[i] == 0)
      {
        for(conc=(i<<5); conc<((i<<5)+32); conc++)
          if(TipoPeriferica[(i<<5)|2] == SC8R2) break;
        if(conc<((i<<5)+32))
        {
          sprintf(buffer, ",{\"area\":%d,\"descr\":\"mm%d\"}", i<<5, i);
          json_add_string(&reply, buffer);
          for(conc=(i<<5); conc<((i<<5)+32); conc++)
            if(TipoPeriferica[conc] == SC8R2)
            {
              sprintf(buffer, ",{\"area\":%d,\"descr\":\"%d-%02d\"}", conc, conc>>5, conc&0x1f);
              json_add_string(&reply, buffer);
            }
        }
      }
    }
    json_add_string(&reply, "]}}");
    SendReply(reply.buffer);
    json_free_buffer(&reply);
  }
}

int json_check_message(char *json)
{
  int cont;
  
  /* Casomai ricevessi roba sporca in testa. */
  json = strstr(json, "/json");
  if(!json) return 0;
  
  cont = 0;
  json += 5;
  while(*json)
  {
    if(*json == '{')
      cont++;
    else if(*json == '}')
    {
      cont--;
      if(cont == 0) return 1;
    }
    json++;
  }
  
  return 0;
}

static char *Nome_param[] = {"Impulsi", "Tempo (s)", "Durata", "Sensibilit\\u00e0", "Accecamento",
  "Sensib. urto", "Sensib. mov.", "Allarmi", "Finestra", "Periodo (s)", "Durata (s)",
  "Sensibilit\\u00e0", "Tipo 12", "Tipo 13", "Tipo 14", "Tipo 15"};

static const int durata_lut868[] = {15, 30, 60, 90, 180, 210, 240};

int ParamFuncNull(int val)
{
  /* Il valore non viene modificato */
  return val;
}

int ParamFuncSensUrto_touser(int val)
{
  if(!val)
    val = 0;
  else
    val = 129-val;
  return val;
}

int ParamFuncSensUrto_fromuser(int val)
{
  if(val == 1) val = 2;
  if(val > 127) val = 127;
  if(val > 0) val = 129-val;
  return val;
}

int ParamFuncSensMov_touser(int val)
{
  if(!val)
    val = 0;
  else
    val = 33-val;
  return val;
}

int ParamFuncSensMov_fromuser(int val)
{
  if(val == 1) val = 2;
  if(val > 31) val = 31;
  if(val > 0) val = 33-val;
  return val;
}

#ifdef SECURGROSS
//const int lutIR[4] = {140, 125, 95, 60};
//const int lutIR[4] = {135, 125, 110, 95};
const int lutIR[4] = {150, 125, 100, 75};	// variate il 03/04/2015

int ParamFuncSensIR_touser(int val)
{
  int i;
  for(i=0; (i<3)&&(lutIR[i]>val); i++);
  return i;
}

int ParamFuncSensIR_fromuser(int val)
{
  if((val < 0) || (val > 3)) val = 3;
  return lutIR[val];
}
#else
int ParamFuncSensIR_touser(int val)
{
  if(val < 50) return 100;
  if(val > 150) return 0;
  return 150-val;
}

int ParamFuncSensIR_fromuser(int val)
{
  if(val < 0) return 150;
  if(val > 100) return 50;
  return 150-val;
}
#endif

const int lutDurata[7] = {15, 30, 60, 90, 120, 150, 180};

int ParamFuncDurata868_touser(int val)
{
  int i;
  for(i=0; (i<6)&&(lutDurata[i]<val); i++);
  return i;
}

int ParamFuncDurata868_fromuser(int val)
{
  if((val < 0) || (val > 6)) val = 6;
  return lutDurata[val];
}

int ParamFuncPeriodo868_touser(int val)
{
  /* Il parametro è il moltiplicatore della base tempi da 500ms dell'attuatore. */
  if(val >= 255) val = 0;
  return val/2;
}

int ParamFuncPeriodo868_fromuser(int val)
{
  /* Il parametro è il moltiplicatore della base tempi da 500ms dell'attuatore. */
  val *= 2;
  /* Il valore 0 ed il valore 255 indicano entrambi attuazione continua. */
  if(!val || (val >= 255)) val = 255;
  return val;
}

/* 0=edit, 1=combo, 2=slider, 3=check */
static const struct {
  int tipo, min, max;
  int (*touser)(int);
  int (*fromuser)(int);
} TipoParam_minmax[16] = {
{0,0,255,ParamFuncNull,ParamFuncNull},	// impulsi
{0,0,255,ParamFuncNull,ParamFuncNull},	// tempo
{1,0,6,ParamFuncNull,ParamFuncNull},	// durata (433)
{1,0,3,ParamFuncNull,ParamFuncNull},	// sensiblità (433)
{3,0,1,ParamFuncNull,ParamFuncNull},	// accecamento
{2,0,127,ParamFuncSensUrto_touser,ParamFuncSensUrto_fromuser},	// sens urto
{2,0,31,ParamFuncSensMov_touser,ParamFuncSensMov_fromuser},	// sens mov
{0,0,255,ParamFuncNull,ParamFuncNull},	// allarmi
{0,0,255,ParamFuncNull,ParamFuncNull},	// finestra
{0,0,255,ParamFuncPeriodo868_touser,ParamFuncPeriodo868_fromuser},	// periodo
{1,0,6,ParamFuncDurata868_touser,ParamFuncDurata868_fromuser},	// durata (868)
#ifdef SECURGROSS
/* Sensibilità con selezione 4 livelli */
{1,0,3,ParamFuncSensIR_touser,ParamFuncSensIR_fromuser},	// sensiblità (868)
#else
/* Sensibilità con slider */
{2,0,100,ParamFuncSensIR_touser,ParamFuncSensIR_fromuser},	// sensiblità (868)
#endif
{0,0,0,ParamFuncNull,ParamFuncNull},	// (non usato)
{0,0,0,ParamFuncNull,ParamFuncNull},	// (non usato)
{0,0,0,ParamFuncNull,ParamFuncNull},	// (non usato)
{0,0,0,ParamFuncNull,ParamFuncNull},	// (non usato)
};

static int toolweb_radio868_param_id = 0;
static unsigned char (*toolweb_radio868_param)[2];

int json_config_parse_auto_param(char *json, int userid)
{
  static void (*AutoImpostaParametro_p)(int id, int idx, int val) = NULL;
  static int (*InviaParametriRadio_p)(int all) = NULL;
  
  char *pp, *limit;
  int id, idx, val, all;
  
  if(!AutoImpostaParametro_p)
    AutoImpostaParametro_p = dlsym(NULL, "radio868_param_set");
  if(!InviaParametriRadio_p)
    InviaParametriRadio_p = dlsym(NULL, "radio868_param_send");
  
  pp = strstr(json, "\"id\"");
  if(pp)
  {
    pp += 4;
    CheckChar(pp, ':');
    sscanf(pp+1, "%d", &id);
  }
  else
    return -1;
  
  all = 0;
  pp = strstr(json, "\"all\"");
  if(pp)
  {
    pp += 5;
    CheckChar(pp, ':');
    sscanf(pp+1, "%d", &all);
  }
  
  pp = strstr(json, "\"param\"");
  if(pp)
  {
    pp += 7;
    CheckChar(pp, ':');
    pp++;
    CheckChar(pp, '[');
    json = pp+1;
    skipspace(&json);
  }
  else
    return -1;
  
  while(*json != ']')
  {
    CheckChar(json, '{');
    for(limit=json; *limit!='}'; limit++);
    pp = strstr(json, "\"idx\"");
    if(!pp || (pp > limit)) return -1;
    pp += 5;
    CheckChar(pp, ':');
    sscanf(pp+1, "%d", &idx);
    pp = strstr(json, "\"val\"");
    if(!pp || (pp > limit)) return -1;
    pp += 5;
    CheckChar(pp, ':');
    sscanf(pp+1, "%d", &val);
    
    /* Per poter riconvertire i valori dei parametri devo
       tenere traccia del tipo parametri ricevuti, e quindi
       devo anche verificare che sia tutto coerente. */
    if((AutoImpostaParametro_p) && (toolweb_radio868_param_id == id) && (idx < 8))
      AutoImpostaParametro_p(id, idx, TipoParam_minmax[toolweb_radio868_param[idx][0]].fromuser(val));
    
    /* Passa al parametro successivo */
    json = limit+1;
    skipspace(&json);
    if(*json == ',')
    {
      json++;
      skipspace(&json);
    }
  }
  
  if(InviaParametriRadio_p)
    return InviaParametriRadio_p(all);
  return 0;
}

int json_config_parse_auto(char *json, int userid)
{
  char *pp, *limit;
  int i, line, id, conc, nconc;
  int link[8], nlink, flags, mflags;
  //JSONbuffer reply;
  
  json += 6;
  CheckChar(json, ':');
  json++;
  CheckChar(json, '{');
  
  /* Verifica se è un comando di impostazione parametri */
  if(strstr(json, "\"param\""))
    return json_config_parse_auto_param(json, userid);
  
  /* Altrimenti deve essere un comando di configurazione delle associazioni */
  pp = strstr(json, "\"line\"");
  if(pp)
  {
    pp += 6;
    CheckChar(pp, ':');
    sscanf(pp+1, "%d", &line);
    
    //if(line >= 16) return -1;
    if(line >= 8) return -1;
  }
  else
    return -1;
  
  pp = strstr(json, "\"id\"");
  if(pp)
  {
    pp += 4;
    CheckChar(pp, ':');
    sscanf(pp+1, "%d", &id);
  }
  else
    return -1;
  
  pp = strstr(json, "\"info\"");
  if(pp)
  {
    pp += 6;
    CheckChar(pp, ':');
    pp++;
    CheckChar(pp, '{');
    pp++;
    limit = pp;
    while(*limit != '}') limit++;
    pp = strstr(pp, "\"conc\"");
    if(pp && (pp < limit))
    {
      pp += 6;
      CheckChar(pp, ':');
      sscanf(pp+1, "%d", &conc);
      
      if((conc != SC8R_1_IND) && (conc != SC8R_2_IND) &&
         (conc != (0x10|SC8R_1_IND)) && (conc != (0x10|SC8R_2_IND))) return -1;
    }
    else
      return -1;
  }
  else
    return -1;
  
  nlink = 0;
  pp = strstr(json, "\"link\"");
  if(pp)
  {
    pp += 6;
    CheckChar(pp, ':');
    pp++;
    CheckChar(pp, '[');
    pp++;
    while(*pp != ']')
    {
      if(nlink < 8)
      {
        sscanf(pp, "%d", &(link[nlink]));
        /* Verifico che il sensore indicato sia coerente con la linea */
        //if(((link[nlink] >= (line*256)) && (link[nlink] < (line*256+128))) || (link[nlink] == -1))
        /* Non posso fare la verifica di coerenza, funziona per i sensori ma attuatori e sirene
           utilizzano degli indici logici sempre uguali (0-15 o 0-3) per tutti i concentratori. */
        {
          /* La gestione interna alla centrale considera l'indirizzo
             relativo del sensore. */
          if(link[nlink] != -1) link[nlink] &= 0x7f;
          nlink++;
        }
      }
      while((*pp!=']')&&(*pp!=',')) pp++;
      if(*pp == ',') pp++;
    }
  }
  
  flags = mflags = 0;
  pp = strstr(json, "\"flags\"");
  if(pp)
  {
    pp += 7;
    CheckChar(pp, ':');
    pp++;
    CheckChar(pp, '{');
    pp++;
    limit = pp;
    while(*limit != '}') limit++;
    pp = strstr(pp, "\"failmask\"");
    if(pp && (pp < limit))
    {
      pp += 10;
      CheckChar(pp, ':');
      sscanf(pp+1, "%d", &i);
      if(!i) flags |= (1<<0);
      mflags |= (1<<0);
    }
  }
  
  /* Trasformo l'indirizzo concentratore in indice 0-3. */
  nconc = (line*4)+(conc&0x10)/8+(((conc&0xf)==SC8R_2_IND)?1:0);
  
  /* Ho tutti i dati per l'associazione della periferica. */
  /* Se sto associando una periferica devo prima vedere se la stessa
     periferica sia associata su un altro concentratore e procedere
     quindi alla cancellazione, dopodiché inserisco i nuovi collegamenti. */
  /* Se sto disassociando una periferica verifico che non sia già
     disassociata, e nel caso non devo fare nulla. */
  if(nlink)
  {
    static int (*AutoAssociaPeriferica_p)(int id, int conc, int nlink, int link[]) = NULL;
    if(!AutoAssociaPeriferica_p)
      AutoAssociaPeriferica_p = dlsym(NULL, "AutoAssociaPeriferica");
    if(AutoAssociaPeriferica_p)
      AutoAssociaPeriferica_p(id, nconc, nlink, link);
  }
  if(mflags)
  {
    static void (*AutoFlagsPeriferica_p)(int id, int conc, int mflags, int flags) = NULL;
    if(!AutoFlagsPeriferica_p)
      AutoFlagsPeriferica_p = dlsym(NULL, "AutoFlagsPeriferica");
    if(AutoFlagsPeriferica_p)
      AutoFlagsPeriferica_p(id, nconc, mflags, flags);
  }
  return 0;
}

void RichiestaListaAuto(int refresh)
{
  void (*radio_p)(int refresh) = NULL;

  toolweb_868_perif_lastid = -1;
  toolweb_868_perif_managed = 0;
  toolweb_868_perif_preload = 1;
  
  radio_p = dlsym(NULL, "radio868_list_auto");
  if(radio_p) radio_p(refresh);
}

void RichiestaListaAutoSingle(int refresh, int mm, int nconc)
{
  void (*radio_p)(int refresh, int mm, int nconc) = NULL;

  toolweb_868_perif_lastid = -1;
  toolweb_868_perif_managed = 0;
  toolweb_868_perif_preload = 1;
  
  radio_p = dlsym(NULL, "radio868_list_auto_single");
  if(radio_p) radio_p(refresh, mm, nconc);
}

void InviaListaAuto()
{
  void (*radio_p)(void) = NULL;

  radio_p = dlsym(NULL, "radio868_list_send");
  if(radio_p) radio_p();
}

int AutoRichiestaParametri(int id)
{
  int (*radio868_param_req_p)(int id);
  
  radio868_param_req_p = dlsym(NULL, "radio868_param_req");
  if(radio868_param_req_p)
    return radio868_param_req_p(id);
  
  return -1;
}

typedef struct _periph_queue {
  int line;
  unsigned int id;
  int type;
  int conc;
  int rssi;
  int batteria;
  int managed;
  int link[8];
  int failmask;
  struct _periph_queue *next;
} Auto_periph_queue;

static pthread_mutex_t auto_perif_mutex = PTHREAD_MUTEX_INITIALIZER;

#define AUTOSENS_QUEUE_LIMIT 10
static int AutoSens_queue_len;
static Auto_periph_queue *auto_perif_head, *auto_perif_cur;

int AutoSens_queue(Auto_periph_queue **head, Auto_periph_queue *perif, int managed)
{
  Auto_periph_queue *temp, *tail;
  
  if(gfd < 0) return 0;
  
  pthread_mutex_lock(&auto_perif_mutex);
  
  if(!*head)
  {
    *head = perif;
    pthread_mutex_unlock(&auto_perif_mutex);
    return 0;
  }
  else
  {
    temp = tail = *head;
    while(temp && (temp->id != perif->id))
    {
      tail = temp;
      temp = temp->next;
    }
    
    if(!temp)
    {
      /* La periferica non è ancora in lista, la aggiungo in coda. */
      /* 'tail' è sicuramente valorizzata, lo è sicuramente anche 'head'. */
      tail->next = perif;
      
      /* Se sto ancora accodando la lista managed e accodo una periferica
         non managed allora la lista managed è completa e posso iniziare ad
         inviare cosa ho in lista. */
      if(managed && !perif->managed)
      {
        pthread_mutex_unlock(&auto_perif_mutex);
        return 1;
      }
    }
    else
    {
      /* La perferica è già presente, aggiorno solo i valori. */
      tail = temp->next;
      memcpy(temp, perif, sizeof(Auto_periph_queue));
      temp->next = tail;
      free(perif);
      
      /* Se sto ancora accodando la lista managed e aggiorno dei valori,
         allora la lista managed è completa e posso inviare. */
      if(managed)
      {
        pthread_mutex_unlock(&auto_perif_mutex);
        return 1;
      }
    }
    
    /* Incremento il contatore solo una volta terminata la lista
       delle periferiche programmate. */
    if(!managed) AutoSens_queue_len++;
    pthread_mutex_unlock(&auto_perif_mutex);
    if(AutoSens_queue_len >= AUTOSENS_QUEUE_LIMIT) return 1;
    return 0;
  }
}

void AutoSens_notify(Auto_periph_queue **head, int max)
{
  JSONbuffer reply;
  Auto_periph_queue *temp;
  char text[32];
  int s, separa;
  
  pthread_mutex_lock(&auto_perif_mutex);
  if(!head || !*head)
  {
    pthread_mutex_unlock(&auto_perif_mutex);
    return;
  }
  
  json_init_buffer(&reply);
  
  json_add_string(&reply, "/json {\"user\":");
  json_add_integer(&reply, userid);
  json_add_string(&reply, ",\"configdiff\":[");
  separa = 0;
  
  /* Attenzione, l'invio viene limitato a 10 periferiche alla volta,
     non tutta la lista accodata, in modo da limitare la quantità di
     dati al secondo che viene inoltrata al toolweb. */
  while(*head && max)
  {
    if(separa) json_add_string(&reply, ",");
    separa = 1;
    
    json_add_string(&reply, "{\"auto\":{\"line\":");
    json_add_integer(&reply, (*head)->line);
    json_add_string(&reply, ",\"id\":");
    json_add_integer(&reply, (*head)->id);
    json_add_string(&reply, ",\"sid\":\"");
    /* Dell'ID prendo solo i 24 bit più bassi, ed assumo che il codice sia mappabile */
    tobase34((*head)->id&0xffffff, text, 4);
    json_add_string(&reply, text);
    json_add_string(&reply, "\",\"type\":");
    json_add_integer(&reply, (*head)->type);
    json_add_string(&reply, ",\"info\":{");
    json_add_string(&reply, "\"conc\":");
    json_add_integer(&reply, (*head)->conc);
    if((*head)->rssi < -1)
    {
      json_add_string(&reply, ",\"rssi\":");
      json_add_integer(&reply, (*head)->rssi);
    }
    if((*head)->batteria && ((*head)->batteria != 0xff))
    {
      json_add_string(&reply, ",\"batt\":");
      json_add_integer(&reply, ((*head)->batteria*5)/100);
      sprintf(text, ".%02d", ((*head)->batteria*5)%100);
      json_add_string(&reply, text);
    }
    json_add_string(&reply, ",\"managed\":");
    json_add_integer(&reply, (*head)->managed);
    json_add_string(&reply, "}");
    json_add_string(&reply, ",\"link\":[");
    for(s=0; s<NumSens[(*head)->type]; s++)
    {
      if(s) json_add_string(&reply, ",");
      if((*head)->link[s] == -1)
        json_add_integer(&reply, -1);
      else
        json_add_integer(&reply, (*head)->line*256+((*head)->conc&0x10)*8+(*head)->link[s]);
    }
    json_add_string(&reply, "],\"descr\":[");
    for(s=0; s<NumSens[(*head)->type]; s++)
    {
      if(s) json_add_string(&reply, ",");
      json_add_string(&reply, json_descr((*head)->type,
        ((*head)->line*4)+((*head)->conc&0x10)/8+((((*head)->conc&0xf)==SC8R_2_IND)?1:0),
        s, (*head)->link));
    }
    json_add_string(&reply, "]");
    if((*head)->managed)
    {
      json_add_string(&reply, ",\"flags\":{\"failmask\":");
      json_add_integer(&reply, (*head)->failmask);
      json_add_string(&reply, "}");
    }
    json_add_string(&reply, "}}");
    
    temp = (*head)->next;
    free(*head);
    *head = temp;
    
    AutoSens_queue_len--;
    if(max > 0) max--;
  }
  json_add_string(&reply, "]}");
  
  SendReply(reply.buffer);
  
  json_free_buffer(&reply);
  
  /* Per scrupolo... */
  if(!*head) AutoSens_queue_len = 0;
  
  pthread_mutex_unlock(&auto_perif_mutex);
}

void json_auto_notify(int conc, int id, int tipo, int rssi, int batt, int link[], int managed)
{
  JSONbuffer reply;
  char text[32];
  int s;
  
  json_init_buffer(&reply);
  
  json_add_string(&reply, "/json {\"user\":");
  json_add_integer(&reply, userid);
  json_add_string(&reply, ",\"configdiff\":[{\"auto\":{\"line\":");
  //json_add_integer(&reply, conc>>1);
  json_add_integer(&reply, conc>>2);
  json_add_string(&reply, ",\"id\":");
  json_add_integer(&reply, id);
  json_add_string(&reply, ",\"sid\":\"");
  /* Dell'ID prendo solo i 24 bit più bassi, ed assumo che il codice sia mappabile */
  tobase34(id&0xffffff, text, 4);
  json_add_string(&reply, text);
  json_add_string(&reply, "\",\"type\":");
  json_add_integer(&reply, tipo);
  json_add_string(&reply, ",\"info\":{");
  json_add_string(&reply, "\"conc\":");
  //json_add_integer(&reply, 2+(conc&1)*5);
  json_add_integer(&reply, (conc&2)*8+2+(conc&1)*5);
  if(rssi != 0xff)
  {
    json_add_string(&reply, ",\"rssi\":");
    json_add_integer(&reply, -rssi);
  }
  if(batt != 0xff)
  {
    json_add_string(&reply, ",\"batt\":");
    json_add_integer(&reply, (batt*5)/100);
    sprintf(text, ".%02d", (batt*5)%100);
    json_add_string(&reply, text);
  }
  json_add_string(&reply, ",\"managed\":");
  json_add_integer(&reply, managed);
  json_add_string(&reply, "},\"link\":[");
  for(s=0; s<NumSens[tipo]; s++)
  {
    if(s) json_add_string(&reply, ",");
    if(link[s] == -1)
      json_add_integer(&reply, -1);
    else
      json_add_integer(&reply, (conc>>1)*128+link[s]);
  }
  json_add_string(&reply, "],\"descr\":[");
  for(s=0; s<NumSens[tipo]; s++)
  {
    if(s) json_add_string(&reply, ",");
    json_add_string(&reply, json_descr(tipo, conc, s, link));
  }
  json_add_string(&reply, "]}}]}");
  
  SendReply(reply.buffer);
  
  json_free_buffer(&reply);
}

void json_notify_param(int id, int tipo, unsigned char param[8][2])
{
  JSONbuffer reply;
  int i;
  
  toolweb_radio868_param_id = id;
  toolweb_radio868_param = param;
  
  json_init_buffer(&reply);
  json_add_string(&reply, "/json {\"user\":");
  json_add_integer(&reply, userid);
  json_add_string(&reply, ",\"config\":[{\"auto\":{\"id\":");
  json_add_integer(&reply, id);
  json_add_string(&reply, ",\"type\":");
  json_add_integer(&reply, tipo);
  json_add_string(&reply, ",\"param\":[");
  for(i=0; (i<8)&&(param[i][0]!=0xff); i++)
  {
    if(i) json_add_string(&reply, ",");
    json_add_string(&reply, "{\"idx\":");
    json_add_integer(&reply, i);
    json_add_string(&reply, ",\"val\":");
    json_add_integer(&reply, TipoParam_minmax[param[i][0]].touser(param[i][1]));
    json_add_string(&reply, ",\"ptype\":");
    json_add_integer(&reply, TipoParam_minmax[param[i][0]].tipo);
    json_add_string(&reply, ",\"descr\":\"");
    json_add_string(&reply, Nome_param[param[i][0]]);
    if(TipoParam_minmax[param[i][0]].tipo != 1)
    {
      json_add_string(&reply, "\",\"min\":");
      json_add_integer(&reply, TipoParam_minmax[param[i][0]].min);
      json_add_string(&reply, ",\"max\":");
      json_add_integer(&reply, TipoParam_minmax[param[i][0]].max);
    }
    else
    {
      /* Per le combobox indico la lista dei valori possibili */
      json_add_string(&reply, "\",\"list\":[");
      switch(param[i][0])
      {
        case 2:	// durata 433
          json_add_string(&reply, "\"15s\",\"30s\",\"1min\",\"2min30\",\"5min\",\"7min\",\"10min\"]");
          break;
        case 3:	// sensibilità
          json_add_string(&reply, "\"0 - bassa\",\"1 - media\",\"2 - medio/alta\",\"3 - alta\"]");
          break;
        case 10:	// durata 868
          json_add_string(&reply, "\"15s\",\"30s\",\"1min\",\"1min30\",\"2min\",\"2min30\",\"3min\"]");
          break;
        case 11:	// sensibilità
          json_add_string(&reply, "\"0 - bassa\",\"1 - media\",\"2 - medio/alta\",\"3 - alta\"]");
          break;
        default:
          break;
      }
    }
    json_add_string(&reply, "}");
  }
  json_add_string(&reply, "]}}]}");
  
  SendReply(reply.buffer);
  
  json_free_buffer(&reply);
}

int json_parse_message(char *json)
{
  json = strstr(json, "/json");
  if(!json) return -1;
  
  json += 5;
  
  CheckChar(json, '{');
  json++;
  CheckChar(json, '"');
  
  /* Alcuni messaggi potrebbero arrivare con "user":0 in testa,
     il tag va quindi spostato. Oppure gestito subito. */
  /* Comunque si tratta sempre di due tag, il config e lo user,
     quindi lo user o è in testa o è in coda. */
  userid = 0;
  
  /* Se lo user è in testa */
  if(!strncmp(json, "\"user\"", 6))
  {
    json += 6;
    CheckChar(json, ':');
    json++;
    sscanf(json, "%d", &userid);
    while(*json && (*json != ',')) json++;
    json++;
    CheckChar(json, '"');
  }
  /* Altrimenti lo cerco in coda */
  else
  {
    char *p;
    
    p = json + strlen(json) - 2;
    while(*p == ' ') p--;
    if(isdigit(*p))
    {
      while(isdigit(*p)) p--;
      /* Intanto recupero lo userid potenziale, al limite lo annullo dopo */
      sscanf(p+1, "%d", &userid);
      while(*p == ' ') p--;
      if(*p == ':')
      {
        p--;
        while(*p == ' ') p--;
        /* Ora, o c'è "user" o lo "user" non c'è proprio nel json,
           se non c'è annullo lo userid precaricato. */
        if(strncmp(p-5, "\"user\"", 6)) userid = 0;
      }
    }
  }
  
  /* Comandi di configurazione */
  if(!strncmp(json, "\"config\"", 8))
  {
    json += 8;
    CheckChar(json, ':');
    json++;
    skipspace(&json);
    if(*json == '"')
    {
      if(!strncmp(json, "\"all\"", 5))
      {
        /* Richiesta configurazione */
        configurazionetoolweb();
      }
      else if(!strncmp(json, "\"auto\"", 6))
      {
        /* Gestione radio 868 */
        if(strstr(json, "\"refresh\""))
        {
          char *p;
          int mm, conc;
          
          configurazionetoolweb();
#if 0
          if(p = strstr(json, "\"sub\""))
          {
            p += 5;
            CheckChar(p, ':');
            p++;
            sscanf(p, "%d", &mm);
            if((mm >= 0) && (mm < 16))
            {
              if(p = strstr(json, "\"conc\""))
              {
                p += 5;
                CheckChar(p, ':');
                p++;
                sscanf(p, "%d", &conc);
                if((conc >= 0) && (conc < 4))
                  RichiestaListaAutoSingle(1, mm, conc);
              }
              else
                RichiestaListaAutoSingle(1, mm, -1);
            }
          }
#else
          if(p = strstr(json, "\"area\""))
          {
            p += 6;
            CheckChar(p, ':');
            p++;
            sscanf(p, "%d", &conc);
            if(conc < 0)
              RichiestaListaAuto(1);
            else
            {
              mm = conc >> 5;
              conc = conc & 0x1f;
              switch(conc)
              {
                case 2:
                  conc = 0;
                  break;
                case 7:
                  conc = 1;
                  break;
                case 18:
                  conc = 2;
                  break;
                case 23:
                  conc = 3;
                  break;
                default:
                  conc = -1;
                  break;
              }
              if((mm >= 0) && (mm < 8))
                RichiestaListaAutoSingle(1, mm, conc);
            }
          }
#endif
          else
            RichiestaListaAuto(1);
        }
        else if(strstr(json, "\"send\""))
          InviaListaAuto();
        else if(strstr(json, "\"autolink\""))
        {
          void (*radio868_auto_associa_periferiche_p)(void);
          
          /* Fermo il timer */
          timeout_off(toolweb_868_perif_notify_timeout);
          /* Blocca l'eventuale apprendimento in corso */
          RichiestaListaAuto(2);
          radio868_auto_associa_periferiche_p = dlsym(NULL, "radio868_auto_associa_periferiche");
          if(radio868_auto_associa_periferiche_p) radio868_auto_associa_periferiche_p();
          /* Restituisce la nuova configurazione */
          configurazionetoolweb();
          RichiestaListaAuto(0);
        }
        else if(strstr(json, "\"param\""))
        {
          json = strstr(json, "\"id\"");
          if(json)
          {
            int id;
            
            json += 4;
            CheckChar(json, ':');
            sscanf(json+1, "%d", &id);
            return AutoRichiestaParametri(id);
          }
          else
            return -1;
        }
        else if(strstr(json, "\"stop\""))
        {
          /* Fermo il timer */
          if(toolweb_868_perif_notify_timeout)
            timeout_off(toolweb_868_perif_notify_timeout);
          /* Blocca l'eventuale apprendimento in corso */
          RichiestaListaAuto(2);
          /* Notifica eventuali residui di lista prima di uscire. */
          AutoSens_notify(&auto_perif_head, 10);
          /* Chiude la connessione */
        }
        else
        {
          RichiestaListaAuto(0);
          configurazionetoolweb();
        }
      }
      else
        return 0;
      
      return 1;
    }
    else if(*json == '[')
    {
      /* Impostazione configurazione */
      /* ATTENZIONE: per ora assumo che le impostazioni avvengano per un elemento alla volta. */
      json++;
      CheckChar(json, '{');
      json++;
      CheckChar(json, '"');
      
      if(!strncmp(json, "\"auto\"", 6))
      {
        return json_config_parse_auto(json, userid);
      }
      else
        return -1;
    }
  }
  else if(!strncmp(json, "\"time\"", 6))
  {
    int t[6];
    char cmd[8];
    
    json += 6;
    CheckChar(json, ':');
    json++;
    skipspace(&json);
    if(*json == '"')
    {
      memset(t, 0, sizeof(t));
      sscanf(json+1, "%04d-%02d-%02dT%02d:%02d:%02d",
        &t[0], &t[1], &t[2], &t[3], &t[4], &t[5]);
      
      support_log("Variata ora da delphitool");
      sprintf(cmd, "0%02d%02d%02d", t[2], t[1], t[0]%100);
      cmd_set_date(cmd);
      sprintf(cmd, "%02d%02d%02d", t[3], t[4], t[5]);
      cmd_set_time(cmd);
    }
  }
  else if(!strncmp(json, "\"command\"", 9))
  {
    json += 9;
    CheckChar(json, ':');
    json++;
    CheckChar(json, '"');
    if(!strncmp(json, "\"reboot\"", 8))
    {
      close(gfd);
      gfd = -1;
      //exit(0);
#ifdef __arm__
      system("/sbin/reboot -f");
#else
      system("/sbin/reboot");
#endif
    }
  }
  
  return 0;
}

void toolweb_868_perif_timeout_func(void *null1, int null2)
{
  /* Invio di dati pendenti */
  if((toolweb_868_perif_lastid >= 0) && auto_perif_cur)
  {
    AutoSens_queue(&auto_perif_head, auto_perif_cur, toolweb_868_perif_preload);
    auto_perif_cur = NULL;
    toolweb_868_perif_lastid = -1;
    toolweb_868_perif_managed = 0;
  }
}

void toolweb_868_perif_notify_timeout_func(void *null1, int null2)
{
  /* Una volta al secondo invio anche una parte della lista in
     modo da mantenere un limite alla quantità di dati trasmessi
     nel tempo per non sovraccaricare il Toolweb. */
  AutoSens_notify(&auto_perif_head, 10);
  /* Reinnesco il timer */
  timeout_on(toolweb_868_perif_notify_timeout, toolweb_868_perif_notify_timeout_func, NULL, 0, 10);
}

void toolweb_868_perif(int conc, int id, int tipo, int rssi, int batt, int input, int ind, int flags)
{
  static pthread_mutex_t toolweb_mutex = PTHREAD_MUTEX_INITIALIZER;
  
  int s;
  
//printf("Rilevato %d-%08x tipo %d input %d ind %d (rssi %d batt %d)\n", conc, id, tipo, input, ind, rssi, batt);
  /* Devo accodare i rilevamenti per poi inviarli a blocchi in modo asincrono.
     Vedi: AutoSens_queue, AutoSens_notify */
  
  /* Questa funzione viene chiamata da due thread distinti. Evito conflitti. */
  pthread_mutex_lock(&toolweb_mutex);
  
  /* Ad ogni rilevamento innesco il timer per la notifica, almeno per gli elementi parziali.
     Anzi, uso due timer, uno per la gestione dei rilevamenti parziali e uno continuo e
     regolare per le notifiche. Quando si interrompono le notifiche? Allo stop. */
  if(!toolweb_868_perif_timeout) toolweb_868_perif_timeout = timeout_init();
  timeout_on(toolweb_868_perif_timeout, toolweb_868_perif_timeout_func, NULL, 0, 10);
  if(!toolweb_868_perif_notify_timeout) toolweb_868_perif_notify_timeout = timeout_init();
  if(!toolweb_868_perif_notify_timeout->active)
    timeout_on(toolweb_868_perif_notify_timeout, toolweb_868_perif_notify_timeout_func, NULL, 0, 10);
  
  if((id != toolweb_868_perif_lastid) && (toolweb_868_perif_lastid >= 0) && auto_perif_cur)
  {
    if(AutoSens_queue(&auto_perif_head, auto_perif_cur, toolweb_868_perif_preload))
    {
      toolweb_868_perif_preload = 0;
    }
    auto_perif_cur = NULL;
  }
  toolweb_868_perif_lastid = id;
  
  if(ind >= 0)
  {
    toolweb_868_perif_managed = 1;
  }
  
  /* Anziché creare messaggi json per ogni singolo rilevamento, creo una
     lista di periferiche che invio a blocchi. Il toolweb in certi casi
     fa fatica a gestire tanti messaggi, meglio messaggi con periferiche
     multiple. */
  if(!auto_perif_cur)
    auto_perif_cur = calloc(1, sizeof(Auto_periph_queue));
  if(auto_perif_cur)
  {
    //auto_perif_cur->line = conc/2;
    auto_perif_cur->line = conc/4;
    auto_perif_cur->id = id;
    auto_perif_cur->type = tipo;
    //auto_perif_cur->conc = 2+5*(conc&1);
    auto_perif_cur->conc = (conc&2)*8+2+5*(conc&1);
    if(rssi != 0xff)
      auto_perif_cur->rssi = -rssi;
    if(batt && (batt != 0xff))
      auto_perif_cur->batteria = batt;
    auto_perif_cur->managed = toolweb_868_perif_managed;
    auto_perif_cur->link[input] = ind;
#define RADIO868_FLAGS_NO_GUASTO	0x01
    auto_perif_cur->failmask = flags&RADIO868_FLAGS_NO_GUASTO?0:1;
  }
  
  if(input == (NumSens[tipo]-1))
  {
    if(AutoSens_queue(&auto_perif_head, auto_perif_cur, toolweb_868_perif_preload))
    {
      toolweb_868_perif_preload = 0;
      timeout_off(toolweb_868_perif_timeout);
    }
    auto_perif_cur = NULL;
    toolweb_868_perif_lastid = -1;
    toolweb_868_perif_managed = 0;
  }
  
  pthread_mutex_unlock(&toolweb_mutex);
}

void* toolweb_loop_json(void *fd_as_voidp)
{
  unsigned char buf[1024];
  fd_set fds;
  int ret, i;
  
  pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
  
  sprintf(buf, "ToolWeb loop json (%d)", getpid());
  support_log(buf);
  
  gfd = (int)fd_as_voidp;
  
  if(!toolweb_868_keepalive_timeout) toolweb_868_keepalive_timeout = timeout_init();
  timeout_on(toolweb_868_keepalive_timeout, toolweb_868_keepalive, NULL, 0, KEEPALIVE_TIMEOUT);
  SEprog_p = dlsym(NULL, "SEprog");

  i = 0;
  FD_ZERO(&fds);
  while(1)
  {
    if(gfd < 0) return NULL;
    
    FD_SET(gfd, &fds);
    select(gfd+1, &fds, &fds, NULL, NULL);
    ret = recv(gfd, buf+i, sizeof(buf)-1-i, 0);
    if(ret <= 0)
    {
      timeout_off(toolweb_868_keepalive_timeout);
      close(gfd);
      gfd = -1;
      return NULL;
    }
    i += ret;
    buf[i] = 0;
    
    if(json_check_message(buf))
    {
#if 0
  struct timeval tv;
  gettimeofday(&tv, NULL);
  printf("%3d.%02d R: %s\n", tv.tv_sec%1000, tv.tv_usec/10000, buf);
#endif
      
      i = 0;
      json_parse_message(buf);
    }
  }
}

void* toolweb_loop(void *null)
{
  int res, fdu, fdl, fd, fdt, i;
  struct sockaddr_in sa;
  unsigned char buf[256];
  char *env;
  fd_set fds;
  pthread_t pth;
  
  pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
  
  sprintf(buf, "ToolWeb loop (%d)", getpid());
  support_log(buf);
  
  sa.sin_family = AF_INET;
  sa.sin_port = htons(9888);
  sa.sin_addr.s_addr = INADDR_ANY;
  fdu = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
  bind(fdu, (struct sockaddr*)&sa, sizeof(struct sockaddr));
  
  fdl = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
  i = 1;
  setsockopt(fdl, SOL_SOCKET, SO_REUSEADDR, &i, sizeof(int));
  bind(fdl, (struct sockaddr*)&sa, sizeof(struct sockaddr));
  listen(fdl, 0);
  
  fd = -1;
  FD_ZERO(&fds);
  
  while(1)
  {
    FD_SET(fdu, &fds);
    FD_SET(fdl, &fds);
    i = fdl>fdu?fdl:fdu;
    select(i+1, &fds, NULL, NULL, NULL);
    
    if(FD_ISSET(fdu, &fds))
    {
      i = sizeof(sa);
      res = recvfrom(fdu, buf, 256, 0, (struct sockaddr*)&sa, &i);
/*
      printf("(%d) ", ntohs(sa.sin_port));
      for(i=0; i<res; i++) printf("%02x ", buf[i]);
      printf("\n");
*/
      if((res == 20) && (buf[0] == 1))
      {
        buf[0] = 2;
        memcpy(buf+2, &config.DeviceID, 2);
        env = getenv("HOSTNAME");
        if(env) strncpy(buf+4, env, 16);
        sendto(fdu, buf, 20, 0, (struct sockaddr*)&sa, sizeof(struct sockaddr_in));
      }
    }
    if(FD_ISSET(fdl, &fds))
    {
      fdt = accept(fdl, NULL, NULL);
      if(fdt >= 0)
      {
        if((fd >= 0) && (fd != fdt))
        {
          shutdown(fd, 2);
          close(fd);
        }
        
        if(auto_perif_cur)
        {
          free(auto_perif_cur);
          auto_perif_cur = NULL;
        }
        while(auto_perif_head)
        {
          auto_perif_cur = auto_perif_head->next;
          free(auto_perif_head);
          auto_perif_head = auto_perif_cur;
        }
        AutoSens_queue_len = 0;
        
        fd = fdt;
        pthread_create(&pth,  NULL, (PthreadFunction)toolweb_loop_json, (void*)fd);
        pthread_detach(pth);
      }
    }
  }
  
  return NULL;
}

void _init()
{
  pthread_t pth;
  
  printf("ToolWeb plugin: " __DATE__ " " __TIME__ "\n");
  
  pthread_create(&pth,  NULL, (PthreadFunction)toolweb_loop, NULL);
  pthread_detach(pth);
}

