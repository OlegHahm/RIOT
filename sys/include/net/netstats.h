/*
 * Copyright (C) 2016 INRIA
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @defgroup    net_netstats Packet statistics per module
 * @ingroup     net
 * @brief       Each module may store information about sent and received packets
 * @{
 *
 * @file
 * @brief       Definition of net statistics
 *
 * @author      Oliver Hahm <oliver.hahm@inria.fr>
 */

#include <stdint.h>

#ifndef NETSTATS_H
#define NETSTATS_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @name @ref net_netstats module names
 * @{
 */
#define NETSTATS_LAYER2     (0x01)
#define NETSTATS_IPV6       (0x02)
#define NETSTATS_RPL        (0x03)
#define NETSTATS_ALL        (0xFF)
/** @} */

/**
 * @brief       Global statistics struct
 */
typedef struct {
    uint32_t tx_unicast_count;  /**< packets sent via unicast */
    uint32_t tx_mcast_count;    /**< packets sent via multicast
                                     (including broadcast) */
    uint32_t tx_success;        /**< successful sending operations
                                     (either acknowledged or unconfirmed
                                     sending operation, e.g. multicast) */
    uint32_t tx_failed;         /**< failed sending operations */
    uint32_t tx_bytes;          /**< sent bytes */
    uint32_t rx_count;          /**< received (data) packets */
    uint32_t rx_bytes;          /**< received bytes */
    uint64_t time_active;       /**< time spent in active mode */
    uint64_t time_sleeping;     /**< time spent in sleeping mode */
} netstats_t;

#ifdef __cplusplus
}
#endif

#endif /* NETSTATS_H */
/** @} */
