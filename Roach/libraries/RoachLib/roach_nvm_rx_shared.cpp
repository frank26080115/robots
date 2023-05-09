#include "RoachLib.h"

extern roach_rx_nvm_t nvm_rx;
extern roach_rf_nvm_t nvm_rf;

roach_nvm_gui_desc_t cfggroup_rf[] = {
    { ((uint32_t)(&(nvm_rf.uid     )) - (uint32_t)(&nvm_rf)), "UID"     , "hex", (int32_t)0x1234ABCD, 0, 0, 1, },
    { ((uint32_t)(&(nvm_rf.salt    )) - (uint32_t)(&nvm_rf)), "salt"    , "hex", (int32_t)0xDEADBEEF, 0, 0, 1, },
    { ((uint32_t)(&(nvm_rf.chan_map)) - (uint32_t)(&nvm_rf)), "chan map", "hex", (int32_t)0x0FFFFFFF, 0, 0, 1, },
    ROACH_NVM_GUI_DESC_END,
};

roach_nvm_gui_desc_t cfggroup_drive[] = {
    { ((uint32_t)(&(nvm_rx.drive_left_flip        )) - (uint32_t)(&nvm_rx)), "L flip"    , "s8"     ,                      0,                                 0,                                 1, 1, },
    { ((uint32_t)(&(nvm_rx.drive_right_flip       )) - (uint32_t)(&nvm_rx)), "R flip"    , "s8"     ,                      0,                                 0,                                 1, 1, },
    { ((uint32_t)(&(nvm_rx.drive_left .center     )) - (uint32_t)(&nvm_rx)), "L center"  , "s16"    ,        ROACH_SERVO_MID,                   ROACH_SERVO_MIN,                   ROACH_SERVO_MAX, 1, },
    { ((uint32_t)(&(nvm_rx.drive_right.center     )) - (uint32_t)(&nvm_rx)), "R center"  , "s16"    ,        ROACH_SERVO_MID,                   ROACH_SERVO_MIN,                   ROACH_SERVO_MAX, 1, },
    { ((uint32_t)(&(nvm_rx.drive_left .deadzone   )) - (uint32_t)(&nvm_rx)), "L deadzone", "s16"    ,        ROACH_ADC_NOISE,                                 0,                   ROACH_SERVO_OVR, 1, },
    { ((uint32_t)(&(nvm_rx.drive_right.deadzone   )) - (uint32_t)(&nvm_rx)), "R deadzone", "s16"    ,        ROACH_ADC_NOISE,                                 0,                   ROACH_SERVO_OVR, 1, },
    { ((uint32_t)(&(nvm_rx.drive_left .trim       )) - (uint32_t)(&nvm_rx)), "L trim"    , "s32"    ,                      0,                                 0,                   ROACH_SERVO_OVR, 1, },
    { ((uint32_t)(&(nvm_rx.drive_right.trim       )) - (uint32_t)(&nvm_rx)), "R trim"    , "s32"    ,                      0,                                 0,                   ROACH_SERVO_OVR, 1, },
    { ((uint32_t)(&(nvm_rx.drive_left .scale      )) - (uint32_t)(&nvm_rx)), "L scale"   , "s16x10" , ROACH_SCALE_MULTIPLIER,                                 0,                           INT_MAX, 1, },
    { ((uint32_t)(&(nvm_rx.drive_right.scale      )) - (uint32_t)(&nvm_rx)), "R scale"   , "s16x10" , ROACH_SCALE_MULTIPLIER,                                 0,                           INT_MAX, 1, },
    { ((uint32_t)(&(nvm_rx.drive_left .limit_min  )) - (uint32_t)(&nvm_rx)), "L lim min" , "s16"    ,        ROACH_SERVO_MIN, ROACH_SERVO_MIN - ROACH_SERVO_OVR,                   ROACH_SERVO_MID, 1, },
    { ((uint32_t)(&(nvm_rx.drive_right.limit_min  )) - (uint32_t)(&nvm_rx)), "R lim min" , "s16"    ,        ROACH_SERVO_MIN, ROACH_SERVO_MIN - ROACH_SERVO_OVR,                   ROACH_SERVO_MID, 1, },
    { ((uint32_t)(&(nvm_rx.drive_left .limit_max  )) - (uint32_t)(&nvm_rx)), "L lim max" , "s16"    ,        ROACH_SERVO_MAX,                   ROACH_SERVO_MID, ROACH_SERVO_MAX + ROACH_SERVO_OVR, 1, },
    { ((uint32_t)(&(nvm_rx.drive_right.limit_max  )) - (uint32_t)(&nvm_rx)), "R lim max" , "s16"    ,        ROACH_SERVO_MAX,                   ROACH_SERVO_MID, ROACH_SERVO_MAX + ROACH_SERVO_OVR, 1, },
    ROACH_NVM_GUI_DESC_END,
};

