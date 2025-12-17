/*
The purpose is to verify the FIR response, and check if there are no overflow and sign issues.
*/
`timescale 1ns/1ps

module fir_filter_tb;

    reg clk = 0;
    reg reset_n = 0;
    reg signed [15:0] sample_in;
    wire signed [21:0] acc_out;
    wire signed [21:0] y_out;

    always #10 clk = ~clk; // 50 MHz

    fir_filter dut (
        .clk(clk),
        .reset_n(reset_n),
        .sample_in(sample_in),
        .acc_out(acc_out),
        .y_out(y_out)
    );

    integer i;

    initial begin
        reset_n = 0;
        sample_in = 0;
        #100;
        reset_n = 1;

        // impulse response
        sample_in = 16'sd1000;
        #20 sample_in = 0;

        for (i = 0; i < 100; i = i + 1)
            #20;

        $finish;
    end
endmodule

// Expected behavior: output shows the python fir coefficients, and in general, nothing unexpected.
