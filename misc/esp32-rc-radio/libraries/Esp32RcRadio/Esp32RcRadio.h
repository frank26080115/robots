#ifndef _ESP32RCRADIO_H_
#define _ESP32RCRADIO_H_

#include <Arduino.h>

#include <stdint.h>
#include <stdbool.h>
#include <esp_wifi_types.h>

#define E32RCRAD_PAYLOAD_SIZE 32
#define E32RCRAD_TX_INTERV    10
#define E32RCRAD_TX_RETRANS   2
#define E32RCRAD_CHAN_MAP_DEFAULT (0x3FFF & (~((uint32_t)((1 << 0) | (1 << 5) | (1 << 10) | (1 << 13)))))
#define E32RCRAD_CHAN_MAP_DEFAULT_SINGLE 0x02
//#define E32RCRAD_OBFUSCATE_ADDRS // does not work very well
//#define E32RCRAD_FORCE_PHY_RATE
//#define E32RCRAD_DEBUG_HOPTABLE
//#define E32RCRAD_DEBUG_TX
//#define E32RCRAD_DEBUG_RX

class Esp32RcRadio
{
    public:
        Esp32RcRadio  (bool is_tx); // constructor, selects if the role is transmitter or receiver
        void var_reset(void);       // sets all variables to initial state
        void begin    (uint16_t chan_map, uint32_t uid, uint32_t salt); // init, provide channel map as a bitmask, unique identifier for pairing, and security salt
        void send     (uint8_t* data, bool immediate); // queue data to be sent, and optionally: send immediately
        int  available(void);                          // check if new packet has been received
        int  read     (uint8_t* data);                 // copy the new packet to a buffer
        void task     (void);                          // periodic task, call from loop()

        inline uint32_t get_last_time    (void) { return _last_time; };
               uint32_t get_data_rate    (void);
        inline uint32_t get_timeout_cnt  (void) { return _timeout_cnt; };
        inline void     reset_timeout_cnt(void) { _timeout_cnt = 0; };
               void     get_rx_stats     (uint32_t* total, uint32_t* good, signed* rssi);

        // do not call this from application, it is called by a static callback routine
        void handle_rx(void* buf, wifi_promiscuous_pkt_type_t type);

    private:
        bool _is_tx;
        uint32_t _salt;
        uint32_t _uid;
        uint16_t _chan_map;
        uint8_t _cur_chan;
        int8_t  _last_chan;
        uint32_t _stat_cnt_1;
        uint32_t _stat_cnt_2;
        uint32_t _stat_tmr_1;
        uint32_t _stat_tmr_2;
        uint32_t _stat_drate_1;
        uint32_t _stat_drate_2;
        uint32_t _stat_rx_total;
        uint32_t _stat_rx_good;
        signed   _stat_rx_rssi;
        uint32_t _last_time;
        uint32_t _timeout_cnt;

        void tx(void);             // actually transmit whatever is queued
        void gen_header(void);     // generate the frame header
        void start_listener(void); // starts promiscuous mode
        void next_chan(void);      // hops to next channel
        void stat_cnt_up(void);    // adds 1 to statistics
};

#endif