roach_nvm_gui_desc_t cfggroup_weap[] = {
    { ((uint32_t)(&(nvm_rx.weapon.center     )) - (uint32_t)(&nvm_rx)), "WM center"  , "s16"    ,        ROACH_SERVO_MIN,                   ROACH_SERVO_MIN,                   ROACH_SERVO_MAX, 1, },
    { ((uint32_t)(&(nvm_rx.weapon.deadzone   )) - (uint32_t)(&nvm_rx)), "WM deadzone", "s16"    ,        ROACH_ADC_NOISE,                                 0,                   ROACH_SERVO_OVR, 1, },
    { ((uint32_t)(&(nvm_rx.weapon.trim       )) - (uint32_t)(&nvm_rx)), "WM trim"    , "s32"    ,                      0,                                 0,                   ROACH_SERVO_OVR, 1, },
    { ((uint32_t)(&(nvm_rx.weapon.scale      )) - (uint32_t)(&nvm_rx)), "WM scale"   , "s16x10" , ROACH_SCALE_MULTIPLIER,                                 0,                           INT_MAX, 1, },
    { ((uint32_t)(&(nvm_rx.weapon.limit_min  )) - (uint32_t)(&nvm_rx)), "WM lim min" , "s16"    ,        ROACH_SERVO_MIN, ROACH_SERVO_MIN - ROACH_SERVO_OVR,                   ROACH_SERVO_MID, 1, },
    { ((uint32_t)(&(nvm_rx.weapon.limit_max  )) - (uint32_t)(&nvm_rx)), "WM lim max" , "s16"    ,        ROACH_SERVO_MAX,                   ROACH_SERVO_MID, ROACH_SERVO_MAX + ROACH_SERVO_OVR, 1, },
    { ((uint32_t)(&(nvm_rx.pid_spindown.p    )) - (uint32_t)(&nvm_rx)), "SD P"       ,  "s16x10", ROACH_SCALE_MULTIPLIER, INT_MIN, INT_MAX  ,   1, },
    { ((uint32_t)(&(nvm_rx.pid_spindown.i    )) - (uint32_t)(&nvm_rx)), "SD I"       ,  "s16x10",                      0, INT_MIN, INT_MAX  ,   1, },
    { ((uint32_t)(&(nvm_rx.pid_spindown.d    )) - (uint32_t)(&nvm_rx)), "SD D"       ,  "s16x10",                      0, INT_MIN, INT_MAX  ,   1, },
    { ((uint32_t)(&(nvm_rx.pid_spindown.output_limit))      - (uint32_t)(&nvm_rx)), "SD out lim", "s16",         INT_MAX,       0, INT_MAX  ,   1, },
    { ((uint32_t)(&(nvm_rx.pid_spindown.accumulator_limit)) - (uint32_t)(&nvm_rx)), "SD i lim"  , "s16",         INT_MAX,       0, INT_MAX  ,   1, },
    { ((uint32_t)(&(nvm_rx.pid_spindown.accumulator_decay)) - (uint32_t)(&nvm_rx)), "SD i decay", "s16",               0,       0, INT_MAX  ,   1, },
    { ((uint32_t)(&(nvm_rx.spindown_limit)) - (uint32_t)(&nvm_rx)), "SD limit", "s16",                               200,       0, INT_MAX  ,   1, },
    //{ ((uint32_t)(&(nvm_rx.weapon2.center    )) - (uint32_t)(&nvm_rx)), "W2 center"  , "s16"    ,        ROACH_SERVO_MIN,                   ROACH_SERVO_MIN,                   ROACH_SERVO_MAX, 1, },
    //{ ((uint32_t)(&(nvm_rx.weapon2.deadzone  )) - (uint32_t)(&nvm_rx)), "W2 deadzone", "s16"    ,        ROACH_ADC_NOISE,                                 0,                   ROACH_SERVO_OVR, 1, },
    //{ ((uint32_t)(&(nvm_rx.weapon2.trim      )) - (uint32_t)(&nvm_rx)), "W2 trim"    , "s32"    ,                      0,                                 0,                   ROACH_SERVO_OVR, 1, },
    //{ ((uint32_t)(&(nvm_rx.weapon2.scale     )) - (uint32_t)(&nvm_rx)), "W2 scale"   , "s16x10" , ROACH_SCALE_MULTIPLIER,                                 0,                           INT_MAX, 1, },
    //{ ((uint32_t)(&(nvm_rx.weapon2.limit_min )) - (uint32_t)(&nvm_rx)), "W2 lim min" , "s16"    ,        ROACH_SERVO_MIN, ROACH_SERVO_MIN - ROACH_SERVO_OVR,                   ROACH_SERVO_MID, 1, },
    //{ ((uint32_t)(&(nvm_rx.weapon2.limit_max )) - (uint32_t)(&nvm_rx)), "W2 lim max" , "s16"    ,        ROACH_SERVO_MAX,                   ROACH_SERVO_MID, ROACH_SERVO_MAX + ROACH_SERVO_OVR, 1, },
    ROACH_NVM_GUI_DESC_END,
};

