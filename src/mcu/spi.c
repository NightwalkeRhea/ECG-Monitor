#include "spi.h"
#include "XMC1100.h"

#define CLK 
static void spi_clock_init(void);
static void spi_config_pins(void);
static void spi_set_pin_mode(void);

void write_register(uintptr_t addr, uint32_t value){
    volatile uint32_t *local_address = (volatile uint32_t *)addr;
    *local_address = value;
}




// spi.c

#include "spi.h"

// Helper function to busy-wait for Transmit Buffer Indication Flag (TBIF)
static void SPI_WaitForTBIF(void) {
    // TBIF is bit 13 in USIC_PSR
    // Wait until the Transmit Buffer is empty, indicating the last word was moved to the shift register.
    while ((USIC_PSR & (1U << 13)) == 0);
    // Clear TBIF flag by writing '1' to the corresponding bit in USIC_PSCR
    USIC_PSCR = (1U << 13);
}

// -----------------------------------------------------------------------------
// 1. SPI Initialization
// -----------------------------------------------------------------------------

void SPI_Init(void) {

    // --- A. Enable USIC0 Clock (SCU) ---
    
    // Clear the clock gating bit for USIC0 (bit 20) in SCU_CGATCLR0 to enable its clock.
    SCU_CGATCLR0 = (1U << SCU_CGATCLR0_USIC0_Pos); 

    // --- B. Configure GPIO Pins for Alternate Function (SSC Output) ---
    
    // P1.3 (SCLKOUT): ALT2 function (USIC0_CH1.SCLKOUT)
    // P1.2 (DOUT0 - MOSI): ALT2 function (USIC0_CH1.DOUT0)
    // P0.0 (SELO0 - CS_FPGA): ALT1 function (USIC0_CH1.SELO0)
    // P0.9 (SELO1 - CS_FLASH): ALT2 function (USIC0_CH1.SELO1)

    // Clear and set P1.3 (PC3 in P1_IOCR0) for ALT2
    P1_IOCR0 &= ~(0x1FUL << 27); // Clear PC3
    P1_IOCR0 |= (0x19U << 27);  // Set P1.3 to ALT2 Push-Pull (11001b)
    
    // Clear and set P1.2 (PC2 in P1_IOCR0) for ALT2
    P1_IOCR0 &= ~(0x1FUL << 19); // Clear PC2
    P1_IOCR0 |= (0x19U << 19);  // Set P1.2 to ALT2 Push-Pull (11001b) 

    // Clear and set P0.0 (PC0 in P0_IOCR0) for ALT1 (Chip Select 0 for FPGA)
    P0_IOCR0 &= ~(0x1FUL << 3);  // Clear PC0
    P0_IOCR0 |= (0x11U << 3);   // Set P0.0 to ALT1 Push-Pull (10001b)
    
    // Clear and set P0.9 (PC9 in P0_IOCR8) for ALT2 (Chip Select 1 for Flash)
    P0_IOCR8 &= ~(0x1FUL << 7);  // Clear PC9
    P0_IOCR8 |= (0x19U << 7);   // Set P0.9 to ALT2 Push-Pull (11001b)
    
    // --- C. Configure USIC0_CH1 for SSC Master Mode ---

    // 1. Disable Channel: Set KSCFG.MODEN = 0.
    USIC_KSCFG &= ~(1U << 0);
    
    // 2. Clear all protocol status flags (STx, RIF, TBIF, etc.)
    USIC_PSCR = 0xFFFFFFFFU; 

    // 3. Set Operating Mode to SSC (SSC Mode is 1H)
    USIC_CCR = (0x1U << 0); // CCR.MODE[3:0] = 0001b (SSC Protocol)
    
    // 4. Baud Rate Generator (BRG): Assuming a 64 MHz fPERIPH. 
    // Target SPI Clock Frequency (fSPI) = 10 MHz (High speed data link)
    // BRG.PDIV calculation: PDIV = fPERIPH / (2 * fSPI) - 1. 
    // PDIV = 64MHz / (2 * 10MHz) - 1 = 3.2 - 1 = 2.2. Use PDIV = 2.
    // BRG.PCTQ = 0 (1 time quantum per bit, standard)
    USIC_BRG = (0x02U << 0) | (0x00U << 10); // PDIV=2, PCTQ=0
    
    // 5. Fractional Divider Register (FDR): Bypass fractional part for simplicity (step=0x3FF, pdiv=1).
    USIC_FDR = (0x1U << 14) | (0x3FFU << 0); // FDR.DM = 1 (Divider Mode), FDR.STEP=0x3FF

    // 6. SSC Protocol Control Register (PCR): Configure phase, polarity, and Master Slave Select control.
    // MCLK = PCLK = 1. SSC clock phase/polarity must match FPGA slave.
    // Assuming standard SPI Mode 0 (CPOL=0, CPHA=0): PCR.CTQ_SCLK=1, PCR.SCLKSEL=0.
    // The SSC Master Select Line (SLS) is controlled by the USIC hardware (PCR.MSLSEN=1).
    USIC_PCR = (1U << 1) | // PCR.CTQ_SCLK: SCLK output delay = 1 (recommended)
               (0U << 4) | // PCR.SCLKSEL: Selects the USIC input stage used to sample SCLK (0=DX1) 
               (1U << 6) | // PCR.MSLSEN: Master Slave Select (MSLS) output enabled (CS active low)
               (0x02U << 12) | // PCR.SELON: Control which SELO pin is active at transfer start. (02h = SELO2 activated) 
               (0x02U << 18);  // PCR.SLSEN: Which Slave Select Line (SELOx) is used. (02h = SELO2 is used)
    
    // In our case, SELO0 (P0.0) is desired for FPGA (CS0) and SELO1 (P0.9) for Flash (CS1).
    // Let's adjust the PCR setting. We will use SELO0 for FPGA and SELO1 for Flash, managed by GPIO initially, 
    // but the XMC USIC SSC supports multiple Slave Select outputs (SELOx).
    // Let's use SELO0 (P0.0) for the FPGA and SELO1 (P0.9) for the Flash.
    
    // --- Re-Configuring CS Handling for Two Devices (FPGA: SELO0; Flash: SELO1) ---
    
    // We modify the selection bits (PCR.SLSEN) for *manual selection* in the transmit function, 
    // but keep MSLSEN=1 for hardware control of the active signal (low).
    
    // 7. Shift Control Register (SCTR): Frame configuration (16-bit data, MSB first)
    // WLE = 16 bits (FH), FLE = 16 bits (FH), SDIR = 1 (MSB first)
    USIC_SCTR = (0xFU << 24) | // WLE (Word Length) = 16 bits (FH) 
                (0xFU << 16) | // FLE (Frame Length) = 16 bits (FH) 
                (1U << 7);    // SDIR = 1 (MSB first) 

    // 8. Transmission Control Status Register (TCSR): Ensure hardware control is active
    // TDEN=1 (transmit data enabled), WLEMD=0 (no automatic frame length update)
    USIC_TCSR = (1U << 3); // TDEN=1 (Transmit Data Enabled)

    // 9. Enable Channel: Set KSCFG.MODEN = 1.
    USIC_KSCFG |= (1U << 0);
}


