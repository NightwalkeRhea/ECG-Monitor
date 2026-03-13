module qrs_actuator (
    input wire clk_core,
    input wire rst_n,
    input wire [7:0] current_bpm,
    output reg alarm_out
);
    /*
     * alarm_out = 1 if (current_bpm > 100 || current_bpm < 50) && current_bpm != 0
     * No hysteresis: could cause chattering if BPM oscillates near threshold
     * I need to consider adding hysteresis in future: alarm_on @ 100, clear @ 95
     */
    // Simple comparison logic goes here.
    localparam BPM_LOW_THRESHOLD = 8'd50;
    localparam BPM_HIGH_THRESHOLD = 8'd100;
    
    always @(posedge clk_core) begin
        if (!rst_n) begin
            alarm_out <= 1'b0;
        end else if (current_bpm == 8'd0) begin
            alarm_out <= 1'b0; // no alarm until a valid BPM estimate is available
        end else if (current_bpm > BPM_HIGH_THRESHOLD || current_bpm < BPM_LOW_THRESHOLD) begin
            alarm_out <= 1'b1; // Turn on alarm
        end else begin
            alarm_out <= 1'b0; // Turn off alarm
        end
    end
endmodule
