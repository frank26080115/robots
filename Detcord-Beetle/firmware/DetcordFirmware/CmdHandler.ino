extern void roachrobot_handleUploadLine(void* cmd, char* argstr, Stream* stream);
extern void roachrobot_handleFileSave(void* cmd, char* argstr, Stream* stream);
extern void roachrobot_handleFileLoad(void* cmd, char* argstr, Stream* stream);

const cmd_def_t cmds[] = {
    { "factoryreset", factory_reset_func},
    { "echo"        , echo_func },
    { "mem"         , memcheck_func },
    { "perf"        , perfcheck_func },
    { "reboot"      , reboot_func },
    { "debug"       , debug_func },
    { "usbmsd"      , usbmsd_func },
    { "unusbmsd"    , unusbmsd_func },
    { "conttx"      , conttx_func },
    { "regenrf"     , regenrf_func },
    { "readrf"      , readrf_func },
    { "save"        , save_func },
    { "esclink"     , esclink_func },
    { "readbatt"    , readbatt_func },
    { "readimu"     , readimu_func },
    { "rtmgrsim"    , rtmgrsim_func },
    { "rtmgrend"    , rtmgrend_func },

    { "cfgwrite"    , roachrobot_handleUploadLine },
    { "filesave"    , roachrobot_handleFileSave },
    { "fileload"    , roachrobot_handleFileLoad },

    { "", NULL }, // end of table
};

RoachCmdLine cmdline(&Serial, (cmd_def_t*)cmds, false, (char*)">>>", (char*)"???", true, 512);

void factory_reset_func(void* cmd, char* argstr, Stream* stream)
{
    settings_factoryReset();
    if (settings_save()) {
        stream->println("factory reset performed");
    }
    else {
        stream->println("factory reset failed");
    }
}

void save_func(void* cmd, char* argstr, Stream* stream)
{
    if (settings_save()) {
        stream->println("settings saved");
    }
    else {
        stream->println("settings save failed");
    }
}

void echo_func(void* cmd, char* argstr, Stream* stream)
{
    stream->println(argstr);
}

void reboot_func(void* cmd, char* argstr, Stream* stream)
{
    stream->println("rebooting...\r\n\r\n");
    delay(100);
    NVIC_SystemReset();
}

void usbmsd_func(void* cmd, char* argstr, Stream* stream)
{
    stream->printf("presenting USB MSD\r\n");
    waitFor(300);
    RoachUsbMsd_presentUsbMsd();
}

void unusbmsd_func(void* cmd, char* argstr, Stream* stream)
{
    stream->printf("disconnecting USB MSD\r\n");
    waitFor(300);
    RoachUsbMsd_unpresent();
}

void memcheck_func(void* cmd, char* argstr, Stream* stream)
{
    stream->printf("free mem: %u\r\n", PerfCnt_ram());
}

void perfcheck_func(void* cmd, char* argstr, Stream* stream)
{
    stream->printf("perf: %u / sec\r\n", PerfCnt_get());
}

void debug_func(void* cmd, char* argstr, Stream* stream)
{
    stream->printf("derp\r\n");
}

void conttx_func(void* cmd, char* argstr, Stream* stream)
{
    int f = atoi(argstr);
    stream->printf("RF cont-tx test f=%d\r\n", f);
    radio.contTxTest(f, false, RoachWdt_feed);
    // this is blocking forever, requires power-down to stop
}

void regenrf_func(void* cmd, char* argstr, Stream* stream)
{
    nvm_rf.uid  = nrf5rand_u32();
    nvm_rf.salt = nrf5rand_u32();
    stream->printf("new RF params 0x%08X 0x%08X\r\n", nvm_rf.uid, nvm_rf.salt);
}

void readrf_func(void* cmd, char* argstr, Stream* stream)
{
    stream->printf("RF params: 0x%08X 0x%08X 0x%08X\r\n", nvm_rf.uid, nvm_rf.salt, nvm_rf.chan_map);
}

void esclink_func(void* cmd, char* argstr, Stream* stream)
{
    stream->printf("ESC Link active\r\n");
    EscLink();
}

void readbatt_func(void* cmd, char* argstr, Stream* stream)
{
    bool forever = atoi(argstr);
    do
    {
        RoachPot_allTask();
        stream->printf("[%u] BATT: %u\r\n", millis(), battery.getAdcFiltered());
        if (forever) {
            waitFor(250);
        }
    }
    while (forever);
}

void readimu_func(void* cmd, char* argstr, Stream* stream)
{
    bool forever = atoi(argstr);
    do
    {
        nbtwi_task();
        imu.task();
        stream->printf("[%u] ROLL: %4.1f    PITCH: %4.1f    YAW: %4.1f\r\n", millis(), imu.euler.roll, imu.euler.pitch, imu.euler.yaw);
        if (forever) {
            waitFor(250);
        }
    }
    while (forever);
}

void rtmgrsim_func(void* cmd, char* argstr, Stream* stream)
{
    rtmgr_setSimulation(atoi(argstr) != 0);
}

void rtmgrend_func(void* cmd, char* argstr, Stream* stream)
{
    rtmgr_permEnd();
}
