#ifndef _NRFRR_CONF_H_
#define _NRFRR_CONF_H_

#include <stdint.h>
#include <stdbool.h>

#define RH_NRF51_MAX_PAYLOAD_LEN 254
#define NRFRR_PAYLOAD_SIZE   (64 * 1)
#define NRFRR_PAYLOAD_SIZE2  (64 * 3)
// we want this to be long enough for strings, but short enough to fit, also shorter sends faster
// doc: Independent of the configuration of MAXLEN, the combined length of S0, LENGTH, S1 and PAYLOAD cannot exceed 258 bytes.

#define NRFRR_TX_INTERV_DEF  10
// maximum transmission time for a 258 byte packet is 2.064 milliseconds if sent at at 1 Mbit/s

#define NRFRR_TX_INTERV_MAX  100
#define NRFRR_TX_INTERV_MIN  5
#define NRFRR_TX_MIN_TIME    2
#define NRFRR_TX_RETRANS_TXT 2
#define NRFRR_CHAN_MAP_DEFAULT        0x0FFFFFFF
#define NRFRR_CHAN_MAP_DEFAULT_SINGLE 0x02

#define NRFRR_REM_USED_SESSIONS 32

#define NRFRR_FINGER_QUOTES_RANDOMNESS
#define NRFRR_USE_NRF_RNG
//#define NRFRR_USE_MANUAL_CRC

//#define NRFRR_DEBUG_PINS
//#define NRFRR_DEBUG_PIN_TX 27
//#define NRFRR_DEBUG_PIN_RX 14
//#define NRFRR_DEBUG_PIN_CH 15

#define NRFRR_BIDIRECTIONAL
#define NRFRR_ADAPTIVE_INTERVAL
//#define NRFRR_USE_INTERRUPTS

//#define NRFRR_USE_FREQ_LOWER
#define NRFRR_USE_FREQ_NORTHAMERICAN
//#define NRFRR_USE_FREQ_EUROPEAN
//#define NRFRR_USE_FREQ_UPPER
//#define NRFRR_USE_FREQ_LOWER_ONLY
//#define NRFRR_USE_FREQ_FULL_LEGAL

//#define NRFRR_DEBUG_HOPTABLE
//#define NRFRR_DEBUG_TX
//#define NRFRR_DEBUG_RX
//#define NRFRR_DEBUG_HOP
#define NRFRR_DEBUG_RX_ERRSTATS

#endif
