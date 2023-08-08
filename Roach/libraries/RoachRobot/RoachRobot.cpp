#include "RoachRobot.h"
#include "RoachRobotPrivate.h"
#include <RoachLib.h>
#include <nRF52RcRadio.h>
#include <RoachCmdLine.h>

extern RoachCmdLine cmdline;

void roachrobot_init(void)
{
    roachnvm_buildrxcfggroup();
}

void roachrobot_telemTask(void)
{
    telem_pkt.rssi = radio.getRssi();
    telem_pkt.chksum_nvm = nvm_checksum;
}

void roachrobot_pipeCmdLine(void)
{
    if (radio.textAvail()) {
        cmdline.sideinput_writes((const char*)radio.textReadPtr(true));
    }
    cmdline.task();
}
