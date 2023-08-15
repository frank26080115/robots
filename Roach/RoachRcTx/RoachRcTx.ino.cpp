# 1 "C:\\Users\\frank\\AppData\\Local\\Temp\\tmpjwol03zl"
#include <Arduino.h>
# 1 "D:/GithubRepos/robots/Roach/RoachRcTx/RoachRcTx.ino"
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
#include <Adafruit_SSD1306.h>
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

RoachButton btn_up = RoachButton(ROACHHW_PIN_BTN_UP);
RoachButton btn_down = RoachButton(ROACHHW_PIN_BTN_DOWN);
RoachButton btn_left = RoachButton(ROACHHW_PIN_BTN_LEFT);
RoachButton btn_right = RoachButton(ROACHHW_PIN_BTN_RIGHT);
RoachButton btn_center = RoachButton(ROACHHW_PIN_BTN_CENTER);
RoachButton btn_g5 = RoachButton(ROACHHW_PIN_BTN_G5);
RoachButton btn_g6 = RoachButton(ROACHHW_PIN_BTN_G6);
RoachButton btn_sw1 = RoachButton(ROACHHW_PIN_BTN_SW1);
RoachButton btn_sw2 = RoachButton(ROACHHW_PIN_BTN_SW2);
RoachButton btn_sw3 = RoachButton(ROACHHW_PIN_BTN_SW3);
RoachButton btn_d7 = RoachButton(ROACHHW_PIN_BTN_D7);

#ifdef ROACHHW_PIN_BTN_SW4

RoachButton btn_sw4 = RoachButton(ROACHHW_PIN_BTN_SW4);
#endif

RoachPot pot_throttle = RoachPot(ROACHHW_PIN_POT_THROTTLE, &(nvm_tx.pot_throttle));
RoachPot pot_steering = RoachPot(ROACHHW_PIN_POT_STEERING, &(nvm_tx.pot_steering));
RoachPot pot_weapon = RoachPot(ROACHHW_PIN_POT_WEAPON , &(nvm_tx.pot_weapon));
RoachPot pot_aux = RoachPot(ROACHHW_PIN_POT_AUX , &(nvm_tx.pot_aux));
RoachPot pot_battery = RoachPot(ROACHHW_PIN_POT_BATTERY , &(nvm_tx.pot_battery));

extern roach_nvm_gui_desc_t cfgdesc_ctrler[];

roach_ctrl_pkt_t tx_pkt = {0};
roach_telem_pkt_t telem_pkt = {0};

char telem_txt[NRFRR_PAYLOAD_SIZE];

int headingx = 0, heading = 0;
bool weap_sw_warning = false;
uint8_t encoder_mode = 0;
bool pots_locked = false;

