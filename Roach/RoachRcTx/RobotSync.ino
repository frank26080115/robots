
enum
{
    ROSYNC_DISCONNECTED,
    ROSYNC_DONE,
    ROSYNC_QUERY,
    ROSYNC_MISMATCHED,
    ROSYNC_DONE_SYNCED,
    ROSYNC_DOWNLOAD_START,
    ROSYNC_DOWNLOAD_WAIT,
    ROSYNC_UPLOAD_START,
    ROSYNC_UPLOAD_WAIT,
};

uint8_t rosync_statemachine = 0;
uint32_t rosync_state_timer = 0;

roach_rx_nvm_t rosync_nvm_cache;
uint8_t* rosync_nvm_cache_ptr = (uint8_t*)&rosync_nvm_cache;
bool rosync_sync_complete = false;
int rosync_download_progress = 0;
int rosync_retry_cnt = 0;
bool rosync_matched = false;

uint8_t rosync_rxitems_cnt;
uint8_t rosync_upload_cache[NRFRR_PAYLOAD_SIZE];
uint32_t rosync_upload_lasttime = 0;

void RoSync_task()
{
    uint32_t now = millis();

    if (radio.connected() == false) {
        rosync_statemachine = ROSYNC_DISCONNECTED;
    }

    switch (rosync_statemachine)
    {
        case ROSYNC_DISCONNECTED:
            {
                if (radio.connected()) {
                    // connected() requires at least one telemetry packet received
                    rosync_statemachine = ROSYNC_QUERY;
                    rosync_rxitems_cnt = roachnvm_rx_getcnt();
                    rosync_matched = false;
                    break;
                }
            }
            break;
        case ROSYNC_QUERY:
            {
                nvm_rx.magic = nvm_rf.seed;
                nvm_rx.checksum = crc32_calc((uint8_t*)nvm_rx, sizeof(roach_rx_nvm_t) - 4);
                rosync_matched = (telem_pkt.checksum == nvm_rx.checksum);
                rosync_statemachine = ROSYNC_DONE;
                break;
            }
            break;
        case ROSYNC_DOWNLOAD_START:
            {
                robot.textSend("syncdownload");
                rosync_statemachine = ROSYNC_DOWNLOAD_WAIT;
                rosync_state_timer = now;
                rosync_sync_complete = false;
                rosync_download_progress = 0;
            }
            break;
        case ROSYNC_DOWNLOAD_WAIT:
            {
                if (rosync_sync_complete || (now - rosync_state_timer) >= 3000)
                {
                    uint32_t old_checksum = rosync_nvm_cache.checksum;
                    uint32_t new_checksum = crc32_calc((uint8_t*)rosync_nvm_cache, sizeof(roach_rx_nvm_t) - 4);
                    if (old_checksum == new_checksum)
                    {
                        memcpy(&nvm_rx, &rosync_nvm_cache, sizeof(roach_rx_nvm_t));
                        rosync_statemachine = ROSYNC_DONE_SYNCED;
                    }
                    else
                    {
                        if (rosync_retry_cnt > 0)
                        {
                            rosync_retry_cnt -= 1;
                            rosync_statemachine = ROSYNC_DOWNLOAD_START;
                        }
                        else
                        {
                            rosync_statemachine = ROSYNC_DOWNLOAD_FAILED;
                        }
                    }
                }
            }
            break;
        case ROSYNC_UPLOAD:
            {
                if (rosync_upload_lasttime == 0 || (now - rosync_upload_lasttime) >= 200)
                {
                    if (rosync_upload_lasttime == 0)
                    {
                        rosync_upload_idx = 0;
                        rosync_rxitems_cnt = roachnvm_rx_getcnt();
                        rosync_sync_complete = false;
                    }
                    else
                    {
                        rosync_upload_idx += 1;
                    }
                    if (rosync_upload_idx >= rosync_rxitems_cnt) {
                        rosync_statemachine = ROSYNC_UPLOAD_DONE;
                        rosync_sync_complete = true;
                        break;
                    }
                    roach_nvm_gui_desc_t* desc = roach_nvm_rx_getAt(rosync_upload_idx);
                    RoSync_uploadChunk(desc);
                }
            }
            break;
        case ROSYNC_UPLOAD_DONE:
            {
                if ((now - rosync_upload_lasttime) >= 2000)
                {
                    rosync_statemachine = ROSYNC_DISCONNECTED;
                    break;
                }
            }
            break;
    }
}

