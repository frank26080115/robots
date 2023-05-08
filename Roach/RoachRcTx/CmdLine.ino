const cmd_def_t cmds[] = {
    { "factoryreset", factory_reset_func},
    { "echo"        , echo_func },
    { "mem"         , memcheck_func },
    { "reboot"      , reboot_func },
    { "debug"       , debug_func },
    { "conttx"      , conttx_func },
    { "regenrf"     , regenrf_func },
    { "readrf"      , readrf_func },
    { "save"        , save_func },
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
