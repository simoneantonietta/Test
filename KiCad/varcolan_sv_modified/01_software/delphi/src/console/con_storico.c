#include "console.h"
#include "con_storico.h"
#include "../codec.h"
#include "../database.h"
#include "../strings.h"
#include "../support.h"
#include <unistd.h>
#include <stdio.h>
#include <string.h>

static Event console_storico_events[DIM_CONSOLIDATED];
static int console_storico_events_in = 0;

static char console_storico_icon[] = {
  'A', 'T', 'K', '1', '1', '2', '0', '0', '0', '1', '5', '0', '2',
  0xf0, 0xfc, 0xfe, 0xfe, 0xff, 0xdf, 0xdf, 0x1b, 0xff, 0xff, 0xff, 0xfe, 0xfe, 0xfc, 0xf0,
  0x07, 0x1f, 0x3f, 0x3f, 0x7f, 0x77, 0x77, 0x70, 0x77, 0x77, 0x7f, 0x3f, 0x3f, 0x1f, 0x07,
  '\r', '\0'};

#define CONSOLE_EVENT_STRING_BASE	150

static char *str_str = "$s";

static char **console_EventString[] = {
    /* SYS events */
	&str_15_50,
	&str_15_51,
	&str_15_52,
	&str_15_53,
	&str_15_54,
	&str_15_55,
	&str_15_56,
	&str_15_57,
	&str_15_58,
	&str_15_59,
	&str_15_60,
	&str_15_61,
	&str_15_62,
	&str_15_63,
	&str_15_64,
	&str_15_65,
	&str_15_66,
	&str_15_67,
	&str_15_68,
	&str_15_69,
	&str_15_70,
	&str_15_71,
	&str_15_72,
	&str_15_73,
	&str_15_74,
	&str_15_75,
	&str_15_76,
	&str_15_77,
	&str_15_78,
	&str_15_79,
	&str_15_80,
	&str_15_81,
	&str_15_82,
	&str_15_83,
	&str_15_84,
	&str_15_85,
	&str_empty,	//&str_15_87,
	&str_15_88,
	&str_15_89,
	&str_15_90,
	&str_15_91,
	&str_15_92,
	&str_15_93,
	&str_15_94,
	&str_15_95,
	&str_15_96,
	&str_15_97,
	&str_15_98,
	&str_15_99,
	&str_empty,	//&str_15_101,
	&str_15_102,
	&str_15_103,
	&str_empty,	//&str_15_105,
	&str_empty,	//&str_15_107,
	&str_15_108,
	&str_15_109,
	&str_15_110,
	&str_15_111,
	&str_15_112,
	&str_15_113,
	&str_15_114,
	&str_15_115,
	&str_15_116,
	&str_15_117,
	&str_15_118,
	&str_15_119,
	&str_15_120,
	&str_15_121,
	&str_15_122,
	&str_15_123,
	&str_15_124,
	&str_15_125,
	&str_15_126,
	&str_15_127,
	&str_15_128,
	&str_empty,	//&str_15_130,
	&str_empty,	//&str_15_132,
	&str_15_133,
	&str_15_134,
	&str_15_135
	};

static char **console_EventExString[] = {
	&str_15_136,
	&str_15_137,
	&str_str};

