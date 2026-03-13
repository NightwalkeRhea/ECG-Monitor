#pragma once
// This module sets up the 250Hz timing signal using the integrated Cortex-M0 SysTick timer.
#include "mcu_definition.h"

// Configure SysTick as the 250 Hz global sampling heartbeat.
void SysTick_Init(void);

// Free-running count of all SysTick interrupts since boot.
extern volatile uint32_t g_systick_ticks;
// Number of pending 250 Hz scheduler slots waiting to be drained by main().
extern volatile uint32_t g_sample_tick;

// Application work function consumed once for each pending sample slot.
void Sampling_ISR_Handler(void);

