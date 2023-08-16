#if defined(ESP32)
#include <SPIFFS.h>
#elif defined(NRF52840_XXAA)
#include <Adafruit_LittleFS.h>
#include <InternalFileSystem.h>
#endif

extern uint8_t* rosync_nvm;
extern roach_nvm_gui_desc_t* rosync_desc_tbl;

roach_nvm_gui_desc_t cfgdesc_ctrler[] = {
    { ((uint32_t)(&(nvm_tx.heading_multiplier      )) - (uint32_t)(&nvm_tx)), "H scale"     , "s32"    ,                   225,                 INT_MIN, INT_MAX               , 1, },
    //{ ((uint32_t)(&(nvm_tx.cross_mix               )) - (uint32_t)(&nvm_tx)), "X mix"       , "s32"    ,                      0,                 INT_MIN, INT_MAX               , 1, },
    { ((uint32_t)(&(nvm_tx.pot_throttle.center     )) - (uint32_t)(&nvm_tx)), "T center"    , "s16"    ,          ROACH_ADC_MID,                       0, ROACH_ADC_MAX         , 1, },
    { ((uint32_t)(&(nvm_tx.pot_throttle.deadzone   )) - (uint32_t)(&nvm_tx)), "T deadzone"  , "s16"    ,        ROACH_ADC_NOISE,                       0, ROACH_ADC_MAX / 32    , 1, },
    { ((uint32_t)(&(nvm_tx.pot_throttle.boundary   )) - (uint32_t)(&nvm_tx)), "T boundary"  , "s16"    ,        ROACH_ADC_NOISE,                       0, ROACH_ADC_MAX / 32    , 1, },
    { ((uint32_t)(&(nvm_tx.pot_throttle.scale      )) - (uint32_t)(&nvm_tx)), "T scale"     , "s16"    , ROACH_SCALE_MULTIPLIER,                       0, INT_MAX               , 1, },
    { ((uint32_t)(&(nvm_tx.pot_throttle.limit_min  )) - (uint32_t)(&nvm_tx)), "T lim min"   , "s16"    ,                      0,                       0, ROACH_ADC_MAX         , 1, },
    { ((uint32_t)(&(nvm_tx.pot_throttle.limit_max  )) - (uint32_t)(&nvm_tx)), "T lim max"   , "s16"    ,          ROACH_ADC_MAX,                       0, ROACH_ADC_MAX         , 1, },
    { ((uint32_t)(&(nvm_tx.pot_throttle.expo       )) - (uint32_t)(&nvm_tx)), "T expo"      , "s16"    ,                      0, -ROACH_SCALE_MULTIPLIER, ROACH_SCALE_MULTIPLIER, 1, },
    { ((uint32_t)(&(nvm_tx.pot_throttle.filter     )) - (uint32_t)(&nvm_tx)), "T filter"    , "s16"    ,   ROACH_FILTER_DEFAULT,                       0, ROACH_SCALE_MULTIPLIER, 1, },
    { ((uint32_t)(&(nvm_tx.pot_steering.center     )) - (uint32_t)(&nvm_tx)), "S center"    , "s16"    ,          ROACH_ADC_MID,                       0, ROACH_ADC_MAX         , 1, },
    { ((uint32_t)(&(nvm_tx.pot_steering.deadzone   )) - (uint32_t)(&nvm_tx)), "S deadzone"  , "s16"    ,        ROACH_ADC_NOISE,                       0, ROACH_ADC_MAX / 32    , 1, },
    { ((uint32_t)(&(nvm_tx.pot_steering.boundary   )) - (uint32_t)(&nvm_tx)), "S boundary"  , "s16"    ,        ROACH_ADC_NOISE,                       0, ROACH_ADC_MAX / 32    , 1, },
    { ((uint32_t)(&(nvm_tx.pot_steering.scale      )) - (uint32_t)(&nvm_tx)), "S scale"     , "s16"    , ROACH_SCALE_MULTIPLIER,                       0, INT_MAX               , 1, },
    { ((uint32_t)(&(nvm_tx.pot_steering.limit_min  )) - (uint32_t)(&nvm_tx)), "S lim min"   , "s16"    ,                      0,                       0, ROACH_ADC_MAX         , 1, },
    { ((uint32_t)(&(nvm_tx.pot_steering.limit_max  )) - (uint32_t)(&nvm_tx)), "S lim max"   , "s16"    ,          ROACH_ADC_MAX,                       0, ROACH_ADC_MAX         , 1, },
    { ((uint32_t)(&(nvm_tx.pot_steering.expo       )) - (uint32_t)(&nvm_tx)), "S expo"      , "s16"    ,                      0, -ROACH_SCALE_MULTIPLIER, ROACH_SCALE_MULTIPLIER, 1, },
    { ((uint32_t)(&(nvm_tx.pot_steering.filter     )) - (uint32_t)(&nvm_tx)), "S filter"    , "s16"    ,   ROACH_FILTER_DEFAULT,                       0, ROACH_SCALE_MULTIPLIER, 1, },
    { ((uint32_t)(&(nvm_tx.pot_weapon  .center     )) - (uint32_t)(&nvm_tx)), "WC center"   , "s16"    ,                      0,                       0, ROACH_ADC_MAX         , 1, },
    { ((uint32_t)(&(nvm_tx.pot_weapon  .deadzone   )) - (uint32_t)(&nvm_tx)), "WC deadzone" , "s16"    ,        ROACH_ADC_NOISE,                       0, ROACH_ADC_MAX / 32    , 1, },
    { ((uint32_t)(&(nvm_tx.pot_weapon  .boundary   )) - (uint32_t)(&nvm_tx)), "WC boundary" , "s16"    ,        ROACH_ADC_NOISE,                       0, ROACH_ADC_MAX / 32    , 1, },
    { ((uint32_t)(&(nvm_tx.pot_weapon  .scale      )) - (uint32_t)(&nvm_tx)), "WC scale"    , "s16"    , ROACH_SCALE_MULTIPLIER,                       0, INT_MAX               , 1, },
    { ((uint32_t)(&(nvm_tx.pot_weapon  .limit_min  )) - (uint32_t)(&nvm_tx)), "WC lim min"  , "s16"    ,                      0,                       0, ROACH_ADC_MAX         , 1, },
    { ((uint32_t)(&(nvm_tx.pot_weapon  .limit_max  )) - (uint32_t)(&nvm_tx)), "WC lim max"  , "s16"    ,          ROACH_ADC_MAX,                       0, ROACH_ADC_MAX         , 1, },
    { ((uint32_t)(&(nvm_tx.pot_weapon  .expo       )) - (uint32_t)(&nvm_tx)), "WC expo"     , "s16"    ,                      0, -ROACH_SCALE_MULTIPLIER, ROACH_SCALE_MULTIPLIER, 1, },
    { ((uint32_t)(&(nvm_tx.pot_weapon  .filter     )) - (uint32_t)(&nvm_tx)), "WC filter"   , "s16"    ,   ROACH_FILTER_DEFAULT,                       0, ROACH_SCALE_MULTIPLIER, 1, },
    { ((uint32_t)(&(nvm_tx.pot_aux     .center     )) - (uint32_t)(&nvm_tx)), "POT center"  , "s16"    ,                      0,                       0, ROACH_ADC_MAX         , 1, },
    { ((uint32_t)(&(nvm_tx.pot_aux     .deadzone   )) - (uint32_t)(&nvm_tx)), "POT deadzone", "s16"    ,        ROACH_ADC_NOISE,                       0, ROACH_ADC_MAX / 32    , 1, },
    { ((uint32_t)(&(nvm_tx.pot_aux     .boundary   )) - (uint32_t)(&nvm_tx)), "POT boundary", "s16"    ,        ROACH_ADC_NOISE,                       0, ROACH_ADC_MAX / 32    , 1, },
    { ((uint32_t)(&(nvm_tx.pot_aux     .scale      )) - (uint32_t)(&nvm_tx)), "POT scale"   , "s16"    , ROACH_SCALE_MULTIPLIER,                       0, INT_MAX               , 1, },
    { ((uint32_t)(&(nvm_tx.pot_aux     .limit_min  )) - (uint32_t)(&nvm_tx)), "POT lim min" , "s16"    ,                      0,                       0, ROACH_ADC_MAX         , 1, },
    { ((uint32_t)(&(nvm_tx.pot_aux     .limit_max  )) - (uint32_t)(&nvm_tx)), "POT lim max" , "s16"    ,          ROACH_ADC_MAX,                       0, ROACH_ADC_MAX         , 1, },
    { ((uint32_t)(&(nvm_tx.pot_aux     .expo       )) - (uint32_t)(&nvm_tx)), "POT expo"    , "s16"    ,                      0, -ROACH_SCALE_MULTIPLIER, ROACH_SCALE_MULTIPLIER, 1, },
    { ((uint32_t)(&(nvm_tx.pot_aux     .filter     )) - (uint32_t)(&nvm_tx)), "POT filter"  , "s16"    ,   ROACH_FILTER_DEFAULT,                       0, ROACH_SCALE_MULTIPLIER, 1, },
    { ((uint32_t)(&(nvm_tx.pot_battery .filter     )) - (uint32_t)(&nvm_tx)), "BAT filter"  , "s16"    ,   ROACH_FILTER_DEFAULT,                       0, ROACH_SCALE_MULTIPLIER, 1, },
    { ((uint32_t)(&(nvm_tx.startup_switches        )) - (uint32_t)(&nvm_tx)), "ST SW"       , "hex"    ,                   0x01,                       0,                   0x07, 1, },
    { ((uint32_t)(&(nvm_tx.startup_switches_mask   )) - (uint32_t)(&nvm_tx)), "ST SW mask"  , "hex"    ,                   0x01,                       0,                   0x07, 1, },
    ROACH_NVM_GUI_DESC_END,
};