static char **console_EventEx2String[][MAX_NUM_EVENT_EX2] = {
    {
    /* Tebe */
        &str_15_138,
        &str_15_139,
        &str_15_140,
        &str_15_141,
        &str_15_142,
        &str_15_143,
        &str_15_144,
	&str_15_145,
	&str_15_146,
	&str_15_147,
	&str_15_148,
	&str_15_149,
	&str_15_150,
	&str_15_151,
	&str_15_152,
	&str_15_153,
	&str_15_154,
	&str_15_155,
	&str_15_156,
	&str_15_157,
	&str_15_158,
	&str_15_159,
	&str_15_160,
	&str_15_161,
	&str_15_162,
	&str_15_163,
	&str_15_164,
	&str_15_165,
	&str_15_166,
	&str_15_167,
	&str_15_168,
	&str_15_169,
	&str_15_170,
	&str_15_171,
	&str_15_172,
	&str_15_173,
	&str_15_174,
	&str_15_175,
	&str_15_176,
	&str_15_177,
	&str_15_178,
	&str_15_179,
	&str_15_180,
	&str_15_181,
	&str_15_182,
	&str_15_183,
	&str_15_184,
	&str_15_185,
	&str_15_186,
	&str_15_187,
	&str_15_188,
	&str_15_189,
	&str_15_190,
	&str_15_191,
	&str_15_192,
	&str_15_193,
	&str_15_194,
	&str_15_195,
	&str_15_196,
	&str_15_197,
	&str_15_198,
	&str_15_199,
	&str_15_200
    },
    {
    /* Delphi */
        &str_15_201,
        &str_15_202,
        &str_15_240,
        &str_15_240,
        &str_15_240,
        &str_15_240,
        &str_15_240,
        &str_15_240,
        &str_15_240,
        &str_15_240,
        &str_15_240,	// 10
        &str_15_240,
        &str_15_240,
        &str_15_240,
        &str_15_240,
        &str_15_240,
        &str_15_240,
        &str_15_240,
        &str_15_240,
        &str_15_240,
        &str_15_240,	// 20
        &str_15_240,
        &str_15_240,
        &str_15_240,
        &str_15_240,
        &str_15_240,
        &str_15_240,
        &str_15_240,
        &str_15_240,
        &str_15_240,
        &str_15_240,	// 30
        &str_15_240,
        &str_15_240,
        &str_15_240,
        &str_15_240,
        &str_15_240,
        &str_15_240,
        &str_15_240,
        &str_15_240,
        &str_15_240,
        &str_15_240,	// 40
        &str_15_240,
        &str_15_240,
        &str_15_240,
        &str_15_240,
        &str_15_240,
        &str_15_240,
        &str_15_240,
        &str_15_240,
        &str_15_240,
        &str_15_240,	// 50
        &str_15_240,
        &str_15_240,
        &str_15_240,
        &str_15_240,
        &str_15_240,
        &str_15_240,
        &str_15_240,
        &str_15_240,
        &str_15_240,
        &str_15_240,	// 60
        &str_15_240,
        &str_15_240,
        &str_15_240,
        &str_15_240,
        &str_15_240,
        &str_15_240,
        &str_15_240,
        &str_15_240,
        &str_15_240,
        &str_15_240,	// 70
        &str_15_240,
        &str_15_240,
        &str_15_240,
        &str_15_240,
        &str_15_240,
        &str_15_240,
        &str_15_240,
        &str_15_240,
        &str_15_240,
        &str_15_240,	// 80
        &str_15_240,
        &str_15_240,
        &str_15_240,
        &str_15_240,
        &str_15_240,
        &str_15_240,
        &str_15_240,
        &str_15_240,
        &str_15_240,
        &str_15_240,	// 90
        &str_15_240,
        &str_15_240,
        &str_15_240,
        &str_15_240,
        &str_15_240,
        &str_15_240,
        &str_15_240,
        &str_15_240,
        &str_15_240,
        &str_15_240,	// 100
        &str_15_241,
        &str_15_240,
    },
};

static char **console_SensorEventType[] = {&str_15_203, &str_15_204, &str_15_205};
static char **console_SensorEventState[] = {&str_15_206, &str_empty, &str_15_208, &str_15_209};

