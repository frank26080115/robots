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
    _txt   = (char*)malloc(slen + 1);
    strncpy0(_txt, name, slen);
}

RoachMenuFileItem::RoachMenuFileItem(const char* fname)
{
    int slen = strlen(fname);
    _txt   = (char*)malloc(slen + 1);
    _fname = (char*)malloc(slen + 1);
    strncpy0(_txt, fname, slen);
    strncpy0(_fname, fname, slen);
    int i;
    for (i = 2; i < slen - 4; i++)
    {
        if (   memcmp(".txt", &(_txt[i]), 5) == 0
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
    RoachEnc_get(true); // clear the counter
    encoder_mode = ENCODERMODE_USEPOT;
    debug_printf("[%u] RoachMenuCfgItemEditor::onEnter \"%s\"\r\n", millis(), _desc->name);
    if (_desc->step == 0) {
        Serial.printf("WARNING: descriptor of \"%s\" step is 0\r\n", _desc->name);
    }
}

void RoachMenuCfgItemEditor::taskLP(void)
{
    //if (cfg_last_change_time != 0 && (uint32_t)_struct == (uint32_t)&nvm_rx)
    //{
    //    if ((millis() - cfg_last_change_time) >= 1000) {
    //        rosync_uploadChunk(_desc);
    //        cfg_last_change_time = 0;
    //    }
    //}
}

void RoachMenuCfgItemEditor::onExit(void)
{
    //if ((uint32_t)_struct == (uint32_t)&nvm_rx && cfg_last_change_time != 0)
    //{
    //    rosync_uploadChunk(_desc);
    //}
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
    int mul = _desc->step != 0 ? _desc->step : 1;
    switch (btn)
    {
        case BTNID_UP:
            modVal(1);
            break;
        case BTNID_DOWN:
            modVal(-1);
            break;
        case BTNID_G6:
            _exit = EXITCODE_BACK;
            break;
    }
}

void RoachMenuCfgItemEditor::checkButtons(void)
{
    RoachMenu::checkButtons(); // this will call RoachMenuCfgItemEditor::onButton if needed
    modVal(-RoachEnc_get(true));
}

void RoachMenuCfgItemEditor::modVal(int x)
{
    if (x != 0)
    {
        debug_printf("[%u] cfg val change \"%s\" by %d , new val = ", millis(), _desc->name, x);
        int mul = _desc->step != 0 ? _desc->step : 1;
        roachnvm_incval((uint8_t*)_struct, _desc, x * mul);
        debug_printf("%d\r\n", roachnvm_getval((uint8_t*)_struct, _desc));
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

    if (_list_cnt <= ROACHMENU_LIST_MAX) // the list is not long enough to require paging
    {
        _draw_start_idx = 0;
        _draw_end_idx = _list_cnt - 1;
    }
    else // the list requires paging
    {
        _draw_start_idx = _list_idx - 3;
        _draw_end_idx = _draw_start_idx + ROACHMENU_LIST_MAX;
        if (_draw_start_idx < 0) { // selected index is too high up
            _draw_start_idx = 0;
            _draw_end_idx = ROACHMENU_LIST_MAX - 1;
        }
        else if (_draw_end_idx >= _list_cnt) { // selected index is too far down
            _draw_end_idx = _list_cnt;
            _draw_start_idx = _draw_end_idx - ROACHMENU_LIST_MAX;
        }
    }
    int i; // displayed index from top of screen
    int j; // item index
    for (i = 0; i < ROACHMENU_LIST_MAX && i < _list_cnt; i++)
    {
        j = _draw_start_idx + i;
        oled.setCursor(0, ROACHGUI_LINE_HEIGHT * (i + 2));
        if (j == _list_idx) // current selected index is the item
        {
            oled.write((char)GUISYMB_THIS_ARROW);
        }
        else if (i == 0 && _draw_start_idx != 0) // top of screen and there's stuff above
        {
            oled.write((char)GUISYMB_UP_ARROW);
        }
        else if (i >= (ROACHMENU_LIST_MAX - 1) && _draw_end_idx < _list_cnt) // bottom of screen and stuff below
        {
            oled.write((char)GUISYMB_DOWN_ARROW);
        }
        else
        {
            oled.write((char)' ');
        }

        if (j >= _draw_start_idx && j <= _draw_end_idx) // item is within view
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
        addCancelNode();
        _delete_on_exit = true;
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
                matches = (   memcmp("rf"    , sfname, 2) == 0
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

                debug_printf("[%u] buildFileList added \"%s\" -> %u\r\n", millis(), sfname, _list_cnt);

                _list_cnt += 1;
                _delete_on_exit = true;
            }
            else
            {
                debug_printf("[%u] buildFileList ignored \"%s\"\r\n", millis(), sfname);
            }
        }
        fatfile.close();
    }
    fatroot.close();

    addCancelNode();
    _delete_on_exit = true;
}

void RoachMenuLister::addCancelNode(void)
{
    RoachMenuFileItem* nc = new RoachMenuFileItem("CANCEL");
    if (_head_node == NULL) {
        _head_node = (RoachMenuListItem*)nc;
        _tail_node = (RoachMenuListItem*)nc;
    }
    else if (_tail_node != NULL) {
        _tail_node->next_node = (void*)nc;
        _tail_node = (RoachMenuListItem*)nc;
    }
    debug_printf("[%u] buildFileList added CANCEL -> %u\r\n", millis(), _list_cnt);
    _list_cnt++;
}

void RoachMenuLister::onExit(void)
{
    RoachMenu::onExit();
    if (_delete_on_exit)
    {
        if (_head_node == NULL) {
            return;
        }
        while (_head_node != NULL) {
            RoachMenuListItem* n = (RoachMenuListItem*)_head_node->next_node;
            delete _head_node;
            _head_node = n;
        }
        _head_node = NULL;
        _tail_node = NULL;
        _list_cnt = 0;
    }
}

RoachMenuListItem* RoachMenuLister::getNodeAt(int idx)
{
    int i = 0;
    if (_head_node == NULL) { // no nodes at all in list
        return NULL;
    }
    // iterate through the linked list until the count matches
    RoachMenuListItem* n = _head_node;
    while (i != idx && n != NULL) {
        n = (RoachMenuListItem*)(n->next_node);
        i += 1;
    }
    return n;
}

void RoachMenuLister::addNode(RoachMenuListItem* item)
{
    if (_head_node == NULL) { // first ever item
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
            debug_printf("list idx up %d/%d\r\n", _list_idx, _list_cnt);
            break;
        case BTNID_DOWN:
            _list_idx = (_list_idx < (_list_cnt - 1)) ? (_list_idx + 1) : _list_idx;
            debug_printf("list idx down %d/%d\r\n", _list_idx, _list_cnt);
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
                if (_list_idx <= 0) { // at top, left button changes menu
                    _exit = EXITCODE_LEFT;
                    break;
                }
                else // not at top, left button seeks for categories
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
                if (_list_idx >= _list_cnt - 1) { // at bottom, right button changes menu
                    _exit = EXITCODE_RIGHT;
                    break;
                }
                else // not at bottom, right button seeks for categories
                {
                    bool found = false;
                    for (; _list_idx < _list_cnt; _list_idx--)
                    {
                        if (memcmp("cat", _desc_tbl[_list_idx].type_code, 4) == 0)
                        {
                            found = true;
                            strncpy0(_title, _desc_tbl[_list_idx].name, 30);
                            break;
                        }
                    }
                    if (found == false)
                    {
                        _list_idx = _list_cnt - 1;
                        _exit = EXITCODE_RIGHT;
                        break;
                    }
                }
            }
            break;
        case BTNID_UP:
        case BTNID_DOWN:
            {
                RoachMenuLister::onButton(btn); // this will handle the change of list index
                if (memcmp("cat", _desc_tbl[_list_idx].type_code, 4) == 0) // hit a new category
                {
                    strncpy0(_title, _desc_tbl[_list_idx].name, 30);
                    break;
                }
            }
            break;
        case BTNID_CENTER:
            {
                if (memcmp("cat", _desc_tbl[_list_idx].type_code, 4) != 0) // cannot click on a category item
                {
                    if (memcmp("func", _desc_tbl[_list_idx].type_code, 5) == 0)
                    {
                        // TODO: send a command
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
