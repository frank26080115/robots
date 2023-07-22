roach_nvm_gui_desc_t cfggroup_drive[] = {
    { ((uint32_t)(&(nvm.drive_left_flip        )) - (uint32_t)(&nvm)), "L flip"    , "s8"     ,                      0,                                 0,                                 1, 1, },
    { ((uint32_t)(&(nvm.drive_right_flip       )) - (uint32_t)(&nvm)), "R flip"    , "s8"     ,                      0,                                 0,                                 1, 1, },
    { ((uint32_t)(&(nvm.drive_left .center     )) - (uint32_t)(&nvm)), "L center"  , "s16"    ,        ROACH_SERVO_MID,                   ROACH_SERVO_MIN,                   ROACH_SERVO_MAX, 1, },
    { ((uint32_t)(&(nvm.drive_right.center     )) - (uint32_t)(&nvm)), "R center"  , "s16"    ,        ROACH_SERVO_MID,                   ROACH_SERVO_MIN,                   ROACH_SERVO_MAX, 1, },
    { ((uint32_t)(&(nvm.drive_left .deadzone   )) - (uint32_t)(&nvm)), "L deadzone", "s16"    ,        ROACH_ADC_NOISE,                                 0,                   ROACH_SERVO_OVR, 1, },
    { ((uint32_t)(&(nvm.drive_right.deadzone   )) - (uint32_t)(&nvm)), "R deadzone", "s16"    ,        ROACH_ADC_NOISE,                                 0,                   ROACH_SERVO_OVR, 1, },
    { ((uint32_t)(&(nvm.drive_left .trim       )) - (uint32_t)(&nvm)), "L trim"    , "s32"    ,                      0,                                 0,                   ROACH_SERVO_OVR, 1, },
    { ((uint32_t)(&(nvm.drive_right.trim       )) - (uint32_t)(&nvm)), "R trim"    , "s32"    ,                      0,                                 0,                   ROACH_SERVO_OVR, 1, },
    { ((uint32_t)(&(nvm.drive_left .scale      )) - (uint32_t)(&nvm)), "L scale"   , "s16x10" , ROACH_SCALE_MULTIPLIER,                                 0,                           INT_MAX, 1, },
    { ((uint32_t)(&(nvm.drive_right.scale      )) - (uint32_t)(&nvm)), "R scale"   , "s16x10" , ROACH_SCALE_MULTIPLIER,                                 0,                           INT_MAX, 1, },
    { ((uint32_t)(&(nvm.drive_left .limit_min  )) - (uint32_t)(&nvm)), "L lim min" , "s16"    ,        ROACH_SERVO_MIN, ROACH_SERVO_MIN - ROACH_SERVO_OVR,                   ROACH_SERVO_MID, 1, },
    { ((uint32_t)(&(nvm.drive_right.limit_min  )) - (uint32_t)(&nvm)), "R lim min" , "s16"    ,        ROACH_SERVO_MIN, ROACH_SERVO_MIN - ROACH_SERVO_OVR,                   ROACH_SERVO_MID, 1, },
    { ((uint32_t)(&(nvm.drive_left .limit_max  )) - (uint32_t)(&nvm)), "L lim max" , "s16"    ,        ROACH_SERVO_MAX,                   ROACH_SERVO_MID, ROACH_SERVO_MAX + ROACH_SERVO_OVR, 1, },
    { ((uint32_t)(&(nvm.drive_right.limit_max  )) - (uint32_t)(&nvm)), "R lim max" , "s16"    ,        ROACH_SERVO_MAX,                   ROACH_SERVO_MID, ROACH_SERVO_MAX + ROACH_SERVO_OVR, 1, },
    ROACH_NVM_GUI_DESC_END,
};

