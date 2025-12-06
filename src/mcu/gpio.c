#include "gpio.h"

// Assuming the passive buzzer/LED is connected to P0.1
#define ALARM_PIN_MASK (1U << 1) 
#define ALARM_PIN_BIT_POS (1U)

void GPIO_Actuator_Init(void) {
    // Explanation: Configure P0.1 as a standard push-pull output.
    // Bits 7:3 of P0_IOCR0 control P0.1 pin function.
    
    // Clear P0.1 control bits first
    P0_IOCR0 &= ~(0x1FUL << ALARM_PIN_BIT_POS); 
    
    // Set P0.1 (PC1) to Push-Pull Alternate Function (e.g., ALT1: 10001b) 
    // or simply Push-Pull General Purpose Output (which is usually sufficient if no ALT function is needed)
    P0_IOCR0 |= (0x10U << ALARM_PIN_BIT_POS); 
    
    // Ensure it starts OFF
    Deactivate_Alarm(); 
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