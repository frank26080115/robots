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
    if (rosync_statemachine != ROSYNC_SM_DISCONNECTED
        #ifdef DEVMODE_NO_RADIO
        && radio.isConnected() == false
        #endif
        )
    {
        debug_printf("[%u] rosync disconnected", millis());
        if (rosync_descDlFile.isOpen()) {
            debug_printf(", desc file closed");
            rosync_descDlFile.close();
        }
        debug_printf("\r\n");

        rosync_statemachine = ROSYNC_SM_DISCONNECTED;
    }

    switch (rosync_statemachine)
    {
        case ROSYNC_SM_DISCONNECTED:
            #ifndef DEVMODE_NO_RADIO
            if (radio.isConnected())
            {
                // the robot has reconnected, the disconnection might've been momentary, so all the previous data is kept and verified

                if (telem_pkt.chksum_desc != rosync_checksum_desc && telem_pkt.chksum_desc != 0)
                {
                    // robot is not recognized
                    debug_printf("[%u] rosync new robot not recognized (0x%04X != 0x%04X)\r\n", millis(), telem_pkt.chksum_desc, rosync_checksum_desc);

                    // if the user is in the menu, we cannot do anything yet
                    // because rosync_loadDescFileId calls delete on rosync_menu
                    if (rosync_menu != NULL) {
                        if (rosync_menu->isRunning()) {
                            rosync_menu->interrupt(EXITCODE_BACK);
                            debug_printf("[%u] rosync need to interrupt rosync_menu\r\n", millis());
                            break;
                        }
                    }

                    // load the description file if it's available
                    if (rosync_loadDescFileId(telem_pkt.chksum_desc))
                    {
                        rosync_loadNvmFile(ROACH_STARTUP_CONF_NAME);
                        debug_printf("[%u] rosync loaded all files for robot\r\n", millis());
                        uint32_t c = roachnvm_getConfCrc(rosync_nvm, rosync_desc_tbl);
                        if (c == telem_pkt.chksum_nvm)
                        {
                            // in sync
                            rosync_checksum_nvm = c;
                            rosync_statemachine = ROSYNC_SM_ALLGOOD;
                            debug_printf("[%u] rosync robot NVM is in sync with local NVM\r\n", millis());
                        }
                        else
                        {
                            rosync_statemachine = ROSYNC_SM_NOSYNC;
                            debug_printf("[%u] rosync robot NVM is NOT in sync\r\n", millis());
                        }
                    }
                    else
                    {
                        debug_printf("[%u] rosync failed to load desc file for robot\r\n", millis());
                        rosync_statemachine = ROSYNC_SM_NODESC;
                    }
                }
                else if (rosync_checksum_desc != 0)
                {
                    // robot is recognized
                    debug_printf("[%u] rosync new robot is recognized (0x%04X)\r\n", millis(), rosync_checksum_desc);

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
                                    debug_printf("[%u] rosync need to interrupt rosync_menu\r\n", millis());
                                    break;
                                }
                            }

                            if (rosync_loadDescFileId(telem_pkt.chksum_desc) == false)
                            {
                                rosync_statemachine = ROSYNC_SM_NODESC;
                                debug_printf("[%u] rosync failed to load desc file for robot\r\n", millis());
                                break;
                            }
                            // at this point in the code, rosync_nvm is not null
                            rosync_loadNvmFile(ROACH_STARTUP_CONF_NAME);
                            debug_printf("[%u] rosync loaded all files for robot\r\n", millis());
                        }
                        // at this point in the code, rosync_nvm is not null and rosync_desc_tbl should not be null
                        uint32_t c = roachnvm_getConfCrc(rosync_nvm, rosync_desc_tbl);
                        if (c == telem_pkt.chksum_nvm)
                        {
                            rosync_checksum_nvm = c;
                            rosync_statemachine = ROSYNC_SM_ALLGOOD;
                            debug_printf("[%u] rosync robot NVM is in sync with local NVM\r\n", millis());
                        }
                        else
                        {
                            rosync_statemachine = ROSYNC_SM_NOSYNC;
                            debug_printf("[%u] rosync robot NVM is NOT in sync\r\n", millis());
                        }
                    }
                    else
                    {
                        rosync_statemachine = ROSYNC_SM_ALLGOOD;
                        debug_printf("[%u] rosync new robot NVM is already in sync with local NVM\r\n", millis());
                    }
                }
                else
                {
                    // TODO: what happens if robot does not report a descriptor?
                    // right now, do nothing, there won't be anything to sync
                }
            }
            #endif
            break;
        case ROSYNC_SM_DOWNLOADDESC:
            #ifndef DEVMODE_NO_RADIO
            if (radio.textAvail())
            {
                if (rosync_downloadDescChunk(radio.textReadPtr(false)))
                {
                    radio.textReadPtr(true); // clear the buffer for other tasks
                }
            }
            if ((millis() - rosync_lastRxTime) >= 1000) // timeout
            {
                debug_printf("[%u] ERR: rosync ROSYNC_SM_DOWNLOADDESC timed out\r\n", millis());
                if (rosync_descDlFile.isOpen()) {
                    rosync_descDlFile.close();
                }
                rosync_statemachine = ROSYNC_SM_NODESC_ERR;
            }
            #endif
            break;
        case ROSYNC_SM_DOWNLOADNVM:
            #ifndef DEVMODE_NO_RADIO
            if (radio.textAvail())
            {
                if (rosync_downloadNvmChunk(radio.textReadPtr(false)))
                {
                    radio.textReadPtr(true); // clear the buffer for other tasks
                }
            }
            if ((millis() - rosync_lastRxTime) >= 1000) // timeout
            {
                debug_printf("[%u] ERR: rosync ROSYNC_SM_DOWNLOADNVM timed out\r\n", millis());
                rosync_statemachine = ROSYNC_SM_NOSYNC_ERR;
            }
            #endif
            break;
        case ROSYNC_SM_UPLOAD:
            #ifndef DEVMODE_NO_RADIO
            if (radio.textIsDone()) // if not busy
            {
                rosync_uploadNextChunk();
            }
            #endif
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
    #ifndef DEVMODE_NO_RADIO
    radio.textSendByte(ROACHCMD_SYNC_DOWNLOAD_DESC);
    #endif
    rosync_statemachine = ROSYNC_SM_DOWNLOADDESC;
    debug_printf("[%u] rosync_downloadDescFile started, file: \"%s\"\r\n", millis(), fname);
    return true;
}

