#include "Esp32RcRadio.h"

#include <esp_wifi.h>
#include <esp_wifi_types.h>
#include <esp_system.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct
{
    // 802.11 packet structure
    uint16_t frame_ctrl;
    uint16_t duration;   // note: this field might be changed by API, do not check it or checksum it
    uint32_t addrs[3];   // MAC addresses for RX and TX, 6 bytes each, but since we are using 32 bit integers, we can stuff 3 items in here
                         // the 1st item is just random for each packet
                         // the 2nd item is the UID of the pairing, but XOR'ed with the random number
                         // the 3rd item is the session ID, but XOR'ed with the random number
    uint8_t  padding[2]; // some frame types use these field for things
    uint8_t  chan_hint;  // what channel index was actually used to TX
    uint32_t seq_num;
    uint8_t  flags;      // contains things like a reply-request
    #ifdef E32RCRAD_ADAPTIVE_INTERVAL
    uint8_t  interval;   // transmission interval that the receiver can adapt the timeout
    #endif
    uint8_t  payload[E32RCRAD_PAYLOAD_SIZE]; // our custom payload
    uint32_t chksum;     // userspace checksum over the payload, accounts for the salt, not to be confused with FCS
                         // combined with the salt, this is meant to prevent impersonation attacks
}
__attribute__ ((packed))
e32rcrad_pkt_t;

#define E32RCRAD_FRAME_CONTROL_ACTION 0x00D0
#define E32RCRAD_FRAME_CONTROL_PROBE  0x0040
#define E32RCRAD_FRAME_CONTROL_BA     0x0094 // not allowed
#define E32RCRAD_FRAME_CONTROL        E32RCRAD_FRAME_CONTROL_ACTION

static uint8_t tx_buffer[E32RCRAD_PAYLOAD_SIZE];     // application buffer
static uint8_t rx_buffer[E32RCRAD_PAYLOAD_SIZE];     // application buffer
static uint8_t frame_buffer[sizeof(e32rcrad_pkt_t)]; // the buffer that's actually sent (and also used for rx comparisons)
static bool    rx_flag;                              // indicator of new packet received

static char    txt_rx_buffer[E32RCRAD_PAYLOAD_SIZE]; // application text buffer
static char    txt_tx_buffer[E32RCRAD_PAYLOAD_SIZE]; // application text buffer
static bool    txt_flag;                             // indicator of text packet received

static uint32_t invalid_sessions[E32RCRAD_REM_USED_SESSIONS];

static bool has_wifi_init = false; // only init once

static Esp32RcRadio* instance; // only one instance allowed for the callback

static void promiscuous_handler(void* buf, wifi_promiscuous_pkt_type_t type);
extern uint32_t e32rcrad_getCheckSum(uint32_t salt, uint8_t* data, uint8_t len);

Esp32RcRadio::Esp32RcRadio(bool is_tx)
{
    _is_tx = is_tx;
    memset(invalid_sessions, 0, sizeof(uint32_t) * E32RCRAD_REM_USED_SESSIONS);
    var_reset();
}

void Esp32RcRadio::var_reset(void)
{
    _tx_interval = E32RCRAD_TX_INTERV_DEF;
    _session_id = 0;
    _seq_num = 0;
    _seq_num_prev = 0;
    _statemachine = 0;
    _stat_tmr = 0;
    _stat_tx_cnt = 0;
    _stat_rx_cnt = 0;
    _stat_txt_cnt = 0;
    _stat_drate = 0;
    _stat_rx_lost = 0;
    _stat_loss_rate = 0;
    #ifdef E32RCRAD_DEBUG_RX_ERRSTATS
    memset(_stat_rx_errs, 0, 4*12);
    #endif
    _stat_rx_total = 0;
    _stat_rx_good = 0;
    _last_rx_time = 0;
    _last_tx_time = 0;
    _last_hop_time = 0;
    _last_rx_time_4hop = 0;
    _sync_hop_cnt = 0;
    _tx_replyreq_tmr = 0;
    _reply_request_rate = 1000;
    _reply_request_latch = false;
    _cur_chan = 0;
    _last_chan = -1;
    _text_send_timer = 0;
    _stat_rx_spent_time = 0;
    rx_flag = false;
    txt_flag = false;
}

