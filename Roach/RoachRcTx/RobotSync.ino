enum
{
    ROSYNC_SM_DISCONNECTED,
    ROSYNC_SM_ALLGOOD,
    ROSYNC_SM_NOSYNC,
    ROSYNC_SM_NOSYNC_ERR,
    ROSYNC_SM_NODESC,
    ROSYNC_SM_NODESC_ERR,
    ROSYNC_SM_DOWNLOADDESC,
    ROSYNC_SM_DOWNLOADNVM,
    ROSYNC_SM_UPLOAD,
    ROSYNC_SM_UPLOAD_DONE,
};

uint8_t rosync_statemachine = ROSYNC_SM_DISCONNECTED;

uint32_t rosync_checksum_nvm  = 0;
uint32_t rosync_checksum_desc = 0;

RoachMenuCfgLister* rosync_menu = NULL;
roach_nvm_gui_desc_t* rosync_desc_tbl = NULL; // this stores a massive table that's downloaded from the robot receiver
uint8_t* rosync_nvm = NULL; // this mirrors the NVM structure stored on the robot receiver
uint32_t rosync_nvm_wptr = 0;
uint32_t rosync_nvm_sz = 0;
RoachFile rosync_descDlFile; // this is basically rosync_desc_tbl but in a file on the flash
uint32_t rosync_descDlFileSize = 0;
uint32_t rosync_lastRxTime = 0;
uint32_t rosync_uploadIdx = 0, rosync_uploadTotal = 0;

void rosync_task(void)
{
    if (rosync_statemachine != ROSYNC_SM_DISCONNECTED && radio.isConnected() == false)
    {
        if (rosync_descDlFile.isOpen()) {
            rosync_descDlFile.close();
        }

        rosync_statemachine = ROSYNC_SM_DISCONNECTED;
    }

    switch (rosync_statemachine)
    {
        case ROSYNC_SM_DISCONNECTED:
            if (radio.isConnected())
            {
                // the robot has reconnected, the disconnection might've been momentary, so all the previous data is kept and verified

                if (telem_pkt.chksum_desc != rosync_checksum_desc)
                {
                    // robot is not recognized

                    // if the user is in the menu, we cannot do anything yet
                    // because rosync_loadDescFileId calls delete on rosync_menu
                    if (rosync_menu != NULL) {
                        if (rosync_menu->isRunning()) {
                            rosync_menu->interrupt(EXITCODE_BACK);
                            break;
                        }
                    }

                    // load the description file if it's available
                    if (rosync_loadDescFileId(telem_pkt.chksum_desc))
                    {
                        rosync_loadNvmFile(ROACH_STARTUP_CONF_NAME);
                        uint32_t c = roachnvm_getConfCrc(rosync_nvm, rosync_desc_tbl);
                        if (c == telem_pkt.chksum_nvm)
                        {
                            // in sync
                            rosync_checksum_nvm = c;
                            rosync_statemachine = ROSYNC_SM_ALLGOOD;
                        }
                        else
                        {
                            rosync_statemachine = ROSYNC_SM_NOSYNC;
                        }
                    }
                    else
                    {
                        rosync_statemachine = ROSYNC_SM_NODESC;
                    }
                }
                else
                {
                    // robot is recognized
                    // check if NVM is synchronized
                    if (telem_pkt.chksum_nvm != rosync_checksum_nvm)
                    {
                        if (rosync_nvm == NULL)
                        {
                            // if the user is in the menu, we cannot do anything yet
                            // because rosync_loadDescFileId calls delete on rosync_menu
                            if (rosync_menu != NULL) {
                                if (rosync_menu->isRunning()) {
                                    rosync_menu->interrupt(EXITCODE_BACK);
                                    break;
                                }
                            }

                            if (rosync_loadDescFileId(telem_pkt.chksum_desc) == false)
                            {
                                rosync_statemachine = ROSYNC_SM_NODESC;
                                break;
                            }
                            // at this point in the code, rosync_nvm is not null
                            rosync_loadNvmFile(ROACH_STARTUP_CONF_NAME);
                        }
                        // at this point in the code, rosync_nvm is not null
                        uint32_t c = roachnvm_getConfCrc(rosync_nvm, rosync_desc_tbl);
                        if (c == telem_pkt.chksum_nvm)
                        {
                            rosync_checksum_nvm = c;
                            rosync_statemachine = ROSYNC_SM_ALLGOOD;
                        }
                        else
                        {
                            rosync_statemachine = ROSYNC_SM_NOSYNC;
                        }
                    }
                    else
                    {
                        rosync_statemachine = ROSYNC_SM_ALLGOOD;
                    }
                }
            }
            break;
        case ROSYNC_SM_DOWNLOADDESC:
            if (radio.textAvail())
            {
                radio_binpkt_t pkt;
                radio.textReadBin(&pkt, false);
                if (rosync_downloadDescChunk(&pkt))
                {
                    radio.textReadPtr(true);
                }
            }
            if ((millis() - rosync_lastRxTime) >= 1000)
            {
                if (rosync_descDlFile.isOpen()) {
                    rosync_descDlFile.close();
                }
                rosync_statemachine = ROSYNC_SM_NODESC_ERR;
            }
            break;
        case ROSYNC_SM_DOWNLOADNVM:
            if (radio.textAvail())
            {
                if (rosync_downloadNvmChunk(radio.textReadPtr(false)))
                {
                    radio.textReadPtr(true);
                }
            }
            if ((millis() - rosync_lastRxTime) >= 1000)
            {
                rosync_statemachine = ROSYNC_SM_NOSYNC_ERR;
            }
            break;
        case ROSYNC_SM_UPLOAD:
            if (radio.textIsDone())
            {
                rosync_uploadNextChunk();
            }
            break;
    }
}

