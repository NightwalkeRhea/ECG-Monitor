// clk_systick.c

#include "clock_systick.h"

// Note: Ensure USIC0 clock is enabled in a system initialization function 
// before calling USIC/common.c functions from the ISR. 
// TODO: How?

void SysTick_Init(void) {
    // Explanation: Initializes the SysTick timer to generate an interrupt 
    // at the required 250 Hz sampling frequency (1 / 250 Hz = 4 ms period).
    // The XMC1100's peripheral clock (PERIPHERAL_CLK_FREQ, typically 64 MHz) is the source.

    // 1. Set Reload Value: (64,000,000 / 250) - 1
    // The timer counts down from SYSTICK_RELOAD_VALUE + 1.
    SYST_RVR = SYSTICK_RELOAD_VALUE - 1; 

    // 2. Clear Current Value, for making sure that the timer starts counting from the maximum value.
    SYST_CVR = 0; 
    // A write of any value clears the field to 0, and also clears the SYST_CSR.COUNTFLAG bit to 0.

    // 3. Configure Control and Status Register
    // Bit 0: ENABLE = 1 (Enable the counter)
    // Bit 1: TICKINT = 1 (Enable SysTick exception request)-> In software, COUNTFLAG bit can be used to determine if SysTick has counted to zero.(bit 16)
    // Bit 2: CLKSOURCE = 1 (Use core clock/PERIPHERAL_CLK_FREQ as clock source)
    SYST_CSR = (1U << 2) | (1U << 1) | (1U << 0);
    /*
    When ENABLE is set to 1, the counter loads the RELOAD value from the SYST_RVR
    register and then counts down. On reaching 0, it sets the COUNTFLAG to 1 and
    optionally asserts the SysTick depending on the value of TICKINT. It then loads the
    RELOAD value again, and begins counting.
    */
    // The SysTick interrupt is automatically enabled in the NVIC when TICKINT is set.
}

// System-defined interrupt handler (must match startup file name)
void SysTick_Handler(void) {
    // This is the actual interrupt service routine (ISR) that runs every 4 ms (250 Hz).
    // It calls the main application logic handler.
    Sampling_ISR_Handler();
}

// User-defined sampling handler (to be implemented in main.c)
void Sampling_ISR_Handler(void) {
    // TODO
    // Placeholder for application logic (ADC read, data processing, SPI send, actuation check).
    // This function must be implemented in main.c or a separate data processing file.
}