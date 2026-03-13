// usic.c

#include "usic.h"
#include <stdbool.h>
#include <stddef.h>

// --- Register Pointers for USIC0 Channel 0 ---
// USIC0_CH0_BASE = 0x48000000UL -> check mcu_definition.h for the register mapping. 
#define CH0_KSCFG          (*(volatile uint32_t*)(USIC0_CH0_BASE + 0x0C)) // Kernel State Configuration
#define CH0_CCR            (*(volatile uint32_t*)(USIC0_CH0_BASE + 0x40)) // Channel Control Register
#define CH0_FDR            (*(volatile uint32_t*)(USIC0_CH0_BASE + 0x10)) // Fractional Divider Register
#define CH0_BRG            (*(volatile uint32_t*)(USIC0_CH0_BASE + 0x14)) // Baud Rate Generator
#define CH0_SCTR           (*(volatile uint32_t*)(USIC0_CH0_BASE + 0x34)) // Shift Control Register
#define CH0_PCR            (*(volatile uint32_t*)(USIC0_CH0_BASE + 0x3C)) // Protocol Control Register
#define CH0_PSR            (*(volatile uint32_t*)(USIC0_CH0_BASE + 0x48)) // Protocol Status Register
#define CH0_PSCR           (*(volatile uint32_t*)(USIC0_CH0_BASE + 0x4C)) // Protocol Status Clear Register
#define CH0_TBUF           (*(volatile uint32_t*)(USIC0_CH0_BASE + 0x80)) // Transmit Buffer Input Location
#define CH0_RBUF           (*(volatile uint32_t*)(USIC0_CH0_BASE + 0x54)) // Receive Buffer Register
#define CH0_DX0CR          (*(volatile uint32_t*)(USIC0_CH0_BASE + 0x1C)) // Input Control Register 0
#define CH0_DX1CR          (*(volatile uint32_t*)(USIC0_CH0_BASE + 0x20)) // Input Control Register 1
#define CH0_DX2CR          (*(volatile uint32_t*)(USIC0_CH0_BASE + 0x24)) // Input Control Register 2

#define CCFG_SSC (1U<<0)
#define CCFG_IIC (1U<<2)
#define CCFG_RB  (1U<<6)
#define CCFG_TB  (1U<<7)


// --- USIC0 Channel 1 Register Pointers (SPI) ---
// USIC0_CH1_BASE = 0x48000200UL
#define CH1_KSCFG          (*(volatile uint32_t*)(USIC0_CH1_BASE + 0x0C))
#define CH1_FDR            (*(volatile uint32_t*)(USIC0_CH1_BASE + 0x10))
#define CH1_CCR            (*(volatile uint32_t*)(USIC0_CH1_BASE + 0x40))
#define CH1_BRG            (*(volatile uint32_t*)(USIC0_CH1_BASE + 0x14))
#define CH1_SCTR           (*(volatile uint32_t*)(USIC0_CH1_BASE + 0x34))
#define CH1_PCR            (*(volatile uint32_t*)(USIC0_CH1_BASE + 0x3C))
#define CH1_PSR            (*(volatile uint32_t*)(USIC0_CH1_BASE + 0x48))
#define CH1_PSCR           (*(volatile uint32_t*)(USIC0_CH1_BASE + 0x4C))
#define CH1_TBUF           (*(volatile uint32_t*)(USIC0_CH1_BASE + 0x80))
#define CH1_RBUF           (*(volatile uint32_t*)(USIC0_CH1_BASE + 0x54))
#define CH1_TCSR           (*(volatile uint32_t*)(USIC0_CH1_BASE + 0x38))
#define CH1_DX0CR          (*(volatile uint32_t*)(USIC0_CH1_BASE + 0x1C))
#define CH1_DX1CR          (*(volatile uint32_t*)(USIC0_CH1_BASE + 0x20))
#define CH1_DX2CR          (*(volatile uint32_t*)(USIC0_CH1_BASE + 0x24)) // Input Control Register 2 (CS/SELIN)

// Temporary isolation remap:
// use USIC0_CH1 on P2.10/P2.11 for the ADS1115 probe because those pins are
// exposed on the XMC2Go header
// Restore the original design by setting this back to 0.
#define I2C_ISOLATION_USE_CH1_P2_10_P2_11 (1U)

