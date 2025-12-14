// usic.c

#include "usic.h"
#include <stdbool.h>

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

#define CCFG_SSC (1U<<0)
#define CCFG_ASC (1U<<1)
#define CCFG_IIC (1U<<2)
#define CCFG_RB  (1U<<6)
#define CCFG_TB  (1U<<7)


// --- USIC0 Channel 1 Register Pointers (SPI) ---
// USIC0_CH1_BASE = 0x48000200UL
#define CH1_KSCFG          (*(volatile uint32_t*)(USIC0_CH1_BASE + 0x0C))
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
// -----------------------------------------------------------------------------
// Helper Functions
// -----------------------------------------------------------------------------

/**
 * @brief Waits until the specified flag in the Protocol Status Register (PSR) of USIC0_CH0 is set.
 */
static void USIC_WaitForFlag_CH0(uint32_t flag_mask) {
    while ((CH0_PSR & flag_mask) == 0);
}

/**
 * @brief Waits until the specified flag in the Protocol Status Register (PSR) of USIC0_CH1 is set.
 */
static void USIC_WaitForFlag_CH1(uint32_t flag_mask) {
    while ((CH1_PSR & flag_mask) == 0);
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
    // Enable USIC0 module clock (Bit 20 in SCU_CGATCLR0) [14-153]
    // Writing '1' to CGATCLR0 clears the clock gating (enables the clock).
    SCU_CGATCLR0 = (1U << 20); 
}

// -----------------------------------------------------------------------------
// I2C Implementation (USIC0_CH0)
// -----------------------------------------------------------------------------