char* console_event_format(const char *msg, unsigned char *ev, char *buf)
{
  char *cod, *descr;
  int i, j, k, v;
  
  if(!msg)
  {
    buf[0] = 0;
    return buf;
  }
  
  for(i=j=k=0; msg[i]; i++)
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
            sprintf(buf+j, "%04d", v);
            j += 4;
            for(; j<((j + 15) & 0xf0); j++) buf[j] = ' ';
            string_sensor_name(v, NULL, &descr);
            sprintf(buf+j, "%s", descr);
            j += strlen(buf+j);
            break;
          case '1':
            v = ev[k++] * 256;
            v += ev[k++];
            sprintf(buf+j, "%04d", v);
            j += 4;
            for(; j<((j + 15) & 0xf0); j++) buf[j] = ' ';
            string_actuator_name(v, NULL, &descr);
            sprintf(buf+j, "%s", descr);
            j += strlen(buf+j);
            break;
          case '2':
            v = ev[k++];
            string_zone_name(v, &cod, &descr);
            sprintf(buf+j, "%s", cod);
            j += strlen(buf+j);
            for(; j<((j + 15) & 0xf0); j++) buf[j] = ' ';
            sprintf(buf+j, "%s", descr);
            j += strlen(buf+j);
            break;
          case '3':
            v = ev[k++];
            if(v != 255)
            {
              sprintf(buf+j, "%d", v);
              j += strlen(buf+j);
            }
            break;
          case '4':
            v = ev[k++] * 256;
            v += ev[k++];
            sprintf(buf+j, "%d-%02d", v>>5, v&0x1f);
            j += strlen(buf+j);
            break;
          case '5':
            v = ev[k++];
            if((v&0xf) == 0xf)
              buf[j] = '-';
            else if((v&0xf) == 0xe)
              buf[j] = '*';
            else
              buf[j] = (v&0xf) + '0';
            j++;
            v >>= 4;
            if(v == 0xf)
              buf[j] = '-';
            else if(v == 0xe)
              buf[j] = '*';
            else
              buf[j] = v + '0';
            j++;
            break;
          case '6':
            v = ev[k++] * 256;
            v += ev[k++];
            sprintf(buf+j, "%04d", v);
            j += 4;
            break;
          case '7':
            v = ev[k++] * 256;
            v += ev[k++];
            string_command_name(v, &cod, &descr);
            sprintf(buf+j, "%s", cod);
            j += strlen(buf+j);
            for(; j<((j + 15) & 0xf0); j++) buf[j] = ' ';
            sprintf(buf+j, "%s", descr);
            j += strlen(buf+j);
            break;
          case '8':
            v = ev[k++];
            if(v != 255)
              sprintf(buf+j, "%03d", v);
            else
              strcpy(buf+j, " - ");
            j += 3;
            break;
          case '9':
            v = ev[k++];
            if(v != 255)
              sprintf(buf+j, "%02d:%02d", v, ev[k]);
            else
              sprintf(buf+j, "--:--");
            k++;
            j += 5;
            break;
          case 'a':
            v = ev[k++];
            strcpy(buf+j, "+---");
            if(v & bitOOS) buf[j] = 'S';
            if(v & bitFailure) buf[j+1] = 'G';
            if(v & bitSabotage) buf[j+2] = 'M';
            if(v & bitAlarm) buf[j+3] = 'A';
            j += 4;
            break;
          case 'd':
            v = ev[k++];
            sprintf(buf+j, "%02d", v);
            j += 2;
            break;
          case 'D':
            v = ev[k++];
            sprintf(buf+j, "%03d", v);
            j += 3;
            break;
          case 'e':
            v = ev[k++];
            sprintf(buf+j, "%s", *console_SensorEventType[v-1]);
            j += strlen(buf+j);
            break;
          case 'E':
            v = ev[k++];
            sprintf(buf+j, "%s", *console_SensorEventState[v-1]);
            j += strlen(buf+j);
            break;
          case 'f':
            v = ev[k++];
            if(v)
            {
              strcpy(buf+j, str_15_210);
              j += 4;
            }
            else
            {
              strcpy(buf+j, str_15_211);
              j += 6;
            }
            break;
          case 'F':
            v = ev[k++];
            {
              static char **lara_filter[] = {&str_15_212, &str_15_213, &str_15_214, &str_15_215};
              sprintf(buf+j, str_15_216, *lara_filter[(v>>2)&0x03], *lara_filter[v&0x03]);
              j += strlen(buf+j);
            }
            break;
          case 'i':
            v = ev[k++];
            v += ev[k++] * 256;
            sprintf(buf+j, "%05d", v);
            j += 5;
            break;
          case 'n':
            v = (j + 15) & 0xf0;
            for(; j<v; j++) buf[j] = ' ';
            break;
          case 's':
            v = ev[k++];
            strncpy(buf+j, ev+k, v);
            k += v;
            j += v;
            break;
          case 'S':
            v = ev[k++];
            i++;
            switch(msg[i])
            {
              case '1':
                if(!(v&0xe7))
                {
                  sprintf(buf+j, "-");
                  j++;
                }
                else
                {
                  if(v & 0x02)
                  {
                    sprintf(buf+j, str_15_217);
                    j += 5;
                  }
                  if(!(v & 0x04))
                  {
                    sprintf(buf+j, str_15_218);
                    j += 4;
                  }
                  if(v & 0x01)
                  {
                    sprintf(buf+j, str_15_219);
                    j += 3;
                  }
                  if((v & 0xe0) == 0xe0)
                  {
                    sprintf(buf+j, str_15_220);
                    j += 4;
                  }
                }
                break;
              case '2':
                sprintf(buf+j, str_15_221, (v&0x01)?'A':'D', (v&0x02)?'D':'A');
                j += 15;
                break;
              case '3':
                sprintf(buf+j, str_15_222, (v&0x01)?'D':'A', (v&0x10)?'A':'D');
                j += 15;
                break;
              default:
                break;
            }
            break;
          case 'x':
            v = ev[k++] * 256;
            v += ev[k++];
            sprintf(buf+j, "%04x", v);
            j += 4;
            break;
          case 'X':
            v = ev[k++];
            sprintf(buf+j, "%02x", v);
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
  
  buf[j++] = '\0';
  return buf;
}

