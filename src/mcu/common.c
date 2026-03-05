#include "common.h"
#include "usic.h"
#include "clock_systick.h"
#include "gpio.h" // Assuming this driver exists
#include <stddef.h>

// ADS1115 Constants (defined here for usage clarity)
#define ADS1115_SLAVE_ADDR  (0x48U)
#define ADS1115_CONV_REG    (0x00U)
#define ADS1115_CONFIG_REG  (0x01U)
#define ADS1115_CONFIG_WORD (0xC3A3U) // Single-shot, AIN0/GND, PGA +/-4.096V, 250SPS
#define ADS1115_OS_READY_MASK (0x80U)
#define ADS1115_OS_POLL_LIMIT (32U)

// XMC SPI Slave Select Pins
#define CS_FPGA_SELECT (0U)
#define CS_FLASH_SELECT (1U)

#ifndef DIGITAL_DC_OFFSET
#define DIGITAL_DC_OFFSET (0)
#endif

static int16_t g_last_adc_sample = 0;

static bool ADS1115_StartSingleShot(void) {
    uint16_t config_word_start = (uint16_t)(ADS1115_CONFIG_WORD | (1U << 15));

    USIC_I2C_StartWrite(ADS1115_SLAVE_ADDR, ADS1115_CONFIG_REG);
    USIC_I2C_SendByte((uint8_t)(config_word_start >> 8), 0);
    USIC_I2C_SendByte((uint8_t)(config_word_start & 0xFFU), 1);

    if (USIC_I2C_GetErrorFlags() != 0U) {
        USIC_I2C_ClearErrorFlags();
        return false;
    }

    return true;
}

static bool ADS1115_ReadConfigMSB(uint8_t *status_msb) {
    if (status_msb == NULL) {
        return false;
    }

    USIC_I2C_StartWrite(ADS1115_SLAVE_ADDR, ADS1115_CONFIG_REG);
    USIC_I2C_RepeatedStartRead(ADS1115_SLAVE_ADDR);

    *status_msb = USIC_I2C_ReadByte(1, 0);
    (void)USIC_I2C_ReadByte(0, 1); // NACK + STOP on the LSB

    if (USIC_I2C_GetErrorFlags() != 0U) {
        USIC_I2C_ClearErrorFlags();
        return false;
    }

    return true;
}

static bool ADS1115_ReadConversionWord(int16_t *raw_out) {
    uint8_t msb;
    uint8_t lsb;

    if (raw_out == NULL) {
        return false;
    }

    USIC_I2C_StartWrite(ADS1115_SLAVE_ADDR, ADS1115_CONV_REG);
    USIC_I2C_RepeatedStartRead(ADS1115_SLAVE_ADDR);

    msb = USIC_I2C_ReadByte(1, 0);
    lsb = USIC_I2C_ReadByte(0, 1); // NACK + STOP

    if (USIC_I2C_GetErrorFlags() != 0U) {
        USIC_I2C_ClearErrorFlags();
        return false;
    }

    *raw_out = (int16_t)(((uint16_t)msb << 8) | (uint16_t)lsb);
    return true;
}

void System_Init(void) {
    // 1. Enable USIC Clock
    USIC_Clock_Enable();

    // 2. Initialize Communication Interfaces
    USIC_I2C_Init();  // ADC Link
    USIC_SPI_Init();  // FPGA Link
    USIC_UART_Init(); // PC Link
    
    // 3. Initialize Actuator GPIO
    GPIO_Actuator_Init(); // From gpio_actuator.c

    // 4. Initialize Sampling Timer
    SysTick_Init();
    
    // 5. Initial ADC Configuration (Set PGA, MUX, Single-Shot Mode)
    uint8_t config_msb = (ADS1115_CONFIG_WORD >> 8);
    uint8_t config_lsb = (ADS1115_CONFIG_WORD & 0xFFU);

    // Sequence: START, Slave+W, Pointer(0x01)
    USIC_I2C_StartWrite(ADS1115_SLAVE_ADDR, ADS1115_CONFIG_REG);
    // MSB, then LSB, then STOP
    USIC_I2C_SendByte(config_msb, 0); 
    USIC_I2C_SendByte(config_lsb, 1); // True sends STOP condition
    USIC_I2C_ClearErrorFlags();
}

