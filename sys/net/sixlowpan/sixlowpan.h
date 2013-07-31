/*
 * 6lowpan constants, data structs, and prototypes
 *
 * Copyright (C) 2013  INRIA.
 *
 * This file subject to the terms and conditions of the GNU Lesser General
 * Public License. See the file LICENSE in the top level directory for more
 * details.
 *
 * @ingroup sixlowpan
 * @{
 * @file    sixlowpan.h
 * @brief   6lowpan header 
 * @author  Stephan Zeisberg <zeisberg@mi.fu-berlin.de>
 * @author  Martin Lenders <mlenders@inf.fu-berlin.de>
 * @author  Oliver Gesch <oliver.gesch@googlemail.com>
 * @author  Eric Engel <eric.engel@fu-berlin.de>
 * @}
 */

#ifndef SIXLOWPAN_H
#define SIXLOWPAN_H

#ifdef NATIVE_STACK
#define NATIVE_STACK_FACTOR (10240)
#else
#define NATIVE_STACK_FACTOR (1)
#endif

#define IP_PROCESS_STACKSIZE           	(3072*NATIVE_STACK_FACTOR)
#define NC_STACKSIZE                   	(512*NATIVE_STACK_FACTOR)
#define CON_STACKSIZE                  	(512*NATIVE_STACK_FACTOR)
#define LOWPAN_TRANSFER_BUF_STACKSIZE  	(512*NATIVE_STACK_FACTOR)

/* fragment size in bytes*/
#define FRAG_PART_ONE_HDR_LEN  	(4)
#define FRAG_PART_N_HDR_LEN    	(5)

#define SIXLOWPAN_MAX_REGISTERED     (4)

#define LOWPAN_IPHC_DISPATCH   	(0x60)
#define LOWPAN_IPHC_FL_C       	(0x10)
#define LOWPAN_IPHC_TC_C       	(0x08)
#define LOWPAN_IPHC_CID        	(0x80)
#define LOWPAN_IPHC_SAC        	(0x40)
#define LOWPAN_IPHC_SAM        	(0x30)
#define LOWPAN_IPHC_DAC        	(0x04)
#define LOWPAN_IPHC_DAM        	(0x03)
#define LOWPAN_IPHC_M          	(0x08)
#define LOWPAN_IPHC_NH         	(0x04)
#define LOWPAN_IPV6_DISPATCH   	(0x41)
#define LOWPAN_CONTEXT_MAX     	(16)

#define LOWPAN_REAS_BUF_TIMEOUT (15 * 1000 * 1000) /* TODO: Set back to 3 * 1000 *	(1000) */

/* icmp message types rfc4443 */
#define ICMP_PARA_PROB                 	(4)
#define ICMP_ECHO_REQ                   (128)
#define ICMP_ECHO_REPL                  (129)
/* icmp message types rfc4861 4.*/
#define ICMP_RTR_ADV                   	(134)
#define ICMP_RTR_SOL                   	(133)
#define ICMP_NBR_ADV                   	(136)
#define ICMP_NBR_SOL                   	(135)
#define ICMP_REDIRECT                  	(137)	/* will be filtered out by the border router */
#define ICMP_RPL_CONTROL                (155)

#include "transceiver.h"
#include "sixlowip.h"
#include "vtimer.h"
#include "mutex.h"

extern mutex_t lowpan_context_mutex;
extern uint16_t local_address;

typedef struct {
    uint8_t num;
    ipv6_addr_t prefix;
    uint8_t length;
    uint8_t comp;
    uint16_t lifetime;
} lowpan_context_t;

typedef struct lowpan_interval_list_t {
    uint8_t                         start;
    uint8_t                         end;
    struct lowpan_interval_list_t   *next;
} lowpan_interval_list_t;