void Esp32RcRadio::begin(uint16_t chan_map, uint32_t uid, uint32_t salt)
{
    instance = (Esp32RcRadio*)this;

    #ifdef E32RCRAD_DEBUG_PINS
    pinMode     (E32RCRAD_DEBUG_PIN_TX, OUTPUT);
    digitalWrite(E32RCRAD_DEBUG_PIN_TX, _dbgpin_tx = LOW);
    pinMode     (E32RCRAD_DEBUG_PIN_RX, OUTPUT);
    digitalWrite(E32RCRAD_DEBUG_PIN_RX, _dbgpin_rx = LOW);
    pinMode     (E32RCRAD_DEBUG_PIN_CH, OUTPUT);
    digitalWrite(E32RCRAD_DEBUG_PIN_CH, _dbgpin_ch = LOW);
    #endif

    var_reset();
    _uid = uid;
    _salt = salt;

    set_chan_map(chan_map);

    if (has_wifi_init == false) // only start once
    {
        has_wifi_init = true;
        //esp_netif_init();
        //esp_event_loop_create_default();
        //esp_netif_create_default_wifi_ap();
        wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
        esp_wifi_init(&cfg);
        esp_wifi_set_mode(WIFI_MODE_AP);
        wifi_config_t ap_config = {
            .ap = {
                .ssid = {0},
                .password = {0},
                .ssid_len = 0,
                .channel = _hop_table[_cur_chan % _hop_tbl_len],
                .authmode = WIFI_AUTH_WPA2_PSK,
                .ssid_hidden = 1,
                .max_connection = 4,
                .beacon_interval = 60000
            }
        };

        esp_wifi_set_config(WIFI_IF_AP, &ap_config);
        #ifdef E32RCRAD_FORCE_PHY_RATE
        esp_wifi_config_80211_tx_rate(WIFI_IF_AP, WIFI_PHY_RATE_54M); // sets PHY rate
        #endif
        #ifdef E32RCRAD_FORCE_BANDWIDTH
        esp_wifi_set_bandwidth(WIFI_IF_AP, WIFI_BW_HT20);
        #endif
        esp_wifi_start();
        esp_wifi_set_ps(WIFI_PS_NONE);
        #ifdef E32RCRAD_FORCE_RFPOWER
        esp_wifi_set_max_tx_power(84);
        #endif
        prep_listener();
    }
    else
    {
        next_chan();
    }

    if (_is_tx == false)
    {
        start_listener();
    }
    else
    {
        while (_session_id == 0) {
            _session_id = esp_random(); // this needs to happen after wifi init to get the real randomness from RF
        }
        srand(_session_id);
    }
}

void Esp32RcRadio::gen_hop_table(void)
{
    // generate hop table, fill table with an entry if the bit of the map bitmask is set
    int i, j;
    for (i = 0, j = 0; i < 14; i++)
    {
        if ((_chan_map & (1 << i)) != 0) {
            _hop_table[j] = i + 1;
            _hop_table[j + 1] = 0;
            j++;
            _hop_tbl_len = j;
        }
    }

    if (_salt != 0)
    {
        uint32_t tmp32[4];
        tmp32[0] = _uid;
        tmp32[1] = _salt;
        tmp32[2] = e32rcrad_getCheckSum(0, (uint8_t*)tmp32, 8);
        tmp32[3] = e32rcrad_getCheckSum(0, (uint8_t*)&(tmp32[1]), 8);
        uint8_t* tmp8 = (uint8_t*)tmp32;

        // shuffle the table according to our salt
        // this prevents jamming by following a known hop sequence
        for (i = 0; i < _hop_tbl_len; i++)
        {
            j = tmp8[i] % _hop_tbl_len;
            uint8_t t = _hop_table[j];
            _hop_table[j] = _hop_table[i];
            _hop_table[i] = t;
        }
    }
    #ifdef E32RCRAD_DEBUG_HOPTABLE
    Serial.printf("Hop Table: ");
    for (i = 0; i < _hop_tbl_len; i++) {
        Serial.printf("%d  ", _hop_table[i]);
    }
    #endif
    _cur_chan %= _hop_tbl_len;
}

