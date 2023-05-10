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
