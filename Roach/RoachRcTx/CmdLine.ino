const cmd_def_t cmds[] = {
    { "factoryreset", factory_reset_func},
    { "nvmdebug"    , nvmdebug_func},
    { "nvmload"    , nvmload_func},
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
    { "dfu"         , dfuenter_func },
    { "fakebtn"     , fakebtn_func },
    { "fakepott"    , fakepott_func },
    { "fakepots"    , fakepots_func },
    { "fakepotw"    , fakepotw_func },
    { "fakeenc"     , fakeenc_func },
    { "hwreport"    , hwreport_func },
    { "rptenc"      , rptenc_func },
    { "rptbtnisr"   , rptbtnisr_func },
    { "rptpotcal"   , rptpotcal_func },
    { "rptbatt"     , rptbatt_func },
    { "", NULL }, // end of table
};

RoachCmdLine cmdline(&Serial, (cmd_def_t*)cmds, false, (char*)">>>", (char*)"???", true, 512);

void hwreport_func(void* cmd, char* argstr, Stream* stream)
{
    stream->printf("HW report loop\r\n");
    while (true)
    {
        RoachWdt_feed();
        hw_report();
    }
}

void factory_reset_func(void* cmd, char* argstr, Stream* stream)
{
    settings_factoryReset();
    if (atoi(argstr) == 123)
    {
        volatile uint32_t t_start = millis();
        volatile uint32_t t_end;
        if (settings_save()) {
            stream->println("factory reset performed and saved");
        }
        else {
            stream->println("factory reset saving failed");
        }
        t_end = millis();
        stream->printf("time taken = %u ms\r\n", t_end - t_start);
    }
    else
    {
        stream->println("factory reset performed");
    }
}

void nvmdebug_func(void* cmd, char* argstr, Stream* stream)
{
    stream->printf("flash JEDEC ID 0x%08X\r\n", RoachUsbMsd_getJedecId());
    stream->printf("FS can save? %u\r\n", RoachUsbMsd_canSave());
    stream->printf("flash size = %u\r\n", RoachUsbMsd_getFlashSize());
    if (RoachUsbMsd_canSave()) {
        stream->printf("FS free space = %u\r\n", RoachUsbMsd_getFreeSpace());
    }

    settings_debugNvm(stream);
    settings_debugListFiles(stream);
}

void nvmload_func(void* cmd, char* argstr, Stream* stream)
{
    if (argstr == NULL || strlen(argstr) == 0)
    {
        settings_loadFile(ROACH_STARTUP_CONF_NAME);
    }
    else
    {
        settings_loadFile(argstr);
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
    radio.contTxTest(f, false);
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

void dfuenter_func(void* cmd, char* argstr, Stream* stream)
{
    stream->printf("ENTERING DFU\r\n");
    waitFor(200);
    if (atoi(argstr) == 2) {
        enterSerialDfu();
    }
    else {
        enterUf2Dfu();
    }
}

void fakebtn_func(void* cmd, char* argstr, Stream* stream)
{
    bool done = false;
    if (memcmp("up", argstr, 1) == 0) {
        btn_up.fakePress();
        done = true;
    }
    if (memcmp("down", argstr, 1) == 0) {
        btn_down.fakePress();
        done = true;
    }
    if (memcmp("left", argstr, 1) == 0) {
        btn_left.fakePress();
        done = true;
    }
    if (memcmp("right", argstr, 1) == 0) {
        btn_right.fakePress();
        done = true;
    }
    if (memcmp("center", argstr, 1) == 0) {
        btn_center.fakePress();
        done = true;
    }
    if (memcmp("5", argstr, 1) == 0) {
        btn_g5.fakePress();
        done = true;
    }
    if (memcmp("6", argstr, 1) == 0) {
        btn_g6.fakePress();
        done = true;
    }
    if (memcmp("1", argstr, 1) == 0) {
        btn_sw1.fakeToggle();
        done = true;
    }
    if (memcmp("2", argstr, 1) == 0) {
        btn_sw2.fakeToggle();
        done = true;
    }
    if (memcmp("3", argstr, 1) == 0) {
        btn_sw3.fakeToggle();
        done = true;
    }
    if (done) {
        stream->printf("fake btn press %s\r\n", argstr);
    }
    else {
        stream->printf("fake btn %s error: not found\r\n", argstr);
    }
}

void fakepots_func(void* cmd, char* argstr, Stream* stream)
{
    int x = atoi(argstr);
    pot_steering.simulate(x);
    stream->printf("fake steering %d\r\n", x);
}

void fakepott_func(void* cmd, char* argstr, Stream* stream)
{
    int x = atoi(argstr);
    pot_throttle.simulate(x);
    stream->printf("fake throttle %d\r\n", x);
}

void fakepotw_func(void* cmd, char* argstr, Stream* stream)
{
    int x = atoi(argstr);
    pot_weapon.simulate(x);
    stream->printf("fake weapon %d\r\n", x);
}

void fakeenc_func(void* cmd, char* argstr, Stream* stream)
{
    int x = atoi(argstr);
    RoachEnc_simulate(x);
    stream->printf("fake encoder %d\r\n", x);
}

void rptenc_func(void* cmd, char* argstr, Stream* stream)
{
    int ar = atoi(argstr);
    RoachEnc_task();
    int enc = RoachEnc_get(false);
    stream->printf("[%u] enc = %d\r\n", millis(), enc);
    int prev_enc = enc;

    while (ar == 2)
    {
        RoachWdt_feed();
        RoachEnc_task();
        enc = RoachEnc_get(false);
        if (enc != prev_enc) {
            stream->printf("[%u] enc = %d\r\n", millis(), enc);
        }
        prev_enc = enc;
    }
}

void rptbtnisr_func(void* cmd, char* argstr, Stream* stream)
{
    #ifdef ROACHBUTTON_TRACK_ISR_RATE
    stream->printf("[%u] RoachButton ISR    rate %u    total %u\r\n", millis(), RoachButton_getIsrRate(), RoachButton_getIsrTotal());
    #else
    stream->println("ERROR: ROACHBUTTON_TRACK_ISR_RATE is not configured");
    #endif
}

void rptrosync_func(void* cmd, char* argstr, Stream* stream)
{
    rosync_debugState(stream);
}

void rptpotcal_func(void* cmd, char* argstr, Stream* stream)
{
    debug_pot_calib_all();
}

void rptbatt_func(void* cmd, char* argstr, Stream* stream)
{
    stream->printf("[%u]: batt ADC %u\r\n", millis(), pot_battery.getAdcRaw());
}