#include "i2c.h"
// #include "gpio.h"
/*
The IIC mode is selected by CCR.MODE = 0100B with CCFG.IIC = 1 (IIC mode
available).
*/
static void i2c_start(uint8_t addr, uint8_t rw_bit);
static void i2c_stop(void);
static int  i2c_write_byte(uint8_t byte);
static int  i2c_read_byte_ack(uint8_t *byte);
static int  i2c_read_byte_nack(uint8_t *byte);

// CCR Channel Control Register (40H) needs bits [3:0] for operating mode.
/*
When switching between two
protocols, the USIC channel has to be disabled
before selecting a new protocol. In this case, registers
PCR and PSR have to be cleared or updated by
software.
0H The USIC channel is disabled. All protocolrelated
state machines are set to an idle state.
1H The SSC (SPI) protocol is selected.
2H The ASC (SCI, UART) protocol is selected.
3H The IIS protocol is selected.
4H The IIC protocol is selected.
Other bit combinations are reserved.
*/
void i2c_init(void){
    // cfg->speed_hz = 10000;
    //1. call to begin the transaction
    /*
    enable i2c periph clk, gpio periph clk,
    configure gpio with mode, speed, output type
    configure i2c with clk speed, mode, duty cycle, own address, ack, ack_address [6:0] */
    
}

void i2c_send_byte(){
    // 2.send the slave device's address with r/w bit set to 0 for write operation
    // 3.call again to send the desired register address
    // 4. call again to send the data byte to that register (we won't need now)
}

static void i2c_start(uint8_t addr, uint8_t rw_bit){
    // how am I supposed to find TBUF?
    // (0x48000080)
    // write on TBUF, with start condition control bits set.
    // Generate START condition
    // Send slave address with R/W bit
    // Wait for address acknowledge
}
static void i2c_stop(void){
    // 5. call to end the transaction
}


int i2c_write(uint8_t addr, const uint8_t *data, uint16_t  len){
    
    // start, address and write (0)
    i2c_start(addr, 0);
    // start sending bytes, check if it's not receiving ack
    for(uint16_t i = 0; i<len; ++i){
        if(i2c_write_byte(data[i]) != 0){
            i2c_stop();
            return -1;
        }
    }
    // now stop
    i2c_stop();
    return 0;
}

int i2c_read(uint8_t addr, uint8_t *data, uint16_t len){
    i2c_start(addr, 1); // 1 for reading
      
    for(uint16_t i = 0; i<len; ++i){
        if(i == (len - 1)){
            // it's the last byte, so read and send NACK. WHY NACK?
            if(i2c_rfead_byte_nack(&data[i]) != 0){
                i2c_stop();
                return -1;
            }
        } else {
            // read and send ack. 
            if(i2c_read_byte_ack(&data[i]) != 0) {
                i2c_stop();
                return -1;
            }
        }
    }
    i2c_stop();
    return 0;
}

int i2c_write_reg(uint8_t addr, uint8_t reg, const uint8_t *data, uint16_t len){
    // build temp buffer: [reg, data0, data1, etc]
    // then call i2c_write(addr, buf, len+1)
}

int i2c_read_reg(uint8_t addr, uint8_t reg, uint8_t *data, uint16_t len){
    // write only the register address (pointer)
    // i2c_write(addr, &reg, 1);
    // then i2c_read(addr, data, len);
    /*
    This is where you will use the register addresses from ADS1115 chapter 8: ADS1115_REG_CONFIG = 0x01, etc.
    */
}


// Note: This example assumes a system clock (fPERIPH) of 64 MHz, a common clock for XMC1100.
// We target 100 kHz I2C clock (Standard Mode, STIM = 0).

// Helper function to busy-wait for a status flag in the Protocol Status Register (PSR)
static void I2C_WaitForFlag(uint32_t flag_mask) {
    // Wait until the specified flag bit in the Protocol Status Register (USIC_PSR) is set.
    while ((USIC_PSR & flag_mask) == 0);
}

// Helper function to clear a status flag using the Protocol Status Clear Register (PSCR)
static void I2C_ClearFlag(uint32_t flag_mask) {
    // Write '1' to the corresponding bit in the Protocol Status Clear Register (USIC_PSCR)
    // to clear the flag in USIC_PSR.
    USIC_PSCR = flag_mask;
}

// -----------------------------------------------------------------------------
// 1. I2C Initialization
// -----------------------------------------------------------------------------

