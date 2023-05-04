enum
{
    MENU_HOME,                  // display input visualizations, hold center button for 3 seconds for gyro calibration
    MENU_STATS,                 // display extra live data
    MENU_INFO,                  // display stuff like profile, UID, version
    MENU_RAW,                   // display raw input data
    MENU_CONFIG_CONN,           // allows user to select between profiles of RF parameters
    MENU_CONFIG_PROFILE_ROBOT,  // allows user to select between profiles of robot parameters
    MENU_CONFIG_PROFILE_CTRLER, // allows user to select between profiles of controller parameters
    MENU_CONFIG_DRIVE,          // edit parameters about drive train
    MENU_CONFIG_CALIB,          // options for calibrating controller
    MENU_CONFIG_WEAP,           // edit parameters about the weapon
    MENU_CONFIG_CTRLER,         // edit parameters about the controller
    MENU_CONFIG_IMU,            // edit parameters for the IMU (orientation, PID, timeout, etc)
};

roach_nvm_gui_desc_t cfggroup_rf[] = {
    { ((uint32_t)(&(nvm_rf.uid     )) - (uint32_t)(&nvm_rf)), "UID"     , "hex", 0x1234ABCD, 0, 0xFFFFFFFF, 1, },
    { ((uint32_t)(&(nvm_rf.salt    )) - (uint32_t)(&nvm_rf)), "salt"    , "hex", 0xDEADBEEF, 0, 0xFFFFFFFF, 1, },
    { ((uint32_t)(&(nvm_rf.chan_map)) - (uint32_t)(&nvm_rf)), "chan map", "hex", 0x0FFFFFFF, 0, 0xFFFFFFFF, 1, },
    ROACH_NVM_GUI_DESC_END,
};

roach_rf_nvm_t nvm_rf;
roach_rx_nvm_t nvm_rx;
roach_tx_nvm_t nvm_tx;

roach_nvm_gui_desc_t cfggroup_drive[] = {
    { ((uint32_t)(&(nvm_rx.drive_left_flip        )) - (uint32_t)(&nvm_rx)), "L flip"    , "s8"     ,    0,    0,    1, 1, },
    { ((uint32_t)(&(nvm_rx.drive_right_flip       )) - (uint32_t)(&nvm_rx)), "R flip"    , "s8"     ,    0,    0,    1, 1, },
    { ((uint32_t)(&(nvm_rx.drive_left .center     )) - (uint32_t)(&nvm_rx)), "L center"  , "s16"    , 1500, 1000, 2000, 1, },
    { ((uint32_t)(&(nvm_rx.drive_right.center     )) - (uint32_t)(&nvm_rx)), "R center"  , "s16"    , 1500, 1000, 2000, 1, },
    { ((uint32_t)(&(nvm_rx.drive_left .deadzone   )) - (uint32_t)(&nvm_rx)), "L deadzone", "s16"    , 30  ,    0,  100, 1, },
    { ((uint32_t)(&(nvm_rx.drive_right.deadzone   )) - (uint32_t)(&nvm_rx)), "R deadzone", "s16"    , 30  ,    0,  100, 1, },
    { ((uint32_t)(&(nvm_rx.drive_left .trim       )) - (uint32_t)(&nvm_rx)), "L trim"    , "s32"    , 0   ,    0,  100, 1, },
    { ((uint32_t)(&(nvm_rx.drive_right.trim       )) - (uint32_t)(&nvm_rx)), "R trim"    , "s32"    , 0   ,    0,  100, 1, },
    { ((uint32_t)(&(nvm_rx.drive_left .scale      )) - (uint32_t)(&nvm_rx)), "L scale"   , "s16x100", 100 ,    0, 1000, 1, },
    { ((uint32_t)(&(nvm_rx.drive_right.scale      )) - (uint32_t)(&nvm_rx)), "R scale"   , "s16x100", 100 ,    0, 1000, 1, },
    { ((uint32_t)(&(nvm_rx.drive_left .limit_min  )) - (uint32_t)(&nvm_rx)), "L lim min" , "s16"    , 1000,  500, 1500, 1, },
    { ((uint32_t)(&(nvm_rx.drive_right.limit_min  )) - (uint32_t)(&nvm_rx)), "R lim min" , "s16"    , 1000,  500, 1500, 1, },
    { ((uint32_t)(&(nvm_rx.drive_left .limit_max  )) - (uint32_t)(&nvm_rx)), "L lim max" , "s16"    , 2000, 1500, 2500, 1, },
    { ((uint32_t)(&(nvm_rx.drive_right.limit_max  )) - (uint32_t)(&nvm_rx)), "R lim max" , "s16"    , 2000, 1500, 2500, 1, },
    ROACH_NVM_GUI_DESC_END,
};

