import numpy as np
from scipy.signal import firwin
from matplotlib import pyplot

# Sampling frequency
Fs = 250.0

# FIR bandpass parameters
lowcut = 5.0
highcut = 15.0
numtaps = 31 # small enough for FPGA, good enough for ECG

# generate FIR bandpass coefficients
coefs = firwin(
    numtaps=numtaps,
    cutoff=[lowcut,highcut],
    fs=Fs,
    pass_zero=False
)

# Convert to fixed-point Q1.15
q15_coeffs = np.round(coefs * (2**15)).astype(int)

print("Floating-point coefficients:")
print(coefs)
print("\nQ15 fixed-point coefficients: ")
print(q15_coeffs)