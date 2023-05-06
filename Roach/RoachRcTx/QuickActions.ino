
#define QUICKACTION_HOLD_TIME 2000

class QuickAction
{
    public:
        QuickAction(const char* txt, RoachBtn* btn)
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
        RoachBtn* _btn;
        uint32_t t = 0;
        uint32_t last_time = 0;

        virtual void draw_start(void)
        {
            gui_drawWait();
            oled.clear();
            oled.setCursor(0, 0);
            oled.print("QIKACT");
            oled.setCursor(0, 8);
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

class QuickActionDownload : QuickAction
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
            x = oled.width() - 9;
            h = oled.height() - 1;
            oled.drawRect(x, 0, 8, h, 1);
        };

        virtual void draw_barFill(void)
        {
            int h2 = map(t, 0, QUICKACTION_HOLD_TIME, 0, h);
            oled.fillRect(x, 0, 8, h2, 1);
        };
};

class QuickActionUpload : QuickAction
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
            x = oled.width() - 9;
            h = oled.height() - 1;
            oled.drawRect(x, 0, 8, h, 1);
        };

        virtual void draw_barFill(void)
        {
            int h2 = map(t, 0, QUICKACTION_HOLD_TIME, 0, h);
            oled.fillRect(x, oled.height() - h2, 8, h2, 1);
        };
};

class QuickActionCalibGyro : QuickAction
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
            y = (oled.height() / 2) - 4;
            w = oled.width();
            oled.drawRect(0, y, w, 8, 1);
        };

        virtual void draw_barFill(void)
        {
            int x = map(t, 0, QUICKACTION_HOLD_TIME, 0, w);
            oled.fillRect(w - x, y, w, 8, 1);
        };
};

class QuickActionCalibCenters : QuickAction
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
            x = oled.width() - 32;
            y = oled.height() / 2;
            r = 30;
            oled.drawCircle(x, y, r, 0xFF, 1);
        };

        virtual void draw_barFill(void)
        {
            int r2 = map(t, 0, QUICKACTION_HOLD_TIME, 0, r);
            oled.fillCircle(x, y, r2, 0xFF, 1);
        };
};

class QuickActionCalibLimits : QuickAction
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
            y = (oled.height() / 2) - 4;
            w = oled.width();
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
            QuickActionCalibLimits* q = new QuickActionCalibLimits();
            q->run();
            delete q;
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
