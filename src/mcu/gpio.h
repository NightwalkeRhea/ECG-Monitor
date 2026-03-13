#pragma once

#include <stdint.h>

// Configure the two XMC2Go board LEDs used for firmware/debug visibility.
void GPIO_Debug_Init(void);

// Expose scheduler activity on a board LED.
void GPIO_Debug_Scheduler_Tick(void);

// Expose successful sample pushes toward the FPGA on a board LED.
void GPIO_Debug_SPI_Sent(void);
