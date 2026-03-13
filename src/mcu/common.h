#pragma once

#include "mcu_definition.h"
#include <stdbool.h>

// --- Global Initialization ---
// Bring up the active MCU-side protocol stack:
// ADS1115 I2C, GPIO-based SPI toward the FPGA, actuator GPIO,
// and the 250 Hz SysTick scheduler.
void System_Init(void);

// --- ADC Interface ---
// Blocking single-shot ADS1115 acquisition helper.
// Protocol: write config -> poll config OS bit -> read conversion register ->
// digitally remove the tracked baseline/DC component before returning the sample.
int16_t ADC_Read_Sample(void);

// Non-blocking ADS1115 acquisition state machine used by the scheduler.
// Protocol: keep one conversion in flight so each 250 Hz tick can harvest at
// most one finished sample without stalling the main loop, then baseline-correct
// that sample before the MCU forwards it to the FPGA.
bool Sampling_Task(int16_t *sample_out);

// One-shot I2C validation routine for the ADS1115 link.
// Protocol: write config, read config back, poll OS-ready, read conversion.
void I2C_Debug_BootProbe(void);

// Debug globals exported by the ADS1115 boot-probe sequence.
extern volatile uint32_t g_i2c_probe_stage;
extern volatile uint32_t g_i2c_probe_errors;
extern volatile uint32_t g_i2c_probe_ack;
extern volatile uint32_t g_i2c_probe_config_written;
extern volatile uint32_t g_i2c_probe_config_readback;
extern volatile uint32_t g_i2c_probe_status_word;
extern volatile uint32_t g_i2c_probe_conversion_word;
extern volatile uint32_t g_i2c_probe_poll_count;

// --- FPGA Interface ---
// Send one 16-bit ECG sample to the FPGA.
// Protocol: GPIO-generated SPI Mode 0, MSB first. Payload is already
// baseline-corrected by the MCU acquisition path.
bool FPGA_Send_Data(int16_t zero_centered_sample);
