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

#define EMAIL_MAX_MSG_SIZE  (512)

static msg_t queue[8];

static int _readline(char *buf, size_t size)
{
    char *line_buf_ptr = buf;

    while (1) {
        if ((line_buf_ptr - buf) >= ((int) size) - 1) {
            return -1;
        }

        int c = getchar();
        if (c < 0) {
            return 1;
        }

        /* We allow Unix linebreaks (\n), DOS linebreaks (\r\n), and Mac linebreaks (\r). */
        /* QEMU transmits only a single '\r' == 13 on hitting enter ("-serial stdio"). */
        /* DOS newlines are handled like hitting enter twice, but empty lines are ignored. */
        if (c == '\r' || c == '\n') {
            *line_buf_ptr = '\0';
#ifndef SHELL_NO_ECHO
            putchar('\r');
            putchar('\n');
#endif

            /* return 1 if line is empty, 0 otherwise */
            return line_buf_ptr == buf;
        }
        /* QEMU uses 0x7f (DEL) as backspace, while 0x08 (BS) is for most terminals */
        else if (c == 0x08 || c == 0x7f) {
            if (line_buf_ptr == buf) {
                /* The line is empty. */
                continue;
            }

            *--line_buf_ptr = '\0';
            /* white-tape the character */
#ifndef SHELL_NO_ECHO
            putchar('\b');
            putchar(' ');
            putchar('\b');
#endif
        }
        else {
            *line_buf_ptr++ = c;
#ifndef SHELL_NO_ECHO
            putchar(c);
#endif
        }
    }
}

static int cmd_sendmail(int argc, char **argv)
{
    char mail_buf[EMAIL_MAX_MSG_SIZE];
    if (argc < 3) {
        return -EINVAL;
    }
    printf("Sending mail to %s,\n, subject is %s\n", argv[1], argv[2]);
    puts("Please enter message text and enter with an empty line.");
    int cnt = 0;
    char *buf = mail_buf;
    while (!cnt) {
        cnt = _readline(buf, EMAIL_MAX_MSG_SIZE);
        if (cnt < 0) {
            puts("Error");
            return -EIO;
        }
        buf += cnt;
    }
    smtp_sendmail(argv[1], strlen(argv[1]), argv[2], strlen(argv[2]), mail_buf, cnt);
    return 0;
}

static int cmd_relay(int argc, char **argv)
{
    (void) argc;
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