RoachHeartbeat hb_red = RoachHeartbeat(ROACHHW_PIN_LED_RED);
RoachHeartbeat hb_blu = RoachHeartbeat(ROACHHW_PIN_LED_BLU);
void setup(void);
void loop(void);
void radio_init(void);
void ctrler_tasks(void);
void ctrler_buildPkt(void);
void ctrler_pktDebug(void);
void menu_install_calibSync(void);
void factory_reset_func(void* cmd, char* argstr, Stream* stream);
void save_func(void* cmd, char* argstr, Stream* stream);
void echo_func(void* cmd, char* argstr, Stream* stream);
void reboot_func(void* cmd, char* argstr, Stream* stream);
void usbmsd_func(void* cmd, char* argstr, Stream* stream);
void unusbmsd_func(void* cmd, char* argstr, Stream* stream);
void memcheck_func(void* cmd, char* argstr, Stream* stream);
void perfcheck_func(void* cmd, char* argstr, Stream* stream);
void debug_func(void* cmd, char* argstr, Stream* stream);
void conttx_func(void* cmd, char* argstr, Stream* stream);
void regenrf_func(void* cmd, char* argstr, Stream* stream);
void readrf_func(void* cmd, char* argstr, Stream* stream);
void dfuenter_func(void* cmd, char* argstr, Stream* stream);
void fakebtn_func(void* cmd, char* argstr, Stream* stream);
void fakepots_func(void* cmd, char* argstr, Stream* stream);
void fakepott_func(void* cmd, char* argstr, Stream* stream);
void fakepotw_func(void* cmd, char* argstr, Stream* stream);
void fakeenc_func(void* cmd, char* argstr, Stream* stream);
void gui_init(void);
void gui_reboot(void);
void gui_drawSplash(void);
bool gui_canDisplay(void);
void gui_drawWait(void);
void gui_drawNow(void);
void showError(const char* s);
void showMessage(const char* s1, const char* s2);
void printEllipses(void);
void drawSideBar(const char* upper, const char* lower, bool line);
void drawTitleBar(const char* s, bool center, bool line, bool arrows);
void drawProgressBar(int x, int y, int w, int h, int prog, int total);
void heartbeat_task();
void hw_bringup(void);
int printRfStats(int y);
void menu_run(void);
void menu_setup(void);
void menu_install(RoachMenu* m);
void menu_install_fileOpener(void);
void settings_init(void);
void settings_factoryReset(void);
bool settings_save(void);
void settings_saveIfNeeded(uint32_t span);
bool settings_loadFile(const char* fname);
bool settings_saveToFile(const char* fname);
void settings_markDirty(void);
bool roachnvm_fileCopy(const char* fin_name, const char* fout_name);
void settings_initValidate(void);
void QuickAction_check(uint8_t btn);
void rosync_task(void);
bool rosync_downloadDescFile(void);
void rosync_downloadNvm(void);
bool rosync_downloadStart(void);
bool rosync_downloadDescChunk(radio_binpkt_t* pkt);
bool rosync_downloadNvmChunk(radio_binpkt_t* pkt);
bool rosync_loadDescFileId(uint32_t id);
bool rosync_loadDescFile(const char* fname, uint32_t id);
bool rosync_loadDescFileObj(RoachFile* f, uint32_t id);
bool rosync_loadNvmFile(const char* fname);
void rosync_uploadStart(void);
void rosync_uploadChunk(roach_nvm_gui_desc_t* desc);
void rosync_uploadNextChunk(void);
bool rosync_markOnlyFile(void);
bool rosync_markStartupFile(const char* fname);
bool rosync_loadStartup(void);
bool rosync_isSynced(void);
void rosync_draw(void);
void rosync_drawShort(void);
void menu_install_robot();
void safeboot_check(void);
bool btns_hasAnyPressed(void);
bool btns_isAnyHeld(void);
void btns_clearAll(void);
void btns_disableAll(void);
uint8_t switches_getFlags(void);
void strncpy0(char* dest, const void* src, size_t n);
void waitFor(uint32_t x);
float batt_get(void);
#line 84 "D:/GithubRepos/robots/Roach/RoachRcTx/RoachRcTx.ino"
void setup(void)
{

    safeboot_check();


    RoachWdt_init(ROACH_WDT_TIMEOUT_MS);
    hb_red.begin();
    hb_blu.begin();

    nrf5rand_init(NRF5RAND_BUFF_SIZE, true, false);
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

    switches_getFlags();


    while (millis() <= 200)
    {
        ctrler_tasks();
    }

    Serial.println("OLED init start");

    gui_init();


    while (millis() <= 3000
        #ifdef DEVMODE_WAIT_SERIAL
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
    menu_run();
}

void radio_init(void)
{
    radio.begin();
    radio.config(nvm_rf.chan_map, nvm_rf.uid, nvm_rf.salt);
}

void ctrler_tasks(void)
{

    PerfCnt_task();
    RoachWdt_feed();
    RoachButton_allTask();
    RoachPot_allTask();

    nbtwi_task();
    #ifndef DEVMODE_NO_RADIO
    radio.task();
    #endif

    #ifndef DEVMODE_NO_RADIO
    if (radio.isBusy())
    #endif
    {
        heartbeat_task();
        RoachEnc_task();
        nrf5rand_task();
        #ifndef DEVMODE_NO_RADIO
        rosync_task();
        #endif
        RoachUsbMsd_task();
        cmdline.task();

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
        tx_pkt.steering = 0;
        int hx = headingx;
        hx += roach_value_map((pots_locked == false) ? pot_steering.get() : 0, 0, ROACH_SCALE_MULTIPLIER, 0, (90 * ROACH_ANGLE_MULTIPLIER), false);
        ROACH_WRAP_ANGLE(hx, ROACH_ANGLE_MULTIPLIER);
        tx_pkt.heading = hx;
    }
    else
    {
        tx_pkt.steering = (pots_locked == false) ? pot_steering.get() : 0;
        int h = RoachEnc_get(true);
        headingx += h * nvm_tx.heading_multiplier;
        ROACH_WRAP_ANGLE(headingx, ROACH_ANGLE_MULTIPLIER);
        tx_pkt.heading = headingx;
    }

    tx_pkt.pot_weap = (pots_locked == false) ? pot_weapon.get() : 0;
    tx_pkt.pot_aux = (pots_locked == false) ? pot_aux.get() : 0;

    tx_pkt.flags = switches_getFlags();
}

void ctrler_pktDebug(void)
{
    return;
    static uint32_t last_time = 0;
    uint32_t now = millis();
    if ((now - last_time) >= 1000)
    {
        last_time = now;
        Serial.printf("TX[%u]:  %d , %d , %d , %d , 0x%08X\r\n"
            , now
            , tx_pkt.throttle
            , tx_pkt.steering
            , tx_pkt.pot_weap
            , tx_pkt.pot_aux
            , tx_pkt.flags
            );
    }
}
# 1 "D:/GithubRepos/robots/Roach/RoachRcTx/Bitmaps.ino"
const uint8_t splash[] =
{
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0x00, 0x01, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x01, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x01, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x01, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x80, 0x03, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0xC0, 0x00, 0x01, 0x80, 0x03, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x01, 0xE0, 0x00, 0x01, 0x80, 0x03, 0x00, 0x00, 0x0F, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x03, 0xF0, 0x00, 0x00, 0x1F, 0xF0, 0x00, 0x00, 0x1F, 0x80, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x07, 0x38, 0x00, 0x00, 0xFF, 0xFE, 0x00, 0x00, 0x39, 0xC0, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x0E, 0x1C, 0x00, 0x07, 0xFF, 0xFF, 0xC0, 0x00, 0x70, 0xE0, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x1C, 0x0E, 0x00, 0x1F, 0xFF, 0xFF, 0xF0, 0x00, 0xE0, 0x70, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x38, 0x07, 0x00, 0x3F, 0xFF, 0xFF, 0xF8, 0x01, 0xC0, 0x38, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x70, 0x03, 0x80, 0x7F, 0x3F, 0xF9, 0xFC, 0x03, 0x80, 0x1C, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0xE0, 0x01, 0xC0, 0xFE, 0x1F, 0xF0, 0xFE, 0x07, 0x00, 0x0E, 0x00, 0x00, 0x00,
0x00, 0x00, 0x01, 0xC0, 0x00, 0xE1, 0xFE, 0x1F, 0xF0, 0xFF, 0x0E, 0x00, 0x07, 0x00, 0x00, 0x00,
0x00, 0x00, 0x01, 0x80, 0x00, 0x63, 0xFF, 0x3F, 0xF9, 0xFF, 0x8C, 0x00, 0x03, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0xFF, 0xFF, 0xFF, 0xFF, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x0F, 0xFF, 0xFF, 0xFF, 0xFF, 0xE0, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x0F, 0xFF, 0xFF, 0xFF, 0xFF, 0xE0, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x1F, 0xFF, 0xFF, 0xFF, 0xFF, 0xF0, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x1F, 0xFF, 0xFF, 0xFF, 0xFF, 0xF0, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x1F, 0xFF, 0xFF, 0xFF, 0xFF, 0xF0, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x3F, 0xFF, 0xFF, 0xFF, 0xFF, 0xF8, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x0C, 0x00, 0x3F, 0xFF, 0xFF, 0xFF, 0xFF, 0xF8, 0x00, 0x60, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x1F, 0x00, 0x3F, 0xFF, 0xFF, 0xFF, 0xFF, 0xF8, 0x01, 0xF0, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x3F, 0xC0, 0x7F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFC, 0x07, 0xF8, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x71, 0xF0, 0x7F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFC, 0x1F, 0x1C, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0xE0, 0x7C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7C, 0x0E, 0x00, 0x00, 0x00,
0x00, 0x00, 0x01, 0xC0, 0x1E, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF0, 0x07, 0x00, 0x00, 0x00,
0x00, 0x00, 0x03, 0x80, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xC0, 0x03, 0x80, 0x00, 0x00,
0x00, 0x00, 0x07, 0x00, 0x00, 0x7F, 0xFF, 0xFC, 0x7F, 0xFF, 0xFC, 0x00, 0x01, 0xC0, 0x00, 0x00,
0x00, 0x00, 0x0E, 0x00, 0x00, 0x7F, 0xFF, 0xFC, 0x7F, 0xFF, 0xFC, 0x00, 0x00, 0xE0, 0x00, 0x00,
0x00, 0x00, 0x1C, 0x00, 0x00, 0x7F, 0xFF, 0xFC, 0x7F, 0xFF, 0xFC, 0x00, 0x00, 0x70, 0x00, 0x00,
0x00, 0x00, 0x18, 0x00, 0x00, 0x7F, 0xFF, 0xFC, 0x7F, 0xFF, 0xFC, 0x00, 0x00, 0x30, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x3F, 0xFF, 0xFC, 0x7F, 0xFF, 0xF8, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x3F, 0xFF, 0xFC, 0x7F, 0xFF, 0xF8, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x3F, 0xFF, 0xFC, 0x7F, 0xFF, 0xF8, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x1F, 0xFF, 0xFC, 0x7F, 0xFF, 0xF0, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x1F, 0xFF, 0xFC, 0x7F, 0xFF, 0xF0, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x1F, 0xFF, 0xFC, 0x7F, 0xFF, 0xF0, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x0F, 0xFF, 0xFC, 0x7F, 0xFF, 0xE0, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0xF0, 0x0F, 0xFF, 0xFC, 0x7F, 0xFF, 0xE0, 0x1E, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x01, 0xFF, 0x07, 0xFF, 0xFC, 0x7F, 0xFF, 0xC1, 0xFF, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x03, 0x9F, 0xC3, 0xFF, 0xFC, 0x7F, 0xFF, 0x87, 0xF3, 0x80, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x07, 0x01, 0xC1, 0xFF, 0xFC, 0x7F, 0xFF, 0x07, 0x01, 0xC0, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x0E, 0x00, 0x00, 0xFF, 0xFC, 0x7F, 0xFE, 0x00, 0x00, 0xE0, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x1C, 0x00, 0x00, 0x7F, 0xFC, 0x7F, 0xFC, 0x00, 0x00, 0x70, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x38, 0x00, 0x00, 0x3F, 0xFC, 0x7F, 0xF8, 0x00, 0x00, 0x38, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x70, 0x00, 0x00, 0x1F, 0xFC, 0x7F, 0xF0, 0x00, 0x00, 0x1C, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0xE0, 0x00, 0x00, 0x07, 0xFC, 0x7F, 0xC0, 0x00, 0x00, 0x0E, 0x00, 0x00, 0x00,
0x00, 0x00, 0x01, 0xC0, 0x00, 0x00, 0x00, 0xFC, 0x7E, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00,
0x00, 0x00, 0x01, 0x80, 0x00, 0x00, 0x00, 0x1C, 0x70, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

const uint8_t icon_disconnected[] =
{
0xFE, 0x82, 0x92, 0x44, 0x54, 0x28, 0x38, 0x10, 0x10, 0x28, 0x10, 0x44, 0x10, 0x82, 0x00, 0x00
};

const uint8_t icon_connected[] =
{
0xFE, 0x02, 0x92, 0x02, 0x54, 0x0A, 0x38, 0x0A, 0x10, 0x2A, 0x10, 0x2A, 0x10, 0xAA, 0x00, 0x00
};

const uint8_t icon_mismatched[] =
{
0xFE, 0x04, 0x92, 0x08, 0x54, 0xCA, 0x38, 0x10, 0x10, 0xA6, 0x10, 0x20, 0x10, 0x40, 0x00, 0x00
};
# 1 "D:/GithubRepos/robots/Roach/RoachRcTx/CalibSync.ino"
class RoachMenuFuncCalibGyro : public RoachMenuFunctionItem
{
    public:
        RoachMenuFuncCalibGyro(void) : RoachMenuFunctionItem("calib gyro")
        {
        };

        virtual void draw(void)
        {
            oled.setCursor(0, 30);
            oled.print("message sent");
        };

    protected:
        virtual void onEnter(void)
        {
            RoachMenu::onEnter();
            #ifndef DEVMODE_NO_RADIO
            radio.textSend("calibgyro");
            #endif
        };

        virtual void draw_sidebar(void)
        {
            drawSideBar("OK", "REDO", true);
        };

        virtual void draw_title(void)
        {
            drawTitleBar((const char*)_txt, true, true, false);
        };

        virtual void onButton(uint8_t btn)
        {
            RoachMenuFunctionItem::onButton(btn);
            switch (btn)
            {
                case BTNID_G6:
                    _exit = EXITCODE_BACK;
                    break;
                case BTNID_G5:
                    #ifndef DEVMODE_NO_RADIO
                    radio.textSend("calibgyro");
                    #endif
                    break;
            }
        };
};

class RoachMenuFuncCalibAdcCenter : public RoachMenuFunctionItem
{
    public:
        RoachMenuFuncCalibAdcCenter(void) : RoachMenuFunctionItem("cal centers")
        {
        };

        virtual void draw(void)
        {
            int y = 11;
            oled.setCursor(0, y);
            oled.printf("T: %d", pot_throttle.cfg->center);
            y += ROACHGUI_LINE_HEIGHT;
            oled.setCursor(0, y);
            oled.printf("S: %d", pot_steering.cfg->center);
            y += ROACHGUI_LINE_HEIGHT;
            oled.setCursor(0, y);
            oled.printf("W: %d", pot_weapon.cfg->center);
            y += ROACHGUI_LINE_HEIGHT;
            oled.setCursor(0, y);
            oled.printf("A: %d", pot_aux.cfg->center);
            y += ROACHGUI_LINE_HEIGHT;
        };

    protected:
        virtual void onEnter(void)
        {
            pots_locked = true;
            RoachMenu::onEnter();
            pot_throttle.calib_center();
            pot_steering.calib_center();
            pot_weapon.calib_center();
            pot_aux.calib_center();
            settings_markDirty();
        };

        virtual void onExit(void)
        {
            pot_throttle.calib_stop();
            pot_steering.calib_stop();
            pot_weapon.calib_stop();
            pot_aux.calib_stop();
            pots_locked = false;
            settings_markDirty();
        };

        virtual void draw_sidebar(void)
        {
            drawSideBar("OK", "REDO", true);
        };

        virtual void draw_title(void)
        {
            drawTitleBar((const char*)_txt, true, true, false);
        };

        virtual void onButton(uint8_t btn)
        {
            RoachMenuFunctionItem::onButton(btn);
            switch (btn)
            {
                case BTNID_G6:
                    _exit = EXITCODE_BACK;
                    break;
                case BTNID_G5:
                    pot_throttle.calib_center();
                    pot_steering.calib_center();
                    pot_weapon.calib_center();
                    pot_aux.calib_center();
                    break;
            }
        };
};


class RoachMenuFuncCalibAdcLimits : public RoachMenuFunctionItem
{
    public:
        RoachMenuFuncCalibAdcLimits(void) : RoachMenuFunctionItem("cal limits")
        {
        };

        virtual void draw(void)
        {
            int y = 11;
            oled.setCursor(0, y);
            oled.printf("T: %d <> %d", pot_throttle.cfg->limit_min, pot_throttle.cfg->limit_max);
            y += ROACHGUI_LINE_HEIGHT;
            oled.setCursor(0, y);
            oled.printf("S: %d <> %d", pot_steering.cfg->limit_min, pot_steering.cfg->limit_max);
            y += ROACHGUI_LINE_HEIGHT;
            oled.setCursor(0, y);
            oled.printf("W: %d <> %d", pot_weapon.cfg->limit_min, pot_weapon.cfg->limit_max);
            y += ROACHGUI_LINE_HEIGHT;
            oled.setCursor(0, y);
            oled.printf("A: %d <> %d", pot_aux.cfg->limit_min, pot_aux.cfg->limit_max);
            y += ROACHGUI_LINE_HEIGHT;
        };

    protected:
        virtual void onEnter(void)
        {
            pots_locked = true;
            RoachMenu::onEnter();
            pot_throttle.calib_limits();
            pot_steering.calib_limits();
            pot_weapon.calib_limits();
            pot_aux.calib_limits();
            settings_markDirty();
        };

        virtual void onExit(void)
        {
            pot_throttle.calib_stop();
            pot_steering.calib_stop();
            pot_weapon.calib_stop();
            pot_aux.calib_stop();
            pots_locked = false;
            settings_markDirty();
        };

        virtual void draw_sidebar(void)
        {
            drawSideBar("OK", "REDO", true);
        };

        virtual void draw_title(void)
        {
            drawTitleBar((const char*)_txt, true, true, false);
        };

        virtual void onButton(uint8_t btn)
        {
            RoachMenuFunctionItem::onButton(btn);
            switch (btn)
            {
                case BTNID_G6:
                    _exit = EXITCODE_BACK;
                    break;
                case BTNID_G5:
                    pot_throttle.calib_limits();
                    pot_steering.calib_limits();
                    pot_weapon.calib_limits();
                    pot_aux.calib_limits();
                    break;
            }
        };
};

class RoachMenuFuncSyncDownload : public RoachMenuFunctionItem
{
    public:
        RoachMenuFuncSyncDownload(void) : RoachMenuFunctionItem("sync download")
        {
        };

        virtual void draw(void)
        {
            rosync_draw();
        };

    protected:
        virtual void onEnter(void)
        {
            RoachMenu::onEnter();
            if (RoachUsbMsd_canSave()) {
                debug_printf("[%u] RoachMenuFuncSyncDownload onEnter rosync_downloadStart\r\n", millis());
                #ifndef DEVMODE_NO_RADIO
                rosync_downloadStart();
                #endif
            }
            else {
                showError("cannot download\nUSB MSD is on");
                _exit = EXITCODE_BACK;
            }
        };

        virtual void draw_sidebar(void)
        {
            drawSideBar("BACK", "REDO", true);
        };

        virtual void draw_title(void)
        {
            drawTitleBar((const char*)_txt, true, true, false);
        };

        virtual void onButton(uint8_t btn)
        {
            RoachMenuFunctionItem::onButton(btn);
            switch (btn)
            {
                case BTNID_G6:
                    _exit = EXITCODE_BACK;
                    break;
                case BTNID_G5:
                    if (RoachUsbMsd_canSave()) {
                        debug_printf("[%u] RoachMenuFuncSyncDownload onButton BTNID_G5 rosync_downloadStart\r\n", millis());
                        #ifndef DEVMODE_NO_RADIO
                        rosync_downloadStart();
                        #endif
                    }
                    else {
                        showError("cannot download\nUSB MSD is on");
                    }
                    break;
            }
        };
};

class RoachMenuFuncSyncUpload : public RoachMenuFunctionItem
{
    public:
        RoachMenuFuncSyncUpload(void) : RoachMenuFunctionItem("sync upload")
        {
        };

        virtual void draw(void)
        {
            rosync_draw();
        };

    protected:
        virtual void onEnter(void)
        {
            RoachMenu::onEnter();
            debug_printf("[%u] RoachMenuFuncSyncUpload onEnter rosync_uploadStart\r\n", millis());
            #ifndef DEVMODE_NO_RADIO
            rosync_uploadStart();
            #endif
        };

        virtual void draw_sidebar(void)
        {
            drawSideBar("BACK", "REDO", true);
        };

        virtual void draw_title(void)
        {
            drawTitleBar((const char*)_txt, true, true, false);
        };

        virtual void onButton(uint8_t btn)
        {
            RoachMenuFunctionItem::onButton(btn);
            switch (btn)
            {
                case BTNID_G6:
                    _exit = EXITCODE_BACK;
                    break;
                case BTNID_G5:
                    debug_printf("[%u] RoachMenuFuncSyncUpload onButton BTNID_G5 rosync_uploadStart\r\n", millis());
                    #ifndef DEVMODE_NO_RADIO
                    rosync_uploadStart();
                    #endif
                    break;
            }
        };
};

class RoachMenuFuncUsbMsd : public RoachMenuFunctionItem
{
    public:
        RoachMenuFuncUsbMsd(void) : RoachMenuFunctionItem("USB MSD")
        {
        };

    protected:
        virtual void onEnter(void)
        {
            RoachMenu::onEnter();
            if (RoachUsbMsd_hasVbus() && RoachUsbMsd_isUsbPresented() == false)
            {
                debug_printf("[%u] RoachMenuFuncUsbMsd onEnter RoachUsbMsd_presentUsbMsd\r\n", millis());
                RoachUsbMsd_presentUsbMsd();
                showMessage("USB MSD", "connecting");
            }
            else
            {
                debug_printf("[%u] RoachMenuFuncUsbMsd onEnter unable to USB connect\r\n", millis());
                showError("unable USB conn");
            }
            _exit = EXITCODE_BACK;
        };

        virtual void onButton(uint8_t btn)
        {
            RoachMenuFunctionItem::onButton(btn);
            switch (btn)
            {
                case BTNID_G6:
                    _exit = EXITCODE_BACK;
                    break;
                case BTNID_G5:
                    break;
            }
        };
};

class RoachMenuFuncRegenRf : public RoachMenuFunctionItem
{
    public:
        RoachMenuFuncRegenRf(void) : RoachMenuFunctionItem("regen RF")
        {
        };





        virtual void draw(void)
        {
            int y = 0;
            oled.setCursor(0, y);
            oled.printf("UID : 0x%08X", _uid);
            y += ROACHGUI_LINE_HEIGHT;
            oled.setCursor(0, y);
            oled.printf("SALT: 0x%08X", _salt);
            if (RoachUsbMsd_canSave())
            {
                y += ROACHGUI_LINE_HEIGHT;
                oled.setCursor(0, y);
                oled.printf("%c ACCEPT+SAVE", GUISYMB_LEFT_ARROW);
                y += ROACHGUI_LINE_HEIGHT;
                oled.setCursor(0, y);
                oled.printf("%c NEW FILE", GUISYMB_RIGHT_ARROW);
            }
            else
            {
                y += ROACHGUI_LINE_HEIGHT;
                oled.setCursor(0, y);
                oled.printf("%c ACCEPT", GUISYMB_LEFT_ARROW);
                y += ROACHGUI_LINE_HEIGHT;
                oled.setCursor(0, y);
                oled.printf("WARN: cannot save");
                y += ROACHGUI_LINE_HEIGHT;
                oled.setCursor(0, y);
                oled.printf("while USB is MSD");
            }
        };

    protected:
        uint32_t _uid;
        uint32_t _salt;

        virtual void onEnter(void)
        {
            RoachMenu::onEnter();
            _uid = nrf5rand_u32();
            _salt = nrf5rand_u32();
        };

        virtual void draw_sidebar(void)
        {
            drawSideBar("BACK", "REDO", true);
        };

        virtual void draw_title(void)
        {
            drawTitleBar((const char*)_txt, true, true, false);
        };

        virtual void onButton(uint8_t btn)
        {
            RoachMenuFunctionItem::onButton(btn);
            switch (btn)
            {
                case BTNID_LEFT:
                    nvm_rf.uid = _uid;
                    nvm_rf.salt = _salt;
                    if (RoachUsbMsd_canSave())
                    {
                        debug_printf("[%u] saving new RF params %08X %08X\r\n", millis(), _uid, _salt);
                        if (settings_save() == false) {
                            showError("did not save");
                        }
                    }
                    else
                    {
                        debug_printf("[%u] cannot save new RF params\r\n", millis());
                        showError("cannot save");
                        settings_markDirty();
                    }
                    _exit = EXITCODE_BACK;
                    break;
                case BTNID_RIGHT:
                    if (RoachUsbMsd_canSave())
                    {
                        char newfilename[32];
                        sprintf(newfilename, "rf_%08X.txt", _uid);
                        debug_printf("[%u] saving new RF params to %s\r\n", millis(), newfilename);
                        if (settings_saveToFile(newfilename)) {
                            showMessage("saved file", newfilename);
                        }
                        else {
                            showError("cannot save file");
                        }
                    }
                    else
                    {
                        debug_printf("[%u] cannot save new RF params\r\n", millis());
                        showError("cannot save");
                    }
                    break;
                case BTNID_G6:
                    _exit = EXITCODE_BACK;
                    break;
                case BTNID_G5:
                    _uid = nrf5rand_u32();
                    _salt = nrf5rand_u32();
                    break;
            }
        };
};

class RoachMenuCalibSync : public RoachMenuLister
{
    public:
        RoachMenuCalibSync(void) : RoachMenuLister(MENUID_CONFIG_CALIBSYNC)
        {
            addNode((RoachMenuListItem*)(new RoachMenuFuncCalibGyro()));
            addNode((RoachMenuListItem*)(new RoachMenuFuncCalibAdcCenter()));
            addNode((RoachMenuListItem*)(new RoachMenuFuncCalibAdcLimits()));
            addNode((RoachMenuListItem*)(new RoachMenuFuncSyncDownload()));
            addNode((RoachMenuListItem*)(new RoachMenuFuncSyncUpload()));
            addNode((RoachMenuListItem*)(new RoachMenuFuncUsbMsd()));
            addNode((RoachMenuListItem*)(new RoachMenuFuncRegenRf()));

        };

    protected:
        virtual void draw_sidebar(void)
        {
            drawSideBar("EXIT", "RUN", true);
        };

        virtual void draw_title(void)
        {
            drawTitleBar("CALIB/SYNC", true, true, true);
        };

        virtual void onButton(uint8_t btn)
        {
            RoachMenuLister::onButton(btn);
            if (_exit == 0)
            {
                switch (btn)
                {
                    case BTNID_G6:
                        _exit = EXITCODE_HOME;
                        break;
                    case BTNID_LEFT:
                        _exit = EXITCODE_LEFT;
                        break;
                    case BTNID_RIGHT:
                        _exit = EXITCODE_RIGHT;
                        break;
                    case BTNID_CENTER:
                        {
                            RoachMenuFunctionItem* itm = (RoachMenuFunctionItem*)getNodeAt(_list_idx);
                            itm->run();
                        }
                        break;
                }
            }
        };
};

void menu_install_calibSync(void)
{
    menu_install(new RoachMenuCalibSync());
}
# 1 "D:/GithubRepos/robots/Roach/RoachRcTx/CmdLine.ino"
const cmd_def_t cmds[] = {
    { "factoryreset", factory_reset_func},
    { "echo" , echo_func },
    { "mem" , memcheck_func },
    { "perf" , perfcheck_func },
    { "reboot" , reboot_func },
    { "debug" , debug_func },
    { "usbmsd" , usbmsd_func },
    { "unusbmsd" , unusbmsd_func },
    { "conttx" , conttx_func },
    { "regenrf" , regenrf_func },
    { "readrf" , readrf_func },
    { "save" , save_func },
    { "dfu" , dfuenter_func },
    { "fakebtn" , fakebtn_func },
    { "fakepott" , fakepott_func },
    { "fakepots" , fakepots_func },
    { "fakepotw" , fakepotw_func },
    { "fakeenc" , fakeenc_func },
    { "", NULL },
};

RoachCmdLine cmdline(&Serial, (cmd_def_t*)cmds, false, (char*)">>>", (char*)"???", true, 512);

void factory_reset_func(void* cmd, char* argstr, Stream* stream)
{
    settings_factoryReset();
    if (settings_save()) {
        stream->println("factory reset performed");
    }
    else {
        stream->println("factory reset failed");
    }
}

void save_func(void* cmd, char* argstr, Stream* stream)
{
    if (settings_save()) {
        stream->println("settings saved");
    }
    else {
        stream->println("settings save failed");
    }
}

void echo_func(void* cmd, char* argstr, Stream* stream)
{
    stream->println(argstr);
}

void reboot_func(void* cmd, char* argstr, Stream* stream)
{
    stream->println("rebooting...\r\n\r\n");
    delay(100);
    NVIC_SystemReset();
}

void usbmsd_func(void* cmd, char* argstr, Stream* stream)
{
    stream->printf("presenting USB MSD\r\n");
    waitFor(300);
    RoachUsbMsd_presentUsbMsd();
}

void unusbmsd_func(void* cmd, char* argstr, Stream* stream)
{
    stream->printf("disconnecting USB MSD\r\n");
    waitFor(300);
    RoachUsbMsd_unpresent();
}

void memcheck_func(void* cmd, char* argstr, Stream* stream)
{
    stream->printf("free mem: %u\r\n", PerfCnt_ram());
}

void perfcheck_func(void* cmd, char* argstr, Stream* stream)
{
    stream->printf("perf: %u / sec\r\n", PerfCnt_get());
}

void debug_func(void* cmd, char* argstr, Stream* stream)
{
    stream->printf("derp\r\n");
}

void conttx_func(void* cmd, char* argstr, Stream* stream)
{
    int f = atoi(argstr);
    stream->printf("RF cont-tx test f=%d\r\n", f);
    radio.contTxTest(f, false);
}

void regenrf_func(void* cmd, char* argstr, Stream* stream)
{
    nvm_rf.uid = nrf5rand_u32();
    nvm_rf.salt = nrf5rand_u32();
    stream->printf("new RF params 0x%08X 0x%08X\r\n", nvm_rf.uid, nvm_rf.salt);
}

void readrf_func(void* cmd, char* argstr, Stream* stream)
{
    stream->printf("RF params: 0x%08X 0x%08X 0x%08X\r\n", nvm_rf.uid, nvm_rf.salt, nvm_rf.chan_map);
}

void dfuenter_func(void* cmd, char* argstr, Stream* stream)
{
    stream->printf("ENTERING DFU\r\n");
    waitFor(200);
    if (atoi(argstr) == 2) {
        enterUf2Dfu();
    }
    else {
        enterSerialDfu();
    }
}

void fakebtn_func(void* cmd, char* argstr, Stream* stream)
{
    bool done = false;
    if (memcmp("up", argstr, 1) == 0) {
        btn_up.fakePress();
        done = true;
    }
    if (memcmp("down", argstr, 1) == 0) {
        btn_down.fakePress();
        done = true;
    }
    if (memcmp("left", argstr, 1) == 0) {
        btn_left.fakePress();
        done = true;
    }
    if (memcmp("right", argstr, 1) == 0) {
        btn_right.fakePress();
        done = true;
    }
    if (memcmp("center", argstr, 1) == 0) {
        btn_center.fakePress();
        done = true;
    }
    if (memcmp("5", argstr, 1) == 0) {
        btn_g5.fakePress();
        done = true;
    }
    if (memcmp("6", argstr, 1) == 0) {
        btn_g6.fakePress();
        done = true;
    }
    if (memcmp("1", argstr, 1) == 0) {
        btn_sw1.fakeToggle();
        done = true;
    }
    if (memcmp("2", argstr, 1) == 0) {
        btn_sw2.fakeToggle();
        done = true;
    }
    if (memcmp("3", argstr, 1) == 0) {
        btn_sw3.fakeToggle();
        done = true;
    }
    if (done) {
        stream->printf("fake btn press %s\r\n", argstr);
    }
    else {
        stream->printf("fake btn %s error: not found\r\n", argstr);
    }
}

void fakepots_func(void* cmd, char* argstr, Stream* stream)
{
    int x = atoi(argstr);
    pot_steering.simulate(x);
    stream->printf("fake steering %d\r\n", x);
}

void fakepott_func(void* cmd, char* argstr, Stream* stream)
{
    int x = atoi(argstr);
    pot_throttle.simulate(x);
    stream->printf("fake throttle %d\r\n", x);
}

void fakepotw_func(void* cmd, char* argstr, Stream* stream)
{
    int x = atoi(argstr);
    pot_weapon.simulate(x);
    stream->printf("fake weapon %d\r\n", x);
}

void fakeenc_func(void* cmd, char* argstr, Stream* stream)
{
    int x = atoi(argstr);
    RoachEnc_simulate(x);
    stream->printf("fake encoder %d\r\n", x);
}
# 1 "D:/GithubRepos/robots/Roach/RoachRcTx/Gui.ino"
#include "MenuClass.h"

uint32_t gui_last_draw_time = 0;

void gui_init(void)
{
    oled.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS);
    oled.setRotation(0);
    oled.setTextColor(SSD1306_WHITE, SSD1306_BLACK);
    oled.setCursor(0, 0);
    oled.setTextWrap(false);
    gui_drawSplash();
    menu_setup();
}

void gui_reboot(void)
{
    oled.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS);
}

