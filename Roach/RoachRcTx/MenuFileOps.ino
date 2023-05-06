
RoachMenuFileOpenList::RoachMenuFileOpenList() : RoachMenuLister(MENUID_CONFIG_FILELOAD)
{
}

virtual void RoachMenuFileOpenList::draw_sidebar(void)
{
    drawSideBar("EXIT", "OPEN", true);
}

virtual void RoachMenuFileOpenList::draw_title(void)
{
    drawTitleBar("LOAD FILE", true, true, false);
}

virtual RoachMenuListItem* RoachMenuFileOpenList::getItem(int idx)
{
    return getNodeAt(idx);
}

virtual void RoachMenuFileOpenList::onEnter(void)
{
    RoachMenuLister::onEnter();
    buildFileList(NULL);
}

virtual void RoachMenuFileOpenList::onButton(uint8_t btn)
{
    RoachMenuLister::onButton(btn);
    if (_exit == 0)
    {
        switch (btn)
        {
            case BTNID_G5:
                char* x = getCurItemText();
                if (x != NULL)
                {
                    if (strcmp(x, "CANCEL") == 0) {
                        _exit = EXITCODE_HOME;
                        return;
                    }

                    if (memcmp(x, "rf", 2) == 0) {
                        // TODO: load RF parameters from file and restart the radio engine
                    }
                    else if (memcmp(x, "ctrler", 2) == 0) {
                        // TODO: load controller parameters from file
                    }
                    else if (memcmp(x, "robot", 2) == 0) {
                        // TODO: load robot parameters from file, and then sync it to the robot
                    }
                    else {
                        showError("invalid file name");
                    }
                }
                else {
                    showError("file is null");
                }
                _exit = EXITCODE_BACK;
                break;
            case BTNID_G6:
                _exit = EXITCODE_HOME;
                break;
        }
    }
}

RoachMenuFileSaveList::RoachMenuFileSaveList(const char* filter) : RoachMenuLister(0)
{
    strncpy(_filter, filter, 14);
}

virtual void RoachMenuFileSaveList::draw_sidebar(void)
{
    drawSideBar("EXIT", "SAVE", true);
}

virtual void RoachMenuFileSaveList::draw_title(void)
{
    drawTitleBar("SAVE FILE", true, true, false);
}

virtual RoachMenuListItem* RoachMenuFileSaveList::getItem(int idx)
{
    return getNodeAt(idx);
}

virtual void RoachMenuFileSaveList::onEnter(void)
{
    RoachMenuLister::onEnter();
    RoachMenuFileItem* n = new RoachMenuFileItem("NEW FILE");
    _head_node = (RoachMenuListItem*)n;
    _tail_node = (RoachMenuListItem*)n;
    _list_cnt++;
    buildFileList(_filter[0] != 0 ? _filter : NULL);
}

virtual void RoachMenuFileSaveList::onButton(uint8_t btn)
{
    RoachMenuLister::onButton(btn);
    if (_exit == 0)
    {
        switch (btn)
        {
            case BTNID_G5:
                char* x = getCurItemText();
                if (x != NULL)
                {
                    if (strcmp(x, "CANCEL") == 0) {
                        _exit = EXITCODE_HOME;
                        return;
                    }

                    if (strcmp(x, "NEW FILE") == 0 && _filter[0] != 0)
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
                    else if (memcmp(x, "ctrler", 6) == 0 || memcmp(x, "robot", 5) == 0) {
                        saveToFile(x);
                    }
                    else {
                        showError("invalid file name");
                    }
                }
                else {
                    showError("file is null");
                }
                _exit = EXITCODE_BACK;
                break;
            case BTNID_G6:
                _exit = EXITCODE_BACK;
                break;
        }
    }
}