bool rosync_downloadDescFile(void)
{
    char fname[32];
    sprintf(fname, "rdesc_0x%08X.bin", telem_pkt.chksum_desc);
    bool x = rosync_descDlFile.open(fname, O_RDWR | O_CREAT);
    if (x == false) {
        rosync_statemachine = ROSYNC_SM_NODESC_ERR;
        Serial.printf("ERR[%u]: unable to open or create \"%s\" to be written\r\n", millis(), fname);
        return false;
    }
    radio.textSendByte(ROACHCMD_SYNC_DOWNLOAD_DESC);
    rosync_statemachine = ROSYNC_SM_DOWNLOADDESC;
    return true;
}

void rosync_downloadNvm(void)
{
    radio.textSendByte(ROACHCMD_SYNC_DOWNLOAD_CONF);
    rosync_nvm_wptr = 0;
    rosync_statemachine = ROSYNC_SM_DOWNLOADNVM;
}

bool rosync_downloadStart(void)
{
    if (rosync_statemachine == ROSYNC_SM_NODESC || rosync_statemachine == ROSYNC_SM_NODESC_ERR)
    {
        if (rosync_descDlFile.isOpen()) {
            rosync_descDlFile.close();
        }
        return rosync_downloadDescFile();
    }
    else if (rosync_nvm_sz > 0 && rosync_nvm != NULL && radio.isConnected())
    {
        rosync_downloadNvm();
        return true;
    }
    return false;
}

bool rosync_downloadDescChunk(radio_binpkt_t* pkt)
{
    if (pkt->typecode == ROACHCMD_SYNC_DOWNLOAD_DESC)
    {
        int i, dlen = pkt->len;
        if (dlen == 0) // indicate end of transfer
        {
            rosync_descDlFile.close();
            if (rosync_loadDescFileId(telem_pkt.chksum_desc))
            {
                if (telem_pkt.chksum_nvm != rosync_checksum_nvm)
                {
                    rosync_loadNvmFile(ROACH_STARTUP_CONF_NAME);
                    if (rosync_checksum_nvm == telem_pkt.chksum_nvm)
                    {
                        rosync_statemachine = ROSYNC_SM_ALLGOOD;
                    }
                    else
                    {
                        rosync_downloadNvm();
                    }
                }
                else
                {
                    rosync_statemachine = ROSYNC_SM_ALLGOOD;
                }
            }
            else
            {
                rosync_statemachine = ROSYNC_SM_NODESC_ERR;
            }
            return true;
        }

        dlen = dlen > NRFRR_PAYLOAD_SIZE2 ? NRFRR_PAYLOAD_SIZE2 : dlen;

        for (i = 0; i < dlen; i++)
        {
            rosync_descDlFile.write(pkt->data[i]);
            rosync_descDlFileSize += 1;
        }
        rosync_lastRxTime = millis();
        return true;
    }
    return false;
}

