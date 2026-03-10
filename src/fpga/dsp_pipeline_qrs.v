// Basic ECG DSP pipeline.
// This is still a simplified detector, but it now uses the FIR result-valid pulse
// instead of assuming a single-cycle filter latency.

module dsp_pipeline_qrs #(
    parameter integer FS_HZ = 250,
    parameter [15:0]  QRS_THRESH = 16'd1200
) (
    input  wire clk_core,
    input  wire rst_n,
    input  wire signed [15:0] raw_in,
    input  wire valid_in,
    output reg  signed [15:0] filtered_out,
    output reg                qrs_detected_out,
    output reg  [7:0]         current_bpm_out,
    output reg                log_trigger_out,
    output reg  [15:0]        log_word_out,
    output reg                log_word_valid
);

    localparam integer BPM_NUM = 60 * FS_HZ;
    localparam integer BPM_MAX_VALID = 220;
    localparam integer BPM_MIN_VALID = 30;
    localparam integer REFRACTORY_MS = 180;
    localparam [15:0] RR_MIN_SAMPLES = (BPM_NUM + BPM_MAX_VALID - 1) / BPM_MAX_VALID;
    localparam [15:0] RR_MAX_SAMPLES = BPM_NUM / BPM_MIN_VALID;
    localparam [15:0] REFRACTORY_SAMPLES = ((FS_HZ * REFRACTORY_MS) + 999) / 1000;

    wire signed [37:0] fir_acc;
    wire signed [22:0] fir_y;
    wire               fir_out_valid;

    reg  above;
    reg  [15:0] rr_samples;
    reg  [15:0] bpm_div_den;
    reg  [15:0] bpm_div_rem;
    reg  [7:0]  bpm_div_q;
    reg         bpm_div_busy;
    reg  [15:0] refractory_count;

    wire [15:0] rr_interval = rr_samples + 16'd1;
    wire signed [15:0] filtered_sat = sat23_to16(fir_y);
    wire [15:0] filtered_abs = abs16(filtered_sat);
    wire rr_interval_valid = (rr_interval >= RR_MIN_SAMPLES) && (rr_interval <= RR_MAX_SAMPLES);

    function signed [15:0] sat23_to16;
        input signed [22:0] value;
        begin
            if (value > 23'sd32767) begin
                sat23_to16 = 16'sd32767;
            end else if (value < -23'sd32768) begin
                sat23_to16 = -16'sd32768;
            end else begin
                sat23_to16 = value[15:0];
            end
        end
    endfunction

    function [15:0] abs16;
        input signed [15:0] value;
        begin
            if (value[15]) begin
                abs16 = (~value) + 16'd1;
            end else begin
                abs16 = value[15:0];
            end
        end
    endfunction

    fir_filter fir_inst (
        .clk       (clk_core),
        .reset_n   (rst_n),
        .sample_en (valid_in),
        .sample_in (raw_in),
        .acc_out   (fir_acc),
        .y_out     (fir_y),
        .out_valid (fir_out_valid)
    );

    always @(posedge clk_core) begin
        if (!rst_n) begin
            filtered_out     <= 16'sd0;
            qrs_detected_out <= 1'b0;
            current_bpm_out  <= 8'd0;
            log_trigger_out  <= 1'b0;
            log_word_out     <= 16'd0;
            log_word_valid   <= 1'b0;
            above            <= 1'b0;
            rr_samples       <= 16'd0;
            bpm_div_den      <= 16'd0;
            bpm_div_rem      <= 16'd0;
            bpm_div_q        <= 8'd0;
            bpm_div_busy     <= 1'b0;
            refractory_count <= 16'd0;
        end else begin
            qrs_detected_out <= 1'b0;
            log_trigger_out  <= 1'b0;
            log_word_valid   <= 1'b0;

            if (bpm_div_busy) begin
                if ((bpm_div_den != 16'd0) && (bpm_div_rem >= bpm_div_den) && (bpm_div_q != 8'hFF)) begin
                    bpm_div_rem <= bpm_div_rem - bpm_div_den;
                    bpm_div_q   <= bpm_div_q + 8'd1;
                end else begin
                    current_bpm_out <= bpm_div_q;
                    bpm_div_busy    <= 1'b0;
                end
            end

            if (fir_out_valid) begin
                filtered_out   <= filtered_sat;
                log_word_out   <= filtered_sat;
                log_word_valid <= 1'b1;

                if (rr_samples != 16'hFFFF) begin
                    rr_samples <= rr_interval;
                end
                if (refractory_count != 16'd0) begin
                    refractory_count <= refractory_count - 16'd1;
                end

                if (!above && (refractory_count == 16'd0) && (filtered_abs > QRS_THRESH)) begin
                    above            <= 1'b1;
                    qrs_detected_out <= 1'b1;
                    log_trigger_out  <= 1'b1;
                    rr_samples       <= 16'd0;
                    refractory_count <= REFRACTORY_SAMPLES;

                    if (rr_interval_valid) begin
                        bpm_div_den  <= rr_interval;
                        bpm_div_rem  <= BPM_NUM[15:0];
                        bpm_div_q    <= 8'd0;
                        bpm_div_busy <= 1'b1;
                    end else begin
                        bpm_div_busy    <= 1'b0;
                        current_bpm_out <= 8'd0;
                    end
                end else if (above && (filtered_abs < (QRS_THRESH >> 1))) begin
                    above <= 1'b0;
                end
            end
        end
    end

endmodule
