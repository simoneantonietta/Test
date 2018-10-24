#include "badge.h"
#include "master.h"
#include "codec.h"
#include "lara.h"
#include <string.h>
#include <byteswap.h>

int badge_request(int periph)
{
  unsigned char cmd[4];
  unsigned char ev[20];
  int idx;
  
  if(MMPresente[periph>>5] == MM_LARA)
  {
    idx = lara_get_id_free();
    memset(ev, 0, sizeof(ev));
    ev[0] = StatoBadge;
#ifdef GEST16MM
    ev[1] = periph>>8;
    ev[2] = periph;
//    *(unsigned short*)&ev[3] = bswap_16(idx);
    inttouchar(bswap_16(idx), ev+3, 2);
#else
    ev[1] = periph;
//    *(unsigned short*)&ev[2] = bswap_16(idx);
    inttouchar(bswap_16(idx), ev+2, 2);
#endif
    codec_queue_event(ev);
  }
  else if(MMPresente[periph>>5] != 0xff)
  {
    cmd[0] = 29;
    cmd[1] = periph & 0x1f;
    master_queue(periph >> 5, cmd);
  }
  
  return 0;
}

static int badge_common(int periph, unsigned int badge, unsigned char c, unsigned char com)
{
  unsigned char cmd[8];
  unsigned char ev[24];
  int secret, group, status;
  
  memset(ev, 0, sizeof(ev));
  ev[0] = StatoBadge;
  
  if(MMPresente[periph>>5] >= 2)
  {
    if(master_periph_type(periph) == 7)
    {
      if(MMPresente[periph>>5] == MM_LARA)
      {
#ifdef GEST16MM
        ev[1] = periph>>8;
        ev[2] = periph;
//        *(unsigned short*)&ev[3] = badge;
        inttouchar(badge, ev+3, 2);
#else
        ev[1] = periph;
//        *(unsigned short*)&ev[2] = badge;
        inttouchar(badge, ev+2, 2);
#endif
        badge = bswap_16(badge);
        
        if(c & 0x01)
          lara_set_abil(badge, BADGE_ABIL);
        else if(c & 0x02)
          lara_set_abil(badge, BADGE_DISABIL);
        else if(c & 0x04)
          lara_set_abil(badge, BADGE_CANC);
        else if(c & 0x80)
        {
          for(badge=0; badge<BADGE_NUM; badge++)
          {
#ifdef GEST16MM
            if(lara_get_id_data(badge, &secret, ev+9, &group, &status) >= 0)
            {
//              *(unsigned short*)&ev[3] = bswap_16(badge);
              inttouchar(bswap_16(badge), ev+3, 2);
              ev[5] = group;
              ev[6] = status;
//              *(unsigned short*)&ev[7] = secret;
              inttouchar(secret, ev+7, 2);
#else
            if(lara_get_id_data(badge, &secret, ev+8, &group, &status) >= 0)
            {
//              *(unsigned short*)&ev[2] = bswap_16(badge);
              inttouchar(bswap_16(badge), ev+2, 2);
              ev[4] = group;
              ev[5] = status;
//              *(unsigned short*)&ev[6] = secret;
              inttouchar(secret, ev+6, 2);
#endif
              codec_queue_event(ev);
            }
          }
#ifdef GEST16MM
//          *(unsigned short*)&ev[3] = 0xffff;
          inttouchar(0xffff, ev+3, 2);
#else
//          *(unsigned short*)&ev[2] = 0xffff;
          inttouchar(0xffff, ev+2, 2);
#endif
          codec_queue_event(ev);
        }
        else
        {
#ifdef GEST16MM
          lara_get_id_data(badge, &secret, ev+9, &group, &status);
          ev[5] = group;
          ev[6] = status;
//          *(unsigned short*)&ev[7] = secret;
          inttouchar(secret, ev+7, 2);
#else
          lara_get_id_data(badge, &secret, ev+8, &group, &status);
          ev[4] = group;
          ev[5] = status;
//          *(unsigned short*)&ev[6] = secret;
          inttouchar(secret, ev+6, 2);
#endif
          codec_queue_event(ev);
        }
      }
      else if(MMPresente[periph>>5] != 0xff)
      {
        cmd[0] = com;
        cmd[1] = periph & 0x1f;
//        *(unsigned short*)&cmd[2] = badge;
        inttouchar(badge, cmd+2, 2);
        cmd[4] = c;
        master_queue(periph >> 5, cmd);
      }
    }
    else
    {
#ifdef GEST16MM
      ev[5] = 254;
#else
      ev[4] = 254;
#endif
      codec_queue_event(ev);
    }
  }
  else
  {
#ifdef GEST16MM
    ev[5] = 255;
#else
    ev[4] = 255;
#endif
    codec_queue_event(ev);
  }

  return 0;
}

int badge_manage(int periph, unsigned int badge, unsigned char c)
{  
  badge_common(periph, badge, c-'0', 30);
  
  return 0;
}

int badge_acquire(int periph, unsigned int badge)
{
  badge_common(periph, badge, 0, 32);

  return 0;
}

int badge_load(unsigned char *data)
{
  unsigned char cmd[20];

  if(MMPresente[data[0]>>5] == MM_LARA)
//    lara_load_badge(bswap_16(*(unsigned short*)&data[1]), &data[7], *(unsigned short*)&data[5], data[3], data[4]);
    lara_load_badge(bswap_16(uchartoshort(data+1)), &data[7], uchartoshort(data+5), data[3], data[4]);
  else if(MMPresente[data[0]>>5] != 0xff)
  {
    cmd[0] = 31;
    memcpy(cmd + 1, data, 17);
    cmd[1] &= 0x1f;
    master_queue(cmd[1] >> 5, cmd);
  }
  
  return 0;
}

