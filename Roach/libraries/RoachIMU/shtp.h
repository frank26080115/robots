/*
 * Copyright 2015-21 CEVA, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License and 
 * any applicable agreements you may have with CEVA, Inc.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * Sensor Hub Transport Protocol (SHTP) API
 */

#ifndef SHTP_H
#define SHTP_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

#include "sh2_hal.h"

#define SHTP_INSTANCES (1)  // Number of SHTP devices supported
#define SHTP_MAX_CHANS (8)  // Max channels per SHTP device
#define SHTP_HDR_LEN (4)

typedef enum shtp_Event_e {
    SHTP_SHORT_FRAGMENT = 1,
    SHTP_TOO_LARGE_PAYLOADS = 2,
    SHTP_BAD_RX_CHAN = 3,
    SHTP_BAD_TX_CHAN = 4,
    SHTP_BAD_FRAGMENT = 5,
    SHTP_BAD_SN = 6,
    SHTP_INTERRUPTED_PAYLOAD = 7,
} shtp_Event_t;

typedef void shtp_Callback_t(void * cookie, uint8_t *payload, uint16_t len, uint32_t timestamp);
typedef void shtp_EventCallback_t(void *cookie, shtp_Event_t shtpEvent);

typedef struct shtp_Channel_s {
    uint8_t nextOutSeq;
    uint8_t nextInSeq;
    shtp_Callback_t *callback;
    void *cookie;
} shtp_Channel_t;

// Per-instance data for SHTP
typedef struct shtp_s {
    // Associated SHTP HAL
    // If 0, this indicates the SHTP instance is available for new opens
    sh2_Hal_t *pHal;

    // Asynchronous Event callback and it's cookie
    shtp_EventCallback_t *eventCallback;
    void * eventCookie;

    // Transmit support
    uint8_t outTransfer[SH2_HAL_MAX_TRANSFER_OUT];

    // Receive support
    uint16_t inRemaining;
    uint8_t  inChan;
    uint8_t  inPayload[SH2_HAL_MAX_PAYLOAD_IN];
    uint16_t inCursor;
    uint32_t inTimestamp;
    uint8_t inTransfer[SH2_HAL_MAX_TRANSFER_IN];

    // SHTP Channels
    shtp_Channel_t      chan[SHTP_MAX_CHANS];

    // Stats
    uint32_t rxBadChan;
    uint32_t rxShortFragments;
    uint32_t rxTooLargePayloads;
    uint32_t rxInterruptedPayloads;
    
    uint32_t badTxChan;
    uint32_t txDiscards;
    uint32_t txTooLargePayloads;

} shtp_t;

// Open the SHTP communications session.
// Takes a pointer to a HAL, which will be opened by this function.
// Returns a pointer referencing the open SHTP session.  (Pass this as pInstance to later calls.)
void * shtp_open(sh2_Hal_t *pHal);

// Closes and SHTP session.
// The associated HAL will be closed.
void shtp_close(void *pShtp);

// Set the callback function for reporting SHTP asynchronous events
void shtp_setEventCallback(void *pInstance,
                           shtp_EventCallback_t * eventCallback, 
                           void *eventCookie);

// Register a listener for an SHTP channel
int shtp_listenChan(void *pShtp,
                    uint8_t channel,
                    shtp_Callback_t *callback, void * cookie);

// Send an SHTP payload on a particular channel
int shtp_send(void *pShtp,
              uint8_t channel, const uint8_t *payload, uint16_t len);

// Check for received data and process it.
void shtp_service(void *pShtp);

#ifdef __cplusplus
}
#endif

// #ifdef SHTP_H
#endif
