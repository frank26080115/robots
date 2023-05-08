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
            radio.textSend("calibgyro");
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
                    radio.textSend("calibgyro");
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
        };

        virtual void onExit(void)
        {
            pot_throttle.calib_stop();
            pot_steering.calib_stop();
            pot_weapon.calib_stop();
            pot_aux.calib_stop();
            pots_locked = false;
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
        };

        virtual void onExit(void)
        {
            pot_throttle.calib_stop();
            pot_steering.calib_stop();
            pot_weapon.calib_stop();
            pot_aux.calib_stop();
            pots_locked = false;
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
            RoSync_downloadPrint();
        };

    protected:
        virtual void onEnter(void)
        {
            RoachMenu::onEnter();
            RoSync_downloadStart();
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
                    RoSync_downloadStart();
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
            RoSync_uploadPrint();
        };

    protected:
        virtual void onEnter(void)
        {
            RoachMenu::onEnter();
            RoSync_uploadStart();
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
                    RoSync_uploadStart();
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
            RoachMenuLister::onButton(btn);
            if (_exit == 0)
            {
                switch (btn)
                {
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