void Esp32RcRadio::set_chan_map(uint16_t x)
{
    _single_chan_mode = (x & 0x8000) != 0;
    _chan_map = x & 0x3FFF;
    _chan_map = _chan_map == 0 ? E32RCRAD_CHAN_MAP_DEFAULT : _chan_map;
    gen_hop_table();
}

void Esp32RcRadio::set_tx_interval(uint16_t x)
{
    _tx_interval = x > E32RCRAD_TX_INTERV_MAX ? E32RCRAD_TX_INTERV_MAX : (x < E32RCRAD_TX_INTERV_MIN ? E32RCRAD_TX_INTERV_MIN : x);
}

void Esp32RcRadio::send(uint8_t* data, bool immediate)
{
    memcpy(tx_buffer, data, E32RCRAD_PAYLOAD_SIZE);

    if (immediate) {
        tx();
    }
}

void Esp32RcRadio::tx(void)
{
    bool is_text = _text_send_timer > 0;

    e32rcrad_pkt_t* ptr = (e32rcrad_pkt_t*)frame_buffer;
    gen_header();
    if (is_text) {
        ptr->flags ^= (1 << E32RCRAD_FLAG_TEXT);
    }

    uint8_t* src_buf = is_text ? (uint8_t*)txt_tx_buffer : (uint8_t*)tx_buffer;

    #ifdef E32RCRAD_FINGER_QUOTES_ENCRYPTION
    // copy while "encrypting"
    uint32_t r = ptr->addrs[0];
    int j;
    for (j = 0; j < E32RCRAD_PAYLOAD_SIZE; j += 4)
    {
        uint32_t* ptr_src = (uint32_t*)&(src_buf[j]);
        uint32_t* ptr_dst = (uint32_t*)&(ptr->payload[j]);
        *ptr_dst = (*ptr_src) ^ r;
    }
    #else
    memcpy(ptr->payload, src_buf, E32RCRAD_PAYLOAD_SIZE);
    #endif

    ptr->chksum = e32rcrad_getCheckSum(_salt, &(frame_buffer[4]), sizeof(e32rcrad_pkt_t) - 8);

    #ifdef E32RCRAD_DEBUG_PINS
    digitalWrite(E32RCRAD_DEBUG_PIN_TX, _dbgpin_tx = (_dbgpin_tx == LOW ? HIGH : LOW));
    #endif

    int i;
    for (i = 0; i == 0 || (i < E32RCRAD_TX_RETRANS && _is_tx && is_text == false); i++)
    {
        uint32_t tx_len = sizeof(e32rcrad_pkt_t);
        tx_len += rand() % 8; // randomize length to prevent analysis
        esp_wifi_80211_tx(WIFI_IF_AP, frame_buffer, tx_len, false);
        //free80211_send(frame_buffer, tx_len);
    }

    #ifdef E32RCRAD_DEBUG_TX
    Serial.printf("TX[t %u][c %u][s %u]: ", millis(), hop_table[_cur_chan], sizeof(e32rcrad_pkt_t));
    int i;
    for (i = 0; i < sizeof(e32rcrad_pkt_t); i++)
    {
        Serial.printf("0x%02X ", frame_buffer[i]);
    }
    Serial.printf("\r\n");
    #endif

    _stat_tx_cnt++;
    _last_tx_time = millis();

    #ifdef E32RCRAD_DEBUG_HOP
    Serial.printf("TX %u %u\r\n", _last_tx_time, _seq_num);
    #endif

    _statemachine = E32RCRAD_SM_SENDING;

    if (is_text)
    {
        // text needs to be repeatedly sent for redundancy but we also want to avoid duplicates
        // so we do not increment sequence number, unless we have finished sending the text
        _text_send_timer--;
        if (_text_send_timer <= 0) {
            _seq_num++;
        }
    }
    else
    {
        // increment sequence number since it's not text, we want to avoid duplicates
        _seq_num++;
    }

    if (_seq_num == 0) {
        // roll-over occured, start new session
        _session_id = esp_random();
        _seq_num++;
    }
}

