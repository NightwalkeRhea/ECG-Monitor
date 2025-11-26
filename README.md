# ECG-Monitor
FPGA-assisted ECG monitor with real-time QRS detection (Pan–Tompkins reference flow + FIR design).

## Project Layout
- src/fpga/ : Python FIR design + Pan–Tompkins reference (`fir_coef.py`, `pan_tompkins.py`, `requirements.txt`, Makefile)
- src/mcu/  : MCU-side SPI/I2C drivers and Makefile
- docs/     : (add design docs here)
- logs/     : capture or simulation logs
- test/     : tests when you add them
- Makefile  : top-level convenience targets
- requirements.txt : Python deps for FPGA scripts

## Prerequisites
- Python 3.10+ (for numpy/scipy)
- (Optional) clang/clang-tidy for C linting
- Make

## Quick Start
```bash
# Create/upgrade venv + install deps
make install

# Run FIR design (prints floats + Q15)
make -C src/fpga coef

# Run Pan–Tompkins reference on synthetic ECG
make -C src/fpga run-pan

# Build MCU demo binary (adjust CC/CFLAGS as needed)
make -C src/mcu

# Lint Python + C (if tools installed)
make lint
```
## Make Targets
Root Makefile:
install: create .venv and pip install -r requirements.txt
lint: flake8/ruff for fpga, clang-format/tidy for mcu
test: hook up pytest or C tests when added
clean: remove caches/build artifacts
src/fpga/Makefile:
install, run-pan, coef, lint, format
src/mcu/Makefile:
build (default), lint, clean
Usage Notes
Activate venv manually if you don’t use make install: .\.venv\Scripts\activate
If you add real ECG data, place it under test/ or logs/ and wire a target like make -C src/fpga demo DATA=....
Keep generated coefficients (fir_coef.py output) in version control if the FPGA build depends on them.
## License
MIT (see LICENSE)