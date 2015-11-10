/*
 * Copyright (C) 2013-15, Christian Tschudin, University of Basel
 * Copyright (C) 2015, Oliver Hahm, Inria
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * File history:
 * 2015-11-09  created (based on ccn-lite-peek.c)
 */

#define USE_SUITE_CCNB
#define USE_SUITE_CCNTLV
#define USE_SUITE_CISTLV
#define USE_SUITE_IOTTLV
#define USE_SUITE_NDNTLV

#define USE_FRAG
#define NEEDS_PACKET_CRAFTING

#include <unistd.h>
#include "sys/socket.h"
#include "arpa/inet.h"
#include "ccn-lite-riot.h"
#include "ccnl-core.h"
#include "ccnl-headers.h"
#include "ccnl-pkt-ndntlv.h"
#include "ccnl-defs.h"
#include "ccnl-ext.h"

#include "ccnl-pkt-ccntlv.h"
#include "ccnl-pkt-iottlv.h"

#define ENABLE_DEBUG    (1)
#include "debug.h"

typedef int (*ccnl_mkInterestFunc)(struct ccnl_prefix_s*, int*, unsigned char*, int);
typedef int (*ccnl_isContentFunc)(unsigned char*, int);
typedef int (*ccnl_isFragmentFunc)(unsigned char*, int);

extern ccnl_mkInterestFunc ccnl_suite2mkInterestFunc(int suite);
extern ccnl_isContentFunc ccnl_suite2isContentFunc(int suite);
extern ccnl_isFragmentFunc ccnl_suite2isFragmentFunc(int suite);

extern int ccnl_switch_dehead(unsigned char **buf, int *len, int *code);
extern int ccnl_enc2suite(int enc);
extern int ccnl_iottlv_dehead(unsigned char **buf, int *len, unsigned int *typ, int *vallen);

unsigned char out[8 * CCNL_MAX_PACKET_SIZE];
int outlen;

int frag_cb(struct ccnl_relay_s *relay, struct ccnl_face_s *from,
            unsigned char * *data, int *len)
{
    (void) relay;
    (void) from;
    DEBUG("frag_cb\n");

    memcpy(out, *data, *len);
    outlen = *len;
    return 0;
}

static void _usage(char *arg)
{
    printf("usage: %s URI\n"
            "%% %s /ndn/edu/wustl/ping             (classic lookup)\n",
            arg, arg);
}

