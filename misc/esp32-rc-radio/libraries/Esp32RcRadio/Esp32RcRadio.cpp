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
    uint8_t  interval;   // transmission interval that the receiver can adapt the timeout
    uint8_t  payload[E32RCRAD_PAYLOAD_SIZE]; // our custom payload
    uint32_t chksum;     // userspace checksum over the payload, accounts for the salt, not to be confused with FCS
                         // combined with the salt, this is meant to prevent impersonation attacks
}
__attribute__ ((packed))
e32rcrad_pkt_t;

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
    _tx_interval = E32RCRAD_TX_INTERV;
    _session_id = 0;
    _seq_num = 0;
    _seq_num_prev = 0;
    _statemachine = 0;
    _stat_tmr = 0;
    _stat_tx_cnt = 0;
    _stat_rx_cnt = 0;
    _stat_txt_cnt = 0;
    _stat_drate = 0;
    _stat_rx_total = 0;
    _stat_rx_good = 0;
    _last_rx_time = 0;
    _last_tx_time = 0;
    _last_rxhop_time = 0;
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
    var_reset();
    _chan_map = chan_map & 0x3FFF;
    _chan_map = _chan_map == 0 ? E32RCRAD_CHAN_MAP_DEFAULT : _chan_map;
    _uid = uid;
    _salt = salt;

    gen_hop_table();

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

    if (_is_tx)
    {
        next_chan();
        #ifdef E32RCRAD_BIDIRECTIONAL
        start_listener();
        _statemachine = E32RCRAD_SM_LISTENING;
        #endif
    }

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
    if (nchan != _last_chan) // prevent wasting time changing to the same channel
    {
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

        if (_is_tx == false) {
            start_listener();
        }
        _last_chan = nchan;
    }
}

void Esp32RcRadio::gen_header(void)
{
    e32rcrad_pkt_t* ptr = (e32rcrad_pkt_t*)frame_buffer;
    //ptr->frame_ctrl = 0x0094; // block ack
    ptr->frame_ctrl = 0x0040; // probe request
    ptr->duration = 0;

    uint32_t r =
        #ifdef E32RCRAD_FINGER_QUOTES_RANDOMNESS
            rand()
        #else
            0
        #endif
        ;
    // random number is used to simply make the packet look like it's encrypted
    // it is not...
    // if the attacker reads this source code, the random number is pretty much useless
    ptr->padding[0] = r;
    ptr->padding[1] = ~r;
    ptr->addrs[0]   = r;
    ptr->addrs[1]   = _uid         ^ r;
    ptr->addrs[2]   = _session_id  ^ r;
    ptr->chan_hint  = _cur_chan    ^ r;
    ptr->seq_num    = _seq_num     ^ r;
    ptr->interval   = _tx_interval ^ r;
    ptr->flags      = 0;

    #ifdef E32RCRAD_BIDIRECTIONAL
    if (_is_tx == false)
    {
        ptr->flags |= (1 << E32RCRAD_FLAG_REPLY);
    }
    else
    {
        if ((_seq_num % (2 + (rand() % 4))) == 0) {
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
        #ifdef E32RCRAD_BIDIRECTIONAL
        if (_statemachine == E32RCRAD_SM_REPLYING || _text_send_timer > 0)
        {
            esp_wifi_set_promiscuous(false);
            tx();
            start_listener();
            _statemachine = E32RCRAD_SM_LISTENING; // reply only once
        }
        #endif

        if ((now - _last_rxhop_time) >= (_tx_interval <= 0 ? 100 : (_tx_interval * 3)))
        {
            // timeout waiting for packet, go to next channel just in case this one is completely useless
            _last_rxhop_time = now;
            next_chan();
        }

        // prevent statistics overflow
        if (_stat_rx_total > 1000 || _stat_rx_good > 1000) {
            _stat_rx_total /= 2;
            _stat_rx_good  /= 2;
        }
    }
    else // _is_tx == true
    {
        #ifdef E32RCRAD_BIDIRECTIONAL
        if (_statemachine == E32RCRAD_SM_LISTENING)
        {
            if ((now - _last_tx_time) >= (_tx_interval - 2))
            {
                _statemachine = E32RCRAD_SM_IDLE;
                esp_wifi_set_promiscuous(false);
            }
        }
        else
        #endif
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
        _stat_tx_cnt = 0;
        _stat_rx_cnt = 0;
        _stat_tmr = now;
    }
}

void Esp32RcRadio::start_listener(void)
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
    esp_wifi_set_promiscuous(true);
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
    if (dbg_ptr[0] == 0x40)
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

    if (parsed->frame_ctrl != 0x0040) { // frame type must match
        return;
    }
    if (rx_uid != _uid) { // UID must match
        return;
    }
    if (rx_sess == 0) { // invalid session ID
        return;
    }

    // verify checksum of payload against claimed checksum
    uint32_t chksum = e32rcrad_getCheckSum(_salt, (uint8_t*)&(u8_ptr[4]), sizeof(e32rcrad_pkt_t) - 8);
    if (parsed->chksum != chksum) {
        return;
    }
    // checksum needs to be valid for the next few security features to work and not be overwhelmed

    if (_is_tx == false) // only receiver requires additional security
    {
        if (_session_id == 0) // first packet ever received
        {
            // start the new session
            _session_id = rx_sess;
            _seq_num_prev = rx_seq;
        }
        else if (_session_id == rx_sess)
        {
            if (rx_seq <= _seq_num_prev) {
                // ignore duplicate or replay-attack
                return;
            }
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

        _tx_interval = parsed->interval ^ r;

        #ifdef E32RCRAD_BIDIRECTIONAL
        if ((rx_flags & (1 << E32RCRAD_FLAG_REPLYREQUEST)) != 0 || _text_send_timer > 0) {
            _statemachine = E32RCRAD_SM_REPLYING; // signal that the tx function needs to run
        }
        #endif
    }
    #ifdef E32RCRAD_BIDIRECTIONAL
    else
    {
        // a reply isn't critical, but make sure the session ID is correct
        if (_session_id != rx_sess) {
            return;
        }

        if (rx_seq <= _seq_num_prev) {
            // ignore duplicate or replay-attack
            return;
        }
        _seq_num_prev = rx_seq;
    }
    #endif

    // all checks pass

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

    if (_is_tx == false)
    {
        _cur_chan = parsed->chan_hint ^ r; // sync with channel hop
        next_chan();
        _last_rxhop_time = now;
    }
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
