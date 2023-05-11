#include "RoachRobot.h"
#include "RoachRobotPrivate.h"
#include <RoachLib.h>
#include <nRF52RcRadio.h>

void roachrobot_init(void)
{
    roachnvm_buildrxcfggroup();
}

void roachrobot_telemTask(void)
{
    telem_pkt.rssi = radio.get_rssi();
    telem_pkt.checksum = nvm_rx.checksum;
}

void roachrobot_pipeCmdLine(void)
{
    if (radio.textAvail()) {
        cmdline.sideinput_writes(radio.textReadPtr(true));
    }
    cmdline.task();
}
