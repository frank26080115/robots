#if defined(ESP32)
#include <SPIFFS.h>
#elif defined(NRF52840_XXAA)
#include <Adafruit_LittleFS.h>
#include <InternalFileSystem.h>
#endif

roach_nvm_gui_desc_t cfggroup_ctrler[] = {
    { ((uint32_t)(&(nvm_tx.heading_multiplier      )) - (uint32_t)(&nvm_tx)), "H scale"     , "s32x10" ,                     90,                 INT_MIN, INT_MAX               , 1, },
    { ((uint32_t)(&(nvm_tx.cross_mix               )) - (uint32_t)(&nvm_tx)), "X mix"       , "s32x10" ,                      0,                 INT_MIN, INT_MAX               , 1, },
    { ((uint32_t)(&(nvm_tx.pot_throttle.center     )) - (uint32_t)(&nvm_tx)), "T center"    , "s16"    ,          ROACH_ADC_MID,                       0, ROACH_ADC_MAX         , 1, },
    { ((uint32_t)(&(nvm_tx.pot_throttle.deadzone   )) - (uint32_t)(&nvm_tx)), "T deadzone"  , "s16"    ,        ROACH_ADC_NOISE,                       0, ROACH_ADC_MAX / 32    , 1, },
    { ((uint32_t)(&(nvm_tx.pot_throttle.boundary   )) - (uint32_t)(&nvm_tx)), "T boundary"  , "s16"    ,        ROACH_ADC_NOISE,                       0, ROACH_ADC_MAX / 32    , 1, },
    { ((uint32_t)(&(nvm_tx.pot_throttle.scale      )) - (uint32_t)(&nvm_tx)), "T scale"     , "s16x10" , ROACH_SCALE_MULTIPLIER,                       0, INT_MAX               , 1, },
    { ((uint32_t)(&(nvm_tx.pot_throttle.limit_min  )) - (uint32_t)(&nvm_tx)), "T lim min"   , "s16"    ,                      0,                       0, ROACH_ADC_MAX         , 1, },
    { ((uint32_t)(&(nvm_tx.pot_throttle.limit_max  )) - (uint32_t)(&nvm_tx)), "T lim max"   , "s16"    ,          ROACH_ADC_MAX,                       0, ROACH_ADC_MAX         , 1, },
    { ((uint32_t)(&(nvm_tx.pot_throttle.expo       )) - (uint32_t)(&nvm_tx)), "T expo"      , "s16x10" ,                      0, -ROACH_SCALE_MULTIPLIER, ROACH_SCALE_MULTIPLIER, 1, },
    { ((uint32_t)(&(nvm_tx.pot_throttle.filter     )) - (uint32_t)(&nvm_tx)), "T filter"    , "s16x10" ,   ROACH_FILTER_DEFAULT,                       0, ROACH_SCALE_MULTIPLIER, 1, },
    { ((uint32_t)(&(nvm_tx.pot_steering.center     )) - (uint32_t)(&nvm_tx)), "S center"    , "s16"    ,          ROACH_ADC_MID,                       0, ROACH_ADC_MAX         , 1, },
    { ((uint32_t)(&(nvm_tx.pot_steering.deadzone   )) - (uint32_t)(&nvm_tx)), "S deadzone"  , "s16"    ,        ROACH_ADC_NOISE,                       0, ROACH_ADC_MAX / 32    , 1, },
    { ((uint32_t)(&(nvm_tx.pot_steering.boundary   )) - (uint32_t)(&nvm_tx)), "S boundary"  , "s16"    ,        ROACH_ADC_NOISE,                       0, ROACH_ADC_MAX / 32    , 1, },
    { ((uint32_t)(&(nvm_tx.pot_steering.scale      )) - (uint32_t)(&nvm_tx)), "S scale"     , "s16x10" , ROACH_SCALE_MULTIPLIER,                       0, INT_MAX               , 1, },
    { ((uint32_t)(&(nvm_tx.pot_steering.limit_min  )) - (uint32_t)(&nvm_tx)), "S lim min"   , "s16"    ,                      0,                       0, ROACH_ADC_MAX         , 1, },
    { ((uint32_t)(&(nvm_tx.pot_steering.limit_max  )) - (uint32_t)(&nvm_tx)), "S lim max"   , "s16"    ,          ROACH_ADC_MAX,                       0, ROACH_ADC_MAX         , 1, },
    { ((uint32_t)(&(nvm_tx.pot_steering.expo       )) - (uint32_t)(&nvm_tx)), "S expo"      , "s16x10" ,                      0, -ROACH_SCALE_MULTIPLIER, ROACH_SCALE_MULTIPLIER, 1, },
    { ((uint32_t)(&(nvm_tx.pot_steering.filter     )) - (uint32_t)(&nvm_tx)), "S filter"    , "s16x10" ,   ROACH_FILTER_DEFAULT,                       0, ROACH_SCALE_MULTIPLIER, 1, },
    { ((uint32_t)(&(nvm_tx.pot_weapon  .center     )) - (uint32_t)(&nvm_tx)), "WC center"   , "s16"    ,                      0,                       0, ROACH_ADC_MAX         , 1, },
    { ((uint32_t)(&(nvm_tx.pot_weapon  .deadzone   )) - (uint32_t)(&nvm_tx)), "WC deadzone" , "s16"    ,        ROACH_ADC_NOISE,                       0, ROACH_ADC_MAX / 32    , 1, },
    { ((uint32_t)(&(nvm_tx.pot_weapon  .boundary   )) - (uint32_t)(&nvm_tx)), "WC boundary" , "s16"    ,        ROACH_ADC_NOISE,                       0, ROACH_ADC_MAX / 32    , 1, },
    { ((uint32_t)(&(nvm_tx.pot_weapon  .scale      )) - (uint32_t)(&nvm_tx)), "WC scale"    , "s16x10" , ROACH_SCALE_MULTIPLIER,                       0, INT_MAX               , 1, },
    { ((uint32_t)(&(nvm_tx.pot_weapon  .limit_min  )) - (uint32_t)(&nvm_tx)), "WC lim min"  , "s16"    ,                      0,                       0, ROACH_ADC_MAX         , 1, },
    { ((uint32_t)(&(nvm_tx.pot_weapon  .limit_max  )) - (uint32_t)(&nvm_tx)), "WC lim max"  , "s16"    ,          ROACH_ADC_MAX,                       0, ROACH_ADC_MAX         , 1, },
    { ((uint32_t)(&(nvm_tx.pot_weapon  .expo       )) - (uint32_t)(&nvm_tx)), "WC expo"     , "s16x10" ,                      0, -ROACH_SCALE_MULTIPLIER, ROACH_SCALE_MULTIPLIER, 1, },
    { ((uint32_t)(&(nvm_tx.pot_weapon  .filter     )) - (uint32_t)(&nvm_tx)), "WC filter"   , "s16x10" ,   ROACH_FILTER_DEFAULT,                       0, ROACH_SCALE_MULTIPLIER, 1, },
    { ((uint32_t)(&(nvm_tx.pot_aux     .center     )) - (uint32_t)(&nvm_tx)), "POT center"  , "s16"    ,                      0,                       0, ROACH_ADC_MAX         , 1, },
    { ((uint32_t)(&(nvm_tx.pot_aux     .deadzone   )) - (uint32_t)(&nvm_tx)), "POT deadzone", "s16"    ,        ROACH_ADC_NOISE,                       0, ROACH_ADC_MAX / 32    , 1, },
    { ((uint32_t)(&(nvm_tx.pot_aux     .boundary   )) - (uint32_t)(&nvm_tx)), "POT boundary", "s16"    ,        ROACH_ADC_NOISE,                       0, ROACH_ADC_MAX / 32    , 1, },
    { ((uint32_t)(&(nvm_tx.pot_aux     .scale      )) - (uint32_t)(&nvm_tx)), "POT scale"   , "s16x10" , ROACH_SCALE_MULTIPLIER,                       0, INT_MAX               , 1, },
    { ((uint32_t)(&(nvm_tx.pot_aux     .limit_min  )) - (uint32_t)(&nvm_tx)), "POT lim min" , "s16"    ,                      0,                       0, ROACH_ADC_MAX         , 1, },
    { ((uint32_t)(&(nvm_tx.pot_aux     .limit_max  )) - (uint32_t)(&nvm_tx)), "POT lim max" , "s16"    ,          ROACH_ADC_MAX,                       0, ROACH_ADC_MAX         , 1, },
    { ((uint32_t)(&(nvm_tx.pot_aux     .expo       )) - (uint32_t)(&nvm_tx)), "POT expo"    , "s16x10" ,                      0, -ROACH_SCALE_MULTIPLIER, ROACH_SCALE_MULTIPLIER, 1, },
    { ((uint32_t)(&(nvm_tx.pot_aux     .filter     )) - (uint32_t)(&nvm_tx)), "POT filter"  , "s16x10" ,   ROACH_FILTER_DEFAULT,                       0, ROACH_SCALE_MULTIPLIER, 1, },
    { ((uint32_t)(&(nvm_tx.pot_battery .filter     )) - (uint32_t)(&nvm_tx)), "BAT filter"  , "s16x10" ,   ROACH_FILTER_DEFAULT,                       0, ROACH_SCALE_MULTIPLIER, 1, },
    { ((uint32_t)(&(nvm_tx.startup_switches        )) - (uint32_t)(&nvm_tx)), "ST SW"       , "hex"    ,                   0x01,                       0,                   0x07, 1, },
    { ((uint32_t)(&(nvm_tx.startup_switches_mask   )) - (uint32_t)(&nvm_tx)), "ST SW mask"  , "hex"    ,                   0x01,                       0,                   0x07, 1, },
    ROACH_NVM_GUI_DESC_END,
};

