
class RoachMenuHome : RoachMenu
{
    public:
        RoachMenuHome() : RoachMenu(MENUID_HOME)
        {
        };

        virtual void draw(void)
        {
            int y = 0;
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
                y += 8;
                oled.setCursor(0, y);
                oled.print("sig %d ; %d");
                y += 8;
                oled.setCursor(0, y);
                oled.print("pkt loss %0.3f");
            }
            y += 8;
            oled.setCursor(0, y);
            oled.print("T:%4d S:%d");
            y += 8;
            oled.setCursor(0, y);
            oled.print("H:%4d W:%d");
            // TODO: if weapon switch is on at start, give warning here
            y += 8;
            oled.setCursor(0, y);
            // TODO: switches
            y += 8;
            oled.setCursor(0, y);
            // TODO: battery
        };

    protected:
        virtual void draw_sidebar(void)
        {
            drawSideBar("CONF", "PAGE", true);
        };

        virtual void draw_titlevoid)
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

class RoachMenuInfo : RoachMenu
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
            //y += 8;
            //oled.setCursor(0, y);
            //oled.printf("seed %08X");
            y += 8;
            oled.setCursor(0, y);
            oled.printf("map  %08X");
            y += 8;
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
                y += 8;
                oled.setCursor(0, y);
                oled.print("sig %d ; %d");
                y += 8;
                oled.setCursor(0, y);
                oled.print("pkt loss %0.3f");
            }
            y += 8;
            oled.setCursor(0, y);
            oled.print("dsk free %d kb", _freeSpace);
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

void menu_task(void)
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
        current_menu = (RoachMenu*)menuListCur->prev_node;
    }
    else if (ec == EXITCODE_RIGHT) {
        current_menu = (RoachMenu*)menuListCur->next_node;
    }
}

void menu_setup(void)
{
    menu_install(new RoachMenuCfgLister(MENUID_CONFIG_DRIVE , "DRIVE"     , "robot" , &nvm_rx, cfggroup_drive));
    menu_install(new RoachMenuCfgLister(MENUID_CONFIG_IMU   , "IMU/PID"   , "robot" , &nvm_rx, cfggroup_imu));
    menu_install(new RoachMenuCfgLister(MENUID_CONFIG_WEAP  , "WEAPON"    , "robot" , &nvm_rx, cfggroup_weap));
    menu_install_calibSync();
    menu_install(new RoachMenuCfgLister(MENUID_CONFIG_CTRLER, "CONTROLLER", "ctrler", &nvm_tx, cfggroup_ctrler));
    menu_install(new RoachMenuFileOpenList());
}

void menu_install(RoachMenu* m)
{
    if (menuListHead == NULL) {
        menuListHead = n;
        menuListTail = n;
        menuListCur  = n;
    }
    else
    {
        menuListTail->next_node = n;
        menuListTail            = n;
        menuListHead->prev_node = n;
    }
}
