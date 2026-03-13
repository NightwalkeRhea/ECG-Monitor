#include "common.h"
#include "usic.h"
#include "clock_systick.h"
#include "gpio.h" // Assuming this driver exists
#include <stddef.h>


/* not a precise timing source, and interrupts can still stretch the SPI bit period.
 BUT AT LEAST BETTER THAN MY PREVIOUS NAIVE BUSY-WAIT! */
#ifndef __NOP
#define __NOP() __asm volatile ("nop")
#endif

// ADS1115 Constants (defined here for usage clarity)
#define ADS1115_SLAVE_ADDR (0x48U)
#define ADS1115_CONV_REG   (0x00U)
#define ADS1115_CONFIG_REG (0x01U)
#define ADS1115_CONFIG_WORD (0xC3A3U) // Single-shot, AIN0/GND, PGA +/-4.096V, 250SPS
#define ADS1115_OS_READY_MASK (0x80U)
#define ADS1115_OS_POLL_LIMIT (32U)
#define GPIO_MODE_PP_GPIO (0x10U)
#define P0_0_IOCR_SHIFT (3U)
#define P0_7_IOCR_SHIFT (27U)
#define P0_8_IOCR_SHIFT (3U)
#define FPGA_SPI_CS_MASK (1U << 0)
#define FPGA_SPI_MOSI_MASK (1U << 7)
#define FPGA_SPI_SCLK_MASK (1U << 8)

// First-order baseline tracker: alpha = 1/128, about 0.31 Hz equivalent corner at 250 Hz.
#define BASELINE_TRACKER_SHIFT (7U)
#define BASELINE_Q_SHIFT       (8U)

// XMC SPI Slave Select Pins
#define CS_FPGA_SELECT (0U)
#define CS_FLASH_SELECT (1U)

#ifndef DIGITAL_DC_OFFSET
    #define DIGITAL_DC_OFFSET (0)
#endif

static int16_t g_last_adc_sample = 0;
static int32_t g_baseline_estimate_q8 = 0;
static uint8_t g_baseline_initialized = 0U;
volatile uint32_t g_i2c_probe_stage = 0U;
volatile uint32_t g_i2c_probe_errors = 0U;
volatile uint32_t g_i2c_probe_ack = 0U;
volatile uint32_t g_i2c_probe_config_written = ADS1115_CONFIG_WORD;
volatile uint32_t g_i2c_probe_config_readback = 0U;
volatile uint32_t g_i2c_probe_status_word = 0U;
volatile uint32_t g_i2c_probe_conversion_word = 0U;
volatile uint32_t g_i2c_probe_poll_count = 0U;

static inline void FPGA_SPI_BitDelay(void){
    // throttling for the GPIO SPI
    // Keep the spacing explicit so the compiler cannot collapse it into a loop body.
    __NOP(); __NOP(); __NOP(); __NOP();
    __NOP(); __NOP(); __NOP(); __NOP();
    __NOP(); __NOP(); __NOP(); __NOP();
    __NOP(); __NOP(); __NOP(); __NOP();
}
/* FPGA_GPIO_SPI_Init is a static helper that configures the relevant Port‑0 pins (P0.0, P0.7, P0.8)
 as push‑pull GPIOs for the FPGA SPI lines and then drives 
the output‑modification register to put the SPI bus in its idle mode (CS high, SCLK low, MOSI low).*/
static void FPGA_GPIO_SPI_Init(void){
    P0_IOCR0 &= ~(0x1FUL << P0_0_IOCR_SHIFT);
    P0_IOCR0 |= (GPIO_MODE_PP_GPIO << P0_0_IOCR_SHIFT);

    P0_IOCR4 &= ~(0x1FUL << P0_7_IOCR_SHIFT);
    P0_IOCR4 |= (GPIO_MODE_PP_GPIO << P0_7_IOCR_SHIFT);

    P0_IOCR8 &= ~(0x1FUL << P0_8_IOCR_SHIFT);
    P0_IOCR8 |= (GPIO_MODE_PP_GPIO << P0_8_IOCR_SHIFT);

    // Idle state for SPI mode 0: CS high, SCLK low, MOSI low.
    P0_OMR = FPGA_SPI_CS_MASK;
    P0_OMR = (FPGA_SPI_MOSI_MASK << 16);
    P0_OMR = (FPGA_SPI_SCLK_MASK << 16);
}

/* FPGA_GPIO_SPI_SendWord is a file‑local helper that bit‑bangs a 16‑bit word out of the GPIO pins to 
 an FPGA over SPI, toggling CS,
 MOSI and SCLK via P0_OMR with inter‑bit delays and always returns true.*/
