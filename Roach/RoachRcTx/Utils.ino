bool btns_hasAnyPressed(void)
{
    bool x = false;

    x |= btn_up     .hasPressed(false);
    x |= btn_down   .hasPressed(false);
    x |= btn_left   .hasPressed(false);
    x |= btn_right  .hasPressed(false);
    x |= btn_center .hasPressed(false);
    x |= btn_g5     .hasPressed(false);
    x |= btn_g6     .hasPressed(false);

    return x;
}

bool btns_isAnyHeld(void)
{
    bool x = false;

    x |= btn_up     .isHeld() > 0;
    x |= btn_down   .isHeld() > 0;
    x |= btn_left   .isHeld() > 0;
    x |= btn_right  .isHeld() > 0;
    x |= btn_center .isHeld() > 0;
    x |= btn_g5     .isHeld() > 0;
    x |= btn_g6     .isHeld() > 0;

    return x;
}

void btns_clearAll(void)
{
    btn_up     .hasPressed(true);
    btn_down   .hasPressed(true);
    btn_left   .hasPressed(true);
    btn_right  .hasPressed(true);
    btn_center .hasPressed(true);
    btn_g5     .hasPressed(true);
    btn_g6     .hasPressed(true);
}

void btns_disableAll(void)
{
    btn_up     .disableUntilRelease();
    btn_down   .disableUntilRelease();
    btn_left   .disableUntilRelease();
    btn_right  .disableUntilRelease();
    btn_center .disableUntilRelease();
    btn_g5     .disableUntilRelease();
    btn_g6     .disableUntilRelease();
}

bool switches_alarm = false;
uint8_t switches_getFlags(void)
{
    static bool released = false;
    uint8_t f = 0;
    f |= btn_sw1.isHeld() > 0 ? ROACHPKTFLAG_BTN1 : 0;
    f |= btn_sw2.isHeld() > 0 ? ROACHPKTFLAG_BTN2 : 0;
    f |= btn_sw3.isHeld() > 0 ? ROACHPKTFLAG_BTN3 : 0;
    #ifdef ROACHHW_PIN_BTN_SW4
    f |= btn_sw4.isHeld() > 0 ? ROACHPKTFLAG_BTN4 : 0;
    #endif
    if (released == false) {
        if (f == (nvm_tx.startup_switches & nvm_tx.startup_switches_mask)) {
            switches_alarm = false;
            released = true;
        }
        else {
            f = nvm_tx.startup_switches;
            switches_alarm = true;
        }
    }
    return f;
}

void strncpy0(char* dest, const void* src, size_t n)
{
    unsigned int slen = strlen((char*)src);
    slen = (slen > n && n > 0) ? n : 0;
    strncpy(dest, (const char*)src, slen);
    dest[slen] = 0;
}

void waitFor(uint32_t x)
{
    uint32_t tstart = millis();
    while ((millis() - tstart) < x)
    {
        yield();
        ctrler_tasks();
    }
}

float batt_get(void)
{
    return 0;
}
