// clock_systick.c
#include "clock_systick.h"
#include <stdint.h>

volatile uint32_t g_systick_ticks = 0;   // increments every SysTick (4ms)
volatile uint32_t g_sample_tick   = 0;   // pending sample periods for main loop

/**
 * @brief Initialize SysTick timer for 250 Hz ECG sampling.
 *        RELOAD = fCPU / 250 - 1
 *        At each tick, g_sample_tick increments, triggering Sampling_ISR_Handler
 */
void SysTick_Init(void)
{
    // SysTick Reload Value (SYST_RVR): sets timer period
    // SYSTICK_RELOAD_VALUE = fCPU / 250: CLKSOURCE=1 selects core clock
    SYST_RVR = (uint32_t)(SYSTICK_RELOAD_VALUE - 1U);

    // SysTick Current Value (SYST_CVR): reset counter
    SYST_CVR = 0U;

    // SysTick Control and Status (SYST_CSR):
    // Bit 2: CLKSOURCE=1 (core clock, not external reference)
    // Bit 1: TICKINT=1 (enable SysTick exception)
    // Bit 0: ENABLE=1 (start counter)
    SYST_CSR = (1U << 2) | (1U << 1) | (1U << 0);
}

void SysTick_Handler(void)
{
    // Increment total ticks counter for debugging/timing measurements
    g_systick_ticks++;

    // Increment pending sample count for main loop (saturates at max to prevent overflow)
    // Each pending tick triggers one call to Sampling_ISR_Handler()
    if (g_sample_tick != 0xFFFFFFFFUL) {
        g_sample_tick++;
    }
}
