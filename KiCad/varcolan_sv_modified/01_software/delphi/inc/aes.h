#ifndef __AES_H__
#define __AES_H__

typedef unsigned char u8;
typedef unsigned int u32;

struct aes_ctx {
  int key_length;
  u32 E[60];
  u32 D[60];
};

unsigned short CRC16(unsigned char *data, int len);

int aes_set_key(void *ctx_arg, const u8 *in_key, unsigned int key_len);
void aes_encrypt(void *ctx_arg, u8 *out, const u8 *in);
void aes_decrypt(void *ctx_arg, u8 *out, const u8 *in);

#endif /* __AES_H__ */
