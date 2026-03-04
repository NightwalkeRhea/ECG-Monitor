# ============================================================================
# FPGA Design Python Scripts Makefile
# ============================================================================
# Targets for FIR filter design and Pan-Tompkins reference implementation

.PHONY: all install coef run-pan gowin lint format clean help

# Python interpreter (uses venv if available)
PYTHON := python
ifneq (,$(wildcard ../../.venv/Scripts/python.exe))
    PYTHON := ../../.venv/Scripts/python
endif

GOWIN_SH ?= C:/Gowin/Gowin_V1.9.12_x64/IDE/bin/gw_sh.exe
GOWIN_TCL := gowin_build.tcl
GOWIN_LOG_DIR := ../../logs/fpga
GOWIN_BUILD_LOG := $(GOWIN_LOG_DIR)/gowin_build.log

# Default target
all: coef run-pan

# Dependencies installation
install:
	@echo "Installing FPGA Python dependencies..."
	$(PYTHON) -m pip install --upgrade pip
	$(PYTHON) -m pip install -r requirements.txt

# FIR Filter Coefficient Generation
# Generates filter coefficients in various formats (float, Q15, etc.)
coef:
	@echo "Generating FIR filter coefficients..."
	$(PYTHON) fir_coef.py

# Pan-Tompkins Algorithm Reference
# Runs reference implementation on synthetic ECG signal
run-pan: coef
	@echo "Running Pan-Tompkins reference implementation..."
	$(PYTHON) pan_tompkins.py

# Gowin Synthesis + Implementation
gowin:
	@echo "Running Gowin synthesis and implementation..."
	powershell -Command "New-Item -ItemType Directory -Force -Path '$(GOWIN_LOG_DIR)' | Out-Null"
	powershell -Command "& '$(GOWIN_SH)' '$(GOWIN_TCL)' *> '$(GOWIN_BUILD_LOG)'; if ($$LASTEXITCODE -ne 0) { exit $$LASTEXITCODE }"
	@echo "Gowin console log written to $(GOWIN_BUILD_LOG)"
	@echo "Vendor reports copied to $(GOWIN_LOG_DIR) by $(GOWIN_TCL)"

# Code Quality
lint:
	@echo "Linting Python code with ruff..."
	$(PYTHON) -m ruff check .

format:
	@echo "Formatting Python code with black..."
	$(PYTHON) -m black .

# Cleanup
clean:
	@echo "Cleaning FPGA build artifacts..."
	powershell -Command "Remove-Item -Path '__pycache__' -Recurse -ErrorAction SilentlyContinue; $$null"
	powershell -Command "Remove-Item -Path '*.pyc' -ErrorAction SilentlyContinue; $$null"
	powershell -Command "Remove-Item -Path '../../build/gowin' -Recurse -ErrorAction SilentlyContinue; $$null"

help:
	@echo "FPGA Design Makefile"
	@echo ""
	@echo "Targets:"
	@echo "  make install      - Install Python dependencies from requirements.txt"
	@echo "  make coef         - Generate FIR filter coefficients"
	@echo "  make run-pan      - Run Pan-Tompkins reference (implies coef)"
	@echo "  make gowin        - Run Gowin build and copy reports into logs/fpga"
	@echo "  make all          - Run coef and run-pan"
	@echo "  make lint         - Lint Python code"
	@echo "  make format       - Format Python code with black"
	@echo "  make clean        - Remove build artifacts"
	@echo "  make help         - Show this help message"
	@echo ""
	@echo "Generated artifacts are typically stored in logs/"