extern const uint8_t splash[];

void gui_drawSplash(void)
{
    oled.clearDisplay();
    oled.drawBitmap(0, 0, splash, SCREEN_WIDTH, SCREEN_HEIGHT, SSD1306_WHITE);
    gui_drawNow();
}

bool gui_canDisplay(void)
{
    if (nbtwi_isBusy() != false) {
        return false;
    }
    #ifndef DEVMODE_NO_RADIO
    if (radio.isBusy())
    {
        uint32_t now = millis();
        if ((now - gui_last_draw_time) >= 80) {
            gui_last_draw_time = now;
            return true;
        }
    }
    #else
    return true;
    #endif
    return false;
}

void gui_drawWait(void)
{
    while (nbtwi_isBusy()) {
        yield();
        ctrler_tasks();
    }
}

void gui_drawNow(void)
{
    oled.display();
    while (nbtwi_isBusy()) {
        yield();
        ctrler_tasks();
    }
}

void showError(const char* s)
{
    btns_disableAll();
    oled.clearDisplay();
    oled.setCursor(0, 0);
    oled.print("ERROR:");
    oled.setCursor(0, ROACHGUI_LINE_HEIGHT);
    oled.print(s);
    oled.display();
    uint32_t t = millis();
    while ((millis() - t) < ROACHGUI_ERROR_SHOW_TIME) {
        yield();
        ctrler_tasks();
        if (btns_hasAnyPressed()) {
            break;
        }
    }
}

void showMessage(const char* s1, const char* s2)
{
    btns_disableAll();
    oled.clearDisplay();
    oled.setCursor(0, 0);
    oled.print(s1);
    oled.setCursor(0, ROACHGUI_LINE_HEIGHT);
    oled.print(s2);
    oled.display();
    uint32_t t = millis();
    while ((millis() - t) < ROACHGUI_ERROR_SHOW_TIME) {
        yield();
        ctrler_tasks();
        if (btns_hasAnyPressed()) {
            break;
        }
    }
}

void printEllipses(void)
{
    uint8_t ticks;
    uint32_t now = millis();
    uint32_t now_mod = now % 1500;
    ticks = (now_mod / 500) + 1;
    uint8_t i;
    for (i = 0; i < 3; i++)
    {
        oled.write(i < ticks ? '.' : ' ');
    }
}

void drawSideBar(const char* upper, const char* lower, bool line)
{
    int i, x, y;
    int slen;
    if (upper != NULL)
    {
        x = SCREEN_WIDTH - 5;
        slen = strlen(upper);
        for (i = 0, y = 0; i < slen; i++, y += ROACHGUI_LINE_HEIGHT)
        {
            oled.setCursor(x, y);
            oled.write(upper[i]);
        }
    }
    if (lower != NULL)
    {
        slen = strlen(lower);
        x = SCREEN_WIDTH - 10;
        y = SCREEN_HEIGHT - (slen * ROACHGUI_LINE_HEIGHT);
        for (i = 0; i < slen; i++, y += ROACHGUI_LINE_HEIGHT)
        {
            oled.setCursor(x, y);
            oled.write(lower[i]);
        }
    }
    if (line)
    {
        x = SCREEN_WIDTH - 12;
        oled.drawFastVLine(x, 0, SCREEN_HEIGHT, 1);
    }
}

void drawTitleBar(const char* s, bool center, bool line, bool arrows)
{
    int slen = strlen(s);
    int x, y;
    x = center ? (((SCREEN_WIDTH - 12) / 2) - ((slen * 6) / 2)) : 0;
    oled.setCursor(x, 0);
    oled.print(s);
    if (arrows)
    {
        oled.setCursor(0, 0);
        oled.write((char)GUISYMB_LEFT_ARROW);
        oled.setCursor(SCREEN_WIDTH - 12 - 6, 0);
        oled.write((char)GUISYMB_RIGHT_ARROW);
    }
    if (line)
    {
        y = 9;
        oled.drawFastHLine(0, y, SCREEN_WIDTH - 12, 1);
    }
}

void drawProgressBar(int x, int y, int w, int h, int prog, int total)
{
    oled.drawRect(x, y, w, h, SSD1306_WHITE);
    oled.fillRect(x, y, map(prog, 0, total, 0, w), h, SSD1306_WHITE);
}
# 1 "D:/GithubRepos/robots/Roach/RoachRcTx/Heartbeat.ino"
static const uint8_t hbani_triple_fast[] = { 0x11, 0x11, 0x1A, 0x00, };
static const uint8_t hbani_double_fast[] = { 0x11, 0x1A, 0x00, };
static const uint8_t hbani_double_slow[] = { 0x53, 0x5B, 0x00, };
static const uint8_t hbani_single_fast[] = { 0x1B, 0x00, };
static const uint8_t hbani_single_slow[] = { 0x5B, 0x00, };

void heartbeat_task()
{
    static bool prev_radio_connected = false;
    static bool prev_usb_connected = false;
    bool _radio_connected = radio.isConnected();
    bool _usb_connected = RoachUsbMsd_isUsbPresented();

    hb_red.task();

    if (_radio_connected && prev_radio_connected == false)
    {

        hb_red.play(_usb_connected ? hbani_triple_fast : hbani_double_fast);
    }
    else if (_radio_connected == false && prev_radio_connected != false)
    {

        hb_red.play(_usb_connected ? hbani_single_slow : hbani_single_fast);
    }

    if (_usb_connected && prev_usb_connected == false)
    {
        hb_red.play(_radio_connected ? hbani_triple_fast : hbani_single_slow);
    }
    else if (_usb_connected == false && prev_usb_connected != false)
    {
        hb_red.play(_radio_connected ? hbani_double_fast : hbani_single_fast);
    }

    prev_radio_connected = _radio_connected;
    prev_usb_connected = _usb_connected;
}
# 1 "D:/GithubRepos/robots/Roach/RoachRcTx/HwBringup.ino"
void hw_bringup(void)
{
    uint32_t now;
    pinMode(ROACHHW_PIN_LED_RED, OUTPUT);
    Serial.begin(115200);

#if 0

    pinMode(ROACHHW_PIN_BTN_UP, INPUT_PULLUP);
    pinMode(ROACHHW_PIN_BTN_DOWN, INPUT_PULLUP);
    pinMode(ROACHHW_PIN_BTN_LEFT, INPUT_PULLUP);
    pinMode(ROACHHW_PIN_BTN_RIGHT, INPUT_PULLUP);
    pinMode(ROACHHW_PIN_BTN_CENTER, INPUT_PULLUP);
    pinMode(ROACHHW_PIN_BTN_G5, INPUT_PULLUP);
    pinMode(ROACHHW_PIN_BTN_G6, INPUT_PULLUP);

    uint8_t px = 0;
    while (1)
    {
        static uint32_t lt = 0;
        now = millis();
        digitalWrite(ROACHHW_PIN_LED_RED, ((now % 500) <= 100));

        uint8_t x = 0;
        x |= (digitalRead(ROACHHW_PIN_BTN_UP) == LOW ? 1 : 0) << 0;
        x |= (digitalRead(ROACHHW_PIN_BTN_DOWN) == LOW ? 1 : 0) << 1;
        x |= (digitalRead(ROACHHW_PIN_BTN_LEFT) == LOW ? 1 : 0) << 2;
        x |= (digitalRead(ROACHHW_PIN_BTN_RIGHT) == LOW ? 1 : 0) << 3;
        x |= (digitalRead(ROACHHW_PIN_BTN_CENTER) == LOW ? 1 : 0) << 4;
        x |= (digitalRead(ROACHHW_PIN_BTN_G5) == LOW ? 1 : 0) << 5;
        x |= (digitalRead(ROACHHW_PIN_BTN_G6) == LOW ? 1 : 0) << 6;
        if (x != px && (now - lt) >= 100)
        {
            Serial.printf("[%u]: 0x%02X\r\n", millis(), x);
            px = x;
            lt = now;
        }
        yield();
    }

#endif

    RoachButton_allBegin();
    RoachPot_allBegin();
    RoachEnc_begin(ROACHHW_PIN_ENC_A, ROACHHW_PIN_ENC_B);
    nbtwi_init(ROACHHW_PIN_I2C_SCL, ROACHHW_PIN_I2C_SDA, (SCREEN_WIDTH * SCREEN_HEIGHT / 8) * 2);
    Serial.println("\r\nRoach RC Transmitter - HW Bringup!");
    while (true)
    {
        static uint32_t lt = 0;
        static uint32_t pb = 0;
        now = millis();

        bool need_show = false;
        if ((now - lt) >= 200)
        {
            need_show = true;
        }

        RoachButton_allTask();
        RoachPot_allTask();
        RoachEnc_task();

        uint32_t b = RoachButton_isAnyHeld();
        if (b != pb)
        {
            need_show = true;
            pb = b;
        }

        if (RoachEnc_hasMoved(true))
        {
            need_show = true;
        }

        if (need_show)
        {
            lt = now;
            Serial.printf("[%u]: 0x%08X, %d, ", now, b, RoachEnc_get(false));

            int i;
            #if 1
            for (i = 0; i < POT_CNT_MAX; i++)
            {
                int x = RoachPot_getRawAtIdx(i);
                if (x >= 0) {
                    Serial.printf("%d, ", x);
                }
                else {
                    break;
                }
            }
            #else
            Serial.printf("%d, ", analogRead(ROACHHW_PIN_POT_THROTTLE));
            Serial.printf("%d, ", analogRead(ROACHHW_PIN_POT_STEERING));
            Serial.printf("%d, ", analogRead(ROACHHW_PIN_POT_WEAPON));
            Serial.printf("%d, ", analogRead(ROACHHW_PIN_POT_AUX));
            Serial.printf("%d, ", analogRead(ROACHHW_PIN_POT_BATTERY));
            #endif
            Serial.printf("\r\n");
        }

        digitalWrite(ROACHHW_PIN_LED_RED, ((now % 500) <= 100));
        yield();
    }
}
# 1 "D:/GithubRepos/robots/Roach/RoachRcTx/Menu.ino"
#include "MenuClass.h"

