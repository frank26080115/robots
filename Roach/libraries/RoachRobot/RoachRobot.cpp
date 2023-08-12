#include "RoachRobot.h"
#include "RoachRobotPrivate.h"
#include <RoachLib.h>
#include <nRF52RcRadio.h>
#include <RoachCmdLine.h>

extern RoachCmdLine cmdline;

nRF52RcRadio radio = nRF52RcRadio(false);
roach_ctrl_pkt_t  rx_pkt    = {0};
roach_telem_pkt_t telem_pkt = {0};

void roachrobot_init(uint8_t* nvm_ptr, uint32_t nvm_size, roach_nvm_gui_desc_t* desc_ptr, uint32_t desc_size)
{
    rosync_nvm    = nvm_ptr;
    rosync_nvm_sz = nvm_size;
    cfg_desc      = desc_ptr;
    cfg_desc_sz   = desc_size;
    roachrobot_recalcChecksum();
    roachrobot_calcDescChecksum();

    radio.begin();
    radio.config(nvm_rf.chan_map, nvm_rf.uid, nvm_rf.salt);
}

void roachrobot_telemTask(uint32_t now)
{
    telem_pkt.timestamp   = now / 10;
    telem_pkt.loss_rate   = radio.stats_rate.loss;
    telem_pkt.rssi        = radio.getRssi();
    telem_pkt.chksum_nvm  = rosync_checksum_nvm;
    telem_pkt.chksum_desc = rosync_checksum_desc;
    radio.send((uint8_t*)&telem_pkt); // radio is in RX mode, the telemetry will only actually be sent when requested
}

void roachrobot_pipeCmdLine(void)
{
    if (radio.textAvail())
    {
        radio_binpkt_t* binpkt = radio.textReadPtr(false);
        if (binpkt->typecode == ROACHCMD_TEXT) {
            cmdline.sideinput_writes((const char*)binpkt->data);
            radio.textReadPtr(true);
            cmdline.task();
        }
    }
}
