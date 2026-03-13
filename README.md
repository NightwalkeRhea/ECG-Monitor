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

Fractional Divider
The frequency of MCLK and PCLK are programmable through a fractional divider. PCLK
has a range of frequency from 125kHz to 64MHz and MCLK has a range from 125kHz
to 32MHz.
The following formula calculate the MCLK clock frequency.
(12.1) MCLK =  (dco_dclk / (2) * (IDIV + FDIV/256))
for IDIV > 0
The following formula calculate the PCLK clock frequency when CLKCR.PCLKSEL is set
to 1 and it is double the frequency of MCLK.
(12.2)  PCLK =  (dco_dclk /(IDIV + FDIV/256)) for IDIV >0 
IDIV represents an unsigned 8-bit integer from the bit field CLKCR.IDIV and FDIV/256
defines the fractional divider selection in CLKCR.FDIV. Changing of the MCLK and
PCLK can be done within 2 clock cycles. While changing the frequency of MCLK clock
and PCLK clock, it is recommended to disable all interrupts to prevent any access to
flash that may results in an unsuccessful flash operation.
Note: Changing the MCLK and PCLK frequency may result in a load change that causes
clock blanking to happen. Refer to Clock Blanking and VDDC Response During
Load Change for more details.
12.5.3 Clock Gating Control
The clock to peripherals can be individually gated and parts of the system can be
stopped by registers CGATSET0. After a master reset, only core, memories, SCU and
PORT peripheral are not clock gated. The rest of the peripherals are default clock gated.
User can select the clock of individual modules to be enabled by SSW after reset by defining it in the flash memory location 1000 1014H. Refer to Boot and Startup chapter for more details.
Module clock gating in Sleep and Deep-Sleep modes
It is recommended to gate the clock using registers CGATSET0 for module that is not
needed during sleep mode or deep-sleep mode. These modules must be disabled
before entering sleep or deep-sleep mode. In addition, the PCLK and MCLK will be
switched to a slow standby clock and DCO1 will be put into power-down mode in deepsleep mode.
![alt text](image.png)

PASSWD:
![alt text](image-1.png)
![alt text](image-2.png)

14 Universal Serial Interface Channel (USIC)
The Universal Serial Interface Channel module (USIC) is a flexible interface module
covering several serial communication protocols. A USIC module contains two
independent communication channels named USICx_CH0 and USICx_CH1, with x
being the number of the USIC module (e.g. channel 0 of USIC module 0 is referenced
as USIC0_CH0). The user can program during run-time which protocol will be handled
by each communication channel and which pins are used.

GPIO:
A write operation to Pn_OUT updates all port pins of
that port (e.g. P0) that are configured as general purpose output. Updating just one or a
selected few general purpose output pins via Pn_OUT requires a masked read-modifywrite operation to avoid disturbing pins that shall not be changed. Direct writes to
Pn_OUT will also affect Pn_OUT bits configured for use with the Pin Power-save
function, Chapter 18.4.
Because of that, it is preferred to modify Pn_OUT bits by the Output Modification
Register Pn_OMR (Chapter 18.8.5). The bits in Pn_OMR allow to individually set, clear
or toggle the bits in the Pn_OUT register and only update the “addressed” Pn_OUT bits.