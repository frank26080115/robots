#include <Arduino.h>

#if NRFX_CHECK(NRFX_DELAY_DWT_BASED)

#if !NRFX_DELAY_DWT_PRESENT
#error "DWT unit not present in the SoC that is used."
#endif

void nrfows_delayloops(uint32_t loops)
{
    // Save the current state of the DEMCR register to be able to restore it before exiting
    // this function. Enable the trace and debug blocks (including DWT).
    uint32_t core_debug = CoreDebug->DEMCR;
    CoreDebug->DEMCR = core_debug | CoreDebug_DEMCR_TRCENA_Msk;

    // Save the current state of the CTRL register in the DWT block. Make sure
    // that the cycle counter is enabled.
    uint32_t dwt_ctrl = DWT->CTRL;
    DWT->CTRL = dwt_ctrl | DWT_CTRL_CYCCNTENA_Msk;

    // Store start value of the cycle counter.
    uint32_t cyccnt_initial = DWT->CYCCNT;

    // Delay required time.
    while ((DWT->CYCCNT - cyccnt_initial) < loops)
    {}

    // Restore preserved registers.
    DWT->CTRL = dwt_ctrl;
    CoreDebug->DEMCR = core_debug;
}

#else // NRFX_CHECK(NRFX_DELAY_DWT_BASED)

// NRFX_DELAY_CPU_FREQ_MHZ is 64
// 1000000 loops is 15.69266 ms

void nrfows_delayloops(uint32_t loops)
{
    // Allow overriding the number of cycles per loop iteration, in case it is
    // needed to adjust this number externally (for example, when the SoC is
    // emulated).
    #ifndef NRFX_COREDEP_DELAY_US_LOOP_CYCLES
        #if defined(NRF51)
            // The loop takes 4 cycles: 1 for SUBS, 3 for BHI.
            #define NRFX_COREDEP_DELAY_US_LOOP_CYCLES  4
        #elif defined(NRF52810_XXAA) || defined(NRF52811_XXAA)
            // The loop takes 7 cycles: 1 for SUBS, 2 for BHI, 2 wait states
            // for each instruction.
            #define NRFX_COREDEP_DELAY_US_LOOP_CYCLES  7
        #else
            // The loop takes 3 cycles: 1 for SUBS, 2 for BHI.
            #define NRFX_COREDEP_DELAY_US_LOOP_CYCLES  3
        #endif
    #endif // NRFX_COREDEP_DELAY_US_LOOP_CYCLES
    // Align the machine code, so that it can be cached properly and no extra
    // wait states appear.
    __ALIGN(16)
    static const uint16_t delay_machine_code[] = {
        0x3800 + NRFX_COREDEP_DELAY_US_LOOP_CYCLES, // SUBS r0, #loop_cycles
        0xd8fd, // BHI .-2
        0x4770  // BX LR
    };

    typedef void (* delay_func_t)(uint32_t);
    const delay_func_t delay_cycles =
        // Set LSB to 1 to execute the code in the Thumb mode.
        (delay_func_t)((((uint32_t)delay_machine_code) | 1));
    delay_cycles(loops);
}

#endif // !NRFX_CHECK(NRFX_DELAY_DWT_BASED_DELAY)