void Esp32RcRadio::next_chan(void)
{
    _cur_chan++;
    _cur_chan %= _hop_tbl_len;
    uint8_t nchan = _hop_table[_cur_chan];
    //if (nchan != _last_chan) // prevent wasting time changing to the same channel
    {
        #ifdef E32RCRAD_DEBUG_HOP
        Serial.printf("HOP %u %u %u\r\n", millis(), _cur_chan, nchan);
        #endif

        if (_is_tx == false) {
            esp_wifi_set_promiscuous(false);
        }

        // I'm like 99% sure we don't need to actually retry these calls
        int retries;
        for (retries = 100; retries > 0; retries--)
        {
            if (esp_wifi_set_channel(nchan, WIFI_SECOND_CHAN_NONE) == ESP_OK)
            {
                uint8_t x;
                wifi_second_chan_t x2;
                if (esp_wifi_get_channel(&x, &x2) == ESP_OK) {
                    if (x == nchan) {
                        break;
                    }
                }
            }
        }

        #ifdef E32RCRAD_DEBUG_PINS
        digitalWrite(E32RCRAD_DEBUG_PIN_CH, _dbgpin_ch = (_dbgpin_ch == LOW ? HIGH : LOW));
        #endif

        if (_is_tx == false) {
            start_listener();
        }
        _last_chan = nchan;
    }

    _last_hop_time = millis();
}

void Esp32RcRadio::gen_header(void)
{
    uint32_t now = millis();

    e32rcrad_pkt_t* ptr = (e32rcrad_pkt_t*)frame_buffer;
    ptr->frame_ctrl = E32RCRAD_FRAME_CONTROL;
    ptr->duration = 0;

    uint32_t r =
        #ifdef E32RCRAD_FINGER_QUOTES_RANDOMNESS
            rand()
        #else
            0
        #endif
        ;
    // the random number is used to make the MAC addresses random each time just in case it's a match for a real device
    // we don't want that device to actually be confused
    // it also makes the packet look like it's encrypted
    // it is not...
    // if the attacker reads this source code, the random number is pretty much useless
    ptr->padding[0] = r;
    ptr->padding[1] = ~r;
    ptr->addrs[0]   = r;
    ptr->addrs[1]   = _uid         ^ r;
    ptr->addrs[2]   = _session_id  ^ r;
    ptr->chan_hint  = _cur_chan    ^ r;
    ptr->seq_num    = _seq_num     ^ r;
    #ifdef E32RCRAD_ADAPTIVE_INTERVAL
    ptr->interval   = _tx_interval ^ r;
    #endif
    ptr->flags      = 0;

    #ifdef E32RCRAD_BIDIRECTIONAL
    if (_is_tx == false)
    {
        ptr->flags |= (1 << E32RCRAD_FLAG_REPLY);
    }
    else
    {
        if ((now - _tx_replyreq_tmr) >= _reply_request_rate && _reply_request_rate != 0) {
            _reply_request_latch = true;
            _tx_replyreq_tmr = now;
        }
        if (_reply_request_latch) {
            ptr->flags |= (1 << E32RCRAD_FLAG_REPLYREQUEST);
        }
    }
    #endif

    ptr->flags ^= r;
}

