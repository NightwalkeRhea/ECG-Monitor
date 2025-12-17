// clock_systick.c
#include "clock_systick.h"
#include <stdint.h>

volatile uint32_t g_systick_ticks = 0;   // increments every SysTick (4ms)
volatile uint8_t  g_sample_tick   = 0;   // flag for main loop (optional)

void SysTick_Init(void)
{
    // SYSTICK_RELOAD_VALUE should be defined as (PERIPHERAL_CLK_FREQ / 250U)
    // so the interrupt rate is 250 Hz (4 ms).
    SYST_RVR = (uint32_t)(SYSTICK_RELOAD_VALUE - 1U);
    SYST_CVR = 0U;

    // CLKSOURCE=1 (core clock), TICKINT=1 (interrupt), ENABLE=1
    SYST_CSR = (1U << 2) | (1U << 1) | (1U << 0);
}

void SysTick_Handler(void)
{
    g_systick_ticks++;
    g_sample_tick = 1U;          // let main loop run sampling work
    // DO NOT do long I2C/SPI transactions here if you can avoid it.
}

// User-defined sampling handler (to be implemented in main.c)
void Sampling_ISR_Handler(void) {
    // TODO
    // Placeholder for application logic (ADC read, data processing, SPI send, actuation check).
    // This function must be implemented in main.c or a separate data processing file.
}