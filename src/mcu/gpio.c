#include "gpio.h"

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
