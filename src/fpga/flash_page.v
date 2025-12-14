module w25q64_page_logger #(
    parameter integer CLK_DIV    = 4,
    parameter [23:0]  START_ADDR = 24'h0010_0000  // keep clear of bitstream
) (
    input  wire clk,
    input  wire reset_n,
    // page buffer interface
    input  wire [7:0] buf_dout,
    output reg  [7:0] buf_addr,
    input  wire       buf_full,    // asserted when 256 bytes ready
    input  wire       buf_flush,   // force flush early
    // SPI pins
    output wire spi_sck,
    output wire spi_mosi,
    input  wire spi_miso,
    output reg  spi_csn,
    // status
    output reg  busy
);
    localparam CMD_WREN = 8'h06;
    localparam CMD_PP   = 8'h02;
    localparam CMD_RDSR = 8'h05;

    reg [2:0] state;
    localparam S_IDLE=0;
    localparam S_WREN=1;
    localparam S_PP_CMD=2;
    localparam S_PP_DATA=3;
    localparam S_POLL=4;

    reg sh_start;
    reg [7:0] sh_tx;
    wire [7:0] sh_rx;
    wire sh_busy, sh_done;

    spi_byte_shifter #(.CLK_DIV(CLK_DIV)) sh (
        .clk(clk), .reset_n(reset_n),
        .start(sh_start),
        .tx_byte(sh_tx),
        .mosi_oe(1'b1),
        .rx_byte(sh_rx), 
        .busy(sh_busy), 
        .sck(spi_sck),
        .mosi(spi_mosi), 
        .miso(spi_miso), 
        .done(sh_done)
    );

    reg [23:0] addr;
    reg [9:0]  byte_cnt; // 0..255

    always @(posedge clk or negedge reset_n) begin
        if (!reset_n) begin
            state <= S_IDLE; 
            spi_csn <= 1'b1; 
            busy <= 1'b0;
            sh_start <= 1'b0; 
            sh_tx <= 8'd0; 
            addr <= START_ADDR; 
            buf_addr <= 0; 
            byte_cnt <= 0;
        end else begin
            sh_start <= 1'b0;
            case (state)
                S_IDLE: begin
                    busy <= 1'b0;
                    if (buf_full || buf_flush) begin
                        busy <= 1'b1;
                        spi_csn <= 1'b0;
                        sh_tx <= CMD_WREN; 
                        sh_start <= 1'b1;
                        state <= S_WREN;
                    end
                end
                S_WREN: if (sh_done) begin
                    spi_csn <= 1'b1; // end WREN
                    state <= S_PP_CMD;
                end
                S_PP_CMD: begin
                    spi_csn <= 1'b0;
                    // send PP + 3-byte addr
                    if (!sh_busy && !sh_start) begin
                        case (byte_cnt)
                            0: begin sh_tx <= CMD_PP;    
                                sh_start <= 1'b1; 
                                byte_cnt <= 1; 
                                end
                            1: begin sh_tx <= addr[23:16]; 
                                sh_start <= 1'b1; 
                                byte_cnt <= 2; 
                                end
                            2: begin sh_tx <= addr[15:8];  
                                sh_start <= 1'b1; 
                                byte_cnt <= 3; 
                                end
                            3: begin sh_tx <= addr[7:0];   
                                sh_start <= 1'b1; 
                                byte_cnt <= 4; 
                                buf_addr <= 0; 
                                end
                            default: state <= S_PP_DATA;
                        endcase
                    end
                end
                S_PP_DATA: begin
                    if (!sh_busy && !sh_start) begin
                        sh_tx <= buf_dout; 
                        sh_start <= 1'b1;
                        buf_addr <= buf_addr + 1'b1;
                        byte_cnt <= byte_cnt + 1'b1;
                        if (byte_cnt == 4 + 255) begin
                            state <= S_POLL;
                        end
                    end
                end
                S_POLL: begin
                    if (!sh_busy && !sh_start) begin
                        // send RDSR and read one byte repeatedly until WIP=0
                        sh_tx <= CMD_RDSR; 
                        sh_start <= 1'b1;
                        state <= S_POLL + 1;
                    end
                end
                S_POLL+1: begin
                    if (sh_done) begin
                        sh_start <= 1'b1;
                        sh_tx <= 8'hFF; // dummy to read status
                        state <= S_POLL+2;
                    end
                end
                S_POLL+2: begin
                    if (sh_done) begin
                        if (sh_rx[0] == 1'b0) begin
                            spi_csn <= 1'b1;
                            addr <= addr + 24'd256;
                            byte_cnt <= 0;
                            state <= S_IDLE;
                        end else begin
                            // still busy, keep CS low and poll again
                            sh_start <= 1'b1; 
                            sh_tx <= CMD_RDSR; 
                            state <= S_POLL+1;
                        end
                    end
                end
                default: state <= S_IDLE;
            endcase
        end
    end
endmodule
