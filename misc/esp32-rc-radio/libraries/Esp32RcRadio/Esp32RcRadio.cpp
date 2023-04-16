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
    uint16_t duration;   // note: this field might be changed by API
    uint32_t addrs[3];   // MAC addresses for RX and TX, 6 bytes each, but since we are using 32 bit integers, we can stuff 3 items in here
    uint8_t  padding[2]; // this can be changed by the underlying api
    uint8_t  payload[E32RCRAD_PAYLOAD_SIZE]; // our custom payload
    uint32_t chksum;     // userspace checksum over the payload, accounts for the salt, not to be confused with FCS
                         // combined with the salt, this is meant to prevent impersonation attacks
}
__attribute__ ((packed))
e32rcrad_pkt_t;

static uint8_t buffer[E32RCRAD_PAYLOAD_SIZE];        // application buffer
static uint8_t frame_buffer[sizeof(e32rcrad_pkt_t)]; // the buffer that's actually sent (and also used for rx comparisons)
static bool    rx_flag;                              // indicator of new packet received

static uint8_t hop_table[14]; // channel hop order
static uint8_t hop_tbl_len;   // length of the table

static bool has_wifi_init = false; // only init once

static Esp32RcRadio* instance; // only one instance allowed for the callback

static void promiscuous_handler(void* buf, wifi_promiscuous_pkt_type_t type);
//extern int8_t free80211_send(uint8_t *buffer, uint16_t len);
extern uint32_t _rotl_u32(uint32_t value, int shift);
extern uint32_t esp32rcrad_getCheckSum(uint32_t salt, uint8_t* data, uint8_t len);

Esp32RcRadio::Esp32RcRadio(bool is_tx)
{
    _is_tx = is_tx;
    var_reset();
}

void Esp32RcRadio::var_reset(void)
{
    _stat_tmr_1 = 0;
    _stat_tmr_2 = 0;
    _stat_cnt_1 = 0;
    _stat_cnt_2 = 0;
    _stat_drate_1 = 0;
    _stat_drate_2 = 0;
    _stat_rx_total = 0;
    _stat_rx_good = 0;
    _last_time = 0;
    _timeout_cnt = 0;
    _cur_chan = 0;
    _last_chan = -1;
    rx_flag = false;
}

void Esp32RcRadio::begin(uint16_t chan_map, uint32_t uid, uint32_t salt)
{
    instance = (Esp32RcRadio*)this;
    var_reset();
    _chan_map = chan_map & 0x3FFF;
    _chan_map = _chan_map == 0 ? E32RCRAD_CHAN_MAP_DEFAULT : _chan_map;
    _uid = uid;
    _salt = salt;

    // generate hop table, fill table with an entry if the bit of the map bitmask is set
    int i, j;
    for (i = 0, j = 0; i < 14; i++)
    {
        if ((_chan_map & (1 << i)) != 0) {
            hop_table[j] = i + 1;
            hop_table[j + 1] = 0;
            j++;
            hop_tbl_len = j;
        }
    }
    if (_salt != 0)
    {
        uint32_t old_rand = rand();
        srand(_salt);
        // shuffle the table according to our salt
        // this prevents jamming by following a known hop sequence
        for (i = 0; i < hop_tbl_len - 1; i++)
        {
            j = i + rand() / (RAND_MAX / (hop_tbl_len - i) + 1);
            uint8_t t = hop_table[j];
            hop_table[j] = hop_table[i];
            hop_table[i] = t;
        }
        srand(old_rand); // remove our salt
    }
    #ifdef E32RCRAD_DEBUG_HOPTABLE
    Serial.printf("Hop Table: ");
    for (i = 0; i < hop_tbl_len; i++) {
        Serial.printf("%d  ", hop_table[i]);
    }
    #endif

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
                .channel = hop_table[_cur_chan % hop_tbl_len],
                .authmode = WIFI_AUTH_WPA2_PSK,
                .ssid_hidden = 1,
                .max_connection = 4,
                .beacon_interval = 60000
            }
        };

        esp_wifi_set_config(WIFI_IF_AP, &ap_config);
        //esp_wifi_config_80211_tx_rate(WIFI_IF_AP, WIFI_PHY_RATE_1M_L); // sets PHY rate
        esp_wifi_start();
        esp_wifi_set_ps(WIFI_PS_NONE);
        gen_header();
    }
    else
    {
        next_chan();
    }

    if (_is_tx == false)
    {
        start_listener();
    }
}

void Esp32RcRadio::send(uint8_t* data, bool immediate)
{
    memcpy(buffer, data, E32RCRAD_PAYLOAD_SIZE);
    if (immediate) {
        tx();
    }
}

void Esp32RcRadio::tx(void)
{
    e32rcrad_pkt_t* ptr = (e32rcrad_pkt_t*)frame_buffer;
    memcpy(ptr->payload, buffer, E32RCRAD_PAYLOAD_SIZE);
    ptr->chksum = esp32rcrad_getCheckSum(_salt, ptr->payload, E32RCRAD_PAYLOAD_SIZE);

    int i;
    for (i = 0; i == 0 || i < E32RCRAD_TX_RETRANS; i++)
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

    stat_cnt_up();
    next_chan();
}