uint32_t nvm_dirty = 0;

void settings_init(void)
{
    settings_factoryReset();
    settings_loadFile(ROACH_STARTUP_CONF_NAME);
}

void settings_factoryReset(void)
{
    memset(&nvm_tx, 0, sizeof(roach_tx_nvm_t));
    roachnvm_setdefaults((uint8_t*)&nvm_rf, cfgdesc_rf);
    roachnvm_setdefaults((uint8_t*)&nvm_tx, cfgdesc_ctrler);
}

bool settings_save(void)
{
    return settings_saveToFile(ROACH_STARTUP_CONF_NAME);
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
    debug_printf("[%u] settings_loadFile \"%s\"\r\n", millis(), fname);
    RoachFile f;
    bool suc = f.open(fname);
    debug_printf("f.open = %u\r\n", suc);
    uint8_t flags = 0;
    if (memcmp(fname, "rf", 2) == 0) {
        flags |= (1 << 0);
    }
    else if (memcmp(fname, "ctrler", 6) == 0) {
        flags |= (1 << 1);
    }
    else if (memcmp(fname, "robot", 5) == 0) {
        flags |= (1 << 2);
    }
    else if (memcmp(fname, "rdesc", 5) == 0) {
        flags |= (1 << 3);
    }
    debug_printf("flags = 0x%02X\r\n", flags);
    if (suc)
    {
        if ((flags & (1 << 0)) != 0 || flags == 0) {
            roachnvm_readfromfile(&f, (uint8_t*)&nvm_rf, cfgdesc_rf);
            #ifndef DEVMODE_NO_RADIO
            radio_init();
            #endif
        }
        if ((flags & (1 << 1)) != 0 || flags == 0) {
            roachnvm_readfromfile(&f, (uint8_t*)&nvm_tx, cfgdesc_ctrler);
        }
        if (rosync_nvm != NULL && rosync_desc_tbl != NULL) 
        {
            if ((flags & (1 << 2)) != 0 || flags == 0) {
                roachnvm_readfromfile(&f, (uint8_t*)rosync_nvm, rosync_desc_tbl);
            }
        }
        if ((flags & (1 << 3)) != 0) {
            rosync_loadDescFileObj(&f, 0);
        }
        debug_printf("read done, closing file\r\n");
        f.close();
        #ifdef ROACHTX_AUTOSAVE
        if (strncmp(fname, ROACH_STARTUP_CONF_NAME, 32) != 0) {
            settings_markDirty();
        }
        #endif
        return true;
    }
    return false;
}

