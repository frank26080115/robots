#include "RoachRobot.h"
#include "RoachRobotPrivate.h"

uint32_t roachrobot_lastUploadTime;

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