void Esp32RcRadio::task(void)
{
    uint32_t now = millis();

    if (_is_tx == false)
    {
        if (_statemachine == E32RCRAD_SM_SENDING)
        {
            if ((now - _last_tx_time) >= E32RCRAD_TX_MIN_TIME)
            {
                _statemachine = E32RCRAD_SM_LISTENING;
            }
        }
        else if (_statemachine == E32RCRAD_SM_REPLYING)
        {
            //esp_wifi_set_promiscuous(false);
            if ((now - _last_rx_time) >= E32RCRAD_TX_MIN_TIME)
            {
                tx();
                // state will be set to sending
            }
        }
        else if (_statemachine == E32RCRAD_SM_LISTENING && (now - _last_rx_time_4hop) >= _tx_interval) // reply has been sent, can switch channels
        {
            _last_rx_time_4hop = now;
            if (_single_chan_mode == false) {
                next_chan(); // this should also restart the listening
            }
            _statemachine = E32RCRAD_SM_IDLE;
        }
        else if (_sync_hop_cnt > 0 && (now - _last_rx_time_4hop) >= _tx_interval)
        {
            // we do quick hops in attempts to quickly resynchronize with the transmitter
            _last_rx_time_4hop = now;
            _sync_hop_cnt--;
            if (_single_chan_mode == false) {
                next_chan();
            }
        }
        else if (_sync_hop_cnt <= 0 && (now - _last_hop_time) >= (_tx_interval <= 0 ? 100 : (_tx_interval * 3)))
        {
            // quick hops didn't work, so we've lost sync with transmitter, do slow hops now
            // this is so that we don't stay in a useless channel forever
            if (_single_chan_mode == false) {
                next_chan();
            }
        }

        // prevent statistics overflow
        if (_stat_rx_total > 1000 || _stat_rx_good > 1000) {
            _stat_rx_total /= 2;
            _stat_rx_good  /= 2;
        }
    }
    else // _is_tx == true
    {
        if (_statemachine == E32RCRAD_SM_SENDING)
        {
            #ifdef E32RCRAD_BIDIRECTIONAL
            if ((now - _last_tx_time) >= E32RCRAD_TX_MIN_TIME)
            {
                start_listener(); // start listening for a reply
                _statemachine = E32RCRAD_SM_LISTENING;
            }
            #else
            _statemachine = E32RCRAD_SM_LISTENING;
            #endif
        }
        else if (_statemachine == E32RCRAD_SM_LISTENING)
        {
            if ((now - _last_tx_time) >= (_tx_interval / 3))
            {
                _statemachine = E32RCRAD_SM_IDLE;
                if (_single_chan_mode == false)
                {
                    #ifdef E32RCRAD_BIDIRECTIONAL
                    esp_wifi_set_promiscuous(false);
                    #endif
                    next_chan();
                }
            }
        }
        else
        {
            if ((now - _last_tx_time) >= _tx_interval)
            {
                // send queued packet if enough time has passed
                tx();
                // _last_time will be set from inside
            }
        }
    }

    if ((now - _stat_tmr) >= 1000)
    {
        _stat_drate = _is_tx ? _stat_tx_cnt : _stat_rx_cnt;
        _stat_loss_rate = _stat_rx_lost;
        _stat_tx_cnt = 0;
        _stat_rx_cnt = 0;
        _stat_rx_lost = 0;
        _stat_tmr = now;

        // every second, if in single channel RX mode, assess if the data rate is poor, and switch channel if it is
        if (_is_tx == false && _single_chan_mode)
        {
            if (_stat_drate < ((1000 / 20) - 5)) {
                next_chan();
            }
        }
    }
}

void Esp32RcRadio::start_listener(void)
{
    //prep_listener();
    esp_wifi_set_promiscuous(true);
}

void Esp32RcRadio::prep_listener(void)
{
    wifi_promiscuous_filter_t filter1 = {
        .filter_mask = WIFI_PROMIS_FILTER_MASK_MGMT
    };
    //wifi_promiscuous_filter_t filter2 = {
    //    .filter_mask = WIFI_PROMIS_FILTER_MASK_CTRL | WIFI_PROMIS_CTRL_FILTER_MASK_BA
    //};
    esp_wifi_set_promiscuous_filter(&filter1);
    //esp_wifi_set_promiscuous_ctrl_filter(&filter2);
    esp_wifi_set_promiscuous_rx_cb(&promiscuous_handler);
}

