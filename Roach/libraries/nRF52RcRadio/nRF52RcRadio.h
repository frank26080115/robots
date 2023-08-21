#ifndef _NRF52RAWRADIO_H_
#define _NRF52RAWRADIO_H_

#include <Arduino.h>

#include <RoachLib.h>

#include <stdint.h>
#include <stdbool.h>

#include "nrfrr_conf.h"
#include "nrfrr_defs.h"
#include "nrfrr_types.h"

class nRF52RcRadio
{
    public:
        nRF52RcRadio  (bool is_tx); // constructor, selects if the role is transmitter or receiver
        void begin    (int fem_tx = -1, int fem_rx = -1);      // init hardware
        void config   (uint32_t chan_map, uint32_t uid, uint32_t salt); // provide channel map as a bitmask, unique identifier for pairing, and security salt
        void send     (uint8_t* data);                         // queue data to be sent
        int  available(void);                                  // check if new packet has been received
        int  read     (uint8_t* data);                         // copy the new packet to a buffer
        void task     (void);                                  // periodic task, call from loop()
        bool state_machine_run(uint32_t t, bool is_isr);       // innards of task(), but can be called from ISR
        void start_new_session(void);                          // randomly generate new session ID
        int  textAvail(void);                                  // check if text/bin message is available
        int  textRead(const char* buf, bool clr);              // read available text/bin message
        int  textReadBin(radio_binpkt_t* buf, bool clr);       // read available text/bin message
        void textSend(const char* buf);                        // send text message
        void textSendBin(radio_binpkt_t* pkt);                 // send binary message
        void textSendByte(uint8_t x);                          // send binary message, but only single byte
        radio_binpkt_t* textReadPtr(bool clr);                 // read the message in-place
        bool textIsDone(void);
        uint8_t* readPtr(void);

        inline bool   isConnected(void) { return _connected; }; // check if the other end is connected, based on a timeout
        inline int8_t getRssi    (void) { return _rssi; };      // RSSI, 16 is the best it can do, higher numer is worse

        void pause(void);        // halts the state machine until resume is called
        void resume(void);       // resumes halted state machine
        bool isPaused(void);     // checks if state machine is halted
        bool isBusy(void);       // radio is busy waiting for something, so there's time for other application processing

        void pairingStart(void); // starts pairing mode
        void pairingStop(void);  // stops pairing mode

        inline uint32_t getSessionId(void) { return _session_id; };

        inline void setReplyReqRate(uint16_t x) { _reply_request_rate = x; }; // sets how often the robot is asked to reply
        void setChanMap(uint32_t x);                                          // gives the radio a new channel map during runtime
        void setTxInterval(uint16_t x);                                       // sets new transmission interval while respecting the limits
        inline uint16_t getTxInterval(void) { return _tx_interval; };

        radiostats_t stats_rate;
        radiostats_t stats_sum;

        #ifdef NRFRR_DEBUG_RX_ERRSTATS
        inline uint32_t* getRxErrStat(void) { return _stat_rx_errs; };
        #endif
        inline uint8_t getLastSentLength(void) { return _last_sent_length; };
        inline uint8_t getLastRxLength  (void) { return _last_rx_length  ; };

        void contTxTest(uint16_t f, bool mod, void(*loop_cb)(void) = NULL); // continuous transmission test, modulated or unmodulated carrier

    private:
        bool     _is_tx; // role

        // connection identifiers
        uint32_t _salt;
        uint32_t _uid;
        uint32_t _chan_map;

        int8_t   _fem_tx, _fem_rx;     // pin numbers for FEM, negative for unconnected

        bool     _is_pairing;          // whether pairing mode is active or not, pairing disables packet validation

        uint32_t _session_id = 0;
        uint32_t _seq_num;
        uint32_t _seq_num_prev;
        uint32_t _reply_seq;
        uint32_t _txt_seq;
        uint8_t  _statemachine = NRFRR_SM_IDLE_WAIT;
        uint8_t  _statemachine_next;
        uint32_t _sm_time;
        uint32_t _sm_evt_time;
        bool     _connected;
        uint8_t  _cur_chan;
        int8_t   _last_chan;
        bool     _single_chan_mode;
        uint8_t* _hop_table;
        uint8_t  _hop_tbl_len;
        uint32_t _tx_interval;
        uint32_t _tx_replyreq_tmr;
        bool     _reply_requested;
        bool     _reply_request_latch;
        uint16_t _reply_request_rate;
        uint8_t  _last_sent_length, _last_rx_length;

        int8_t   _rssi;
        radiostats_t stats_tmp;

        uint32_t _last_rx_time;        // time of reception, after the packet is validated
        uint32_t _last_hop_time;       // time of last frequency hop
        uint32_t _last_tx_time;        // time of last transmission
        uint32_t _rx_time_cache;       // time of reception, before the packet is validated
        uint32_t _rx_miss_cnt;         // used to determine if a missed packet is the first, second, or n-th, missed packet
        uint32_t _reply_bad_session;   // count bad session IDs in the replies, used to detect replay attacks

        int32_t  _text_send_timer;     // counter for text sending redundancy

        #ifdef NRFRR_DEBUG_RX_ERRSTATS
        uint32_t _stat_rx_errs[12];
        #endif

        #ifdef NRFRR_DEBUG_PINS
        uint8_t _dbgpin_tx, _dbgpin_rx, _dbgpin_ch; // used to track previous pin state for sake of toggling
        #endif

        bool    _encrypting;
        uint8_t _encryption_cnf[RH_NRF51_AES_CCM_CNF_SIZE];
        uint8_t _scratch[RH_NRF51_MAX_PAYLOAD_LEN+1+16];
        void set_encryption(void);

        void var_reset(void);          // sets all variables to initial state
        bool validate_rx(void);        // process the packet, return if it is valid
        void set_net_addr(uint32_t x); // place UID into hardware registers
        void init_hw(void);            // low level hardware init
        void init_txpwr(void);         // set RF transmit power
        void prep_tx(void);            // prepare payload
        void gen_header(void);         // generate the frame header
        void gen_hop_table(void);      // generate freq hop table
        void next_chan(void);          // hops to next channel
};

#endif