void RoSync_downloadStart(void)
{
    rosync_statemachine = ROSYNC_DOWNLOAD_START;
    rosync_sync_complete = false;
    rosync_download_progress = 0;
}

void RoSync_downloadChunk(int addr, uint8_t* data, int len)
{
    memcpy(&(rosync_nvm_cache_ptr[addr]), data, len);
    rosync_download_progress = addr + len;
    if (rosync_download_progress >= sizeof(roach_rx_nvm_t)) {
        rosync_sync_complete = true;
    }
}

void RoSync_downloadPrint()
{
    int y = 11;
    oled.setCursor(0, y);
    if (rosync_statemachine == ROSYNC_DOWNLOAD_START)
    {
        oled.print("starting");
    }
    else if (rosync_sync_complete)
    {
        oled.print("done");
    }
    else if (rosync_statemachine == ROSYNC_DOWNLOAD_WAIT)
    {
        int percent = map(rosync_download_progress, 0, sizeof(roach_rx_nvm_t), 0, 100);
        oled.printf("progress %d %%", percent);
        y += 8
        int w = oled.width() - 12;
        int s = map(rosync_download_progress, 0, sizeof(roach_rx_nvm_t), 0, w);
        oled.drawRect(0, y, w, y + 8, 1);
        oled.fillRect(0, y, s, y + 8, 1);
    }
    else if (rosync_statemachine == ROSYNC_DOWNLOAD_FAILED)
    {
        oled.print("error: failed");
    }
    else if (rosync_statemachine == ROSYNC_DISCONNECTED)
    {
        oled.print("error:");
        y += 8
        oled.setCursor(0, y);
        oled.print("disconnected");
    }
    else
    {
        oled.print("error:");
        y += 8
        oled.setCursor(0, y);
        oled.print("unknown state");
    }
}

void RoSync_decodeDownload(const char* s)
{
    int slen = strlen(s);
    int nbytes = (slen - 4) / 2;
    uint8_t* data = malloc(nbytes);
    if (data == NULL) {
        return;
    }
    char hex[5];
    int addr;
    memcpy(hex, s, 4);
    hex[4] = 0;
    addr = strtol(hex, NULL, 16);
    hex[3] = 0;
    int i;
    for (i = 0; i < nbytes; i++) {
        hex[0] = s[4 + (i * 2)];
        hex[1] = s[4 + (i * 2) + 1];
        data[i] = strtol(hex, NULL, 16);
    }
    RoSync_downloadChunk(addr, data, nbytes);
    free(data);
}

void RoSync_uploadChunk(roach_nvm_gui_desc_t* desc)
{
    sprintf(rosync_upload_cache, "ul %s=%d\n", desc->name, roach_nvm_getval(&nvm_rx, desc));
    radio.textSend(rosync_upload_cache);
    rosync_upload_lasttime = now;
}

void RoSync_uploadPrint()
{
    int y = 11;
    oled.setCursor(0, y);
    if (rosync_sync_complete || rosync_statemachine == ROSYNC_UPLOAD_DONE)
    {
        oled.print("done");
    }
    else if (rosync_statemachine == ROSYNC_UPLOAD)
    {
        int percent = map(rosync_upload_idx, 0, rosync_rxitems_cnt, 0, 100);
        oled.print("progress %d %%", percent);
        y += 8
        int w = oled.width() - 12;
        int s = map(rosync_upload_idx, 0, rosync_rxitems_cnt, 0, w);
        oled.drawRect(0, y, w, y + 8, 1);
        oled.fillRect(0, y, s, y + 8, 1);
    }
    else if (rosync_statemachine == ROSYNC_DISCONNECTED)
    {
        oled.print("error:");
        y += 8
        oled.setCursor(0, y);
        oled.print("disconnected");
    }
    else
    {
        oled.print("error:");
        y += 8
        oled.setCursor(0, y);
        oled.print("unknown state");
    }
}
