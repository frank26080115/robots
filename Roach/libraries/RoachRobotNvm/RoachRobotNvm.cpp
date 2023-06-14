#include "RoachRobotNvm.h"

static uint8_t robotnvm_statemachine = RONVM_SM_IDLE;

static roach_nvm_gui_desc_t* robotnvm_desc;
uint32_t robotnvm_desc_chksum = 0;
static uint32_t desc_size = 0;

static void* robotnvm_ptr;
static uint32_t robotnvm_sz;
uint32_t robotnvm_chksum;

bool robotnvm_hasUpdate = false;

bool robotnvm_init(roach_nvm_gui_desc_t* desc, void* nvm_ptr, uint32_t nvm_sz)
{
    robotnvm_ptr = nvm_ptr;
    robotnvm_sz = nvm_sz;
    robotnvm_desc = desc;
    desc_size = roachnvm_getDescCnt(desc);

    robotnvm_desc_chksum = roachnvm_getDescCrc(robotnvm_desc);
    robotnvm_genDescFile(RONVM_DEFAULT_DESC_FILENAME, robotnvm_desc, false);
    if (robotnvm_loadFromFile(RONVM_DEFAULT_CFG_FILENAME) == false)
    {
        robotnvm_setDefaults();
        robotnvm_saveToFile(RONVM_DEFAULT_CFG_FILENAME);
    }
    //robotnvm_chksum = roachnvm_getConfCrc(robotnvm_ptr, robotnvm_sz); // already done by previous functions
    robotnvm_hasUpdate = true;
}

bool robotnvm_task(nRF52RcRadio* radio)
{
    switch (robotnvm_statemachine)
    {
        case RONVM_SM_IDLE:
            break;
        case RONVM_SM_SENDINGDESC:
            if (radio->textIsDone())
            {
                robotnvm_sendDescFileChunk(radio);
            }
            break;
        case RONVM_SM_SENDINGCONF:
            if (radio->textIsDone())
            {
                robotnvm_sendConfChunk(radio);
            }
            break;
    }

    if (radio->textAvail())
    {
        radio_binpkt_t pkt;
        radio->textReadBin(&pkt, false);
        bool suc = robotnvm_handlePkt(&pkt);
        if (suc) {
            radio->textReadBin(&pkt, true);
        }
    }
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
    roachnvm_setdefaults((uint8_t*)robotnvm_ptr, robotnvm_desc);
    robotnvm_chksum = roachnvm_getConfCrc(robotnvm_ptr, robotnvm_sz);
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
    roachnvm_writetofile(&f, (uint8_t*)robotnvm_ptr, robotnvm_desc);
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
    roachnvm_readfromfile(&f, (uint8_t*)robotnvm_ptr, robotnvm_desc);
    f.close();
    robotnvm_chksum = roachnvm_getConfCrc(robotnvm_ptr, robotnvm_sz);
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
    uint8_t* ptr = (uint8_t*)robotnvm_desc;
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
    int32_t len = robotnvm_sz - send_idx;
    len = len > NRFRR_PAYLOAD_SIZE2 ? NRFRR_PAYLOAD_SIZE2 : len;
    pkt.length = len;
    uint8_t* ptr = (uint8_t*)robotnvm_ptr;
    memcpy(pkt.data, &ptr[send_idx], len);
    send_idx += len;

    if (pkt.addr == 0 && robotnvm_sz >= NRFRR_PAYLOAD_SIZE2) {
        pkt.len = robotnvm_sz;
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
            if (roachnvm_parsecmd(robotnvm_ptr, robotnvm_desc, pkt->data)) {
                robotnvm_chksum = roachnvm_getConfCrc(robotnvm_ptr, robotnvm_sz);
                robotnvm_hasUpdate = true;
            }
            return true;
    }
}
