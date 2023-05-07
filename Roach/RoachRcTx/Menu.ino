#include "MenuClass.h"

extern bool rosync_matched;

int printRfStats(int y)
{
    oled.setCursor(0, y);
    if (radio.connected() == false)
    {
        oled.print("DISCONNECTED");
    }
    else
    {
        if (rosync_matched)
        {
            oled.print("CONN'ed, SYNC'ed");
        }
        else
        {
            oled.print("CONN'ed, MISMATCH");
        }
        y += ROACHGUI_LINE_HEIGHT;
        oled.setCursor(0, y);
        oled.printf("sig %d ; %d", radio.get_rssi(), telem_pkt.rssi);
        y += ROACHGUI_LINE_HEIGHT;
        oled.setCursor(0, y);
        oled.printf("pkt loss %0.3f", ((float)telem_pkt.loss_rate) / 100.0);
    }
    return y;
}

class RoachMenuHome : public RoachMenu
{
    public:
        RoachMenuHome() : RoachMenu(MENUID_HOME)
        {
        };

        virtual void draw(void)
        {
            int y = printRfStats(0);
            y += ROACHGUI_LINE_HEIGHT;
            oled.setCursor(0, y);
            oled.printf("T:%4d S:%d", tx_pkt.throttle, tx_pkt.steering);
            y += ROACHGUI_LINE_HEIGHT;
            oled.setCursor(0, y);
            oled.printf("H:%4d W:%d", tx_pkt.heading, tx_pkt.pot_weap);
            if (weap_sw_warning) {
                oled.print("!");
            }
            y += ROACHGUI_LINE_HEIGHT;
            oled.setCursor(0, y);
            oled.printf("A:%d SW:%d%d%d"
                #ifdef ROACHHW_PIN_BTN_SW4
                "%d"
                #endif
                , tx_pkt.pot_aux
                , ((tx_pkt.flags & ROACHPKTFLAG_BTN1) != 0 ? 1 : 0)
                , ((tx_pkt.flags & ROACHPKTFLAG_BTN2) != 0 ? 1 : 0)
                , ((tx_pkt.flags & ROACHPKTFLAG_BTN3) != 0 ? 1 : 0)
                #ifdef ROACHHW_PIN_BTN_SW4
                , ((tx_pkt.flags & ROACHPKTFLAG_BTN4) != 0 ? 1 : 0)
                #endif
                );
            y += ROACHGUI_LINE_HEIGHT;
            oled.setCursor(0, y);
            // TODO: battery
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
            QuickAction_check(btn);
            switch (btn)
            {
                case BTNID_G5:
                    _exit = EXITCODE_RIGHT;
                    break;
                case BTNID_G6:
                    _exit = EXITCODE_LEFT;
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

        virtual void draw(void)
        {
            int y = 0;
            oled.setCursor(0, y);
            oled.printf("UID  %08X");
            //y += ROACHGUI_LINE_HEIGHT;
            //oled.setCursor(0, y);
            //oled.printf("salt %08X");
            //y += ROACHGUI_LINE_HEIGHT;
            //oled.setCursor(0, y);
            //oled.printf("map  %08X");
            y += ROACHGUI_LINE_HEIGHT;
            y = printRfStats(y);
            y += ROACHGUI_LINE_HEIGHT;
            oled.setCursor(0, y);
            oled.printf("dsk free %d kb", _freeSpace);
            y += ROACHGUI_LINE_HEIGHT;
            oled.setCursor(0, y);
            oled.printf("RAM free %d b", minimum_ram);
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
            switch (btn)
            {
                case BTNID_G5:
                    _exit = EXITCODE_RIGHT;
                    break;
                case BTNID_G6:
                    _exit = EXITCODE_LEFT;
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
    if (current_menu == NULL)
    {
        current_menu = &menuHome;
    }
    current_menu->run();
    int ec = current_menu->getExitCode();
    if (ec == EXITCODE_HOME) {
        current_menu = &menuHome;
    }
    else if (ec == EXITCODE_BACK && current_menu->parent_menu != NULL) {
        current_menu = (RoachMenu*)current_menu->parent_menu;
    }
    else if (current_menu == &menuHome && ec == EXITCODE_RIGHT) {
        current_menu = &menuInfo;
    }
    else if ((current_menu == &menuHome || current_menu == &menuInfo) && ec == EXITCODE_LEFT) {
        current_menu = (RoachMenu*)menuListCur;
    }
    else if (ec == EXITCODE_LEFT) {
        current_menu = (RoachMenu*)menuListCur->prev_menu;
    }
    else if (ec == EXITCODE_RIGHT) {
        current_menu = (RoachMenu*)menuListCur->next_menu;
    }
}

void menu_setup(void)
{
    menu_install(new RoachMenuCfgLister(MENUID_CONFIG_DRIVE , "DRIVE"     , "robot" , &nvm_rx, cfggroup_drive));
    menu_install(new RoachMenuCfgLister(MENUID_CONFIG_IMU   , "IMU/PID"   , "robot" , &nvm_rx, cfggroup_imu));
    menu_install(new RoachMenuCfgLister(MENUID_CONFIG_WEAP  , "WEAPON"    , "robot" , &nvm_rx, cfggroup_weap));
    menu_install_calibSync();
    menu_install(new RoachMenuCfgLister(MENUID_CONFIG_CTRLER, "CONTROLLER", "ctrler", &nvm_tx, cfggroup_ctrler));
    menu_install_fileOpener();
}

void menu_install(RoachMenu* m)
{
    if (menuListHead == NULL) {
        menuListHead = m;
        menuListTail = m;
        menuListCur  = m;
    }
    else
    {
        menuListTail->next_menu = m;
        menuListTail            = m;
        menuListHead->prev_menu = m;
    }
}
