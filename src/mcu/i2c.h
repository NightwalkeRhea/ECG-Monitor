#pragma once

#include <stdint.h>

#define I2C_BASE 0x20000000

#define I2C_RESET_REG I2C_BASE+0x00
#define I2C_CONTROL_REG I2C_BASE+0x00
#define I2C_STATUS_REG I2C_BASE+0x00
#define I2C_SLAVE_SELECT I2C_BASE+0x00
#define I2C_INTERRUPT_STATUS_REG I2C_BASE+0x00
#define I2C_GLBL_INTERRUPT I2C_BASE+0x00


void i2c_bus_init(void);
uint32_t i2c_start();
uint32_t i2c_send_bytes(uint8_t *bytes, uint8_t len, uint8_t *ret);
