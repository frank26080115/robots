#include "nRF52RcRadio.h"

#include "nrf_radio.h"
#include "nrf_ppi.h"
#include "nrf_egu.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

    // payload
    uint8_t payload[NRFRR_PAYLOAD_SIZE];

    #ifdef NRFRR_USE_MANUAL_CRC
    uint32_t checksum; // checksum starting from end of the header, excluding self
                       // not to be confused with automatic CRC
    #endif
}
__attribute__ ((packed))
nrfrr_pkt_t;

// nRF52's frequency register doesn't care about 802.15.4 channels, it's actually just raw frequency
// so we only make the "good" ones available
static const uint16_t freq_lookup[] {
#ifdef NRFRR_USE_FREQ_LOWER
    0x100 +  0, // 2360 MHz
    0x100 +  5, // 2365 MHz
    0x100 + 10, // 2370 MHz
    0x100 + 15, // 2375 MHz
    0x100 + 20, // 2380 MHz
    0x100 + 25, // 2385 MHz
    0x100 + 30, // 2390 MHz
    0x100 + 35, // 2395 MHz
#endif
    0x000 + 25, // 2425 MHz
#ifdef NRFRR_USE_FREQ_EUROPEAN
    0x000 + 30, // 2430 MHz
#endif
#ifdef NRFRR_USE_FREQ_NORTHAMERICAN
    0x000 + 50, // 2450 MHz
#endif
#ifdef NRFRR_USE_FREQ_EUROPEAN
    0x000 + 55, // 2455 MHz
    0x000 + 60, // 2460 MHz
#endif
#ifdef NRFRR_USE_FREQ_NORTHAMERICAN
    0x000 + 75, // 2475 MHz
    0x000 + 80, // 2480 MHz
#endif
#ifdef NRFRR_USE_FREQ_UPPER
    // note: wifi chan 13 is 2472 MHz, ending at 2483 MHz
    0x000 + 90,  // 2490 MHz
    0x000 + 95,  // 2495 MHz
    0x000 + 100, // 2500 MHz
#endif
};

static uint8_t tx_buffer[NRFRR_PAYLOAD_SIZE];          // application buffer
static uint8_t rx_buffer[NRFRR_PAYLOAD_SIZE];          // application buffer
static uint8_t frame_buffer[sizeof(nrfrr_pkt_t) + 32]; // the buffer that's used by underlying hardware
static bool    rx_flag;                                // indicator of new packet received

static char    txt_rx_buffer[NRFRR_PAYLOAD_SIZE]; // application text buffer
static char    txt_tx_buffer[NRFRR_PAYLOAD_SIZE]; // application text buffer
static bool    txt_flag;                          // indicator of text packet received

static uint32_t invalid_sessions[NRFRR_REM_USED_SESSIONS];

static bool hw_inited = false; // only init once

static nRF52RcRadio* instance; // only one instance allowed for the callback

extern uint32_t nrfrr_getCheckSum(uint32_t salt, uint8_t* data, uint8_t len);
extern uint32_t nrfrr_rand(void);

nRF52RcRadio::nRF52RcRadio(bool is_tx)
{
    _is_tx = is_tx;
    memset(invalid_sessions, 0, sizeof(uint32_t) * NRFRR_REM_USED_SESSIONS);
    var_reset();
}

