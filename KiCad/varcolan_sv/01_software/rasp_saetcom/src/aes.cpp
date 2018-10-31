/*
 *-----------------------------------------------------------------------------
 * PROJECT: rasp_saetcom
 * PURPOSE: see module aes.h file
 *-----------------------------------------------------------------------------
 */

#include "aes.h"


inline uint32_t generic_rotr32(const uint32_t x, const unsigned bits)
{
const unsigned n = bits % 32;
return (x >> n) | (x << (32 - n));
}

inline uint32_t generic_rotl32(const uint32_t x, const unsigned bits)
{
const unsigned n = bits % 32;
return (x << n) | (x >> (32 - n));
}

#define rotl generic_rotl32
#define rotr generic_rotr32

/*
 * #define byte(x, nr) ((unsigned char)((x) >> (nr*8))) 
 */
inline uint8_t byte(const uint32_t x, const unsigned n)
{
return x >> (n << 3);
}

#define u32_in(x) (*(const uint32_t *)(x))
#define u32_out(to, from) (*(uint32_t *)(to) = (from))


#define E_KEY ctx->E
#define D_KEY ctx->D

uint8_t pow_tab[256];
uint8_t log_tab[256];
uint8_t sbx_tab[256];
uint8_t isb_tab[256];
uint32_t rco_tab[10];
uint32_t ft_tab[4][256];
uint32_t it_tab[4][256];

uint32_t fl_tab[4][256];
uint32_t il_tab[4][256];

inline uint8_t f_mult(uint8_t a, uint8_t b)
{
uint8_t aa = log_tab[a], cc = aa + log_tab[b];

return pow_tab[cc + (cc < aa ? 1 : 0)];
}

#define ff_mult(a,b)    (a && b ? f_mult(a, b) : 0)

#define f_rn(bo, bi, n, k)					\
    bo[n] =  ft_tab[0][byte(bi[n],0)] ^				\
             ft_tab[1][byte(bi[(n + 1) & 3],1)] ^		\
             ft_tab[2][byte(bi[(n + 2) & 3],2)] ^		\
             ft_tab[3][byte(bi[(n + 3) & 3],3)] ^ *(k + n)

#define i_rn(bo, bi, n, k)					\
    bo[n] =  it_tab[0][byte(bi[n],0)] ^				\
             it_tab[1][byte(bi[(n + 3) & 3],1)] ^		\
             it_tab[2][byte(bi[(n + 2) & 3],2)] ^		\
             it_tab[3][byte(bi[(n + 1) & 3],3)] ^ *(k + n)

#define ls_box(x)				\
    ( fl_tab[0][byte(x, 0)] ^			\
      fl_tab[1][byte(x, 1)] ^			\
      fl_tab[2][byte(x, 2)] ^			\
      fl_tab[3][byte(x, 3)] )

#define f_rl(bo, bi, n, k)					\
    bo[n] =  fl_tab[0][byte(bi[n],0)] ^				\
             fl_tab[1][byte(bi[(n + 1) & 3],1)] ^		\
             fl_tab[2][byte(bi[(n + 2) & 3],2)] ^		\
             fl_tab[3][byte(bi[(n + 3) & 3],3)] ^ *(k + n)

#define i_rl(bo, bi, n, k)					\
    bo[n] =  il_tab[0][byte(bi[n],0)] ^				\
             il_tab[1][byte(bi[(n + 3) & 3],1)] ^		\
             il_tab[2][byte(bi[(n + 2) & 3],2)] ^		\
             il_tab[3][byte(bi[(n + 1) & 3],3)] ^ *(k + n)

void aes_gen_tabs(void)
{
uint32_t i, t;
uint8_t p, q;

/* log and power tables for GF(2**8) finite field with
 0x011b as modular polynomial - the simplest prmitive
 root is 0x03, used here to generate the tables */

for(i = 0, p = 1;i < 256;++i)
	{
	pow_tab[i] = (uint8_t) p;
	log_tab[p] = (uint8_t) i;

	p ^= (p << 1) ^ (p & 0x80 ? 0x01b : 0);
	}

log_tab[1] = 0;

for(i = 0, p = 1;i < 10;++i)
	{
	rco_tab[i] = p;

	p = (p << 1) ^ (p & 0x80 ? 0x01b : 0);
	}

for(i = 0;i < 256;++i)
	{
	p = (i ? pow_tab[255 - log_tab[i]] : 0);
	q = ((p >> 7) | (p << 1)) ^ ((p >> 6) | (p << 2));
	p ^= 0x63 ^ q ^ ((q >> 6) | (q << 2));
	sbx_tab[i] = p;
	isb_tab[p] = (uint8_t) i;
	}

for(i = 0;i < 256;++i)
	{
	p = sbx_tab[i];

	t = p;
	fl_tab[0][i] = t;
	fl_tab[1][i] = rotl(t, 8);
	fl_tab[2][i] = rotl(t, 16);
	fl_tab[3][i] = rotl(t, 24);

	t = ((uint32_t) ff_mult (2, p)) | ((uint32_t) p << 8) | ((uint32_t) p << 16) | ((uint32_t) ff_mult (3, p) << 24);

	ft_tab[0][i] = t;
	ft_tab[1][i] = rotl(t, 8);
	ft_tab[2][i] = rotl(t, 16);
	ft_tab[3][i] = rotl(t, 24);

	p = isb_tab[i];

	t = p;
	il_tab[0][i] = t;
	il_tab[1][i] = rotl(t, 8);
	il_tab[2][i] = rotl(t, 16);
	il_tab[3][i] = rotl(t, 24);

	t = ((uint32_t) ff_mult (14, p)) | ((uint32_t) ff_mult (9, p) << 8) | ((uint32_t) ff_mult (13, p) << 16) | ((uint32_t) ff_mult (11, p) << 24);

	it_tab[0][i] = t;
	it_tab[1][i] = rotl(t, 8);
	it_tab[2][i] = rotl(t, 16);
	it_tab[3][i] = rotl(t, 24);
	}
}

