/* Implementare la lettura multipla (func 0x14) */

#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <byteswap.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/select.h>

#include <sys/ioctl.h>

#include "database.h"
#include "command.h"
#include "support.h"
#include "serial.h"

#include <errno.h>

#define DEBUG

#define MAX_CONN	5

#define BUFLEN	512

typedef struct {
  unsigned short tid;
  unsigned short pid;
  unsigned short len;
  unsigned char ui;
} MBAP;

typedef struct {
  unsigned char func;
  unsigned char data[256];
} MBPDU;

typedef struct {
  MBAP	ap;
  MBPDU	pdu;
} ADU;

/* Table of CRC values for high–order byte */
static unsigned char auchCRCHi[] = {
0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81,
0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0,
0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01,
0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41,
0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81,
0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0,
0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01,
0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81,
0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0,
0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01,
0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81,
0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0,
0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01,
0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81,
0x40
};

/* Table of CRC values for low–order byte */
static char auchCRCLo[] = {
0x00, 0xC0, 0xC1, 0x01, 0xC3, 0x03, 0x02, 0xC2, 0xC6, 0x06, 0x07, 0xC7, 0x05, 0xC5, 0xC4,
0x04, 0xCC, 0x0C, 0x0D, 0xCD, 0x0F, 0xCF, 0xCE, 0x0E, 0x0A, 0xCA, 0xCB, 0x0B, 0xC9, 0x09,
0x08, 0xC8, 0xD8, 0x18, 0x19, 0xD9, 0x1B, 0xDB, 0xDA, 0x1A, 0x1E, 0xDE, 0xDF, 0x1F, 0xDD,
0x1D, 0x1C, 0xDC, 0x14, 0xD4, 0xD5, 0x15, 0xD7, 0x17, 0x16, 0xD6, 0xD2, 0x12, 0x13, 0xD3,
0x11, 0xD1, 0xD0, 0x10, 0xF0, 0x30, 0x31, 0xF1, 0x33, 0xF3, 0xF2, 0x32, 0x36, 0xF6, 0xF7,
0x37, 0xF5, 0x35, 0x34, 0xF4, 0x3C, 0xFC, 0xFD, 0x3D, 0xFF, 0x3F, 0x3E, 0xFE, 0xFA, 0x3A,
0x3B, 0xFB, 0x39, 0xF9, 0xF8, 0x38, 0x28, 0xE8, 0xE9, 0x29, 0xEB, 0x2B, 0x2A, 0xEA, 0xEE,
0x2E, 0x2F, 0xEF, 0x2D, 0xED, 0xEC, 0x2C, 0xE4, 0x24, 0x25, 0xE5, 0x27, 0xE7, 0xE6, 0x26,
0x22, 0xE2, 0xE3, 0x23, 0xE1, 0x21, 0x20, 0xE0, 0xA0, 0x60, 0x61, 0xA1, 0x63, 0xA3, 0xA2,
0x62, 0x66, 0xA6, 0xA7, 0x67, 0xA5, 0x65, 0x64, 0xA4, 0x6C, 0xAC, 0xAD, 0x6D, 0xAF, 0x6F,
0x6E, 0xAE, 0xAA, 0x6A, 0x6B, 0xAB, 0x69, 0xA9, 0xA8, 0x68, 0x78, 0xB8, 0xB9, 0x79, 0xBB,
0x7B, 0x7A, 0xBA, 0xBE, 0x7E, 0x7F, 0xBF, 0x7D, 0xBD, 0xBC, 0x7C, 0xB4, 0x74, 0x75, 0xB5,
0x77, 0xB7, 0xB6, 0x76, 0x72, 0xB2, 0xB3, 0x73, 0xB1, 0x71, 0x70, 0xB0, 0x50, 0x90, 0x91,
0x51, 0x93, 0x53, 0x52, 0x92, 0x96, 0x56, 0x57, 0x97, 0x55, 0x95, 0x94, 0x54, 0x9C, 0x5C,
0x5D, 0x9D, 0x5F, 0x9F, 0x9E, 0x5E, 0x5A, 0x9A, 0x9B, 0x5B, 0x99, 0x59, 0x58, 0x98, 0x88,
0x48, 0x49, 0x89, 0x4B, 0x8B, 0x8A, 0x4A, 0x4E, 0x8E, 0x8F, 0x4F, 0x8D, 0x4D, 0x4C, 0x8C,
0x44, 0x84, 0x85, 0x45, 0x87, 0x47, 0x46, 0x86, 0x82, 0x42, 0x43, 0x83, 0x41, 0x81, 0x80,
0x40
};

