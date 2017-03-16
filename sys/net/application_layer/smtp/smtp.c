/*
 * Copyright (C) 2017 INRIA 
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     net_smtp
 * @{
 *
 * @file
 * @brief       SMTP implementation
 *
 * @author      Oliver Hahm <oliver.hahm@inria.fr> 
 *
 * @}
 */

#include <stddef.h>

#include "net/smtp.h"
#include "net/ipv6/addr.h"
#include "net/sock.h"
#include "net/sock/tcp.h"

sock_tcp_ep_t smtp_mx_relay = { AF_INET6, IPV6_ADDR_UNSPECIFIED, 0, SMTP_DEFAULT_PORT };

static int _smtp_cmd(sock_tcp_t *s, char *cmd, size_t len);

void smtp_server_init(sock_tcp_ep_t *s)
{
    (void) s;
}

int smtp_sendmail(char *recipient, size_t rlen, char *subject, size_t slen,
                  char *message, size_t mlen)
{
    sock_tcp_t sock;

    if (sock_tcp_connect(&sock, &smtp_mx_relay, 0, 0) < 0) {
        puts("Error connecting sock");
        return 1;
    }
    _smtp_cmd(&sock, "HELO" SMTP_DEFAULT_HOSTNAME "\n", sizeof("HELO\n") + strlen(SMTP_DEFAULT_HOSTNAME));
    _smtp_cmd(&sock, "MAIL FROM: <" SMTP_DEFAULT_USER"@" SMTP_DEFAULT_HOSTNAME ">\n",
              sizeof("MAIL FROM: <@>\n") + sizeof(SMTP_DEFAULT_USER) + sizeof(SMTP_DEFAULT_HOSTNAME));
    char rcpt_to_str[sizeof("RCPT TO: \n") + rlen];
    sprintf(rcpt_to_str, "RCPT TO: <%s>\n", recipient);
    _smtp_cmd(&sock, rcpt_to_str, sizeof(rcpt_to_str));
    sock_tcp_disconnect(&sock);
    return 0;
}

int smtp_add_local_user(char *recipient, smtp_cb_t *cb)
{
    (void) recipient;
    (void) cb;
    return 0;
}

static int _smtp_cmd(sock_tcp_t *s, char *cmd, size_t len)
{
    uint8_t buf[128];
    if ((res = sock_tcp_write(&sock, )) < 0) {
        puts("Errored on write");
    }
    else {
        if ((res = sock_tcp_read(&sock, buf, sizeof(buf),
                                 SOCK_NO_TIMEOUT)) < 0) {
            puts("Disconnected");
        }
        printf("Read: \"");
        for (int i = 0; i < res; i++) {
            printf("%c", buf[i]);
        }
        puts("\"");
    }

    return 0;
}
