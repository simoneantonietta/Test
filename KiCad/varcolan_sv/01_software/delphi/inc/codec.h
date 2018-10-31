#ifndef _SAET_CODEC_H
#define _SAET_CODEC_H

#include "protocol.h"
#include "event.h"

#define DIM_SECRET		6
#define DIM_EVENT_BUFFER	150000

#define INDEX(cmd) (((cmd)[4] << 8) + (cmd)[5])
#define GVAL(cmd)  ((cmd)[0]-'0')
#define GVAL2(cmd) (((cmd)[0]-'0') * 10 + ((cmd)[1]-'0'))

void codec_init();

int codec_send_null(ProtDevice *dev);
int codec_send_event(Event *ev, ProtDevice *dev);
int codec_parse_cmd(unsigned char *cmd, int len, ProtDevice *dev);
int codec_parse_cmd2(unsigned char *cmd, ProtDevice *dev);
int codec_parse_command(Command *cmd, ProtDevice *dev);
int codec_queue_event(unsigned char *ev);
int codec_try_queue_event(unsigned char *ev, int time);
int codec_get_event(Event *ev, ProtDevice *dev);
int codec_drop_events(int consumer, int dim);
int codec_consume_event(int consumer);
int codec_consume_event_needed();

void codec_sync_register(ProtDevice *dev, InitFunction sync);

#endif
