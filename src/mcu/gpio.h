#pragma once

#include <stdint.h>
#include "mcu_definition.h"
#include "XMC1100.h"


// General Purpose I/O Port 0 Base Address
#define PORT0_BASE             0x40040000UL
#define PORT1_BASE              0x40040100UL
#define PORT1_END             0x400401FF

#define GPIO_P1_OUT_REG PORT1_BASE // port 1 output reg
#define GPIO_P1_OMR_REG PORT1_BASE + 0x0004 // output modif reg
#define PORT0_IOCR0_PC0_Pos                   (3UL)                     /*!< PORT0 IOCR0: PC0 (Bit 3)     
/*Pn_IOCR0 (n=0-1)
Port n Input/Output Control Register 0
(4004 0010H + n*100H) Reset Value: 0000 0000H
*/

#define GPIO_P1_IOCR0_REG GPIO_P1_BASE + 0x0010 // i/o ctrl reg0
#define GPIO_P1_IOCR4_REG GPIO_P1_BASE + 0x0014 // i/o ctrl reg4
#define GPIO_P1_IOCR8_REG GPIO_P1_BASE + 0x0018 
#define GPIO_P1_IOCR12_REG GPIO_P1_BASE + 0x001c 
#define GPIO_P1_IN_REG GPIO_P1_BASE + 0x0024 // port 1 input reg
#define GPIO_P1_HWSEL_REG GPIO_P1_BASE + 0x0074 // port 1 hardware select





// Initialize the GPIO pin for the buzzer/LED (e.g., P0.1) as an output
void GPIO_Actuator_Init(void);

// Activate the buzzer/LED (e.g., if BPM is too high/low)
void Activate_Alarm(void);

// Deactivate the buzzer/LED
void Deactivate_Alarm(void);
