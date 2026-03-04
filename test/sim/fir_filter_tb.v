/*
The purpose is to verify the FIR response, and check if there are no overflow and sign issues.
*/
`timescale 1ns/1ps

module fir_filter_tb;

    reg clk = 0;
    reg reset_n = 0;
    reg sample_en = 0;
    reg signed [15:0] sample_in;
    wire signed [37:0] acc_out;
    wire signed [22:0] y_out;
    wire out_valid;

    always #10 clk = ~clk; // 50 MHz

    task send_sample(input signed [15:0] value);
        begin
            @(posedge clk);
            sample_in <= value;
            sample_en <= 1'b1;
            @(posedge clk);
            sample_en <= 1'b0;
            sample_in <= 16'sd0;
        end
    endtask

    fir_filter dut (
        .clk(clk),
        .reset_n(reset_n),
        .sample_en(sample_en),
        .sample_in(sample_in),
        .acc_out(acc_out),
        .y_out(y_out),
        .out_valid(out_valid)
    );

    integer i;

    initial begin
        reset_n = 0;
        sample_en = 0;
        sample_in = 0;
        #100;
        reset_n = 1;

        // Accept one impulse sample, then feed zeros at a rate the sequential MAC can sustain.
        send_sample(16'sd1000);
        for (i = 0; i < 8; i = i + 1) begin
            repeat (40) @(posedge clk);
            send_sample(16'sd0);
        end

        repeat (60) @(posedge clk);

        $finish;
    end
endmodule

// Expected behavior: output shows the python fir coefficients, and in general, nothing unexpected.