int16_t ADC_Read_Sample(void)
{
    uint32_t poll_count = ADS1115_OS_POLL_LIMIT;
    uint8_t status_msb = 0U;
    int16_t raw = 0;

    if (!ADS1115_StartSingleShot()) {
        return g_last_adc_sample;
    }

    while (poll_count-- != 0U) {
        if (!ADS1115_ReadConfigMSB(&status_msb)) {
            return g_last_adc_sample;
        }

        if ((status_msb & ADS1115_OS_READY_MASK) != 0U) {
            if (!ADS1115_ReadConversionWord(&raw)) {
                return g_last_adc_sample;
            }

            g_last_adc_sample = (int16_t)(raw - (int16_t)DIGITAL_DC_OFFSET);
            return g_last_adc_sample;
        }
    }

    return g_last_adc_sample;
}

void FPGA_Send_Data(int16_t zero_centered_sample) {
    // Transmit the 16-bit sample using the SPI Master on USIC0_CH1.
    // CS_FPGA_SELECT (P0.0) is currently driven manually inside the SPI driver.
    USIC_SPI_Transfer((uint16_t)zero_centered_sample, CS_FPGA_SELECT);
}

void PC_Transmit_Filtered_Data(int16_t filtered_data, uint16_t bpm) {
    // 1. Save Current Mode and Switch to UART (ASC) Mode
    // The previous mode MUST be I2C due to the SysTick ISR timing.
    USIC_SetMode_UART_CH0();

    // 2. Transmit Data Packet
    // Example packet structure: Start Byte (0xAA), Filtered MSB, Filtered LSB, BPM MSB, BPM LSB
    
    // Transmit Start Marker
    USIC_UART_Transmit(0xAA);

    // Transmit Filtered Data (16-bit, MSB first)
    USIC_UART_Transmit((filtered_data >> 8) & 0xFFU);
    USIC_UART_Transmit(filtered_data & 0xFFU);
    
    // Transmit BPM (16-bit, MSB first)
    USIC_UART_Transmit((bpm >> 8) & 0xFFU);
    USIC_UART_Transmit(bpm & 0xFFU);
    
    // 3. Switch Channel back to I2C (IIC) Mode
    // This is vital to ensure the next 250 Hz SysTick ISR finds the channel ready for the ADC read.
    USIC_SetMode_I2C_CH0(); 
}

// Non-blocking sampling state machine.
// Returns true only when a new sample is available in *sample_out.
bool Sampling_Task(int16_t *sample_out)
{
    static bool conversion_primed = false;
    uint8_t status_msb = 0U;
    int16_t raw = 0;
    int16_t converted = 0;

    if (sample_out == NULL) {
        return false;
    }

    // Prime the pipeline with the first trigger.
    if (!conversion_primed) {
        if (ADS1115_StartSingleShot()) {
            conversion_primed = true;
        }
        return false;
    }

    // Poll the conversion status once per scheduler tick.
    if (!ADS1115_ReadConfigMSB(&status_msb)) {
        conversion_primed = false;
        return false;
    }

    if ((status_msb & ADS1115_OS_READY_MASK) == 0U) {
        return false;
    }

    if (!ADS1115_ReadConversionWord(&raw)) {
        conversion_primed = false;
        return false;
    }

    converted = (int16_t)(raw - (int16_t)DIGITAL_DC_OFFSET);
    g_last_adc_sample = converted;
    *sample_out = converted;

    // Trigger the next conversion immediately after a successful read so the
    // next sample can be ready by the next 250 Hz scheduler tick.
    conversion_primed = ADS1115_StartSingleShot();
    return true;
}
