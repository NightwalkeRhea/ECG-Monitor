// page_buffer_256B.v
// Accumulates 16-bit ECG samples (as two bytes) into a 256-byte dual-ported RAM structure.
// Signals 'buf_full' when ready to be consumed by the w25q64_page_logger.

module page_buffer_256B (
    input  wire clk,
    input  wire reset_n,
    // write side (from DSP pipeline)
    input  wire push_word,        // Goes high when a 16-bit word is ready
    input  wire [15:0] word_in,
    // read side (for flash logger)
    output wire [7:0]  buf_dout,    // Byte output to the logger
    input  wire [7:0]  buf_addr,    // Address requested by logger (0 to 255)
    output reg   buf_full,    // Asserted when the buffer has 256 bytes (128 samples)
    input  wire  logger_busy  // Logger status: high when logger is performing WREN/PageProgram
);

    // Page memory: 256 depth, 8-bit width
    reg [7:0] mem [0:255];
    reg [7:0] wr_ptr;
    reg  armed; // tracks whether we have started a logger transaction

    assign buf_dout = mem[buf_addr]; // Asynchronous read for logger (simple dual port read)

    always @(posedge clk) begin
        if (!reset_n) begin
            wr_ptr   <= 8'd0;
            buf_full <= 1'b0;
            armed    <= 1'b0;
        end else begin
            // 1. Consumption Logic (Wait for logger to finish)
            if (buf_full) begin
                if (logger_busy) armed <= 1'b1; // Logger started consuming the page
                if (armed &&!logger_busy) begin
                    // Logger finished the page write, reset buffer state
                    buf_full <= 1'b0;
                    wr_ptr   <= 8'd0;
                    armed    <= 1'b0;
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