static unsigned short CRC16(unsigned char *puchMsg, int usDataLen)
{
  unsigned char uchCRCHi = 0xFF;
  unsigned char uchCRCLo = 0xFF;
  int uIndex;

  while(usDataLen--)
  {
    uIndex = uchCRCLo ^ *puchMsg++;
    uchCRCLo = uchCRCHi ^ auchCRCHi[uIndex];
    uchCRCHi = auchCRCLo[uIndex];
  }
  return ((uchCRCHi << 8) | uchCRCLo);
}

static void modbus_parse(int fd, int serial, ADU *req, int len)
{
  ADU resp;
  int ldata, i, n, b;
  unsigned short addr, value, tmp;
  char log[256];
  
#ifdef DEBUG
sprintf(log, "IN: Func: %02x ", req->pdu.func);
for(i=0; i<len; i++) sprintf(log+13+i*2, "%02x", ((unsigned char*)req)[i]);
support_log(log);
#endif

  switch(req->pdu.func)
  {
    case 1:	// read coils
      addr = bswap_16(*((unsigned short*)(req->pdu.data)));
      value = bswap_16(*((unsigned short*)(req->pdu.data+2)));
      
      ldata = 1;
      resp.pdu.func = 0x81;
      if(value > 2000)
        resp.pdu.data[0] = 3;	// illegal value
      else if((addr+value) > 16384)
        resp.pdu.data[0] = 2;	// illegal address
      else
      {
        b = 0;
        n = 1;
        resp.pdu.data[n] = 0;
        
        for(i=0; i<value; i++)
        {
          if(addr < 8192)
          {
            // FUORI SERVIZIO SENSORE
            if(SE[addr] & bitOOS)
              resp.pdu.data[n] |= 1<<b;
          }
          else
          {
            // FUORI SERVIZIO ATTUATORE
            if(AT[addr-8192] & bitOOS)
              resp.pdu.data[n] |= 1<<b;
          }
          addr++;
          b++;
          if(b == 8)
          {
            b = 0;
            n++;
            resp.pdu.data[n] = 0;
          }
        }
        ldata = (value+7)/8 + 1;
        resp.pdu.func = 0x01;
        resp.pdu.data[0] = (value+7)/8;
      }
      break;
    case 2:	// read discrete inputs
      addr = bswap_16(*((unsigned short*)(req->pdu.data)));
      value = bswap_16(*((unsigned short*)(req->pdu.data+2)));
      
      ldata = 1;
      resp.pdu.func = 0x82;
      if(value > 2000)
        resp.pdu.data[0] = 3;	// illegal value
      else if((addr+value) > 41216)
        resp.pdu.data[0] = 2;	// illegal address
      else
      {
        b = 0;
        n = 1;
        resp.pdu.data[n] = 0;
        
        for(i=0; i<value; i++)
        {
          if(addr < 8192)
          {
            // ALLARME SENSORE
            if(SE[addr] & bitMUAlarm)
              resp.pdu.data[n] |= 1<<b;
          }
          else if(addr < 16384)
          {
            // MANOMISSIONE SENSORE
            if(SE[addr-8192] & bitMUSabotage)
              resp.pdu.data[n] |= 1<<b;
          }
          else if(addr < 24576)
          {
            // GUASTO SENSORE
            if(SE[addr-16384] & bitMUFailure)
              resp.pdu.data[n] |= 1<<b;
          }
          else if(addr < 32768)
          {
            // ATTIVAZIONE ATTUATORE
            if(AT[addr-24576] & bitON)
              resp.pdu.data[n] |= 1<<b;
          }
          else if(addr < 33024)
          {
            // ALLARME ZONA
            if(ZONA[addr-32768] & bitAlarm)
              resp.pdu.data[n] |= 1<<b;
          }
          else
          {
            // STATO ALLARME SENSORE
            if(SE[addr-33024] & bitAlarm)
              resp.pdu.data[n] |= 1<<b;
          }
          addr++;
          b++;
          if(b == 8)
          {
            b = 0;
            n++;
            resp.pdu.data[n] = 0;
          }
        }
        ldata = (value+7)/8 + 1;
        resp.pdu.func = 0x02;
        resp.pdu.data[0] = (value+7)/8;
      }
      break;
    case 3:	// read holding registers
      addr = bswap_16(*((unsigned short*)(req->pdu.data)));
      value = bswap_16(*((unsigned short*)(req->pdu.data+2)));
      
      ldata = 1;
      resp.pdu.func = 0x83;
      if(value > 16)
        resp.pdu.data[0] = 3;	// illegal data
      else if((addr+value-1) > 15)
        resp.pdu.data[0] = 2;	// illegal address
      else
      {
        // STATO (DIS)ATTIVAZIONE ZONE
        database_lock();
        for(n=0; n<value; n++)
        {
          tmp = 0;
          for(i=0; i<16; i++)
          {
            tmp <<= 1;
            if(ZONA[(addr<<4)+i] & bitActive) tmp |= 0x0001;
          }
          ((unsigned short*)(resp.pdu.data+1))[n] = bswap_16(tmp);
          addr++;
        }
        database_unlock();
        ldata = value*2+1;
        resp.pdu.func = 0x03;
        resp.pdu.data[0] = value*2;
      }
      break;
    case 4:	// read input registers
      addr = bswap_16(*((unsigned short*)(req->pdu.data))) * 16;
      value = bswap_16(*((unsigned short*)(req->pdu.data+2))) * 16;
      
      ldata = 1;
      resp.pdu.func = 0x84;
      if(value > (125*16))
        resp.pdu.data[0] = 3;	// illegal value
      else if((addr+value) > 41216)
        resp.pdu.data[0] = 2;	// illegal address
      else
      {
        b = 7;
        n = 1;
        resp.pdu.data[n] = 0;
        
        for(i=0; i<value; i++)
        {
          if(addr < 8192)
          {
            // ALLARME SENSORE
            if(SE[addr] & bitMUAlarm)
              resp.pdu.data[n] |= 1<<b;
          }
          else if(addr < 16384)
          {
            // MANOMISSIONE SENSORE
            if(SE[addr-8192] & bitMUSabotage)
              resp.pdu.data[n] |= 1<<b;
          }
          else if(addr < 24576)
          {
            // GUASTO SENSORE
            if(SE[addr-16384] & bitMUFailure)
              resp.pdu.data[n] |= 1<<b;
          }
          else if(addr < 32768)
          {
            // ATTIVAZIONE ATTUATORE
            if(AT[addr-24576] & bitON)
              resp.pdu.data[n] |= 1<<b;
          }
          else if(addr < 33024)
          {
            // ALLARME ZONA
            if(ZONA[addr-32768] & bitAlarm)
              resp.pdu.data[n] |= 1<<b;
          }
          else
          {
            // STATO ALLARME SENSORE
            if(SE[addr-33024] & bitAlarm)
              resp.pdu.data[n] |= 1<<b;
          }
          addr++;
          b--;
          if(b == -1)
          {
            b = 7;
            n++;
            resp.pdu.data[n] = 0;
          }
        }
        ldata = n;
        resp.pdu.func = 0x04;
        resp.pdu.data[0] = n-1;
      }
      break;
    case 5:	// write single coil
      addr = bswap_16(*((unsigned short*)(req->pdu.data)));
      value = bswap_16(*((unsigned short*)(req->pdu.data+2)));
      
      ldata = 1;
      resp.pdu.func = 0x85;
      if(addr > 16383)
        resp.pdu.data[0] = 2;	// illegal address
      else if(value && (value != 0xff00))
        resp.pdu.data[0] = 3;	// illegal value
      else
      {
        if(addr < 8192)
        {
          // FUORI SERVIZIO SENSORE
          database_lock();
          if(value)
            cmd_sensor_off(addr);
          else
            cmd_sensor_on(addr);
          if(SE[addr] & bitOOS)
            value = 0xff00;
          else
            value = 0;
          database_unlock();
        }
        else
        {
          // FUORI SERVIZIO ATTUATORE
          database_lock();
          if(value)
            cmd_actuator_off(addr-8192);
          else
            cmd_actuator_on(addr-8192);
          if(AT[addr-8192] & bitOOS)
            value = 0xff00;
          else
            value = 0;
          database_unlock();
        }
        
        ldata = 4;
        resp.pdu.func = 0x05;
        *(unsigned short*)(resp.pdu.data) = *(unsigned short*)(req->pdu.data);
        *(unsigned short*)(resp.pdu.data+2) = bswap_16(value);
      }
      break;
    case 6:	// write single register
      addr = bswap_16(*((unsigned short*)(req->pdu.data)));
      value = bswap_16(*((unsigned short*)(req->pdu.data+2)));
      
      ldata = 1;
      resp.pdu.func = 0x86;
      if(addr > 256)
        resp.pdu.data[0] = 2;	// illegal address
      else if(addr == 256)
      {
        // TELECOMANDO
        if(value >= 512)
          resp.pdu.data[0] = 3;	// illegal value
        else
        {
          database_lock();
          database_set_alarm2(&ME[value]);
          database_unlock();
          ldata = 4;
          resp.pdu.func = 0x06;
          *(int*)(resp.pdu.data) = *(int*)(req->pdu.data);
        }
      }
      else if(addr == 255)
      {
        ldata = 4;
        resp.pdu.func = 0x06;
        *(unsigned short*)(resp.pdu.data) = *(unsigned short*)(req->pdu.data);
        *(unsigned short*)(resp.pdu.data+2) = 0x0000;
      }
      else
      {
        // (DIS)ATTIVAZIONE ZONE
        database_lock();
        if(value)
          cmd_zone_on(addr, 0);
        else
          cmd_zone_off(addr, 0);
        if(ZONA[addr] & bitActive)
          tmp = 0x0100;
        else
          tmp = 0x0000;
        database_unlock();
        ldata = 4;
        resp.pdu.func = 0x06;
        *(unsigned short*)(resp.pdu.data) = *(unsigned short*)(req->pdu.data);
        *(unsigned short*)(resp.pdu.data+2) = tmp;
      }
      break;
    case 0x14:	// read file record
      addr = bswap_16(*((unsigned short*)(req->pdu.data+4)));
      value = bswap_16(*((unsigned short*)(req->pdu.data+6)));
      
      ldata = 1;
      resp.pdu.func = 0x94;
      if(value > 256)
        resp.pdu.data[0] = 3;	// illegal value
      if((addr+value)>256)
        resp.pdu.data[0] = 2;	// illegal address
      else
      {
        database_lock();
        for(i=0; i<value; i++)
        {
          // STATO (DIS)ATTIVAZIONE ZONE
          if((addr < 255) && (ZONA[addr] & bitActive))
            *(unsigned short*)(resp.pdu.data+3+(i*2)) = 0x0100;
          else
            *(unsigned short*)(resp.pdu.data+3+(i*2)) = 0x0000;
          addr++;
        }
        database_unlock();
        
        ldata = 3+(i*2);
        resp.pdu.func = 0x14;
        resp.pdu.data[0] = i*2+2;
        resp.pdu.data[1] = i*2+1;
        resp.pdu.data[2] = 6;
      }
      break;
    case 0x15:	// write file record
      addr = bswap_16(*((unsigned short*)(req->pdu.data+4)));
      value = bswap_16(*((unsigned short*)(req->pdu.data+6)));
      
      ldata = 1;
      resp.pdu.func = 0x95;
      if(value > 257)
        resp.pdu.data[0] = 3;	// illegal value
      if((addr+value)>257)
        resp.pdu.data[0] = 2;	// illegal address
      else
      {
        database_lock();
        for(i=0; i<value; i++)
        {
          tmp = bswap_16(*((unsigned short*)(req->pdu.data+8+(i*2))));
           
          if(addr == 256)
          {
            // TELECOMANDO
            if(tmp < 512)
            {
              database_set_alarm2(&ME[tmp]);
              *(unsigned short*)(resp.pdu.data+8+(i*2)) = *((unsigned short*)(req->pdu.data+8+(i*2)));
            }
            else
              *(unsigned short*)(resp.pdu.data+8+(i*2)) = 0xffff;
          }
          else if(addr == 255)
          {
            *(unsigned short*)(resp.pdu.data+8+(i*2)) = 0x0000;
          }
          else
          {
            // (DIS)ATTIVAZIONE ZONE
            if(tmp)
              cmd_zone_on(addr, 0);
            else
              cmd_zone_off(addr, 0);
            if(ZONA[addr] & bitActive)
              *(unsigned short*)(resp.pdu.data+8+(i*2)) = 0x0100;
            else
              *(unsigned short*)(resp.pdu.data+8+(i*2)) = 0x0000;
          }
          addr++;
        }
        database_unlock();    
            
        ldata = 8+(i*2);
        resp.pdu.func = 0x15;
        *(unsigned short*)(resp.pdu.data) = *(unsigned short*)(req->pdu.data);
        *(unsigned short*)(resp.pdu.data+2) = *(unsigned short*)(req->pdu.data+2);
        *(unsigned short*)(resp.pdu.data+4) = *(unsigned short*)(req->pdu.data+4);
        *(unsigned short*)(resp.pdu.data+6) = *(unsigned short*)(req->pdu.data+6);
      }
      break;
    default:
      ldata = 1;
      resp.pdu.func = req->pdu.func | 0x80;
      resp.pdu.data[0] = 1;	// illegal function
      break;
  }
  
  if(serial)
  {
    int flag = TIOCM_RTS;
    
    /* Indirizzo origine */
    resp.ap.ui = req->ap.ui;
    
    *(unsigned short*)(resp.pdu.data+ldata) = CRC16(&resp.ap.ui, ldata+2);
    
#ifdef DEBUG
sprintf(log, "OUT: Func: %02x ", resp.pdu.func);
for(i=0; i<(ldata+4); i++) sprintf(log+14+i*2, "%02x", ((unsigned char*)&(resp.ap.ui))[i]);
support_log(log);
#endif
    
    ioctl(fd, TIOCMBIS, &flag);
    write(fd, &resp.ap.ui, ldata+4);
    tcdrain(fd);
    ioctl(fd, TIOCMBIC, &flag);
  }
  else
  {
    memcpy(&resp.ap, &req->ap, sizeof(MBAP));
    resp.ap.len = bswap_16(ldata + 2);
    
#ifdef DEBUG
sprintf(log, "OUT: Func: %02x ", resp.pdu.func);
for(i=0; i<(ldata+2+sizeof(MBAP)-1); i++) sprintf(log+14+i*2, "%02x", ((unsigned char*)&resp)[i]);
support_log(log);
#endif
    
    write(fd, &resp, ldata+2+sizeof(MBAP)-1);
  }
  
  return;
}

