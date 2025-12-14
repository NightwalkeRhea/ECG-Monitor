#pragma once

#include <stdint.h>
// This header file will serve as a Centralized header for all peripheral base addresses and fundamental constants.

// -----------------------------------------------------------------------------
// MCU CORE AND SYSTEM DEFINITIONS (XMC1100)
// -----------------------------------------------------------------------------

// System Clock Definition (Assumption: 64 MHz Peripheral Clock frequency)
#define PERIPHERAL_CLK_FREQ    64000000UL // 64MHz DCO1, ranging from 125 kHz to 64 MHz. 
#define SAMPLE_RATE_HZ         250UL  // adjust with ADS1115 
#define SYSTICK_RELOAD_VALUE   (PERIPHERAL_CLK_FREQ / SAMPLE_RATE_HZ)

// Base Addresses for Major Peripherals
#define SCU_BASE               0x40010000UL // System Control Unit, end address 4001 FFFFH

// The XMC1100 has 35 digital General Purpose Input/Output (GPIO) port lines which are
// connected to the on-chip peripheral units.
#define PORT0_BASE             0x40040000UL // GPIO Port 0 Base 
#define PORT1_BASE             0x40040100UL // GPIO Port 1 Base, high current bi-directional pad
#define USIC0_CH0_BASE         0x48000000UL // USIC Channel 0 (for I2C, UART), end address 480001FFH
#define USIC0_CH1_BASE         0x48000200UL // USIC Channel 1 (for SPI), end address 480003FFH
#define SYST_BASE              0xE000E000UL // Cortex-M0 SysTick Timer Base 
#define NVIC_BASE              0xE000E100UL // Nested Vectored Interrupt Controller Base 

// -----------------------------------------------------------------------------
// CORE PERIPHERAL REGISTER POINTERS
// -----------------------------------------------------------------------------

// SCU Register for Peripheral Clock Gating
#define SCU_CGATCLR0           (*(volatile uint32_t*)(SCU_BASE + 0x10UL))
#define SCU_CGATSET0           (*(volatile uint32_t*)(SCU_BASE + 0x0CUL))
// GPIO Control Registers
#define P0_IOCR0               (*(volatile uint32_t*)(PORT0_BASE + 0x10UL)) // P0.0 to P0.3 
#define P0_IOCR8               (*(volatile uint32_t*)(PORT0_BASE + 0x18UL)) // P0.8 to P0.11
#define P0_IOCR12              (*(volatile uint32_t*)(PORT0_BASE + 0x1CUL)) // P0.12 to P0.15
#define P1_IOCR0               (*(volatile uint32_t*)(PORT1_BASE + 0x10UL)) // P1.0 to P1.3


// General Output Registers (for setting/clearing pins)
#define P0_OUT                 (*(volatile uint32_t*)(PORT0_BASE + 0x00UL))
#define P0_OMR                 (*(volatile uint32_t*)(PORT0_BASE + 0x04UL)) // Output Modification Register

// SysTick Registers (Cortex-M0 Private Peripheral Bus, PPB)
// SysTick control and status register, bit 2 will determine the systick timer clock source. if 0, External clock, 1 Processor clock
#define SYST_CSR               (*(volatile uint32_t*)(SYST_BASE + 0x010)) // Control and Status Register 
#define SYST_RVR               (*(volatile uint32_t*)(SYST_BASE + 0x014)) // Reload Value Register 
#define SYST_CVR               (*(volatile uint32_t*)(SYST_BASE + 0x018)) // Current Value Register 

// NVIC Registers (used for interrupt control)
#define NVIC_ISER              (*(volatile uint32_t*)(NVIC_BASE + 0x000)) // Interrupt Set Enable Register 

