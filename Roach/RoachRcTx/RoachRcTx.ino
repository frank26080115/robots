#include "roachtx_conf.h"
#include <Adafruit_TinyUSB.h>
#include <RoachLib.h>
#include <nRF52RcRadio.h>
#include <RoachPot.h>
#include <RoachButton.h>
#include <RoachRotaryEncoder.h>
#include <RoachCmdLine.h>
#include <RoachUsbMsd.h>
#include <RoachPerfCnt.h>
#include <RoachHeartbeat.h>
#if defined(ESP32)
#include <SPIFFS.h>
#elif defined(NRF52840_XXAA)
#include <Adafruit_LittleFS.h>
#include <InternalFileSystem.h>
#include <SPI.h>
#include <SdFat.h>
#include <Adafruit_SPIFlash.h>
#endif

#include <AsyncAdc.h>
#include <nRF5Rand.h>
#include <NonBlockingTwi.h>
#include "MenuClass.h"
#include <Adafruit_SSD1306.h> // IMPORTANT: this is a customized version of the library that uses nbtwi (non-blocking TWI)
#include <Adafruit_GFX.h>

#if defined(ESP32)
#include <SPIFFS.h>
#elif defined(NRF52840_XXAA)
#include <Adafruit_LittleFS.h>
#include <InternalFileSystem.h>
#endif

Adafruit_SSD1306 oled;
FatFile fatroot;
FatFile fatfile;
extern RoachCmdLine cmdline;

nRF52RcRadio radio = nRF52RcRadio(true);

roach_rf_nvm_t nvm_rf;
roach_tx_nvm_t nvm_tx;

RoachButton btn_up     = RoachButton(ROACHHW_PIN_BTN_UP);
RoachButton btn_down   = RoachButton(ROACHHW_PIN_BTN_DOWN);
RoachButton btn_left   = RoachButton(ROACHHW_PIN_BTN_LEFT);
RoachButton btn_right  = RoachButton(ROACHHW_PIN_BTN_RIGHT);
RoachButton btn_center = RoachButton(ROACHHW_PIN_BTN_CENTER);
RoachButton btn_g5     = RoachButton(ROACHHW_PIN_BTN_G5);
RoachButton btn_g6     = RoachButton(ROACHHW_PIN_BTN_G6);
RoachButton btn_sw1    = RoachButton(ROACHHW_PIN_BTN_SW1, true, 0, 50);
RoachButton btn_sw2    = RoachButton(ROACHHW_PIN_BTN_SW2, true, 0, 50);
RoachButton btn_sw3    = RoachButton(ROACHHW_PIN_BTN_SW3, true, 0, 50);
RoachButton btn_d7     = RoachButton(ROACHHW_PIN_BTN_D7 , true, 0, 50);

#ifdef ROACHHW_PIN_BTN_SW4
// ran out of pins, switch 4 does not exist
RoachButton btn_sw4    = RoachButton(ROACHHW_PIN_BTN_SW4);
#endif

RoachPot pot_throttle = RoachPot(ROACHHW_PIN_POT_THROTTLE, &(nvm_tx.pot_throttle), ROACHHW_POT_GAIN_THROTTLE);
RoachPot pot_steering = RoachPot(ROACHHW_PIN_POT_STEERING, &(nvm_tx.pot_steering), ROACHHW_POT_GAIN_STEERING);
RoachPot pot_weapon   = RoachPot(ROACHHW_PIN_POT_WEAPON  , &(nvm_tx.pot_weapon  ), ROACHHW_POT_GAIN_WEAPON  );
RoachPot pot_aux      = RoachPot(ROACHHW_PIN_POT_AUX     , &(nvm_tx.pot_aux     ), ROACHHW_POT_GAIN_AUX     );
RoachPot pot_battery  = RoachPot(ROACHHW_PIN_POT_BATTERY , &(nvm_tx.pot_battery ), ROACHHW_POT_GAIN_BATTERY );

extern roach_nvm_gui_desc_t cfgdesc_ctrler[];

roach_ctrl_pkt_t  tx_pkt    = {0};
roach_telem_pkt_t telem_pkt = {0};

char telem_txt[NRFRR_PAYLOAD_SIZE];

int headingx = 0, heading = 0;
bool weap_sw_warning = false;
uint8_t encoder_mode = 0;
bool pots_locked = false;
bool move_locked = true;
int move_zero_cnt = 0;

RoachHeartbeat hb_red = RoachHeartbeat(ROACHHW_PIN_LED_RED);
RoachHeartbeat hb_blu = RoachHeartbeat(ROACHHW_PIN_LED_BLU);

void setup(void)
{
    //sd_softdevice_disable();
    safeboot_check(); // check if user wants to enter bootloader mode
    //hw_bringup();

    #ifdef DEVMODE_WAIT_SERIAL_PRE
    Serial.begin(115200);
    while (Serial.available() == 0) {
        Serial.println("waiting for input");
        delay(100);
    }
    Serial.read();
    #endif

    RoachWdt_init(ROACH_WDT_TIMEOUT_MS);
    hb_red.begin();
    hb_blu.begin();

    nrf5rand_init(NRF5RAND_BUFF_SIZE, true, false);
    nrf5rand_seed(false);
    Serial.begin(115200);
    RoachUsbMsd_begin();
    settings_init();
    radio_init();
    RoachButton_allBegin();
    RoachPot_allBegin();
    RoachEnc_begin(ROACHHW_PIN_ENC_A, ROACHHW_PIN_ENC_B);
    nbtwi_init(ROACHHW_PIN_I2C_SCL, ROACHHW_PIN_I2C_SDA, (SCREEN_WIDTH * SCREEN_HEIGHT / 8) * 2);
    PerfCnt_init();
    Serial.println("\r\nRoach RC Transmitter - Hello World!");

    settings_initValidate();

    switches_getFlags(); // this call here will get the initial switch states and check if they are safe

    // seems like the OLED needs a bit more time to power up
    while (millis() <= 200)
    {
        ctrler_tasks();
    }

    Serial.println("OLED init start");

    gui_init();
    // show splash screen for a short time, user can exit with a button
    // this will also ensure that the RNG buffer fills up
    while (millis() <= 3000
        #ifdef DEVMODE_WAIT_SERIAL_POST
        || cmdline.has_interaction() == false
        #endif
    )
    {
        ctrler_tasks();
        if (btns_hasAnyPressed()) {
            break;
        }
    }
    RoachButton_clearAll();

    Serial.println("splash screen ended, entering menu system loop");
}

