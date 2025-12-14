// =================================================================
// Placeholder Module Definitions (Must be written in full Verilog)
// =================================================================

// Define the interfaces for the black boxes to prevent synthesis errors
// These are the files you must write next.

module dsp_pipeline_qrs (
    input wire clk_core,
    input wire rst_n,
    input wire [15:0] raw_ecg_in,
    input wire valid_in,
    output wire [15:0] filtered_out,
    output wire qrs_detected_out,
    output wire [7:0] current_bpm_out,
    output wire [23:0] log_data_out,
    output wire log_trigger_out
);
    // FIR filter implementation, derivative, squaring, MWI, and adaptive threshold goes here.
    // Critical: Use fixed-point arithmetic (Q1.15).
endmodule