void Esp32RcRadio::next_chan(void)
{
    _cur_chan++;
    _cur_chan %= hop_tbl_len;
    uint8_t nchan = hop_table[_cur_chan];
    if (nchan != _last_chan) // prevent wasting time changing to the same channel
    {
        if (_is_tx == false) {
            esp_wifi_set_promiscuous(false);
        }
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
    gen_header();
}

void Esp32RcRadio::gen_header(void)
{
    uint8_t c = hop_table[_cur_chan];
    e32rcrad_pkt_t* ptr = (e32rcrad_pkt_t*)frame_buffer;
    //ptr->frame_ctrl = 0x0094; // block ack
    ptr->frame_ctrl = 0x0040; // probe request
    ptr->duration = 0;
    ptr->padding[0] = 0;
    ptr->padding[1] = 0;
    #ifdef E32RCRAD_OBFUSCATE_ADDRS
    if (_salt != 0)
    {
        // obfuscate the header to look like randomized MACs
        // this is meant to prevent anybody sniffing from recognizing our activity
        ptr->addrs[0] = _rotl_u32(_uid, c);
        ptr->addrs[1] = _rotl_u32(~_uid, ~c);
        ptr->addrs[2] = _rotl_u32(_uid ^ _rotl_u32(_salt, c), c);
    }
    else // no security
    #endif
    {
        ptr->addrs[0] = _uid;
        ptr->addrs[1] = _uid;
        ptr->addrs[2] = _uid;
    }
}

void Esp32RcRadio::task(void)
{
    uint32_t now = millis();
    if (_is_tx == false)
    {
        if ((now - _last_time) >=
            #if E32RCRAD_TX_INTERV > 0
                (E32RCRAD_TX_INTERV * 3)
            #else
                100
            #endif
            )
        {
            // timeout waiting for packet, go to next channel just in case this one is completely useless
            _last_time = now;
            _timeout_cnt++;
            next_chan();
        }

        // prevent statistics overflow
        if (_stat_rx_total > 1000 || _stat_rx_good > 1000) {
            _stat_rx_total /= 2;
            _stat_rx_good   /= 2;
        }
    }
    else // _is_tx == true
    {
        if ((now - _last_time) >= E32RCRAD_TX_INTERV)
        {
            // send queued packet if enough time has passed
            tx();
            // _last_time will be set from inside
        }
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

void Esp32RcRadio::stat_cnt_up(void)
{
    uint32_t now = millis();
    _last_time = now;
    _stat_cnt_1++;
    _stat_cnt_2++;
    if ((now - _stat_tmr_1) >= 2000)
    {
        _stat_drate_1 = _stat_cnt_1 / 2;
        _stat_cnt_1 = 0;
        _stat_tmr_1 = now;
    }
    if ((now - _stat_tmr_1) >= 1000 && _stat_tmr_2 <= 0)
    {
        _stat_cnt_2 = 0;
        _stat_tmr_2 = now;
    }
    if ((now - _stat_tmr_2) >= 2000)
    {
        _stat_drate_2 = _stat_cnt_2 / 2;
        _stat_cnt_2 = 0;
        _stat_tmr_2 = now;
    }
}

uint32_t Esp32RcRadio::get_data_rate(void)
{
    if (_stat_drate_1 <= 0) {
        return _stat_drate_2;
    }
    else if (_stat_drate_2 <= 0) {
        return _stat_drate_1;
    }
    else {
        return (_stat_drate_1 + _stat_drate_2) / 2;
    }
}

static void promiscuous_handler(void* buf, wifi_promiscuous_pkt_type_t type)
{
    instance->handle_rx(buf, type);
}

void Esp32RcRadio::handle_rx(void* buf, wifi_promiscuous_pkt_type_t type)
{
    _stat_rx_total++;
    wifi_promiscuous_pkt_t* pkt = (wifi_promiscuous_pkt_t*)buf;
    wifi_pkt_rx_ctrl_t ctrl = (wifi_pkt_rx_ctrl_t)pkt->rx_ctrl;

    #ifdef E32RCRAD_DEBUG_RX
    uint8_t* dbg_ptr = (uint8_t*)pkt->payload;
    if (dbg_ptr[0] == 0x40)
    {
        Serial.printf("RX[t %u]: ", millis());
        int i;
        for (i = 0; i < ctrl.sig_len; i++)
        {
            Serial.printf("0x%02X ", dbg_ptr[i]);
        }
        Serial.printf("\r\n");
    }
    #endif

    if (ctrl.sig_len < sizeof(e32rcrad_pkt_t)) { // TODO: better length check
        return;
    }
    e32rcrad_pkt_t* parsed = (e32rcrad_pkt_t*)(pkt->payload);
    //uint32_t memadr1 = (int)parsed;
    //uint32_t memadr2 = (int)(parsed->padding);
    //uint32_t hdr_len = memadr2 - memadr1;
    uint32_t hdr_len = 4 + (6 * 2);
    uint32_t i;
    // check header for match, quit on first mismatch, skip 2 "duration" bytes as they are modified by tx api
    // the faster we fail these tests, the less time is wasted on irrelevant packets
    for (i = 0; i < 2; i++) {
        if (frame_buffer[i] != pkt->payload[i]) {
            return;
        }
    }
    for (i = 4; i < hdr_len; i++) {
        if (frame_buffer[i] != pkt->payload[i]) {
            return;
        }
    }
    // verify checksum of payload against claimed checksum
    uint32_t chksum = esp32rcrad_getCheckSum(_salt, parsed->payload, E32RCRAD_PAYLOAD_SIZE);
    if (parsed->chksum != chksum) {
        return;
    }
    // all checks pass, store in buffer and mark as new
    memcpy(buffer, (void*)parsed->payload, E32RCRAD_PAYLOAD_SIZE);
    rx_flag = true;
    stat_cnt_up();
    next_chan();
    reset_timeout_cnt();
    _stat_rx_rssi = ctrl.rssi;
    _stat_rx_good++;
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
            memcpy(data, buffer, E32RCRAD_PAYLOAD_SIZE);
        }
        rx_flag = false; // mark as read
        return E32RCRAD_PAYLOAD_SIZE; // report success
    }
    return -1; // report no data
}
