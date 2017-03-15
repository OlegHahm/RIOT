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

sock_tcp_ep_t smtp_mx_relay;

void smtp_server_init(sock_tcp_ep_t *s)
{
}

int smtp_sendmail(char *recipient, char *subject, size_t slen,
                       char *message, size_t mlen)
{
    int res;
    sock_tcp_ep_t remote = SOCK_IPV6_EP_ANY;
    remote.port = 12345;
    ipv6_addr_from_str((ipv6_addr_t *)&remote.addr,
                       "fe80::d8fa:55ff:fedf:4523");
    return 0;
}

void smtp_set_relay(sock_tcp_ep_t *relay)
{
}

int smtp_add_local_user(char *recipient, smtp_cb_t *cb)
{
    return 0;
}

#endif /* SMTP_H */