#define GPIO_MODE_INPUT_TRISTATE (0x00U)
#define GPIO_MODE_PP_GPIO        (0x10U)
#define GPIO_MODE_PP_ALT7        (0x17U)

#define P0_0_IOCR_SHIFT        (3U)
#define P0_6_IOCR_SHIFT        (19U)
#define P0_7_IOCR_SHIFT        (27U)
#define P0_8_IOCR_SHIFT        (3U)
#define P0_9_IOCR_SHIFT        (11U)
#define P2_HWSEL_SDA_MASK      (0x3U << 20)
#define P2_HWSEL_SCL_MASK      (0x3U << 22)
#define P2_PDISC_SDA_MASK      (1U << 10)
#define P2_PDISC_SCL_MASK      (1U << 11)

#define SPI_CS_FPGA_MASK       (1U << 0)
#define SPI_CS_FLASH_MASK      (1U << 9)

// Common USIC bit field helpers
#define USIC_CCR_MODE_DISABLED (0x0U)
#define USIC_CCR_MODE_SSC      (0x1U)
#define USIC_CCR_MODE_IIC      (0x4U)

#define USIC_SCTR_SDIR_MSB     (1U << 0)
#define USIC_SCTR_PDL_MASK     (1U << 1)
#define USIC_SCTR_DOCFG_OD     (0U << 6)
#define USIC_SCTR_TRM_STD      (1U << 8) // SSC standard transfer mode: TRM = 01b
#define USIC_SCTR_TRM_IIC      (3U << 8) // IIC recommendation: TRM = 11b
#define USIC_SCTR_FLE(v)       ((uint32_t)(v) << 16)
#define USIC_SCTR_WLE(v)       ((uint32_t)(v) << 24)

#define USIC_FDR_DM_DIV        (0x1U << 14)
#define USIC_FDR_STEP_MAX      (0x3FFU)

#define USIC_BRG_CLKSEL_DX1T   (0U << 0)
#define USIC_BRG_DCTQ(v)       ((uint32_t)(v) << 10)
#define USIC_BRG_PDIV(v)       ((uint32_t)(v) << 16)
#define USIC_BRG_SCLKCFG(v)    ((uint32_t)(v) << 30)

#define USIC_DXCR_DSEL_A       (0U << 0)
#define USIC_DXCR_DSEL_B       (1U << 0)
#define USIC_DXCR_DSEL_C       (2U << 0)
#define USIC_DXCR_DSEL_D       (3U << 0)
#define USIC_DXCR_DSEL_E       (4U << 0)
#define USIC_DXCR_DSEL_F       (5U << 0)
#define USIC_DXCR_DSEL_HIGH    (7U << 0)
#define USIC_DXCR_INSW_DIRECT  (0U << 4)
#define USIC_DXCR_INSW_INPUT   (1U << 4)

#define USIC_KSCFG_MODEN       (1U << 0) 
#define USIC_KSCFG_BPMODEN     (1U << 1)

#define USIC_TCSR_TDSSM        (1U << 8)
#define USIC_TCSR_TDEN(v)      ((uint32_t)(v) << 10)

#define USIC_PCR_SSC_MSLSEN    (1U << 0)
#define USIC_PCR_SSC_SLPHSEL   (1U << 25)

// IIC TDF (Transfer Data Format) Codes (Bits [10:8] when writing to TBUF) [14-121]
#define TDF_MASTER_TX          (0x0U) // 000b: Send data byte (master transmit)
#define TDF_MASTER_RX_ACK      (0x2U) // 010b: Receive byte and send ACK
#define TDF_MASTER_RX_NACK     (0x3U) // 011b: Receive Byte and send NACK
#define TDF_MASTER_START       (0x4U) // 100b: Send START and address in TBUF[7:0]
#define TDF_MASTER_RESTART     (0x5U) // 101b: Send repeated START and address in TBUF[7:0]
#define TDF_MASTER_STOP        (0x6U) // 110b: Send STOP Condition