roach_nvm_gui_desc_t cfggroup_imu[] = {
    { ((uint32_t)(&(nvm_rx.imu_orientation  )) - (uint32_t)(&nvm_rx)), "IMU ori"     , "s8"     ,                      0,       0, 6 * 8 - 1,   1, },
    { ((uint32_t)(&(nvm_rx.heading_timeout  )) - (uint32_t)(&nvm_rx)), "IMU timeout" , "s16"    ,                   3000,       0, INT_MAX  , 100, },
    { ((uint32_t)(&(nvm_rx.pid_heading.p    )) - (uint32_t)(&nvm_rx)), "PID P"       , "s16x10" , ROACH_SCALE_MULTIPLIER, INT_MIN, INT_MAX  ,   1, },
    { ((uint32_t)(&(nvm_rx.pid_heading.i    )) - (uint32_t)(&nvm_rx)), "PID I"       , "s16x10" ,                      0, INT_MIN, INT_MAX  ,   1, },
    { ((uint32_t)(&(nvm_rx.pid_heading.d    )) - (uint32_t)(&nvm_rx)), "PID D"       , "s16x10" ,                      0, INT_MIN, INT_MAX  ,   1, },
    { ((uint32_t)(&(nvm_rx.pid_heading.output_limit))      - (uint32_t)(&nvm_rx)), "PID out lim",   "s16",       INT_MAX,       0, INT_MAX  ,   1, },
    { ((uint32_t)(&(nvm_rx.pid_heading.accumulator_limit)) - (uint32_t)(&nvm_rx)), "PID i lim"  ,   "s16",       INT_MAX,       0, INT_MAX  ,   1, },
    { ((uint32_t)(&(nvm_rx.pid_heading.accumulator_decay)) - (uint32_t)(&nvm_rx)), "PID i decay",   "s16",             0,       0, INT_MAX  ,   1, },
    ROACH_NVM_GUI_DESC_END,
};

roach_nvm_gui_desc_t** cfggroup_rxall = NULL;
int cfggroup_rxall_cnt = 0;

int roachnvm_cntgroup(roach_nvm_gui_desc_t* g)
{
    int cnt = 0;
    while (true)
    {
        roach_nvm_gui_desc_t* x = &(g[cnt]);
        if (x->name == NULL || x->name[0] == 0)
        {
            return cnt;
        }
        cnt += 1;
    }
}

void roachnvm_buildrxcfggroup(void)
{
    int cnt = 0;
    int c1 = roachnvm_cntgroup(cfggroup_drive);
    int c2 = roachnvm_cntgroup(cfggroup_weap);
    int c3 = roachnvm_cntgroup(cfggroup_imu);
    cnt += c1;
    cnt += c2;
    cnt += c3;
    cfggroup_rxall = (roach_nvm_gui_desc_t**)malloc((cnt + 1) * sizeof(void*));
    if (cfggroup_rxall == NULL) {
        return;
    }
    cfggroup_rxall_cnt = cnt;
    int i = 0, j;
    for (i = 0, j = 0; j < c1 && i < cfggroup_rxall_cnt; i++, j++)
    {
        cfggroup_rxall[i] = &(cfggroup_drive[j]);
    }
    for (j = 0; j < c2 && i < cfggroup_rxall_cnt; i++, j++)
    {
        cfggroup_rxall[i] = &(cfggroup_weap[j]);
    }
    for (j = 0; j < c3 && i < cfggroup_rxall_cnt; i++, j++)
    {
        cfggroup_rxall[i] = &(cfggroup_imu[j]);
    }
}

int roachnvm_rx_getcnt(void)
{
    return cfggroup_rxall_cnt;
}

roach_nvm_gui_desc_t* roachnvm_rx_getAt(int idx)
{
    return cfggroup_rxall[idx];
}
