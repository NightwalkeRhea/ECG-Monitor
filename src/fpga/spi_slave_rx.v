
/**
 * SPI slave receiver for 16-bit words (Mode 0: CPOL=0, CPHA=0), MSB-first.
 *
 * CLOCK DOMAIN CROSSING (CDC) STRATEGY:
 * SPI clock (spi_sclk) is much slower (~1-5 MHz for first-bring-up)
 * System clock (clk_system) is 27 MHz FPGA core clock
 * Data transfer uses asynchronous toggle handshake (single-bit CDC-safe)
 * req_toggle_spi toggles when a complete 16-bit word is received
 * req_sync1/req_sync2 create 2-stage synchronizer chain (reduces metastability risk)
 *
 * DESIGN RATIONALE:
 * Single holding register (data_latch_spi) is sufficient since ECG samples arrive at 250 Hz
 * No FIFO needed: new sample won't arrive until MCU's next SPI transaction (~4ms away)
 * Toggle synchronizer is safer than level-based CDC for slow sample rates
 */

module spi_slave_rx (
    input wire clk_system,   // FPGA core clock (27 MHz), always-on domain
    input wire rst_n,   // Asynchronous active-low reset

    // SPI interface (from MCU master)
    input wire spi_sclk, // SPI clock (domain crossing)
    input wire spi_cs_n, // Chip select (active low)
    input wire spi_mosi, // Master Out Slave In (data from MCU)

    // Output signals (synchronized to clk_system)
    output reg [15:0] ecg_data_out,       // Captured 16-bit ECG sample
    output reg data_valid          // Pulse strobe: high for 1 clk_system period
);
    // Local reset synchronizer for the always-running system clock domain.
    reg [1:0] rst_sys_sync;
    wire rst_n_sys = rst_sys_sync[1];


    // SPI clock domain registers
    reg [15:0] rx_shift;
    reg [3:0] bit_cnt;

    reg [15:0] data_latch_spi;   // stable until the next word completes
    reg req_toggle_spi;   // toggles when a word is ready

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
            rx_shift <= 16'd0;
            bit_cnt <= 4'd0;
            data_latch_spi <= 16'd0;
            req_toggle_spi <= 1'b0;
        end else begin
            if (spi_cs_n) begin //start of a new transaction
                rx_shift <= 16'd0;
                bit_cnt <= 4'd0;
            end else begin
                // shift in bits MSB-first
                rx_shift <= {rx_shift[14:0], spi_mosi};

                if (bit_cnt == 4'd15) begin //once the 16th bit arrives 
                    data_latch_spi <= {rx_shift[14:0], spi_mosi};
                    req_toggle_spi <= ~req_toggle_spi;  //toggle req to signal a new word being latched
                    bit_cnt <= 4'd0;
                end else begin
                    bit_cnt <= bit_cnt + 4'd1;
                end
            end
        end
    end

    // System clock domain (CDC)
    reg req_sync1_sys, req_sync2_sys;
    reg req_prev_sys;

    // Synchronize req toggle into system domain
    // standard two-flop synchronizer: the req_toggle_spi passed through synch1 and synch2, both are clocked by clk_system
    always @(posedge clk_system) begin
        if (!rst_n_sys) begin
            req_sync1_sys <= 1'b0;
            req_sync2_sys <= 1'b0;
        end else begin
            req_sync1_sys <= req_toggle_spi;
            req_sync2_sys <= req_sync1_sys;
        end
    end

    // Capture data on req toggle change, then toggle ack
    always @(posedge clk_system) begin
        if (!rst_n_sys) begin
            ecg_data_out <= 16'd0;
            data_valid <= 1'b0;
            req_prev_sys <= 1'b0;
        end else begin
            data_valid <= 1'b0;

            if (req_sync2_sys != req_prev_sys) begin
                req_prev_sys <= req_sync2_sys;
                // Safe because SPI side holds data_latch_spi stable until the next
                // complete 16-bit word arrives, which is much slower than clk_system.
                ecg_data_out <= data_latch_spi;
                data_valid <= 1'b1;
            end
        end
    end
endmodule