static void console_storico_show(ProtDevice *dev, int idx, Event *event_list);

static void console_storico_event(Event *ev)
{
  memcpy(&(console_storico_events[console_storico_events_in]), ev, sizeof(Event));
  console_storico_events_in++;
  if(console_storico_events_in >= DIM_CONSOLIDATED)
    console_storico_events_in = 0;
}

void console_storico_event_loop(ProtDevice *condev)
{
  //Event ev;
  int res, idx, cons, oldactive = 0;
  static int loopstartedcont = 0;
  int loopstarted;
  ProtDevice *dev;
  int keepalive = 0;
  
  loopstarted = loopstartedcont++;

  for(res=0; res<DIM_CONSOLIDATED; res++)
    console_storico_events[res].MsgType = 0;

  dev = condev;
  
  while(1)
  {
    if(CONSOLE->active != oldactive)
      oldactive = 0;
    
    while(!(res = codec_get_event(&(condev->event), dev)));
    if(res < 0)	/* no events queued */
    {
      usleep(100000);
      keepalive++;
      if(keepalive > 50)
      {
        /* Ogni 5 secondi controlla che il terminale sia ancora connesso. */
        keepalive = 0;
        if(CONSOLE->active) console_send(dev, "ATI\r");
      }
    }
    else
    {
      if(CONSOLE->active == 2)
        console_send(dev, "ATP10\r");
      else if((CONSOLE->active != oldactive) && (CONSOLE->active == 1))
        console_send(dev, console_storico_icon);
      oldactive = CONSOLE->active;
      
      do
        if(res && !loopstarted)
          console_storico_event(&(condev->event));
      while((res = codec_get_event(&(condev->event), dev)) >= 0);
      
      if(!loopstarted)
      {
        idx = console_storico_events_in - 1;
        if(idx < 0)
          idx = DIM_CONSOLIDATED - 1;

        cons = -1;
        while((cons = support_find_serial_consumer(PROT_CONSOLE, ++cons)) >= 0)
        {
          dev = config.consumer[cons].dev;
          if(CONSOLE && (CONSOLE->active == 2) && CONSOLE->storico_realtime_active)
            console_storico_show(dev, idx, console_storico_events);
        }
      }
    }
  }
}

static CALLBACK void* console_storico_rt_accept_all(ProtDevice *dev, int press, void *null)
{
  int i;
  
  for(i=0; i<DIM_CONSOLIDATED; i++)
  {
    console_storico_events[i].MsgType = 0;
    CONSOLE->storico_events_copy[i].MsgType = 0;
  }
  console_storico_events_in = 0;
  
  console_storico_show(dev, 0, console_storico_events);
  return NULL;
}

static CALLBACK void* console_storico_rt_accept_event(ProtDevice *dev, int press, void *idx_as_voidp)
{
  int i = (int)idx_as_voidp;
    
  memcpy(&(console_storico_events[i]), &(console_storico_events[i+1]), sizeof(Event) * (DIM_CONSOLIDATED-i-1));
  if(console_storico_events_in < i)
  {
    memcpy(&(console_storico_events[DIM_CONSOLIDATED-1]), &(console_storico_events[0]), sizeof(Event));
    memcpy(&(console_storico_events[0]), &(console_storico_events[1]), sizeof(Event) * (i-1));
  }
  
  console_storico_events_in--;
  if(console_storico_events_in < 0)
    console_storico_events_in = DIM_CONSOLIDATED - 1;
  console_storico_events[console_storico_events_in].MsgType = 0;

  console_storico_show(dev, 0, console_storico_events);
  return NULL;
}

