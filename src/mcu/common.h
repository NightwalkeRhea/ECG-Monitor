#pragma once

#include "mcu_definition.h"
#include <stdbool.h>

// --- Global Initialization ---
void System_Init(void); // Calls USIC/SysTick/GPIO initializations

// --- ADC Interface ---
// Reads a single 16-bit sample from the ADS1115 (Zero-Centered)
int16_t ADC_Read_Sample(void);
bool Sampling_Task(int16_t *sample_out);

// --- FPGA Interface ---
// Transmits the 16-bit zero-centered sample to the FPGA over SPI
void FPGA_Send_Data(int16_t zero_centered_sample);

// --- PC Terminal Interface ---
// Transmits a complete, filtered data packet to the PC via UART
void PC_Transmit_Filtered_Data(int16_t filtered_data, uint16_t bpm);
