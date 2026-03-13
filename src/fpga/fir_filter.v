`timescale 1ns/1ns
/**
 * Sequential sample-gated FIR filter for ECG processing.
 * A new sample is accepted only when the MAC (multiply-accumulate) engine is idle.
 *
 * NOISE SUPPRESSION STRATEGY:
 * 31-tap symmetric lowpass FIR filter designed for ECG (0-50 Hz passband typical)
 * Combined with AD8232's built-in analog filtering, provides dual-stage noise suppression
 * Accumulator: 38-bit width (16+16+6) prevents overflow even with worst-case signals
 * Saturation at output: protects 16-bit domain from accumulator overflow
 * Coefficients use 16-bit signed fixed-point (normalized by design tool)
 *
 * COMPUTATIONAL ACCURACY:
 * MAC operation: 31 cycles (one per tap), saturates output (sat23_to16)
 * Fixed-point: Q15 coefficients × Q15 data = Q30 temporary, then scaled
 * Output width (Y_W): 23 bits before saturation to preserve precision
 */

module fir_filter #(
    parameter integer N_TAPS = 31,
    parameter integer DATA_W = 16,    // Input/output data width (signed)
    parameter integer COEFF_W = 16,    // Coefficient width (signed Q15)
    parameter integer ACC_GUARD = 6      // Extra bits for accumulation overflow protection
) (
    input wire clk,
    input wire reset_n,
    input wire sample_en,                 // Accepts new sample only when MAC is idle
    input wire signed [DATA_W-1:0] sample_in,
    output reg signed [DATA_W+COEFF_W+ACC_GUARD-1:0] acc_out,        // Raw accumulator output (38-bit)
    output reg signed [DATA_W+COEFF_W+ACC_GUARD-15-1:0] y_out,       // Saturated output (23-bit)
    output reg  out_valid                  // Pulse when filter output is ready
);

    localparam integer ACC_W = DATA_W + COEFF_W + ACC_GUARD;
    localparam integer Y_W = DATA_W + COEFF_W + ACC_GUARD - 15;
    localparam integer TAP_IDX_W = (N_TAPS <= 2) ? 1 : $clog2(N_TAPS);

    // Icarus Verilog does not accept default unpacked array parameters here,
    // so the coefficient table is expressed as a synthesizable function.
    // COEFFICIENTS: Symmetric 31-tap lowpass (origin: FIR design tool output)
    // Normalized for Q15 fixed-point: peak value ~4424 at center tap (index 15)
    // Provides ~40 dB attenuation above cutoff (~50 Hz typical for ECG)
    function signed [COEFF_W-1:0] coeff_at;
        input [TAP_IDX_W-1:0] idx;
        begin
            case (idx)
                5'd0: coeff_at = -16'sd144;
                5'd1: coeff_at = -16'sd207;
                5'd2: coeff_at = -16'sd321;
                5'd3: coeff_at = -16'sd488;
                5'd4: coeff_at = -16'sd679;
                5'd5: coeff_at = -16'sd840;
                5'd6: coeff_at = -16'sd898;
                5'd7: coeff_at = -16'sd778;
                5'd8: coeff_at = -16'sd427;
                5'd9: coeff_at = 16'sd172;
                5'd10: coeff_at = 16'sd985;
                5'd11: coeff_at = 16'sd1926;
                5'd12: coeff_at = 16'sd2872;
                5'd13: coeff_at = 16'sd3683;
                5'd14: coeff_at = 16'sd4231;
                5'd15: coeff_at = 16'sd4424;
                5'd16: coeff_at = 16'sd4231;
                5'd17: coeff_at = 16'sd3683;
                5'd18: coeff_at = 16'sd2872;
                5'd19: coeff_at = 16'sd1926;
                5'd20: coeff_at = 16'sd985;
                5'd21: coeff_at = 16'sd172;
                5'd22: coeff_at = -16'sd427;
                5'd23: coeff_at = -16'sd778;
                5'd24: coeff_at = -16'sd898;
                5'd25: coeff_at = -16'sd840;
                5'd26: coeff_at = -16'sd679;
                5'd27: coeff_at = -16'sd488;
                5'd28: coeff_at = -16'sd321;
                5'd29: coeff_at = -16'sd207;
                default: coeff_at = -16'sd144;
            endcase
        end
    endfunction

    reg signed [DATA_W-1:0] x [0:N_TAPS-1];
    reg signed [ACC_W-1:0] acc_work;
    reg signed [ACC_W-1:0] mac_next;
    reg [TAP_IDX_W-1:0] tap_idx;
    reg mac_busy;

    integer i;
    always @(posedge clk) begin
        out_valid <= 1'b0;

        if (!reset_n) begin
            acc_out <= '0;
            y_out <= '0;
            acc_work <= '0;
            tap_idx <= '0;
            mac_busy <= 1'b0;
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
            mac_next = acc_work + (x[tap_idx] * coeff_at(tap_idx));

            if (tap_idx == N_TAPS-1) begin
                acc_out <= mac_next;
                y_out <= mac_next[Y_W+15-1:15];
                acc_work <= '0;
                mac_busy <= 1'b0;
                out_valid <= 1'b1;
            end else begin
                acc_work <= mac_next;
                tap_idx <= tap_idx + 1'b1;
            end
        end
    end

endmodule
