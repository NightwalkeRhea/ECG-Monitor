.PHONY: default all install venv clean help
.PHONY: fpga fpga-coef fpga-pan fpga-gowin fpga-lint fpga-clean
.PHONY: mcu mcu-build mcu-clean mcu-lint
.PHONY: lint test logs-clean

# Python environment configuration for Windows
PYTHON := python
VENV_DIR := .venv
VENV_PYTHON := $(VENV_DIR)\Scripts\python
VENV_PIP := $(VENV_DIR)\Scripts\pip
VENV_ACTIVATE := $(VENV_DIR)\Scripts\activate

# Default target
default: install

# Virtual environment setup
venv:
	$(PYTHON) -m venv $(VENV_DIR)
	$(VENV_PIP) install --upgrade pip setuptools wheel

# Install all dependencies
install: venv
	$(VENV_PIP) install -r src/fpga/requirements.txt
	@echo Installation complete. Run 'make help' for available targets.

# ============================================================================
# FPGA Targets
# ============================================================================
fpga: fpga-coef fpga-pan

fpga-coef:
	@echo "Generating FIR filter coefficients..."
	$(VENV_PYTHON) src/fpga/fir_coef.py

fpga-pan:
	@echo "Running Pan-Tompkins reference implementation..."
	$(VENV_PYTHON) src/fpga/pan_tompkins.py

fpga-gowin:
	@echo "Running Gowin FPGA build..."
	$(MAKE) -C src/fpga gowin

fpga-lint:
	@echo "Linting FPGA Python code..."
	$(VENV_PYTHON) -m ruff check src/fpga

fpga-clean:
	@echo "Cleaning FPGA build artifacts..."
	$(MAKE) -C src/fpga clean

# ============================================================================
# MCU Targets
# ============================================================================
mcu: mcu-build

mcu-build:
	@echo "Building MCU firmware..."
	$(MAKE) -C src/mcu build

mcu-lint:
	@echo "Linting MCU C code..."
	$(MAKE) -C src/mcu lint

mcu-clean:
	@echo "Cleaning MCU build artifacts..."
	$(MAKE) -C src/mcu clean

# ============================================================================
# Project-wide Targets
# ============================================================================
all: fpga mcu

lint: fpga-lint mcu-lint
	@echo "All linting checks complete."

clean: fpga-clean mcu-clean logs-clean
	@echo "Cleaning Python cache files..."
	powershell -Command "Remove-Item -Path '__pycache__' -Recurse -ErrorAction SilentlyContinue; $$null"
	powershell -Command "Remove-Item -Path '.pytest_cache' -Recurse -ErrorAction SilentlyContinue; $$null"

logs-clean:
	@echo "Cleaning log files..."
	powershell -Command "Remove-Item -Path 'logs/*' -Recurse -ErrorAction SilentlyContinue; $$null"

help:
	@echo "ECG-Monitor Build System"
	@echo ""
	@echo "Virtual Environment Targets:"
	@echo "  make install       - Setup venv and install dependencies"
	@echo "  make venv          - Create Python virtual environment"
	@echo ""
	@echo "FPGA Targets:"
	@echo "  make fpga-coef     - Generate FIR filter coefficients"
	@echo "  make fpga-pan      - Run Pan-Tompkins reference implementation"
	@echo "  make fpga-gowin    - Run Gowin build and collect FPGA reports in logs/fpga"
	@echo "  make fpga          - Run all FPGA generation (coef + pan)"
	@echo "  make fpga-lint     - Lint FPGA Python code"
	@echo "  make fpga-clean    - Clean FPGA build artifacts"
	@echo ""
	@echo "MCU Targets:"
	@echo "  make mcu-build     - Build MCU firmware"
	@echo "  make mcu-lint      - Lint MCU C code"
	@echo "  make mcu-clean     - Clean MCU build artifacts"
	@echo "  make mcu           - Build MCU firmware"
	@echo ""
	@echo "Project-wide Targets:"
	@echo "  make all           - Build FPGA and MCU"
	@echo "  make lint          - Lint all code (FPGA + MCU)"
	@echo "  make clean         - Clean all build artifacts"
	@echo "  make help          - Show this help message"