void i2c_init(void) {

    // --- A. Enable USIC0 Clock (SCU) ---
    
    // Clear the clock gating bit for USIC0 (bit 20) in SCU_CGATCLR0 to enable its clock.
    // USIC0 is bit 20 in CGATSET0/CGATCLR0 register (Table 12-7).
    SCU_CGATCLR0 = (1U << SCU_CGATCLR0_USIC0_Pos); 

    // --- B. Configure GPIO Pins (P0.15 = SCL, P0.14 = SDA) ---
    
    // Both SCL and SDA need to be configured for:
    // 1. Alternate Output Function (e.g., ALT7 for USIC0_CH0 - PCx[4:0] = 11111b or similar Open-Drain setting)
    // 2. Open-Drain output mode (PCx[1] = 1, PCx[2]=1). We use 11000b for Open-Drain GPIO (Base for ALT functions).
    // The safest selection is typically the Open-Drain configuration for the highest Alternate Function (ALT7).
    // For P0.15 (SCL), we target bit field PC15 (bits 31:27 of P0_IOCR12).
    // For P0.14 (SDA), we target bit field PC14 (bits 23:19 of P0_IOCR12).
    
    // Clear the bits first
    P0_IOCR12 &= ~((0x1FUL << 27) | (0x1FUL << 19));

    // Set PC15 for SCL (P0.15) and PC14 for SDA (P0.14) to Open-Drain Alternate Function (e.g., ALT7: 11111b)
    // We use a general Open-Drain mode (11000b) and rely on USIC's internal logic to assign SCL/SDA.
    // In IIC mode, the USIC controls the SCL/SDA lines.
    // We use 11000b for Open-Drain + General Purpose (which is then overridden by USIC IIC mode).
    // A better choice for ALT7 Open-Drain is often 11111b. We use 11111b for robust ALT selection.
    #define ALT_IIC_OPEN_DRAIN (0x1FU) // 11111b for Open-Drain ALT7
    P0_IOCR12 |= (ALT_IIC_OPEN_DRAIN << 27); // P0.15 (SCL)
    P0_IOCR12 |= (ALT_IIC_OPEN_DRAIN << 19); // P0.14 (SDA)

    // --- C. Configure USIC0_CH0 for IIC Mode ---
    
    // 1. Disable Channel: Set KSCFG.MODEN = 0.
    USIC_KSCFG &= ~(1U << 0);
    
    // 2. Clear flags before configuring.
    // Clear all protocol status flags (STx, RSIF, DLIF, etc.) by writing 1s to USIC_PSCR.
    USIC_PSCR = 0xFFFFFFFFU; 

    // 3. Set Operating Mode to IIC (IIC Mode is 4H)
    USIC_CCR = (0x4U << 0); // CCR.MODE[3:0] = 0100b (IIC Protocol)
    
    // 4. Baud Rate Generator (BRG): Standard IIC is typically 100/400 kHz.
    // We assume a 64 MHz fPERIPH. The actual calculation requires accurate system clock values.
    // This example uses typical values for 100kHz I2C:
    // fIIC = fPERIPH / (PDIV + 1) * (STIM + 1)
    // For 100 kHz, with STIM=0 (10 time quanta, PCR.STIM[3]=0) we use a PDIV of 4.
    // PDIV is 10 bits [9:0] of BRG.
    USIC_BRG = (4U << 0); // BRG.PDIV = 4
    
    // 5. Fractional Divider Register (FDR): Bypass fractional part (step=0, pdiv=1).
    // We use step=0x3ff, pdiv=0 (standard run mode for non-fractional timing).
    USIC_FDR = (0x1U << 14) | (0x3FFU << 0); // FDR.DM = 1 (Divider Mode), FDR.STEP=0x3FF

    // 6. Set Protocol Control Register (PCR) for IIC timings.
    // STIM = 0: 10 time quanta symbol timing (Standard Mode 100kHz) 
    USIC_PCR &= ~(1U << 17); // PCR.STIM = 0 (100 kbit/s symbol timing)
    
    // Set 8-bit Data Transfer (WLE = 7 for 8 bits) and MSB first (SDIR=1)
    USIC_SCTR = (7U << 8) | (1U << 7); 
    
    // 7. Enable Channel: Set KSCFG.MODEN = 1.
    USIC_KSCFG |= (1U << 0);
}


// -----------------------------------------------------------------------------
// 2. Write ADS1115 Configuration Register
// -----------------------------------------------------------------------------

