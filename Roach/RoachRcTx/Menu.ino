enum
{
    MENU_HOME,                  // display input visualizations, hold center button for 3 seconds for gyro calibration
    MENU_STATS,                 // display extra live data
    MENU_INFO,                  // display stuff like profile, UID, version
    MENU_RAW,                   // display raw input data
    MENU_CONFIG_CALIBSYNC,      // options for calibration and synchronization
    MENU_CONFIG_FILELOAD,       // choose to load a file
    MENU_CONFIG_DRIVE,          // edit parameters about drive train
    MENU_CONFIG_WEAP,           // edit parameters about the weapon
    MENU_CONFIG_CTRLER,         // edit parameters about the controller
    MENU_CONFIG_IMU,            // edit parameters for the IMU (orientation, PID, timeout, etc)
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

roach_nvm_gui_desc_t cfggroup_drive[] = {
    { ((uint32_t)(&(nvm_rx.drive_left_flip        )) - (uint32_t)(&nvm_rx)), "L flip"    , "s8"     ,    0,    0,    1, 1, },
    { ((uint32_t)(&(nvm_rx.drive_right_flip       )) - (uint32_t)(&nvm_rx)), "R flip"    , "s8"     ,    0,    0,    1, 1, },
    { ((uint32_t)(&(nvm_rx.drive_left .center     )) - (uint32_t)(&nvm_rx)), "L center"  , "s16"    , 1500, 1000, 2000, 1, },
    { ((uint32_t)(&(nvm_rx.drive_right.center     )) - (uint32_t)(&nvm_rx)), "R center"  , "s16"    , 1500, 1000, 2000, 1, },
    { ((uint32_t)(&(nvm_rx.drive_left .deadzone   )) - (uint32_t)(&nvm_rx)), "L deadzone", "s16"    , ROACH_ADC_NOISE  ,    0,  100, 1, },
    { ((uint32_t)(&(nvm_rx.drive_right.deadzone   )) - (uint32_t)(&nvm_rx)), "R deadzone", "s16"    , ROACH_ADC_NOISE  ,    0,  100, 1, },
    { ((uint32_t)(&(nvm_rx.drive_left .trim       )) - (uint32_t)(&nvm_rx)), "L trim"    , "s32"    , 0   ,    0,  100, 1, },
    { ((uint32_t)(&(nvm_rx.drive_right.trim       )) - (uint32_t)(&nvm_rx)), "R trim"    , "s32"    , 0   ,    0,  100, 1, },
    { ((uint32_t)(&(nvm_rx.drive_left .scale      )) - (uint32_t)(&nvm_rx)), "L scale"   , "s16x10" , ROACH_SCALE_MULTIPLIER, 0, INT_MAX, 1, },
    { ((uint32_t)(&(nvm_rx.drive_right.scale      )) - (uint32_t)(&nvm_rx)), "R scale"   , "s16x10" , ROACH_SCALE_MULTIPLIER, 0, INT_MAX, 1, },
    { ((uint32_t)(&(nvm_rx.drive_left .limit_min  )) - (uint32_t)(&nvm_rx)), "L lim min" , "s16"    , 1000,  500, 1500, 1, },
    { ((uint32_t)(&(nvm_rx.drive_right.limit_min  )) - (uint32_t)(&nvm_rx)), "R lim min" , "s16"    , 1000,  500, 1500, 1, },
    { ((uint32_t)(&(nvm_rx.drive_left .limit_max  )) - (uint32_t)(&nvm_rx)), "L lim max" , "s16"    , 2000, 1500, 2500, 1, },
    { ((uint32_t)(&(nvm_rx.drive_right.limit_max  )) - (uint32_t)(&nvm_rx)), "R lim max" , "s16"    , 2000, 1500, 2500, 1, },
    ROACH_NVM_GUI_DESC_END,
};

roach_nvm_gui_desc_t cfggroup_weap[] = {
    { ((uint32_t)(&(nvm_rx.weapon.center     )) - (uint32_t)(&nvm_rx)), "WM center"  , "s16"    , 1000, 1000, 2000, 1, },
    { ((uint32_t)(&(nvm_rx.weapon.deadzone   )) - (uint32_t)(&nvm_rx)), "WM deadzone", "s16"    , 0   ,    0,  100, 1, },
    { ((uint32_t)(&(nvm_rx.weapon.trim       )) - (uint32_t)(&nvm_rx)), "WM trim"    , "s32"    , 0   ,    0,  100, 1, },
    { ((uint32_t)(&(nvm_rx.weapon.scale      )) - (uint32_t)(&nvm_rx)), "WM scale"   , "s16x10" , ROACH_SCALE_MULTIPLIER, 0, INT_MAX, 1, },
    { ((uint32_t)(&(nvm_rx.weapon.limit_min  )) - (uint32_t)(&nvm_rx)), "WM lim min" , "s16"    , 1000,  500, 1500, 1, },
    { ((uint32_t)(&(nvm_rx.weapon.limit_max  )) - (uint32_t)(&nvm_rx)), "WM lim max" , "s16"    , 2000, 1500, 2500, 1, },
    ROACH_NVM_GUI_DESC_END,
};

roach_nvm_gui_desc_t cfggroup_imu[] = {
    { ((uint32_t)(&(nvm_rx.imu_orientation  )) - (uint32_t)(&nvm_rx)), "IMU ori"     , "s8"     ,                      0,       0, 6 * 8 - 1, 1, },
    { ((uint32_t)(&(nvm_rx.heading_timeout  )) - (uint32_t)(&nvm_rx)), "IMU timeout" , "s16"    ,                   3000,       0, INT_MAX  , 100, },
    { ((uint32_t)(&(nvm_rx.pid_heading.p    )) - (uint32_t)(&nvm_rx)), "PID P"       , "s16x10" , ROACH_SCALE_MULTIPLIER, INT_MIN, INT_MAX  , 1, },
    { ((uint32_t)(&(nvm_rx.pid_heading.i    )) - (uint32_t)(&nvm_rx)), "PID I"       , "s16x10" ,                      0, INT_MIN, INT_MAX  , 1, },
    { ((uint32_t)(&(nvm_rx.pid_heading.d    )) - (uint32_t)(&nvm_rx)), "PID D"       , "s16x10" ,                      0, INT_MIN, INT_MAX  , 1, },
    { ((uint32_t)(&(nvm_rx.pid_heading.output_limit))      - (uint32_t)(&nvm_rx)), "PID out lim" , "s16" ,       INT_MAX,       0, INT_MAX  , 1, },
    { ((uint32_t)(&(nvm_rx.pid_heading.accumulator_limit)) - (uint32_t)(&nvm_rx)), "PID i lim"   , "s16" ,       INT_MAX,       0, INT_MAX  , 1, },
    { ((uint32_t)(&(nvm_rx.pid_heading.accumulator_decay)) - (uint32_t)(&nvm_rx)), "PID i decay" , "s16" ,             0,       0, INT_MAX  , 1, },
    ROACH_NVM_GUI_DESC_END,
};

roach_nvm_gui_desc_t cfggroup_ctrler[] = {
    { ((uint32_t)(&(nvm_tx.heading_multiplier      )) - (uint32_t)(&nvm_rx)), "H scale"    , "s32x10" , ROACH_SCALE_MULTIPLIER,                 INT_MIN, INT_MAX               , 1, },
    { ((uint32_t)(&(nvm_tx.cross_mix               )) - (uint32_t)(&nvm_rx)), "X mix  "    , "s32x10" ,                      0,                 INT_MIN, INT_MAX               , 1, },
    { ((uint32_t)(&(nvm_tx.pot_throttle.center     )) - (uint32_t)(&nvm_rx)), "T center"   , "s16"    ,          ROACH_ADC_MID,                       0, ROACH_ADC_MAX         , 1, },
    { ((uint32_t)(&(nvm_tx.pot_throttle.deadzone   )) - (uint32_t)(&nvm_rx)), "T deadzone" , "s16"    ,        ROACH_ADC_NOISE,                       0, ROACH_ADC_MAX / 32    , 1, },
    { ((uint32_t)(&(nvm_tx.pot_throttle.boundary   )) - (uint32_t)(&nvm_rx)), "T boundary" , "s16"    ,        ROACH_ADC_NOISE,                       0, ROACH_ADC_MAX / 32    , 1, },
    { ((uint32_t)(&(nvm_tx.pot_throttle.scale      )) - (uint32_t)(&nvm_rx)), "T scale"    , "s16x10" , ROACH_SCALE_MULTIPLIER,                       0, INT_MAX               , 1, },
    { ((uint32_t)(&(nvm_tx.pot_throttle.limit_min  )) - (uint32_t)(&nvm_rx)), "T lim min"  , "s16"    ,                      0,                       0, ROACH_ADC_MAX         , 1, },
    { ((uint32_t)(&(nvm_tx.pot_throttle.limit_max  )) - (uint32_t)(&nvm_rx)), "T lim max"  , "s16"    ,          ROACH_ADC_MAX,                       0, ROACH_ADC_MAX         , 1, },
    { ((uint32_t)(&(nvm_tx.pot_throttle.expo       )) - (uint32_t)(&nvm_rx)), "T expo"     , "s16x10" ,                      0, -ROACH_SCALE_MULTIPLIER, ROACH_SCALE_MULTIPLIER, 1, },
    { ((uint32_t)(&(nvm_tx.pot_throttle.filter     )) - (uint32_t)(&nvm_rx)), "T filter"   , "s16x10" ,   ROACH_FILTER_DEFAULT,                       0, ROACH_SCALE_MULTIPLIER, 1, },
    { ((uint32_t)(&(nvm_tx.pot_steering.center     )) - (uint32_t)(&nvm_rx)), "S center"   , "s16"    ,          ROACH_ADC_MID,                       0, ROACH_ADC_MAX         , 1, },
    { ((uint32_t)(&(nvm_tx.pot_steering.deadzone   )) - (uint32_t)(&nvm_rx)), "S deadzone" , "s16"    ,        ROACH_ADC_NOISE,                       0, ROACH_ADC_MAX / 32    , 1, },
    { ((uint32_t)(&(nvm_tx.pot_steering.boundary   )) - (uint32_t)(&nvm_rx)), "S boundary" , "s16"    ,        ROACH_ADC_NOISE,                       0, ROACH_ADC_MAX / 32    , 1, },
    { ((uint32_t)(&(nvm_tx.pot_steering.scale      )) - (uint32_t)(&nvm_rx)), "S scale"    , "s16x10" , ROACH_SCALE_MULTIPLIER,                       0, INT_MAX               , 1, },
    { ((uint32_t)(&(nvm_tx.pot_steering.limit_min  )) - (uint32_t)(&nvm_rx)), "S lim min"  , "s16"    ,                      0,                       0, ROACH_ADC_MAX         , 1, },
    { ((uint32_t)(&(nvm_tx.pot_steering.limit_max  )) - (uint32_t)(&nvm_rx)), "S lim max"  , "s16"    ,          ROACH_ADC_MAX,                       0, ROACH_ADC_MAX         , 1, },
    { ((uint32_t)(&(nvm_tx.pot_steering.expo       )) - (uint32_t)(&nvm_rx)), "S expo"     , "s16x10" ,                      0, -ROACH_SCALE_MULTIPLIER, ROACH_SCALE_MULTIPLIER, 1, },
    { ((uint32_t)(&(nvm_tx.pot_steering.filter     )) - (uint32_t)(&nvm_rx)), "S filter"   , "s16x10" ,   ROACH_FILTER_DEFAULT,                       0, ROACH_SCALE_MULTIPLIER, 1, },
    { ((uint32_t)(&(nvm_tx.pot_weapon  .center     )) - (uint32_t)(&nvm_rx)), "WC center"  , "s16"    ,                      0,                       0, ROACH_ADC_MAX         , 1, },
    { ((uint32_t)(&(nvm_tx.pot_weapon  .deadzone   )) - (uint32_t)(&nvm_rx)), "WC deadzone", "s16"    ,        ROACH_ADC_NOISE,                       0, ROACH_ADC_MAX / 32    , 1, },
    { ((uint32_t)(&(nvm_tx.pot_weapon  .boundary   )) - (uint32_t)(&nvm_rx)), "WC boundary", "s16"    ,        ROACH_ADC_NOISE,                       0, ROACH_ADC_MAX / 32    , 1, },
    { ((uint32_t)(&(nvm_tx.pot_weapon  .scale      )) - (uint32_t)(&nvm_rx)), "WC scale"   , "s16x10" , ROACH_SCALE_MULTIPLIER,                       0, INT_MAX               , 1, },
    { ((uint32_t)(&(nvm_tx.pot_weapon  .limit_min  )) - (uint32_t)(&nvm_rx)), "WC lim min" , "s16"    ,                      0,                       0, ROACH_ADC_MAX         , 1, },
    { ((uint32_t)(&(nvm_tx.pot_weapon  .limit_max  )) - (uint32_t)(&nvm_rx)), "WC lim max" , "s16"    ,          ROACH_ADC_MAX,                       0, ROACH_ADC_MAX         , 1, },
    { ((uint32_t)(&(nvm_tx.pot_weapon  .expo       )) - (uint32_t)(&nvm_rx)), "WC expo"    , "s16x10" ,                      0, -ROACH_SCALE_MULTIPLIER, ROACH_SCALE_MULTIPLIER, 1, },
    { ((uint32_t)(&(nvm_tx.pot_weapon  .filter     )) - (uint32_t)(&nvm_rx)), "WC filter"  , "s16x10" ,   ROACH_FILTER_DEFAULT,                       0, ROACH_SCALE_MULTIPLIER, 1, },
    ROACH_NVM_GUI_DESC_END,
};

class RoachMenu
{
    public:
        RoachMenu(uint8_t id)
        {
            _id = id;
        };

        inline bool canDisplay(void)
        {
            if (nbtwi_isBusy() != false) {
                return false;
            }
            uint32_t now = millis();
            if ((now - _last_time) >= 80) {
                _last_time = now;
                return true;
            }
            return false;
        };

        virtual void draw(void)
        {
        };

        virtual void clear(void)
        {
            oled.clear();
            _dirty = true;
        };

        inline void display(void)
        {
            oled.display();
        };

        virtual void taskHP(void)
        {
            ctrler_tasks();
            nbtwi_task();
        };

        virtual void taskLP(void)
        {
        };

        virtual void run(void)
        {
            onEnter();

            while (_exit == 0)
            {
                yield();

                taskHP();

                if (canDisplay() == false) {
                    continue;
                }

                checkButtons();
                taskLP();
                draw();

                if (_dirty) {
                    display();
                    _dirty = false;
                }
            }

            onExit();
        };

    protected:
        uint8_t _id;
        uint32_t _last_time = 0;
        bool _dirty = false;
        int _exit = 0;

        virtual void onEnter(void)
        {
            _exit = 0;
            _dirty = true;
            clear();
        };

        virtual void onExit(void)
        {
            _exit = 0;
        }

        virtual void onButton(uint8_t btn)
        {
        };

        virtual void checkButtons(void)
        {
            if (btn_up.hasPressed(true)) {
                this->onButton(BTNID_UP);
            }
            if (btn_down.hasPressed(true)) {
                this->onButton(BTNID_DOWN);
            }
            if (btn_left.hasPressed(true)) {
                this->onButton(BTNID_LEFT);
            }
            if (btn_right.hasPressed(true)) {
                this->onButton(BTNID_RIGHT);
            }
            if (btn_center.hasPressed(true)) {
                this->onButton(BTNID_CENTER);
            }
            if (btn_g5.hasPressed(true)) {
                this->onButton(BTNID_G5);
            }
            if (btn_g6.hasPressed(true)) {
                this->onButton(BTNID_G6);
            }
        };
};

class RoachMenuListItem
{
    public:
        ~RoachMenuListItem(void)
        {
            if (_txt) {
                free(_txt);
            }
        };

        virtual void sprintName(char* s)
        {
            strncpy(s, _txt, (128 / 4));
        };

        virtual char* getName(void)
        {
            return _txt;
        };

        void* next_node;

    protected:
        char* _txt = NULL;
};

class RoachMenuFileItem : RoachMenuListItem
{
    public:
        RoachMenuFileItem(const char* fname)
        {
            int slen = strlen(fname);
            _txt = malloc(slen + 1);
            _fname = malloc(slen + 1);
            strcpy(_txt, fname, slen);
            strcpy(_fname, fname, slen);
            int i;
            for (i = 2; i < slen - 4; i++)
            {
                if (   memcmp(".txt", &(_txt[i]), 5) == 0
                    || memcmp(".TXT", &(_txt[i]), 5) == 0)
                {
                    _txt[i] = 0;
                }
            }
        };

        ~RoachMenuFileItem(void)
        {
            if (_fname) {
                free(_fname);
            }
            if (_txt) {
                free(_txt);
            }
        };

        void sprintFName(char* s)
        {
            strncpy(s, _fname, (128 / 4));
        };

        char* getFName(void)
        {
            return _fname;
        };

    protected:
        char* _fname = NULL;
};

class RoachMenuCfgItem
{
    public:
        RoachMenuCfgItem(void* struct_ptr, roach_nvm_gui_desc_t* desc)
        {
            _struct = struct_ptr;
            _desc = desc;
        };

        virtual void sprintName(char* s)
        {
            strncpy(s, _desc->name, (128 / 4));
        };

        virtual char* getName(void)
        {
            return _desc->name;
        };

    protected:
        roach_nvm_gui_desc_t* _desc = NULL;
        void* _struct = NULL;
};

class RoachMenuCfgItemEditor : RoachMenu
{
    public:
        RoachMenuCfgItemEditor(void* struct_ptr, roach_nvm_gui_desc_t* desc)
             : RoachMenu(0)
        {
            _struct = struct_ptr;
            _desc = desc;
        };

    protected:
        roach_nvm_gui_desc_t* _desc = NULL;
        void* _struct = NULL;

        virtual void onEnter(void)
        {
            RoachMenu::onEnter();
            encoder.get(true); // clear the counter
        };

        virtual void onButton(uint8_t btn)
        {
            switch (btn)
            {
                case BTNID_UP:
                    roach_nvm_incval((uint8_t*)_struct, _desc, _desc->step);
                    _dirty |= true;
                    break;
                case BTNID_DOWN:
                    roach_nvm_incval((uint8_t*)_struct, _desc, -_desc->step);
                    _dirty |= true;
                    break;
                case BTNID_G5:
                case BTNID_G6:
                    _exit = EXITCODE_BACK;
                    break;
            }
        };

        virtual void checkButtons(void)
        {
            RoachMenu::checkButtons();
            int x = encoder.get(true);
            if (x != 0) {
                roach_nvm_incval((uint8_t*)_struct, _desc, -x * _desc->step);
                _dirty |= true;
            }
        }
}

class RoachMenuLister : RoachMenu
{
    public:
        RoachMenuLister(uint8_t id) : RoachMenu(id)
        {
        };

        ~RoachMenuFileOpenList(void)
        {
            if (_head_node == NULL) {
                return;
            }
            while (_head_node != NULL) {
                if (_head_node->text != NULL) {
                    free(_head_node->text);
                }
                roach_txtnode_t* n = _head_node->next;
                free(_head_node);
                _head_node = n;
            }
        };

        virtual void draw(void)
        {
            draw_title();
            draw_sidebar();

            if (_list_cnt <= ROACHMENU_LIST_MAX)
            {
                _draw_start_idx = 0;
                _draw_end_idx = _list_cnt - 1;
            }
            else
            {
                _draw_start_idx = _list_idx - 3;
                _draw_end_idx = _list_idx + 3;
                if (_draw_start_idx < 0) {
                    _draw_start_idx = 0;
                    _draw_end_idx = ROACHMENU_LIST_MAX - 1;
                }
                else if (_draw_end_idx >= (_list_idx - 1)) {
                    _draw_end_idx = _list_idx - 1;
                    _draw_start_idx = _draw_end_idx - (ROACHMENU_LIST_MAX - 1);
                }
            }
            int i, j;
            for (i = 0; i < ROACHMENU_LIST_MAX; i++)
            {
                j = _draw_start_idx + i;
                RoachMenuListItem* itm = getItem(j);
                oled.setCursor(0, ROACHMENU_LINE_HEIGHT * (i + 2));
                if (j == _list_idx)
                {
                    oled.printf(">");
                }
                else if (i == 0 && _draw_start_idx != 0)
                {
                    oled.printf("^");
                }
                else if (i >= (ROACHMENU_LIST_MAX - 1) && _draw_end_idx < (_list_cnt - 1))
                {
                    oled.printf("v");
                }
                if (j >= _draw_start_idx && j <= _draw_end_idx)
                {
                    oled.printf("%s", getCurItem()->name);
                }
            }
        };

    protected:
        virtual RoachMenuListItem* getItem(int idx);
        inline RoachMenuListItem* getCurItem(void) { return getItem(_list_idx); };
        uint8_t _list_cnt, _list_idx;
        int8_t _draw_start_idx, _draw_end_idx;
        RoachMenuListItem* _head_node = NULL, _tail_node = NULL;

        virtual void onEnter(void)
        {
            RoachMenu::onEnter();
            _list_idx = 0;
        };

        virtual void draw_sidebar(void)
        {
            
        };

        virtual void draw_title(void)
        {
            
        };

        void buildFileList(char* filter)
        {
            if (!root.open("/"))
            {
                Serial.println("open root failed");
                return;
            }
            while (file.openNext(&root, O_RDONLY))
            {
                if (file.isDir() == false)
                {
                    char sfname[64];
                    file.getName7(sfname, 62);
                    bool matches = false;
                    if (filter != NULL) {
                        matches = memcmp(filter, sfname, strlen(filter)) == 0;
                    }
                    else {
                        matches = (   memcmp("rf"    , sfname, 2) == 0
                                   || memcmp("robot" , sfname, 5) == 0
                                   || memcmp("ctrler", sfname, 6) == 0
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
                file.close();
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
        };

        RoachMenuListItem* getNodeAt(int idx)
        {
            int i = 0;
            if (_head_node == NULL) {
                return NULL;
            }
            RoachMenuListItem* n = _head_node;
            while (i != idx) {
                n = (RoachMenuListItem*)(n->next_node);
                i += 1;
            }
            return n;
        };

        virtual void onButton(uint8_t btn)
        {
            switch (btn)
            {
                case BTNID_UP:
                    _list_idx = (_list_idx > 0) ? (_list_idx - 1) : _list_idx;
                    _dirty |= true;
                    break;
                case BTNID_DOWN:
                    _list_idx = (_list_idx < (_list_cnt - 1)) ? (_list_idx + 1) : _list_idx;
                    _dirty |= true;
                    break;
                case BTNID_LEFT:
                    _exit = EXITCODE_LEFT;
                    break;
                case BTNID_RIGHT:
                    _exit = EXITCODE_RIGHT;
                    break;
                case BTNID_EXIT:
                    _exit = EXITCODE_HOME;
                    break;
            }
        };

};

class RoachMenuFileOpenList : RoachMenuLister
{
    public:
        RoachMenuFileOpenList(uint8_t id) : RoachMenuLister(id)
        {
        };

    protected:

        virtual RoachMenuListItem* getItem(int idx)
        {
            return getNodeAt(idx);
        }

        virtual void onEnter(void)
        {
            RoachMenuLister::onEnter();
            buildFileList(NULL);
        };

        virtual void onButton(uint8_t btn)
        {
            RoachMenuLister::onButton(btn);
            if (_exit == 0)
            {
                switch (btn)
                {
                    case BTNID_ACTION:
                        RoachMenuListItem* x = getCurItem();
                        if (x != NULL)
                        {
                            if (x->getName() != NULL)
                            {
                                if (strcmp(x->getName(), "CANCEL") == 0) {
                                    _exit = EXITCODE_HOME;
                                    return;
                                }

                                if (memcmp(x->getName(), "rf", 2) == 0) {
                                    // TODO: load RF parameters from file and restart the radio engine
                                }
                                else if (memcmp(x->getName(), "ctrler", 2) == 0) {
                                    // TODO: load controller parameters from file
                                }
                                else if (memcmp(x->getName(), "robot", 2) == 0) {
                                    // TODO: load robot parameters from file, and then sync it to the robot
                                }
                                else {
                                    showError("invalid file name");
                                }
                            }
                            else {
                                showError("file name is null");
                            }
                        }
                        else {
                            showError("file selection is null");
                        }
                        _exit = EXITCODE_HOME;
                        break;
                }
            }
        };
};

class RoachMenuFileSaveList : RoachMenuLister
{
    public:
        RoachMenuFileSaveList(char* filter) : RoachMenuLister(0)
        {
            strncpy(_filter, filter, 14);
        };

    protected:

        char _filter[16];
        char _newfilename[32];

        virtual RoachMenuListItem* getItem(int idx)
        {
            return getNodeAt(idx);
        }

        virtual void onEnter(void)
        {
            RoachMenuLister::onEnter();
            RoachMenuFileItem* n = new RoachMenuFileItem("NEW FILE");
            _head_node = (RoachMenuListItem*)n;
            _tail_node = (RoachMenuListItem*)n;
            _list_cnt++;
            buildFileList(_filter[0] != 0 ? _filter : NULL);
        };

        virtual void onButton(uint8_t btn)
        {
            RoachMenuLister::onButton(btn);
            if (_exit == 0)
            {
                switch (btn)
                {
                    case BTNID_ACTION:
                        RoachMenuListItem* x = getCurItem();
                        if (x != NULL)
                        {
                            if (x->getName() != NULL)
                            {
                                if (strcmp(x->getName(), "CANCEL") == 0) {
                                    _exit = EXITCODE_HOME;
                                    return;
                                }

                                if (strcmp(x->getName(), "NEW FILE") == 0 && _filter[0] != 0)
                                {
                                    _newfilename[0] = '/'; // optional space with the root slash
                                    bool can_save = false;
                                    int i;
                                    for (i = 1; i <= 9999; i++)
                                    {
                                        sprintf(&(_newfilename[1]), "%s_%u.txt", _filter, i);
                                        if (root.exists(&(_newfilename[1])) == false)
                                        {
                                            can_save = true;
                                            break;
                                        }
                                    }
                                    if (can_save)
                                    {
                                        saveToFile(_newfilename);
                                    }
                                    else
                                    {
                                        showError("cannot save new file");
                                    }
                                }
                                else if (memcmp(x->getName(), "ctrler", 6) == 0 || memcmp(x->getName(), "robot", 5) == 0) {
                                    saveToFile(x->getName());
                                }
                                else {
                                    showError("invalid file name");
                                }
                            }
                            else {
                                showError("file name is null");
                            }
                        }
                        else {
                            showError("file selection is null");
                        }
                        _exit = EXITCODE_HOME;
                        break;
                }
            }
        };
};