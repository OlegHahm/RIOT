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

#define ENABLE_DEBUG    (1)
#include "debug.h"

sock_tcp_ep_t smtp_mx_relay = { AF_INET6, IPV6_ADDR_UNSPECIFIED, 0, SMTP_DEFAULT_PORT };

static int _smtp_cmd(sock_tcp_t *sock, const char *cmd, const size_t len, unsigned smtp_code);

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
    unsigned smtp_codes[] = {
        250,
        250,
        250,
        354,
        0,
        0,
        0
    };

    sock_tcp_t sock;

    if (sock_tcp_connect(&sock, &smtp_mx_relay, 0, 0) < 0) {
        DEBUG("Error connecting sock\n");
        return 1;
    }

    uint8_t buf[128];
    if (sock_tcp_read(&sock, buf, sizeof(buf), SOCK_NO_TIMEOUT) < 0) {
        DEBUG("Disconnected\n");
    }
    DEBUG("[smtp] MTA greets: %s\n", buf);

    for (unsigned i = 0; i < sizeof(commands) / sizeof(*commands); i++) {
        if (_smtp_cmd(&sock, commands[i], fmt_strlen(commands[i]), smtp_codes[i]) < 0) {
            return 1;
        }
    }
    _smtp_cmd(&sock, message, mlen, 0);
    _smtp_cmd(&sock, ".\n", 3, 0);
    _smtp_cmd(&sock, "QUIT\n", 6, 0);

    sock_tcp_disconnect(&sock);
    return 0;
}

int smtp_add_local_user(char *recipient, smtp_cb_t *cb)
{
    (void) recipient;
    (void) cb;
    return 0;
}

static int _smtp_cmd(sock_tcp_t *sock, const char *cmd, const size_t len, unsigned smtp_code)
{
    DEBUG("[smtp]: sending %s\n", cmd);
    uint8_t buf[128];
    int res;
    if (smtp_code && ((res = sock_tcp_write(sock, cmd, len)) < 0)) {
        DEBUG("Errored on write\n");
        return -1;
    }
    else {
        if ((res = sock_tcp_write(sock, "\n", sizeof("\n"))) < 0) {
            DEBUG("Errored on write\n");
            return -1;
        }
        if ((res = sock_tcp_read(sock, buf, sizeof(buf), SOCK_NO_TIMEOUT)) < 0) {
            DEBUG("Disconnected\n");
        }
        DEBUG("[smtp] Got response: %s\n", buf);
    }

    return 0;
}