// PSR Flag Masks
#define PSR_RIF_MASK           (1U << 14) // Receive Indication Flag
#define PSR_AIF_MASK           (1U << 15) // Alternative Receive Indication Flag
#define PSR_PCR_MASK           (1U << 4)  // IIC: Stop Condition Received Flag
#define PSR_TSIF_MASK          (1U << 12) // indicates the word has finished shifting out.

#define PSR_IIC_NACK_MASK      (1U << 5)
#define PSR_IIC_ARL_MASK       (1U << 6)
#define PSR_IIC_ERR_MASK       (1U << 8)
#define PSR_IIC_ACK_MASK       (1U << 9)
#define PSR_IIC_RESPONSE_MASK  (PSR_IIC_ACK_MASK | PSR_IIC_NACK_MASK)
#define PSR_IIC_FATAL_MASK     (PSR_IIC_ARL_MASK | PSR_IIC_ERR_MASK)

#define USIC_WAIT_TIMEOUT      (200000UL)
#define USIC_I2C_ERR_TIMEOUT   (1UL << 31)

#if I2C_ISOLATION_USE_CH1_P2_10_P2_11
#define I2C_KSCFG   CH1_KSCFG
#define I2C_CCR     CH1_CCR
#define I2C_FDR     CH1_FDR
#define I2C_BRG     CH1_BRG
#define I2C_SCTR    CH1_SCTR
#define I2C_PCR     CH1_PCR
#define I2C_PSR     CH1_PSR
#define I2C_PSCR    CH1_PSCR
#define I2C_TBUF    CH1_TBUF
#define I2C_RBUF    CH1_RBUF
#define I2C_TCSR    CH1_TCSR
#define I2C_DX0CR   CH1_DX0CR
#define I2C_DX1CR   CH1_DX1CR
#define I2C_DX2CR   CH1_DX2CR
#else
#define I2C_KSCFG   CH0_KSCFG
#define I2C_CCR     CH0_CCR
#define I2C_FDR     CH0_FDR
#define I2C_BRG     CH0_BRG
#define I2C_SCTR    CH0_SCTR
#define I2C_PCR     CH0_PCR
#define I2C_PSR     CH0_PSR
#define I2C_PSCR    CH0_PSCR
#define I2C_TBUF    CH0_TBUF
#define I2C_RBUF    CH0_RBUF
#define I2C_TCSR    (*(volatile uint32_t*)(USIC0_CH0_BASE + 0x38))
#define I2C_DX0CR   CH0_DX0CR
#define I2C_DX1CR   CH0_DX1CR
#define I2C_DX2CR   CH0_DX2CR
#endif

static volatile uint32_t g_usic_i2c_errors = 0U;
// Helper Functions

/*Waits until the specified flag in the Protocol Status Register (PSR) of USIC0_CH0 is set.*/
static bool USIC_WaitForFlag_CH0(uint32_t flag_mask){
    uint32_t timeout = USIC_WAIT_TIMEOUT;

    while (timeout-- != 0U){
        uint32_t psr = I2C_PSR;

        if ((psr & PSR_IIC_FATAL_MASK) != 0U){
            g_usic_i2c_errors |= (psr & PSR_IIC_FATAL_MASK);
            I2C_PSCR = (psr & PSR_IIC_FATAL_MASK);
            return false;
        }

        if ((psr & flag_mask) != 0U){
            return true;
        }
    }
    g_usic_i2c_errors |= USIC_I2C_ERR_TIMEOUT;
    return false;
}

static bool USIC_WaitForAckOrNack_CH0(void){
    uint32_t timeout = USIC_WAIT_TIMEOUT;
    while (timeout-- != 0U) {
        uint32_t psr = I2C_PSR;

        if ((psr & PSR_IIC_FATAL_MASK) != 0U) {
            g_usic_i2c_errors |= (psr & PSR_IIC_FATAL_MASK);
            I2C_PSCR = (psr & PSR_IIC_FATAL_MASK);
            return false;
        }
        if ((psr & PSR_IIC_ACK_MASK) != 0U) {
            I2C_PSCR = PSR_IIC_ACK_MASK;
            return true;
        }
        if ((psr & PSR_IIC_NACK_MASK) != 0U) {
            g_usic_i2c_errors |= PSR_IIC_NACK_MASK;
            I2C_PSCR = PSR_IIC_NACK_MASK;
            return false;
        }
    }
    g_usic_i2c_errors |= USIC_I2C_ERR_TIMEOUT;
    return false;
}

