#pragma once

#include "mcu_definition.h"
#include <stdbool.h>

// --- Global Initialization ---
void System_Init(void); // Calls USIC/SysTick/GPIO initializations

// --- ADC Interface ---
// Reads a single 16-bit sample from the ADS1115 (Zero-Centered)
int16_t ADC_Read_Sample(void);
bool Sampling_Task(int16_t *sample_out);
void I2C_Debug_BootProbe(void);

extern volatile uint32_t g_i2c_probe_stage;
extern volatile uint32_t g_i2c_probe_errors;
extern volatile uint32_t g_i2c_probe_ack;
extern volatile uint32_t g_i2c_probe_config_written;
extern volatile uint32_t g_i2c_probe_config_readback;
extern volatile uint32_t g_i2c_probe_status_word;
extern volatile uint32_t g_i2c_probe_conversion_word;
extern volatile uint32_t g_i2c_probe_poll_count;

// --- FPGA Interface ---
// Transmits the 16-bit zero-centered sample to the FPGA over SPI
bool FPGA_Send_Data(int16_t zero_centered_sample);

// --- PC Terminal Interface ---
// Transmits a complete, filtered data packet to the PC via UART
void PC_Transmit_Filtered_Data(int16_t filtered_data, uint16_t bpm);
