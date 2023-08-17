#include "RoachUsbMsd.h"

#include "SPI.h"
#include "SdFat.h"

#include "Adafruit_SPIFlash.h"
#include "Adafruit_TinyUSB.h"

// for flashTransport definition
#include "flash_config.h"

extern Adafruit_USBD_Device TinyUSBDevice;
static void* usbd_mem_cache;

Adafruit_SPIFlash flash(&flashTransport);

// file system object from SdFat
FatVolume fatfs;

// USB Mass Storage object
static Adafruit_USBD_MSC* usb_msc = NULL;

// Check if flash is formatted
static bool fs_formatted = false;

// Set to true when PC write to flash
static bool fs_changed = true;
static uint32_t fs_change_time = 0;

static int32_t msc_read_cb(uint32_t lba, void* buffer, uint32_t bufsize);
static int32_t msc_write_cb(uint32_t lba, uint8_t* buffer, uint32_t bufsize);
static void msc_flush_cb(void);

enum
{
    RUSBMSD_SM_INVISIBLE,
    RUSBMSD_SM_FORCE_DPDM,
    RUSBMSD_SM_DISABLE,
    RUSBMSD_SM_ENABLE,
    RUSBMSD_SM_RECONNECT,
    RUSBMSD_SM_PRESENT,
};

static uint8_t rusbmsd_statemachine = 0;
static uint32_t rusbmsd_disconnect_time = 0;
static bool rusbmsd_wanted = false;
static bool fs_need_reinit = false; // if the computer changed the file system, re-initializing the cache is required
static bool flash_failed;
static uint32_t jedec_id;

bool RoachUsbMsd_begin(void)
{
    Serial.begin(115200);

    flash_failed = (flash.begin() == false);
    jedec_id = flash.getJEDECID();

    fs_formatted = fatfs.begin(&flash);
    fs_need_reinit = false;

    return flash_failed == false;
}

void RoachUsbMsd_actuallyBegin(void)
{
    usb_msc = new Adafruit_USBD_MSC();
    usb_msc->setID("Adafruit", "External Flash", "1.0");
    usb_msc->setReadWriteCallback(msc_read_cb, msc_write_cb, msc_flush_cb);
    usb_msc->setCapacity(flash.size()/512, 512);
    usb_msc->setUnitReady(true);
    usb_msc->begin();
    Serial.begin(115200);
}

void RoachUsbMsd_task(void)
{
    if (flash_failed) {
        return;
    }

    if (rusbmsd_statemachine == RUSBMSD_SM_INVISIBLE)
    {
        // do not check fs_formatted == false here, it will make the system too slow, as it probably failed previously
        if (fs_need_reinit) // if the computer changed the file system, re-initializing the cache is required
        {
            fs_formatted = fatfs.begin(&flash);
            fs_need_reinit = false;
        }
    }
    else if (rusbmsd_statemachine == RUSBMSD_SM_PRESENT)
    {
        if (fs_formatted == false || fs_need_reinit)
        {
            fs_formatted = fatfs.begin(&flash);
            fs_need_reinit = false;
        }
        if (RoachUsbMsd_hasVbus() == false) // unexpected disconnect
        {
            fs_need_reinit = true;
            if (usb_msc != NULL) {
                delete usb_msc;
            }
            rusbmsd_statemachine = RUSBMSD_SM_INVISIBLE;
            rusbmsd_wanted = false;
            if (usbd_mem_cache != NULL) {
                memcpy((void*)(&TinyUSBDevice), usbd_mem_cache, sizeof(Adafruit_USBD_Device));
            }
        }
    }
    else
    {
        uint32_t now = millis();
        if (rusbmsd_statemachine == RUSBMSD_SM_FORCE_DPDM)
        {
            if ((now - rusbmsd_disconnect_time) >= 500)
            {
                rusbmsd_disconnect_time = now;
                NRF_USBD->ENABLE = 0;
                rusbmsd_statemachine = RUSBMSD_SM_DISABLE;
            }
        }
        else if (rusbmsd_statemachine == RUSBMSD_SM_DISABLE)
        {
            if ((now - rusbmsd_disconnect_time) >= 500)
            {
                rusbmsd_disconnect_time = now;
                if (rusbmsd_wanted) {
                    RoachUsbMsd_actuallyBegin();
                }
                else {
                    if (usbd_mem_cache != NULL) {
                        memcpy((void*)(&TinyUSBDevice), usbd_mem_cache, sizeof(Adafruit_USBD_Device));
                    }
                }
                NRF_USBD->ENABLE = 1;
                NRF_USBD->USBPULLUP = 1;
                rusbmsd_statemachine = RUSBMSD_SM_ENABLE;
            }
        }
        else if (rusbmsd_statemachine == RUSBMSD_SM_ENABLE)
        {
            if ((now - rusbmsd_disconnect_time) >= 500)
            {
                rusbmsd_disconnect_time = now;
                NRF_USBD->TASKS_DPDMNODRIVE = 1;
                rusbmsd_statemachine = RUSBMSD_SM_RECONNECT;
            }
        }
        else if (rusbmsd_statemachine == RUSBMSD_SM_RECONNECT)
        {
            if ((now - rusbmsd_disconnect_time) >= 500)
            {
                rusbmsd_disconnect_time = now;
                if (rusbmsd_wanted) {
                    rusbmsd_statemachine = RUSBMSD_SM_PRESENT;
                }
                else {
                    rusbmsd_statemachine = RUSBMSD_SM_INVISIBLE;
                }
            }
        }
    }
}

