// usic.c

#include "usic.h"
#include <stdbool.h>
#include <stddef.h>

// --- Register Pointers for USIC0 Channel 0 (I2C/UART) ---
// USIC0_CH0_BASE = 0x48000000UL -> check mcu_definition.h for the register mapping. 
#define CH0_KSCFG          (*(volatile uint32_t*)(USIC0_CH0_BASE + 0x0C)) // Kernel State Configuration
#define CH0_CCR            (*(volatile uint32_t*)(USIC0_CH0_BASE + 0x40)) // Channel Control Register
#define CH0_CCFG           (*(volatile uint32_t*)(USIC0_CH0_BASE + 0x04))
#define CH0_FDR            (*(volatile uint32_t*)(USIC0_CH0_BASE + 0x10)) // Fractional Divider Register
#define CH0_BRG            (*(volatile uint32_t*)(USIC0_CH0_BASE + 0x14)) // Baud Rate Generator
#define CH0_SCTR           (*(volatile uint32_t*)(USIC0_CH0_BASE + 0x34)) // Shift Control Register
#define CH0_PCR            (*(volatile uint32_t*)(USIC0_CH0_BASE + 0x3C)) // Protocol Control Register (IIC/ASC specific)
#define CH0_PSR            (*(volatile uint32_t*)(USIC0_CH0_BASE + 0x48)) // Protocol Status Register
#define CH0_PSCR           (*(volatile uint32_t*)(USIC0_CH0_BASE + 0x4C)) // Protocol Status Clear Register
#define CH0_TBUF           (*(volatile uint32_t*)(USIC0_CH0_BASE + 0x80)) // Transmit Buffer Input Location
#define CH0_RBUF           (*(volatile uint32_t*)(USIC0_CH0_BASE + 0x54)) // Receive Buffer Register
#define CH0_DX0CR          (*(volatile uint32_t*)(USIC0_CH0_BASE + 0x1C)) // Input Control Register 0 (SDA/RXD)
#define CH0_DX1CR          (*(volatile uint32_t*)(USIC0_CH0_BASE + 0x20)) // Input Control Register 1 (SCL/TXD)
#define CH0_DX2CR          (*(volatile uint32_t*)(USIC0_CH0_BASE + 0x24)) // Input Control Register 2 (ASC collision/idle detection path)

#define CCFG_SSC (1U<<0)
#define CCFG_ASC (1U<<1)
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

#define GPIO_MODE_INPUT_TRISTATE (0x00U)
#define GPIO_MODE_PP_GPIO        (0x10U)
#define GPIO_MODE_PP_ALT7        (0x17U)

#define P0_0_IOCR_SHIFT        (3U)
#define P0_6_IOCR_SHIFT        (19U)
#define P0_7_IOCR_SHIFT        (27U)
#define P0_8_IOCR_SHIFT        (3U)
#define P0_9_IOCR_SHIFT        (11U)

#define SPI_CS_FPGA_MASK       (1U << 0)
#define SPI_CS_FLASH_MASK      (1U << 9)

// Common USIC bit field helpers
#define USIC_CCR_MODE_DISABLED (0x0U)
#define USIC_CCR_MODE_SSC      (0x1U)
#define USIC_CCR_MODE_ASC      (0x2U)
#define USIC_CCR_MODE_IIC      (0x4U)

#define USIC_SCTR_SDIR_MSB     (1U << 0)
#define USIC_SCTR_PDL_MASK     (1U << 1)
#define USIC_SCTR_DOCFG_OD     (0U << 6)
#define USIC_SCTR_TRM_STD      (1U << 8) // ASC/SSC recommendation: TRM = 01b
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
#define USIC_DXCR_INSW_DIRECT  (0U << 4)
#define USIC_DXCR_INSW_INPUT   (1U << 4)

