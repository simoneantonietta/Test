/****************************************/

static unsigned char s_box[256] = {
  0x00, 0x04, 0x08, 0x0c, 0x10, 0x14, 0x18, 0x1c, 0x20, 0x24, 0x28, 0x2c, 0x30, 0x34, 0x38, 0x3c,
  0x40, 0x44, 0x48, 0x4c, 0x50, 0x54, 0x58, 0x5c, 0x60, 0x64, 0x68, 0x6c, 0x70, 0x74, 0x78, 0x7c,
  0x80, 0x84, 0x88, 0x8c, 0x90, 0x94, 0x98, 0x9c, 0xa0, 0xa4, 0xa8, 0xac, 0xb0, 0xb4, 0xb8, 0xbc,
  0xc0, 0xc4, 0xc8, 0xcc, 0xd0, 0xd4, 0xd8, 0xdc, 0xe0, 0xe4, 0xe8, 0xec, 0xf0, 0xf4, 0xf8, 0xfc,
  0x01, 0x05, 0x09, 0x0d, 0x11, 0x15, 0x19, 0x1d, 0x21, 0x25, 0x29, 0x2d, 0x31, 0x35, 0x39, 0x3d,
  0x41, 0x45, 0x49, 0x4d, 0x51, 0x55, 0x59, 0x5d, 0x61, 0x65, 0x69, 0x6d, 0x71, 0x75, 0x79, 0x7d,
  0x81, 0x85, 0x89, 0x8d, 0x91, 0x95, 0x99, 0x9d, 0xa1, 0xa5, 0xa9, 0xad, 0xb1, 0xb5, 0xb9, 0xbd,
  0xc1, 0xc5, 0xc9, 0xcd, 0xd1, 0xd5, 0xd9, 0xdd, 0xe1, 0xe5, 0xe9, 0xed, 0xf1, 0xf5, 0xf9, 0xfd,
  0x02, 0x06, 0x0a, 0x0e, 0x12, 0x16, 0x1a, 0x1e, 0x22, 0x26, 0x2a, 0x2e, 0x32, 0x36, 0x3a, 0x3e,
  0x42, 0x46, 0x4a, 0x4e, 0x52, 0x56, 0x5a, 0x5e, 0x62, 0x66, 0x6a, 0x6e, 0x72, 0x76, 0x7a, 0x7e,
  0x82, 0x86, 0x8a, 0x8e, 0x92, 0x96, 0x9a, 0x9e, 0xa2, 0xa6, 0xaa, 0xae, 0xb2, 0xb6, 0xba, 0xbe,
  0xc2, 0xc6, 0xca, 0xce, 0xd2, 0xd6, 0xda, 0xde, 0xe2, 0xe6, 0xea, 0xee, 0xf2, 0xf6, 0xfa, 0xfe,
  0x03, 0x07, 0x0b, 0x0f, 0x13, 0x17, 0x1b, 0x1f, 0x23, 0x27, 0x2b, 0x2f, 0x33, 0x37, 0x3b, 0x3f,
  0x43, 0x47, 0x4b, 0x4f, 0x53, 0x57, 0x5b, 0x5f, 0x63, 0x67, 0x6b, 0x6f, 0x73, 0x77, 0x7b, 0x7f,
  0x83, 0x87, 0x8b, 0x8f, 0x93, 0x97, 0x9b, 0x9f, 0xa3, 0xa7, 0xab, 0xaf, 0xb3, 0xb7, 0xbb, 0xbf,
  0xc3, 0xc7, 0xcb, 0xcf, 0xd3, 0xd7, 0xdb, 0xdf, 0xe3, 0xe7, 0xeb, 0xef, 0xf3, 0xf7, 0xfb, 0xff,
};

static void feal_start(CEI_data *cei, int idx, int mode, unsigned char *key, int iter)
{
  int r, i, j, l;
  unsigned char a[4], b[4], d[4], t[4];
  int n, temp;
  
  cei->key[idx].r = iter*2;
  n = iter*2 + 16;
  memset(cei->key[idx].ki, 0, 32);
  
  if(mode & 0x01)
  {
    /* Cifratura */
    i = 4;
    j = 0;
    l = 0;
    cei->key[idx].kp = cei->key[idx].ki + cei->key[idx].r;
    cei->key[idx].ke = cei->key[idx].kp + 8;
  }
  else
  {
    /* Decifratura */
    i = -4;
    j = cei->key[idx].r - 4;
    l = 2;
    cei->key[idx].ke = cei->key[idx].ki + cei->key[idx].r;
    cei->key[idx].kp = cei->key[idx].ke + 8;
  }
  a[0] = key[0]; b[0] = key[4]; d[0] = 0;
  a[1] = key[1]; b[1] = key[5]; d[1] = 0;
  a[2] = key[2]; b[2] = key[6]; d[2] = 0;
  a[3] = key[3]; b[3] = key[7]; d[3] = 0;
  for(r=0, n/=2; r<n; r+=2)
  {
    t[0] = b[0] ^ d[0]; d[0] = a[0]; a[0] = b[0];
    t[1] = b[1] ^ d[1]; d[1] = a[1]; a[1] = b[1];
    t[2] = b[2] ^ d[2]; d[2] = a[2]; a[2] = b[2];
    t[3] = b[3] ^ d[3]; d[3] = a[3]; a[3] = b[3];
    b[1] = d[1] ^ d[0];
    b[2] = d[2] ^ d[3];
    temp = (b[1] + (b[2] ^ t[0]) + 1);
    b[1] = s_box[temp&0xff]; 
    temp = (b[2] + (b[1] ^ t[1]));
    b[2] = s_box[temp&0xff];
    temp =  (d[0] + (b[1] ^  t[2]));
    b[0] = s_box[temp&0xff];
    temp = (d[3] + (b[2] ^ t[3]) + 1);
    b[3] = s_box[temp&0xff];
    
    cei->key[idx].ki[j]     = b[l % 4];
    cei->key[idx].ki[j + 1] = b[(l + 1) % 4];
    cei->key[idx].ki[j + 2] = b[(l + 2) % 4];
    cei->key[idx].ki[j + 3] = b[(l + 3) % 4];
    j += i;
    if(j < 0)
    {
      i = 4;
      j = cei->key[idx].r;
      l = 0;
    }
  }
}