static unsigned char req[MAX_CONN+MAX_NUM_CONSUMER][1024];
static int reqidx[MAX_CONN+MAX_NUM_CONSUMER] = {0, };

static void modbus_message(int fd, int idx, unsigned char *msg, int len)
{
  memcpy(req[idx]+reqidx[idx], msg, len);
  reqidx[idx] += len;
  
  while((reqidx[idx] > 5) && (reqidx[idx] >= (bswap_16(*(unsigned short*)(req[idx]+4))+6)))
  {
    len = bswap_16(*(unsigned short*)(req[idx]+4))+6;
    modbus_parse(fd, 0, (ADU*)req[idx], len);
    reqidx[idx] -= len;
    memmove(req[idx], req[idx]+len, reqidx[idx]);
    memset(req[idx]+reqidx[idx], 0, 1024-reqidx[idx]);
  }
}

static void modbus(void *null)
{
  struct sockaddr_in sa;
  static int one = 1;
  int i, n, maxfd, num, fdl, fd[MAX_CONN+1];
  fd_set fdr;
  unsigned char buf[BUFLEN];
  
  pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
  
  fdl = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
  if(fdl < 0) return;
  
  setsockopt(fdl, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(int));
  sa.sin_family = AF_INET;
  sa.sin_port = htons(502);
  sa.sin_addr.s_addr = INADDR_ANY;
  if(bind(fdl, (struct sockaddr*)&sa, sizeof(struct sockaddr_in)))
  {
    close(fdl);
    return;
  }
  listen(fdl, 0);
  
  for(i=0; i<MAX_CONN; i++) fd[i] = -1;
  fd[MAX_CONN] = fdl;
  num = 0;
  
  while(1)
  {
    FD_ZERO(&fdr);
    if(num < MAX_CONN)
    {
      FD_SET(fdl, &fdr);
      maxfd = fdl;
    }
    else
      maxfd = 0;
    
    for(i=0; i<MAX_CONN; i++)
    {
      if(fd[i] > maxfd) maxfd = fd[i];
      if(fd[i] >= 0) FD_SET(fd[i], &fdr);
    }
    
    n = select(maxfd+1, &fdr, NULL, NULL, NULL);
    if(n > 0)
    {
      for(i=0; i<MAX_CONN; i++)
      {
        if((fd[i]>=0) && (FD_ISSET(fd[i], &fdr)))
        {
          n = recv(fd[i], buf, BUFLEN, 0);
          if(n <= 0)
          {
//printf("Chiusura socket %d (%d)\n", i, errno);
            shutdown(fd[i], 0);
            close(fd[i]);
            fd[i] = -1;
            num--;
          }
          else
          {
            modbus_message(fd[i], i, buf, n);
          }
        }
      }
      
      if(FD_ISSET(fdl, &fdr))
      {
        for(i=0; (i<MAX_CONN)&&(fd[i]>=0); i++);
        if(i == MAX_CONN) continue;	// non dovrebbe mai accadere
        fd[i] = accept(fdl, NULL, NULL);
        num++;
//printf("Apertura connessione %d (%d)\n", i, num);
      }
    }
  }
}

