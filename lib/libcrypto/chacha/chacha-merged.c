/* $OpenBSD: chacha-merged.c,v 1.12 2024/06/01 17:56:44 tb Exp $ */
/*
chacha-merged.c version 20080118
D. J. Bernstein
Public domain.
*/

#include <stdint.h>

#if !defined(HAVE_ATTRIBUTE__BOUNDED__) && !defined(__OpenBSD__)
#define __bounded__(x, y, z)
#endif

#define CHACHA_MINKEYLEN	16
#define CHACHA_NONCELEN		8
#define CHACHA_CTRLEN		8
#define CHACHA_STATELEN		(CHACHA_NONCELEN+CHACHA_CTRLEN)
#define CHACHA_BLOCKLEN		64

typedef uint8_t u8;
typedef uint32_t u32;

struct chacha_ctx {
	u32 input[16];
	u8 ks[CHACHA_BLOCKLEN];
	u8 unused;
};

static inline void chacha_keysetup(struct chacha_ctx *x, const u8 *k, u32 kbits)
    __attribute__((__bounded__(__minbytes__, 2, CHACHA_MINKEYLEN)));
static inline void chacha_ivsetup(struct chacha_ctx *x, const u8 *iv,
    const u8 *ctr)
    __attribute__((__bounded__(__minbytes__, 2, CHACHA_NONCELEN)))
    __attribute__((__bounded__(__minbytes__, 3, CHACHA_CTRLEN)));
static inline void chacha_encrypt_bytes(struct chacha_ctx *x, const u8 *m,
    u8 *c, u32 bytes)
    __attribute__((__bounded__(__buffer__, 2, 4)))
    __attribute__((__bounded__(__buffer__, 3, 4)));

typedef struct chacha_ctx chacha_ctx;

#define U8C(v) (v##U)
#define U32C(v) (v##U)

#define U8V(v) ((u8)(v) & U8C(0xFF))
#define U32V(v) ((u32)(v) & U32C(0xFFFFFFFF))

#define ROTL32(v, n) \
  (U32V((v) << (n)) | ((v) >> (32 - (n))))

#define U8TO32_LITTLE(p) \
  (((u32)((p)[0])) | \
   ((u32)((p)[1]) <<  8) | \
   ((u32)((p)[2]) << 16) | \
   ((u32)((p)[3]) << 24))

#define U32TO8_LITTLE(p, v) \
  do { \
    (p)[0] = U8V((v)); \
    (p)[1] = U8V((v) >>  8); \
    (p)[2] = U8V((v) >> 16); \
    (p)[3] = U8V((v) >> 24); \
  } while (0)

#define ROTATE(v,c) (ROTL32(v,c))
#define XOR(v,w) ((v) ^ (w))
#define PLUS(v,w) (U32V((v) + (w)))
#define PLUSONE(v) (PLUS((v),1))

#define QUARTERROUND(a,b,c,d) \
  a = PLUS(a,b); d = ROTATE(XOR(d,a),16); \
  c = PLUS(c,d); b = ROTATE(XOR(b,c),12); \
  a = PLUS(a,b); d = ROTATE(XOR(d,a), 8); \
  c = PLUS(c,d); b = ROTATE(XOR(b,c), 7);

/* Initialise with "expand 32-byte k". */
static const char sigma[16] = {
	0x65, 0x78, 0x70, 0x61, 0x6e, 0x64, 0x20, 0x33,
	0x32, 0x2d, 0x62, 0x79, 0x74, 0x65, 0x20, 0x6b,
};

/* Initialise with "expand 16-byte k". */
static const char tau[16] = {
	0x65, 0x78, 0x70, 0x61, 0x6e, 0x64, 0x20, 0x31,
	0x36, 0x2d, 0x62, 0x79, 0x74, 0x65, 0x20, 0x6b,
};

static inline void
chacha_keysetup(chacha_ctx *x, const u8 *k, u32 kbits)
{
	const char *constants;

	x->input[4] = U8TO32_LITTLE(k + 0);
	x->input[5] = U8TO32_LITTLE(k + 4);
	x->input[6] = U8TO32_LITTLE(k + 8);
	x->input[7] = U8TO32_LITTLE(k + 12);
	if (kbits == 256) { /* recommended */
		k += 16;
		constants = sigma;
	} else { /* kbits == 128 */
		constants = tau;
	}
	x->input[8] = U8TO32_LITTLE(k + 0);
	x->input[9] = U8TO32_LITTLE(k + 4);
	x->input[10] = U8TO32_LITTLE(k + 8);
	x->input[11] = U8TO32_LITTLE(k + 12);
	x->input[0] = U8TO32_LITTLE(constants + 0);
	x->input[1] = U8TO32_LITTLE(constants + 4);
	x->input[2] = U8TO32_LITTLE(constants + 8);
	x->input[3] = U8TO32_LITTLE(constants + 12);
}

