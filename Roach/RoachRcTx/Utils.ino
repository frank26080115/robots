extern void* _sbrk(int);

uint32_t minimum_ram = INT_MAX;

uint32_t getFreeRam(void)
{
    // post #9 from http://forum.pjrc.com/threads/23256-Get-Free-Memory-for-Teensy-3-0
    uint32_t stackTop;
    uint32_t heapTop;

    // current position of the stack.
    stackTop = (uint32_t) &stackTop;
    stackTop &= 0xFFFFFF;

    // current position of heap.
    void* hTop = malloc(1);
    if (hTop == NULL) {
        return 0;
    }
    heapTop = (uint32_t) hTop;
    free(hTop);
    heapTop &= 0xFFFFFF;

    // The difference is the free, available ram.
    uint32_t x;
    if (stackTop > heapTop) {
        x = (stackTop - heapTop);// - current_stack_depth();
    }
    else {
        x = 256000 - heapTop;
    }
    minimum_ram = x < minimum_ram ? x : minimum_ram;
    return x;
}

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
    uint8_t f;
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