static void fealnx(CEI_data *cei, int e, unsigned char *s)
{
  unsigned char c;
  unsigned char t[4];
  int i, temp;

  s[0] ^= cei->key[e].kp[0]; s[4] ^= s[0] ^ cei->key[e].kp[4];
  s[1] ^= cei->key[e].kp[1]; s[5] ^= s[1] ^ cei->key[e].kp[5];
  s[2] ^= cei->key[e].kp[2]; s[6] ^= s[2] ^ cei->key[e].kp[6];
  s[3] ^= cei->key[e].kp[3]; s[7] ^= s[3] ^ cei->key[e].kp[7];


  for (i = 0; i < cei->key[e].r; i++) {
    t[1] = s[4] ^ cei->key[e].ki[i++] ^ s[5];
    t[2] = s[6] ^ cei->key[e].ki[i]   ^ s[7];
    
    temp = (t[1] + t[2] + 1);
    t[1] = s_box[temp&0xff];
    temp = (t[1] + t[2]);
    t[2] = s_box[temp&0xff]; 
    temp = (s[4] + t[1]);
    t[0] = s_box[temp&0xff];
    temp = (s[7] + t[2] + 1);
    t[3] = s_box[temp&0xff];
    c = s[4]; s[4] = s[0] ^ t[0]; s[0] = c;
    c = s[5]; s[5] = s[1] ^ t[1]; s[1] = c;
    c = s[6]; s[6] = s[2] ^ t[2]; s[2] = c;
    c = s[7]; s[7] = s[3] ^ t[3]; s[3] = c;
  } /* endfor */
  c = s[4] ^ cei->key[e].ke[0]; s[4] ^= s[0] ^ cei->key[e].ke[4]; s[0] = c;
  c = s[5] ^ cei->key[e].ke[1]; s[5] ^= s[1] ^ cei->key[e].ke[5]; s[1] = c;
  c = s[6] ^ cei->key[e].ke[2]; s[6] ^= s[2] ^ cei->key[e].ke[6]; s[2] = c;
  c = s[7] ^ cei->key[e].ke[3]; s[7] ^= s[3] ^ cei->key[e].ke[7]; s[3] = c;
}

static void feal_set_key(CEI_data *cei, int idx, unsigned char *key, int iter)
{
  feal_start(cei, idx, ENCIPHER_8, key, iter);
  feal_start(cei, idx+3, DECIPHER_8, key, iter);
}

static void mxor(unsigned char *source, unsigned char *last_block, int len)
{
  int i;
  
  for(i=0; i<len; i++)
    source[i] = source[i] ^ last_block[i];
}

static void feal_crypt(CEI_data *cei, unsigned char *data, int key, unsigned char *init, int len)
{
  unsigned char last_block[8];
  
      while (len)
      {
//        if (((mode_last_block == EQUAL_LEN8_LEN7) && (len > 8)) ||
//            ((mode_last_block == DIF_LEN8_LEN7)	&& (len >= 8))) 
        if(len >= 8)
        {
          /* blocco completo */
          mxor(data, init, 8);
          fealnx(cei, key, data);
          init = data;
          data += 8;
          len -= 8;
        } 
        else
        {
          /* blocco incompleto */
          memcpy(last_block, init, 8);
          fealnx(cei, key, last_block);
          mxor(data, last_block, len);
          break;
        }
      }
}

static void feal_decrypt(CEI_data *cei, unsigned char *data, int key, unsigned char *init, int len)
{
  unsigned char curr_block[8], last_block[8];
  
      memcpy(last_block, init, 8);
      while (len)
      {
//        if (((mode_last_block == EQUAL_LEN8_LEN7) && (len > 8))||
//            ((mode_last_block == DIF_LEN8_LEN7) && (len >= 8)))
        if(len >= 8)
        {
          /* blocco completo */
          memcpy(curr_block, data, 8);
          fealnx(cei, key+3, data);	// decrypt
          mxor(data, last_block, 8);
          memcpy(last_block, curr_block, 8);
          data += 8;
          len -= 8;
        } 
        else
        {
          /* blocco incompleto */
          fealnx(cei, key, last_block);
          mxor(data, last_block, len);
          break;
        }
      }
}