roach_nvm_gui_desc_t cfggroup_weap[] = {
    { ((uint32_t)(&(nvm.weapon.center     )) - (uint32_t)(&nvm)), "WM center"  , "s16"    ,        ROACH_SERVO_MIN,                   ROACH_SERVO_MIN,                   ROACH_SERVO_MAX, 1, },
    { ((uint32_t)(&(nvm.weapon.deadzone   )) - (uint32_t)(&nvm)), "WM deadzone", "s16"    ,        ROACH_ADC_NOISE,                                 0,                   ROACH_SERVO_OVR, 1, },
    { ((uint32_t)(&(nvm.weapon.trim       )) - (uint32_t)(&nvm)), "WM trim"    , "s32"    ,                      0,                                 0,                   ROACH_SERVO_OVR, 1, },
    { ((uint32_t)(&(nvm.weapon.scale      )) - (uint32_t)(&nvm)), "WM scale"   , "s16x10" , ROACH_SCALE_MULTIPLIER,                                 0,                           INT_MAX, 1, },
    { ((uint32_t)(&(nvm.weapon.limit_min  )) - (uint32_t)(&nvm)), "WM lim min" , "s16"    ,        ROACH_SERVO_MIN, ROACH_SERVO_MIN - ROACH_SERVO_OVR,                   ROACH_SERVO_MID, 1, },
    { ((uint32_t)(&(nvm.weapon.limit_max  )) - (uint32_t)(&nvm)), "WM lim max" , "s16"    ,        ROACH_SERVO_MAX,                   ROACH_SERVO_MID, ROACH_SERVO_MAX + ROACH_SERVO_OVR, 1, },
    //{ ((uint32_t)(&(nvm.pid_spindown.p    )) - (uint32_t)(&nvm)), "SD P"       ,  "s16x10", ROACH_SCALE_MULTIPLIER, INT_MIN, INT_MAX  ,   1, },
    //{ ((uint32_t)(&(nvm.pid_spindown.i    )) - (uint32_t)(&nvm)), "SD I"       ,  "s16x10",                      0, INT_MIN, INT_MAX  ,   1, },
    //{ ((uint32_t)(&(nvm.pid_spindown.d    )) - (uint32_t)(&nvm)), "SD D"       ,  "s16x10",                      0, INT_MIN, INT_MAX  ,   1, },
    //{ ((uint32_t)(&(nvm.pid_spindown.output_limit))      - (uint32_t)(&nvm)), "SD out lim", "s16",         INT_MAX,       0, INT_MAX  ,   1, },
    //{ ((uint32_t)(&(nvm.pid_spindown.accumulator_limit)) - (uint32_t)(&nvm)), "SD i lim"  , "s16",         INT_MAX,       0, INT_MAX  ,   1, },
    //{ ((uint32_t)(&(nvm.pid_spindown.accumulator_decay)) - (uint32_t)(&nvm)), "SD i decay", "s16",               0,       0, INT_MAX  ,   1, },
    //{ ((uint32_t)(&(nvm.spindown_limit)) - (uint32_t)(&nvm)), "SD limit", "s16",                               200,       0, INT_MAX  ,   1, },
    //{ ((uint32_t)(&(nvm.weapon2.center    )) - (uint32_t)(&nvm)), "W2 center"  , "s16"    ,        ROACH_SERVO_MIN,                   ROACH_SERVO_MIN,                   ROACH_SERVO_MAX, 1, },
    //{ ((uint32_t)(&(nvm.weapon2.deadzone  )) - (uint32_t)(&nvm)), "W2 deadzone", "s16"    ,        ROACH_ADC_NOISE,                                 0,                   ROACH_SERVO_OVR, 1, },
    //{ ((uint32_t)(&(nvm.weapon2.trim      )) - (uint32_t)(&nvm)), "W2 trim"    , "s32"    ,                      0,                                 0,                   ROACH_SERVO_OVR, 1, },
    //{ ((uint32_t)(&(nvm.weapon2.scale     )) - (uint32_t)(&nvm)), "W2 scale"   , "s16x10" , ROACH_SCALE_MULTIPLIER,                                 0,                           INT_MAX, 1, },
    //{ ((uint32_t)(&(nvm.weapon2.limit_min )) - (uint32_t)(&nvm)), "W2 lim min" , "s16"    ,        ROACH_SERVO_MIN, ROACH_SERVO_MIN - ROACH_SERVO_OVR,                   ROACH_SERVO_MID, 1, },
    //{ ((uint32_t)(&(nvm.weapon2.limit_max )) - (uint32_t)(&nvm)), "W2 lim max" , "s16"    ,        ROACH_SERVO_MAX,                   ROACH_SERVO_MID, ROACH_SERVO_MAX + ROACH_SERVO_OVR, 1, },
    ROACH_NVM_GUI_DESC_END,
};

roach_nvm_gui_desc_t cfggroup_sensor[] = {
    //{ ((uint32_t)(&(nvm.imu_orientation              )) - (uint32_t)(&nvm)), "IMU ori"    , "s8"     ,                      0,       0, 6 * 8 - 1,   1, },
    { ((uint32_t)(&(nvm.heading_timeout              )) - (uint32_t)(&nvm)), "IMU timeout", "s16"    ,                   3000,       0, INT_MAX  , 100, },
    { ((uint32_t)(&(nvm.pid_heading.p                )) - (uint32_t)(&nvm)), "PID P"      , "s16x10" , ROACH_SCALE_MULTIPLIER, INT_MIN, INT_MAX  ,   1, },
    { ((uint32_t)(&(nvm.pid_heading.i                )) - (uint32_t)(&nvm)), "PID I"      , "s16x10" ,                      0, INT_MIN, INT_MAX  ,   1, },
    { ((uint32_t)(&(nvm.pid_heading.d                )) - (uint32_t)(&nvm)), "PID D"      , "s16x10" ,                      0, INT_MIN, INT_MAX  ,   1, },
    { ((uint32_t)(&(nvm.pid_heading.output_limit     )) - (uint32_t)(&nvm)), "PID out lim",     "s16",                INT_MAX,       0, INT_MAX  ,   1, },
    { ((uint32_t)(&(nvm.pid_heading.accumulator_limit)) - (uint32_t)(&nvm)), "PID i lim"  ,     "s16",                INT_MAX,       0, INT_MAX  ,   1, },
    { ((uint32_t)(&(nvm.pid_heading.accumulator_decay)) - (uint32_t)(&nvm)), "PID i decay",     "s16",                      0,       0, INT_MAX  ,   1, },
    ROACH_NVM_GUI_DESC_END,
};

void settings_init(void)
{
    // TODO: set defaults for radio RF parameters here
    roachrobot_defaultSettings((uint8_t*)&nvm);
    roachrobot_loadSettings((uint8_t*)&nvm);
}