extern bool switches_alarm;

extern const uint8_t icon_disconnected[];
extern const uint8_t icon_connected[];
extern const uint8_t icon_mismatched[];

int printRfStats(int y)
{
    oled.setCursor(0, y);

    if (radio.isConnected() == false)
    {
        oled.drawBitmap(0, 0, icon_disconnected, 16, 8, SSD1306_WHITE);
    }
    else
    {
        if (rosync_isSynced())
        {
            oled.drawBitmap(0, 0, icon_connected, 16, 8, SSD1306_WHITE);
        }
        else
        {
            oled.drawBitmap(0, 0, icon_mismatched, 16, 8, SSD1306_WHITE);
        }
        oled.setCursor(16, y);

        oled.printf("%d %d %0.1f", radio.getRssi(), telem_pkt.rssi, ((float)telem_pkt.loss_rate) / 100.0);
    }
    return y;
}

class RoachMenuHome : public RoachMenu
{
    public:
        RoachMenuHome() : RoachMenu(MENUID_HOME)
        {
        };

        virtual void taskLP(void)
        {
            RoachMenu::taskLP();
            #ifdef ROACHTX_AUTOSAVE
            settings_saveIfNeeded(2 * 1000);
            #endif
            #ifdef ROACHTX_AUTOEXIT
            gui_last_activity_time = 0;
            #endif
        };

        virtual void draw(void)
        {
            oled.fillRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 0);
            draw_title();
            draw_sidebar();

            int y = printRfStats(0);
            y += ROACHGUI_LINE_HEIGHT;
            oled.setCursor(0, y);
            oled.printf("%c%-4d %c%d", tx_pkt.throttle >= 0 ? 0x18 : 0x19, abs(tx_pkt.throttle), tx_pkt.steering < 0 ? 0x1B : 0x1A, abs(tx_pkt.steering));
            y += ROACHGUI_LINE_HEIGHT;
            oled.setCursor(0, y);
            if (radio.isConnected() && telem_pkt.heading == 0x7FFF) {
                oled.printf("IMUFAIL");
            }
            else if (radio.isConnected()) {
                oled.printf("H:%-4d  %d", roach_div_rounded(tx_pkt.heading, 100), telem_pkt.heading);
            }
            else {
                oled.printf("H:%-4d", roach_div_rounded(tx_pkt.heading, 100));
            }
            y += ROACHGUI_LINE_HEIGHT;
            oled.setCursor(0, y);
            oled.printf("%d %d %c%c%c"
                #ifdef ROACHHW_PIN_BTN_SW4
                "%c"
                #endif
                , tx_pkt.pot_weap
                , tx_pkt.pot_aux
                , ((tx_pkt.flags & ROACHPKTFLAG_BTN1) != 0 ? 0x07 : 0x09)
                , ((tx_pkt.flags & ROACHPKTFLAG_BTN2) != 0 ? 0x07 : 0x09)
                , ((tx_pkt.flags & ROACHPKTFLAG_BTN3) != 0 ? 0x07 : 0x09)
                #ifdef ROACHHW_PIN_BTN_SW4
                , ((tx_pkt.flags & ROACHPKTFLAG_BTN4) != 0 ? 0x07 : 0x09)
                #endif
                );
            if (switches_alarm) {
                oled.print("!");
            }



        };

    protected:
        virtual void draw_sidebar(void)
        {
            drawSideBar("CONF", "PAGE", true);
        };

        virtual void draw_title(void)
        {
        };

        virtual void onButton(uint8_t btn)
        {
            RoachMenu::onButton(btn);
            QuickAction_check(btn);
            switch (btn)
            {
                case BTNID_G5:
                    _exit = EXITCODE_LEFT;
                    break;
                case BTNID_G6:
                    _exit = EXITCODE_RIGHT;
                    break;
            }
        };
};

class RoachMenuInfo : public RoachMenu
{
    public:
        RoachMenuInfo() : RoachMenu(MENUID_INFO)
        {
        };

        #ifdef ROACHTX_AUTOSAVE
        virtual void taskLP(void)
        {
            RoachMenu::taskLP();
            settings_saveIfNeeded(2 * 1000);
        };
        #endif

        virtual void draw(void)
        {
            oled.fillRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 0);
            draw_title();
            draw_sidebar();

            int y = printRfStats(0);
            y += ROACHGUI_LINE_HEIGHT;
            oled.setCursor(0, y);
            oled.printf("UID %08X", nvm_rf.uid);






            y += ROACHGUI_LINE_HEIGHT;
            oled.setCursor(0, y);
            rosync_drawShort();
            y += ROACHGUI_LINE_HEIGHT;
            y = printRfStats(y);
            y += ROACHGUI_LINE_HEIGHT;
            oled.setCursor(0, y);
            oled.printf("dsk free %d kb", _freeSpace);
            #ifdef PERFCNT_ENABLED
            y += ROACHGUI_LINE_HEIGHT;
            oled.setCursor(0, y);
            oled.printf("RAM free %d b", PerfCnt_ram());
            #endif
        };

    protected:
        uint64_t _freeSpace = 0;

        virtual void onEnter(void)
        {
            RoachMenu::onEnter();
            _freeSpace = RoachUsbMsd_getFreeSpace();
        };

        virtual void draw_sidebar(void)
        {
            drawSideBar("CONF", "PAGE", true);
        };

        virtual void draw_title(void)
        {
        };

        virtual void onButton(uint8_t btn)
        {
            RoachMenu::onButton(btn);
            switch (btn)
            {
                case BTNID_G5:
                    _exit = EXITCODE_LEFT;
                    break;
                case BTNID_G6:
                    _exit = EXITCODE_RIGHT;
                    break;
                default:
                    _exit = EXITCODE_HOME;
                    break;
            }
        };
};

RoachMenuHome menuHome;
RoachMenuInfo menuInfo;
RoachMenu* current_menu = NULL;
RoachMenu* menuListHead = NULL;
RoachMenu* menuListTail = NULL;
RoachMenu* menuListCur = NULL;

void menu_run(void)
{
    if (current_menu == NULL)
    {
        current_menu = &menuHome;
        Serial.println("current menu is null, assigning home screen");
    }
    Serial.printf("[%u]: running menu screen ID=%d\r\n", millis(), current_menu->getId());
    current_menu->run();
    int ec = current_menu->getExitCode();
    Serial.printf("[%u]: menu exited to top level, exitcode=%d\r\n", millis(), ec);
    if (ec == EXITCODE_HOME) {
        current_menu = &menuHome;
    }
    else if (ec == EXITCODE_BACK && current_menu->parent_menu != NULL) {
        current_menu = (RoachMenu*)current_menu->parent_menu;
    }
    else if (ec == EXITCODE_BACK && current_menu->parent_menu == NULL) {
        current_menu = (RoachMenu*)&menuHome;
    }
    else if (current_menu == &menuHome && ec == EXITCODE_LEFT) {
        current_menu = &menuInfo;
    }
    else if ((current_menu == &menuHome || current_menu == &menuInfo) && ec == EXITCODE_LEFT) {
        current_menu = (RoachMenu*)menuListCur;
        menuListCur = current_menu;
    }
    else if (ec == EXITCODE_LEFT) {
        current_menu = (RoachMenu*)menuListCur->prev_menu;
        menuListCur = current_menu;
    }
    else if (ec == EXITCODE_RIGHT) {
        current_menu = (RoachMenu*)menuListCur->next_menu;
        menuListCur = current_menu;
    }
    else if (ec == EXITCODE_BACK) {
        current_menu = (RoachMenu*)menuListCur->next_menu;
        menuListCur = current_menu;
    }
    else {
        Serial.printf("[%u] err, menu exit (%u) unhandled\r\n", ec);
    }
}

void menu_setup(void)
{



    menu_install_calibSync();
    menu_install_robot();
    menu_install(new RoachMenuCfgLister(MENUID_CONFIG_CTRLER, "CONTROLLER", "ctrler", &nvm_tx, cfgdesc_ctrler));
    menu_install_fileOpener();
}

void menu_install(RoachMenu* m)
{
    if (menuListHead == NULL) {
        menuListHead = m;
        menuListTail = m;
        menuListCur = m;
    }
    else
    {
        menuListTail->next_menu = m;
        menuListTail = m;
        menuListHead->prev_menu = m;
    }
}
# 1 "D:/GithubRepos/robots/Roach/RoachRcTx/MenuClass.ino"
#include "MenuClass.h"

uint32_t gui_last_activity_time = 0;

RoachMenu::RoachMenu(uint8_t id)
{
    _id = id;
};

void RoachMenu::draw(void)
{
};

void RoachMenu::clear(void)
{
    oled.clearDisplay();
};

void RoachMenu::display(void)
{
    oled.display();
};

void RoachMenu::taskHP(void)
{
    ctrler_tasks();
    nbtwi_task();
};

void RoachMenu::taskLP(void)
{
    #ifdef ROACHTX_AUTOSAVE
    settings_saveIfNeeded(10 * 1000);
    #endif
};

void RoachMenu::run(void)
{
    _running = true;
    onEnter();

    while (_exit == 0)
    {
        yield();

        taskHP();

        if (gui_canDisplay() == false) {
            continue;
        }

        checkButtons();

        #ifdef ROACHTX_AUTOEXIT
        #ifndef DEVMODE_SUBMENU_FOREVER
        if (gui_last_activity_time != 0 && _id != MENUID_HOME)
        {
            if ((millis() - gui_last_activity_time) >= 10000) {
                gui_last_activity_time = 0;
                _exit = EXITCODE_HOME;
            }
        }
        #endif
        #endif

        if (_interrupt == EXITCODE_BACK) {
            _exit = EXITCODE_BACK;
        }

        if (_exit == 0)
        {
            taskLP();

            draw();

            display();
        }

        #ifdef DEVMODE_SLOW_LOOP
        waitFor(DEVMODE_SLOW_LOOP);
        #endif
    }

    onExit();
    _running = false;
}

void RoachMenu::draw_sidebar(void)
{

};

void RoachMenu::draw_title(void)
{

};

void RoachMenu::onEnter(void)
{
    _exit = 0;
    clear();
}

void RoachMenu::onExit(void)
{
}

void RoachMenu::onButton(uint8_t btn)
{
}

void RoachMenu::onButtonCheckExit(uint8_t btn)
{

}

void RoachMenu::checkButtons(void)
{
    if (btn_up.hasPressed(true)) {
        gui_last_activity_time = millis();
        debug_printf("[%u] btn up\r\n", millis());
        this->onButton(BTNID_UP);
    }
    if (btn_down.hasPressed(true)) {
        gui_last_activity_time = millis();
        debug_printf("[%u] btn down\r\n", millis());
        this->onButton(BTNID_DOWN);
    }
    if (btn_left.hasPressed(true)) {
        gui_last_activity_time = millis();
        debug_printf("[%u] btn left\r\n", millis());
        this->onButton(BTNID_LEFT);
    }
    if (btn_right.hasPressed(true)) {
        gui_last_activity_time = millis();
        debug_printf("[%u] btn right\r\n", millis());
        this->onButton(BTNID_RIGHT);
    }
    if (btn_center.hasPressed(true)) {
        gui_last_activity_time = millis();
        debug_printf("[%u] btn center\r\n", millis());
        this->onButton(BTNID_CENTER);
    }
    if (btn_g5.hasPressed(true)) {
        gui_last_activity_time = millis();
        debug_printf("[%u] btn g5\r\n", millis());
        this->onButton(BTNID_G5);
    }
    if (btn_g6.hasPressed(true)) {
        gui_last_activity_time = millis();
        debug_printf("[%u] btn g6\r\n", millis());
        this->onButton(BTNID_G6);
    }
}

RoachMenuListItem::~RoachMenuListItem(void)
{
    if (_txt) {
        free(_txt);
    }
}

void RoachMenuListItem::sprintName(char* s)
{
    strcpy(s, _txt);
}

char* RoachMenuListItem::getName(void)
{
    return _txt;
}

RoachMenuFunctionItem::RoachMenuFunctionItem(const char* name)
{
    int slen = strlen(name);
    _txt = (char*)malloc(slen + 1);
    strncpy0(_txt, name, slen);
}

RoachMenuFileItem::RoachMenuFileItem(const char* fname)
{
    int slen = strlen(fname);
    _txt = (char*)malloc(slen + 1);
    _fname = (char*)malloc(slen + 1);
    strncpy0(_txt, fname, slen);
    strncpy0(_fname, fname, slen);
    int i;
    for (i = 2; i < slen - 4; i++)
    {
        if ( memcmp(".txt", &(_txt[i]), 5) == 0
            || memcmp(".TXT", &(_txt[i]), 5) == 0)
        {
            _txt[i] = 0;
        }
    }
}

RoachMenuFileItem::~RoachMenuFileItem(void)
{
    if (_fname) {
        free(_fname);
    }
    if (_txt) {
        free(_txt);
    }
}

void RoachMenuFileItem::sprintFName(char* s)
{
    strcpy(s, _fname);
}

char* RoachMenuFileItem::getFName(void)
{
    return _fname;
}

RoachMenuCfgItem::RoachMenuCfgItem(void* struct_ptr, roach_nvm_gui_desc_t* desc) : RoachMenuListItem()
{
    _struct = struct_ptr;
    _desc = desc;
}

void RoachMenuCfgItem::sprintName(char* s)
{
    strcpy(s, _desc->name);
}

char* RoachMenuCfgItem::getName(void)
{
    return _desc->name;
}

uint32_t cfg_last_change_time = 0;

RoachMenuCfgItemEditor::RoachMenuCfgItemEditor(void* struct_ptr, roach_nvm_gui_desc_t* desc) : RoachMenu(0)
{
    _struct = struct_ptr;
    _desc = desc;
}

void RoachMenuCfgItemEditor::onEnter(void)
{
    RoachMenu::onEnter();
    RoachEnc_get(true);
    encoder_mode = ENCODERMODE_USEPOT;
}

void RoachMenuCfgItemEditor::taskLP(void)
{







}

void RoachMenuCfgItemEditor::onExit(void)
{




    RoachMenu::onExit();
    encoder_mode = 0;
}

void RoachMenuCfgItemEditor::draw(void)
{
    RoachMenu::draw();

    oled.fillRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 0);
    draw_sidebar();
    draw_title();

    char valstr[64];
    roachnvm_formatitem(valstr, (uint8_t*)_struct, _desc);
    oled.setCursor(0, ROACHGUI_LINE_HEIGHT * 2);
    oled.print(valstr);
}

void RoachMenuCfgItemEditor::draw_sidebar(void)
{
    drawSideBar("SET", "", true);
}

void RoachMenuCfgItemEditor::draw_title(void)
{
    char str[64];
    sprintf(str, "EDIT: %s", _desc->name);
    drawTitleBar(str, true, true, false);
}