static CALLBACK void* console_storico_rt_accept_cancel(ProtDevice *dev, int press, void *idx_as_voidp)
{
  console_storico_show(dev, (int)idx_as_voidp, console_storico_events);
  return NULL;
}

static CALLBACK void* console_storico_rt_accept(ProtDevice *dev, int press, void *idx_as_voidp)
{
  console_send(dev, "ATS03\r");
  
  if(press)
  {
    console_send(dev, str_15_223);
    console_send(dev, str_15_224);
  
    console_register_callback(dev, KEY_ENTER, console_storico_rt_accept_all, NULL);
  }
  else
  {
    console_send(dev, str_15_223);
    console_send(dev, str_15_226);
  
    console_register_callback(dev, KEY_ENTER, console_storico_rt_accept_event, idx_as_voidp);
  }
  
  console_register_callback(dev, KEY_CANCEL, console_storico_rt_accept_cancel, NULL);
  return NULL;
}

static CALLBACK void* console_storico_rt_down(ProtDevice *dev, int press, void *idx_as_voidp);

static CALLBACK void* console_storico_rt_up(ProtDevice *dev, int press, void *idx_as_voidp)
{
  int console_storico_idx = (int)idx_as_voidp;
  
  if(press)
  {
    console_storico_rt_down(dev, 0, (void*)console_storico_events_in-1);
  }
  else
  {
    console_storico_idx--;
    if(console_storico_idx < 0)
      console_storico_idx = DIM_CONSOLIDATED - 1;

    if((!console_storico_events[console_storico_idx].MsgType) ||
      ((console_storico_events_in > console_storico_idx) && 
      ((console_storico_events_in - console_storico_idx) == (DIM_REALTIME+1))) ||
      ((console_storico_events_in < console_storico_idx) && 
      ((console_storico_events_in + DIM_CONSOLIDATED - console_storico_idx) == (DIM_REALTIME+1))))
    {
      console_storico_idx = console_storico_events_in - 1;
      if(console_storico_idx < 0)
        console_storico_idx = DIM_CONSOLIDATED - 1;
    }
    
    console_storico_show(dev, console_storico_idx, console_storico_events);
  }
  
  return NULL;
}

static CALLBACK void* console_storico_rt_down(ProtDevice *dev, int press, void *idx_as_voidp)
{
  int i;
  int console_storico_idx = (int)idx_as_voidp;
  
  if(press)
  {
    console_storico_rt_up(dev, 0, (void*)console_storico_events_in);
  }
  else
  {    
    console_storico_idx++;
    if(console_storico_idx >= DIM_CONSOLIDATED)
      console_storico_idx = 0;
    
    if(console_storico_idx == console_storico_events_in)
    {
      console_storico_idx = console_storico_events_in;
      for(i=0; i<DIM_REALTIME; i++)
      {
        console_storico_idx--;
        if(console_storico_idx < 0)
          console_storico_idx = DIM_CONSOLIDATED - 1;
      }
      
      while(!console_storico_events[console_storico_idx].MsgType)
      {
        console_storico_idx++;
        if(console_storico_idx >= DIM_CONSOLIDATED)
          console_storico_idx = 0;
      }
    }
    
    console_storico_show(dev, console_storico_idx, console_storico_events);
  }
  
  return NULL;
}

CALLBACK void* console_storico_realtime(ProtDevice *dev, int press, void *null)
{
  int console_storico_idx;

  console_disable_menu(dev, str_15_227);
  
  console_storico_idx = console_storico_events_in - 1;
  if(console_storico_idx < 0)
    console_storico_idx = DIM_CONSOLIDATED - 1;
  
  console_storico_show(dev, console_storico_idx, console_storico_events);
  
  return NULL;
}

static CALLBACK void* console_storico_accept_all(ProtDevice *dev, int press, void *null)
{
  int i;
  
  for(i=0; i<DIM_CONSOLIDATED; i++)
  {
    console_storico_events[i].MsgType = 0;
    CONSOLE->storico_events_copy[i].MsgType = 0;
  }
  console_storico_events_in = 0;
  
  console_storico_show(dev, 0, CONSOLE->storico_events_copy);
  return NULL;
}

