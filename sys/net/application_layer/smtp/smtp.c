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
#include <string.h>

#include "fmt.h"
#include "net/smtp.h"
#include "net/ipv6/addr.h"
#include "net/sock.h"
#include "net/sock/tcp.h"

sock_tcp_ep_t smtp_mx_relay = { AF_INET6, IPV6_ADDR_UNSPECIFIED, 0, SMTP_DEFAULT_PORT };

static int _smtp_cmd(sock_tcp_t *s, const char *cmd, const size_t len);

void smtp_server_init(sock_tcp_ep_t *s)
{
    (void) s;
}

int smtp_sendmail(char *recipient, size_t rlen, char *subject, size_t slen,
                  char *message, size_t mlen)
{
    char rcpt_to_str[fmt_strlen("RCPT TO: <>\n") + rlen];
    size_t written = fmt_str(rcpt_to_str, "RCPT TO: <");
    written += fmt_str(rcpt_to_str + written, recipient);
    fmt_str(rcpt_to_str + written, ">\n");
    
    char hdr_to_str[fmt_strlen("To: <>\n") + rlen];
    written = fmt_str(hdr_to_str, "To: <");
    written += fmt_str(hdr_to_str + written, recipient);
    fmt_str(hdr_to_str + written, ">\n");
    
    char hdr_subject_str[fmt_strlen("Subject: \n") + slen];
    written = fmt_str(hdr_subject_str, "Subject: ");
    written += fmt_str(hdr_subject_str + written, subject);
    fmt_str(hdr_subject_str + written, "\n");
    
    char *commands[] = {
        "HELO " SMTP_HOSTNAME "\n",
        "MAIL FROM: <" SMTP_DEFAULT_USER "@" SMTP_HOSTNAME ">\n",
        rcpt_to_str,
        "DATA",
        "From: <" SMTP_DEFAULT_USER "@" SMTP_HOSTNAME ">\n",
        hdr_to_str,
        hdr_subject_str,
        ""
    };
    sock_tcp_t sock;

    if (sock_tcp_connect(&sock, &smtp_mx_relay, 0, 0) < 0) {
        puts("Error connecting sock");
        return 1;
    }


    for (unsigned i = 0; i < sizeof(commands) / sizeof(*commands); i++) {
        if (_smtp_cmd(&sock, commands[i], fmt_strlen(commands[i])) < 0) {
            return 1;
        }
    }
    _smtp_cmd(&sock, message, mlen);
    _smtp_cmd(&sock, ".\n", 3);

    sock_tcp_disconnect(&sock);
    return 0;
}

int smtp_add_local_user(char *recipient, smtp_cb_t *cb)
{
    (void) recipient;
    (void) cb;
    return 0;
}

static int _smtp_cmd(sock_tcp_t *sock, const char *cmd, const size_t len)
{
    uint8_t buf[128];
    int res;
    if (len && ((res = sock_tcp_write(sock, cmd, len)) < 0)) {
        puts("Errored on write");
        return -1;
    }
    else {
        if ((res = sock_tcp_write(sock, "\n", sizeof("\n"))) < 0) {
            puts("Errored on write");
            return -1;
        }
        if ((res = sock_tcp_read(sock, buf, sizeof(buf), SOCK_NO_TIMEOUT)) < 0) {
            puts("Disconnected");
        }
    }

    return 0;
}
