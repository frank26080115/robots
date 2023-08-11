#include "RoachRobot.h"
#include "RoachRobotPrivate.h"

uint32_t roachrobot_lastUploadTime;
bool roachrobot_downloadRequestedNvm = false;
bool roachrobot_downloadRequestedDesc = false;
uint32_t roachrobot_downloadIdx = 0;

bool roachrobot_syncHandleMsg(radio_binpkt_t*); // returns true if the message was handled
void roachrobot_handleUploadLine(void* cmd, char* argstr, Stream* stream);
void roachrobot_sendChunkNvm(void);
void roachrobot_sendChunkDesc(void);

static radio_binpkt_t tx_pkt;

void roachrobot_syncTask(void)
{
    uint32_t now = millis();
    if (roachrobot_lastUploadTime != 0 && (now - roachrobot_lastUploadTime) >= 500)
    {
        roachrobot_recalcChecksum();
        roachrobot_saveSettings(rosync_nvm);
        roachrobot_lastUploadTime = 0;
    }

    if (radio.textAvail())
    {
        radio_binpkt_t* binpkt = radio.textReadPtr(false);
        if (roachrobot_syncHandleMsg(binpkt)) {
            radio.textReadPtr(true);
        }
    }

    if (roachrobot_downloadRequestedNvm) {
        roachrobot_sendChunkNvm();
    }
    if (roachrobot_downloadRequestedDesc) {
        roachrobot_sendChunkDesc();
    }
}

bool roachrobot_syncHandleMsg(radio_binpkt_t* binpkt)
{
    if (binpkt->addr == 0 && binpkt->len == 0 && binpkt->data[0] == 0)
    {
        switch (binpkt->typecode)
        {
            case ROACHCMD_SYNC_DOWNLOAD_DESC:
                roachrobot_downloadRequestedDesc = true;
                roachrobot_downloadRequestedNvm  = false;
                roachrobot_downloadIdx = 0;
                return true;
            case ROACHCMD_SYNC_DOWNLOAD_CONF:
                roachrobot_downloadRequestedNvm  = true;
                roachrobot_downloadRequestedDesc = false;
                roachrobot_downloadIdx = 0;
                return true;
        }
    }
    else
    {
        switch (binpkt->typecode)
        {
            case ROACHCMD_SYNC_UPLOAD_CONF:
                // incoming string is in binpkt->data
                // format is "%s=%s\n"
                roachrobot_handleUploadLine(NULL, (char*)(binpkt->data), NULL);
                return true;
        }
    }
    return false;
}

void roachrobot_sendChunkNvm(void)
{
    if (radio.textIsDone() == false) {
        return;
    }
    tx_pkt.typecode = ROACHCMD_SYNC_DOWNLOAD_CONF;
    int dlen = rosync_nvm_sz - roachrobot_downloadIdx;
    dlen = dlen > NRFRR_PAYLOAD_SIZE2 ? NRFRR_PAYLOAD_SIZE2 : dlen;
    tx_pkt.len = dlen > 0 ? dlen : 0;
    if (tx_pkt.len > 0)
    {
        memcpy(tx_pkt.data, &(rosync_nvm[roachrobot_downloadIdx]), tx_pkt.len);
        roachrobot_downloadIdx += tx_pkt.len;
    }
    radio.textSendBin(&tx_pkt);
    if (dlen <= 0) {
        roachrobot_downloadRequestedNvm = false;
    }
}

void roachrobot_sendChunkDesc(void)
{
    if (radio.textIsDone() == false) {
        return;
    }
    tx_pkt.typecode = ROACHCMD_SYNC_DOWNLOAD_DESC;
    int dlen = cfg_desc_sz - roachrobot_downloadIdx;
    dlen = dlen > NRFRR_PAYLOAD_SIZE2 ? NRFRR_PAYLOAD_SIZE2 : dlen;
    tx_pkt.len = dlen > 0 ? dlen : 0;
    if (tx_pkt.len > 0)
    {
        uint8_t* ptr = (uint8_t*)cfg_desc;
        memcpy(tx_pkt.data, &(ptr[roachrobot_downloadIdx]), tx_pkt.len);
        roachrobot_downloadIdx += tx_pkt.len;
    }
    radio.textSendBin(&tx_pkt);
    if (dlen <= 0) {
        roachrobot_downloadRequestedDesc = false;
    }
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
    argstr[i] = 0; // this splits the string at the '='

    int j;
    // this loop does trim-end on the left string
    for (j = i - 1; j > 0; j--)
    {
        char c = argstr[j];
        if (c == ' ' || c == '\t') {
            argstr[j] = 0;
        }
        else {
            break;
        }
    }

    // this loop does trim-start on the right string
    for (i += 1; i < slen; i++)
    {
        char c = argstr[i];
        if (c != ' ' && c != '\t') {
            break;
        }
    }
    i++;

    // this loop does trim-end on the right string
    for (j = slen - 1; j > i; j--)
    {
        char c = argstr[j];
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
            argstr[j] = 0;
        }
        else {
            break;
        }
    }

    roachnvm_parseitem(rosync_nvm, cfg_desc, argstr, &(argstr[i]));
    roachrobot_lastUploadTime = millis();
    //roachrobot_recalcChecksum();
}