void RoachMenuCfgItemEditor::onButton(uint8_t btn)
{
    RoachMenu::onButton(btn);
    switch (btn)
    {
        case BTNID_UP:
            roachnvm_incval((uint8_t*)_struct, _desc, _desc->step);
            cfg_last_change_time = millis();
            settings_markDirty();
            break;
        case BTNID_DOWN:
            roachnvm_incval((uint8_t*)_struct, _desc, -_desc->step);
            cfg_last_change_time = millis();
            settings_markDirty();
            break;
        case BTNID_G6:
            _exit = EXITCODE_BACK;
            break;
    }
}

void RoachMenuCfgItemEditor::checkButtons(void)
{
    RoachMenu::checkButtons();
    int x = RoachEnc_get(true);
    if (x != 0) {
        roachnvm_incval((uint8_t*)_struct, _desc, -x * _desc->step);
        cfg_last_change_time = millis();
        settings_markDirty();
    }
}

RoachMenuLister::RoachMenuLister(uint8_t id) : RoachMenu(id)
{
}

void RoachMenuLister::draw(void)
{
    RoachMenu::draw();

    oled.fillRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 0);
    draw_title();
    draw_sidebar();

    int8_t _draw_start_idx, _draw_end_idx;

    if (_list_cnt <= ROACHMENU_LIST_MAX)
    {
        _draw_start_idx = 0;
        _draw_end_idx = _list_cnt - 1;
    }
    else
    {
        _draw_start_idx = _list_idx - 3;
        _draw_end_idx = _draw_start_idx + ROACHMENU_LIST_MAX;
        if (_draw_start_idx < 0) {
            _draw_start_idx = 0;
            _draw_end_idx = ROACHMENU_LIST_MAX - 1;
        }
        else if (_draw_end_idx >= _list_cnt) {
            _draw_end_idx = _list_cnt;
            _draw_start_idx = _draw_end_idx - ROACHMENU_LIST_MAX;
        }
    }
    int i;
    int j;
    for (i = 0; i < ROACHMENU_LIST_MAX; i++)
    {
        j = _draw_start_idx + i;
        oled.setCursor(0, ROACHGUI_LINE_HEIGHT * (i + 2));
        if (j == _list_idx)
        {
            oled.write((char)GUISYMB_THIS_ARROW);
        }
        else if (i == 0 && _draw_start_idx != 0)
        {
            oled.write((char)GUISYMB_UP_ARROW);
        }
        else if (i >= (ROACHMENU_LIST_MAX - 1) && _draw_end_idx < _list_cnt)
        {
            oled.write((char)GUISYMB_DOWN_ARROW);
        }
        else
        {
            oled.write((char)' ');
        }

        if (j >= _draw_start_idx && j <= _draw_end_idx)
        {
            char* t = getItemText(j);
            if (t != NULL) {
                oled.print(t);
            }
        }
    }
}

void RoachMenuLister::onEnter(void)
{
    RoachMenu::onEnter();
    _list_idx = 0;
}

void RoachMenuLister::buildFileList(const char* filter)
{
    if (!fatroot.open("/"))
    {
        Serial.println("open root failed");
        return;
    }
    while (fatfile.openNext(&fatroot, O_RDONLY))
    {
        if (fatfile.isDir() == false)
        {
            char sfname[64];
            fatfile.getName7(sfname, 62);
            bool matches = false;
            if (filter != NULL) {
                matches = memcmp(filter, sfname, strlen(filter)) == 0;
            }
            else {
                matches = ( memcmp("rf" , sfname, 2) == 0
                           || memcmp("robot" , sfname, 5) == 0
                           || memcmp("ctrler", sfname, 6) == 0
                           || memcmp("rdesc" , sfname, 5) == 0
                          );
            }
            if (matches)
            {
                RoachMenuFileItem* n = new RoachMenuFileItem(sfname);
                if (_head_node == NULL) {
                    _head_node = (RoachMenuListItem*)n;
                    _tail_node = (RoachMenuListItem*)n;
                }
                else if (_tail_node != NULL) {
                    _tail_node->next_node = (void*)n;
                    _tail_node = (RoachMenuListItem*)n;
                }
                _list_cnt += 1;
            }
        }
        fatfile.close();
    }
    RoachMenuFileItem* nc = new RoachMenuFileItem("CANCEL");
    if (_head_node == NULL) {
        _head_node = (RoachMenuListItem*)nc;
        _tail_node = (RoachMenuListItem*)nc;
    }
    else if (_tail_node != NULL) {
        _tail_node->next_node = (void*)nc;
        _tail_node = (RoachMenuListItem*)nc;
    }
    _list_cnt++;
}

void RoachMenuLister::onExit(void)
{
    RoachMenu::onExit();
    if (_head_node == NULL) {
        return;
    }
    while (_head_node != NULL) {
        RoachMenuListItem* n = (RoachMenuListItem*)_head_node->next_node;
        delete _head_node;
        _head_node = n;
    }
}

RoachMenuListItem* RoachMenuLister::getNodeAt(int idx)
{
    int i = 0;
    if (_head_node == NULL) {
        return NULL;
    }

    RoachMenuListItem* n = _head_node;
    while (i != idx && n != NULL) {
        n = (RoachMenuListItem*)(n->next_node);
        i += 1;
    }
    return n;
}

void RoachMenuLister::addNode(RoachMenuListItem* item)
{
    if (_head_node == NULL) {
        _head_node = (RoachMenuListItem*)item;
        _tail_node = (RoachMenuListItem*)item;
    }
    else if (_tail_node != NULL) {
        _tail_node->next_node = (void*)item;
        _tail_node = (RoachMenuListItem*)item;
    }
    _list_cnt++;
}

char* RoachMenuLister::getItemText(int idx)
{
    RoachMenuListItem* n = (RoachMenuListItem*)getNodeAt(idx);
    if (n == NULL) {
        return NULL;
    }
    return n->getName();
}

void RoachMenuLister::onButton(uint8_t btn)
{
    RoachMenu::onButton(btn);
    switch (btn)
    {
        case BTNID_UP:
            _list_idx = (_list_idx > 0) ? (_list_idx - 1) : _list_idx;
            debug_printf("list idx up %d\r\n", _list_idx);
            break;
        case BTNID_DOWN:
            _list_idx = (_list_idx < (_list_cnt - 1)) ? (_list_idx + 1) : _list_idx;
            debug_printf("list idx down %d\r\n", _list_idx);
            break;
    }
}

RoachMenuCfgLister::RoachMenuCfgLister(uint8_t id, const char* name, const char* filter, void* struct_ptr, roach_nvm_gui_desc_t* desc_tbl) : RoachMenuLister(id)
{
    _struct = struct_ptr;
    _desc_tbl = desc_tbl;
    _list_cnt = roachnvm_cntgroup(desc_tbl);
    strncpy0(_title, name, 30);
}

char* RoachMenuCfgLister::getItemText(int idx)
{
    return _desc_tbl[idx].name;
}

void RoachMenuCfgLister::draw_sidebar(void)
{
    if (RoachUsbMsd_canSave()) {
        drawSideBar("EXIT", "SAVE", true);
    }
    else {
        drawSideBar("EXIT", "", true);
    }
}

void RoachMenuCfgLister::draw_title(void)
{
    drawTitleBar((const char*)_title, true, true, true);
}

void RoachMenuCfgLister::onButton(uint8_t btn)
{
    switch (btn)
    {
        case BTNID_LEFT:
            {
                if (_list_idx <= 0) {
                    _exit = EXITCODE_LEFT;
                    break;
                }
                else
                {
                    for (; _list_idx >= 0; _list_idx--)
                    {
                        if (memcmp("cat", _desc_tbl[_list_idx].type_code, 4) == 0)
                        {
                            strncpy0(_title, _desc_tbl[_list_idx].name, 30);
                            break;
                        }
                    }
                }
            }
            break;
        case BTNID_RIGHT:
            {
                if (_list_idx >= _list_cnt - 1) {
                    _exit = EXITCODE_RIGHT;
                    break;
                }
                else
                {
                    for (; _list_idx < _list_cnt; _list_idx--)
                    {
                        if (memcmp("cat", _desc_tbl[_list_idx].type_code, 4) == 0)
                        {
                            strncpy0(_title, _desc_tbl[_list_idx].name, 30);
                            break;
                        }
                    }
                }
            }
            break;
        case BTNID_UP:
        case BTNID_DOWN:
            {
                RoachMenuLister::onButton(btn);
                if (memcmp("cat", _desc_tbl[_list_idx].type_code, 4) == 0)
                {
                    strncpy0(_title, _desc_tbl[_list_idx].name, 30);
                    break;
                }
            }
            break;
        case BTNID_CENTER:
            {
                if (memcmp("cat", _desc_tbl[_list_idx].type_code, 4) != 0)
                {
                    if (memcmp("func", _desc_tbl[_list_idx].type_code, 5) == 0)
                    {

                    }
                    else
                    {
                        RoachMenuCfgItemEditor* n = new RoachMenuCfgItemEditor(_struct, &(_desc_tbl[_list_idx]));
                        n->run();
                        delete n;
                    }
                }
            }
            break;
        case BTNID_G5:
            {
                if (RoachUsbMsd_canSave()) {
                    RoachMenuFileSaveList* n = new RoachMenuFileSaveList(_filter);
                    n->run();
                    delete n;
                }
                else {
                    showError("cannot save");
                }
            }
            break;
        case BTNID_G6:
            {
                _exit = EXITCODE_BACK;
            }
            break;
    }
}
# 1 "D:/GithubRepos/robots/Roach/RoachRcTx/MenuFileOps.ino"
#include "MenuClass.h"

RoachMenuFileOpenList::RoachMenuFileOpenList() : RoachMenuLister(MENUID_CONFIG_FILELOAD)
{
}

void RoachMenuFileOpenList::draw_sidebar(void)
{
    drawSideBar("EXIT", "OPEN", true);
}

void RoachMenuFileOpenList::draw_title(void)
{
    drawTitleBar("LOAD FILE", true, true, false);
}






void RoachMenuFileOpenList::onEnter(void)
{
    RoachMenuLister::onEnter();
    buildFileList(NULL);
}

void RoachMenuFileOpenList::onButton(uint8_t btn)
{
    RoachMenuLister::onButton(btn);
    if (_exit == 0)
    {
        switch (btn)
        {
            case BTNID_G5:
            {
                char* x = getCurItemText();
                if (x != NULL)
                {
                    if (strcmp(x, "CANCEL") == 0) {
                        _exit = EXITCODE_HOME;
                        return;
                    }

                    if ( memcmp(x, "rf", 2) == 0
                        || memcmp(x, "ctrler", 2) == 0
                        || memcmp(x, "robot", 2) == 0
                       )
                    {
                        settings_loadFile(x);
                        showMessage("file loaded from", x);
                    }
                    else {
                        showError("invalid file name");
                    }
                }
                else {
                    showError("file is null");
                }
                _exit = EXITCODE_BACK;
                break;
            }
            case BTNID_G6:
                _exit = EXITCODE_HOME;
                break;
        }
    }
}

RoachMenuFileSaveList::RoachMenuFileSaveList(const char* filter) : RoachMenuLister(0)
{
    strncpy(_filter, filter, 14);
}

void RoachMenuFileSaveList::draw_sidebar(void)
{
    if (RoachUsbMsd_canSave()) {
        drawSideBar("EXIT", "SAVE", true);
    }
    else {
        drawSideBar("EXIT", "", true);
    }
}

void RoachMenuFileSaveList::draw_title(void)
{
    drawTitleBar("SAVE FILE", true, true, false);
}






void RoachMenuFileSaveList::draw(void)
{
    draw_sidebar();
    draw_title();
    RoachMenuLister::draw();
}

void RoachMenuFileSaveList::onEnter(void)
{
    RoachMenuLister::onEnter();
    if (RoachUsbMsd_canSave())
    {
        RoachMenuFileItem* n = new RoachMenuFileItem("NEW FILE");
        _head_node = (RoachMenuListItem*)n;
        _tail_node = (RoachMenuListItem*)n;
        _list_cnt++;
        buildFileList(_filter[0] != 0 ? _filter : NULL);
    }
    else
    {
        RoachMenuFileItem* n = new RoachMenuFileItem("ERR: cannot save");
        _head_node = (RoachMenuListItem*)n;
        _tail_node = (RoachMenuListItem*)n;
        _list_cnt++;
    }
}

void RoachMenuFileSaveList::onButton(uint8_t btn)
{
    RoachMenuLister::onButton(btn);
    if (_exit == 0)
    {
        switch (btn)
        {
            case BTNID_G5:
            {
                if (RoachUsbMsd_canSave() == false) {
                    showError("cannot save");
                    break;
                }
                char* x = getCurItemText();
                if (x != NULL)
                {
                    if (strcmp(x, "CANCEL") == 0) {
                        _exit = EXITCODE_HOME;
                        return;
                    }

                    if (strcmp(x, "NEW FILE") == 0 && _filter[0] != 0)
                    {
                        _newfilename[0] = '/';
                        bool can_save = false;
                        int i;
                        for (i = 1; i <= 9999; i++)
                        {
                            sprintf(&(_newfilename[1]), "%s_%u.txt", _filter, i);
                            if (fatroot.exists(&(_newfilename[1])) == false)
                            {
                                can_save = true;
                                break;
                            }
                        }
                        if (can_save)
                        {
                            if (settings_saveToFile(_newfilename)) {
                                showMessage("new file saved to", _newfilename);
                            }
                            else {
                                showError("cannot save new file");
                            }
                        }
                        else
                        {
                            showError("cannot save new file");
                        }
                    }
                    else if (memcmp(x, "ctrler", 6) == 0 || memcmp(x, "robot", 5) == 0) {
                        if (settings_saveToFile(x)) {
                            showMessage("file saved to", x);
                        }
                        else {
                            showError("cannot save file");
                        }
                    }
                    else {
                        showError("invalid file name");
                    }
                }
                else {
                    showError("file is null");
                }
                _exit = EXITCODE_BACK;
                break;
            }
            case BTNID_G6:
                _exit = EXITCODE_BACK;
                break;
        }
    }
}

void menu_install_fileOpener(void)
{
    menu_install(new RoachMenuFileOpenList());
}
# 1 "D:/GithubRepos/robots/Roach/RoachRcTx/Nvm.ino"
#if defined(ESP32)
#include <SPIFFS.h>
#elif defined(NRF52840_XXAA)
#include <Adafruit_LittleFS.h>
#include <InternalFileSystem.h>
#endif

extern uint8_t* rosync_nvm;
extern roach_nvm_gui_desc_t* rosync_desc_tbl;

