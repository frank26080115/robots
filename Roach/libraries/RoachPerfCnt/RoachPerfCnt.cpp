#include "RoachPerfCnt.h"

static uint32_t cnt = 0;
static uint32_t val = 0;
static uint32_t t   = 0;
static uint32_t minimum_ram = 0xFFFFFF;

static uint32_t PerfCnt_measureFreeRam(void);

void PerfCnt_init(void)
{
}

#ifdef PERFCNT_ENABLED

void PerfCnt_task(void)
{
    PerfCnt_measureFreeRam();
    cnt++;
    uint32_t now = millis();
    if ((now - t) >= 1000)
    {
        t = now;
        val = cnt;
        cnt = 0;
    }
}

int PerfCnt_get(void)
{
    return val;
}

extern void* _sbrk(int);

static uint32_t PerfCnt_measureFreeRam(void)
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

int PerfCnt_ram(void)
{
    return minimum_ram;
}

#endif