bool rosync_downloadNvmChunk(radio_binpkt_t* pkt)
{
    if (rosync_nvm == NULL)
    {
        if (pkt->typecode == ROACHCMD_SYNC_DOWNLOAD_CONF && pkt->addr == 0 && pkt->len > 0)
        {
            rosync_nvm = (uint8_t*)malloc(pkt->len);
            rosync_nvm_sz = pkt->len;
        }
        else
        {
            return false;
        }
    }

    bool ret = false;
    bool done = false;

    if (pkt->typecode == ROACHCMD_SYNC_DOWNLOAD_CONF)
    {
        ret = true;
        if (pkt->len == 0) {
            done = true;
        }

        int i, dlen = pkt->len;
        dlen = dlen > NRFRR_PAYLOAD_SIZE2 ? NRFRR_PAYLOAD_SIZE2 : dlen;
        for (i = 0, rosync_nvm_wptr = pkt->addr; done == false && i < dlen && rosync_nvm_wptr < rosync_nvm_sz; i++)
        {
            rosync_nvm[rosync_nvm_wptr] = pkt->data[i];
            rosync_nvm_wptr += 1;
        }
        if (rosync_nvm_wptr >= rosync_nvm_sz) {
            done = true;
        }
        rosync_lastRxTime = millis();
    }

    if (done)
    {
        rosync_checksum_nvm = roachnvm_getConfCrc(rosync_nvm, rosync_desc_tbl);
        if (rosync_checksum_nvm == telem_pkt.chksum_nvm)
        {
            rosync_statemachine = ROSYNC_SM_ALLGOOD;
        }
        else
        {
            rosync_statemachine = ROSYNC_SM_NOSYNC_ERR;
        }
    }
    return ret;
}

bool rosync_loadDescFileId(uint32_t id)
{
    char fname[32];
    sprintf(fname, "rdesc_0x%08X.bin", id);
    return rosync_loadDescFile((const char*)fname, id);
}

bool rosync_loadDescFile(const char* fname, uint32_t id)
{
    RoachFile f;
    bool x = f.open(fname, O_RDONLY);
    if (x == false) {
        Serial.printf("ERR[%u]: tried loading robot desc file \"%s\" but file does not exist\r\n", millis(), fname);
        return false;
    }
    return rosync_loadDescFileObj(&f, id);
}

bool rosync_loadDescFileObj(RoachFile* f, uint32_t id)
{
    if (rosync_menu != NULL)
    {
        if (rosync_menu->isRunning()) {
            // there shouldn't actually be a way to reach here
            rosync_menu->interrupt(EXITCODE_BACK);
            return false;
        }

        delete rosync_menu;
        rosync_menu = NULL;
    }

    if (rosync_desc_tbl != NULL)
    {
        free(rosync_desc_tbl);
        rosync_desc_tbl = NULL;
    }
    if (rosync_nvm != NULL)
    {
        free(rosync_nvm);
        rosync_nvm = NULL;
    }
    uint32_t sz = f->fileSize();
    rosync_desc_tbl = (roach_nvm_gui_desc_t*)malloc(sz);
    int chunk_sz = 256;
    int i, j;
    for (i = 0, j = 1; i < sz && j > 0; i += j)
    {
        j = f->read((void*)rosync_desc_tbl, chunk_sz);
    }

    f->close();

    uint32_t chksum_desc = roach_crcCalc((uint8_t*)rosync_desc_tbl, sz, NULL);
    if (chksum_desc != id && id != 0) {
        char tmpname[32];
        f->getName7(tmpname, 30);
        Serial.printf("ERR[%u]: tried loading robot desc file \"%s\" but contents do not match checksum 0x%08X\r\n", millis(), tmpname, chksum_desc);
        free(rosync_desc_tbl);
        rosync_desc_tbl = NULL;
        return false;
    }

    int sum = 0;
    for (i = 0; ; i++)
    {
        roach_nvm_gui_desc_t* d = &rosync_desc_tbl[i];
        sum = d->byte_offset > sum ? d->byte_offset : sum;
    }
    sum += 4; // just in case
    rosync_nvm_sz = sum;
    rosync_nvm = (uint8_t*)malloc(sum);

    if (rosync_nvm == NULL) {
        Serial.printf("ERR[%u]: unable to allocate memory for rosync_nvm\r\n", millis());
        free(rosync_desc_tbl);
        rosync_desc_tbl = NULL;
        return false;
    }

    rosync_menu = new RoachMenuCfgLister(MENUID_CONFIG_ROBOT, "ROBOT CFG", "robot", (void*)rosync_nvm, rosync_desc_tbl);
    if (rosync_menu == NULL) {
        Serial.printf("ERR[%u]: unable to allocate memory for rosync_menu\r\n", millis());
        free(rosync_desc_tbl);
        rosync_desc_tbl = NULL;
        free(rosync_nvm);
        rosync_nvm = NULL;
        return false;
    }

    rosync_checksum_desc = chksum_desc;
    roachnvm_setdefaults(rosync_nvm, rosync_desc_tbl);
    rosync_checksum_nvm = roachnvm_getConfCrc(rosync_nvm, rosync_desc_tbl);
    rosync_markOnlyFile();
    return true;
}