// -----------------------------------------------------------------------------
// 2. SPI Transmission (FPGA - uses SELO0 / P0.0)
// -----------------------------------------------------------------------------

uint16_t SPI_Transmit_FPGA(uint16_t data) {
    
    // Select SELO0 (P0.0) to activate the FPGA Chip Select line.
    // The selection is controlled by bits [17:16] in PCR. We use SELON.
    // SELON (bits [13:12] in PCR) selects the SELO line active at frame start.
    // SELON = 00b (SELO0)
    USIC_PCR &= ~(0x3U << 12); // Clear bits 13:12 (SELOx selection)
    USIC_PCR |= (0x0U << 12);  // Set SELON to 0 (SELO0/P0.0 is selected and asserted low by hardware)
    
    // Wait for transmit buffer ready and clear flag
    SPI_WaitForTBIF();
    
    // Write 16-bit data to TBUF. This initiates the transmission sequence and activates SELO0.
    USIC_TBUF = data; 

    // Wait for transmission complete (TBIF should be checked again after the write)
    SPI_WaitForTBIF();

    // Read received data (dummy read for data sent by FPGA Slave)
    // Reading RBUF also clears the Receive Indication Flag (RIF), bit 14 in PSR.
    return (uint16_t)USIC_RBUF;
}

// -----------------------------------------------------------------------------
// 3. SPI Transmission (Flash - uses SELO1 / P0.9)
// -----------------------------------------------------------------------------
// Note: This function is slightly different to ensure the correct chip select pin (P0.9) 
// is enabled when accessing the Flash memory (W25Q64).

uint16_t SPI_Transmit_Flash(uint16_t data) {

    // Select SELO1 (P0.9) to activate the Flash Chip Select line.
    // SELON = 01b (SELO1)
    USIC_PCR &= ~(0x3U << 12); // Clear bits 13:12
    USIC_PCR |= (0x1U << 12);  // Set SELON to 1 (SELO1/P0.9 is selected and asserted low by hardware)

    // Wait for transmit buffer ready and clear flag
    SPI_WaitForTBIF();

    // Write 16-bit data to TBUF. This initiates the transmission sequence and activates SELO1.
    USIC_TBUF = data;

    // Wait for transmission complete
    SPI_WaitForTBIF();
    
    // Read received data
    return (uint16_t)USIC_RBUF;
}