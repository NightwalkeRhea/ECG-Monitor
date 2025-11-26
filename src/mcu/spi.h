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

void spi_init();

uint8_t spi_tx_rx(uint8_t byte);

// return -1 if sth went wrong
int spi_write_bytes(uint8_t *bytes, uint32_t len, uint8_t *ret);