/****************************************/

static int cei_mac(CEI_data *cei, unsigned char *data, int key, unsigned char *init, int len)
{
  unsigned char mac[8], tmp[8];
  
  memcpy(mac, init, 8);
  
  while(len)
  {
    if(len < 8)
    {
      memset(tmp, 0, 8);
      memcpy(tmp, data, len);
      mxor(mac, tmp, 8);
      fealnx(cei, key, mac);
      len = 0;
    }
    else
    {
      mxor(mac, data, 8);
      fealnx(cei, key, mac);
      len -= 8;
      data += 8;
    }
  }
  
  return *(int*)mac;
}

static unsigned short cei_CRC2(unsigned char *data, int len)
{
  unsigned short crc = 0;
  int i;
  
  for(i=0; i<len; i++)
    crc += data[i];
    
  return crc;
}

static unsigned short cei_CRC16(unsigned char *data, int len)
{
  unsigned short crc = 0;
  unsigned char d;
  int i, j;

  for (i = 0; i < len; i++)
  {
    d = data[i];
    for (j = 0; j < 8; j++)
    {
      if((d ^ crc) & 0x0001)
        crc = (crc >> 1) ^ 0xa001;
      else
        crc >>= 1;
      d >>= 1;
    }
  }
  return crc;
}

static int cei_byte_stuff(unsigned char *from, unsigned char *to, int len)
{
  int len2 = 0;
  int i;
  
  for(i=0; i<len; i++)
  {
    if((from[i] == STX) || (from[i] == ETX) || (from[i] == DLE))
    {
      to[len2++] = DLE;
      to[len2++] = from[i] | 0x80;
    }
    else
      to[len2++] = from[i];
  }
  
  return len2;
}

static int cei_L2_send_packet(ProtDevice *dev, unsigned char *pkt, int len)
{
  unsigned char *buf;
  unsigned short crc;
  
  if(CEI->L2out)
  {
    free(CEI->L2out);
    CEI->L2out = NULL;
    CEI->L2out_len = 0;
  }
  
  buf = (unsigned char*)malloc(len+12);
  if(!buf) return 0;
  
  CEI->L2out = (unsigned char*)malloc((len+10) * 2);
  if(!CEI->L2out)
  {
    free(buf);
    return 0;
  }
  
  CEI->L2out[0] = STX;

  buf[1] = CEI->Qout | CEI->dir | 0x08;
  CEI->Qout ^= CEI_L2_FLAG_Q;
  CEI->Qout &= ~CEI_L2_FLAG_NAK;
  buf[2] = config.DeviceID >> 8;
  buf[3] = config.DeviceID & 0xff;
  if(len < 249)
  {
    buf[0] = len;
    memcpy(buf + 4, pkt, len);
    switch(CEI->dir & 0x03)
    {
      case CEI_CRC2: crc = cei_CRC2(buf, len + 4); break;
      case CEI_CRC16: crc = cei_CRC16(buf, len + 4); break;
      default: crc = 0; len -= 2; break;
    }
    if(buf[0] == len)
    {
      buf[len+4] = crc >> 8;
      buf[len+5] = crc & 0xff;
    }
    CEI->L2out_len = cei_byte_stuff(buf, CEI->L2out + 1, len + 6) + 1;
  }
  else
  {
    buf[0] = 0xff;
    buf[4] = len >> 8;
    buf[5] = len & 0xff;
    memcpy(buf + 6, pkt, len);
    switch(CEI->dir & 0x03)
    {
      case CEI_CRC2: crc = cei_CRC2(buf, len + 6); break;
      case CEI_CRC16: crc = cei_CRC16(buf, len + 6); break;
      default: crc = 0; len -= 2; break;
    }
    if(buf[5] == (len & 0xff))
    {
      buf[len+6] = crc >> 8;
      buf[len+7] = crc & 0xff;
    }
    CEI->L2out_len = cei_byte_stuff(buf, CEI->L2out + 1, len + 8) + 1;
  }
  
  CEI->L2out[CEI->L2out_len++] = ETX;
  free(buf);
  
#ifdef DEBUG
{
int ret;
printf("TX:");
for(ret=0; ret<CEI->L2out_len; ret++)
printf("%02x ", CEI->L2out[ret]);
printf("\n");
}
#endif
  write(dev->fd, CEI->L2out, CEI->L2out_len);
  
  return 1;
}

