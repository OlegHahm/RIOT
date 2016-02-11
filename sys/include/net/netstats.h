/*
 * Copyright (C) INRIA
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @defgroup    net_netstats Packet statistics per layer
 * @ingroup     net
 * @brief       Each layer may store information about sent and received packets
 * @{
 *
 * @file
 * @brief       Definition of net statistics
 *
 * @author      Oliver Hahm <oliver.hahm@inria.fr>
 */

#ifndef NETSTATS_H
#define NETSTATS_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief       Global statistics struct
 */
typedef struct {
    uint32_t tx_unicast_count;  /**< packets sent via unicast */
    uint32_t tx_mcast_count;    /**< packets sent via multicast
                                     (including broadcast */
    uint32_t tx_failed;         /**< failed sending operations */
    uint32_t acks_count;        /**< received acknowledgements */
    uint32_t rx_count;          /**< received (data) packets */
} netstats_t;

#ifdef __cplusplus
}
#endif

#endif /* NETSTATS_H */
/** @} */
