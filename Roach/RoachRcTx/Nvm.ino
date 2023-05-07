roach_rf_nvm_t nvm_rf;
roach_rx_nvm_t nvm_rx;
roach_tx_nvm_t nvm_tx;

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
    { ((uint32_t)(&(nvm_tx.pot_battery .center     )) - (uint32_t)(&nvm_tx)), "BAT center"  , "s16"    ,                      0,                       0, ROACH_ADC_MAX         , 1, },
    { ((uint32_t)(&(nvm_tx.pot_battery .limit_max  )) - (uint32_t)(&nvm_tx)), "BAT lim max" , "s16"    ,          ROACH_ADC_MAX,                       0, ROACH_ADC_MAX         , 1, },
    { ((uint32_t)(&(nvm_tx.pot_battery .filter     )) - (uint32_t)(&nvm_tx)), "BAT filter"  , "s16x10" ,   ROACH_FILTER_DEFAULT,                       0, ROACH_SCALE_MULTIPLIER, 1, },
    ROACH_NVM_GUI_DESC_END,
};

#define ROACH_STARTUP_FILE_NAME "startup.txt"

void settings_init(void)
{
    memset(&nvm_tx, 0, sizeof(roach_tx_nvm_t));
    roachnvm_setdefaults(&nvm_rf, cfggroup_rf);
    roachnvm_setdefaults(&nvm_tx, cfggroup_ctrler);
    roachnvm_setdefaults(&nvm_rx, cfggroup_drive);
    roachnvm_setdefaults(&nvm_rx, cfggroup_weap);
    roachnvm_setdefaults(&nvm_rx, cfggroup_imu);
    FatFile f;
    bool suc = f.open(ROACH_STARTUP_FILE_NAME);
    if (suc)
    {
        roachnvm_readfromfile(&f, &nvm_rf, cfggroup_rf);
        roachnvm_readfromfile(&f, &nvm_tx, cfggroup_ctrler);
        roachnvm_readfromfile(&f, &nvm_rx, cfggroup_drive);
        roachnvm_readfromfile(&f, &nvm_rx, cfggroup_weap);
        roachnvm_readfromfile(&f, &nvm_rx, cfggroup_imu);
        f.close();
    }
}

void settings_save(void)
{
    FatFile f;
    bool suc = f.open(ROACH_STARTUP_FILE_NAME, O_RDWR | O_CREAT);
    if (suc)
    {
        roachnvm_writetofile(&f, &nvm_rf, cfggroup_rf);
        roachnvm_writetofile(&f, &nvm_tx, cfggroup_ctrler);
        roachnvm_writetofile(&f, &nvm_rx, cfggroup_drive);
        roachnvm_writetofile(&f, &nvm_rx, cfggroup_weap);
        roachnvm_writetofile(&f, &nvm_rx, cfggroup_imu);
        f.close();
    }
}