bool settings_saveToFile(const char* fname)
{
    debug_printf("[%u] settings_saveToFile \"%s\"\r\n", millis(), fname);
    RoachFile f;
    bool suc = f.open(fname, O_RDWR | O_CREAT);
    debug_printf("f.open = %u\r\n", suc);
    uint8_t flags = 0;
    if (memcmp(fname, "rf", 2) == 0) {
        flags |= (1 << 0);
    }
    else if (memcmp(fname, "ctrler", 6) == 0) {
        flags |= (1 << 1);
    }
    else if (memcmp(fname, "robot", 5) == 0) {
        flags |= (1 << 2);
    }
    debug_printf("flags = 0x%02X\r\n", flags);
    if (suc)
    {
        if ((flags & (1 << 0)) != 0 || flags == 0) {
            roachnvm_writetofile(&f, (uint8_t*)&nvm_rf, cfgdesc_rf);
            debug_printf("wrote RF params\r\n");
        }
        if ((flags & (1 << 1)) != 0 || flags == 0) {
            roachnvm_writetofile(&f, (uint8_t*)&nvm_tx, cfgdesc_ctrler);
            debug_printf("wrote ctrler cfg\r\n");
        }
        if (rosync_nvm != NULL && rosync_desc_tbl != NULL) 
        {
            if ((flags & (1 << 2)) != 0 || flags == 0) {
                roachnvm_writetofile(&f, (uint8_t*)rosync_nvm, rosync_desc_tbl);
                debug_printf("wrote robot cfg\r\n");
            }
        }
        debug_printf("write done, closing file\r\n");
        f.close();
        return true;
    }
    return false;
}

