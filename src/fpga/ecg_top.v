// ecg_top.v
// Top-level integration for the Tang Nano 9K ECG processing path.

module ecg_top (
    // System interfaces
    input  wire CLK_27MHZ,
    input  wire RST_N,

    // Interface to XMC1100 (SPI slave)
    input  wire SPI_SCLK_IN,
    input  wire SPI_CS_N_IN,
    input  wire SPI_MOSI_IN,

    // Alarm output to LED/buzzer or MCU input
    output wire ALARM_OUT,

    // Interface to external W25Q64 flash (SPI master)
    output wire FLASH_SCLK_OUT,
    output wire FLASH_CS_N_OUT,
    output wire FLASH_MOSI_OUT,
    input  wire FLASH_MISO_IN
);

    // Core clock definition
    wire clk_core = CLK_27MHZ;

    // Synchronize reset deassertion; assertion remains asynchronous from the button.
    reg [1:0] rst_release_sync;
    always @(posedge clk_core or negedge RST_N) begin
        if (!RST_N) begin
            rst_release_sync <= 2'b00;
        end else begin
            rst_release_sync <= {rst_release_sync[0], 1'b1};
        end
    end

    wire rst_n_core = rst_release_sync[1];

    // 1. Data path signals (driven by spi_slave_rx)
    wire signed [15:0] ecg_raw_sample_in;
    wire               sample_valid_pulse;

    // 2. DSP pipeline outputs
    wire signed [15:0] filtered_data_out;
    wire               qrs_detected;
    wire [7:0]         current_bpm;
    wire               log_trigger;
    wire [15:0]        log_word;
    wire               log_word_valid;

    // 3. Flash logger control signals
    wire [7:0] buf_dout;
    wire [7:0] buf_addr;
    wire       buf_full;
    wire       logger_busy;

    // SPI slave: receive 16-bit ECG samples from the MCU.
    spi_slave_rx spi_rx_inst (
        .clk_system   (clk_core),
        .rst_n        (RST_N),
        .spi_sclk     (SPI_SCLK_IN),
        .spi_cs_n     (SPI_CS_N_IN),
        .spi_mosi     (SPI_MOSI_IN),
        .ecg_data_out (ecg_raw_sample_in),
        .data_valid   (sample_valid_pulse)
    );

    // DSP pipeline: filter, detect QRS, produce log words.
    dsp_pipeline_qrs dsp_pipeline_inst (
        .clk_core         (clk_core),
        .rst_n            (rst_n_core),
        .raw_in           (ecg_raw_sample_in),
        .valid_in         (sample_valid_pulse),
        .filtered_out     (filtered_data_out),
        .qrs_detected_out (qrs_detected),
        .current_bpm_out  (current_bpm),
        .log_trigger_out  (log_trigger),
        .log_word_out     (log_word),
        .log_word_valid   (log_word_valid)
    );

    // Alarm output based on BPM thresholds.
    qrs_actuator actuator_inst (
        .clk_core    (clk_core),
        .rst_n       (rst_n_core),
        .current_bpm (current_bpm),
        .alarm_out   (ALARM_OUT)
    );

    // Page buffer: stores 128 16-bit samples per flash page.
    page_buffer_256B buffer_inst (
        .clk         (clk_core),
        .reset_n     (rst_n_core),
        .push_word   (log_word_valid),
        .word_in     (log_word),
        .buf_dout    (buf_dout),
        .buf_addr    (buf_addr),
        .buf_full    (buf_full),
        .logger_busy (logger_busy)
    );

    // Flash logger: writes only full pages.
    // Partial flush is intentionally disabled until the logger tracks valid byte count.
    w25q64_page_logger flash_log_inst (
        .clk       (clk_core),
        .reset_n   (rst_n_core),
        .buf_dout  (buf_dout),
        .buf_addr  (buf_addr),
        .buf_full  (buf_full),
        .buf_flush (1'b0),
        .spi_sck   (FLASH_SCLK_OUT),
        .spi_mosi  (FLASH_MOSI_OUT),
        .spi_miso  (FLASH_MISO_IN),
        .spi_csn   (FLASH_CS_N_OUT),
        .busy      (logger_busy)
    );

endmodule