/* set pid for using a netif interface, sun for using a socket */
int send_interest(kernel_pid_t pid, int *sock, int suite, int argc, char **argv)
{
    struct ccnl_prefix_s *prefix;
    struct sockaddr sa;
    unsigned int chunknum = UINT_MAX;
    ccnl_mkInterestFunc mkInterest;
    ccnl_isContentFunc isContent;
    ccnl_isFragmentFunc isFragment;

    if (argc != 2) {
        _usage(argv[0]);
    }

    /* TODO: initialize random numbers */

    mkInterest = ccnl_suite2mkInterestFunc(suite);
    isContent = ccnl_suite2isContentFunc(suite);
    isFragment = ccnl_suite2isFragmentFunc(suite);

    if (!mkInterest || !isContent) {
        puts("No functions for this suite were found!");
        return(-1);
    }

    if (pid != KERNEL_PID_UNDEF) {
    }
    else if (sock != NULL) {
        puts("Not yet implemented");
    }

    prefix = ccnl_URItoPrefix(argv[1], suite, NULL, chunknum == UINT_MAX ? NULL : &chunknum);

    DEBUG(ccnl_prefix_to_path(prefix));

    for (int cnt = 0; cnt < 3; cnt++) {
        int nonce = random();
        int rc;
        struct ccnl_face_s dummyFace;

        DEBUG("cnt: %i\n", cnt);

        memset(&dummyFace, 0, sizeof(dummyFace));

        int len = mkInterest(prefix, &nonce, out, sizeof(out));

        DEBUG("len: %i\n", len);
        if (pid != KERNEL_PID_UNDEF) {
            /* TODO */
            /* TODO */
            /* TODO: sending interest over netif/netapi*/
            /* TODO */
            /* TODO */
        }
        else if (sock != NULL) {
            size_t socksize = sizeof(struct sockaddr_in);
            rc = sendto(*sock, out, len, 0, (struct sockaddr *)&sa, socksize);
        }
        if (rc < 0) {
            puts("Error while sending");
            return 0;
        }
        DEBUG("rc: %i\n", rc);

        while (1) { /* wait for a content pkt (ignore interests) */
            unsigned char *cp = out;
            int enc, suite2, len2;
            DEBUG("  waiting for packet\n");

            /* TODO: handle timeout */
            /* TODO: receive from socket or interface */
            len = recv(*sock, out, sizeof(out), 0);

            DEBUG("len: %i\n", len);
            suite2 = -1;
            len2 = len;
            while (!ccnl_switch_dehead(&cp, &len2, &enc)) { 
                suite2 = ccnl_enc2suite(enc);
            }
            if (suite2 != -1 && suite2 != suite) {
                DEBUG("suite: %i\n", suite);
                continue;
            }

#ifdef USE_FRAG
            if (isFragment && isFragment(cp, len2)) {
                unsigned int t;
                unsigned int len3;
                DEBUG("len2: %i\n", len2);
                switch (suite) {
                    case CCNL_SUITE_CCNTLV: {
                                                struct ccnx_tlvhdr_ccnx2015_s *hp;
                                                hp = (struct ccnx_tlvhdr_ccnx2015_s *) out;
                                                cp = out + sizeof(*hp);
                                                len2 -= sizeof(*hp);
                                                if (ccnl_ccntlv_dehead(&cp, &len2, &t, &len3) < 0 ||
                                                        t != CCNX_TLV_TL_Fragment) {
                                                    DEBUG("  error parsing fragment\n");
                                                    continue;
                                                }
                                                rc = ccnl_frag_RX_BeginEnd2015(frag_cb, NULL, &dummyFace,
                                                        4096, hp->fill[0] >> 6,
                                                        ntohs(*(uint16_t *) hp->fill) & 0x03fff,
                                                        &cp, (int *) &len3);
                                                break;
                                            }
                    case CCNL_SUITE_IOTTLV: {
                                                uint16_t tmp;

                                                if (ccnl_iottlv_dehead(&cp, &len2, &t, (int*) &len3)) { // IOT_TLV_Fragment
                                                    DEBUG("problem parsing fragment\n");
                                                    continue;
                                                }
                                                DEBUG("len3: %u\n", len3);
                                                if (t == IOT_TLV_F_OptFragHdr) { // skip it for the time being
                                                    cp += len3;
                                                    len2 -= len3;
                                                    if (ccnl_iottlv_dehead(&cp, &len2, &t, (int*) &len3)) {
                                                        continue;
                                                    }
                                                }
                                                if (t != IOT_TLV_F_FlagsAndSeq || len3 < 2) {
                                                    DEBUG("t: %i\n", t);
                                                    continue;
                                                }
                                                tmp = ntohs(*(uint16_t *) cp);
                                                cp += len3;
                                                len2 -= len3;

                                                if (ccnl_iottlv_dehead(&cp, &len2, &t, (int*) &len3)) {
                                                    DEBUG("  cannot parse frag payload\n");
                                                    continue;
                                                }
                                                DEBUG("len3: %i\n", len3);
                                                if (t != IOT_TLV_F_Data) {
                                                    DEBUG("t: %i\n", t);
                                                    continue;
                                                }
                                                rc = ccnl_frag_RX_BeginEnd2015(frag_cb, NULL, &dummyFace,
                                                        4096, tmp >> 14, tmp & 0x3fff, &cp, (int *) &len3);
                                                puts("--");
                                                break;
                                            }
                    default:
                                            continue;
                }
                if (!outlen) {
                    continue;
                }
                len = outlen;
            }
#endif

            rc = isContent(out, len);
            if (rc < 0) {
                DEBUG("error when checking type of packet\n");
                goto done;
            }
            if (rc == 0) { /* it's an interest, ignore it */
                DEBUG("skipping non-data packet\n");
                continue;
            }
            puts("Something happened");
        }
        if (cnt < 2) {
            DEBUG("re-sending interest\n");
        }
    }
    puts("timeout");

done:
    if (sock != NULL) {
        close(*sock);
    }
    return 0;
}