static bool FPGA_GPIO_SPI_SendWord(uint16_t data){
    uint8_t bit_index = 0U;

    P0_OMR = (FPGA_SPI_CS_MASK << 16);
    FPGA_SPI_BitDelay();

    for (bit_index = 0U; bit_index < 16U; ++bit_index){
        if ((data & 0x8000U) != 0U){
            P0_OMR = FPGA_SPI_MOSI_MASK;
        } else {
            P0_OMR = (FPGA_SPI_MOSI_MASK << 16);
        }
        FPGA_SPI_BitDelay();
        P0_OMR = FPGA_SPI_SCLK_MASK;
        FPGA_SPI_BitDelay();
        P0_OMR = (FPGA_SPI_SCLK_MASK << 16);
        FPGA_SPI_BitDelay();

        data <<= 1;
    }

    P0_OMR = (FPGA_SPI_MOSI_MASK << 16);
    P0_OMR = FPGA_SPI_CS_MASK;
    return true;
}

static bool ADS1115_WriteRegisterWord(uint8_t reg_addr, uint16_t word){
    USIC_I2C_ClearErrorFlags();
    USIC_I2C_StartWrite(ADS1115_SLAVE_ADDR, reg_addr);
    USIC_I2C_SendByte((uint8_t)(word >> 8), 0U);
    USIC_I2C_SendByte((uint8_t)(word & 0xFFU), 1U);
    return (USIC_I2C_GetErrorFlags() == 0U);
}

static bool ADS1115_ReadRegisterWord(uint8_t reg_addr, uint16_t *word_out){
    uint8_t msb = 0U;
    uint8_t lsb = 0U;

    if (word_out == NULL){
        return false;
    }

    USIC_I2C_ClearErrorFlags();
    USIC_I2C_StartWrite(ADS1115_SLAVE_ADDR, reg_addr);
    USIC_I2C_RepeatedStartRead(ADS1115_SLAVE_ADDR);

    msb = USIC_I2C_ReadByte(1U, 0U);
    lsb = USIC_I2C_ReadByte(0U, 1U);

    if (USIC_I2C_GetErrorFlags() != 0U) {
        return false;
    }

    *word_out = ((uint16_t)msb << 8) | (uint16_t)lsb;
    return true;
}

void I2C_Debug_BootProbe(void) {
    uint16_t word = 0U;

    g_i2c_probe_stage = 1U;
    g_i2c_probe_ack = 0U;
    g_i2c_probe_errors = 0U;
    g_i2c_probe_config_readback = 0U;
    g_i2c_probe_status_word = 0U;
    g_i2c_probe_conversion_word = 0U;
    g_i2c_probe_poll_count = 0U;

    if (!ADS1115_WriteRegisterWord(ADS1115_CONFIG_REG, (uint16_t)g_i2c_probe_config_written)) {
        g_i2c_probe_stage = 0xE1U;
        g_i2c_probe_errors = USIC_I2C_GetErrorFlags();
        return;
    }
    g_i2c_probe_ack = 1U;

    g_i2c_probe_stage = 2U;
    if (!ADS1115_ReadRegisterWord(ADS1115_CONFIG_REG, &word)) {
        g_i2c_probe_stage = 0xE2U;
        g_i2c_probe_errors = USIC_I2C_GetErrorFlags();
        return;
    }
    g_i2c_probe_config_readback = word;

    g_i2c_probe_stage = 3U;
    while (g_i2c_probe_poll_count < ADS1115_OS_POLL_LIMIT) {
        g_i2c_probe_poll_count++;

        if (!ADS1115_ReadRegisterWord(ADS1115_CONFIG_REG, &word)) {
            g_i2c_probe_stage = 0xE3U;
            g_i2c_probe_errors = USIC_I2C_GetErrorFlags();
            return;
        }
        g_i2c_probe_status_word = word;

        if ((g_i2c_probe_status_word & 0x8000U) != 0U) {
            break;
        }
    }

    if ((g_i2c_probe_status_word & 0x8000U) == 0U) {
        g_i2c_probe_stage = 0xE4U;
        return;
    }

    if (!ADS1115_ReadRegisterWord(ADS1115_CONV_REG, &word)) {
        g_i2c_probe_stage = 0xE5U;
        g_i2c_probe_errors = USIC_I2C_GetErrorFlags();
        return;
    }
    g_i2c_probe_conversion_word = word;

    g_i2c_probe_stage = 4U;
    g_i2c_probe_errors = USIC_I2C_GetErrorFlags();
}

static bool ADS1115_StartSingleShot(void){
    return ADS1115_WriteRegisterWord(ADS1115_CONFIG_REG, ADS1115_CONFIG_WORD);
}