#define star_x(x) (((x) & 0x7f7f7f7f) << 1) ^ ((((x) & 0x80808080) >> 7) * 0x1b)

#define imix_col(y,x)       \
    u   = star_x(x);        \
    v   = star_x(u);        \
    w   = star_x(v);        \
    t   = w ^ (x);          \
   (y)  = u ^ v ^ w;        \
   (y) ^= rotr(u ^ t,  8) ^ \
          rotr(v ^ t, 16) ^ \
          rotr(t,24)

/* initialise the key schedule from the user supplied key */

#define loop4(i)                                    \
{   t = rotr(t,  8); t = ls_box(t) ^ rco_tab[i];    \
    t ^= E_KEY[4 * i];     E_KEY[4 * i + 4] = t;    \
    t ^= E_KEY[4 * i + 1]; E_KEY[4 * i + 5] = t;    \
    t ^= E_KEY[4 * i + 2]; E_KEY[4 * i + 6] = t;    \
    t ^= E_KEY[4 * i + 3]; E_KEY[4 * i + 7] = t;    \
}

#define loop6(i)                                    \
{   t = rotr(t,  8); t = ls_box(t) ^ rco_tab[i];    \
    t ^= E_KEY[6 * i];     E_KEY[6 * i + 6] = t;    \
    t ^= E_KEY[6 * i + 1]; E_KEY[6 * i + 7] = t;    \
    t ^= E_KEY[6 * i + 2]; E_KEY[6 * i + 8] = t;    \
    t ^= E_KEY[6 * i + 3]; E_KEY[6 * i + 9] = t;    \
    t ^= E_KEY[6 * i + 4]; E_KEY[6 * i + 10] = t;   \
    t ^= E_KEY[6 * i + 5]; E_KEY[6 * i + 11] = t;   \
}

#define loop8(i)                                    \
{   t = rotr(t,  8); ; t = ls_box(t) ^ rco_tab[i];  \
    t ^= E_KEY[8 * i];     E_KEY[8 * i + 8] = t;    \
    t ^= E_KEY[8 * i + 1]; E_KEY[8 * i + 9] = t;    \
    t ^= E_KEY[8 * i + 2]; E_KEY[8 * i + 10] = t;   \
    t ^= E_KEY[8 * i + 3]; E_KEY[8 * i + 11] = t;   \
    t  = E_KEY[8 * i + 4] ^ ls_box(t);    \
    E_KEY[8 * i + 12] = t;                \
    t ^= E_KEY[8 * i + 5]; E_KEY[8 * i + 13] = t;   \
    t ^= E_KEY[8 * i + 6]; E_KEY[8 * i + 14] = t;   \
    t ^= E_KEY[8 * i + 7]; E_KEY[8 * i + 15] = t;   \
}

int aes_set_key(void *ctx_arg, const uint8_t *in_key, unsigned int key_len)
{
struct aes_ctx *ctx = (struct aes_ctx*)ctx_arg;
uint32_t i, t, u, v, w;

if(key_len != 16 && key_len != 24 && key_len != 32)
	{
	return -1;
	}

ctx->key_length = key_len;

E_KEY[0] = u32_in(in_key);
E_KEY[1] = u32_in(in_key + 4);
E_KEY[2] = u32_in(in_key + 8);
E_KEY[3] = u32_in(in_key + 12);

switch(key_len)
	{
	case 16:
		t = E_KEY[3];
		for(i = 0;i < 10;++i)
			loop4(i)
		;
		break;

	case 24:
		E_KEY[4] = u32_in(in_key + 16);
		t = E_KEY[5] = u32_in(in_key + 20);
		for(i = 0;i < 8;++i)
			loop6(i)
		;
		break;

	case 32:
		E_KEY[4] = u32_in(in_key + 16);
		E_KEY[5] = u32_in(in_key + 20);
		E_KEY[6] = u32_in(in_key + 24);
		t = E_KEY[7] = u32_in(in_key + 28);
		for(i = 0;i < 7;++i)
			loop8(i)
		;
		break;
	}

D_KEY[0] = E_KEY[0];
D_KEY[1] = E_KEY[1];
D_KEY[2] = E_KEY[2];
D_KEY[3] = E_KEY[3];

for(i = 4;i < key_len + 24;++i)
	{
	imix_col(D_KEY[i], E_KEY[i]);
	}

return 0;
}