static CALLBACK void* console_storico_accept_event(ProtDevice *dev, int press, void *idx_as_voidp)
{
  int i;
    
  for(i=0; i<DIM_CONSOLIDATED; i++)
  {
    if(!memcmp(&(console_storico_events[i]), &(CONSOLE->storico_events_copy[(int)idx_as_voidp]), sizeof(Event)))
    {
      memcpy(&(console_storico_events[i]), &(console_storico_events[i+1]), sizeof(Event) * (DIM_CONSOLIDATED-i-1));
      if(console_storico_events_in < i)
      {
        memcpy(&(console_storico_events[DIM_CONSOLIDATED-1]), &(console_storico_events[0]), sizeof(Event));
        memcpy(&(console_storico_events[0]), &(console_storico_events[1]), sizeof(Event) * (i-1));
      }
      
      console_storico_events_in--;
      if(console_storico_events_in < 0)
        console_storico_events_in = DIM_CONSOLIDATED - 1;
      console_storico_events[console_storico_events_in].MsgType = 0;
      
      i = DIM_CONSOLIDATED;
    }
  }

  CONSOLE->storico_events_copy[(int)idx_as_voidp].MsgType = 0;

  console_storico_show(dev, 0, CONSOLE->storico_events_copy);
  return NULL;
}

static CALLBACK void* console_storico_accept_cancel(ProtDevice *dev, int press, void *idx_as_voidp)
{
  console_storico_show(dev, (int)idx_as_voidp, CONSOLE->storico_events_copy);
  return NULL;
}

static CALLBACK void* console_storico_accept(ProtDevice *dev, int press, void *idx_as_voidp)
{
  console_send(dev, "ATS03\r");
  
  if(press)
  {
    console_send(dev, str_15_223);
    console_send(dev, str_15_224);
  
    console_register_callback(dev, KEY_ENTER, console_storico_accept_all, NULL);
  }
  else
  {
    console_send(dev, str_15_223);
    console_send(dev, str_15_227);
  
    console_register_callback(dev, KEY_ENTER, console_storico_accept_event, idx_as_voidp);
  }
  
  console_register_callback(dev, KEY_CANCEL, console_storico_accept_cancel, NULL);
  return NULL;
}

static CALLBACK void* console_storico_down(ProtDevice *dev, int press, void *idx_as_voidp);

static CALLBACK void* console_storico_up(ProtDevice *dev, int press, void *idx_as_voidp)
{
  int i;
  int console_storico_idx = (int)idx_as_voidp;
  
  if(press)
  {
    console_storico_down(dev, 0, (void*)console_storico_events_in);
  }
  else
  {
    console_storico_idx--;
    if(console_storico_idx < 0)
      console_storico_idx = DIM_CONSOLIDATED - 1;

    for(i=0; (i<DIM_CONSOLIDATED) && (CONSOLE->storico_events_copy[console_storico_idx].MsgType==0); i++)
    {
      console_storico_idx--;
      if(console_storico_idx < 0)
        console_storico_idx = DIM_CONSOLIDATED - 1;
    }
    
    console_storico_show(dev, console_storico_idx, CONSOLE->storico_events_copy);
  }
  
  return NULL;
}

static CALLBACK void* console_storico_down(ProtDevice *dev, int press, void *idx_as_voidp)
{
  int i;
  int console_storico_idx = (int)idx_as_voidp;
  
  if(press)
  {
    console_storico_up(dev, 0, (void*)console_storico_events_in);
  }
  else
  {
    console_storico_idx++;
    if(console_storico_idx >= DIM_CONSOLIDATED)
      console_storico_idx = 0;

    for(i=0; (i<DIM_CONSOLIDATED) && (CONSOLE->storico_events_copy[console_storico_idx].MsgType==0); i++)
    {
      console_storico_idx++;
      if(console_storico_idx >= DIM_CONSOLIDATED)
        console_storico_idx = 0;
    }
    
    console_storico_show(dev, console_storico_idx, CONSOLE->storico_events_copy);
  }
  
  return NULL;
}