uint32_t nvm_dirty = 0;

void settings_init(void)
{
    settings_factoryReset();
    settings_loadFile(ROACH_STARTUP_FILE_NAME);
}

void settings_factoryReset(void)
{
    memset(&nvm_tx, 0, sizeof(roach_tx_nvm_t));
    roachnvm_setdefaults((uint8_t*)&nvm_rf, cfggroup_rf);
    roachnvm_setdefaults((uint8_t*)&nvm_tx, cfggroup_ctrler);
    roachnvm_setdefaults((uint8_t*)&nvm_rx, cfggroup_drive);
    roachnvm_setdefaults((uint8_t*)&nvm_rx, cfggroup_weap);
    roachnvm_setdefaults((uint8_t*)&nvm_rx, cfggroup_imu);
}

bool settings_save(void)
{
    return settings_saveToFile(ROACH_STARTUP_FILE_NAME);
}

void settings_saveIfNeeded(uint32_t span)
{
    if (nvm_dirty > 0)
    {
        uint32_t now = millis();
        if ((now - nvm_dirty) >= span && RoachUsbMsd_canSave())
        {
            Serial.printf("[%u]:autosaving startup file\r\n", millis());
            settings_save();
            nvm_dirty = 0;
        }
    }
}

bool settings_loadFile(const char* fname)
{
    RoachFile f;
    bool suc = f.open(fname);
    uint8_t flags = 0;
    if (memcmp(fname, "rf", 2) == 0) {
        flags |= (1 << 0);
    }
    if (memcmp(fname, "ctrler", 6) == 0) {
        flags |= (1 << 1);
    }
    if (memcmp(fname, "robot", 5) == 0) {
        flags |= (1 << 2);
    }
    if (suc)
    {
        if ((flags & (1 << 0)) != 0 || flags == 0) {
            roachnvm_readfromfile(&f, (uint8_t*)&nvm_rf, cfggroup_rf);
            radio_init();
        }
        if ((flags & (1 << 1)) != 0 || flags == 0) {
            roachnvm_readfromfile(&f, (uint8_t*)&nvm_tx, cfggroup_ctrler);
        }
        if ((flags & (1 << 2)) != 0 || flags == 0) {
            roachnvm_readfromfile(&f, (uint8_t*)&nvm_rx, cfggroup_drive);
            roachnvm_readfromfile(&f, (uint8_t*)&nvm_rx, cfggroup_weap);
            roachnvm_readfromfile(&f, (uint8_t*)&nvm_rx, cfggroup_imu);
        }
        f.close();
        #ifdef ROACHTX_AUTOSAVE
        if (strncmp(fname, ROACH_STARTUP_FILE_NAME, 32) != 0) {
            settings_markDirty();
        }
        #endif
        return true;
    }
    return false;
}

