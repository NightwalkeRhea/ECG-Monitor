#include "gpio.h"

// Assuming the passive buzzer/LED is connected to P0.1
#define ALARM_PIN_MASK (1U << 1)
#define ALARM_PIN_IOCR_SHIFT (11U)

// XMC2Go user LEDs.
#define DEBUG_LED1_MASK       (1U << 0) // P1.0
#define DEBUG_LED2_MASK       (1U << 1) // P1.1
#define DEBUG_LED1_IOCR_SHIFT (3U)
#define DEBUG_LED2_IOCR_SHIFT (11U)
#define GPIO_OUTPUT_PP_CONFIG (0x10U)

static uint8_t g_debug_led1_state = 0U;
static uint8_t g_debug_led2_state = 0U;
static uint8_t g_scheduler_tick_div = 0U;
static uint8_t g_spi_send_div = 0U;

static void GPIO_Debug_SetLED(uint32_t mask, uint8_t *state, uint8_t on)
{
    if (on != 0U) {
        P1_OMR = mask;
        *state = 1U;
    } else {
        P1_OMR = (mask << 16);
        *state = 0U;
    }
}

static void GPIO_Debug_ToggleLED(uint32_t mask, uint8_t *state)
{
    GPIO_Debug_SetLED(mask, state, (uint8_t)(*state == 0U));
}

void GPIO_Actuator_Init(void) {
    // Explanation: Configure P0.1 as a standard push-pull output.
    // Bits 7:3 of P0_IOCR0 control P0.1 pin function.
    
    // Clear P0.1 control bits first
    P0_IOCR0 &= ~(0x1FUL << ALARM_PIN_IOCR_SHIFT); 
    
    // Set P0.1 (PC1) to Push-Pull Alternate Function (e.g., ALT1: 10001b) 
    // or simply Push-Pull General Purpose Output (which is usually sufficient if no ALT function is needed)
    P0_IOCR0 |= (0x10U << ALARM_PIN_IOCR_SHIFT); 
    
    // Ensure it starts OFF
    Deactivate_Alarm(); 
}

void GPIO_Debug_Init(void)
{
    P1_IOCR0 &= ~((0x1FUL << DEBUG_LED1_IOCR_SHIFT) | (0x1FUL << DEBUG_LED2_IOCR_SHIFT));
    P1_IOCR0 |= ((GPIO_OUTPUT_PP_CONFIG << DEBUG_LED1_IOCR_SHIFT) |
                 (GPIO_OUTPUT_PP_CONFIG << DEBUG_LED2_IOCR_SHIFT));

    g_debug_led1_state = 0U;
    g_debug_led2_state = 0U;
    g_scheduler_tick_div = 0U;
    g_spi_send_div = 0U;
    GPIO_Debug_SetLED(DEBUG_LED1_MASK, &g_debug_led1_state, 0U);
    GPIO_Debug_SetLED(DEBUG_LED2_MASK, &g_debug_led2_state, 0U);
}

void Activate_Alarm(void) {
    // Explanation: Set the output pin P0.1 high to activate the buzzer/LED.
    // Writing to the P0_OMR register is the atomic way to set or clear a pin.
    // OMR bits [15:0] are Set bits, [31:16] are Clear bits.
    P0_OMR = ALARM_PIN_MASK; // Set P0.1 (Writing 1 to bit 1 in OMR[15:0] sets the output)
}

void Deactivate_Alarm(void) {
    // Explanation: Clear the output pin P0.1 low to turn off the buzzer/LED.
    P0_OMR = (ALARM_PIN_MASK << 16); // Clear P0.1 (Writing 1 to bit 1 in OMR[31:16] clears the output)
}

void GPIO_Debug_Scheduler_Tick(void)
{
    g_scheduler_tick_div++;
    if (g_scheduler_tick_div >= 32U) {
        g_scheduler_tick_div = 0U;
        GPIO_Debug_ToggleLED(DEBUG_LED1_MASK, &g_debug_led1_state);
    }
}

void GPIO_Debug_SPI_Sent(void)
{
    g_spi_send_div++;
    if (g_spi_send_div >= 16U) {
        g_spi_send_div = 0U;
        GPIO_Debug_ToggleLED(DEBUG_LED2_MASK, &g_debug_led2_state);
    }
}

void GPIO_Debug_ShowI2CBusLevels(uint8_t sda_high, uint8_t scl_high)
{
    GPIO_Debug_SetLED(DEBUG_LED1_MASK, &g_debug_led1_state, sda_high);
    GPIO_Debug_SetLED(DEBUG_LED2_MASK, &g_debug_led2_state, scl_high);
}