roach_nvm_gui_desc_t cfggroup_weap[] = {
    { ((uint32_t)(&(nvm_rx.weapon.center     )) - (uint32_t)(&nvm_rx)), "WM center"  , "s16"    , 1000, 1000, 2000, 1, },
    { ((uint32_t)(&(nvm_rx.weapon.deadzone   )) - (uint32_t)(&nvm_rx)), "WM deadzone", "s16"    , 0   ,    0,  100, 1, },
    { ((uint32_t)(&(nvm_rx.weapon.trim       )) - (uint32_t)(&nvm_rx)), "WM trim"    , "s32"    , 0   ,    0,  100, 1, },
    { ((uint32_t)(&(nvm_rx.weapon.scale      )) - (uint32_t)(&nvm_rx)), "WM scale"   , "s16x100", 100 ,    0, 1000, 1, },
    { ((uint32_t)(&(nvm_rx.weapon.limit_min  )) - (uint32_t)(&nvm_rx)), "WM lim min" , "s16"    , 1000,  500, 1500, 1, },
    { ((uint32_t)(&(nvm_rx.weapon.limit_max  )) - (uint32_t)(&nvm_rx)), "WM lim max" , "s16"    , 2000, 1500, 2500, 1, },
    ROACH_NVM_GUI_DESC_END,
};

roach_nvm_gui_desc_t cfggroup_imu[] = {
    { ((uint32_t)(&(nvm_rx.imu_orientation  )) - (uint32_t)(&nvm_rx)), "IMU ori"     , "s8"     , 0     , 0     , 6 * 8 - 1, 1  , },
    { ((uint32_t)(&(nvm_rx.heading_timeout  )) - (uint32_t)(&nvm_rx)), "IMU timeout" , "s16"    , 3000  , 0     , 0xFFFF   , 100, },
    { ((uint32_t)(&(nvm_rx.pid_heading.p    )) - (uint32_t)(&nvm_rx)), "PID P"       , "s16x100", 100   , -30000, 30000    , 1  , },
    { ((uint32_t)(&(nvm_rx.pid_heading.i    )) - (uint32_t)(&nvm_rx)), "PID I"       , "s16x100",   0   , -30000, 30000    , 1  , },
    { ((uint32_t)(&(nvm_rx.pid_heading.d    )) - (uint32_t)(&nvm_rx)), "PID D"       , "s16x100",   0   , -30000, 30000    , 1  , },
    { ((uint32_t)(&(nvm_rx.pid_heading.output_limit))      - (uint32_t)(&nvm_rx)), "PID out lim" , "s16" , 0xFFFF, 0, 30000, 1, },
    { ((uint32_t)(&(nvm_rx.pid_heading.accumulator_limit)) - (uint32_t)(&nvm_rx)), "PID i lim"   , "s16" , 0xFFFF, 0, 30000, 1, },
    { ((uint32_t)(&(nvm_rx.pid_heading.accumulator_decay)) - (uint32_t)(&nvm_rx)), "PID i decay" , "s16" , 0     , 0, 30000, 1, },
    ROACH_NVM_GUI_DESC_END,
};