static inline void
chacha_ivsetup(chacha_ctx *x, const u8 *iv, const u8 *counter)
{
	x->input[12] = counter == NULL ? 0 : U8TO32_LITTLE(counter + 0);
	x->input[13] = counter == NULL ? 0 : U8TO32_LITTLE(counter + 4);
	x->input[14] = U8TO32_LITTLE(iv + 0);
	x->input[15] = U8TO32_LITTLE(iv + 4);
}

static inline void
chacha_encrypt_bytes(chacha_ctx *x, const u8 *m, u8 *c, u32 bytes)
{
	u32 x0, x1, x2, x3, x4, x5, x6, x7;
	u32 x8, x9, x10, x11, x12, x13, x14, x15;
	u32 j0, j1, j2, j3, j4, j5, j6, j7;
	u32 j8, j9, j10, j11, j12, j13, j14, j15;
	u8 *ctarget = NULL;
	u8 tmp[64];
	u32 i;

	if (!bytes)
		return;

	j0 = x->input[0];
	j1 = x->input[1];
	j2 = x->input[2];
	j3 = x->input[3];
	j4 = x->input[4];
	j5 = x->input[5];
	j6 = x->input[6];
	j7 = x->input[7];
	j8 = x->input[8];
	j9 = x->input[9];
	j10 = x->input[10];
	j11 = x->input[11];
	j12 = x->input[12];
	j13 = x->input[13];
	j14 = x->input[14];
	j15 = x->input[15];

	for (;;) {
		if (bytes < 64) {
			for (i = 0; i < bytes; ++i)
				tmp[i] = m[i];
			m = tmp;
			ctarget = c;
			c = tmp;
		}
		x0 = j0;
		x1 = j1;
		x2 = j2;
		x3 = j3;
		x4 = j4;
		x5 = j5;
		x6 = j6;
		x7 = j7;
		x8 = j8;
		x9 = j9;
		x10 = j10;
		x11 = j11;
		x12 = j12;
		x13 = j13;
		x14 = j14;
		x15 = j15;
		for (i = 20; i > 0; i -= 2) {
			QUARTERROUND(x0, x4, x8, x12)
			QUARTERROUND(x1, x5, x9, x13)
			QUARTERROUND(x2, x6, x10, x14)
			QUARTERROUND(x3, x7, x11, x15)
			QUARTERROUND(x0, x5, x10, x15)
			QUARTERROUND(x1, x6, x11, x12)
			QUARTERROUND(x2, x7, x8, x13)
			QUARTERROUND(x3, x4, x9, x14)
		}
		x0 = PLUS(x0, j0);
		x1 = PLUS(x1, j1);
		x2 = PLUS(x2, j2);
		x3 = PLUS(x3, j3);
		x4 = PLUS(x4, j4);
		x5 = PLUS(x5, j5);
		x6 = PLUS(x6, j6);
		x7 = PLUS(x7, j7);
		x8 = PLUS(x8, j8);
		x9 = PLUS(x9, j9);
		x10 = PLUS(x10, j10);
		x11 = PLUS(x11, j11);
		x12 = PLUS(x12, j12);
		x13 = PLUS(x13, j13);
		x14 = PLUS(x14, j14);
		x15 = PLUS(x15, j15);

		if (bytes < 64) {
			U32TO8_LITTLE(x->ks + 0, x0);
			U32TO8_LITTLE(x->ks + 4, x1);
			U32TO8_LITTLE(x->ks + 8, x2);
			U32TO8_LITTLE(x->ks + 12, x3);
			U32TO8_LITTLE(x->ks + 16, x4);
			U32TO8_LITTLE(x->ks + 20, x5);
			U32TO8_LITTLE(x->ks + 24, x6);
			U32TO8_LITTLE(x->ks + 28, x7);
			U32TO8_LITTLE(x->ks + 32, x8);
			U32TO8_LITTLE(x->ks + 36, x9);
			U32TO8_LITTLE(x->ks + 40, x10);
			U32TO8_LITTLE(x->ks + 44, x11);
			U32TO8_LITTLE(x->ks + 48, x12);
			U32TO8_LITTLE(x->ks + 52, x13);
			U32TO8_LITTLE(x->ks + 56, x14);
			U32TO8_LITTLE(x->ks + 60, x15);
		}

		x0 = XOR(x0, U8TO32_LITTLE(m + 0));
		x1 = XOR(x1, U8TO32_LITTLE(m + 4));
		x2 = XOR(x2, U8TO32_LITTLE(m + 8));
		x3 = XOR(x3, U8TO32_LITTLE(m + 12));
		x4 = XOR(x4, U8TO32_LITTLE(m + 16));
		x5 = XOR(x5, U8TO32_LITTLE(m + 20));
		x6 = XOR(x6, U8TO32_LITTLE(m + 24));
		x7 = XOR(x7, U8TO32_LITTLE(m + 28));
		x8 = XOR(x8, U8TO32_LITTLE(m + 32));
		x9 = XOR(x9, U8TO32_LITTLE(m + 36));
		x10 = XOR(x10, U8TO32_LITTLE(m + 40));
		x11 = XOR(x11, U8TO32_LITTLE(m + 44));
		x12 = XOR(x12, U8TO32_LITTLE(m + 48));
		x13 = XOR(x13, U8TO32_LITTLE(m + 52));
		x14 = XOR(x14, U8TO32_LITTLE(m + 56));
		x15 = XOR(x15, U8TO32_LITTLE(m + 60));

		j12 = PLUSONE(j12);
		if (!j12) {
			j13 = PLUSONE(j13);
			/*
			 * Stopping at 2^70 bytes per nonce is the user's
			 * responsibility.
			 */
		}

		U32TO8_LITTLE(c + 0, x0);
		U32TO8_LITTLE(c + 4, x1);
		U32TO8_LITTLE(c + 8, x2);
		U32TO8_LITTLE(c + 12, x3);
		U32TO8_LITTLE(c + 16, x4);
		U32TO8_LITTLE(c + 20, x5);
		U32TO8_LITTLE(c + 24, x6);
		U32TO8_LITTLE(c + 28, x7);
		U32TO8_LITTLE(c + 32, x8);
		U32TO8_LITTLE(c + 36, x9);
		U32TO8_LITTLE(c + 40, x10);
		U32TO8_LITTLE(c + 44, x11);
		U32TO8_LITTLE(c + 48, x12);
		U32TO8_LITTLE(c + 52, x13);
		U32TO8_LITTLE(c + 56, x14);
		U32TO8_LITTLE(c + 60, x15);

		if (bytes <= 64) {
			if (bytes < 64) {
				for (i = 0; i < bytes; ++i)
					ctarget[i] = c[i];
			}
			x->input[12] = j12;
			x->input[13] = j13;
			x->unused = 64 - bytes;
			return;
		}
		bytes -= 64;
		c += 64;
		m += 64;
	}
}

