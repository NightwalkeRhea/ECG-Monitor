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


typedef enum {      // to say which port.
    GPIO_PORT0, GPIO_PORT1, GPIO_PORT2
} gpio_port_t;
typedef enum {      // to say which port.
    GPIO_MODE_OUTPUT_PUSH_PULL = 0x80UL,		                      /**< Push-pull general-purpose output */
     GPIO_MODE_OUTPUT_OPEN_DRAIN = 0xc0UL, 	                      /**< Open-drain general-purpose output */
    GPIO_PORT2
} gpio_mode_t;

typedef struct gpio_config_t
{
    gpio_mode_t mode;
    uint8_t pinNumber;
};

/*
The port input/output control registers select the digital output and input driver
functionality and characteristics of a GPIO port pin. Port direction (input or output), pullup or pull-down devices for inputs, and push-pull or open-drain functionality for outputs
can be selected by the corresponding bit fields PCx (x = 0-15). Each 32-bit wide port
input/output control register controls four GPIO port lines:
Register Pn_IOCR0 controls the Pn.[3:0] port lines
Register Pn_IOCR4 controls the Pn.[7:4] port lines
Register Pn_IOCR8 controls the Pn.[11:8] port lines
Register Pn_IOCR12 controls the Pn.[15:12] port lines

//10000B
//1) For the analog and digital input port P2.2 - P2.9, the combinations with PCx[4]=1B is reserved

Field Bits Type Description
Px
(x = 0-6)
x rwh Port 1 Output Bit x
This bit determines the level at the output pin P1.x if
the output is selected as GPIO output.
0B The output level of P1.x is 0.
1B The output level of P1.x is 1.
P1.x can also be se
*/

// I put raw uint32_t for function prototyping, 
// void gpio_init(gpio_port_t *port, uint8_t pin, gpio_config_t *const cfg);




// Initialize the GPIO pin for the buzzer/LED (e.g., P0.1) as an output
void GPIO_Actuator_Init(void);

// Activate the buzzer/LED (e.g., if BPM is too high/low)
void Activate_Alarm(void);

// Deactivate the buzzer/LED
void Deactivate_Alarm(void);