/* Waits until the specified flag in the Protocol Status Register (PSR) of USIC0_CH1 is set.*/
static bool USIC_WaitForFlag_CH1(uint32_t flag_mask) {
    uint32_t timeout = USIC_WAIT_TIMEOUT;

    while (timeout-- != 0U) {
        if ((CH1_PSR & flag_mask) != 0U) {
            return true;
        }
    }
    return false;
}

static bool USIC_WaitForAnyFlag_CH0(uint32_t flag_mask, uint32_t *observed_flags) {
    uint32_t timeout = USIC_WAIT_TIMEOUT;

    while (timeout-- != 0U) {
        uint32_t psr = I2C_PSR;

        if ((psr & PSR_IIC_FATAL_MASK) != 0U) {
            g_usic_i2c_errors |= (psr & PSR_IIC_FATAL_MASK);
            I2C_PSCR = (psr & PSR_IIC_FATAL_MASK);
            return false;
        }
        if ((psr & flag_mask) != 0U) {
            if (observed_flags != NULL) {
                *observed_flags = (psr & flag_mask);
            }
            return true;
        }
    }
    g_usic_i2c_errors |= USIC_I2C_ERR_TIMEOUT;
    return false;
}

/*Clears the specified flag in the Protocol Status Register (PSR) of USIC0_CH0.*/
static void USIC_ClearFlag_CH0(uint32_t flag_mask){
    I2C_PSCR = flag_mask;
}

/*Clears the specified flag in the Protocol Status Register (PSR) of USIC0_CH1.*/
static void USIC_ClearFlag_CH1(uint32_t flag_mask) {
    CH1_PSCR = flag_mask;
}


// USIC Core Functions

void USIC_Clock_Enable(void) {
    // Enable USIC0 module clock (bit 3 in SCU_CGATCLR0)
    // We use the same bit-protection sequence used by startup code so this works
    // regardless of current SCU protection state.
    uint32_t timeout = USIC_WAIT_TIMEOUT;

    SCU_PASSWD = 0x000000C0UL; // disable bit protection
    SCU_CGATCLR0 = (1U << 3);
    SCU_PASSWD = 0x000000C3UL; // enable bit protection

    while ((timeout-- != 0U) && ((SCU_CGATSTAT0 & (1U << 3)) != 0U)) {
    }
}



// I2C Implementation (USIC0_CH0)

