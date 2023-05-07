extern void* _sbrk(int);

uint32_t minimum_ram = INT_MAX;

uint32_t getFreeRam(void)
{
    // post #9 from http://forum.pjrc.com/threads/23256-Get-Free-Memory-for-Teensy-3-0
    uint32_t stackTop;
    uint32_t heapTop;

    // current position of the stack.
    stackTop = (uint32_t) &stackTop;

    // current position of heap.
    void* hTop = malloc(1);
    heapTop = (uint32_t) hTop;
    free(hTop);

    // The difference is the free, available ram.
    uint32_t x = (stackTop - heapTop);// - current_stack_depth();
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
