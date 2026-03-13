// main.c

#include "common.h"
#include "gpio.h"
#include "clock_systick.h"
#include <stdbool.h>

#ifndef __WFI
    #define __WFI() __asm volatile ("wfi")
#endif

#ifndef __disable_irq
    #define __disable_irq() __asm volatile ("cpsid i" : : : "memory")
#endif

/* (changes CPSR i with a memory clobber) which enables 
(unmasks) the processor’s interrupt flag. */
#ifndef __enable_irq
    #define __enable_irq() __asm volatile ("cpsie i" : : : "memory")
#endif

//the compiler shouldn't optimize away reads,
// as the variable is modified externally by the ISR.
volatile int16_t g_raw_adc_sample = 0; 


// Periodic sampling, triggered by the 250 Hz SysTick flag

/*Main sampling and processing handler, called from the main loop whenever the SysTick flag indicates a new sample period.
  This function organizes the real-time data flow of ADC Read -> SPI Send.*/
void Sampling_ISR_Handler(void){
    int16_t sample = 0;

    GPIO_Debug_Scheduler_Tick(); //used only in sw debugging

    //Read Analog Data (I2C Master to ADS1115 Slave), returns true only when a fresh conversion
    // is ready, so we can keep each scheduler step bounded in time.
    if (!Sampling_Task(&sample)) {
        return;
    }
    g_raw_adc_sample = sample;

    // Send baseline-corrected data to the FPGA
    if (FPGA_Send_Data(g_raw_adc_sample)) {
        GPIO_Debug_SPI_Sent();
    }
}


int main(void) {
    // System Initialization
    System_Init();
    // Temporary ADS1115 boot probe hook, kept here for fallback
    // I2C_Debug_BootProbe();
    // while (true) {
    // __WFI(); 
    // }

    while (true) {
        uint32_t pending_ticks = 0U;

        __disable_irq();
        pending_ticks = g_sample_tick;
        g_sample_tick = 0U;
        __enable_irq();

        while (pending_ticks-- != 0U) {
            Sampling_ISR_Handler();
        }

        // Avoid sleeping if a new tick arrived
        if (g_sample_tick == 0U){
            //low-power Sleep mode. The core will immediately wake up when the
            //SysTick ISR (at 250 Hz) or any other enabled interrupt occurs.
            __WFI();
        }
        // After the ISR, execution returns here and waits for the next interrupt
    }

    // Should never be reached
    return 0;
}
