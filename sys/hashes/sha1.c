/*
 * Copyright (C) 2016 Oliver Hahm <oliver.hahm@inria.fr>
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/* This code is public-domain - it is based on libcrypt
 * placed in the public domain by Wei Dai and other contributors.
 */

/**
 * @defgroup    sys_hashes_sha1 SHA-1
 * @ingroup     sys_hashes
 * @brief       Implementation of the SHA-1 hashing function

 * @{
 *
 * @file
 * @brief       SHA-1 interface definition
 *
 * @author      Wei Dai and others
 * @author      Oliver Hahm <oliver.hahm@inria.fr>
 */

#include <stdint.h>
#include <string.h>

#include "hashes/sha1.h"

#define SHA1_K0  0x5a827999
#define SHA1_K20 0x6ed9eba1
#define SHA1_K40 0x8f1bbcdc
#define SHA1_K60 0xca62c1d6

void sha1_init(sha1_context *s)
{
    s->state[0] = 0x67452301;
    s->state[1] = 0xefcdab89;
    s->state[2] = 0x98badcfe;
    s->state[3] = 0x10325476;
    s->state[4] = 0xc3d2e1f0;
    s->byte_count = 0;
    s->buffer_offset = 0;
}

uint32_t sha1_rol32(uint32_t number, uint8_t bits)
{
    return ((number << bits) | (number >> (32 - bits)));
}

void sha1_hash_block(sha1_context *s)
{
    uint8_t i;
    uint32_t a, b, c, d, e, t;

    a = s->state[0];
    b = s->state[1];
    c = s->state[2];
    d = s->state[3];
    e = s->state[4];
    for (i = 0; i < 80; i++) {
        if (i >= 16) {
            t = s->buffer[(i + 13) & 15] ^ s->buffer[(i + 8) & 15] ^
                s->buffer[(i + 2) & 15] ^ s->buffer[i & 15];
            s->buffer[i & 15] = sha1_rol32(t, 1);
        }
        if (i < 20) {
            t = (d ^ (b & (c ^ d))) + SHA1_K0;
        }
        else if (i < 40) {
            t = (b ^ c ^ d) + SHA1_K20;
        }
        else if (i < 60) {
            t = ((b & c) | (d & (b | c))) + SHA1_K40;
        }
        else {
            t = (b ^ c ^ d) + SHA1_K60;
        }
        t += sha1_rol32(a, 5) + e + s->buffer[i & 15];
        e = d;
        d = c;
        c = sha1_rol32(b, 30);
        b = a;
        a = t;
    }
    s->state[0] += a;
    s->state[1] += b;
    s->state[2] += c;
    s->state[3] += d;
    s->state[4] += e;
}

void sha1_add_uncounted(sha1_context *s, uint8_t data)
{
    uint8_t *const b = (uint8_t *) s->buffer;

#ifdef __BIG_ENDIAN__
    b[s->buffer_offset] = data;
#else
    b[s->buffer_offset ^ 3] = data;
#endif
    s->buffer_offset++;
    if (s->buffer_offset == SHA1_BLOCK_LENGTH) {
        sha1_hash_block(s);
        s->buffer_offset = 0;
    }
}

void sha1_update(sha1_context *s, const unsigned char *data, size_t len)
{
    while (len--) {
        ++s->byte_count;
        sha1_add_uncounted(s, *data++);
    }
}

void sha1_pad(sha1_context *s)
{
    /* Implement SHA-1 padding (fips180-2 §5.1.1) */

    /* Pad with 0x80 followed by 0x00 until the end of the block */
    sha1_add_uncounted(s, 0x80);
    while (s->buffer_offset != 56) {
        sha1_add_uncounted(s, 0x00);
    }

    /* Append length in the last 8 bytes */
    sha1_add_uncounted(s, 0);                   /* We're only using 32 bit lengths */
    sha1_add_uncounted(s, 0);                   /* But SHA-1 supports 64 bit lengths */
    sha1_add_uncounted(s, 0);                   /* So zero pad the top bits */
    sha1_add_uncounted(s, s->byte_count >> 29); /* Shifting to multiply by 8 */
    sha1_add_uncounted(s, s->byte_count >> 21); /* as SHA-1 supports bitstreams as well as */
    sha1_add_uncounted(s, s->byte_count >> 13); /* byte. */
    sha1_add_uncounted(s, s->byte_count >> 5);
    sha1_add_uncounted(s, s->byte_count << 3);
}

uint8_t *sha1_final(sha1_context *s)
{
    /* Pad to complete the last block */
    sha1_pad(s);

    /* Swap byte order back */
    for (int i = 0; i < 5; i++) {
        s->state[i] =
            (((s->state[i]) << 24) & 0xff000000)
            | (((s->state[i]) << 8) & 0x00ff0000)
            | (((s->state[i]) >> 8) & 0x0000ff00)
            | (((s->state[i]) >> 24) & 0x000000ff);
    }

    /* Return pointer to hash (20 characters) */
    return (uint8_t *) s->state;
}

void sha1(uint8_t *dst, const uint8_t *src, size_t len)
{
    sha1_context ctx;
    sha1_init(&ctx);
    sha1_update(&ctx, (unsigned char*) src, len);
    memcpy(dst, sha1_final(&ctx), SHA1_DIGEST_LENGTH);
}

unsigned char * hmac_sha1(const unsigned char *key,
                            size_t key_length,
                            const unsigned *message,
                            size_t message_length,
                            unsigned char *result)
{
    unsigned char k[64];
    memset((void *)k, 0x00, 64);

    if (key_length > 64) {
        sha1((uint8_t*)k, key, key_length);
    }
    else {
        memcpy((void *)k, key, key_length);
    }

    /* 
     * create the inner and outer keypads 
     * rising hamming distance enforcing i_* and o_* are distinct
     * in at least one bit 
     */
    unsigned char o_key_pad[64];
    unsigned char i_key_pad[64];

    for (size_t i = 0; i < 64; ++i) {
        o_key_pad[i] = 0x5c^k[i];
        i_key_pad[i] = 0x36^k[i];
    }

    /*
     * Create the inner hash 
     * tmp = hash(i_key_pad CONCAT message) 
     */
    sha1_context c;
    unsigned char tmp[SHA1_DIGEST_LENGTH];

    sha1_init(&c);
    sha1_update(&c, i_key_pad, 64);
    sha1_update(&c, (unsigned char *)message, message_length);
    memcpy(tmp, sha1_final(&c), SHA1_DIGEST_LENGTH);

    static unsigned char m[SHA1_DIGEST_LENGTH];

    if (result == NULL) {
        result = m;
    }

    /*
     * Create the outer hash 
     * result = hash(o_key_pad CONCAT tmp) 
     */
    sha1_init(&c);
    sha1_update(&c, o_key_pad, 64);
    sha1_update(&c, tmp, SHA1_DIGEST_LENGTH);
    memcpy(result, sha1_final(&c), SHA1_DIGEST_LENGTH);

    return result;
}