void USIC_I2C_Init(void){
    /* at 100 kHz
     * Configuration: Single-ended AIN0/GND with +-4.096V PGA (conservative range, safe against clipping).
     * ADS1115 Address: 0x48 (ADDR pin tied low, didn't I? TODO: check the breadboard).
     * Single-shot mode: MCU initiates each conversion via I2C command.
     */
    I2C_KSCFG = (USIC_KSCFG_MODEN | USIC_KSCFG_BPMODEN);    
    while ((I2C_KSCFG & USIC_KSCFG_MODEN) == 0U) {
    }

    // Keep the channel in idle while its protocol registers are configured.
    I2C_CCR = USIC_CCR_MODE_DISABLED;
    I2C_PSCR = 0xFFFFFFFFU;
    g_usic_i2c_errors = 0U;


    // P2.10 = SDA  -> USIC0_CH1 DOUT0 / DX0F
    // P2.11 = SCL  -> USIC0_CH1 SCLKOUT / DX1E
    // Restore the original design by setting I2C_ISOLATION_USE_CH1_P2_10_P2_11 back to 0.
    P2_HWSEL &= ~(P2_HWSEL_SDA_MASK | P2_HWSEL_SCL_MASK);
    P2_PDISC &= ~(P2_PDISC_SDA_MASK | P2_PDISC_SCL_MASK);
    #define ALT7_OPEN_DRAIN (0x1FU) 
    #define ALT6_OPEN_DRAIN (0x1EU)
    P2_IOCR8 &= ~((0x1FUL << 27) | (0x1FUL << 19)); // Clear bits for P2.11/P2.10
    P2_IOCR8 |= (ALT6_OPEN_DRAIN << 27) | (ALT7_OPEN_DRAIN << 19); // SCL + SDA
    // Input Stage Configuration (IIC mode uses DX0 for SDA, DX1 for SCL)
    I2C_DX0CR = USIC_DXCR_INSW_DIRECT | USIC_DXCR_DSEL_F; // DX0F input: SDA -> P2.10
    I2C_DX1CR = USIC_DXCR_INSW_DIRECT | USIC_DXCR_DSEL_E; // DX1E input: SCL -> P2.11
    I2C_DX2CR = USIC_DXCR_INSW_DIRECT | USIC_DXCR_DSEL_A; // Not used in IIC, keep direct path.

    // Data format per the vendor I2C init: TRM=11b, 8-bit words, unlimited
    // frame length, MSB first, passive data level high
    I2C_SCTR = USIC_SCTR_SDIR_MSB |
               USIC_SCTR_TRM_IIC |
               USIC_SCTR_FLE(0x3FU) |
               USIC_SCTR_WLE(7U) |
               USIC_SCTR_PDL_MASK;

    //Baud Rate Configuration (100kHz I2C Standard Mode)
    I2C_FDR = USIC_FDR_DM_DIV | USIC_FDR_STEP_MAX;
    I2C_BRG = USIC_BRG_CLKSEL_DX1T | USIC_BRG_DCTQ(9U) | USIC_BRG_PDIV(63U) | USIC_BRG_SCLKCFG(0U);

    // Enable transfer data valid handling
    I2C_TCSR = USIC_TCSR_TDSSM | USIC_TCSR_TDEN(1U);
    
    // IIC Protocol Control
    // STIM = 0 (10 Time Quanta per symbol for 100 kHz Standard Mode) [14-130]
    // HDEL = 3 (Hardware Delay for SDA Hold Time ~300ns) [14-131, 14-118]
    I2C_PCR = (0U << 17) | // PCR.STIM = 0 (100 kBaud Standard Mode)
              (3U << 26); // PCR.HDEL = 3 Adjusted delay for hold time

    //Protocol Setup (IIC Mode 4H)
    I2C_CCR = USIC_CCR_MODE_IIC;
}

/*Sends the START condition, Slave Address (with Write bit), and Register Address.
 Takes as inputs slave_addr 7-bit I2C address of the target device, and reg_addr Internal register address to target.
 */
void USIC_I2C_StartWrite(uint8_t slave_addr, uint8_t reg_addr){
    // Prepares the USIC TBUF to send the I2C START condition, 
    // the target slave address (left-shifted, with R/W bit low), and the 
    // internal register pointer address byte (reg_addr).
    uint8_t addr_w = (uint8_t)((slave_addr << 1) | 0U);
    I2C_PSCR = PSR_IIC_RESPONSE_MASK;
    // 1. Start + address for writing.
    // The IIC PPP extracts the slave address from the TBUF location write. [14-121]
    I2C_TBUF = ((uint32_t)TDF_MASTER_START << 8) | addr_w; 
    if (!USIC_WaitForAckOrNack_CH0()) {
        return;
    }
    // Send pointer/register address byte
    I2C_PSCR = PSR_IIC_RESPONSE_MASK;
    I2C_TBUF = ((uint32_t)TDF_MASTER_TX << 8) | reg_addr;
    if (!USIC_WaitForAckOrNack_CH0()){
        return;
    }
}

/*Sends the repeated START condition, takes as input, slave_addr 7-bit I2C address of the target device */
void USIC_I2C_RepeatedStartRead(uint8_t slave_addr){
    uint8_t addr_r = (uint8_t)((slave_addr << 1) | 1U);

    I2C_PSCR = PSR_IIC_RESPONSE_MASK;
    I2C_TBUF = ((uint32_t)TDF_MASTER_RESTART << 8) | addr_r;
    if (!USIC_WaitForAckOrNack_CH0()) {
        return;
    }
}

/*Sends a data byte followed by an optional STOP condition.
Takse as input the data byte to send (MSB first due to SCTR configuration). stop_condition True 
if this is the last byte and a STOP condition should be sent.
 */