static void promiscuous_handler(void* buf, wifi_promiscuous_pkt_type_t type)
{
    #ifdef E32RCRAD_COUNT_RX_SPENT_TIME
    volatile uint32_t t_start = micros();
    #endif
    instance->handle_rx(buf, type);
    #ifdef E32RCRAD_COUNT_RX_SPENT_TIME
    volatile uint32_t t_end = micros();
    instance->add_rx_spent_time(t_end - t_start);
    #endif
}

void Esp32RcRadio::handle_rx(void* buf, wifi_promiscuous_pkt_type_t type)
{
    uint32_t now = millis();

    _stat_rx_total++;
    wifi_promiscuous_pkt_t* pkt = (wifi_promiscuous_pkt_t*)buf;
    wifi_pkt_rx_ctrl_t ctrl = (wifi_pkt_rx_ctrl_t)pkt->rx_ctrl;

    uint8_t* u8_ptr = (uint8_t*)pkt->payload;
    e32rcrad_pkt_t* parsed = (e32rcrad_pkt_t*)u8_ptr;

    #ifdef E32RCRAD_DEBUG_RX
    if (parsed->frame_ctrl == E32RCRAD_FRAME_CONTROL)
    {
        Serial.printf("RX[t %u]: ", millis());
        int i;
        for (i = 0; i < ctrl.sig_len; i++)
        {
            Serial.printf("0x%02X ", u8_ptr[i]);
        }
        Serial.printf("\r\n");
    }
    #endif

    if (ctrl.sig_len < sizeof(e32rcrad_pkt_t)) {
        #ifdef E32RCRAD_DEBUG_RX_ERRSTATS
        _stat_rx_errs[0]++;
        #endif
        return;
    }

    uint32_t* hdr = (uint32_t*)u8_ptr;
    uint32_t r = 
        #ifdef E32RCRAD_FINGER_QUOTES_RANDOMNESS
            hdr[1] // extract the meaningless random number to use to recover other data fields
        #else
            0
        #endif
        ;
    uint32_t rx_uid   = hdr[2] ^ r;
    uint32_t rx_sess  = hdr[3] ^ r;
    uint8_t  rx_flags = parsed->flags ^ r;
    uint32_t rx_seq   = parsed->seq_num ^ r;

    if (parsed->frame_ctrl != E32RCRAD_FRAME_CONTROL) { // frame type must match
        #ifdef E32RCRAD_DEBUG_RX_ERRSTATS
        _stat_rx_errs[1]++;
        //Serial.printf("E HDR 0x%04X\n", parsed->frame_ctrl);
        #endif
        return;
    }
    if (rx_uid != _uid) { // UID must match
        #ifdef E32RCRAD_DEBUG_RX_ERRSTATS
        _stat_rx_errs[2]++;
        #endif
        return;
    }
    if (rx_sess == 0) { // invalid session ID
        #ifdef E32RCRAD_DEBUG_RX_ERRSTATS
        _stat_rx_errs[3]++;
        #endif
        return;
    }

    // verify checksum of payload against claimed checksum
    uint32_t chksum = e32rcrad_getCheckSum(_salt, (uint8_t*)&(u8_ptr[4]), sizeof(e32rcrad_pkt_t) - 8);
    if (parsed->chksum != chksum) {
        #ifdef E32RCRAD_DEBUG_RX_ERRSTATS
        _stat_rx_errs[4]++;
        #endif
        return;
    }
    // checksum needs to be valid for the next few security features to work and not be overwhelmed

    bool to_hop = false;

    if (_is_tx == false) // only receiver requires additional security
    {
        to_hop = true; // receiver should hop immediately if no need to reply

        if (_session_id == 0) // first packet ever received
        {
            // start the new session
            _session_id = rx_sess;
            _seq_num_prev = rx_seq;
        }
        else if (_session_id == rx_sess)
        {
            if (rx_seq <= _seq_num_prev)
            {
                // ignore duplicate or replay-attack
                // still do a channel hop if it's a text
                if ((rx_flags & (1 << E32RCRAD_FLAG_TEXT)) != 0) {
                    _cur_chan = parsed->chan_hint ^ r; // sync with channel hop
                    if (_single_chan_mode == false) {
                        next_chan();
                    }
                    _last_rx_time = now;
                }
                #ifdef E32RCRAD_DEBUG_RX_ERRSTATS
                _stat_rx_errs[5]++;
                #endif
                return;
            }

            // calculate how many packets were lost
            uint32_t diff = rx_seq - _seq_num_prev;
            diff -= 1;
            diff = diff > 0xFFFF ? 0xFFFF : diff;
            _stat_rx_lost += diff;

            _seq_num_prev = rx_seq;
        }
        else if (_session_id != rx_sess)
        {
            int i;
            // check if session ID is still valid
            for (i = 0; i < E32RCRAD_REM_USED_SESSIONS; i++)
            {
                uint32_t j = invalid_sessions[i];
                if (j == rx_sess) { // is invalid
                    #ifdef E32RCRAD_DEBUG_RX_ERRSTATS
                    _stat_rx_errs[6]++;
                    #endif
                    return;
                }
                else if (j == 0) {
                    break;
                }
            }
            // store old session as invalid
            if (invalid_sessions[E32RCRAD_REM_USED_SESSIONS - 1] == 0) // enough space
            {
                // store in next empty slot
                for (i = 0; i < E32RCRAD_REM_USED_SESSIONS; i++)
                {
                    uint32_t j = invalid_sessions[i];
                    if (j == 0) {
                        invalid_sessions[i] = _session_id;
                    }
                }
            }
            else
            {
                // store in any slot
                invalid_sessions[rand() % E32RCRAD_REM_USED_SESSIONS] = _session_id;
            }
            // start new session
            _session_id = rx_sess;
            _seq_num_prev = rx_seq;
        }

        #ifdef E32RCRAD_ADAPTIVE_INTERVAL
        _tx_interval = parsed->interval ^ r;
        #endif

        #ifdef E32RCRAD_BIDIRECTIONAL
        if ((rx_flags & (1 << E32RCRAD_FLAG_REPLYREQUEST)) != 0 || _text_send_timer > 0) {
            _statemachine = E32RCRAD_SM_REPLYING; // signal that the tx function needs to run
            to_hop = false; // receiver should hop immediately if no need to reply, but wait to hop if need to reply
        }
        else
        #endif
        {
            _statemachine = E32RCRAD_SM_IDLE;
        }
    }
    #ifdef E32RCRAD_BIDIRECTIONAL
    else // _is_tx == true
    {
        // a reply isn't critical, but make sure the session ID is correct
        if (_session_id != rx_sess) {
            #ifdef E32RCRAD_DEBUG_RX_ERRSTATS
            _stat_rx_errs[7]++;
            #endif
            return;
        }

        if (rx_seq <= _seq_num_prev)
        {
            // ignore duplicate or replay-attack
            #ifdef E32RCRAD_DEBUG_RX_ERRSTATS
            _stat_rx_errs[8]++;
            #endif
            return;
        }
        _seq_num_prev = rx_seq;
    }
    #endif

    // all checks pass

    #ifdef E32RCRAD_DEBUG_PINS
    digitalWrite(E32RCRAD_DEBUG_PIN_RX, _dbgpin_rx = (_dbgpin_rx == LOW ? HIGH : LOW));
    #endif

    uint8_t* dst_buf = ((rx_flags & (1 << E32RCRAD_FLAG_TEXT)) != 0) ? (uint8_t*)txt_rx_buffer : (uint8_t*)rx_buffer;

    #ifdef E32RCRAD_FINGER_QUOTES_ENCRYPTION
    // copy while "decrypting"
    int j;
    for (j = 0; j < E32RCRAD_PAYLOAD_SIZE; j += 4)
    {
        uint32_t* ptr_dst = (uint32_t*)&(dst_buf[j]);
        uint32_t* ptr_src = (uint32_t*)&(parsed->payload[j]);
        *ptr_dst = (*ptr_src) ^ r;
    }
    #else
    memcpy(dst_buf, (void*)parsed->payload, E32RCRAD_PAYLOAD_SIZE);
    #endif

    if ((rx_flags & (1 << E32RCRAD_FLAG_TEXT)) == 0) {
        rx_flag = true;
    }
    else {
        txt_flag = true;
        _stat_txt_cnt++;
    }
    _reply_request_latch = false;

    #ifdef E32RCRAD_DEBUG_HOP
    bool hop_dbg_nl = true;
    Serial.printf("RX %u %u", now, _seq_num_prev);
    #endif

    if (_is_tx == false)
    {
        _cur_chan = parsed->chan_hint ^ r; // sync with channel hop
        #ifdef E32RCRAD_DEBUG_HOP
        Serial.printf(" %u", _cur_chan);
        #endif
        if (to_hop) {
            #ifdef E32RCRAD_DEBUG_HOP
            Serial.printf("\r\nR");
            hop_dbg_nl = false;
            #endif
            if (_single_chan_mode == false) {
                next_chan();
            }
            _last_rx_time_4hop = now - (_tx_interval / 8); // offset the time of the next hop very slightly
        }
        else {
            _last_rx_time_4hop = now - _tx_interval + (_tx_interval / 4); // offset the time of the next hop
        }

        // do this many quick hops if the transmitter disappears
        _sync_hop_cnt = _hop_tbl_len * 3;
    }

    #ifdef E32RCRAD_DEBUG_HOP
    if (hop_dbg_nl) {
        Serial.printf("\r\n");
    }
    #endif

    _last_rx_time = now;
    _stat_rx_rssi = ctrl.rssi;
    _stat_rx_good++;
    _stat_rx_cnt++;
}

