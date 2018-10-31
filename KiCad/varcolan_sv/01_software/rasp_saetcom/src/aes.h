/**----------------------------------------------------------------------------
 * PROJECT: rasp_saetcom
 * PURPOSE:
 * 
 *-----------------------------------------------------------------------------  
 * CREATION: Nov 11, 2016
 * Author: Luca Mini
 * 
 * LICENCE: please see LICENCE.TXT file
 * 
 * HISTORY (of the module):
 *-----------------------------------------------------------------------------
 * Author              | Date        | Description
 *-----------------------------------------------------------------------------
 *-----------------------------------------------------------------------------
 */

#ifndef AES_H_
#define AES_H_

#include <stdint.h>

#define AES_MIN_KEY_SIZE	16
#define AES_MAX_KEY_SIZE	32

#define AES_BLOCK_SIZE		16

struct aes_ctx
{
	int key_length;
	uint32_t E[60];
	uint32_t D[60];
};

void aes_gen_tabs(void);
int aes_set_key(void *ctx_arg, const uint8_t *in_key, unsigned int key_len);
void aes_encrypt(void *ctx_arg, uint8_t *out, const uint8_t *in);
void aes_decrypt(void *ctx_arg, uint8_t *out, const uint8_t *in);
//-----------------------------------------------
#endif /* AES_H_ */