void USIC_I2C_SendByte(uint8_t data, uint8_t stop_condition){
    // Sends a single data byte (e.g., config MSB or LSB). 
    // If stop_condition is true, transaction terminated with TDF_NACK_STOP
    
    I2C_PSCR = PSR_IIC_RESPONSE_MASK;
    I2C_TBUF = ((uint32_t)TDF_MASTER_TX << 8) | data;
    if (!USIC_WaitForAckOrNack_CH0()) {
        return;
    }
    if (stop_condition) {
        I2C_TBUF = ((uint32_t)TDF_MASTER_STOP << 8);
        if (!USIC_WaitForFlag_CH0(PSR_PCR_MASK)) {
            return;
        }
        USIC_ClearFlag_CH0(PSR_PCR_MASK);
    }
}

/* Reads a single data byte (Master Receive).
   ack_condition True to send ACK (expecting more data), False to send NACK (last byte).
   stop_condition True to terminate transaction with STOP after NACK.
   The received data byte (bits 7:0 of RBUF).*/
uint8_t USIC_I2C_ReadByte(uint8_t ack_condition, uint8_t stop_condition) {
    //This function is called after the Repeated START is sent. 
    // It triggers the next byte read and controls the ACK/NACK/STOP handshake.

    // Send dummy word to trigger the read operation and set the ACK/NACK condition.
    uint32_t tdf = ack_condition ? TDF_MASTER_RX_ACK : TDF_MASTER_RX_NACK;
    // 010b (ACK) for intermediate bytes and 011b (NACK) for the last byte, then STOP.
    
    // Trigger receive (TBUF[7:0] ignored for RX formats)
    I2C_TBUF = ((uint32_t)tdf << 8);

    // Wait until data arrives: first byte may set AIF, later set RIF (either is fine)
    uint32_t observed_flags = 0U;
    if (!USIC_WaitForAnyFlag_CH0((PSR_AIF_MASK | PSR_RIF_MASK), &observed_flags)) {
        return 0U;
    }

    // Clear whichever flag(s) fired
    if ((observed_flags & PSR_AIF_MASK) != 0U) {
        USIC_ClearFlag_CH0(PSR_AIF_MASK);
    }
    if ((observed_flags & PSR_RIF_MASK) != 0U) {
        USIC_ClearFlag_CH0(PSR_RIF_MASK);
    }
    uint8_t data = (uint8_t)I2C_RBUF;

    if (stop_condition) {
        // STOP must be a separate TDF after the last receive
        I2C_TBUF = ((uint32_t)TDF_MASTER_STOP << 8);
        if (!USIC_WaitForFlag_CH0(PSR_PCR_MASK)) {
            return data;
        }
        USIC_ClearFlag_CH0(PSR_PCR_MASK);
    }
    return data;
}

uint32_t USIC_I2C_GetErrorFlags(void) {
    return g_usic_i2c_errors;
}

void USIC_I2C_ClearErrorFlags(void) {
    g_usic_i2c_errors = 0U;
    I2C_PSCR = (PSR_IIC_FATAL_MASK | PSR_IIC_NACK_MASK);
}

// SPI Implementation (USIC0_CH1)