void nRF52RcRadio::var_reset(void)
{
    _tx_interval = NRFRR_TX_INTERV_DEF;
    _session_id = 0;
    _seq_num = 0;
    _seq_num_prev = 0;
    _statemachine = 0;
    _statemachine_next = 0;
    _stat_tmr = 0;
    _stat_tx_cnt = 0;
    _stat_rx_cnt = 0;
    _stat_txt_cnt = 0;
    _stat_drate = 0;
    _stat_rx_lost = 0;
    _stat_loss_rate = 0;
    #ifdef NRFRR_DEBUG_RX_ERRSTATS
    memset(_stat_rx_errs, 0, 4*12);
    #endif
    _stat_rx_total = 0;
    _stat_rx_good = 0;
    _last_rx_time = 0;
    _last_tx_time = 0;
    _last_hop_time = 0;
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

void nRF52RcRadio::begin(uint32_t chan_map, uint32_t uid, uint32_t salt)
{
    instance = (nRF52RcRadio*)this;

    #ifdef NRFRR_DEBUG_PINS
    pinMode     (NRFRR_DEBUG_PIN_TX, OUTPUT);
    digitalWrite(NRFRR_DEBUG_PIN_TX, _dbgpin_tx = LOW);
    pinMode     (NRFRR_DEBUG_PIN_RX, OUTPUT);
    digitalWrite(NRFRR_DEBUG_PIN_RX, _dbgpin_rx = LOW);
    pinMode     (NRFRR_DEBUG_PIN_CH, OUTPUT);
    digitalWrite(NRFRR_DEBUG_PIN_CH, _dbgpin_ch = LOW);
    #endif

    var_reset();
    _uid = uid;
    _salt = salt;

    set_chan_map(chan_map);

    if (hw_inited == false) {
        init_hw();
        hw_inited = true;
    }

    // Power on into disabled mode
    NRF_RADIO->EVENTS_DISABLED = 0;
    NRF_RADIO->TASKS_DISABLE = 1;
    while (NRF_RADIO->EVENTS_DISABLED == 0) {
        yield(); // wait for the radio to be disabled
    }
    NRF_RADIO->EVENTS_END = 0;

    set_net_addr(_uid);

    _cur_chan = -1;
    next_chan();

    set_encryption();

    // Configure the automatic CRC
    #ifdef NRFRR_USE_MANUAL_CRC
    NRF_RADIO->CRCCNF = (RADIO_CRCCNF_LEN_One << RADIO_CRCCNF_LEN_Pos);
    NRF_RADIO->CRCINIT = 0xFFFFUL;
    #else
    NRF_RADIO->CRCCNF = (RADIO_CRCCNF_LEN_Two << RADIO_CRCCNF_LEN_Pos);
    NRF_RADIO->CRCINIT = _salt;
    #endif
    NRF_RADIO->CRCPOLY = 0x11021UL;     // CRC poly: x^16+x^12^x^5+1

    if (_is_tx)
    {
        while (_session_id == 0) {
            _session_id = nrfrr_rand();
        }
    }
}

void nRF52RcRadio::init_hw(void)
{
    if ((NRF_CLOCK->HFCLKSTAT & CLOCK_HFCLKSTAT_STATE_Msk) == 0)
    {
        // Enable the High Frequency clock to the system as a whole
        NRF_CLOCK->EVENTS_HFCLKSTARTED = 0;
        NRF_CLOCK->TASKS_HFCLKSTART = 1;
        // Wait for the external oscillator to start up
        while (NRF_CLOCK->EVENTS_HFCLKSTARTED == 0) {
            yield();
        }
    }

    // Disable and reset the radio
    NRF_RADIO->POWER = RADIO_POWER_POWER_Disabled;
    NRF_RADIO->POWER = RADIO_POWER_POWER_Enabled;
    NRF_RADIO->EVENTS_DISABLED = 0;
    NRF_RADIO->TASKS_DISABLE   = 1;
    // Wait until we are in DISABLE state
    while (NRF_RADIO->EVENTS_DISABLED == 0) {
        yield();
    }

    // Physical on-air address is set in PREFIX0 + BASE0 by set_net_addr
    NRF_RADIO->TXADDRESS    = 0x00;	// Use logical address 0 (PREFIX0 + BASE0)
    NRF_RADIO->RXADDRESSES  = 0x01;	// Enable reception on logical address 0 (PREFIX0 + BASE0)

    // These shorts will make the radio transition from Ready to Start to Disable automatically
    // for both TX and RX, which makes for much shorter on-air times
    NRF_RADIO->SHORTS = (RADIO_SHORTS_READY_START_Enabled << RADIO_SHORTS_READY_START_Pos)
                      | (RADIO_SHORTS_END_DISABLE_Enabled << RADIO_SHORTS_END_DISABLE_Pos)
                      ;

    NRF_RADIO->PCNF0 =   (8 << RADIO_PCNF0_LFLEN_Pos)  // Payload size length in bits
                       | (1 << RADIO_PCNF0_S0LEN_Pos)  // S0 is 1 octet
                       | (8 << RADIO_PCNF0_S1LEN_Pos); // S1 is 1 octet

    NRF_RADIO->MODE    = ((
        #if defined(RADIO_MODE_MODE_Nrf_250Kbit) && false
            RADIO_MODE_MODE_Nrf_250Kbit
        #else
            RADIO_MODE_MODE_Nrf_1Mbit
            // TODO: test 2Mbit to see reliability
        #endif
        << RADIO_MODE_MODE_Pos) & RADIO_MODE_MODE_Msk);
    NRF_RADIO->TXPOWER = ((
        #ifdef RADIO_TXPOWER_TXPOWER_Pos8dBm
            RADIO_TXPOWER_TXPOWER_Pos8dBm
        #else
            RADIO_TXPOWER_TXPOWER_Pos4dBm
        #endif
        << RADIO_TXPOWER_TXPOWER_Pos) & RADIO_TXPOWER_TXPOWER_Msk);

    #ifdef NRFRR_USE_INTERRUPTS
    NVIC_SetPriority(RADIO_IRQn, 0);
    NVIC_ClearPendingIRQ(RADIO_IRQn);
    NRF_RADIO->INTENSET = RADIO_INTENSET_END_Msk | RADIO_INTENSET_DISABLED_Msk | RADIO_INTENSET_ADDRESS_Msk | RADIO_INTENSET_RSSIEND_Msk;
    #endif
}

#ifdef NRFRR_USE_INTERRUPTS
void RADIO_IRQHandler_CPP(void)
{
    instance->state_machine_run(millis(), true);
}

#ifdef __cplusplus
extern "C" {
#endif

void RADIO_IRQHandler(void)
{
    RADIO_IRQHandler_CPP();
}

#ifdef __cplusplus
}
#endif
#endif

void nRF52RcRadio::set_net_addr(uint32_t x)
{
    uint8_t* address = (uint8_t*)&x;
    int len = 4;

    // First byte is the prefix, remainder are base
    NRF_RADIO->PREFIX0 = ((address[0] << RADIO_PREFIX0_AP0_Pos) & RADIO_PREFIX0_AP0_Msk);
    uint32_t base;
    memcpy(&base, address+1, len-1);
    NRF_RADIO->BASE0 = base;

    NRF_RADIO->PCNF1 =  (
        (((sizeof(nrfrr_pkt_t))  << RADIO_PCNF1_MAXLEN_Pos)  & RADIO_PCNF1_MAXLEN_Msk)  // maximum length of payload
        | (((0UL)                << RADIO_PCNF1_STATLEN_Pos) & RADIO_PCNF1_STATLEN_Msk)	// expand the payload with 0 bytes
        | (((len-1)              << RADIO_PCNF1_BALEN_Pos)   & RADIO_PCNF1_BALEN_Msk)); // base address length in number of bytes.
}

void nRF52RcRadio::set_encryption(void)
{
    if (_salt != 0 && (_salt & 0x80000000) != 0) // use the last bit in the salt to indicate if encryption is required
    {
        // fake an encryption key with existing information
        // TODO: maybe add a way to load a real key from flash file
        uint32_t tmp32[4];
        tmp32[0] = _uid;
        tmp32[1] = _salt;
        tmp32[2] = nrfrr_getCheckSum(0, (uint8_t*)tmp32, 8);
        tmp32[3] = nrfrr_getCheckSum(0, (uint8_t*)&(tmp32[1]), 8);
        uint8_t* tmp8 = (uint8_t*)tmp32;

        int i, j;
        for (i = 0, j = 0; i < RH_NRF51_AES_CCM_CNF_SIZE; i++)
        {
            _encryption_cnf[i] = tmp8[j];
            j = (j + 1) % (4 * 4);
        }

        // AES configuration data area
        NRF_CCM->CNFPTR  = (uint32_t)_encryption_cnf;

        // Set AES CCM input and putput buffers
        // Make sure the _buf is encrypted and put back into _buf
        NRF_CCM->INPTR  = (uint32_t)frame_buffer;
        NRF_CCM->OUTPTR = (uint32_t)frame_buffer;
        // Also need to set SCRATCHPTR temp buffer os size 16+MAXPACKETSIZE in RAM
        // FIXME: shared buffers if several radios
        NRF_CCM->SCRATCHPTR = (uint32_t)_scratch;

        // SHORT from RADIO READY to AESCCM KSGEN using PPI predefined channel 24
        // Also RADIO ADDRESS to AESCCM CRYPT using PPI predefined channel 25
        NRF_PPI->CHENSET =   (PPI_CHENSET_CH24_Enabled << PPI_CHENSET_CH24_Pos)
                           | (PPI_CHENSET_CH25_Enabled << PPI_CHENSET_CH25_Pos)
            ;

        // SHORT from AESCCM ENDKSGEN to AESCCM CRYPT
        NRF_CCM->SHORTS = (CCM_SHORTS_ENDKSGEN_CRYPT_Enabled << CCM_SHORTS_ENDKSGEN_CRYPT_Pos);

        // Enable the CCM module
        NRF_CCM->ENABLE = (CCM_ENABLE_ENABLE_Enabled << CCM_ENABLE_ENABLE_Pos);

        _encrypting = true;
    }
    else
    {
        // Disable the CCM module
        NRF_CCM->ENABLE = (CCM_ENABLE_ENABLE_Disabled << CCM_ENABLE_ENABLE_Pos);
        _encrypting = false;
    }
}

void nRF52RcRadio::gen_hop_table(void)
{
    // generate hop table, fill table with an entry if the bit of the map bitmask is set
    _hop_table = (uint8_t*)malloc((sizeof(freq_lookup) / 2) + 1);
    int i, j;
    for (i = 0, j = 0; i < (sizeof(freq_lookup) / 2); i++)
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
        tmp32[2] = nrfrr_getCheckSum(0, (uint8_t*)tmp32, 8);
        tmp32[3] = nrfrr_getCheckSum(0, (uint8_t*)&(tmp32[1]), 8);
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
    #ifdef NRFRR_DEBUG_HOPTABLE
    Serial.printf("Hop Table: ");
    for (i = 0; i < _hop_tbl_len; i++) {
        Serial.printf("%d  ", _hop_table[i]);
    }
    #endif
    _cur_chan %= _hop_tbl_len;
}

void nRF52RcRadio::set_chan_map(uint32_t x)
{
    _single_chan_mode = (x & 0x80000000) != 0;
    uint32_t i, m;
    for (i = 0, m = 0; i < (sizeof(freq_lookup) / 2); i++)
    {
        m |= 1 << i;
    }
    _chan_map = x & m;
    _chan_map = _chan_map == 0 ? NRFRR_CHAN_MAP_DEFAULT : _chan_map;
    gen_hop_table();
}

void nRF52RcRadio::set_tx_interval(uint16_t x)
{
    _tx_interval = x > NRFRR_TX_INTERV_MAX ? NRFRR_TX_INTERV_MAX : (x < NRFRR_TX_INTERV_MIN ? NRFRR_TX_INTERV_MIN : x);
}

void nRF52RcRadio::send(uint8_t* data)
{
    memcpy(tx_buffer, data, NRFRR_PAYLOAD_SIZE);
}

void nRF52RcRadio::prep_tx(void)
{
    bool is_text = _text_send_timer > 0;

    nrfrr_pkt_t* ptr = (nrfrr_pkt_t*)frame_buffer;
    gen_header();
    if (is_text) {
        ptr->flags ^= (1 << NRFRR_FLAG_TEXT);
    }

    uint8_t* src_buf = is_text ? (uint8_t*)txt_tx_buffer : (uint8_t*)tx_buffer;

    memcpy(ptr->payload, src_buf, NRFRR_PAYLOAD_SIZE);

    #ifdef NRFRR_USE_MANUAL_CRC
    ptr->chksum = nrfrr_getCheckSum(_salt, &(frame_buffer[NRFRR_CHECKSUM_START_IDX]), sizeof(nrfrr_pkt_t) - 4 - NRFRR_CHECKSUM_START_IDX);
    #endif

    #ifdef NRFRR_DEBUG_TX
    Serial.printf("TX[t %u][c %u][s %u]: ", millis(), hop_table[_cur_chan], sizeof(nrfrr_pkt_t));
    int i;
    for (i = 0; i < sizeof(nrfrr_pkt_t); i++)
    {
        Serial.printf("0x%02X ", frame_buffer[i]);
    }
    Serial.printf("\r\n");
    #endif

    _stat_tx_cnt++;
    _last_tx_time = millis();

    #ifdef NRFRR_DEBUG_HOP
    Serial.printf("TX %u %u\r\n", _last_tx_time, _seq_num);
    #endif

    if (is_text)
    {
        // text needs to be repeatedly sent for redundancy but we also want to avoid duplicates
        // so we do not increment sequence number, unless we have finished sending the text
        _text_send_timer--;
        if (_text_send_timer <= 0) {
            _seq_num++;
            _text_send_timer = 0;
        }
    }
    else
    {
        // increment sequence number since it's not text, we want to avoid duplicates
        _seq_num++;
    }

    if (_seq_num == 0) {
        // roll-over occured, start new session
        _session_id = nrfrr_rand();
        _seq_num++;
    }
}

void nRF52RcRadio::next_chan(void)
{
    _cur_chan++;
    _cur_chan %= _hop_tbl_len;
    uint8_t nchan = _hop_table[_cur_chan] - 1;
    //if (nchan != _last_chan) // prevent wasting time changing to the same channel
    {
        uint16_t freq = freq_lookup[nchan];

        #ifdef NRFRR_DEBUG_HOP
        Serial.printf("HOP %u %u %u %u\r\n", millis(), _cur_chan, nchan, ((freq & 0x100) != 0) ? (2360 + (freq & 0xFF)) : (2400 + freq));
        #endif

        NRF_RADIO->FREQUENCY = freq;

        #ifdef NRFRR_DEBUG_PINS
        digitalWrite(NRFRR_DEBUG_PIN_CH, _dbgpin_ch = (_dbgpin_ch == LOW ? HIGH : LOW));
        #endif

        _last_chan = nchan;
    }

    _last_hop_time = millis();
}

void nRF52RcRadio::gen_header(void)
{
    uint32_t now = millis();

    nrfrr_pkt_t* ptr = (nrfrr_pkt_t*)frame_buffer;

    ptr->s0 = 0;
    ptr->s1 = 0;
    ptr->len = sizeof(nrfrr_pkt_t);

    uint32_t r =
        #ifdef NRFRR_FINGER_QUOTES_RANDOMNESS
            nrfrr_rand()
            // TODO: check speed of RNG
        #else
            0
        #endif
        ;
    // the random number is used to make the MAC addresses random each time just in case it's a match for a real device
    // we don't want that device to actually be confused
    // it also makes the packet look like it's encrypted
    // it is not...
    // if the attacker reads this source code, the random number is pretty much useless
    #ifdef NRFRR_FINGER_QUOTES_RANDOMNESS
    ptr->rand       = r;
    #endif
    ptr->uid        = _uid         ^ r;
    ptr->session    = _session_id  ^ r;
    ptr->chan_hint  = _cur_chan    ^ r;
    ptr->seq_num    = _seq_num     ^ r;
    #ifdef NRFRR_ADAPTIVE_INTERVAL
    ptr->interval   = _tx_interval ^ r;
    #endif
    ptr->flags      = 0;

    #ifdef NRFRR_BIDIRECTIONAL
    if (_is_tx == false)
    {
        ptr->flags |= (1 << NRFRR_FLAG_REPLY);
    }
    else
    {
        if ((now - _tx_replyreq_tmr) >= _reply_request_rate && _reply_request_rate != 0) {
            _reply_request_latch = true;
            _tx_replyreq_tmr = now;
        }
        if (_reply_request_latch) {
            ptr->flags |= (1 << NRFRR_FLAG_REPLYREQUEST);
        }
    }
    #endif

    ptr->flags ^= r;
}

bool nRF52RcRadio::state_machine_run(uint32_t now, bool is_isr)
{
    bool ret = false; // return true for "run again"

    #ifdef NRFRR_USE_INTERRUPTS
    if (is_isr == false) {
        __disable_irq();
    }
    #endif

    #ifdef NRFRR_USE_INTERRUPTS
    if (is_isr)
    {
        if (NRF_RADIO->EVENTS_END)
        {
            if (_statemachine >= NRFRR_SM_TX_START && _statemachine <= NRFRR_SM_TX_WAIT)
            {
                NRF_RADIO->EVENTS_END = 0;
                NRF_RADIO->EVENTS_DISABLED = 0;
                NRF_RADIO->TASKS_DISABLE = 1;
                if (_is_tx)
                {
                    _statemachine = NRFRR_SM_RX_START;
                }
                else
                {
                    _statemachine = NRFRR_SM_HOP_START;
                    _statemachine_next = NRFRR_SM_RX_START;
                }
                _sm_time = now;
                _sm_evt_time = now;
                ret = true;
            }
            else if (_statemachine >= NRFRR_SM_RX_START && _statemachine <= NRFRR_SM_RX_END_WAIT2)
            {
                NRF_RADIO->EVENTS_END = 0;
                NRF_RADIO->EVENTS_DISABLED = 0;
                NRF_RADIO->TASKS_DISABLE = 1;
                NRF_RADIO->TASKS_RSSISTOP = 1;
                if (_encrypting) {
                    _statemachine = NRFRR_SM_RX_END_WAIT;
                }
                else {
                    _statemachine = NRFRR_SM_RX_END_WAIT;
                }
                _rx_time_cache = now;
                _sm_time = now;
                _sm_evt_time = now;
                ret = true;
            }
        }
        else if (NRF_RADIO->EVENTS_DISABLED)
        {
            if (_statemachine == NRFRR_SM_RX_END_WAIT)
            {
                NRF_RADIO->EVENTS_DISABLED = 0;
                NRF_RADIO->EVENTS_END = 0;
                if (_encrypting) {
                    _statemachine = NRFRR_SM_RX_END_WAIT2;
                }
                else
                {
                    _statemachine = NRFRR_SM_RX_END; // this will cause validate_rx() to run in this ISR
                    ret = true;
                }
                _sm_time = now;
            }
        }
        else if (NRF_RADIO->EVENTS_ADDRESS)
        {
            if (_statemachine >= NRFRR_SM_RX_START && _statemachine <= NRFRR_SM_RX_END_WAIT2)
            {
                NRF_RADIO->EVENTS_ADDRESS = 0;
                NRF_RADIO->TASKS_RSSISTART = 1;
            }
        }
        else if (NRF_RADIO->EVENTS_RSSIEND)
        {
            NRF_RADIO->EVENTS_RSSIEND = 0;
            NRF_RADIO->TASKS_RSSISTOP = 1;
        }
    }
    #endif

    switch(_statemachine)
    {
        case NRFRR_SM_IDLE_WAIT:
            if (NRF_RADIO->STATE == NRF_RADIO_STATE_DISABLED)
            {
                NRF_RADIO->EVENTS_DISABLED = 0;
                NRF_RADIO->EVENTS_END = 0;
                _statemachine = NRFRR_SM_IDLE;
                _sm_time = now;
                _sm_evt_time = now;
                ret = true;
            }
            break;
        case NRFRR_SM_IDLE:
            if (_is_tx)
            {
                if ((now - _last_tx_time) >= _tx_interval)
                {
                    prep_tx();
                    _statemachine = NRFRR_SM_TX_START;
                    _sm_time = now;
                }
            }
            else
            {
                //prep_tx();
                //_statemachine = NRFRR_SM_TX_START;
                _statemachine = NRFRR_SM_RX_START;
                _sm_time = now;
            }
            ret = true;
            break;
        case NRFRR_SM_TX_START_DELAY:
            if ((now - _sm_evt_time) >= 2)
            {
                _statemachine = NRFRR_SM_TX_START;
                _sm_time = now;
                ret = true;
            }
            break;
        case NRFRR_SM_TX_START:
            // Maybe set the AES CCA module for the correct encryption mode
            if (_encrypting) {
                NRF_CCM->MODE = (CCM_MODE_MODE_Encryption << CCM_MODE_MODE_Pos); // Encrypt
            }
            // Radio will transition automatically to Disable state at the end of transmission
            NRF_RADIO->PACKETPTR = (uint32_t)frame_buffer;
            NRF_RADIO->EVENTS_READY = 0;
            NRF_RADIO->EVENTS_END = 0; // So we can detect end of transmission
            NRF_RADIO->TASKS_TXEN = 1;

            #ifdef NRFRR_DEBUG_PINS
            digitalWrite(NRFRR_DEBUG_PIN_TX, HIGH);
            #endif

            _statemachine = NRFRR_SM_TX_WAIT;
            _sm_time = now;
            break;
        case NRFRR_SM_TX_WAIT:
            if (NRF_RADIO->EVENTS_END || NRF_RADIO->EVENTS_DISABLED)
            {
                NRF_RADIO->EVENTS_END = 0;
                NRF_RADIO->EVENTS_DISABLED = 0;
                NRF_RADIO->TASKS_DISABLE = 1;

                #ifdef NRFRR_DEBUG_PINS
                digitalWrite(NRFRR_DEBUG_PIN_TX, LOW);
                #endif

                if (_is_tx)
                {
                    #ifdef NRFRR_BIDIRECTIONAL
                    _statemachine = NRFRR_SM_RX_START;
                    #else
                    _statemachine = NRFRR_SM_HOP_START;
                    _statemachine_next = NRFRR_SM_IDLE_WAIT;
                    #endif
                }
                else
                {
                    _statemachine = NRFRR_SM_HOP_START;
                    _statemachine_next = NRFRR_SM_RX_START;
                }
                _sm_time = now;
                _sm_evt_time = now;
                ret = true;
            }
            break;
        case NRFRR_SM_RX_START:
            if (NRF_RADIO->STATE == NRF_RADIO_STATE_DISABLED)
            {
                NRF_RADIO->EVENTS_DISABLED = 0;
                NRF_RADIO->EVENTS_END = 0;
                // Maybe set the AES CCA module for the correct encryption mode
                if (_encrypting) {
                    NRF_CCM->MODE = (CCM_MODE_MODE_Decryption << CCM_MODE_MODE_Pos); // Decrypt
                }
                //NRF_CCM->MICSTATUS = 0;
                // Radio will transition automatically to Disable state when a message is received
                NRF_RADIO->PACKETPTR = (uint32_t)frame_buffer;
                NRF_RADIO->EVENTS_READY = 0;
                NRF_RADIO->TASKS_RXEN = 1;
                NRF_RADIO->EVENTS_END = 0; // So we can detect end of reception
                _statemachine = NRFRR_SM_RX;
                _sm_time = now;
                _sm_evt_time = now;
            }
            break;
        case NRFRR_SM_RX:
            if (NRF_RADIO->EVENTS_ADDRESS)
            {
                NRF_RADIO->EVENTS_ADDRESS = 0;
                NRF_RADIO->TASKS_RSSISTART = 1;
            }
            if (NRF_RADIO->EVENTS_RSSIEND)
            {
                NRF_RADIO->EVENTS_RSSIEND = 0;
                NRF_RADIO->TASKS_RSSISTOP = 1;
            }

            if (NRF_RADIO->EVENTS_END)
            {
                NRF_RADIO->EVENTS_END = 0;
                NRF_RADIO->EVENTS_DISABLED = 0;
                NRF_RADIO->TASKS_DISABLE = 1;
                NRF_RADIO->TASKS_RSSISTOP = 1;
                _statemachine = NRFRR_SM_RX_END_WAIT;
                _rx_time_cache = now;
                _sm_time = now;
                _sm_evt_time = now;
                ret = true;
                break;
            }
            #ifdef NRFRR_BIDIRECTIONAL
            if (_is_tx)
            {
                if ((now - _last_tx_time) >= (_tx_interval - 2)) // timeout waiting for a reply
                {
                    NRF_RADIO->EVENTS_DISABLED = 0;
                    NRF_RADIO->TASKS_DISABLE = 1;
                    _statemachine = NRFRR_SM_HOP_START;
                    _statemachine_next = NRFRR_SM_IDLE_WAIT;
                    _sm_time = now;
                    break;
                }
            }
            else
            // note: without bidirectional support, a transmitter is never in the RX state
            #endif
            {
                if (_rx_miss_cnt == 0)
                {
                    // when we lose just one packet, assume the same hopping schedule, but turn on the receiver just a tad earlier
                    if ((now - _last_rx_time) >= (_tx_interval + (_tx_interval / 2)))
                    {
                        _rx_miss_cnt++;
                        _statemachine = NRFRR_SM_HOP_START;
                        _statemachine_next = NRFRR_SM_RX_START;
                        _sm_time = now;
                        break;
                    }
                }
                else
                {
                    uint32_t timeout = ((now - _last_rx_time) <= 1000) ? _tx_interval : (_tx_interval * 3);

                    // when we lose just one packet, probably just follow the same hop schedule for a while
                    // but after a while, the transmitter might've been rebooted
                    // so it's better to slow down the hopping, so the transmitter can get back in sync
                    // (not hopping at all is another option but what if that channel is just bad?)

                    if ((now - _last_hop_time) >= timeout)
                    {
                        _rx_miss_cnt++;
                        _statemachine = NRFRR_SM_HOP_START;
                        _statemachine_next = NRFRR_SM_RX_START;
                        _sm_time = now;
                        break;
                    }
                }
            }
            break;
        case NRFRR_SM_RX_END_WAIT:
            if (NRF_RADIO->STATE == NRF_RADIO_STATE_DISABLED)
            {
                NRF_RADIO->EVENTS_DISABLED = 0;
                NRF_RADIO->EVENTS_END = 0;
                if (_encrypting) {
                    _statemachine = NRFRR_SM_RX_END_WAIT2;
                }
                else
                {
                    _statemachine = NRFRR_SM_RX_END;
                    ret = true;
                }
                _sm_time = now;
            }
            break;
        case NRFRR_SM_RX_END_WAIT2:
            // this wait state is for the decryption to finish
            if ((now - _sm_evt_time) > 2)
            {
                _statemachine = NRFRR_SM_RX_END;
                _sm_time = now;
                ret = true;
            }
            break;
        case NRFRR_SM_RX_END:
            if (validate_rx())
            {
                if (_is_tx)
                {
                    if (_single_chan_mode == false) {
                        next_chan();
                    }
                    _statemachine = NRFRR_SM_CHAN_WAIT;
                    _statemachine_next = NRFRR_SM_IDLE;
                    _sm_time = now;
                }
                else
                {
                    #ifdef NRFRR_BIDIRECTIONAL
                    if (_reply_requested)
                    {
                        prep_tx();
                        _statemachine = NRFRR_SM_TX_START_DELAY;
                    }
                    else
                    #endif
                    {
                        if (_single_chan_mode == false) {
                            next_chan();
                        }
                        _statemachine = NRFRR_SM_CHAN_WAIT;
                        _statemachine_next = NRFRR_SM_RX_START;
                    }
                    _sm_time = now;
                }
            }
            else
            {
                _statemachine = NRFRR_SM_RX_START; // this will go to IDLE and TX if not enough time left
                _sm_time = now;
                ret = true;
            }
            break;
        case NRFRR_SM_HOP_START:
            if (NRF_RADIO->STATE != NRF_RADIO_STATE_DISABLED)
            {
                NRF_RADIO->EVENTS_DISABLED = 0;
                NRF_RADIO->TASKS_DISABLE = 1;
            }
            _statemachine = NRFRR_SM_HOP_WAIT;
            _sm_time = now;
            ret = true;
            break;
        case NRFRR_SM_HOP_WAIT:
            if (NRF_RADIO->STATE == NRF_RADIO_STATE_DISABLED)
            {
                NRF_RADIO->EVENTS_END = 0;
                if (_single_chan_mode == false) {
                    next_chan();
                }
                _statemachine = NRFRR_SM_CHAN_WAIT;
                _sm_time = now;
                _sm_evt_time = now;
            }
            break;
        case NRFRR_SM_CHAN_WAIT:
            if (
                (now - _sm_time) >= 2
                || (_is_tx && (now - _last_tx_time) >= _tx_interval)
                )
            {
                _statemachine = _statemachine_next;
                _sm_time = now;
                ret = true;
            }
            break;
        case NRFRR_SM_HALT_START:
            if (NRF_RADIO->STATE != NRF_RADIO_STATE_DISABLED)
            {
                NRF_RADIO->EVENTS_DISABLED = 0;
                NRF_RADIO->TASKS_DISABLE = 1;
            }
            _statemachine = NRFRR_SM_HALT_WAIT;
            _sm_time = now;
            break;
         case NRFRR_SM_CONT_TX:
            break;
    }

    #ifdef NRFRR_USE_INTERRUPTS
    if (is_isr == false) {
        __enable_irq();
    }
    #endif

    return ret;
}

bool nRF52RcRadio::is_busy(void)
{
    if (_statemachine == NRFRR_SM_TX_WAIT) {
        return true;
    }
    if (_statemachine == NRFRR_SM_RX) {
        if ((millis() - _last_tx_time) < (_tx_interval - 4)) {
            return true;
        }
    }
    return false;
}

void nRF52RcRadio::task(void)
{
    uint32_t now;

    while (state_machine_run(now = millis(), false)) {
        yield();
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

bool nRF52RcRadio::validate_rx(void)
{
    uint32_t now = millis();

    _stat_rx_total++;

    uint8_t* u8_ptr = (uint8_t*)frame_buffer;
    nrfrr_pkt_t* parsed = (nrfrr_pkt_t*)u8_ptr;

    #ifdef NRFRR_DEBUG_RX
    Serial.printf("RX[t %u]: ", millis());
    int i;
    for (i = 0; i < parsed->len; i++)
    {
        Serial.printf("0x%02X ", u8_ptr[i]);
    }
    Serial.printf("\r\n");
    #endif

    if (parsed->len < sizeof(nrfrr_pkt_t) - 4 || parsed->len > sizeof(nrfrr_pkt_t) + 4) {
        #ifdef NRFRR_DEBUG_RX_ERRSTATS
        _stat_rx_errs[0]++;
        #endif
        return false;
    }

    uint32_t r = 
        #ifdef NRFRR_FINGER_QUOTES_RANDOMNESS
            parsed->rand // extract the meaningless random number to use to recover other data fields
        #else
            0
        #endif
        ;
    uint32_t rx_uid   = parsed->uid     ^ r;
    uint32_t rx_sess  = parsed->session ^ r;
    uint8_t  rx_flags = parsed->flags   ^ r;
    uint32_t rx_seq   = parsed->seq_num ^ r;

    if (rx_uid != (_uid & 0x00FFFFFF)) { // UID must match
        #ifdef NRFRR_DEBUG_RX_ERRSTATS
        _stat_rx_errs[2]++;
        #endif
        return false;
    }
    if (rx_sess == 0) { // invalid session ID
        #ifdef NRFRR_DEBUG_RX_ERRSTATS
        _stat_rx_errs[3]++;
        #endif
        return false;
    }

    #ifdef NRFRR_USE_MANUAL_CRC
    // verify checksum of payload against claimed checksum
    uint32_t chksum = nrfrr_getCheckSum(_salt, &(u8_ptr[NRFRR_CHECKSUM_START_IDX]), sizeof(nrfrr_pkt_t) - 4 - NRFRR_CHECKSUM_START_IDX);
    if (parsed->chksum != chksum)
    #else
    if (!NRF_RADIO->CRCSTATUS)
    #endif
    {
        #ifdef NRFRR_DEBUG_RX_ERRSTATS
        _stat_rx_errs[4]++;
        #endif
        return false;
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
            if (rx_seq <= _seq_num_prev)
            {
                // ignore duplicate or replay-attack
                // still do a channel hop if it's a text
                if ((rx_flags & (1 << NRFRR_FLAG_TEXT)) != 0) {
                    _cur_chan = parsed->chan_hint ^ r; // sync with channel hop
                    if (_single_chan_mode == false) {
                        next_chan();
                    }
                    _last_rx_time = now;
                }
                #ifdef NRFRR_DEBUG_RX_ERRSTATS
                _stat_rx_errs[5]++;
                #endif
                return false;
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
            for (i = 0; i < NRFRR_REM_USED_SESSIONS; i++)
            {
                uint32_t j = invalid_sessions[i];
                if (j == rx_sess) { // is invalid
                    #ifdef NRFRR_DEBUG_RX_ERRSTATS
                    _stat_rx_errs[6]++;
                    #endif
                    return false;
                }
                else if (j == 0) {
                    break;
                }
            }
            // store old session as invalid
            if (invalid_sessions[NRFRR_REM_USED_SESSIONS - 1] == 0) // enough space
            {
                // store in next empty slot
                for (i = 0; i < NRFRR_REM_USED_SESSIONS; i++)
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
                invalid_sessions[rand() % NRFRR_REM_USED_SESSIONS] = _session_id;
            }
            // start new session
            _session_id = rx_sess;
            _seq_num_prev = rx_seq;
        }

        #ifdef NRFRR_ADAPTIVE_INTERVAL
        _tx_interval = parsed->interval ^ r;
        #endif

        #ifdef NRFRR_BIDIRECTIONAL
        if ((rx_flags & (1 << NRFRR_FLAG_REPLYREQUEST)) != 0 || _text_send_timer > 0) {
            _reply_requested = true;
        }
        else
        {
            _reply_requested = false;
        }
        #endif
    }
    #ifdef NRFRR_BIDIRECTIONAL
    else // _is_tx == true
    {
        // a reply isn't critical, but make sure the session ID is correct
        if (_session_id != rx_sess) {
            #ifdef NRFRR_DEBUG_RX_ERRSTATS
            _stat_rx_errs[7]++;
            #endif
            return false;
        }

        if (rx_seq <= _seq_num_prev)
        {
            // ignore duplicate or replay-attack
            #ifdef NRFRR_DEBUG_RX_ERRSTATS
            _stat_rx_errs[8]++;
            #endif
            return false;
        }
        _seq_num_prev = rx_seq;
    }
    #endif

    // all checks pass

    #ifdef NRFRR_DEBUG_PINS
    digitalWrite(NRFRR_DEBUG_PIN_RX, _dbgpin_rx = (_dbgpin_rx == LOW ? HIGH : LOW));
    #endif

    uint8_t* dst_buf = ((rx_flags & (1 << NRFRR_FLAG_TEXT)) != 0) ? (uint8_t*)txt_rx_buffer : (uint8_t*)rx_buffer;

    memcpy(dst_buf, (void*)parsed->payload, NRFRR_PAYLOAD_SIZE);

    if ((rx_flags & (1 << NRFRR_FLAG_TEXT)) == 0) {
        rx_flag = true;
    }
    else {
        txt_flag = true;
        _stat_txt_cnt++;
    }
    _reply_request_latch = false;

    #ifdef NRFRR_DEBUG_HOP
    bool hop_dbg_nl = true;
    Serial.printf("RX %u %u", now, _seq_num_prev);
    #endif

    if (_is_tx == false)
    {
        _cur_chan = parsed->chan_hint ^ r; // sync with channel hop
        #ifdef NRFRR_DEBUG_HOP
        Serial.printf(" %u", _cur_chan);
        #endif
    }

    #ifdef NRFRR_DEBUG_HOP
    if (hop_dbg_nl) {
        Serial.printf("\r\n");
    }
    #endif

    _last_rx_time = _rx_time_cache;
    _stat_rx_rssi = NRF_RADIO->RSSISAMPLE;
    _stat_rx_good++;
    _stat_rx_cnt++;
    _rx_miss_cnt = 0;

    return true;
}

void nRF52RcRadio::get_rx_stats(uint32_t* total, uint32_t* good, signed* rssi)
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

int nRF52RcRadio::available(void)
{
    task();
    return rx_flag ? NRFRR_PAYLOAD_SIZE : 0;
}

int nRF52RcRadio::read(uint8_t* data)
{
    task();
    if (rx_flag) // has data
    {
        if (data != NULL) {
            // copy to user buffer if available
            memcpy(data, rx_buffer, NRFRR_PAYLOAD_SIZE);
        }
        rx_flag = false; // mark as read
        return NRFRR_PAYLOAD_SIZE; // report success
    }
    return -1; // report no data
}

int nRF52RcRadio::textAvail(void)
{
    task();
    return txt_flag ? NRFRR_PAYLOAD_SIZE : 0;
}

int nRF52RcRadio::textRead(char* buf)
{
    task();
    if (txt_flag) // has data
    {
        if (buf != NULL) {
            // copy to user buffer if available
            strncpy((char*)buf, (const char*)txt_rx_buffer, NRFRR_PAYLOAD_SIZE - 2);
        }
        txt_flag = false; // mark as read
        return NRFRR_PAYLOAD_SIZE; // report success
    }
    return -1; // report no data
}

void nRF52RcRadio::textSend(char* buf)
{
    strncpy((char*)txt_tx_buffer, (const char*)buf, NRFRR_PAYLOAD_SIZE - 2);
    _text_send_timer = _hop_tbl_len * NRFRR_TX_RETRANS_TXT; // makes sure the text is sent over all the channels a few times
    _seq_num++; // make sure the text is considered fresh
    while (_seq_num == 0) {
        // roll-over occured, start new session
        _session_id = nrfrr_rand();
        _seq_num++;
    }
}

void nRF52RcRadio::pause(void)
{
    if (_statemachine < NRFRR_SM_HALT_START)
    {
        _statemachine = NRFRR_SM_HALT_START;
        _statemachine_next = _statemachine;
        while (is_paused() == false) {
            task();
            yield();
        }
    }
}

bool nRF52RcRadio::is_paused(void)
{
    return _statemachine == NRFRR_SM_HALTED;
}

void nRF52RcRadio::resume(void)
{
    if (_statemachine == NRFRR_SM_CONT_TX) {
        NRF_RADIO->EVENTS_DISABLED = 0;
        NRF_RADIO->TEST            = 0;
        NRF_RADIO->TASKS_DISABLE   = 1;
        _statemachine = NRFRR_SM_HALT_START;
    }
    if (_statemachine >= NRFRR_SM_HALT_START)
    {
        while (is_paused() == false) {
            task();
            yield();
        }
    }
    _statemachine = NRFRR_SM_IDLE_WAIT;
    _statemachine_next = _statemachine;
}

void nRF52RcRadio::cont_tx(uint16_t freq)
{
    pause();
    set_freq(freq);
    _statemachine = NRFRR_SM_CONT_TX;
    NRF_RADIO->FREQUENCY  = freq;
    NRF_RADIO->TEST       = (RADIO_TEST_CONST_CARRIER_Enabled << RADIO_TEST_CONST_CARRIER_Pos) | (RADIO_TEST_PLL_LOCK_Enabled << RADIO_TEST_PLL_LOCK_Pos);
    NRF_RADIO->TASKS_TXEN = 1;
}