void
CRYPTO_hchacha_20(unsigned char subkey[32], const unsigned char key[32],
    const unsigned char nonce[16])
{
	uint32_t x[16];
	int i;

	x[0] = U8TO32_LITTLE(sigma + 0);
	x[1] = U8TO32_LITTLE(sigma + 4);
	x[2] = U8TO32_LITTLE(sigma + 8);
	x[3] = U8TO32_LITTLE(sigma + 12);
	x[4] = U8TO32_LITTLE(key + 0);
	x[5] = U8TO32_LITTLE(key + 4);
	x[6] = U8TO32_LITTLE(key + 8);
	x[7] = U8TO32_LITTLE(key + 12);
	x[8] = U8TO32_LITTLE(key + 16);
	x[9] = U8TO32_LITTLE(key + 20);
	x[10] = U8TO32_LITTLE(key + 24);
	x[11] = U8TO32_LITTLE(key + 28);
	x[12] = U8TO32_LITTLE(nonce + 0);
	x[13] = U8TO32_LITTLE(nonce + 4);
	x[14] = U8TO32_LITTLE(nonce + 8);
	x[15] = U8TO32_LITTLE(nonce + 12);

	for (i = 20; i > 0; i -= 2) {
		QUARTERROUND(x[0], x[4], x[8], x[12])
		QUARTERROUND(x[1], x[5], x[9], x[13])
		QUARTERROUND(x[2], x[6], x[10], x[14])
		QUARTERROUND(x[3], x[7], x[11], x[15])
		QUARTERROUND(x[0], x[5], x[10], x[15])
		QUARTERROUND(x[1], x[6], x[11], x[12])
		QUARTERROUND(x[2], x[7], x[8], x[13])
		QUARTERROUND(x[3], x[4], x[9], x[14])
	}

	U32TO8_LITTLE(subkey + 0, x[0]);
	U32TO8_LITTLE(subkey + 4, x[1]);
	U32TO8_LITTLE(subkey + 8, x[2]);
	U32TO8_LITTLE(subkey + 12, x[3]);

	U32TO8_LITTLE(subkey + 16, x[12]);
	U32TO8_LITTLE(subkey + 20, x[13]);
	U32TO8_LITTLE(subkey + 24, x[14]);
	U32TO8_LITTLE(subkey + 28, x[15]);
}
LCRYPTO_ALIAS(CRYPTO_hchacha_20);
