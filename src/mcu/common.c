#include "common.h"
#include "usic.h"
#include "clock_systick.h"
#include "gpio.h" // Assuming this driver exists

// ADS1115 Constants (defined here for usage clarity)
#define ADS1115_SLAVE_ADDR  (0x48U)
#define ADS1115_CONV_REG    (0x00U)
#define ADS1115_CONFIG_REG  (0x01U)
#define ADS1115_CONFIG_WORD (0xC383U) // Single-Shot, AIN0/GND, PGA +/-4.096V, 128SPS

// XMC SPI Slave Select Pins
#define CS_FPGA_SELECT (0U)
#define CS_FLASH_SELECT (1U)

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
}

int16_t ADC_Read_Sample(void) {
    // 1. Trigger Single-Shot Conversion (write '1' to OS bit)
    // This is typically done by re-writing the config register with OS=1, which
    // is simplified by the fact that the ADS1115 returns to power-down state (OS=0) 
    // Sequence: START, Slave+W, Pointer(0x01), MSB(OS=1), LSB, STOP
    uint16_t config_word_start = ADS1115_CONFIG_WORD | (1U << 15); 
    USIC_I2C_StartWrite(ADS1115_SLAVE_ADDR, ADS1115_CONFIG_REG);
    USIC_I2C_SendByte((config_word_start >> 8), 0);
    USIC_I2C_SendByte((config_word_start & 0xFFU), 1);

    // 2. Wait for Conversion Ready (or implement alert pin polling)
    // For simplicity, a brief delay can be used, but polling the ALERT/RDY pin (if wired) or 
    // the OS bit is safer. Assume polling OS bit (register read) or short delay here.
    //... wait delay or poll conversion status... 

    // 3. Read Conversion Register (0x00)
    // Sequence: START, Slave+W, Pointer(0x00), Repeated START, Slave+R, Read MSB(ACK), Read LSB(NACK), STOP
    USIC_I2C_StartWrite(ADS1115_SLAVE_ADDR, ADS1115_CONV_REG); 
    
    uint8_t msb = USIC_I2C_ReadByte(1, 0); // Read MSB, send ACK (more data coming)
    uint8_t lsb = USIC_I2C_ReadByte(0, 1); // Read LSB, send NACK (stop transfer)
    
    int16_t raw_value = (int16_t)((msb << 8) | lsb);

    // 4. Zero-Centering (Remove DC Offset)
    // The AD8232 outputs a signal centered at ~1.5V (half of 3.3V supply).
    // PGA +/- 4.096V has 1 LSB = 4.096V / 32768 = 0.125 mV/step.
    // The digital equivalent of the 1.5V offset must be subtracted.
    // Digital_Offset = 1.5V / 0.125mV = 12000 (Example value)
    #define DIGITAL_DC_OFFSET (12000) 
    return raw_value - DIGITAL_DC_OFFSET;
}

void FPGA_Send_Data(int16_t zero_centered_sample) {
    // Transmit the 16-bit sample using the SPI Master on USIC0_CH1.
    // CS_FPGA_SELECT (P0.0) is automatically toggled by the underlying SPI driver.
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