static int cei_L2_get_packet_SM(ProtDevice *dev)
{
  int n, idx, state;
  fd_set fds;
  struct timeval to;
  
  /* Il timeout di LIN_TO potrebbe chiudere il socket ed
     impostare il file descriptor a -1. In questo caso
     la FD_SET darebbe segmentation fault. */
  if(dev->fd < 0) return -1;
  
  idx = 0;
  state = 1;
  FD_ZERO(&fds);
  
  while(1)
  {
    if((state != 3) && (state != 5))
    {
      if(config.consumer[dev->consumer].configured == 5)
      {
        to.tv_sec = CEI_TIMEOUT/10;
        to.tv_usec = (CEI_TIMEOUT%10)*100000;
        FD_SET(dev->fd, &fds);
        n = select(dev->fd+1, &fds, NULL, NULL, &to);
        if(n > 0)
        {
          n = read(dev->fd, CEI->L2 + idx, 1);
          if(n <= 0)	// socket closed or error
            return -1;
        }
        else if(n == 0)	// timeout
          state = 5;
        else
          return -1;	// error
      }
      else
      {
        n = read(dev->fd, CEI->L2 + idx, 1);
        if(n <= 0) state = 5;	// timeout or error
      }
    }
    
    switch(state)
    {
      case 1:
        if(CEI->L2[0] == STX)
        {
          idx++;
          state = 2;
        }
        break;
      case 2:
        if(CEI->L2[idx] == STX)
          state = 5;
        else if(CEI->L2[idx] == ETX)
          state = 3;
        else if(CEI->L2[idx] == DLE)
          state = 4;
        else
          idx++;
        break;
      case 3:
        if(CEI->L2[1] != 0xff)
        {
          CEI->L4 = CEI->L2 + 5;
          CEI->L4_len = CEI->L2[1];
        }
        else
        {
          CEI->L4 = CEI->L2 + 7;
          CEI->L4_len = bswap_16(*(unsigned short*)(CEI->L2 + 5));
        }
        return 1;
        break;
      case 4:
        if((CEI->L2[idx] == (STX|0x80)) || (CEI->L2[idx] == (ETX|0x80)) || (CEI->L2[idx] == (DLE|0x80)))
        {
          CEI->L2[idx] &= 0x7f;
          idx++;
          state = 2;
        }
        else
          state = 5;
        break;
      case 5:
        CEI->L4 = NULL;
        CEI->L4_len = 0;
        return -2;
        break;
    }
  }
}

static int cei_L2_get_packet(ProtDevice *dev)
{
  int ret;

  ret = cei_L2_get_packet_SM(dev);
  
  if(ret < 0)
  {
    if(ret == -1)	// connection closed
    {
      free(CEI->L2out);
      CEI->L2out = NULL;
    }
    return ret;
  }
  
#ifdef DEBUG
printf("RX:");
for(ret=0; ret<((CEI->L4-CEI->L2)+CEI->L4_len+3); ret++)
printf("%02x ", CEI->L2[ret]);
printf("\n");
#endif
  
  if(!((CEI->L2[2] & CEI_L2_FLAG_DIR) ^ CEI->dir)) return 0;
  
  if((CEI->L2[2] & CEI_L2_FLAG_Q) != CEI->Q)
  {
    CEI->Q ^= CEI_L2_FLAG_Q;
  }
  
  if((((CEI->L2[2] & 0x03) == 0x00) &&
            (bswap_16(*((unsigned short*)(CEI->L4+CEI->L4_len))) != cei_CRC2(CEI->L2+1, CEI->L4-CEI->L2+CEI->L4_len-1))) ||
     (((CEI->L2[2] & 0x03) == 0x01) &&
            (bswap_16(*((unsigned short*)(CEI->L4+CEI->L4_len))) != cei_CRC16(CEI->L2+1, CEI->L4-CEI->L2+CEI->L4_len-1))))
  {
    CEI->Qout |= CEI_L2_FLAG_NAK;
    cei_L2_send_packet(dev, NULL, 0);
    return -2;
  }
    
  CEI->Q ^= CEI_L2_FLAG_Q;
  
  if(CEI->L2[2] & CEI_L2_FLAG_NAK)
  {
    if(CEI->L2out)
    {
#ifdef DEBUG
printf("TX:");
for(ret=0; ret<CEI->L2out_len; ret++)
printf("%02x ", CEI->L2out[ret]);
printf("\n");
#endif
      write(dev->fd, CEI->L2out, CEI->L2out_len);
    }
    return -2;
  }
  
  return 1;
}

static int cei_L3_send_packet(ProtDevice *dev, unsigned char *pkt, int len)
{
  /* Predisposizione al livello di rete */
  return cei_L2_send_packet(dev, pkt, len);
}

static int cei_L3_get_packet(ProtDevice *dev)
{
  /* Predisposizione al livello di rete */
  return cei_L2_get_packet(dev);
}

