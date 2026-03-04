# ============================================================================
# Test Suite Makefile
# ============================================================================
# Manages simulation and testbench execution for FPGA and MCU

.PHONY: all sim-all sim-fir sim-spi-shifter sim-spi-rx sim-w25q64 clean help

# Verilog simulator (configure based on your tool)
SIMULATOR ?= iverilog
SIMULATOR_FLAGS ?= -g2009

# Simulation testbenches
TESTBENCHES := \
	sim/fir_filter_tb.v \
	sim/spi_byte_shifter_tb.v \
	sim/spi_slave_rx_tb.v \
	sim/w25q64_page_logger_tb.v

# Default target
all: help

# Run all simulations
sim-all: sim-fir sim-spi-shifter sim-spi-rx sim-w25q64
	@echo "All simulations complete."

# FIR Filter testbench
sim-fir:
	@echo "Running FIR Filter simulation..."
	@command -v $(SIMULATOR) >/dev/null 2>&1 || (echo "$(SIMULATOR) not found. Install Icarus Verilog or configure SIMULATOR." && exit 1)
	$(SIMULATOR) $(SIMULATOR_FLAGS) -o sim_fir.vvp sim/fir_filter_tb.v ../src/fpga/fir_filter.v
	vvp sim_fir.vvp

# SPI Byte Shifter testbench
sim-spi-shifter:
	@echo "Running SPI Byte Shifter simulation..."
	@command -v $(SIMULATOR) >/dev/null 2>&1 || (echo "$(SIMULATOR) not found. Install Icarus Verilog or configure SIMULATOR." && exit 1)
	$(SIMULATOR) $(SIMULATOR_FLAGS) -o sim_spi_shifter.vvp sim/spi_byte_shifter_tb.v ../src/fpga/spi_shifter.v
	vvp sim_spi_shifter.vvp

# SPI Slave RX testbench
sim-spi-rx:
	@echo "Running SPI Slave RX simulation..."
	@command -v $(SIMULATOR) >/dev/null 2>&1 || (echo "$(SIMULATOR) not found. Install Icarus Verilog or configure SIMULATOR." && exit 1)
	$(SIMULATOR) $(SIMULATOR_FLAGS) -o sim_spi_rx.vvp sim/spi_slave_rx_tb.v ../src/fpga/spi_slave_rx.v
	vvp sim_spi_rx.vvp

# W25Q64 Page Logger testbench
sim-w25q64:
	@echo "Running W25Q64 Page Logger simulation..."
	@command -v $(SIMULATOR) >/dev/null 2>&1 || (echo "$(SIMULATOR) not found. Install Icarus Verilog or configure SIMULATOR." && exit 1)
	$(SIMULATOR) $(SIMULATOR_FLAGS) -o sim_w25q64.vvp sim/w25q64_page_logger_tb.v ../src/fpga/flash_page.v
	vvp sim_w25q64.vvp

# Clean simulation artifacts
clean:
	@echo "Cleaning simulation artifacts..."
	powershell -Command "Remove-Item -Path 'sim_*.vvp' -ErrorAction SilentlyContinue; $$null"
	powershell -Command "Remove-Item -Path '*.vcd' -ErrorAction SilentlyContinue; $$null"

help:
	@echo "Test Suite Makefile"
	@echo ""
	@echo "Targets:"
	@echo "  make sim-all          - Run all simulations"
	@echo "  make sim-fir          - Simulate FIR filter"
	@echo "  make sim-spi-shifter  - Simulate SPI byte shifter"
	@echo "  make sim-spi-rx       - Simulate SPI slave RX"
	@echo "  make sim-w25q64       - Simulate W25Q64 page logger"
	@echo "  make clean            - Remove simulation artifacts"
	@echo "  make help             - Show this help message"
	@echo ""
	@echo "Variables:"
	@echo "  SIMULATOR             - Verilog simulator (default: iverilog)"
	@echo "  SIMULATOR_FLAGS       - Simulator flags (default: -g2009)"
	@echo ""
	@echo "Setup:"
	@echo "  Install Icarus Verilog: https://steveicarus.github.io/iverilog/"
	@echo ""
	@echo "Example:"
	@echo "  make SIMULATOR=xvlog sim-all"