static void modbus_message_serial(int fd, int idx, unsigned char *msg, int len)
{
  int i, e;
  ADU adu;
  
  memcpy(req[idx]+reqidx[idx], msg, len);
  reqidx[idx] += len;
  
  while(1)
  {
    i = reqidx[idx];
    e = 2;
    
    if(i > 1)
    {
      if(req[idx][1] & 0x80)
      {
        /* Errore */
        e = 6 + 2;
      }
      else
      {
          switch(req[idx][1])
          {
            case 0x01:
            case 0x02:
            case 0x03:
            case 0x04:
            case 0x05:
            case 0x06:
              e = 6 + 2;
              break;
            case 0x0f:
            case 0x10:
              e = 7;
              if(i >= 7) e = 7 + req[idx][6] + 2;
              break;
            default:
              /* se la funzione non è nota proseguo con e=1
                 per svuotare il buffer di ricezione fino
                 al timeout */
              e = 1000;
              break;
          }
      }
      
      if(i >= e)
      {
        memset(&adu.ap, 0, sizeof(adu.ap));
        adu.ap.ui = req[idx][0];
        memcpy(&adu.pdu, req[idx]+1, e-1);
        if(config.DeviceID == adu.ap.ui) modbus_parse(fd, 1, &adu, e+6);
        reqidx[idx] -= e;
        memmove(req[idx], req[idx]+e, reqidx[idx]);
        memset(req[idx]+reqidx[idx], 0, 1024-reqidx[idx]);
      }
      else
        return;
    }
    else
      return;
  }
}

