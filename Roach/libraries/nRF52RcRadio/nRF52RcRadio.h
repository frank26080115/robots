#ifndef _NRF52RAWRADIO_H_
#define _NRF52RAWRADIO_H_

#include <Arduino.h>

#include <stdint.h>
#include <stdbool.h>

#define RH_NRF51_MAX_PAYLOAD_LEN 254
#define NRFRR_PAYLOAD_SIZE   (64 * 1)
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

//#define NRFRR_DEBUG_PINS
#define NRFRR_DEBUG_PIN_TX 27
#define NRFRR_DEBUG_PIN_RX 14
#define NRFRR_DEBUG_PIN_CH 15

#define NRFRR_BIDIRECTIONAL
#define NRFRR_ADAPTIVE_INTERVAL
//#define NRFRR_USE_INTERRUPTS

#define NRFRR_USE_FREQ_LOWER
#define NRFRR_USE_FREQ_NORTHAMERICAN
//#define NRFRR_USE_FREQ_EUROPEAN
#define NRFRR_USE_FREQ_UPPER

#define NRFRR_DEBUG_HOPTABLE
//#define NRFRR_DEBUG_TX
//#define NRFRR_DEBUG_RX
//#define NRFRR_DEBUG_HOP
#define NRFRR_DEBUG_RX_ERRSTATS
//#define NRFRR_COUNT_RX_SPENT_TIME

#define RH_NRF51_AES_CCM_CNF_SIZE 33

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
    NRFRR_SM_CONT_TX,
};

enum
{
    NRFRR_FLAG_REPLYREQUEST,
    NRFRR_FLAG_REPLY,
    NRFRR_FLAG_TEXT,
};

class nRF52RcRadio
{
    public:
        nRF52RcRadio  (bool is_tx); // constructor, selects if the role is transmitter or receiver
        void var_reset(void);       // sets all variables to initial state
        void begin    (uint32_t chan_map, uint32_t uid, uint32_t salt, int fem_tx = -1, int fem_rx = -1); // init, provide channel map as a bitmask, unique identifier for pairing, and security salt
        void send     (uint8_t* data);                         // queue data to be sent, and optionally: send immediately
        int  available(void);                                  // check if new packet has been received
        int  read     (uint8_t* data);                         // copy the new packet to a buffer
        void task     (void);                                  // periodic task, call from loop()
        bool state_machine_run(uint32_t t, bool is_isr);       // innards of task(), but can be called from ISR
        int  textAvail(void);                                  // check if text message is available
        int  textRead (const char* buf);                             // read available text message
        void textSend (const char* buf);                             // send text message

        inline bool connected(void) { return _connected; };
        inline int  get_rssi(void)  { return _stat_rx_rssi; };

        void pause(void);
        void resume(void);
        bool is_paused(void);
        bool is_busy(void);

        inline void set_reply_req_rate(uint16_t x) { _reply_request_rate = x; };
        void set_chan_map(uint32_t x);
        void set_tx_interval(uint16_t x);
        inline uint16_t get_tx_interval(void) { return _tx_interval; };

        inline uint32_t get_last_rx_time (void) { return _last_rx_time; };
        inline uint32_t get_data_rate    (void) { return _stat_drate; };
        inline uint32_t get_loss_rate    (void) { return _stat_loss_rate; };
               void     get_rx_stats     (uint32_t* total, uint32_t* good, signed* rssi);
        inline uint32_t get_rx_spent_time(void) { return _stat_rx_spent_time; };
        inline void     add_rx_spent_time(uint32_t x) { _stat_rx_spent_time += x; };
        inline uint32_t get_txt_cnt      (void) { return _stat_txt_cnt; };

        #ifdef NRFRR_DEBUG_RX_ERRSTATS
        inline uint32_t* get_rx_err_stat(void) { return _stat_rx_errs; };
        #endif

        void cont_tx(uint16_t f);

    private:
        bool     _is_tx;
        uint32_t _session_id;
        uint32_t _seq_num;
        uint32_t _seq_num_prev;
        uint8_t  _statemachine = NRFRR_SM_IDLE_WAIT;
        uint8_t  _statemachine_next;
        uint32_t _sm_time;
        uint32_t _sm_evt_time;
        bool     _connected;
        uint32_t _salt;
        uint32_t _uid;
        uint32_t _chan_map;
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
        uint32_t _stat_tx_cnt;
        uint32_t _stat_rx_cnt;
        uint32_t _stat_txt_cnt;
        uint32_t _stat_rx_lost;
        uint32_t _stat_tmr;
        uint32_t _stat_drate;
        uint32_t _stat_loss_rate;
        uint32_t _rx_miss_cnt;
        #ifdef NRFRR_DEBUG_RX_ERRSTATS
        uint32_t _stat_rx_errs[12];
        #endif

        uint32_t _stat_rx_total;
        uint32_t _stat_rx_good;
        signed   _stat_rx_rssi;
        uint32_t _stat_rx_spent_time;

        uint32_t _last_rx_time;
        uint32_t _last_hop_time;
        uint32_t _last_tx_time;
        uint32_t _rx_time_cache;

        int32_t  _text_send_timer;

        #ifdef NRFRR_DEBUG_PINS
        uint8_t _dbgpin_tx, _dbgpin_rx, _dbgpin_ch;
        #endif

        bool    _encrypting;
        uint8_t _encryption_cnf[RH_NRF51_AES_CCM_CNF_SIZE];
        uint8_t _scratch[RH_NRF51_MAX_PAYLOAD_LEN+1+16];
        void set_encryption(void);

        bool validate_rx(void);        // process the packet, return if it is valid
        void set_net_addr(uint32_t x); // place UID into hardware registers
        void init_hw(void);            // low level hardware init
        void prep_tx(void);            // prepare payload
        void gen_header(void);         // generate the frame header
        void gen_hop_table(void);      // generate freq hop table
        void next_chan(void);          // hops to next channel
};

#endif