bool rosync_loadNvmFile(const char* fname)
{
    RoachFile f;
    char fname_buf[32];
    if (fname == NULL)
    {
        sprintf(fname_buf, ROACH_STARTUP_CONF_NAME);
    }
    else
    {
        sprintf(fname_buf, fname);
    }
    bool x = f.open(fname_buf, O_RDONLY);
    if (x == false) {
        Serial.printf("ERR[%u]: unable to find robot NVM file \"%s\"\r\n", millis(), fname_buf);
        return false;
    }

    roachnvm_readfromfile(&f, (uint8_t*)rosync_nvm, rosync_desc_tbl);

    f.close();

    rosync_checksum_nvm = roachnvm_getConfCrc(rosync_nvm, rosync_desc_tbl);
    return true;
}

void rosync_uploadStart(void)
{
    rosync_statemachine = ROSYNC_SM_UPLOAD;
    rosync_uploadIdx = 0;
    rosync_uploadTotal = roachnvm_getDescCnt(rosync_desc_tbl);
}

void rosync_uploadChunk(roach_nvm_gui_desc_t* desc)
{
    radio_binpkt_t pkt;
    pkt.typecode = ROACHCMD_SYNC_UPLOAD_CONF;
    char tmp[32];
    roachnvm_formatitem(tmp, (uint8_t*)rosync_nvm, desc);
    int i = sprintf((char*)(pkt.data), "%s=%s\n", desc->name, tmp);
    pkt.addr = 0;
    pkt.len = i;
    radio.textSendBin(&pkt);
}

void rosync_uploadNextChunk(void)
{
    roach_nvm_gui_desc_t* desc = &rosync_desc_tbl[rosync_uploadIdx];
    if (desc->name[0] == 0) {
        rosync_statemachine = ROSYNC_SM_UPLOAD_DONE;
        return;
    }
    rosync_uploadChunk(desc);
    rosync_uploadIdx += 1;
}

bool rosync_markOnlyFile(void)
{
    if (!fatroot.open("/"))
    {
        Serial.println("open root failed");
        return false;
    }
    bool has_startup = false;
    int cnt = 0;
    char sfname[64];
    char sfname_only[64];
    while (fatfile.openNext(&fatroot, O_RDONLY))
    {
        if (fatfile.isDir() == false)
        {
            fatfile.getName7(sfname, 62);
            if (strncmp("rdesc", sfname, 5) == 0)
            {
                cnt++;
                strncpy(sfname_only, sfname, 62);
            }
            else if (strcmp(ROACH_STARTUP_DESC_NAME, sfname) == 0)
            {
                has_startup = true;
            }
        }
    }
    if (has_startup == false || cnt == 1)
    {
        return rosync_markStartupFile(sfname_only);
    }
    return false;
}

bool rosync_markStartupFile(const char* fname)
{
    return roachnvm_fileCopy(fname, ROACH_STARTUP_DESC_NAME);
}

bool rosync_loadStartup(void)
{
    return rosync_loadDescFile(ROACH_STARTUP_DESC_NAME, 0);
}

bool rosync_isSynced(void)
{
    return rosync_statemachine == ROSYNC_SM_ALLGOOD;
}