static void modbus_loop(ProtDevice *dev)
{
  unsigned char buf[BUFLEN];
  struct timeval to;
  fd_set fdr;
  int n;
  
  pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
  
  sprintf(buf, "Consumatore ModBus [%d]", getpid());
  support_log(buf);
  
  /* Elimino il controllo di flusso */
  ser_setspeed(dev->fd, config.consumer[dev->consumer].data.serial.baud1, 0);
  
  while(1)
  {
    FD_ZERO(&fdr);
    FD_SET(dev->fd, &fdr);
    
    to.tv_sec = 0;
    to.tv_usec = 100000;
    n = select(dev->fd+1, &fdr, NULL, NULL, &to);
    if(n > 0)
    {
      n = read(dev->fd, buf, BUFLEN);
      modbus_message_serial(dev->fd, dev->consumer+MAX_CONN, buf, n);
    }
    else
      reqidx[dev->consumer+MAX_CONN] = 0;
  }
}

void _init()
{
  pthread_t pth;
  
  printf("ModBus (plugin): " __DATE__ " " __TIME__ "\n");
  
  prot_plugin_register("MODBUS", 0, NULL, NULL, (PthreadFunction)modbus_loop);
  
  pthread_create(&pth,  NULL, (PthreadFunction)modbus, NULL);
  pthread_detach(pth);
}

