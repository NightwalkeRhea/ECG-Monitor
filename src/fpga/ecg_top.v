// ecg_top.v
// Top-level integration for the Tang Nano 9K ECG processing path.

module ecg_top (
    // System interfaces
    input  wire CLK_27MHZ,
    input  wire RST_N,

    // On-board debug LEDs on Tang Nano 9K (active low, Bank 3)
    output wire DBG_LED0_N,
    output wire DBG_LED1_N,
    output wire DBG_LED2_N,
    output wire DBG_LED3_N,

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

    // Alarm/debug signaling.
    wire alarm_condition;
    reg [12:0] buzzer_divider;
    reg        buzzer_tone;
    reg [24:0] debug_divider;
    reg [23:0] raw_spi_activity_ctr;
    reg [23:0] sample_activity_ctr;
    reg spi_sclk_meta;
    reg spi_sclk_sync;
    reg spi_sclk_prev;
    reg spi_cs_meta;
    reg spi_cs_sync;

    localparam [23:0] SAMPLE_ACTIVITY_HOLD = 24'd13500000; // ~0.5 s at 27 MHz
    localparam [12:0] BUZZER_HALF_PERIOD = 13'd6750; // ~2 kHz tone from 27 MHz
    wire debug_blink_slow = debug_divider[23];
    wire debug_blink_fast = debug_divider[21];
    wire debug_blink_alive = debug_divider[24];
    wire spi_sclk_edge = (spi_sclk_sync ^ spi_sclk_prev);
    wire raw_spi_active = (raw_spi_activity_ctr != 24'd0);
    wire sample_stream_active = (sample_activity_ctr != 24'd0);

    always @(posedge clk_core) begin
        if (!rst_n_core) begin
            buzzer_divider      <= 13'd0;
            buzzer_tone         <= 1'b0;
            debug_divider       <= 25'd0;
            raw_spi_activity_ctr <= 24'd0;
            sample_activity_ctr <= 24'd0;
            spi_sclk_meta       <= 1'b0;
            spi_sclk_sync       <= 1'b0;
            spi_sclk_prev       <= 1'b0;
            spi_cs_meta         <= 1'b1;
            spi_cs_sync         <= 1'b1;
        end else begin
            debug_divider <= debug_divider + 25'd1;
            spi_sclk_meta <= SPI_SCLK_IN;
            spi_sclk_sync <= spi_sclk_meta;
            spi_sclk_prev <= spi_sclk_sync;
            spi_cs_meta   <= SPI_CS_N_IN;
            spi_cs_sync   <= spi_cs_meta;

            if (!alarm_condition) begin
                buzzer_divider <= 13'd0;
                buzzer_tone    <= 1'b0;
            end else if (buzzer_divider == (BUZZER_HALF_PERIOD - 13'd1)) begin
                buzzer_divider <= 13'd0;
                buzzer_tone    <= ~buzzer_tone;
            end else begin
                buzzer_divider <= buzzer_divider + 13'd1;
            end

            if (spi_sclk_edge || !spi_cs_sync) begin
                raw_spi_activity_ctr <= SAMPLE_ACTIVITY_HOLD;
            end else if (raw_spi_activity_ctr != 24'd0) begin
                raw_spi_activity_ctr <= raw_spi_activity_ctr - 24'd1;
            end

            if (sample_valid_pulse) begin
                sample_activity_ctr <= SAMPLE_ACTIVITY_HOLD;
            end else if (sample_activity_ctr != 24'd0) begin
                sample_activity_ctr <= sample_activity_ctr - 24'd1;
            end
        end
    end

    // Reuse ALARM_OUT as a passive-buzzer drive: 2 kHz tone gated by alarm_condition.
    assign ALARM_OUT = alarm_condition ? buzzer_tone : 1'b0;

    // Tang Nano 9K onboard LEDs are active low.
    assign DBG_LED0_N = ~debug_blink_alive;                        // FPGA configured/running
    assign DBG_LED1_N = ~(raw_spi_active & debug_blink_slow);      // raw SPI activity seen
    assign DBG_LED2_N = ~(sample_stream_active & debug_blink_fast);// full 16-bit samples accepted
    assign DBG_LED3_N = ~alarm_condition;                          // Alarm asserted

    // Alarm condition based on BPM thresholds.
    qrs_actuator actuator_inst (
        .clk_core    (clk_core),
        .rst_n       (rst_n_core),
        .current_bpm (current_bpm),
        .alarm_out   (alarm_condition)
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
