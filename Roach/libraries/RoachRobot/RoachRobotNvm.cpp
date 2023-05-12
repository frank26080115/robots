#include "RoachRobot.h"
#include "RoachRobotPrivate.h"

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
