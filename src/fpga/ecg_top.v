// ecg_top.v
// Top module integrating CDC-safe SPI RX, DSP pipeline, Actuation, and Flash Logger.

module ecg_top (
    // System Interfaces (Mapped via.cst file)
    input  wire CLK_27MHZ,        // Pin PIN35 (27 MHz Oscillator)
    input  wire RST_N,            // Active low reset 

    // Interface to XMC1100 (SPI Slave)
    input  wire SPI_SCLK_IN,      // Pin IOL10A
    input  wire SPI_CS_N_IN,      // Pin IOL8B
    input  wire SPI_MOSI_IN,      // Pin IOL9B

    // Interface to Passive Buzzer/LED via dedicated MCU input pin
    output reg ALARM_OUT,        // Pin IOL16B (Alarm Output to XMC GPIO)

    // Interface to W25Q64 External Flash (SPI Master)
    output wire FLASH_SCLK_OUT,   // Pin IOL7B
    output wire FLASH_CS_N_OUT,   // Pin IOL4B
    output wire FLASH_MOSI_OUT,   // Pin IOL6B
    input  wire FLASH_MISO_IN     // Pin IOL5B
);

    // --- Internal Signals ---
    
    // 1. Data Path Signals (Driven by spi_slave_rx)
    wire signed [15:0] ecg_raw_sample_in; 
    wire               sample_valid_pulse;

    // 2. DSP Pipeline Outputs (Driven by dsp_pipeline_qrs)
    wire signed [15:0] filtered_data_out;
    wire               qrs_detected;
    wire [7:0]         current_bpm;       
    wire               log_trigger;       
    wire [15:0]        log_word;          // 16-bit word to push to buffer
    wire               log_word_valid;    // Valid pulse for log_word

    // 3. Flash Logger Control Signals
    wire [7:0]  buf_dout;          // Data byte out from Page Buffer
    wire [7:0]  buf_addr;          // Address requested by Logger
    wire        buf_full;          // Page buffer status (Ready to write to Flash)
    wire        logger_busy;       // Logger status (WREN/Program In Progress)

    // Core Clock Definition
    wire clk_core = CLK_27MHZ;
    
    // --- 1. SPI Slave: Receive Data from MCU (CDC-Safe) ---
    // Instantiates the robust module provided previously.
    spi_slave_rx spi_rx_inst (
   .clk_system     (clk_core),
   .rst_n          (RST_N),
   .spi_sclk       (SPI_SCLK_IN),
   .spi_cs_n       (SPI_CS_N_IN),
   .spi_mosi       (SPI_MOSI_IN),
   .ecg_data_out   (ecg_raw_sample_in),
   .data_valid     (sample_valid_pulse)
    );


    // --- 2. DSP Pipeline (Uses your FIR, generates logging data) ---
    // This is the core logic that computes QRS and BPM.
    dsp_pipeline_qrs dsp_pipeline_inst (
   .clk_core         (clk_core),
   .rst_n            (RST_N),
   .raw_in           (ecg_raw_sample_in),
   .valid_in         (sample_valid_pulse),
   .filtered_out     (filtered_data_out), // Output for potential DAC/Debug use
   .qrs_detected_out (qrs_detected),
   .current_bpm_out  (current_bpm),
   .log_trigger_out  (log_trigger),       // Generic trigger (e.g., QRS event or timer)
   .log_word_out     (log_word),          // Data word chosen for logging (e.g., filtered sample)
   .log_word_valid   (log_word_valid)     // Pulse when log_word is ready
    );


    // --- 3. Actuator Logic (BPM Threshold Check) ---
    // Reads results from the DSP pipeline and sets the ALARM_OUT pin.
    
    localparam BPM_LOW_THRESHOLD  = 8'd50;
    localparam BPM_HIGH_THRESHOLD = 8'd100;

    always @(posedge clk_core or negedge RST_N) begin
        if (!RST_N) begin
            ALARM_OUT <= 1'b0;
        end else begin
            if ((current_bpm > BPM_HIGH_THRESHOLD) || (current_bpm < BPM_LOW_THRESHOLD)) begin
                ALARM_OUT <= 1'b1; 
            end else begin
                ALARM_OUT <= 1'b0;
            end
        end
    end

    // --- 4. Log Buffering: Page Buffer Integration ---
    // Acts as the FIFO/buffer holding 128 samples until the full page is ready.
    page_buffer_256B buffer_inst (
       .clk         (clk_core),
       .reset_n     (RST_N),
       .push_word   (log_word_valid),      // Push data only when DSP says it's ready
       .word_in     (log_word),
       .buf_dout    (buf_dout),
       .buf_addr    (buf_addr),
       .buf_full    (buf_full),            // Output status: Page is ready
       .logger_busy (logger_busy)          // Input status: Is the logger currently writing?
    );

    // --- 5. Flash Logger: SPI Master Integration ---
    // Instantiates your actual low-level flash controller module.
    // Assuming w25q64_page_logger generates SCLK/MOSI/CS and requires buf_full to start.
    w25q64_page_logger #(
        // Add necessary parameters (e.g., CLK_DIV, START_ADDR) here
    ) flash_log_inst (
       .clk       (clk_core),
       .reset_n   (RST_N),
       .buf_dout  (buf_dout),           // Data source (from page buffer)
       .buf_addr  (buf_addr),           // Address request (back to page buffer)
       .buf_full  (buf_full),           // Start condition
       .buf_flush (log_trigger),        // You may use log_trigger to flush the buffer early if needed
        // Physical SPI Pins
       .spi_sck   (FLASH_SCLK_OUT),
       .spi_mosi  (FLASH_MOSI_OUT),
       .spi_miso  (FLASH_MISO_IN),
       .spi_csn   (FLASH_CS_N_OUT),
       .busy      (logger_busy)
    );

endmodule


// =================================================================
// DSP Pipeline and Logger Skeletons (To be replaced by your files)
// =================================================================

// Skeleton for dsp_pipeline_qrs.v (Maintained for single-driver compliance)
module dsp_pipeline_qrs (
    input wire clk_core,
    input wire rst_n,
    input wire signed [15:0] raw_in,
    input wire valid_in,
    output wire signed [15:0] filtered_out,
    output wire qrs_detected_out,
    output wire [7:0] current_bpm_out,
    output wire log_trigger_out,
    output wire [15:0] log_word_out,
    output wire log_word_valid
);
    // Dummy outputs (Must be replaced by actual signal processing logic)
    // Placeholder logic for debug: log every sample.
    assign filtered_out = raw_in;
    assign qrs_detected_out = 1'b0;
    assign current_bpm_out = 8'd75;
    assign log_trigger_out = 1'b0; // This signal would trigger logging on event
    
    // Log word is the raw input, valid when the sample arrives
    assign log_word_out = raw_in;
    assign log_word_valid = valid_in;
endmodule

// Skeleton for w25q64_page_logger.v (Your SPI Master Flash Controller)
module w25q64_page_logger (
    input wire clk,
    input wire reset_n,
    input wire [7:0] buf_dout,
    output wire [7:0] buf_addr,
    input wire buf_full,
    input wire buf_flush,
    output wire spi_sck,
    output wire spi_mosi,
    input wire spi_miso,
    output wire spi_csn,
    output wire busy
);
    // Placeholder outputs (Replace with state machine logic for WREN, PROGRAM, etc.)
    assign buf_addr = 8'd0;
    assign spi_sck = clk;
    assign spi_mosi = 1'b0;
    assign spi_csn = 1'b1;
    assign busy = 1'b0;
endmodule