static void console_storico_show(ProtDevice *dev, int console_storico_idx, Event *event_list)
{
  char cmd[32], cmd2[256];
  int i;
  
  console_send(dev, "ATS03\r");

  if(event_list[console_storico_idx].MsgType != 0)
  {
    sprintf(cmd, str_15_232,
        event_list[console_storico_idx].Event[0] * 256 +
        event_list[console_storico_idx].Event[1]);
    console_send(dev, cmd);
    switch(console_localization)
    {
      case 1:
        sprintf(cmd, "%s%02d:%02d:%02d  %s %02d\r", str_15_242,
          event_list[console_storico_idx].Event[5],
          event_list[console_storico_idx].Event[6] & 0x3f,
          event_list[console_storico_idx].Event[7],
          *console_months[event_list[console_storico_idx].Event[3]-1],
          event_list[console_storico_idx].Event[2]);
        break;
      default:
        sprintf(cmd, "%s%02d:%02d:%02d  %02d %s\r", str_15_242,
          event_list[console_storico_idx].Event[5],
          event_list[console_storico_idx].Event[6] & 0x3f,
          event_list[console_storico_idx].Event[7],
          event_list[console_storico_idx].Event[2],
          *console_months[event_list[console_storico_idx].Event[3]-1]);
        break;
    }
    console_send(dev, cmd);
    if(event_list[console_storico_idx].Event[8] == Evento_Esteso2)
      console_event_format(*console_EventEx2String[event_list[console_storico_idx].Event[9]][event_list[console_storico_idx].Event[10]],
            event_list[console_storico_idx].Event + 11, cmd2);
    else if(event_list[console_storico_idx].Event[8] == Evento_Esteso)
      console_event_format(*console_EventExString[event_list[console_storico_idx].Event[9]],
            event_list[console_storico_idx].Event + 10, cmd2);
    else
      console_event_format(*console_EventString[event_list[console_storico_idx].Event[8] - CONSOLE_EVENT_STRING_BASE],
            event_list[console_storico_idx].Event + 9, cmd2);
    for(i=0; (i<48) && (i<strlen(cmd2)); i+=16)
    {
      sprintf(cmd, str_15_243, (i/16)+6, cmd2 + i);
      console_send(dev, cmd);
    }
  }
  else
  {
    console_send(dev, str_15_233);
  }
  
  if(event_list == console_storico_events)
  {
    CONSOLE->storico_realtime_active = 1;
    console_register_callback(dev, KEY_UP, console_storico_rt_up, (void*)console_storico_idx);
    console_register_callback(dev, KEY_DOWN, console_storico_rt_down, (void*)console_storico_idx);
    console_register_callback(dev, KEY_ENTER, console_storico_rt_accept, (void*)console_storico_idx);
  }
  else
  {
    console_register_callback(dev, KEY_UP, console_storico_up, (void*)console_storico_idx);
    console_register_callback(dev, KEY_DOWN, console_storico_down, (void*)console_storico_idx);
    console_register_callback(dev, KEY_ENTER, console_storico_accept, (void*)console_storico_idx);
  }
  console_register_callback(dev, KEY_CANCEL, console_show_menu, NULL);
}

CALLBACK void* console_storico_consolidated(ProtDevice *dev, int press, void *null)
{
  int i, console_storico_idx;

  console_disable_menu(dev, str_15_234);
  
  memcpy(CONSOLE->storico_events_copy, console_storico_events, sizeof(console_storico_events));
  console_storico_idx = console_storico_events_in + 1;
  if(console_storico_idx >= DIM_CONSOLIDATED)
    console_storico_idx = 0;

  for(i=0; (i<DIM_CONSOLIDATED) && (CONSOLE->storico_events_copy[console_storico_idx].MsgType==0); i++)
  {
    console_storico_idx++;
    if(console_storico_idx >= DIM_CONSOLIDATED)
      console_storico_idx = 0;
  }
  
  console_storico_show(dev, console_storico_idx, CONSOLE->storico_events_copy);
  
  return NULL;
}

static CALLBACK void* console_storico_delete1(ProtDevice *dev, int press, void *null)
{
  int i;
  
  for(i=0; i<DIM_CONSOLIDATED; i++)
    console_storico_events[i].MsgType = 0;
  console_storico_events_in = 0;
  
  console_send(dev, "ATS00\r");
  console_send(dev, str_15_235);
  console_send(dev, str_15_236);
  
  timeout_on(CONSOLE->timeout, (timeout_func)console_return_to_menu, dev, 0, TIMEOUT_MSG);
  
  return NULL;
}

CALLBACK void* console_storico_delete(ProtDevice *dev, int press, void *null)
{
  console_disable_menu(dev, str_15_237);
  
  console_send(dev, str_15_223);
  console_send(dev, str_15_224);
  
  console_register_callback(dev, KEY_ENTER, console_storico_delete1, NULL);
  console_register_callback(dev, KEY_CANCEL, console_show_menu, NULL);

  return NULL;
}

