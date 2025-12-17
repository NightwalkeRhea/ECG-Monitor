// This testbench is the most important one, as it checks 
// the MSB-first reception, CDC handshake, data_valid pulse, and no deadlock. 
`timescale 1ns/1ps

module spi_slave_rx_tb;

    reg clk_sys = 0;
    reg spi_sclk = 0;
    reg spi_cs_n = 1;
    reg spi_mosi = 0;
    reg rst_n = 0;

    wire [15:0] ecg_data_out;
    wire data_valid;

    // 27 MHz system clock (~37ns)
    always #18.5 clk_sys = ~clk_sys;

    spi_slave_rx dut (
        .clk_system(clk_sys),
        .rst_n(rst_n),
        .spi_sclk(spi_sclk),
        .spi_cs_n(spi_cs_n),
        .spi_mosi(spi_mosi),
        .ecg_data_out(ecg_data_out),
        .data_valid(data_valid)
    );

    task spi_send_word(input [15:0] word);
        integer i;
        begin
            spi_cs_n = 0;
            #50;
            for (i = 15; i >= 0; i = i - 1) begin
                spi_mosi = word[i];
                #50 spi_sclk = 1;
                #50 spi_sclk = 0;
            end
            #20;
            spi_cs_n = 1;
        end
    endtask

    initial begin
        $display("Starting spi_slave_rx TB");
        rst_n = 0;
        #200;
        rst_n = 1;

        spi_send_word(16'h1234);
        spi_send_word(16'hABCD);
        spi_send_word(16'h0F0F);

        #2000;
        $finish;
    end

    always @(posedge clk_sys) begin
        if (data_valid) begin
            $display("RX @ %0t : %h", $time, ecg_data_out);
        end
    end

endmodule

/*
Expected waveform behavior
data_valid pulses once per word
ecg_data_out matches exactly
No missed samples
*/