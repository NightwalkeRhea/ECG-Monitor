// spi_slave_rx.v
// SPI slave receiver for 16-bit words (Mode 0: CPOL=0, CPHA=0), MSB-first.
// CDC from spi_sclk domain -> clk_system domain uses a request toggle only.
// The sample source is much slower than clk_system, so a single-word holding
// register is sufficient and avoids requiring extra SPI edges between frames.

module spi_slave_rx (
    input  wire        clk_system,
    input  wire        rst_n,

    // SPI (from MCU)
    input  wire        spi_sclk,
    input  wire        spi_cs_n,
    input  wire        spi_mosi,

    // Output (clk_system domain)
    output reg  [15:0] ecg_data_out,
    output reg         data_valid
);

    // Local reset synchronizer for the always-running system clock domain.
    reg [1:0] rst_sys_sync;

    wire rst_n_sys = rst_sys_sync[1];

    // -----------------------------
    // SPI clock domain registers
    // -----------------------------
    reg [15:0] rx_shift;
    reg [3:0]  bit_cnt;

    reg [15:0] data_latch_spi;   // stable until the next word completes
    reg        req_toggle_spi;   // toggles when a word is ready

    // Reset deassertion synchronized into system domain.
    always @(posedge clk_system or negedge rst_n) begin
        if (!rst_n) begin
            rst_sys_sync <= 2'b00;
        end else begin
            rst_sys_sync <= {rst_sys_sync[0], 1'b1};
        end
    end

    // Main SPI capture (Mode 0: sample MOSI on rising edge).
    always @(posedge spi_sclk or negedge rst_n) begin
        if (!rst_n) begin
            rx_shift       <= 16'd0;
            bit_cnt        <= 4'd0;
            data_latch_spi <= 16'd0;
            req_toggle_spi <= 1'b0;
        end else begin
            if (spi_cs_n) begin
                bit_cnt <= 4'd0;
            end else begin
                // shift in bits MSB-first
                rx_shift <= {rx_shift[14:0], spi_mosi};

                if (bit_cnt == 4'd15) begin
                    data_latch_spi <= {rx_shift[14:0], spi_mosi};
                    req_toggle_spi <= ~req_toggle_spi;
                    bit_cnt        <= 4'd0;
                end else begin
                    bit_cnt <= bit_cnt + 4'd1;
                end
            end
        end
    end

    // -----------------------------
    // System clock domain (CDC)
    // -----------------------------
    reg req_sync1_sys, req_sync2_sys;
    reg req_prev_sys;

    // Synchronize req toggle into system domain.
    always @(posedge clk_system) begin
        if (!rst_n_sys) begin
            req_sync1_sys <= 1'b0;
            req_sync2_sys <= 1'b0;
        end else begin
            req_sync1_sys <= req_toggle_spi;
            req_sync2_sys <= req_sync1_sys;
        end
    end

    // Capture data on req toggle change, then toggle ack.
    always @(posedge clk_system) begin
        if (!rst_n_sys) begin
            ecg_data_out       <= 16'd0;
            data_valid         <= 1'b0;
            req_prev_sys       <= 1'b0;
        end else begin
            data_valid <= 1'b0;

            if (req_sync2_sys != req_prev_sys) begin
                req_prev_sys <= req_sync2_sys;

                // Safe because SPI side holds data_latch_spi stable until the next
                // complete 16-bit word arrives, which is much slower than clk_system.
                ecg_data_out <= data_latch_spi;
                data_valid   <= 1'b1;
            end
        end
    end

endmodule