void rosync_draw(void)
{
    int y = 0;
    oled.setCursor(0, y);
    oled.print("ROBOT SYNC");
    y += ROACHGUI_LINE_HEIGHT;
    oled.setCursor(0, y);
    switch (rosync_statemachine)
    {
        case ROSYNC_SM_ALLGOOD:
            oled.print("SUCCESS");
            y += ROACHGUI_LINE_HEIGHT;
            oled.setCursor(0, y);
            oled.printf("C 0x%08X", rosync_checksum_desc);
            break;
        case ROSYNC_SM_DISCONNECTED:
            oled.print("DISCONNECTED");
            break;
        case ROSYNC_SM_DOWNLOADDESC:
            oled.print("downloading desc");
            y += ROACHGUI_LINE_HEIGHT;
            oled.setCursor(0, y);
            oled.printf("%u bytes ", rosync_descDlFileSize);
            printEllipses();
            break;
        case ROSYNC_SM_DOWNLOADNVM:
            oled.print("downloading conf");
            y += ROACHGUI_LINE_HEIGHT;
            oled.setCursor(0, y);
            oled.printf("%u / %u b ", rosync_nvm_wptr, rosync_nvm_sz);
            printEllipses();
            y += ROACHGUI_LINE_HEIGHT;
            oled.setCursor(0, y);
            oled.printf("%u %%", map(rosync_nvm_wptr, 0, rosync_nvm_sz, 0, 100));
            y += ROACHGUI_LINE_HEIGHT;
            drawProgressBar(0, y, SCREEN_WIDTH - 16, 7, rosync_nvm_wptr, rosync_nvm_sz);
            break;
        case ROSYNC_SM_UPLOAD:
            oled.print("uploading conf");
            y += ROACHGUI_LINE_HEIGHT;
            oled.setCursor(0, y);
            oled.printf("# %u / %u ", rosync_uploadIdx, rosync_uploadTotal);
            printEllipses();
            y += ROACHGUI_LINE_HEIGHT;
            oled.setCursor(0, y);
            oled.printf("%u %%", map(rosync_uploadIdx, 0, rosync_uploadTotal, 0, 100));
            y += ROACHGUI_LINE_HEIGHT;
            drawProgressBar(0, y, SCREEN_WIDTH - 16, 7, rosync_uploadIdx, rosync_uploadTotal);
            break;
        case ROSYNC_SM_UPLOAD_DONE:
            oled.print("uploading conf");
            y += ROACHGUI_LINE_HEIGHT;
            oled.setCursor(0, y);
            oled.printf("100%% done");
            break;
        case ROSYNC_SM_NODESC:
            oled.print("no desc file");
            break;
        case ROSYNC_SM_NODESC_ERR:
            oled.print("desc file error");
            break;
        case ROSYNC_SM_NOSYNC:
            oled.print("not sync'ed");
            break;
        case ROSYNC_SM_NOSYNC_ERR:
            oled.print("error:");
            y += ROACHGUI_LINE_HEIGHT;
            oled.setCursor(0, y);
            oled.print("not sync'ed");
            y += ROACHGUI_LINE_HEIGHT;
            oled.setCursor(0, y);
            oled.printf("after download");
            break;
    }
}

void rosync_drawShort(void)
{
    switch (rosync_statemachine)
    {
        case ROSYNC_SM_ALLGOOD:
        case ROSYNC_SM_UPLOAD_DONE:
            oled.printf("robot %08X", rosync_checksum_desc);
            break;
        case ROSYNC_SM_DOWNLOADDESC:
        case ROSYNC_SM_DOWNLOADNVM:
            oled.printf("sync busy %c", 0x19);
            break;
        case ROSYNC_SM_UPLOAD:
            oled.printf("sync busy %c", 0x18);
            break;
        default:
            oled.printf("not sync'ed");
            break;
    }
}

// this is a fake object that only exits, passing along the exit code, it doesn't actually show anything
class RobotMenu : public RoachMenu
{
    public:
        RobotMenu(void) : RoachMenu(0)
        {
        };

    protected:
        virtual void onEnter(void)
        {
            if (rosync_menu == NULL)
            {
                // robot is not loaded, try loading the file now
                rosync_loadStartup();
            }

            if (rosync_menu != NULL)
            {
                rosync_menu->run();
                int ec = rosync_menu->getExitCode();
                _exit = ec;
            }
            else
            {
                showError("robot not loaded");
                _exit = EXITCODE_RIGHT;
            }
        };
};

void menu_install_robot()
{
    static RobotMenu m;
    menu_install(&m);
}