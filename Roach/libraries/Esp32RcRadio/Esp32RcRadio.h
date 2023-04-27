#ifndef _ESP32RCRADIO_H_
#define _ESP32RCRADIO_H_

#include <Arduino.h>

#include <stdint.h>
#include <stdbool.h>
#include <esp_wifi_types.h>

#define E32RCRAD_PAYLOAD_SIZE   32
#define E32RCRAD_TX_INTERV_DEF  10
#define E32RCRAD_TX_INTERV_MAX  100
#define E32RCRAD_TX_INTERV_MIN  5
#define E32RCRAD_TX_RETRANS     1
#define E32RCRAD_TX_RETRANS_TXT 5
#define E32RCRAD_CHAN_MAP_DEFAULT (0x3FFF & (~((uint32_t)((1 << 0) | (1 << 5) | (1 << 10) | (1 << 13)))))
#define E32RCRAD_CHAN_MAP_DEFAULT_SINGLE 0x02

#define E32RCRAD_REM_USED_SESSIONS 32

#define E32RCRAD_FINGER_QUOTES_RANDOMNESS
#define E32RCRAD_FINGER_QUOTES_ENCRYPTION

#define E32RCRAD_BIDIRECTIONAL
#define E32RCRAD_ADAPTIVE_INTERVAL

//#define E32RCRAD_FORCE_PHY_RATE
#define E32RCRAD_FORCE_BANDWIDTH
#define E32RCRAD_FORCE_RFPOWER
//#define E32RCRAD_DEBUG_HOPTABLE
//#define E32RCRAD_DEBUG_TX
//#define E32RCRAD_DEBUG_RX
//#define E32RCRAD_DEBUG_HOP
#define E32RCRAD_DEBUG_RX_ERRSTATS
//#define E32RCRAD_COUNT_RX_SPENT_TIME

enum
{
    E32RCRAD_SM_IDLE,
    E32RCRAD_SM_LISTENING,
    E32RCRAD_SM_REPLYING,
};

enum
{
    E32RCRAD_FLAG_REPLYREQUEST,
    E32RCRAD_FLAG_REPLY,
    E32RCRAD_FLAG_TEXT,
};

class Esp32RcRadio
{
    public:
        Esp32RcRadio  (bool is_tx); // constructor, selects if the role is transmitter or receiver
        void var_reset(void);       // sets all variables to initial state
        void begin    (uint16_t chan_map, uint32_t uid, uint32_t salt); // init, provide channel map as a bitmask, unique identifier for pairing, and security salt
        void send     (uint8_t* data, bool immediate = false); // queue data to be sent, and optionally: send immediately
        int  available(void);                                  // check if new packet has been received
        int  read     (uint8_t* data);                         // copy the new packet to a buffer
        void task     (void);                                  // periodic task, call from loop()
        int  textAvail(void);                                  // check if text message is available
        int  textRead (char* buf);                             // read available text message
        void textSend (char* buf);                             // send text message

        inline void set_reply_req_rate(uint16_t x) { _reply_request_rate = x; };
        void set_chan_map(uint16_t x);
        void set_tx_interval(uint16_t x);
        inline uint16_t get_tx_interval(void) { return _tx_interval; };

        inline uint32_t get_last_rx_time (void) { return _last_rx_time; };
        inline uint32_t get_data_rate    (void) { return _stat_drate; };
        inline uint32_t get_loss_rate    (void) { return _stat_loss_rate; };
               void     get_rx_stats     (uint32_t* total, uint32_t* good, signed* rssi);
        inline uint32_t get_rx_spent_time(void) { return _stat_rx_spent_time; };
        inline void     add_rx_spent_time(uint32_t x) { _stat_rx_spent_time += x; };
        inline uint32_t get_txt_cnt      (void) { return _stat_txt_cnt; };

        #ifdef E32RCRAD_DEBUG_RX_ERRSTATS
        inline uint32_t* get_rx_err_stat(void) { return _stat_rx_errs; };
        #endif

        // do not call this from application, it is called by a static callback routine
        void handle_rx(void* buf, wifi_promiscuous_pkt_type_t type);

    private:
        bool     _is_tx;
        uint32_t _session_id;
        uint32_t _seq_num;
        uint32_t _seq_num_prev;
        uint8_t  _statemachine;
        uint32_t _salt;
        uint32_t _uid;
        uint16_t _chan_map;
        uint8_t  _cur_chan;
        int8_t   _last_chan;
        uint8_t  _hop_table[14];
        uint8_t  _hop_tbl_len;
        uint8_t  _sync_hop_cnt;
        uint32_t _tx_interval;
        uint32_t _tx_replyreq_tmr;
        bool     _reply_request_latch;
        uint16_t _reply_request_rate;
        uint32_t _stat_tx_cnt;
        uint32_t _stat_rx_cnt;
        uint32_t _stat_txt_cnt;
        uint32_t _stat_rx_lost;
        uint32_t _stat_tmr;
        uint32_t _stat_drate;
        uint32_t _stat_loss_rate;
        #ifdef E32RCRAD_DEBUG_RX_ERRSTATS
        uint32_t _stat_rx_errs[12];
        #endif

        uint32_t _stat_rx_total;
        uint32_t _stat_rx_good;
        signed   _stat_rx_rssi;
        uint32_t _stat_rx_spent_time;

        uint32_t _last_rx_time;
        uint32_t _last_hop_time;
        uint32_t _last_rx_time_4hop;
        uint32_t _last_tx_time;

        uint32_t _text_send_timer;

        void tx(void);                 // actually transmit whatever is queued
        void gen_header(void);         // generate the frame header
        void gen_hop_table(void);      // generate freq hop table
        void start_listener(void);     // starts promiscuous mode
        void prep_listener(void);      // prepares promiscuous mode
        void next_chan(void);          // hops to next channel
};

#endif
