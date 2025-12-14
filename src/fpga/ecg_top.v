// ecg_top.v
// Top module instantiating all components.

module ecg_top (
    // External Interfaces
    input  wire         clk_27mhz,       // Onboard 27 MHz oscillator
    input  wire         rst_n,           // Active low reset (e.g., from MCU/JTAG programmer)

    // Interface to XMC1100 (SPI Slave)
    input  wire         spi_sclk_in,     // XMC SCLK (Input)
    input  wire         spi_cs_n_in,     // XMC CS (Input)
    input  wire         spi_mosi_in,     // XMC MOSI (Input)

    // Interface to Passive Buzzer/LED via MCU GPIO (Actuation Output)
    output wire         alarm_signal_out,// Connects to dedicated MCU input pin

    // Interface to W25Q64 External Flash (SPI Master)
    output wire         flash_sclk_out,
    output wire         flash_cs_n_out,
    output wire         flash_mosi_out,
    input  wire         flash_miso_in
);

    // --- Internal Signals ---
    
    // 1. DSP Data Path Signals
    wire [15:0] ecg_data_in_q1_15;
    wire        ecg_data_valid;

    wire [15:0] filtered_data_q1_15;
    wire        qrs_detected;
    wire [7:0]  current_bpm;             // BPM output (8-bit sufficient for 0-255 BPM)

    // 2. Logging and Actuation Signals
    wire        log_trigger;             // DSP wants to save a block of data
    wire [23:0] log_data_block;          // Block of data/status to save

    // 3. Clock Generation
    // Clock divider for core DSP logic (optional, keep at 27MHz for max throughput)
    // Placeholder for slower Flash control clock if needed, but 27MHz is fine.
    // We assume clk_core = clk_27mhz for DSP.
    wire clk_core = clk_27mhz;


    // --- I. SPI Slave: Receive Data from MCU ---
    spi_slave_rx spi_rx_inst (
       .clk_system     (clk_core),
       .rst_n          (rst_n),
       .spi_sclk       (spi_sclk_in),
       .spi_cs_n       (spi_cs_n_in),
       .spi_mosi       (spi_mosi_in),
       .ecg_data_out   (ecg_data_in_q1_15),
       .data_valid     (ecg_data_valid)
    );


    // --- II. DSP Pipeline and QRS Detection (Fixed-Point Logic) ---
    // This is the largest and most complex section, implemented as a black box here.
    /* synthesis black_box */
    dsp_pipeline_qrs dsp_inst (
       .clk_core           (clk_core),
       .rst_n              (rst_n),
       .raw_ecg_in         (ecg_data_in_q1_15),
       .valid_in           (ecg_data_valid),
       .filtered_out       (filtered_data_q1_15),
       .qrs_detected_out   (qrs_detected),
       .current_bpm_out    (current_bpm),
       .log_data_out       (log_data_block), // Example: pack BPM + last filtered value
       .log_trigger_out    (log_trigger)     // Trigger to initiate flash write
    );


    // --- III. Actuator Logic ---
    // Simple logic: Trigger alarm if BPM is outside the acceptable range (e.g., 50-100 BPM)
    // The Actuator module simplifies triggering the pin based on DSP output.
    qrs_actuator actuator_inst (
       .clk_core           (clk_core),
       .rst_n              (rst_n),
       .current_bpm        (current_bpm),
       .alarm_out          (alarm_signal_out)
    );


    // --- IV. SPI Master: Control W25Q64 Flash ---
    // This module handles the slow, complex Flash command sequences (Write Enable, Page Program)
    /* synthesis black_box */
    w25q64_spi_master flash_master_inst (
       .clk_core           (clk_core),
       .rst_n              (rst_n),
       .start_write_i      (log_trigger), 
       .address_in         (24'h000000), // Placeholder: address management needed in actuator
       .byte_data_in       (log_data_block[7:0]), // Placeholder: data stream control needed
       .new_byte_valid     (log_trigger),
       .spi_sclk_out       (flash_sclk_out),
       .spi_mosi_out       (flash_mosi_out),
       .spi_cs_n_out       (flash_cs_n_out),
       .spi_miso_in        (flash_miso_in),
       .busy_flag_out      () // Status flags ignored at top level
    );

endmodule




