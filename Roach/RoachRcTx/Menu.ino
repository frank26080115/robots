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
        
        oled.printf("%d%c%d L%0.1f", radio.getRssi(), GUISYMB_RIGHT_ARROW, telem_pkt.rssi, ((float)telem_pkt.loss_rate) / 100.0);
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
            settings_saveIfNeeded(ROACHTX_AUTOSAVE_INTERVAL_SHORT);
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
            oled.printf("%c%-3d %c%d"
                , tx_pkt.throttle >= 0 ? 0x18 : 0x19
                , roach_div_rounded(abs(tx_pkt.throttle), ROACH_SCALE_MULTIPLIER / 100)
                , tx_pkt.steering < 0 ? 0x1B : 0x1A
                , roach_div_rounded(abs(tx_pkt.steering), ROACH_SCALE_MULTIPLIER / 100)
                );

            if (move_locked) {
                oled.printf("  !!");
            }

            y += ROACHGUI_LINE_HEIGHT;
            oled.setCursor(0, y);
            #ifdef ROACHTX_ENABLE_DETCORD_FEATURES
            if ((tx_pkt.flags & ROACHPKTFLAG_BTN3) == 0)
            {
                oled.printf("h");
            }
            else
            #endif
            {
                oled.printf("H");
            }
            oled.printf(":%-4d ", roach_div_rounded(tx_pkt.heading, ROACH_ANGLE_MULTIPLIER));
            if (radio.isConnected())
            {
                if (radio.isConnected() && (uint16_t)(telem_pkt.heading) == (uint16_t)ROACH_HEADING_INVALID_HASFAILED) {
                    oled.printf(" FAIL");
                }
                else if (radio.isConnected() && (uint16_t)(telem_pkt.heading) == (uint16_t)ROACH_HEADING_INVALID_NOTREADY) {
                    oled.printf(" NRDY");
                }
                else
                {
                    oled.printf(" %d", roach_div_rounded(tx_pkt.heading, ROACH_ANGLE_MULTIPLIER), telem_pkt.heading);
                }
            }

            y += ROACHGUI_LINE_HEIGHT;
            oled.setCursor(0, y);
            oled.printf("%d %d %c%c%c"
                #ifdef ROACHHW_PIN_BTN_SW4
                "%c"
                #endif
                , roach_div_rounded(tx_pkt.pot_weap, ROACH_SCALE_MULTIPLIER / 100)
                , roach_div_rounded(tx_pkt.pot_aux , ROACH_SCALE_MULTIPLIER / 100)
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
            #ifdef ROACHTX_ENABLE_DETCORD_FEATURES
            oled.printf("%c%c"
                , ((tx_pkt.flags & ROACHPKTFLAG_BTN1) != 0 ? 'W' : ' ')
                , ((tx_pkt.flags & ROACHPKTFLAG_BTN3) != 0 ? 'G' : ' ')
            );
            #endif

            y += ROACHGUI_LINE_HEIGHT;
            oled.setCursor(0, y);
            oled.printf("BAT: ");
            oled.printf("%.1f  ", batt_get());
            if (radio.isConnected()) {
                oled.printf("%.1f", ((float)telem_pkt.battery)/1000.0f);
            }
            else {
                oled.printf("??");
            }

            if (RoachUsbMsd_isUsbPresented() == false)
            {
                if (RoachUsbMsd_isReady() == false) {
                    y += ROACHGUI_LINE_HEIGHT;
                    oled.setCursor(0, y);
                    oled.printf("WARN: FILESYS FMT");
                }
                else if (RoachUsbMsd_canSave() == false) {
                    y += ROACHGUI_LINE_HEIGHT;
                    oled.setCursor(0, y);
                    oled.printf("WARN: FSYS CAN'T");
                }
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
            _can_autoexit = true;
        };

        #ifdef ROACHTX_AUTOSAVE
        virtual void taskLP(void)
        {
            RoachMenu::taskLP();
            settings_saveIfNeeded(ROACHTX_AUTOSAVE_INTERVAL_SHORT);
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
            //y += ROACHGUI_LINE_HEIGHT;
            //oled.setCursor(0, y);
            //oled.printf("salt %08X", nvm_rf.salt);
            //y += ROACHGUI_LINE_HEIGHT;
            //oled.setCursor(0, y);
            //oled.printf("map  %08X", nvm_rf.chan_map);
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
            drawSideBar("CONF", "LOAD", true);
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
RoachMenu* menuListCur  = NULL;

void menu_run(void)
{
    static int prev_ec = 0;
    if (current_menu == NULL)
    {
        current_menu = &menuHome;
        menuListCur = menuListHead;
        Serial.println("current menu is null, assigning home screen");
    }
    Serial.printf("[%u]: running menu screen ID=%d\r\n", millis(), current_menu->getId());
    current_menu->run();
    int ec = current_menu->getExitCode();
    Serial.printf("[%u]: menu exited to top level, exitcode=%d\r\n", millis(), ec);
    if (ec == EXITCODE_HOME) {
        current_menu = &menuHome;
        menuListCur = menuListHead;
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
    else if ((current_menu == &menuHome || current_menu == &menuInfo) && ec == EXITCODE_RIGHT) {
        current_menu = (RoachMenu*)menuListCur;
        menuListCur = current_menu;
    }
    else if (ec == EXITCODE_LEFT) {
        current_menu = (RoachMenu*)menuListCur->prev_menu;
        menuListCur = current_menu;
        prev_ec = ec;
    }
    else if (ec == EXITCODE_RIGHT) {
        current_menu = (RoachMenu*)menuListCur->next_menu;
        menuListCur = current_menu;
        prev_ec = ec;
    }
    else if (ec == EXITCODE_BACK || ec == EXITCODE_UNABLE) {
        if (prev_ec == EXITCODE_LEFT) {
            current_menu = (RoachMenu*)menuListCur->prev_menu;
        }
        else {
            current_menu = (RoachMenu*)menuListCur->next_menu;
        }
        menuListCur = current_menu;
    }
    else {
        Serial.printf("[%u] err, menu exit (%u) unhandled\r\n", ec);
    }
}

void menu_setup(void)
{
    // individual code modules that represent a particular menu screen will implement a installer function
    // call that installer function here, in order

    menu_install_calibSync();
    menu_install_robot();
    menu_install(new RoachMenuCfgLister(MENUID_CONFIG_CTRLER, "CTRL CFG", "ctrler", &nvm_tx, cfgdesc_ctrler));
    menu_install_fileOpener();
}

void menu_install(RoachMenu* m)
{
    if (menuListHead == NULL) {
        menuListHead = m;
        menuListTail = m;
        menuListCur  = m;
        m->next_menu = m;
        m->prev_menu = m;
    }
    else
    {
        m->prev_menu = menuListTail;
        m->next_menu = menuListHead;
        menuListTail->next_menu = m;
        menuListTail            = m;
        menuListHead->prev_menu = m;
    }
}