static bool ADS1115_ReadConfigMSB(uint8_t *status_msb) {
    uint16_t status_word = 0U;

    if (status_msb == NULL) {
        return false;
    }

    if (!ADS1115_ReadRegisterWord(ADS1115_CONFIG_REG, &status_word)) {
        return false;
    }

    *status_msb = (uint8_t)(status_word >> 8);
    return true;
}

static bool ADS1115_ReadConversionWord(int16_t *raw_out) {
    uint16_t raw_word = 0U;

    if (raw_out == NULL) {
        return false;
    }

    if (!ADS1115_ReadRegisterWord(ADS1115_CONV_REG, &raw_word)) {
        return false;
    }

    *raw_out = (int16_t)raw_word;
    return true;
}

static int16_t SaturateQ8ToInt16(int32_t value_q8)
{
    int32_t rounded_q8 = value_q8;

    if (rounded_q8 >= 0) {
        rounded_q8 += (1L << (BASELINE_Q_SHIFT - 1U));
    } else {
        rounded_q8 -= (1L << (BASELINE_Q_SHIFT - 1U));
    }

    rounded_q8 >>= BASELINE_Q_SHIFT;

    if (rounded_q8 > 32767L) {
        return 32767;
    }

    if (rounded_q8 < -32768L) {
        return -32768;
    }

    return (int16_t)rounded_q8;
}

static int16_t Baseline_Remove_Sample(int16_t raw_sample)
{   //running average of the signals value
    int32_t raw_sample_q8 = ((int32_t)raw_sample << BASELINE_Q_SHIFT);
    int32_t centered_sample_q8 = 0;

    /*
     * ADS1115 delivers a single-ended code for the AD8232 mid-rail waveform.
     * The MCU tracks only the very slow baseline/DC component with a first-order
     * IIR estimator, then subtracts that estimate before the SPI transfer.
     * The FPGA therefore receives the same 16-bit sample framing as before
     * (SPI Mode 0, MSB first), but now centered around 0 instead of mid-supply.
     */
    if (g_baseline_initialized == 0U) {
        g_baseline_estimate_q8 = raw_sample_q8;
        g_baseline_initialized = 1U;
        return 0;
    }

    g_baseline_estimate_q8 += ((raw_sample_q8 - g_baseline_estimate_q8) >> BASELINE_TRACKER_SHIFT);
    centered_sample_q8 = raw_sample_q8 - g_baseline_estimate_q8;
    return SaturateQ8ToInt16(centered_sample_q8);
}

void System_Init(void) {
    //Enable USIC Clock
    USIC_Clock_Enable();
    //Initialize Communication Interfaces
    USIC_I2C_Init();
    FPGA_GPIO_SPI_Init();
    // 3. Initialize debug GPIO
    GPIO_Debug_Init();

    // 4. Initialize sampling timer
    SysTick_Init();
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

            g_last_adc_sample = Baseline_Remove_Sample((int16_t)(raw - (int16_t)DIGITAL_DC_OFFSET));
            return g_last_adc_sample;
        }
    }

    return g_last_adc_sample;
}

bool FPGA_Send_Data(int16_t zero_centered_sample) {
    (void)CS_FPGA_SELECT;
    return FPGA_GPIO_SPI_SendWord((uint16_t)zero_centered_sample);
}

// Non-blocking sampling state machine, returns true only when a new sample is available
bool Sampling_Task(int16_t *sample_out){
    static bool conversionBusy = false;
    uint8_t status_msb = 0U;
    int16_t raw= 0;
    int16_t converted = 0;

    if (sample_out == NULL) {
        return false;
    }

    //set the flag with the first trigger.
    if (!conversionBusy) {
        if (ADS1115_StartSingleShot()){
            conversionBusy = true;
        }
        return false;
    }

    // Poll the conversion status once per scheduler tick.
    if (!ADS1115_ReadConfigMSB(&status_msb)) {
        conversionBusy = false;
        return false;
    }

    if ((status_msb & ADS1115_OS_READY_MASK) == 0U) {
        return false;
    }

    if (!ADS1115_ReadConversionWord(&raw)) {
        conversionBusy = false;
        return false;
    }

    /*note:Each scheduler tick takes care of at most one completed ADS1115 conversion.
    The MCU removes the tracked baseline before publishing the sample. That zero-centered sample
    is what the next GPIO-SPI transaction sends to the FPGA as a 16-bit Mode-0 word.*/
    converted = Baseline_Remove_Sample((int16_t)(raw - (int16_t)DIGITAL_DC_OFFSET));
    g_last_adc_sample = converted;
    *sample_out = converted;

    // Trigger the next conversion immediately after a successful read so the
    // next sample can be ready by the next 250 Hz scheduler tBusy = ADS1115_StartSingleShot();
    return true;
}