roach_nvm_gui_desc_t cfgdesc_ctrler[] = {
    { ((uint32_t)(&(nvm_tx.heading_multiplier )) - (uint32_t)(&nvm_tx)), "H scale" , "s32x10" , 9, INT_MIN, INT_MAX , 1, },

    { ((uint32_t)(&(nvm_tx.pot_throttle.center )) - (uint32_t)(&nvm_tx)), "T center" , "s16" , ROACH_ADC_MID, 0, ROACH_ADC_MAX , 1, },
    { ((uint32_t)(&(nvm_tx.pot_throttle.deadzone )) - (uint32_t)(&nvm_tx)), "T deadzone" , "s16" , ROACH_ADC_NOISE, 0, ROACH_ADC_MAX / 32 , 1, },
    { ((uint32_t)(&(nvm_tx.pot_throttle.boundary )) - (uint32_t)(&nvm_tx)), "T boundary" , "s16" , ROACH_ADC_NOISE, 0, ROACH_ADC_MAX / 32 , 1, },
    { ((uint32_t)(&(nvm_tx.pot_throttle.scale )) - (uint32_t)(&nvm_tx)), "T scale" , "s16x10" , ROACH_SCALE_MULTIPLIER, 0, INT_MAX , 1, },
    { ((uint32_t)(&(nvm_tx.pot_throttle.limit_min )) - (uint32_t)(&nvm_tx)), "T lim min" , "s16" , 0, 0, ROACH_ADC_MAX , 1, },
    { ((uint32_t)(&(nvm_tx.pot_throttle.limit_max )) - (uint32_t)(&nvm_tx)), "T lim max" , "s16" , ROACH_ADC_MAX, 0, ROACH_ADC_MAX , 1, },
    { ((uint32_t)(&(nvm_tx.pot_throttle.expo )) - (uint32_t)(&nvm_tx)), "T expo" , "s16x10" , 0, -ROACH_SCALE_MULTIPLIER, ROACH_SCALE_MULTIPLIER, 1, },
    { ((uint32_t)(&(nvm_tx.pot_throttle.filter )) - (uint32_t)(&nvm_tx)), "T filter" , "s16x10" , ROACH_FILTER_DEFAULT, 0, ROACH_SCALE_MULTIPLIER, 1, },
    { ((uint32_t)(&(nvm_tx.pot_steering.center )) - (uint32_t)(&nvm_tx)), "S center" , "s16" , ROACH_ADC_MID, 0, ROACH_ADC_MAX , 1, },
    { ((uint32_t)(&(nvm_tx.pot_steering.deadzone )) - (uint32_t)(&nvm_tx)), "S deadzone" , "s16" , ROACH_ADC_NOISE, 0, ROACH_ADC_MAX / 32 , 1, },
    { ((uint32_t)(&(nvm_tx.pot_steering.boundary )) - (uint32_t)(&nvm_tx)), "S boundary" , "s16" , ROACH_ADC_NOISE, 0, ROACH_ADC_MAX / 32 , 1, },
    { ((uint32_t)(&(nvm_tx.pot_steering.scale )) - (uint32_t)(&nvm_tx)), "S scale" , "s16x10" , ROACH_SCALE_MULTIPLIER, 0, INT_MAX , 1, },
    { ((uint32_t)(&(nvm_tx.pot_steering.limit_min )) - (uint32_t)(&nvm_tx)), "S lim min" , "s16" , 0, 0, ROACH_ADC_MAX , 1, },
    { ((uint32_t)(&(nvm_tx.pot_steering.limit_max )) - (uint32_t)(&nvm_tx)), "S lim max" , "s16" , ROACH_ADC_MAX, 0, ROACH_ADC_MAX , 1, },
    { ((uint32_t)(&(nvm_tx.pot_steering.expo )) - (uint32_t)(&nvm_tx)), "S expo" , "s16x10" , 0, -ROACH_SCALE_MULTIPLIER, ROACH_SCALE_MULTIPLIER, 1, },
    { ((uint32_t)(&(nvm_tx.pot_steering.filter )) - (uint32_t)(&nvm_tx)), "S filter" , "s16x10" , ROACH_FILTER_DEFAULT, 0, ROACH_SCALE_MULTIPLIER, 1, },
    { ((uint32_t)(&(nvm_tx.pot_weapon .center )) - (uint32_t)(&nvm_tx)), "WC center" , "s16" , 0, 0, ROACH_ADC_MAX , 1, },
    { ((uint32_t)(&(nvm_tx.pot_weapon .deadzone )) - (uint32_t)(&nvm_tx)), "WC deadzone" , "s16" , ROACH_ADC_NOISE, 0, ROACH_ADC_MAX / 32 , 1, },
    { ((uint32_t)(&(nvm_tx.pot_weapon .boundary )) - (uint32_t)(&nvm_tx)), "WC boundary" , "s16" , ROACH_ADC_NOISE, 0, ROACH_ADC_MAX / 32 , 1, },
    { ((uint32_t)(&(nvm_tx.pot_weapon .scale )) - (uint32_t)(&nvm_tx)), "WC scale" , "s16x10" , ROACH_SCALE_MULTIPLIER, 0, INT_MAX , 1, },
    { ((uint32_t)(&(nvm_tx.pot_weapon .limit_min )) - (uint32_t)(&nvm_tx)), "WC lim min" , "s16" , 0, 0, ROACH_ADC_MAX , 1, },
    { ((uint32_t)(&(nvm_tx.pot_weapon .limit_max )) - (uint32_t)(&nvm_tx)), "WC lim max" , "s16" , ROACH_ADC_MAX, 0, ROACH_ADC_MAX , 1, },
    { ((uint32_t)(&(nvm_tx.pot_weapon .expo )) - (uint32_t)(&nvm_tx)), "WC expo" , "s16x10" , 0, -ROACH_SCALE_MULTIPLIER, ROACH_SCALE_MULTIPLIER, 1, },
    { ((uint32_t)(&(nvm_tx.pot_weapon .filter )) - (uint32_t)(&nvm_tx)), "WC filter" , "s16x10" , ROACH_FILTER_DEFAULT, 0, ROACH_SCALE_MULTIPLIER, 1, },
    { ((uint32_t)(&(nvm_tx.pot_aux .center )) - (uint32_t)(&nvm_tx)), "POT center" , "s16" , 0, 0, ROACH_ADC_MAX , 1, },
    { ((uint32_t)(&(nvm_tx.pot_aux .deadzone )) - (uint32_t)(&nvm_tx)), "POT deadzone", "s16" , ROACH_ADC_NOISE, 0, ROACH_ADC_MAX / 32 , 1, },
    { ((uint32_t)(&(nvm_tx.pot_aux .boundary )) - (uint32_t)(&nvm_tx)), "POT boundary", "s16" , ROACH_ADC_NOISE, 0, ROACH_ADC_MAX / 32 , 1, },
    { ((uint32_t)(&(nvm_tx.pot_aux .scale )) - (uint32_t)(&nvm_tx)), "POT scale" , "s16x10" , ROACH_SCALE_MULTIPLIER, 0, INT_MAX , 1, },
    { ((uint32_t)(&(nvm_tx.pot_aux .limit_min )) - (uint32_t)(&nvm_tx)), "POT lim min" , "s16" , 0, 0, ROACH_ADC_MAX , 1, },
    { ((uint32_t)(&(nvm_tx.pot_aux .limit_max )) - (uint32_t)(&nvm_tx)), "POT lim max" , "s16" , ROACH_ADC_MAX, 0, ROACH_ADC_MAX , 1, },
    { ((uint32_t)(&(nvm_tx.pot_aux .expo )) - (uint32_t)(&nvm_tx)), "POT expo" , "s16x10" , 0, -ROACH_SCALE_MULTIPLIER, ROACH_SCALE_MULTIPLIER, 1, },
    { ((uint32_t)(&(nvm_tx.pot_aux .filter )) - (uint32_t)(&nvm_tx)), "POT filter" , "s16x10" , ROACH_FILTER_DEFAULT, 0, ROACH_SCALE_MULTIPLIER, 1, },
    { ((uint32_t)(&(nvm_tx.pot_battery .filter )) - (uint32_t)(&nvm_tx)), "BAT filter" , "s16x10" , ROACH_FILTER_DEFAULT, 0, ROACH_SCALE_MULTIPLIER, 1, },
    { ((uint32_t)(&(nvm_tx.startup_switches )) - (uint32_t)(&nvm_tx)), "ST SW" , "hex" , 0x01, 0, 0x07, 1, },
    { ((uint32_t)(&(nvm_tx.startup_switches_mask )) - (uint32_t)(&nvm_tx)), "ST SW mask" , "hex" , 0x01, 0, 0x07, 1, },
    ROACH_NVM_GUI_DESC_END,
};

uint32_t nvm_dirty = 0;

void settings_init(void)
{
    settings_factoryReset();
    settings_loadFile(ROACH_STARTUP_CONF_NAME);
}

void settings_factoryReset(void)
{
    memset(&nvm_tx, 0, sizeof(roach_tx_nvm_t));
    roachnvm_setdefaults((uint8_t*)&nvm_rf, cfgdesc_rf);
    roachnvm_setdefaults((uint8_t*)&nvm_tx, cfgdesc_ctrler);
}

bool settings_save(void)
{
    return settings_saveToFile(ROACH_STARTUP_CONF_NAME);
}

void settings_saveIfNeeded(uint32_t span)
{
    if (nvm_dirty > 0)
    {
        uint32_t now = millis();
        if ((now - nvm_dirty) >= span && RoachUsbMsd_canSave())
        {
            Serial.printf("[%u]:autosaving startup file\r\n", millis());
            settings_save();
            nvm_dirty = 0;
        }
    }
}

bool settings_loadFile(const char* fname)
{
    RoachFile f;
    bool suc = f.open(fname);
    uint8_t flags = 0;
    if (memcmp(fname, "rf", 2) == 0) {
        flags |= (1 << 0);
    }
    else if (memcmp(fname, "ctrler", 6) == 0) {
        flags |= (1 << 1);
    }
    else if (memcmp(fname, "robot", 5) == 0) {
        flags |= (1 << 2);
    }
    else if (memcmp(fname, "rdesc", 5) == 0) {
        flags |= (1 << 3);
    }
    if (suc)
    {
        if ((flags & (1 << 0)) != 0 || flags == 0) {
            roachnvm_readfromfile(&f, (uint8_t*)&nvm_rf, cfgdesc_rf);
            radio_init();
        }
        if ((flags & (1 << 1)) != 0 || flags == 0) {
            roachnvm_readfromfile(&f, (uint8_t*)&nvm_tx, cfgdesc_ctrler);
        }
        if ((flags & (1 << 2)) != 0 || flags == 0) {
            roachnvm_readfromfile(&f, (uint8_t*)rosync_nvm, rosync_desc_tbl);
        }
        if ((flags & (1 << 3)) != 0) {
            rosync_loadDescFileObj(&f, 0);
        }
        f.close();
        #ifdef ROACHTX_AUTOSAVE
        if (strncmp(fname, ROACH_STARTUP_CONF_NAME, 32) != 0) {
            settings_markDirty();
        }
        #endif
        return true;
    }
    return false;
}

bool settings_saveToFile(const char* fname)
{
    RoachFile f;
    bool suc = f.open(fname, O_RDWR | O_CREAT);
    uint8_t flags = 0;
    if (memcmp(fname, "rf", 2) == 0) {
        flags |= (1 << 0);
    }
    else if (memcmp(fname, "ctrler", 6) == 0) {
        flags |= (1 << 1);
    }
    else if (memcmp(fname, "robot", 5) == 0) {
        flags |= (1 << 2);
    }
    if (suc)
    {
        if ((flags & (1 << 0)) != 0 || flags == 0) {
            roachnvm_writetofile(&f, (uint8_t*)&nvm_rf, cfgdesc_rf);
        }
        if ((flags & (1 << 1)) != 0 || flags == 0) {
            roachnvm_writetofile(&f, (uint8_t*)&nvm_tx, cfgdesc_ctrler);
        }
        if ((flags & (1 << 2)) != 0 || flags == 0) {
            roachnvm_writetofile(&f, (uint8_t*)rosync_nvm, rosync_desc_tbl);
        }
        f.close();
        return true;
    }
    return false;
}

void settings_markDirty(void)
{
    nvm_dirty = millis();
}

bool roachnvm_fileCopy(const char* fin_name, const char* fout_name)
{
    RoachFile fin;
    RoachFile fout;
    bool suc;
    suc = fin.open(fin_name);
    if (suc == false) {
        Serial.printf("ERR[%u]: roachnvm_fileCopy cannot open fin \"%s\"\r\n", millis(), fin_name);
        return false;
    }
    suc = fout.open(fout_name, O_RDWR | O_CREAT);
    if (suc == false) {
        Serial.printf("ERR[%u]: roachnvm_fileCopy cannot open fout \"%s\"\r\n", millis(), fout_name);
        fin.close();
        return false;
    }
    #define ROACHNVM_FILECOPY_CHUNK 256
    uint8_t tmpbuf[ROACHNVM_FILECOPY_CHUNK];
    while (true)
    {
        int r = fin.read(tmpbuf, ROACHNVM_FILECOPY_CHUNK);
        if (r > 0)
        {
            fout.write(tmpbuf, r);
        }
        else
        {
            fin.close();
            fout.close();
            return true;
        }
    }
}

void settings_initValidate(void)
{
    if (nvm_tx.heading_multiplier == 0) {
        nvm_tx.heading_multiplier = 9;
    }
}
# 1 "D:/GithubRepos/robots/Roach/RoachRcTx/QuickActions.ino"
#include "MenuClass.h"

class QuickAction
{
    public:
        QuickAction(const char* txt, RoachButton* btn)
        {
            strncpy(_txt, txt, 30);
            _btn = btn;
        };

        virtual void run(void)
        {
            draw_start();
            if (wait())
            {
                action();
            }
        };

    protected:
        char _txt[32];
        RoachButton* _btn;
        uint32_t t = 0;
        uint32_t last_time = 0;

        virtual void draw_start(void)
        {
            gui_drawWait();
            oled.clearDisplay();
            oled.setCursor(0, 0);
            oled.print("QIKACT");
            oled.setCursor(0, ROACHGUI_LINE_HEIGHT);
            oled.print(_txt);
            draw_barOutline();
            gui_drawNow();
        };

        virtual bool wait(void)
        {
            while ((t = _btn->isHeld()) != 0)
            {
                yield();
                ctrler_tasks();
                if (t < last_time) {
                    break;
                }
                last_time = t;
                if (t >= QUICKACTION_HOLD_TIME)
                {
                    btns_clearAll();
                    btns_disableAll();
                    return true;
                }

                if (gui_canDisplay())
                {
                    draw_barFill();
                    gui_drawNow();
                }
            }
            btns_clearAll();
            return false;
        };

        virtual void action(void);
        virtual void draw_barOutline(void);
        virtual void draw_barFill(void);
};

class QuickActionDownload : public QuickAction
{
    public:
        QuickActionDownload(void) : QuickAction("sync download", &btn_down)
        {
        };

    protected:
        int x, h;
        virtual void action(void)
        {
            RoachMenuFuncSyncDownload* m = new RoachMenuFuncSyncDownload();
            m->run();
            delete m;
        };

        virtual void draw_barOutline(void)
        {
            x = SCREEN_WIDTH - 9;
            h = SCREEN_HEIGHT - 1;
            oled.drawRect(x, 0, 8, h, 1);
        };

        virtual void draw_barFill(void)
        {
            int h2 = map(t, 0, QUICKACTION_HOLD_TIME, 0, h);
            oled.fillRect(x, 0, 8, h2, 1);
        };
};

class QuickActionUpload : public QuickAction
{
    public:
        QuickActionUpload(void) : QuickAction("sync upload", &btn_up)
        {
        };

    protected:
        int x, h;
        virtual void action(void)
        {
            RoachMenuFuncSyncUpload* m = new RoachMenuFuncSyncUpload();
            m->run();
            delete m;
        };

        virtual void draw_barOutline(void)
        {
            x = SCREEN_WIDTH - 9;
            h = SCREEN_HEIGHT - 1;
            oled.drawRect(x, 0, 8, h, 1);
        };

        virtual void draw_barFill(void)
        {
            int h2 = map(t, 0, QUICKACTION_HOLD_TIME, 0, h);
            oled.fillRect(x, SCREEN_HEIGHT - h2, 8, h2, 1);
        };
};

