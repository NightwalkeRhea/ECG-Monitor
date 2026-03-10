module w25q64_page_logger #(
    parameter integer CLK_DIV    = 4,
    parameter [23:0]  START_ADDR = 24'h100000
) (
    input  wire clk,
    input  wire reset_n,
    // Page buffer interface
    input  wire [7:0] buf_dout,
    output reg  [7:0] buf_addr,
    input  wire       buf_full,
    input  wire       buf_flush,
    // SPI pins
    output wire spi_sck,
    output wire spi_mosi,
    input  wire spi_miso,
    output reg  spi_csn,
    // Status
    output reg  busy
);
    localparam [7:0] CMD_WREN = 8'h06;
    localparam [7:0] CMD_PP   = 8'h02;
    localparam [7:0] CMD_RDSR = 8'h05;

    localparam [3:0] S_IDLE           = 4'd0;
    localparam [3:0] S_WREN_WAIT      = 4'd1;
    localparam [3:0] S_PP_HDR_START   = 4'd2;
    localparam [3:0] S_PP_HDR_WAIT    = 4'd3;
    localparam [3:0] S_PP_DATA_START  = 4'd4;
    localparam [3:0] S_PP_DATA_WAIT   = 4'd5;
    localparam [3:0] S_RDSR_CMD_START = 4'd6;
    localparam [3:0] S_RDSR_CMD_WAIT  = 4'd7;
    localparam [3:0] S_RDSR_DATA_START= 4'd8;
    localparam [3:0] S_RDSR_DATA_WAIT = 4'd9;

    wire start_write = buf_full; // Partial-page flush is intentionally disabled for now.

    reg  [3:0]  state;
    reg  [1:0]  hdr_idx;
    reg  [7:0]  byte_cnt;
    reg  [23:0] addr;
    reg         sh_start;
    reg  [7:0]  sh_tx;
    wire [7:0]  sh_rx;
    wire        sh_busy;
    wire        sh_done;

    // Keep the port in the interface so the module shape stays stable until
    // partial-page support is implemented.
    wire unused_buf_flush = buf_flush;

    spi_byte_shifter #(.CLK_DIV(CLK_DIV)) sh (
        .clk     (clk),
        .reset_n (reset_n),
        .start   (sh_start),
        .tx_byte (sh_tx),
        .mosi_oe (1'b1),
        .rx_byte (sh_rx),
        .busy    (sh_busy),
        .sck     (spi_sck),
        .mosi    (spi_mosi),
        .miso    (spi_miso),
        .done    (sh_done)
    );

    always @(posedge clk) begin
        if (!reset_n) begin
            state    <= S_IDLE;
            hdr_idx  <= 2'd0;
            byte_cnt <= 8'd0;
            addr     <= START_ADDR;
            buf_addr <= 8'd0;
            sh_start <= 1'b0;
            sh_tx    <= 8'd0;
            spi_csn  <= 1'b1;
            busy     <= 1'b0;
        end else begin
            sh_start <= 1'b0;

            case (state)
                S_IDLE: begin
                    busy    <= 1'b0;
                    spi_csn <= 1'b1;
                    if (start_write) begin
                        busy     <= 1'b1;
                        spi_csn  <= 1'b0;
                        sh_tx    <= CMD_WREN;
                        sh_start <= 1'b1;
                        state    <= S_WREN_WAIT;
                    end
                end

                S_WREN_WAIT: begin
                    if (sh_done) begin
                        spi_csn  <= 1'b1;
                        hdr_idx  <= 2'd0;
                        byte_cnt <= 8'd0;
                        state    <= S_PP_HDR_START;
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
                        state    <= S_PP_HDR_WAIT;
                    end
                end

                S_PP_HDR_WAIT: begin
                    if (sh_done) begin
                        if (hdr_idx == 2'd3) begin
                            buf_addr <= 8'd0;
                            state    <= S_PP_DATA_START;
                        end else begin
                            hdr_idx <= hdr_idx + 2'd1;
                            state   <= S_PP_HDR_START;
                        end
                    end
                end

                S_PP_DATA_START: begin
                    if (!sh_busy) begin
                        sh_tx    <= buf_dout;
                        sh_start <= 1'b1;
                        state    <= S_PP_DATA_WAIT;
                    end
                end

                S_PP_DATA_WAIT: begin
                    if (sh_done) begin
                        if (byte_cnt == 8'hFF) begin
                            spi_csn <= 1'b1; // Finish the page program command before polling.
                            state   <= S_RDSR_CMD_START;
                        end else begin
                            byte_cnt <= byte_cnt + 8'd1;
                            buf_addr <= buf_addr + 8'd1;
                            state    <= S_PP_DATA_START;
                        end
                    end
                end

                S_RDSR_CMD_START: begin
                    spi_csn <= 1'b0;
                    if (!sh_busy) begin
                        sh_tx    <= CMD_RDSR;
                        sh_start <= 1'b1;
                        state    <= S_RDSR_CMD_WAIT;
                    end
                end

                S_RDSR_CMD_WAIT: begin
                    if (sh_done) begin
                        state <= S_RDSR_DATA_START;
                    end
                end

                S_RDSR_DATA_START: begin
                    if (!sh_busy) begin
                        sh_tx    <= 8'hFF;
                        sh_start <= 1'b1;
                        state    <= S_RDSR_DATA_WAIT;
                    end
                end

                S_RDSR_DATA_WAIT: begin
                    if (sh_done) begin
                        spi_csn <= 1'b1;
                        if (sh_rx[0] == 1'b0) begin
                            addr  <= addr + 24'd256;
                            busy  <= 1'b0;
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
