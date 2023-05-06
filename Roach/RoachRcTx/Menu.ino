enum
{
    MENUID_NONE,
    MENUID_HOME,                  // display input visualizations
    //MENUID_STATS,                 // display extra live data
    MENUID_INFO,                  // display stuff like profile, UID, version
    //MENUID_RAW,                   // display raw input data
    MENUID_CONFIG_CALIBSYNC,      // options for calibration and synchronization
    MENUID_CONFIG_FILELOAD,       // choose to load a file
    MENUID_CONFIG_DRIVE,          // edit parameters about drive train
    MENUID_CONFIG_WEAP,           // edit parameters about the weapon
    MENUID_CONFIG_CTRLER,         // edit parameters about the controller
    MENUID_CONFIG_IMU,            // edit parameters for the IMU (orientation, PID, timeout, etc)
};

roach_nvm_gui_desc_t cfggroup_rf[] = {
    { ((uint32_t)(&(nvm_rf.uid     )) - (uint32_t)(&nvm_rf)), "UID"     , "hex", 0x1234ABCD, 0, 0xFFFFFFFF, 1, },
    { ((uint32_t)(&(nvm_rf.salt    )) - (uint32_t)(&nvm_rf)), "salt"    , "hex", 0xDEADBEEF, 0, 0xFFFFFFFF, 1, },
    { ((uint32_t)(&(nvm_rf.chan_map)) - (uint32_t)(&nvm_rf)), "chan map", "hex", 0x0FFFFFFF, 0, 0xFFFFFFFF, 1, },
    ROACH_NVM_GUI_DESC_END,
};

roach_rf_nvm_t nvm_rf;
roach_rx_nvm_t nvm_rx;
roach_tx_nvm_t nvm_tx;

