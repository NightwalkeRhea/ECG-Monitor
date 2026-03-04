create_clock -name CLK_27MHZ -period 37.037 [get_ports {CLK_27MHZ}]
create_clock -name SPI_SCLK_IN -period 20.000 [get_ports {SPI_SCLK_IN}]

set_false_path -from [get_clocks {SPI_SCLK_IN}] -to [get_clocks {CLK_27MHZ}]
set_false_path -from [get_clocks {CLK_27MHZ}] -to [get_clocks {SPI_SCLK_IN}]

# Optional: add a real SPI clock constraint once the MCU SPI frequency is fixed.
# Replace the 20.000 ns SPI clock above with the real MCU SPI period once fixed.
