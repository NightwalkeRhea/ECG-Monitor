#pragma once
// This module sets up the 250Hz timing signal using the integrated Cortex-M0 SysTick timer.
#include "mcu_definition.h"

// Initialize the SysTick timer for the required sampling rate (250 Hz)
void SysTick_Init(void);

extern volatile uint32_t g_systick_ticks;
extern volatile uint32_t g_sample_tick;

// User-defined handler called by the actual SysTick_Handler IRQ
void Sampling_ISR_Handler(void);