roach_nvm_gui_desc_t cfggroup_ctrler[] = {
    { ((uint32_t)(&(nvm_tx.heading_multiplier      )) - (uint32_t)(&nvm_tx)), "H scale"    , "s32x10" , ROACH_SCALE_MULTIPLIER,                 INT_MIN, INT_MAX               , 1, },
    { ((uint32_t)(&(nvm_tx.cross_mix               )) - (uint32_t)(&nvm_tx)), "X mix"      , "s32x10" ,                      0,                 INT_MIN, INT_MAX               , 1, },
    { ((uint32_t)(&(nvm_tx.pot_throttle.center     )) - (uint32_t)(&nvm_tx)), "T center"   , "s16"    ,          ROACH_ADC_MID,                       0, ROACH_ADC_MAX         , 1, },
    { ((uint32_t)(&(nvm_tx.pot_throttle.deadzone   )) - (uint32_t)(&nvm_tx)), "T deadzone" , "s16"    ,        ROACH_ADC_NOISE,                       0, ROACH_ADC_MAX / 32    , 1, },
    { ((uint32_t)(&(nvm_tx.pot_throttle.boundary   )) - (uint32_t)(&nvm_tx)), "T boundary" , "s16"    ,        ROACH_ADC_NOISE,                       0, ROACH_ADC_MAX / 32    , 1, },
    { ((uint32_t)(&(nvm_tx.pot_throttle.scale      )) - (uint32_t)(&nvm_tx)), "T scale"    , "s16x10" , ROACH_SCALE_MULTIPLIER,                       0, INT_MAX               , 1, },
    { ((uint32_t)(&(nvm_tx.pot_throttle.limit_min  )) - (uint32_t)(&nvm_tx)), "T lim min"  , "s16"    ,                      0,                       0, ROACH_ADC_MAX         , 1, },
    { ((uint32_t)(&(nvm_tx.pot_throttle.limit_max  )) - (uint32_t)(&nvm_tx)), "T lim max"  , "s16"    ,          ROACH_ADC_MAX,                       0, ROACH_ADC_MAX         , 1, },
    { ((uint32_t)(&(nvm_tx.pot_throttle.expo       )) - (uint32_t)(&nvm_tx)), "T expo"     , "s16x10" ,                      0, -ROACH_SCALE_MULTIPLIER, ROACH_SCALE_MULTIPLIER, 1, },
    { ((uint32_t)(&(nvm_tx.pot_throttle.filter     )) - (uint32_t)(&nvm_tx)), "T filter"   , "s16x10" ,   ROACH_FILTER_DEFAULT,                       0, ROACH_SCALE_MULTIPLIER, 1, },
    { ((uint32_t)(&(nvm_tx.pot_steering.center     )) - (uint32_t)(&nvm_tx)), "S center"   , "s16"    ,          ROACH_ADC_MID,                       0, ROACH_ADC_MAX         , 1, },
    { ((uint32_t)(&(nvm_tx.pot_steering.deadzone   )) - (uint32_t)(&nvm_tx)), "S deadzone" , "s16"    ,        ROACH_ADC_NOISE,                       0, ROACH_ADC_MAX / 32    , 1, },
    { ((uint32_t)(&(nvm_tx.pot_steering.boundary   )) - (uint32_t)(&nvm_tx)), "S boundary" , "s16"    ,        ROACH_ADC_NOISE,                       0, ROACH_ADC_MAX / 32    , 1, },
    { ((uint32_t)(&(nvm_tx.pot_steering.scale      )) - (uint32_t)(&nvm_tx)), "S scale"    , "s16x10" , ROACH_SCALE_MULTIPLIER,                       0, INT_MAX               , 1, },
    { ((uint32_t)(&(nvm_tx.pot_steering.limit_min  )) - (uint32_t)(&nvm_tx)), "S lim min"  , "s16"    ,                      0,                       0, ROACH_ADC_MAX         , 1, },
    { ((uint32_t)(&(nvm_tx.pot_steering.limit_max  )) - (uint32_t)(&nvm_tx)), "S lim max"  , "s16"    ,          ROACH_ADC_MAX,                       0, ROACH_ADC_MAX         , 1, },
    { ((uint32_t)(&(nvm_tx.pot_steering.expo       )) - (uint32_t)(&nvm_tx)), "S expo"     , "s16x10" ,                      0, -ROACH_SCALE_MULTIPLIER, ROACH_SCALE_MULTIPLIER, 1, },
    { ((uint32_t)(&(nvm_tx.pot_steering.filter     )) - (uint32_t)(&nvm_tx)), "S filter"   , "s16x10" ,   ROACH_FILTER_DEFAULT,                       0, ROACH_SCALE_MULTIPLIER, 1, },
    { ((uint32_t)(&(nvm_tx.pot_weapon  .center     )) - (uint32_t)(&nvm_tx)), "WC center"  , "s16"    ,                      0,                       0, ROACH_ADC_MAX         , 1, },
    { ((uint32_t)(&(nvm_tx.pot_weapon  .deadzone   )) - (uint32_t)(&nvm_tx)), "WC deadzone", "s16"    ,        ROACH_ADC_NOISE,                       0, ROACH_ADC_MAX / 32    , 1, },
    { ((uint32_t)(&(nvm_tx.pot_weapon  .boundary   )) - (uint32_t)(&nvm_tx)), "WC boundary", "s16"    ,        ROACH_ADC_NOISE,                       0, ROACH_ADC_MAX / 32    , 1, },
    { ((uint32_t)(&(nvm_tx.pot_weapon  .scale      )) - (uint32_t)(&nvm_tx)), "WC scale"   , "s16x10" , ROACH_SCALE_MULTIPLIER,                       0, INT_MAX               , 1, },
    { ((uint32_t)(&(nvm_tx.pot_weapon  .limit_min  )) - (uint32_t)(&nvm_tx)), "WC lim min" , "s16"    ,                      0,                       0, ROACH_ADC_MAX         , 1, },
    { ((uint32_t)(&(nvm_tx.pot_weapon  .limit_max  )) - (uint32_t)(&nvm_tx)), "WC lim max" , "s16"    ,          ROACH_ADC_MAX,                       0, ROACH_ADC_MAX         , 1, },
    { ((uint32_t)(&(nvm_tx.pot_weapon  .expo       )) - (uint32_t)(&nvm_tx)), "WC expo"    , "s16x10" ,                      0, -ROACH_SCALE_MULTIPLIER, ROACH_SCALE_MULTIPLIER, 1, },
    { ((uint32_t)(&(nvm_tx.pot_weapon  .filter     )) - (uint32_t)(&nvm_tx)), "WC filter"  , "s16x10" ,   ROACH_FILTER_DEFAULT,                       0, ROACH_SCALE_MULTIPLIER, 1, },
    ROACH_NVM_GUI_DESC_END,
};

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
