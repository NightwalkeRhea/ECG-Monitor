`timescale 1ns/1ns
// this module assumes 16-bit signed samples, 16-bit signed Q1.15 coefficients,
// and produces a widened accumulator that is scaled back to Q1.15 with a right shift of 15.

module fir_filter #(
    parameter integer N_TAPS      = 31,
    parameter integer DATA_W      = 16,  // signed input width
    parameter integer COEFF_W     = 16,  // signed Q1.15 coeffs
    parameter integer ACC_GUARD   = 6,   // ceil(log2(31)) = 5, add 1 for headroom
    //  Q1.15 tap values from fir_coef.py
    parameter signed [COEFF_W-1:0] COEFFS [0:N_TAPS-1] = '{
        16'sd-144, 16'sd-207, 16'sd-321, 16'sd-488, 16'sd-679, 16'sd-840, 16'sd-898, 16'sd-778, 16'sd-427,
        16'sd172, 16'sd985, 16'sd1926, 16'sd2872, 16'sd3683, 16'sd4231, 16'sd4424, 16'sd4231, 16'sd3683,
        16'sd2872, 16'sd1926, 16'sd985, 16'sd172, 16'sd-427, 16'sd-778, 16'sd-898, 16'sd-840, 16'sd-679,
        16'sd-488, 16'sd-321, 16'sd-207, 16'sd-144
    }                
) (
    input  wire                     clk,
    input  wire                     reset_n,   // active-low async
    input  wire signed [DATA_W-1:0] sample_in,
    output reg  signed [DATA_W+COEFF_W+ACC_GUARD-1:0] acc_out, // widened accumulator
    output reg  signed [DATA_W+ACC_GUARD-1:0]         y_out    // scaled back to Q1.15
);

    // Shift register for past samples
    reg signed [DATA_W-1:0] x [0:N_TAPS-1];

    integer i;
    always @(posedge clk or negedge reset_n) begin
        if (!reset_n) begin
            for (i = 0; i < N_TAPS; i = i + 1)
                x[i] <= '0;
        end else begin
            // shift down, x[0] is newest
            x[0] <= sample_in;
            for (i = 1; i < N_TAPS; i = i + 1)
                x[i] <= x[i-1];
        end
    end

    // Multiply-accumulate
    reg signed [DATA_W+COEFF_W-1:0] prod [0:N_TAPS-1];
    reg signed [DATA_W+COEFF_W+ACC_GUARD-1:0] sum;

    always @* begin
        for (i = 0; i < N_TAPS; i = i + 1)
            prod[i] = x[i] * COEFFS[i];

        sum = '0;
        for (i = 0; i < N_TAPS; i = i + 1)
            sum = sum + prod[i];
    end

    // Register outputs; right-shift by 15 to return to Q1.15
    always @(posedge clk or negedge reset_n) begin
        if (!reset_n) begin
            acc_out <= '0;
            y_out   <= '0;
        end else begin
            acc_out <= sum;
            y_out   <= sum >>> 15; // arithmetic shift, preserves sign
        end
    end

endmodule
