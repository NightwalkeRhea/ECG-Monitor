`timescale 1ns/1ns

// MCU interface role, having inputs SCLK, NOSI, CS_N, and outputs ecg_data_out, data_valid
/*
This module must reliably receive the $16$-bit data words sent by the XMC Master at $250 \,
Hz$ and safely transfer them to the DSP logic running on the internal $27 \, MHz$ system
clock using Clock Domain Crossing (CDC) logic.
*/
// Receives 16-bit ECG data from XMC Master (SPI Mode 0: CPOL=0, CPHA=0)
// Includes essential Clock Domain Crossing (CDC) synchronization.
module spi_slave_rx (
    // System Clock Domain (FPGA internal clock)
    input  wire         clk_system,      // e.g., 27 MHz
    input  wire         rst_n,           // Asynchronous Reset (Active Low)

    // SPI Input Interface (from XMC Master)
    input  wire         spi_sclk,        // Serial Clock (P1.3 from XMC)
    input  wire         spi_cs_n,        // Chip Select (P0.0 from XMC, Active Low)
    input  wire         spi_mosi,        // Master Out Slave In (P1.2 from XMC)

    // Output to DSP Pipeline (System Clock Domain)
    output reg  [15:0]  ecg_data_out,    // 16-bit fixed-point ECG sample (Q1.15)
    output reg          data_valid       // Pulse high for one clock cycle when a new sample is ready
);

    // --- 1. SPI Data Capture Logic (Synchronous to spi_sclk) ---
    
    // Internal Registers in the SPI Clock Domain
    reg  [15:0]  rx_shift_reg;     // Holds the incoming data bits
    reg  [4:0]   bit_counter;      // Counts from 0 to 15
    reg          sclk_d1;          // Synchronizer for sclk edge detection

    always @(posedge spi_sclk or negedge rst_n) begin
        if (!rst_n) begin
            bit_counter <= 5'd0;
            rx_shift_reg <= 16'd0;
        end else if (spi_cs_n == 1'b1) begin
            // Reset counter and shift register when CS is inactive (idle state)
            bit_counter <= 5'd0;
        end else begin
            // Data is clocked in on the rising edge of SCLK (Mode 0)
            if (bit_counter < 5'd16) begin
                // MSB first transmission assumed based on XMC typical config (SDIR=1)
                rx_shift_reg <= {rx_shift_reg[14:0], spi_mosi};
                bit_counter <= bit_counter + 5'd1;
            end
        end
    end

    // --- 2. Clock Domain Crossing (CDC) Synchronization ---

    // The data is parallelized at the end of the 16th clock cycle, 
    // when bit_counter transitions to 16.

    reg  [15:0]  spi_domain_data;      // Data register in SPI clock domain
    reg          spi_data_ready_pulse; // Pulse flag in SPI clock domain
    
    wire data_capture_end = (bit_counter == 5'd15) && (spi_cs_n == 1'b0);

    always @(posedge spi_sclk or negedge rst_n) begin
        if (!rst_n) begin
            spi_domain_data <= 16'd0;
            spi_data_ready_pulse <= 1'b0;
        end else begin
            // Latch final data and set pulse flag when all 16 bits are received
            if (data_capture_end) begin
                spi_domain_data <= rx_shift_reg; // Data is ready on the next clock
                spi_data_ready_pulse <= 1'b1;
            end else begin
                spi_data_ready_pulse <= 1'b0; // Clear pulse immediately
            end
        end
    end

    // Standard two-flop synchronizer for the 'data_ready' pulse
    // This safely transfers the 'data_ready' pulse to the clk_system domain.
    reg         sync_flop1, sync_flop2;
    
    always @(posedge clk_system or negedge rst_n) begin
        if (!rst_n) begin
            sync_flop1 <= 1'b0;
            sync_flop2 <= 1'b0;
            data_valid <= 1'b0;
            ecg_data_out <= 16'd0;
        end else begin
            sync_flop1 <= spi_data_ready_pulse;
            sync_flop2 <= sync_flop1;

            // data_valid pulses when the synchronized signal transitions from 0 to 1
            if (sync_flop2 &&!sync_flop1) begin 
                data_valid <= 1'b1;
                // Latch the data output when the pulse is valid
                ecg_data_out <= spi_domain_data;
            end else begin
                data_valid <= 1'b0;
            end
        end
    end

endmodule
