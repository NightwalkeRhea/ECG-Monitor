module spi_byte_shifter #(
    parameter integer CLK_DIV = 4  // sysclk ticks per half SCK
) (
    input  wire clk,
    input  wire reset_n,
    input  wire start,
    input  wire [7:0] tx_byte,
    input  wire mosi_oe,           // drive MOSI when 1
    output reg  [7:0] rx_byte,
    output reg  busy,
    output reg  sck,
    output reg  mosi,
    input  wire miso,
    output reg  done
);
    reg [7:0] shreg;
    reg [3:0] bitcnt;
    reg [$clog2(CLK_DIV*2)-1:0] divcnt;

    always @(posedge clk) begin
        if (!reset_n) begin
            busy <= 0; done <= 0; sck <= 0; mosi <= 0; rx_byte <= 0;
            bitcnt <= 0; divcnt <= 0; shreg <= 0;
        end else begin
            done <= 1'b0;
            if (start && !busy) begin
                busy   <= 1'b1;
                shreg  <= tx_byte;
                bitcnt <= 4'd8;
                divcnt <= 0;
                sck    <= 1'b0;
                mosi   <= tx_byte[7];
            end else if (busy) begin
                divcnt <= divcnt + 1'b1;
                if (divcnt == CLK_DIV-1) begin
                    sck <= 1'b1; // rising edge: sample MISO
                end else if (divcnt == CLK_DIV*2-1) begin
                    // falling edge: shift out next bit
                    divcnt <= 0;
                    shreg <= {shreg[6:0], miso};
                    bitcnt <= bitcnt - 1'b1;
                    sck <= 1'b0;
                    mosi <= shreg[6];
                    if (bitcnt == 1) begin
                        busy <= 1'b0;
                        done <= 1'b1;
                        rx_byte <= {shreg[6:0], miso};
                        mosi <= 1'b0;
                    end
                end
            end
            if (!mosi_oe) mosi <= 1'b0; // tri-state externally with OBUFT if needed
        end
    end
endmodule
