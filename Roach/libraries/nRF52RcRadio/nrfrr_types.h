#ifndef _NRFRR_TYPES_H_
#define _NRFRR_TYPES_H_

#include <stdint.h>
#include <stdbool.h>

#include "nrfrr_conf.h"
#include "nrfrr_defs.h"

typedef struct
{
    uint8_t s0;
    uint8_t len;
    uint8_t s1;

    // automatic encryption covers the following
    // checksum coverage is from here as well
    // I think there's room here for 254 bytes

    // metadata
    #ifdef NRFRR_FINGER_QUOTES_RANDOMNESS
    uint32_t rand;
    #endif
    uint32_t uid;
    uint32_t seq_num;
    uint32_t session;
    uint8_t  chan_hint; // what channel index was actually used to TX
    uint8_t  flags;     // contains things like a reply-request
    #ifdef NRFRR_ADAPTIVE_INTERVAL
    uint8_t  interval;  // transmission interval that the receiver can adapt the timeout
    #endif
    #ifdef NRFRR_BIDIRECTIONAL
    uint8_t  ack_seq;
    #endif

    // payload
    uint8_t payload[NRFRR_PAYLOAD_SIZE];
}
__attribute__ ((packed))
nrfrr_pkt_t;

enum
{
    NRFRR_SM_IDLE_WAIT,
    NRFRR_SM_IDLE,
    NRFRR_SM_TX_START,
    NRFRR_SM_TX_START_DELAY,
    NRFRR_SM_TX_WAIT,
    NRFRR_SM_RX_START,
    NRFRR_SM_RX,
    NRFRR_SM_RX_END_WAIT,
    NRFRR_SM_RX_END_WAIT2,
    NRFRR_SM_RX_END,
    NRFRR_SM_HOP_START,
    NRFRR_SM_HOP_WAIT,
    NRFRR_SM_CHAN_WAIT,
    NRFRR_SM_HALT_START,
    NRFRR_SM_HALT_WAIT,
    NRFRR_SM_HALTED,
    NRFRR_SM_CONT_TX_CARRIER,
    NRFRR_SM_CONT_TX_MOD,
};

enum
{
    NRFRR_FLAG_REPLYREQUEST,
    NRFRR_FLAG_REPLY,
    NRFRR_FLAG_TEXT,
    NRFRR_FLAG_PAIRING,
};

typedef struct
{
    uint32_t timestamp;
    uint32_t drate;
    uint32_t tx;
    uint32_t rx;
    uint32_t txt;
    uint32_t good;
    uint32_t calls;
    uint32_t loss;
    uint32_t seq_miss;
    uint32_t rx_miss;
}
radiostats_t;

typedef struct
{
    uint8_t  typecode;
    uint32_t addr;
    uint8_t  len;
    uint8_t  data[NRFRR_PAYLOAD_SIZE2];
}
__attribute__ ((packed))
radio_binpkt_t;

#endif
