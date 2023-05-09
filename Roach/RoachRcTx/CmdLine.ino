const cmd_def_t cmds[] = {
    { "factoryreset", factory_reset_func},
    { "echo"        , echo_func },
    { "mem"         , memcheck_func },
    { "reboot"      , reboot_func },
    { "debug"       , debug_func },
    { "initgfx"     , initgfx_func },
    { "conttx"      , conttx_func },
    { "regenrf"     , regenrf_func },
    { "readrf"      , readrf_func },
    { "save"        , save_func },
    { "fakebtn"     , fakebtn_func },
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

extern uint32_t minimum_ram;
void memcheck_func(void* cmd, char* argstr, Stream* stream)
{
    stream->printf("free mem: %u , min: %u\r\n", getFreeRam(), minimum_ram);
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
    if (memcmp("up", argstr, 1) == 0) {
        btn_up.fakePress();
    }
    if (memcmp("down", argstr, 1) == 0) {
        btn_down.fakePress();
    }
    if (memcmp("left", argstr, 1) == 0) {
        btn_left.fakePress();
    }
    if (memcmp("right", argstr, 1) == 0) {
        btn_right.fakePress();
    }
    if (memcmp("center", argstr, 1) == 0) {
        btn_center.fakePress();
    }
    if (memcmp("5", argstr, 1) == 0) {
        btn_g5.fakePress();
    }
    if (memcmp("6", argstr, 1) == 0) {
        btn_g6.fakePress();
    }
}

void initgfx_func(void* cmd, char* argstr, Stream* stream)
{
    stream->printf("init gui\r\n");
    gui_init();
}