bool settings_saveToFile(const char* fname)
{
    RoachFile f;
    bool suc = f.open(fname, O_RDWR | O_CREAT);
    uint8_t flags = 0;
    if (memcmp(fname, "rf", 2) == 0) {
        flags |= (1 << 0);
    }
    if (memcmp(fname, "ctrler", 6) == 0) {
        flags |= (1 << 1);
    }
    if (memcmp(fname, "robot", 5) == 0) {
        flags |= (1 << 2);
    }
    if (suc)
    {
        if ((flags & (1 << 0)) != 0 || flags == 0) {
            roachnvm_writetofile(&f, (uint8_t*)&nvm_rf, cfggroup_rf);
        }
        if ((flags & (1 << 1)) != 0 || flags == 0) {
            roachnvm_writetofile(&f, (uint8_t*)&nvm_tx, cfggroup_ctrler);
        }
        if ((flags & (1 << 2)) != 0 || flags == 0) {
            roachnvm_writetofile(&f, (uint8_t*)&nvm_rx, cfggroup_drive);
            roachnvm_writetofile(&f, (uint8_t*)&nvm_rx, cfggroup_weap);
            roachnvm_writetofile(&f, (uint8_t*)&nvm_rx, cfggroup_imu);
        }
        f.close();
        return true;
    }
    return false;
}

void settings_markDirty(void)
{
    nvm_dirty = millis();
}
