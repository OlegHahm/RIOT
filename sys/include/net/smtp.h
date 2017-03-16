/*
 * Copyright (C) 2017 INRIA 
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @defgroup    net_smtp SMTP library 
 * @ingroup     net
 * @brief       Provides SMTP server and client implementation 
 *
 * @{
 * @file
 * @brief       SMTP interface 
 *
 * @author      Oliver Hahm <oliver.hahm@inria.fr> 
 */
 
#include "net/sock/tcp.h"

#ifndef SMTP_H
#define SMTP_H

#ifndef SMTP_MAX_USERS
#   define SMTP_MAX_USERS  (3)
#endif

#ifndef SMTP_DEFAULT_HOSTNAME
#   define SMTP_DEFAULT_HOSTNAME "node.riot-os.org"
#endif

#ifndef SMTP_DEFAULT_USER
#   define SMTP_DEFAULT_USER "ritos"
#endif

#define SMTP_DEFAULT_PORT   (25)

/**
 * @brief The default MX relay
 */
extern sock_tcp_ep_t smtp_mx_relay;

/**
 * @brief   Signature for callbacks fired when a mail for a certain user is received
 *
 * @param[in] subject       Subject of the email
 * @param[in] slen          Length of the subject
 * @param[in] message       The email's body
 * @param[in] mlen          Length of the body 
 */
typedef void(*smtp_cb_t)(const char *subject, size_t slen, char *message,
                         size_t mlen);

/**
 * @brief   Initializes and start a local SMTP server
 *
 * @param[in] s TCP sock endpoint the server should listen at
 */
void smtp_server_init(sock_tcp_ep_t *s);

/**
 * @brief   Sends a mail
 *
 * @param[in] recipient     The recipient's email address
 * @param[in] rlen          Length of the recipient's address 
 * @param[in] subject       Subject of the email
 * @param[in] slen          Length of the subject
 * @param[in] message       The email's body
 * @param[in] mlen          Length of the body 
 *
 * @return      0 on success, a negative errno value otherwise 
 */
int smtp_sendmail(char *recipient, size_t rlen, char *subject, size_t slen,
                  char *message, size_t mlen);

/**
 * @brief   Adds local email recipient
 *
 * @param[in] recipient     The email address of the local user
 * @param[in] cb            The callback to be fired when an email for the
 *                          added user is received
                            *
 * @return      0 on success, a negative errno value otherwise 
 */
int smtp_add_local_user(char *recipient, smtp_cb_t *cb);

#endif /* SMTP_H */
