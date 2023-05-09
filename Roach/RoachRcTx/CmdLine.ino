const cmd_def_t cmds[] = {
    { "factoryreset", factory_reset_func},
    { "echo"        , echo_func },
    { "mem"         , memcheck_func },
    { "perf"        , perfcheck_func },
    { "reboot"      , reboot_func },
    { "debug"       , debug_func },
    { "conttx"      , conttx_func },
    { "regenrf"     , regenrf_func },
    { "readrf"      , readrf_func },
    { "save"        , save_func },
    { "fakebtn"     , fakebtn_func },
    { "fakepott"    , fakepott_func },
    { "fakepots"    , fakepots_func },
    { "fakepotw"    , fakepotw_func },
    { "fakeenc"     , fakeenc_func },
    { "", NULL }, // end of table
};

RoachCmdLine cmdline(&Serial, (cmd_def_t*)cmds, false, (char*)">>>", (char*)"???", true, 512);

void factory_reset_func(void* cmd, char* argstr, Stream* stream)
{
    settings_factoryReset();
    settings_save();
    stream->println("factory reset performed");
}

void save_func(void* cmd, char* argstr, Stream* stream)
{
    settings_save();
    stream->println("settings saved");
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
    radio.cont_tx(f);
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
