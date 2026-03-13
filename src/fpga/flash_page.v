module w25q64_page_logger #(
    parameter integer CLK_DIV = 4,               // SPI clock divider (CLK_DIV periods per half-SCK)
    parameter [23:0] START_ADDR = 24'h100000      // Starting flash address (1 MB offset for safety)
) (
    input wire clk,
    input wire reset_n,
    // Page buffer interface
    input wire [7:0] buf_dout,    // Byte data from page buffer
    output reg [7:0] buf_addr,    // Read address for page buffer
    input wire buf_full,  // Page buffer full: start transaction
    input wire buf_flush, // Partial flush (currently unused)

    // SPI master pins to W25Q64FV flash
    output wire spi_sck,     // SPI clock output
    output wire spi_mosi,    // SPI MOSI (Master Out Slave In)
    input wire spi_miso,    // SPI MISO (Master In Slave Out)
    output reg  spi_csn,     // Chip select (active low)

    // Status
    output reg busy   // High during WREN/PP/RDSR sequence
);

    /**
     * W25Q64FV SPI Flash Protocol: Page Program Sequence
     *
     * SEQUENCE: WREN -> PP (Program Page) -> RDSR (Read Status) poll
     * Each command is atomic (CS stays low within command, high between commands)
     *
     * STATE MACHINE:
     * S_IDLE           -> S_WREN_WAIT         : Send WREN (0x06) to enable writes
     * S_WREN_WAIT      -> S_PP_HDR_START      : Wait for WREN completion
     * S_PP_HDR_START   -> S_PP_HDR_WAIT       : Send PP (0x02) + 24-bit address (3 bytes, MSB first)
     * S_PP_HDR_WAIT    -> S_PP_DATA_START     : Wait for header completion
     * S_PP_DATA_START  -> S_PP_DATA_WAIT      : Send 256 data bytes (one byte per cycle)
     * S_PP_DATA_WAIT   -> S_RDSR_CMD_START    : Wait until all 256 bytes sent, then release CS
     * S_RDSR_CMD_START -> S_RDSR_CMD_WAIT     : Send RDSR (0x05) to read status
     * S_RDSR_CMD_WAIT  -> S_RDSR_DATA_START   : Wait for RDSR command
     * S_RDSR_DATA_START-> S_RDSR_DATA_WAIT    : Send dummy byte and capture status
     * S_RDSR_DATA_WAIT -> S_IDLE (if done)    : Check WIP bit (bit 0 of status): if 0 = write done
     *                  or S_RDSR_CMD_START      : If WIP=1, poll again
     *
     * TIMING:
     * Each byte is shifted at reduced clock speed (CLK_DIV divider)
     * Payload phase = 256 bytes x 8 bits = 2048 bits
     * With CLK_DIV = 4, SCK = 27 MHz / (2*4) = 3.375 MHz
     * Payload shift time approx 2048 / 3.375 MHz = 607 microseconds
     * W25Q64FV typical write time: 0.3-0.75 ms
     * Polling ensures write is complete before next page
     *
     * OPTIMIZATION (if needed):
     * Page size is full 256 bytes (no partial writes to save flash wear)
     * Address auto-increments by 256 bytes per transaction
     * No read-modify-write: direct page overwrites
     */

    // SPI flash command bytes
    localparam [7:0] CMD_WREN = 8'h06;   // Write Enable command
    localparam [7:0] CMD_PP = 8'h02;    // Page Program command (256 bytes)
    localparam [7:0] CMD_RDSR = 8'h05;  // Read Status Register command

    // State machine definitions
    localparam [3:0] S_IDLE  = 4'd0;   // Waiting for buf_full
    localparam [3:0] S_WREN_WAI = 4'd1;   // Send WREN command
    localparam [3:0] S_PP_HDR_START = 4'd2;   // Send PP command + 24-bit address
    localparam [3:0] S_PP_HDR_WAIT  = 4'd3;   // Wait for address bytes
    localparam [3:0] S_PP_DATA_START = 4'd4;   // Start data phase (256 bytes)
    localparam [3:0] S_PP_DATA_WAIT = 4'd5;   // Wait for data byte
    localparam [3:0] S_RDSR_CMD_START = 4'd6;   // Send RDSR command (status polling)
    localparam [3:0] S_RDSR_CMD_WAIT = 4'd7;   // Wait for RDSR command
    localparam [3:0] S_RDSR_DATA_START= 4'd8;   // Capture status byte
    localparam [3:0] S_RDSR_DATA_WAIT = 4'd9;   // Wait for status capture

    wire start_write = buf_full; // Partial-page flush is intentionally disabled for now.
    reg [3:0] state;
    reg [1:0] hdr_idx;
    reg [7:0] byte_cnt;
    reg [23:0] addr;
    reg sh_start;
    reg [7:0] sh_tx;
    wire [7:0] sh_rx;
    wire sh_busy;
    wire sh_done;

    // Keep the port in the interface so the module shape stays stable until
    // partial-page support is implemented.
    wire unused_buf_flush = buf_flush;

    spi_byte_shifter #(.CLK_DIV(CLK_DIV)) sh (
        .clk (clk),
        .reset_n (reset_n),
        .start (sh_start),
        .tx_byte (sh_tx),
        .mosi_oe (1'b1),
        .rx_byte (sh_rx),
        .busy  (sh_busy),
        .sck  (spi_sck),
        .mosi  (spi_mosi),
        .miso  (spi_miso),
        .done  (sh_done)
    );

    always @(posedge clk) begin
        if (!reset_n) begin
            state <= S_IDLE;
            hdr_idx  <= 2'd0;
            byte_cnt <= 8'd0;
            addr <= START_ADDR;
            buf_addr <= 8'd0;
            sh_start <= 1'b0;
            sh_tx <= 8'd0;
            spi_csn <= 1'b1;
            busy <= 1'b0;
        end else begin
            sh_start <= 1'b0;
            case (state)
                S_IDLE: begin
                    busy <= 1'b0;
                    spi_csn <= 1'b1;
                    if (start_write) begin
                        busy <= 1'b1;
                        spi_csn  <= 1'b0;
                        sh_tx  <= CMD_WREN;
                        sh_start <= 1'b1;
                        state <= S_WREN_WAIT;
                    end
                end

                S_WREN_WAIT: begin
                    if (sh_done) begin
                        spi_csn <= 1'b1;
                        hdr_idx  <= 2'd0;
                        byte_cnt <= 8'd0;
                        state <= S_PP_HDR_START;
                    end
                end

                S_PP_HDR_START: begin
                    spi_csn <= 1'b0;
                    if (!sh_busy) begin
                        case (hdr_idx)
                            2'd0: sh_tx <= CMD_PP;
                            2'd1: sh_tx <= addr[23:16];
                            2'd2: sh_tx <= addr[15:8];
                            default: sh_tx <= addr[7:0];
                        endcase
                        sh_start <= 1'b1;
                        state <= S_PP_HDR_WAIT;
                    end
                end

                S_PP_HDR_WAIT: begin
                    if (sh_done) begin
                        if (hdr_idx == 2'd3) begin
                            buf_addr <= 8'd0;
                            state <= S_PP_DATA_START;
                        end else begin
                            hdr_idx <= hdr_idx + 2'd1;
                            state <= S_PP_HDR_START;
                        end
                    end
                end

                S_PP_DATA_START: begin
                    if (!sh_busy) begin
                        sh_tx <= buf_dout;
                        sh_start <= 1'b1;
                        state <= S_PP_DATA_WAIT;
                    end
                end

                S_PP_DATA_WAIT: begin
                    if (sh_done) begin
                        if (byte_cnt == 8'hFF) begin
                            spi_csn <= 1'b1; // Finish the page program command before polling.
                            state <= S_RDSR_CMD_START;
                        end else begin
                            byte_cnt <= byte_cnt + 8'd1;
                            buf_addr <= buf_addr + 8'd1;
                            state <= S_PP_DATA_START;
                        end
                    end
                end

                S_RDSR_CMD_START: begin
                    spi_csn <= 1'b0;
                    if (!sh_busy) begin
                        sh_tx <= CMD_RDSR;
                        sh_start <= 1'b1;
                        state <= S_RDSR_CMD_WAIT;
                    end
                end

                S_RDSR_CMD_WAIT: begin
                    if (sh_done) begin
                        state <= S_RDSR_DATA_START;
                    end
                end

                S_RDSR_DATA_START: begin
                    if (!sh_busy) begin
                        sh_tx <= 8'hFF;
                        sh_start <= 1'b1;
                        state <= S_RDSR_DATA_WAIT;
                    end
                end

                S_RDSR_DATA_WAIT: begin
                    if (sh_done) begin
                        spi_csn <= 1'b1;
                        if (sh_rx[0] == 1'b0) begin
                            addr <= addr + 24'd256;
                            busy <= 1'b0;
                            state <= S_IDLE;
                        end else begin
                            state <= S_RDSR_CMD_START;
                        end
                    end
                end

                default: begin
                    state <= S_IDLE;
                end
            endcase
        end
    end
endmodule
