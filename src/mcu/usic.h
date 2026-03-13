#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "mcu_definition.h"

// Enable the shared USIC peripheral clock before touching any channel register.
void USIC_Clock_Enable(void);

// Initialize the active ADS1115 I2C master path.
void USIC_I2C_Init(void);
// Legacy CH1 SPI setup kept as a reference path.
void USIC_SPI_Init(void);

// --- I2C Low-Level Primitives ---
// These functions emit the actual bus phases used by the ADS1115 protocol.
void USIC_I2C_StartWrite(uint8_t slave_addr, uint8_t reg_addr);
void USIC_I2C_SendByte(uint8_t data, uint8_t stop_condition);
uint8_t USIC_I2C_ReadByte(uint8_t ack_condition, uint8_t stop_condition);
void USIC_I2C_RepeatedStartRead(uint8_t slave_addr);
uint32_t USIC_I2C_GetErrorFlags(void);
void USIC_I2C_ClearErrorFlags(void);

// --- Legacy SPI Primitive ---
bool USIC_SPI_Transfer(uint16_t data, uint8_t cs_select);