roach_nvm_gui_desc_t cfggroup_ctrler[] = {
    { ((uint32_t)(&(nvm_tx.heading_multiplier      )) - (uint32_t)(&nvm_rx)), "H scale"    , "s32x100", 100 ,-1000, 1000, 1, },
    { ((uint32_t)(&(nvm_tx.cross_mix               )) - (uint32_t)(&nvm_rx)), "X mix  "    , "s32x100",   0 ,-1000, 1000, 1, },
    { ((uint32_t)(&(nvm_tx.pot_throttle.center     )) - (uint32_t)(&nvm_rx)), "T center"   , "s16"    , 2048,    0, 4095, 1, },
    { ((uint32_t)(&(nvm_tx.pot_throttle.deadzone   )) - (uint32_t)(&nvm_rx)), "T deadzone" , "s16"    , 30  ,    0,  100, 1, },
    { ((uint32_t)(&(nvm_tx.pot_throttle.boundary   )) - (uint32_t)(&nvm_rx)), "T boundary" , "s16"    , 30  ,    0,  100, 1, },
    { ((uint32_t)(&(nvm_tx.pot_throttle.scale      )) - (uint32_t)(&nvm_rx)), "T scale"    , "s16x100", 100 ,    0, 1000, 1, },
    { ((uint32_t)(&(nvm_tx.pot_throttle.limit_min  )) - (uint32_t)(&nvm_rx)), "T lim min"  , "s16"    , 0   ,    0, 4095, 1, },
    { ((uint32_t)(&(nvm_tx.pot_throttle.limit_max  )) - (uint32_t)(&nvm_rx)), "T lim max"  , "s16"    , 4095,    0, 4095, 1, },
    { ((uint32_t)(&(nvm_tx.pot_throttle.expo       )) - (uint32_t)(&nvm_rx)), "T expo"     , "s16x100",    0, -100,  100, 1, },
    { ((uint32_t)(&(nvm_tx.pot_throttle.filter     )) - (uint32_t)(&nvm_rx)), "T filter"   , "s16x100",   90,    0, 1000, 1, },
    { ((uint32_t)(&(nvm_tx.pot_steering.center     )) - (uint32_t)(&nvm_rx)), "S center"   , "s16"    , 2048,    0, 4095, 1, },
    { ((uint32_t)(&(nvm_tx.pot_steering.deadzone   )) - (uint32_t)(&nvm_rx)), "S deadzone" , "s16"    , 30  ,    0,  100, 1, },
    { ((uint32_t)(&(nvm_tx.pot_steering.boundary   )) - (uint32_t)(&nvm_rx)), "S boundary" , "s16"    , 30  ,    0,  100, 1, },
    { ((uint32_t)(&(nvm_tx.pot_steering.scale      )) - (uint32_t)(&nvm_rx)), "S scale"    , "s16x100", 100 ,    0, 1000, 1, },
    { ((uint32_t)(&(nvm_tx.pot_steering.limit_min  )) - (uint32_t)(&nvm_rx)), "S lim min"  , "s16"    , 0   ,    0, 4095, 1, },
    { ((uint32_t)(&(nvm_tx.pot_steering.limit_max  )) - (uint32_t)(&nvm_rx)), "S lim max"  , "s16"    , 4095,    0, 4095, 1, },
    { ((uint32_t)(&(nvm_tx.pot_steering.expo       )) - (uint32_t)(&nvm_rx)), "S expo"     , "s16x100",    0, -100,  100, 1, },
    { ((uint32_t)(&(nvm_tx.pot_steering.filter     )) - (uint32_t)(&nvm_rx)), "S filter"   , "s16x100",   90,    0, 1000, 1, },
    { ((uint32_t)(&(nvm_tx.pot_weapon  .center     )) - (uint32_t)(&nvm_rx)), "WC center"  , "s16"    , 0   ,    0, 4095, 1, },
    { ((uint32_t)(&(nvm_tx.pot_weapon  .deadzone   )) - (uint32_t)(&nvm_rx)), "WC deadzone", "s16"    , 30  ,    0,  100, 1, },
    { ((uint32_t)(&(nvm_tx.pot_weapon  .boundary   )) - (uint32_t)(&nvm_rx)), "WC boundary", "s16"    , 30  ,    0,  100, 1, },
    { ((uint32_t)(&(nvm_tx.pot_weapon  .scale      )) - (uint32_t)(&nvm_rx)), "WC scale"   , "s16x100", 100 ,    0, 1000, 1, },
    { ((uint32_t)(&(nvm_tx.pot_weapon  .limit_min  )) - (uint32_t)(&nvm_rx)), "WC lim min" , "s16"    , 0   ,    0, 4095, 1, },
    { ((uint32_t)(&(nvm_tx.pot_weapon  .limit_max  )) - (uint32_t)(&nvm_rx)), "WC lim max" , "s16"    , 4095,    0, 4095, 1, },
    { ((uint32_t)(&(nvm_tx.pot_weapon  .expo       )) - (uint32_t)(&nvm_rx)), "WC expo"    , "s16x100",    0, -100,  100, 1, },
    { ((uint32_t)(&(nvm_tx.pot_weapon  .filter     )) - (uint32_t)(&nvm_rx)), "WC filter"  , "s16x100",   90,    0, 1000, 1, },
    ROACH_NVM_GUI_DESC_END,
};