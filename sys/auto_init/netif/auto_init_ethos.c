/*
 * Copyright (C) 2015 Kaspar Schleiser <kaspar@schleiser.de>
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 *
 */

/**
 * @ingroup auto_init_gnrc_netif
 * @{
 *
 * @file
 * @brief   Auto initialization for ethernet-over-serial module
 *
 * @author  Kaspar Schleiser <kaspar@schleiser.de>
 */

#ifdef MODULE_ETHOS

#include "ethos.h"
#include "periph/uart.h"
#include "net/gnrc/netdev2/eth.h"

#define ENABLE_DEBUG (0)
#include "debug.h"

/**
 * @brief global ethos object, used by uart_stdio
 */
ethos_t ethos;

/**
 * @brief   Define stack parameters for the MAC layer thread
 * @{
 */
#define MAC_STACKSIZE           (THREAD_STACKSIZE_DEFAULT + DEBUG_EXTRA_STACKSIZE)
#define MAC_PRIO                (THREAD_PRIORITY_MAIN - 4)

/**
 * @brief   Stacks for the MAC layer threads
 */
static char _netdev2_eth_stack[MAC_STACKSIZE];
static gnrc_netdev2_t _gnrc_ethos;

static uint8_t _inbuf[2048];

void auto_init_ethos(void)
{
    DEBUG("auto_init_ethos(): initializing device...\n");

    /* setup netdev2 device */
    ethos_setup(&ethos, ETHOS_UART,
            ETHOS_BAUDRATE, _inbuf, sizeof(_inbuf));

#ifdef MODULE_GNRC
    /* initialize netdev2<->gnrc adapter state */
    gnrc_netdev2_eth_init(&_gnrc_ethos, (netdev2_t*)&ethos);

    /* start gnrc netdev2 thread */
    gnrc_netdev2_init(_netdev2_eth_stack, MAC_STACKSIZE,
            MAC_PRIO, "gnrc_ethos", &_gnrc_ethos);
#else
    netdev2_t *netdev = (netdev2_t *)&ethos;
    netdev->driver->init(netdev);
#endif
#ifdef MODULE_NETDEV_DEFAULT
    netdev_default = (netdev2_t *)&ethos;
#endif
}

#else
typedef int dont_be_pedantic;
#endif /* MODULE_ETHOS */
/** @} */