class QuickActionCalibGyro : public QuickAction
{
    public:
        QuickActionCalibGyro(void) : QuickAction("cal. gyro", &btn_left)
        {
        };

    protected:
        int y, w;
        virtual void action(void)
        {
            RoachMenuFuncCalibGyro* m = new RoachMenuFuncCalibGyro();
            m->run();
            delete m;
        };

        virtual void draw_barOutline(void)
        {
            y = (SCREEN_HEIGHT / 2) - 4;
            w = SCREEN_WIDTH;
            oled.drawRect(0, y, w, 8, 1);
        };

        virtual void draw_barFill(void)
        {
            int x = map(t, 0, QUICKACTION_HOLD_TIME, 0, w);
            oled.fillRect(w - x, y, w, 8, 1);
        };
};

class QuickActionCalibCenters : public QuickAction
{
    public:
        QuickActionCalibCenters(void) : QuickAction("cal. centers", &btn_center)
        {
        };

    protected:
        int x, y, r;
        virtual void action(void)
        {
            RoachMenuFuncCalibAdcCenter* m = new RoachMenuFuncCalibAdcCenter();
            m->run();
            delete m;
        };

        virtual void draw_barOutline(void)
        {
            x = SCREEN_WIDTH - 32;
            y = SCREEN_HEIGHT / 2;
            r = 30;
            oled.drawCircle(x, y, r, 1);
        };

        virtual void draw_barFill(void)
        {
            int r2 = map(t, 0, QUICKACTION_HOLD_TIME, 0, r);
            oled.fillCircle(x, y, r2, 1);
        };
};

class QuickActionCalibLimits : public QuickAction
{
    public:
        QuickActionCalibLimits(void) : QuickAction("cal. limits", &btn_right)
        {
        };

    protected:
        int y, w;
        virtual void action(void)
        {
            RoachMenuFuncCalibAdcLimits* m = new RoachMenuFuncCalibAdcLimits();
            m->run();
            delete m;
        };

        virtual void draw_barOutline(void)
        {
            y = (SCREEN_HEIGHT / 2) - 4;
            w = SCREEN_WIDTH;
            oled.drawRect(0, y, w, 8, 1);
        };

        virtual void draw_barFill(void)
        {
            int x = map(t, 0, QUICKACTION_HOLD_TIME, 0, w);
            oled.fillRect(0, y, x, 8, 1);
        };
};

class QuickActionSaveStartup : public QuickAction
{
    public:
        QuickActionSaveStartup(void) : QuickAction("save startup", &btn_right)
        {
        };

    protected:
        int y, w;
        virtual void action(void)
        {
            oled.setCursor(0, ROACHGUI_LINE_HEIGHT);
            if (settings_save()) {
                oled.print("done");
            }
            else {
                oled.print("ERR saving");
            }
            gui_drawNow();
            while (btns_isAnyHeld()) {
                ctrler_tasks();
            }
        };

        virtual void draw_barOutline(void)
        {
            y = (SCREEN_HEIGHT / 2) - 4;
            w = SCREEN_WIDTH;
            oled.drawRect(0, y, w, 8, 1);
        };

        virtual void draw_barFill(void)
        {
            int x = map(t, 0, QUICKACTION_HOLD_TIME, 0, w);
            oled.fillRect(0, y, x, 8, 1);
        };
};

class QuickActionUsbMsd : public QuickAction
{
    public:
        QuickActionUsbMsd(void) : QuickAction("USB MSD", &btn_right)
        {
        };

    protected:
        int y, w;
        virtual void action(void)
        {
            RoachUsbMsd_presentUsbMsd();
            oled.setCursor(0, ROACHGUI_LINE_HEIGHT);
            oled.print("done");
            gui_drawNow();
            while (btns_isAnyHeld()) {
                ctrler_tasks();
            }
        };

        virtual void draw_barOutline(void)
        {
            y = (SCREEN_HEIGHT / 2) - 4;
            w = SCREEN_WIDTH;
            oled.drawRect(0, y, w, 8, 1);
        };

        virtual void draw_barFill(void)
        {
            int x = map(t, 0, QUICKACTION_HOLD_TIME, 0, w);
            oled.fillRect(0, y, x, 8, 1);
        };
};

void QuickAction_check(uint8_t btn)
{
    switch (btn)
    {
        case BTNID_UP:
        {
            QuickActionUpload* q = new QuickActionUpload();
            q->run();
            delete q;
            break;
        }
        case BTNID_DOWN:
        {
            QuickActionDownload* q = new QuickActionDownload();
            q->run();
            delete q;
            break;
        }
        case BTNID_LEFT:
        {
            QuickActionCalibGyro* q = new QuickActionCalibGyro();
            q->run();
            delete q;
            break;
        }
        case BTNID_RIGHT:
        {
            #ifndef ROACHTX_AUTOSAVE
            if (RoachUsbMsd_canSave())
            {
                QuickActionSaveStartup* q = new QuickActionSaveStartup();
                q->run();
                delete q;
            }
            else
            #endif
            if (RoachUsbMsd_hasVbus() && RoachUsbMsd_isUsbPresented() == false)
            {
                QuickActionUsbMsd* q = new QuickActionUsbMsd();
                q->run();
                delete q;
            }
            else
            {
                QuickActionCalibLimits* q = new QuickActionCalibLimits();
                q->run();
                delete q;
            }
            break;
        }
        case BTNID_CENTER:
        {
            QuickActionCalibCenters* q = new QuickActionCalibCenters();
            q->run();
            delete q;
            break;
        }
    }
}
# 1 "D:/GithubRepos/robots/Roach/RoachRcTx/RobotSync.ino"
enum
{
    ROSYNC_SM_DISCONNECTED,
    ROSYNC_SM_ALLGOOD,
    ROSYNC_SM_NOSYNC,
    ROSYNC_SM_NOSYNC_ERR,
    ROSYNC_SM_NODESC,
    ROSYNC_SM_NODESC_ERR,
    ROSYNC_SM_DOWNLOADDESC,
    ROSYNC_SM_DOWNLOADNVM,
    ROSYNC_SM_UPLOAD,
    ROSYNC_SM_UPLOAD_DONE,
};

uint8_t rosync_statemachine = ROSYNC_SM_DISCONNECTED;

uint32_t rosync_checksum_nvm = 0;
uint32_t rosync_checksum_desc = 0;

RoachMenuCfgLister* rosync_menu = NULL;
roach_nvm_gui_desc_t* rosync_desc_tbl = NULL;
uint8_t* rosync_nvm = NULL;
uint32_t rosync_nvm_wptr = 0;
uint32_t rosync_nvm_sz = 0;
RoachFile rosync_descDlFile;
uint32_t rosync_descDlFileSize = 0;
uint32_t rosync_lastRxTime = 0;
uint32_t rosync_uploadIdx = 0, rosync_uploadTotal = 0;

void rosync_task(void)
{
    if (rosync_statemachine != ROSYNC_SM_DISCONNECTED && radio.isConnected() == false)
    {
        if (rosync_descDlFile.isOpen()) {
            rosync_descDlFile.close();
        }

        rosync_statemachine = ROSYNC_SM_DISCONNECTED;
    }

    switch (rosync_statemachine)
    {
        case ROSYNC_SM_DISCONNECTED:
            if (radio.isConnected())
            {


                if (telem_pkt.chksum_desc != rosync_checksum_desc)
                {




                    if (rosync_menu != NULL) {
                        if (rosync_menu->isRunning()) {
                            rosync_menu->interrupt(EXITCODE_BACK);
                            break;
                        }
                    }


                    if (rosync_loadDescFileId(telem_pkt.chksum_desc))
                    {
                        rosync_loadNvmFile(ROACH_STARTUP_CONF_NAME);
                        uint32_t c = roachnvm_getConfCrc(rosync_nvm, rosync_desc_tbl);
                        if (c == telem_pkt.chksum_nvm)
                        {

                            rosync_checksum_nvm = c;
                            rosync_statemachine = ROSYNC_SM_ALLGOOD;
                        }
                        else
                        {
                            rosync_statemachine = ROSYNC_SM_NOSYNC;
                        }
                    }
                    else
                    {
                        rosync_statemachine = ROSYNC_SM_NODESC;
                    }
                }
                else
                {


                    if (telem_pkt.chksum_nvm != rosync_checksum_nvm)
                    {
                        if (rosync_nvm == NULL)
                        {


                            if (rosync_menu != NULL) {
                                if (rosync_menu->isRunning()) {
                                    rosync_menu->interrupt(EXITCODE_BACK);
                                    break;
                                }
                            }

                            if (rosync_loadDescFileId(telem_pkt.chksum_desc) == false)
                            {
                                rosync_statemachine = ROSYNC_SM_NODESC;
                                break;
                            }

                            rosync_loadNvmFile(ROACH_STARTUP_CONF_NAME);
                        }

                        uint32_t c = roachnvm_getConfCrc(rosync_nvm, rosync_desc_tbl);
                        if (c == telem_pkt.chksum_nvm)
                        {
                            rosync_checksum_nvm = c;
                            rosync_statemachine = ROSYNC_SM_ALLGOOD;
                        }
                        else
                        {
                            rosync_statemachine = ROSYNC_SM_NOSYNC;
                        }
                    }
                    else
                    {
                        rosync_statemachine = ROSYNC_SM_ALLGOOD;
                    }
                }
            }
            break;
        case ROSYNC_SM_DOWNLOADDESC:
            if (radio.textAvail())
            {
                radio_binpkt_t pkt;
                radio.textReadBin(&pkt, false);
                if (rosync_downloadDescChunk(&pkt))
                {
                    radio.textReadPtr(true);
                }
            }
            if ((millis() - rosync_lastRxTime) >= 1000)
            {
                if (rosync_descDlFile.isOpen()) {
                    rosync_descDlFile.close();
                }
                rosync_statemachine = ROSYNC_SM_NODESC_ERR;
            }
            break;
        case ROSYNC_SM_DOWNLOADNVM:
            if (radio.textAvail())
            {
                if (rosync_downloadNvmChunk(radio.textReadPtr(false)))
                {
                    radio.textReadPtr(true);
                }
            }
            if ((millis() - rosync_lastRxTime) >= 1000)
            {
                rosync_statemachine = ROSYNC_SM_NOSYNC_ERR;
            }
            break;
        case ROSYNC_SM_UPLOAD:
            if (radio.textIsDone())
            {
                rosync_uploadNextChunk();
            }
            break;
    }
}

bool rosync_downloadDescFile(void)
{
    char fname[32];
    sprintf(fname, "rdesc_0x%08X.bin", telem_pkt.chksum_desc);
    bool x = rosync_descDlFile.open(fname, O_RDWR | O_CREAT);
    if (x == false) {
        rosync_statemachine = ROSYNC_SM_NODESC_ERR;
        Serial.printf("ERR[%u]: unable to open or create \"%s\" to be written\r\n", millis(), fname);
        return false;
    }
    radio.textSendByte(ROACHCMD_SYNC_DOWNLOAD_DESC);
    rosync_statemachine = ROSYNC_SM_DOWNLOADDESC;
    return true;
}

void rosync_downloadNvm(void)
{
    radio.textSendByte(ROACHCMD_SYNC_DOWNLOAD_CONF);
    rosync_nvm_wptr = 0;
    rosync_statemachine = ROSYNC_SM_DOWNLOADNVM;
}

bool rosync_downloadStart(void)
{
    if (rosync_statemachine == ROSYNC_SM_NODESC || rosync_statemachine == ROSYNC_SM_NODESC_ERR)
    {
        if (rosync_descDlFile.isOpen()) {
            rosync_descDlFile.close();
        }
        return rosync_downloadDescFile();
    }
    else if (rosync_nvm_sz > 0 && rosync_nvm != NULL && radio.isConnected())
    {
        rosync_downloadNvm();
        return true;
    }
    return false;
}

bool rosync_downloadDescChunk(radio_binpkt_t* pkt)
{
    if (pkt->typecode == ROACHCMD_SYNC_DOWNLOAD_DESC)
    {
        int i, dlen = pkt->len;
        if (dlen == 0)
        {
            rosync_descDlFile.close();
            if (rosync_loadDescFileId(telem_pkt.chksum_desc))
            {
                if (telem_pkt.chksum_nvm != rosync_checksum_nvm)
                {
                    rosync_loadNvmFile(ROACH_STARTUP_CONF_NAME);
                    if (rosync_checksum_nvm == telem_pkt.chksum_nvm)
                    {
                        rosync_statemachine = ROSYNC_SM_ALLGOOD;
                    }
                    else
                    {
                        rosync_downloadNvm();
                    }
                }
                else
                {
                    rosync_statemachine = ROSYNC_SM_ALLGOOD;
                }
            }
            else
            {
                rosync_statemachine = ROSYNC_SM_NODESC_ERR;
            }
            return true;
        }

        dlen = dlen > NRFRR_PAYLOAD_SIZE2 ? NRFRR_PAYLOAD_SIZE2 : dlen;

        for (i = 0; i < dlen; i++)
        {
            rosync_descDlFile.write(pkt->data[i]);
            rosync_descDlFileSize += 1;
        }
        rosync_lastRxTime = millis();
        return true;
    }
    return false;
}

bool rosync_downloadNvmChunk(radio_binpkt_t* pkt)
{
    if (rosync_nvm == NULL)
    {
        if (pkt->typecode == ROACHCMD_SYNC_DOWNLOAD_CONF && pkt->addr == 0 && pkt->len > 0)
        {
            rosync_nvm = (uint8_t*)malloc(pkt->len);
            rosync_nvm_sz = pkt->len;
        }
        else
        {
            return false;
        }
    }

    bool ret = false;
    bool done = false;

    if (pkt->typecode == ROACHCMD_SYNC_DOWNLOAD_CONF)
    {
        ret = true;
        if (pkt->len == 0) {
            done = true;
        }

        int i, dlen = pkt->len;
        dlen = dlen > NRFRR_PAYLOAD_SIZE2 ? NRFRR_PAYLOAD_SIZE2 : dlen;
        for (i = 0, rosync_nvm_wptr = pkt->addr; done == false && i < dlen && rosync_nvm_wptr < rosync_nvm_sz; i++)
        {
            rosync_nvm[rosync_nvm_wptr] = pkt->data[i];
            rosync_nvm_wptr += 1;
        }
        if (rosync_nvm_wptr >= rosync_nvm_sz) {
            done = true;
        }
        rosync_lastRxTime = millis();
    }

    if (done)
    {
        rosync_checksum_nvm = roachnvm_getConfCrc(rosync_nvm, rosync_desc_tbl);
        if (rosync_checksum_nvm == telem_pkt.chksum_nvm)
        {
            rosync_statemachine = ROSYNC_SM_ALLGOOD;
        }
        else
        {
            rosync_statemachine = ROSYNC_SM_NOSYNC_ERR;
        }
    }
    return ret;
}

bool rosync_loadDescFileId(uint32_t id)
{
    char fname[32];
    sprintf(fname, "rdesc_0x%08X.bin", id);
    return rosync_loadDescFile((const char*)fname, id);
}

bool rosync_loadDescFile(const char* fname, uint32_t id)
{
    RoachFile f;
    bool x = f.open(fname, O_RDONLY);
    if (x == false) {
        Serial.printf("ERR[%u]: tried loading robot desc file \"%s\" but file does not exist\r\n", millis(), fname);
        return false;
    }
    return rosync_loadDescFileObj(&f, id);
}

