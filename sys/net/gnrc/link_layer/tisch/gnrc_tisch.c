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
#include <string.h>

#include "irq.h"
#include "msg.h"
#include "thread.h"

#include "net/ieee802154.h"
#include "net/gnrc/netdev2/ieee802154.h"
#include "net/gnrc/tisch.h"
#include "net/gnrc.h"
#include "net/gnrc/nettype.h"

#include "radio.h"
#include "idmanager.h"
#include "openqueue.h"
#include "openrandom.h"
#include "opentimers.h"
#include "adaptive_sync.h"
#include "IEEE802154E.h"
#include "sixtop.h"
#include "neighbors.h"
#include "scheduler.h"

#define ENABLE_DEBUG    (0)
#include "debug.h"

/* For PRIu16 etc. */
#include <inttypes.h>

/* TODO: in order to use tisch on more than one device these variables need to
 * moved to thread scope */
scheduler_vars_t scheduler_vars;
scheduler_dbg_t  scheduler_dbg;
kernel_pid_t gnrc_tisch_scheduler_pid;

msg_t gnrc_tisch_msg = { .type = GNRC_TISCH_NETAPI_MSG_TYPE, .content.ptr = NULL};

void scheduler_push_task(task_cbt cb, task_prio_t prio)
{
    taskList_item_t*  taskContainer;
    taskList_item_t** taskListWalker;

    unsigned state = irq_disable();
    DEBUG("tisch: push task\n");
    /* find an empty task container */
    taskContainer = &scheduler_vars.taskBuf[0];
    while (taskContainer->cb!=NULL &&
           taskContainer <= &scheduler_vars.taskBuf[TASK_LIST_DEPTH-1]) {
        taskContainer++;
    }
    if (taskContainer>&scheduler_vars.taskBuf[TASK_LIST_DEPTH-1]) {
        /* task list has overflown. This should never happpen! */

        core_panic(PANIC_GENERAL_ERROR, "tisch scheduler overflow");
    }
    /* fill that task container with this task */
    taskContainer->cb              = cb;
    taskContainer->prio            = prio;

    /* find position in queue */
    taskListWalker                 = &scheduler_vars.task_list;
    while (*taskListWalker != NULL && (*taskListWalker)->prio < taskContainer->prio) {
        taskListWalker              = (taskList_item_t**) &((*taskListWalker)->next);
    }
    /* insert at that position */
    taskContainer->next            = *taskListWalker;
    *taskListWalker                = taskContainer;
    /* maintain debug stats */
    scheduler_dbg.numTasksCur++;

    if (scheduler_dbg.numTasksCur > scheduler_dbg.numTasksMax) {
        scheduler_dbg.numTasksMax   = scheduler_dbg.numTasksCur;
    }

    irq_restore(state);
    SCHEDULER_WAKEUP();
}

void _tsch_send(gnrc_pktsnip_t *snip)
{
    /* XXX: convert packet */
    gnrc_netif_hdr_t *netif_hdr = snip->data;
    uint8_t *addr = gnrc_netif_hdr_get_dst_addr(netif_hdr);

    /* first create openqueue entry */
    OpenQueueEntry_t* pkt;
    pkt = openqueue_getFreePacketBuffer(COMPONENT_RIOT);
    if (pkt==NULL) {
        DEBUG("tisch: error getting free packet buffer entry\n");
        return;
    }

    pkt->creator                   = COMPONENT_RIOT;
    pkt->owner                     = COMPONENT_RIOT;

    /* TODO: support short addresses */
    pkt->l2_nextORpreviousHop.type = ADDR_64B;
    memcpy(&pkt->l2_nextORpreviousHop, addr, LENGTH_ADDR64b);

    /* copy everything after netif_hdr into payload */
    snip = snip->next;
    size_t offset = 0;
    while (snip != NULL) {
        memcpy(pkt->payload + offset, snip->data, snip->size);
        offset += snip->size;
        snip = snip->next;
    }
    gnrc_pktbuf_release(snip);

    /* call TSCH sending function */
    sixtop_send(pkt);
}

