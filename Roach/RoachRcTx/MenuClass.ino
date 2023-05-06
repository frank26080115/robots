RoachMenu::RoachMenu(uint8_t id)
{
    _id = id;
};

virtual void RoachMenu::draw(void)
{
};

virtual void RoachMenu::clear(void)
{
    oled.clear();
    gui_dirty = true;
};

void RoachMenu::display(void)
{
    oled.display();
};

virtual void RoachMenu::taskHP(void)
{
    ctrler_tasks();
    nbtwi_task();
};

virtual void RoachMenu::taskLP(void)
{
};

virtual void RoachMenu::run(void)
{
    onEnter();

    while (_exit == 0)
    {
        yield();

        taskHP();

        if (gui_canDisplay() == false) {
            continue;
        }

        checkButtons();
        if (_exit == 0)
        {
            taskLP();
            draw();

            if (gui_dirty) {
                display();
                gui_dirty = false;
            }
        }
    }

    onExit();
}

virtual void RoachMenu::draw_sidebar(void)
{
    
};

virtual void RoachMenu::draw_title(void)
{
    
};

virtual void RoachMenu::onEnter(void)
{
    _exit = 0;
    gui_dirty = true;
    clear();
}

virtual void RoachMenu::onExit(void)
{
}

virtual void RoachMenu::onButton(uint8_t btn)
{
}

virtual void RoachMenu::checkButtons(void)
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
}

RoachMenuListItem::~RoachMenuListItem(void)
{
    if (_txt) {
        free(_txt);
    }
}

virtual void RoachMenuListItem::sprintName(char* s)
{
    strncpy(s, _txt, (128 / 4));
}

virtual char* RoachMenuListItem::getName(void)
{
    return _txt;
}

RoachMenuFileItem::RoachMenuFileItem(const char* fname) : RoachMenu(0)
{
    int slen = strlen(fname);
    _txt = malloc(slen + 1);
    strcpy(_txt, fname, slen);
}

RoachMenuFileItem::RoachMenuFileItem(const char* fname)
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
    strncpy(s, _fname, (128 / 4));
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

virtual void RoachMenuCfgItem::sprintName(char* s)
{
    strncpy(s, _desc->name, (128 / 4));
}

virtual char* RoachMenuCfgItem::getName(void)
{
    return _desc->name;
}

RoachMenuCfgItemEditor::RoachMenuCfgItemEditor(void* struct_ptr, roach_nvm_gui_desc_t* desc) : RoachMenu(0)
{
    _struct = struct_ptr;
    _desc = desc;
}

virtual void RoachMenuCfgItemEditor::onEnter(void)
{
    RoachMenu::onEnter();
    encoder.get(true); // clear the counter
    roach_lockHeading(true);
}

virtual void RoachMenuCfgItemEditor::onExit(void)
{
    if ((uint32_t)_struct == (uint32_t)&nvm_rx)
    {
        RoSync_uploadChunk(_desc);
    }
    RoachMenu::onExit();
    roach_lockHeading(false);
}

virtual void RoachMenuCfgItemEditor::onButton(uint8_t btn)
{
    switch (btn)
    {
        case BTNID_UP:
            roach_nvm_incval((uint8_t*)_struct, _desc, _desc->step);
            gui_dirty |= true;
            break;
        case BTNID_DOWN:
            roach_nvm_incval((uint8_t*)_struct, _desc, -_desc->step);
            gui_dirty |= true;
            break;
        case BTNID_G5:
        case BTNID_G6:
            _exit = EXITCODE_BACK;
            break;
    }
}

virtual void RoachMenuCfgItemEditor::checkButtons(void)
{
    RoachMenu::checkButtons();
    int x = encoder.get(true);
    if (x != 0) {
        roach_nvm_incval((uint8_t*)_struct, _desc, -x * _desc->step);
        gui_dirty |= true;
    }
}

RoachMenuLister::RoachMenuLister(uint8_t id) : RoachMenu(id)
{
}

virtual void RoachMenuLister::draw(void)
{
    draw_title();
    draw_sidebar();

    int8_t _draw_start_idx, _draw_end_idx;

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
            oled.print(getItemText(j));
        }
    }
}

virtual void RoachMenuLister::onEnter(void)
{
    RoachMenu::onEnter();
    _list_idx = 0;
}

void RoachMenuLister::buildFileList(const char* filter)
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
}

virtual void RoachMenuLister::onExit(void)
{
    RoachMenu::onExit();
    if (_head_node == NULL) {
        return;
    }
    while (_head_node != NULL) {
        RoachMenuListItem* n = (RoachMenuListItem*)_head_node->next;
        delete _head_node;
        _head_node = n;
    }
}

RoachMenuListItem* RoachMenuLister::getNodeAt(int idx)
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
}

void RoachMenuLister::addNode(RoachMenuListItem* item)
{
    if (_head_node == NULL) {
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
    RoachMenuListItem* n = getItem(idx);
    if (n == NULL) {
        return NULL;
    }
    return n->getName();
}

virtual void RoachMenuLister::onButton(uint8_t btn)
{
    switch (btn)
    {
        case BTNID_UP:
            _list_idx = (_list_idx > 0) ? (_list_idx - 1) : _list_idx;
            gui_dirty |= true;
            break;
        case BTNID_DOWN:
            _list_idx = (_list_idx < (_list_cnt - 1)) ? (_list_idx + 1) : _list_idx;
            gui_dirty |= true;
            break;
    }
}

RoachMenuCfgLister::RoachMenuCfgLister(uint8_t id, const char* name, const char* filter, void* struct_ptr, roach_nvm_gui_desc_t* desc_tbl) : RoachMenuLister(id)
{
    _struct = struct_ptr;
    _desc_tbl = desc_tbl;
    _list_cnt = roachnvm_cnt_group(desc_tbl);
    strncpy(_title, name, 30);
}

virtual char* RoachMenuCfgLister::getItemText(int idx)
{
    return desc_tbl[idx].name;
}

virtual void RoachMenuCfgLister::draw_sidebar(void)
{
    drawSideBar("EXIT", "SAVE", true);
}

virtual void RoachMenuCfgLister::draw_title(void)
{
    drawTitleBar((const char*)_title, true, true, true);
}

virtual void RoachMenuCfgLister::onButton(uint8_t btn)
{
    RoachMenuLister::onButton(btn);
    if (_exit == 0)
    {
        switch (btn)
        {
            case BTNID_CENTER:
                {
                    RoachMenuCfgItemEditor* n = new RoachMenuCfgItemEditor(_struct, &(_desc_tbl[_list_idx]));
                    n->run();
                    delete n;
                    gui_dirty = true;
                }
                break;
            case BTNID_G5:
                {
                    RoachMenuFileSaveList* n = new RoachMenuFileSaveList(_filter);
                    n->run();
                    delete n;
                    gui_dirty = true;
                }
                break;
        }
    }
}