void loop(void)
{
    menu_run(); // the menu's run loop will execute other tasks, such as ctrler_task
}

void radio_init(void)
{
    radio.begin(
        #if defined(ROACHHW_PIN_FEM_TX) && defined(ROACHHW_PIN_FEM_RX)
        ROACHHW_PIN_FEM_TX, ROACHHW_PIN_FEM_RX
        #endif
    );
    radio.config(nvm_rf.chan_map, nvm_rf.uid, nvm_rf.salt);
}

void ctrler_tasks(void)
{
    //uint32_t now = millis();
    PerfCnt_task();
    RoachWdt_feed();
    RoachButton_allTask();

    #ifdef DEVMODE_DEBUG_BUTTONS
    static uint32_t prev_btns = 0;
    uint32_t cur_btns = RoachButton_hasAnyPressed();
    if (cur_btns != prev_btns) {
        if (cur_btns != 0) {
            debug_printf("[%u] btn press detected 0x%04X\r\n", millis(), cur_btns);
        }
        else {
            debug_printf("[%u] btn all released detected 0x%04X\r\n", millis(), cur_btns);
        }
        prev_btns = cur_btns;
    }
    #endif

    RoachPot_allTask(); // polls ADCs non-blocking
    // TODO: buttons are using interrupts only, no tasks, but check reliability
    nbtwi_task(); // send any queued data to OLED
    #ifndef DEVMODE_NO_RADIO
    radio.task(); // run the radio state machine, sends buffered data when time is right
    #endif

    #ifndef DEVMODE_NO_RADIO
    if (radio.isBusy()) // if the transmission is happening, then we have time to do extra stuff
    #endif
    {
        heartbeat_task();
        RoachEnc_task();     // transfers hardware values to RAM automically
        nrf5rand_task();     // collect random numbers from RNG
        rosync_task();       // checks for robot synchronization
        RoachUsbMsd_task();  // handles tasks for USB flash mass storage
        cmdline.task();      // handles command line

        if (nbtwi_hasError(true)) {
            Serial.printf("ERROR[%u]: I2C bus problem, rebooting OLED\r\n", millis());
            gui_reboot();
        }

        ctrler_buildPkt();
        #ifndef DEVMODE_NO_RADIO
        radio.send((uint8_t*)&tx_pkt);

        if (radio.available())
        {
            radio.read((uint8_t*)&telem_pkt);
        }

        ctrler_pktDebug();
        #endif
    }
}

void ctrler_buildPkt(void)
{
    tx_pkt.throttle = (pots_locked == false) ? pot_throttle.get() : 0;

    if (encoder_mode == ENCODERMODE_USEPOT)
    {
        tx_pkt.steering = 0; // pot has been repurposed, do not use steering override
        int hx = headingx;
        hx += roach_value_map((pots_locked == false) ? pot_steering.get() : 0, 0, ROACH_SCALE_MULTIPLIER, 0, (90 * ROACH_ANGLE_MULTIPLIER * 100), false);
        ROACH_WRAP_ANGLE(hx, ROACH_ANGLE_MULTIPLIER * 100);
        tx_pkt.heading = roach_div_rounded(hx, 100);
    }
    else
    {
        tx_pkt.steering = (pots_locked == false) ? pot_steering.get() : 0;
        int h = RoachEnc_get(true);
        headingx += h * nvm_tx.heading_multiplier;
        ROACH_WRAP_ANGLE(headingx, ROACH_ANGLE_MULTIPLIER * 100);
        tx_pkt.heading = roach_div_rounded(headingx, 100);
    }

    tx_pkt.pot_weap = (pots_locked == false) ? pot_weapon.get() : 0;
    tx_pkt.pot_aux  = (pots_locked == false) ? pot_aux.get()    : 0;

    tx_pkt.flags = switches_getFlags();

    if (move_locked)
    {
        if (tx_pkt.throttle == 0 && tx_pkt.steering == 0) {
            move_zero_cnt += 1;
        }
        else {
            move_zero_cnt = 0;
        }
        if (move_zero_cnt > 10) {
            move_locked = false;
        }
    }
    if (move_locked)
    {
        tx_pkt.flags |= ROACHPKTFLAG_SAFE;
    }
}

void ctrler_pktDebug(void)
{
    #ifdef DEVMODE_DEBUG_PACKET
    static uint32_t last_time = 0;
    uint32_t now = millis();
    if ((now - last_time) >= 250)
    {
        last_time = now;
        Serial.printf("TX[%u]:  %d , %d , %d , %d , 0x%08X , drate %u"
            , now
            , tx_pkt.throttle
            , tx_pkt.steering
            , tx_pkt.pot_weap
            , tx_pkt.pot_aux
            , tx_pkt.flags
            , radio.stats_rate.drate
            );
        if (radio.isConnected()) {
            Serial.printf(" , rssi %d", radio.getRssi());
        }
        Serial.printf("\r\n");
    }
    #endif
}