bool rosync_loadDescFileObj(RoachFile* f, uint32_t id)
{
    if (rosync_menu != NULL)
    {
        if (rosync_menu->isRunning()) {

            rosync_menu->interrupt(EXITCODE_BACK);
            return false;
        }

        delete rosync_menu;
        rosync_menu = NULL;
    }

    if (rosync_desc_tbl != NULL)
    {
        free(rosync_desc_tbl);
        rosync_desc_tbl = NULL;
    }
    if (rosync_nvm != NULL)
    {
        free(rosync_nvm);
        rosync_nvm = NULL;
    }
    uint32_t sz = f->fileSize();
    rosync_desc_tbl = (roach_nvm_gui_desc_t*)malloc(sz);
    int chunk_sz = 256;
    int i, j;
    for (i = 0, j = 1; i < sz && j > 0; i += j)
    {
        j = f->read((void*)rosync_desc_tbl, chunk_sz);
    }

    f->close();

    uint32_t chksum_desc = roach_crcCalc((uint8_t*)rosync_desc_tbl, sz, NULL);
    if (chksum_desc != id && id != 0) {
        char tmpname[32];
        f->getName7(tmpname, 30);
        Serial.printf("ERR[%u]: tried loading robot desc file \"%s\" but contents do not match checksum 0x%08X\r\n", millis(), tmpname, chksum_desc);
        free(rosync_desc_tbl);
        rosync_desc_tbl = NULL;
        return false;
    }

    int sum = 0;
    for (i = 0; ; i++)
    {
        roach_nvm_gui_desc_t* d = &rosync_desc_tbl[i];
        sum = d->byte_offset > sum ? d->byte_offset : sum;
    }
    sum += 4;
    rosync_nvm_sz = sum;
    rosync_nvm = (uint8_t*)malloc(sum);

    if (rosync_nvm == NULL) {
        Serial.printf("ERR[%u]: unable to allocate memory for rosync_nvm\r\n", millis());
        free(rosync_desc_tbl);
        rosync_desc_tbl = NULL;
        return false;
    }

    rosync_menu = new RoachMenuCfgLister(MENUID_CONFIG_ROBOT, "ROBOT CFG", "robot", (void*)rosync_nvm, rosync_desc_tbl);
    if (rosync_menu == NULL) {
        Serial.printf("ERR[%u]: unable to allocate memory for rosync_menu\r\n", millis());
        free(rosync_desc_tbl);
        rosync_desc_tbl = NULL;
        free(rosync_nvm);
        rosync_nvm = NULL;
        return false;
    }

    rosync_checksum_desc = chksum_desc;
    roachnvm_setdefaults(rosync_nvm, rosync_desc_tbl);
    rosync_checksum_nvm = roachnvm_getConfCrc(rosync_nvm, rosync_desc_tbl);
    rosync_markOnlyFile();
    return true;
}

bool rosync_loadNvmFile(const char* fname)
{
    RoachFile f;
    char fname_buf[32];
    if (fname == NULL)
    {
        sprintf(fname_buf, ROACH_STARTUP_CONF_NAME);
    }
    else
    {
        sprintf(fname_buf, fname);
    }
    bool x = f.open(fname_buf, O_RDONLY);
    if (x == false) {
        Serial.printf("ERR[%u]: unable to find robot NVM file \"%s\"\r\n", millis(), fname_buf);
        return false;
    }

    roachnvm_readfromfile(&f, (uint8_t*)rosync_nvm, rosync_desc_tbl);

    f.close();

    rosync_checksum_nvm = roachnvm_getConfCrc(rosync_nvm, rosync_desc_tbl);
    return true;
}

void rosync_uploadStart(void)
{
    rosync_statemachine = ROSYNC_SM_UPLOAD;
    rosync_uploadIdx = 0;
    rosync_uploadTotal = roachnvm_getDescCnt(rosync_desc_tbl);
}

void rosync_uploadChunk(roach_nvm_gui_desc_t* desc)
{
    radio_binpkt_t pkt;
    pkt.typecode = ROACHCMD_SYNC_UPLOAD_CONF;
    char tmp[32];
    roachnvm_formatitem(tmp, (uint8_t*)rosync_nvm, desc);
    int i = sprintf((char*)(pkt.data), "%s=%s\n", desc->name, tmp);
    pkt.addr = 0;
    pkt.len = i;
    radio.textSendBin(&pkt);
}

void rosync_uploadNextChunk(void)
{
    roach_nvm_gui_desc_t* desc = &rosync_desc_tbl[rosync_uploadIdx];
    if (desc->name[0] == 0) {
        rosync_statemachine = ROSYNC_SM_UPLOAD_DONE;
        return;
    }
    rosync_uploadChunk(desc);
    rosync_uploadIdx += 1;
}

bool rosync_markOnlyFile(void)
{
    if (!fatroot.open("/"))
    {
        Serial.println("open root failed");
        return false;
    }
    bool has_startup = false;
    int cnt = 0;
    char sfname[64];
    char sfname_only[64];
    while (fatfile.openNext(&fatroot, O_RDONLY))
    {
        if (fatfile.isDir() == false)
        {
            fatfile.getName7(sfname, 62);
            if (strncmp("rdesc", sfname, 5) == 0)
            {
                cnt++;
                strncpy(sfname_only, sfname, 62);
            }
            else if (strcmp(ROACH_STARTUP_DESC_NAME, sfname) == 0)
            {
                has_startup = true;
            }
        }
    }
    if (has_startup == false || cnt == 1)
    {
        return rosync_markStartupFile(sfname_only);
    }
    return false;
}

bool rosync_markStartupFile(const char* fname)
{
    return roachnvm_fileCopy(fname, ROACH_STARTUP_DESC_NAME);
}

bool rosync_loadStartup(void)
{
    return rosync_loadDescFile(ROACH_STARTUP_DESC_NAME, 0);
}

bool rosync_isSynced(void)
{
    return rosync_statemachine == ROSYNC_SM_ALLGOOD;
}

void rosync_draw(void)
{
    int y = 0;
    oled.setCursor(0, y);
    oled.print("ROBOT SYNC");
    y += ROACHGUI_LINE_HEIGHT;
    oled.setCursor(0, y);
    switch (rosync_statemachine)
    {
        case ROSYNC_SM_ALLGOOD:
            oled.print("SUCCESS");
            y += ROACHGUI_LINE_HEIGHT;
            oled.setCursor(0, y);
            oled.printf("C 0x%08X", rosync_checksum_desc);
            break;
        case ROSYNC_SM_DISCONNECTED:
            oled.print("DISCONNECTED");
            break;
        case ROSYNC_SM_DOWNLOADDESC:
            oled.print("downloading desc");
            y += ROACHGUI_LINE_HEIGHT;
            oled.setCursor(0, y);
            oled.printf("%u bytes ", rosync_descDlFileSize);
            printEllipses();
            break;
        case ROSYNC_SM_DOWNLOADNVM:
            oled.print("downloading conf");
            y += ROACHGUI_LINE_HEIGHT;
            oled.setCursor(0, y);
            oled.printf("%u / %u b ", rosync_nvm_wptr, rosync_nvm_sz);
            printEllipses();
            y += ROACHGUI_LINE_HEIGHT;
            oled.setCursor(0, y);
            oled.printf("%u %%", map(rosync_nvm_wptr, 0, rosync_nvm_sz, 0, 100));
            y += ROACHGUI_LINE_HEIGHT;
            drawProgressBar(0, y, SCREEN_WIDTH - 16, 7, rosync_nvm_wptr, rosync_nvm_sz);
            break;
        case ROSYNC_SM_UPLOAD:
            oled.print("uploading conf");
            y += ROACHGUI_LINE_HEIGHT;
            oled.setCursor(0, y);
            oled.printf("# %u / %u ", rosync_uploadIdx, rosync_uploadTotal);
            printEllipses();
            y += ROACHGUI_LINE_HEIGHT;
            oled.setCursor(0, y);
            oled.printf("%u %%", map(rosync_uploadIdx, 0, rosync_uploadTotal, 0, 100));
            y += ROACHGUI_LINE_HEIGHT;
            drawProgressBar(0, y, SCREEN_WIDTH - 16, 7, rosync_uploadIdx, rosync_uploadTotal);
            break;
        case ROSYNC_SM_UPLOAD_DONE:
            oled.print("uploading conf");
            y += ROACHGUI_LINE_HEIGHT;
            oled.setCursor(0, y);
            oled.printf("100%% done");
            break;
        case ROSYNC_SM_NODESC:
            oled.print("no desc file");
            break;
        case ROSYNC_SM_NODESC_ERR:
            oled.print("desc file error");
            break;
        case ROSYNC_SM_NOSYNC:
            oled.print("not sync'ed");
            break;
        case ROSYNC_SM_NOSYNC_ERR:
            oled.print("error:");
            y += ROACHGUI_LINE_HEIGHT;
            oled.setCursor(0, y);
            oled.print("not sync'ed");
            y += ROACHGUI_LINE_HEIGHT;
            oled.setCursor(0, y);
            oled.printf("after download");
            break;
    }
}

void rosync_drawShort(void)
{
    switch (rosync_statemachine)
    {
        case ROSYNC_SM_ALLGOOD:
        case ROSYNC_SM_UPLOAD_DONE:
            oled.printf("robot %08X", rosync_checksum_desc);
            break;
        case ROSYNC_SM_DOWNLOADDESC:
        case ROSYNC_SM_DOWNLOADNVM:
            oled.printf("sync busy %c", 0x19);
            break;
        case ROSYNC_SM_UPLOAD:
            oled.printf("sync busy %c", 0x18);
            break;
        default:
            oled.printf("not sync'ed");
            break;
    }
}


class RobotMenu : public RoachMenu
{
    public:
        RobotMenu(void) : RoachMenu(0)
        {
        };

    protected:
        virtual void onEnter(void)
        {
            if (rosync_menu == NULL)
            {

                rosync_loadStartup();
            }

            if (rosync_menu != NULL)
            {
                rosync_menu->run();
                int ec = rosync_menu->getExitCode();
                _exit = ec;
            }
            else
            {
                showError("robot not loaded");
                _exit = EXITCODE_RIGHT;
            }
        };
};

void menu_install_robot()
{
    static RobotMenu m;
    menu_install(&m);
}
# 1 "D:/GithubRepos/robots/Roach/RoachRcTx/SafeBoot.ino"





void safeboot_check(void)
{
    pinMode(ROACHHW_PIN_BTN_UP , INPUT_PULLUP);
    pinMode(ROACHHW_PIN_BTN_DOWN , INPUT_PULLUP);
    pinMode(ROACHHW_PIN_BTN_CENTER, INPUT_PULLUP);

    if ((digitalRead(ROACHHW_PIN_BTN_UP) == LOW || digitalRead(ROACHHW_PIN_BTN_DOWN) == LOW) || digitalRead(ROACHHW_PIN_BTN_CENTER) == LOW)
    {
        pinMode(ROACHHW_PIN_LED_RED, OUTPUT);
        pinMode(ROACHHW_PIN_LED_BLU, OUTPUT);

        uint32_t now = millis();
        uint32_t tstart = now;
        while (digitalRead(ROACHHW_PIN_BTN_UP) == LOW || digitalRead(ROACHHW_PIN_BTN_DOWN) == LOW || digitalRead(ROACHHW_PIN_BTN_CENTER) == LOW)
        {
            now = millis();

            bool blynk = ((now % 200) <= 100);
            digitalWrite(ROACHHW_PIN_LED_RED, blynk ? HIGH : LOW );
            digitalWrite(ROACHHW_PIN_LED_BLU, blynk ? LOW : HIGH);

            if ((now - tstart) >= 500)
            {
                if (RoachUsbMsd_hasVbus() == false)
                {
                    return;
                }
            }
            if ((now - tstart) >= 3000)
            {
                if (digitalRead(ROACHHW_PIN_BTN_UP) == LOW)
                {
                    enterUf2Dfu();
                }
                else if (digitalRead(ROACHHW_PIN_BTN_DOWN) == LOW)
                {
                    enterSerialDfu();
                }
                else if (digitalRead(ROACHHW_PIN_BTN_CENTER) == LOW)
                {

                    Serial.begin(115200);
                    while (true)
                    {
                        now = millis();
                        blynk = ((now % 200) <= 100);
                        digitalWrite(ROACHHW_PIN_LED_RED, blynk ? HIGH : LOW );
                        digitalWrite(ROACHHW_PIN_LED_BLU, blynk ? LOW : HIGH);
                        if ((now - tstart) >= 1000)
                        {
                            tstart = now;
                            Serial.printf("safe boot %u\r\n", now);
                        }
                        yield();
                    }
                }
            }
        }
    }
}
# 1 "D:/GithubRepos/robots/Roach/RoachRcTx/Utils.ino"
bool btns_hasAnyPressed(void)
{
    bool x = false;

    x |= btn_up .hasPressed(false);
    x |= btn_down .hasPressed(false);
    x |= btn_left .hasPressed(false);
    x |= btn_right .hasPressed(false);
    x |= btn_center .hasPressed(false);
    x |= btn_g5 .hasPressed(false);
    x |= btn_g6 .hasPressed(false);

    return x;
}

bool btns_isAnyHeld(void)
{
    bool x = false;

    x |= btn_up .isHeld() > 0;
    x |= btn_down .isHeld() > 0;
    x |= btn_left .isHeld() > 0;
    x |= btn_right .isHeld() > 0;
    x |= btn_center .isHeld() > 0;
    x |= btn_g5 .isHeld() > 0;
    x |= btn_g6 .isHeld() > 0;

    return x;
}

void btns_clearAll(void)
{
    btn_up .hasPressed(true);
    btn_down .hasPressed(true);
    btn_left .hasPressed(true);
    btn_right .hasPressed(true);
    btn_center .hasPressed(true);
    btn_g5 .hasPressed(true);
    btn_g6 .hasPressed(true);
}

void btns_disableAll(void)
{
    btn_up .disableUntilRelease();
    btn_down .disableUntilRelease();
    btn_left .disableUntilRelease();
    btn_right .disableUntilRelease();
    btn_center .disableUntilRelease();
    btn_g5 .disableUntilRelease();
    btn_g6 .disableUntilRelease();
}

bool switches_alarm = false;
uint8_t switches_getFlags(void)
{
    static bool released = false;
    uint8_t f = 0;
    f |= btn_sw1.isHeld() > 0 ? ROACHPKTFLAG_BTN1 : 0;
    f |= btn_sw2.isHeld() > 0 ? ROACHPKTFLAG_BTN2 : 0;
    f |= btn_sw3.isHeld() > 0 ? ROACHPKTFLAG_BTN3 : 0;
    #ifdef ROACHHW_PIN_BTN_SW4
    f |= btn_sw4.isHeld() > 0 ? ROACHPKTFLAG_BTN4 : 0;
    #endif
    if (released == false) {
        if (f == (nvm_tx.startup_switches & nvm_tx.startup_switches_mask)) {
            switches_alarm = false;
            released = true;
        }
        else {
            f = nvm_tx.startup_switches;
            switches_alarm = true;
        }
    }
    return f;
}

void strncpy0(char* dest, const void* src, size_t n)
{
    unsigned int slen = strlen((char*)src);
    slen = (slen > n && n > 0) ? n : slen;
    strncpy(dest, (const char*)src, slen);
    dest[slen] = 0;
}

void waitFor(uint32_t x)
{
    uint32_t tstart = millis();
    while ((millis() - tstart) < x)
    {
        yield();
        ctrler_tasks();
    }
}

float batt_get(void)
{
    return 0;
}