static void _pass_on_packet(gnrc_pktsnip_t *pkt)
{
    /* throw away packet if no one is interested */
    if (!gnrc_netapi_dispatch_receive(pkt->type, GNRC_NETREG_DEMUX_CTX_ALL, pkt)) {
        DEBUG("gnrc_netdev2: unable to forward packet of type %i\n", pkt->type);
        gnrc_pktbuf_release(pkt);
        return;
    }
}

void iphc_receive(OpenQueueEntry_t *msg)
{
    size_t nread = msg->length;

    DEBUG("tisch: received packet\n");
    /* convert OpenQueueEntry to pktsnip */
    gnrc_pktsnip_t *pkt = gnrc_pktbuf_add(NULL, NULL, nread, GNRC_NETTYPE_UNDEF);

    if (pkt == NULL) {
        DEBUG("tisch iphc_receive: cannot allocate pktsnip.\n");
        return;
    }

    memcpy(pkt->data, msg->packet, msg->length);

    gnrc_pktsnip_t *ieee802154_hdr, *netif_hdr;
    gnrc_netif_hdr_t *hdr;
#if ENABLE_DEBUG
    char src_str[GNRC_NETIF_HDR_L2ADDR_MAX_LEN];
#endif
    size_t mhr_len = ieee802154_get_frame_hdr_len(pkt->data);

    if (mhr_len == 0) {
        DEBUG("tisch iphc_receive: illegally formatted frame received\n");
        gnrc_pktbuf_release(pkt);
        return;
    }
    nread -= mhr_len;
    /* mark IEEE 802.15.4 header */
    ieee802154_hdr = gnrc_pktbuf_mark(pkt, mhr_len, GNRC_NETTYPE_UNDEF);
    if (ieee802154_hdr == NULL) {
        DEBUG("tisch iphc_receive: no space left in packet buffer\n");
        gnrc_pktbuf_release(pkt);
        return;
    }
    netif_hdr = gnrc_netdev2_ieee802154_make_netif_hdr(ieee802154_hdr->data);
    if (netif_hdr == NULL) {
        DEBUG("tisch iphc_receive: no space left in packet buffer\n");
        gnrc_pktbuf_release(pkt);
        return;
    }
    hdr = netif_hdr->data;
    hdr->lqi = msg->l1_lqi;
    hdr->rssi = msg->l1_rssi;
    hdr->if_pid = thread_getpid();
    /* XXX */
#ifdef MODULE_SIXLOWPAN
    pkt->type = GNRC_NETTYPE_SIXLOWPAN;
#elif MODULE_CCN_LITE
    pkt->type = GNRC_NETTYPE_CCN;
#else
    pkt->type = GNRC_NETTYPE_UNDEF;
#endif

#if ENABLE_DEBUG
    DEBUG("tisch iphc_receive: received packet from %s of length %u\n",
          gnrc_netif_addr_to_str(src_str, sizeof(src_str),
                                 gnrc_netif_hdr_get_src_addr(hdr),
                                 hdr->src_l2addr_len),
          nread);
#if defined(MODULE_OD)
    od_hex_dump(pkt->data, nread, OD_WIDTH_DEFAULT);
#endif
#endif
    gnrc_pktbuf_remove_snip(pkt, ieee802154_hdr);
    LL_APPEND(pkt, netif_hdr);

    /* TODO: pass packet up */
    _pass_on_packet(pkt);

    openqueue_freePacketBuffer(msg);
}

/**
 * @brief   Function called by the device driver on device events
 *
 * @param[in] event         type of event
 * @param[in] data          optional parameter
 */
static void _event_cb(netdev2_t *dev, netdev2_event_t event, void *data)
{
    (void) data;
    gnrc_netdev2_t *gnrc_netdev2 = (gnrc_netdev2_t*) dev->isr_arg;
    (void) gnrc_netdev2;

    void event_cb(netdev2_t *dev, netdev2_event_t event, void *data);
    event_cb(dev, event, data);
    if (event == NETDEV2_EVENT_ISR) {

        /* TODO: pipe this into 802154 from TISCH */
    }
    else {
        DEBUG("tisch: event triggered -> %i\n", event);
        switch(event) {
            case NETDEV2_EVENT_RX_COMPLETE:
                DEBUG("tisch: received RX complete - this should not have happened!\n");
                break;
            default:
                DEBUG("tisch: warning: unhandled event %u.\n", event);
        }
    }
}

