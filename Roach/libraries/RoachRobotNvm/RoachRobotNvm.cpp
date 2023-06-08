#include "RoachRobotNvm.h"

static roach_nvm_gui_desc_t* robot_nvm_desc;
static uint32_t robot_nvm_desc_chksum = 0;
static uint32_t desc_size = 0;

static void* robot_nvm_ptr;
static uint32_t robot_nvm_sz;
static uint32_t robot_nvm_chksum;

bool robotnvm_init(roach_nvm_gui_desc_t* desc, void* nvm_ptr, uint32_t nvm_sz)
{
    robot_nvm_ptr = nvm_ptr;
    robot_nvm_sz = nvm_sz;
    robot_nvm_desc = desc;
    desc_size = roachnvm_getDescCnt(desc);

    robot_nvm_desc_chksum = roachnvm_getDescCrc(robot_nvm_desc);
    robotnvm_genDescFile(RONVM_DEFAULT_DESC_FILENAME, robot_nvm_desc, false);
    if (robotnvm_loadFromFile(RONVM_DEFAULT_CFG_FILENAME))
    {
        robotnvm_setDefaults();
        robotnvm_saveToFile(RONVM_DEFAULT_CFG_FILENAME);
    }
    robot_nvm_chksum = roachnvm_getConfCrc(robot_nvm_ptr, robot_nvm_sz);
}

bool robotnvm_genDescFile(const char* fname, roach_nvm_gui_desc_t* desc_tbl, bool force)
{
    RoachFile f;
    bool suc;
    suc = f.open(fname, O_RDONLY);
    if (suc && force == false)
    {
        return true;
    }
    suc = f.open(fname, O_RDWR | O_CREAT | O_TRUNC);
    if (suc == false)
    {
        return false;
    }
    f.seek(0);
    uint8_t* data_ptr = (uint8_t*)desc_tbl;
    uint32_t data_sz = roachnvm_getDescCnt(desc_tbl) * sizeof(roach_nvm_gui_desc_t);
    f.write(data_ptr, data_sz);
    f.close();
    return true;
}

void robotnvm_setDefaults(void)
{
    roachnvm_setdefaults((uint8_t*)robot_nvm_ptr, robot_nvm_desc);
}

bool robotnvm_saveToFile(const char* fname)
{
    RoachFile f;
    bool suc;
    suc = f.open(fname, O_RDWR | O_CREAT | O_TRUNC);
    if (suc == false)
    {
        return false;
    }
    f.seek(0);
    roachnvm_writetofile(&f, (uint8_t*)robot_nvm_ptr, robot_nvm_desc);
    f.close();
    return true;
}

bool robotnvm_loadFromFile(const char* fname)
{
    RoachFile f;
    bool suc;
    suc = f.open(fname, O_RDONLY);
    if (suc == false)
    {
        return false;
    }
    f.seek(0);
    roachnvm_readfromfile(&f, (uint8_t*)robot_nvm_ptr, robot_nvm_desc);
    f.close();
    return true;
}

void robotnvm_sendDescFileStart(void)
{
    robotnvm_statemachine = RONVM_SM_SENDINGDESC;
    send_idx = 0;
}

void robotnvm_sendDescFileChunk(nRF52RcRadio* radio)
{
    radio_binpkt_t pkt;
    pkt.typecode = ROACHCMD_SYNC_DOWNLOAD_DESC;
    pkt.addr = send_idx;
    int32_t len = desc_size - send_idx;
    len = len > NRFRR_PAYLOAD_SIZE2 ? NRFRR_PAYLOAD_SIZE2 : len;
    pkt.length = len;
    uint8_t* ptr = (uint8_t*)robot_nvm_desc;
    memcpy(pkt.data, &ptr[send_idx], len);
    send_idx += len;

    if (pkt.addr == 0 && desc_size >= NRFRR_PAYLOAD_SIZE2) {
        pkt.len = desc_size;
    }

    radio->textSendBin(&pkt);
    if (len == 0) {
        robotnvm_statemachine = RONVM_SM_IDLE;
    }
}

void robotnvm_sendConfStart(void)
{
    robotnvm_statemachine = RONVM_SM_SENDINGCONF;
    send_idx = 0;
}

void robotnvm_sendConfChunk(nRF52RcRadio* radio)
{
    radio_binpkt_t pkt;
    pkt.typecode = ROACHCMD_SYNC_DOWNLOAD_CONF;
    pkt.addr = send_idx;
    int32_t len = robot_nvm_sz - send_idx;
    len = len > NRFRR_PAYLOAD_SIZE2 ? NRFRR_PAYLOAD_SIZE2 : len;
    pkt.length = len;
    uint8_t* ptr = (uint8_t*)robot_nvm_ptr;
    memcpy(pkt.data, &ptr[send_idx], len);
    send_idx += len;

    if (pkt.addr == 0 && robot_nvm_sz >= NRFRR_PAYLOAD_SIZE2) {
        pkt.len = robot_nvm_sz;
    }

    radio->textSendBin(&pkt);
    if (len == 0) {
        robotnvm_statemachine = RONVM_SM_IDLE;
    }
}

bool robotnvm_handlePkt(radio_binpkt_t* pkt)
{
    switch (pkt->typecode)
    {
        case ROACHCMD_SYNC_DOWNLOAD_DESC:
            robotnvm_sendDescFileStart();
            return true;
        case ROACHCMD_SYNC_DOWNLOAD_CONF:
            robotnvm_sendConfStart();
            return true;
        case ROACHCMD_SYNC_UPLOAD_CONF:
            if (roachnvm_parsecmd(robot_nvm_ptr, robot_nvm_desc, pkt->data)) {
                robot_nvm_chksum = roachnvm_getConfCrc(robot_nvm_ptr, robot_nvm_sz);
            }
            return true;
    }
}