void USIC_I2C_Init(void) {
    // 1. Disable Channel and Clear Flags
    CH0_KSCFG &= ~(1U << 0);          // Disable Channel (MODEN=0) [14-165]
    CH0_CCR = 0x0U;         // CCR.MODE = 0000b
    CH0_PSCR = 0xFFFFFFFFU;           // Clear all status flags [14-170]
    // we make sure when configuring the input stages, the channel is not in IIC mode

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
    CH0_DX0CR = 1U; // DX0B Input: SDA input  -> P0.15 
    CH0_DX1CR = 0U; // DX1A Input: SCL input  -> P0.14  

    // Configuring protocol framing, 8bit data transfer and MSB first
    CH0_SCTR = (7U << 8) | (1U << 7); // SCTR.WLE=7, SDIR=1  [14-182]

    // 3. Protocol Setup (IIC Mode 4H)
    CH0_CCR = (0x4U << 0); // Set Mode to IIC (0100b) [14-159]

    // 4. Baud Rate Configuration (Targeting 100 kHz I2C Standard Mode)
    // PERIPHERAL_CLK_FREQ = 64 MHz. Target TQ frequency (fPCTQ) must be 10 * fIIC = 1 MHz.  
    // (STIM=0): PDIV=4, DCTQ=9 (10 TQ)
    // Baud Rate Formula (simplified for non-fractional divider): fPCTQ = fPERIPH / (PDIV + 1)
    // PDIV = (64 MHz / 1 MHz) - 1 = 63.
    CH0_FDR = (0x1U << 14) | (0x3FFU << 0); // select the clock source: FDR.DM=1 (Divider Mode), Bypass fractional step [14-177]
    CH0_BRG = (63U << 0); // BRG.PDIV = 63. fPCTQ = 64M / 64 = 1 MHz. [14-178]
    
    // 5. IIC Protocol Control
    // STIM = 0 (10 Time Quanta per symbol for 100 kHz Standard Mode) [14-130]
    // HDEL = 3 (Hardware Delay for SDA Hold Time ~300ns) [14-131, 14-118]
    CH0_PCR = (0U << 17) | // PCR.STIM = 0 (100 kBaud Standard Mode)
              (3U << 26); // PCR.HDEL = 3 (Adjust delay for hold time)
    
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
    USIC_WaitForFlag_CH0(PSR_TBIF_MASK);
    USIC_ClearFlag_CH0(PSR_TBIF_MASK);
    // Send pointer/register address byte
    CH0_TBUF = ((uint32_t)TDF_MASTER_TX << 8) | reg_addr;
    USIC_WaitForFlag_CH0(PSR_TBIF_MASK);
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
    USIC_WaitForFlag_CH0(PSR_TBIF_MASK);
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
    USIC_WaitForFlag_CH0(PSR_TBIF_MASK);
    USIC_ClearFlag_CH0(PSR_TBIF_MASK);

    if (stop_condition) {
        CH0_TBUF = ((uint32_t)TDF_MASTER_STOP << 8);
        USIC_WaitForFlag_CH0(PSR_PCR_MASK);   // Stop detected flag
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
    while (((CH0_PSR & PSR_AIF_MASK) == 0U) && ((CH0_PSR & PSR_RIF_MASK) == 0U)) { }

    // Clear whichever flag(s) fired
    if (CH0_PSR & PSR_AIF_MASK) USIC_ClearFlag_CH0(PSR_AIF_MASK);
    if (CH0_PSR & PSR_RIF_MASK) USIC_ClearFlag_CH0(PSR_RIF_MASK);
    uint8_t data = (uint8_t)CH0_RBUF;

    if (stop_condition) {
        // STOP must be a separate TDF after the last receive
        CH0_TBUF = ((uint32_t)TDF_MASTER_STOP << 8);
        USIC_WaitForFlag_CH0(PSR_PCR_MASK);
        USIC_ClearFlag_CH0(PSR_PCR_MASK);
    }
    return data;
}

// -----------------------------------------------------------------------------
// SPI Implementation (USIC0_CH1)
// -----------------------------------------------------------------------------

uint16_t USIC_SPI_Transfer(uint16_t data, uint8_t cs_select) {
    // Explanation: Performs a single 16-bit simultaneous transmission and reception 
    // (full-duplex) using USIC0_CH1 Master, controlling the correct Chip Select line.
    
    // 1. Select the correct Slave Select Output (SELOx) using PCR.SELON [14-99, 14-100]
    // cs_select=0 (FPGA) -> SELON=00b (SELO0/P0.0)
    // cs_select=1 (Flash) -> SELON=01b (SELO1/P0.9)
    CH1_PCR &= ~(0x3U << 12); // Clear bits 13:12
    CH1_PCR |= (cs_select << 12);  // Set SELON to 0 or 1 [14-100]
    
    // 2. Wait for Transmit Buffer Indication Flag (TBIF)
    USIC_WaitForFlag_CH1(PSR_TBIF_MASK);
    USIC_ClearFlag_CH1(PSR_TBIF_MASK);
    
    // 3. Write 16-bit data to TBUF (This initiates the SPI transfer and activates CS)
    CH1_TBUF = data; 

    // 4. Wait for Transmit Shift Indication Flag (TSIF) or RIF/AIF
    // TSIF (bit 12) indicates the word has finished shifting out. [14-132]
    USIC_WaitForFlag_CH1(PSR_TSIF_MASK);
    USIC_ClearFlag_CH1(PSR_TSIF_MASK);

    // 5. Read received data from RBUF (This automatically clears RIF/AIF when in buffer mode).
    return (uint16_t)CH1_RBUF;
}

// -----------------------------------------------------------------------------
// UART Implementation (USIC0_CH0)
// -----------------------------------------------------------------------------

void USIC_UART_Transmit(uint8_t data) {
    // Explanation: Transmits a single byte via the UART (ASC) protocol on USIC0_CH0.
    // This assumes USIC_UART_Init has been called.

    // 1. Wait for Transmit Buffer Indication Flag (TBIF) - ensures TBUF is ready.
    USIC_WaitForFlag_CH0(PSR_TBIF_MASK);
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

    // 2. Configure Pins (P0.15 = SCL, P0.14 = SDA) for Open-Drain ALT function
    // We assume GPIO pins were configured for ALT function during System_Init().
    
    // 3. Set Protocol Mode: IIC Mode (4H = 0100b)
    CH0_CCR = (0x4U << 0);            // Set Mode to IIC (0100b) 

    // 4. IIC Protocol Control (Minimal requirements for Master):
    // STIM = 0 (100 kBaud Standard Mode Timing)
    // HDEL = 3 (Hardware Delay)
    CH0_PCR = (0U << 17) | // PCR.STIM = 0 
              (3U << 26);             // PCR.HDEL = 3 (Delay for SDA Hold Time) 

    // 5. Enable Channel
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

    // 2. Configure Pins (P0.15 = TXD, P0.14 = RXD) 
    // We assume pins were configured for ALT function during System_Init().

    // 3. Set Protocol Mode: ASC Mode (2H = 0010b)
    CH0_CCR = (0x2U << 0);            // Set Mode to ASC (0010b) 
    
    // 4. UART Protocol Control (Minimal requirements for TX only):
    // Baud Rate setup (PDIV/FDR must be configured during USIC_UART_Init)
    // Here we skip complex PCR settings for simplicity in bare-metal TX, only setting the mode.

    // 5. Enable Channel
    CH0_KSCFG |= (1U << 0);
    (void)CH0_KSCFG;
}

// -----------------------------------------------------------------------------
// Initialization Functions (Now only configure registers that are mode-agnostic)
// -----------------------------------------------------------------------------

void USIC_I2C_Init(void) {
    // Configure shared I2C/UART pins (P0.15, P0.14) for the alternate function needed by USIC.
    // For I2C, they need Open-Drain ALT function. For UART, they need Push-Pull ALT function.
    // We choose the most compatible output type (typically Push-Pull/ALT function if pins support both)
    // and let the USIC internal hardware handle the final output mode (Open-Drain for I2C).
    
    // Example: P0.15 (SCL/TXD) and P0.14 (SDA/RXD) to ALT functionality. 
    // This part is done once during System_Init().
    // We then call USIC_SetMode_I2C_CH0() immediately after this function.
    
    // Set up Baud Rate Generator for 100 kHz I2C (or 115200 UART) 
    // Since BRG is shared, set it conservatively here (e.g., for I2C):
    CH0_FDR = (0x1U << 14) | (0x3FFU << 0); // FDR.DM=1, Bypass fractional step 
    CH0_BRG = (63U << 0); // PDIV=63 for 1 MHz TQ (64 MHz/64 = 1 MHz) 

    // Set Initial Operating Mode (I2C)
    USIC_SetMode_I2C_CH0(); 
}

void USIC_UART_Init(void) {
    // In this revised plan, USIC_UART_Init is essentially absorbed by USIC_I2C_Init()
    // regarding shared hardware setup (Clock, FDR, BRG).
    // The specific UART mode (ASC) configuration is handled exclusively in USIC_SetMode_UART_CH0().
    
    // Therefore, this function can be empty if it only sets up shared channel 0 resources. 
    // If it was intended for CH1, it would contain dedicated pin setup for UART on CH1 pins.
}