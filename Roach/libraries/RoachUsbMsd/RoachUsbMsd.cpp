#include "RoachUsbMsd.h"

#include "SPI.h"
#include "SdFat.h"
#include "Adafruit_SPIFlash.h"
#include "Adafruit_TinyUSB.h"

// for flashTransport definition
#include "flash_config.h"

Adafruit_SPIFlash flash(&flashTransport);

// file system object from SdFat
FatVolume fatfs;

// USB Mass Storage object
Adafruit_USBD_MSC usb_msc;

// Check if flash is formatted
static bool fs_formatted = false;

// Set to true when PC write to flash
static bool fs_changed = true;
static uint32_t fs_change_time = 0;

static int32_t msc_read_cb(uint32_t lba, void* buffer, uint32_t bufsize);
static int32_t msc_write_cb(uint32_t lba, uint8_t* buffer, uint32_t bufsize);
static void msc_flush_cb(void);

void RoachUsbMsd_begin(void)
{
    flash.begin();
    usb_msc.setID("Adafruit", "External Flash", "1.0");
    usb_msc.setReadWriteCallback(msc_read_cb, msc_write_cb, msc_flush_cb);
    usb_msc.setCapacity(flash.size()/512, 512);
    usb_msc.setUnitReady(true);
    usb_msc.begin();
    fs_formatted = fatfs.begin(&flash);
    Serial.begin(115200);
}

void RoachUsbMsd_task(void)
{
    if ( !fs_formatted )
    {
        fs_formatted = fatfs.begin(&flash);
    }
}

bool RoachUsbMsd_isReady(void)
{
    return fs_formatted;
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

static int32_t msc_read_cb(uint32_t lba, void* buffer, uint32_t bufsize)
{
  // Note: SPIFLash Block API: readBlocks/writeBlocks/syncBlocks
  // already include 4K sector caching internally. We don't need to cache it, yahhhh!!
  return flash.readBlocks(lba, (uint8_t*) buffer, bufsize/512) ? bufsize : -1;
}

// Callback invoked when received WRITE10 command.
// Process data in buffer to disk's storage and 
// return number of written bytes (must be multiple of block size)
static int32_t msc_write_cb(uint32_t lba, uint8_t* buffer, uint32_t bufsize)
{
  // Note: SPIFLash Block API: readBlocks/writeBlocks/syncBlocks
  // already include 4K sector caching internally. We don't need to cache it, yahhhh!!
  return flash.writeBlocks(lba, buffer, bufsize/512) ? bufsize : -1;
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