void USIC_SPI_Init(void) {
    /* The active sample link in the current hardware build is the GPIO-based
     * SPI path in common.c: P0.8 -> FPGA SPI_SCLK_IN (FPGA pin 36), MCU P0.7 -> FPGA SPI_MOSI_IN (FPGA pin 37)
     * MCU P0.0 -> FPGA SPI_CS_N_IN (FPGA pin 38)
     *
     * This CH1 setup is kept only as a reference path and is not the primary
     * transport used by the working prototype.
     */

    // 1. Disable channel and clear status.
    CH1_KSCFG &= ~(USIC_KSCFG_MODEN | USIC_KSCFG_BPMODEN);
    CH1_CCR = USIC_CCR_MODE_DISABLED;
    CH1_PSCR = 0xFFFFFFFFU;

    // 2) Configure output pins
    // P0.6  -> CH1 DX0C (MISO input)
    // P0.7  -> CH1 DOUT0 (MOSI, ALT7)
    // P0.8  -> CH1 SCLKOUT (ALT7)
    // P0.0  -> manual CS for FPGA
    // P0.9  -> manual CS for external flash
    P0_IOCR4 &= ~(0x1FUL << P0_6_IOCR_SHIFT); // P0.6 as input
    P0_IOCR4 |= (GPIO_MODE_INPUT_TRISTATE << P0_6_IOCR_SHIFT);

    P0_IOCR4 &= ~(0x1FUL << P0_7_IOCR_SHIFT);
    P0_IOCR4 |= (GPIO_MODE_PP_ALT7 << P0_7_IOCR_SHIFT);

    P0_IOCR8 &= ~((0x1FUL << P0_8_IOCR_SHIFT) | (0x1FUL << P0_9_IOCR_SHIFT));
    P0_IOCR8 |= (GPIO_MODE_PP_ALT7 << P0_8_IOCR_SHIFT);
    P0_IOCR8 |= (GPIO_MODE_PP_GPIO << P0_9_IOCR_SHIFT);

    P0_IOCR0 &= ~(0x1FUL << P0_0_IOCR_SHIFT);
    P0_IOCR0 |= (GPIO_MODE_PP_GPIO << P0_0_IOCR_SHIFT);

    // Deassert both manual chip-select outputs.
    P0_OMR = SPI_CS_FPGA_MASK | SPI_CS_FLASH_MASK;

    // 3. Configure a conservative SPI clock.
    // This is an intentionally slow first-bring-up setting; verify the exact
    // divider formula and SSC clock path against the XMC1100 manual.
    CH1_DX0CR = USIC_DXCR_INSW_INPUT | USIC_DXCR_DSEL_C;  // DIN0 from P0.6 (DX0C)
    CH1_DX1CR = USIC_DXCR_INSW_DIRECT | USIC_DXCR_DSEL_A; // Use BRG clock path directly.
    CH1_DX2CR = USIC_DXCR_INSW_DIRECT | USIC_DXCR_DSEL_HIGH; // No external SEL input; keep internal path active.

    CH1_FDR = USIC_FDR_DM_DIV | USIC_FDR_STEP_MAX;
    CH1_BRG = USIC_BRG_CLKSEL_DX1T |
              USIC_BRG_DCTQ(1U) |
              USIC_BRG_PDIV(63U) |
              USIC_BRG_SCLKCFG(0U);

    // 4. Configure 16-bit, MSB-first transfers in SSC mode.
    CH1_SCTR = USIC_SCTR_SDIR_MSB |
               USIC_SCTR_TRM_STD |
               USIC_SCTR_FLE(15U) |
               USIC_SCTR_WLE(15U);
    CH1_SCTR &= ~USIC_SCTR_PDL_MASK; // PDL=0 (idle-low base assumption)
    CH1_TCSR = USIC_TCSR_TDSSM | USIC_TCSR_TDEN(3U);
    CH1_PCR  = USIC_PCR_SSC_MSLSEN;
    CH1_PCR &= ~USIC_PCR_SSC_SLPHSEL; // SLPHSEL=0 (leading-edge sampling base assumption)
    CH1_CCR  = USIC_CCR_MODE_SSC;

    // 5. Enable the channel.
    CH1_KSCFG |= (USIC_KSCFG_MODEN | USIC_KSCFG_BPMODEN);
    (void)CH1_KSCFG;
}

bool USIC_SPI_Transfer(uint16_t data, uint8_t cs_select) {
    // Performs a single 16-bit simultaneous transmission and reception 
    // (full-duplex) using USIC0_CH1 Master, controlling the correct Chip Select line.
    uint32_t cs_mask = (cs_select == 0U) ? SPI_CS_FPGA_MASK : SPI_CS_FLASH_MASK;

    P0_OMR = (cs_mask << 16);   // 1. Assert the selected chip-select line.
    // 2. Clear any stale shift-complete indication, then start the transfer.
    USIC_ClearFlag_CH1(PSR_TSIF_MASK);
    CH1_TBUF = data; 

    // 3. Wait until the 16-bit word has shifted out completely.
    if (!USIC_WaitForFlag_CH1(PSR_TSIF_MASK)) {
        P0_OMR = cs_mask;
        return false;
    }
    USIC_ClearFlag_CH1(PSR_TSIF_MASK);

    P0_OMR = cs_mask;   // 4. Deassert the selected chip-select line.
    // 5. Read the receive register to clear the receive side path even though
    // the current FPGA path only uses MOSI.
    (void)CH1_RBUF;
    return true;
}