static int cei_L4_send_packet2(ProtDevice *dev, unsigned char *pkt, int len)
{
  int ret;
  unsigned char init[8], *tpkt;
  
  if(!pkt) return cei_L3_send_packet(dev, NULL, 0);
  
  pkt[0] = 0;	// Ind. O
  pkt[1] = 0;
  pkt[2] = 0;	// Ind. D
  pkt[3] = 0;
  pkt[5] = 0;
  pkt[6] = CEI->S;	// S
  pkt[7] = CEI->R;	// R
  
  tpkt = NULL;
  
  if(CEI->option)
  {
    if(CEI->skvalid > 0)
    {
      if(len > 8)
      {
        if(!(pkt[4] & CEI_L4_FLAG_TA))
        {
          tpkt = malloc(len+4);
          if(tpkt)
          {
            memcpy(tpkt, pkt, len);
            pkt = tpkt;
          
            for(ret=0; ret<8; ret+=2) init[ret] = CEI->S;
            for(ret=1; ret<8; ret+=2) init[ret] = CEI->R;
            if(CEI->option == CEI_PROT_FEAL)
              feal_crypt(CEI, pkt+8, FEAL_SK, init, len-8);
            *(int*)(pkt+len) = cei_mac(CEI, pkt+8, FEAL_SK, init, len-8);
            pkt[4] |= (CEI->option << 5);
            len += 4;
          }
          else
          {
            /* degradato */
            pkt[4] |= CEI_L4_FLAG_S;
          }
        }
//        else
//          pkt[4] |= CEI_L4_FLAG_S;
      }
    }
    else
    {
      if(!(pkt[4] & CEI_L4_FLAG_TA))
      {
        /* degradato */
        pkt[4] &= (CEI_L4_FLAG_LP|CEI_L4_FLAG_TA);
        pkt[4] |= CEI_L4_FLAG_S;
      }
    }
  }
  
  ret = cei_L3_send_packet(dev, pkt, len);
  
  free(tpkt);
  return ret;
}

static int cei_L4_send_packet(ProtDevice *dev, void *pkt, int len)
{
  unsigned char tpkt[24];
  
  if(len == -1)
  {
    if(!CEI->lastpkt) CEI->lastpkt = malloc(12);
    if(CEI->flag & CEI_SYNCHRO)
    {
      CEI->lastpkt[4] &= CEI_L4_FLAG_TA;
      CEI->lastpkt[4] |= CEI_L4_FLAG_R;
      CEI->flag &= ~CEI_SYNCHRO;
      if(!CEI->lastlen) CEI->lastlen = 8;
    }
    else
    {
      CEI->lastpkt[4] &= CEI_L4_FLAG_TA;
    }
    return cei_L4_send_packet2(dev, CEI->lastpkt, CEI->lastlen);
  }
  
  if(pkt && (len > 8))
    CEI->flag |= CEI_WAITREPLY;
  
  if(!pkt && (CEI->flag & CEI_SENDREPLY))
  {
    memset(tpkt, 0, 8);
    tpkt[4] = CEI_L4_FLAG_TA;
    pkt = tpkt;
    len = 8;
  }
  
  CEI->flag &= ~CEI_SENDREPLY;
  
  free(CEI->lastpkt);
  CEI->lastpkt = NULL;
  CEI->lastlen = 0;
  
  if(!pkt)
  {
    if(!CEI->identvalid)
    {
      if((CEI->dir & CEI_L2_FLAG_DIR) == CEI_CSC)
      {
        memset(tpkt, 0, sizeof(tpkt));
        pkt = tpkt;
        ((unsigned char*)pkt)[4] = CEI_L4_FLAG_TA;
        ((unsigned char*)pkt)[8] = 6;	// richiesta identificazione
        *(int*)(CEI->idR1) = rand();
        *(int*)(CEI->idR1+4) = rand();
        memcpy(pkt+9, CEI->idR1, 8);
        len = 17;
        CEI->flag |= CEI_WAITREPLY;
      }
      else
      {
        cei_L4_send_packet2(dev, NULL, 0);
      }
    }
    else if(CEI->option && (CEI->skvalid > 1))
    {
      if((CEI->dir & CEI_L2_FLAG_DIR) == CEI_CSC)
      {
        memset(tpkt, 0, sizeof(tpkt));
        pkt = tpkt;
        ((unsigned char*)pkt)[4] = CEI_L4_FLAG_TA;
        ((unsigned char*)pkt)[8] = 1;	// richiesta trasferimento chiavi
        len = 9;
        CEI->flag |= CEI_WAITREPLY;
      }
    }
    else
      return cei_L4_send_packet2(dev, NULL, 0);
  }
  
  if(len < 8) return 0;
  
  if(CEI->flag & CEI_SYNCHRO)
  {
    ((unsigned char*)pkt)[4] &= CEI_L4_FLAG_TA|CEI_L4_FLAG_S;
    ((unsigned char*)pkt)[4] |= CEI_L4_FLAG_R;
    CEI->flag &= ~CEI_SYNCHRO;
  }
  else
  {
    ((unsigned char*)pkt)[4] &= CEI_L4_FLAG_TA|CEI_L4_FLAG_S;
  }
  
  CEI->lastpkt = malloc(len + 4);
  memcpy(CEI->lastpkt, pkt, len);
  CEI->lastlen = len;
  
  return cei_L4_send_packet2(dev, (unsigned char*)pkt, len);
}

