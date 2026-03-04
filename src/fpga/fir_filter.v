`timescale 1ns/1ns
// Sequential sample-gated FIR filter for ECG processing.
// A new sample is accepted only when the MAC engine is idle.

module fir_filter #(
    parameter integer N_TAPS      = 31,
    parameter integer DATA_W      = 16,
    parameter integer COEFF_W     = 16,
    parameter integer ACC_GUARD   = 6,
    parameter signed [COEFF_W-1:0] COEFFS [0:N_TAPS-1] = '{
        -16'sd144, -16'sd207, -16'sd321, -16'sd488, -16'sd679, -16'sd840, -16'sd898, -16'sd778, -16'sd427,
        16'sd172, 16'sd985, 16'sd1926, 16'sd2872, 16'sd3683, 16'sd4231, 16'sd4424, 16'sd4231, 16'sd3683,
        16'sd2872, 16'sd1926, 16'sd985, 16'sd172, -16'sd427, -16'sd778, -16'sd898, -16'sd840, -16'sd679,
        -16'sd488, -16'sd321, -16'sd207, -16'sd144
    }
) (
    input  wire clk,
    input  wire reset_n,
    input  wire sample_en,
    input  wire signed [DATA_W-1:0] sample_in,
    output reg  signed [DATA_W+COEFF_W+ACC_GUARD-1:0] acc_out,
    output reg  signed [DATA_W+COEFF_W+ACC_GUARD-15-1:0] y_out,
    output reg  out_valid
);

    localparam integer ACC_W     = DATA_W + COEFF_W + ACC_GUARD;
    localparam integer Y_W       = DATA_W + COEFF_W + ACC_GUARD - 15;
    localparam integer TAP_IDX_W = (N_TAPS <= 2) ? 1 : $clog2(N_TAPS);

    reg signed [DATA_W-1:0] x [0:N_TAPS-1];
    reg signed [ACC_W-1:0]  acc_work;
    reg signed [ACC_W-1:0]  mac_next;
    reg [TAP_IDX_W-1:0]     tap_idx;
    reg                     mac_busy;

    integer i;
    always @(posedge clk) begin
        out_valid <= 1'b0;

        if (!reset_n) begin
            acc_out   <= '0;
            y_out     <= '0;
            acc_work  <= '0;
            tap_idx   <= '0;
            mac_busy  <= 1'b0;
            out_valid <= 1'b0;
            for (i = 0; i < N_TAPS; i = i + 1) begin
                x[i] <= '0;
            end
        end else if (!mac_busy) begin
            if (sample_en) begin
                x[0] <= sample_in;
                for (i = 1; i < N_TAPS; i = i + 1) begin
                    x[i] <= x[i-1];
                end

                acc_work <= '0;
                tap_idx  <= '0;
                mac_busy <= 1'b1;
            end
        end else begin
            mac_next = acc_work + (x[tap_idx] * COEFFS[tap_idx]);

            if (tap_idx == N_TAPS-1) begin
                acc_out   <= mac_next;
                y_out     <= mac_next[Y_W+15-1:15];
                acc_work  <= '0;
                mac_busy  <= 1'b0;
                out_valid <= 1'b1;
            end else begin
                acc_work <= mac_next;
                tap_idx  <= tap_idx + 1'b1;
            end
        end
    end

endmodule
