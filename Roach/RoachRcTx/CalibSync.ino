class RoachMenuFuncCalibGyro : public RoachMenuFunctionItem
{
    public:
        RoachMenuFuncCalibGyro(void) : RoachMenuFunctionItem("calib gyro")
        {
        };

        virtual void draw(void)
        {
            draw_title();
            draw_sidebar();
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
            draw_title();
            draw_sidebar();
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
            draw_title();
            draw_sidebar();
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
            draw_title();
            draw_sidebar();
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
            draw_title();
            draw_sidebar();
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

        /*
        there wil
        */

        virtual void draw(void)
        {
            draw_title();
            draw_sidebar();
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
            _uid  = nrf5rand_u32();
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
                    nvm_rf.uid  = _uid;
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
                    _uid  = nrf5rand_u32();
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
            //addNode((RoachMenuListItem*)(new RoachMenuFuncFactoryReset()));
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
            RoachMenuLister::onButton(btn); // this should handle the up and down buttons
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
