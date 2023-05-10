#include "RoachRobot.h"
#include <RoachLib.h>
#include <nRF52RcRadio.h>

extern nRF52RcRadio radio;
extern roach_telem_pkt_t telem_pkt;
extern roach_nvm_gui_desc_t** cfggroup_rxall;

uint32_t roachrobot_lastUploadTime;

void roachrobot_init(void)
{
    roachnvm_buildrxcfggroup();
}

void roachrobot_telemTask(void)
{
    telem_pkt.rssi = radio.get_rssi();
    telem_pkt.checksum = nvm_rx.checksum;
}

void roachrobot_syncTask(void)
{
    uint32_t now = millis();
    if (roachrobot_lastUploadTime != 0 && (now - roachrobot_lastUploadTime) >= 500)
    {
        roachrobot_recalcChecksum();
        roachrobot_saveSettings();
        roachrobot_lastUploadTime = 0;
    }

    if (roachrobot_downloadRequested && radio.textIsDone())
    {
        roachrobot_downloadChunk();
    }
}

void roachrobot_downloadChunk(void)
{
    uint8_t* ptr = (uint8_t*)&nvm_rx;
    char tmpbuf[NRFRR_PAYLOAD_SIZE];
    int i = 0;
    i = sprintf(tmpbuf, "D %04X ", roachrobot_downloadIdx);
    int j;
    for (j = roachrobot_downloadIdx; j < sizeof(roach_rx_nvm_t) && i < NRFRR_PAYLOAD_SIZE - 1; j++) {
        uint8_t d = ptr[j];
        i += sprintf(&tmpbuf[i], "%02X", d);
    }
    radio.textSend(tmpbuf);
    roachrobot_downloadIdx = j;
    if (roachrobot_downloadIdx >= sizeof(roach_rx_nvm_t)) {
        roachrobot_downloadRequested = false;
    }
}

void roachrobot_pipeCmdLine(void)
{
    if (radio.textAvail()) {
        cmdline.sideinput_writes(radio.textReadPtr(true));
    }
    cmdline.task();
}

void roachrobot_recalcChecksum(void)
{
    nvm_rx.magic = nvm_rf.salt;
    nvm_rx.checksum = roach_crcCalc((uint8_t*)&nvm_rx, sizeof(roach_rx_nvm_t) - 4, NULL);
}

void roachrobot_handleDownloadRequest(void* cmd, char* argstr, Stream* stream)
{
    roachrobot_downloadRequested = true;
    roachrobot_downloadIdx = 0;
}

void roachrobot_handleUploadLine(void* cmd, char* argstr, Stream* stream)
{
    int i;
    int slen = strlen(argstr);
    for (i = 0; i < slen; i++)
    {
        char c = argstr[i];
        if (c == ':' || c == '=') {
            break;
        }
    }
    argstr[i] = 0;
    int j;
    for (j = i - 1; j > 0; i++)
    {
        char c = argstr[i];
        if (c != ' ' && c != '\t') {
            argstr[j] = 0;
        }
        else {
            break;
        }
    }
    for (i += 1; i < strlen; i++)
    {
        char c = argstr[i];
        if (c != ' ' && c != '\t') {
            break;
        }
    }
    i++;
    roachnvm_parseitem(&nvm_rx, cfggroup_drive, argstr, &(argstr[i]));
    roachnvm_parseitem(&nvm_rx, cfggroup_weap,  argstr, &(argstr[i]));
    roachnvm_parseitem(&nvm_rx, cfggroup_imu,   argstr, &(argstr[i]));
    roachrobot_lastUploadTime = millis();
    //roachrobot_recalcChecksum();
}

void roachrobot_handleFileLoad(void* cmd, char* argstr, Stream* stream)
{
    if (argstr[0] == 0)
    {
        if (roachrobot_loadSettings()) {
            stream->println("loaded startup file");
        }
        else {
            stream->println("ERR: failed to load startup file");
        }
    }
    else
    {
        if (roachrobot_loadSettingsFile(argstr)) {
            stream->printf("loaded file %s\r\n", argstr);
        }
        else {
            stream->printf("ERR: failed to load file %s\r\n", argstr);
        }
    }
    roachrobot_recalcChecksum();
}

void roachrobot_handleFileSave(void* cmd, char* argstr, Stream* stream)
{
    if (RoachUsbMsd_isUsbPresented()) {
        stream->println("ERR: unable to save while USB is MSD");
        return;
    }
    if (argstr[0] == 0)
    {
        if (roachrobot_saveSettings()) {
            stream->println("saved to startup file");
        }
        else {
            stream->println("ERR: failed to save startup file");
        }
    }
    else
    {
        if (roachrobot_saveSettingsToFile(argstr)) {
            stream->printf("saved to %s\r\n", argstr);
        }
        else {
            stream->printf("ERR: failed to save  %s\r\n", argstr);
        }
    }
}

bool roachrobot_saveSettingsToFile(const char* fname)
{
    RoachFile f;
    bool suc = f.open(fname, O_RDWR | O_CREAT);
    if (suc)
    {
        roachnvm_writetofile(&f, (uint8_t*)&nvm_rx, cfggroup_drive);
        roachnvm_writetofile(&f, (uint8_t*)&nvm_rx, cfggroup_weap);
        roachnvm_writetofile(&f, (uint8_t*)&nvm_rx, cfggroup_imu);
        f.close();
        return true;
    }
    return false
}

bool roachrobot_loadSettingsFile(const char* fname)
{
    RoachFile f;
    bool suc = f.open(fname);
    if (suc)
    {
        roachnvm_readfromfile(&f, (uint8_t*)&nvm_rx, cfggroup_drive);
        roachnvm_readfromfile(&f, (uint8_t*)&nvm_rx, cfggroup_weap);
        roachnvm_readfromfile(&f, (uint8_t*)&nvm_rx, cfggroup_imu);
        f.close();
        return true;
    }
    return false
}

bool roachrobot_loadSettings(void)
{
    return roachrobot_loadSettingsFile(ROACHROBOT_STARTUP_FILE_NAME);
}

bool roachrobot_saveSettings(void)
{
    return roachrobot_saveSettingsToFile(ROACHROBOT_STARTUP_FILE_NAME);
}