/* encrypt a block of text */

#define f_nround(bo, bi, k) \
    f_rn(bo, bi, 0, k);     \
    f_rn(bo, bi, 1, k);     \
    f_rn(bo, bi, 2, k);     \
    f_rn(bo, bi, 3, k);     \
    k += 4

#define f_lround(bo, bi, k) \
    f_rl(bo, bi, 0, k);     \
    f_rl(bo, bi, 1, k);     \
    f_rl(bo, bi, 2, k);     \
    f_rl(bo, bi, 3, k)

void aes_encrypt(void *ctx_arg, uint8_t *out, const uint8_t *in)
{
const struct aes_ctx *ctx = (struct aes_ctx *)ctx_arg;
uint32_t b0[4], b1[4];
const uint32_t *kp = E_KEY + 4;

b0[0] = u32_in (in) ^ E_KEY[0];
b0[1] = u32_in (in + 4) ^ E_KEY[1];
b0[2] = u32_in (in + 8) ^ E_KEY[2];
b0[3] = u32_in (in + 12) ^ E_KEY[3];

if(ctx->key_length > 24)
	{
	f_nround(b1, b0, kp);
	f_nround(b0, b1, kp);
	}

if(ctx->key_length > 16)
	{
	f_nround(b1, b0, kp);
	f_nround(b0, b1, kp);
	}

f_nround(b1, b0, kp);
f_nround(b0, b1, kp);
f_nround(b1, b0, kp);
f_nround(b0, b1, kp);
f_nround(b1, b0, kp);
f_nround(b0, b1, kp);
f_nround(b1, b0, kp);
f_nround(b0, b1, kp);
f_nround(b1, b0, kp);
f_lround(b0, b1, kp);

u32_out(out, b0[0]);
u32_out(out + 4, b0[1]);
u32_out(out + 8, b0[2]);
u32_out(out + 12, b0[3]);
}

/* decrypt a block of text */

#define i_nround(bo, bi, k) \
    i_rn(bo, bi, 0, k);     \
    i_rn(bo, bi, 1, k);     \
    i_rn(bo, bi, 2, k);     \
    i_rn(bo, bi, 3, k);     \
    k -= 4

#define i_lround(bo, bi, k) \
    i_rl(bo, bi, 0, k);     \
    i_rl(bo, bi, 1, k);     \
    i_rl(bo, bi, 2, k);     \
    i_rl(bo, bi, 3, k)

void aes_decrypt(void *ctx_arg, uint8_t *out, const uint8_t *in)
{
const struct aes_ctx *ctx = (struct aes_ctx *)ctx_arg;
uint32_t b0[4], b1[4];
const int key_len = ctx->key_length;
const uint32_t *kp = D_KEY + key_len + 20;

b0[0] = u32_in (in) ^ E_KEY[key_len + 24];
b0[1] = u32_in (in + 4) ^ E_KEY[key_len + 25];
b0[2] = u32_in (in + 8) ^ E_KEY[key_len + 26];
b0[3] = u32_in (in + 12) ^ E_KEY[key_len + 27];

if(key_len > 24)
	{
	i_nround(b1, b0, kp);
	i_nround(b0, b1, kp);
	}

if(key_len > 16)
	{
	i_nround(b1, b0, kp);
	i_nround(b0, b1, kp);
	}

i_nround(b1, b0, kp);
i_nround(b0, b1, kp);
i_nround(b1, b0, kp);
i_nround(b0, b1, kp);
i_nround(b1, b0, kp);
i_nround(b0, b1, kp);
i_nround(b1, b0, kp);
i_nround(b0, b1, kp);
i_nround(b1, b0, kp);
i_lround(b0, b1, kp);

u32_out(out, b0[0]);
u32_out(out + 4, b0[1]);
u32_out(out + 8, b0[2]);
u32_out(out + 12, b0[3]);
}

#if 0
int main(void)
	{
	struct aes_ctx ctx;
	uint8_t key[24] =
		{0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23};
	uint8_t in[AES_BLOCK_SIZE] =
		{0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff};
	uint8_t out[AES_BLOCK_SIZE], out2[AES_BLOCK_SIZE];
	int i;

	aes_gen_tabs();
	aes_set_key(&ctx, key, 24);

	aes_encrypt(&ctx, out, in);
	for(i=0; i<16; i++) printf("%02x ", out[i]);
	printf("\n");

	aes_decrypt(&ctx, out2, out);
	for(i=0; i<16; i++) printf("%02x ", out2[i]);
	printf("\n");

	return 0;
	}
#endif

