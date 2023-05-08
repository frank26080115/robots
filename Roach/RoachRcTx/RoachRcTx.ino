#include "roachtx_conf.h"
#include <RoachLib.h>
#include <nRF52RcRadio.h>
#include <RoachPot.h>
#include <RoachButton.h>
#include <RoachRotaryEncoder.h>
#include <RoachCmdLine.h>
#include <RoachUsbMsd.h>
#if defined(ESP32)
#include <SPIFFS.h>
#elif defined(NRF52840_XXAA)
#include <Adafruit_LittleFS.h>
#include <InternalFileSystem.h>
#include <SPI.h>
#include <SdFat.h>
#include "Adafruit_SPIFlash.h"
#endif

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
roach_rx_nvm_t nvm_rx;
roach_tx_nvm_t nvm_tx;

RoachButton btn_up     = RoachButton(ROACHHW_PIN_BTN_UP);
RoachButton btn_down   = RoachButton(ROACHHW_PIN_BTN_DOWN);
RoachButton btn_left   = RoachButton(ROACHHW_PIN_BTN_LEFT);
RoachButton btn_right  = RoachButton(ROACHHW_PIN_BTN_RIGHT);
RoachButton btn_center = RoachButton(ROACHHW_PIN_BTN_CENTER);
RoachButton btn_g5     = RoachButton(ROACHHW_PIN_BTN_G5);
RoachButton btn_g6     = RoachButton(ROACHHW_PIN_BTN_G6);
RoachButton btn_sw1    = RoachButton(ROACHHW_PIN_BTN_SW1);
RoachButton btn_sw2    = RoachButton(ROACHHW_PIN_BTN_SW2);
RoachButton btn_sw3    = RoachButton(ROACHHW_PIN_BTN_SW3);
#ifdef ROACHHW_PIN_BTN_SW4
RoachButton btn_sw4    = RoachButton(ROACHHW_PIN_BTN_SW4);
#endif

RoachPot pot_throttle = RoachPot(ROACHHW_PIN_POT_THROTTLE, &(nvm_tx.pot_throttle));
RoachPot pot_steering = RoachPot(ROACHHW_PIN_POT_STEERING, &(nvm_tx.pot_steering));
RoachPot pot_weapon   = RoachPot(ROACHHW_PIN_POT_WEAPON  , &(nvm_tx.pot_weapon));
RoachPot pot_aux      = RoachPot(ROACHHW_PIN_POT_AUX     , &(nvm_tx.pot_aux));
RoachPot pot_battery  = RoachPot(ROACHHW_PIN_POT_BATTERY , &(nvm_tx.pot_battery));

extern roach_nvm_gui_desc_t cfggroup_ctrler[];

roach_ctrl_pkt_t  tx_pkt    = {0};
roach_telem_pkt_t telem_pkt = {0};

char telem_txt[NRFRR_PAYLOAD_SIZE];

int headingx = 0, heading = 0;
bool weap_sw_warning = false;
uint8_t encoder_mode = 0;
bool pots_locked = false;

void setup(void)
{
    nrf5rand_init(NRF5RAND_BUFF_SIZE, true, false);
    Serial.begin(115200);
    RoachUsbMsd_begin();
    settings_init();
    radio_init();
    RoachButton_allBegin();
    RoachPot_allBegin();
    RoachEnc_begin(ROACHHW_PIN_ENC_A, ROACHHW_PIN_ENC_B);
    gui_init();
    Serial.println("\r\nRoach RC Transmitter - Hello World!");
}

void loop(void)
{
    // detect if weapon switch is on at boot
    if (btn_sw1.isHeld() > 0) {
        weap_sw_warning = true;
    }

    // show splash screen for a short time, user can exit with a button
    // this will also ensure that the RNG buffer fills up
    while (millis() <= 3000)
    {
        ctrler_tasks();
        if (btns_hasAnyPressed()) {
            break;
        }
    }
    RoachButton_clearAll();
    menu_run(); // the menu's run loop will execute other tasks, such as ctrler_task

    Serial.printf("ERROR[%u]: menu loop has exited\r\n", millis());
    // the code should not actually reach this point
}

