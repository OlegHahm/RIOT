/*
 * Copyright (C) 2016 Freie Universität Berlin
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @defgroup    pkg_lwip_sock   lwIP-specific implementation of sock API
 * @ingroup     pkg_lwip
 * @brief       Provides an implementation of the @ref net_sock for the
 *              @ref pkg_lwip
 * @{
 *
 * @file
 * @brief       lwIP-specific function @ref  definitions
 *
 * @author  Martine Lenders <mlenders@inf.fu-berlin.de>
 */
#ifndef SOCK_INTERNAL_H_
#define SOCK_INTERNAL_H_

#include <stdbool.h>
#include <stdint.h>

#include "net/af.h"
#include "net/sock.h"

#include "lwip/ip_addr.h"
#include "lwip/api.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Internal helper functions for lwIP
 * @internal
 * @{
 */
int lwip_sock_create(struct netconn **conn, const struct _sock_tl_ep *local,
                      const struct _sock_tl_ep *remote, int proto,
                      uint16_t flags, int type);
uint16_t lwip_sock_bind_addr_to_netif(const ip_addr_t *bind_addr);
int lwip_sock_get_addr(struct netconn *conn, struct _sock_tl_ep *ep, u8_t local);
int lwip_sock_recv(struct netconn *conn, uint32_t timeout, struct netbuf **buf);
ssize_t lwip_sock_send(struct netconn **conn, const void *data, size_t len,
                       int proto, const struct _sock_tl_ep *remote, int type);
/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* SOCK_INTERNAL_H_ */
/** @} */