static int cei_L4_get_packet(ProtDevice *dev)
{
  int ret;
  unsigned char pkt[48];
  unsigned char init[8];
  
  memset(pkt, 0, 48);
  
  if((ret = cei_L3_get_packet(dev)) <= 0)
  {
    /* se stavo aspettando una risposta, al timeout reinvio il messaggio */
    if((ret == -2) && (CEI->flag & CEI_WAITREPLY)) ret = -3;
    return ret;
  }
  
  if(CEI->L4_len)
  {
    /* controllo flusso di trasporto */
    
    if(CEI->L4[4] & CEI_L4_FLAG_R)
    {
      /* forzatura contatore S */
      CEI->S = CEI->L4[7];
      /* forzatura contatore R */
      CEI->R = CEI->L4[6];
    }
    
    if(CEI->flag & CEI_WAITREPLY)	/* attesa risposta */
    {
      if(CEI->L4[7] != (unsigned char)(CEI->S+1))
      {
        CEI->outoforder++;
        if(CEI->outoforder == 3)
        {
          /* alla terza risposta fuori sequenza, reinvio il messaggio forzando i contatori */
          CEI->flag |= CEI_SYNCHRO;
          return -3;
        }
        /* se non ricevo la conferma, al quarto tentativo chiudo e riparto */
        if(CEI->outoforder == 4) return -1;
        /* aspetto la conferma con la sequenza corretta */
        return -2;
      }
      CEI->S = CEI->L4[7];
      CEI->flag &= ~CEI_WAITREPLY;
    }
    
    if(CEI->L4_len > 8)
    {
      CEI->flag |= CEI_SENDREPLY;
      if(CEI->L4[6] != CEI->R)
      {
        cei_L4_send_packet(dev, pkt, 8);
        return -2;
      }
      CEI->R++;
    }
    
    CEI->outoforder = 0;
        
    /* livello di protezione */
    if((CEI->L4[4] & CEI_L4_FLAG_LP) && !(CEI->L4[4] & CEI_L4_FLAG_S))
    {
      if((CEI->skvalid > 0) && (CEI->L4_len > 8))
      {
        for(ret=0; ret<8; ret+=2) init[ret] = CEI->L4[6];
        for(ret=1; ret<8; ret+=2) init[ret] = CEI->L4[7];
        if(*(int*)(CEI->L4+CEI->L4_len-4) != cei_mac(CEI, CEI->L4+8, FEAL_SK, init, CEI->L4_len-12))
        {
          pkt[4] |= CEI_L4_FLAG_TA;
          ((CEI_trasporto*)pkt)->funzione = 11;
          cei_L4_send_packet(dev, pkt, sizeof(CEI_trasporto));
          return -2;
        }
        
        if((CEI->L4[4] & CEI_L4_FLAG_LP) == 0x40)
          feal_decrypt(CEI, CEI->L4+8, FEAL_SK, init, CEI->L4_len-12);
        
        CEI->L4_len -= 4;	// non considera il MAC
      }
      else if(CEI->skvalid <= 0)
        return -2;
    }
    
    if((CEI->L4_len > 8) && (CEI->L4[4] & CEI_L4_FLAG_TA))
    {
      /* dati di trasporto */
      pkt[4] = CEI_L4_FLAG_TA | (CEI->L4[4] & CEI_L4_FLAG_S);
      switch(((CEI_trasporto*)(CEI->L4))->funzione)
      {
        case 1:
          srand(time(NULL));
          if(CEI->option)
          {
            *(int*)(CEI->idR1) = rand();
            *(int*)(CEI->idR1+4) = rand();
            ((CEI_trasporto*)pkt)->funzione = 2;
            memcpy(((CEI_trasporto*)pkt)->dati, CEI->idR1, 8);
            cei_L4_send_packet(dev, pkt, sizeof(CEI_trasporto)+8);
          }
          else
          {
            ((CEI_trasporto*)pkt)->funzione = 255;
            ((CEI_trasporto*)pkt)->dati[0] = 1;
            ((CEI_trasporto*)pkt)->dati[1] = 4;
            cei_L4_send_packet(dev, pkt, sizeof(CEI_trasporto)+2);
          }
          return -2;
        case 2:
          memcpy(CEI->idR1, ((CEI_trasporto*)CEI->L4)->dati, 8);
          *(int*)(CEI->idR2) = rand();
          *(int*)(CEI->idR2+4) = rand();
          *(int*)(CEI->skt) = rand();
          *(int*)(CEI->skt+4) = rand();
          feal_set_key(CEI, FEAL_SKT, CEI->skt, FEAL_ITER);
          ((CEI_trasporto*)pkt)->funzione = 3;
          memcpy(((CEI_trasporto*)pkt)->dati, CEI->idR1, 8);
          memcpy(((CEI_trasporto*)pkt)->dati+8, CEI->idR2, 8);
          memcpy(((CEI_trasporto*)pkt)->dati+16, CEI->skt, 8);
          for(ret=0; ret<8; ret+=2) init[ret] = CEI->S;
          for(ret=1; ret<8; ret+=2) init[ret] = CEI->R;
          feal_crypt(CEI, ((CEI_trasporto*)pkt)->dati, FEAL_MK, init, 24);
          cei_L4_send_packet(dev, pkt, sizeof(CEI_trasporto)+24);
          return -2;
        case 3:
          for(ret=0; ret<8; ret+=2) init[ret] = CEI->L4[6];
          for(ret=1; ret<8; ret+=2) init[ret] = CEI->L4[7];
          feal_decrypt(CEI, ((CEI_trasporto*)(CEI->L4))->dati, FEAL_MK, init, 24);
          if(memcmp(CEI->idR1, ((CEI_trasporto*)(CEI->L4))->dati, 8))
          {
            CEI->skvalid = -1;
            ((CEI_trasporto*)pkt)->funzione = 255;
            ((CEI_trasporto*)pkt)->dati[0] = 3;
            ((CEI_trasporto*)pkt)->dati[1] = 1;
            cei_L4_send_packet(dev, pkt, sizeof(CEI_trasporto)+2);
          }
          else
          {
            memcpy(CEI->idR2, ((CEI_trasporto*)(CEI->L4))->dati+8, 8);
            memcpy(CEI->skt, ((CEI_trasporto*)(CEI->L4))->dati+16, 8);
            feal_set_key(CEI, FEAL_SKT, CEI->skt, FEAL_ITER);
            memcpy(((CEI_trasporto*)pkt)->dati, CEI->idR2, 8);
            for(ret=0; ret<8; ret+=2) init[ret] = CEI->S;
            for(ret=1; ret<8; ret+=2) init[ret] = CEI->R;
            feal_crypt(CEI, ((CEI_trasporto*)pkt)->dati, FEAL_SKT, init, 8);
            ((CEI_trasporto*)pkt)->funzione = 4;
            cei_L4_send_packet(dev, pkt, sizeof(CEI_trasporto)+8);
          }
          return -2;
        case 4:
          for(ret=0; ret<8; ret+=2) init[ret] = CEI->L4[6];
          for(ret=1; ret<8; ret+=2) init[ret] = CEI->L4[7];
          feal_decrypt(CEI, ((CEI_trasporto*)(CEI->L4))->dati, FEAL_SKT, init, 8);
          if(memcmp(CEI->idR2, ((CEI_trasporto*)(CEI->L4))->dati, 8))
          {
            ((CEI_trasporto*)pkt)->funzione = 255;
            ((CEI_trasporto*)pkt)->dati[0] = 4;
            ((CEI_trasporto*)pkt)->dati[1] = 2;
            cei_L4_send_packet(dev, pkt, sizeof(CEI_trasporto)+2);
          }
          else
          {
            ((CEI_trasporto*)pkt)->funzione = 5;
            memcpy(((CEI_trasporto*)pkt)->dati, CEI->idR2, 8);
            cei_L4_send_packet(dev, pkt, sizeof(CEI_trasporto)+8);
            memcpy(CEI->sk, CEI->skt, 8);
            feal_set_key(CEI, FEAL_SK, CEI->sk, FEAL_ITER);
            CEI->skvalid = 1;
          }
          return -2;
        case 5:
          if(memcmp(CEI->idR2, ((CEI_trasporto*)(CEI->L4))->dati, 8))
          {
            CEI->skvalid = -1;
            ((CEI_trasporto*)pkt)->funzione = 255;
            ((CEI_trasporto*)pkt)->dati[0] = 5;
            ((CEI_trasporto*)pkt)->dati[1] = 3;
            cei_L4_send_packet(dev, pkt, sizeof(CEI_trasporto)+2);
          }
          else
          {
            memcpy(CEI->sk, CEI->skt, 8);
            feal_set_key(CEI, FEAL_SK, CEI->sk, FEAL_ITER);
            CEI->skvalid = 1;
            cei_L4_send_packet(dev, NULL, 0);
            support_log("CEI: impostata chiave SK");
          }
          return -2;
        case 6:
          if(!CEI->option || (CEI->L4[4] & CEI_L4_FLAG_S) || (CEI->skvalid))
          {
            memcpy(CEI->idR1, ((CEI_trasporto*)(CEI->L4))->dati, 8);
            srand(time(NULL));
            *(int*)(CEI->idR2) = rand();
            *(int*)(CEI->idR2+4) = rand();
            ((CEI_trasporto*)pkt)->funzione = 7;
            memcpy(((CEI_trasporto*)pkt)->dati, CEI->idR1, 8);
            memcpy(((CEI_trasporto*)pkt)->dati+8, CEI->idR2, 8);
            if(CEI->skvalid > 0)
            {
              for(ret=0; ret<8; ret+=2) init[ret] = CEI->S;
              for(ret=1; ret<8; ret+=2) init[ret] = CEI->R;
              feal_crypt(CEI, ((CEI_trasporto*)pkt)->dati, FEAL_SK, init, 16);
            }
            cei_L4_send_packet(dev, pkt, sizeof(CEI_trasporto)+16);
          }
          else
          {
            CEI->skvalid = 0;
            ((CEI_trasporto*)pkt)->funzione = 255;
            ((CEI_trasporto*)pkt)->dati[0] = 6;
            ((CEI_trasporto*)pkt)->dati[1] = 1;
            cei_L4_send_packet(dev, pkt, sizeof(CEI_trasporto)+2);
          }
          return -2;
        case 7:
          if(CEI->skvalid > 0)
          {
            for(ret=0; ret<8; ret+=2) init[ret] = CEI->L4[6];
            for(ret=1; ret<8; ret+=2) init[ret] = CEI->L4[7];
            feal_decrypt(CEI, ((CEI_trasporto*)(CEI->L4))->dati, FEAL_SKT, init, 16);
          }
          if(memcmp(CEI->idR1, ((CEI_trasporto*)(CEI->L4))->dati, 8))
          {
            CEI->skvalid = 0;
            ((CEI_trasporto*)pkt)->funzione = 255;
            ((CEI_trasporto*)pkt)->dati[0] = 7;
            ((CEI_trasporto*)pkt)->dati[1] = 2;
            cei_L4_send_packet(dev, pkt, sizeof(CEI_trasporto)+2);
          }
          else
          {
            memcpy(CEI->idR2, ((CEI_trasporto*)(CEI->L4))->dati+8, 8);
            ((CEI_trasporto*)pkt)->funzione = 8;
            memcpy(((CEI_trasporto*)pkt)->dati, CEI->idR2, 8);
            cei_L4_send_packet(dev, pkt, sizeof(CEI_trasporto)+8);
            CEI->identvalid = 1;
          }
          return -2;
        case 8:
          if(memcmp(((CEI_trasporto*)(CEI->L4))->dati, CEI->idR2, 8))
          {
            ((CEI_trasporto*)pkt)->funzione = 255;
            ((CEI_trasporto*)pkt)->dati[0] = 8;
            ((CEI_trasporto*)pkt)->dati[1] = 3;
            cei_L4_send_packet(dev, pkt, sizeof(CEI_trasporto)+2);
          }
          else
          {
            CEI->identvalid = 1;
            cei_L4_send_packet(dev, pkt, 8);
          }
          return -2;
        case 11:
          CEI->skvalid = 0;
          if((CEI->dir & CEI_L2_FLAG_DIR) == CEI_CA)
            ((CEI_trasporto*)pkt)->funzione = 12;
          else
            ((CEI_trasporto*)pkt)->funzione = 1;
          cei_L4_send_packet(dev, pkt, sizeof(CEI_trasporto));
          return -2;
        case 12:
          ((CEI_trasporto*)pkt)->funzione = 1;
          cei_L4_send_packet(dev, pkt, sizeof(CEI_trasporto));
          return -2;
        case 13:
          if(!CEI->option)
          {
            ((CEI_trasporto*)pkt)->funzione = 255;
            ((CEI_trasporto*)pkt)->dati[0] = 13;
            ((CEI_trasporto*)pkt)->dati[1] = 5;
            cei_L4_send_packet(dev, pkt, sizeof(CEI_trasporto)+2);
          }
          else if(!((CEI_trasporto*)pkt)->dati[0])
          {
            srand(time(NULL));
            *(int*)(CEI->idR1) = rand();
            *(int*)(CEI->idR1+4) = rand();
            ((CEI_trasporto*)pkt)->funzione = 2;
            memcpy(((CEI_trasporto*)pkt)->dati, CEI->idR1, 8);
            cei_L4_send_packet(dev, pkt, sizeof(CEI_trasporto)+8);
          }
          else
          {
            ((CEI_trasporto*)pkt)->funzione = 255;
            ((CEI_trasporto*)pkt)->dati[0] = 13;
            ((CEI_trasporto*)pkt)->dati[1] = 6;
            cei_L4_send_packet(dev, pkt, sizeof(CEI_trasporto)+2);
          }
          return -2;
        case 255:
          switch(((CEI_trasporto*)(CEI->L4))->dati[1])
          {
            case 1:
              if((((CEI_trasporto*)(CEI->L4))->dati[0] == 6) && (CEI->skvalid >= 0))
              {
                ((CEI_trasporto*)pkt)->funzione = 1;
                cei_L4_send_packet(dev, pkt, sizeof(CEI_trasporto));
              }
              else
              {
                CEI->skvalid = -1;
                cei_L4_send_packet(dev, pkt, 8);
              }
              break;
            case 2:
              // ricevuto su CA, ignorato.
              break;
            case 3:
            case 4:
            case 5:
            case 6:
              if(((CEI_trasporto*)(CEI->L4))->dati[0] == 7)
                CEI->skvalid = 0;
              else
                CEI->skvalid = -1;
              cei_L4_send_packet(dev, pkt, 8);
              break;
            default:
              break;
          }
          return -2;
        default:
          if(CEI->L4_len <= 8) return CEI->L4_len;
          return -2;
      }
    }
  }
  else if(CEI->flag & CEI_WAITREPLY)
    return -3;
  
#ifdef DEBUG_L4
if(CEI->L4_len)
{
printf("** L4:");
for(ret=0; ret<CEI->L4_len; ret++)
printf("%02x ", CEI->L4[ret]);
printf("**\n");
}
#endif
  return CEI->L4_len;
}

static int cei_configure(CEI_data *cei, int type, int prot_lvl)
{
  cei->dir = type & (CEI_L2_FLAG_DIR | 0x03);
  cei->option = prot_lvl;
  
  feal_set_key(cei, FEAL_MK, FEAL_MK_VAL, FEAL_ITER);
  
  return 1;
}