void radio_init(void)
{
    radio.begin(nvm_rf.chan_map, nvm_rf.uid, nvm_rf.salt, ROACHHW_PIN_FEM_TX, ROACHHW_PIN_FEM_RX);
}

void ctrler_tasks(void)
{
    //uint32_t now = millis();
    getFreeRam(); // since ctrler_tasks is called from deeper menus, getting the free heap RAM here is a good idea
    RoachPot_allTask(); // polls ADCs non-blocking
    // TODO: buttons are using interrupts only, no tasks, but check reliability
    nbtwi_task(); // send any queued data to OLED
    radio.task(); // run the radio state machine, sends buffered data when time is right

    if (radio.is_busy()) // if the transmission is happening, then we have time to do extra stuff
    {
        RoachEnc_task();     // transfers hardware values to RAM atomically
        nrf5rand_task();     // collect random numbers from RNG
        RoSync_task();       // checks for robot synchronization
        RoachUsbMsd_task();  // handles tasks for USB flash mass storage
        cmdline.task();      // handles command line

        ctrler_buildPkt();
        radio.send((uint8_t*)&tx_pkt);

        if (radio.available())
        {
            radio.read((uint8_t*)&telem_pkt);
        }

        if (radio.textAvail())
        {
            radio.textRead(telem_txt);
            Serial.printf("RX-TXT[%u]: %s\r\n", millis(), telem_txt);
        }

        ctrler_pktDebug();
    }
}

void ctrler_buildPkt(void)
{
    tx_pkt.throttle = (pots_locked == false) ? pot_throttle.get() : 0;
    if (encoder_mode == ENCODERMODE_NORMAL)
    {
        tx_pkt.steering = (pots_locked == false) ? pot_steering.get() : 0;
        int h = RoachEnc_get(true);
        headingx += h * nvm_tx.heading_multiplier;
        while (headingx >= 18000) {
            headingx -= 36000;
        }
        while (headingx <= -18000) {
            headingx += 36000;
        }
        heading = (headingx + 5) / 10;
        tx_pkt.heading = heading;
    }
    else if (encoder_mode == ENCODERMODE_USEPOT)
    {
        tx_pkt.steering = 0;
        int hx = headingx;
        hx += roach_value_map((pots_locked == false) ? pot_steering.get() : 0, 0, ROACH_SCALE_MULTIPLIER, 0, 9000, false);
        while (hx >= 18000) {
            hx -= 36000;
        }
        while (hx <= -18000) {
            hx += 36000;
        }
        tx_pkt.heading = (hx + 5) / 10;
    }
    tx_pkt.pot_weap = (pots_locked == false) ? pot_weapon.get() : 0;
    tx_pkt.pot_aux  = (pots_locked == false) ? pot_aux.get()    : 0;

    // if the user turns off the switch, no more warning is needed
    if (btn_sw1.isHeld() <= 0) {
        weap_sw_warning = false;
    }

    uint32_t f = 0;
    f |= (btn_sw1.isHeld() > 0 && weap_sw_warning == false) ? ROACHPKTFLAG_BTN1 : 0;
    f |= btn_sw2.isHeld() > 0 ? ROACHPKTFLAG_BTN2 : 0;
    f |= btn_sw3.isHeld() > 0 ? ROACHPKTFLAG_BTN3 : 0;
    #ifdef ROACHHW_PIN_BTN_SW4
    f |= btn_sw4.isHeld() > 0 ? ROACHPKTFLAG_BTN4 : 0;
    #endif
    tx_pkt.flags = f;
}

void ctrler_pktDebug(void)
{
    static uint32_t last_time = 0;
    uint32_t now = millis();
    if ((now - last_time) >= 1000)
    {
        last_time = now;
        Serial.printf("TX[%u]:  %d , %d , %d , %d , 0x%08X\r\n"
            , tx_pkt.throttle
            , tx_pkt.steering
            , tx_pkt.pot_weap
            , tx_pkt.pot_aux
            , tx_pkt.flags
            );
    }
}