void I2C_WriteConfig(void) {
    // This routine writes the ADS1115_CONFIG_WORD (0xC383) to the CONFIG register (0x01)
    // using the following sequence: START, Slave+W, Pointer, MSB, LSB, STOP.
    
    uint16_t config_msb = (ADS1115_CONFIG_WORD >> 8); // 0xC3
    uint16_t config_lsb = (ADS1115_CONFIG_WORD & 0xFFU); // 0x83

    // --- Step 1: Send START + Slave Address + Write Bit, followed by the Pointer Register Address (0x01) ---
    
    // Write 1st word to TBUF: Pointer Register Address (0x01) + TDF Code (100b = START + Slave+W)
    // The IIC protocol pre-processor extracts the slave address from the TBUF write, based on ADS1115_ADDR.
    USIC_TBUF = (ADS1115_CONFIG_REG) | (TDF_START_WRITE << 8);

    // Wait for Transmit Buffer Indication Flag (TBIF) to confirm the start of transmission.
    I2C_WaitForFlag(1U << 13); // TBIF is bit 13 in USIC_PSR
    I2C_ClearFlag(1U << 13);
    
    // --- Step 2: Send Config Register MSB (0xC3) ---
    
    // Write 2nd word to TBUF: Config MSB (0xC3) + TDF Code (000b = Data)
    USIC_TBUF = (config_msb) | (TDF_DATA << 8);

    // Wait for TBIF
    I2C_WaitForFlag(1U << 13); 
    I2C_ClearFlag(1U << 13);

    // --- Step 3: Send Config Register LSB (0x83) + STOP ---
    
    // Write 3rd word to TBUF: Config LSB (0x83) + TDF Code (110b = NACK + STOP)
    // Since this is the last byte, we terminate the transaction with a STOP condition.
    USIC_TBUF = (config_lsb) | (TDF_NACK_STOP << 8); 

    // Wait for TBIF (last byte transfer started)
    I2C_WaitForFlag(1U << 13);
    I2C_ClearFlag(1U << 13);

    // Wait for Stop Condition Received (PCR) flag (bit 4 in USIC_PSR for IIC)
    I2C_WaitForFlag(1U << 4); 
    I2C_ClearFlag(1U << 4); 
}

// -----------------------------------------------------------------------------
// 3. Read ADS1115 Conversion Result
// -----------------------------------------------------------------------------

int16_t I2C_ReadConversion(void) {
    // This routine reads the 16-bit conversion result from the ADS1115 CONVERSION register (0x00).
    // Sequence: START, Slave+W, Pointer(0x00), Repeated START, Slave+R, Read MSB (ACK), Read LSB (NACK), STOP.
    
    uint16_t msb_byte;
    uint16_t lsb_byte;
    int16_t conversion_result;

    // --- Step 1: Write Pointer Register (0x00) (Send START, Slave+W, Pointer) ---
    
    // Write 1st word to TBUF: Pointer Register Address (0x00) + TDF Code (100b = START + Slave+W)
    USIC_TBUF = (ADS1115_CONV_REG) | (TDF_START_WRITE << 8);

    // Wait for TBIF
    I2C_WaitForFlag(1U << 13); 
    I2C_ClearFlag(1U << 13);

    // --- Step 2: Switch to Read Mode (Send Repeated START, Slave+R, followed by 1st Read Byte) ---
    
    // Write 2nd word to TBUF: Dummy data (0x00) + TDF Code (101b = Repeated START + Slave+R)
    USIC_TBUF = (0x00U) | (TDF_REPEATED_START_READ << 8); 

    // Wait for Alternative Receive Indication Flag (AIF) - Indicates first byte MSB is received.
    I2C_WaitForFlag(1U << 15); // AIF is bit 15 in USIC_PSR
    I2C_ClearFlag(1U << 15);
    
    // Read the MSB byte (RBUF holds the received data)
    msb_byte = (uint16_t)USIC_RBUF;

    // --- Step 3: Read 2nd Byte (LSB) and terminate with NACK + STOP ---
    
    // Write 3rd word to TBUF: Dummy data (0x00) + TDF Code (110b = NACK + STOP)
    // This controls the IIC state machine for the last byte read: NACK after LSB, then STOP.
    USIC_TBUF = (0x00U) | (TDF_NACK_STOP << 8); 
    
    // Wait for Receive Indication Flag (RIF) - Indicates second byte LSB is received.
    I2C_WaitForFlag(1U << 14); // RIF is bit 14 in USIC_PSR
    I2C_ClearFlag(1U << 14);
    
    // Read the LSB byte
    lsb_byte = (uint16_t)USIC_RBUF;

    // Wait for Stop Condition Received (PCR) flag
    I2C_WaitForFlag(1U << 4); 
    I2C_ClearFlag(1U << 4); 

    // Combine MSB and LSB into a signed 16-bit result
    conversion_result = (int16_t)((msb_byte << 8) | lsb_byte);
    return conversion_result;
}