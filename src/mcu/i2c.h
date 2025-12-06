#pragma once

#include <stdint.h>
#include "gpio.h"

// defining the address map

#define SCU_BASE 0x40010000UL         //system control unit, for clk ctrl

#define SCU_CGATCLR0     (*(volatile uint32_t*)(SCU_BASE + 0x0C)) //enable periph clk
#define SCU_CGATCTRL0_USIC0_Pos 20U     //defined in the library aswell. what's this?

// Channel 0 (USIC0_CH0) Base Address
#define USIC0_CH0_BASE         0x48000000UL






// -----------------------------------------------------------------------------
// MCU Register Base Addresses (XMC1100)
// -----------------------------------------------------------------------------

// System Control Unit (SCU) Base Address (for clock control)
#define SCU_BASE               (0x40010000UL)
// Clock Gating Clear Register 0: Used to enable peripheral clocks.
#define SCU_CGATCLR0           (*(volatile uint32_t*)(SCU_BASE + 0x0C))
// Bit position for USIC0 clock (Table 12-7 in SCU reference manual)
#define SCU_CGATCLR0_USIC0_Pos (20U)


// Port 0 Input/Output Control Register 12 (Controls P0.12 to P0.15)
#define P0_IOCR12              (*(volatile uint32_t*)(PORT0_BASE + 0x1CUL))

// -----------------------------------------------------------------------------
// USIC0_CH0 Register Offsets
// -----------------------------------------------------------------------------

// Kernel State Configuration Register (for enabling/disabling the module)
#define USIC_KSCFG             (*(volatile uint32_t*)(USIC0_CH0_BASE + 0x0C))
// Fractional Divider Register (for clock generation)
#define USIC_FDR               (*(volatile uint32_t*)(USIC0_CH0_BASE + 0x10))
// Baud Rate Generator Register
#define USIC_BRG               (*(volatile uint32_t*)(USIC0_CH0_BASE + 0x14))
// Input Control Register 0 (for SDA)
#define USIC_DX0CR             (*(volatile uint32_t*)(USIC0_CH0_BASE + 0x1C))
// Input Control Register 1 (for SCL)
#define USIC_DX1CR             (*(volatile uint32_t*)(USIC0_CH0_BASE + 0x20))
// Shift Control Register (frame format: word length, MSB/LSB first)
#define USIC_SCTR              (*(volatile uint32_t*)(USIC0_CH0_BASE + 0x34))
// Protocol Control Register (I2C specific settings: STIM, SLAD)
#define USIC_PCR               (*(volatile uint32_t*)(USIC0_CH0_BASE + 0x3C))
// Channel Control Register (protocol mode selection: IIC)
#define USIC_CCR               (*(volatile uint32_t*)(USIC0_CH0_BASE + 0x40))
// Protocol Status Clear Register (to clear flags like RIF, TBIF)
#define USIC_PSCR              (*(volatile uint32_t*)(USIC0_CH0_BASE + 0x4C))
// Receive Buffer Register (to read incoming data)
#define USIC_RBUF              (*(volatile uint32_t*)(USIC0_CH0_BASE + 0x54))
// Transmit Buffer Input Location (to write outgoing data and commands)
#define USIC_TBUF              (*(volatile uint32_t*)(USIC0_CH0_BASE + 0x80))
// Protocol Status Register (to check flags like TBIF, RIF)
#define USIC_PSR               (*(volatile uint32_t*)(USIC0_CH0_BASE + 0x48))




// -----------------------------------------------------------------------------
// ADS1115 Constants
// -----------------------------------------------------------------------------

// ADS1115 I2C 7-bit slave address (assuming ADDR pin is connected to GND: 1001000b = 0x48)
#define ADS1115_ADDR           (0x48U)
// ADS1115 internal register addresses
#define ADS1115_CONV_REG       (0x00U) // Conversion Register
#define ADS1115_CONFIG_REG     (0x01U) // Configuration Register

// Configuration Word (16-bit) based on project requirements:
// Single-Shot (OS=1), AIN0/GND (MUX=100), PGA +/-4.096V (PGA=001), 
// Single-Shot Mode (MODE=1), 128 SPS (DR=100), Comparator Disabled (CQUE=11)
// MSB: 1100 0011b (0xC3) | LSB: 1000 0011b (0x83)
#define ADS1115_CONFIG_WORD    (0xC383U) 

// ADS1115 IIC TDF (Transfer Data Format) Codes (Bits [10:8] when writing to TBUF)
#define TDF_START_WRITE        (0x04U) // 100b: Send START + Slave Address + Write Bit
#define TDF_REPEATED_START_READ (0x05U) // 101b: Send Repeated START + Slave Address + Read Bit
#define TDF_DATA               (0x00U) // 000b: Send Data Byte
#define TDF_NACK_STOP          (0x06U) // 110b: Send NACK + STOP Condition
/*typedef struct {
    uint32_t speed_hz;   // e.g. 100000
    // later: which USIC channel, SDA/SCL port/pin, etc.
} i2c_config_t;
*/

void i2c_init(void);       // initialize the bus
void i2c_stop();        // generate stop condition on the bus
// int i2c_write();       // Transmit a buffer of data
// void i2c_read();        // Read a buffer of data
int i2c_write(uint8_t addr, const uint8_t *data, uint16_t len);
int i2c_read(uint8_t addr, uint8_t *data, uint16_t len);
// write a register, then read from that register. 
int i2c_write_reg(uint8_t addr, uint8_t reg, const uint8_t *data, uint16_t len);
// this will send [reg_address], then repeated START + read N bytes.
int i2c_read_reg(uint8_t addr, uint8_t reg, uint8_t *data, uint16_t len);
// then the ADS1115 driver will call:
void I2C_WriteConfig(void);
int16_t I2C_ReadConversion(void);