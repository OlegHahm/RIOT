/*
 * Copyright (C) 2016 Oliver Hahm <oliver.hahm@inria.fr>
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
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

/* This code is public-domain - it is based on libcrypt
 * placed in the public domain by Wei Dai and other contributors. */

#ifndef _SHA1_H
#define _SHA1_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief   Length of SHA-1 digests in byte
 */
#define SHA1_DIGEST_LENGTH  (20)

/**
 * @brief   Length of SHA-1 block in byte
 */
#define SHA1_BLOCK_LENGTH   (64)

/**
 * @brief SHA-1 algorithm context
 * */
typedef struct {
    uint32_t buffer[SHA1_BLOCK_LENGTH / sizeof(uint32_t)];  /**< @internal */
    uint32_t state[SHA1_DIGEST_LENGTH / sizeof(uint32_t)];  /**< @internal */
    uint32_t byte_count;                                    /**< @internal */
    uint8_t buffer_offset;                                  /**< @internal */
} sha1_context;


/**
 * @brief Initialize SHA-1 message digest context
 *
 * @param[in] s     Pointer to the SHA-1 context to initialize
 */
void sha1_init(sha1_context *s);

/**
 * @brief Update the SHA-1 context with a portion of the message being hashed
 *
 * @param[in] s     Pointer to the SHA-1 context to update
 * @param[in] data  Pointer to the buffer to be hashed
 * @param[in] len   Length of the buffer
 */
void sha1_update(sha1_context *s, const unsigned char *data, size_t len);
/**
 * @brief Finalizes the SHA-1 message digest
 *
 * @param[in] s     Pointer to the SHA-1 contex
 *
 * @return Caluclated digest
 */
uint8_t *sha1_final(sha1_context *s);

/**
 * @brief   Calculate a SHA1 hash from the given data
 *
 * @param[out] dst      Result location, must be 20 byte
 * @param[in] src       Input data
 * @param[in] len       Length of @p buf
 */
void sha1(uint8_t *dst, const uint8_t *src, size_t len);

#ifdef __cplusplus
}
#endif

#endif /* _SHA1_H */
/** @} */
