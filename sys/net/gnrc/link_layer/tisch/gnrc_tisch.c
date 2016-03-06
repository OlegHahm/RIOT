/*
 * Copyright (C) 2015 Freie Universität Berlin
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @{
 * @ingroup     net_tisch
 * @file
 * @brief       Implementation of the TISCH MAC protocol
 *
 * @author      Hauke Petersen <hauke.petersen@fu-berlin.de>
 * @}
 */

#include <errno.h>

#include "irq.h"
#include "kernel.h"
#include "msg.h"
#include "thread.h"

#include "net/gnrc/tisch.h"
#include "net/gnrc.h"

#include "idmanager.h"
#include "openqueue.h"
#include "openrandom.h"
#include "opentimers.h"
#include "adaptive_sync.h"
#include "IEEE802154E.h"
#include "sixtop.h"
#include "neighbors.h"

#define ENABLE_DEBUG    (0)
#include "debug.h"

#if ENABLE_DEBUG
/* For PRIu16 etc. */
#include <inttypes.h>
#endif

/**
 * @brief   Function called by the device driver on device events
 *
 * @param[in] event         type of event
 * @param[in] data          optional parameter
 */
static void _event_cb(gnrc_netdev_event_t event, void *data)
{
    DEBUG("tisch: event triggered -> %i\n", event);
    /* TISCH only understands the RX_COMPLETE event... */
    if (event == NETDEV_EVENT_RX_COMPLETE) {
        gnrc_pktsnip_t *pkt;

        /* get pointer to the received packet */
        pkt = (gnrc_pktsnip_t *)data;
        /* send the packet to everyone interested in it's type */
        if (!gnrc_netapi_dispatch_receive(pkt->type, GNRC_NETREG_DEMUX_CTX_ALL, pkt)) {
            DEBUG("tisch: unable to forward packet of type %i\n", pkt->type);
            gnrc_pktbuf_release(pkt);
        }
    }
}

/**
 * @brief   Startup code and event loop of the TISCH layer
 *
 * @param[in] args          expects a pointer to the underlying netdev device
 *
 * @return                  never returns
 */
static void *_tisch_thread(void *args)
{
    gnrc_netdev_t *dev = (gnrc_netdev_t *)args;
    gnrc_netapi_opt_t *opt;
    int res;
    msg_t msg, reply, msg_queue[GNRC_TISCH_MSG_QUEUE_SIZE];

    /* setup the MAC layers message queue */
    msg_init_queue(msg_queue, GNRC_TISCH_MSG_QUEUE_SIZE);
    /* save the PID to the device descriptor and register the device */
    dev->mac_pid = thread_getpid();
    gnrc_netif_add(dev->mac_pid);
    /* register the event callback with the device driver */
    dev->driver->add_event_callback(dev, _event_cb);

    /* start the event loop */
    while (1) {
        DEBUG("tisch: waiting for incoming messages\n");
        msg_receive(&msg);
        /* dispatch NETDEV and NETAPI messages */
        switch (msg.type) {
            case GNRC_NETDEV_MSG_TYPE_EVENT:
                DEBUG("tisch: GNRC_NETDEV_MSG_TYPE_EVENT received\n");
                dev->driver->isr_event(dev, msg.content.value);
                break;
            case GNRC_NETAPI_MSG_TYPE_SND:
                DEBUG("tisch: GNRC_NETAPI_MSG_TYPE_SND received\n");
                dev->driver->send_data(dev, (gnrc_pktsnip_t *)msg.content.ptr);
                break;
            case GNRC_NETAPI_MSG_TYPE_SET:
                /* TODO: filter out MAC layer options -> for now forward
                         everything to the device driver */
                DEBUG("tisch: GNRC_NETAPI_MSG_TYPE_SET received\n");
                /* read incoming options */
                opt = (gnrc_netapi_opt_t *)msg.content.ptr;
                /* set option for device driver */
                res = dev->driver->set(dev, opt->opt, opt->data, opt->data_len);
                DEBUG("tisch: response of netdev->set: %i\n", res);
                /* send reply to calling thread */
                reply.type = GNRC_NETAPI_MSG_TYPE_ACK;
                reply.content.value = (uint32_t)res;
                msg_reply(&msg, &reply);
                break;
            case GNRC_NETAPI_MSG_TYPE_GET:
                /* TODO: filter out MAC layer options -> for now forward
                         everything to the device driver */
                DEBUG("tisch: GNRC_NETAPI_MSG_TYPE_GET received\n");
                /* read incoming options */
                opt = (gnrc_netapi_opt_t *)msg.content.ptr;
                /* get option from device driver */
                res = dev->driver->get(dev, opt->opt, opt->data, opt->data_len);
                DEBUG("tisch: response of netdev->get: %i\n", res);
                /* send reply to calling thread */
                reply.type = GNRC_NETAPI_MSG_TYPE_ACK;
                reply.content.value = (uint32_t)res;
                msg_reply(&msg, &reply);
                break;
            default:
                DEBUG("tisch: Unknown command %" PRIu16 "\n", msg.type);
                break;
        }
    }
    /* never reached */
    return NULL;
}

kernel_pid_t gnrc_tisch_init(char *stack, int stacksize, char priority,
                             const char *name, gnrc_netdev_t *dev)
{
    kernel_pid_t res;

    /* check if given netdev device is defined and the driver is set */
    if (dev == NULL || dev->driver == NULL) {
        return -ENODEV;
    }

    //===== stack
    //-- cross-layer
    idmanager_init();    // call first since initializes EUI64 and isDAGroot
    openqueue_init();
    openrandom_init();
    opentimers_init();
    //-- 02a-TSCH
    adaptive_sync_init();
    ieee154e_init();
    //-- 02b-RES
    schedule_init();
    sixtop_init();
    neighbors_init();

    /* create new TISCH thread */
    res = thread_create(stack, stacksize, priority, CREATE_STACKTEST,
            _tisch_thread, (void *)dev, name);
    if (res <= 0) {
        return -EINVAL;
    }
    return res;
}
