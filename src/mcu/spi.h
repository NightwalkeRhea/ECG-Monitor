#pragma once

#include <stdint.h>

#define SPI_BASE 0x20000000

#define SPI_RESET_REG SPI_BASE+0x00
#define SPI_CONTROL_REG SPI_BASE+0x00
#define SPI_STATUS_REG SPI_BASE+0x00
#define SPI_TRANSMIT_REG SPI_BASE+0x00
#define SPI_RECEIVE_REG SPI_BASE+0x00
#define SPI_SLAVE_SELECT SPI_BASE+0x00 // select by de/asserting the cs line low
#define SPI_TRANSMIT_OCCUPANCY SPI_BASE + 0x74
#define SPI_RECEIVE_OCCUPANCY SPI_BASE + 0x78
#define SPI_INTERRUPT_GLOBAL_ENABLE_REG SPI_BASE + 0x1c
#define SPI_INTERRUPT_STATUS_REG SPI_BASE + 0x20
#define SPI_INTERRUPT_ENABLE_REG SPI_BASE + 0x28

void spi_init(void);


uint8_t spi_tx_rx(uint8_t byte);

// return -1 if sth went wrong
int spi_write_bytes(uint8_t *bytes, uint32_t len, uint8_t *ret);

typedef struct i2c_struct{
    uint32_t clk_speed;
};


// spi.h

#ifndef SPI_H
#define SPI_H

#include <stdint.h>

// -----------------------------------------------------------------------------
// MCU Register Base Addresses (XMC1100)
// -----------------------------------------------------------------------------

// System Control Unit (SCU) Base Address
#define SCU_BASE               (0x40010000UL)
// Clock Gating Clear Register 0: Enables peripheral clocks.
#define SCU_CGATCLR0           (*(volatile uint32_t*)(SCU_BASE + 0x0C))
// Bit position for USIC0 clock
#define SCU_CGATCLR0_USIC0_Pos (20U)

// General Purpose I/O Port 0 Base Address
#define PORT0_BASE             (0x40040000UL)
#define P0_IOCR0               (*(volatile uint32_t*)(PORT0_BASE + 0x10UL)) // P0.0 to P0.3
#define P0_IOCR8               (*(volatile uint32_t*)(PORT0_BASE + 0x18UL)) // P0.8 to P0.11

// General Purpose I/O Port 1 Base Address
#define PORT1_BASE             (0x40040100UL)
#define P1_IOCR0               (*(volatile uint32_t*)(PORT1_BASE + 0x10UL)) // P1.0 to P1.3

// USIC0 Channel 1 (USIC0_CH1) Base Address
#define USIC0_CH1_BASE         (0x48000100UL)

// -----------------------------------------------------------------------------
// USIC0_CH1 Register Offsets (Same structure as USIC0_CH0)
// -----------------------------------------------------------------------------
#define USIC_KSCFG             (*(volatile uint32_t*)(USIC0_CH1_BASE + 0x0C))
#define USIC_FDR               (*(volatile uint32_t*)(USIC0_CH1_BASE + 0x10))
#define USIC_BRG               (*(volatile uint32_t*)(USIC0_CH1_BASE + 0x14))
#define USIC_SCTR              (*(volatile uint32_t*)(USIC0_CH1_BASE + 0x34))
#define USIC_PCR               (*(volatile uint32_t*)(USIC0_CH1_BASE + 0x3C)) // SSC Protocol Control
#define USIC_CCR               (*(volatile uint32_t*)(USIC0_CH1_BASE + 0x40))
#define USIC_PSCR              (*(volatile uint32_t*)(USIC0_CH1_BASE + 0x4C))
#define USIC_RBUF              (*(volatile uint32_t*)(USIC0_CH1_BASE + 0x54)) // Receive Buffer (16-bit data)
#define USIC_TBUF              (*(volatile uint32_t*)(USIC0_CH1_BASE + 0x80)) // Transmit Buffer (16-bit data)
#define USIC_PSCR              (*(volatile uint32_t*)(USIC0_CH1_BASE + 0x4C))
#define USIC_PSR               (*(volatile uint32_t*)(USIC0_CH1_BASE + 0x48))
#define USIC_TCSR              (*(volatile uint32_t*)(USIC0_CH1_BASE + 0x38)) // Transmission Control/Status

// -----------------------------------------------------------------------------
// Function Prototypes
// -----------------------------------------------------------------------------

void SPI_Init(void);
uint16_t SPI_Transmit_FPGA(uint16_t data);
uint16_t SPI_Transmit_Flash(uint16_t data);

#endif // SPI_H
