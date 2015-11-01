#include <stdio.h>

#include "msg.h"
#include "timex.h"
#include "shell_commands.h"
#include "sys/socket.h"
#include "arpa/inet.h"
#include "ccn-lite-riot.h"
#include "ccnl-core.h"
#include "ccnl-headers.h"
#include "ccnl-pkt-ndntlv.h"
#include "ccnl-defs.h"
#include "net/gnrc/nettype.h"

#define MAIN_QUEUE_SIZE     (8)
static msg_t _main_msg_queue[MAIN_QUEUE_SIZE];
//static char addr_str[IPV6_ADDR_MAX_STR_LEN];

struct ccnl_if_s *i;

extern struct ccnl_relay_s theRelay;

extern int ccnl_open_udpdev(int port);

static int _ccnl_fwd(int argc, char **argv);

static const shell_command_t shell_commands[] = {
    { "ccnl", "start a NDN forwarder", _ccnl_fwd},
    { NULL, NULL, NULL }
};

static void _usage(void)
{
    puts("ccnl <transport> { <interface> | <defaultgw> <UDP port> }");
}

static int _open_udp_socket(char **argv)
{
    sockunion sun;
    int udpport = NDN_UDP_PORT;
    if (i->sock < 0) {
        puts("Something went wrong opening the UDP port");
        return(-1);
    }

    if (inet_pton(AF_INET6, argv[2], &sun.ip6.sin6_addr) != 1) {
        _usage();
        return -1;
    }
    sun.ip6.sin6_port = atoi(argv[3]);
    i->sock = ccnl_open_udpdev(udpport);

    return 0;
}

static int _ccnl_fwd(int argc, char **argv)
{
    struct ccnl_if_s *i;
    i = &theRelay.ifs[theRelay.ifcount];

    if (argc < 3) {
        _usage();
        return -1;
    }

    switch (argv[1][0]) {
        case 'u':
            if (argc < 4) {
                _usage();
                return -1;
            }
            if (_open_udp_socket(argv) < 0) {
                puts("Error opening UDP socket!");
                return -1;
            }
            theRelay.ifcount++;
            break;
        case 'i': {
            int pid = atoi(argv[2]);
            i->if_pid = pid;
            if (ccnl_open_netif(pid, GNRC_NETTYPE_CCN) < 0) {
                puts("Error registering at network interface!");
                i->if_pid = KERNEL_PID_UNDEF;
            }
            theRelay.ifcount++;
            break;
            }
        default:
            _usage();
            puts("Transport must be either u (UDP) or i (network interface)");
            return -1;
    }

    ccnl_set_timer(SEC_IN_USEC, ccnl_minimalrelay_ageing, &theRelay, 0);
    ccnl_event_loop(&theRelay);

    return 0;
}


int main(void)
{
    msg_init_queue(_main_msg_queue, MAIN_QUEUE_SIZE);

    puts("Basic CCN-Lite example");

    ccnl_core_init();

    /* TODO: use gnrc_netif_get() to get a device for sending */

    i = &theRelay.ifs[0];
    i->mtu = NDN_DEFAULT_MTU;
    i->fwdalli = 1;
    theRelay.ifcount++;

    char line_buf[SHELL_DEFAULT_BUFSIZE];
    shell_run(shell_commands, line_buf, SHELL_DEFAULT_BUFSIZE);
    return 0;
}
