// main.c

#include "common.h"
#include "gpio.h"
#include "mcu_definition.h"
#include <stdbool.h>

// --- Global System State Variables ---
// These variables store the latest data, accessible by both the ISR and the main loop.

// Volatile keyword ensures the compiler doesn't optimize away reads, 
// as the variable is modified externally by the ISR.
volatile int16_t g_raw_adc_sample = 0; 
volatile int16_t g_filtered_data = 0;   // Placeholder for data received back from FPGA/DSP
volatile uint16_t g_current_bpm = 72;    // Current measured heart rate (BPM)
const uint16_t BPM_THRESHOLD = 120;    // Alarm threshold for tachycardia (e.g., 120 BPM)


// --- Placeholder Function for FPGA Output ---
// In a real system, this function would handle reading the final processed data 
// (e.g., filtered sample and BPM result) from the FPGA via SPI MISO or dedicated registers.

/**
 * @brief Reads processed data and BPM from the FPGA.
 * @return Simulated BPM value.
 */
static uint16_t FPGA_BPM_Read(int16_t raw_sample, int16_t *filtered_out) {
    // --- SIMULATION/PLACEHOLDER LOGIC ---
    // In a final embedded system:
    // 1. MCU might poll an FPGA register via SPI read command.
    // 2. MCU might wait for a dedicated IRQ pin from the FPGA signaling a new BPM result.
    
    // For this demonstration, we simulate the FPGA processing:
    // Simple digital filter simulation (crude low-pass by averaging)
    static int16_t previous_filtered = 0;
    *filtered_out = (raw_sample + previous_filtered) / 2;
    previous_filtered = *filtered_out;

    // Simulated detection logic: trigger a high BPM momentarily if the raw sample exceeds a peak.
    if (raw_sample > 1500) { 
        return 130; // Simulated abnormal heart rate (Tachycardia)
    } else {
        return 75; // Simulated normal heart rate
    }
}


// ----------------------------------------------------------------------
// SysTick Interrupt Service Routine Handler (Executed at 250 Hz)
// ----------------------------------------------------------------------

/**
 * @brief Main sampling and processing handler, executed precisely by the SysTick ISR.
 * 
 * This function orchestrates the real-time data flow: 
 * ADC Read -> SPI Send -> Actuation Check -> UART Transmit.
 */
void Sampling_ISR_Handler(void) {
    // 1. Read Analog Data (I2C Master to ADS1115 Slave)
    // The result is a 16-bit signed integer, zero-centered (DC offset removed).
    g_raw_adc_sample = ADC_Read_Sample(); 

    // 2. Send Raw Data to FPGA for real-time DSP (SPI Master to FPGA Slave)
    // The FPGA starts processing (Filtering, Squaring, MWI).
    FPGA_Send_Data(g_raw_adc_sample);

    // 3. Receive Processed Results (Simulated FPGA Output Read)
    // The FPGA provides the calculated filtered waveform and the final BPM result.
    g_current_bpm = FPGA_BPM_Read(g_raw_adc_sample, &g_filtered_data); 

    // 4. Actuation Logic (Check for Abnormal Heart Rate)
    if (g_current_bpm > BPM_THRESHOLD) {
        // Heart rate is too high: Activate the passive buzzer/LED.
        Activate_Alarm();
    } else {
        // Heart rate is normal: Ensure the alarm is off.
        Deactivate_Alarm();
    }
    
    // 5. Transmit Filtered Data and BPM to PC (UART/ASC)
    // This allows the user to plot the data and monitor the heart rate in a terminal.
    PC_Transmit_Filtered_Data(g_filtered_data, g_current_bpm);
}


// ----------------------------------------------------------------------
// Main Application Entry Point
// ----------------------------------------------------------------------

int main(void) {
    // 1. System Initialization
    // Calls USIC_Clock_Enable(), USIC_I2C_Init(), USIC_SPI_Init(), USIC_UART_Init(), 
    // GPIO_Actuator_Init(), and SysTick_Init().
    // This function sets up all hardware and configures the 250 Hz sampling timer.
    System_Init();

    // 2. Main Execution Loop
    while (true) {
        // Use the WFI (Wait For Interrupt) instruction to put the Cortex-M0 core 
        // into a low-power Sleep mode. The core will immediately wake up when the 
        // SysTick ISR (at 250 Hz) or any other enabled interrupt occurs.
        // This dramatically reduces power consumption while maintaining real-time responsiveness.
        __WFI(); 
        
        // After the ISR runs, execution returns here to wait for the next interrupt.
    }

    // Should never be reached
    return 0;
}