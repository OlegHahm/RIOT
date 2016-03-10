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

    unsigned state = disableIRQ();
    /* find an empty task container */
    taskContainer = &scheduler_vars.taskBuf[0];
    while (taskContainer->cb!=NULL &&
           taskContainer <= &scheduler_vars.taskBuf[TASK_LIST_DEPTH-1]) {
        taskContainer++;
    }
    if (taskContainer>&scheduler_vars.taskBuf[TASK_LIST_DEPTH-1]) {
        /* task list has overflown. This should never happpen! */

        core_panic(PANIC_GENERAL_ERROR, "scheduler overflow");
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

    restoreIRQ(state);
}

void _tsch_send(gnrc_pktsnip_t *pkt)
{
    /* XXX: convert packet */

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

    /* call TSCH sending function */
    sixtop_send(pkt);
}

void iphc_receive(OpenQueueEntry_t *msg)
{
    DEBUG("tisch: received packet\n");
    /* TODO: convert OpenQueueEntry to pktsnip */
    /* TODO: pass packet up */
    openqueue_freePacketBuffer(msg);
}

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
                dev->driver->isr_event(dev, msg.content.value);
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
                             const char *name, gnrc_netdev_t *dev)
{
    /* check if given netdev device is defined and the driver is set */
    if (dev == NULL || dev->driver == NULL) {
        return -ENODEV;
    }

    radio_init(dev);
    radiotimer_init();

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
    gnrc_tisch_scheduler_pid = thread_create(stack, stacksize, priority,
                                             CREATE_STACKTEST, _tisch_thread,
                                             (void *)dev, name);
    if (gnrc_tisch_scheduler_pid <= 0) {
        return -EINVAL;
    }
    return gnrc_tisch_scheduler_pid;
}
