/**
 * 256-byte page buffer for flash logging (128 × 16-bit ECG samples).
 *
 * BUFFERING STRATEGY:
 * Stores exactly 256 bytes (128 samples) before signaling buf_full
 * Matches W25Q64FV page size (256-byte write bursts)
 * Prevents partial-page writes which would waste flash storage
 *
 * STATE MACHINE:
 * 1. FILLING: Accumulates words until 256 bytes (wr_ptr reaches 254+2)
 * 2. FULL: Waits for logger to start (logger_busy rises)
 * 3. ARMED: Tracks logger operation
 * 4. CONSUME: When logger_busy falls, reset pointers and resume filling
 *
 * OUTPUT FORMAT: MSB-first byte ordering for each 16-bit sample
 * - mem[wr_ptr]  = sample[15:8] (MSB first)
 * - mem[wr_ptr+1] = sample[7:0]  (LSB second)
 */

module page_buffer_256B (
    input wire clk,
    input wire reset_n,

    // Write interface (from DSP pipeline)
    input wire push_word,    // Push new 16-bit sample strobe
    input wire [15:0] word_in,  // 16-bit filtered ECG sample

    // Read interface (asynchronous for flash logger)
    output wire [7:0]  buf_dout,  // Byte output (combinatorial from mem array)
    input wire [7:0]  buf_addr,    // Byte address from logger (0-255)

    // Status signals
    output reg   buf_full,     // High when 256 bytes are ready
    input  wire  logger_busy   // High when logger is performing transaction
);

    // Page memory: 256 depth, 8-bit width
    reg [7:0] mem [0:255];
    reg [7:0] wr_ptr;
    reg  armed; // tracks whether we have started a logger transaction

    assign buf_dout = mem[buf_addr]; // Asynchronous read for logger (simple dual port read)

    always @(posedge clk) begin
        if (!reset_n) begin
            wr_ptr  <= 8'd0;
            buf_full <= 1'b0;
            armed   <= 1'b0;
        end else begin
            // 1. Consumption Logic (Wait for logger to finish)
            if (buf_full) begin
                if (logger_busy) armed <= 1'b1; // Logger started consuming the page
                if (armed &&!logger_busy) begin
                    // Logger finished the page write, reset buffer state
                    buf_full <= 1'b0;
                    wr_ptr <= 8'd0;
                    armed <= 1'b0;
                end
            end 
            // 2. Filling Logic
            else begin
                if (push_word) begin
                    // Store MSB first
                    mem[wr_ptr] <= word_in[15:8];
                    mem[wr_ptr + 8'd1] <= word_in[7:0];
                    wr_ptr <= wr_ptr + 8'd2;
                    // Check if we hit 256 bytes (0xFF + 2 = 0x0100, checking 0xFE before push)
                    // 254 = 256 - 2. Check if the next push would fill the buffer.
                    if (wr_ptr == 8'd254) begin
                        buf_full <= 1'b1;
                    end
                end
            end
        end
    end
endmodule