// IIC TDF (Transfer Data Format) Codes (Bits [10:8] when writing to TBUF) [14-121]
#define TDF_MASTER_TX          (0x0U) // 000b: Send data byte (master transmit)
#define TDF_MASTER_RX_ACK      (0x2U) // 010b: Receive byte and send ACK
#define TDF_MASTER_RX_NACK     (0x3U) // 011b: Receive Byte and send NACK
#define TDF_MASTER_START       (0x4U) // 100b: Send START and address in TBUF[7:0]
#define TDF_MASTER_RESTART     (0x5U) // 101b: Send repeated START and address in TBUF[7:0]
#define TDF_MASTER_STOP        (0x6U) // 110b: Send STOP Condition

// PSR Flag Masks
#define PSR_TBIF_MASK          (1U << 13) // Transmit Buffer Indication Flag
#define PSR_RIF_MASK           (1U << 14) // Receive Indication Flag
#define PSR_AIF_MASK           (1U << 15) // Alternative Receive Indication Flag
#define PSR_PCR_MASK           (1U << 4)  // IIC: Stop Condition Received Flag
#define PSR_TSIF_MASK          (1U << 12) // indicates the word has finished shifting out.

#define PSR_IIC_NACK_MASK      (1U << 5)
#define PSR_IIC_ARL_MASK       (1U << 6)
#define PSR_IIC_ERR_MASK       (1U << 8)
#define PSR_IIC_ERROR_MASK     (PSR_IIC_NACK_MASK | PSR_IIC_ARL_MASK | PSR_IIC_ERR_MASK)

#define USIC_WAIT_TIMEOUT      (200000UL)
#define USIC_I2C_ERR_TIMEOUT   (1UL << 31)

static volatile uint32_t g_usic_i2c_errors = 0U;
// -----------------------------------------------------------------------------
// Helper Functions
// -----------------------------------------------------------------------------

/**
 * @brief Waits until the specified flag in the Protocol Status Register (PSR) of USIC0_CH0 is set.
 */
static bool USIC_WaitForFlag_CH0(uint32_t flag_mask) {
    uint32_t timeout = USIC_WAIT_TIMEOUT;

    while (timeout-- != 0U) {
        uint32_t psr = CH0_PSR;

        if ((psr & PSR_IIC_ERROR_MASK) != 0U) {
            g_usic_i2c_errors |= (psr & PSR_IIC_ERROR_MASK);
            CH0_PSCR = (psr & PSR_IIC_ERROR_MASK);
            return false;
        }

        if ((psr & flag_mask) != 0U) {
            return true;
        }
    }

    g_usic_i2c_errors |= USIC_I2C_ERR_TIMEOUT;
    return false;
}