void settings_markDirty(void)
{
    nvm_dirty = millis();
}

bool roachnvm_fileCopy(const char* fin_name, const char* fout_name)
{
    RoachFile fin;
    RoachFile fout;
    bool suc;
    suc = fin.open(fin_name);
    if (suc == false) {
        Serial.printf("ERR[%u]: roachnvm_fileCopy cannot open fin \"%s\"\r\n", millis(), fin_name);
        return false;
    }
    suc = fout.open(fout_name, O_RDWR | O_CREAT);
    if (suc == false) {
        Serial.printf("ERR[%u]: roachnvm_fileCopy cannot open fout \"%s\"\r\n", millis(), fout_name);
        fin.close();
        return false;
    }
    #define ROACHNVM_FILECOPY_CHUNK 256
    uint8_t tmpbuf[ROACHNVM_FILECOPY_CHUNK];
    while (true)
    {
        int r = fin.read(tmpbuf, ROACHNVM_FILECOPY_CHUNK);
        if (r > 0)
        {
            fout.write(tmpbuf, r);
        }
        else
        {
            fin.close();
            fout.close();
            debug_printf("[%u] roachnvm_fileCopy \"%s\" -> \"%s\"\r\n", millis(), fin_name, fout_name);
            return true;
        }
    }
}

void settings_initValidate(void)
{
    roachnvm_validateAll((uint8_t*)&nvm_tx, cfgdesc_ctrler);
}

void settings_debugNvm(Stream* stream)
{
    stream->printf("NVM debug:\r\n");
    stream->printf("==========\r\n");
    stream->printf("RF params:\r\n");
    roachnvm_debugNvm(stream, (uint8_t*)&nvm_rf, sizeof(nvm_rf), cfgdesc_rf);
    stream->printf("==========\r\n");
    stream->printf("TX params [%u]:\r\n", sizeof(nvm_tx));
    roachnvm_debugNvm(stream, (uint8_t*)&nvm_tx, sizeof(nvm_tx), cfgdesc_ctrler);
    stream->printf("==========\r\n");
}

void settings_debugListFiles(Stream* stream)
{
    stream->println("settings_debugListFiles");
    if (!fatroot.open("/"))
    {
        stream->println("open root failed");
        return;
    }
    stream->println("open root success");
    int i = 1;
    while (fatfile.openNext(&fatroot, O_RDONLY))
    {
        if (fatfile.isDir() == false)
        {
            char sfname[64];
            fatfile.getName7(sfname, 62);
            stream->printf("%d: \"%s\"\r\n", i, sfname);
            i++;
        }
        fatfile.close();
    }
    fatroot.close();
    stream->println("==========");
}
