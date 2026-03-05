// clock_systick.c
#include "clock_systick.h"
#include <stdint.h>

volatile uint32_t g_systick_ticks = 0;   // increments every SysTick (4ms)
volatile uint32_t g_sample_tick   = 0;   // pending sample periods for main loop

void SysTick_Init(void)
{
    // SYSTICK_RELOAD_VALUE is derived from the CPU clock because CLKSOURCE=1
    // selects the processor clock for the SysTick counter.
    SYST_RVR = (uint32_t)(SYSTICK_RELOAD_VALUE - 1U);
    SYST_CVR = 0U;

    // CLKSOURCE=1 (core clock), TICKINT=1 (interrupt), ENABLE=1
    SYST_CSR = (1U << 2) | (1U << 1) | (1U << 0);
}

void SysTick_Handler(void)
{
    g_systick_ticks++;
    if (g_sample_tick != 0xFFFFFFFFUL) {
        g_sample_tick++;
    }
    // DO NOT do long I2C/SPI transactions here if you can avoid it.
}
