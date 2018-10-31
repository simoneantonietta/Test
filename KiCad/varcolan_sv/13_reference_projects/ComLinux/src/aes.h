#ifndef __AES_H__
#define __AES_H__

#define AES_MIN_KEY_SIZE	16
#define AES_MAX_KEY_SIZE	32

#define AES_BLOCK_SIZE		16

typedef unsigned char u8;
typedef unsigned int u32;

struct aes_ctx {
	int key_length;
	u32 E[60];
	u32 D[60];
};

void aes_gen_tabs (void);
int aes_set_key(void *ctx_arg, const u8 *in_key, unsigned int key_len);
void aes_encrypt(void *ctx_arg, u8 *out, const u8 *in);
void aes_decrypt(void *ctx_arg, u8 *out, const u8 *in);


#endif /* __AES_H__ */

