/*
 * Copyright (C) 2017 INRIA
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     examples
 * @{
 *
 * @file
 * @brief       Example application for email functionality
 *
 * @author      Oliver Hahm <oliver.hahm@inria.fr> 
 *
 * @}
 */

#include "msg.h"
#include "shell.h"
#include "net/af.h"
#include "net/ipv6/addr.h"
#include "net/sock/tcp.h"
#include "net/smtp.h"

static char stack[THREAD_STACKSIZE_DEFAULT];
static msg_t queue[8];

static int cmd_sendmail(int argc, char **argv)
{
    smtp_sendmail(argv[1], argv[2]);
    return 0;
}

static int cmd_relay(int argc, char **argv)
{
    if (ipv6_addr_from_str((ipv6_addr_t *)&smtp_mx_relay.addr.ipv6, argv[1]) == NULL) {
        printf("error parsing IPv6 address\n");
        return 1;
    }
    return 0;
}

static const shell_command_t shell_commands[] = {
    { "mail", "send an email", cmd_sendmail },
    { "mx", "configures the default MX relay", cmd_relay },
    { NULL, NULL, NULL }
};

int main(void)
{
    puts("email example application\n");
    puts("Type 'help' to get started. Have a look at the README.md for more"
         "information.");

    smtp_mx_relay.family = AF_INET6;

    /* the main thread needs a msg queue to be able to run `ping6`*/
    msg_init_queue(queue, (sizeof(queue) / sizeof(msg_t)));

    /* start shell */
    char line_buf[SHELL_DEFAULT_BUFSIZE];
    shell_run(shell_commands, line_buf, SHELL_DEFAULT_BUFSIZE);

    /* should be never reached */
    return 0;
}