void RoachUsbMsd_presentUsbMsd(void)
{
    if (flash_failed) {
        return;
    }
    if (RoachUsbMsd_hasVbus() == false) {
        return;
    }
    if (RoachUsbMsd_isUsbPresented()) {
        return;
    }
    if (usbd_mem_cache == NULL) {
        usbd_mem_cache = malloc(sizeof(Adafruit_USBD_Device));
        memcpy(usbd_mem_cache, (void*)(&TinyUSBDevice), sizeof(Adafruit_USBD_Device));
    }
    rusbmsd_wanted = true; // signal to state machine
    NRF_USBD->DPDMVALUE       = 0;
    NRF_USBD->TASKS_DPDMDRIVE = 1;
    NRF_USBD->USBPULLUP       = 0;
    rusbmsd_statemachine = RUSBMSD_SM_FORCE_DPDM;
    rusbmsd_disconnect_time = millis();
}

void RoachUsbMsd_unpresent(void)
{
    rusbmsd_wanted = false;
    if (RoachUsbMsd_hasVbus() == false) {
        // disconnections are handled by state machine task
        return;
    }
    if (RoachUsbMsd_isUsbPresented() == false) {
        return;
    }
    if (usb_msc != NULL) {
        delete usb_msc;
    }
    NRF_USBD->DPDMVALUE       = 0;
    NRF_USBD->TASKS_DPDMDRIVE = 1;
    NRF_USBD->USBPULLUP       = 0;
    rusbmsd_statemachine = RUSBMSD_SM_FORCE_DPDM;
    rusbmsd_disconnect_time = millis();
}

bool RoachUsbMsd_isReady(void)
{
    return fs_formatted;
}

bool RoachUsbMsd_isUsbPresented(void)
{
    return fs_formatted && rusbmsd_statemachine == RUSBMSD_SM_PRESENT;
}

bool RoachUsbMsd_canSave(void)
{
    if (fs_formatted == false || fs_need_reinit) {
        return false;
    }
    if (RoachUsbMsd_hasVbus() == false) {
        return true;
    }
    if (RoachUsbMsd_isUsbPresented()) {
        return false;
    }
    return true;
}

bool RoachUsbMsd_hasChange(bool clr)
{
    bool x = fs_changed && (millis() - fs_change_time) >= 1000;
    if (x && clr) {
        fs_changed = false;
        fs_change_time = 0;
    }
    return x;
}

bool RoachUsbMsd_hasVbus(void)
{
    return (NRF_POWER->USBREGSTATUS & POWER_USBREGSTATUS_VBUSDETECT_Msk) != 0;
}

uint64_t RoachUsbMsd_getFreeSpace(void)
{
    uint64_t x;
    x = fatfs.freeClusterCount();
    x *= fatfs.sectorsPerCluster()/2;
    return x;
}

uint32_t RoachUsbMsd_getFlashSize(void)
{
    return flash.size();
}

uint32_t RoachUsbMsd_getJedecId(void)
{
    return jedec_id;
}

static int32_t msc_read_cb(uint32_t lba, void* buffer, uint32_t bufsize)
{
  // Note: SPIFLash Block API: readBlocks/writeBlocks/syncBlocks
  // already include 4K sector caching internally. We don't need to cache it, yahhhh!!
  int32_t x = flash.readBlocks(lba, (uint8_t*) buffer, bufsize/512) ? bufsize : -1;
  fs_change_time = millis();
  return x;
}

// Callback invoked when received WRITE10 command.
// Process data in buffer to disk's storage and 
// return number of written bytes (must be multiple of block size)
static int32_t msc_write_cb(uint32_t lba, uint8_t* buffer, uint32_t bufsize)
{
  // Note: SPIFLash Block API: readBlocks/writeBlocks/syncBlocks
  // already include 4K sector caching internally. We don't need to cache it, yahhhh!!
  int32_t x = flash.writeBlocks(lba, buffer, bufsize/512) ? bufsize : -1;
  fs_change_time = millis();
  return x;
}

// Callback invoked when WRITE10 command is completed (status received and accepted by host).
// used to flush any pending cache.
static void msc_flush_cb(void)
{
  // sync with flash
  flash.syncBlocks();

  // clear file system's cache to force refresh
  fatfs.cacheClear();

  // indicate change
  fs_changed = true;
  fs_change_time = millis();
}

uint32_t RoachUsbMsd_lastActivityTime(void)
{
    return fs_change_time;
}
