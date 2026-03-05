#pragma once

#include <stdint.h>
#include "mcu_definition.h"

// Initialize the GPIO pin for the buzzer/LED (e.g., P0.1) as an output
void GPIO_Actuator_Init(void);

// Activate the buzzer/LED (e.g., if BPM is too high/low)
void Activate_Alarm(void);

// Deactivate the buzzer/LED
void Deactivate_Alarm(void);
