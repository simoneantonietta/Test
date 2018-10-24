#ifndef _SAET_AUDIO_H
#define _SAET_AUDIO_H

void audio_reset(int fd);
int audio_init();
int audio_rec_prepare(int num);
int audio_rec(int fd);
int audio_rec_start();
int audio_rec_stop(int store);
int audio_play(int fd, int num, int now);
int audio_play_base(int fd, int audiolen, unsigned char *audiobuf, int *cont, int *errori);
int audio_play_stop();
int audio_ready();

extern unsigned int audio_version;

#endif