void Esp32RcRadio::get_rx_stats(uint32_t* total, uint32_t* good, signed* rssi)
{
    if (total != NULL) {
        *total = _stat_rx_total;
    }
    if (good != NULL) {
        *good = _stat_rx_good;
    }
    if (rssi != NULL) {
        *rssi = _stat_rx_rssi;
    }
}

int Esp32RcRadio::available(void)
{
    task();
    return rx_flag ? E32RCRAD_PAYLOAD_SIZE : 0;
}

int Esp32RcRadio::read(uint8_t* data)
{
    task();
    if (rx_flag) // has data
    {
        if (data != NULL) {
            // copy to user buffer if available
            memcpy(data, rx_buffer, E32RCRAD_PAYLOAD_SIZE);
        }
        rx_flag = false; // mark as read
        return E32RCRAD_PAYLOAD_SIZE; // report success
    }
    return -1; // report no data
}

int Esp32RcRadio::textAvail(void)
{
    task();
    return txt_flag ? E32RCRAD_PAYLOAD_SIZE : 0;
}

int Esp32RcRadio::textRead(char* buf)
{
    task();
    if (txt_flag) // has data
    {
        if (buf != NULL) {
            // copy to user buffer if available
            strncpy((char*)buf, (const char*)txt_rx_buffer, E32RCRAD_PAYLOAD_SIZE - 2);
        }
        txt_flag = false; // mark as read
        return E32RCRAD_PAYLOAD_SIZE; // report success
    }
    return -1; // report no data
}

void Esp32RcRadio::textSend(char* buf)
{
    strncpy((char*)txt_tx_buffer, (const char*)buf, E32RCRAD_PAYLOAD_SIZE - 2);
    _text_send_timer = _hop_tbl_len * E32RCRAD_TX_RETRANS_TXT; // makes sure the text is sent over all the channels a few times
    _seq_num++; // make sure the text is considered fresh
    if (_seq_num == 0) {
        // roll-over occured, start new session
        _session_id = esp_random();
        _seq_num++;
    }
}