static int _tisch_init(gnrc_netdev2_t *gnrc_netdev2)
{
    /* check if given netdev device is defined and the driver is set */
    if (gnrc_netdev2 == NULL || gnrc_netdev2->dev == NULL) {
        return -ENODEV;
    }

    //-- cross-layer
    idmanager_init();    // call first since initializes EUI64 and isDAGroot
    radio_init(gnrc_netdev2);
    radiotimer_init();

    //===== stack
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

    return 0;
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
    gnrc_netdev2_t *gnrc_netdev2 = (gnrc_netdev2_t *)args;
    netdev2_t *dev = gnrc_netdev2->dev;
    msg_t msg, reply, msg_queue[GNRC_TISCH_MSG_QUEUE_SIZE];

    /* setup the MAC layers message queue */
    msg_init_queue(msg_queue, GNRC_TISCH_MSG_QUEUE_SIZE);
    /* save the PID to the device descriptor and register the device */
    gnrc_netdev2->pid = thread_getpid();

    _tisch_init(gnrc_netdev2);

    /* register the event callback with the device driver */
    dev->event_callback = _event_cb;
    dev->isr_arg = (void*) gnrc_netdev2;

    /* register the device to the network stack*/
    gnrc_netif_add(thread_getpid());

    taskList_item_t *pThisTask;

    /* start the event loop */
    while (1) {
        DEBUG("tisch: waiting for incoming messages\n");
        msg_receive(&msg);

        /* dispatch NETDEV and NETAPI messages */
        switch (msg.type) {
            case GNRC_TISCH_NETAPI_MSG_TYPE:
                DEBUG("tisch: GNRC_TISCH_NETAPI_MSG_TYPE received.\n");
                while(scheduler_vars.task_list != NULL) {
                    /* there is still at least one task in the linked-list of tasks */

                    /* the task to execute is the one at the head of the queue */
                    pThisTask                = scheduler_vars.task_list;

                    /* shift the queue by one task */
                    scheduler_vars.task_list = pThisTask->next;

                    /* execute the current task */
                    pThisTask->cb();

                    /* free up this task container */
                    pThisTask->cb            = NULL;
                    pThisTask->prio          = TASKPRIO_NONE;
                    pThisTask->next          = NULL;
                    scheduler_dbg.numTasksCur--;
                }
                break;
            case GNRC_NETDEV_MSG_TYPE_EVENT:
                DEBUG("tisch: GNRC_NETDEV_MSG_TYPE_EVENT received\n");
                dev->driver->isr(dev);
                break;
            case GNRC_NETAPI_MSG_TYPE_SND:
                DEBUG("tisch: GNRC_NETAPI_MSG_TYPE_SND received\n");
                _tsch_send((gnrc_pktsnip_t *)msg.content.ptr);

                break;
            case GNRC_NETAPI_MSG_TYPE_SET:
                /* TODO: filter out MAC layer options -> for now forward
                         everything to the device driver */
                DEBUG("tisch: GNRC_NETAPI_MSG_TYPE_SET received\n");

                /* TODO: for now we don't support anything */
                reply.type = GNRC_NETAPI_MSG_TYPE_ACK;
                reply.content.value = -ENOTSUP;
                msg_reply(&msg, &reply);
                break;
            case GNRC_NETAPI_MSG_TYPE_GET:
                /* TODO: filter out MAC layer options -> for now forward
                         everything to the device driver */
                DEBUG("tisch: GNRC_NETAPI_MSG_TYPE_GET received\n");
                /* read incoming options */

                /* TODO: for now we don't support anything */
                reply.type = GNRC_NETAPI_MSG_TYPE_ACK;
                reply.content.value = -ENOTSUP;
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
                             const char *name, gnrc_netdev2_t *gnrc_netdev2)
{
    /* create new TISCH thread */
    gnrc_tisch_scheduler_pid = thread_create(stack, stacksize, priority,
                                             THREAD_CREATE_STACKTEST, _tisch_thread,
                                             (void *)gnrc_netdev2, name);
    if (gnrc_tisch_scheduler_pid <= 0) {
        return -EINVAL;
    }
    return gnrc_tisch_scheduler_pid;
}