void rosync_downloadNvm(void)
{
    #ifndef DEVMODE_NO_RADIO
    radio.textSendByte(ROACHCMD_SYNC_DOWNLOAD_CONF);
    #endif
    rosync_nvm_wptr = 0;
    rosync_statemachine = ROSYNC_SM_DOWNLOADNVM;
    debug_printf("[%u] rosync_downloadNvm started\r\n", millis());
}

bool rosync_downloadStart(void)
{
    if (rosync_statemachine == ROSYNC_SM_NODESC || rosync_statemachine == ROSYNC_SM_NODESC_ERR) // descriptor must be downloaded first
    {
        if (rosync_descDlFile.isOpen()) {
            debug_printf("[%u] WARNING: rosync_downloadStart previous file needed to be closed\r\n", millis());
            rosync_descDlFile.close();
        }
        return rosync_downloadDescFile();
    }
    #ifndef DEVMODE_NO_RADIO
    else if (rosync_nvm_sz > 0 && rosync_nvm != NULL && radio.isConnected()) // there is room allocated to download
    {
        rosync_downloadNvm();
        return true;
    }
    #endif
    else
    {
        Serial.printf("[%u] ERROR: rosync_downloadStart unable to start download\r\n", millis());
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
            debug_printf("[%u] rosync_downloadDescChunk end of transfer\r\n", millis());
            rosync_descDlFile.close();
            if (rosync_loadDescFileId(telem_pkt.chksum_desc))
            {
                if (telem_pkt.chksum_nvm != rosync_checksum_nvm)
                {
                    rosync_loadNvmFile(ROACH_STARTUP_CONF_NAME);
                    if (rosync_checksum_nvm == telem_pkt.chksum_nvm)
                    {
                        debug_printf("NVM in sync\r\n");
                        rosync_statemachine = ROSYNC_SM_ALLGOOD;
                    }
                    else
                    {
                        debug_printf("not in sync, starting sync\r\n");
                        rosync_downloadNvm();
                    }
                }
                else
                {
                    debug_printf("NVM already in sync\r\n");
                    rosync_statemachine = ROSYNC_SM_ALLGOOD;
                }
            }
            else
            {
                Serial.printf("[%u] ERROR: rosync_downloadDescChunk unable to read the file that was just written\r\n", millis());
                rosync_statemachine = ROSYNC_SM_NODESC_ERR;
            }
            return true;
        }

        dlen = dlen > NRFRR_PAYLOAD_SIZE2 ? NRFRR_PAYLOAD_SIZE2 : dlen;

        for (i = 0; i < dlen; i++)
        {
            RoachWdt_feed();
            rosync_descDlFile.write(pkt->data[i]);
            rosync_descDlFileSize += 1;
        }
        rosync_lastRxTime = millis();

        debug_printf("[%u] rosync_downloadDescChunk %d %d\r\n", rosync_lastRxTime, dlen, rosync_descDlFileSize);

        return true;
    }
    else
    {
        debug_printf("[%u] ERROR: rosync_downloadDescChunk unknown pkt->typecode 0x%02X\r\n", millis(), pkt->typecode);
    }
    return false;
}

