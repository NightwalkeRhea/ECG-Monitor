// This is to verify the MSB-first shift, the spi timing and the "done" timing.

`timescale 1ns/1ps

module spi_byte_shifter_tb;

    reg clk = 0;
    reg reset_n = 0;
    reg start = 0;
    reg [7:0] tx_byte = 8'hA5;
    wire [7:0] rx_byte;
    wire busy, done;
    wire sck, mosi;
    reg miso = 1;

    always #5 clk = ~clk; // 100 MHz

    spi_byte_shifter dut (
        .clk(clk),
        .reset_n(reset_n),
        .start(start),
        .tx_byte(tx_byte),
        .mosi_oe(1'b1),
        .rx_byte(rx_byte),
        .busy(busy),
        .sck(sck),
        .mosi(mosi),
        .miso(miso),
        .done(done)
    );

    initial begin
        $dumpfile("spi_byte_shifter_tb.vcd");
        $dumpvars(0, spi_byte_shifter_tb);

        reset_n = 0;
        #50 reset_n = 1;

        #50 start = 1;
        #10 start = 0;

        wait(done);
        $display("RX byte = %h", rx_byte);

        #200;
        $finish;
    end
endmodule
