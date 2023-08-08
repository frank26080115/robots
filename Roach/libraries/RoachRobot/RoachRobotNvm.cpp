#include "RoachRobot.h"
#include "RoachRobotPrivate.h"
#include <RoachUsbMsd.h>

#define ROACHROBOT_STARTUP_FILE_NAME "cfg.txt"

uint32_t nvm_checksum;
uint8_t* nvm_ptr8;

void roachrobot_handleFileLoad(void* cmd, char* argstr, Stream* stream)
{
    if (argstr[0] == 0)
    {
        if (nvm_ptr8 == NULL) {
            stream->println("ERR: NVM structure pointer is null when loading");
        }
        else if (roachrobot_loadSettings(nvm_ptr8)) {
            stream->println("loaded startup file");
        }
        else {
            stream->println("ERR: failed to load startup file");
        }
    }
    else
    {
        if (roachrobot_loadSettingsFile(argstr, nvm_ptr8)) {
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
        if (nvm_ptr8 == NULL) {
            stream->println("ERR: NVM structure pointer is null when saving");
        }
        else if (roachrobot_saveSettings(nvm_ptr8)) {
            stream->println("saved to startup file");
        }
        else {
            stream->println("ERR: failed to save startup file");
        }
    }
    else
    {
        if (roachrobot_saveSettingsToFile(argstr, nvm_ptr8)) {
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
        roachnvm_writetofile(&f, data, cfggroup_drive);
        roachnvm_writetofile(&f, data, cfggroup_weap);
        roachnvm_writetofile(&f, data, cfggroup_sensor);
        f.close();
        return true;
    }
    return false;
}

bool roachrobot_loadSettingsFile(const char* fname, uint8_t* data)
{
    RoachFile f;
    bool suc = f.open(fname);
    if (suc)
    {
        roachnvm_readfromfile(&f, data, cfggroup_drive);
        roachnvm_readfromfile(&f, data, cfggroup_weap);
        roachnvm_readfromfile(&f, data, cfggroup_sensor);
        f.close();
        return true;
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
    roachnvm_setdefaults(data, cfggroup_drive);
    roachnvm_setdefaults(data, cfggroup_weap);
    roachnvm_setdefaults(data, cfggroup_sensor);
}
