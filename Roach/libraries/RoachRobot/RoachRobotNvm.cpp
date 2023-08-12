#include "RoachRobot.h"
#include "RoachRobotPrivate.h"
#include <RoachUsbMsd.h>
#include <RoachLib.h>

#define ROACHROBOT_STARTUP_FILE_NAME "cfg.txt"

roach_rf_nvm_t nvm_rf;

uint32_t rosync_checksum_nvm  = 0;
uint32_t rosync_checksum_desc = 0;
uint8_t* rosync_nvm = NULL;
uint32_t rosync_nvm_sz = 0;
roach_nvm_gui_desc_t* cfg_desc;
uint32_t cfg_desc_sz = 0;

void roachrobot_handleFileLoad(void* cmd, char* argstr, Stream* stream)
{
    if (argstr[0] == 0)
    {
        if (rosync_nvm == NULL) {
            stream->println("ERR: NVM structure pointer is null when loading");
        }
        else if (roachrobot_loadSettings(rosync_nvm)) {
            stream->println("loaded startup file");
        }
        else {
            stream->println("ERR: failed to load startup file");
        }
    }
    else
    {
        if (roachrobot_loadSettingsFile(argstr, rosync_nvm)) {
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
        if (rosync_nvm == NULL) {
            stream->println("ERR: NVM structure pointer is null when saving");
        }
        else if (roachrobot_saveSettings(rosync_nvm)) {
            stream->println("saved to startup file");
        }
        else {
            stream->println("ERR: failed to save startup file");
        }
    }
    else
    {
        if (roachrobot_saveSettingsToFile(argstr, rosync_nvm)) {
            stream->printf("saved to %s\r\n", argstr);
        }
        else {
            stream->printf("ERR: failed to save  %s\r\n", argstr);
        }
    }
}

bool roachrobot_saveSettingsToFile(const char* fname, uint8_t* data)
{
    RoachFile f;
    bool suc = f.open(fname, O_RDWR | O_CREAT);
    if (suc)
    {
        roachnvm_writetofile(&f, (uint8_t*)&nvm_rf, cfgdesc_rf);
        roachnvm_writetofile(&f, data, cfg_desc);
        f.close();
        return true;
    }
    else
    {
        debug_printf("ERR: roachrobot_saveSettingsToFile failed to open file\r\n");
    }
    return false;
}

bool roachrobot_loadSettingsFile(const char* fname, uint8_t* data)
{
    RoachFile f;
    bool suc = f.open(fname);
    if (suc)
    {
        roachnvm_readfromfile(&f, (uint8_t*)&nvm_rf, cfgdesc_rf);
        roachnvm_readfromfile(&f, data, cfg_desc);
        f.close();
        return true;
    }
    else
    {
        debug_printf("ERR: roachrobot_loadSettingsFile failed to open file\r\n");
    }
    return false;
}

bool roachrobot_loadSettings(uint8_t* data)
{
    return roachrobot_loadSettingsFile(ROACHROBOT_STARTUP_FILE_NAME, data);
}

bool roachrobot_saveSettings(uint8_t* data)
{
    return roachrobot_saveSettingsToFile(ROACHROBOT_STARTUP_FILE_NAME, data);
}

void roachrobot_defaultSettings(uint8_t* data)
{
    roachnvm_setdefaults(data, cfg_desc);
}

uint32_t roachrobot_recalcChecksum(void)
{
    rosync_checksum_nvm = roachnvm_getConfCrc(rosync_nvm, cfg_desc);
    return rosync_checksum_nvm;
}

uint32_t roachrobot_calcDescChecksum(void)
{
    rosync_checksum_desc = roach_crcCalc((uint8_t*)cfg_desc, cfg_desc_sz, NULL);
    return rosync_checksum_desc;
}

bool roachrobot_generateDescFile(void)
{
    RoachFile f;
    bool suc = f.open("cfg_desc.bin", O_RDWR | O_CREAT);
    if (suc)
    {
        f.write(cfg_desc, cfg_desc_sz);
        f.close();
        return true;
    }
    return false;
}
