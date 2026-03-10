#pragma once

#include <stdint.h>
#include "mcu_definition.h"

// Initialize the GPIO pin for the buzzer/LED (e.g., P0.1) as an output
void GPIO_Actuator_Init(void);
void GPIO_Debug_Init(void);

// Activate the buzzer/LED (e.g., if BPM is too high/low)
void Activate_Alarm(void);

// Deactivate the buzzer/LED
void Deactivate_Alarm(void);

// Visible firmware activity on XMC2Go user LEDs.
void GPIO_Debug_Scheduler_Tick(void);
void GPIO_Debug_SPI_Sent(void);
void GPIO_Debug_ShowI2CBusLevels(uint8_t sda_high, uint8_t scl_high);