typedef struct lowpan_reas_buf_t {
    /* Source Address */
    ieee_802154_long_t       s_laddr;
    /* Destination Address */
    ieee_802154_long_t       d_laddr;
    /* Identification Number */
    uint16_t                 ident_no;
    /* Timestamp of last packet fragment */
    long                     timestamp;
    /* Size of reassembled packet with possible IPHC header */
    uint16_t                 packet_size;
    /* Additive size of currently already received fragments */
    uint16_t                 current_packet_size;
    /* Pointer to allocated memory for reassembled packet + 6LoWPAN Dispatch Byte */
    uint8_t                  *packet;
    /* Pointer to list of intervals of received packet fragments (if any) */
    lowpan_interval_list_t   *interval_list_head;
    /* Pointer to next reassembly buffer (if any) */
    struct lowpan_reas_buf_t *next;
} lowpan_reas_buf_t;

typedef struct {
    uint16_t length;
    uint8_t *data;
} lowpan_datagram_t;

extern lowpan_reas_buf_t *head;

typedef enum {
    LOWPAN_IPHC_DISABLE = 0,
    LOWPAN_IPHC_ENABLE = 1
} lowpan_iphc_status_t;

/**
 * @brief   Initializes 6lowpan
 *
 * @param[in] trans     transceiver to use with 6lowpan
 * @param[in] r_addr    phy layer address
 * @param[in] as_border 1 if node shoud act as border router, 0 otherwise
 */
void sixlowpan_init(transceiver_type_t trans, uint8_t r_addr, int as_border);

/**
 * @brief Initializes a 6lowpan router with address prefix
 *
 * @param[in] trans     transceiver to use with 6lowpan
 * @param[in] prefix    the address prefix to advertise
 * @param[in] r_addr    phy layer address
 */
void sixlowpan_adhoc_init(transceiver_type_t trans, ipv6_addr_t *prefix, 
                          uint8_t r_addr);

/**
 * @brief Initialize datagram and deliver it to the MAC layer
 *
 * @param[in] addr      IEEE802.15.4 desgination address (long address)
 * @param[in] data      Pointer to IP datagram
 * @param[in] len       Length of IP datagram (including header)
 **/
void lowpan_init(ieee_802154_long_t *addr, uint8_t *data, uint16_t len);
void lowpan_set_iphc_status(lowpan_iphc_status_t status);
void lowpan_read(uint8_t *data, uint8_t length, ieee_802154_long_t *s_laddr,
                 ieee_802154_long_t *d_laddr);
void lowpan_iphc_encoding(ieee_802154_long_t *dest, ipv6_hdr_t *ipv6_buf_extra, uint8_t *ptr, uint16_t *comp_len);

/**
 * @brief Decode a header compressed datagram
 *
 * @param[in] data              Pointer to the datagram
 * @param[in] length            Length of the datagram
 * @param[in] s_laddr           IEEE802.15.4 source address (long address)
 * @param[in] d_laddr           IEEE802.15.4 destination address (long address)
 * @param[out] decomp_length    Packet length of the uncompressed packet
 */
void lowpan_iphc_decoding(uint8_t *data, uint8_t length,
                          ieee_802154_long_t *s_laddr,
                          ieee_802154_long_t *d_laddr,
                          uint16_t *decomp_length);
uint8_t lowpan_context_len(void);
void add_fifo_packet(lowpan_reas_buf_t *current_packet);
lowpan_context_t *lowpan_context_update(
    uint8_t num, const ipv6_addr_t *prefix,
    uint8_t length, uint8_t comp,
    uint16_t lifetime);
lowpan_context_t *lowpan_context_get(void);
lowpan_context_t *lowpan_context_lookup(ipv6_addr_t *addr);
lowpan_context_t *lowpan_context_num_lookup(uint8_t num);
lowpan_reas_buf_t *collect_garbage_fifo(lowpan_reas_buf_t *current_buf);
lowpan_reas_buf_t *collect_garbage(lowpan_reas_buf_t *current_buf);
void check_timeout(void);
void lowpan_ipv6_set_dispatch(uint8_t *data, uint16_t packet_length);
void init_reas_bufs(lowpan_reas_buf_t *buf);
void printReasBuffers(void);
void printFIFOBuffers(void);
#endif