/**
 * @brief Waits until the specified flag in the Protocol Status Register (PSR) of USIC0_CH1 is set.
 */
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
        uint32_t psr = CH0_PSR;

        if ((psr & PSR_IIC_ERROR_MASK) != 0U) {
            g_usic_i2c_errors |= (psr & PSR_IIC_ERROR_MASK);
            CH0_PSCR = (psr & PSR_IIC_ERROR_MASK);
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

/**
 * @brief Clears the specified flag in the Protocol Status Register (PSR) of USIC0_CH0.
 */
static void USIC_ClearFlag_CH0(uint32_t flag_mask) {
    CH0_PSCR = flag_mask;
}

/**
 * @brief Clears the specified flag in the Protocol Status Register (PSR) of USIC0_CH1.
 */
static void USIC_ClearFlag_CH1(uint32_t flag_mask) {
    CH1_PSCR = flag_mask;
}

// -----------------------------------------------------------------------------
// USIC Core Functions
// -----------------------------------------------------------------------------

void USIC_Clock_Enable(void) {
    // Enable USIC0 module clock (bit 3 in SCU_CGATCLR0)
    // Use the same bit-protection sequence used by startup code so this works
    // regardless of current SCU protection state.
    SCU_PASSWD = 0x000000C0UL; // disable bit protection
    SCU_CGATCLR0 = (1U << 3);
    SCU_PASSWD = 0x000000C3UL; // enable bit protection
}

// -----------------------------------------------------------------------------
// I2C Implementation (USIC0_CH0)
// -----------------------------------------------------------------------------

void USIC_I2C_Init(void) {
    // 1. Disable Channel and Clear Flags
    CH0_KSCFG &= ~(1U << 0);          // Disable Channel (MODEN=0)
    CH0_CCR = USIC_CCR_MODE_DISABLED; // CCR.MODE = 0000b
    CH0_PSCR = 0xFFFFFFFFU;           // Clear all status flags [14-170]
    g_usic_i2c_errors = 0U;
    // Configure input stages while mode is disabled to avoid unintended edges.

    // 2. Configure Pins (P0.15 = SDA, P0.14 = SCL)
    // We target Alternate Function Open-Drain Output (e.g., ALT7: 11111b)
    // Looking at the user manual, P0.14 -> USIC0_CH0. DOUT0 is ALT6, USIC0_CH0. SCLKOUT is ALT7.
    // P0.15 -> USIC0_CH0. DOUT0 is ALT6, USIC0_CH1. MCLKOUT is ALT7 
    #define ALT7_OPEN_DRAIN (0x1FU) 
    #define ALT6_OPEN_DRAIN (0x1EU)
    P0_IOCR12 &= ~((0x1FUL << 27) | (0x1FUL << 19)); // Clear bits for P0.15/P0.14
    P0_IOCR12 |= (ALT6_OPEN_DRAIN << 27) | (ALT7_OPEN_DRAIN << 19); // set SDA + SCL
    // Input Stage Configuration (IIC mode uses DX0 for SDA, DX1 for SCL)
    // Select the pin inputs. 
    // Note: IIC automatically links output DOUT0 to the input DX0 line internally for sensing [14-109].
    CH0_DX0CR = USIC_DXCR_INSW_DIRECT | USIC_DXCR_DSEL_B; // DX0B input: SDA -> P0.15
    CH0_DX1CR = USIC_DXCR_INSW_DIRECT | USIC_DXCR_DSEL_A; // DX1A input: SCL -> P0.14
    CH0_DX2CR = USIC_DXCR_INSW_DIRECT | USIC_DXCR_DSEL_A; // Not used in IIC, keep direct path.

    // Data format per IIC guidance:
    // WLE=7 (8-bit), FLE=0x3F (unlimited), SDIR=1 (MSB first), TRM=11b.
    CH0_SCTR = USIC_SCTR_SDIR_MSB |
               USIC_SCTR_DOCFG_OD |
               USIC_SCTR_TRM_IIC |
               USIC_SCTR_FLE(0x3FU) |
               USIC_SCTR_WLE(7U);

    // 3. Baud Rate Configuration (Targeting 100 kHz I2C Standard Mode)
    // Use divider mode with STEP=max (base clock), then set BRG fields explicitly.
    CH0_FDR = USIC_FDR_DM_DIV | USIC_FDR_STEP_MAX;
    CH0_BRG = USIC_BRG_CLKSEL_DX1T |
              USIC_BRG_DCTQ(9U) |
              USIC_BRG_PDIV(63U) |
              USIC_BRG_SCLKCFG(0U);
    
    // 4. IIC Protocol Control
    // STIM = 0 (10 Time Quanta per symbol for 100 kHz Standard Mode) [14-130]
    // HDEL = 3 (Hardware Delay for SDA Hold Time ~300ns) [14-131, 14-118]
    CH0_PCR = (0U << 17) | // PCR.STIM = 0 (100 kBaud Standard Mode)
              (3U << 26); // PCR.HDEL = 3 (Adjust delay for hold time)

    // 5. Protocol Setup (IIC Mode 4H)
    CH0_CCR = USIC_CCR_MODE_IIC;
    
    // 8. Enable Channel
    CH0_KSCFG |= (1U << 0); // MODEN=1 [14-165]
}

/**
 * @brief Sends the START condition, Slave Address (with Write bit), and Register Address.
 * @param slave_addr 7-bit I2C address of the target device.
 * @param reg_addr Internal register address to target.
 */
void USIC_I2C_StartWrite(uint8_t slave_addr, uint8_t reg_addr) {
    // Explanation: Prepares the USIC TBUF to send the I2C START condition, 
    // the target slave address (left-shifted, with R/W bit low), and the 
    // internal register pointer address byte (reg_addr).
    uint8_t addr_w = (uint8_t)((slave_addr << 1) | 0U);
    // 1. Start + address for writing.
    // The IIC PPP extracts the slave address from the TBUF location write. [14-121]
    CH0_TBUF = ((uint32_t)TDF_MASTER_START << 8) | addr_w; 
    if (!USIC_WaitForFlag_CH0(PSR_TBIF_MASK)) {
        return;
    }
    USIC_ClearFlag_CH0(PSR_TBIF_MASK);
    // Send pointer/register address byte
    CH0_TBUF = ((uint32_t)TDF_MASTER_TX << 8) | reg_addr;
    if (!USIC_WaitForFlag_CH0(PSR_TBIF_MASK)) {
        return;
    }
    USIC_ClearFlag_CH0(PSR_TBIF_MASK);
}

/**
 * @brief Sends the repeated START condition
 * @param slave_addr 7-bit I2C address of the target device 
 */
void USIC_I2C_RepeatedStartRead(uint8_t slave_addr)
{
    uint8_t addr_r = (uint8_t)((slave_addr << 1) | 1U);

    CH0_TBUF = ((uint32_t)TDF_MASTER_RESTART << 8) | addr_r;
    if (!USIC_WaitForFlag_CH0(PSR_TBIF_MASK)) {
        return;
    }
    USIC_ClearFlag_CH0(PSR_TBIF_MASK);
}

/**
 * @brief Sends a data byte followed by an optional STOP condition.
 * @param data Data byte to send (MSB first due to SCTR configuration).
 * @param stop_condition True if this is the last byte and a STOP condition should be sent.
 */
void USIC_I2C_SendByte(uint8_t data, uint8_t stop_condition) {
    // Explanation: Sends a single data byte (e.g., config MSB or LSB). 
    // If stop_condition is true, the transaction is terminated with TDF_NACK_STOP code.
    
    CH0_TBUF = ((uint32_t)TDF_MASTER_TX << 8) | data;
    if (!USIC_WaitForFlag_CH0(PSR_TBIF_MASK)) {
        return;
    }
    USIC_ClearFlag_CH0(PSR_TBIF_MASK);

    if (stop_condition) {
        CH0_TBUF = ((uint32_t)TDF_MASTER_STOP << 8);
        if (!USIC_WaitForFlag_CH0(PSR_PCR_MASK)) {
            return;
        }
        USIC_ClearFlag_CH0(PSR_PCR_MASK);
    }
}

/**
 * @brief Reads a single data byte (Master Receive).
 * @param ack_condition True to send ACK (expecting more data), False to send NACK (last byte).
 * @param stop_condition True to terminate transaction with STOP after NACK.
 * @return The received data byte (bits 7:0 of RBUF).
 */
uint8_t USIC_I2C_ReadByte(uint8_t ack_condition, uint8_t stop_condition) {
    // Explanation: This function is called after the Repeated START is sent (in comm_api). 
    // It triggers the next byte read and controls the ACK/NACK/STOP handshake.

    // 1. Send dummy word to trigger the read operation and set the ACK/NACK condition.
    uint32_t tdf = ack_condition ? TDF_MASTER_RX_ACK : TDF_MASTER_RX_NACK;
    // 010b (ACK) for intermediate bytes and 011b (NACK) for the last byte, then STOP.
    
    // Trigger receive (TBUF[7:0] ignored for RX formats)
    CH0_TBUF = ((uint32_t)tdf << 8);

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
    uint8_t data = (uint8_t)CH0_RBUF;

    if (stop_condition) {
        // STOP must be a separate TDF after the last receive
        CH0_TBUF = ((uint32_t)TDF_MASTER_STOP << 8);
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
    CH0_PSCR = PSR_IIC_ERROR_MASK;
}

// -----------------------------------------------------------------------------
// SPI Implementation (USIC0_CH1)
// -----------------------------------------------------------------------------

void USIC_SPI_Init(void) {
    // Configure USIC0_CH1 as a simple SPI master for 16-bit writes to the FPGA.
    // The current implementation drives chip-select manually on GPIO pins so the
    // basic data path does not depend on SSC-specific SELO automation.

    // 1. Disable channel and clear status.
    CH1_KSCFG &= ~(1U << 0);
    CH1_CCR = USIC_CCR_MODE_DISABLED;
    CH1_PSCR = 0xFFFFFFFFU;

    // 2. Configure output pins.
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
    CH1_DX2CR = USIC_DXCR_INSW_DIRECT | USIC_DXCR_DSEL_A; // Keep internal MSLS path direct.

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
    CH1_PCR  = 0x0U;
    CH1_PCR &= ~(1U << 25);   // SLPHSEL=0 (leading-edge sampling base assumption)
    CH1_CCR  = USIC_CCR_MODE_SSC;

    // 5. Enable the channel.
    CH1_KSCFG |= (1U << 0);
}

uint16_t USIC_SPI_Transfer(uint16_t data, uint8_t cs_select) {
    // Explanation: Performs a single 16-bit simultaneous transmission and reception 
    // (full-duplex) using USIC0_CH1 Master, controlling the correct Chip Select line.
    uint32_t cs_mask = (cs_select == 0U) ? SPI_CS_FPGA_MASK : SPI_CS_FLASH_MASK;

    // 1. Assert the selected chip-select line.
    P0_OMR = (cs_mask << 16);

    // 2. Clear any stale shift-complete indication, then start the transfer.
    USIC_ClearFlag_CH1(PSR_TSIF_MASK);
    CH1_TBUF = data; 

    // 3. Wait until the 16-bit word has shifted out completely.
    if (!USIC_WaitForFlag_CH1(PSR_TSIF_MASK)) {
        P0_OMR = cs_mask;
        return 0U;
    }
    USIC_ClearFlag_CH1(PSR_TSIF_MASK);

    // 4. Deassert the selected chip-select line.
    P0_OMR = cs_mask;

    // 5. Return the receive register for completeness, even though the current
    // FPGA path only needs MOSI.
    return (uint16_t)CH1_RBUF;
}

// -----------------------------------------------------------------------------
// UART Implementation (USIC0_CH0)
// -----------------------------------------------------------------------------

void USIC_UART_Transmit(uint8_t data) {
    // Explanation: Transmits a single byte via the UART (ASC) protocol on USIC0_CH0.
    // This assumes USIC_UART_Init has been called.

    // 1. Wait for Transmit Buffer Indication Flag (TBIF) - ensures TBUF is ready.
    uint32_t timeout = USIC_WAIT_TIMEOUT;
    while ((timeout-- != 0U) && ((CH0_PSR & PSR_TBIF_MASK) == 0U)) { }
    if ((CH0_PSR & PSR_TBIF_MASK) == 0U) {
        return;
    }
    USIC_ClearFlag_CH0(PSR_TBIF_MASK);
    
    // 2. Write data byte to TBUF. This triggers the transmission.
    // The data transfer is 8 bits (WLE=7) as configured in USIC_UART_Init.
    CH0_TBUF = data; 
}

// USIC0 mode switching implementation
/**
 * @brief Configures USIC0_CH0 pins for I2C and sets protocol to IIC Mode (4H).
 * 
 * This sequence is called during System_Init() and when switching back from UART mode.
 */
void USIC_SetMode_I2C_CH0(void) {
    // 1. Disable Channel (Required before setting/changing mode)
    CH0_KSCFG &= ~(1U << 0);          
    CH0_PSCR = 0xFFFFFFFFU;           // Clear all flags
    CH0_CCR = USIC_CCR_MODE_DISABLED; // Configure while mode is disabled.

    // 2. Restore IIC input paths and data format.
    CH0_DX0CR = USIC_DXCR_INSW_DIRECT | USIC_DXCR_DSEL_B; // SDA path
    CH0_DX1CR = USIC_DXCR_INSW_DIRECT | USIC_DXCR_DSEL_A; // SCL path
    CH0_DX2CR = USIC_DXCR_INSW_DIRECT | USIC_DXCR_DSEL_A;
    CH0_SCTR = USIC_SCTR_SDIR_MSB |
               USIC_SCTR_DOCFG_OD |
               USIC_SCTR_TRM_IIC |
               USIC_SCTR_FLE(0x3FU) |
               USIC_SCTR_WLE(7U);
    CH0_FDR = USIC_FDR_DM_DIV | USIC_FDR_STEP_MAX;
    CH0_BRG = USIC_BRG_CLKSEL_DX1T |
              USIC_BRG_DCTQ(9U) |
              USIC_BRG_PDIV(63U) |
              USIC_BRG_SCLKCFG(0U);

    // 3. IIC Protocol Control (Minimal requirements for Master):
    // STIM = 0 (100 kBaud Standard Mode Timing)
    // HDEL = 3 (Hardware Delay)
    CH0_PCR = (0U << 17) | // PCR.STIM = 0 
              (3U << 26);             // PCR.HDEL = 3 (Delay for SDA Hold Time) 

    CH0_CCR = USIC_CCR_MODE_IIC;

    // 4. Enable Channel
    CH0_KSCFG |= (1U << 0);
    // Read back KSCFG after enabling to avoid pipeline effects 
    (void)CH0_KSCFG;
}

/**
 * @brief Configures USIC0_CH0 pins for UART and sets protocol to ASC Mode (2H).
 * 
 * This sequence is called just before sending data to the PC Terminal.
 */
void USIC_SetMode_UART_CH0(void) {
    // 1. Disable Channel and Clear Flags
    CH0_KSCFG &= ~(1U << 0);          // Disable Channel
    CH0_PSCR = 0xFFFFFFFFU;           // Clear all flags
    CH0_CCR = USIC_CCR_MODE_DISABLED; // Configure while mode is disabled.

    // 2. Configure ASC input paths (INSW=0 as recommended).
    CH0_DX0CR = USIC_DXCR_INSW_DIRECT | USIC_DXCR_DSEL_A; // RX input selection
    CH0_DX1CR = USIC_DXCR_INSW_DIRECT | USIC_DXCR_DSEL_A; // TX feedback/collision path
    CH0_DX2CR = USIC_DXCR_INSW_DIRECT | USIC_DXCR_DSEL_A;

    // 3. Configure basic ASC framing with 8-bit words and TRM=01b.
    CH0_SCTR = USIC_SCTR_TRM_STD |
               USIC_SCTR_FLE(7U) |
               USIC_SCTR_WLE(7U);
    CH0_PCR = 0x0U;
    CH0_CCR = USIC_CCR_MODE_ASC;
    
    // 4. Enable Channel
    CH0_KSCFG |= (1U << 0);
    (void)CH0_KSCFG;
}

void USIC_UART_Init(void) {
    // In this revised plan, USIC_UART_Init is essentially absorbed by USIC_I2C_Init()
    // regarding shared hardware setup (Clock, FDR, BRG).
    // The specific UART mode (ASC) configuration is handled exclusively in USIC_SetMode_UART_CH0().
    
    // Therefore, this function can be empty if it only sets up shared channel 0 resources. 
    // If it was intended for CH1, it would contain dedicated pin setup for UART on CH1 pins.
}
