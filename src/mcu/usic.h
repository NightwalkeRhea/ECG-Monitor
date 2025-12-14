#pragma once
#include <stdint.h>
#include "mcu_definition.h"


// --- USIC Register Abstraction (for USIC0_CH0/CH1) ---
// Using base addresses defined in mcu_definition.h 

// Function to enable the USIC module clock
void USIC_Clock_Enable(void);

// Initialize USIC Channel 0 for I2C Master operation (ADC interface)
void USIC_I2C_Init(void);
// Initialize USIC Channel 0 for UART Asynchronous operation (PC Terminal), just in case.
void USIC_UART_Init(void);
// Initialize USIC Channel 1 for SPI Master operation (FPGA interface)
void USIC_SPI_Init(void);

// --- Low-Level Protocol Functions (used internally by common_api.h) ---

// I2C Functions (Master)
void USIC_I2C_StartWrite(uint8_t slave_addr, uint8_t reg_addr);
void USIC_I2C_SendByte(uint8_t data, uint8_t stop_condition);
uint8_t USIC_I2C_ReadByte(uint8_t ack_condition, uint8_t stop_condition);
void USIC_I2C_RepeatedStartRead(uint8_t slave_addr);

// SPI Functions (Master)
uint16_t USIC_SPI_Transfer(uint16_t data, uint8_t cs_select);

// UART Functions (Transmit)
void USIC_UART_Transmit(uint8_t data);
// These functions switch the protocol mode for USIC channel0
void USIC_SetMode_I2C_CH0(void);
void USIC_SetMode_UART_CH0(void);