bool rosync_downloadNvmChunk(radio_binpkt_t* pkt)
{
    if (rosync_nvm == NULL) // unknown robot
    {
        if (pkt->typecode == ROACHCMD_SYNC_DOWNLOAD_CONF && pkt->addr == 0 && pkt->len > 0)
        {
            // the first ever transmission will indicate how long the actual structure is
            rosync_nvm = (uint8_t*)malloc(pkt->len);
            rosync_nvm_sz = pkt->len;
            debug_printf("[%u] rosync_downloadNvmChunk allocated memory for rosync_nvm %u\r\n", millis(), rosync_nvm_sz);
        }
        else
        {
            Serial.printf("[%u] ERROR: rosync_downloadNvmChunk rosync_nvm is NULL, but robot did not indicate how much memory to allocate (%u %u %u)\r\n", millis(), pkt->typecode, pkt->addr, pkt->len);
            return false;
        }
    }
    // note: rosync_nvm is free'ed when a new descriptor file is loaded

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
        debug_printf("[%u] rosync_downloadNvmChunk %d %d\r\n", rosync_lastRxTime, dlen, rosync_nvm_wptr);
    }
    else
    {
        debug_printf("[%u] ERROR: rosync_downloadNvmChunk unknown pkt->typecode 0x%02X\r\n", millis(), pkt->typecode);
    }

    if (done)
    {
        debug_printf("[%u] rosync_downloadNvmChunk end of transfer", millis());

        rosync_checksum_nvm = roachnvm_getConfCrc(rosync_nvm, rosync_desc_tbl);
        if (rosync_checksum_nvm == telem_pkt.chksum_nvm)
        {
            debug_printf(", robot is in sync\r\n");
            rosync_statemachine = ROSYNC_SM_ALLGOOD;
        }
        else
        {
            debug_printf(", robot is NOT in sync (0x%04X != 0x%04X)\r\n", rosync_checksum_nvm, telem_pkt.chksum_nvm);
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
    RoachWdt_feed();
    bool x = f.open(fname, O_RDONLY);
    if (x == false) {
        Serial.printf("ERR[%u]: tried loading robot desc file \"%s\" but file does not exist\r\n", millis(), fname);
        return false;
    }
    debug_printf("[%u] rosync_loadDescFile succesful open \"%s\" 0x%08X\r\n", millis(), fname, id);
    return rosync_loadDescFileObj(&f, id);
}

bool rosync_loadDescFileObj(RoachFile* f, uint32_t id)
{
    // the descriptor of the robot is stored as a file on the flash
    // load that into RAM

    if (rosync_menu != NULL)
    {
        if (rosync_menu->isRunning()) {
            // there shouldn't actually be a way to reach here
            Serial.printf("[%u] ERROR: rosync_loadDescFileObj but rosync_menu is still running\r\n", millis());
            rosync_menu->interrupt(EXITCODE_BACK);
            return false;
        }

        delete rosync_menu;
        rosync_menu = NULL;
        debug_printf("[%u] rosync_loadDescFileObj deleted rosync_menu\r\n", millis());
    }

    if (rosync_desc_tbl != NULL)
    {
        free(rosync_desc_tbl);
        rosync_desc_tbl = NULL;
        debug_printf("[%u] rosync_loadDescFileObj deleted rosync_desc_tbl\r\n", millis());
    }
    if (rosync_nvm != NULL)
    {
        free(rosync_nvm);
        rosync_nvm = NULL;
        debug_printf("[%u] rosync_loadDescFileObj deleted rosync_nvm\r\n", millis());
    }
    uint32_t sz = f->fileSize();
    rosync_desc_tbl = (roach_nvm_gui_desc_t*)malloc(sz);
    int chunk_sz = 256;
    int i, j;
    for (i = 0, j = 1; i < sz && j > 0; i += j)
    {
        RoachWdt_feed();
        j = f->read((void*)rosync_desc_tbl, chunk_sz);
    }

    RoachWdt_feed();
    f->close();
    debug_printf("[%u] rosync_loadDescFileObj finished loading the file (%u)\r\n", millis(), sz);

    uint32_t chksum_desc = roach_crcCalc((uint8_t*)rosync_desc_tbl, sz, NULL);
    if (chksum_desc != id && id != 0) {
        char tmpname[32];
        f->getName7(tmpname, 30);
        Serial.printf("ERR[%u]: tried loading robot desc file \"%s\" but contents do not match checksum 0x%08X\r\n", millis(), tmpname, chksum_desc);
        free(rosync_desc_tbl);
        rosync_desc_tbl = NULL;
        return false;
    }

    int highest_addr = 0;
    for (i = 0; ; i++)
    {
        roach_nvm_gui_desc_t* d = &rosync_desc_tbl[i];
        highest_addr = d->byte_offset > highest_addr ? d->byte_offset : highest_addr;
    }
    highest_addr += 4; // just in case
    rosync_nvm_sz = highest_addr;
    rosync_nvm = (uint8_t*)malloc(highest_addr);

    debug_printf("[%u] rosync_loadDescFileObj allocated rosync_nvm by estimate (%u)\r\n", millis(), highest_addr);

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
    roachnvm_setdefaults(rosync_nvm, rosync_desc_tbl); // the contents have not been sync'ed yet, but the UI still needs some valid data
    rosync_checksum_nvm = roachnvm_getConfCrc(rosync_nvm, rosync_desc_tbl);
    rosync_markOnlyFile();
    debug_printf("[%u] rosync_loadDescFileObj all done (0x%04X 0x%04X))\r\n", millis(), rosync_checksum_desc, rosync_checksum_nvm);
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
    RoachWdt_feed();
    bool x = f.open(fname_buf, O_RDONLY);
    if (x == false) {
        Serial.printf("ERR[%u]: unable to find robot NVM file \"%s\"\r\n", millis(), fname_buf);
        return false;
    }

    debug_printf("[%u] rosync_loadNvmFile succesful open \"%s\"\r\n", millis(), fname_buf);

    roachnvm_readfromfile(&f, (uint8_t*)rosync_nvm, rosync_desc_tbl);

    RoachWdt_feed();
    f.close();

    rosync_checksum_nvm = roachnvm_getConfCrc(rosync_nvm, rosync_desc_tbl);
    return true;
}

void rosync_uploadStart(void)
{
    rosync_statemachine = ROSYNC_SM_UPLOAD;
    rosync_uploadIdx = 0;
    rosync_uploadTotal = roachnvm_getDescCnt(rosync_desc_tbl);
    debug_printf("[%u] rosync_uploadStart about to send %u items\r\n", millis(), rosync_uploadTotal);
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
    #ifndef DEVMODE_NO_RADIO
    radio.textSendBin(&pkt);
    #endif
    debug_printf("[%u] rosync_uploadNextChunk sent {%u} \"%s=%s\"\r\n", millis(), rosync_uploadIdx, desc->name, tmp);
}

void rosync_uploadNextChunk(void)
{
    roach_nvm_gui_desc_t* desc = &rosync_desc_tbl[rosync_uploadIdx];
    if (desc->name == NULL || desc->name[0] == 0) {
        debug_printf("[%u] rosync_uploadNextChunk finished %u\r\n", millis(), rosync_uploadIdx);
        rosync_statemachine = ROSYNC_SM_UPLOAD_DONE;
        return;
    }
    rosync_uploadChunk(desc);
    rosync_uploadIdx += 1;
}

bool rosync_markOnlyFile(void)
{
    RoachWdt_feed();
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
        RoachWdt_feed();
        if (fatfile.isDir() == false)
        {
            fatfile.getName7(sfname, 62);
            if (strncmp("rdesc", sfname, 5) == 0)
            {
                cnt++;
                strncpy0(sfname_only, sfname, 62);
                debug_printf("[%u] rosync_markOnlyFile found file \"%s\" {%u}\r\n", millis(), sfname, cnt);
            }
            else if (strcmp(ROACH_STARTUP_DESC_NAME, sfname) == 0)
            {
                debug_printf("[%u] rosync_markOnlyFile found startup file \"%s\" {%u}\r\n", millis(), sfname, cnt);
                has_startup = true;
            }
        }
    }
    RoachWdt_feed();
    fatfile.close();
    fatroot.close();
    if (has_startup == false || cnt == 1)
    {
        debug_printf("[%u] rosync_markOnlyFile is marking the only file \"%s\" as the startup file\r\n", millis(), sfname_only);
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

void rosync_debugState(Stream* stream)
{
    stream->printf("[%u] rosync_debugState\r\n", millis());
    stream->printf("state machine = %u\r\n", rosync_statemachine);
    stream->printf("rosync_checksum_nvm = 0x%04X\r\n", rosync_checksum_nvm);
    stream->printf("rosync_checksum_desc = 0x%04X\r\n", rosync_checksum_desc);

    if (rosync_menu == NULL) {
        stream->printf("rosync_menu = NULL\r\n");
    }
    else {
        if (rosync_menu->isRunning()) {
            stream->printf("rosync_menu is running\r\n");
        }
        else {
            stream->printf("rosync_menu is allocated but not running\r\n");
        }
    }

    if (rosync_desc_tbl == NULL) {
        stream->printf("rosync_desc_tbl = NULL\r\n");
    }
    else {
        stream->printf("rosync_desc_tbl has %u items\r\n", roachnvm_getDescCnt(rosync_desc_tbl));
    }

    if (rosync_nvm == NULL) {
        stream->printf("rosync_nvm = NULL\r\n");
    }
    else {
        stream->printf("rosync_nvm size %u, chksum 0x%04X, wptr %d\r\n", rosync_nvm_sz, rosync_checksum_nvm, rosync_nvm_wptr);
    }

    stream->printf("rosync_descDlFileSize = %u\r\n", rosync_descDlFileSize);
    stream->printf("rosync_uploadIdx = %u ; rosync_uploadTotal = %u\r\n", rosync_uploadIdx, rosync_uploadTotal);
    stream->printf("=========\r\n");
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
                _exit = EXITCODE_UNABLE;
            }
        };
};

void menu_install_robot()
{
    static RobotMenu m;
    menu_install(&m);
}