#ifndef CCN_LITE_RIOT_H
#define CCN_LITE_RIOT_H

#define CCNL_RIOT

#undef USE_NFN

#define USE_IPV6
#define USE_SUITE_NDNTLV
#define NEEDS_PREFIX_MATCHING

#include "ccnl-defs.h"
#include "ccnl-core.h"
#include "net/gnrc/netreg.h"

#define ccnl_malloc(s)                  malloc(s)
#define ccnl_calloc(n,s)                calloc(n,s)
#define ccnl_realloc(p,s)               realloc(p,s)
#define ccnl_free(p)                    free(p)

void ccnl_event_loop(struct ccnl_relay_s *ccnl);
void ccnl_minimalrelay_ageing(void *relay, void *aux);
int ccnl_open_netif(kernel_pid_t if_pid, gnrc_nettype_t netreg_type);

#endif /* CCN_LITE_RIOT_H */
