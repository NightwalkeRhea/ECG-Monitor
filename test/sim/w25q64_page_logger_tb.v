// purpose is to verify the command sequence, and to check cs/sclk/mosi toggling
`timescale 1ns/1ps

module w25q64_page_logger_tb;

    reg clk = 0;
    reg reset_n = 0;

    reg [7:0] buf_mem [0:255];
    reg [7:0] buf_addr;
    wire [7:0] buf_dout = buf_mem[buf_addr];

    reg buf_full = 0;
    wire spi_sck, spi_mosi;
    reg spi_miso = 1'b1;
    wire spi_csn;
    wire busy;

    always #10 clk = ~clk;

    w25q64_page_logger dut (
        .clk(clk),
        .reset_n(reset_n),
        .buf_dout(buf_dout),
        .buf_addr(buf_addr),
        .buf_full(buf_full),
        .buf_flush(1'b0),
        .spi_sck(spi_sck),
        .spi_mosi(spi_mosi),
        .spi_miso(spi_miso),
        .spi_csn(spi_csn),
        .busy(busy)
    );

    integer i;

    initial begin
        for (i = 0; i < 256; i = i + 1)
            buf_mem[i] = i;

        reset_n = 0;
        #100 reset_n = 1;

        #200 buf_full = 1;
        #50 buf_full = 0;

        #5000;
        $finish;
    end
endmodule
//expected behavior: cs goes low, 06 wren, 02 pp, address, 256 bytes, rdsr polling loop
